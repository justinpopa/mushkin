#!/bin/bash
# build-linux-static.sh - Release build for Linux with static Qt
#
# Builds a binary with Qt statically linked. Other dependencies (pcre,
# luajit, sqlite, openssl, X11/XCB) are linked dynamically from system packages.
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt install cmake ninja-build build-essential pkg-config \
#       libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev \
#       libssl-dev zlib1g-dev libssh-dev libgl1-mesa-dev libglu1-mesa-dev \
#       libxkbcommon-dev libxcb1-dev libxcb-cursor-dev libxcb-icccm4-dev \
#       libxcb-keysyms1-dev libxcb-shape0-dev libxcb-xfixes0-dev \
#       libxcb-sync-dev libxcb-randr0-dev libxcb-render-util0-dev \
#       libxcb-image0-dev libxcb-glx0-dev libxcb-shm0-dev \
#       libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev \
#       libxext-dev libxfixes-dev libxi-dev libxrender-dev \
#       libatspi2.0-dev libglib2.0-dev
#   pip3 install aqtinstall
#
# Usage:
#   ./scripts/build-linux-static.sh
#
# Output:
#   build-static/mushkin - Binary with static Qt
#   build-static/lib/    - Bundled dynamic libraries
#   build-static/lua/    - Lua scripts
#
# Note: First run takes ~60 minutes to build Qt.
#       Subsequent builds take ~2 minutes (Qt is cached).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-static"
CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/mushkin"
QT_STATIC_DIR="$CACHE_DIR/Qt-static/6.9.3-static"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
echo_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

NUM_CORES=$(nproc)

# Find the highest available clang version (C++26 requires clang 19+)
find_clang() {
    for ver in 22 21 20 19; do
        if command -v "clang-$ver" &>/dev/null; then
            echo "$ver"
            return
        fi
    done
    echo ""
}

CLANG_VER=$(find_clang)
if [ -n "$CLANG_VER" ]; then
    export CC="clang-$CLANG_VER"
    export CXX="clang++-$CLANG_VER"
else
    echo_warn "No clang 19+ found, falling back to system compiler"
fi

# ============================================================
# Prerequisites
# ============================================================

check_prerequisites() {
    echo_info "Checking prerequisites..."
    local missing=()

    command -v cmake &>/dev/null || missing+=("cmake")
    command -v ninja &>/dev/null || missing+=("ninja-build")
    command -v aqt &>/dev/null || missing+=("aqtinstall (pip3 install aqtinstall)")
    command -v pkg-config &>/dev/null || missing+=("pkg-config")

    # Check for required libraries using pkg-config
    pkg-config --exists libpcre 2>/dev/null || missing+=("libpcre3-dev")
    pkg-config --exists sqlite3 2>/dev/null || missing+=("libsqlite3-dev")
    pkg-config --exists luajit 2>/dev/null || missing+=("libluajit-5.1-dev")
    pkg-config --exists openssl 2>/dev/null || missing+=("libssl-dev")
    pkg-config --exists zlib 2>/dev/null || missing+=("zlib1g-dev")
    pkg-config --exists libssh 2>/dev/null || missing+=("libssh-dev")
    pkg-config --exists xcb 2>/dev/null || missing+=("libxcb1-dev")
    pkg-config --exists xkbcommon 2>/dev/null || missing+=("libxkbcommon-dev")

    if [ ${#missing[@]} -gt 0 ]; then
        echo_error "Missing: ${missing[*]}"
        echo_error "Install with: sudo apt install cmake ninja-build build-essential pkg-config \\"
        echo_error "    libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev libssl-dev zlib1g-dev libssh-dev \\"
        echo_error "    libgl1-mesa-dev libxkbcommon-dev libxcb1-dev libxcb-cursor-dev libxcb-icccm4-dev \\"
        echo_error "    libxcb-keysyms1-dev libxcb-shape0-dev libxcb-xfixes0-dev libxcb-sync-dev \\"
        echo_error "    libxcb-randr0-dev libxcb-render-util0-dev libxcb-image0-dev libxcb-glx0-dev \\"
        echo_error "    libxcb-shm0-dev libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev \\"
        echo_error "    libxext-dev libxfixes-dev libxi-dev libxrender-dev libatspi2.0-dev libglib2.0-dev"
        exit 1
    fi
    echo_info "Prerequisites OK"
}

# ============================================================
# Static Qt Build
# ============================================================

qt_is_built() {
    [ -f "$QT_STATIC_DIR/lib/libQt6Core.a" ]
}

build_static_qt() {
    if qt_is_built; then
        echo_info "Static Qt already built"
        return 0
    fi

    echo_info "Building static Qt (~60 min)..."

    local QT_VERSION="6.9.3"
    local QT_BASE_DIR="$CACHE_DIR/Qt-static"
    local QT_SRC_DIR="$QT_BASE_DIR/$QT_VERSION/Src"
    local QT_BUILD_DIR="$QT_SRC_DIR/build-static"

    # Download Qt source (verify required submodules exist)
    if [ ! -d "$QT_SRC_DIR" ] || [ ! -d "$QT_SRC_DIR/qtmultimedia" ]; then
        if [ -d "$QT_SRC_DIR" ]; then
            echo_warn "Incomplete Qt source detected, re-downloading..."
            rm -rf "$QT_SRC_DIR"
        fi
        echo_info "Downloading Qt source..."
        mkdir -p "$QT_BASE_DIR"
        aqt install-src linux "$QT_VERSION" --outputdir "$QT_BASE_DIR"
    fi

    # Configure
    mkdir -p "$QT_BUILD_DIR"
    cd "$QT_BUILD_DIR"

    # Only build what we need, skip optional QML/Quick deps
    # -no-pch disables precompiled headers to reduce disk usage
    # Note: qtshadertools is required by qtmultimedia, so we can't skip it
    "$QT_SRC_DIR/configure" \
        -static \
        -release \
        -prefix "$QT_STATIC_DIR" \
        -submodules qtbase,qtmultimedia,qtsvg,qtshadertools \
        -skip qtdeclarative -skip qtquick3d \
        -nomake examples -nomake tests \
        -no-pch

    # Build & install
    cmake --build . --parallel "$NUM_CORES"
    cmake --install .

    echo_info "Static Qt built successfully"
}

# ============================================================
# Build Mushkin
# ============================================================

build_mushkin() {
    echo_info "Building mushkin..."

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    local cmake_args=(
        -DSTATIC_BUILD=ON
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_PREFIX_PATH="$QT_STATIC_DIR"
    )
    if [ -n "$CLANG_VER" ]; then
        cmake_args+=(
            -DCMAKE_C_COMPILER="clang-$CLANG_VER"
            -DCMAKE_CXX_COMPILER="clang++-$CLANG_VER"
        )
    fi

    cmake "$PROJECT_ROOT" "${cmake_args[@]}"

    cmake --build . -j"$NUM_CORES"
}

# ============================================================
# Bundle Dynamic Libraries
# ============================================================

bundle_dylibs() {
    echo_info "Bundling dynamic libraries..."

    local lib_dir="$BUILD_DIR/lib"
    mkdir -p "$lib_dir"

    # Whitelist of libraries to bundle (these are the non-system deps)
    local whitelist=(
        "libpcre.so"
        "libluajit-5.1.so"
        "libsqlite3.so"
        "libssl.so"
        "libcrypto.so"
        "libssh.so"
        "libz.so"
    )

    local binary="$BUILD_DIR/mushkin"

    # For each whitelisted library, find it via ldd and copy
    for lib_pattern in "${whitelist[@]}"; do
        # Find the actual path from ldd output
        local lib_path
        lib_path=$(ldd "$binary" 2>/dev/null | grep "$lib_pattern" | awk '{print $3}')

        if [ -z "$lib_path" ] || [ ! -f "$lib_path" ]; then
            echo_warn "  $lib_pattern not found in ldd output (may be statically linked)"
            continue
        fi

        # Resolve symlinks to get the real file
        local real_path
        real_path=$(realpath "$lib_path")
        local real_name
        real_name=$(basename "$real_path")

        # Copy the real library file
        if [ ! -f "$lib_dir/$real_name" ]; then
            echo_info "  Copying $real_name"
            cp "$real_path" "$lib_dir/$real_name"
        fi

        # Recreate symlinks that the linker uses
        # e.g., libpcre.so.3 -> libpcre.so.3.13.3
        #        libpcre.so -> libpcre.so.3
        local link_name
        link_name=$(basename "$lib_path")
        if [ "$link_name" != "$real_name" ] && [ ! -e "$lib_dir/$link_name" ]; then
            ln -sf "$real_name" "$lib_dir/$link_name"
        fi

        # Also create the base .so symlink if it doesn't exist
        # e.g., libpcre.so -> libpcre.so.3.13.3
        local base_name="$lib_pattern"
        if [ ! -e "$lib_dir/$base_name" ] && [ "$base_name" != "$real_name" ]; then
            ln -sf "$real_name" "$lib_dir/$base_name"
        fi
    done

    # Lua .so modules should already be in build-static/lib/ from cmake build
    # Verify they exist
    local lua_modules=("socket/core.so" "socket/unix.so" "mime/core.so" "ssl.so" "yue.so" "llthreads2.so")
    echo_info "Checking Lua C modules..."
    for mod in "${lua_modules[@]}"; do
        if [ -f "$lib_dir/$mod" ]; then
            echo_info "  Found $mod"
        else
            echo_warn "  Missing $mod (may not have been built)"
        fi
    done

    echo_info "Bundling complete"
}

# ============================================================
# Verify Build
# ============================================================

verify_build() {
    local binary="$BUILD_DIR/mushkin"

    if [ ! -f "$binary" ]; then
        echo_error "Build failed - binary not found"
        exit 1
    fi

    echo ""
    echo_info "=== Build Summary ==="
    echo "  Size: $(du -h "$binary" | cut -f1)"
    echo "  Location: $binary"
    echo ""

    # Show bundled libraries
    local lib_dir="$BUILD_DIR/lib"
    if [ -d "$lib_dir" ]; then
        echo_info "Bundled libraries:"
        find "$lib_dir" -maxdepth 1 -type f -name "*.so*" | sort | while read -r f; do
            echo "  $(basename "$f") ($(du -h "$f" | cut -f1))"
        done
        echo ""
        echo_info "Bundled Lua modules:"
        find "$lib_dir" -name "*.so" -not -maxdepth 1 2>/dev/null | sort | while read -r f; do
            echo "  $(echo "$f" | sed "s|$lib_dir/||")"
        done
    fi

    echo ""

    # Verify RPATH is set correctly
    echo_info "Checking RPATH..."
    local rpath
    rpath=$(readelf -d "$binary" 2>/dev/null | grep -E "RPATH|RUNPATH" || true)
    if echo "$rpath" | grep -q 'RPATH.*\$ORIGIN/lib'; then
        echo_info "  OK - RPATH set to \$ORIGIN/lib"
    elif echo "$rpath" | grep -q 'RUNPATH.*\$ORIGIN/lib'; then
        echo_warn "  RUNPATH set (not RPATH) - transitive deps may not resolve"
    else
        echo_warn "  RPATH not found - bundled libs may not be found at runtime"
    fi

    # Check dynamic dependencies resolve correctly
    echo_info "Dynamic dependencies:"
    ldd "$binary" | grep -v "linux-vdso\|ld-linux" | head -20
}

# ============================================================
# Main
# ============================================================

main() {
    echo "=========================================="
    echo "  Mushkin Release Build (Linux)"
    echo "=========================================="
    echo ""

    check_prerequisites
    build_static_qt
    build_mushkin
    bundle_dylibs
    verify_build

    echo ""
    echo_info "Build complete!"
    echo_info "Binary: $BUILD_DIR/mushkin"
}

main "$@"
