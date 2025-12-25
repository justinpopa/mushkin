/**
 * lua_registration.h - Forward declarations for Lua API registration
 *
 * This header provides forward declarations for all Lua API registration
 * functions. These functions register Lua API functions in their respective
 * categories (output, network, triggers, etc.) with the Lua state.
 *
 * Extracted from lua_methods.cpp for modular organization.
 */

#ifndef LUA_REGISTRATION_H
#define LUA_REGISTRATION_H

// Lua 5.1 C headers
extern "C" {
struct lua_State;
}

// ========== Forward Declarations ==========

/**
 * RegisterLuaRoutines - Register all MUSHclient API functions
 *
 * Main registration function that creates the "world" table and registers
 * all API functions. Also creates error_code, trigger_flag, alias_flag,
 * sendto, and miniwin constants tables.
 *
 * Called from ScriptEngine::openLua().
 *
 * @param L Lua state
 * @return 0 (for lua_CFunction compatibility)
 */
int RegisterLuaRoutines(lua_State* L);

/**
 * register_output_functions - Register output API functions
 *
 * Registers functions for displaying text in the output window:
 * - Note, ColourNote, ColourTell, Trim
 *
 * @param L Lua state
 */
void register_output_functions(lua_State* L);

/**
 * register_network_functions - Register network API functions
 *
 * Registers functions for MUD connection:
 * - Send, Connect, Disconnect, IsConnected
 *
 * @param L Lua state
 */
void register_network_functions(lua_State* L);

/**
 * register_variable_functions - Register variable API functions
 *
 * Registers functions for world variables:
 * - GetVariable, SetVariable, DeleteVariable, GetVariableList
 *
 * @param L Lua state
 */
void register_variable_functions(lua_State* L);

/**
 * register_world_info_functions - Register world info API functions
 *
 * Registers functions for world information and configuration:
 * - GetInfo, GetWorldName, SetOption, SetStatus, Repaint, TextRectangle
 *
 * @param L Lua state
 */
void register_world_info_functions(lua_State* L);

/**
 * register_color_functions - Register color API functions
 *
 * Registers functions for color queries:
 * - GetNormalColour, GetBoldColour
 *
 * @param L Lua state
 */
void register_color_functions(lua_State* L);

/**
 * register_trigger_functions - Register trigger API functions
 *
 * Registers functions for trigger management:
 * - AddTrigger, DeleteTrigger, EnableTrigger, GetTriggerInfo, GetTriggerList
 *
 * @param L Lua state
 */
void register_trigger_functions(lua_State* L);

/**
 * register_alias_functions - Register alias API functions
 *
 * Registers functions for alias management:
 * - AddAlias, DeleteAlias, EnableAlias, GetAliasInfo, GetAliasList
 *
 * @param L Lua state
 */
void register_alias_functions(lua_State* L);

/**
 * register_timer_functions - Register timer API functions
 *
 * Registers functions for timer management:
 * - AddTimer, DeleteTimer, EnableTimer, IsTimer, GetTimerInfo, GetTimerList
 * - ResetTimer, ResetTimers, DoAfter, DoAfterNote, DoAfterSpeedWalk, DoAfterSpecial
 * - EnableTimerGroup, DeleteTimerGroup, DeleteTemporaryTimers
 * - GetTimerOption, SetTimerOption
 *
 * @param L Lua state
 */
void register_timer_functions(lua_State* L);

/**
 * register_utility_functions - Register utility API functions
 *
 * Registers utility functions:
 * - Hash, Base64Encode, Base64Decode, Trim, GetUniqueNumber, Execute
 * - SetStatus, GetGlobalOption
 *
 * @param L Lua state
 */
void register_utility_functions(lua_State* L);

/**
 * register_logging_functions - Register logging API functions
 *
 * Registers logging functions:
 * - OpenLog, CloseLog, WriteLog, FlushLog, IsLogOpen
 *
 * @param L Lua state
 */
void register_logging_functions(lua_State* L);

/**
 * register_plugin_functions - Register plugin API functions
 *
 * Registers plugin management functions:
 * - GetPluginID, GetPluginName, GetPluginList, IsPluginInstalled, GetPluginInfo
 * - LoadPlugin, ReloadPlugin, UnloadPlugin, EnablePlugin, CallPlugin
 * - PluginSupports, BroadcastPlugin, SendPkt, SaveState
 * - GetPluginVariable, GetPluginVariableList
 * - GetPluginTriggerList, GetPluginAliasList, GetPluginTimerList
 * - GetPluginTriggerInfo, GetPluginAliasInfo, GetPluginTimerInfo
 * - GetPluginTriggerOption, GetPluginAliasOption, GetPluginTimerOption
 *
 * @param L Lua state
 */
