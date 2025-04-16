# BetterSpray

**BetterSpray** is a plugin for MetaHookSV that enhances Sven Co-op’s spray system with support for multiple images, true aspect ratios, and dynamic reloading.

![BetterSpray Preview](https://raw.githubusercontent.com/KazamiiSC/BetterSpray---Sven-Co-op/refs/heads/main/preview/20250416020128_1.jpg)

## 🌟 Main Features

### 🖼️ Spray Management
- ✅ Supports multiple sprays (PNG/JPG)
- ✅ Auto-detects images in `custom_sprays`
- ✅ Dynamic reload without restarting (`sprayreload`)
- ✅ Organized list display (`spraylist`)

### 🎨 Advanced Customization
- 🔧 Size adjustment (`sprayscale 5-60`)
- 🔄 Free rotation (`sprayrotate 0-360`)
- 💡 Brightness control (`spraybrightness 0.1-2.0`)
- ⏳ Duration: 60s + 2s fade out
- 📐 **Preserves original aspect ratio** (720x420, etc.)

### 🚀 Performance & Quality
- 🖌️ OpenGL rendering
- 🔍 Supports textures up to 1024x1024
- 💾 Memory optimization

## 📥 Installation

1. Copy `BetterSpray.dll` to:  
   `Sven Co-op/svencoop/metahook/plugins/`

2. Create folder:  
   `Sven Co-op/custom_sprays/`

3. Add images (PNG/JPG):  
spray1.png
my_logo.jpg
any_name.jpeg

## 🕹️ Play & Enjoy

| Command              | Description                          | Example               |
|----------------------|--------------------------------------|-----------------------|
| `spray`              | Places the current spray             | -                     |
| `spraynext`          | Switches to the next spray           | -                     |
| `spraynext name`     | Uses a specific spray                | `spraynext logo.png`  |
| `spraylist`          | Lists all available sprays           | -                     |
| `sprayreload`        | Reloads the spray folder             | -                     |

## ⚙️ Visual Settings

| Command              | Range       | Description               | Example              |
|----------------------|-------------|---------------------------|----------------------|
| `sprayscale`         | 5.0 - 60.0  | Vertical size             | `sprayscale 50`      |
| `sprayrotate`        | 0 - 360     | Rotation in degrees       | `sprayrotate 45`     |
| `spraybrightness`    | 0.1 - 2.0   | Spray brightness/intensity| `spraybrightness 1.5`|

## 📌 Important Notes

- ✨ **New**: Images now preserve their original aspect ratio!
- 📏 Max size: 1024x1024 pixels
- 🔄 Use `sprayreload` after adding new sprays
- 🎨 Supported formats: PNG, JPG, JPEG

### 🎯 Recommended Settings
sprayscale 35
spraybrightness 1.2

## 🚀 Roadmap

- Support for true aspect ratios  
- Dynamic spray reloading  
- Favorites system  
- Multiplayer support  
- Special effects (glow, borders)

## ❓ FAQ

**Q:** Why does my image look stretched?  
**A:** It shouldn’t anymore! The plugin now preserves original aspect ratios.

**Q:** How do I update the spray list?  
**A:** Use the `sprayreload` command after adding new images.

**Q:** What is the maximum image size allowed?  
**A:** The limit is 1024x1024 pixels.

---

🛠 Current Version: **3.1**  
❤️ Created by: **KZ-Sheez** Thanks to: Hzqst, Supah-R7 🕹️
