[返回 README](../../README.zh-CN.md) | [English](../en/build-instruction.md)

# 构建说明

本文介绍 BetterSpray 的构建、依赖、gamedata 和回归测试。
部署方法见[安装说明](installation.md)，CI 流程见[自动化构建与发布](ci-cd.md)。

## 环境要求

- Windows、Visual Studio 2022 C++ 桌面开发工作负载、x86 MSVC 和 Windows SDK
- `PATH` 中可用的 CMake 3.21+、Git 和 Python 3
- 首次配置和 gamedata 同步时能够访问网络
- 打包与压缩包校验使用 7-Zip

## 构建

```bat
scripts\build-BetterSpray-x86-Debug.bat
scripts\build-BetterSpray-x86-Release.bat
```

两个入口均使用 `Visual Studio 17 2022 -A Win32`，依次完成配置、构建和安装。
可以从仓库外调用；未设置 `SolutionDir` 时根据脚本路径定位项目根目录。
任一步骤失败均返回非零退出码。构建目录为 `build/x86/<Debug|Release>`，
安装目录为 `install/x86/<Debug|Release>`，安装后手动部署到游戏。

## 依赖和源码路径

CMake 自动下载固定版本的 MetaHook SDK、VGUI2Extension、UtilThreadTask 和 UtilHTTPClient 接口源码、
SteamSDK、FreeImage、libxml2、ScopeExit 和 Chocobo1Hash。
版本记录在 [cmake/Dependencies.cmake](../../cmake/Dependencies.cmake)。
VC-LTL 5.3.1 使用 SHA-256 校验的二进制包，Debug 和 Release 共用缓存。

以下源码路径均为可选参数，可用于复用已有 checkout：

| 参数 | 所需内容 |
| --- | --- |
| `METAHOOK_SOURCE_PATH` | 含 `include/metahook.h`、HLSDK、SourceSDK 和 VGUI 源码的 MetaHook 根目录 |
| `VGUI2EXTENSION_SOURCE_PATH` | 含 `include/Interface/` 的 VGUI2Extension 根目录 |
| `UTILTHREADTASK_SOURCE_PATH` | 含 `include/Interface/IUtilThreadTask.h` 的 UtilThreadTask 根目录 |
| `UTILHTTPCLIENT_SOURCE_PATH` | 含 `include/Interface/IUtilHTTPClient.h` 的 UtilHTTPClient_libcurl 根目录，仅使用接口头文件 |
| `STEAMSDK_SOURCE_PATH` | 含 `steam/`、`lib/steam_api.lib`、`bin/steam_api.dll` 和许可声明的 SteamSDK 根目录 |
| `FREEIMAGE_SOURCE_PATH` | 含 CMake 工程与 `Source/FreeImage.h` 的 FreeImage 3.18.0 fork |
| `LIBXML2_SOURCE_PATH` | 官方 libxml2 2.14.2 CMake 源码目录 |
| `SCOPEEXIT_SOURCE_PATH` | 含 `include/ScopeExit/ScopeExit.h` 的 ScopeExit 根目录 |
| `CHOCOBO1HASH_SOURCE_PATH` | 含 `src/md5.h` 的 Chocobo1Hash 根目录 |
| `VC_LTL_Root` | 已解压的 VC-LTL 5.3.1 二进制包 |

示例：

```bat
scripts\build-BetterSpray-x86-Release.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DVGUI2EXTENSION_SOURCE_PATH=D:/VGUI2Extension" "-DUTILTHREADTASK_SOURCE_PATH=D:/UtilThreadTask" "-DSTEAMSDK_SOURCE_PATH=D:/SteamSDK"
```

首次配置时也可通过同名环境变量传入参数。使用 `-DNAME=value` 修改缓存值；
将源码路径设为空可恢复自动下载。相对路径以项目根目录为基准。
显式路径无效时会在下载前报错。外部 checkout 保持只读，库的构建产物位于 CMake 构建目录。

FreeImage 保留内置编解码器及原有 Debug/Release DLL 名称。
libxml2 保留 HTML、XPath、线程及内置编码支持，关闭 HTTP、iconv、ICU、zlib、
lzma、命令行工具、Python 绑定和第三方测试。HTTP 传输使用另行安装的 UtilHTTPClient 运行时。

## 构建选项

| 选项 | 默认值 | 用途 |
| --- | --- | --- |
| `BETTERSPRAY_BUILD_TESTS` | `OFF` | 构建 gamedata、图像库和 DLL 装载回归测试 |
| `BETTERSPRAY_SYNC_GAMEDATA` | `ON` | 同步、裁剪并验证插件目录 |
| `BETTERSPRAY_GAMEDATA_DIR` | `<build>/assets/svencoop/metahook/gamedata/betterspray` | gamedata 输出目录 |
| `BETTERSPRAY_DEPENDENCY_CACHE_DIR` | `thirdparty/cache` | VC-LTL 压缩包及解压缓存 |

## gamedata 和离线构建

启用同步时，构建会下载目录、保留 manifest 指定的 Windows 引擎记录，并在构建 DLL 前验证。
原始快照缓存在 `<build>/gamedata-sync/`。运行时要求见 [gamedata](gamedata.md)。

关闭同步时，仅安装已有目录；新的构建目录在此模式下不会提供 gamedata。
离线构建前应通过一次在线构建准备源码、VC-LTL 和 gamedata 缓存，
或提供本地源码路径、`VC_LTL_Root` 和已准备的 `BETTERSPRAY_GAMEDATA_DIR`。

## 回归测试

```bat
scripts\build-BetterSpray-x86-Release.bat -DBETTERSPRAY_BUILD_TESTS=ON
ctest --test-dir build/x86/Release -C Release --output-on-failure
scripts\build-BetterSpray-x86-Debug.bat -DBETTERSPRAY_BUILD_TESTS=ON
ctest --test-dir build/x86/Debug -C Debug --output-on-failure
python -m unittest discover -s scripts/tests -v
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/betterspray --manifest scripts/manifests/betterspray.json
```

Release 测试保留断言。覆盖主机 gamedata 调用和错误路径、PNG 透明通道/WebP 编解码、
UTF-8 HTML/XPath 解析，以及 DLL 的两个工厂接口装载。
编译与模拟测试不能证明游戏内喷漆、UI 或云分享行为已验证。
