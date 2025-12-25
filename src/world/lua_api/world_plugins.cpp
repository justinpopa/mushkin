/**
 * world_plugins.cpp - Plugin Management Lua API
 *
 * This file implements the Lua C bindings for plugin-related functions.
 * Functions are registered in the global namespace for compatibility with legacy plugins.
 *
 * Extracted from lua_methods.cpp
 */

#include "../../automation/plugin.h"
#include "../../automation/sendto.h"
#include "../../automation/variable.h"
#include "../../world/world_document.h"
#include "../script_engine.h"

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Forward declarations for helper functions
extern bool GetNestedFunction(lua_State* L, const QString& name, bool errorIfNotFound);
extern int callLuaWithTraceBack(lua_State* L, int nArgs, int nResults);


// Note: eConnectConnectedToMud is defined in world_document.h
// Note: ON_PLUGIN_BROADCAST is defined in plugin.h

// ========== Helper Functions ==========

#include "logging.h"
#include "lua_common.h"

// ========== Plugin Functions ==========

/**
 * CallPlugin - Call function in another plugin's Lua environment
 *
 * Lua signature: status, ... = CallPlugin(pluginID, routine, ...)
 *
 * @param pluginID - Plugin GUID
 * @param routine - Function name to call (can be nested like "foo.bar.baz")
 * @param ... - Arguments to pass (nil, boolean, number, string only)
 *
 * Returns:
 *   On success: eOK (0), followed by return values from called function
 *   On error: error_code, error_message[, lua_error]
 *
 * Error codes:
 *   eNoSuchRoutine (-32) - Function name empty or function not found
 *   eNoSuchPlugin (-30) - Plugin not installed
 *   ePluginDisabled (-31) - Plugin is disabled
 *   eBadParameter (-1) - Cannot pass argument type
 *   eErrorCallingPluginRoutine (-33) - Runtime error (includes Lua error as 3rd return)
 *
 * Features:
 * - Cross-plugin communication with type safety
 * - Handles self-calls efficiently (no stack copying)
 * - Preserves plugin context during call
 * - Supports nested function names (e.g., "module.submodule.func")
 * - Only transfers simple types (nil, boolean, number, string)
 *
 * Based on CMUSHclientDoc::CallPlugin() from methods_plugins.cpp
 */
