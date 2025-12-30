/**
 * world_miniwindows.cpp - Miniwindow Lua API Functions
 *
 * Miniwindow System
 *
 * This file implements all miniwindow-related Lua API functions.
 * Extracted from lua_methods.cpp for better code organization.
 */

#include "../../automation/plugin.h"
#include "../../ui/views/output_view.h"
#include "../../world/miniwindow.h"
#include "../../world/world_document.h"
#include "../script_engine.h"
#include <QMenu>
#include <QStack>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "logging.h"
#include "lua_common.h"
#include "miniwindow.h"

// Helper to get MiniWindow from map
static inline MiniWindow* getMiniWindow(WorldDocument* doc, const QString& name)
{
    auto it = doc->m_MiniWindowMap.find(name);
    return (it != doc->m_MiniWindowMap.end() && it->second) ? it->second.get() : nullptr;
}

// ========== Miniwindow Creation and Management ==========

/**
 * world.WindowCreate(name, left, top, width, height, position, flags, bgColor)
 *
 * Creates a new miniwindow or updates an existing one. Miniwindows are
 * overlay graphics that can display custom content, images, and hotspots.
 *
 * Position modes:
 *   - 0: Custom position (left/top ignored unless absolute flag set)
 *   - 1: Top left       5: Center left
 *   - 2: Top center     6: Center
 *   - 3: Top right      7: Center right
 *   - 4: Bottom left    8: Bottom center
 *   - 9: Bottom right
 *
 * Flag values (combine with bitwise OR):
 *   - 0x00: Use position mode
 *   - 0x02: Absolute position (use left/top directly)
 *   - 0x04: Transparent background
 *   - 0x08: Draw underneath text
 *
 * @param name (string) Unique window identifier
 * @param left (number) Left position or X offset
 * @param top (number) Top position or Y offset
 * @param width (number) Window width in pixels (0 for font setup)
 * @param height (number) Window height in pixels (0 for font setup)
 * @param position (number) Position mode (0-9)
 * @param flags (number) Positioning and drawing flags
 * @param bgColor (number) Background color in BGR format (0x00BBGGRR)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *
 * @example
 * -- Create a 200x100 window at top-left with blue background
 * WindowCreate("mywin", 0, 0, 200, 100, 1, 0, 0xFF0000)
 *
 * -- Create at absolute position (10, 20) with red background
 * WindowCreate("stats", 10, 20, 150, 80, 0, 2, 0x0000FF)
 *
 * @see WindowShow, WindowDelete, WindowResize, WindowPosition
 */
int L_WindowCreate(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 left = luaL_checkinteger(L, 2);
    qint32 top = luaL_checkinteger(L, 3);
    qint32 width = luaL_checkinteger(L, 4);
    qint32 height = luaL_checkinteger(L, 5);
    qint16 position = luaL_checkinteger(L, 6);
    qint32 flags = luaL_checkinteger(L, 7);
    QRgb bgColor = luaL_checkinteger(L, 8);

    QString windowName = QString::fromUtf8(name);

    // Allow 0x0 windows for initial font setup (common pattern in MUSHclient plugins)
    // They'll call WindowCreate again with proper dimensions after setting up fonts

    // Get or create miniwindow (WindowCreate reuses existing windows like original)
    MiniWindow* winPtr = nullptr;
    auto it = pDoc->m_MiniWindowMap.find(windowName);
    if (it != pDoc->m_MiniWindowMap.end()) {
        // Reuse existing window
        winPtr = it->second.get();
    } else {
        // Create new miniwindow
        auto win = std::make_unique<MiniWindow>(pDoc);
        win->setName(windowName);
        winPtr = win.get();
        pDoc->m_MiniWindowMap[windowName] = std::move(win);
    }

    // Update/set properties
    winPtr->setLocation(QPoint(left, top));
    winPtr->setPosition(position);
    winPtr->setFlags(flags);
    winPtr->setBackgroundColor(bgColor);

    // Create fresh pixmap if dimensions provided (allow 0x0 for initial font setup)
    // Unlike WindowResize, WindowCreate always creates a clean pixmap (no content preservation)
    if (width > 0 && height > 0) {
        // Use Resize method which handles pixmap creation and initialization
        winPtr->Resize(width, height, bgColor);
    }

    // Track creating plugin
    if (pDoc->m_CurrentPlugin) {
        winPtr->setCreatingPlugin(pDoc->m_CurrentPlugin->m_strID);
        winPtr->setCallbackPlugin(pDoc->m_CurrentPlugin->m_strID);
    }

    // Add to rendering order list
    pDoc->m_MiniWindowsOrder.append(windowName);

    // Emit signal so WorldWidget/OutputView can connect needsRedraw
    emit pDoc->miniwindowCreated(winPtr);

    return luaReturnOK(L);
}

/**
 * world.WindowShow(name, show)
 *
 * Shows or hides a miniwindow. Hidden windows are not drawn but
 * retain their contents and hotspots.
 *
 * @param name (string) Miniwindow name
 * @param show (boolean) true to show, false to hide
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Show the window
 * WindowShow("mywin", true)
 *
 * -- Hide the window
 * WindowShow("mywin", false)
 *
 * -- Toggle visibility
 * local visible = WindowInfo("mywin", 5)
 * WindowShow("mywin", not visible)
 *
 * @see WindowCreate, WindowDelete, WindowInfo
 */
int L_WindowShow(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    bool show = lua_toboolean(L, 2);

    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    win->show = show;
    win->dirty = true;

    qCDebug(lcScript) << "WindowShow:" << windowName << (show ? "SHOWN" : "HIDDEN");

    // Trigger redraw
    emit win->needsRedraw();

    return luaReturnOK(L);
}

/**
 * world.WindowPosition(name, left, top, position, flags)
 *
 * Changes the position of an existing miniwindow without recreating it.
 * Use this for animations or dynamic positioning.
 *
 * Position modes:
 *   - 0: Custom position    5: Center left
 *   - 1: Top left           6: Center
 *   - 2: Top center         7: Center right
 *   - 3: Top right          8: Bottom center
 *   - 4: Bottom left        9: Bottom right
 *
 * @param name (string) Miniwindow name
 * @param left (number) Left position (used with absolute flag)
 * @param top (number) Top position (used with absolute flag)
 * @param position (number) Position mode (0-9)
 * @param flags (number) Positioning flags (0x02 = absolute position)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Move to absolute position
 * WindowPosition("mywin", 100, 50, 0, 2)
 *
 * -- Snap to bottom-right corner
 * WindowPosition("mywin", 0, 0, 9, 0)
 *
 * @see WindowCreate, WindowInfo, WindowSetZOrder
 */
int L_WindowPosition(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 left = luaL_checkinteger(L, 2);
    qint32 top = luaL_checkinteger(L, 3);
    qint16 position = luaL_checkinteger(L, 4);
    qint32 flags = luaL_checkinteger(L, 5);

    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Update positioning
    win->location = QPoint(left, top);
    win->position = position;
    win->flags = flags;
    win->dirty = true;

    // Trigger redraw
    emit win->needsRedraw();

    return luaReturnOK(L);
}

