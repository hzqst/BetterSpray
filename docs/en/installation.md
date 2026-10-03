[Back to README](../../README.md) | [中文](../zh-CN/installation.md)

# Installation

See [Build instruction](build-instruction.md) for local builds and [Features](features.md)
for image selection and Steam screenshot sharing.

## Install layout

`install/x86/<Debug|Release>/`:

```text
svencoop/
  metahook/plugins/BetterSpray.dll
  metahook/plugins/BetterSpray.pdb
  metahook/dlls/libxml2.dll
  metahook/dlls/libxml2.pdb
  metahook/dlls/FreeImage/FreeImage.dll
  metahook/dlls/FreeImage/FreeImage.pdb
  metahook/gamedata/betterspray/
  metahook/licenses/betterspray/
  bettersprays/
include/Interface/
  ISprayDatabase.h
```

Debug uses `FreeImaged.dll` and `FreeImaged.pdb`. The runtime archive contains
`svencoop/` with `licenses/` excluded; the public interface header is available in the install tree for development.

## Runtime dependencies

- MetaHook with API 115 or newer, at least the API version used to compile the plugin,
  and support for merging nested gamedata catalogs. Keep its primary catalog installed.
- [UtilThreadTask](https://github.com/MetaHookSv/UtilThreadTask): install
  `UtilThreadTask.dll` under `<mod>/metahook/dlls/`.
- An HTTP client: install [UtilHTTPClient_libcurl](https://github.com/MetaHookSv/UtilHTTPClient_libcurl)
  with its libcurl runtime, or [UtilHTTPClient_SteamAPI](https://github.com/MetaHookSv/UtilHTTPClient_SteamAPI),
  according to that component's installation instructions. BetterSpray tries libcurl first.
- [VGUI2Extension](https://github.com/MetaHookSv/VGUI2Extension) enables the integrated settings
  and task-list UI. Enable its DLL in `plugins.lst`; when it is absent, UI callbacks are skipped.
- A compatible x86 `steam_api.dll`, normally supplied by the game/MetaHook installation.
  The matching runtime also comes from [SteamSDK](https://github.com/MetaHookSv/SteamSDK).

These separate runtimes are installed independently. FreeImage and libxml2 are bundled.

## Enable the plugin

1. Install the runtime dependencies above.
2. Merge the archive's `svencoop/` into the target mod directory.
3. Keep `metahook/gamedata/betterspray/` together with the matching DLL and resources.
4. Add `BetterSpray.dll` to `<mod>/metahook/configs/plugins.lst`.
5. Launch the game through MetaHook.
