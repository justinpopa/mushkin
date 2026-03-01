/**
 * world_miniwindow_lifecycle.cpp - Miniwindow Lifecycle Lua API Functions
 *
 * Window creation, management, positioning, z-order, deletion, info,
 * resize, and listing functions.
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
