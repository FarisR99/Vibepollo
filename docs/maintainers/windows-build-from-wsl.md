# Building on Windows from WSL (agent recipe)

This is the exact, verified procedure an agent used to produce a clean Windows build of
`sunshine.exe` and the NVIDIA Surround tool while running inside WSL2. It drives the
Windows MSYS2 UCRT64 toolchain through Windows interop; it never compiles with the Linux
toolchain. The general prerequisites and CMake options come from
[building-windows.md](../building-windows.md) and the "Windows agent build workflow" section
of `AGENTS.md`; this page only records the WSL-specific mechanics and pitfalls.

Verified with MSYS2 GCC 16.2.0, CMake 4.4.4, and Ninja from `/ucrt64/bin`, MSYS2 installed
at `C:\msys64`, and a checkout at `C:\dev\Vibepollo`. Adjust those two paths for the host.

## 1. Use a Windows-drive checkout

Build from a checkout on a Windows drive, seen from WSL under `/mnt/c/...`
(for example `/mnt/c/dev/Vibepollo` = `C:\dev\Vibepollo`). Do not build a checkout that lives
in the WSL filesystem (`/home/...`, `\\wsl.localhost\...`); Windows `cmd.exe`, CMake, and
Ninja do not handle those paths reliably.

Git through `/mnt/c` is very slow. `git status` on this tree can exceed two minutes from WSL,
and it may report many files as modified because of line-ending/mode differences seen through
the mount. Prefer cheap commands (`git log -1`, `git rev-parse`, `git fetch`,
`git rev-list --left-right --count HEAD...origin/master`) to confirm the commit being built,
and do not "fix" those reported modifications as part of a build.

Confirm the submodules match their gitlinks before building, for example
`git ls-tree HEAD third-party/libdisplaydevice` versus
`git -C third-party/libdisplaydevice rev-parse HEAD`. Initialize missing ones with
`git submodule update --init --recursive` (never `--remote`).

## 2. Call Windows programs by absolute path

`cmd.exe` may not be on the WSL `PATH` even when interop is enabled
(`/proc/sys/fs/binfmt_misc/WSLInterop` reports `enabled`). Call it as
`/mnt/c/Windows/System32/cmd.exe`.

Find a Windows-visible scratch directory for the helper scripts. The WSL scratchpad and
`/tmp` are not reachable from Windows tools:

```bash
W=$(/mnt/c/Windows/System32/cmd.exe /c 'echo %LOCALAPPDATA%\Temp' 2>/dev/null | tr -d '\r')
D=$(wslpath "$W")/vibepollo-agent   # e.g. /mnt/c/Users/<user>/AppData/Local/Temp/vibepollo-agent
mkdir -p "$D"
```

The rest of this page writes `<user>` for the Windows account name in that path.

## 3. Do not pass the build command inline to cmd.exe

Passing the MSYS2 launcher command inline from WSL fails. WSL escapes the embedded double
quotes when it builds the `cmd.exe` command line, so both of these attempts fail with
`... was unexpected at this time.`:

```bash
# Fails: cmd.exe misparses the quoted -c argument.
/mnt/c/Windows/System32/cmd.exe /c 'cd /d C:\dev\Vibepollo && C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "echo $MSYSTEM"'
```

Instead, write a small `.cmd` wrapper with CRLF line endings that changes into the checkout and
runs a named bash script through the launcher. The first argument picks the script:

```bash
printf '@echo off\r\ncd /d C:\\dev\\Vibepollo\r\nC:\\msys64\\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "bash /c/Users/<user>/AppData/Local/Temp/vibepollo-agent/%%1.sh"\r\n' > "$D/run.cmd"
```

Run it with the working directory on the Windows drive, so `cmd.exe` does not start from a
UNC path, and strip carriage returns from the output:

```bash
cd /mnt/c/dev && /mnt/c/Windows/System32/cmd.exe /c 'C:\Users\<user>\AppData\Local\Temp\vibepollo-agent\run.cmd' check 2>&1 | tr -d '\r'
```

The scripts run in UCRT64 bash with the checkout as the current directory. Write them from
WSL with LF line endings.

## 4. Check the toolchain

`$D/check.sh`:

```bash
set -u
echo "MSYSTEM=$MSYSTEM"; pwd
for t in gcc cmake ninja node npm.cmd dotnet.exe powershell.exe; do printf '%s: ' "$t"; command -v "$t" || echo MISSING; done
gcc --version | head -1; cmake --version | head -1
```

