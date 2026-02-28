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
 * Creates a new alias that matches user input and performs an action.
 * Aliases can send text, execute scripts, or queue commands.
 *
 * Flag values (combine with bitwise OR):
 * - eEnabled (1): Alias is active
 * - eKeepEvaluating (8): Continue checking other aliases after match
 * - eIgnoreAliasCase (32): Case-insensitive matching
 * - eOmitFromLogFile (64): Don't log alias matches
 * - eAliasRegularExpression (128): Use regex pattern
 * - eExpandVariables (512): Expand @variables in response
 * - eReplace (1024): Replace existing alias with same name
 * - eAliasSpeedWalk (2048): Treat response as speedwalk
 * - eAliasQueue (4096): Queue response instead of sending
 * - eAliasMenu (8192): Show in alias menu
 * - eTemporary (16384): Delete when world closes
 * - eAliasOmitFromOutput (65536): Don't show matched command
 * - eAliasOneShot (32768): Delete after first match
 *
 * @param name (string) Unique alias identifier
 * @param match (string) Pattern to match against user input
 * @param response (string) Text to send or script to execute
 * @param flags (number) Bitwise OR of flag constants
 * @param script (string) Script function name (optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eAliasAlreadyExists: Alias with this name exists (and eReplace not set)
 *   - eAliasCannotBeEmpty: Match pattern is empty
 *   - eInvalidObjectLabel: Invalid alias name
 *
 * @example
 * -- Simple alias to send a command
 * AddAlias("heal", "^heal$", "cast 'cure light'", eEnabled, "")
 *
 * @example
 * -- Regex alias with script callback
 * AddAlias("target", "^t (.+)$", "", eEnabled + eAliasRegularExpression, "OnTarget")
 *
 * @see DeleteAlias, EnableAlias, GetAlias, IsAlias
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
        auto it = currentPlugin->m_AliasMap.find(qName);
        if (it != currentPlugin->m_AliasMap.end()) {
            if (!replace) {
                return luaReturnError(L, eAliasAlreadyExists);
            }
            // Cannot replace an alias whose script is currently executing (use-after-free guard)
            if (it->second->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            // Delete existing alias for replacement
            currentPlugin->m_AliasMap.erase(it);
        }
    } else {
        Alias* existing = pDoc->getAlias(qName);
        if (existing) {
            if (!replace) {
                return luaReturnError(L, eAliasAlreadyExists);
            }
            // Cannot replace an alias whose script is currently executing (use-after-free guard)
            if (existing->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            (void)pDoc->deleteAlias(qName); // safe: already checked executing_script
        }
    }

    // Cannot have empty match
    if (qMatch.isEmpty()) {
        return luaReturnError(L, eAliasCannotBeEmpty);
    }

    // Create alias
    auto alias = std::make_unique<Alias>();
    alias->label = qName;
    alias->internal_name = qName;
    alias->name = qMatch;
    alias->contents = QString::fromUtf8(response);
    alias->enabled = (flags & eEnabled) != 0;
    alias->ignore_case = (flags & eIgnoreAliasCase) != 0;
    alias->omit_from_log = (flags & eOmitFromLogFile) != 0;
    alias->use_regexp = (flags & eAliasRegularExpression) != 0;
    alias->omit_from_output = (flags & eAliasOmitFromOutput) != 0;
    alias->expand_variables = (flags & eExpandVariables) != 0;
    alias->menu = (flags & eAliasMenu) != 0;
    alias->temporary = (flags & eTemporary) != 0;
    alias->one_shot = (flags & eAliasOneShot) != 0;
    // Note: keep_evaluating defaults to true (MUSHclient behavior)
    // Only explicitly set if flag is present
    if (flags & eKeepEvaluating) {
        alias->keep_evaluating = true;
    }
    // else: keep the default from Alias constructor (true)
    alias->procedure = QString::fromUtf8(script);
    alias->sequence = 100; // Default sequence

    // Handle special send-to flags
    // If a script procedure is specified, default to eSendToScript
    if (flags & eAliasSpeedWalk) {
        alias->send_to = eSendToSpeedwalk;
    } else if (flags & eAliasQueue) {
        alias->send_to = eSendToCommandQueue;
    } else if (!alias->procedure.isEmpty()) {
        alias->send_to = eSendToScript;
    } else {
        alias->send_to = eSendToWorld;
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
        if (!pDoc->addAlias(qName, std::move(alias)).has_value()) {
            return luaReturnError(L, eAliasAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteAlias(name)
 *
 * Permanently removes an alias from the world. The alias will no longer
 * match user input after deletion.
 *
 * @param name (string) Name of the alias to delete
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eAliasNotFound: No alias with this name exists
 *
 * @example
 * -- Remove an alias when no longer needed
 * DeleteAlias("target")
 *
 * @see AddAlias, DeleteAliasGroup, DeleteTemporaryAliases, IsAlias
 */
int L_DeleteAlias(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    if (!pDoc->deleteAlias(qName).has_value()) {
        return luaReturnError(L, eAliasNotFound);
    }

    return luaReturnOK(L);
}

/**
 * world.IsAlias(name)
 *
 * Checks whether an alias with the given name exists in the current world.
 *
 * @param name (string) Name of the alias to check
 *
 * @return (number) Error code:
 *   - eOK (0): Alias exists
 *   - eAliasNotFound: No alias with this name
 *
 * @example
 * if IsAlias("heal") == eOK then
 *     Note("Heal alias is defined")
 * else
 *     Note("Heal alias not found")
 * end
 *
 * @see AddAlias, GetAlias, GetAliasList
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
 * Retrieves complete details about an alias including its pattern,
 * response text, flags, and script name. Returns multiple values.
 *
 * @param name (string) Name of the alias to retrieve
 *
 * @return (number, string, string, number, string) Multiple values:
 *   1. Error code (eOK on success, eAliasNotFound if not found)
 *   2. Match pattern (the regex or text pattern)
 *   3. Response text (what gets sent when matched)
 *   4. Flags (bitwise OR of alias flags)
 *   5. Script name (function to call, empty if none)
 *
 * @example
 * local code, match, response, flags, script = GetAlias("heal")
 * if code == eOK then
 *     Note("Pattern: " .. match)
 *     Note("Response: " .. response)
 * end
 *
 * @see AddAlias, GetAliasInfo, GetAliasOption, IsAlias
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
    if (alias->enabled)
        flags |= eEnabled;
    if (alias->ignore_case)
        flags |= eIgnoreAliasCase;
    if (alias->omit_from_log)
        flags |= eOmitFromLogFile;
    if (alias->use_regexp)
        flags |= eAliasRegularExpression;
    if (alias->expand_variables)
        flags |= eExpandVariables;
    if (alias->omit_from_output)
        flags |= eAliasOmitFromOutput;
    if (alias->one_shot)
        flags |= eAliasOneShot;
    if (alias->keep_evaluating)
        flags |= eKeepEvaluating;

    // Return: error_code, match, response, flags, script
    lua_pushnumber(L, eOK);
    lua_pushstring(L, alias->name.toUtf8().constData());
    lua_pushstring(L, alias->contents.toUtf8().constData());
    lua_pushnumber(L, flags);
    lua_pushstring(L, alias->procedure.toUtf8().constData());

    return 5;
}

/**
 * world.EnableAlias(name, enabled)
 *
 * Enables or disables an alias without deleting it. Disabled aliases
 * remain in memory but won't match user input until re-enabled.
 *
 * @param name (string) Name of the alias to enable/disable
 * @param enabled (boolean) True to enable, false to disable
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eAliasNotFound: No alias with this name exists
 *
 * @example
 * -- Disable combat aliases when not fighting
 * EnableAlias("attack", false)
 *
 * @example
 * -- Re-enable when combat starts
 * EnableAlias("attack", true)
 *
 * @see AddAlias, EnableAliasGroup, GetAliasInfo
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

    alias->enabled = enabled;
    return luaReturnOK(L);
}

/**
 * world.GetAliasInfo(name, info_type)
 *
 * Gets specific information about an alias using numeric info type codes.
 * MUSHclient-compatible function for querying individual alias properties.
 *
 * Info types:
 * - 1: Match pattern (string)
 * - 2: Response/send text (string)
 * - 3: Script procedure name (string)
 * - 4: Omit from log (boolean)
 * - 5: Omit from output (boolean)
 * - 6: Enabled (boolean)
 * - 7: Is regexp (boolean)
 * - 8: Ignore case (boolean)
 * - 9: Expand variables (boolean)
 * - 10: Invocation count (number)
 * - 11: Times matched (number)
 * - 12: Is menu item (boolean)
 * - 13: When last matched (Unix timestamp)
 * - 14: Is temporary (boolean)
 * - 15: Is included (boolean)
 * - 16: Group name (string)
 * - 17: Variable name (string)
 * - 18: Send-to destination (number)
 * - 19: Keep evaluating (boolean)
 * - 20: Sequence number (number)
 * - 21: Echo alias (boolean)
 * - 22: Omit from command history (boolean)
 * - 23: User option (number)
 * - 24: Regexp match count (number)
 * - 25: Last matching string (string)
 * - 26: Currently executing script (boolean)
 * - 27: Has script (boolean)
 * - 28: Regexp error code (number)
 * - 29: One-shot (boolean)
 * - 30: Regexp execution time (number)
 * - 31: Regexp match attempts (number)
 * - 101-109: Wildcards 1-9 (string)
 * - 110: Wildcard 0 / entire match (string)
 *
 * @param name (string) Name of the alias
 * @param info_type (number) Type of information to retrieve (1-31, 101-110)
 *
 * @return (varies) Requested info, or nil if alias not found
 *
 * @example
 * -- Check if alias is enabled
 * local enabled = GetAliasInfo("heal", 6)
 * if enabled then
 *     Note("Heal alias is enabled")
 * end
 *
 * @example
 * -- Get wildcard from last match
 * local wildcard1 = GetAliasInfo("target", 101)
 *
 * @see GetAlias, GetAliasOption, SetAliasOption
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
        case 3: // procedure (script name)
        {
            QByteArray ba = alias->procedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // omit_from_log
            lua_pushboolean(L, alias->omit_from_log);
            break;
        case 5: // omit_from_output
            lua_pushboolean(L, alias->omit_from_output);
            break;
        case 6: // enabled
            lua_pushboolean(L, alias->enabled);
            break;
        case 7: // use_regexp
            lua_pushboolean(L, alias->use_regexp);
            break;
        case 8: // ignore_case
            lua_pushboolean(L, alias->ignore_case);
            break;
        case 9: // expand_variables
            lua_pushboolean(L, alias->expand_variables);
            break;
        case 10: // invocation_count
            lua_pushnumber(L, alias->invocation_count);
            break;
        case 11: // matched
            lua_pushnumber(L, alias->matched);
            break;
        case 12: // menu
            lua_pushboolean(L, alias->menu);
            break;
        case 13: // when_matched
            if (alias->when_matched.isValid()) {
                // Return as Unix timestamp (seconds since epoch)
                lua_pushnumber(L, alias->when_matched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L); // Return nil if never matched
            }
            break;
        case 14: // temporary
            lua_pushboolean(L, alias->temporary);
            break;
        case 15: // included
            lua_pushboolean(L, alias->included);
            break;
        case 16: // group
        {
            QByteArray ba = alias->group.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // variable
        {
            QByteArray ba = alias->variable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 18: // send_to
            lua_pushnumber(L, static_cast<int>(alias->send_to));
            break;
        case 19: // keep_evaluating
            lua_pushboolean(L, alias->keep_evaluating);
            break;
        case 20: // sequence
            lua_pushnumber(L, alias->sequence);
            break;
        case 21: // echo_alias
            lua_pushboolean(L, alias->echo_alias);
            break;
        case 22: // omit_from_command_history
            lua_pushboolean(L, alias->omit_from_command_history);
            break;
        case 23: // user_option
            lua_pushnumber(L, alias->user_option);
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
        case 26: // executing_script
            lua_pushboolean(L, alias->executing_script);
            break;
        case 27: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, alias->dispid != -1);
            break;
        case 28: // regexp execution error
            // We don't track regexp errors in Qt (always 0)
            lua_pushnumber(L, 0);
            break;
        case 29: // one_shot
            lua_pushboolean(L, alias->one_shot);
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
 * Returns an array of all alias names defined in the current world.
 * Useful for iterating over all aliases or checking what aliases exist.
 *
 * @return (table) Array of alias name strings, or empty table if no aliases
 *
 * @example
 * local aliases = GetAliasList()
 * Note("Found " .. #aliases .. " aliases:")
 * for i, name in ipairs(aliases) do
 *     Note("  " .. name)
 * end
 *
 * @see GetAlias, GetAliasInfo, IsAlias, GetPluginAliasList
 */
int L_GetAliasList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);
    int i = 1;
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        QByteArray ba = name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/**
 * world.GetPluginAliasList(pluginID)
 *
 * Returns an array of all alias names defined in a specific plugin.
 * Allows inspection of aliases from other plugins.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 *
 * @return (table) Array of alias name strings, or empty table if plugin not found
 *
 * @example
 * local aliases = GetPluginAliasList("abc12345-1234-1234-1234-123456789012")
 * for i, name in ipairs(aliases) do
 *     Note("Plugin alias: " .. name)
 * end
 *
 * @see GetAliasList, GetPluginAliasInfo, GetPluginAliasOption, GetPluginList
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
 * world.GetPluginAliasInfo(pluginID, aliasName, infoType)
 *
 * Gets specific information about an alias in another plugin.
 * Uses the same info type codes as GetAliasInfo.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 * @param aliasName (string) Name of the alias in the plugin
 * @param infoType (number) Type of information to retrieve (see GetAliasInfo)
 *
 * @return (varies) Requested info, or nil if plugin/alias not found
 *
 * @example
 * -- Check if a plugin's alias is enabled
 * local enabled = GetPluginAliasInfo(pluginID, "combat", 6)
 *
 * @see GetAliasInfo, GetPluginAliasList, GetPluginAliasOption
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
        case 3: // procedure (script name)
        {
            QByteArray ba = alias->procedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // omit_from_log
            lua_pushboolean(L, alias->omit_from_log);
            break;
        case 5: // omit_from_output
            lua_pushboolean(L, alias->omit_from_output);
            break;
        case 6: // enabled
            lua_pushboolean(L, alias->enabled);
            break;
        case 7: // use_regexp
            lua_pushboolean(L, alias->use_regexp);
            break;
        case 8: // ignore_case
            lua_pushboolean(L, alias->ignore_case);
            break;
        case 9: // expand_variables
            lua_pushboolean(L, alias->expand_variables);
            break;
        case 10: // invocation_count
            lua_pushnumber(L, alias->invocation_count);
            break;
        case 11: // matched
            lua_pushnumber(L, alias->matched);
            break;
        case 12: // menu
            lua_pushboolean(L, alias->menu);
            break;
        case 13: // when_matched
            if (alias->when_matched.isValid()) {
                lua_pushnumber(L, alias->when_matched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 14: // temporary
            lua_pushboolean(L, alias->temporary);
            break;
        case 15: // included
            lua_pushboolean(L, alias->included);
            break;
        case 16: // group
        {
            QByteArray ba = alias->group.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // variable
        {
            QByteArray ba = alias->variable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 18: // send_to
            lua_pushnumber(L, static_cast<int>(alias->send_to));
            break;
        case 19: // keep_evaluating
            lua_pushboolean(L, alias->keep_evaluating);
            break;
        case 20: // sequence
            lua_pushnumber(L, alias->sequence);
            break;
        case 21: // echo_alias
            lua_pushboolean(L, alias->echo_alias);
            break;
        case 22: // omit_from_command_history
            lua_pushboolean(L, alias->omit_from_command_history);
            break;
        case 23: // user_option
            lua_pushnumber(L, alias->user_option);
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
        case 26: // executing_script
            lua_pushboolean(L, alias->executing_script);
            break;
        case 27: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, alias->dispid != -1);
            break;
        case 28: // regexp execution error
            lua_pushnumber(L, 0);
            break;
        case 29: // one_shot
            lua_pushboolean(L, alias->one_shot);
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
 * world.GetPluginAliasOption(pluginID, aliasName, optionName)
 *
 * Gets an option value for an alias in another plugin.
 * Uses the same option names as GetAliasOption.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 * @param aliasName (string) Name of the alias in the plugin
 * @param optionName (string) Option name (see GetAliasOption for valid names)
 *
 * @return (varies) Option value, or nil if plugin/alias not found
 *
 * @example
 * local seq = GetPluginAliasOption(pluginID, "heal", "sequence")
 *
 * @see GetAliasOption, GetPluginAliasList, GetPluginAliasInfo
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
            lua_pushboolean(L, alias->enabled);
        } else if (option == "keep_evaluating") {
            lua_pushboolean(L, alias->keep_evaluating);
        } else if (option == "sequence") {
            lua_pushnumber(L, alias->sequence);
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
 * Gets an option value for an alias using named option strings.
 * More readable alternative to GetAliasInfo's numeric codes.
 *
 * Numeric options: send_to, sequence, user
 * Boolean options: enabled, expand_variables, ignore_case, omit_from_log,
 *   omit_from_command_history, omit_from_output, regexp, menu,
 *   keep_evaluating, echo_alias, temporary, one_shot
 * String options: group, match, script, send, variable
 *
 * @param alias_name (string) Name of the alias
 * @param option_name (string) Name of the option (case-insensitive)
 *
 * @return (varies) Option value, or nil if alias/option not found
 *
 * @example
 * -- Get the sequence number of an alias
 * local seq = GetAliasOption("heal", "sequence")
 *
 * @example
 * -- Check if alias uses regexp
 * if GetAliasOption("target", "regexp") then
 *     Note("Target uses regular expressions")
 * end
 *
 * @see SetAliasOption, GetAliasInfo, GetAlias
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
        lua_pushnumber(L, static_cast<int>(alias->send_to));
    } else if (qOption == "sequence") {
        lua_pushnumber(L, alias->sequence);
    } else if (qOption == "user") {
        lua_pushnumber(L, alias->user_option);
    }
    // Boolean options
    else if (qOption == "enabled") {
        lua_pushboolean(L, alias->enabled);
    } else if (qOption == "expand_variables") {
        lua_pushboolean(L, alias->expand_variables);
    } else if (qOption == "ignore_case") {
        lua_pushboolean(L, alias->ignore_case);
    } else if (qOption == "omit_from_log") {
        lua_pushboolean(L, alias->omit_from_log);
    } else if (qOption == "omit_from_command_history") {
        lua_pushboolean(L, alias->omit_from_command_history);
    } else if (qOption == "omit_from_output") {
        lua_pushboolean(L, alias->omit_from_output);
    } else if (qOption == "regexp") {
        lua_pushboolean(L, alias->use_regexp);
    } else if (qOption == "menu") {
        lua_pushboolean(L, alias->menu);
    } else if (qOption == "keep_evaluating") {
        lua_pushboolean(L, alias->keep_evaluating);
    } else if (qOption == "echo_alias") {
        lua_pushboolean(L, alias->echo_alias);
    } else if (qOption == "temporary") {
        lua_pushboolean(L, alias->temporary);
    } else if (qOption == "one_shot") {
        lua_pushboolean(L, alias->one_shot);
    }
    // String options
    else if (qOption == "group") {
        QByteArray ba = alias->group.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "match") {
        QByteArray ba = alias->name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "script") {
        QByteArray ba = alias->procedure.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "send") {
        QByteArray ba = alias->contents.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "variable") {
        QByteArray ba = alias->variable.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/**
 * world.SetAliasOption(alias_name, option_name, value)
 *
 * Sets an option value for an alias using named option strings.
 * Changes take effect immediately for subsequent alias matches.
 *
 * Numeric options: send_to, sequence, user
 * Boolean options: enabled, expand_variables, ignore_case, omit_from_log,
 *   omit_from_command_history, omit_from_output, menu, keep_evaluating,
 *   echo_alias, temporary, one_shot
 * String options: group, match, script, send, variable
 *
 * Note: The "regexp" option cannot be changed after creation.
 *
 * @param alias_name (string) Name of the alias
 * @param option_name (string) Name of the option (case-insensitive)
 * @param value (varies) New value for the option
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eAliasNotFound: No alias with this name
 *   - eAliasCannotBeEmpty: Tried to set empty match pattern
 *   - eUnknownOption: Invalid option name
 *   - ePluginCannotSetOption: Option cannot be modified (regexp)
 *
 * @example
 * -- Change alias sequence for priority
 * SetAliasOption("heal", "sequence", 50)
 *
 * @example
 * -- Update the response text
 * SetAliasOption("heal", "send", "cast 'cure critical'")
 *
 * @see GetAliasOption, EnableAlias, AddAlias
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
            alias->send_to = static_cast<SendTo>(value);
        } else if (qOption == "sequence") {
            alias->sequence = value;
            // TODO: Re-sort aliases
        } else if (qOption == "user") {
            alias->user_option = value;
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
            alias->enabled = value;
        } else if (qOption == "expand_variables") {
            alias->expand_variables = value;
        } else if (qOption == "ignore_case") {
            alias->ignore_case = value;
            (void)alias->compileRegexp(); // Recompile with new case sensitivity
        } else if (qOption == "omit_from_log") {
            alias->omit_from_log = value;
        } else if (qOption == "omit_from_command_history") {
            alias->omit_from_command_history = value;
        } else if (qOption == "omit_from_output") {
            alias->omit_from_output = value;
        } else if (qOption == "regexp") {
            return luaReturnError(L, ePluginCannotSetOption); // Cannot write
        } else if (qOption == "menu") {
            alias->menu = value;
        } else if (qOption == "keep_evaluating") {
            alias->keep_evaluating = value;
        } else if (qOption == "echo_alias") {
            alias->echo_alias = value;
        } else if (qOption == "temporary") {
            alias->temporary = value;
        } else if (qOption == "one_shot") {
            alias->one_shot = value;
        }
    }
    // String options
    else if (qOption == "group" || qOption == "match" || qOption == "script" || qOption == "send" ||
             qOption == "variable") {
        const char* value = luaL_checkstring(L, 3);
        QString qValue = QString::fromUtf8(value);

        if (qOption == "group") {
            alias->group = qValue;
        } else if (qOption == "match") {
            if (qValue.isEmpty()) {
                return luaReturnError(L, eAliasCannotBeEmpty);
            }
            alias->name = qValue;
            (void)alias->compileRegexp(); // Recompile with new pattern
        } else if (qOption == "script") {
            alias->procedure = qValue;
            // TODO: Update dispid
        } else if (qOption == "send") {
            alias->contents = qValue;
        } else if (qOption == "variable") {
            alias->variable = qValue;
        }
    } else {
        return luaReturnError(L, eUnknownOption);
    }

    return luaReturnOK(L);
}

/**
 * world.EnableAliasGroup(groupName, enabled)
 *
 * Enables or disables all aliases that belong to a named group.
 * Groups provide a way to organize related aliases and control them together.
 *
 * @param groupName (string) Name of the alias group
 * @param enabled (boolean) True to enable all, false to disable all
 *
 * @return (number) Count of aliases affected
 *
 * @example
 * -- Disable all combat aliases
 * local count = EnableAliasGroup("combat", false)
 * Note("Disabled " .. count .. " combat aliases")
 *
 * @example
 * -- Enable all aliases in the "movement" group
 * EnableAliasGroup("movement", true)
 *
 * @see EnableAlias, DeleteAliasGroup, SetAliasOption
 */
int L_EnableAliasGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qGroupName = QString::fromUtf8(groupName);
    int count = 0;

    // Iterate through all aliases
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        Alias* alias = aliasPtr.get();
        if (alias->group == qGroupName) {
            alias->enabled = enabled;
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.DeleteAliasGroup(groupName)
 *
 * Permanently deletes all aliases that belong to a named group.
 * Useful for cleaning up related aliases together.
 *
 * @param groupName (string) Name of the alias group
 *
 * @return (number) Count of aliases deleted
 *
 * @example
 * -- Remove all combat aliases
 * local count = DeleteAliasGroup("combat")
 * Note("Deleted " .. count .. " combat aliases")
 *
 * @see DeleteAlias, DeleteTemporaryAliases, EnableAliasGroup
 */
int L_DeleteAliasGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);

    QString qGroupName = QString::fromUtf8(groupName);
    QStringList toDelete;

    // Find all aliases in this group
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        if (aliasPtr->group == qGroupName) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        (void)pDoc->deleteAlias(name); // intentional: bulk group/temporary delete
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.DeleteTemporaryAliases()
 *
 * Deletes all aliases that were created with the eTemporary flag.
 * Temporary aliases are normally deleted when the world closes, but
 * this allows manual cleanup at any time.
 *
 * @return (number) Count of aliases deleted
 *
 * @example
 * -- Clean up temporary aliases
 * local count = DeleteTemporaryAliases()
 * Note("Removed " .. count .. " temporary aliases")
 *
 * @see DeleteAlias, DeleteAliasGroup, AddAlias
 */
int L_DeleteTemporaryAliases(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList toDelete;

    // Find all temporary aliases
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        if (aliasPtr->temporary) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        (void)pDoc->deleteAlias(name); // intentional: bulk group/temporary delete
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
