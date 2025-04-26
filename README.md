# BetterSpray

**BetterSpray** is a plugin for MetaHookSV that enhances Sven Co-op and GoldSrc’s spray system with support
for multiple images, true aspect ratios, and dynamic reloading.

MetaHookSV: https://github.com/hzqst/MetaHookSv

![BetterSpray Preview](https://raw.githubusercontent.com/KazamiiSC/BetterSpray-Sven-Coop/refs/heads/main/preview/render3.png)

## 🌟 Main Features

### 🖼️ Spray Management
- ✅ Supports loading JPG/PNG/BMP/TGA/WEBP as spray texture, with alpha channel.
- ✅ Auto-convert your image into .jpg in `/Sven Co-op/svencoop_downloads/custom_sprays/`, and upload to Steam profile. 
- ✅ You may need to fill the screenshot description with "!Spray" and click the "Share" by yourself. In this case other guys with `BetterSprays.dll` installed will automatically download your sprays from Steam profile.
- ✅ Auto-convert your image into `tempdecal.wad` in `/Sven Co-op/svencoop/` as a fallback for non-MetaHookSv users. (A game restart is required to refresh the cache of loaded `tempdecal.wad` if you has already been connected to a server)

### 🚀 Performance & Quality
- 🖌️ Vanilla decal rendering by engine itself.
- 🔍 Has no limitation on texture size.

### Compatibility

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | x    |
| GoldSrc_legacy (< 6153)     | x    |
| GoldSrc_new    (8684 ~)     | x    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |

## 📥 Installation

[**DOWNLOAD:BetterSpray**](https://github.com/KazamiiSC/BetterSpray-Sven-Coop/releases/download/BetterSpray/BetterSpray.Plugin.rar)

Simply download it from the Releases section and place it in your main Sven Co-op folder.
Copy and replace SDL2.dll, SDL3.dll, and svencoop.exe in:
Steam/steamapps/common/Sven Co-op
(Make a backup of your svencoop.exe first)
Or follow these steps manually:

1. Copy `/Build/metahook` to `/Sven Co-op/svencoop/metahook/`

2. Add `BetterSpray.dll` in `/Sven Co-op/svencoop/metahook/configs/plugins.lst`

3. Copy `/Build/bettersprays` to `/Sven Co-op/svencoop/bettersprays/`

4. Run game, click "Better Sprays" in the main menu.

5. Click "Load" button and load whatever image you want.

 Note on Antivirus False Positives
Some antivirus software like Avast may flag this plugin (or the game executable) as "Win32:MalwareX-gen [Misc]". 
This is a false positive caused by the plugin modifying and extending the game's behavior 
using techniques like hooking or custom rendering (just like any advanced mod).

👉 The plugin does NOT contain any viruses or malicious software.
If you'd like to verify, you can upload the file to VirusTotal to confirm it's a heuristic detection, not a real threat.

If your antivirus blocks it, we recommend adding the game folder to 
the exceptions list or temporarily disabling the scan while you play.

Thanks for playing and supporting Sven Co-op modding!

---

🛠 Current Version: **1.4**  
❤️ Created by: **KZ-Sheez** Thanks to: Hzqst, Supah-R7 🕹️
