/**
 * world_colors.cpp - Color Functions
 *
 * Functions for color conversion, manipulation, and palette management.
 * Colors use BGR format (0x00BBGGRR) for MUSHclient compatibility.
 */

#include "../color_utils.h"
#include "lua_common.h"
#include <QColorDialog>
#include <cstdint>

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
    // Empty name: original returns uninitialized value (UB in methods_colours.cpp:29-34).
    // Return -1 (error indicator) as the safest defined behavior.
    if (name.trimmed().isEmpty()) {
        return static_cast<QRgb>(-1);
    }

    QString lowerName = name.toLower();

    // Standard X11/HTML color names - stored in BGR format for MUSHclient compatibility
    static const QMap<QString, quint32> colorMap = {
        {"aliceblue", BGR(240, 248, 255)},
        {"antiquewhite", BGR(250, 235, 215)},
        {"aqua", BGR(0, 255, 255)},
        {"aquamarine", BGR(127, 255, 212)},
        {"azure", BGR(240, 255, 255)},
        {"beige", BGR(245, 245, 220)},
        {"bisque", BGR(255, 228, 196)},
        {"black", BGR(0, 0, 0)},
        {"blanchedalmond", BGR(255, 235, 205)},
        {"blue", BGR(0, 0, 255)},
        {"blueviolet", BGR(138, 43, 226)},
        {"brown", BGR(165, 42, 42)},
        {"burlywood", BGR(222, 184, 135)},
        {"cadetblue", BGR(95, 158, 160)},
        {"chartreuse", BGR(127, 255, 0)},
        {"chocolate", BGR(210, 105, 30)},
        {"coral", BGR(255, 127, 80)},
        {"cornflowerblue", BGR(100, 149, 237)},
        {"cornsilk", BGR(255, 248, 220)},
        {"crimson", BGR(220, 20, 60)},
        {"cyan", BGR(0, 255, 255)},
        {"darkblue", BGR(0, 0, 139)},
        {"darkcyan", BGR(0, 139, 139)},
        {"darkgoldenrod", BGR(184, 134, 11)},
        {"darkgray", BGR(169, 169, 169)},
        {"darkgreen", BGR(0, 100, 0)},
        {"darkgrey", BGR(169, 169, 169)},
        {"darkkhaki", BGR(189, 183, 107)},
        {"darkmagenta", BGR(139, 0, 139)},
        {"darkolivegreen", BGR(85, 107, 47)},
        {"darkorange", BGR(255, 140, 0)},
        {"darkorchid", BGR(153, 50, 204)},
        {"darkred", BGR(139, 0, 0)},
        {"darksalmon", BGR(233, 150, 122)},
        {"darkseagreen", BGR(143, 188, 143)},
        {"darkslateblue", BGR(72, 61, 139)},
        {"darkslategray", BGR(47, 79, 79)},
        {"darkslategrey", BGR(47, 79, 79)},
        {"darkturquoise", BGR(0, 206, 209)},
        {"darkviolet", BGR(148, 0, 211)},
        {"darkyellow", BGR(204, 204, 0)},
        {"deeppink", BGR(255, 20, 147)},
        {"deepskyblue", BGR(0, 191, 255)},
        {"dimgray", BGR(105, 105, 105)},
        {"dimgrey", BGR(105, 105, 105)},
        {"dodgerblue", BGR(30, 144, 255)},
        {"firebrick", BGR(178, 34, 34)},
        {"floralwhite", BGR(255, 250, 240)},
        {"forestgreen", BGR(34, 139, 34)},
        {"fuchsia", BGR(255, 0, 255)},
        {"gainsboro", BGR(220, 220, 220)},
        {"ghostwhite", BGR(248, 248, 255)},
        {"gold", BGR(255, 215, 0)},
        {"goldenrod", BGR(218, 165, 32)},
        {"gray", BGR(128, 128, 128)},
        {"green", BGR(0, 128, 0)},
        {"greenyellow", BGR(173, 255, 47)},
        {"grey", BGR(128, 128, 128)},
        {"honeydew", BGR(240, 255, 240)},
        {"hotpink", BGR(255, 105, 180)},
        {"indianred", BGR(205, 92, 92)},
        {"indigo", BGR(75, 0, 130)},
        {"ivory", BGR(255, 255, 240)},
        {"khaki", BGR(240, 230, 140)},
        {"lavender", BGR(230, 230, 250)},
        {"lavenderblush", BGR(255, 240, 245)},
        {"lawngreen", BGR(124, 252, 0)},
        {"lemonchiffon", BGR(255, 250, 205)},
        {"lightblue", BGR(173, 216, 230)},
        {"lightcoral", BGR(240, 128, 128)},
        {"lightcyan", BGR(224, 255, 255)},
        {"lightgoldenrodyellow", BGR(250, 250, 210)},
        {"lightgray", BGR(211, 211, 211)},
        {"lightgreen", BGR(144, 238, 144)},
        {"lightgrey", BGR(211, 211, 211)},
        {"lightmagenta", BGR(255, 128, 255)},
        {"lightpink", BGR(255, 182, 193)},
        {"lightred", BGR(255, 128, 128)},
        {"lightsalmon", BGR(255, 160, 122)},
        {"lightseagreen", BGR(32, 178, 170)},
        {"lightskyblue", BGR(135, 206, 250)},
        {"lightslategray", BGR(119, 136, 153)},
        {"lightslategrey", BGR(119, 136, 153)},
        {"lightsteelblue", BGR(176, 196, 222)},
        {"lightyellow", BGR(255, 255, 224)},
        {"lime", BGR(0, 255, 0)},
        {"limegreen", BGR(50, 205, 50)},
        {"linen", BGR(250, 240, 230)},
        {"magenta", BGR(255, 0, 255)},
        {"maroon", BGR(128, 0, 0)},
        {"mediumaquamarine", BGR(102, 205, 170)},
        {"mediumblue", BGR(0, 0, 205)},
        {"mediumorchid", BGR(186, 85, 211)},
        {"mediumpurple", BGR(147, 112, 219)},
        {"mediumseagreen", BGR(60, 179, 113)},
        {"mediumslateblue", BGR(123, 104, 238)},
        {"mediumspringgreen", BGR(0, 250, 154)},
        {"mediumturquoise", BGR(72, 209, 204)},
        {"mediumvioletred", BGR(199, 21, 133)},
        {"midnightblue", BGR(25, 25, 112)},
        {"mintcream", BGR(245, 255, 250)},
        {"mistyrose", BGR(255, 228, 225)},
        {"moccasin", BGR(255, 228, 181)},
        {"navy", BGR(0, 0, 128)},
        {"navajowhite", BGR(255, 222, 173)},
        {"oldlace", BGR(253, 245, 230)},
        {"olive", BGR(128, 128, 0)},
        {"olivedrab", BGR(107, 142, 35)},
        {"orange", BGR(255, 165, 0)},
        {"orangered", BGR(255, 69, 0)},
        {"orchid", BGR(218, 112, 214)},
        {"palegoldenrod", BGR(238, 232, 170)},
        {"palegreen", BGR(152, 251, 152)},
        {"paleturquoise", BGR(175, 238, 238)},
        {"palevioletred", BGR(219, 112, 147)},
        {"papayawhip", BGR(255, 239, 213)},
        {"peachpuff", BGR(255, 218, 185)},
        {"peru", BGR(205, 133, 63)},
        {"pink", BGR(255, 192, 203)},
        {"plum", BGR(221, 160, 221)},
        {"powderblue", BGR(176, 224, 230)},
        {"purple", BGR(128, 0, 128)},
        {"rebeccapurple", BGR(102, 51, 153)}, // added in MUSHclient 5.07
        {"red", BGR(255, 0, 0)},
        {"rosybrown", BGR(188, 143, 143)},
        {"royalblue", BGR(65, 105, 225)},
        {"saddlebrown", BGR(139, 69, 19)},
        {"salmon", BGR(250, 128, 114)},
        {"sandybrown", BGR(244, 164, 96)},
        {"seagreen", BGR(46, 139, 87)},
        {"seashell", BGR(255, 245, 238)},
        {"sienna", BGR(160, 82, 45)},
        {"silver", BGR(192, 192, 192)},
        {"skyblue", BGR(135, 206, 235)},
        {"slateblue", BGR(106, 90, 205)},
        {"slategray", BGR(112, 128, 144)},
        {"slategrey", BGR(112, 128, 144)},
        {"snow", BGR(255, 250, 250)},
        {"springgreen", BGR(0, 255, 127)},
        {"steelblue", BGR(70, 130, 180)},
        {"tan", BGR(210, 180, 140)},
        {"teal", BGR(0, 128, 128)},
        {"thistle", BGR(216, 191, 216)},
        {"tomato", BGR(255, 99, 71)},
        {"turquoise", BGR(64, 224, 208)},
        {"violet", BGR(238, 130, 238)},
        {"wheat", BGR(245, 222, 179)},
        {"white", BGR(255, 255, 255)},
        {"whitesmoke", BGR(245, 245, 245)},
        {"yellow", BGR(255, 255, 0)},
        {"yellowgreen", BGR(154, 205, 50)},
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

    // Original returns -1 (0xFFFFFFFF) for unknown color names
    return static_cast<QRgb>(0xFFFFFFFF);
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

        map[BGR(240, 248, 255)] = "aliceblue";
        map[BGR(250, 235, 215)] = "antiquewhite";
        map[BGR(127, 255, 212)] = "aquamarine";
        map[BGR(240, 255, 255)] = "azure";
        map[BGR(245, 245, 220)] = "beige";
        map[BGR(255, 228, 196)] = "bisque";
        map[BGR(0, 0, 0)] = "black";
        map[BGR(255, 235, 205)] = "blanchedalmond";
        map[BGR(0, 0, 255)] = "blue";
        map[BGR(138, 43, 226)] = "blueviolet";
        map[BGR(165, 42, 42)] = "brown";
        map[BGR(222, 184, 135)] = "burlywood";
        map[BGR(95, 158, 160)] = "cadetblue";
        map[BGR(127, 255, 0)] = "chartreuse";
        map[BGR(210, 105, 30)] = "chocolate";
        map[BGR(255, 127, 80)] = "coral";
        map[BGR(100, 149, 237)] = "cornflowerblue";
        map[BGR(255, 248, 220)] = "cornsilk";
        map[BGR(220, 20, 60)] = "crimson";
        map[BGR(0, 255, 255)] = "cyan";
        map[BGR(0, 0, 139)] = "darkblue";
        map[BGR(0, 139, 139)] = "darkcyan";
        map[BGR(184, 134, 11)] = "darkgoldenrod";
        map[BGR(169, 169, 169)] = "darkgray";
        map[BGR(0, 100, 0)] = "darkgreen";
        map[BGR(189, 183, 107)] = "darkkhaki";
        map[BGR(139, 0, 139)] = "darkmagenta";
        map[BGR(85, 107, 47)] = "darkolivegreen";
        map[BGR(255, 140, 0)] = "darkorange";
        map[BGR(153, 50, 204)] = "darkorchid";
        map[BGR(139, 0, 0)] = "darkred";
        map[BGR(233, 150, 122)] = "darksalmon";
        map[BGR(143, 188, 143)] = "darkseagreen";
        map[BGR(72, 61, 139)] = "darkslateblue";
        map[BGR(47, 79, 79)] = "darkslategray";
        map[BGR(0, 206, 209)] = "darkturquoise";
        map[BGR(148, 0, 211)] = "darkviolet";
        map[BGR(204, 204, 0)] = "darkyellow";
        map[BGR(255, 20, 147)] = "deeppink";
        map[BGR(0, 191, 255)] = "deepskyblue";
        map[BGR(105, 105, 105)] = "dimgray";
        map[BGR(30, 144, 255)] = "dodgerblue";
        map[BGR(178, 34, 34)] = "firebrick";
        map[BGR(255, 250, 240)] = "floralwhite";
        map[BGR(34, 139, 34)] = "forestgreen";
        map[BGR(255, 0, 255)] = "fuchsia";
        map[BGR(220, 220, 220)] = "gainsboro";
        map[BGR(248, 248, 255)] = "ghostwhite";
        map[BGR(255, 215, 0)] = "gold";
        map[BGR(218, 165, 32)] = "goldenrod";
        map[BGR(128, 128, 128)] = "gray";
        map[BGR(0, 128, 0)] = "green";
        map[BGR(173, 255, 47)] = "greenyellow";
        map[BGR(240, 255, 240)] = "honeydew";
        map[BGR(255, 105, 180)] = "hotpink";
        map[BGR(205, 92, 92)] = "indianred";
        map[BGR(75, 0, 130)] = "indigo";
        map[BGR(255, 255, 240)] = "ivory";
        map[BGR(240, 230, 140)] = "khaki";
        map[BGR(230, 230, 250)] = "lavender";
        map[BGR(255, 240, 245)] = "lavenderblush";
        map[BGR(124, 252, 0)] = "lawngreen";
        map[BGR(255, 250, 205)] = "lemonchiffon";
        map[BGR(173, 216, 230)] = "lightblue";
        map[BGR(240, 128, 128)] = "lightcoral";
        map[BGR(224, 255, 255)] = "lightcyan";
        map[BGR(250, 250, 210)] = "lightgoldenrodyellow";
        map[BGR(211, 211, 211)] = "lightgray";
        map[BGR(144, 238, 144)] = "lightgreen";
        map[BGR(255, 128, 255)] = "lightmagenta";
        map[BGR(255, 182, 193)] = "lightpink";
        map[BGR(255, 128, 128)] = "lightred";
        map[BGR(255, 160, 122)] = "lightsalmon";
        map[BGR(32, 178, 170)] = "lightseagreen";
        map[BGR(135, 206, 250)] = "lightskyblue";
        map[BGR(119, 136, 153)] = "lightslategray";
        map[BGR(176, 196, 222)] = "lightsteelblue";
        map[BGR(255, 255, 224)] = "lightyellow";
        map[BGR(0, 255, 0)] = "lime";
        map[BGR(50, 205, 50)] = "limegreen";
        map[BGR(250, 240, 230)] = "linen";
        map[BGR(128, 0, 0)] = "maroon";
        map[BGR(102, 205, 170)] = "mediumaquamarine";
        map[BGR(0, 0, 205)] = "mediumblue";
        map[BGR(186, 85, 211)] = "mediumorchid";
        map[BGR(147, 112, 219)] = "mediumpurple";
        map[BGR(60, 179, 113)] = "mediumseagreen";
        map[BGR(123, 104, 238)] = "mediumslateblue";
        map[BGR(0, 250, 154)] = "mediumspringgreen";
        map[BGR(72, 209, 204)] = "mediumturquoise";
        map[BGR(199, 21, 133)] = "mediumvioletred";
        map[BGR(25, 25, 112)] = "midnightblue";
        map[BGR(245, 255, 250)] = "mintcream";
        map[BGR(255, 228, 225)] = "mistyrose";
        map[BGR(255, 228, 181)] = "moccasin";
        map[BGR(255, 222, 173)] = "navajowhite";
        map[BGR(0, 0, 128)] = "navy";
        map[BGR(253, 245, 230)] = "oldlace";
        map[BGR(128, 128, 0)] = "olive";
        map[BGR(107, 142, 35)] = "olivedrab";
        map[BGR(255, 165, 0)] = "orange";
        map[BGR(255, 69, 0)] = "orangered";
        map[BGR(218, 112, 214)] = "orchid";
        map[BGR(238, 232, 170)] = "palegoldenrod";
        map[BGR(152, 251, 152)] = "palegreen";
        map[BGR(175, 238, 238)] = "paleturquoise";
        map[BGR(219, 112, 147)] = "palevioletred";
        map[BGR(255, 239, 213)] = "papayawhip";
        map[BGR(255, 218, 185)] = "peachpuff";
        map[BGR(205, 133, 63)] = "peru";
        map[BGR(255, 192, 203)] = "pink";
        map[BGR(221, 160, 221)] = "plum";
        map[BGR(176, 224, 230)] = "powderblue";
        map[BGR(128, 0, 128)] = "purple";
        map[BGR(102, 51, 153)] = "rebeccapurple";
        map[BGR(255, 0, 0)] = "red";
        map[BGR(188, 143, 143)] = "rosybrown";
        map[BGR(65, 105, 225)] = "royalblue";
        map[BGR(139, 69, 19)] = "saddlebrown";
        map[BGR(250, 128, 114)] = "salmon";
        map[BGR(244, 164, 96)] = "sandybrown";
        map[BGR(46, 139, 87)] = "seagreen";
        map[BGR(255, 245, 238)] = "seashell";
        map[BGR(160, 82, 45)] = "sienna";
        map[BGR(192, 192, 192)] = "silver";
        map[BGR(135, 206, 235)] = "skyblue";
        map[BGR(106, 90, 205)] = "slateblue";
        map[BGR(112, 128, 144)] = "slategray";
        map[BGR(255, 250, 250)] = "snow";
        map[BGR(0, 255, 127)] = "springgreen";
        map[BGR(70, 130, 180)] = "steelblue";
        map[BGR(210, 180, 140)] = "tan";
        map[BGR(0, 128, 128)] = "teal";
        map[BGR(216, 191, 216)] = "thistle";
        map[BGR(255, 99, 71)] = "tomato";
        map[BGR(64, 224, 208)] = "turquoise";
        map[BGR(238, 130, 238)] = "violet";
        map[BGR(245, 222, 179)] = "wheat";
        map[BGR(255, 255, 255)] = "white";
        map[BGR(245, 245, 245)] = "whitesmoke";
        map[BGR(255, 255, 0)] = "yellow";
        map[BGR(154, 205, 50)] = "yellowgreen";

        return map;
    }();

    // Mask off alpha channel (use only color components)
    quint32 bgrOnly = bgr & 0x00FFFFFF;

    // Look up color name
    if (reverseColorMap.contains(bgrOnly)) {
        return reverseColorMap[bgrOnly];
    }

    // Not a named color — return "#RRGGBB" hex string (original never returns empty)
    int r = bgrOnly & 0xFF;
    int g = (bgrOnly >> 8) & 0xFF;
    int b = (bgrOnly >> 16) & 0xFF;
    return QStringLiteral("#%1%2%3")
        .arg(r, 2, 16, QChar('0'))
        .arg(g, 2, 16, QChar('0'))
        .arg(b, 2, 16, QChar('0'))
        .toUpper();
}

