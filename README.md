# athrill-target-v850e2m
![example workflow](https://github.com/toppers/athrill-target-v850e2m/actions/workflows/build.yml/badge.svg)

v850e2m Target dependencies for [Athrill](https://github.com/toppers/athrill)

See the [configuration reference](docs/configuration.md) for every V850 target parameter and the link to the common
Athrill options, memory map, and core parameters.

## Build

Check out this repository and [Athrill](https://github.com/toppers/athrill) side by side; the build uses the sibling `../athrill` checkout:

```shell
git clone https://github.com/toppers/athrill.git
git clone https://github.com/toppers/athrill-target-v850e2m.git
cd athrill-target-v850e2m
```

The Business Pack-compatible entry point is:

```shell
python tools/hako.py doctor
python tools/hako.py build
python tools/hako.py smoke
```

The checked-in `hakoniwa-build.yaml` selects the sibling `../athrill`
checkout and, on Windows, `C:\project\vcpkg`. The Windows driver locates
Visual Studio, initializes the x64 developer environment, and maps a WSL UNC
workspace before invoking CMake. Build settings can be overridden with
`--config <manifest>`.

To install the executable and emit a Business Pack Component Receipt:

```shell
python tools/hako.py --install-dir .hako/install install
```

The lower-level Windows presets remain available from a Visual Studio
Developer PowerShell:

```powershell
$env:VCPKG_ROOT = 'C:\project\vcpkg'
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-smoke
```

### Generic CMake build

From the `athrill-target-v850e2m` directory:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Pass `-DATHRILL_SOURCE_DIR=<path>` to use an Athrill checkout somewhere else.


## License

This repository is distributed with [TOPPERS License](https://toppers.jp/en/license.html).
