#!/bin/bash
# Deploy mushkin.app to a target directory with Qt frameworks bundled
# Usage: ./scripts/deploy-macos.sh [target_dir]
# Default target: ~/Desktop/Mushkin

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
TARGET_DIR="${1:-$HOME/Desktop/Mushkin}"

# Qt location
QT_DIR="$HOME/.cache/mushkin/Qt/6.9.3/macos"

# Check prerequisites
if [[ ! -d "$BUILD_DIR/mushkin.app" ]]; then
    echo "Error: mushkin.app not found in $BUILD_DIR"
    echo "Run 'cmake --build build' first"
    exit 1
fi

if [[ ! -f "$QT_DIR/bin/macdeployqt" ]]; then
    echo "Error: macdeployqt not found at $QT_DIR/bin/macdeployqt"
    echo "Run './scripts/install-qt-dev.sh' first"
    exit 1
fi

# Create target directory
mkdir -p "$TARGET_DIR"

# Remove old app bundle
echo "Removing old app bundle..."
rm -rf "$TARGET_DIR/mushkin.app"

# Copy new app bundle (use ditto to strip resource forks)
echo "Copying mushkin.app to $TARGET_DIR..."
ditto --noextattr --norsrc "$BUILD_DIR/mushkin.app" "$TARGET_DIR/mushkin.app"

# Run macdeployqt to bundle Qt frameworks
echo "Bundling Qt frameworks..."
"$QT_DIR/bin/macdeployqt" "$TARGET_DIR/mushkin.app" -always-overwrite 2>&1 | grep -v "^ERROR:" || true

# Clean resource forks and extended attributes (macdeployqt adds these)
echo "Cleaning extended attributes..."
# Copy to temp location stripping all resource forks, then move back
TEMP_APP=$(mktemp -d)/mushkin.app
ditto --noextattr --norsrc "$TARGET_DIR/mushkin.app" "$TEMP_APP"
rm -rf "$TARGET_DIR/mushkin.app"
mv "$TEMP_APP" "$TARGET_DIR/mushkin.app"
rmdir "$(dirname "$TEMP_APP")" 2>/dev/null || true

# Ad-hoc codesign
echo "Signing app bundle..."
codesign --force --deep --sign - "$TARGET_DIR/mushkin.app"

echo ""
echo "Deployed to: $TARGET_DIR/mushkin.app"
