/**
 * world_miniwindow_drawing.cpp - Miniwindow Drawing Lua API Functions
 *
 * Drawing primitives (rectangles, circles, lines, polygons, arcs, beziers,
 * gradients, pixels) and text/font rendering functions.
 *
 * Extracted from world_miniwindows.cpp for better code organization.
 */

#include "../../world/miniwindow.h"
#include "../../world/world_document.h"
#include "logging.h"
#include "lua_common.h"

// Helper to get MiniWindow from map
static inline MiniWindow* getMiniWindow(WorldDocument* doc, const QString& name)
{
    auto it = doc->m_MiniWindowMap.find(name);
    return (it != doc->m_MiniWindowMap.end() && it->second) ? it->second.get() : nullptr;
}

/**
 * world.WindowRectOp(name, action, left, top, right, bottom, penColor, brushColor)
 *
 * Draws rectangles with various operations.
 *
 * Action codes:
 *   1 = Frame rectangle (outline only)
 *   2 = Fill rectangle (solid)
 *   3 = Invert colors in rectangle
 *   4 = 3D raised rectangle
 *   5 = 3D sunken rectangle
 *   6 = Flood fill from point
 *
 * @param name (string) Miniwindow name
 * @param action (number) Drawing operation (1-6)
 * @param left (number) Left coordinate
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate
 * @param bottom (number) Bottom coordinate
 * @param penColor (number) Outline color (BGR format)
 * @param brushColor (number) Fill color (BGR format, optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a red filled rectangle
 * WindowRectOp("mywin", 2, 10, 10, 100, 50, 0x0000FF, 0x0000FF)
 *
 * -- Draw a blue frame
 * WindowRectOp("mywin", 1, 10, 10, 100, 50, 0xFF0000, 0)
 *
 * -- Draw 3D raised button
 * WindowRectOp("mywin", 4, 10, 10, 100, 30, 0xC0C0C0, 0xC0C0C0)
 *
 * @see WindowCircleOp, WindowLine, WindowGradient
 */
