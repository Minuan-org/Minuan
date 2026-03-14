# 🌟 Minuan - C++ Browser for SimVerse

> A kid-powered browser built **by kids, for kids** ages 5-10
> 
> **Supports HTML6, CSS Next, and JS²**

## 🎯 What is Minuan?

**Minuan** is a **simple, safe, and powerful browser** written in **C++17** specifically designed for children to explore, create, and build within the **SimVerse universe**.

### Built on HTML6
Minuan uses the **HTML6 specification** created by Commander AbdulSamad (age 8), which includes:
- ✨ HTML6 - New simplified markup language
- 🎨 CSS Next - Modern styling
- 💻 JS² v2.0 - Interactive scripting
- 🖥️ Integrated backend servers (cloud + multiplayer support)

### Created for SimVerse
SimVerse is a kid-powered company created by **Commander AbdulSamad** (age 8). It brings together:
- 🎮 A Real-Life 3D Simulator
- 🛠️ Tools for everyone from babies to grandparents
- 🌐 A new kind of internet (HTML6, CSS Next, JS²)
- 🛡️ A safe creative space for kids

**Learn more:** See [SIMVERSE_PETITION.md](SIMVERSE_PETITION.md)

---

## ✨ Features

- 🌈 **Colorful, Kid-Friendly UI** - Built with native C++ widgets
- 📄 **HTML6 Support** - Native HTML6 parser and engine
- 🎨 **CSS Next Support** - Modern CSS parsing and rendering
- 💻 **JS² v2.0 Integration** - Interactive script execution
- 🖥️ **Server Support** - Cloud and local backend servers
- 🌐 **Multi-Platform** - Windows, macOS, Linux
- 🔒 **Safe Browsing** - Only SimVerse whitelisted URLs
- 📑 **Tabbed Browsing** - Open multiple pages simultaneously
- 🔖 **Bookmarks** - Save favorite SimVerse pages
- 📜 **History** - Track visited pages with timestamps
- ⚙️ **Parental Controls** - Safe for kids, configurable by parents
- ⚡ **Fast & Lightweight** - Efficient C++ implementation
- 🔐 **Content Security** - URL validation and filtering

---

## 🚀 Quick Start

### Requirements
- **C++17** compiler (GCC, Clang, or MSVC)
- **CMake 3.15+**
- **WebView2** (Windows) or equivalents (Linux/macOS)
- **HTML6 Parser** (included)
- **SQLite3** for storage

### Build Instructions

#### Windows
```bash
cd minuan
mkdir build
cd build
cmake ..
cmake --build . --config Release
./minuan.exe