# BetterSpray

**BetterSpray** is a plugin for MetaHookSV that enhances Sven Co-op and GoldSrc’s spray system with support for high-res images, dynamic reloading and cloud sharing.

MetaHookSV: https://github.com/hzqst/MetaHookSv

## 🌟 Main Features

- ✅ Supports loading JPG/PNG/BMP/TGA/WEBP as spray texture, with alpha channel.
- ✅ Auto-convert your image into `/Sven Co-op/svencoop_downloads/custom_sprays/[STEAMID].jpg`. 
- ✅ Supports cloud sharing via Steam screenshot.
- ✅ Auto-convert your image into `/Sven Co-op/svencoop/tempdecal.wad` as a fallback solution, so that non-MetaHookSv users are able to see low-res version of your spray. (A game restart is required to refresh the cache of loaded `tempdecal.wad` if you has already been connected to a server)

### Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | x    |
| GoldSrc_legacy (< 6153)     | x    |
| GoldSrc_new    (8684 ~)     | x    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

### Cloud Sharing

You may fill the screenshot description with `!Spray` and share it as `Public` on Steam. In this case other guys with `BetterSprays.dll` installed will automatically download your sprays from Steam profile and be able to see jpeg version of your spray.

![](/img/00.png)

![](/img/01.png)

![](/img/02.png)

![](/img/03.png)

* Make sure the uploaded spray appears in `https://steamcommunity.com/profiles/7656************/screenshots/?appid=225840&sort=newestfirst&browsefilter=myfiles&view=grid`, where `7656************` is your STEAMID64, `225840` is [the AppId of Sven Co-op](https://steamdb.info/app/225840/)
