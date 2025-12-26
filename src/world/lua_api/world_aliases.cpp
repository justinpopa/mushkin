/**
 * world_aliases.cpp - Alias API Functions
 *
 * Extracted from lua_methods.cpp
 * Implements alias management functions for the Lua API.
 */

#include "../../automation/alias.h"
#include "../../automation/plugin.h"
#include "../../automation/sendto.h"
#include "../../world/world_document.h"
#include "../script_engine.h"

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "lua_common.h"

/**
 * world.AddAlias(name, match, response, flags, script)
 *
 * Creates a new alias.
 *
 * @param name Alias label
 * @param match Pattern to match
 * @param response Text to send when triggered
 * @param flags Bit flags (enabled, regexp, etc.)
 * @param script Script procedure to call (optional)
 * @return error code (eOK=0 on success)
 */
int L_AddAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* match = luaL_checkstring(L, 2);
    const char* response = luaL_checkstring(L, 3);
    int flags = luaL_checkinteger(L, 4);
    const char* script = luaL_optstring(L, 5, "");

    QString qName = QString::fromUtf8(name);
    QString qMatch = QString::fromUtf8(match);

    // Validate and normalize alias name
    qint32 nameStatus = validateObjectName(qName);
    if (nameStatus != eOK) {
        lua_pushnumber(L, nameStatus);
        return 1;
    }

    // Check if alias already exists (check appropriate map based on context)
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* currentPlugin = plugin(L);
    bool replace = (flags & eReplace) != 0;
    if (currentPlugin) {
        if (currentPlugin->m_AliasMap.find(qName) != currentPlugin->m_AliasMap.end()) {
            if (!replace) {
                return luaReturnError(L, eAliasAlreadyExists);
            }
            // Delete existing alias for replacement
            currentPlugin->m_AliasMap.erase(qName);
        }
    } else {
        if (pDoc->getAlias(qName)) {
            if (!replace) {
                return luaReturnError(L, eAliasAlreadyExists);
            }
            // Delete existing alias for replacement
            pDoc->deleteAlias(qName);
        }
    }

    // Cannot have empty match
    if (qMatch.isEmpty()) {
        return luaReturnError(L, eAliasCannotBeEmpty);
    }

    // Create alias
    auto alias = std::make_unique<Alias>();
    alias->strLabel = qName;
    alias->strInternalName = qName;
    alias->name = qMatch;
    alias->contents = QString::fromUtf8(response);
    alias->bEnabled = (flags & eEnabled) != 0;
    alias->bIgnoreCase = (flags & eIgnoreAliasCase) != 0;
    alias->bOmitFromLog = (flags & eOmitFromLogFile) != 0;
    alias->bRegexp = (flags & eAliasRegularExpression) != 0;
    alias->bOmitFromOutput = (flags & eAliasOmitFromOutput) != 0;
    alias->bExpandVariables = (flags & eExpandVariables) != 0;
    alias->bMenu = (flags & eAliasMenu) != 0;
    alias->bTemporary = (flags & eTemporary) != 0;
    alias->bOneShot = (flags & eAliasOneShot) != 0;
    // Note: bKeepEvaluating defaults to true (MUSHclient behavior)
    // Only explicitly set if flag is present
    if (flags & eKeepEvaluating) {
        alias->bKeepEvaluating = true;
    }
    // else: keep the default from Alias constructor (true)
    alias->strProcedure = QString::fromUtf8(script);
    alias->iSequence = 100; // Default sequence

    // Handle special send-to flags
    // If a script procedure is specified, default to eSendToScript
    if (flags & eAliasSpeedWalk) {
        alias->iSendTo = eSendToSpeedwalk;
    } else if (flags & eAliasQueue) {
        alias->iSendTo = eSendToCommandQueue;
    } else if (!alias->strProcedure.isEmpty()) {
        alias->iSendTo = eSendToScript;
    } else {
        alias->iSendTo = eSendToWorld;
    }

    // Add to appropriate alias map (plugin or world)
    if (currentPlugin) {
        currentPlugin->m_AliasMap[qName] = std::move(alias);
        // Rebuild alias array for matching
        currentPlugin->m_AliasArray.clear();
        for (const auto& [name, a] : currentPlugin->m_AliasMap) {
            currentPlugin->m_AliasArray.push_back(a.get());
        }
    } else {
        if (!pDoc->addAlias(qName, std::move(alias))) {
            return luaReturnError(L, eAliasAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteAlias(name)
 *
 * Deletes an alias.
 *
 * @param name Alias label
 * @return error code (eOK=0 on success)
 */
int L_DeleteAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    if (!pDoc->deleteAlias(qName)) {
        return luaReturnError(L, eAliasNotFound);
    }

    return luaReturnOK(L);
}

/**
 * world.IsAlias(name)
 *
 * Checks if an alias exists.
 *
 * @param name Alias label
 * @return eOK if exists, eAliasNotFound if not
 */
int L_IsAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Alias* alias = pDoc->getAlias(qName);

    lua_pushnumber(L, alias ? eOK : eAliasNotFound);
    return 1;
}

/**
 * world.GetAlias(name)
 *
 * Gets alias details.
 *
 * @param name Alias label
 * @return error_code, match_text, response_text, flags, script_name
 *         Returns just error_code if alias not found
 */
int L_GetAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Alias* alias = pDoc->getAlias(qName);

    if (!alias) {
        return luaReturnError(L, eAliasNotFound);
    }

    // Build flags
    int flags = 0;
    if (alias->bEnabled)
        flags |= eEnabled;
    if (alias->bIgnoreCase)
        flags |= eIgnoreAliasCase;
    if (alias->bOmitFromLog)
        flags |= eOmitFromLogFile;
    if (alias->bRegexp)
        flags |= eAliasRegularExpression;
    if (alias->bExpandVariables)
        flags |= eExpandVariables;
    if (alias->bOmitFromOutput)
        flags |= eAliasOmitFromOutput;
    if (alias->bOneShot)
        flags |= eAliasOneShot;
    if (alias->bKeepEvaluating)
        flags |= eKeepEvaluating;

    // Return: error_code, match, response, flags, script
    lua_pushnumber(L, eOK);
    lua_pushstring(L, alias->name.toUtf8().constData());
    lua_pushstring(L, alias->contents.toUtf8().constData());
    lua_pushnumber(L, flags);
    lua_pushstring(L, alias->strProcedure.toUtf8().constData());

    return 5;
}

