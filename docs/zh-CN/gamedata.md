[返回 README](../../README.zh-CN.md) | [English](../en/gamedata.md)

# gamedata

本文说明 BetterSpray 所需的私有引擎函数。
构建命令见[构建说明](build-instruction.md)，部署方法见[安装说明](installation.md)。

## 运行时要求

插件目录安装到 `<mod>/metahook/gamedata/betterspray/`，并与 DLL 一起更新。
MetaHook 必须支持合并嵌套目录，并提供 API 115 或更新版本，
且 API 至少达到编译 BetterSpray 所用 SDK 的版本。插件目录补充主机的主目录。

两个函数均为必需记录。通过实际引擎模块调用 `ResolveGameSymbol`，
直接使用返回地址；不再进行镜像地址换算或签名扫描回退。
记录缺失、类型不匹配、目录冲突或空地址都会报告模块、符号、错误和引擎版本，并阻止安装 hook。

## GameSymbols 清单

声明位于 [scripts/manifests/betterspray.json](../../scripts/manifests/betterspray.json)。

| 模块 | GameSymbol | 类型 | 用途 |
| --- | --- | --- | --- |
| `engine` | `GL_LoadTexture2` | `function` | 上传喷漆纹理 |
| `engine` | `Draw_DecalTexture` | `function` | 挂钩贴花纹理请求 |

初始 Windows 目录覆盖 `svencoop-8948`、`svencoop-10257` 和 `hl-10210`，
每个版本均包含两个必需记录。目录通过引擎二进制 CRC64 识别模块，
不能根据 mod 目录名称认定存在匹配的引擎记录。

manifest 没有可选记录或扫描回退。扩大目录覆盖范围本身不能证明其他引擎的兼容性。

## 同步与验证

构建复用 VGUI2Extension 的通用同步和验证脚本。
同步保留两个 Windows 引擎函数，下载经过哈希校验的快照，重新生成索引，并验证结果。

```bat
cmake --build build/x86/Release --config Release --target BetterSprayGameDataValidate
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/betterspray --manifest scripts/manifests/betterspray.json
```

对裁剪后的插件目录使用 `--manifest`。完整主机及消费者目录的
`--full-catalog` 检查不适用于此目录。原始快照缓存在 `<build>/gamedata-sync/`。

消费需求变化时，应一起更新 manifest、源码查询和本清单。