/**
 * world.ColourNameToRGB(name)
 *
 * Converts a color name to its BGR integer value. Supports X11/HTML color names
 * ("red", "blue", "darkgreen") and hex strings ("#FF0000", "0xFF0000").
 *
 * Returns BGR format (0x00BBGGRR) for MUSHclient compatibility.
 *
 * @param name (string) Color name or hex string (case-insensitive)
 *
 * @return (number) BGR color value (0x00BBGGRR format)
 *
 * @example
 * local red = ColourNameToRGB("red")
 * local blue = ColourNameToRGB("#0000FF")
 * ColourNote(red, 0, "Red text!")
 *
 * @see RGBColourToName, AdjustColour
 */
int L_ColourNameToRGB(lua_State* L)
{
    auto [name] = luaArgs<QString>(L);
    // Original (methods_colours.cpp:31-32) returns a signed long -1 on failure,
    // which Lua sees as -1. QRgb is unsigned 32-bit, so the unknown-name sentinel
    // 0xFFFFFFFF would otherwise reach Lua as 4294967295 on 64-bit platforms,
    // breaking plugin `== -1` checks. Sign-extend the 32-bit result: valid BGR
    // values are 0x00BBGGRR (top byte zero, always positive) and pass through
    // unchanged, while the 0xFFFFFFFF sentinel becomes -1.
    const QRgb rgb = ColourNameToRGB(name);
    return luaReturn(L, static_cast<lua_Integer>(static_cast<std::int32_t>(rgb)));
}

