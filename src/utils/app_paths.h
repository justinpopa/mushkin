#pragma once

#include <QString>

namespace AppPaths {

// Returns the base directory for application data (worlds, lua, plugins, etc.)
//
// On macOS .app bundles, this returns the directory containing the .app bundle,
// allowing users to place mushkin.app alongside their worlds/ and lua/ folders.
//
// On standalone binaries (all platforms), this returns the directory containing
// the executable.
QString getAppDirectory();

// Returns the directory containing the actual executable binary.
// Use this for resources that must be alongside the binary itself (e.g., bundled
// C libraries in lib/).
QString getExecutableDirectory();

} // namespace AppPaths
