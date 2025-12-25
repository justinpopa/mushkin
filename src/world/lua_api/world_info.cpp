/**
 * world_info.cpp - World Information and Statistics Functions
 */

#include "lua_common.h"
#include <QApplication>
#include <QDateTime>
#include <QScreen>
#include <QStyle>
#include <QWidget>

// MUSHclient Qt version string
// TODO: Generate from CMake PROJECT_VERSION
#define MUSHCLIENT_QT_VERSION "0.0.4"

/**
 * world.GetLineCount()
 *
 * Gets the total number of lines received from the MUD.
 *
 * Based on methods_info.cpp
 *
 * @return Number of lines received
 */
int L_GetLineCount(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->m_total_lines);
    return 1;
}

/**
 * world.GetSentBytes()
 *
 * Gets the total number of bytes sent to the MUD.
 *
 * Based on methods_info.cpp
 *
 * @return Number of bytes sent
 */
int L_GetSentBytes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->m_nBytesOut);
    return 1;
}

/**
 * world.GetReceivedBytes()
 *
 * Gets the total number of bytes received from the MUD.
 *
 * Based on methods_info.cpp
 *
 * @return Number of bytes received
 */
int L_GetReceivedBytes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->m_nBytesIn);
    return 1;
}

/**
 * world.GetConnectDuration()
 *
 * Gets the number of seconds connected to the MUD.
 * Returns 0 if not currently connected.
 *
 * Based on methods_info.cpp
 *
 * @return Seconds connected (0 if not connected)
 */
int L_GetConnectDuration(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        lua_pushnumber(L, 0);
        return 1;
    }

    // Calculate seconds since connection
    qint64 msecs = pDoc->m_tConnectTime.msecsTo(QDateTime::currentDateTime());
    qint64 seconds = msecs / 1000;

    lua_pushnumber(L, seconds);
    return 1;
}

/**
 * world.WorldAddress()
 *
 * Gets the MUD server address (hostname or IP).
 *
 * Based on methods_info.cpp
 *
 * @return Server address string
 */
int L_WorldAddress(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QByteArray ba = pDoc->m_server.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * world.WorldPort()
 *
 * Gets the MUD server port number.
 *
 * Based on methods_info.cpp
 *
 * @return Port number
 */
int L_WorldPort(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->m_port);
    return 1;
}

/**
 * world.WorldName()
 *
 * Gets the world name. This is an alias for GetWorldName()
 * for backward compatibility with original MUSHclient.
 *
 * Based on methods_info.cpp
 *
 * @return World name string
 */
int L_WorldName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QByteArray ba = pDoc->m_mush_name.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * world.Version()
 *
 * Gets the MUSHclient version string.
 *
 * Based on methods_info.cpp
 *
 * @return Version string
 */
int L_Version(lua_State* L)
{
    lua_pushstring(L, MUSHCLIENT_QT_VERSION);
    return 1;
}

/**
 * world.GetLinesInBufferCount()
 *
 * Gets the actual number of lines currently in the output buffer.
 * This is different from GetLineCount() which returns total lines received.
 *
 * Based on methods_info.cpp
 *
 * @return Number of lines currently in scrollback buffer
 */
int L_GetLinesInBufferCount(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->m_lineList.size());
    return 1;
}

/**
 * world.GetSysColor(index)
 *
 * Gets a system color value by index.
 * Maps Windows system color indices to Qt palette colors.
 *
 * Based on methods_info.cpp
 *
 * @param index System color index (Windows COLOR_* constants)
 * @return RGB color value (0xRRGGBB)
 */