/**
 * world.RGBColourToName(rgb)
 *
 * Converts a BGR color value back to its color name. Only works for standard
 * X11/HTML colors; custom colors return an empty string.
 *
 * @param rgb (number) BGR color value (0x00BBGGRR format)
 *
 * @return (string) Color name (e.g., "red", "blue"), or empty string if not a named color
 *
 * @example
 * local name = RGBColourToName(255)  -- Returns "red"
 * if name ~= "" then
 *     Note("Color name: " .. name)
 * else
 *     Note("Custom color")
 * end
 *
 * @see ColourNameToRGB
 */
int L_RGBColourToName(lua_State* L)
{
    auto [rgbValue] = luaArgs<int>(L);
    return luaReturn(L, RGBColourToName(static_cast<QRgb>(rgbValue)));
}

/**
 * world.GetNormalColour(whichColour)
 *
 * Gets the normal (non-bold) ANSI color value from the world's color palette.
 * These are the base 8 ANSI colors (black, red, green, yellow, blue, magenta,
 * cyan, white).
 *
 * @param whichColour (number) Color index 1-8 (1=black, 2=red, 3=green, etc.)
 *
 * @return (number) BGR color value, or 0 if index out of range
 *
 * @example
 * local red = GetNormalColour(2)  -- Get normal red
 * Note("Normal red is: " .. red)
 *
 * @see GetBoldColour, SetNormalColour
 */
