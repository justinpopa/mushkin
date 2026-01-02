/**
 * test_qt_static.h - Qt static build support for tests
 *
 * For static Qt builds, platform plugins must be explicitly imported.
 * Include this header in test files that use QCoreApplication/QApplication.
 */

#ifndef TEST_QT_STATIC_H
#define TEST_QT_STATIC_H

#include <QtGlobal>

// Import the offscreen platform plugin for static Linux builds
// Linux CI is headless and needs this. macOS CI has a display and uses Cocoa.
#if defined(QT_STATIC) && defined(__linux__)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QOffscreenIntegrationPlugin)
#endif

#endif // TEST_QT_STATIC_H
