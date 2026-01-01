#ifndef FONT_UTILS_H
#define FONT_UTILS_H

#include <QFont>
#include <QtGlobal>

/**
 * @brief Create a QFont with cross-platform Windows-compatible sizing
 *
 * World files store font sizes as point sizes assuming Windows 96 DPI.
 * On macOS (72 DPI logical), Qt would render these fonts ~25% smaller.
 *
 * This function converts the point size to pixel size on macOS to match
 * Windows rendering, while using native point sizing on Windows/Linux.
 *
 * @param family Font family name
 * @param pointSize Point size (as stored in world files)
 * @return QFont configured for cross-platform consistent rendering
 */
inline QFont createScaledFont(const QString& family, int pointSize)
{
    QFont font(family);

#ifdef Q_OS_MACOS
    // macOS uses 72 DPI logical, but world files assume Windows 96 DPI
    // Convert: pixelSize = pointSize * 96 / 72
    int pixelSize = qRound(pointSize * 96.0 / 72.0);
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);
#else
    // Windows/Linux: use point size directly
    font.setPointSize(pointSize > 0 ? pointSize : 1);
#endif

    return font;
}

/**
 * @brief Convert a point size to pixels for cross-platform consistency
 *
 * On macOS, converts Windows 96 DPI point sizes to pixel sizes.
 * On Windows/Linux, returns the point size unchanged (Qt handles DPI).
 *
 * Useful for stylesheet font-size values where we need pixels.
 *
 * @param pointSize Point size (as stored in world files)
 * @return Pixel size on macOS, point size on other platforms
 */
inline int scaledFontSize(int pointSize)
{
#ifdef Q_OS_MACOS
    return qRound(pointSize * 96.0 / 72.0);
#else
    return pointSize;
#endif
}

/**
 * @brief Create a QFont with cross-platform sizing (floating point version)
 *
 * Same as createScaledFont but accepts floating point sizes for
 * more precise font sizing (used by miniwindows).
 *
 * @param family Font family name
 * @param pointSize Point size as floating point
 * @return QFont configured for cross-platform consistent rendering
 */
inline QFont createScaledFontF(const QString& family, double pointSize)
{
    QFont font(family);

#ifdef Q_OS_MACOS
    // macOS uses 72 DPI logical, but world files assume Windows 96 DPI
    // Convert: pixelSize = pointSize * 96 / 72
    int pixelSize = qRound(pointSize * 96.0 / 72.0);
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);
#else
    // Windows/Linux: use point size directly
    font.setPointSizeF(pointSize > 0.0 ? pointSize : 1.0);
#endif

    return font;
}

#endif // FONT_UTILS_H