int L_CallPlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* routine = luaL_checkstring(L, 2);

    // Remove plugin ID and function name from stack
    // (This ensures if the called function does CallPlugin back, stack is clean)
    lua_remove(L, 1); // remove plugin ID
    lua_remove(L, 1); // remove function name

    int nArgs = lua_gettop(L); // number of remaining arguments

    // Preliminary checks

    // Function name given?
    if (!routine || routine[0] == '\0') {
        lua_pushnumber(L, eNoSuchRoutine);
        lua_pushstring(L, "No function name supplied");
        return 2;
    }

    // Plugin exists?
    Plugin* pPlugin = pDoc->getPlugin(QString::fromUtf8(pluginID));
    if (!pPlugin) {
        lua_pushnumber(L, eNoSuchPlugin);
        QString errorMsg = QString("Plugin ID (%1) is not installed").arg(pluginID);
        lua_pushstring(L, errorMsg.toUtf8().constData());
        return 2;
    }

    // Plugin is enabled?
    if (!pPlugin->enabled()) {
        lua_pushnumber(L, ePluginDisabled);
        QString errorMsg = QString("Plugin '%1' (%2) disabled").arg(pPlugin->name(), pluginID);
        lua_pushstring(L, errorMsg.toUtf8().constData());
        return 2;
    }

    // Plugin has a script engine?
    ScriptEngine* targetEngine = pPlugin->scriptEngine();
    if (!targetEngine || !targetEngine->isLua()) {
        lua_pushnumber(L, eNoSuchRoutine);
        QString errorMsg =
            QString("Scripting not enabled in plugin '%1' (%2)").arg(pPlugin->name(), pluginID);
        lua_pushstring(L, errorMsg.toUtf8().constData());
        return 2;
    }

    lua_State* pL = targetEngine->L; // target plugin's Lua state

    // Don't clear if we are calling ourselves
    if (pL != L) {
        lua_settop(pL, 0); // clear stack in target plugin
    }

    // Get wanted function onto stack
    if (!GetNestedFunction(pL, QString::fromUtf8(routine), false)) {
        lua_pushnumber(L, eNoSuchRoutine);
        QString errorMsg =
            QString("No function '%1' in plugin '%2' (%3)").arg(routine, pPlugin->name(), pluginID);
        lua_pushstring(L, errorMsg.toUtf8().constData());
        return 2;
    }

    // If calling ourselves, don't copy everything
    if (pL == L) {
        lua_insert(pL, 1); // move function to first position
    } else {
        // Calling a different plugin - copy arguments to target Lua state
        lua_checkstack(pL, nArgs + 2); // ensure space for function + args + return

        for (int i = 1; i <= nArgs; i++) {
            switch (lua_type(L, i)) {
                case LUA_TNIL:
                    lua_pushnil(pL);
                    break;
                case LUA_TBOOLEAN:
                    lua_pushboolean(pL, lua_toboolean(L, i));
                    break;
                case LUA_TNUMBER:
                    lua_pushnumber(pL, lua_tonumber(L, i));
                    break;
                case LUA_TSTRING: {
                    size_t len;
                    const char* s = lua_tolstring(L, i, &len);
                    lua_pushlstring(pL, s, len);
                    break;
                }
                default:
                    // Can't handle this type
                    lua_settop(pL, 0); // clear target stack
                    lua_pushnumber(L, eBadParameter);
                    QString errorMsg = QString("Cannot pass argument #%1 (%2 type) to CallPlugin")
                                           .arg(i + 2) // +2 because we removed pluginID and routine
                                           .arg(luaL_typename(L, i));
                    lua_pushstring(L, errorMsg.toUtf8().constData());
                    return 2;
            }
        }
    }

    // Save current plugin context
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* pSavedPlugin = plugin(L);
    QString strOldCallingPluginID = pPlugin->callingPluginID();

    if (pSavedPlugin) {
        pPlugin->setCallingPluginID(pSavedPlugin->id());
    } else {
        pPlugin->setCallingPluginID("");
    }

    pDoc->setCurrentPlugin(pPlugin);

    // Call the function in the target plugin
    if (callLuaWithTraceBack(pL, nArgs, LUA_MULTRET)) {
        // Runtime error occurred
        QString strLuaError = QString::fromUtf8(lua_tostring(pL, -1));

        // Restore context
        pDoc->setCurrentPlugin(pSavedPlugin);
        pPlugin->setCallingPluginID(strOldCallingPluginID);

        lua_settop(pL, 0); // clean stack

        // Return error information
        lua_pushnumber(L, eErrorCallingPluginRoutine);
        QString errorMsg = QString("Runtime error in function '%1', plugin '%2' (%3)")
                               .arg(routine, pPlugin->name(), pluginID);
        lua_pushstring(L, errorMsg.toUtf8().constData());
        lua_pushstring(L, strLuaError.toUtf8().constData());
        return 3;
    }

    // Success - restore context
    pDoc->setCurrentPlugin(pSavedPlugin);
    pPlugin->setCallingPluginID(strOldCallingPluginID);

    int nReturnValues = lua_gettop(pL); // number of values returned

    lua_pushnumber(L, eOK); // first return value: success code

    // If calling ourselves, don't copy
    if (pL == L) {
        lua_insert(L, 1); // put eOK as first item
        return 1 + nReturnValues;
    }

    // Copy return values back to caller's Lua state
    lua_checkstack(L, nReturnValues + 1);

    for (int i = 1; i <= nReturnValues; i++) {
        switch (lua_type(pL, i)) {
            case LUA_TNIL:
                lua_pushnil(L);
                break;
            case LUA_TBOOLEAN:
                lua_pushboolean(L, lua_toboolean(pL, i));
                break;
            case LUA_TNUMBER:
                lua_pushnumber(L, lua_tonumber(pL, i));
                break;
            case LUA_TSTRING: {
                size_t len;
                const char* s = lua_tolstring(pL, i, &len);
                lua_pushlstring(L, s, len);
                break;
            }
            default:
                // Can't handle this return type
                lua_pushnumber(L, eErrorCallingPluginRoutine);
                QString errorMsg = QString("Cannot handle return value #%1 (%2 type) from function "
                                           "'%3' in plugin '%4' (%5)")
                                       .arg(i)
                                       .arg(luaL_typename(pL, i))
                                       .arg(routine, pPlugin->name(), pluginID);
                lua_pushstring(L, errorMsg.toUtf8().constData());
                lua_settop(pL, 0);
                return 2;
        }
    }

    lua_settop(pL, 0);        // clean target plugin's stack
    return 1 + nReturnValues; // eOK plus all return values
}

