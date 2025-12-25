/**
 * world_colors.cpp - Color Functions
 */

#include "lua_common.h"
#include <QColorDialog>

// Helper macro to create BGR color value (Windows COLORREF format: 0x00BBGGRR)
// This matches original MUSHclient for compatibility with existing plugins
#define BGR(r, g, b) (static_cast<quint32>((r) | ((g) << 8) | ((b) << 16)))

/**
 * ColourNameToRGB - Convert color name to BGR value
 *
 * Supports both X11/HTML color names ("red", "blue") and BGR integers.
 * Case-insensitive.
 *
 * Returns BGR format (0x00BBGGRR) to match original MUSHclient COLORREF format.
 * This ensures compatibility with existing plugins that use hardcoded color values.
 *
 * @param name Color name or empty string
 * @return BGR color value (0x00BBGGRR format)
 */
QRgb ColourNameToRGB(const QString& name)
{
    // If empty, return default (white)
    if (name.isEmpty()) {
        return BGR(255, 255, 255);
    }

    QString lowerName = name.toLower();

    // Standard X11/HTML color names - stored in BGR format for MUSHclient compatibility
    static const QMap<QString, quint32> colorMap = {
        // Basic colors
        {"black", BGR(0, 0, 0)},
        {"white", BGR(255, 255, 255)},
        {"red", BGR(255, 0, 0)},
        {"green", BGR(0, 128, 0)},
        {"blue", BGR(0, 0, 255)},
        {"yellow", BGR(255, 255, 0)},
        {"cyan", BGR(0, 255, 255)},
        {"magenta", BGR(255, 0, 255)},
        {"gray", BGR(128, 128, 128)},
        {"grey", BGR(128, 128, 128)},

        // Extended colors
        {"silver", BGR(192, 192, 192)},
        {"maroon", BGR(128, 0, 0)},
        {"purple", BGR(128, 0, 128)},
        {"fuchsia", BGR(255, 0, 255)},
        {"lime", BGR(0, 255, 0)},
        {"olive", BGR(128, 128, 0)},
        {"navy", BGR(0, 0, 128)},
        {"teal", BGR(0, 128, 128)},
        {"aqua", BGR(0, 255, 255)},

        // Light colors
        {"lightgray", BGR(211, 211, 211)},
        {"lightgrey", BGR(211, 211, 211)},
        {"lightred", BGR(255, 128, 128)},
        {"lightgreen", BGR(144, 238, 144)},
        {"lightblue", BGR(173, 216, 230)},
        {"lightyellow", BGR(255, 255, 224)},
        {"lightcyan", BGR(224, 255, 255)},
        {"lightmagenta", BGR(255, 128, 255)},

        // Dark colors
        {"darkgray", BGR(169, 169, 169)},
        {"darkgrey", BGR(169, 169, 169)},
        {"darkred", BGR(139, 0, 0)},
        {"darkgreen", BGR(0, 100, 0)},
        {"darkblue", BGR(0, 0, 139)},
        {"darkyellow", BGR(204, 204, 0)},
        {"darkcyan", BGR(0, 139, 139)},
        {"darkmagenta", BGR(139, 0, 139)},

        // Common names
        {"orange", BGR(255, 165, 0)},
        {"darkorange", BGR(255, 140, 0)},
        {"pink", BGR(255, 192, 203)},
        {"brown", BGR(165, 42, 42)},
        {"tan", BGR(210, 180, 140)},
        {"gold", BGR(255, 215, 0)},
        {"violet", BGR(238, 130, 238)},
        {"indigo", BGR(75, 0, 130)},
        {"azure", BGR(240, 255, 255)},
        {"beige", BGR(245, 245, 220)},
        {"coral", BGR(255, 127, 80)},
        {"crimson", BGR(220, 20, 60)},
        {"khaki", BGR(240, 230, 140)},
        {"lavender", BGR(230, 230, 250)},
        {"orchid", BGR(218, 112, 214)},
        {"peru", BGR(205, 133, 63)},
        {"plum", BGR(221, 160, 221)},
        {"salmon", BGR(250, 128, 114)},
        {"sienna", BGR(160, 82, 45)},
        {"wheat", BGR(245, 222, 179)},
    };

    if (colorMap.contains(lowerName)) {
        return colorMap[lowerName];
    }

    // Try parsing as hex number (e.g., "#FF0000" or "0xFF0000" or "FF0000")
    // Hex strings use HTML/CSS RGB format (#RRGGBB), need to convert to BGR
    QString hexStr = lowerName;
    if (hexStr.startsWith("#")) {
        hexStr = hexStr.mid(1);
    } else if (hexStr.startsWith("0x")) {
        hexStr = hexStr.mid(2);
    }

    bool ok;
    quint32 rgb = hexStr.toUInt(&ok, 16);
    if (ok) {
        // Convert from RGB (0x00RRGGBB) to BGR (0x00BBGGRR)
        int r = (rgb >> 16) & 0xFF;
        int g = (rgb >> 8) & 0xFF;
        int b = rgb & 0xFF;
        return BGR(r, g, b);
    }

    // Default to white if unknown
    return BGR(255, 255, 255);
}

