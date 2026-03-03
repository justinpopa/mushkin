# Mushkin

[![CI](https://github.com/justinpopa/mushkin/actions/workflows/ci.yml/badge.svg)](https://github.com/justinpopa/mushkin/actions/workflows/ci.yml)
[![Release](https://github.com/justinpopa/mushkin/actions/workflows/release.yml/badge.svg)](https://github.com/justinpopa/mushkin/actions/workflows/release.yml)
[![Latest Release](https://img.shields.io/github/v/release/justinpopa/mushkin?include_prereleases)](https://github.com/justinpopa/mushkin/releases)
[![License](https://img.shields.io/github/license/justinpopa/mushkin)](LICENSE)
[![macOS](https://img.shields.io/badge/macOS-arm64%20%7C%20x64-black?logo=apple)](https://github.com/justinpopa/mushkin/releases)
[![Linux](https://img.shields.io/badge/Linux-x64-black?logo=linux&logoColor=white)](https://github.com/justinpopa/mushkin/releases)
[![Windows](https://img.shields.io/badge/Windows-x64-black?logo=windows)](https://github.com/justinpopa/mushkin/releases)

A modern MUD client built with Qt. Mushkin is a cross-platform rewrite of [MUSHclient](http://www.gammon.com.au/mushclient/) designed to run natively on macOS, Linux, and Windows.

## Features

- **MUSHclient Compatibility**: 424/428 Lua API functions ported (~99%) - existing plugins work without modification
- **Plugin Support**: Triggers, aliases, timers, miniwindows, GMCP, database (SQLite), sound, and more
- **Cross-Platform**: Native builds on macOS (ARM/Intel), Linux (x86_64), and Windows (x64)
- **Modern Codebase**: C++26, Qt 6.9.3 Widgets, CMake + Ninja

## Status

**Beta** - Core functionality is working across all three platforms. The Lua API is near-complete and tested against MUSHclient for behavioral parity. Many MUSHclient plugins work out of the box.

## Requirements

- **Clang 19+** (22 recommended for full C++26 support)
- **Qt 6.9.3**
- **CMake 3.16+**
- **Ninja** (recommended)

## Building

### macOS

```bash
# Prerequisites
xcode-select --install
brew install llvm cmake ninja pcre luajit sqlite openssl libssh
pip3 install aqtinstall
aqt install-qt mac desktop 6.9.3 -m qtmultimedia --outputdir ~/Qt

# Clone and build
git clone https://github.com/justinpopa/mushkin.git
cd mushkin

LLVM_PREFIX=$(brew --prefix llvm)
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=${LLVM_PREFIX}/bin/clang \
    -DCMAKE_CXX_COMPILER=${LLVM_PREFIX}/bin/clang++ \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++"
cmake --build build --parallel

# Run
open build/bin/mushkin.app
```

### Linux (Ubuntu 24.04)

```bash
# Clang 22 from LLVM repos
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-22 main" | \
    sudo tee /etc/apt/sources.list.d/llvm.list
sudo apt-get update
sudo apt-get install -y clang-22

# Dependencies
sudo apt-get install -y \
    cmake ninja-build g++ python3-pip \
    libgl1-mesa-dev libxkbcommon-dev \
    libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 libxcb-shape0 \
    libluajit-5.1-dev libpcre3-dev libsqlite3-dev \
    libssl-dev zlib1g-dev libssh-dev \
    libpulse-dev libasound2-dev libpipewire-0.3-dev

# Qt
pip3 install --break-system-packages aqtinstall
aqt install-qt linux desktop 6.9.3 linux_gcc_64 -m qtmultimedia --outputdir ~/Qt

# Clone and build
git clone https://github.com/justinpopa/mushkin.git
cd mushkin

cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-22 \
    -DCMAKE_CXX_COMPILER=clang++-22
cmake --build build --parallel

# Run
export QT_QPA_PLATFORM=xcb  # or wayland
./build/bin/mushkin
```

### Windows

```powershell
# Prerequisites: Visual Studio 2022 Build Tools, Chocolatey
choco install cmake ninja llvm python3 git -y

# Qt
python -m pip install aqtinstall
python -m aqt install-qt windows desktop 6.9.3 win64_msvc2022_64 -m qtmultimedia --outputdir C:\Qt

# vcpkg dependencies
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install pcre:x64-windows luajit:x64-windows sqlite3:x64-windows `
    openssl:x64-windows zlib:x64-windows libssh:x64-windows

# Clone and build
git clone https://github.com/justinpopa/mushkin.git
cd mushkin

cmake -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=clang-cl `
    -DCMAKE_CXX_COMPILER=clang-cl `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release --parallel

# Run
.\build\bin\mushkin.exe
```

### Running Tests

```bash
# Unix
export QT_QPA_PLATFORM=offscreen
ctest --test-dir build --output-on-failure

# Windows
ctest --test-dir build --output-on-failure --build-config Release
```

## Configuration

Mushkin uses portable relative paths like the original MUSHclient. All data directories are relative to the current working directory:

- `./worlds/` - World files (.mcl)
- `./worlds/plugins/` - Plugin files
- `./worlds/plugins/state/` - Plugin state files
- `./logs/` - Log files

**Settings storage:**
- macOS: `~/Library/Preferences/com.Gammon.MUSHclient.plist`
- Linux: `~/.config/Gammon/MUSHclient.conf`
- Windows: Registry `HKEY_CURRENT_USER\Software\Gammon\MUSHclient`

## Plugin Compatibility

Mushkin aims for behavioral parity with MUSHclient's Lua API. Most plugins work without modification, including:
- Miniwindow-based UIs (Aardwolf plugins, etc.)
- GMCP/MSDP handlers
- Trigger/alias/timer systems
- Database plugins (SQLite)
- Sound playback
- Notepad windows

The 4 unported functions are zChat-related (deprecated protocol, won't-implement).

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run the test suite: `ctest --test-dir build --output-on-failure`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Nick Gammon](http://www.gammon.com.au/) for creating MUSHclient
