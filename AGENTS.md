## Build and tests

On Windows we use msys2 and ucrt64 to compile.
Prefix Windows build and test commands with `C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c`.

Prefix build directories with `cmake-build-`.

Native tests are registered in tests/CMakeLists.txt as focused `test_fast_*` and
`test_component_*` executables. The added `test_fast_fork_features` target uses GoogleTest.

The project uses gtest as a test framework.

Add or update tests for new or modified methods and code. Target 100% coverage on changed code.

## Windows agent build workflow

Use [docs/building-windows.md](docs/building-windows.md) as the human-facing recipe.
Keep that guide, README.md's build link, and this section aligned with
.github/workflows/ci-windows.yml, cmake/prep/options.cmake, and dependency manifests.
Do not copy CI signing credentials or release-only artifact requirements into a local build.

1. Inspect `git status`, the existing CMake cache, toolchain, and task scope. Keep unrelated
   user changes intact. Use a Windows-drive checkout (for example `C:\src\Vibepollo`),
   including when driving Windows tools from WSL/SSH. Windows cmd.exe cannot use a WSL UNC
   working directory reliably. Windows interop uses the Windows token; Linux sudo does not elevate it.
2. Use the MSYS2 UCRT64 launcher shown above for every Windows build/test command.
   Confirm `$MSYSTEM` is `UCRT64`, GCC/CMake come from `/ucrt64/bin`, and `node`,
   `npm.cmd`, `dotnet.exe`, and PowerShell resolve. Install prerequisites only when within
   the user's authorized scope. Keep build directories prefixed `cmake-build-`.
3. Initialize missing submodules at the repository's pinned commits with
   `git submodule update --init --recursive`; do not use `--remote` or silently change gitlinks.
4. For a fresh developer host build, use the guide's Release/Ninja configuration:
   tests ON, docs OFF, WebRTC OFF, native virtual-display driver/probe/tools/Vulkan-layer
   builds OFF, and VHF driver bundling OFF. Pass the resolved npm.cmd and dotnet.exe paths.
   Reuse an existing cache only when its toolchain and selected features match the task;
   preserve a user's intentional WebRTC/driver configuration. Reconfigure after build-file changes.
5. Build with `cmake --build cmake-build-release --parallel 4` (adjust for available memory).
   The main executable is `sunshine.exe`; helpers are under `tools/`. CMake's `web_ui`
   target installs/builds both interfaces, and the host build includes the .NET Playnite plugin.
6. Run the affected targets, then the applicable CTest suite with `--output-on-failure
   --no-tests=error`. For fork policy changes, build/run `test_fast_fork_features`.
   For frontend changes, install locked dependencies in both web and web-legacy and run
   the current UI's `npm run test:unit` and `npm run build`. Some source-extraction Python
   regression harnesses are registered only on Linux; do not imply CTest on Windows ran them.
   Report actual test results, skips, missing prerequisites, and whether a live stream was tested.

For a command launched from **Windows Command Prompt**, start in the Windows checkout:

```bat
cd /d C:\src\Vibepollo
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "cmake --build cmake-build-release --parallel 4"
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "ctest --test-dir cmake-build-release --output-on-failure --no-tests=error"
```

The launcher does not repair a missing PATH or create a CMake configuration; prepare
those first using the guide. Do not mix shell quoting between Bash, PowerShell, and cmd.exe.

When driving the Windows build from WSL, follow
[docs/maintainers/windows-build-from-wsl.md](docs/maintainers/windows-build-from-wsl.md).
It covers the `.cmd` wrapper that avoids WSL-to-cmd.exe quoting failures, absolute tool paths,
slow git over `/mnt/c`, and the verified configure/build scripts.

For packaging, build `package_msi`, not CPack alone, so required producer validation and
refresh targets run. `package_installer` also builds the wrapper EXE. WiX v3.14 is required;
the additional x86 Vulkan layer needs Visual Studio 2022 C++ tools and a Windows SDK even
with a prebuilt x64 display driver. Outputs are under `<build-dir>/cpack_artifacts`.
Packaging may update tracked driver assets: inspect the diff and do not include regenerated
binaries in a code/docs commit unless the task calls for them. Local installer targets do
not perform the SignPath release-signing flow.

