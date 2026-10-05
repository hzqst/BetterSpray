[Back to README](../../README.md) | [中文](../zh-CN/ci-cd.md)

# Automated builds

- [LiveBuild](../../.github/workflows/livebuild.yml) builds Windows x86 Release on
  pushes to `main`, pull requests targeting `main`, and manual runs.
- [Build](../../.github/workflows/msbuild.yml) runs for `v*` tags and creates a
  `BetterSpray-<tag>` GitHub Release.

Both workflows use the shared
[build-windows-x86 action](../../.github/actions/build-windows-x86/action.yml).
It checks out a sibling MetaHook source tree from `main`, records its commit, and
passes the explicit SDK path to CMake. This overrides the auto-fetched SDK (latest `main`) used by local
builds without `METAHOOK_SOURCE_PATH`. Other dependencies retain their fixed versions.

The action runs the Release build script with regression tests enabled, Python
pruning tests, CTest and validation of the installed gamedata against the manifest.
It packages the installed `svencoop/` with 7-Zip and checks archive integrity.

The artifact and release file are `BetterSpray-windows-x86.7z`, containing DLLs, PDBs,
resources, the plugin gamedata and dependency notices. Public interface headers are
available in the install tree. Independent runtime components are installed separately.

Workflow definitions and local verification do not establish a successful remote run.
See [Build instruction](build-instruction.md) for local commands.