Expect `MSYSTEM=UCRT64` and `gcc`, `cmake`, `ninja` under `/ucrt64/bin`. `dotnet.exe` was
**not** on the MSYS2 `PATH` on the verified host even though it was installed at
`C:\Program Files\dotnet\dotnet.exe`. The configure step below passes it by absolute path,
together with the Node.js `npm.cmd`.

Compare the installed MSYS2 packages with the list in
[building-windows.md](../building-windows.md#1-install-the-tools). From WSL, the package
database is readable directly:

```bash
ls /mnt/c/msys64/var/lib/pacman/local | sed -E 's/-[^-]+-[^-]+$//' | sort -u
```

A name-only comparison flags `mingw-w64-ucrt-x86_64-onevpl` as missing, but the installed
`mingw-w64-ucrt-x86_64-libvpl` provides it, so use `pacman -Q <name>` or let pacman resolve it.
On the verified host, `mingw-w64-ucrt-x86_64-nlohmann-json` was genuinely missing. Without
it, configuration still succeeds (nlohmann_json falls back to FetchContent), but the build
fails at `tools/display_settings_helper_main.cpp` with
`fatal error: nlohmann/json.hpp: No such file or directory`. The `sunshine_display_helper`
target does not link `nlohmann_json::nlohmann_json` and relies on the system headers that CI
installs. Install missing guide prerequisites only with the user's approval, as `$D/deps.sh`:

```bash
set -euo pipefail
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-onevpl
pacman -Q mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-onevpl
```

After installing a dependency, delete the build directory or reconfigure so `find_package`
results are refreshed.

## 5. Configure and build

`$D/build.sh` uses the guide's developer profile for the main build, then builds the
independent NVIDIA Surround tool, and prints a sentinel only if every step succeeds:

```bash
set -euo pipefail
echo "== configure main"
cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_DOCS=OFF -DSUNSHINE_ENABLE_WEBRTC=OFF \
  -DBUILD_SUNSHINE_VIRTUAL_DISPLAY_DRIVER=OFF -DBUILD_VIRTUALDISPLAY_PROBE=OFF \
  -DBUILD_VIRTUALDISPLAY_TOOLS=OFF -DBUILD_VIRTUALDISPLAY_VULKAN_LAYER=OFF \
  -DSUNSHINE_BUNDLE_VHF_GAMEPAD_DRIVER=OFF \
  "-DSUNSHINE_DOTNET_EXECUTABLE=C:/Program Files/dotnet/dotnet.exe" \
  "-DSUNSHINE_NPM_EXECUTABLE=C:/Program Files/nodejs/npm.cmd"
echo "== build main"
cmake --build cmake-build-release --parallel 4
echo "== configure surround"
cmake -S contrib/nvidia-surround-toggle -B cmake-build-surround -G Ninja -DCMAKE_BUILD_TYPE=Release
echo "== build surround"
cmake --build cmake-build-surround
ls -l cmake-build-release/sunshine.exe cmake-build-surround/nvidia-surround-toggle.exe
echo "== BUILD OK"
```

For a clean rebuild, remove the old directories first through the same wrapper, for example
`rm -rf cmake-build-release cmake-build-surround` in a script. Inspect the directories and
their `CMakeCache.txt` before deleting them, and keep unrelated build directories.

The main build is about 900 Ninja steps and takes a long time, longer than a foreground tool
call allows. Run it in the background and log to a WSL file:

```bash
cd /mnt/c/dev && /mnt/c/Windows/System32/cmd.exe /c 'C:\Users\<user>\AppData\Local\Temp\vibepollo-agent\run.cmd' build 2>&1 | tr -d '\r' > build.log; echo "exit=${PIPESTATUS[0]}"
```

The `cmd.exe` exit status propagates the script's failure, but treat `== BUILD OK` as the
success signal. Check progress with `grep -o '^\[[0-9]*/[0-9]*\]' build.log | tail -1` and
failures with `grep -n 'FAILED\|error:' build.log`.

## 6. Outputs and next steps

- `C:\dev\Vibepollo\cmake-build-release\sunshine.exe`, with helpers under
  `cmake-build-release\tools\`
- `C:\dev\Vibepollo\cmake-build-surround\nvidia-surround-toggle.exe`. Building it does not
  change display settings; read its
  [usage guide](../../contrib/nvidia-surround-toggle/README.md) before running it.

A successful build only proves compilation. Run tests the same way, with a script such as
`ctest --test-dir cmake-build-release --output-on-failure --no-tests=error`, and report test
results, skips, and whether a live stream was tested separately from the build result.