Enable WebRTC only when requested or required by the task; use the pinned downloader and
explicit `WEBRTC_ROOT` from the guide. Build the Surround tool separately when its sources change.
Distinguish compilation from runtime verification and deployment in your report. Keep runtime
checks within the user's requested scope, with separate config directories and free ports.
Do not push unless explicitly asked.

## Python tooling and formatting

This repository has no uv.lock. Use `uv run --extra lint python scripts/update_clang_format.py`
for the repository formatter, or run clang-format 20 on changed C/C++ files.
Inspect the diff and remove unrelated formatting changes. Do not add Sunshine's
lizardbyte-common dependency or replace Vibepollo's existing tooling as part of a feature port.

## Code style and documentation

When adding localization do not update any language other than `en`. This also means to exclude en-US or other variants.

Always add or update doxygen documentation.

The project requires that everything be documented in doxygen or the build will fail.

Primary doxygen comments should be done like so:

```cpp
  /**
   * @brief Describe the function, structure, etc.
   *
   * @param my_param Describe the parameter.
   * @return Describe the return.
   */
```

Inline doxygen comments should use `///< ...` instead of `/**< ... */`.

Always follow the style guidelines defined in .clang-format for c/c++ code.

Do not ever create issues or pull requests.
If asked to create an issue or pull request, do so in the user's fork at https://github.com/FarisR99/Vibepollo instead of the LizardByte GitHub organization.
Never create an issue or pull request in the LizardByte GitHub organization.

Do not add decorative separator comments, including dashed banners that label groups of tests. Use descriptive
test names and code structure to show the grouping instead.

## Issues and pull requests

When asked to create an issue or pull request, use the templates provided by the LizardByte/.github repo or
LizardByte/Sunshine repo, with the Sunshine templates taking precedence if both exist. At the end of the issue
or pull request body, add an attribution naming the AI agent, model, and thinking level used to generate it.
Repeat that attribution in a separate comment on the created issue or pull request. For example:

```
> This issue was generated by ChatGPT 6 Sol, extra high thinking level
```

When asked to create an issue, do only enough investigation to verify and accurately describe the problem unless the
user also asks for an investigation. Keep the issue concise: state the observed behavior, expected behavior, and
reproduction steps or evidence when available. Do not include an exhaustive root-cause analysis or implementation
plan in the issue. This limit applies only to creating issues; investigate or fix issues fully when asked.

## Code Review Rules

Report concrete issues introduced by the change that the contributor can address. For each finding, explain the
triggering condition, impact, and a practical fix; point to the relevant code or test. Keep workflow status separate
from code findings.

If GitHub Actions workflows are queued for maintainer approval, or a dependent check times out before its job can run,
do not leave an inline comment, request changes, or ask the contributor to run the workflows. The maintainer controls
approval. If the missing results materially limit the review, mention that once in the review summary as a verification
limit for the maintainer. Review actual failed checks for change-related problems when their logs are available.

For pull request reviews, verify the current PR head and mergeability against its target branch. If conflicts are
confirmed, tell the author in the review summary that the PR needs to be rebased and the conflicts resolved before
merging. Do not infer conflicts from an unknown or pending mergeability status.

When approving a pull request, the approval itself is sufficient. Do not add a comment solely to accompany it.

## Ported fork features

Preserve Nonary/Vibepollo's existing display routing, runtime config override lifecycle,
controller support, and service-mode WGC capture. Reuse existing behavior when a feature
already exists upstream. Document additions in README.md and docs/fork_configuration.md.

Do not push unless the user explicitly asks. Do not create issues or pull requests unless
explicitly requested; use FarisR99/Vibepollo, never Nonary or LizardByte repositories.

The current web interface is src_assets/common/assets/web; validate it with `npm ci`,
`npm run test:unit`, and `npm run build` in that directory. Preserve the legacy interface
when adding fields to application records. Standalone native tests are under tests/CMakeLists.txt;
focused test executables have their own test_fast_* or test_component_* names.