/**
 * world.EnableAlias(name, enabled)
 *
 * Enables or disables an alias.
 *
 * @param name Alias label
 * @param enabled true to enable, false to disable
 * @return error code (eOK=0 on success)
 */
int L_EnableAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qName = QString::fromUtf8(name);
    Alias* alias = pDoc->getAlias(qName);

    if (!alias) {
        return luaReturnError(L, eAliasNotFound);
    }

    alias->bEnabled = enabled;
    return luaReturnOK(L);
}

/**
 * world.GetAliasInfo(name, info_type)
 *
 * Gets information about an alias.
 * Matches original MUSHclient info type mappings.
 *
 * Info types:
 *   1 = name (pattern)          18 = send_to
 *   2 = contents (send text)    19 = keep_evaluating
 *   3 = script (strProcedure)   20 = sequence
 *   4 = omit_from_log           21 = echo_alias
 *   5 = omit_from_output        22 = omit_from_command_history
 *   6 = enabled                 23 = user option
 *   7 = regexp                  24 = regexp match count
 *   8 = ignore_case             25 = last matching string
 *   9 = expand_variables        26 = executing script
 *  10 = invocation count        27 = has script (dispid)
 *  11 = times matched           28 = regexp error
 *  12 = menu                    29 = one_shot
 *  13 = when matched (date)     30 = regexp time
 *  14 = temporary               31 = regexp attempts
 *  15 = included                101-110 = wildcards
 *  16 = group
 *  17 = variable
 *
 * @param name Alias label
 * @param info_type Type of information to get
 * @return Information value or nil if not found
 */
