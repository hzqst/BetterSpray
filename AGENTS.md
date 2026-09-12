# AGENTS.md - BetterSpray Project Guide

## Project Overview

**BetterSpray** is a plugin for MetaHookSV that enhances the spray system of Sven Co-op and the GoldSrc engine, supporting high-resolution images, dynamic reloading, and cloud sharing.

- **Project type**: Native C++ plugin (Windows DLL)
- **Engine**: GoldSrc / SvEngine / Half-Life 25
- **Framework**: MetaHookSV Plugin API
- **Main dependencies**: FreeImage, Steam API, libxml2, SQLite3

## Project Structure

```
BetterSpray/
├── src/                      # Source code
│   ├── exportfuncs.cpp       # Core: image loading, conversion, upload
│   ├── plugins.cpp           # Plugin initialization and MetaHook interfaces
│   ├── privatehook.cpp       # Engine function hooks
│   ├── SprayDatabase.cpp     # Spray database management (Steam cloud sync)
│   ├── UtilHTTPClient.cpp    # HTTP client (spray downloads)
│   ├── UtilThreadTask.cpp    # Threaded task scheduler
│   ├── BetterSprayDialog.cpp # VGUI2 dialog UI
│   ├── BetterSpraySettingsPage.cpp # Settings page
│   ├── TaskListPanel.cpp     # Task list UI panel
│   └── wad3.hpp              # WAD3 file format definitions
├── scripts/                  # Third-party library build scripts
├── thirdparty/               # Third-party dependencies
├── MetaHookSv.props          # MSBuild configuration
└── Directory.build.props     # MSBuild build properties

```

## Core Modules

### 1. Image Loading and Conversion (`exportfuncs.cpp`)

**Key functions**:
- `Draw_LoadSprayTexture()`: Loads a spray texture from the file system
- `Draw_LoadSprayTexture_ConvertToBGRA32()`: Converts an image to BGRA32
- `Draw_LoadSprayTexture_BGRA8ToRGBA8()`: BGRA to RGBA conversion
- `Draw_UploadSprayTextureRGBA8()`: Uploads an RGBA8 texture to OpenGL
- `BS_SaveBitmapToTempDecal()`: Saves a bitmap to tempdecal.wad

**Supported image formats**:
- Input: JPG, PNG, BMP, TGA, WEBP (via FreeImage)
- Output: JPEG (Steam cloud sharing), WAD3 (backward compatibility)
- Alpha handling: Supports alpha channels with automatic detection and conversion

**Special format handling**:
- **Double-width format**: width = height * 2, left half is RGB, right half is alpha
- **Background-marker format**: specific resolutions (5160x2160, 3440x1440, 2560x1080) combined with a corner-pixel color marker
  - Red corner: center crop
  - Green corner: top-left crop
  - Blue corner: top-right crop
  - Different colors on opposite corners: enable the alpha channel

### 2. Spray Database (`SprayDatabase.cpp`)

**Features**:
- Steam screenshot cloud sync
- Automatic download of other players' sprays
- Query state management (Unknown, Querying, Finished, Failed)
- Local cache management

**Data flow**:
1. Detect a player's spray → query the Steam screenshot API
2. Download the screenshot tagged `!Spray`
3. Save to `custom_sprays/{SteamID}.jpg`
4. Load and upload to the game engine

### 3. HTTP Client (`UtilHTTPClient.cpp`)

Uses libxml2's HTTP facilities to download spray images.

### 4. Threaded Task System (`UtilThreadTask.cpp`)

- Asynchronous image loading and conversion
- Game-thread task queue
- Worker thread pool integration

### 5. Engine Hooks (`privatehook.cpp`)

- `Draw_DecalTexture()`: Intercepts spray texture requests
- `GL_LoadTexture2()`: OpenGL texture loading
- File system API hooks

## Key Code Flows

### Spray Upload Flow

```
User selects an image
    ↓
BS_UploadSprayBitmap()
    ↓
[optional] Normalize to square → BS_NormalizeToSquare*()
    ↓
[optional] Add random background → BS_CreateBackgroundRGBA24()
    ↓
Save as {SteamID}.jpg → GAMEDOWNLOAD/custom_sprays/
    ↓
Save as tempdecal.wad (256-color quantization)
    ↓
Upload to the Steam screenshot library
    ↓
[if in game] Load into the engine immediately
```

### Spray Load Flow

```
Draw_DecalTexture() [hook] engine requests a decal texture (~playerindex)
    ↓
Check local cache custom_sprays/{SteamID}.jpg
    ↓
[found / not found] → SprayDatabase::QueryPlayerSpray() → download from that player's Steam profile → download screenshot → save to custom_sprays/{SteamID}.jpg ↓
    ↓
Draw_LoadSprayTexture calls FreeImage to open the corresponding jpg under custom_sprays <---------------------------------------------------------------------------------------------------
    ↓
Start a WorkItem on a worker thread; inside the WorkItem convert the jpg to RGBA8 for later upload to the GPU
    ↓
Send a task to the main thread via GameThreadTaskScheduler, and upload to OpenGL on the main thread
```