/**
 * world.WindowSetZOrder(name, zOrder)
 *
 * Sets the z-order of a miniwindow for controlling draw order.
 * Lower z-order values draw first (underneath), higher values draw last (on top).
 * Windows with the same z-order are drawn in alphabetical order by name.
 *
 * @param name (string) Miniwindow name
 * @param zOrder (number) Z-order value (lower = draw first)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Put this window on top
 * WindowSetZOrder("tooltip", 100)
 *
 * -- Put this window behind others
 * WindowSetZOrder("background", -10)
 *
 * @see WindowCreate, WindowInfo, WindowPosition
 */
int L_WindowSetZOrder(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 zOrder = luaL_checkinteger(L, 2);

    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Update z-order
    win->zOrder = zOrder;
    win->dirty = true;

    // Trigger redraw
    emit win->needsRedraw();

    return luaReturnOK(L);
}

/**
 * world.WindowDelete(name)
 *
 * Deletes a miniwindow and frees all associated resources including
 * fonts, images, and hotspots. Cannot delete a window during its
 * own callback execution.
 *
 * @param name (string) Miniwindow name
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eItemInUse: Window is executing a script callback
 *
 * @example
 * -- Clean up a window when done
 * WindowDelete("mywin")
 *
 * -- Delete all windows from a list
 * for _, name in ipairs(WindowList()) do
 *     WindowDelete(name)
 * end
 *
 * @see WindowCreate, WindowList, WindowShow
 */
int L_WindowDelete(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Don't delete if currently executing script callback
    if (win->executingScript) {
        return luaReturnError(L, eItemInUse);
    }

    // Remove from order list
    pDoc->m_MiniWindowsOrder.removeAll(windowName);

    // Remove from map (unique_ptr automatically deletes)
    pDoc->m_MiniWindowMap.erase(windowName);

    return luaReturnOK(L);
}

/**
 * world.WindowInfo(name, infoType)
 *
 * Returns various information about a miniwindow.
 *
 * Info types:
 *   1 = left position (from WindowCreate)
 *   2 = top position (from WindowCreate)
 *   3 = width in pixels
 *   4 = height in pixels
 *   5 = show flag (true if visible)
 *   6 = hidden flag (true if hidden)
 *   7 = layout/position mode
 *   8 = flags value
 *   9 = background color (BGR)
 *   10 = rect left (actual position after layout)
 *   11 = rect top
 *   12 = rect right
 *   13 = rect bottom
 *   14 = last mouse X (miniwindow-relative)
 *   15 = last mouse Y (miniwindow-relative)
 *   16 = last mouse update count
 *   17 = client mouse X (output-window-relative)
 *   18 = client mouse Y (output-window-relative)
 *   19 = mouse-over hotspot ID (string)
 *   20 = mouse-down hotspot ID (string)
 *   22 = z-order value
 *
 * @param name (string) Miniwindow name
 * @param infoType (number) Type of information to retrieve (1-22)
 *
 * @return (varies) Requested information, or nil if window doesn't exist
 *
 * @example
 * -- Get window dimensions
 * local width = WindowInfo("mywin", 3)
 * local height = WindowInfo("mywin", 4)
 * Note("Window size: " .. width .. "x" .. height)
 *
 * -- Check if window is visible
 * if WindowInfo("mywin", 5) then
 *     Note("Window is visible")
 * end
 *
 * -- Get mouse position in window
 * local mx = WindowInfo("mywin", 14)
 * local my = WindowInfo("mywin", 15)
 *
 * @see WindowCreate, WindowShow, WindowPosition
 */
int L_WindowInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    int infoType = luaL_checkinteger(L, 2);

    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    switch (infoType) {
        case 1: // Left position (from WindowCreate)
            lua_pushnumber(L, win->location.x());
            break;

        case 2: // Top position (from WindowCreate)
            lua_pushnumber(L, win->location.y());
            break;

        case 3: // Width
            lua_pushnumber(L, win->width);
            break;

        case 4: // Height
            lua_pushnumber(L, win->height);
            break;

        case 5: // Show flag (visible)
            lua_pushboolean(L, win->show);
            break;

        case 6: // Hidden flag (opposite of visible)
            lua_pushboolean(L, !win->show);
            break;

        case 7: // Layout/position mode
            lua_pushnumber(L, win->position);
            break;

        case 8: // Flags
            lua_pushnumber(L, win->flags);
            break;

        case 9: // Background color
            lua_pushnumber(L, win->backgroundColor);
            break;

        case 10: // Rect left (current position after layout)
            lua_pushnumber(L, win->rect.left());
            break;

        case 11: // Rect top
            lua_pushnumber(L, win->rect.top());
            break;

        case 12: // Rect right
            lua_pushnumber(L, win->rect.right());
            break;

        case 13: // Rect bottom
            lua_pushnumber(L, win->rect.bottom());
            break;

        case 14: // Last mouse X position (miniwindow-relative)
            lua_pushnumber(L, win->lastMousePosition.x());
            break;

        case 15: // Last mouse Y position (miniwindow-relative)
            lua_pushnumber(L, win->lastMousePosition.y());
            break;

        case 16: // Last mouse update count - TODO: implement update tracking
            lua_pushnil(L);
            break;

        case 17: // Client mouse X position (output-window-relative)
            lua_pushnumber(L, win->clientMousePosition.x());
            break;

        case 18: // Client mouse Y position (output-window-relative)
            lua_pushnumber(L, win->clientMousePosition.y());
            break;

        case 19: // Mouse-over hotspot ID
            if (win->mouseOverHotspot.isEmpty()) {
                lua_pushstring(L, "");
            } else {
                lua_pushstring(L, win->mouseOverHotspot.toUtf8().constData());
            }
            break;

        case 20: // Mouse-down hotspot ID
            if (win->mouseDownHotspot.isEmpty()) {
                lua_pushstring(L, "");
            } else {
                lua_pushstring(L, win->mouseDownHotspot.toUtf8().constData());
            }
            break;

        case 22: // Z-order
            lua_pushnumber(L, win->zOrder);
            break;

        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.WindowResize(name, width, height, backgroundColor)
 *
 * Resizes a miniwindow and recreates its drawing surface.
 * Existing content is preserved where possible; new areas are
 * filled with the background color.
 *
 * @param name (string) Miniwindow name
 * @param width (number) New width in pixels
 * @param height (number) New height in pixels
 * @param backgroundColor (number) Background color (BGR format)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Resize window to 300x200 with black background
 * WindowResize("mywin", 300, 200, 0x000000)
 *
 * -- Double the window size
 * local w = WindowInfo("mywin", 3)
 * local h = WindowInfo("mywin", 4)
 * WindowResize("mywin", w * 2, h * 2, 0x000000)
 *
 * @see WindowCreate, WindowInfo
 */
int L_WindowResize(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 width = luaL_checkinteger(L, 2);
    qint32 height = luaL_checkinteger(L, 3);
    QRgb bgColor = luaL_checkinteger(L, 4);

    QString windowName = QString::fromUtf8(name);

    qCDebug(lcScript) << "WindowResize: Resizing miniwindow" << windowName << "to size:" << width
                      << "x" << height;

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        qCDebug(lcScript) << "WindowResize: Miniwindow" << windowName << "not found!";
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->Resize(width, height, bgColor);
    qCDebug(lcScript) << "WindowResize: Result:" << result;
    lua_pushnumber(L, result);
    return 1;
}

// ========== Miniwindow Drawing Primitives ==========

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

// ========== Miniwindow Text and Fonts ==========

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

// ========== Miniwindow Image Loading ==========

/**
 * world.WindowLoadImage(name, imageId, filename)
 *
 * Loads an image file into a miniwindow for later drawing.
 * Supports PNG, BMP, JPG, GIF, and other common formats.
 * Pass an empty filename to remove a previously loaded image.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Unique ID to reference this image
 * @param filename (string) Path to image file, or "" to remove
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eFileNotFound (30051): Image file not found or unreadable
 *
 * @example
 * -- Load an image
 * WindowLoadImage("mywin", "logo", "C:/Images/logo.png")
 * WindowDrawImage("mywin", "logo", 10, 10, 0, 0, 1)
 *
 * -- Remove the image to free memory
 * WindowLoadImage("mywin", "logo", "")
 *
 * @see WindowDrawImage, WindowImageInfo, WindowImageList, WindowLoadImageMemory
 */
int L_WindowLoadImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* filename = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    QString qImageId = QString::fromUtf8(imageId);
    QString qFilename = QString::fromUtf8(filename);

    // Get miniwindow
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Load the image file using Qt
    qFilename = qFilename.trimmed();

    // Empty filename means remove the image
    if (qFilename.isEmpty()) {
        // Erase from map (unique_ptr automatically deletes)
        win->images.erase(qImageId);
        return luaReturnOK(L);
    }

    // Load the image as a QImage
    auto image = std::make_unique<QImage>(qFilename);
    if (image->isNull()) {
        lua_pushnumber(L, 30051); // eFileNotFound
        return 1;
    }

    // Store in miniwindow's image map (replaces old one if it exists; old unique_ptr automatically
    // deletes)
    win->images[qImageId] = std::move(image);

    return luaReturnOK(L);
}

