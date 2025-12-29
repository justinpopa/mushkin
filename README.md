# Mushkin

A modern MUD client built with Qt. Mushkin is a cross-platform rewrite of [MUSHclient](http://www.gammon.com.au/mushclient/) designed to run natively on macOS, Linux, and Windows.

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

#### Development Build

For contributors and active development. This build links dynamically against Qt - meant for fast iteration and debugging, not distribution.

```bash
# Prerequisites
xcode-select --install
brew install cmake ninja pcre luajit sqlite openssl
pip3 install aqtinstall

# Clone and build
git clone https://github.com/justinpopa/mushkin.git
cd mushkin
./scripts/build-macos.sh

open build/mushkin.app
```

#### Static Build (Release Binary)

**This is the recommended build for end users.** Creates a native binary (arm64 on Apple Silicon, x86_64 on Intel) with Qt statically linked. Other dependencies are dynamically linked from Homebrew.

```bash
# Prerequisites
xcode-select --install
brew install cmake ninja pcre luajit sqlite openssl
pip3 install aqtinstall

# Build (first run ~60 min for Qt, subsequent runs ~2 min)
git clone https://github.com/justinpopa/mushkin.git
cd mushkin
./scripts/build-macos-static.sh

# Output: build-static/mushkin
```

The binary requires the Homebrew dependencies to be installed, but Qt is embedded so users don't need to install Qt separately.

### Linux (Debian/Ubuntu)

#### Development Build

For contributors and active development. Links dynamically against Qt.

```bash
# Prerequisites
sudo apt install cmake ninja-build build-essential pkg-config \
    libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev \
    libssl-dev zlib1g-dev libgl1-mesa-dev libxkbcommon-dev \
    libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 libxcb-shape0
pip3 install aqtinstall

# Clone and build
git clone https://github.com/justinpopa/mushkin.git
cd mushkin
./scripts/build-linux.sh

./build/mushkin
```

#### Static Build (Release Binary)

**This is the recommended build for end users.** Creates a binary with Qt statically linked. Other dependencies are dynamically linked from system packages.

```bash
# Prerequisites
sudo apt install cmake ninja-build build-essential pkg-config \
    libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev \
    libssl-dev zlib1g-dev libgl1-mesa-dev libglu1-mesa-dev \
    libxkbcommon-dev libxcb1-dev libxcb-cursor-dev libxcb-icccm4-dev \
    libxcb-keysyms1-dev libxcb-shape0-dev libxcb-xfixes0-dev \
    libxcb-sync-dev libxcb-randr0-dev libxcb-render-util0-dev \
    libxcb-image0-dev libxcb-glx0-dev libxcb-shm0-dev \
    libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev \
    libxext-dev libxfixes-dev libxi-dev libxrender-dev \
    libatspi2.0-dev libglib2.0-dev
pip3 install aqtinstall

# Build (first run ~60 min for Qt, subsequent runs ~2 min)
git clone https://github.com/justinpopa/mushkin.git
cd mushkin
./scripts/build-linux-static.sh

# Output: build-static/mushkin
```

The binary requires the apt packages to be installed, but Qt is embedded so users don't need to install Qt separately.

### Windows

#### Prerequisites

1. **Visual Studio 2022** with C++ workload
2. **Git for Windows**
3. **Python 3.x** and aqtinstall: `pip install aqtinstall`
4. **vcpkg** for dependencies:

```powershell
# Install vcpkg (one-time setup)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")

# Install dependencies
C:\vcpkg\vcpkg install pcre:x64-windows luajit:x64-windows sqlite3:x64-windows openssl:x64-windows zlib:x64-windows
```

5. **Enable Long Paths** (required for Qt build):

```powershell
# Run as Administrator
Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name 'LongPathsEnabled' -Value 1 -Type DWord
```

Qt's source tree has deeply nested paths that exceed Windows' default 260 character limit. This registry change enables long path support system-wide.

#### Static Build (Release Binary)

Run from **Developer PowerShell for VS 2022**:

```powershell
git clone https://github.com/justinpopa/mushkin.git
cd mushkin
.\scripts\build-windows-static.ps1

# Output: build-static\mushkin.exe
```

First run takes ~60 minutes to build Qt. Subsequent builds take ~5 minutes.

## Configuration

Mushkin uses portable relative paths like the original MUSHclient. By default, all data directories are relative to the current working directory:

**Default directories (relative to working directory):**
- `./worlds/` - World files (.mcl)
- `./worlds/plugins/` - Plugin files
- `./worlds/plugins/state/` - Plugin state files
- `./logs/` - Log files

This means you can put Mushkin in any folder and run it from there - all your worlds, plugins, and logs stay together in that folder.

**Settings storage:**
- macOS: `~/Library/Preferences/com.Gammon.MUSHclient.plist`
- Linux: `~/.config/Gammon/MUSHclient.conf`
- Windows: Registry `HKEY_CURRENT_USER\Software\Gammon\MUSHclient`

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
