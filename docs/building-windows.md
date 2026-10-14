# Building Vibepollo on Windows

This guide builds the **Windows x64 host** from this fork using MSYS2 UCRT64,
GCC, CMake, and Ninja. It follows the checked-in
[Windows workflow](../.github/workflows/ci-windows.yml), with optional release
features disabled for the first developer build. You can stream to Moonlight
without building Chromium/WebRTC or rebuilding the display driver.

Run Bash examples in **MSYS2 UCRT64**, PowerShell examples in **Windows PowerShell**,
and Command Prompt examples in **cmd.exe**. Start each section from the repository
root unless it says otherwise. Building does not install or start a service.

## 1. Install the tools

| Tool | Used for |
| --- | --- |
| [MSYS2](https://www.msys2.org/), installed at `C:\msys64` | UCRT64 GCC, CMake, Ninja, and native dependencies. |
| [Node.js](https://nodejs.org/en/download) with npm | Both browser interfaces. The checked-in Windows CI uses Node 22.22.2. |
| [.NET SDK](https://dotnet.microsoft.com/download) | The Playnite plugin, even for a host-only build. CI uses the 10.x SDK; a runtime-only install is insufficient. |
| Windows PowerShell 5.1 | Build helpers and package validation; normally included with Windows. |
| Visual Studio 2022 or its Build Tools, with **Desktop development with C++**, MSVC x86/x64 tools, and a Windows SDK | MSI packaging builds an additional x86 Vulkan layer with MSVC, even when the x64 driver is prebuilt. Also needed to build WebRTC from source. The host itself still uses UCRT64 GCC. |
| [WiX Toolset v3](https://github.com/wixtoolset/wix3/releases), 3.14 | MSI packaging; its `candle.exe` and `light.exe` must be available. WiX v4+ is not a substitute for this build's WiX v3 sources. |

Visual Studio and WiX are needed for the packaging steps below, rather than the
initial GCC host build. Use a normal user shell for compiling. Installing a
finished package or registering services is a separate administrator action.

Open **MSYS2 UCRT64**, update packages, and reopen the shell if the updater asks:

```bash
pacman -Syu
```

After reopening, repeat the update to finish any remaining packages, then install
these dependencies, matching the Windows CI package list with Python added for tests:

```bash
pacman -Syu
pacman -S --needed \
  git \
  mingw-w64-ucrt-x86_64-boost \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-cppwinrt \
  mingw-w64-ucrt-x86_64-curl-winssl \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-libjpeg-turbo \
  mingw-w64-ucrt-x86_64-libpng \
  mingw-w64-ucrt-x86_64-libwebp \
  mingw-w64-ucrt-x86_64-MinHook \
  mingw-w64-ucrt-x86_64-miniupnpc \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-nlohmann-json \
  mingw-w64-ucrt-x86_64-onevpl \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-opus \
  mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-tools
```

Expose Windows-installed tools in this shell; adjust paths if you installed them elsewhere:

```bash
export PATH="/c/Program Files/nodejs:/c/Program Files/dotnet:/c/Windows/System32/WindowsPowerShell/v1.0:$PATH"
command -v g++ cmake ninja node npm.cmd dotnet.exe powershell.exe
printf '%s\n' "$MSYSTEM"
g++ --version
node --version
dotnet.exe --list-sdks
```

`MSYSTEM` should print `UCRT64`, and GCC/CMake should resolve from `/ucrt64/bin`.
Do not mix MINGW64, UCRT64, MSVC, or Linux object files in one build directory.

## 2. Clone this fork

Use a short path on a local Windows drive, such as `C:\src\Vibepollo`.
Avoid network shares and `\\wsl.localhost\...` checkouts: Windows CMake/Ninja
commands can fail when cmd.exe receives a UNC working directory. If controlling
the build from WSL or SSH, still build with Windows MSYS2 tools against a Windows-drive checkout.

```bash
mkdir -p /c/src
cd /c/src
git -c core.longpaths=true clone https://github.com/FarisR99/Vibepollo.git
cd Vibepollo
git -c core.longpaths=true submodule update --init --recursive
```

For an existing checkout, run the submodule command again after updating the
branch. Keep the revisions pinned by the parent repository; do not use
`git submodule update --remote`. Initial submodule downloads can take a while.

## 3. Configure and compile

From `/c/src/Vibepollo` in UCRT64:

```bash
cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF \
  -DSUNSHINE_ENABLE_WEBRTC=OFF \
  -DBUILD_SUNSHINE_VIRTUAL_DISPLAY_DRIVER=OFF \
  -DBUILD_VIRTUALDISPLAY_PROBE=OFF \
  -DBUILD_VIRTUALDISPLAY_TOOLS=OFF \
  -DBUILD_VIRTUALDISPLAY_VULKAN_LAYER=OFF \
  -DSUNSHINE_BUNDLE_VHF_GAMEPAD_DRIVER=OFF \
  -DSUNSHINE_DOTNET_EXECUTABLE="$(cygpath -m "$(command -v dotnet.exe)")" \
  -DSUNSHINE_NPM_EXECUTABLE="$(cygpath -m "$(command -v npm.cmd)")"
cmake --build cmake-build-release --parallel 4
```

Reduce the parallel count if memory is tight. Configuration downloads prebuilt
FFmpeg when it is not cached, and the build restores npm/NuGet dependencies, so
internet access is needed. CMake builds both browser interfaces and the Playnite
plugin automatically. You do not need a separate frontend build first.

The main output is `cmake-build-release/sunshine.exe`; the binary retains the
Sunshine name. Helper executables are under `cmake-build-release/tools/`, and
browser assets are under `cmake-build-release/assets/web/`, with the current
interface in `v2/`.

This profile leaves browser streaming/WebRTC and VHF gamepad-driver bundling off.
It does not disable the host's existing controller implementations. Native display
driver source compilation is off; packaging uses the repository's pinned producer
release instead. These switches are not equivalent to an official release build.

## 4. Run tests and a development instance

```bash
ctest --test-dir cmake-build-release --output-on-failure --no-tests=error
```

For a focused check of this fork's crop and instance policy:

```bash
cmake --build cmake-build-release --target test_fast_fork_features
ctest --test-dir cmake-build-release -R '^test_fast_fork_features$' --output-on-failure --no-tests=error
```

Some tests require Windows facilities or GPU hardware; report skips or failures
with their test names. Passing the focused target does not establish that the full
suite or an actual stream works.

For frontend-only changes, install dependencies in both interfaces before testing:

```bash
(cd src_assets/common/assets/web-legacy && npm ci --ignore-scripts)
(cd src_assets/common/assets/web && npm ci --ignore-scripts && npm run test:unit && npm run build)
```

To run a development host from the build tree, stay in UCRT64 so dependency DLLs
are on PATH, and use its build directory as the working directory:

```bash
cd cmake-build-release
./sunshine.exe --config-dir C:/src/vibepollo-dev-config port=48989
```

Use a free base port and a separate config directory from your installed host.
For this example the web UI is `https://localhost:48990`. Building and testing do
not install virtual display/input drivers; runtime driver features need their
normal installed prerequisites. See [fork configuration](fork_configuration.md)
for additional Windows service instances and split-screen setup.

## 5. Build a local installer

Install the Visual Studio components and WiX v3 from step 1. In UCRT64, add the
WiX v3 tools to PATH (adjust the versioned directory to your installation):

```bash
export PATH="/c/Program Files (x86)/WiX Toolset v3.14/bin:$PATH"
command -v candle.exe light.exe
cmake --build cmake-build-release --target package_msi --parallel 4
```

The MSI is written to `cmake-build-release/cpack_artifacts/Vibepollo.msi`.
Use `package_msi` rather than invoking CPack directly: the target runs required
driver refresh/validation prerequisites. It downloads the display package pinned
in [the release lock](../third-party/libvirtualdisplay-release/windows-x64.json)
and builds the additional x86 Vulkan layer using MSVC. It can refresh tracked
files under `src_assets/windows/drivers/`; inspect `git status` afterward.

To build the wrapper installer EXE as well:

```bash
cmake --build cmake-build-release --target package_installer --parallel 4
```

Find its output in the same `cpack_artifacts` directory. These local targets do
not use the project's SignPath release-signing flow. The VHF gamepad package is
not bundled by this recipe; release-equivalent packaging has additional producer
artifact requirements documented in the Windows workflow and
[driver safety guide](../packaging/windows/driver_install_safety.md).
Building an installer does not run it or register a Windows service.

## Optional: enable browser streaming

Use the pinned WebRTC downloader instead of building Chromium/WebRTC for a first
WebRTC-enabled build. In **Windows PowerShell**, from `C:\src\Vibepollo`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_webrtc_release.ps1
```

The downloader validates the checked-in manifest and stages the SDK under
`.vibepollo-deps\libwebrtc\out`. Back in **UCRT64**, reconfigure the existing
build, keeping its other cached options:

```bash
cmake -S . -B cmake-build-release \
  -DSUNSHINE_ENABLE_WEBRTC=ON \
  -DWEBRTC_ROOT="$(cygpath -m "$PWD/.vibepollo-deps/libwebrtc/out")"
cmake --build cmake-build-release --parallel 4
```

To work on WebRTC itself, use `scripts/build_mingw_webrtc.ps1` instead. Its staged
SDK defaults to `%LOCALAPPDATA%\Vibepollo\deps\libwebrtc\out`, shared across
checkouts; point `WEBRTC_ROOT` there if overriding a previously cached location.
That source build is much larger and requires Visual Studio and the Windows SDK.
See the script's parameters before changing its cache directories.

## Optional: build the NVIDIA Surround tool

The tool is independent of the main build and is not built by `sunshine`:

```bash
cmake -S contrib/nvidia-surround-toggle -B cmake-build-surround -G Ninja
cmake --build cmake-build-surround
```

This produces `cmake-build-surround/nvidia-surround-toggle.exe`. Building it does
not change display settings. See its [usage guide](../contrib/nvidia-surround-toggle/README.md)
before running commands that apply display changes.

## Troubleshooting

- **Missing headers or CMakeLists.txt in third-party:** rerun the recursive
  submodule update; ZIP source downloads do not contain the required submodules.
- **npm or dotnet not found:** reopen UCRT64 after installing the tools, check
  PATH, and pass the explicit executable paths from step 3. A .NET runtime is
  not the SDK. A missing Vue module from `web-legacy` usually means only the
  current UI's dependencies were installed.
- **Compiler or CMake cache mismatch:** configure a fresh `cmake-build-*`
  directory after changing toolchains. Do not reuse a Linux or MSVC host cache.
- **candle/light missing or incompatible WiX:** use WiX v3.14 and verify both
  tools resolve in the shell performing packaging.
- **VsDevCmd.bat, cl.exe, or Windows SDK missing during packaging:** add the
  Visual Studio C++ x86/x64 components. The x86 Vulkan packaging step needs MSVC
  even though the host was built with GCC.
- **GitHub dependency download failures:** check connectivity and API rate
  limits. The downloader scripts support `GH_TOKEN`/`GITHUB_TOKEN` when needed;
  keep tokens out of commands, documentation, and commits.

For the authoritative options and dependency versions, consult
[options.cmake](../cmake/prep/options.cmake),
[Windows CI](../.github/workflows/ci-windows.yml), and the pinned dependency
manifests in `third-party/`. Revisit these instructions when those files change.
