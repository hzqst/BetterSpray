[返回 README](../../README.zh-CN.md) | [English](../en/ci-cd.md)

# 自动化构建与发布

- [LiveBuild](../../.github/workflows/livebuild.yml) 在推送到 `main`、
  向 `main` 发起 PR 和手动运行时构建 Windows x86 Release。
- [Build](../../.github/workflows/msbuild.yml) 在推送 `v*` tag 时运行，
  创建 `BetterSpray-<tag>` GitHub Release。

两个 workflow 共用
[build-windows-x86 action](../../.github/actions/build-windows-x86/action.yml)。
它从 `main` 检出相邻 MetaHook 源码、记录实际提交，并将 SDK 路径传入 CMake。
该显式路径覆盖本地未设置 `METAHOOK_SOURCE_PATH` 时使用的固定 SDK；
其他依赖仍使用固定版本。

action 启用回归测试运行 Release 构建脚本，执行 Python 裁剪测试、CTest，
并按 manifest 验证安装后的 gamedata。使用 7-Zip 打包安装目录中的 `svencoop/`，
随后检查压缩包完整性。

artifact 和发布文件均为 `BetterSpray-windows-x86.7z`，
包含 DLL、PDB、资源、插件 gamedata 和依赖许可声明。
公共接口头文件位于 install 目录，独立运行时组件需另行安装。

workflow 定义和本地验证不代表远程运行已经成功。
本地命令见[构建说明](build-instruction.md)。
