// output_view_miniwindows.cpp
// MiniWindow Rendering and Interaction for OutputView
//
// Implements MiniWindow display, mouse interaction, and positioning within
// the output view. MiniWindows are scripted overlays for status bars,
// gauges, maps, and other custom UI elements.
//
// Extracted from output_view.cpp for better code organization.

#include "../../automation/plugin.h"
#include "../../world/miniwindow.h"
#include "../../world/script_engine.h"
#include "../../world/world_document.h"
#include "hotspot.h"
#include "logging.h"
#include "output_view.h"
#include <QGuiApplication>
#include <QPainter>
#include <algorithm>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static Hotspot* findHotspotAtPosition(MiniWindow* mw, const QPoint& mwPos);
static qint32 buildHotspotFlags(Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
static void invokeHotspotCallback(WorldDocument* doc, MiniWindow* mw, const QString& hotspotId,
                                  const QString& callbackName, qint32 flags);

// ============================================================================
// MINIWINDOW RENDERING
// ============================================================================

void OutputView::drawMiniWindows(QPainter& painter, bool underneath)
{
    if (!m_doc)
        return;

    // Calculate miniwindow positions based on text rectangle
    calculateMiniWindowRectangles(underneath);

    // Get list of miniwindows sorted by z-order, then name
    QList<MiniWindow*> windows;
    for (auto it = m_doc->m_MiniWindowMap.cbegin(); it != m_doc->m_MiniWindowMap.cend(); ++it) {
        if (it->second) {
            windows.append(it->second.get());
        }
    }

    // Sort by z-order (lower first), then by name (alphabetical)
    std::sort(windows.begin(), windows.end(), [](MiniWindow* a, MiniWindow* b) {
        if (a->getZOrder() == b->getZOrder())
            return a->getName() < b->getName(); // Alphabetical
        return a->getZOrder() < b->getZOrder();
    });

    // Draw miniwindows in the specified layer
    for (MiniWindow* iwin : windows) {
        // Cast to concrete MiniWindow for rendering (UI layer knows the concrete type)
        MiniWindow* win = iwin;
        if (!win)
            continue;

        // Check layer
        bool isUnderneath = (win->flags & MINIWINDOW_DRAW_UNDERNEATH);
        if (isUnderneath != underneath) {
            continue;
        }

        // Check visible
        if (!win->show || win->temporarilyHide) {
            continue;
        }

        // Check image exists
        if (!win->getImage()) {
            continue;
        }

        // Use calculated position from calculateMiniWindowRectangles()
        QPoint pos = win->rect.topLeft();

        // Draw the miniwindow
        if (win->flags & MINIWINDOW_TRANSPARENT) {
            // Transparent blit using background color as transparency key
            // Convert QImage to QPixmap for display and create transparency mask
            QPixmap masked = win->toPixmap();
            QBitmap mask = masked.createMaskFromColor(QColor(win->backgroundColor));
            masked.setMask(mask);
            painter.drawPixmap(pos, masked);
        } else {
            // Normal blit - convert QImage to QPixmap for display
            painter.drawPixmap(pos, win->toPixmap());
        }

        // Mark as drawn (clear dirty flag)
        win->dirty = false;
    }
}

/**
 * Miniwindow Mouse Event Handling
 */

/**
 * mouseOverMiniwindow - Find miniwindow under mouse cursor
 *
 * Searches for the topmost miniwindow at the given position.
 * Iterates in reverse z-order (topmost first) and returns the first
 * visible, non-underneath, non-ignore-mouse miniwindow that contains the point.
 *
 * Based on CMUSHView::Mouse_Over_Miniwindow()
 */

MiniWindow* OutputView::mouseOverMiniwindow(const QPoint& pos)
{
    if (!m_doc)
        return nullptr;

    // Get list of miniwindows sorted by z-order
    QList<MiniWindow*> windows;
    for (auto it = m_doc->m_MiniWindowMap.cbegin(); it != m_doc->m_MiniWindowMap.cend(); ++it) {
        if (it->second) {
            windows.append(it->second.get());
        }
    }

    // Sort by z-order (lower first), then by name (alphabetical)
    std::sort(windows.begin(), windows.end(), [](MiniWindow* a, MiniWindow* b) {
        if (a->getZOrder() == b->getZOrder())
            return a->getName() < b->getName();
        return a->getZOrder() < b->getZOrder();
    });

    // Iterate in REVERSE z-order (topmost first)
    for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
        // Cast to concrete MiniWindow (UI layer knows the concrete type)
        MiniWindow* mw = dynamic_cast<MiniWindow*>(*it);
        if (!mw)
            continue;

        // Skip if not shown
        if (!mw->show || mw->temporarilyHide)
            continue;

        // Skip underneath miniwindows and those that ignore mouse
        if (mw->flags & (MINIWINDOW_DRAW_UNDERNEATH | MINIWINDOW_IGNORE_MOUSE))
            continue;

        // Check if point is within miniwindow rect
        if (mw->rect.contains(pos)) {
            return mw; // Found it!
        }
    }

    return nullptr; // No miniwindow at this position
}