/**
 * RGBColourToName - Convert BGR value to color name
 *
 * Takes a BGR integer and returns the corresponding color name if it's a
 * standard X11/HTML color, otherwise returns empty string.
 *
 * @param bgr BGR color value (0x00BBGGRR format - Windows COLORREF)
 * @return Color name string, or empty string if not a named color
 */
QString RGBColourToName(QRgb bgr)
{
    // Build reverse lookup map (BGR -> name)
    // Static so it's only built once
    static const QMap<quint32, QString> reverseColorMap = []() {
        QMap<quint32, QString> map;

        // Basic colors
        map[BGR(0, 0, 0)] = "black";
        map[BGR(255, 255, 255)] = "white";
        map[BGR(255, 0, 0)] = "red";
        map[BGR(0, 128, 0)] = "green";
        map[BGR(0, 0, 255)] = "blue";
        map[BGR(255, 255, 0)] = "yellow";
        map[BGR(0, 255, 255)] = "cyan";
        map[BGR(255, 0, 255)] = "magenta";
        map[BGR(128, 128, 128)] = "gray";

        // Extended colors
        map[BGR(192, 192, 192)] = "silver";
        map[BGR(128, 0, 0)] = "maroon";
        map[BGR(128, 0, 128)] = "purple";
        map[BGR(0, 255, 0)] = "lime";
        map[BGR(128, 128, 0)] = "olive";
        map[BGR(0, 0, 128)] = "navy";
        map[BGR(0, 128, 128)] = "teal";

        // Light colors
        map[BGR(211, 211, 211)] = "lightgray";
        map[BGR(255, 128, 128)] = "lightred";
        map[BGR(144, 238, 144)] = "lightgreen";
        map[BGR(173, 216, 230)] = "lightblue";
        map[BGR(255, 255, 224)] = "lightyellow";
        map[BGR(224, 255, 255)] = "lightcyan";
        map[BGR(255, 128, 255)] = "lightmagenta";

        // Dark colors
        map[BGR(169, 169, 169)] = "darkgray";
        map[BGR(139, 0, 0)] = "darkred";
        map[BGR(0, 100, 0)] = "darkgreen";
        map[BGR(0, 0, 139)] = "darkblue";
        map[BGR(204, 204, 0)] = "darkyellow";
        map[BGR(0, 139, 139)] = "darkcyan";
        map[BGR(139, 0, 139)] = "darkmagenta";

        // Common names
        map[BGR(255, 165, 0)] = "orange";
        map[BGR(255, 140, 0)] = "darkorange";
        map[BGR(255, 192, 203)] = "pink";
        map[BGR(165, 42, 42)] = "brown";
        map[BGR(210, 180, 140)] = "tan";
        map[BGR(255, 215, 0)] = "gold";
        map[BGR(238, 130, 238)] = "violet";
        map[BGR(75, 0, 130)] = "indigo";
        map[BGR(240, 255, 255)] = "azure";
        map[BGR(245, 245, 220)] = "beige";
        map[BGR(255, 127, 80)] = "coral";
        map[BGR(220, 20, 60)] = "crimson";
        map[BGR(240, 230, 140)] = "khaki";
        map[BGR(230, 230, 250)] = "lavender";
        map[BGR(218, 112, 214)] = "orchid";
        map[BGR(205, 133, 63)] = "peru";
        map[BGR(221, 160, 221)] = "plum";
        map[BGR(250, 128, 114)] = "salmon";
        map[BGR(160, 82, 45)] = "sienna";
        map[BGR(245, 222, 179)] = "wheat";

        return map;
    }();

    // Mask off alpha channel (use only color components)
    quint32 bgrOnly = bgr & 0x00FFFFFF;

    // Look up color name
    if (reverseColorMap.contains(bgrOnly)) {
        return reverseColorMap[bgrOnly];
    }

    // Not a named color - return empty string
    return QString();
}