## Build Instructions

### Dependencies
- **MetaHookSV SDK**: `$(SolutionDir)Directory.build.props` automatically loads `$(SolutionDir)MetaHookSv.props`
- **FreeImage**: Image processing library
- **Steam API**: Screenshot upload and queries
- **libxml2**: HTTP requests
- **SQLite3**: Database storage
- **zlib, liblzma, libiconv**: libxml2 dependencies

### Build Scripts
Located in `scripts/`, used to build third-party libraries:
- `build-sqlite3-x86-*.bat`
- `build-libxml2-x86-*.bat`
- `build-zlib-x86-*.bat`
- and so on

### Configuration
- Platform: Win32 (x86)
- Configuration: Debug / Release
- Output: BetterSpray.dll

## Engine Compatibility

| Engine version | Support |
|---------|---------|
| GoldSrc_blob (3248~4554) | ❌ |
| GoldSrc_legacy (< 6153) | ❌ |
| GoldSrc_new (8684 ~) | ❌ |
| SvEngine (8832 ~) | ✅ |
| GoldSrc_HL25 (>= 9884) | ✅ |

## Important Constants and Macros

```cpp
#define CUSTOM_SPRAY_DIRECTORY "custom_sprays"
#define MAX_CLIENTS 32

// File system macros
FILESYSTEM_ANY_OPEN(path, mode, pathId)
FILESYSTEM_ANY_READ(buffer, size, handle)
FILESYSTEM_ANY_WRITE(buffer, size, handle)
FILESYSTEM_ANY_SEEK(handle, offset, origin)
FILESYSTEM_ANY_TELL(handle)
FILESYSTEM_ANY_CLOSE(handle)
FILESYSTEM_ANY_CREATEDIR(path, pathId)
FILESYSTEM_ANY_GETLOCALPATH(path, buffer, size)
```

## Debugging Tips

1. **Console output**: Use `gEngfuncs.Con_Printf()` / `Con_DPrintf()` for logging
2. **Breakpoint locations**:
   - `Draw_DecalTexture()`: Spray texture request
   - `BS_UploadSprayBitmap()`: Image upload
   - `Draw_LoadSprayTexture_WorkItem()`: Asynchronous loading
3. **Inspecting images**:
   - Output path: `svencoop_downloads/custom_sprays/`
   - Temporary WAD: `svencoop/tempdecal.wad`

## FAQ

### Q: Why is tempdecal.wad needed?
A: Backward compatibility. Users without MetaHookSv can only see the 256-color WAD version of a spray.

### Q: How is the alpha channel handled?
A:
- For double-width images: the right half acts as the alpha mask
- For 32bpp images: the alpha channel is read directly
- Alpha values below 125 become transparent (blue 255) when converting to WAD

### Q: How does Steam cloud sync work?
A:
1. The user uploads a screenshot to their Steam library and sets the description to `!Spray`
2. Other players' plugins query that user's Steam profile screenshot page and look for a screenshot whose description starts with `!`
3. The full-size version of that screenshot is downloaded automatically and cached locally

### Q: What are the image size limits?
A:
- WAD version limit: ~14336 total pixels (256x256 or smaller is recommended)
- JPEG cloud sharing: square images are recommended for best compatibility

## File Naming Conventions

- JPEG spray file: `{SteamID64}.jpg`
- WAD spray: `tempdecal.wad`
- Random background: `bettersprays/random_background_{0-N}.jpg`

## Performance Considerations

1. **Asynchronous loading**: Image conversion runs on worker threads to avoid blocking the game
2. **Texture upload**: The final upload runs on the game thread (an OpenGL requirement)
3. **Caching strategy**: Loaded sprays are kept in memory
4. **Query limiting**: The Steam API is queried only once per player

## Extension Points

To modify or extend functionality, focus on these areas:

1. **Add a new image format**: modify `Draw_LoadSprayTexture()`
2. **Change conversion logic**: edit `Draw_LoadSprayTexture_ConvertToBGRA32()`
3. **Cloud storage backend**: extend `SprayDatabase.cpp` to support other APIs
4. **UI customization**: modify `BetterSprayDialog.cpp` and the related VGUI2 code

## Code Style

- **Naming conventions**:
  - Functions: `Draw_LoadSprayTexture()`, `BS_SaveBitmapToTempDecal()`
  - Classes: `CLoadSprayTextureWorkItemContext`
  - Macros: `FILESYSTEM_ANY_OPEN`
- **Error handling**: Return status codes and print error messages to the console
- **Resource management**: Use the `SCOPE_EXIT` macro to guarantee resource release

## Related Links

- **MetaHookSV**: https://github.com/hzqst/MetaHookSv
- **FreeImage**: http://freeimage.sourceforge.net/
- **Steam API**: https://partner.steamgames.com/doc/api