/**
 * findHotspotAtPosition - Find hotspot at position within miniwindow
 *
 * Hotspot Detection
 *
 * Searches through miniwindow's hotspots to find one containing the given position.
 * Position must be in miniwindow-relative coordinates.
 *
 * @param mw Miniwindow to search
 * @param mwPos Position relative to miniwindow top-left
 * @return Hotspot at position, or nullptr if none
 */

static Hotspot* findHotspotAtPosition(MiniWindow* mw, const QPoint& mwPos)
{
    if (!mw)
        return nullptr;

    // Iterate through hotspots in FORWARD alphabetical order
    // This ensures resize hotspots (which typically start with earlier letters)
    // are checked BEFORE the movewindow hotspot (which starts with "zz_")
    // QMap stores keys in sorted order, so keys() gives us alphabetical order
    for (auto it = mw->hotspots.cbegin(); it != mw->hotspots.cend(); ++it) {
        if (it->second && it->second->m_rect.contains(mwPos)) {
            return it->second.get();
        }
    }

    return nullptr;
}

/**
 * buildHotspotFlags - Convert Qt mouse button and keyboard modifiers to hotspot flags
 *
 * Hotspot Callbacks
 *
 * Hotspot callbacks receive a flags parameter that combines mouse button
 * and keyboard modifiers into a single integer bitmask.
 *
 * Based on miniwin.lua constants
 *
 * @param button Qt::MouseButton that was pressed/released
 * @param modifiers Qt::KeyboardModifiers from event
 * @return Flags bitmask (mouse button + keyboard modifiers)
 */

static qint32 buildHotspotFlags(Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    qint32 flags = 0;

    // Mouse button flags
    // miniwin.mouse_left = 0x01, miniwin.mouse_right = 0x02, miniwin.mouse_middle = 0x04
    if (button == Qt::LeftButton)
        flags |= 0x01;
    else if (button == Qt::RightButton)
        flags |= 0x02;
    else if (button == Qt::MiddleButton)
        flags |= 0x04;

    // Keyboard modifier flags
    // miniwin.hotspot_got_shift = 0x10, miniwin.hotspot_got_ctrl = 0x20, miniwin.hotspot_got_alt =
    // 0x40
    if (modifiers & Qt::ShiftModifier)
        flags |= 0x10;
    if (modifiers & Qt::ControlModifier)
        flags |= 0x20;
    if (modifiers & Qt::AltModifier)
        flags |= 0x40;

    return flags;
}

/**
 * invokeHotspotCallback - Invoke Lua callback for hotspot event
 *
 * Hotspot Callbacks
 *
 * Calls the Lua function specified in the hotspot callback string.
 * The callback receives ONLY flags and hotspot ID.
 * Scripts query mouse position via WindowInfo(windowName, 17/18).
 *
 * Based on Send_Mouse_Event_To_Plugin()
 *
 * @param doc WorldDocument
 * @param mw Miniwindow containing hotspot
 * @param hotspotId Hotspot identifier
 * @param callbackName Lua function name to call
 * @param flags Mouse button and keyboard modifier flags (from Qt event)
 */

