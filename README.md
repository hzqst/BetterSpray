# BetterSpray

[中文文档](README.zh-CN.md)

BetterSpray is a spray enhancement plugin for MetaHook.

It supports: using high-res images as player decals, sharing spray via Steam screenshots, fallback-WAD auto-generation.

## Compatibility

| Engine | |
| --- | --- |
| GoldSrc_blob (3248~4554) | x |
| GoldSrc_legacy (< 6153) | x |
| GoldSrc_new (8684 ~) | x |
| SvEngine (8832 ~) | √ |
| GoldSrc_HL25 (>= 9884) | √ |

## Quick start

Obtain `BetterSpray-windows-x86.7z` from
[GitHub Releases](https://github.com/MetaHookSv/BetterSpray/releases), or
[build the plugin locally](docs/en/build-instruction.md).

Install the [runtime dependencies](docs/en/installation.md), then merge the extracted
`svencoop/` into the target mod directory.

Enable `BetterSpray.dll` in MetaHook's `metahook/configs/plugins.lst`, then launch the
game through MetaHook. For cloud sharing, publish a Steam screenshot with the
description `!Spray`; see [Features](docs/en/features.md).

## Documentation

- [Build instruction](docs/en/build-instruction.md)
- [Installation](docs/en/installation.md)
- [Features](docs/en/features.md)
- [gamedata](docs/en/gamedata.md)
- [CI/CD](docs/en/ci-cd.md)

## F5 debugging (optional)

Install MetaHook and enable this plugin in the game's `plugins.lst` first. Configure a standalone Visual Studio Win32 solution:

```powershell
cmake -S . -B build/launch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
```

Open the solution, select **LaunchGame** and press **F5**. **DeployGame** builds this plugin and its dependencies, stages Install, and copies plugin DLLs/PDBs/resources before the native debugger starts the existing game launcher. Root launchers/runtime files and plugin lists remain unchanged. Set VS to build before running and **Do not launch** on build errors; stop the game before redeploying. Ordinary builds do not deploy.

`METAHOOKSV_GAME_DIRECTORY` defaults to Steam discovery; `METAHOOKSV_GAME_APPID` defaults to `225840`. Set `METAHOOKSV_GAME_MOD` for a custom mod and `METAHOOKSV_GAME_ARGUMENTS` for extra arguments. Debug and Release are supported.

The shared module uses `METAHOOKSV_LAUNCH_GAME_MODULE_DIR`, the surrounding MetaHookSv checkout, or a pinned source archive. Without Installer sources, it downloads the self-contained CLI from GitHub `latest` (no .NET required); `METAHOOKSV_INSTALLER_RELEASE` selects a fixed tag, and `METAHOOKSV_INSTALLER_CLI_EXECUTABLE` supplies an offline EXE. Plugin mode requires v20261004c or later. Valid caches under `build/launch/launch-game/installer/<release>` are reused without update checks; select another tag or clear that private cache to upgrade. `GH_TOKEN`/`GITHUB_TOKEN` may be supplied through the environment if GitHub API rate limits prevent the first download. The feature defaults OFF and performs no extra downloads when disabled.

## License

BetterSpray is available under the MIT License; see `LICENSE`. Bundled third-party
sources keep their own license files.
