#ifndef FONT_UTILS_H
#define FONT_UTILS_H

#include <QFont>
#include <QtGlobal>

/**
 * @brief Create a QFont with Windows-compatible sizing
 *
 * MUSHclient world files store font sizes as point sizes assuming Windows 96 DPI.
 * On macOS (72 DPI logical), Qt would render these fonts ~25% smaller.
 *
 * This function converts the point size to pixel size on macOS to match
 * Windows rendering, while using native point sizing on Windows/Linux.
 *
 * @param family Font family name
 * @param pointSize Point size (as stored in MUSHclient world files)
 * @return QFont configured for Windows-compatible rendering
 */
inline QFont createMUSHclientFont(const QString& family, int pointSize)
{
    QFont font(family);

#ifdef Q_OS_MACOS
    // macOS uses 72 DPI logical, but MUSHclient assumes Windows 96 DPI
    // Convert: pixelSize = pointSize * 96 / 72
    int pixelSize = qRound(pointSize * 96.0 / 72.0);
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);
#else
    // Windows/Linux: use point size directly
    font.setPointSize(pointSize > 0 ? pointSize : 1);
#endif

    return font;
}

#endif // FONT_UTILS_H
