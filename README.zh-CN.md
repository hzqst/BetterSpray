# BetterSpray

[English README](README.md)

BetterSpray 是喷漆增强插件，支持将WAD喷漆替换成透明通道的高分辨率图片、Steam 截图云分享，以及自动生成WAD版本喷漆作为回退方案。

## 兼容性

| Engine | |
| --- | --- |
| GoldSrc_blob (3248~4554) | x |
| GoldSrc_legacy (< 6153) | x |
| GoldSrc_new (8684 ~) | x |
| SvEngine (8832 ~) | √ |
| GoldSrc_HL25 (>= 9884) | √ |

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

## License

BetterSpray 采用 MIT License，见 `LICENSE`。随附的第三方源码保留各自的许可证文件。