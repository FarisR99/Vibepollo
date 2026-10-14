# Building
Sunshine binaries are built using [CMake](https://cmake.org) and requires `cmake` > 3.25.
The browser interface is built with Node.js and npm. CMake's `web_ui` target
installs the locked dependencies with lifecycle scripts disabled, generates the
design tokens, type-checks the Vue source, and writes the production bundle to
`<build-dir>/assets/web`. The `sunshine` target and installer packaging depend on
this target, and packaging fails rather than shipping an incomplete
configuration interface.

For frontend-only development, run `npm ci --ignore-scripts` and `npm run dev`
from `src_assets/common/assets/web`. Use `npm run build` for a production bundle.

## Building Locally

### Compiler
It is recommended to use one of the following compilers:

| Compiler    | Version |
|:------------|:--------|
| GCC         | 14+     |
| Clang       | 17+     |
| Apple Clang | 15+     |

### Linux release CUDA compatibility

Linux release packages pin CUDA Toolkit **12.9.1** (runfile build **575.57.08**)
to retain Maxwell, Pascal (including GTX 1070), and Volta GPU targets. CUDA 13
cannot compile those targets. Use GCC 14 as the CUDA host compiler; older
supported builders may use GCC 13. The runfile installs only the toolkit,
not a GPU driver.

The shared Linux build script uses the pinned runfile in `build/cuda` by
default, ignoring a newer system toolkit. It rejects an incompatible cached
toolkit. Arch's PKGBUILD downloads architecture-specific, SHA-256-verified
runfiles into its source directory and explicitly selects GCC 14. Arch CI
installs the signed, versioned GCC 14 packages from the Arch Linux Archive,
because rolling Arch repositories no longer provide them. Manual Arch package
builds also need the `gcc14` build dependency installed.

Release packaging enables `SUNSHINE_REQUIRE_CUDA_PASCAL=ON`, which requires
CUDA 12.9 and the `sm_61` target. Flatpak retains its existing 12.9.1 pin.
When changing toolchain versions, run
`python3 tests/unit/test_cuda_release_policy.py` and build the affected
packages locally before publishing. Developer builds can leave that option
off to use newer CUDA toolkits with a reduced set of GPU targets.

### Dependencies

#### FreeBSD
> [!CAUTION]
> Sunshine support for FreeBSD is experimental and may be incomplete or not work as expected

##### Install dependencies
```sh
pkg install -y \
  audio/opus \
  audio/pulseaudio \
  devel/cmake \
  devel/evdev-proto \
  devel/git \
  devel/libayatana-appindicator \
  devel/libevdev \
  devel/libnotify \
  devel/ninja \
  devel/pkgconf \
  ftp/curl \
  graphics/libdrm \
  graphics/wayland \
  multimedia/libva \
  net/miniupnpc \
  ports-mgmt/pkg \
  security/openssl \
  shells/bash \
  x11/libX11 \
  x11/libxcb \
  x11/libXfixes \
  x11/libXrandr \
  x11/libXtst
```

#### Linux
Dependencies vary depending on the distribution. You can reference our
[linux_build.sh](https://github.com/LizardByte/Sunshine/blob/master/scripts/linux_build.sh) script for a list of
dependencies we use in Debian-based, Fedora-based and Arch-based distributions. Please submit a PR if you would like to extend the
script to support other distributions.

##### KMS Capture
Do **not** add file capabilities to the `vibepollo` binary. On Linux the public `/usr/bin/vibepollo`
must stay unprivileged; KMS capture runs inside the private `/usr/libexec/vibeshine/vibepollo-host`,
which the package installs with exactly `cap_sys_admin,cap_sys_nice=p`, and the package hook
verifies that the public binary has no capabilities. Install through the Arch package (or follow the
staged-install recipe in `AGENTS.md`) rather than running a capability-patched build directly.

##### CUDA Toolkit
CUDA-enabled builds require CUDA Toolkit **12.0 or newer**. The native
[local build/deploy script](linux/local-development.md#build-settings) uses an
installed toolkit and compiler; it does not pin CUDA 13 or GCC 15. Choose a
host compiler supported by that toolkit and a C++ compiler/standard library
that meets the project's C++23 requirements.

The selected toolkit determines which GPU generations can be targeted. Release
packages deliberately use **CUDA 12.9.1 with GCC 14**, as described above, to
cover both older and newer GPUs. This release pin is not the minimum for local
builds. CUDA 13 removes compilation support for Maxwell, Pascal, and Volta;
upgrading the toolkit can therefore reduce GPU compatibility. See NVIDIA's
[CUDA 13 release notes](https://docs.nvidia.com/cuda/archive/13.0.0/cuda-toolkit-release-notes/index.html)
and [CUDA compatibility guide](https://docs.nvidia.com/deploy/cuda-compatibility/index.html).

The native Arch/CachyOS release package uses a private build-time toolkit and
does not require users to install the CUDA toolkit to run Vibepollo.

> [!NOTE]
> To install older versions, select the appropriate run file based on your desired CUDA version and architecture
> according to [CUDA Toolkit Archive](https://developer.nvidia.com/cuda-toolkit-archive)

#### macOS
You can either use [Homebrew](https://brew.sh) or [MacPorts](https://www.macports.org) to install dependencies.

##### Homebrew
```bash
dependencies=(
  "boost"  # Optional
  "cmake"
  "doxygen"  # Optional, for docs
  "graphviz"  # Optional, for docs
  "icu4c"  # Optional, if boost is not installed
  "miniupnpc"
  "ninja"
  "openssl@3"
  "opus"
  "pkg-config"
)
brew install "${dependencies[@]}"
```

If there are issues with an SSL header that is not found:

@tabs{
  @tab{ Intel | ```bash
    ln -s /usr/local/opt/openssl/include/openssl /usr/local/include/openssl
    ```}
  @tab{ Apple Silicon | ```bash
    ln -s /opt/homebrew/opt/openssl/include/openssl /opt/homebrew/include/openssl
    ```
  }
}

##### MacPorts
```bash
dependencies=(
  "cmake"
  "curl"
  "doxygen"  # Optional, for docs
  "graphviz"  # Optional, for docs
  "libopus"
  "miniupnpc"
  "ninja"
  "pkgconfig"
)
sudo port install "${dependencies[@]}"
```

#### Windows

Use the [Windows x64 developer build guide](building-windows.md). It contains the
maintained MSYS2 UCRT64 dependency list, .NET and Node setup, fork clone commands,
CMake configuration, tests, installer targets, and optional WebRTC instructions.
Follow that recipe instead of the generic commands below when building Windows.

### Clone
Ensure [git](https://git-scm.com) is installed on your system, then clone the repository using the following command:

```bash
git clone https://github.com/FarisR99/Vibepollo.git --recurse-submodules
cd Vibepollo
```

### Build

```bash
cmake -B cmake-build-release -G Ninja -S .
cmake --build cmake-build-release
```

> [!TIP]
> Available build options can be found in
> [options.cmake](../cmake/prep/options.cmake).

### Package

@tabs{
  @tab{FreeBSD | @tabs{
    @tab{pkg | ```bash
      cpack -G FREEBSD --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{Linux | @tabs{
    @tab{deb | ```bash
      cpack -G DEB --config ./cmake-build-release/CPackConfig.cmake
      ```}
    @tab{rpm | ```bash
      cpack -G RPM --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{macOS | @tabs{
    @tab{DragNDrop | ```bash
      cpack -G DragNDrop --config ./cmake-build-release/CPackConfig.cmake
      ```}
  }}
  @tab{Windows | ```bash
    cmake --build cmake-build-release --target package_msi
    ```
    Follow the [Windows developer guide](building-windows.md#5-build-a-local-installer)
    for prerequisites, driver validation, and the optional installer EXE.
  }

}

### Remote Build
It may be beneficial to build remotely in some cases. This will enable easier building on different operating systems.

1. Fork the project
2. Activate workflows
3. Trigger the *CI* workflow manually
4. Download the artifacts/binaries from the workflow run summary

<div class="section_buttons">

| Previous                              |                            Next |
|:--------------------------------------|--------------------------------:|
| [Troubleshooting](troubleshooting.md) | [Contributing](contributing.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>

### CUDA compatibility for scripted Linux builds

`scripts/linux_build.sh` pins CUDA 12.9.1 and a compatible host compiler to
retain Maxwell, Pascal (including GTX 10-series), and Volta targets. It rejects
incompatible cached toolkits before configuration. Direct CMake builds may
still use newer CUDA with `SUNSHINE_REQUIRE_CUDA_PASCAL=OFF` (the default);
CUDA 13 builds omit those older GPU targets. Set the option to `ON` when
producing a Pascal-compatible build with CUDA 12.9. This does not enable Linux
release publishing in Vibepollo's Windows release workflow.
