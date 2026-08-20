# athrill-target-v850e2m
![example workflow](https://github.com/toppers/athrill-target-v850e2m/actions/workflows/build.yml/badge.svg)

v850e2m Target dependencies for [Athrill](https://github.com/toppers/athrill)
## Build

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

### Legacy Make build

```
git clone --recursive https://github.com/toppers/athrill-target-v850e2m
cd athrill-target-v850e2m/build_linux
make
```

### Generic CMake build

```
git clone --recursive https://github.com/toppers/athrill-target-v850e2m
cmake -S athrill-target-v850e2m -B athrill-target-v850e2m/build
cmake --build athrill-target-v850e2m/build
```


## License

This repository is distributed with [TOPPERS License](https://toppers.jp/en/license.html).

