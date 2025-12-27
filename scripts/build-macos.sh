#!/bin/bash
# build-macos.sh - Development build for macOS
#
# Builds mushkin with dynamic Qt linking for fast iteration and debugging.
# Automatically installs Qt via aqt if not present.
#
# Prerequisites:
#   - Xcode Command Line Tools
#   - brew install cmake ninja pcre luajit sqlite openssl
#   - pip3 install aqtinstall
#
# Usage:
#   ./scripts/build-macos.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
QT_VERSION="6.9.3"
QT_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/mushkin/Qt"
QT_INSTALL_DIR="$QT_DIR/$QT_VERSION/macos"

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
    command -v ninja &>/dev/null || missing+=("ninja")
    command -v aqt &>/dev/null || missing+=("aqtinstall (pip3 install aqtinstall)")

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
    aqt install-qt mac desktop "$QT_VERSION" -O "$QT_DIR" -m qtmultimedia qt5compat qtimageformats

    if [ ! -d "$QT_INSTALL_DIR/lib/cmake/Qt6" ]; then
        echo_error "Qt installation failed"
        exit 1
    fi
    echo_info "Qt installed successfully"
}

# Build mushkin
build_mushkin() {
    echo_info "Configuring build..."
    cmake -B "$BUILD_DIR" -G Ninja "$PROJECT_ROOT"

    echo_info "Building..."
    cmake --build "$BUILD_DIR" --target mushkin

    echo_info "Build complete!"
    echo ""
    echo_info "Run with: open $BUILD_DIR/mushkin.app"
}

# Main
main() {
    echo "=========================================="
    echo "  Mushkin Development Build (macOS)"
    echo "=========================================="
    echo ""

    check_prerequisites
    install_qt
    build_mushkin
}

main "$@"
