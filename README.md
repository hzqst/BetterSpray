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

## License

This repository currently provides no standalone project license. Dependencies
retain their own licenses and terms; bundled notices are installed under
`svencoop/metahook/licenses/betterspray/`.
