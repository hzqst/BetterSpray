[返回 README](../../README.zh-CN.md) | [English](../en/installation.md)

# 安装说明

本地构建见[构建说明](build-instruction.md)，图片选择和 Steam 截图分享见[功能说明](features.md)。

## 安装布局

`install/x86/<Debug|Release>/`：

```text
svencoop/
  metahook/plugins/BetterSpray.dll
  metahook/plugins/BetterSpray.pdb
  metahook/dlls/libxml2.dll
  metahook/dlls/libxml2.pdb
  metahook/dlls/FreeImage/FreeImage.dll
  metahook/dlls/FreeImage/FreeImage.pdb
  metahook/gamedata/betterspray/
  bettersprays/
include/Interface/
  ISprayDatabase.h
```

Debug 使用 `FreeImaged.dll` 和 `FreeImaged.pdb`。
运行时压缩包包含 `svencoop/`；开发用公共接口头文件位于 install 目录。

## 运行时依赖

- MetaHook API 115 或更新版本，且 API 版本至少达到编译插件所用版本；
  支持合并嵌套 gamedata 目录，并保留主机自己的主目录。
- [UtilThreadTask](https://github.com/MetaHookSv/UtilThreadTask)：将
  `UtilThreadTask.dll` 安装到 `<mod>/metahook/dlls/`。
- HTTP 客户端：根据对应组件说明安装
  [UtilHTTPClient_libcurl](https://github.com/MetaHookSv/UtilHTTPClient_libcurl) 及其 libcurl 运行时，
  或 [UtilHTTPClient_SteamAPI](https://github.com/MetaHookSv/UtilHTTPClient_SteamAPI)。
  BetterSpray 优先尝试 libcurl。
- [VGUI2Extension](https://github.com/MetaHookSv/VGUI2Extension) 提供集成设置和任务列表 UI；
  在 `plugins.lst` 中启用该 DLL。未安装时会跳过 UI 回调。
- 兼容的 x86 `steam_api.dll`，通常由游戏或 MetaHook 安装提供；
  匹配的运行时也可从 [SteamSDK](https://github.com/MetaHookSv/SteamSDK) 获取。

上述独立运行时需分别安装；FreeImage 和 libxml2 随 BetterSpray 发布包提供。

## 启用插件

1. 安装以上运行时依赖。
2. 将压缩包中的 `svencoop/` 合并到目标 mod 目录。
3. 将 `metahook/gamedata/betterspray/` 与匹配的 DLL 和资源一起部署。
4. 在 `<mod>/metahook/configs/plugins.lst` 中加入 `BetterSpray.dll`。
5. 通过 MetaHook 启动游戏。