/**
 * GetPluginID - Get current plugin's GUID
 *
 * Lua signature: id = GetPluginID()
 *
 * Returns: String or empty string if not in plugin context
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginID(lua_State* L)
{
    // Use plugin(L) to get the plugin from Lua registry (modal-safe)
    Plugin* currentPlugin = plugin(L);

    QString pluginID;
    if (currentPlugin) {
        pluginID = currentPlugin->m_strID;
        qCDebug(lcScript) << "GetPluginID: returning" << pluginID;
    } else {
        qCDebug(lcScript) << "GetPluginID: plugin(L) is NULL, returning empty string";
    }

    lua_pushstring(L, pluginID.toUtf8().constData());
    return 1;
}

/**
 * GetPluginName - Get current plugin's name
 *
 * Lua signature: name = GetPluginName()
 *
 * Returns: String or empty string if not in plugin context
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginName(lua_State* L)
{
    // Use plugin(L) to get the plugin from Lua registry (modal-safe)
    Plugin* currentPlugin = plugin(L);

    QString pluginName;
    if (currentPlugin) {
        pluginName = currentPlugin->m_strName;
    }

    lua_pushstring(L, pluginName.toUtf8().constData());
    return 1;
}

/**
 * GetPluginList - Get array of all installed plugin IDs
 *
 * Lua signature: plugin_ids = GetPluginList()
 *
 * Returns: Lua table (array) of GUID strings
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    int index = 1;
    for (const auto& plugin : pDoc->m_PluginList) {
        if (plugin) {
            lua_pushstring(L, plugin->m_strID.toUtf8().constData());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

/**
 * IsPluginInstalled - Check if plugin exists
 *
 * Lua signature: installed = IsPluginInstalled(pluginID)
 *
 * Returns: Boolean (true if installed)
 *
 * Based on methods_plugins.cpp
 */
int L_IsPluginInstalled(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    lua_pushboolean(L, plugin != nullptr);
    return 1;
}