/**
 * world.ColourNameToRGB(name)
 *
 * Converts color name to RGB value.
 */
int L_ColourNameToRGB(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    QRgb rgb = ColourNameToRGB(QString::fromUtf8(name));
    lua_pushinteger(L, (lua_Integer)rgb);
    return 1;
}

/**
 * world.RGBColourToName(rgb)
 *
 * Converts RGB value to color name.
 */
int L_RGBColourToName(lua_State* L)
{
    lua_Integer rgbValue = luaL_checkinteger(L, 1);
    QString name = RGBColourToName((QRgb)rgbValue);
    lua_pushstring(L, name.toUtf8().constData());
    return 1;
}

/**
 * world.GetNormalColour(whichColour)
 *
 * Gets normal ANSI color value (1-8).
 */
int L_GetNormalColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > 8) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_normalcolour[whichColour - 1]);
    return 1;
}

/**
 * world.GetBoldColour(whichColour)
 *
 * Gets bold ANSI color value (1-8).
 */
int L_GetBoldColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > 8) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_boldcolour[whichColour - 1]);
    return 1;
}

/**
 * world.SetNormalColour(whichColour, rgb)
 *
 * Sets normal ANSI color value (1-8).
 */
int L_SetNormalColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > 8)
        return 0;

    pDoc->m_normalcolour[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetBoldColour(whichColour, rgb)
 *
 * Sets bold ANSI color value (1-8).
 */
int L_SetBoldColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > 8)
        return 0;

    pDoc->m_boldcolour[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetCustomColourText(whichColour, rgb)
 *
 * Sets custom text color value (1-16).
 */
int L_SetCustomColourText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > MAX_CUSTOM)
        return 0;

    pDoc->m_customtext[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetCustomColourBackground(whichColour, rgb)
 *
 * Sets custom background color value (1-16).
 */
int L_SetCustomColourBackground(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > MAX_CUSTOM)
        return 0;

    pDoc->m_customback[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.GetCustomColourText(whichColour)
 *
 * Gets custom text color value (1-16).
 *
 * @param whichColour Index of custom color (1-16)
 * @return RGB color value, or 0 if out of range
 */
int L_GetCustomColourText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > MAX_CUSTOM) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_customtext[whichColour - 1]);
    return 1;
}

/**
 * world.GetCustomColourBackground(whichColour)
 *
 * Gets custom background color value (1-16).
 *
 * @param whichColour Index of custom color (1-16)
 * @return RGB color value, or 0 if out of range
 */
int L_GetCustomColourBackground(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > MAX_CUSTOM) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_customback[whichColour - 1]);
    return 1;
}

/**
 * world.SetCustomColourName(whichColour, name)
 *
 * Sets the display name for a custom color slot.
 *
 * Based on methods_colours.cpp
 *
 * @param whichColour Index of custom color (1-16)
 * @param name Name for the color (1-30 characters)
 * @return eOK (0) on success, error code otherwise
 */
int L_SetCustomColourName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint16 whichColour = luaL_checkinteger(L, 1);
    size_t len;
    const char* name = luaL_checklstring(L, 2, &len);
    QString qName = QString::fromUtf8(name, len);

    qint32 result = pDoc->SetCustomColourName(whichColour, qName);
    lua_pushinteger(L, result);
    return 1;
}

/**
 * world.PickColour(suggested)
 *
 * Opens a color picker dialog for the user to select a color.
 *
 * Based on methods_colours.cpp
 *
 * @param suggested Suggested color (BGR value - Windows COLORREF), or -1 for no suggestion
 * @return Selected BGR color value, or -1 if canceled
 */
