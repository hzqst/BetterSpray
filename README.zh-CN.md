# BetterSpray

本插件动态链接 `SteamAPIBridge.dll`，安装在 `metahook/dlls`。部署插件时请一并保留该依赖。
Bridge 使用游戏已有的 Steam 运行库，不替换 `steam_api.dll`。独立构建可通过
`STEAMAPIBRIDGE_SOURCE_PATH` 指定源码，否则获取固定提交。

[English README](README.md)

BetterSpray 是喷漆增强插件，支持将WAD喷漆替换成透明通道的高分辨率图片、Steam 截图云分享，以及自动生成WAD版本喷漆作为回退方案。

## 兼容性

| Engine | 支持情况 |
| --- | --- |
| GoldSrc_blob (3248~4554) | 部分支持：仅本机高清喷漆 |
| GoldSrc_legacy (6153, 8684) | 部分支持：仅本机高清喷漆 |
| SvEngine (8832 ~) | 完整支持，含 Steam 云同步 |
| GoldSrc_HL25 (>= 9884) | 完整支持，含 Steam 云同步 |

旧版 GoldSrc（`hl-3248` ~ `hl-8684`）不提供每个玩家的 SteamID：
其 `player_info_t` 缺少 BetterSpray 读取的 Sven Co-op 扩展字段。
在这些引擎上，插件只渲染你自己的高清喷漆并跳过云端查询，其他玩家继续使用引擎自带的 WAD 喷漆。
SvEngine 与 GoldSrc_HL25 保留完整功能。

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/BetterSpray/releases) 获取 `BetterSpray-windows-x86.7z`，或在[本地构建插件](docs/zh-CN/build-instruction.md)。

安装[运行时依赖](docs/zh-CN/installation.md)，将解压出的 `svencoop/` 合并到目标 mod 目录。
在 MetaHook 的 `metahook/configs/plugins.lst` 中启用 `BetterSpray.dll`，然后通过 MetaHook 启动游戏。

云分享时，将 Steam 截图描述设为 `!Spray` 并公开分享；详情见[功能说明](docs/zh-CN/features.md)。

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [安装说明](docs/zh-CN/installation.md)
- [功能说明](docs/zh-CN/features.md)
- [gamedata](docs/zh-CN/gamedata.md)
- [自动化构建与发布](docs/zh-CN/ci-cd.md)
- [F5 调试（可选）](docs/zh-CN/debugging.md)

## License

BetterSpray 采用 MIT License，见 `LICENSE`。随附的第三方源码保留各自的许可证文件。

## C/C++ 格式化

使用 [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
共享工具及固定版本 **clang-format 23.1.3**，采用 DiligentCore 风格（4 空格，保留
include 顺序）。为 CMake 使用的 Python 解释器安装格式工具：

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

格式专用配置需要 CMake 3.21+、Git、Python 3.9+（CI 使用 3.12）及构建生成器；
使用 `-G Ninja` 可无需 Visual Studio。它不准备原生 SDK 或游戏依赖。
格式目标需显式执行，不加入普通 DLL 构建。使用 Visual Studio 生成器时，执行目标
需追加 `--config Debug` 或 `--config Release`。

聚合仓库注入 `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`。
独立组件支持该 CMake 参数及同名环境变量；为空时通过 FetchContent 获取固定工具
提交。相对路径应加引号，例如
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`。
配置时在仓库根目录生成被 gitignore 的 `.clang-format` 供编辑器使用；格式规则应
在共享仓库修改，不修改生成副本。可通过 `FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE`
指定工具路径，但版本仍须与固定版本一致。

检查覆盖 `src/`、`include/`、`tests/` 中维护的 C/C++ 文件，包括未被 Git 忽略的新文件。
相对仓库根目录的排除规则位于 `.clang-format-ignore`；第三方源和构建产物不纳入检查。
`clang-format` workflow 在 push、pull request 和手动运行时执行全量检查。
