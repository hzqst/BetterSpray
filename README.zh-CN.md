# BetterSpray

[English README](README.md)

BetterSpray 是 MetaHook 的喷漆增强插件，支持带透明通道的高分辨率图片、动态重载、
Steam 截图云分享，以及供未安装插件的玩家查看的 WAD 回退版本。

## 兼容性

| Engine | |
| --- | --- |
| GoldSrc_blob (3248~4554) | x |
| GoldSrc_legacy (< 6153) | x |
| GoldSrc_new (8684 ~) | x |
| SvEngine (8832 ~) | √ |
| GoldSrc_HL25 (>= 9884) | √ |

以上沿用项目已有的兼容性声明。随包引擎目录覆盖 Sven Co-op 8948/10257 和
Half-Life 10210；详情见 [gamedata](docs/zh-CN/gamedata.md)。

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/BetterSpray/releases) 获取
`BetterSpray-windows-x86.7z`，或在[本地构建插件](docs/zh-CN/build-instruction.md)。

安装[运行时依赖](docs/zh-CN/installation.md)，将解压出的 `svencoop/` 合并到目标 mod 目录。
在 MetaHook 的 `metahook/configs/plugins.lst` 中启用 `BetterSpray.dll`，然后通过 MetaHook 启动游戏。
云分享时，将 Steam 截图描述设为 `!Spray` 并公开分享；详情见[功能说明](docs/zh-CN/features.md)。

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [安装说明](docs/zh-CN/installation.md)
- [功能说明](docs/zh-CN/features.md)
- [gamedata](docs/zh-CN/gamedata.md)
- [自动化构建与发布](docs/zh-CN/ci-cd.md)

## 许可证

本仓库目前未提供独立的项目许可证。各依赖保留自己的许可证及使用条款；
随包许可声明安装到 `svencoop/metahook/licenses/betterspray/`。
