#ifndef FONT_UTILS_H
#define FONT_UTILS_H

#include "logging.h"

#include <QFont>
#include <QFontInfo>
#include <QGuiApplication>
#include <QScreen>

/**
 * MUSHclient-compatible DPI constant.
 *
 * MUSHclient on Windows renders fonts at 96 DPI:
 *   lfHeight = -MulDiv(pointSize, 96, 72)
 *
 * All world files and plugins assume this DPI. macOS reports 72 DPI via Qt,
 * which makes fonts 25% smaller. We hard-code 96 DPI so fonts render at the
 * same pixel size as the original on all platforms.
 */
inline constexpr double kMushclientDpi = 96.0;

/**
 * @brief Create a QFont from a world-file point size
 *
 * Uses setPixelSize() with the Windows 96 DPI conversion so fonts render
 * at the same pixel size as the original MUSHclient on all platforms.
 *
 * @param family Font family name
 * @param pointSize Point size (as stored in world files)
 * @return QFont configured with the equivalent pixel size
 */
inline QFont createScaledFont(const QString& family, int pointSize)
{
    QFont font(family);
    int pixelSize = qRound(pointSize * kMushclientDpi / 72.0);
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);

    qCDebug(lcFont) << "createScaledFont:" << family << pointSize << "pt"
                    << "-> pixelSize:" << pixelSize;

    return font;
}

/**
 * @brief Get the pixel size for a world-file point size
 *
 * Converts a point size to the pixel value that MUSHclient would use
 * at 96 DPI. Useful for stylesheet font-size values where pixels are needed.
 *
 * @param pointSize Point size (as stored in world files)
 * @return Pixel size equivalent at 96 DPI
 */
inline int scaledFontSize(int pointSize)
{
    return qRound(pointSize * kMushclientDpi / 72.0);
}

/**
 * @brief Create a QFont from a world-file point size (floating point version)
 *
 * Same as createScaledFont but accepts floating point sizes for
 * more precise font sizing (used by miniwindows).
 *
 * @param family Font family name
 * @param pointSize Point size as floating point
 * @return QFont configured with the equivalent pixel size
 */
inline QFont createScaledFontF(const QString& family, double pointSize)
{
    QFont font(family);
    int pixelSize = qRound(pointSize * kMushclientDpi / 72.0);
    font.setPixelSize(pixelSize > 0 ? pixelSize : 1);

    qCDebug(lcFont) << "createScaledFontF:" << family << pointSize << "pt"
                    << "-> pixelSize:" << pixelSize;

    return font;
}

#endif // FONT_UTILS_H
