# BetterSpray 

**BetterSpray** is a MetaHookSV plugin that enhances spray functionality in Sven Co-op with multi-spray support, customization options, and improved visual effects.

## 🌟 Features

### 🖼️ Spray Management
- Multiple spray support (PNG/JPG)
- Automatic loading from `custom_sprays` folder
- `spraylist` command to view available sprays
- `spraynext` to cycle through sprays (or specify by name)

### 🎨 Customization
- Adjustable size (`sprayscale 5-200`)
- Rotation control (`sprayrotate 0-360`)
- Brightness control (`spraybrightness 0.1-2.0`)
- 60-second duration with 5-second fadeout animation

### 🎮 Game Integration
- Classic `impulse 201` support
- Console command `spray`
- Surface projection with normal alignment
- Proper depth testing and blending

## 📥 Installation

1. **Install the plugin**:
   - Copy `BetterSpray.dll` to:
     ```
     Sven Co-op/svencoop/metahook/plugins/
     ```

2. **Prepare your sprays**:
   - Create folder:
     ```
     Sven Co-op/custom_sprays/
     ```
   - Add spray files with naming pattern:
     ```
     spray*.png (or .jpg)
     ```
     Example names:
     ```
     spray1.png
     spray_awesome.jpg
     ```

3. **Launch the game**

## 🕹️ Usage

### Basic Commands
| Command | Description |
|---------|-------------|
| `spray` | Places current spray |
| `spraynext` | Cycles to next spray |
| `spraynext name` | Loads specific spray |
| `spraylist` | Shows available sprays |

### Customization Commands
| Command | Example | Description |
|---------|---------|-------------|
| `sprayscale` | `sprayscale 60` | Set spray size (5-200) |
| `sprayrotate` | `sprayrotate 45` | Set rotation in degrees |
| `spraybrightness` | `spraybrightness 1.5` | Adjust brightness (0.1-2.0) |

---
⚙️ Technical Details
Max Resolution: 512x512 (recommended)
Formats: PNG (with alpha), JPG
Memory: Auto-cleanup on map change
Performance: Single draw call per spray

🚀 Roadmap
Spray favorites system
Animated sprays (GIF/APNG)
Multiplayer synchronization
3D projection effects
Steam Workshop integration

❓ FAQ
Q: Why can't others see my sprays?
A: This is currently client-side only. Multiplayer sync is planned for v2.0.

Q: Can I use transparent sprays?
A: Yes! PNGs with alpha transparency work perfectly.

Q: How many sprays can I have?
A: Unlimited! The plugin loads them dynamically.


Created with ❤️ by KZ-Sheez (KazamiiSC)❤️
