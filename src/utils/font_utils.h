#ifndef FONT_UTILS_H
#define FONT_UTILS_H

#include "logging.h"

#include <QFont>
#include <QFontInfo>
#include <QGuiApplication>
#include <QScreen>

/**
 * @brief Create a QFont from a world-file point size
 *
 * MUSHclient world files store font sizes as typographic point sizes.
 * The original MUSHclient converts these to GDI pixels at runtime via:
 *   lfHeight = -MulDiv(pointSize, GetDeviceCaps(LOGPIXELSY), 72)
 *
 * Qt's setPointSize() performs the same conversion using the screen's
 * logical DPI, so it should produce correct results on all platforms
 * without manual DPI compensation.
 *
 * @param family Font family name
 * @param pointSize Point size (as stored in world files)
 * @return QFont configured with the given point size
 */
inline QFont createScaledFont(const QString& family, int pointSize)
{
    QFont font(family);
    font.setPointSize(pointSize > 0 ? pointSize : 1);

    // Diagnostic: log what Qt resolved so we can verify cross-platform behavior
    if (auto* screen = QGuiApplication::primaryScreen()) {
        qCDebug(lcFont) << "createScaledFont:" << family << pointSize << "pt"
                        << "-> screenDPI:" << screen->logicalDotsPerInchY()
                        << "resolvedPixelSize:" << QFontInfo(font).pixelSize();
    }

    return font;
}

/**
 * @brief Get the pixel size for a world-file point size
 *
 * Converts a point size to the pixel value Qt would use on the current
 * screen. Useful for stylesheet font-size values where pixels are needed.
 *
 * @param pointSize Point size (as stored in world files)
 * @return Resolved pixel size for the current screen DPI
 */
inline int scaledFontSize(int pointSize)
{
    if (auto* screen = QGuiApplication::primaryScreen()) {
        return qRound(pointSize * screen->logicalDotsPerInchY() / 72.0);
    }
    // Fallback: assume 96 DPI (Windows default)
    return qRound(pointSize * 96.0 / 72.0);
}

/**
 * @brief Create a QFont from a world-file point size (floating point version)
 *
 * Same as createScaledFont but accepts floating point sizes for
 * more precise font sizing (used by miniwindows).
 *
 * @param family Font family name
 * @param pointSize Point size as floating point
 * @return QFont configured with the given point size
 */
inline QFont createScaledFontF(const QString& family, double pointSize)
{
    QFont font(family);
    font.setPointSizeF(pointSize > 0.0 ? pointSize : 1.0);

    if (auto* screen = QGuiApplication::primaryScreen()) {
        qCDebug(lcFont) << "createScaledFontF:" << family << pointSize << "pt"
                        << "-> screenDPI:" << screen->logicalDotsPerInchY()
                        << "resolvedPixelSize:" << QFontInfo(font).pixelSize();
    }

    return font;
}

#endif // FONT_UTILS_H