static void invokeHotspotCallback(WorldDocument* doc, MiniWindow* mw, const QString& hotspotId,
                                  const QString& callbackName, qint32 flags)
{
    if (!doc || !mw || callbackName.isEmpty())
        return;

    // Get the plugin that should handle this callback
    QString pluginId = mw->callbackPlugin.isEmpty() ? mw->creatingPlugin : mw->callbackPlugin;

    if (pluginId.isEmpty()) {
        // No plugin specified - try calling in world's Lua state
        if (!doc->m_ScriptEngine || !doc->m_ScriptEngine->L) {
            qCDebug(lcUI) << "ERROR: No world script engine!";
            return;
        }

        lua_State* L = doc->m_ScriptEngine->L;

        // Handle table.field notation (e.g., "mw_info.mousedown")
        QString callbackStr = callbackName;
        int lastDot = callbackStr.lastIndexOf('.');

        if (lastDot != -1) {
            // Callback is in a table
            QString tablePath = callbackStr.left(lastDot);
            QString fieldName = callbackStr.mid(lastDot + 1);

            QStringList tableNames = tablePath.split('.');
            lua_getglobal(L, tableNames[0].toUtf8().constData());

            for (int i = 1; i < tableNames.size(); i++) {
                if (!lua_istable(L, -1)) {
                    qCDebug(lcUI) << "ERROR:" << tableNames[i - 1] << "is not a table in callback"
                                  << callbackName;
                    lua_pop(L, 1);
                    return;
                }
                lua_getfield(L, -1, tableNames[i].toUtf8().constData());
                lua_remove(L, -2);
            }

            if (!lua_istable(L, -1)) {
                qCDebug(lcUI) << "ERROR: Final table" << tablePath << "is not a table in callback"
                              << callbackName;
                lua_pop(L, 1);
                return;
            }

            lua_getfield(L, -1, fieldName.toUtf8().constData());
            lua_remove(L, -2);
        } else {
            // Simple global function
            lua_getglobal(L, callbackName.toUtf8().constData());
        }

        if (!lua_isfunction(L, -1)) {
            qCDebug(lcUI) << "ERROR: Function" << callbackName << "not found in world Lua state";
            lua_pop(L, 1); // Pop non-function
            return;
        }

        // Push arguments: flags, hotspot_id
        // Scripts query mouse position via WindowInfo(windowName, 17/18)
        lua_pushinteger(L, flags);
        lua_pushstring(L, hotspotId.toUtf8().constData());

        // Call function with 2 arguments, 0 results
        if (lua_pcall(L, 2, 0, 0) != 0) {
            qCDebug(lcUI) << "Error calling hotspot callback" << callbackName << ":"
                          << lua_tostring(L, -1);
            lua_pop(L, 1); // Pop error message
        }
    } else {
        // Plugin-specific callback - find plugin by ID
        Plugin* plugin = nullptr;
        for (const auto& p : doc->m_PluginList) {
            if (p && p->m_strID == pluginId) {
                plugin = p.get();
                break;
            }
        }

        if (!plugin) {
            qCDebug(lcUI) << "ERROR: Plugin" << pluginId << "not found for callback"
                          << callbackName;
            return;
        }
        if (!plugin->m_bEnabled) {
            qCDebug(lcUI) << "ERROR: Plugin" << pluginId << "is disabled!";
            return;
        }
        if (!plugin->m_ScriptEngine || !plugin->m_ScriptEngine->L) {
            qCDebug(lcUI) << "ERROR: Plugin" << pluginId << "has no Lua state!";
            return;
        }

        lua_State* L = plugin->m_ScriptEngine->L;

        // Handle table.field notation (e.g., "mw_info.mousedown")
        // Split on the last dot to separate table path from field name
        QString callbackStr = callbackName;
        int lastDot = callbackStr.lastIndexOf('.');

        if (lastDot != -1) {
            // Callback is in a table (e.g., "table.field" or "table.subtable.field")
            QString tablePath = callbackStr.left(lastDot);
            QString fieldName = callbackStr.mid(lastDot + 1);

            // Navigate through nested tables (e.g., "table.subtable")
            QStringList tableNames = tablePath.split('.');
            lua_getglobal(L, tableNames[0].toUtf8().constData());

            for (int i = 1; i < tableNames.size(); i++) {
                if (!lua_istable(L, -1)) {
                    qCDebug(lcUI) << "ERROR:" << tableNames[i - 1] << "is not a table in callback"
                                  << callbackName;
                    lua_pop(L, 1);
                    return;
                }
                lua_getfield(L, -1, tableNames[i].toUtf8().constData());
                lua_remove(L, -2); // Remove previous table, keep current
            }

            // Now get the field from the final table
            if (!lua_istable(L, -1)) {
                int luaType = lua_type(L, -1);
                qCDebug(lcUI) << "ERROR: Final table" << tablePath << "is not a table in callback"
                              << callbackName << "(got type:" << lua_typename(L, luaType) << ")";
                lua_pop(L, 1);
                return;
            }

            lua_getfield(L, -1, fieldName.toUtf8().constData());
            lua_remove(L, -2); // Remove table, keep function
        } else {
            // Simple global function
            lua_getglobal(L, callbackName.toUtf8().constData());
        }

        if (!lua_isfunction(L, -1)) {
            int luaType = lua_type(L, -1);
            qCDebug(lcUI) << "ERROR: Function" << callbackName << "not found in plugin Lua state"
                          << "(got type:" << lua_typename(L, luaType) << ")";
            lua_pop(L, 1); // Pop non-function
            return;
        }

        // Push arguments: flags, hotspot_id
        // Scripts query mouse position via WindowInfo(windowName, 17/18)
        lua_pushinteger(L, flags);
        lua_pushstring(L, hotspotId.toUtf8().constData());

        // Call function with 2 arguments, 0 results
        if (lua_pcall(L, 2, 0, 0) != 0) {
            qCDebug(lcUI) << "Error calling hotspot callback" << callbackName << ":"
                          << lua_tostring(L, -1);
            lua_pop(L, 1); // Pop error message
        }
    }
}