int L_GetSysColor(lua_State* L)
{
    int index = luaL_checkinteger(L, 1);

    // Map common Windows system color indices to Qt palette roles
    // Reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsyscolor
    QPalette palette = QApplication::palette();
    QColor color;

    switch (index) {
        case 0: // COLOR_SCROLLBAR
            color = palette.color(QPalette::Button);
            break;
        case 1: // COLOR_BACKGROUND (desktop)
            color = palette.color(QPalette::Window);
            break;
        case 2: // COLOR_ACTIVECAPTION
            color = palette.color(QPalette::Highlight);
            break;
        case 3: // COLOR_INACTIVECAPTION
            color = palette.color(QPalette::Mid);
            break;
        case 4: // COLOR_MENU
            color = palette.color(QPalette::Base);
            break;
        case 5: // COLOR_WINDOW
            color = palette.color(QPalette::Window);
            break;
        case 6: // COLOR_WINDOWFRAME
            color = palette.color(QPalette::WindowText);
            break;
        case 7: // COLOR_MENUTEXT
            color = palette.color(QPalette::Text);
            break;
        case 8: // COLOR_WINDOWTEXT
            color = palette.color(QPalette::WindowText);
            break;
        case 9: // COLOR_CAPTIONTEXT
            color = palette.color(QPalette::HighlightedText);
            break;
        case 13: // COLOR_HIGHLIGHT
            color = palette.color(QPalette::Highlight);
            break;
        case 14: // COLOR_HIGHLIGHTTEXT
            color = palette.color(QPalette::HighlightedText);
            break;
        case 15: // COLOR_BTNFACE
            color = palette.color(QPalette::Button);
            break;
        case 16: // COLOR_BTNSHADOW
            color = palette.color(QPalette::Dark);
            break;
        case 17: // COLOR_GRAYTEXT
            color = palette.color(QPalette::Disabled, QPalette::Text);
            break;
        case 18: // COLOR_BTNTEXT
            color = palette.color(QPalette::ButtonText);
            break;
        default:
            // Unknown index - return black
            color = QColor(0, 0, 0);
            break;
    }

    // Return as 0xRRGGBB (Windows format, no alpha)
    lua_pushnumber(L, (color.red() << 16) | (color.green() << 8) | color.blue());
    return 1;
}

/**
 * world.GetSystemMetrics(index)
 *
 * Gets a system metric value by index.
 * Maps Windows system metric indices to Qt screen/widget metrics.
 *
 * Based on methods_info.cpp
 *
 * @param index System metric index (Windows SM_* constants)
 * @return Metric value in pixels
 */
int L_GetSystemMetrics(lua_State* L)
{
    int index = luaL_checkinteger(L, 1);

    // Map common Windows system metric indices to Qt equivalents
    // Reference:
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) {
        lua_pushnumber(L, 0);
        return 1;
    }

    QSize screenSize = screen->size();
    qreal dpi = screen->logicalDotsPerInch();

    int value = 0;
    switch (index) {
        case 0: // SM_CXSCREEN - Screen width
            value = screenSize.width();
            break;
        case 1: // SM_CYSCREEN - Screen height
            value = screenSize.height();
            break;
        case 2: // SM_CXVSCROLL - Vertical scroll bar width
            value = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            break;
        case 3: // SM_CYHSCROLL - Horizontal scroll bar height
            value = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            break;
        case 4: // SM_CYCAPTION - Caption bar height
            value = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
            break;
        case 5: // SM_CXBORDER - Window border width
            value = QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
            break;
        case 6: // SM_CYBORDER - Window border height
            value = QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
            break;
        case 15: // SM_CXMIN - Minimum window width
            value = 100;
            break;
        case 16: // SM_CYMIN - Minimum window height
            value = 100;
            break;
        case 61: // SM_CXMAXIMIZED - Maximized window width
            value = screenSize.width();
            break;
        case 62: // SM_CYMAXIMIZED - Maximized window height
            value = screenSize.height();
            break;
        default:
            value = 0;
            break;
    }

    lua_pushnumber(L, value);
    return 1;
}

/**
 * world.GetDeviceCaps(index)
 *
 * Gets device capabilities by index.
 * Maps Windows device capability indices to Qt screen metrics.
 *
 * Based on methods_info.cpp
 *
 * @param index Device capability index (Windows *DPI, *RES constants)
 * @return Capability value
 */