/**
 * world.WindowDrawImage(name, imageId, left, top, right, bottom, mode, srcLeft, srcTop, srcRight,
 *                       srcBottom)
 *
 * Draws a loaded image to the miniwindow with optional scaling and
 * sprite sheet support. Source coordinates allow drawing a portion.
 *
 * Draw modes:
 *   1 = Copy (opaque)
 *   2 = Transparent copy (top-left pixel color is transparent)
 *   3 = Stretch to fit destination rectangle
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (from WindowLoadImage)
 * @param left (number) Destination left
 * @param top (number) Destination top
 * @param right (number) Destination right (0 = use image width)
 * @param bottom (number) Destination bottom (0 = use image height)
 * @param mode (number) Draw mode (1-3)
 * @param srcLeft (number) Source left (default 0)
 * @param srcTop (number) Source top (default 0)
 * @param srcRight (number) Source right (default 0 = full width)
 * @param srcBottom (number) Source bottom (default 0 = full height)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eImageNotInstalled: Image not found
 *
 * @example
 * -- Draw image at position
 * WindowLoadImage("mywin", "bg", "background.png")
 * WindowDrawImage("mywin", "bg", 0, 0, 0, 0, 1)
 *
 * -- Draw a sprite from a sheet (extract 32x32 tile at row 2, col 3)
 * WindowDrawImage("mywin", "sheet", 10, 10, 42, 42, 1, 64, 32, 96, 64)
 *
 * @see WindowLoadImage, WindowBlendImage, WindowDrawImageAlpha
 */
