# Mushkin

A modern MUD client built with Qt. Mushkin is a cross-platform rewrite of [MUSHclient](http://www.gammon.com.au/mushclient/) designed to run natively on macOS and Linux (Windows support in place, but build instructions not ready at this time).

## Features

- **MUSHclient Compatibility**: Lua scripting API compatible with existing MUSHclient plugins
- **Plugin Support**: Load and run MUSHclient plugins with triggers, aliases, timers, and miniwindows
- **Miniwindows**: Plugin-driven UI with drawing, hotspots, and mouse interaction
- **Modern Codebase**: Built with Qt 6 and C++17

## Status

**Alpha** - Core functionality is working but rough edges remain. Many MUSHclient plugins work out of the box, but some features are incomplete or missing.

## Requirements

- **Qt 6.9+**
- **CMake 3.16+**
- **Ninja** (recommended)

## Building

### macOS

**Dependencies:**

```bash
# Xcode Command Line Tools
xcode-select --install

# Build tools and libraries
brew install cmake ninja pcre luajit sqlite openssl

# Qt 6.9+ (installs to current directory)
pip install aqtinstall
aqt install-qt mac desktop 6.9.3 clang_64 -m qtmultimedia qtshadertools
```

**Build:**

```bash
git clone https://github.com/justinpopa/mushkin-public.git
cd mushkin-public
mkdir build && cd build

# Optional: Create your own CMake presets file for convenience
cp ../CMakeUserPresets.json.example ../CMakeUserPresets.json
# Edit CMakeUserPresets.json to set your Qt path, then use:
cmake --preset default

# Or configure directly (adjust Qt path to your aqt install location)
cmake -DCMAKE_PREFIX_PATH=~/6.9.3/macos -G Ninja ..

ninja
open mushkin.app
```

### Linux (Debian/Ubuntu)

**Dependencies:**

```bash
# Build tools and libraries
sudo apt install cmake ninja-build build-essential pkg-config \
    libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev \
    libssl-dev zlib1g-dev libgl1-mesa-dev

# Qt 6.9+ (installs to current directory)
pip install aqtinstall
aqt install-qt linux desktop 6.9.3 linux_gcc_64 -m qtmultimedia qtshadertools
```

**Build:**

```bash
git clone https://github.com/justinpopa/mushkin-public.git
cd mushkin-public
mkdir build && cd build

# Configure (adjust Qt path to your aqt install location)
cmake -DCMAKE_PREFIX_PATH=~/6.9.3/gcc_64 -G Ninja ..

ninja
./mushkin
```

**Headless/CI builds:** If building on a server without a display, set the Qt platform to offscreen:

```bash
QT_QPA_PLATFORM=offscreen ninja
```

## Configuration

Mushkin uses MUSHclient-compatible paths for easy migration:

**World files, plugins, and logs:**
- `~/Documents/MUSHclient/` (worlds, plugins, logs subdirectories)

**Settings:**
- macOS: `~/Library/Preferences/com.mushkin.Mushkin.plist`
- Linux: `~/.config/Mushkin/Mushkin.conf`

## Plugin Compatibility

Mushkin aims for compatibility with MUSHclient's Lua API. Many plugins work without modification, including:
- Miniwindow-based UIs
- GMCP handlers
- Trigger/alias/timer systems
- Database plugins (SQLite)

Some Windows-specific features (COM, certain system calls) are not available.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run the test suite: `cd build && ctest`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Nick Gammon](http://www.gammon.com.au/) for creating MUSHclient