int L_GetDeviceCaps(lua_State* L)
{
    int index = luaL_checkinteger(L, 1);

    // Map Windows device capability indices to Qt equivalents
    // Reference: methods_info.cpp
    // Reference: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getdevicecaps
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) {
        lua_pushnumber(L, 0);
        return 1;
    }

    QSize screenSize = screen->size();
    QSizeF physicalSize = screen->physicalSize(); // in millimeters
    qreal dpi = screen->logicalDotsPerInch();
    int depth = screen->depth(); // bits per pixel

    int value = 0;
    switch (index) {
        case 0:        // DRIVERVERSION - Device driver version
            value = 1; // Stub value
            break;
        case 2:        // TECHNOLOGY - Device classification
            value = 8; // DT_RASDISPLAY (raster display)
            break;
        case 4: // HORZSIZE - Horizontal size in millimeters
            value = (int)physicalSize.width();
            break;
        case 6: // VERTSIZE - Vertical size in millimeters
            value = (int)physicalSize.height();
            break;
        case 8: // HORZRES - Horizontal resolution (pixels)
            value = screenSize.width();
            break;
        case 10: // VERTRES - Vertical resolution (pixels)
            value = screenSize.height();
            break;
        case 12: // BITSPIXEL - Number of bits per pixel
            value = depth;
            break;
        case 14:       // PLANES - Number of planes
            value = 1; // Modern displays use 1 plane
            break;
        case 16:       // NUMBRUSHES - Number of brushes the device has
            value = 0; // Not applicable to modern displays
            break;
        case 18:       // NUMPENS - Number of pens the device has
            value = 0; // Not applicable to modern displays
            break;
        case 20:       // NUMMARKERS - Number of markers the device has
            value = 0; // Not applicable to modern displays
            break;
        case 22:       // NUMFONTS - Number of fonts the device has
            value = 0; // TrueType fonts are scalable
            break;
        case 24: // NUMCOLORS - Number of colors the device supports
            // For > 8bpp, this is -1 (unlimited)
            value = (depth > 8) ? -1 : (1 << depth);
            break;
        case 26:       // PDEVICESIZE - Size required for device descriptor
            value = 0; // Not applicable
            break;
        case 28:          // CURVECAPS - Curve capabilities
            value = 0xFF; // All curve capabilities
            break;
        case 30:          // LINECAPS - Line capabilities
            value = 0xFF; // All line capabilities
            break;
        case 32:          // POLYGONALCAPS - Polygonal capabilities
            value = 0xFF; // All polygon capabilities
            break;
        case 34:            // TEXTCAPS - Text capabilities
            value = 0x0001; // TC_OP_CHARACTER (can do character precision)
            break;
        case 36:       // CLIPCAPS - Clipping capabilities
            value = 1; // CP_RECTANGLE (can clip to rectangles)
            break;
        case 38:            // RASTERCAPS - Bitblt capabilities
            value = 0x2000; // RC_BITBLT (supports bitblt)
            break;
        case 40: // ASPECTX - Length of the X leg
            value = (int)dpi;
            break;
        case 42: // ASPECTY - Length of the Y leg
            value = (int)dpi;
            break;
        case 44:                        // ASPECTXY - Length of the hypotenuse
            value = (int)(dpi * 1.414); // sqrt(2) * dpi
            break;
        case 45:          // SHADEBLENDCAPS - Shading and blending caps
            value = 0x03; // SB_CONST_ALPHA | SB_PIXEL_ALPHA
            break;
        case 88: // LOGPIXELSX - Logical pixels per inch (horizontal)
            value = (int)dpi;
            break;
        case 90: // LOGPIXELSY - Logical pixels per inch (vertical)
            value = (int)dpi;
            break;
        case 104: // SIZEPALETTE - Number of entries in physical palette
            value = (depth <= 8) ? (1 << depth) : 0;
            break;
        case 106:       // NUMRESERVED - Number of reserved entries in palette
            value = 20; // Windows reserves 20 colors
            break;
        case 108: // COLORRES - Actual color resolution
            value = depth;
            break;
        case 116: // VREFRESH - Current vertical refresh rate (Hz)
            value = (int)screen->refreshRate();
            break;
        case 117: // DESKTOPVERTRES - Desktop vertical resolution
            value = screenSize.height();
            break;
        case 118: // DESKTOPHORZRES - Desktop horizontal resolution
            value = screenSize.width();
            break;
        case 119:      // BLTALIGNMENT - Preferred blt alignment
            value = 4; // 4-byte alignment is typical
            break;
        default:
            value = 0;
            break;
    }

    lua_pushnumber(L, value);
    return 1;
}