/**
 * GetPluginInfo - Get plugin metadata by info type number
 *
 * Lua signature: value = GetPluginInfo(pluginID, infoType)
 *
 * Returns: Varies by infoType (string, number, boolean, or nil)
 *
 * Info types (25 total):
 *  1: Name           11: Timer count        21: Load order
 *  2: Author         12: Variable count     22: Date installed
 *  3: Description    13: Date written       23: Calling plugin ID
 *  4: Script         14: Date modified      24: Script time taken
 *  5: Language       15: Save state flag    25: Sequence
 *  6: Source path    16: Scripting enabled
 *  7: ID (GUID)      17: Enabled
 *  8: Purpose        18: Required version
 *  9: Trigger count  19: Version
 * 10: Alias count    20: Directory
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    int infoType = luaL_checkinteger(L, 2);

    qCDebug(lcScript) << "GetPluginInfo called: pluginID=" << pluginID << "infoType=" << infoType;

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        qCDebug(lcScript) << "GetPluginInfo: plugin not found for ID" << pluginID;
        lua_pushnil(L);
        return 1;
    }

    qCDebug(lcScript) << "GetPluginInfo: found plugin" << plugin->m_strName;

    switch (infoType) {
        case 1: // Name
            lua_pushstring(L, plugin->m_strName.toUtf8().constData());
            break;

        case 2: // Author
            lua_pushstring(L, plugin->m_strAuthor.toUtf8().constData());
            break;

        case 3: // Description
            lua_pushstring(L, plugin->m_strDescription.toUtf8().constData());
            break;

        case 4: // Script
            lua_pushstring(L, plugin->m_strScript.toUtf8().constData());
            break;

        case 5: // Language
            lua_pushstring(L, plugin->m_strLanguage.toUtf8().constData());
            break;

        case 6: // Source (file path)
            lua_pushstring(L, plugin->m_strSource.toUtf8().constData());
            break;

        case 7: // ID (GUID)
            lua_pushstring(L, plugin->m_strID.toUtf8().constData());
            break;

        case 8: // Purpose
            lua_pushstring(L, plugin->m_strPurpose.toUtf8().constData());
            break;

        case 9: // Trigger count
            lua_pushnumber(L, plugin->m_TriggerMap.size());
            break;

        case 10: // Alias count
            lua_pushnumber(L, plugin->m_AliasMap.size());
            break;

        case 11: // Timer count
            lua_pushnumber(L, plugin->m_TimerMap.size());
            break;

        case 12: // Variable count
            lua_pushnumber(L, plugin->m_VariableMap.size());
            break;

        case 13: // Date written
            if (plugin->m_tDateWritten.isValid()) {
                lua_pushstring(L,
                               plugin->m_tDateWritten.toString(Qt::ISODate).toUtf8().constData());
            } else {
                lua_pushnil(L);
            }
            break;

        case 14: // Date modified
            if (plugin->m_tDateModified.isValid()) {
                lua_pushstring(L,
                               plugin->m_tDateModified.toString(Qt::ISODate).toUtf8().constData());
            } else {
                lua_pushnil(L);
            }
            break;

        case 15: // Save state flag
            lua_pushboolean(L, plugin->m_bSaveState);
            break;

        case 16: // Scripting enabled (has script engine)
            lua_pushboolean(L, plugin->m_ScriptEngine != nullptr);
            break;

        case 17: // Enabled
            lua_pushboolean(L, plugin->m_bEnabled);
            break;

        case 18: // Required version
            lua_pushnumber(L, plugin->m_dRequiredVersion);
            break;

        case 19: // Version
            lua_pushnumber(L, plugin->m_dVersion);
            break;

        case 20: // Directory (with trailing slash)
        {
            QString dir = plugin->m_strDirectory;
            qCDebug(lcScript) << "GetPluginInfo(20): m_strDirectory =" << plugin->m_strDirectory
                              << "| isEmpty:" << dir.isEmpty();
            if (!dir.isEmpty() && !dir.endsWith("/")) {
                dir += "/";
            }
            qCDebug(lcScript) << "GetPluginInfo(20): returning:" << dir;
            lua_pushstring(L, dir.toUtf8().constData());
        } break;

        case 21: // Load order (recalculated from list position)
            lua_pushnumber(L, plugin->m_iLoadOrder);
            break;

        case 22: // Date installed
            if (plugin->m_tDateInstalled.isValid()) {
                lua_pushstring(L,
                               plugin->m_tDateInstalled.toString(Qt::ISODate).toUtf8().constData());
            } else {
                lua_pushnil(L);
            }
            break;

        case 23: // Calling plugin ID
            lua_pushstring(L, plugin->m_strCallingPluginID.toUtf8().constData());
            break;

        case 24: // Script time taken (in seconds)
            lua_pushnumber(L, plugin->m_iScriptTimeTaken / 1000.0);
            break;

        case 25: // Sequence
            lua_pushnumber(L, plugin->m_iSequence);
            break;

        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * LoadPlugin - Load plugin from XML file
 *
 * Lua signature: error_code = LoadPlugin(filepath)
 *
 * Returns: Integer error code (eOK=0, ePluginFileNotFound, eProblemsLoadingPlugin)
 *
 * Based on methods_plugins.cpp
 */