int L_GetAliasInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    int info_type = luaL_checkinteger(L, 2);

    QString qName = QString::fromUtf8(name);
    Alias* alias = pDoc->getAlias(qName);

    if (!alias) {
        lua_pushnil(L);
        return 1;
    }

    switch (info_type) {
        case 1: // name (match pattern)
        {
            QByteArray ba = alias->name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 2: // contents (send text)
        {
            QByteArray ba = alias->contents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 3: // strProcedure (script name)
        {
            QByteArray ba = alias->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // bOmitFromLog
            lua_pushboolean(L, alias->bOmitFromLog);
            break;
        case 5: // bOmitFromOutput
            lua_pushboolean(L, alias->bOmitFromOutput);
            break;
        case 6: // bEnabled
            lua_pushboolean(L, alias->bEnabled);
            break;
        case 7: // bRegexp
            lua_pushboolean(L, alias->bRegexp);
            break;
        case 8: // bIgnoreCase
            lua_pushboolean(L, alias->bIgnoreCase);
            break;
        case 9: // bExpandVariables
            lua_pushboolean(L, alias->bExpandVariables);
            break;
        case 10: // nInvocationCount
            lua_pushnumber(L, alias->nInvocationCount);
            break;
        case 11: // nMatched
            lua_pushnumber(L, alias->nMatched);
            break;
        case 12: // bMenu
            lua_pushboolean(L, alias->bMenu);
            break;
        case 13: // tWhenMatched
            if (alias->tWhenMatched.isValid()) {
                // Return as Unix timestamp (seconds since epoch)
                lua_pushnumber(L, alias->tWhenMatched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L); // Return nil if never matched
            }
            break;
        case 14: // bTemporary
            lua_pushboolean(L, alias->bTemporary);
            break;
        case 15: // bIncluded
            lua_pushboolean(L, alias->bIncluded);
            break;
        case 16: // strGroup
        {
            QByteArray ba = alias->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // strVariable
        {
            QByteArray ba = alias->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 18: // iSendTo
            lua_pushnumber(L, alias->iSendTo);
            break;
        case 19: // bKeepEvaluating
            lua_pushboolean(L, alias->bKeepEvaluating);
            break;
        case 20: // iSequence
            lua_pushnumber(L, alias->iSequence);
            break;
        case 21: // bEchoAlias
            lua_pushboolean(L, alias->bEchoAlias);
            break;
        case 22: // bOmitFromCommandHistory
            lua_pushboolean(L, alias->bOmitFromCommandHistory);
            break;
        case 23: // iUserOption
            lua_pushnumber(L, alias->iUserOption);
            break;
        case 24: // regexp match count
            if (alias->regexp) {
                // Count of wildcard matches (how many wildcards were captured)
                lua_pushnumber(L, alias->wildcards.size());
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 25: // last matching string
            if (!alias->wildcards.isEmpty()) {
                // wildcards[0] is the entire match
                QByteArray ba = alias->wildcards[0].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 26: // bExecutingScript
            lua_pushboolean(L, alias->bExecutingScript);
            break;
        case 27: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, alias->dispid != -1);
            break;
        case 28: // regexp execution error
            // We don't track regexp errors in Qt (always 0)
            lua_pushnumber(L, 0);
            break;
        case 29: // bOneShot
            lua_pushboolean(L, alias->bOneShot);
            break;
        case 30: // regexp execution time
            // We don't track execution time (always 0)
            lua_pushnumber(L, 0);
            break;
        case 31: // regexp match attempts
            // We don't track match attempts (always 0)
            lua_pushnumber(L, 0);
            break;

        // Wildcards: 101-110 = wildcards[1-9] and [0]
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110: {
            int wildcard_index = (info_type == 110) ? 0 : (info_type - 100);
            if (wildcard_index < alias->wildcards.size()) {
                QByteArray ba = alias->wildcards[wildcard_index].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
        } break;

        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.GetAliasList()
 *
 * Gets a list of all alias names.
 *
 * @return Table (array) of alias names
 */
int L_GetAliasList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);
    int i = 1;
    for (const auto& [name, aliasPtr] : pDoc->m_AliasMap) {
        QByteArray ba = name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/**
 * GetPluginAliasList - Get array of all alias names in plugin
 *
 * Lua signature: aliases = GetPluginAliasList(pluginID)
 *
 * Returns: Lua table (array) of alias names
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginAliasList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    lua_newtable(L);

    if (plugin) {
        int index = 1;
        for (const auto& [name, aliasPtr] : plugin->m_AliasMap) {
            lua_pushstring(L, name.toUtf8().constData());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

/**
 * GetPluginAliasInfo - Get alias info from plugin
 *
 * Lua signature: value = GetPluginAliasInfo(pluginID, aliasName, infoType)
 *
 * Returns: Varies by infoType
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginAliasInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* aliasName = luaL_checkstring(L, 2);
    int infoType = luaL_checkinteger(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find alias in plugin's map
    QString aName = QString::fromUtf8(aliasName);
    Alias* alias = nullptr;
    auto it = plugin->m_AliasMap.find(aName);
    if (it != plugin->m_AliasMap.end()) {
        alias = it->second.get();
    }

    // Get alias info (reuse exact GetAliasInfo logic)
    if (!alias) {
        lua_pushnil(L);
        pDoc->m_CurrentPlugin = savedPlugin;
        return 1;
    }

    switch (infoType) {
        case 1: // name (match pattern)
        {
            QByteArray ba = alias->name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 2: // contents (send text)
        {
            QByteArray ba = alias->contents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 3: // strProcedure (script name)
        {
            QByteArray ba = alias->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // bOmitFromLog
            lua_pushboolean(L, alias->bOmitFromLog);
            break;
        case 5: // bOmitFromOutput
            lua_pushboolean(L, alias->bOmitFromOutput);
            break;
        case 6: // bEnabled
            lua_pushboolean(L, alias->bEnabled);
            break;
        case 7: // bRegexp
            lua_pushboolean(L, alias->bRegexp);
            break;
        case 8: // bIgnoreCase
            lua_pushboolean(L, alias->bIgnoreCase);
            break;
        case 9: // bExpandVariables
            lua_pushboolean(L, alias->bExpandVariables);
            break;
        case 10: // nInvocationCount
            lua_pushnumber(L, alias->nInvocationCount);
            break;
        case 11: // nMatched
            lua_pushnumber(L, alias->nMatched);
            break;
        case 12: // bMenu
            lua_pushboolean(L, alias->bMenu);
            break;
        case 13: // tWhenMatched
            if (alias->tWhenMatched.isValid()) {
                lua_pushnumber(L, alias->tWhenMatched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 14: // bTemporary
            lua_pushboolean(L, alias->bTemporary);
            break;
        case 15: // bIncluded
            lua_pushboolean(L, alias->bIncluded);
            break;
        case 16: // strGroup
        {
            QByteArray ba = alias->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // strVariable
        {
            QByteArray ba = alias->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 18: // iSendTo
            lua_pushnumber(L, alias->iSendTo);
            break;
        case 19: // bKeepEvaluating
            lua_pushboolean(L, alias->bKeepEvaluating);
            break;
        case 20: // iSequence
            lua_pushnumber(L, alias->iSequence);
            break;
        case 21: // bEchoAlias
            lua_pushboolean(L, alias->bEchoAlias);
            break;
        case 22: // bOmitFromCommandHistory
            lua_pushboolean(L, alias->bOmitFromCommandHistory);
            break;
        case 23: // iUserOption
            lua_pushnumber(L, alias->iUserOption);
            break;
        case 24: // regexp match count
            if (alias->regexp) {
                lua_pushnumber(L, alias->wildcards.size());
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 25: // last matching string
            if (!alias->wildcards.isEmpty()) {
                QByteArray ba = alias->wildcards[0].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 26: // bExecutingScript
            lua_pushboolean(L, alias->bExecutingScript);
            break;
        case 27: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, alias->dispid != -1);
            break;
        case 28: // regexp execution error
            lua_pushnumber(L, 0);
            break;
        case 29: // bOneShot
            lua_pushboolean(L, alias->bOneShot);
            break;
        case 30: // regexp execution time
            lua_pushnumber(L, 0);
            break;
        case 31: // regexp match attempts
            lua_pushnumber(L, 0);
            break;

        // Wildcards: 101-110 = wildcards[1-9] and [0]
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
        case 110: {
            int wildcard_index = (infoType == 110) ? 0 : (infoType - 100);
            if (wildcard_index < alias->wildcards.size()) {
                QByteArray ba = alias->wildcards[wildcard_index].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
        } break;

        default:
            lua_pushnil(L);
            break;
    }

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    return 1;
}

/**
 * GetPluginAliasOption - Get alias option from plugin
 *
 * Lua signature: value = GetPluginAliasOption(pluginID, aliasName, optionName)
 *
 * Returns: Varies by option
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginAliasOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* aliasName = luaL_checkstring(L, 2);
    const char* optionName = luaL_checkstring(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find alias
    QString aName = QString::fromUtf8(aliasName);
    Alias* alias = nullptr;
    auto it = plugin->m_AliasMap.find(aName);
    if (it != plugin->m_AliasMap.end()) {
        alias = it->second.get();
    }

    if (alias) {
        QString option = QString::fromUtf8(optionName);
        if (option == "enabled") {
            lua_pushboolean(L, alias->bEnabled);
        } else if (option == "keep_evaluating") {
            lua_pushboolean(L, alias->bKeepEvaluating);
        } else if (option == "sequence") {
            lua_pushnumber(L, alias->iSequence);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
    }

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    return 1;
}

/**
 * world.GetAliasOption(alias_name, option_name)
 *
 * Gets an option value for an alias.
 * Based on CMUSHclientDoc::GetAliasOption() from methods_aliases.cpp
 *
 * @param alias_name Name of the alias
 * @param option_name Name of the option to get
 * @return Option value or nil if not found
 */
int L_GetAliasOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* aliasName = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(aliasName);
    QString qOption = QString::fromUtf8(optionName).toLower().trimmed();

    Alias* alias = pDoc->getAlias(qName);
    if (!alias) {
        lua_pushnil(L);
        return 1;
    }

    // Numeric options
    if (qOption == "send_to") {
        lua_pushnumber(L, alias->iSendTo);
    } else if (qOption == "sequence") {
        lua_pushnumber(L, alias->iSequence);
    } else if (qOption == "user") {
        lua_pushnumber(L, alias->iUserOption);
    }
    // Boolean options
    else if (qOption == "enabled") {
        lua_pushboolean(L, alias->bEnabled);
    } else if (qOption == "expand_variables") {
        lua_pushboolean(L, alias->bExpandVariables);
    } else if (qOption == "ignore_case") {
        lua_pushboolean(L, alias->bIgnoreCase);
    } else if (qOption == "omit_from_log") {
        lua_pushboolean(L, alias->bOmitFromLog);
    } else if (qOption == "omit_from_command_history") {
        lua_pushboolean(L, alias->bOmitFromCommandHistory);
    } else if (qOption == "omit_from_output") {
        lua_pushboolean(L, alias->bOmitFromOutput);
    } else if (qOption == "regexp") {
        lua_pushboolean(L, alias->bRegexp);
    } else if (qOption == "menu") {
        lua_pushboolean(L, alias->bMenu);
    } else if (qOption == "keep_evaluating") {
        lua_pushboolean(L, alias->bKeepEvaluating);
    } else if (qOption == "echo_alias") {
        lua_pushboolean(L, alias->bEchoAlias);
    } else if (qOption == "temporary") {
        lua_pushboolean(L, alias->bTemporary);
    } else if (qOption == "one_shot") {
        lua_pushboolean(L, alias->bOneShot);
    }
    // String options
    else if (qOption == "group") {
        QByteArray ba = alias->strGroup.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "match") {
        QByteArray ba = alias->name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "script") {
        QByteArray ba = alias->strProcedure.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "send") {
        QByteArray ba = alias->contents.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "variable") {
        QByteArray ba = alias->strVariable.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/**
 * world.SetAliasOption(alias_name, option_name, value)
 *
 * Sets an option value for an alias.
 * Based on CMUSHclientDoc::SetAliasOption() from methods_aliases.cpp
 *
 * @param alias_name Name of the alias
 * @param option_name Name of the option to set
 * @param value New value for the option
 * @return Error code (eOK=0 on success)
 */
int L_SetAliasOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* aliasName = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(aliasName);
    QString qOption = QString::fromUtf8(optionName).toLower().trimmed();

    Alias* alias = pDoc->getAlias(qName);
    if (!alias) {
        return luaReturnError(L, eAliasNotFound);
    }

    // Numeric options
    if (qOption == "send_to" || qOption == "sequence" || qOption == "user") {
        long value = luaL_checknumber(L, 3);

        if (qOption == "send_to") {
            alias->iSendTo = value;
        } else if (qOption == "sequence") {
            alias->iSequence = value;
            pDoc->m_aliasesNeedSorting = true;
        } else if (qOption == "user") {
            alias->iUserOption = value;
        }
    }
    // Boolean options
    else if (qOption == "enabled" || qOption == "expand_variables" || qOption == "ignore_case" ||
             qOption == "omit_from_log" || qOption == "omit_from_command_history" ||
             qOption == "omit_from_output" || qOption == "regexp" || qOption == "menu" ||
             qOption == "keep_evaluating" || qOption == "echo_alias" || qOption == "temporary" ||
             qOption == "one_shot") {
        bool value = lua_toboolean(L, 3);

        if (qOption == "enabled") {
            alias->bEnabled = value;
        } else if (qOption == "expand_variables") {
            alias->bExpandVariables = value;
        } else if (qOption == "ignore_case") {
            alias->bIgnoreCase = value;
            alias->compileRegexp(); // Recompile with new case sensitivity
        } else if (qOption == "omit_from_log") {
            alias->bOmitFromLog = value;
        } else if (qOption == "omit_from_command_history") {
            alias->bOmitFromCommandHistory = value;
        } else if (qOption == "omit_from_output") {
            alias->bOmitFromOutput = value;
        } else if (qOption == "regexp") {
            return luaReturnError(L, ePluginCannotSetOption); // Cannot write
        } else if (qOption == "menu") {
            alias->bMenu = value;
        } else if (qOption == "keep_evaluating") {
            alias->bKeepEvaluating = value;
        } else if (qOption == "echo_alias") {
            alias->bEchoAlias = value;
        } else if (qOption == "temporary") {
            alias->bTemporary = value;
        } else if (qOption == "one_shot") {
            alias->bOneShot = value;
        }
    }
    // String options
    else if (qOption == "group" || qOption == "match" || qOption == "script" || qOption == "send" ||
             qOption == "variable") {
        const char* value = luaL_checkstring(L, 3);
        QString qValue = QString::fromUtf8(value);

        if (qOption == "group") {
            alias->strGroup = qValue;
        } else if (qOption == "match") {
            if (qValue.isEmpty()) {
                return luaReturnError(L, eAliasCannotBeEmpty);
            }
            alias->name = qValue;
            alias->compileRegexp(); // Recompile with new pattern
        } else if (qOption == "script") {
            alias->strProcedure = qValue;
            alias->dispid = DISPID_UNKNOWN; // Reset so it gets re-looked up
        } else if (qOption == "send") {
            alias->contents = qValue;
        } else if (qOption == "variable") {
            alias->strVariable = qValue;
        }
    } else {
        return luaReturnError(L, eUnknownOption);
    }

    return luaReturnOK(L);
}

/**
 * world.EnableAliasGroup(groupName, enabled)
 *
 * Enables or disables all aliases in a group.
 *
 * @param groupName Alias group name
 * @param enabled True to enable, false to disable
 * @return Count of aliases affected
 */
int L_EnableAliasGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qGroupName = QString::fromUtf8(groupName);
    int count = 0;

    // Iterate through all aliases
    for (const auto& [name, aliasPtr] : pDoc->m_AliasMap) {
        Alias* alias = aliasPtr.get();
        if (alias->strGroup == qGroupName) {
            alias->bEnabled = enabled;
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.DeleteAliasGroup(groupName)
 *
 * Deletes all aliases in a group.
 *
 * @param groupName Alias group name
 * @return Count of aliases deleted
 */
int L_DeleteAliasGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);

    QString qGroupName = QString::fromUtf8(groupName);
    QStringList toDelete;

    // Find all aliases in this group
    for (const auto& [name, aliasPtr] : pDoc->m_AliasMap) {
        if (aliasPtr->strGroup == qGroupName) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteAlias(name);
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.DeleteTemporaryAliases()
 *
 * Deletes all temporary aliases.
 *
 * @return Count of aliases deleted
 */
int L_DeleteTemporaryAliases(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList toDelete;

    // Find all temporary aliases
    for (const auto& [name, aliasPtr] : pDoc->m_AliasMap) {
        if (aliasPtr->bTemporary) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteAlias(name);
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * register_alias_functions - Register alias API functions
 *
 * Registers all alias-related functions in the worldlib table.
 *
 * @param worldlib Array of luaL_Reg to add functions to
 * @param index Current index in the worldlib array
 * @return Updated index
 */
void register_alias_functions(luaL_Reg* worldlib, int& index)
{
    worldlib[index++] = {"AddAlias", L_AddAlias};
    worldlib[index++] = {"DeleteAlias", L_DeleteAlias};
    worldlib[index++] = {"DeleteAliasGroup", L_DeleteAliasGroup};
    worldlib[index++] = {"DeleteTemporaryAliases", L_DeleteTemporaryAliases};
    worldlib[index++] = {"EnableAlias", L_EnableAlias};
    worldlib[index++] = {"EnableAliasGroup", L_EnableAliasGroup};
    worldlib[index++] = {"GetAlias", L_GetAlias};
    worldlib[index++] = {"GetAliasInfo", L_GetAliasInfo};
    worldlib[index++] = {"GetAliasList", L_GetAliasList};
    worldlib[index++] = {"GetAliasOption", L_GetAliasOption};
    worldlib[index++] = {"IsAlias", L_IsAlias};
    worldlib[index++] = {"SetAliasOption", L_SetAliasOption};
    worldlib[index++] = {"GetPluginAliasList", L_GetPluginAliasList};
    worldlib[index++] = {"GetPluginAliasInfo", L_GetPluginAliasInfo};
    worldlib[index++] = {"GetPluginAliasOption", L_GetPluginAliasOption};
}