/**
 * world.GetFrame()
 *
 * Gets the native window handle/ID for the main application window.
 * This can be used for platform-specific window operations.
 *
 * Based on methods_output.cpp
 *
 * On Windows, this would return the HWND. In Qt, it returns the winId().
 *
 * @return Window ID as a light userdata pointer
 */
int L_GetFrame(lua_State* L)
{
    // Get the main application window
    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        // Fall back to first top-level window if no active window
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        if (!topLevelWidgets.isEmpty()) {
            mainWindow = topLevelWidgets.first();
        }
    }

    if (mainWindow) {
        // Return the native window ID as light userdata
        WId windowId = mainWindow->winId();
        lua_pushlightuserdata(L, (void*)(quintptr)windowId);
    } else {
        // No window found
        lua_pushlightuserdata(L, nullptr);
    }

    return 1;
}

/**
 * world.GetSelectionStartLine()
 *
 * Gets the line number where the text selection starts in the output window.
 *
 * Based on methods_info.cpp
 *
 * @return Line number (1-based) where selection starts, or 0 if no selection
 */
int L_GetSelectionStartLine(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->GetSelectionStartLine());
    return 1;
}

/**
 * world.GetSelectionEndLine()
 *
 * Gets the line number where the text selection ends in the output window.
 *
 * Based on methods_info.cpp
 *
 * @return Line number (1-based) where selection ends, or 0 if no selection
 */
int L_GetSelectionEndLine(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->GetSelectionEndLine());
    return 1;
}

/**
 * world.GetSelectionStartColumn()
 *
 * Gets the column where the text selection starts in the output window.
 *
 * Based on methods_info.cpp
 *
 * @return Column (1-based) where selection starts, or 0 if no selection
 */
int L_GetSelectionStartColumn(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->GetSelectionStartColumn());
    return 1;
}

/**
 * world.GetSelectionEndColumn()
 *
 * Gets the column where the text selection ends in the output window.
 *
 * Based on methods_info.cpp
 *
 * @return Column (1-based) where selection ends, or 0 if no selection
 */
int L_GetSelectionEndColumn(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, pDoc->GetSelectionEndColumn());
    return 1;
}

// ========== Registration ==========

void register_world_info_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"GetLineCount", L_GetLineCount};
    *ptr++ = {"GetSentBytes", L_GetSentBytes};
    *ptr++ = {"GetReceivedBytes", L_GetReceivedBytes};
    *ptr++ = {"GetConnectDuration", L_GetConnectDuration};
    *ptr++ = {"WorldAddress", L_WorldAddress};
    *ptr++ = {"WorldPort", L_WorldPort};
    *ptr++ = {"WorldName", L_WorldName};
    *ptr++ = {"Version", L_Version};
    *ptr++ = {"GetLinesInBufferCount", L_GetLinesInBufferCount};
    *ptr++ = {"GetSysColor", L_GetSysColor};
    *ptr++ = {"GetSystemMetrics", L_GetSystemMetrics};
    *ptr++ = {"GetDeviceCaps", L_GetDeviceCaps};
    *ptr++ = {"GetFrame", L_GetFrame};
    *ptr++ = {"GetSelectionStartLine", L_GetSelectionStartLine};
    *ptr++ = {"GetSelectionEndLine", L_GetSelectionEndLine};
    *ptr++ = {"GetSelectionStartColumn", L_GetSelectionStartColumn};
    *ptr++ = {"GetSelectionEndColumn", L_GetSelectionEndColumn};
}