/**
 * mouseDownMiniWindow - Handle mouse button press in miniwindow
 *
 * Returns true if a miniwindow handled the event, false otherwise.
 *
 * Based on CMUSHView::Mouse_Down_MiniWindow()
 */

bool OutputView::mouseDownMiniWindow(const QPoint& pos, Qt::MouseButton button)
{
    MiniWindow* mw = mouseOverMiniwindow(pos);

    if (!mw)
        return false; // No miniwindow at this position

    // Track which miniwindow mouse was pressed in
    m_mouseDownMiniwindow = mw->name;
    m_mouseDownButton = button; // Remember which button for drag operations

    // Update mouse position for WindowInfo queries
    QPoint mwPos = pos - mw->rect.topLeft();
    mw->lastMousePosition = mwPos; // WindowInfo 14/15: miniwindow-relative
    mw->clientMousePosition = pos; // WindowInfo 17/18: output-window-relative

    // Find hotspot under mouse
    Hotspot* hotspot = findHotspotAtPosition(mw, mwPos);

    if (hotspot) {
        // Find hotspot ID
        QString hotspotId;
        for (auto it = mw->hotspots.cbegin(); it != mw->hotspots.cend(); ++it) {
            if (it->second.get() == hotspot) {
                hotspotId = it->first;
                break;
            }
        }

        // Track which hotspot was pressed
        mw->mouseDownHotspot = hotspotId;

        // Invoke MouseDown callback (scripts query mouse pos via WindowInfo)
        if (!hotspot->m_sMouseDown.isEmpty()) {
            qint32 flags = buildHotspotFlags(button, QGuiApplication::keyboardModifiers());
            invokeHotspotCallback(m_doc, mw, hotspotId, hotspot->m_sMouseDown, flags);
        }
    } else {
        mw->mouseDownHotspot.clear();
    }

    // Capture mouse for drag operations (like Windows SetCapture)
    grabMouse();

    // Remember this miniwindow for drag tracking
    m_previousMiniwindow = mw->name;

    return true; // Miniwindow handled the event
}

/**
 * mouseMoveMiniWindow - Handle mouse move over miniwindow
 *
 * Returns true if a miniwindow handled the event, false otherwise.
 *
 * Based on CMUSHView::Mouse_Move_MiniWindow()
 */

