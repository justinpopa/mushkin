#!/bin/bash
# build-linux.sh - Development build for Linux
#
# Builds mushkin with dynamic Qt linking for fast iteration and debugging.
# Automatically installs Qt via aqt if not present.
#
# Prerequisites (Debian/Ubuntu):
#   sudo apt install cmake ninja-build build-essential pkg-config \
#       libpcre3-dev libsqlite3-dev luajit libluajit-5.1-dev \
#       libssl-dev zlib1g-dev libgl1-mesa-dev libxkbcommon-dev \
#       libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 libxcb-shape0
#   pip3 install aqtinstall
#
# Usage:
#   ./scripts/build-linux.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
QT_VERSION="6.9.3"
QT_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/mushkin/Qt"
QT_INSTALL_DIR="$QT_DIR/$QT_VERSION/gcc_64"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
echo_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check prerequisites
check_prerequisites() {
    local missing=()

    command -v cmake &>/dev/null || missing+=("cmake")
    command -v ninja &>/dev/null || missing+=("ninja-build")
    command -v aqt &>/dev/null || missing+=("aqtinstall (pip3 install aqtinstall)")
    command -v pkg-config &>/dev/null || missing+=("pkg-config")

    if [ ${#missing[@]} -gt 0 ]; then
        echo_error "Missing prerequisites: ${missing[*]}"
        exit 1
    fi
}

# Install Qt if needed
install_qt() {
    if [ -d "$QT_INSTALL_DIR/lib/cmake/Qt6" ]; then
        echo_info "Qt $QT_VERSION found at $QT_INSTALL_DIR"
        return 0
    fi

    echo_info "Installing Qt $QT_VERSION..."
    mkdir -p "$QT_DIR"
    aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 -O "$QT_DIR" -m qtmultimedia qt5compat qtimageformats

    if [ ! -d "$QT_INSTALL_DIR/lib/cmake/Qt6" ]; then
        echo_error "Qt installation failed"
        exit 1
    fi
    echo_info "Qt installed successfully"
}

# Build mushkin
build_mushkin() {
    echo_info "Configuring build..."
    cmake -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_PREFIX_PATH="$QT_INSTALL_DIR" \
        "$PROJECT_ROOT"

    echo_info "Building..."
    cmake --build "$BUILD_DIR" --target mushkin

    echo_info "Build complete!"
    echo ""
    echo_info "Run with: $BUILD_DIR/mushkin"
}

# Main
main() {
    echo "=========================================="
    echo "  Mushkin Development Build (Linux)"
    echo "=========================================="
    echo ""

    check_prerequisites
    install_qt
    build_mushkin
}

main "$@"
