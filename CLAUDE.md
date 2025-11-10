# CLAUDE.md - BetterSpray 项目指南

## 项目概述

**BetterSpray** 是一个用于 MetaHookSV 的插件，增强了 Sven Co-op 和 GoldSrc 引擎的喷漆系统，支持高分辨率图像、动态重载和云共享功能。

- **项目类型**: C++ 原生插件 (Windows DLL)
- **引擎**: GoldSrc / SvEngine / Half-Life 25
- **框架**: MetaHookSV Plugin API
- **主要依赖**: FreeImage, Steam API, libxml2, SQLite3

## 项目结构

```
BetterSpray/
├── src/                      # 源代码目录
│   ├── exportfuncs.cpp       # 核心功能：图像加载、转换、上传
│   ├── plugins.cpp           # 插件初始化和 MetaHook 接口
│   ├── privatehook.cpp       # 引擎函数钩子
│   ├── SprayDatabase.cpp     # 喷漆数据库管理（Steam 云同步）
│   ├── UtilHTTPClient.cpp    # HTTP 客户端（下载喷漆）
│   ├── UtilThreadTask.cpp    # 线程任务调度器
│   ├── BetterSprayDialog.cpp # VGUI2 对话框界面
│   ├── BetterSpraySettingsPage.cpp # 设置页面
│   ├── TaskListPanel.cpp     # 任务列表 UI 面板
│   └── wad3.hpp              # WAD3 文件格式定义
├── scripts/                  # 第三方库编译脚本
├── thirdparty/               # 第三方依赖库
├── MetaHookSv.props          # MSBuild 配置
└── Directory.build.props     # MSBuild 构建属性

```

## 核心功能模块

### 1. 图像加载与转换 (`exportfuncs.cpp`)

**关键函数**:
- `Draw_LoadSprayTexture()`: 从文件系统加载喷漆纹理
- `Draw_LoadSprayTexture_ConvertToBGRA32()`: 转换图像到 BGRA32 格式
- `Draw_LoadSprayTexture_BGRA8ToRGBA8()`: BGRA 到 RGBA 转换
- `Draw_UploadSprayTextureRGBA8()`: 上传 RGBA8 纹理到 OpenGL
- `BS_SaveBitmapToTempDecal()`: 保存位图到 tempdecal.wad

**图像格式支持**:
- 输入: JPG, PNG, BMP, TGA, WEBP (通过 FreeImage)
- 输出: JPEG (Steam 云共享), WAD3 (向后兼容)
- 透明通道处理: 支持 Alpha 通道，自动检测和转换

**特殊格式处理**:
- **双倍宽度格式**: 宽度 = 高度 * 2，左半部分为 RGB，右半部分为 Alpha
- **带背景标记格式**: 特定分辨率 (5160x2160, 3440x1440, 2560x1080) 配合角落像素颜色标记
  - 红色角落: 中心裁剪
  - 绿色角落: 左上角裁剪
  - 蓝色角落: 右上角裁剪
  - 对角不同颜色: 启用 Alpha 通道

### 2. 喷漆数据库 (`SprayDatabase.cpp`)

**功能**:
- Steam 截图云同步
- 自动下载其他玩家的喷漆
- 查询状态管理 (Unknown, Querying, Finished, Failed)
- 本地缓存管理

**数据流**:
1. 检测玩家喷漆 → 查询 Steam 截图 API
2. 下载 `!Spray` 标记的截图
3. 保存到 `custom_sprays/{SteamID}.jpg`
4. 加载并上传到游戏引擎

### 3. HTTP 客户端 (`UtilHTTPClient.cpp`)

使用 libxml2 的 HTTP 功能下载喷漆图像。

### 4. 线程任务系统 (`UtilThreadTask.cpp`)

- 异步图像加载和转换
- 游戏线程任务队列
- 工作线程池集成

### 5. 引擎钩子 (`privatehook.cpp`)

- `Draw_DecalTexture()`: 拦截喷漆纹理请求
- `GL_LoadTexture2()`: OpenGL 纹理加载
- 文件系统 API 钩子

## 关键代码流程

### 喷漆上传流程

```
用户选择图像
    ↓
BS_UploadSprayBitmap()
    ↓
[可选] 归一化为正方形 → BS_NormalizeToSquare*()
    ↓
[可选] 添加随机背景 → BS_CreateBackgroundRGBA24()
    ↓
保存为 {SteamID}.jpg → GAMEDOWNLOAD/custom_sprays/
    ↓
保存为 tempdecal.wad (256色量化)
    ↓
上传到 Steam 截图库
    ↓
[如果在游戏中] 立即加载到引擎
```

### 喷漆加载流程

```
Draw_DecalTexture() [钩子] 引擎请求贴图 (~playerindex)
    ↓
检查本地缓存 custom_sprays/{SteamID}.jpg
    ↓
[找到 / 未找到] → SprayDatabase::QueryPlayerSpray() → 从对应玩家的 Steam Profile 下载 → 下载截图 → 保存至 custom_sprays/{SteamID}.jpg ↓
    ↓
Draw_LoadSprayTexture 调用FreeImage 从custom_sprays打开对应的jpg文件 <---------------------------------------------------------------------------------------------------
    ↓
在工作线程上启动WorkItem，在WorkItem中转换jpg至RGBA8以便后续上传至GPU
    ↓
通过 GameThreadTaskScheduler 发送任务给主线程，在主线程上传至OpenGL
```