bool OutputView::mouseMoveMiniWindow(const QPoint& pos)
{
    // === CRITICAL: Drag-and-Drop Logic (must come first) ===
    // Check if we have a previous miniwindow with active drag
    if (!m_previousMiniwindow.isEmpty()) {
        auto it = m_doc->m_MiniWindowMap.find(m_previousMiniwindow);
        MiniWindow* prevMw =
            (it != m_doc->m_MiniWindowMap.end() && it->second) ? it->second.get() : nullptr;

        if (prevMw) {
            // CRITICAL: Update mouse position BEFORE callback so WindowInfo() returns correct
            // coords
            QPoint prevMwPos = pos - prevMw->rect.topLeft();
            prevMw->lastMousePosition = prevMwPos; // WindowInfo 14/15: miniwindow-relative
            prevMw->clientMousePosition = pos;     // WindowInfo 17/18: output-window-relative

            // Check if we're actively dragging (hotspot was pressed)
            if (!prevMw->mouseDownHotspot.isEmpty()) {
                // Find the hotspot that was originally clicked
                auto it = prevMw->hotspots.find(prevMw->mouseDownHotspot);
                Hotspot* dragHotspot =
                    (it != prevMw->hotspots.end() && it->second) ? it->second.get() : nullptr;

                // Call MoveCallback during drag (THIS IS HOW RESIZING WORKS!)
                // Callback queries mouse position via WindowInfo(windowName, 17/18)
                if (dragHotspot && !dragHotspot->m_sMoveCallback.isEmpty()) {
                    qint32 flags =
                        buildHotspotFlags(m_mouseDownButton, QGuiApplication::keyboardModifiers());
                    invokeHotspotCallback(m_doc, prevMw, prevMw->mouseDownHotspot,
                                          dragHotspot->m_sMoveCallback, flags);
                }

                return true; // Process only drag, exit early
            }
        }
    }
    // === END Drag-and-Drop Logic ===

    // Normal mouse-over processing (only when NOT dragging)
    MiniWindow* mw = mouseOverMiniwindow(pos);

    // If we have a mouse-down miniwindow, don't start text selection
    // even if mouse has moved outside that miniwindow
    if (!m_mouseDownMiniwindow.isEmpty())
        return true;

    if (mw) {
        // Track which miniwindow mouse is over
        m_mouseOverMiniwindow = mw->name;

        // Update mouse position for WindowInfo queries
        QPoint mwPos = pos - mw->rect.topLeft();
        mw->lastMousePosition = mwPos; // WindowInfo 14/15: miniwindow-relative
        mw->clientMousePosition = pos; // WindowInfo 17/18: output-window-relative

        // Find hotspot under mouse
        Hotspot* hotspot = findHotspotAtPosition(mw, mwPos);

        QString newHotspotId;
        if (hotspot) {
            // Find hotspot ID
            for (auto it = mw->hotspots.cbegin(); it != mw->hotspots.cend(); ++it) {
                if (it->second.get() == hotspot) {
                    newHotspotId = it->first;
                    break;
                }
            }
        }

        // Check if we've entered/left a hotspot
        if (newHotspotId != mw->mouseOverHotspot) {
            // Left previous hotspot
            if (!mw->mouseOverHotspot.isEmpty()) {
                auto it = mw->hotspots.find(mw->mouseOverHotspot);
                Hotspot* oldHotspot =
                    (it != mw->hotspots.end() && it->second) ? it->second.get() : nullptr;
                if (oldHotspot && !oldHotspot->m_sCancelMouseOver.isEmpty()) {
                    qint32 flags =
                        buildHotspotFlags(Qt::NoButton, QGuiApplication::keyboardModifiers());
                    invokeHotspotCallback(m_doc, mw, mw->mouseOverHotspot,
                                          oldHotspot->m_sCancelMouseOver, flags);
                }
            }

            // Entered new hotspot
            if (!newHotspotId.isEmpty()) {
                auto it = mw->hotspots.find(newHotspotId);
                Hotspot* newHotspot =
                    (it != mw->hotspots.end() && it->second) ? it->second.get() : nullptr;
                if (newHotspot && !newHotspot->m_sMouseOver.isEmpty()) {
                    qint32 flags =
                        buildHotspotFlags(Qt::NoButton, QGuiApplication::keyboardModifiers());
                    invokeHotspotCallback(m_doc, mw, newHotspotId, newHotspot->m_sMouseOver, flags);
                }
            }

            // Update tracking
            mw->mouseOverHotspot = newHotspotId;
        }

        // Set cursor based on hotspot
        if (hotspot && hotspot->m_Cursor != 0) {
            // Map MUSHclient cursor IDs to Qt cursors
            // Based on methods_output.cpp
            Qt::CursorShape qtCursor;
            switch (hotspot->m_Cursor) {
                case -1:
                    qtCursor = Qt::BlankCursor;
                    break;
                case 0:
                    qtCursor = Qt::ArrowCursor;
                    break;
                case 1:
                    qtCursor = Qt::PointingHandCursor;
                    break;
                case 2:
                    qtCursor = Qt::IBeamCursor;
                    break;
                case 3:
                    qtCursor = Qt::CrossCursor;
                    break;
                case 4:
                    qtCursor = Qt::WaitCursor;
                    break;
                case 5:
                    qtCursor = Qt::UpArrowCursor;
                    break;
                case 6:
                    qtCursor = Qt::SizeFDiagCursor;
                    break;
                case 7:
                    qtCursor = Qt::SizeBDiagCursor;
                    break;
                case 8:
                    qtCursor = Qt::SizeHorCursor;
                    break;
                case 9:
                    qtCursor = Qt::SizeVerCursor;
                    break;
                case 10:
                    qtCursor = Qt::SizeAllCursor;
                    break;
                case 11:
                    qtCursor = Qt::ForbiddenCursor;
                    break;
                case 12:
                    qtCursor = Qt::WhatsThisCursor;
                    break;
                default:
                    qtCursor = Qt::PointingHandCursor;
                    break;
            }
            setCursor(qtCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }

        return true; // Miniwindow handled the event
    } else {
        // Not over any miniwindow
        m_mouseOverMiniwindow.clear();

        // Restore I-beam cursor for text selection
        setCursor(Qt::IBeamCursor);

        return false;
    }
}

/**
 * mouseUpMiniWindow - Handle mouse button release in miniwindow
 *
 * Returns true if a miniwindow handled the event, false otherwise.
 *
 * Based on CMUSHView::Mouse_Up_MiniWindow()
 */

bool OutputView::mouseUpMiniWindow(const QPoint& pos, Qt::MouseButton button)
{
    Q_UNUSED(button);

    // Check if we had a mouse-down in a miniwindow
    if (m_mouseDownMiniwindow.isEmpty())
        return false;

    // Find the miniwindow where mouse was pressed
    auto it = m_doc->m_MiniWindowMap.find(m_mouseDownMiniwindow);
    if (it == m_doc->m_MiniWindowMap.end() || !it->second) {
        m_mouseDownMiniwindow.clear();
        releaseMouse(); // Release capture
        m_previousMiniwindow.clear();
        return false;
    }

    MiniWindow* mw = it->second.get();

    // Update mouse position for WindowInfo queries
    QPoint mwPos = pos - mw->rect.topLeft();
    mw->lastMousePosition = mwPos; // WindowInfo 14/15: miniwindow-relative
    mw->clientMousePosition = pos; // WindowInfo 17/18: output-window-relative

    // Check if we had an active drag operation
    if (!mw->mouseDownHotspot.isEmpty()) {
        auto it = mw->hotspots.find(mw->mouseDownHotspot);
        Hotspot* dragHotspot =
            (it != mw->hotspots.end() && it->second) ? it->second.get() : nullptr;

        // Call ReleaseCallback to end drag operation (THIS IS HOW RESIZE COMPLETES!)
        if (dragHotspot && !dragHotspot->m_sReleaseCallback.isEmpty()) {
            qint32 flags = buildHotspotFlags(button, QGuiApplication::keyboardModifiers());
            invokeHotspotCallback(m_doc, mw, mw->mouseDownHotspot, dragHotspot->m_sReleaseCallback,
                                  flags);
        }
    }

    // Find hotspot under mouse for regular click handling
    Hotspot* hotspot = findHotspotAtPosition(mw, mwPos);

    QString currentHotspotId;
    if (hotspot) {
        // Find hotspot ID
        for (auto it = mw->hotspots.cbegin(); it != mw->hotspots.cend(); ++it) {
            if (it->second.get() == hotspot) {
                currentHotspotId = it->first;
                break;
            }
        }
    }

    // Check if we released over the same hotspot where we pressed
    if (!mw->mouseDownHotspot.isEmpty()) {
        auto it = mw->hotspots.find(mw->mouseDownHotspot);
        Hotspot* downHotspot =
            (it != mw->hotspots.end() && it->second) ? it->second.get() : nullptr;

        if (currentHotspotId == mw->mouseDownHotspot) {
            // MouseUp - released over the same hotspot where we pressed
            if (downHotspot && !downHotspot->m_sMouseUp.isEmpty()) {
                qint32 flags = buildHotspotFlags(button, QGuiApplication::keyboardModifiers());
                invokeHotspotCallback(m_doc, mw, mw->mouseDownHotspot, downHotspot->m_sMouseUp,
                                      flags);
            }
        } else {
            // CancelMouseDown - released outside the hotspot where we pressed
            if (downHotspot && !downHotspot->m_sCancelMouseDown.isEmpty()) {
                qint32 flags = buildHotspotFlags(button, QGuiApplication::keyboardModifiers());
                invokeHotspotCallback(m_doc, mw, mw->mouseDownHotspot,
                                      downHotspot->m_sCancelMouseDown, flags);
            }
        }

        // Clear mouse-down hotspot tracking
        mw->mouseDownHotspot.clear();
    }

    // Release mouse capture (like Windows ReleaseCapture)
    releaseMouse();

    // Clear mouse-down tracking
    m_mouseDownMiniwindow.clear();
    m_previousMiniwindow.clear();

    return true; // Miniwindow handled the event
}

// ============================================================================
// MINIWINDOW POSITIONING
// ============================================================================

/**
 * calculateMiniWindowRectangles - Position miniwindows based on text rectangle
 *
 * Miniwindow Layout Algorithm
 *
 * This implements the complex miniwindow positioning system.
 * Miniwindows can be positioned at corners, centered on edges, or absolutely.
 *
 * Based on CMUSHView::PositionWindows()
 */
void OutputView::calculateMiniWindowRectangles(bool underneath)
{
    if (!m_doc)
        return;

    QRect clientRect = rect();

    // Record how far out each corner extends
    QPoint ptTopLeft(0, 0);
    QPoint ptTopRight(clientRect.right(), 0);
    QPoint ptBottomLeft(0, clientRect.bottom());
    QPoint ptBottomRight(clientRect.right(), clientRect.bottom());

    // Total amount taken by the centered items along each wall
    qint32 topWidths = 0;
    qint32 rightWidths = 0;
    qint32 bottomWidths = 0;
    qint32 leftWidths = 0;

    // Remember each centered miniwindow
    QList<MiniWindow*> topOnes, rightOnes, bottomOnes, leftOnes;

    // Phase 1: Position corners and absolute miniwindows, accumulate centered widths
    for (auto it = m_doc->m_MiniWindowMap.cbegin(); it != m_doc->m_MiniWindowMap.cend(); ++it) {
        if (!it->second)
            continue;
        // Get raw pointer for layout calculations
        MiniWindow* mw = it->second.get();
        if (!mw)
            continue;

        mw->temporarilyHide = false;

        // Skip if not to be shown
        if (!mw->show)
            continue;

        // Skip if wrong layer
        bool isUnderneath = (mw->flags & MINIWINDOW_DRAW_UNDERNEATH);
        if (isUnderneath != underneath)
            continue;

        qint32 iWidth = mw->width;
        qint32 iHeight = mw->height;

        // If absolute location wanted, just take that and continue
        if (mw->flags & MINIWINDOW_ABSOLUTE_LOCATION) {
            QPoint loc = mw->location;
            mw->rect = QRect(loc.x(), loc.y(), iWidth, iHeight);
            continue;
        }

        // Handle positioning modes
        switch (mw->position) {
            case 0: // Stretch to output window
                mw->rect = QRect(0, 0, clientRect.right(), clientRect.bottom());
                continue;

            case 1: // Stretch with aspect ratio
            {
                double ratio = static_cast<double>(iWidth) / static_cast<double>(iHeight);
                mw->rect = QRect(0, 0, clientRect.bottom() * ratio, clientRect.bottom());
                continue;
            }

            case 2: // Stretch to frame (TODO: implement when we have frame access)
                mw->rect = QRect(0, 0, clientRect.right(), clientRect.bottom());
                continue;

            case 3: // Stretch to frame with aspect ratio (TODO: implement when we have frame
                    // access)
            {
                double ratio = static_cast<double>(iWidth) / static_cast<double>(iHeight);
                mw->rect = QRect(0, 0, clientRect.bottom() * ratio, clientRect.bottom());
                continue;
            }

            case 4: // Top left
                mw->rect = QRect(0, 0, iWidth, iHeight);
                ptTopLeft.setX(qMax(ptTopLeft.x(), mw->rect.right())); // Shrinks available space
                ptTopLeft.setY(qMax(ptTopLeft.y(), mw->rect.bottom()));
                break;

            case 5: // Center left-right at top
                topWidths += iWidth;
                topOnes.append(mw);
                break;

            case 6: // Top right
                mw->rect = QRect(clientRect.right() - iWidth, 0, iWidth, iHeight);
                ptTopRight.setX(qMin(ptTopRight.x(), mw->rect.left())); // Shrinks available space
                ptTopRight.setY(qMax(ptTopRight.y(), mw->rect.bottom()));
                break;

            case 7: // On right, center top-bottom
                rightWidths += iHeight;
                rightOnes.append(mw);
                break;

            case 8: // On right, at bottom
                mw->rect = QRect(clientRect.right() - iWidth, clientRect.bottom() - iHeight, iWidth,
                                 iHeight);
                ptBottomRight.setX(
                    qMin(ptBottomRight.x(), mw->rect.left())); // Shrinks available space
                ptBottomRight.setY(qMin(ptBottomRight.y(), mw->rect.top()));
                break;

            case 9: // Center left-right at bottom
                bottomWidths += iWidth;
                bottomOnes.append(mw);
                break;

            case 10: // On left, at bottom
                mw->rect = QRect(0, clientRect.bottom() - iHeight, iWidth, iHeight);
                ptBottomLeft.setX(
                    qMax(ptBottomLeft.x(), mw->rect.right())); // Shrinks available space
                ptBottomLeft.setY(qMin(ptBottomLeft.y(), mw->rect.top()));
                break;

            case 11: // On left, center top-bottom
                leftWidths += iHeight;
                leftOnes.append(mw);
                break;

            case 12: // Center all
                mw->rect = QRect((clientRect.right() - iWidth) / 2,
                                 (clientRect.bottom() - iHeight) / 2, iWidth, iHeight);
                continue;

            case 13: // Tile (ignore for now)
                continue;
        }
    }

    // Phase 2: Calculate available space after corners placed
    qint32 topRoom = ptTopRight.x() - ptTopLeft.x();
    qint32 rightRoom = ptBottomRight.y() - ptTopRight.y();
    qint32 bottomRoom = ptBottomRight.x() - ptBottomLeft.x();
    qint32 leftRoom = ptBottomLeft.y() - ptTopLeft.y();

    // Drop miniwindows that won't fit (from back of list)
    // Top
    while (!topOnes.empty() && (topWidths > topRoom)) {
        MiniWindow* mw = topOnes.back();
        topOnes.pop_back();
        topWidths -= mw->width;
        mw->temporarilyHide = true;
    }

    // Right
    while (!rightOnes.empty() && (rightWidths > rightRoom)) {
        MiniWindow* mw = rightOnes.back();
        rightOnes.pop_back();
        rightWidths -= mw->height;
        mw->temporarilyHide = true;
    }

    // Bottom
    while (!bottomOnes.empty() && (bottomWidths > bottomRoom)) {
        MiniWindow* mw = bottomOnes.back();
        bottomOnes.pop_back();
        bottomWidths -= mw->width;
        mw->temporarilyHide = true;
    }

    // Left
    while (!leftOnes.empty() && (leftWidths > leftRoom)) {
        MiniWindow* mw = leftOnes.back();
        leftOnes.pop_back();
        leftWidths -= mw->height;
        mw->temporarilyHide = true;
    }

    // Phase 3: Position centered miniwindows with even spacing
    // Along top wall
    if (!topOnes.empty()) {
        qint32 gap = (topRoom - topWidths) / (topOnes.size() + 1);
        qint32 start = ptTopLeft.x() + gap;
        for (MiniWindow* mw : topOnes) {
            mw->rect = QRect(start, 0, mw->width, mw->height);
            start += mw->width + gap;
        }
    }

    // Along right wall
    if (!rightOnes.empty()) {
        qint32 gap = (rightRoom - rightWidths) / (rightOnes.size() + 1);
        qint32 start = ptTopRight.y() + gap;
        for (MiniWindow* mw : rightOnes) {
            mw->rect = QRect(clientRect.right() - mw->width, start, mw->width, mw->height);
            start += mw->height + gap;
        }
    }

    // Along bottom wall
    if (!bottomOnes.empty()) {
        qint32 gap = (bottomRoom - bottomWidths) / (bottomOnes.size() + 1);
        qint32 start = ptBottomLeft.x() + gap;
        for (MiniWindow* mw : bottomOnes) {
            mw->rect = QRect(start, clientRect.bottom() - mw->height, mw->width, mw->height);
            start += mw->width + gap;
        }
    }

    // Along left wall
    if (!leftOnes.empty()) {
        qint32 gap = (leftRoom - leftWidths) / (leftOnes.size() + 1);
        qint32 start = ptTopLeft.y() + gap;
        for (MiniWindow* mw : leftOnes) {
            mw->rect = QRect(0, start, mw->width, mw->height);
            start += mw->height + gap;
        }
    }
}

// ============================================================================
// MINIWINDOW SCROLL WHEEL HANDLING
// ============================================================================

/**
 * handleMiniWindowScrollWheel - Handle scroll wheel over miniwindow
 *
 * MiniWindow Scroll Events
 *
 * Checks if the mouse position is over a miniwindow with a scroll wheel handler.
 * If so, invokes the handler and returns true. Otherwise returns false to allow
 * normal text scrolling.
 *
 * @param pos Mouse position in output view coordinates
 * @param angleDelta Scroll wheel angle delta from QWheelEvent
 * @param modifiers Keyboard modifiers from QWheelEvent
 * @return true if miniwindow handled scroll, false otherwise
 */
bool OutputView::handleMiniWindowScrollWheel(const QPoint& pos, const QPoint& angleDelta,
                                             Qt::KeyboardModifiers modifiers)
{
    // Check if mouse is over a miniwindow
    MiniWindow* mw = mouseOverMiniwindow(pos);

    if (mw) {
        // Find hotspot under mouse
        QPoint mwPos = pos - mw->rect.topLeft();
        Hotspot* hotspot = findHotspotAtPosition(mw, mwPos);

        if (hotspot && !hotspot->m_sScrollwheelCallback.isEmpty()) {
            // Update mouse position for WindowInfo queries
            mw->lastMousePosition = mwPos;
            mw->clientMousePosition = pos;

            // Build flags (no button, just modifiers + scroll direction)
            qint32 flags = 0;
            if (modifiers & Qt::ShiftModifier)
                flags |= 0x10;
            if (modifiers & Qt::ControlModifier)
                flags |= 0x20;
            if (modifiers & Qt::AltModifier)
                flags |= 0x40;

            // Add scroll direction (positive = up, negative = down)
            int delta = angleDelta.y();

            // Find hotspot ID
            QString hotspotId;
            for (auto it = mw->hotspots.cbegin(); it != mw->hotspots.cend(); ++it) {
                if (it->second.get() == hotspot) {
                    hotspotId = it->first;
                    break;
                }
            }

            // Invoke scroll wheel callback
            invokeHotspotCallback(m_doc, mw, hotspotId, hotspot->m_sScrollwheelCallback, delta);

            return true; // Miniwindow handled the scroll
        }
    }

    return false; // No miniwindow scroll handler, allow text scrolling
}
