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