int L_GetNormalColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > 8) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_colors.normal_colour[whichColour - 1]);
    return 1;
}

/**
 * world.GetBoldColour(whichColour)
 *
 * Gets the bold (high-intensity) ANSI color value from the world's color palette.
 * These are the bright versions of the 8 ANSI colors, used when bold/bright
 * attribute is active.
 *
 * @param whichColour (number) Color index 1-8 (1=bright black, 2=bright red, etc.)
 *
 * @return (number) BGR color value, or 0 if index out of range
 *
 * @example
 * local brightRed = GetBoldColour(2)  -- Get bold/bright red
 *
 * @see GetNormalColour, SetBoldColour
 */
int L_GetBoldColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > 8) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_colors.bold_colour[whichColour - 1]);
    return 1;
}

/**
 * world.SetNormalColour(whichColour, rgb)
 *
 * Sets a normal (non-bold) ANSI color in the world's color palette.
 *
 * @param whichColour (number) Color index 1-8 (1=black, 2=red, etc.)
 * @param rgb (number) BGR color value (0x00BBGGRR format)
 *
 * @return Nothing
 *
 * @example
 * -- Make normal red darker
 * SetNormalColour(2, ColourNameToRGB("darkred"))
 *
 * @see GetNormalColour, SetBoldColour
 */
