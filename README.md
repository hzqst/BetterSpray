# BetterSpray

**BetterSpray** is a plugin for MetaHookSV that enhances Sven Co-op and GoldSrc’s spray system with support for high-res images, dynamic reloading and cloud sharing.

MetaHookSV: https://github.com/hzqst/MetaHookSv

## 🌟 Main Features

### 🖼️ Spray Management
- ✅ Supports loading JPG/PNG/BMP/TGA/WEBP as spray texture, with alpha channel.
- ✅ Auto-convert your image into .jpg in `/Sven Co-op/svencoop_downloads/custom_sprays/`, and upload to Steam profile. 
- ✅ You may need to fill the screenshot description with "!Spray" and click the "Share" by yourself. In this case other guys with `BetterSprays.dll` installed will automatically download your sprays from Steam profile.
- ✅ Auto-convert your image into `tempdecal.wad` in `/Sven Co-op/svencoop/` as a fallback for non-MetaHookSv users. (A game restart is required to refresh the cache of loaded `tempdecal.wad` if you has already been connected to a server)

### Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | x    |
| GoldSrc_legacy (< 6153)     | x    |
| GoldSrc_new    (8684 ~)     | x    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |