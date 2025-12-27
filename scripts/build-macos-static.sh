#!/bin/bash
# build-macos-static.sh - Release build for macOS with static Qt
#
# Builds a native binary with Qt statically linked. Other dependencies
# (pcre, luajit, sqlite, openssl) are linked dynamically from Homebrew.
#
# Note: This builds for the native architecture only (arm64 on Apple Silicon,
# x86_64 on Intel). Homebrew doesn't provide universal dynamic libraries.
#
# Prerequisites:
#   xcode-select --install
#   brew install cmake ninja pcre luajit sqlite openssl
#   pip3 install aqtinstall
#
# Usage:
#   ./scripts/build-macos-static.sh
#
# Output:
#   build-static/mushkin - Native binary with static Qt
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

NUM_CORES=$(sysctl -n hw.ncpu)
NATIVE_ARCH=$(uname -m)

# ============================================================
# Prerequisites
# ============================================================

check_prerequisites() {
    echo_info "Checking prerequisites..."
    local missing=()

    command -v cmake &>/dev/null || missing+=("cmake")
    command -v ninja &>/dev/null || missing+=("ninja")
    command -v aqt &>/dev/null || missing+=("aqtinstall (pip3 install aqtinstall)")

    # Check Homebrew dependencies
    [ -f /opt/homebrew/opt/pcre/lib/libpcre.dylib ] || [ -f /usr/local/opt/pcre/lib/libpcre.dylib ] || missing+=("pcre")
    [ -f /opt/homebrew/opt/luajit/lib/libluajit-5.1.dylib ] || [ -f /usr/local/opt/luajit/lib/libluajit-5.1.dylib ] || missing+=("luajit")
    [ -f /opt/homebrew/opt/sqlite/lib/libsqlite3.dylib ] || [ -f /usr/local/opt/sqlite/lib/libsqlite3.dylib ] || missing+=("sqlite")
    [ -f /opt/homebrew/opt/openssl/lib/libssl.dylib ] || [ -f /usr/local/opt/openssl/lib/libssl.dylib ] || missing+=("openssl")

    if [ ${#missing[@]} -gt 0 ]; then
        echo_error "Missing: ${missing[*]}"
        echo_error "Install with: brew install cmake ninja pcre luajit sqlite openssl"
        exit 1
    fi
    echo_info "Prerequisites OK"
}

# ============================================================
# Static Qt Build (native architecture only)
# ============================================================

qt_is_built() {
    [ -f "$QT_STATIC_DIR/lib/QtCore.framework/QtCore" ] && \
    file "$QT_STATIC_DIR/lib/QtCore.framework/QtCore" | grep -q "ar archive"
}

build_static_qt() {
    if qt_is_built; then
        echo_info "Static Qt already built"
        return 0
    fi

    echo_info "Building static Qt for $NATIVE_ARCH (~60 min)..."

    local QT_VERSION="6.9.3"
    local QT_BASE_DIR="$CACHE_DIR/Qt-static"
    local QT_SRC_DIR="$QT_BASE_DIR/$QT_VERSION/Src"
    local QT_BUILD_DIR="$QT_SRC_DIR/build-static"

    # Download Qt source
    if [ ! -d "$QT_SRC_DIR" ]; then
        echo_info "Downloading Qt source..."
        mkdir -p "$QT_BASE_DIR"
        aqt install-src mac "$QT_VERSION" --outputdir "$QT_BASE_DIR"
    fi

    # Configure - build for native architecture only
    mkdir -p "$QT_BUILD_DIR"
    cd "$QT_BUILD_DIR"

    "$QT_SRC_DIR/configure" \
        -static \
        -release \
        -prefix "$QT_STATIC_DIR" \
        -skip qtwebengine -skip qtwebview -skip qt3d -skip qtcharts \
        -skip qtdatavis3d -skip qtlottie -skip qtquick3d -skip qtquick3dphysics \
        -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebsockets \
        -skip qtpositioning -skip qtsensors -skip qtserialport -skip qtserialbus \
        -skip qtremoteobjects -skip qthttpserver -skip qtquicktimeline \
        -skip qtquickeffectmaker -skip qtlocation -skip qtcoap -skip qtmqtt \
        -skip qtopcua -skip qtgrpc -skip qtlanguageserver -skip qtspeech \
        -skip qtconnectivity -skip qtactiveqt -skip qtscxml \
        -nomake examples -nomake tests

    # Build & install
    cmake --build . --parallel "$NUM_CORES"
    cmake --install .

    echo_info "Static Qt built successfully"
}

# ============================================================
# Build Mushkin
# ============================================================

build_mushkin() {
    echo_info "Building mushkin for $NATIVE_ARCH..."

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake "$PROJECT_ROOT" \
        -DSTATIC_BUILD=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_STATIC_DIR"

    cmake --build . --target mushkin -j"$NUM_CORES"
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

    # Check architecture
    local archs=$(lipo -archs "$binary" 2>/dev/null)
    echo "  Architecture: $archs"
    echo "  Size: $(du -h "$binary" | cut -f1)"
    echo "  Location: $binary"
    echo ""

    # Show dynamic dependencies
    echo_info "Dynamic dependencies:"
    otool -L "$binary" | grep -v "mushkin:" | head -20
}

# ============================================================
# Main
# ============================================================

main() {
    echo "=========================================="
    echo "  Mushkin Release Build (macOS $NATIVE_ARCH)"
    echo "=========================================="
    echo ""

    check_prerequisites
    build_static_qt
    build_mushkin
    verify_build

    echo ""
    echo_info "Build complete!"
    echo_info "Binary: $BUILD_DIR/mushkin"
}

main "$@"