int L_WindowDrawImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint16 mode = luaL_checkinteger(L, 7);
    qint32 srcLeft = luaL_optinteger(L, 8, 0);
    qint32 srcTop = luaL_optinteger(L, 9, 0);
    qint32 srcRight = luaL_optinteger(L, 10, 0);
    qint32 srcBottom = luaL_optinteger(L, 11, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->DrawImage(QString::fromUtf8(imageId), left, top, right, bottom, mode,
                                   srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowBlendImage(name, imageId, left, top, right, bottom, mode, opacity, srcLeft, srcTop,
 *                        srcRight, srcBottom)
 *
 * Draws an image with opacity and blend modes for visual effects.
 *
 * Blend modes:
 *   1 = Normal (alpha blend)
 *   2 = Multiply (darkens)
 *   3 = Screen (lightens)
 *   4 = Overlay (contrast)
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param left (number) Destination left
 * @param top (number) Destination top
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param mode (number) Blend mode (1-4)
 * @param opacity (number) Opacity from 0.0 (transparent) to 1.0 (opaque)
 * @param srcLeft (number) Source left (default 0)
 * @param srcTop (number) Source top (default 0)
 * @param srcRight (number) Source right (default 0)
 * @param srcBottom (number) Source bottom (default 0)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw semi-transparent image
 * WindowBlendImage("mywin", "overlay", 0, 0, 0, 0, 1, 0.5, 0, 0, 0, 0)
 *
 * -- Apply darkening effect with multiply blend
 * WindowBlendImage("mywin", "shadow", 0, 0, 0, 0, 2, 0.7, 0, 0, 0, 0)
 *
 * @see WindowDrawImage, WindowDrawImageAlpha, WindowMergeImageAlpha
 */
int L_WindowBlendImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint16 mode = luaL_checkinteger(L, 7);
    double opacity = luaL_checknumber(L, 8);
    qint32 srcLeft = luaL_optinteger(L, 9, 0);
    qint32 srcTop = luaL_optinteger(L, 10, 0);
    qint32 srcRight = luaL_optinteger(L, 11, 0);
    qint32 srcBottom = luaL_optinteger(L, 12, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->BlendImage(QString::fromUtf8(imageId), left, top, right, bottom, mode,
                                    opacity, srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageFromWindow(name, imageId, srcWindowName)
 *
 * Copies the contents of another miniwindow as an image.
 * Useful for double-buffering or creating window snapshots.
 *
 * @param name (string) Destination miniwindow name
 * @param imageId (string) Image identifier to store under
 * @param srcWindowName (string) Source miniwindow name to copy from
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Destination window doesn't exist
 *
 * @example
 * -- Create a snapshot of another window
 * WindowImageFromWindow("mywin", "snapshot", "otherwin")
 * WindowDrawImage("mywin", "snapshot", 0, 0, 0, 0, 1)
 *
 * @see WindowLoadImage, WindowDrawImage
 */
int L_WindowImageFromWindow(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* srcWindowName = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->ImageFromWindow(QString::fromUtf8(imageId), pDoc, QString::fromUtf8(srcWindowName));
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageInfo(name, imageId, infoType)
 *
 * Returns information about a loaded image.
 *
 * Info types:
 *   1 = Image width in pixels
 *   2 = Image height in pixels
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param infoType (number) Type of information (1 or 2)
 *
 * @return (number) Requested info, or nil if image/window not found
 *
 * @example
 * -- Get image dimensions
 * local width = WindowImageInfo("mywin", "logo", 1)
 * local height = WindowImageInfo("mywin", "logo", 2)
 * Note("Image size: " .. width .. "x" .. height)
 *
 * @see WindowLoadImage, WindowImageList
 */
int L_WindowImageInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 infoType = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    QVariant result = win->ImageInfo(QString::fromUtf8(imageId), infoType);
    if (!result.isValid()) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, result.toInt());
    }
    return 1;
}

/**
 * world.WindowImageList(name)
 *
 * Returns a table of all loaded image IDs in a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (table) Array of image ID strings (1-indexed), or empty table
 *
 * @example
 * -- List all images in the window
 * local images = WindowImageList("mywin")
 * for i, id in ipairs(images) do
 *     local w = WindowImageInfo("mywin", id, 1)
 *     local h = WindowImageInfo("mywin", id, 2)
 *     Note("Image: " .. id .. " (" .. w .. "x" .. h .. ")")
 * end
 *
 * @see WindowLoadImage, WindowImageInfo
 */
int L_WindowImageList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_newtable(L); // Return empty table
        return 1;
    }

    QStringList imageIds = win->ImageList();

    // Create Lua table
    lua_newtable(L);
    for (int i = 0; i < imageIds.size(); ++i) {
        lua_pushstring(L, imageIds[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1); // Lua tables are 1-indexed
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
 * world.WindowGetImageAlpha(name, imageId, left, top, right, bottom, srcLeft, srcTop)
 *
 * Extracts the alpha channel from a 32-bit image and draws it
 * as a grayscale image (white = opaque, black = transparent).
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (must have alpha channel)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Visualize the alpha channel of an image
 * WindowGetImageAlpha("mywin", "sprite", 0, 0, 0, 0, 0, 0)
 *
 * @see WindowDrawImageAlpha, WindowMergeImageAlpha
 */
int L_WindowGetImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint32 srcLeft = luaL_checkinteger(L, 7);
    qint32 srcTop = luaL_checkinteger(L, 8);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->GetImageAlpha(QString::fromUtf8(imageId), left, top, right, bottom, srcLeft, srcTop);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowDrawImageAlpha(name, imageId, left, top, right, bottom, opacity, srcLeft, srcTop)
 *
 * Draws a 32-bit image using its embedded alpha channel for transparency.
 * An additional opacity multiplier can be applied.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (must have alpha channel)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param opacity (number) Additional opacity (0.0 = transparent, 1.0 = use image alpha)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw PNG with transparency
 * WindowLoadImage("mywin", "icon", "icon.png")
 * WindowDrawImageAlpha("mywin", "icon", 10, 10, 0, 0, 1.0, 0, 0)
 *
 * -- Draw at 50% opacity
 * WindowDrawImageAlpha("mywin", "icon", 10, 10, 0, 0, 0.5, 0, 0)
 *
 * @see WindowGetImageAlpha, WindowMergeImageAlpha, WindowBlendImage
 */
int L_WindowDrawImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    double opacity = luaL_checknumber(L, 7);
    qint32 srcLeft = luaL_checkinteger(L, 8);
    qint32 srcTop = luaL_checkinteger(L, 9);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->DrawImageAlpha(QString::fromUtf8(imageId), left, top, right, bottom,
                                        opacity, srcLeft, srcTop);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowMergeImageAlpha(name, imageId, maskId, left, top, right, bottom, mode, opacity,
 *                             srcLeft, srcTop, srcRight, srcBottom)
 *
 * Draws an image using a separate grayscale image as an alpha mask.
 * White areas in the mask are opaque, black areas are transparent.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Main image identifier
 * @param maskId (string) Mask image identifier (grayscale)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param mode (number) Blend mode (0 = normal, 1 = color key)
 * @param opacity (number) Additional opacity (0.0-1.0)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 * @param srcRight (number) Source right coordinate
 * @param srcBottom (number) Source bottom coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Apply a mask to an image
 * WindowLoadImage("mywin", "photo", "photo.jpg")
 * WindowLoadImage("mywin", "mask", "mask.png")
 * WindowMergeImageAlpha("mywin", "photo", "mask", 0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0)
 *
 * @see WindowDrawImageAlpha, WindowGetImageAlpha
 */
int L_WindowMergeImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* maskId = luaL_checkstring(L, 3);
    qint32 left = luaL_checkinteger(L, 4);
    qint32 top = luaL_checkinteger(L, 5);
    qint32 right = luaL_checkinteger(L, 6);
    qint32 bottom = luaL_checkinteger(L, 7);
    qint16 mode = luaL_checkinteger(L, 8);
    double opacity = luaL_checknumber(L, 9);
    qint32 srcLeft = luaL_checkinteger(L, 10);
    qint32 srcTop = luaL_checkinteger(L, 11);
    qint32 srcRight = luaL_checkinteger(L, 12);
    qint32 srcBottom = luaL_checkinteger(L, 13);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->MergeImageAlpha(QString::fromUtf8(imageId), QString::fromUtf8(maskId), left, top,
                             right, bottom, mode, opacity, srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowTransformImage(name, imageId, left, top, mode, mxx, mxy, myx, myy)
 *
 * Applies an affine transformation to an image (rotate, scale, skew).
 * Uses a 2x2 transformation matrix.
 *
 * Common transformations:
 *   Rotation by θ: mxx=cos(θ), mxy=sin(θ), myx=-sin(θ), myy=cos(θ)
 *   Scale by s: mxx=s, mxy=0, myx=0, myy=s
 *   Horizontal flip: mxx=-1, mxy=0, myx=0, myy=1
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param left (number) X translation offset (destination position)
 * @param top (number) Y translation offset (destination position)
 * @param mode (number) Draw mode (1 = opaque, 3 = transparent)
 * @param mxx (number) Matrix element [0][0]
 * @param mxy (number) Matrix element [0][1]
 * @param myx (number) Matrix element [1][0]
 * @param myy (number) Matrix element [1][1]
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Rotate image 45 degrees
 * local angle = math.rad(45)
 * local cos, sin = math.cos(angle), math.sin(angle)
 * WindowTransformImage("mywin", "img", 100, 100, 1, cos, sin, -sin, cos)
 *
 * -- Scale image to 50%
 * WindowTransformImage("mywin", "img", 0, 0, 1, 0.5, 0, 0, 0.5)
 *
 * @see WindowDrawImage, WindowBlendImage
 */
int L_WindowTransformImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    float left = luaL_checknumber(L, 3);
    float top = luaL_checknumber(L, 4);
    qint16 mode = luaL_checkinteger(L, 5);
    float mxx = luaL_checknumber(L, 6);
    float mxy = luaL_checknumber(L, 7);
    float myx = luaL_checknumber(L, 8);
    float myy = luaL_checknumber(L, 9);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->TransformImage(QString::fromUtf8(imageId), left, top, mode, mxx, mxy, myx, myy);
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

// ========== Miniwindow Hotspots ==========

/**
 * world.WindowAddHotspot(name, hotspotId, left, top, right, bottom,
 *                        mouseOver, cancelMouseOver, mouseDown, cancelMouseDown,
 *                        mouseUp, tooltipText, cursor, flags)
 *
 * Creates an interactive hotspot (clickable area) in a miniwindow.
 * Hotspots respond to mouse events with Lua callback functions.
 *
 * Cursor types:
 *   0 = Arrow (default)
 *   1 = Hand/pointer
 *   6 = I-beam (text)
 *   11 = Cross
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Unique identifier for this hotspot
 * @param left (number) Left coordinate (0 or negative = relative to right edge)
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate (0 or negative = window edge)
 * @param bottom (number) Bottom coordinate (0 or negative = window edge)
 * @param mouseOver (string) Function called when mouse enters
 * @param cancelMouseOver (string) Function called when mouse leaves
 * @param mouseDown (string) Function called on mouse button press
 * @param cancelMouseDown (string) Function called if released outside
 * @param mouseUp (string) Function called on mouse button release inside
 * @param tooltipText (string) Tooltip to show on hover
 * @param cursor (number) Cursor type to display
 * @param flags (number) Hotspot behavior flags
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a clickable button
 * WindowAddHotspot("mywin", "btn1", 10, 10, 100, 40,
 *     "", "",           -- mouse over/cancel
 *     "", "",           -- mouse down/cancel
 *     "OnButtonClick",  -- mouse up
 *     "Click me!",      -- tooltip
 *     1, 0)             -- hand cursor, no flags
 *
 * function OnButtonClick(flags, hotspotId)
 *     Note("Button clicked!")
 * end
 *
 * @see WindowDeleteHotspot, WindowMoveHotspot, WindowHotspotInfo, WindowDragHandler
 */
int L_WindowAddHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Extract all parameters (14 total)
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    const char* mouseOver = luaL_optstring(L, 7, "");
    const char* cancelMouseOver = luaL_optstring(L, 8, "");
    const char* mouseDown = luaL_optstring(L, 9, "");
    const char* cancelMouseDown = luaL_optstring(L, 10, "");
    const char* mouseUp = luaL_optstring(L, 11, "");
    const char* tooltipText = luaL_optstring(L, 12, "");
    qint32 cursor = luaL_optinteger(L, 13, 0);
    qint32 flags = luaL_optinteger(L, 14, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Create or replace hotspot
    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto hotspot = std::make_unique<Hotspot>();

    // Handle special case: right=0 or bottom=0 means "use window edge"
    // CMiniWindow::AddHotSpot
    if (right <= 0) {
        right = win->width + right; // 0 becomes width, -1 becomes width-1, etc.
    }
    if (bottom <= 0) {
        bottom = win->height + bottom; // 0 becomes height, -1 becomes height-1, etc.
    }

    // Set rectangle (miniwindow-relative coordinates)
    hotspot->m_rect = QRect(left, top, right - left, bottom - top);

    // Set mouse event callbacks
    hotspot->m_sMouseOver = QString::fromUtf8(mouseOver);
    hotspot->m_sCancelMouseOver = QString::fromUtf8(cancelMouseOver);
    hotspot->m_sMouseDown = QString::fromUtf8(mouseDown);
    hotspot->m_sCancelMouseDown = QString::fromUtf8(cancelMouseDown);
    hotspot->m_sMouseUp = QString::fromUtf8(mouseUp);

    // Set tooltip
    hotspot->m_sTooltipText = QString::fromUtf8(tooltipText);

    // Set cursor and flags
    hotspot->m_Cursor = cursor;
    hotspot->m_Flags = flags;

    // Store in miniwindow's hotspot map (replaces old one if it exists; old unique_ptr
    // automatically deletes)
    win->hotspots[hotspotIdStr] = std::move(hotspot);

    return luaReturnOK(L);
}

/**
 * world.WindowDeleteHotspot(name, hotspotId)
 *
 * Deletes a specific hotspot from a miniwindow.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to delete
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Remove a button
 * WindowDeleteHotspot("mywin", "btn1")
 *
 * @see WindowAddHotspot, WindowDeleteAllHotspots, WindowHotspotList
 */
int L_WindowDeleteHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end()) {
        return luaReturnError(L, eHotspotNotInstalled);
    }

    // Delete the hotspot
    win->hotspots.erase(it);

    return luaReturnOK(L);
}

/**
 * world.WindowDeleteAllHotspots(name)
 *
 * Deletes all hotspots from a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Clear all hotspots before rebuilding UI
 * WindowDeleteAllHotspots("mywin")
 *
 * @see WindowAddHotspot, WindowDeleteHotspot, WindowHotspotList
 */
int L_WindowDeleteAllHotspots(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Clear all hotspots
    win->hotspots.clear();

    return luaReturnOK(L);
}

/**
 * world.WindowHotspotTooltip(name, hotspotId, tooltipText)
 *
 * Sets or updates the tooltip text for a hotspot.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID
 * @param tooltipText (string) New tooltip text
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Update tooltip dynamically
 * WindowHotspotTooltip("mywin", "hp_bar", "HP: 50/100")
 *
 * @see WindowAddHotspot, WindowHotspotInfo
 */
int L_WindowHotspotTooltip(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* tooltipText = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end()) {
        return luaReturnError(L, eHotspotNotInstalled);
    }

    // Update tooltip
    it->second->m_sTooltipText = QString::fromUtf8(tooltipText);

    return luaReturnOK(L);
}

/**
 * world.WindowDragHandler(name, hotspotId, moveCallback, releaseCallback, flags)
 *
 * Sets up drag-and-drop handling for a miniwindow hotspot.
 * The hotspot must already exist (created with WindowAddHotspot).
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to configure
 * @param moveCallback (string) Function called during drag (on mouse move)
 * @param releaseCallback (string) Function called when drag ends
 * @param flags (number) Drag-and-drop flags
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Make a window draggable
 * WindowAddHotspot("mywin", "drag", 0, 0, 0, 0, "", "", "", "", "", "", 0, 0)
 * WindowDragHandler("mywin", "drag", "OnDrag", "OnDragEnd", 0)
 *
 * function OnDrag(flags, hotspotId, x, y)
 *     local newX = WindowInfo("mywin", 10) + x
 *     local newY = WindowInfo("mywin", 11) + y
 *     WindowPosition("mywin", newX, newY, 0, 2)
 * end
 *
 * @see WindowAddHotspot, WindowMoveHotspot, WindowScrollwheelHandler
 */
int L_WindowDragHandler(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* moveCallback = luaL_optstring(L, 3, "");
    const char* releaseCallback = luaL_optstring(L, 4, "");
    qint32 flags = luaL_optinteger(L, 5, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Set drag-and-drop callbacks and flags
    hotspot->m_sMoveCallback = QString::fromUtf8(moveCallback);
    hotspot->m_sReleaseCallback = QString::fromUtf8(releaseCallback);
    hotspot->m_DragFlags = flags;

    return luaReturnOK(L);
}

/**
 * world.WindowMenu(name, x, y, menuString)
 *
 * Shows a popup context menu and returns the selected item number.
 * Blocks until user selects an item or cancels.
 *
 * Menu prefix flags:
 *   ">" - Start submenu (text is submenu title)
 *   "<" - End submenu, return to parent
 *   "+" - Checked item (checkmark shown)
 *   "!" - Default/bold item
 *   "^" - Disabled/grayed item
 *   "-" - Separator line
 *   "~" - Column break (not implemented)
 *
 * @param name (string) Miniwindow name
 * @param x (number) X coordinate (miniwindow-relative)
 * @param y (number) Y coordinate (miniwindow-relative)
 * @param menuString (string) Pipe-separated menu items with prefix flags
 *
 * @return (string) Selected item's 1-based position number, or "" if canceled
 *
 * @example
 * -- Simple menu
 * local result = WindowMenu("mywin", 10, 10, "Attack|Defend|Run")
 * if result == "1" then Note("Attack!") end
 *
 * -- Menu with separator and disabled item
 * local result = WindowMenu("mywin", 10, 10, "New|Open|-|^Save|Save As")
 *
 * -- Menu with submenu
 * local result = WindowMenu("mywin", 10, 10, "File|>Edit|Cut|Copy|Paste|<|Help")
 *
 * @see WindowAddHotspot
 */
int L_WindowMenu(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 x = luaL_checkinteger(L, 2);
    qint32 y = luaL_checkinteger(L, 3);
    const char* menuString = luaL_checkstring(L, 4);

    QString windowName = QString::fromUtf8(name);
    QString menuStr = QString::fromUtf8(menuString);

    // Get miniwindow to convert coordinates
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushstring(L, "");
        return 1;
    }

    // Parse menu string (pipe-separated items)
    QStringList items = menuStr.split('|');

    // Create popup menu with submenu stack
    QMenu rootMenu;
    QStack<QMenu*> menuStack;
    menuStack.push(&rootMenu);

    QMap<QAction*, int> actionIndexMap; // Map actions to 1-based position index
    int itemIndex = 0; // Counter for item positions (1-based, only counting selectable items)

    for (const QString& item : items) {
        QString text = item.trimmed();

        if (text.isEmpty()) {
            continue;
        }

        // Parse prefix flags
        bool isSubmenu = false;
        bool isSubmenuEnd = false;
        bool isChecked = false;
        bool isDefault = false;
        bool isDisabled = false;
        bool isSeparator = false;

        // Process all prefix characters
        // MUSHclient menu prefixes:
        // + = checked item
        // ! = default item (bold) - NOT checked
        // ^ = disabled/grayed
        // - = separator
        // > = submenu start
        // < = submenu end
        // ~ = column break
        while (!text.isEmpty()) {
            QChar prefix = text.at(0);

            if (prefix == '>') {
                isSubmenu = true;
                text = text.mid(1);
            } else if (prefix == '<') {
                isSubmenuEnd = true;
                text = text.mid(1);
            } else if (prefix == '+') {
                isChecked = true;
                text = text.mid(1);
            } else if (prefix == '!') {
                // Default item (shown bold) - NOT a checkmark
                isDefault = true;
                text = text.mid(1);
            } else if (prefix == '^') {
                isDisabled = true;
                text = text.mid(1);
            } else if (prefix == '-') {
                isSeparator = true;
                text = text.mid(1);
                break; // Separator consumes the rest
            } else if (prefix == '~') {
                // Column break - skip (not easily supported in Qt)
                text = text.mid(1);
            } else {
                break; // No more prefix characters
            }
        }

        QMenu* currentMenu = menuStack.top();

        // Handle submenu end - go back up one level
        if (isSubmenuEnd) {
            if (menuStack.size() > 1) {
                menuStack.pop();
            }
            // If there's remaining text after '<', process it as a menu item
            if (text.isEmpty()) {
                continue;
            }
            currentMenu = menuStack.top();
        }

        // Handle separator
        if (isSeparator) {
            currentMenu->addSeparator();
            continue;
        }

        // Handle submenu start
        if (isSubmenu && !text.isEmpty()) {
            QMenu* submenu = currentMenu->addMenu(text);
            menuStack.push(submenu);
            continue;
        }

        // Handle regular menu item
        if (!text.isEmpty()) {
            itemIndex++; // Increment for each selectable item (1-based)
            QAction* action = currentMenu->addAction(text);

            if (isChecked) {
                action->setCheckable(true);
                action->setChecked(true);
            }

            if (isDefault) {
                // '!' prefix - just ignore, doesn't add checkmark
                // (Some docs say it means "default" but in practice it's often just ignored)
            }

            if (isDisabled) {
                action->setEnabled(false);
            }

            // Store the 1-based index for return value
            actionIndexMap[action] = itemIndex;
        }
    }

    // Convert miniwindow-relative coordinates to output-window coordinates
    QPoint outputPos = win->rect.topLeft() + QPoint(x, y);

    // Convert to global screen coordinates
    QPoint globalPos = outputPos;
    if (pDoc->m_pActiveOutputView) {
        globalPos = pDoc->m_pActiveOutputView->mapToGlobal(outputPos);
    }

    // Show menu at specified position
    QAction* selectedAction = rootMenu.exec(globalPos);

    // Return selected item's 1-based position number as string (or empty string if canceled)
    // MUSHclient returns the position number, not the text
    if (selectedAction && actionIndexMap.contains(selectedAction)) {
        int index = actionIndexMap[selectedAction];
        lua_pushstring(L, QString::number(index).toUtf8().constData());
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

/**
 * world.WindowMoveHotspot(name, hotspotId, left, top, right, bottom)
 *
 * Moves or resizes an existing hotspot to a new location.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to move
 * @param left (number) New left coordinate (0 or negative = relative to edge)
 * @param top (number) New top coordinate
 * @param right (number) New right coordinate (0 or negative = window edge)
 * @param bottom (number) New bottom coordinate (0 or negative = window edge)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Move button to new position
 * WindowMoveHotspot("mywin", "btn1", 50, 50, 150, 80)
 *
 * -- Resize to cover entire window
 * WindowMoveHotspot("mywin", "fullscreen", 0, 0, 0, 0)
 *
 * @see WindowAddHotspot, WindowHotspotInfo, WindowDragHandler
 */
int L_WindowMoveHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Handle special case: right=0 or bottom=0 means "use window edge"
    if (right <= 0) {
        right = win->width + right;
    }
    if (bottom <= 0) {
        bottom = win->height + bottom;
    }

    // Update hotspot rectangle
    hotspot->m_rect = QRect(left, top, right - left, bottom - top);

    return luaReturnOK(L);
}

/**
 * world.WindowHotspotInfo(name, hotspotId, infoType)
 *
 * Gets information about a miniwindow hotspot.
 *
 * Info types:
 *   1 = left coordinate
 *   2 = top coordinate
 *   3 = right coordinate
 *   4 = bottom coordinate
 *   5 = MouseOver callback name
 *   6 = CancelMouseOver callback name
 *   7 = MouseDown callback name
 *   8 = CancelMouseDown callback name
 *   9 = MouseUp callback name
 *   10 = Tooltip text
 *   11 = Cursor type
 *   12 = Flags
 *   13 = Drag move callback name
 *   14 = Drag release callback name
 *   15 = Drag flags
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID
 * @param infoType (number) Type of information (1-15)
 *
 * @return (varies) Requested information, or nil if hotspot doesn't exist
 *
 * @example
 * -- Get hotspot boundaries
 * local left = WindowHotspotInfo("mywin", "btn1", 1)
 * local top = WindowHotspotInfo("mywin", "btn1", 2)
 * local right = WindowHotspotInfo("mywin", "btn1", 3)
 * local bottom = WindowHotspotInfo("mywin", "btn1", 4)
 *
 * @see WindowAddHotspot, WindowMoveHotspot, WindowHotspotList
 */
int L_WindowHotspotInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 infoType = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L); // No such window
        return 1;
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        lua_pushnil(L); // No such hotspot
        return 1;
    }
    Hotspot* hotspot = it->second.get();

    // Return actual hotspot data
    switch (infoType) {
        case 1: // left
            lua_pushinteger(L, hotspot->m_rect.left());
            break;
        case 2: // top
            lua_pushinteger(L, hotspot->m_rect.top());
            break;
        case 3: // right
            lua_pushinteger(L, hotspot->m_rect.right());
            break;
        case 4: // bottom
            lua_pushinteger(L, hotspot->m_rect.bottom());
            break;
        case 5: // MouseOver callback
            lua_pushstring(L, hotspot->m_sMouseOver.toUtf8().constData());
            break;
        case 6: // CancelMouseOver callback
            lua_pushstring(L, hotspot->m_sCancelMouseOver.toUtf8().constData());
            break;
        case 7: // MouseDown callback
            lua_pushstring(L, hotspot->m_sMouseDown.toUtf8().constData());
            break;
        case 8: // CancelMouseDown callback
            lua_pushstring(L, hotspot->m_sCancelMouseDown.toUtf8().constData());
            break;
        case 9: // MouseUp callback
            lua_pushstring(L, hotspot->m_sMouseUp.toUtf8().constData());
            break;
        case 10: // TooltipText
            lua_pushstring(L, hotspot->m_sTooltipText.toUtf8().constData());
            break;
        case 11: // cursor code
            lua_pushinteger(L, hotspot->m_Cursor);
            break;
        case 12: // flags
            lua_pushinteger(L, hotspot->m_Flags);
            break;
        case 13: // MoveCallback (drag-and-drop)
            lua_pushstring(L, hotspot->m_sMoveCallback.toUtf8().constData());
            break;
        case 14: // ReleaseCallback (drag-and-drop)
            lua_pushstring(L, hotspot->m_sReleaseCallback.toUtf8().constData());
            break;
        case 15: // drag flags
            lua_pushinteger(L, hotspot->m_DragFlags);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.WindowScrollwheelHandler(name, hotspotId, scrollCallback)
 *
 * Sets up mouse scroll wheel handling for a hotspot.
 * The hotspot must already exist (created with WindowAddHotspot).
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to configure
 * @param scrollCallback (string) Function called on scroll wheel event
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Handle scroll events for zooming
 * WindowAddHotspot("mywin", "scroll_area", 0, 0, 0, 0, "", "", "", "", "", "", 0, 0)
 * WindowScrollwheelHandler("mywin", "scroll_area", "OnScroll")
 *
 * function OnScroll(flags, hotspotId, delta)
 *     if delta > 0 then
 *         Note("Scroll up")
 *     else
 *         Note("Scroll down")
 *     end
 * end
 *
 * @see WindowAddHotspot, WindowDragHandler
 */
int L_WindowScrollwheelHandler(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* scrollCallback = luaL_optstring(L, 3, "");

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Set scroll wheel callback
    hotspot->m_sScrollwheelCallback = QString::fromUtf8(scrollCallback);

    return luaReturnOK(L);
}

/**
 * world.WindowList()
 *
 * Returns a table of all miniwindow names in this world.
 *
 * @return (table) Array of window name strings (1-indexed)
 *
 * @example
 * -- List all windows
 * local windows = WindowList()
 * for i, name in ipairs(windows) do
 *     local visible = WindowInfo(name, 5)
 *     Note(name .. (visible and " (visible)" or " (hidden)"))
 * end
 *
 * -- Count windows
 * Note("Total windows: " .. #WindowList())
 *
 * @see WindowCreate, WindowDelete, WindowInfo
 */
int L_WindowList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    int index = 1;
    for (auto it = pDoc->m_MiniWindowMap.begin(); it != pDoc->m_MiniWindowMap.end(); ++it) {
        lua_pushstring(L, it->first.toUtf8().constData());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * world.WindowHotspotList(name)
 *
 * Returns a table of all hotspot IDs in a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (table) Array of hotspot ID strings (1-indexed), or nil if window not found
 *
 * @example
 * -- List all hotspots in a window
 * local hotspots = WindowHotspotList("mywin")
 * if hotspots then
 *     for i, id in ipairs(hotspots) do
 *         Note("Hotspot: " .. id)
 *     end
 * end
 *
 * @see WindowAddHotspot, WindowDeleteHotspot, WindowHotspotInfo
 */
int L_WindowHotspotList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    QString windowName = QString::fromUtf8(name);

    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    QStringList list = win->HotspotList();

    lua_newtable(L);
    for (int i = 0; i < list.size(); i++) {
        lua_pushstring(L, list[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/**
 * world.WindowCreateImage(name, imageId, row1, row2, row3, row4, row5, row6, row7, row8)
 *
 * Creates an 8x8 monochrome image from row bit patterns.
 * Each row is 8 bits where bit 7 is the leftmost pixel.
 * Useful for creating small icons or patterns programmatically.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param row1 (number) Bit pattern for row 1 (0-255)
 * @param row2 (number) Bit pattern for row 2
 * @param row3 (number) Bit pattern for row 3
 * @param row4 (number) Bit pattern for row 4
 * @param row5 (number) Bit pattern for row 5
 * @param row6 (number) Bit pattern for row 6
 * @param row7 (number) Bit pattern for row 7
 * @param row8 (number) Bit pattern for row 8
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a simple arrow pattern
 * WindowCreateImage("mywin", "arrow",
 *     0x18,  -- 00011000
 *     0x3C,  -- 00111100
 *     0x7E,  -- 01111110
 *     0xFF,  -- 11111111
 *     0x18,  -- 00011000
 *     0x18,  -- 00011000
 *     0x18,  -- 00011000
 *     0x18)  -- 00011000
 *
 * @see WindowLoadImage, WindowImageOp
 */
int L_WindowCreateImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 row1 = luaL_checkinteger(L, 3);
    qint32 row2 = luaL_checkinteger(L, 4);
    qint32 row3 = luaL_checkinteger(L, 5);
    qint32 row4 = luaL_checkinteger(L, 6);
    qint32 row5 = luaL_checkinteger(L, 7);
    qint32 row6 = luaL_checkinteger(L, 8);
    qint32 row7 = luaL_checkinteger(L, 9);
    qint32 row8 = luaL_checkinteger(L, 10);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->CreateImage(QString::fromUtf8(imageId), row1, row2, row3, row4, row5, row6,
                                     row7, row8);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageOp(name, action, left, top, right, bottom, penColor, penStyle, penWidth,
 *                     brushColor, imageId, ellipseWidth, ellipseHeight)
 *
 * Draws shapes using an image as a brush pattern for fills.
 *
 * Action codes:
 *   1 = Frame rectangle
 *   2 = Fill rectangle with image pattern
 *   3 = Rounded rectangle
 *   4 = Ellipse frame
 *   5 = Filled ellipse with image pattern
 *
 * @param name (string) Miniwindow name
 * @param action (number) Drawing operation (1-5)
 * @param left (number) Left coordinate
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate
 * @param bottom (number) Bottom coordinate
 * @param penColor (number) Outline color (BGR)
 * @param penStyle (number) Pen style (0=solid, etc.)
 * @param penWidth (number) Pen width in pixels
 * @param brushColor (number) Background color (BGR)
 * @param imageId (string) Image ID for brush pattern
 * @param ellipseWidth (number) Corner width for rounded rect (optional)
 * @param ellipseHeight (number) Corner height for rounded rect (optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a pattern image then use it as brush
 * WindowCreateImage("mywin", "dots", 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55)
 * WindowImageOp("mywin", 2, 10, 10, 100, 100, 0, 0, 0, 0xFFFFFF, "dots")
 *
 * @see WindowCreateImage, WindowRectOp, WindowCircleOp
 */
int L_WindowImageOp(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint16 action = luaL_checkinteger(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint32 penColor = luaL_checkinteger(L, 7);
    qint32 penStyle = luaL_checkinteger(L, 8);
    qint32 penWidth = luaL_checkinteger(L, 9);
    qint32 brushColor = luaL_checkinteger(L, 10);
    const char* imageId = luaL_checkstring(L, 11);
    qint32 ellipseWidth = luaL_optinteger(L, 12, 0);
    qint32 ellipseHeight = luaL_optinteger(L, 13, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->ImageOp(action, left, top, right, bottom, static_cast<QRgb>(penColor),
                                 penStyle, penWidth, static_cast<QRgb>(brushColor),
                                 QString::fromUtf8(imageId), ellipseWidth, ellipseHeight);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowLoadImageMemory(name, imageId, data, alpha)
 *
 * Loads an image from a string containing raw image data.
 * Supports PNG, BMP, JPG, GIF and other common formats.
 * Useful for loading embedded images or images from network.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param data (string) Binary string containing image data
 * @param alpha (boolean) true to preserve alpha channel (optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Load image from base64-encoded data
 * local imgData = base64.decode(encodedPng)
 * WindowLoadImageMemory("mywin", "icon", imgData, true)
 *
 * -- Load image from HTTP response
 * -- (assumes you have image data in a string)
 * WindowLoadImageMemory("mywin", "avatar", httpResponseBody, true)
 *
 * @see WindowLoadImage, WindowDrawImage
 */
int L_WindowLoadImageMemory(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);

    // Get data as string (can contain binary data including nulls)
    size_t len;
    const char* data = luaL_checklstring(L, 3, &len);
    bool alpha = lua_toboolean(L, 4);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->LoadImageMemory(QString::fromUtf8(imageId),
                                         reinterpret_cast<const unsigned char*>(data), len, alpha);

    lua_pushnumber(L, result);
    return 1;
}

// ========== Registration ==========

/**
 * register_miniwindow_functions - Register all miniwindow API functions
 *
 * Called from RegisterLuaRoutines() in lua_methods.cpp
 *
 * @param L Lua state
 * @param worldlib Array of luaL_Reg to append functions to
 * @param index Current index in worldlib array
 * @return New index after appending functions
 */
void register_miniwindow_functions(lua_State* L)
{
    // Register functions directly in the already-created "world" table
    lua_getglobal(L, "world");

    // Miniwindow functions
    lua_pushcfunction(L, L_WindowCreate);
    lua_setfield(L, -2, "WindowCreate");

    lua_pushcfunction(L, L_WindowShow);
    lua_setfield(L, -2, "WindowShow");

    lua_pushcfunction(L, L_WindowPosition);
    lua_setfield(L, -2, "WindowPosition");

    lua_pushcfunction(L, L_WindowSetZOrder);
    lua_setfield(L, -2, "WindowSetZOrder");

    lua_pushcfunction(L, L_WindowDelete);
    lua_setfield(L, -2, "WindowDelete");

    lua_pushcfunction(L, L_WindowInfo);
    lua_setfield(L, -2, "WindowInfo");

    lua_pushcfunction(L, L_WindowResize);
    lua_setfield(L, -2, "WindowResize");

    // Miniwindow drawing primitives
    lua_pushcfunction(L, L_WindowRectOp);
    lua_setfield(L, -2, "WindowRectOp");

    lua_pushcfunction(L, L_WindowCircleOp);
    lua_setfield(L, -2, "WindowCircleOp");

    lua_pushcfunction(L, L_WindowLine);
    lua_setfield(L, -2, "WindowLine");

    lua_pushcfunction(L, L_WindowPolygon);
    lua_setfield(L, -2, "WindowPolygon");

    lua_pushcfunction(L, L_WindowSetPixel);
    lua_setfield(L, -2, "WindowSetPixel");

    lua_pushcfunction(L, L_WindowGetPixel);
    lua_setfield(L, -2, "WindowGetPixel");

    // Miniwindow text and fonts
    lua_pushcfunction(L, L_WindowFont);
    lua_setfield(L, -2, "WindowFont");

    lua_pushcfunction(L, L_WindowText);
    lua_setfield(L, -2, "WindowText");

    lua_pushcfunction(L, L_WindowTextWidth);
    lua_setfield(L, -2, "WindowTextWidth");

    lua_pushcfunction(L, L_WindowFontInfo);
    lua_setfield(L, -2, "WindowFontInfo");

    lua_pushcfunction(L, L_WindowFontList);
    lua_setfield(L, -2, "WindowFontList");

    // Miniwindow image loading
    lua_pushcfunction(L, L_WindowLoadImage);
    lua_setfield(L, -2, "WindowLoadImage");

    // Miniwindow hotspots
    lua_pushcfunction(L, L_WindowAddHotspot);
    lua_setfield(L, -2, "WindowAddHotspot");

    lua_pushcfunction(L, L_WindowDragHandler);
    lua_setfield(L, -2, "WindowDragHandler");

    lua_pushcfunction(L, L_WindowMenu);
    lua_setfield(L, -2, "WindowMenu");

    lua_pushcfunction(L, L_WindowHotspotInfo);
    lua_setfield(L, -2, "WindowHotspotInfo");

    lua_pushcfunction(L, L_WindowMoveHotspot);
    lua_setfield(L, -2, "WindowMoveHotspot");

    lua_pushcfunction(L, L_WindowScrollwheelHandler);
    lua_setfield(L, -2, "WindowScrollwheelHandler");

    // Miniwindow list and image functions
    lua_pushcfunction(L, L_WindowList);
    lua_setfield(L, -2, "WindowList");

    lua_pushcfunction(L, L_WindowHotspotList);
    lua_setfield(L, -2, "WindowHotspotList");

    lua_pushcfunction(L, L_WindowCreateImage);
    lua_setfield(L, -2, "WindowCreateImage");

    lua_pushcfunction(L, L_WindowImageOp);
    lua_setfield(L, -2, "WindowImageOp");

    lua_pushcfunction(L, L_WindowLoadImageMemory);
    lua_setfield(L, -2, "WindowLoadImageMemory");

    lua_pop(L, 1); // Pop "world" table
}