int L_PickColour(lua_State* L)
{
    qint32 suggested = luaL_checkinteger(L, 1);

    // Set initial color if suggested
    // Input is BGR format (0x00BBGGRR), need to convert to QColor
    QColor initial = Qt::white;
    if (suggested != -1) {
        quint32 bgr = suggested & 0x00FFFFFF;
        int r = bgr & 0xFF;
        int g = (bgr >> 8) & 0xFF;
        int b = (bgr >> 16) & 0xFF;
        initial = QColor(r, g, b);
    }

    // Open color picker dialog
    QColor color = QColorDialog::getColor(initial, nullptr, "Pick a Colour");

    // Return -1 if canceled, otherwise return BGR value
    if (!color.isValid()) {
        lua_pushinteger(L, -1);
    } else {
        // Convert QColor to BGR format (0x00BBGGRR)
        quint32 bgr = color.red() | (color.green() << 8) | (color.blue() << 16);
        lua_pushinteger(L, bgr);
    }

    return 1;
}

/**
 * world.AdjustColour(colour, method)
 *
 * Adjusts a color value using various methods.
 *
 * Based on Utilities.cpp
 *
 * @param colour BGR color value (0x00BBGGRR format - Windows COLORREF)
 * @param method Adjustment method:
 *   - 0: No change
 *   - 1: Invert
 *   - 2: Lighter (increase luminance by 2%)
 *   - 3: Darker (decrease luminance by 2%)
 *   - 4: Less saturation (decrease by 5%)
 *   - 5: More saturation (increase by 5%)
 * @return Adjusted BGR color value
 */
int L_AdjustColour(lua_State* L)
{
    qint32 colour = luaL_checkinteger(L, 1);
    int method = luaL_checkinteger(L, 2);

    // Mask to BGR only (remove alpha) and convert to RGB components
    quint32 bgr = colour & 0x00FFFFFF;
    int r = bgr & 0xFF;
    int g = (bgr >> 8) & 0xFF;
    int b = (bgr >> 16) & 0xFF;
    QColor qcolor(r, g, b);

    // Helper lambda to convert QColor to BGR
    auto toBgr = [](const QColor& c) -> quint32 {
        return c.red() | (c.green() << 8) | (c.blue() << 16);
    };

    switch (method) {
        case 1: // Invert
        {
            lua_pushinteger(L, BGR(255 - r, 255 - g, 255 - b));
            return 1;
        }

        case 2: // Lighter (increase luminance)
        {
            float h, s, l;
            qcolor.getHslF(&h, &s, &l);
            l += 0.02f;
            if (l > 1.0f)
                l = 1.0f;
            QColor adjusted = QColor::fromHslF(h, s, l);
            lua_pushinteger(L, toBgr(adjusted));
            return 1;
        }

        case 3: // Darker (decrease luminance)
        {
            float h, s, l;
            qcolor.getHslF(&h, &s, &l);
            l -= 0.02f;
            if (l < 0.0f)
                l = 0.0f;
            QColor adjusted = QColor::fromHslF(h, s, l);
            lua_pushinteger(L, toBgr(adjusted));
            return 1;
        }

        case 4: // Less saturation
        {
            float h, s, l;
            qcolor.getHslF(&h, &s, &l);
            s -= 0.05f;
            if (s < 0.0f)
                s = 0.0f;
            QColor adjusted = QColor::fromHslF(h, s, l);
            lua_pushinteger(L, toBgr(adjusted));
            return 1;
        }

        case 5: // More saturation
        {
            float h, s, l;
            qcolor.getHslF(&h, &s, &l);
            s += 0.05f;
            if (s > 1.0f)
                s = 1.0f;
            QColor adjusted = QColor::fromHslF(h, s, l);
            lua_pushinteger(L, toBgr(adjusted));
            return 1;
        }

        default: // No change
            lua_pushinteger(L, bgr);
            return 1;
    }
}

// ========== Registration ==========

void register_world_colors_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"ColourNameToRGB", L_ColourNameToRGB};
    *ptr++ = {"RGBColourToName", L_RGBColourToName};
    *ptr++ = {"GetNormalColour", L_GetNormalColour};
    *ptr++ = {"GetBoldColour", L_GetBoldColour};
    *ptr++ = {"GetCustomColourText", L_GetCustomColourText};
    *ptr++ = {"GetCustomColourBackground", L_GetCustomColourBackground};
    *ptr++ = {"SetCustomColourName", L_SetCustomColourName};
    *ptr++ = {"PickColour", L_PickColour};
    *ptr++ = {"AdjustColour", L_AdjustColour};
}