int L_SetNormalColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > 8)
        return 0;

    pDoc->m_colors.normal_colour[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetBoldColour(whichColour, rgb)
 *
 * Sets a bold (high-intensity) ANSI color in the world's color palette.
 *
 * @param whichColour (number) Color index 1-8 (1=bright black, 2=bright red, etc.)
 * @param rgb (number) BGR color value (0x00BBGGRR format)
 *
 * @return Nothing
 *
 * @example
 * -- Make bold red even brighter
 * SetBoldColour(2, 0x0000FF)  -- Pure red
 *
 * @see GetBoldColour, SetNormalColour
 */
int L_SetBoldColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > 8)
        return 0;

    pDoc->m_colors.bold_colour[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetCustomColourText(whichColour, rgb)
 *
 * Sets the foreground (text) color for a custom color slot. Custom colors
 * are used for Note() output and trigger highlighting.
 *
 * @param whichColour (number) Custom color index 1-16
 * @param rgb (number) BGR color value (0x00BBGGRR format)
 *
 * @return Nothing
 *
 * @example
 * -- Set custom color 1 to gold text
 * SetCustomColourText(1, ColourNameToRGB("gold"))
 *
 * @see GetCustomColourText, SetCustomColourBackground
 */
int L_SetCustomColourText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > MAX_CUSTOM)
        return 0;

    pDoc->m_colors.custom_text[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetCustomColourBackground(whichColour, rgb)
 *
 * Sets the background color for a custom color slot. Custom colors are used
 * for Note() output and trigger highlighting.
 *
 * @param whichColour (number) Custom color index 1-16
 * @param rgb (number) BGR color value (0x00BBGGRR format)
 *
 * @return Nothing
 *
 * @example
 * -- Set custom color 1 background to dark blue
 * SetCustomColourBackground(1, ColourNameToRGB("navy"))
 *
 * @see GetCustomColourBackground, SetCustomColourText
 */
int L_SetCustomColourBackground(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checkinteger(L, 1);
    lua_Integer rgb = luaL_checkinteger(L, 2);

    if (whichColour < 1 || whichColour > MAX_CUSTOM)
        return 0;

    pDoc->m_colors.custom_back[whichColour - 1] = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.GetCustomColourText(whichColour)
 *
 * Gets the foreground (text) color for a custom color slot.
 *
 * @param whichColour (number) Custom color index 1-16
 *
 * @return (number) BGR color value, or 0 if index out of range
 *
 * @example
 * local textColor = GetCustomColourText(1)
 * Note("Custom 1 text color: " .. textColor)
 *
 * @see SetCustomColourText, GetCustomColourBackground
 */
int L_GetCustomColourText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > MAX_CUSTOM) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_colors.custom_text[whichColour - 1]);
    return 1;
}

