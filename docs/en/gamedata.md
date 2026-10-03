[Back to README](../../README.md) | [中文](../zh-CN/gamedata.md)

# gamedata

This page describes the private engine functions required by BetterSpray.
Build commands are in [Build instruction](build-instruction.md); deployment is in
[Installation](installation.md).

## Runtime requirements

Install the plugin catalog under `<mod>/metahook/gamedata/betterspray/` and update it
together with the DLL. MetaHook must merge nested catalogs and provide API 115 or
newer, at least the version of the SDK used to compile BetterSpray.
The plugin catalog supplements the host's primary catalog.

Both functions are required. Resolution uses the actual engine module through
`ResolveGameSymbol`. The returned address is used directly, without mirror-image
rebasing or a signature-scan fallback. A missing record, kind mismatch, catalog conflict
or null address reports the module, symbol, error and engine build and prevents hook installation.

## GameSymbols inventory

The declaration is [scripts/manifests/betterspray.json](../../scripts/manifests/betterspray.json).

| Module | GameSymbol | Kind | Use |
| --- | --- | --- | --- |
| `engine` | `GL_LoadTexture2` | `function` | Upload spray textures |
| `engine` | `Draw_DecalTexture` | `function` | Hook decal texture requests |

Initial Windows catalogs: `svencoop-8948`, `svencoop-10257`, `hl-10210`.
Each has both required records. Catalog identity follows engine binary CRC64;
a mod-directory name does not establish that a matching engine record exists.

The manifest has no optional records or scan fallback. Updating catalog coverage
does not by itself verify compatibility with another engine.

## Synchronization and validation

The build reuses VGUI2Extension's generic synchronization and validation scripts.
Synchronization retains the two Windows engine functions, downloads hash-verified
snapshots, rebuilds the index and validates the result.

```bat
cmake --build build/x86/Release --config Release --target BetterSprayGameDataValidate
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/betterspray --manifest scripts/manifests/betterspray.json
```

Use `--manifest` for this trimmed plugin catalog. `--full-catalog` checks requirements
for the complete host/consumer catalog and is not appropriate for this directory.
Raw snapshots are cached under `<build>/gamedata-sync/`.

Update the manifest, source lookups and this inventory together when consumption changes.