int L_LoadPlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* filepath = luaL_checkstring(L, 1);

    // Save current plugin context (don't let plugin load itself)
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = nullptr;

    // Try to load plugin
    QString errorMsg;
    Plugin* plugin = pDoc->LoadPlugin(QString::fromUtf8(filepath), errorMsg);

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    if (plugin) {
        lua_pushnumber(L, eOK);
    } else {
        // Determine error type based on error message
        if (errorMsg.contains("not found") || errorMsg.contains("Cannot open")) {
            lua_pushnumber(L, ePluginFileNotFound);
        } else {
            lua_pushnumber(L, eProblemsLoadingPlugin);
        }
    }

    return 1;
}

/**
 * ReloadPlugin - Unload and reload plugin (for development)
 *
 * Lua signature: error_code = ReloadPlugin(pluginID)
 *
 * Returns: Integer error code
 *
 * Safety: Cannot reload self (returns eBadParameter)
 *
 * Based on methods_plugins.cpp
 */
int L_ReloadPlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        return luaReturnError(L, eNoSuchPlugin);
    }

    // Can't reload self
    if (plugin == pDoc->m_CurrentPlugin) {
        return luaReturnError(L, eBadParameter);
    }

    // Save plugin filepath before unloading
    QString filepath = plugin->m_strSource;

    // Unload plugin
    if (!pDoc->UnloadPlugin(QString::fromUtf8(pluginID))) {
        return luaReturnError(L, eProblemsLoadingPlugin);
    }

    // Reload it
    QString errorMsg;
    Plugin* newPlugin = pDoc->LoadPlugin(filepath, errorMsg);

    if (newPlugin) {
        lua_pushnumber(L, eOK);
    } else {
        lua_pushnumber(L, ePluginFileNotFound);
    }

    return 1;
}

/**
 * UnloadPlugin - Unload and delete plugin
 *
 * Lua signature: error_code = UnloadPlugin(pluginID)
 *
 * Returns: Integer error code
 *
 * Safety: Cannot unload self (returns eBadParameter)
 *
 * Based on methods_plugins.cpp
 */
int L_UnloadPlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        return luaReturnError(L, eNoSuchPlugin);
    }

    // Can't unload self
    if (plugin == pDoc->m_CurrentPlugin) {
        return luaReturnError(L, eBadParameter);
    }

    // Unload plugin
    if (pDoc->UnloadPlugin(QString::fromUtf8(pluginID))) {
        lua_pushnumber(L, eOK);
    } else {
        lua_pushnumber(L, eProblemsLoadingPlugin);
    }

    return 1;
}

/**
 * EnablePlugin - Enable or disable plugin
 *
 * Lua signature: error_code = EnablePlugin(pluginID, enabled)
 *
 * Returns: Integer error code
 *
 * Calls OnPluginEnable or OnPluginDisable callback
 *
 * Based on methods_plugins.cpp
 */
int L_EnablePlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        return luaReturnError(L, eNoSuchPlugin);
    }

    // Enable/disable plugin
    if (pDoc->EnablePlugin(QString::fromUtf8(pluginID), enabled)) {
        lua_pushnumber(L, eOK);
    } else {
        lua_pushnumber(L, eProblemsLoadingPlugin);
    }

    return 1;
}

/**
 * PluginSupports - Check if plugin has a function
 *
 * Lua signature: error_code = PluginSupports(pluginID, routine)
 *
 * Returns: Integer error code (eOK if exists, eNoSuchRoutine if not)
 *
 * Based on methods_plugins.cpp
 */
