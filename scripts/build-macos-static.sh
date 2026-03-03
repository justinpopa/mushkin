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
#   Full Xcode (not just command-line tools) — Qt static builds need Metal headers
#   brew install cmake ninja llvm pcre luajit sqlite openssl libssh
#   pip3 install aqtinstall
#
# Usage:
#   ./scripts/build-macos-static.sh
#
# Output:
#   build-static/mushkin.app - Self-contained app bundle with static Qt
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

# Use Homebrew LLVM — Apple's clang doesn't support C++26
if [ -x /opt/homebrew/opt/llvm/bin/clang ]; then
    export CC=/opt/homebrew/opt/llvm/bin/clang
    export CXX=/opt/homebrew/opt/llvm/bin/clang++
elif [ -x /usr/local/opt/llvm/bin/clang ]; then
    export CC=/usr/local/opt/llvm/bin/clang
    export CXX=/usr/local/opt/llvm/bin/clang++
fi

# ============================================================
# Prerequisites
# ============================================================

check_prerequisites() {
    echo_info "Checking prerequisites..."
    local missing=()

    # Full Xcode is required for static Qt builds (Metal framework headers, etc.)
    # Command-line tools alone are not sufficient.
    if ! xcode-select -p 2>/dev/null | grep -q "Xcode.app"; then
        echo_error "Full Xcode installation required (not just command-line tools)."
        echo_error "Static Qt builds need Metal framework headers that only ship with Xcode."
        echo_error ""
        echo_error "Install Xcode from the App Store, then run:"
        echo_error "  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
        echo_error ""
        echo_error "If you just want to run Mushkin, download a pre-built release instead:"
        echo_error "  https://github.com/justinpopa/mushkin/releases"
        exit 1
    fi

    command -v cmake &>/dev/null || missing+=("cmake")
    command -v ninja &>/dev/null || missing+=("ninja")
    command -v aqt &>/dev/null || missing+=("aqtinstall (pip3 install aqtinstall)")

    # Require Homebrew LLVM for C++26 support
    if [ -z "$CC" ] || [ -z "$CXX" ]; then
        missing+=("llvm (brew install llvm)")
    else
        echo_info "Using compiler: $CXX"
    fi

    # Check Homebrew dependencies
    [ -f /opt/homebrew/opt/pcre/lib/libpcre.dylib ] || [ -f /usr/local/opt/pcre/lib/libpcre.dylib ] || missing+=("pcre")
    [ -f /opt/homebrew/opt/luajit/lib/libluajit-5.1.dylib ] || [ -f /usr/local/opt/luajit/lib/libluajit-5.1.dylib ] || missing+=("luajit")
    [ -f /opt/homebrew/opt/sqlite/lib/libsqlite3.dylib ] || [ -f /usr/local/opt/sqlite/lib/libsqlite3.dylib ] || missing+=("sqlite")
    [ -f /opt/homebrew/opt/openssl/lib/libssl.dylib ] || [ -f /usr/local/opt/openssl/lib/libssl.dylib ] || missing+=("openssl")
    [ -f /opt/homebrew/opt/libssh/lib/libssh.dylib ] || [ -f /usr/local/opt/libssh/lib/libssh.dylib ] || missing+=("libssh")

    if [ ${#missing[@]} -gt 0 ]; then
        echo_error "Missing: ${missing[*]}"
        echo_error "Install with: brew install cmake ninja llvm pcre luajit sqlite openssl libssh"
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

    # Download Qt source (verify required submodules exist)
    if [ ! -d "$QT_SRC_DIR" ] || [ ! -d "$QT_SRC_DIR/qtmultimedia" ]; then
        if [ -d "$QT_SRC_DIR" ]; then
            echo_warn "Incomplete Qt source detected, re-downloading..."
            rm -rf "$QT_SRC_DIR"
        fi
        echo_info "Downloading Qt source..."
        mkdir -p "$QT_BASE_DIR"
        aqt install-src mac "$QT_VERSION" --outputdir "$QT_BASE_DIR"
    fi

    # Configure - build for native architecture only
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
    echo_info "Building mushkin for $NATIVE_ARCH..."

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake "$PROJECT_ROOT" \
        -DSTATIC_BUILD=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_STATIC_DIR" \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX"

    cmake --build . --target mushkin -j"$NUM_CORES"
}

# ============================================================
# Bundle Dynamic Libraries into .app
# ============================================================

bundle_dylibs() {
    local app_bundle="$BUILD_DIR/mushkin.app"
    local frameworks_dir="$app_bundle/Contents/Frameworks"
    local binary="$app_bundle/Contents/MacOS/mushkin"
    local lib_dir="$app_bundle/Contents/MacOS/lib"

    if [ ! -d "$app_bundle" ]; then
        echo_error ".app bundle not found at $app_bundle"
        exit 1
    fi

    echo_info "Bundling dynamic libraries into .app..."
    mkdir -p "$frameworks_dir"

    # Homebrew prefix (Apple Silicon vs Intel)
    local brew_prefix
    if [ -d /opt/homebrew ]; then
        brew_prefix="/opt/homebrew"
    else
        brew_prefix="/usr/local"
    fi

    # Collect all dylibs referenced by the binary that come from Homebrew
    local homebrew_deps
    homebrew_deps=$(otool -L "$binary" | grep "$brew_prefix" | awk '{print $1}')

    if [ -z "$homebrew_deps" ]; then
        echo_info "No Homebrew dylibs found - binary may already be self-contained"
        return 0
    fi

    # Copy each Homebrew dylib to Frameworks/
    local copied_dylibs=()
    for dep in $homebrew_deps; do
        local real_path
        real_path=$(realpath "$dep")
        local dylib_name
        dylib_name=$(basename "$real_path")

        if [ ! -f "$frameworks_dir/$dylib_name" ]; then
            echo_info "  Copying $dylib_name"
            cp "$real_path" "$frameworks_dir/$dylib_name"
            chmod 755 "$frameworks_dir/$dylib_name"
            copied_dylibs+=("$dylib_name")
        fi
    done

    # Also find transitive dependencies (dylibs that reference other Homebrew dylibs)
    local found_new=true
    while $found_new; do
        found_new=false
        for fw in "$frameworks_dir"/*.dylib; do
            [ -f "$fw" ] || continue
            local transitive
            transitive=$(otool -L "$fw" | grep "$brew_prefix" | awk '{print $1}')
            for dep in $transitive; do
                local real_path
                real_path=$(realpath "$dep")
                local dylib_name
                dylib_name=$(basename "$real_path")
                if [ ! -f "$frameworks_dir/$dylib_name" ]; then
                    echo_info "  Copying transitive dep $dylib_name"
                    cp "$real_path" "$frameworks_dir/$dylib_name"
                    chmod 755 "$frameworks_dir/$dylib_name"
                    found_new=true
                fi
            done
        done
    done

    # Fix install names in Frameworks/ dylibs
    echo_info "Fixing install names..."
    for fw in "$frameworks_dir"/*.dylib; do
        [ -f "$fw" ] || continue
        local dylib_name
        dylib_name=$(basename "$fw")

        # Fix the dylib's own identity
        install_name_tool -id "@rpath/$dylib_name" "$fw" 2>/dev/null || true

        # Fix references to other Homebrew dylibs
        local refs
        refs=$(otool -L "$fw" | grep "$brew_prefix" | awk '{print $1}')
        for ref in $refs; do
            local ref_real
            ref_real=$(realpath "$ref")
            local ref_name
            ref_name=$(basename "$ref_real")
            install_name_tool -change "$ref" "@loader_path/$ref_name" "$fw" 2>/dev/null || true
        done
    done

    # Fix the main binary - replace Homebrew paths with @rpath
    echo_info "Fixing main binary references..."
    local binary_refs
    binary_refs=$(otool -L "$binary" | grep "$brew_prefix" | awk '{print $1}')
    for ref in $binary_refs; do
        local ref_real
        ref_real=$(realpath "$ref")
        local ref_name
        ref_name=$(basename "$ref_real")
        install_name_tool -change "$ref" "@rpath/$ref_name" "$binary" 2>/dev/null || true
    done

    # Fix Lua .so modules in Contents/MacOS/lib/
    echo_info "Fixing Lua module references..."
    if [ -d "$lib_dir" ]; then
        find "$lib_dir" -name "*.so" -type f | while read -r so_file; do
            local so_refs
            so_refs=$(otool -L "$so_file" | grep "$brew_prefix" | awk '{print $1}')
            for ref in $so_refs; do
                local ref_real
                ref_real=$(realpath "$ref")
                local ref_name
                ref_name=$(basename "$ref_real")
                # Lua modules are in Contents/MacOS/lib/ — Frameworks is at ../Frameworks/
                install_name_tool -change "$ref" "@executable_path/../Frameworks/$ref_name" "$so_file" 2>/dev/null || true
            done
        done
    fi

    # Ad-hoc codesign the entire bundle
    echo_info "Codesigning .app bundle..."
    codesign --force --deep --sign - "$app_bundle"

    echo_info "Dylib bundling complete"
}

# ============================================================
# Verify Build
# ============================================================

verify_build() {
    local app_bundle="$BUILD_DIR/mushkin.app"
    local binary="$app_bundle/Contents/MacOS/mushkin"

    if [ ! -f "$binary" ]; then
        echo_error "Build failed - binary not found"
        exit 1
    fi

    echo ""
    echo_info "=== Build Summary ==="

    # Check architecture
    local archs=$(lipo -archs "$binary" 2>/dev/null)
    echo "  Architecture: $archs"
    echo "  App bundle: $app_bundle"
    echo "  Binary size: $(du -h "$binary" | cut -f1)"

    # Show bundled frameworks
    local fw_dir="$app_bundle/Contents/Frameworks"
    if [ -d "$fw_dir" ]; then
        echo ""
        echo_info "Bundled frameworks:"
        ls -lh "$fw_dir"/*.dylib 2>/dev/null | awk '{print "  " $NF " (" $5 ")"}'
    fi

    # Show bundled Lua modules
    local lib_dir="$app_bundle/Contents/MacOS/lib"
    if [ -d "$lib_dir" ]; then
        echo ""
        echo_info "Bundled Lua modules:"
        find "$lib_dir" -name "*.so" -type f | while read -r f; do
            echo "  $(echo "$f" | sed "s|$lib_dir/||")"
        done
    fi

    echo ""

    # Verify no Homebrew paths remain
    echo_info "Checking for leaked Homebrew paths..."
    local leaked=false

    # Check main binary
    if otool -L "$binary" | grep -q "/opt/homebrew\|/usr/local/opt"; then
        echo_error "Main binary still references Homebrew paths:"
        otool -L "$binary" | grep "/opt/homebrew\|/usr/local/opt"
        leaked=true
    fi

    # Check Frameworks
    if [ -d "$fw_dir" ]; then
        for fw in "$fw_dir"/*.dylib; do
            [ -f "$fw" ] || continue
            if otool -L "$fw" | grep -q "/opt/homebrew\|/usr/local/opt"; then
                echo_error "$(basename "$fw") still references Homebrew paths:"
                otool -L "$fw" | grep "/opt/homebrew\|/usr/local/opt"
                leaked=true
            fi
        done
    fi

    # Check Lua modules
    if [ -d "$lib_dir" ]; then
        find "$lib_dir" -name "*.so" -type f | while read -r so_file; do
            if otool -L "$so_file" | grep -q "/opt/homebrew\|/usr/local/opt"; then
                echo_error "$(basename "$so_file") still references Homebrew paths:"
                otool -L "$so_file" | grep "/opt/homebrew\|/usr/local/opt"
                leaked=true
            fi
        done
    fi

    if [ "$leaked" = true ]; then
        echo_error "FAILED: Homebrew paths leaked into bundle"
        exit 1
    else
        echo_info "OK - no Homebrew paths found in bundle"
    fi
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
    bundle_dylibs
    verify_build

    echo ""
    echo_info "Build complete!"
    echo_info "App bundle: $BUILD_DIR/mushkin.app"
}

main "$@"