void register_plugin_functions(lua_State* L);

/**
 * register_gmcp_functions - Register GMCP API functions
 *
 * Registers GMCP functions:
 * - GetGMCP, SendGMCP, GetGMCPList
 *
 * @param L Lua state
 */
void register_gmcp_functions(lua_State* L);

/**
 * register_miniwindow_functions - Register miniwindow API functions
 *
 * Registers miniwindow functions:
 * - WindowCreate, WindowShow, WindowPosition, WindowSetZOrder, WindowDelete
 * - WindowInfo, WindowResize, WindowRectOp, WindowCircleOp, WindowLine
 * - WindowPolygon, WindowSetPixel, WindowGetPixel, WindowFont, WindowText
 * - WindowTextWidth, WindowFontInfo, WindowFontList, WindowLoadImage
 * - WindowAddHotspot, WindowDragHandler, WindowMenu, WindowHotspotInfo
 * - WindowMoveHotspot, WindowScrollwheelHandler
 *
 * @param L Lua state
 */
void register_miniwindow_functions(lua_State* L);

/**
 * register_font_functions - Register font API functions
 *
 * Registers font management functions:
 * - AddFont, ColourNameToRGB, RGBColourToName
 *
 * @param L Lua state
 */
void register_font_functions(lua_State* L);

/**
 * register_lua_constants - Register Lua constant tables
 *
 * Creates and populates global constant tables:
 * - error_code: Error code constants (eOK, eTriggerNotFound, etc.)
 * - trigger_flag: Trigger flag constants (Enabled, KeepEvaluating, etc.)
 * - alias_flag: Alias flag constants (same as trigger_flag)
 * - sendto: SendTo destination constants (World, Command, Output, etc.)
 * - miniwin: Miniwindow constants (position modes, flags, pen/brush styles, etc.)
 * - extended_colours: XTerm 256-color palette array
 *
 * @param L Lua state
 */
void register_lua_constants(lua_State* L);

/**
 * luaopen_utils - Register utils module functions
 *
 * Registers the utils module with helper functions.
 * Defined in lua_utils.cpp.
 *
 * @param L Lua state
 */
void luaopen_utils(lua_State* L);

/**
 * luaopen_rex - Register rex PCRE regex library
 *
 * Registers the rex module for Perl-compatible regular expressions.
 * Provides: rex.new(), rex.flags(), rex.version(), and regex object methods
 * (match, exec, gmatch).
 * Defined in lrexlib.cpp.
 *
 * @param L Lua state
 * @return 1 (returns the rex table)
 */
extern "C" int luaopen_rex(lua_State* L);

/**
 * register_array_functions - Register array API functions
 *
 * Registers functions for named array management:
 * - ArrayCreate, ArrayDelete, ArrayClear
 * - ArraySet, ArrayGet, ArrayDeleteKey
 * - ArrayExists, ArrayKeyExists
 * - ArrayCount, ArraySize
 * - ArrayGetFirstKey, ArrayGetLastKey
 * - ArrayListAll, ArrayListKeys, ArrayListValues
 * - ArrayExport, ArrayExportKeys, ArrayImport
 *
 * Defined in world_arrays.cpp.
 *
 * @param L Lua state
 * @param worldTable Index of world table on Lua stack
 */
void register_array_functions(lua_State* L, int worldTable);

#include <QStringList>

/**
 * getLuaFunctionNames - Get list of all Lua function names for Shift+Tab completion
 *
 * Dynamically queries the Lua state to collect function names from:
 * - world.* table (MUSHclient API functions)
 * - string.*, table.*, math.*, os.*, io.* (Lua standard libraries)
 * - coroutine.*, debug.*, bit.* (additional Lua libraries)
 * - Global functions (pairs, ipairs, type, etc.)
 *
 * Used for Shift+Tab completion in the command input.
 * Based on original MUSHclient's Shift+Tab function completion feature.
 *
 * @param L Lua state to query
 * @return QStringList of function names (e.g., "Note", "string.format", "pairs", ...)
 */
QStringList getLuaFunctionNames(lua_State* L);

#endif // LUA_REGISTRATION_H