int L_PluginSupports(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* routine = luaL_checkstring(L, 2);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        return luaReturnError(L, eNoSuchPlugin);
    }

    if (!plugin->m_ScriptEngine) {
        return luaReturnError(L, eNoSuchRoutine);
    }

    // Check if routine exists
    lua_State* pluginL = plugin->m_ScriptEngine->L;
    lua_getglobal(pluginL, routine);
    bool exists = lua_isfunction(pluginL, -1);
    lua_pop(pluginL, 1);

    if (exists) {
        lua_pushnumber(L, eOK);
    } else {
        lua_pushnumber(L, eNoSuchRoutine);
    }

    return 1;
}

/**
 * BroadcastPlugin - Send message to all plugins (pub/sub pattern)
 *
 * Lua signature: count = BroadcastPlugin(message, text)
 *
 * Returns: Integer count of plugins that received message
 *
 * Calls OnPluginBroadcast callback with (message, senderID, senderName, text)
 *
 * Based on methods_plugins.cpp
 */
int L_BroadcastPlugin(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    int message = luaL_checkinteger(L, 1);
    const char* text = luaL_optstring(L, 2, "");

    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    int count = 0;

    QString senderID, senderName;
    if (savedPlugin) {
        senderID = savedPlugin->m_strID;
        senderName = savedPlugin->m_strName;
    }

    qCDebug(lcScript) << "BroadcastPlugin called from" << senderName << "with message" << message
                      << "to" << pDoc->m_PluginList.size() << "plugins";

    for (const auto& plugin : pDoc->m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        // Don't broadcast to self
        if (plugin.get() == savedPlugin) {
            continue;
        }

        // Set target as current
        pDoc->m_CurrentPlugin = plugin.get();

        // Call OnPluginBroadcast(message, senderID, senderName, text)
        plugin->ExecutePluginScript(ON_PLUGIN_BROADCAST, message, senderID, senderName,
                                    QString::fromUtf8(text));

        count++;
    }

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    lua_pushnumber(L, count);
    return 1;
}

/**
 * SendPkt - Send raw packet to MUD
 *
 * Lua signature: error_code = SendPkt(packet_string)
 *
 * Returns: Integer error code (eOK, eWorldClosed)
 *
 * Sends raw bytes to server (can include nulls)
 * Used by GMCP and other telnet protocol implementations
 *
 * Based on lua_methods.cpp and methods_utilities.cpp
 */
int L_SendPkt(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Check if connected (eWorldClosed = 1, eOK = 0)
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        lua_pushnumber(L, 1); // eWorldClosed
        return 1;
    }

    // Get packet data (can include nulls)
    size_t textLength;
    const char* text = luaL_checklstring(L, 1, &textLength);

    // DEBUG: Log GMCP packets being sent
    // Check if this looks like a GMCP packet (starts with IAC SB GMCP = 0xFF 0xFA 0xC9)
    if (textLength >= 4 && (unsigned char)text[0] == 0xFF && (unsigned char)text[1] == 0xFA &&
        (unsigned char)text[2] == 201) {
        // Extract the GMCP message (skip IAC SB GMCP, end before IAC SE)
        QString gmcpMsg;
        for (size_t i = 3; i < textLength - 2; i++) {
            gmcpMsg += QChar(text[i]);
        }
        qDebug() << "SendPkt: Sending GMCP packet:" << gmcpMsg;
    }

    // Send raw packet
    pDoc->SendPacket((const unsigned char*)text, textLength);

    lua_pushnumber(L, 0); // eOK
    return 1;
}

/**
 * SaveState - Save current plugin's state to file
 *
 * Lua signature: error_code = SaveState()
 *
 * Returns: Integer error code (eOK, eNotAPlugin, ePluginCouldNotSaveState)
 *
 * Safety: Checks m_bSavingStateNow flag to prevent infinite recursion
 *
 * Based on methods_plugins.cpp
 */
