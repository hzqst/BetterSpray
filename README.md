BetterSpray
BetterSpray is a plugin for MetaHookSV that enhances Sven Co-op's spray system with support for multiple images, true aspect ratios, and dynamic reloading.

🌟 Main Features
🖼️ Spray Management
✅ Supports multiple sprays (PNG/JPG)

✅ Automatically detects images in custom_sprays

✅ Dynamic reload without restarting (sprayreload)

✅ Organized listing (spraylist)

🎨 Advanced Customization
🔧 Size adjustment (sprayscale 5-60)

🔄 Free rotation (sprayrotate 0-360)

💡 Brightness control (spraybrightness 0.1-2.0)

⏳ Duration: 60s + 2s fade out

📐 Maintains original aspect ratio (720x420, etc.)

🚀 Performance & Quality
🖌️ OpenGL rendering

🔍 Textures up to 1024x1024

💾 Memory optimization

📥 Installation
Copy BetterSpray.dll to: Sven Co-op/svencoop/metahook/plugins/

Create folder: Sven Co-op/custom_sprays/

Add images (PNG/JPG): spray1.png
my_logo.jpg
any_name.jpeg

🕹️ Play & Enjoy

Command	Description	Example
spray	Places the current spray	-
spraynext	Switches to the next spray	-
spraynext name	Uses a specific spray	spraynext logo.png
spraylist	Shows all available sprays	-
sprayreload	Reloads the spray folder	-
⚙️ Visual Settings

Command	Range	Description	Example
sprayscale	5.0 - 60.0	Vertical size	sprayscale 50
sprayrotate	0 - 360	Rotation in degrees	sprayrotate 45
spraybrightness	0.1 - 2.0	Spray intensity	spraybrightness 1.5
📌 Important Notes
✨ New: Images now preserve their original aspect ratio!

📏 Max size: 1024x1024 pixels

🔄 Use sprayreload after adding new sprays

🎨 Supported formats: PNG, JPG, JPEG

// Recommended settings
sprayscale 35
spraybrightness 1.2

🚀 Roadmap

Support for true aspect ratios

Dynamic spray reloading

Favorites system

Multiplayer support

Special effects (glow, borders)

❓ Frequently Asked Questions
Q: Why does my image look stretched?
A: It shouldn’t anymore! The plugin now maintains original aspect ratios.

Q: How do I update the spray list?
A: Use the sprayreload command after adding new images.

Q: What’s the maximum image size allowed?
A: The limit is 1024x1024 pixels.

🛠 Current version: 1.0
❤️ Created by: KZ-Sheez
