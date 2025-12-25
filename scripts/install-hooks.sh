#!/bin/bash
#
# Install git hooks for the project
#
# This script copies pre-commit hooks to .git/hooks/ directory
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$PROJECT_ROOT/.git/hooks"

echo "Installing git hooks..."

# Check if .git directory exists
if [ ! -d "$PROJECT_ROOT/.git" ]; then
    echo "Error: Not a git repository"
    exit 1
fi

# Create hooks directory if it doesn't exist
mkdir -p "$HOOKS_DIR"

# Install pre-commit hook
if [ -f "$SCRIPT_DIR/pre-commit" ]; then
    cp "$SCRIPT_DIR/pre-commit" "$HOOKS_DIR/pre-commit"
    chmod +x "$HOOKS_DIR/pre-commit"
    echo "✓ Installed pre-commit hook"
else
    echo "Warning: pre-commit hook not found at $SCRIPT_DIR/pre-commit"
fi

echo ""
echo "Git hooks installed successfully!"
echo ""
echo "The pre-commit hook will check code formatting before each commit."
echo "To skip the hook for a specific commit, use: git commit --no-verify"
echo ""
