[Back to README](../../README.md) | [中文](../zh-CN/build-instruction.md)

# Build instruction

This page covers the build, dependencies, gamedata and regression tests of BetterSpray.
See [Installation](installation.md) for deployment and [Automated builds](ci-cd.md) for CI.

## Requirements

- Windows, Visual Studio 2022 C++ desktop workload, x86 MSVC and a Windows SDK
- CMake 3.21+, Git and Python 3 on `PATH`
- Network access on the first configure and gamedata synchronization
- 7-Zip for packaging and archive verification

## Build

```bat
scripts\build-BetterSpray-x86-Debug.bat
scripts\build-BetterSpray-x86-Release.bat
```

Both entry points use `Visual Studio 17 2022 -A Win32` and perform configure, build and
install. They work from outside the repository; when `SolutionDir` is unset, the root
is located from the script path. A failed step returns a non-zero exit code.
Build directories are `build/x86/<Debug|Release>` and installation directories are
`install/x86/<Debug|Release>`. Deploy the installation manually.

## Dependencies and source paths

CMake downloads the MetaHook SDK from the latest `main`, and fixed versions of VGUI2Extension, UtilThreadTask and UtilHTTPClient
interface sources, SteamSDK, FreeImage, libxml2, ScopeExit and Chocobo1Hash.
Versions are recorded in [cmake/Dependencies.cmake](../../cmake/Dependencies.cmake).
VC-LTL 5.3.1 uses a SHA-256 verified binary package shared between configurations.

Every source path below is optional; set it to reuse an existing checkout:

| Parameter | Required contents |
| --- | --- |
| `METAHOOK_SOURCE_PATH` | MetaHook root with `include/metahook.h`, HLSDK, SourceSDK and VGUI sources |
| `VGUI2EXTENSION_SOURCE_PATH` | VGUI2Extension root with `include/Interface/` |
| `UTILTHREADTASK_SOURCE_PATH` | UtilThreadTask root with `include/Interface/IUtilThreadTask.h` |
| `UTILHTTPCLIENT_SOURCE_PATH` | UtilHTTPClient_libcurl root with `include/Interface/IUtilHTTPClient.h`; headers only |
| `STEAMSDK_SOURCE_PATH` | SteamSDK `steam/` headers and `STEAM-SDK-NOTICE.md` (read-only) |
| `STEAMAPIBRIDGE_SOURCE_PATH` | SteamAPIBridge source; empty fetches the pinned commit and builds its shared DLL |
| `FREEIMAGE_SOURCE_PATH` | FreeImage 3.18.0 fork with its CMake project and `Source/FreeImage.h` |
| `LIBXML2_SOURCE_PATH` | Official libxml2 2.14.2 CMake source tree |
| `SCOPEEXIT_SOURCE_PATH` | ScopeExit root with `include/ScopeExit/ScopeExit.h` |
| `CHOCOBO1HASH_SOURCE_PATH` | Chocobo1Hash root with `src/md5.h` |
| `VC_LTL_Root` | Extracted VC-LTL 5.3.1 binary package |

For example:

```bat
scripts\build-BetterSpray-x86-Release.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DVGUI2EXTENSION_SOURCE_PATH=D:/VGUI2Extension" "-DUTILTHREADTASK_SOURCE_PATH=D:/UtilThreadTask" "-DSTEAMSDK_SOURCE_PATH=D:/SteamSDK"
```

Parameters accept same-named environment variables on first configure. Use `-DNAME=value`
to change a cached value; an empty source path restores automatic downloading.
Relative paths are resolved against the project root. Invalid explicit paths fail
before downloads. External checkouts are left unchanged; library build outputs stay
inside the CMake build directory.

FreeImage retains its bundled codecs and the original Debug/Release DLL names.
libxml2 retains HTML, XPath, threads and built-in encodings. HTTP, iconv, ICU, zlib,
lzma, command-line programs, Python bindings and vendor tests are disabled.
HTTP transfers use the separately installed UtilHTTPClient runtime.

## Build options

| Option | Default | Purpose |
| --- | --- | --- |
| `BETTERSPRAY_BUILD_TESTS` | `OFF` | Build gamedata, library and DLL loading regression tests |
| `BETTERSPRAY_SYNC_GAMEDATA` | `ON` | Synchronize, prune and validate the plugin catalog |
| `BETTERSPRAY_GAMEDATA_DIR` | `<build>/assets/svencoop/metahook/gamedata/betterspray` | Catalog output directory |
| `BETTERSPRAY_DEPENDENCY_CACHE_DIR` | `thirdparty/cache` | VC-LTL archive and extraction cache |

## gamedata and offline builds

With synchronization enabled, the build downloads the catalog, keeps the manifest's
Windows engine records and validates them before building the DLL. Raw snapshots are
cached under `<build>/gamedata-sync/`. See [gamedata](gamedata.md) for runtime requirements.

With synchronization disabled, only an existing catalog is installed. A fresh build
directory does not provide a catalog in this mode. For offline use, populate the
source, VC-LTL and gamedata caches with an online build first, or supply local source
paths, `VC_LTL_Root` and a prepared `BETTERSPRAY_GAMEDATA_DIR`.

## Regression tests

```bat
scripts\build-BetterSpray-x86-Release.bat -DBETTERSPRAY_BUILD_TESTS=ON
ctest --test-dir build/x86/Release -C Release --output-on-failure
scripts\build-BetterSpray-x86-Debug.bat -DBETTERSPRAY_BUILD_TESTS=ON
ctest --test-dir build/x86/Debug -C Debug --output-on-failure
python -m unittest discover -s scripts/tests -v
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/betterspray --manifest scripts/manifests/betterspray.json
```

Tests retain assertions in Release. They cover host gamedata calls and error paths,
PNG alpha/WebP round trips, UTF-8 HTML/XPath parsing and loading the DLL's two factories.
Compilation and simulated tests do not establish in-game spray, UI or cloud behavior.