/**
 * world.GetCustomColourBackground(whichColour)
 *
 * Gets the background color for a custom color slot.
 *
 * @param whichColour (number) Custom color index 1-16
 *
 * @return (number) BGR color value, or 0 if index out of range
 *
 * @example
 * local backColor = GetCustomColourBackground(1)
 *
 * @see SetCustomColourBackground, GetCustomColourText
 */
int L_GetCustomColourBackground(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int whichColour = luaL_checknumber(L, 1);

    if (whichColour < 1 || whichColour > MAX_CUSTOM) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, pDoc->m_colors.custom_back[whichColour - 1]);
    return 1;
}

/**
 * world.SetCustomColourName(whichColour, name)
 *
 * Sets the display name for a custom color slot. This name appears in the
 * color configuration UI to help identify the purpose of each custom color.
 *
 * @param whichColour (number) Custom color index 1-16
 * @param name (string) Display name for the color (1-30 characters)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eBadParameter (30046): Index out of range or name too long
 *
 * @example
 * SetCustomColourName(1, "Combat Alert")
 * SetCustomColourName(2, "Channel Chat")
 *
 * @see GetCustomColourText, SetCustomColourText
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
 * Opens a system color picker dialog for the user to select a color. This is
 * useful for letting users customize plugin colors interactively.
 *
 * @param suggested (number) Initial color to display (BGR format), or -1 for default white
 *
 * @return (number) Selected BGR color value, or -1 if the user cancelled
 *
 * @example
 * local currentColor = GetCustomColourText(1)
 * local newColor = PickColour(currentColor)
 * if newColor ~= -1 then
 *     SetCustomColourText(1, newColor)
 *     Note("Color updated!")
 * end
 *
 * @see ColourNameToRGB, AdjustColour
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
 * Adjusts a color value using various transformation methods. Useful for
 * creating color variations (hover effects, disabled states, etc.).
 *
 * Adjustment methods:
 * - 0: No change (returns input color)
 * - 1: Invert (flip all color channels)
 * - 2: Lighter (increase luminance by 2%)
 * - 3: Darker (decrease luminance by 2%)
 * - 4: Less saturation (decrease by 5%)
 * - 5: More saturation (increase by 5%)
 *
 * @param colour (number) BGR color value (0x00BBGGRR format)
 * @param method (number) Adjustment method (0-5)
 *
 * @return (number) Adjusted BGR color value
 *
 * @example
 * local red = ColourNameToRGB("red")
 * local darkRed = AdjustColour(red, 3)  -- Darker
 * local invertedRed = AdjustColour(red, 1)  -- Inverted (cyan)
 *
 * @see ColourNameToRGB, PickColour
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

// ========== Bare-name compatibility aliases (dual get/set dispatch) ==========

/**
 * world.NormalColour(whichColour [, newValue])
 *
 * Compatibility alias for the MUSHclient COM property NormalColour.
 * With one argument acts as a getter (returns the current color value).
 * With two arguments acts as a setter (sets the color and returns nothing).
 *
 * @param whichColour (number) Color index 1-8
 * @param newValue (number, optional) BGR color value to set
 *
 * @return (number|nothing) Current BGR value when getting; nothing when setting
 *
 * @see GetNormalColour, SetNormalColour
 */
