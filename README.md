<div align="center">

# 🎮 Undertale Translation Tool

<p>
  <img src="https://img.shields.io/badge/C++-WinAPI-blue">
  <img src="https://img.shields.io/badge/platform-Windows-lightgrey">
  <img src="https://img.shields.io/badge/status-active-success">
</p>

<p>
A lightweight Windows tool for translating Undertale with a live preview and built-in editor.
</p>

</div>

---

## ✨ Features

- 📝 Built-in JSON editor (UTF-8 support)  
- 🎯 Designed specifically for Undertale language files  
- 🔍 Find / Find Next functionality  
- 💾 One-click save with automatic backup (`.bak`)  
- 🎮 Embedded Undertale window (live preview while translating)  
- 🔄 Restart Undertale directly from the tool  
- 🌙 Dark theme UI  
- 🔠 Proper support for special characters (Hungarian, etc.)  

---

## ⚠️ Required Setup

Before running the tool for the first time, you **must patch your game**:

1. Open `data.win` in **Undertale Mod Tool**  
2. Run the script `UndertaleWithJSONs.csx`  
3. Save the modified file  

Without this step, the tool will not work properly.

---

## 📂 How it works

The tool should be placed inside your Undertale directory.

### Expected structure:

```text
Undertale/
│
├── Undertale.exe
├── undertale_tool.exe
│
└── lang/
    └── lang_en.json
```

### On startup, the tool will:

- Automatically launch Undertale  
- Load `lang/lang_en.json`  
- Display the game inside the right panel  
- Let you edit text live on the left  

---

## 🚀 Usage

1. Place the tool in your Undertale directory  
2. Run `undertale_tool.exe`  
3. Edit text in the left panel  
4. Click **Save JSON**  
5. Click **Restart Undertale** to reload changes  
6. Use **Focus Undertale** to return input to the game  

---

## ⚙️ Building (MinGW-w64)

```bash
g++ undertale_tool.cpp -o undertale_tool.exe -mwindows -municode -static -static-libgcc -static-libstdc++
```

---

## ⚠️ Platform

> This application works **only on Windows**.

---

## 💡 Notes

<details>
<summary>Click to expand</summary>

- The tool uses pure WinAPI (no external libraries)  
- The JSON file is treated as plain text (no validation)  
- Game embedding relies on Windows window re-parenting  
- Some GameMaker behaviors (like scaling) may vary  

</details>

---
