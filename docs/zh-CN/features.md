[返回 README](../../README.zh-CN.md) | [English](../en/features.md)

# 功能说明

BetterSpray 增强喷漆加载、转换和分享。
运行时依赖与启用方法见[安装说明](installation.md)。

## 图片与本地喷漆

- 加载 JPG、PNG、BMP、TGA 和 WEBP 图片，包括透明通道。
- 将选中的图片转换为引擎 `GAMEDOWNLOAD` 目录下的
  `custom_sprays/<SteamID64>.jpg`；Sven Co-op 通常对应 `svencoop_downloads/`。
- 在工作线程解码图片，在游戏线程上传纹理。
- 支持双倍宽度图片：左半边为 RGB，右半边为透明度掩码。
- 支持图片归一化及通过插件背景标记格式编码透明度。
- 写入 `tempdecal.wad`，供未安装 BetterSpray 的玩家查看。
  WAD 使用缩减后的调色板，低于 125 的透明度转换为透明像素；
  已加载 WAD 缓存时可能需要重启游戏刷新。

集成设置和任务列表 UI 需要 VGUI2Extension。
README 沿用项目已有的引擎兼容表，随包目录的实际覆盖范围见 [gamedata](gamedata.md)。

## Steam 云分享

1. 通过插件选择并上传喷漆。
2. 在 Steam 截图库中，将描述设为 `!Spray`，并以 Public 公开分享。
3. 确认它出现在 Sven Co-op 截图页面：
   `https://steamcommunity.com/profiles/<SteamID64>/screenshots/?appid=225840&sort=newestfirst&browsefilter=myfiles&view=grid`。
4. 安装 BetterSpray 及其运行时依赖的其他玩家可以查询并下载该喷漆。

下载器选择描述以 `!` 开头的截图，推荐使用 `!Spray`。
正常使用时，本地缓存可减少重复下载。

![喷漆上传](../images/00.png)

![Steam 截图库](../images/01.png)

![截图分享](../images/02.png)

![公开截图描述](../images/03.png)
