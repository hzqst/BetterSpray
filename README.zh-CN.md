# BetterSpray

本插件动态链接 `SteamAPIBridge.dll`，安装在 `metahook/dlls`。部署插件时请一并保留该依赖。
Bridge 使用游戏已有的 Steam 运行库，不替换 `steam_api.dll`。独立构建可通过
`STEAMAPIBRIDGE_SOURCE_PATH` 指定源码，否则获取固定提交。

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

## F5 调试（可选）

先安装 MetaHook，并在游戏的 `plugins.lst` 中启用本插件，然后配置独立的 Visual Studio Win32 解决方案：

```powershell
cmake -S . -B build/launch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
```

打开解决方案，选择 **LaunchGame** 后按 **F5**。**DeployGame** 编译本插件及依赖，暂存 Install，再复制插件 DLL、PDB 和资源，最后由原生调试器启动已有游戏 launcher。不会修改根目录启动器/运行库或插件列表。VS 应开启运行前构建，并将构建失败策略设为 **不启动**；重新部署前请退出游戏。普通构建不会部署。

`METAHOOKSV_GAME_DIRECTORY` 默认通过 Steam 自动查找，`METAHOOKSV_GAME_APPID` 默认为 `225840`。自定义 mod 使用 `METAHOOKSV_GAME_MOD`，附加参数使用 `METAHOOKSV_GAME_ARGUMENTS`；支持 Debug 和 Release。

共享模块依次从 `METAHOOKSV_LAUNCH_GAME_MODULE_DIR`、所在 MetaHookSv 聚合仓库或固定提交的源码包获取。缺少 Installer 源码时自动下载 GitHub `latest` 的自包含 CLI，无需安装 .NET；可用 `METAHOOKSV_INSTALLER_RELEASE` 固定 tag，或用 `METAHOOKSV_INSTALLER_CLI_EXECUTABLE` 指定离线 EXE。插件模式要求 v20261004c 或之后版本。`build/launch/launch-game/installer/<release>` 下的有效缓存直接复用，不自动升级；切换 tag 或清理该私有缓存后重新下载。首次下载若触发 GitHub API 限流，可通过环境变量 `GH_TOKEN`/`GITHUB_TOKEN` 提供凭据。功能默认 OFF，关闭时不新增下载。

## License

BetterSpray 采用 MIT License，见 `LICENSE`。随附的第三方源码保留各自的许可证文件。