int L_WindowRectOp(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint16 action = luaL_checkinteger(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    QRgb penColor = luaL_checkinteger(L, 7);
    QRgb brushColor = luaL_optinteger(L, 8, 0); // Optional, default to black

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->RectOp(action, left, top, right, bottom, penColor, brushColor);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowCircleOp(name, action, left, top, right, bottom, penColor, penStyle, penWidth,
 *                      brushColor, brushStyle, extra1, extra2, extra3, extra4)
 *
 * Draws circles, ellipses, and rounded rectangles with various styles.
 *
 * Action codes:
 *   1 = Ellipse (outline)
 *   2 = Filled ellipse
 *   3 = Pie slice (arc with lines to center)
 *   4 = Chord (arc with straight line connecting ends)
 *   5 = Rounded rectangle
 *
 * Pen styles: 0=solid, 1=dash, 2=dot, 3=dashdot, 4=dashdotdot
 * Brush styles: 0=solid, 1=null (transparent)
 *
 * @param name (string) Miniwindow name
 * @param action (number) Drawing operation (1-5)
 * @param left (number) Bounding rectangle left
 * @param top (number) Bounding rectangle top
 * @param right (number) Bounding rectangle right
 * @param bottom (number) Bounding rectangle bottom
 * @param penColor (number) Outline color (BGR)
 * @param penStyle (number) Line style (0-4)
 * @param penWidth (number) Line width in pixels
 * @param brushColor (number) Fill color (BGR)
 * @param brushStyle (number) Fill style (0-1)
 * @param extra1 (number) Start angle (pie/chord) or corner width (rounded rect)
 * @param extra2 (number) End angle (pie/chord) or corner height (rounded rect)
 * @param extra3 (number) Reserved
 * @param extra4 (number) Reserved
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a filled blue circle
 * WindowCircleOp("mywin", 2, 10, 10, 110, 110, 0xFF0000, 0, 2, 0xFF0000, 0)
 *
 * -- Draw an outlined ellipse
 * WindowCircleOp("mywin", 1, 10, 10, 200, 100, 0x00FF00, 0, 1, 0, 1)
 *
 * -- Rounded rectangle with 10x10 corners
 * WindowCircleOp("mywin", 5, 10, 10, 100, 50, 0, 0, 1, 0xC0C0C0, 0, 10, 10)
 *
 * @see WindowRectOp, WindowLine, WindowArc
 */
int L_WindowCircleOp(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint16 action = luaL_checkinteger(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    QRgb penColor = luaL_checkinteger(L, 7);
    qint32 penStyle = luaL_checkinteger(L, 8);
    qint32 penWidth = luaL_checkinteger(L, 9);
    QRgb brushColor = luaL_checkinteger(L, 10);
    qint32 brushStyle = luaL_checkinteger(L, 11);
    qint32 extra1 = luaL_optinteger(L, 12, 0);
    qint32 extra2 = luaL_optinteger(L, 13, 0);
    qint32 extra3 = luaL_optinteger(L, 14, 0);
    qint32 extra4 = luaL_optinteger(L, 15, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->CircleOp(action, left, top, right, bottom, penColor, penStyle, penWidth,
                                  brushColor, brushStyle, extra1, extra2, extra3, extra4);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowLine(name, x1, y1, x2, y2, penColor, penStyle, penWidth)
 *
 * Draws a line between two points.
 *
 * Pen styles: 0=solid, 1=dash, 2=dot, 3=dashdot, 4=dashdotdot
 *
 * @param name (string) Miniwindow name
 * @param x1 (number) Start X coordinate
 * @param y1 (number) Start Y coordinate
 * @param x2 (number) End X coordinate
 * @param y2 (number) End Y coordinate
 * @param penColor (number) Line color (BGR format)
 * @param penStyle (number) Line style (0-4)
 * @param penWidth (number) Line width in pixels
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a red diagonal line
 * WindowLine("mywin", 0, 0, 100, 100, 0x0000FF, 0, 2)
 *
 * -- Draw a dashed horizontal line
 * WindowLine("mywin", 10, 50, 190, 50, 0xFFFFFF, 1, 1)
 *
 * @see WindowPolygon, WindowBezier, WindowArc
 */
int L_WindowLine(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 x1 = luaL_checkinteger(L, 2);
    qint32 y1 = luaL_checkinteger(L, 3);
    qint32 x2 = luaL_checkinteger(L, 4);
    qint32 y2 = luaL_checkinteger(L, 5);
    QRgb penColor = luaL_checkinteger(L, 6);
    qint32 penStyle = luaL_checkinteger(L, 7);
    qint32 penWidth = luaL_checkinteger(L, 8);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Line(x1, y1, x2, y2, penColor, penStyle, penWidth);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowPolygon(name, points, penColor, penStyle, penWidth, brushColor, brushStyle, close,
 *                     winding)
 *
 * Draws a polygon from a series of points.
 *
 * Points are specified as comma-separated coordinate pairs: "x1,y1,x2,y2,..."
 * Pen styles: 0=solid, 1=dash, 2=dot, 3=dashdot, 4=dashdotdot
 * Brush styles: 0=solid, 1=null (no fill)
 *
 * @param name (string) Miniwindow name
 * @param points (string) Comma-separated X,Y coordinate pairs
 * @param penColor (number) Outline color (BGR)
 * @param penStyle (number) Line style (0-4)
 * @param penWidth (number) Line width in pixels
 * @param brushColor (number) Fill color (BGR)
 * @param brushStyle (number) Fill style (0-1)
 * @param close (boolean) true to connect last point to first
 * @param winding (boolean) true for winding fill, false for alternate fill
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a triangle
 * WindowPolygon("mywin", "50,10,10,90,90,90", 0xFFFFFF, 0, 1, 0x00FF00, 0, true, false)
 *
 * -- Draw an open polyline (not closed)
 * WindowPolygon("mywin", "0,0,50,50,100,0", 0xFFFFFF, 0, 2, 0, 1, false, false)
 *
 * @see WindowLine, WindowBezier, WindowRectOp
 */
int L_WindowPolygon(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* points = luaL_checkstring(L, 2);
    QRgb penColor = luaL_checkinteger(L, 3);
    qint32 penStyle = luaL_checkinteger(L, 4);
    qint32 penWidth = luaL_checkinteger(L, 5);
    QRgb brushColor = luaL_checkinteger(L, 6);
    qint32 brushStyle = luaL_checkinteger(L, 7);
    bool close = lua_toboolean(L, 8);
    bool winding = lua_toboolean(L, 9);

    QString windowName = QString::fromUtf8(name);
    QString pointsStr = QString::fromUtf8(points);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Polygon(pointsStr, penColor, penStyle, penWidth, brushColor, brushStyle,
                                 close, winding);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowGradient(name, left, top, right, bottom, color1, color2, mode)
 *
 * Fills a rectangle with a smooth gradient between two colors.
 *
 * Gradient modes:
 *   1 = Horizontal (left to right)
 *   2 = Vertical (top to bottom)
 *
 * @param name (string) Miniwindow name
 * @param left (number) Left coordinate
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate
 * @param bottom (number) Bottom coordinate
 * @param color1 (number) Start color (BGR format)
 * @param color2 (number) End color (BGR format)
 * @param mode (number) Gradient direction (1=horizontal, 2=vertical)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Horizontal gradient from blue to red
 * WindowGradient("mywin", 0, 0, 200, 100, 0xFF0000, 0x0000FF, 1)
 *
 * -- Vertical gradient from black to white
 * WindowGradient("mywin", 0, 0, 200, 100, 0x000000, 0xFFFFFF, 2)
 *
 * @see WindowRectOp, WindowCircleOp
 */
int L_WindowGradient(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 left = luaL_checkinteger(L, 2);
    qint32 top = luaL_checkinteger(L, 3);
    qint32 right = luaL_checkinteger(L, 4);
    qint32 bottom = luaL_checkinteger(L, 5);
    QRgb color1 = luaL_checkinteger(L, 6);
    QRgb color2 = luaL_checkinteger(L, 7);
    qint32 mode = luaL_checkinteger(L, 8);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Gradient(left, top, right, bottom, color1, color2, mode);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowSetPixel(name, x, y, color)
 *
 * Sets the color of a single pixel.
 *
 * @param name (string) Miniwindow name
 * @param x (number) X coordinate
 * @param y (number) Y coordinate
 * @param color (number) Pixel color (BGR format)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a red pixel at (50, 50)
 * WindowSetPixel("mywin", 50, 50, 0x0000FF)
 *
 * -- Draw a pattern of pixels
 * for i = 0, 99 do
 *     WindowSetPixel("mywin", i, i, 0xFFFFFF)
 * end
 *
 * @see WindowGetPixel, WindowLine
 */
int L_WindowSetPixel(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 x = luaL_checkinteger(L, 2);
    qint32 y = luaL_checkinteger(L, 3);
    QRgb color = luaL_checkinteger(L, 4);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->SetPixel(x, y, color);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowGetPixel(name, x, y)
 *
 * Gets the color of a single pixel.
 *
 * @param name (string) Miniwindow name
 * @param x (number) X coordinate
 * @param y (number) Y coordinate
 *
 * @return (number) Pixel color (BGR format), or 0 if window doesn't exist
 *
 * @example
 * -- Get color at a point
 * local color = WindowGetPixel("mywin", 50, 50)
 * Note("Color: " .. string.format("0x%06X", color))
 *
 * -- Check if pixel is white
 * if WindowGetPixel("mywin", x, y) == 0xFFFFFF then
 *     Note("Pixel is white")
 * end
 *
 * @see WindowSetPixel
 */
int L_WindowGetPixel(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 x = luaL_checkinteger(L, 2);
    qint32 y = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnumber(L, 0);
        return 1;
    }

    QRgb color = win->GetPixel(x, y);
    lua_pushnumber(L, color);
    return 1;
}

/**
 * world.WindowArc(name, left, top, right, bottom, x1, y1, x2, y2, penColor, penStyle, penWidth)
 *
 * Draws an arc (portion of an ellipse outline) from start point to end point.
 * The arc is drawn counter-clockwise from the start point to the end point
 * along the ellipse defined by the bounding rectangle.
 *
 * Pen styles: 0=solid, 1=dash, 2=dot, 3=dashdot, 4=dashdotdot
 *
 * @param name (string) Miniwindow name
 * @param left (number) Bounding rectangle left
 * @param top (number) Bounding rectangle top
 * @param right (number) Bounding rectangle right
 * @param bottom (number) Bounding rectangle bottom
 * @param x1 (number) Arc start point X (on or near ellipse)
 * @param y1 (number) Arc start point Y
 * @param x2 (number) Arc end point X
 * @param y2 (number) Arc end point Y
 * @param penColor (number) Line color (BGR format)
 * @param penStyle (number) Line style (0-4)
 * @param penWidth (number) Line width in pixels
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a quarter-circle arc
 * WindowArc("mywin", 10, 10, 110, 110, 110, 60, 60, 10, 0xFFFFFF, 0, 2)
 *
 * @see WindowCircleOp, WindowBezier, WindowLine
 */
int L_WindowArc(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 left = luaL_checkinteger(L, 2);
    qint32 top = luaL_checkinteger(L, 3);
    qint32 right = luaL_checkinteger(L, 4);
    qint32 bottom = luaL_checkinteger(L, 5);
    qint32 x1 = luaL_checkinteger(L, 6);
    qint32 y1 = luaL_checkinteger(L, 7);
    qint32 x2 = luaL_checkinteger(L, 8);
    qint32 y2 = luaL_checkinteger(L, 9);
    QRgb penColor = luaL_checkinteger(L, 10);
    qint32 penStyle = luaL_checkinteger(L, 11);
    qint32 penWidth = luaL_checkinteger(L, 12);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->Arc(left, top, right, bottom, x1, y1, x2, y2, penColor, penStyle, penWidth);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowBezier(name, points, penColor, penStyle, penWidth)
 *
 * Draws a Bezier curve through the specified control points.
 * Points must be specified as (3n+1) points where n is the number of
 * cubic Bezier segments. Each segment uses 4 points: start, control1,
 * control2, end (with end being the start of the next segment).
 *
 * Pen styles: 0=solid, 1=dash, 2=dot, 3=dashdot, 4=dashdotdot
 *
 * @param name (string) Miniwindow name
 * @param points (string) Comma-separated X,Y coordinate pairs
 * @param penColor (number) Line color (BGR format)
 * @param penStyle (number) Line style (0-4)
 * @param penWidth (number) Line width in pixels
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw a simple Bezier curve (1 segment = 4 points)
 * WindowBezier("mywin", "10,50,30,10,70,90,90,50", 0xFFFFFF, 0, 2)
 *
 * -- Draw a compound curve (2 segments = 7 points)
 * WindowBezier("mywin", "0,50,25,0,50,0,75,50,100,100,125,100,150,50", 0x00FF00, 0, 1)
 *
 * @see WindowLine, WindowPolygon, WindowArc
 */
int L_WindowBezier(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* points = luaL_checkstring(L, 2);
    QRgb penColor = luaL_checkinteger(L, 3);
    qint32 penStyle = luaL_checkinteger(L, 4);
    qint32 penWidth = luaL_checkinteger(L, 5);

    QString windowName = QString::fromUtf8(name);
    QString pointsStr = QString::fromUtf8(points);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Bezier(pointsStr, penColor, penStyle, penWidth);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowFont(name, fontId, fontName, size, bold, italic, underline, strikeout)
 *
 * Creates or updates a named font for use with WindowText.
 * Fonts are stored per-miniwindow and referenced by fontId.
 *
 * @param name (string) Miniwindow name
 * @param fontId (string) Unique font identifier for this window
 * @param fontName (string) System font name (e.g., "Arial", "Courier New")
 * @param size (number) Font size in points
 * @param bold (boolean) true for bold weight
 * @param italic (boolean) true for italic style
 * @param underline (boolean) true for underlined text
 * @param strikeout (boolean) true for strikethrough text
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a 12pt bold Arial font
 * WindowFont("mywin", "title", "Arial", 12, true, false, false, false)
 *
 * -- Create a monospace font for code
 * WindowFont("mywin", "code", "Courier New", 10, false, false, false, false)
 *
 * -- Use the font to draw text
 * WindowText("mywin", "title", "Hello World", 10, 10, 0, 0, 0xFFFFFF, false)
 *
 * @see WindowText, WindowTextWidth, WindowFontInfo, WindowFontList
 */
int L_WindowFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* fontId = luaL_checkstring(L, 2);
    const char* fontName = luaL_checkstring(L, 3);
    double size = luaL_checknumber(L, 4);
    bool bold = lua_toboolean(L, 5);
    bool italic = lua_toboolean(L, 6);
    bool underline = lua_toboolean(L, 7);
    bool strikeout = lua_toboolean(L, 8);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Font(QString::fromUtf8(fontId), QString::fromUtf8(fontName), size, bold,
                              italic, underline, strikeout);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowText(name, fontId, text, left, top, right, bottom, color, unicode)
 *
 * Draws text using a previously created font. Returns the text width
 * to allow for measuring and positioning.
 *
 * If right and bottom are 0, text is drawn at the specified position
 * without clipping. Otherwise, text is clipped to the rectangle.
 *
 * @param name (string) Miniwindow name
 * @param fontId (string) Font identifier (from WindowFont)
 * @param text (string) Text to draw
 * @param left (number) Left position
 * @param top (number) Top position
 * @param right (number) Right clip boundary (0 = no clipping)
 * @param bottom (number) Bottom clip boundary (0 = no clipping)
 * @param color (number) Text color (BGR format)
 * @param unicode (boolean) true if text is Unicode encoded
 *
 * @return (number) Width of the drawn text in pixels
 *
 * @example
 * -- Simple text drawing
 * WindowFont("mywin", "f", "Arial", 12, false, false, false, false)
 * local width = WindowText("mywin", "f", "Hello!", 10, 10, 0, 0, 0xFFFFFF, false)
 * Note("Text width: " .. width)
 *
 * -- Right-aligned text
 * local text = "Score: 100"
 * local tw = WindowTextWidth("mywin", "f", text, false)
 * WindowText("mywin", "f", text, 190 - tw, 10, 0, 0, 0xFFFFFF, false)
 *
 * @see WindowFont, WindowTextWidth, WindowFontInfo
 */
int L_WindowText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* fontId = luaL_checkstring(L, 2);
    const char* text = luaL_checkstring(L, 3);
    qint32 left = luaL_checkinteger(L, 4);
    qint32 top = luaL_checkinteger(L, 5);
    qint32 right = luaL_checkinteger(L, 6);
    qint32 bottom = luaL_checkinteger(L, 7);
    QRgb color = luaL_checkinteger(L, 8);
    bool unicode = lua_toboolean(L, 9);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Text(QString::fromUtf8(fontId), QString::fromUtf8(text), left, top, right,
                              bottom, color, unicode);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowTextWidth(name, fontId, text, unicode)
 *
 * Measures the width of text without drawing it. Useful for layout
 * calculations, centering, and right-alignment.
 *
 * @param name (string) Miniwindow name
 * @param fontId (string) Font identifier (from WindowFont)
 * @param text (string) Text to measure
 * @param unicode (boolean) true if text is Unicode encoded
 *
 * @return (number) Width in pixels, or 0 if window/font not found
 *
 * @example
 * -- Center text horizontally in a 200px window
 * WindowFont("mywin", "f", "Arial", 12, false, false, false, false)
 * local text = "Centered"
 * local width = WindowTextWidth("mywin", "f", text, false)
 * WindowText("mywin", "f", text, (200 - width) / 2, 10, 0, 0, 0xFFFFFF, false)
 *
 * @see WindowText, WindowFont, WindowFontInfo
 */
int L_WindowTextWidth(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* fontId = luaL_checkstring(L, 2);
    const char* text = luaL_checkstring(L, 3);
    bool unicode = lua_toboolean(L, 4);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnumber(L, 0);
        return 1;
    }

    qint32 width = win->TextWidth(QString::fromUtf8(fontId), QString::fromUtf8(text), unicode);
    lua_pushnumber(L, width);
    return 1;
}

/**
 * world.WindowFontInfo(name, fontId, infoType)
 *
 * Returns information about a font in a miniwindow.
 *
 * Info types:
 *   1 = Font height in pixels
 *   2 = Ascent (baseline to top)
 *   3 = Descent (baseline to bottom)
 *   4 = Internal leading
 *   5 = External leading
 *   6 = Average character width
 *   7 = Maximum character width
 *   8 = Weight (400=normal, 700=bold)
 *   9 = Pitch and family
 *   10 = Character set
 *   11 = Italic flag (boolean)
 *   12 = Underline flag (boolean)
 *   13 = Strikeout flag (boolean)
 *   14 = Font name (string)
 *   15 = True if fixed-width font
 *
 * @param name (string) Miniwindow name
 * @param fontId (string) Font identifier
 * @param infoType (number) Type of information (1-15)
 *
 * @return (varies) Requested information, or nil if font not found
 *
 * @example
 * -- Get font height for line spacing
 * local height = WindowFontInfo("mywin", "f", 1)
 * for i = 0, 5 do
 *     WindowText("mywin", "f", "Line " .. i, 10, i * height, 0, 0, 0xFFFFFF, false)
 * end
 *
 * @see WindowFont, WindowText, WindowFontList
 */
int L_WindowFontInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* fontId = luaL_checkstring(L, 2);
    qint32 infoType = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    QVariant info = win->FontInfo(QString::fromUtf8(fontId), infoType);

    if (!info.isValid()) {
        lua_pushnil(L);
        return 1;
    }

    // Convert QVariant to Lua value
    switch (info.typeId()) {
        case QMetaType::QString:
            lua_pushstring(L, info.toString().toUtf8().constData());
            break;
        case QMetaType::Int:
            lua_pushnumber(L, info.toInt());
            break;
        case QMetaType::Bool:
            lua_pushboolean(L, info.toBool());
            break;
        case QMetaType::Double:
            lua_pushnumber(L, info.toDouble());
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.WindowFontList(name)
 *
 * Returns a table of all font IDs defined in a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (table) Array of font ID strings (1-indexed)
 *
 * @example
 * -- List all fonts in the window
 * local fonts = WindowFontList("mywin")
 * for i, fontId in ipairs(fonts) do
 *     local height = WindowFontInfo("mywin", fontId, 1)
 *     Note("Font: " .. fontId .. " height: " .. height)
 * end
 *
 * @see WindowFont, WindowFontInfo
 */
int L_WindowFontList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_newtable(L); // Return empty table if window not found
        return 1;
    }

    QStringList fontList = win->FontList();

    // Create Lua table
    lua_newtable(L);
    for (int i = 0; i < fontList.size(); i++) {
        lua_pushstring(L, fontList[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1); // Lua arrays are 1-indexed
    }

    return 1;
}

/**
 * world.WindowWrite(name, filename)
 *
 * Saves the miniwindow contents to an image file.
 * Supports BMP and PNG formats based on file extension.
 *
 * @param name (string) Miniwindow name
 * @param filename (string) Output file path (must end in .bmp or .png)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Save window as PNG
 * WindowWrite("mywin", "screenshot.png")
 *
 * -- Save with timestamp
 * local filename = "capture_" .. os.date("%Y%m%d_%H%M%S") .. ".png"
 * WindowWrite("mywin", filename)
 *
 * @see WindowCreate, WindowLoadImage
 */
int L_WindowWrite(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* filename = luaL_checkstring(L, 2);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Write(QString::fromUtf8(filename));
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowFilter(name, left, top, right, bottom, operation, options)
 *
 * Applies a pixel filter to a rectangular region.
 *
 * Filter operations:
 *   1 = Noise (options = amount)
 *   2 = Monochrome (grayscale)
 *   3 = Brightness (options = -100 to 100)
 *   4 = Contrast (options = -100 to 100)
 *   5 = Gamma correction (options = gamma value)
 *   6 = Invert colors
 *   7 = Red channel only
 *   8 = Green channel only
 *   9 = Blue channel only
 *   10 = Average blur (options = radius)
 *   11 = Sharpen
 *   12 = Emboss (options = depth)
 *   13 = Edge detection
 *
 * @param name (string) Miniwindow name
 * @param left (number) Left coordinate
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate (0 = window width)
 * @param bottom (number) Bottom coordinate (0 = window height)
 * @param operation (number) Filter type (1-27)
 * @param options (number) Filter-specific parameter
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Convert to grayscale
 * WindowFilter("mywin", 0, 0, 0, 0, 2, 0)
 *
 * -- Increase brightness by 50
 * WindowFilter("mywin", 0, 0, 0, 0, 3, 50)
 *
 * -- Apply blur with radius 3
 * WindowFilter("mywin", 0, 0, 0, 0, 10, 3)
 *
 * @see WindowRectOp, WindowDrawImage
 */
int L_WindowFilter(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 left = luaL_checkinteger(L, 2);
    qint32 top = luaL_checkinteger(L, 3);
    qint32 right = luaL_checkinteger(L, 4);
    qint32 bottom = luaL_checkinteger(L, 5);
    qint16 operation = luaL_checkinteger(L, 6);
    double options = luaL_checknumber(L, 7);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Filter(left, top, right, bottom, operation, options);
    lua_pushnumber(L, result);
    return 1;
}