int L_SaveState(lua_State* L)
{
    // Use plugin(L) to get the plugin from Lua registry (modal-safe)
    Plugin* currentPlugin = plugin(L);

    // Must be called from plugin
    if (!currentPlugin) {
        return luaReturnError(L, eNotAPlugin);
    }

    // Check if already saving (prevent recursion)
    if (currentPlugin->m_bSavingStateNow) {
        return luaReturnError(L, ePluginCouldNotSaveState);
    }

    // Save state
    bool success = currentPlugin->SaveState();

    if (success) {
        lua_pushnumber(L, eOK);
    } else {
        lua_pushnumber(L, ePluginCouldNotSaveState);
    }

    return 1;
}

/**
 * GetPluginVariable - Get variable from plugin's VariableMap
 *
 * Lua signature: value = GetPluginVariable(pluginID, variableName)
 *
 * Returns: String value or nil
 *
 * Uses GET_PLUGIN_STUFF pattern: sets m_CurrentPlugin, calls GetVariable, restores
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginVariable(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* variableName = luaL_checkstring(L, 2);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Get variable from plugin's map
    QString varName = QString::fromUtf8(variableName);
    auto it = plugin->m_VariableMap.find(varName);
    if (it != plugin->m_VariableMap.end()) {
        lua_pushstring(L, it->second->strContents.toUtf8().constData());
    } else {
        lua_pushnil(L);
    }

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    return 1;
}

/**
 * GetPluginVariableList - Get array of all variable names in plugin
 *
 * Lua signature: vars = GetPluginVariableList(pluginID)
 *
 * Returns: Lua table (array) of variable names
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginVariableList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    lua_newtable(L);

    if (plugin) {
        int index = 1;
        for (const auto& [name, var] : plugin->m_VariableMap) {
            lua_pushstring(L, name.toUtf8().constData());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

// ========== Registration ==========

/**
 * register_plugin_functions - Register all plugin-related Lua API functions
 *
 * Registers functions as globals for compatibility with legacy plugins.
 *
 * @param L Lua state
 */
void register_plugin_functions(lua_State* L)
{
    // Plugin management functions
    lua_pushcfunction(L, L_CallPlugin);
    lua_setglobal(L, "CallPlugin");

    lua_pushcfunction(L, L_GetPluginID);
    lua_setglobal(L, "GetPluginID");

    lua_pushcfunction(L, L_GetPluginName);
    lua_setglobal(L, "GetPluginName");

    lua_pushcfunction(L, L_GetPluginList);
    lua_setglobal(L, "GetPluginList");

    lua_pushcfunction(L, L_IsPluginInstalled);
    lua_setglobal(L, "IsPluginInstalled");

    lua_pushcfunction(L, L_GetPluginInfo);
    lua_setglobal(L, "GetPluginInfo");

    lua_pushcfunction(L, L_LoadPlugin);
    lua_setglobal(L, "LoadPlugin");

    lua_pushcfunction(L, L_ReloadPlugin);
    lua_setglobal(L, "ReloadPlugin");

    lua_pushcfunction(L, L_UnloadPlugin);
    lua_setglobal(L, "UnloadPlugin");

    lua_pushcfunction(L, L_EnablePlugin);
    lua_setglobal(L, "EnablePlugin");

    lua_pushcfunction(L, L_PluginSupports);
    lua_setglobal(L, "PluginSupports");

    lua_pushcfunction(L, L_BroadcastPlugin);
    lua_setglobal(L, "BroadcastPlugin");

    lua_pushcfunction(L, L_SendPkt);
    lua_setglobal(L, "SendPkt");

    lua_pushcfunction(L, L_SaveState);
    lua_setglobal(L, "SaveState");

    lua_pushcfunction(L, L_GetPluginVariable);
    lua_setglobal(L, "GetPluginVariable");

    lua_pushcfunction(L, L_GetPluginVariableList);
    lua_setglobal(L, "GetPluginVariableList");
}
