🎮 Undertale Translation Tool

A lightweight Windows tool designed to make translating Undertale easier by combining a JSON editor and a live game preview in a single window.

✨ Features
📝 Built-in JSON editor (UTF-8 support)
🎯 Designed for editing Undertale language files
🔍 Find / Find Next functionality
💾 One-click save with automatic backup (.bak)
🎮 Embedded Undertale window (live preview while translating)
🔄 Restart Undertale directly from the tool
🌙 Dark theme UI
🔠 Proper support for special characters (Hungarian, etc.)

📂 How it works

The tool is designed to be placed inside your Undertale folder.

Before you first run this tool you need to patch the current data.win with the UndertaleWithJSONs.csx script using the Undertale Mod Tool

Expected structure:

Undertale/
│
├── Undertale.exe
├── undertale_tool.exe
│
└── lang/
    └── lang_en.json

On startup, the tool will:

Automatically launch Undertale
Load lang/lang_en.json
Display the game inside the right panel
Let you edit text live on the left

🚀 Usage
Place the tool in your Undertale directory
Run undertale_tool.exe
Edit text in the left panel
Click Save JSON
Click Restart Undertale to reload changes
Use Focus Undertale to return input to the game

⚙️ Building (MinGW-w64)

Compile using:

g++ undertale_tool.cpp -o undertale_tool.exe -mwindows -municode -static -static-libgcc -static-libstdc++

WARNING: This program only works on windows.
