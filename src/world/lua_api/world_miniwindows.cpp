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
 * Miniwindow Creation
 * Creates or updates a miniwindow.
 *
 * @param name Miniwindow name
 * @param left, top Position coordinates
 * @param width, height Dimensions in pixels
 * @param position Position mode (0-8: center, corners, edges)
 * @param flags Positioning flags (2=absolute location)
 * @param bgColor Background color (RGB)
 * @return Error code (eOK on success)
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
 * Miniwindow Creation
 * Shows or hides a miniwindow.
 *
 * @param name Miniwindow name
 * @param show true to show, false to hide
 * @return Error code (eOK on success)
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
 * Miniwindow Creation
 * Changes the position of an existing miniwindow.
 *
 * @param name Miniwindow name
 * @param left Left coordinate (for absolute positioning)
 * @param top Top coordinate (for absolute positioning)
 * @param position Position mode (0-8: center, corners, edges)
 * @param flags Positioning flags (2=absolute location)
 * @return Error code (eOK on success)
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
 * Miniwindow Z-Order
 * Sets the z-order of a miniwindow for controlling draw order.
 *
 * CMUSHclientDoc::WindowSetZOrder
 *
 * Lower z-order values draw first (underneath), higher values draw last (on top).
 * Windows with the same z-order are drawn in alphabetical order by name.
 *
 * @param name Miniwindow name
 * @param zOrder Z-order value (lower = draw first)
 * @return Error code (eOK on success, eNoSuchWindow if window doesn't exist)
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
 * Miniwindow Creation
 * Deletes a miniwindow and all its resources (fonts, images, hotspots).
 *
 * @param name Miniwindow name
 * @return Error code (eOK on success)
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
 * Miniwindow Creation
 * Returns information about a miniwindow.
 *
 * @param name Miniwindow name
 * @param infoType Information type (MUSHclient compatible):
 *   1 = left position (from WindowCreate)
 *   2 = top position (from WindowCreate)
 *   3 = width
 *   4 = height
 *   5 = show flag (visible)
 *   6 = hidden flag (opposite of visible)
 *   7 = layout/position mode
 *   8 = flags
 *   9 = background color
 *   10 = rect left (current position after layout)
 *   11 = rect top
 *   12 = rect right
 *   13 = rect bottom
 *   14 = last mouse x position (miniwindow-relative)
 *   15 = last mouse y position (miniwindow-relative)
 *   16 = last mouse update count
 *   17 = client mouse x position (output-window-relative)
 *   18 = client mouse y position (output-window-relative)
 *   19 = mouse-over hotspot ID
 *   20 = mouse-down hotspot ID
 *   22 = z-order
 * @return Value (or nil if not found)
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
 * Miniwindow Creation
 * Resize miniwindow and recreate offscreen pixmap.
 *
 * @param name Miniwindow name
 * @param width New width in pixels
 * @param height New height in pixels
 * @param backgroundColor New background color (RGB)
 * @return Error code (eOK on success)
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
 * Drawing Primitives
 * Draw rectangle with various operations
 *
 * @param name Miniwindow name
 * @param action 1=frame, 2=fill, 3=invert, 5=3D rect
 * @param left, top, right, bottom Rectangle coordinates
 * @param penColor Pen color (RGB)
 * @param brushColor Brush color (RGB)
 * @return Error code
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
 * Drawing Primitives
 * Draw circle/ellipse with various operations
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
 * Drawing Primitives
 * Draw line
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
 * winding)
 *
 * Drawing Primitives
 * Draw polygon
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
 * Drawing Primitives
 * Draw gradient fill
 *
 * @param name Miniwindow name
 * @param left Left coordinate
 * @param top Top coordinate
 * @param right Right coordinate
 * @param bottom Bottom coordinate
 * @param color1 Start color (ARGB)
 * @param color2 End color (ARGB)
 * @param mode Gradient mode (1=horizontal, 2=vertical)
 * @return Error code (eOK on success)
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
 * Drawing Primitives
 * Set pixel color
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
 * Drawing Primitives
 * Get pixel color
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
 * Drawing Primitives
 * Draw arc (portion of ellipse)
 *
 * @param name Miniwindow name
 * @param left Left coordinate of bounding rectangle
 * @param top Top coordinate of bounding rectangle
 * @param right Right coordinate of bounding rectangle
 * @param bottom Bottom coordinate of bounding rectangle
 * @param x1 X coordinate of arc start point
 * @param y1 Y coordinate of arc start point
 * @param x2 X coordinate of arc end point
 * @param y2 Y coordinate of arc end point
 * @param penColor Pen color (ARGB)
 * @param penStyle Pen style (0=solid, 1=dash, 2=dot, etc.)
 * @param penWidth Pen width in pixels
 * @return Error code (eOK on success)
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
 * Drawing Primitives
 * Draw Bezier curve
 *
 * @param name Miniwindow name
 * @param points Comma-separated coordinate pairs (x1,y1,x2,y2,...) - must be (3n+1) points
 * @param penColor Pen color (ARGB)
 * @param penStyle Pen style (0=solid, 1=dash, 2=dot, etc.)
 * @param penWidth Pen width in pixels
 * @return Error code (eOK on success)
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
 * Text and Font Management
 * Create or update font
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
 * Text and Font Management
 * Draw text
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
 * Text and Font Management
 * Measure text width
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
 * Text and Font Management
 * Get font information
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
 * Text and Font Management
 * Get list of all font IDs in miniwindow
 *
 * @param name Miniwindow name
 * @return Table (array) of font IDs
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
 *
 * @param name - Miniwindow name
 * @param imageId - Unique ID for this image within the window
 * @param filename - Path to image file (PNG, BMP, JPG, etc.)
 * @return eOK on success, error code on failure
 *
 * The image is loaded into memory and cached with the given imageId.
 * You can then use WindowDrawImage to actually draw it.
 *
 * Based on methods_miniwindows.cpp:WindowLoadImage
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
 * srcBottom)
 *
 * Image Operations
 * Draw a loaded image to the miniwindow with optional scaling and sprite sheet support.
 *
 * @param name Miniwindow name
 * @param imageId Image identifier
 * @param left Destination left
 * @param top Destination top
 * @param right Destination right
 * @param bottom Destination bottom
 * @param mode Draw mode (1=copy, 2=transparent_copy)
 * @param srcLeft Source left (default 0)
 * @param srcTop Source top (default 0)
 * @param srcRight Source right (default 0 = full width)
 * @param srcBottom Source bottom (default 0 = full height)
 * @return Error code (eOK on success)
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
 * srcRight, srcBottom)
 *
 * Image Operations
 * Draw a loaded image with opacity and blend modes for visual effects.
 *
 * @param name Miniwindow name
 * @param imageId Image identifier
 * @param left Destination left
 * @param top Destination top
 * @param right Destination right
 * @param bottom Destination bottom
 * @param mode Blend mode (1=normal, 2=multiply, 3=screen, 4=overlay)
 * @param opacity Opacity (0.0-1.0)
 * @param srcLeft Source left (default 0)
 * @param srcTop Source top (default 0)
 * @param srcRight Source right (default 0)
 * @param srcBottom Source bottom (default 0)
 * @return Error code (eOK on success)
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
 * Image Operations
 * Copy the contents of another miniwindow as an image.
 *
 * @param name Destination miniwindow name
 * @param imageId Image identifier to store under
 * @param srcWindowName Source miniwindow name
 * @return Error code (eOK on success)
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
 * Image Operations
 * Get information about a loaded image.
 *
 * @param name Miniwindow name
 * @param imageId Image identifier
 * @param infoType Info type (1=width, 2=height)
 * @return Requested info, or nil on error
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
 * Image Operations
 * Get list of all loaded image IDs in a miniwindow.
 *
 * @param name Miniwindow name
 * @return Table array of image IDs
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
 * Image Operations
 * Save miniwindow to BMP or PNG file
 *
 * @param name Miniwindow name
 * @param filename Output file path (must end in .bmp or .png)
 * @return Error code (eOK on success)
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
 * Image Operations
 * Extract alpha channel from image as grayscale
 *
 * @param name Miniwindow name
 * @param imageId Image identifier (must be 32-bit with alpha)
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param srcLeft Source left coordinate
 * @param srcTop Source top coordinate
 * @return Error code (eOK on success)
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
 * Image Operations
 * Draw image with alpha channel blending
 *
 * @param name Miniwindow name
 * @param imageId Image identifier (must be 32-bit with alpha)
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param opacity Additional opacity multiplier (0.0=transparent, 1.0=opaque)
 * @param srcLeft Source left coordinate
 * @param srcTop Source top coordinate
 * @return Error code (eOK on success)
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
 * srcLeft, srcTop, srcRight, srcBottom)
 *
 * Image Operations
 * Merge image with separate alpha mask
 *
 * @param name Miniwindow name
 * @param imageId Main image identifier
 * @param maskId Mask image identifier
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param mode Blend mode (0=normal, 1=color key transparency)
 * @param opacity Additional opacity multiplier (0.0-1.0)
 * @param srcLeft Source left coordinate
 * @param srcTop Source top coordinate
 * @param srcRight Source right coordinate
 * @param srcBottom Source bottom coordinate
 * @return Error code (eOK on success)
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
 * Image Operations
 * Apply affine transformation to image (rotate, scale, skew)
 *
 * @param name Miniwindow name
 * @param imageId Image identifier
 * @param left X translation offset
 * @param top Y translation offset
 * @param mode Draw mode (1=opaque, 3=transparent with color key)
 * @param mxx Transformation matrix [0][0] (cos θ for rotation)
 * @param mxy Transformation matrix [1][0] (sin θ for rotation)
 * @param myx Transformation matrix [0][1] (-sin θ for rotation)
 * @param myy Transformation matrix [1][1] (cos θ for rotation)
 * @return Error code (eOK on success)
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
 * Image Operations
 * Apply pixel filter to rectangular region
 *
 * @param name Miniwindow name
 * @param left Left coordinate
 * @param top Top coordinate
 * @param right Right coordinate (0 = window width)
 * @param bottom Bottom coordinate (0 = window height)
 * @param operation Filter type (1-27)
 * @param options Filter-specific parameter
 * @return Error code (eOK on success)
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
 *                         mouseOver, cancelMouseOver, mouseDown, cancelMouseDown,
 *                         mouseUp, tooltipText, cursor, flags)
 *
 * Hotspot Management
 * Creates an interactive hotspot (clickable area) in a miniwindow.
 *
 * Based on: CMUSHclientDoc::WindowAddHotspot from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Unique identifier for this hotspot
 * @param left, top, right, bottom Hotspot rectangle (miniwindow-relative coordinates)
 * @param mouseOver Function to call when mouse enters hotspot
 * @param cancelMouseOver Function to call when mouse leaves hotspot
 * @param mouseDown Function to call when mouse button pressed in hotspot
 * @param cancelMouseDown Function to call when mouse released outside hotspot
 * @param mouseUp Function to call when mouse button released in hotspot
 * @param tooltipText Tooltip text to display
 * @param cursor Cursor type (0=arrow, 1=hand, etc.)
 * @param flags Hotspot flags
 * @return eOK (success), eNoSuchWindow if window doesn't exist
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
 * Hotspot Management
 * Deletes a specific hotspot from a miniwindow.
 *
 * Based on: CMUSHclientDoc::WindowDeleteHotspot from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Hotspot ID to delete
 * @return eOK (success), eNoSuchWindow, eNoSuchHotspot
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
 * Hotspot Management
 * Deletes all hotspots from a miniwindow.
 *
 * @param name Miniwindow name
 * @return eOK (success), eNoSuchWindow
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
 * Hotspot Management
 * Sets the tooltip text for a hotspot.
 *
 * Based on: CMUSHclientDoc::WindowHotspotTooltip from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Hotspot ID
 * @param tooltipText New tooltip text
 * @return eOK (success), eNoSuchWindow, eNoSuchHotspot
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
 * Hotspot Management
 * Sets up drag-and-drop handling for a miniwindow hotspot.
 *
 * Based on: CMUSHclientDoc::WindowDragHandler from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Hotspot ID to configure
 * @param moveCallback Function to call during drag (on mouse move)
 * @param releaseCallback Function to call when drag ends (on mouse release)
 * @param flags Drag-and-drop flags
 * @return eOK (success), eNoSuchWindow if window doesn't exist, eHotspotNotInstalled if hotspot
 * doesn't exist
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
 * Miniwindow Right-Click Menus
 * Shows a popup context menu and returns the selected item.
 *
 * Based on: CMUSHclientDoc::WindowMenu from methods_miniwindows.cpp
 *
 * @param name Miniwindow name (or "" for world)
 * @param x X coordinate (screen coordinates)
 * @param y Y coordinate (screen coordinates)
 * @param menuString Pipe-separated menu items with prefix flags
 * @return Selected menu item text (empty string if canceled)
 *
 * Menu format (pipe-separated items with prefix flags):
 *   ">"  - Start a submenu (text after > is submenu title)
 *   "<"  - Go back up one submenu level
 *   "+" or "!" - Checked item (checkmark shown)
 *   "^"  - Disabled/grayed item
 *   "-"  - Separator line
 *   "~"  - Break to new column (not implemented)
 *
 * Examples:
 *   "Item1|Item2|Item3"              - Simple menu
 *   "Item1|-|Item2"                  - Menu with separator
 *   "+Checked|Unchecked"             - Menu with checkmark on first item
 *   ">Submenu|SubItem1|SubItem2|<|MainItem"  - Menu with submenu
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
 * Hotspot Management
 * Moves/resizes an existing hotspot to a new location.
 *
 * Based on: CMUSHclientDoc::WindowMoveHotspot from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Hotspot ID to move
 * @param left, top, right, bottom New rectangle (miniwindow-relative coordinates)
 * @return eOK (success), eNoSuchWindow if window doesn't exist, eHotspotNotInstalled if hotspot
 * doesn't exist
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
 * Hotspot Management
 * Gets information about a miniwindow hotspot.
 *
 * Based on: CMUSHclientDoc::WindowHotspotInfo from methods_miniwindows.cpp
 *
 * @param name Miniwindow name
 * @param hotspotId Hotspot ID
 * @param infoType Type of information to retrieve (1-15)
 *   1 = left, 2 = top, 3 = right, 4 = bottom
 *   5 = MouseOver callback, 6 = CancelMouseOver callback
 *   7 = MouseDown callback, 8 = CancelMouseDown callback
 *   9 = MouseUp callback, 10 = TooltipText
 *   11 = cursor code, 12 = flags
 *   13 = MoveCallback (drag), 14 = ReleaseCallback (drag), 15 = drag flags
 * @return Varies by infoType, or nil if hotspot doesn't exist
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
 * world.WindowScrollwheelHandler(name, hotspotId, moveCallback)
 *
 * Hotspot Management (STUB IMPLEMENTATION)
 * Sets up mouse wheel handling for a miniwindow hotspot.
 *
 * @return eOK (success)
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
 * Returns a table of all miniwindow names.
 *
 * @return Table of window names (strings)
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
 * @param name Window name
 * @return Table of hotspot IDs (strings), or nil if window not found
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
 * Each row is 8 bits where bit 7 is leftmost pixel.
 *
 * @param name Window name
 * @param imageId Image identifier
 * @param row1-row8 Bit patterns for each row
 * @return Error code
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
 * brushColor, imageId, ellipseWidth, ellipseHeight)
 *
 * Draws shapes using an image as a brush pattern.
 *
 * @param name Window name
 * @param action Operation (1=frame rect, 2=fill rect, 3=round rect, 4=ellipse frame, 5=filled
 * ellipse)
 * @param left,top,right,bottom Coordinates
 * @param penColor Outline color
 * @param penStyle Pen style
 * @param penWidth Pen width
 * @param brushColor Background color
 * @param imageId Image for brush pattern
 * @param ellipseWidth,ellipseHeight Corner size for rounded rect
 * @return Error code
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
 * Loads an image from a string containing raw image data (PNG, BMP, JPG, etc.)
 *
 * @param name Window name
 * @param imageId Image identifier
 * @param data String containing raw image bytes
 * @param alpha (optional) Whether to preserve alpha channel
 * @return Error code
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
