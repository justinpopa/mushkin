/**
 * color_utils.h - Color format utilities
 *
 * Provides macros and helpers for working with BGR color format
 * used throughout MUSHclient compatibility code.
 *
 * Windows COLORREF format is 0x00BBGGRR (BGR byte order).
 * Qt uses 0xAARRGGBB (RGB with alpha). These utilities handle conversion.
 */

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <QColor>
#include <QRgb>

/**
 * BGR macro - Convert RGB values to BGR format
 *
 * Use this with human-readable RGB values. The macro handles byte swapping
 * to produce the BGR format expected by MUSHclient-compatible code.
 *
 * Example: BGR(255, 140, 0) produces orange in BGR format
 */
#define BGR(r, g, b) (static_cast<QRgb>((r) | ((g) << 8) | ((b) << 16)))

/**
 * bgrToQColor / bgrToColor - Convert BGR value to QColor
 *
 * @param bgr Color in 0x00BBGGRR format
 * @return QColor with correct RGB values
 */
inline QColor bgrToQColor(quint32 bgr)
{
    return QColor(bgr & 0xFF, (bgr >> 8) & 0xFF, (bgr >> 16) & 0xFF);
}

// Alias for compatibility with existing code
inline QColor bgrToColor(quint32 bgr) { return bgrToQColor(bgr); }

/**
 * bgrToQRgb - Convert BGR to QRgb (ARGB with full alpha)
 *
 * @param bgr Color in 0x00BBGGRR format
 * @return QRgb in 0xFFRRGGBB format
 */
inline QRgb bgrToQRgb(quint32 bgr)
{
    int r = bgr & 0xFF;
    int g = (bgr >> 8) & 0xFF;
    int b = (bgr >> 16) & 0xFF;
    return qRgba(r, g, b, 255);
}

/**
 * qRgbToBgr - Convert QRgb (ARGB) to BGR
 *
 * @param argb Color in 0xAARRGGBB format
 * @return Color in 0x00BBGGRR format
 */
inline quint32 qRgbToBgr(QRgb argb)
{
    return (qBlue(argb) << 16) | (qGreen(argb) << 8) | qRed(argb);
}

#endif // COLOR_UTILS_H