## 编译说明

### 依赖项
- **MetaHookSV SDK**: 由 `$(SolutionDir)Directory.build.props` 自动加载 `$(SolutionDir)MetaHookSv.props`
- **FreeImage**: 图像处理库
- **Steam API**: 截图上传和查询
- **libxml2**: HTTP 请求
- **SQLite3**: 数据库存储
- **zlib, liblzma, libiconv**: libxml2 的依赖项

### 构建脚本
位于 `scripts/` 目录，用于编译第三方库:
- `build-sqlite3-x86-*.bat`
- `build-libxml2-x86-*.bat`
- `build-zlib-x86-*.bat`
- 等等

### 配置
- 平台: Win32 (x86)
- 配置: Debug / Release
- 输出: BetterSpray.dll

## 引擎兼容性

| 引擎版本 | 支持状态 |
|---------|---------|
| GoldSrc_blob (3248~4554) | ❌ |
| GoldSrc_legacy (< 6153) | ❌ |
| GoldSrc_new (8684 ~) | ❌ |
| SvEngine (8832 ~) | ✅ |
| GoldSrc_HL25 (>= 9884) | ✅ |

## 重要常量和宏

```cpp
#define CUSTOM_SPRAY_DIRECTORY "custom_sprays"
#define MAX_CLIENTS 32

// 文件系统宏
FILESYSTEM_ANY_OPEN(path, mode, pathId)
FILESYSTEM_ANY_READ(buffer, size, handle)
FILESYSTEM_ANY_WRITE(buffer, size, handle)
FILESYSTEM_ANY_SEEK(handle, offset, origin)
FILESYSTEM_ANY_TELL(handle)
FILESYSTEM_ANY_CLOSE(handle)
FILESYSTEM_ANY_CREATEDIR(path, pathId)
FILESYSTEM_ANY_GETLOCALPATH(path, buffer, size)
```

## 调试技巧

1. **控制台命令**: 使用 `gEngfuncs.Con_Printf()` / `Con_DPrintf()` 输出日志
2. **断点位置**:
   - `Draw_DecalTexture()`: 喷漆纹理请求
   - `BS_UploadSprayBitmap()`: 图像上传
   - `Draw_LoadSprayTexture_WorkItem()`: 异步加载
3. **查看图像**:
   - 输出路径: `svencoop_downloads/custom_sprays/`
   - 临时 WAD: `svencoop/tempdecal.wad`

## 常见问题

### Q: 为什么需要 tempdecal.wad？
A: 向后兼容。非 MetaHookSv 用户只能看到 256 色 WAD 版本的喷漆。

### Q: Alpha 通道如何处理？
A: 
- 对于双倍宽度图像：右半部分作为 Alpha 蒙版
- 对于 32bpp 图像：直接读取 Alpha 通道
- 低于 125 的 Alpha 值在 WAD 转换时变为透明 (蓝色 255)

### Q: Steam 云同步如何工作？
A: 
1. 用户上传截图到 Steam 库，描述填写 `!Spray`
2. 其他玩家的插件查询对应用户的 Steam Profile Screenshot 页面并查找描述以`!`开头的Screenshot
3. 自动下载对应Screenshot的完整版图片并缓存至本地

### Q: 图像尺寸限制？
A: 
- WAD版本限制: ~14336 像素总面积 (建议 256x256 或更小)
- jpeg云共享: 建议使用正方形图像以获得最佳兼容性

## 文件命名约定

- jpeg版本喷漆文件: `{SteamID64}.jpg`
- WAD版本喷漆: `tempdecal.wad`
- 随机背景: `bettersprays/random_background_{0-N}.jpg`

## 性能考虑

1. **异步加载**: 图像转换在工作线程执行，避免阻塞游戏
2. **纹理上传**: 最终上传在游戏线程执行（OpenGL 要求）
3. **缓存策略**: 已加载的喷漆保存在内存中
4. **查询限制**: 每个玩家的 Steam API 查询只执行一次

## 扩展点

如需修改或扩展功能，关注以下区域：

1. **添加新图像格式**: 修改 `Draw_LoadSprayTexture()`
2. **修改转换逻辑**: 编辑 `Draw_LoadSprayTexture_ConvertToBGRA32()`
3. **云存储后端**: 扩展 `SprayDatabase.cpp` 支持其他 API
4. **UI 定制**: 修改 `BetterSprayDialog.cpp` 和相关 VGUI2 代码

## 代码风格

- **命名约定**: 
  - 函数: `Draw_LoadSprayTexture()`, `BS_SaveBitmapToTempDecal()`
  - 类: `CLoadSprayTextureWorkItemContext`
  - 宏: `FILESYSTEM_ANY_OPEN`
- **错误处理**: 返回状态码，通过 Console 输出错误信息
- **资源管理**: 使用 `SCOPE_EXIT` 宏确保资源释放

## 相关链接

- **MetaHookSV**: https://github.com/hzqst/MetaHookSv
- **FreeImage**: http://freeimage.sourceforge.net/
- **Steam API**: https://partner.steamgames.com/doc/api

## 更新日志

维护此文件时，请记录重要的架构更改：

- 2024: 初始版本，支持基本喷漆加载和 Steam 云同步