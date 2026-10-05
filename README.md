# BetterSpray

This plugin dynamically links SteamAPIBridge.dll, installed under `metahook/dlls`.
Keep this dependency with the plugin when deploying. The bridge uses the game's
existing Steam runtime; it does not replace `steam_api.dll`. Standalone builds accept
`STEAMAPIBRIDGE_SOURCE_PATH` or fetch a fixed bridge commit.

[中文文档](README.zh-CN.md)

BetterSpray is a spray enhancement plugin for MetaHook.

It supports: using high-res images as player decals, sharing spray via Steam screenshots, fallback-WAD auto-generation.

## Compatibility

| Engine | Support |
| --- | --- |
| GoldSrc_blob (3248~4554) | Partial: local high-res spray only |
| GoldSrc_legacy (6153, 8684) | Partial: local high-res spray only |
| SvEngine (8832 ~) | Full, including Steam cloud sync |
| GoldSrc_HL25 (>= 9884) | Full, including Steam cloud sync |

Legacy GoldSrc builds (`hl-3248` ~ `hl-8684`) do not expose a per-player SteamID:
their `player_info_t` lacks the Sven Co-op extension BetterSpray reads. On those
engines the plugin renders your own high-res spray and skips cloud lookup, so other
players keep the engine's WAD decal. SvEngine and GoldSrc_HL25 keep the full feature
set.

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
- [F5 debugging (optional)](docs/en/debugging.md)

## License

BetterSpray is available under the MIT License; see `LICENSE`. Bundled third-party
sources keep their own license files.
