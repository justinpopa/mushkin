/**
 * lua_api.h - Lua API Module Registration
 *
 * Miniwindow System - API Organization
 *
 * This header declares registration functions for modular Lua API components.
 * Each module (miniwindows, settings, etc.) has its own .cpp file for better organization.
 */

#ifndef LUA_API_H
#define LUA_API_H

// Lua 5.1 C headers
extern "C" {
#include <lua.h>
}

/**
 * register_miniwindow_functions - Register all miniwindow API functions
 *
 * Registers WindowCreate, WindowShow, WindowPosition, WindowRectOp, WindowText,
 * WindowFont, WindowAddHotspot, WindowMenu, and all other miniwindow functions.
 *
 * @param L Lua state (must have "world" table already created)
 */
void register_miniwindow_functions(lua_State* L);

/**
 * register_setting_functions - Register all world settings and info API functions
 *
 * Registers GetInfo, GetWorldName, SetOption, GetOption, SetStatus, Repaint,
 * TextRectangle, SetBackgroundImage, GetCommand, SetCommandWindowHeight,
 * SetScroll, AddFont, and SetCursor.
 *
 * @param L Lua state (must have "world" table already created)
 */
void register_setting_functions(lua_State* L);

#endif // LUA_API_H
