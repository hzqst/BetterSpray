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

## C/C++ formatting

Formatting uses [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
and clang-format **23.1.3**, with the DiligentCore style (4 spaces, preserved include
order). Install the formatter for the Python interpreter used by CMake:

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

The format-only configuration needs CMake 3.21+, Git, Python 3.9+ (CI uses 3.12),
and a build generator; `-G Ninja` works without Visual Studio. It prepares no native
SDK or game dependencies. Formatting targets are explicit and are not part of a
normal DLL build. With a Visual Studio generator, add `--config Debug` or
`--config Release` when building a formatting target.

The aggregate provides `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`.
Standalone components accept that CMake variable or its environment counterpart;
if empty, FetchContent downloads the fixed tooling commit. Quote relative paths,
for example `"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`.
Configuration generates the ignored root `.clang-format` for editors; change the
shared style rather than that generated copy. An optional
`FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE` selects an explicit formatter, whose
version must still match the pin.

Checks cover owned C/C++ files in `src/`, `include/`, and `tests/`, including
non-ignored new files. Repository-relative exclusions live in `.clang-format-ignore`.
Third-party sources and build artifacts are excluded. The `clang-format` workflow
checks the full scope on pushes, pull requests, and manual runs.