int L_NormalColour(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        return L_SetNormalColour(L);
    return L_GetNormalColour(L);
}

/**
 * world.BoldColour(whichColour [, newValue])
 *
 * Compatibility alias for the MUSHclient COM property BoldColour.
 * With one argument acts as a getter; with two acts as a setter.
 *
 * @param whichColour (number) Color index 1-8
 * @param newValue (number, optional) BGR color value to set
 *
 * @return (number|nothing) Current BGR value when getting; nothing when setting
 *
 * @see GetBoldColour, SetBoldColour
 */
int L_BoldColour(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        return L_SetBoldColour(L);
    return L_GetBoldColour(L);
}

/**
 * world.CustomColourText(whichColour [, newValue])
 *
 * Compatibility alias for the MUSHclient COM property CustomColourText.
 * With one argument acts as a getter; with two acts as a setter.
 *
 * @param whichColour (number) Custom color index 1-16
 * @param newValue (number, optional) BGR color value to set
 *
 * @return (number|nothing) Current BGR value when getting; nothing when setting
 *
 * @see GetCustomColourText, SetCustomColourText
 */
int L_CustomColourText(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        return L_SetCustomColourText(L);
    return L_GetCustomColourText(L);
}

/**
 * world.CustomColourBackground(whichColour [, newValue])
 *
 * Compatibility alias for the MUSHclient COM property CustomColourBackground.
 * With one argument acts as a getter; with two acts as a setter.
 *
 * @param whichColour (number) Custom color index 1-16
 * @param newValue (number, optional) BGR color value to set
 *
 * @return (number|nothing) Current BGR value when getting; nothing when setting
 *
 * @see GetCustomColourBackground, SetCustomColourBackground
 */
int L_CustomColourBackground(lua_State* L)
{
    if (lua_gettop(L) >= 2)
        return L_SetCustomColourBackground(L);
    return L_GetCustomColourBackground(L);
}

/**
 * world.GetCustomColourName(which)
 *
 * Returns the display name assigned to a custom color slot. Custom color
 * names are set with SetCustomColourName() and appear in the configuration
 * UI to help identify each slot's purpose. Returns an empty string if the
 * slot has no name set or if the index is out of range.
 *
 * @param which (number) Custom color index 1-16
 *
 * @return (string) Name of the custom color slot, or empty string if not set
 *
 * @example
 * local name = GetCustomColourName(1)
 * if name ~= "" then
 *     Note("Custom color 1 is named: " .. name)
 * else
 *     Note("Custom color 1 has no name")
 * end
 *
 * @see SetCustomColourName, GetCustomColourText, GetCustomColourBackground
 */
int L_GetCustomColourName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int which = luaL_checknumber(L, 1);

    if (which < 1 || which > MAX_CUSTOM) {
        lua_pushlstring(L, "", 0);
        return 1;
    }

    luaPushQString(L, pDoc->m_colors.custom_colour_name[which - 1]);
    return 1;
}
