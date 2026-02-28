/**
 * world_triggers.cpp - Trigger API Functions
 *
 * Extracted from lua_methods.cpp
 * Implements trigger management functions for the Lua API.
 */

#include "../../automation/plugin.h"
#include "../../automation/trigger.h"
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
 * world.AddTrigger(name, match, response, flags, color, wildcard, sound_file, script, send_to,
 * sequence)
 *
 * Creates a new trigger that matches incoming MUD output and performs an action.
 * Triggers can change text colors, send commands, play sounds, or execute scripts.
 *
 * Flag values (combine with bitwise OR):
 * - eEnabled (1): Trigger is active
 * - eKeepEvaluating (8): Continue checking other triggers after match
 * - eIgnoreAliasCase (32): Case-insensitive matching
 * - eOmitFromLogFile (64): Don't log matched lines
 * - eAliasRegularExpression (128): Use regex pattern
 * - eExpandVariables (512): Expand @variables in response
 * - eReplace (1024): Replace existing trigger with same name
 * - eTemporary (16384): Delete when world closes
 * - eAliasOmitFromOutput (65536): Don't display matched line
 * - eAliasOneShot (32768): Delete after first match
 *
 * @param name (string) Unique trigger identifier
 * @param match (string) Pattern to match against MUD output
 * @param response (string) Text to send when triggered
 * @param flags (number) Bitwise OR of flag constants
 * @param color (number) Custom color index (optional, default 0)
 * @param wildcard (number) Unused parameter for compatibility
 * @param sound_file (string) Sound file path to play (optional)
 * @param script (string) Script function name (optional)
 * @param send_to (number) Send destination 0-14 (optional, default 0)
 * @param sequence (number) Evaluation order 0-10000 (optional, default 100)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eTriggerAlreadyExists: Trigger with this name exists
 *   - eTriggerCannotBeEmpty: Match pattern is empty
 *   - eTriggerSequenceOutOfRange: Sequence not 0-10000
 *   - eTriggerSendToInvalid: Invalid send_to value
 *
 * @example
 * -- Simple trigger to highlight health warnings
 * AddTrigger("low_health", "You are bleeding", "", eEnabled, 0, 0, "", "", 0, 100)
 *
 * @example
 * -- Regex trigger with script callback
 * AddTrigger("mob_enters", "^(\\w+) arrives from", "", eEnabled + eAliasRegularExpression, 0, 0,
 * "", "OnMobEnters", 0, 100)
 *
 * @see AddTriggerEx, DeleteTrigger, EnableTrigger, GetTrigger
 */
int L_AddTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* match = luaL_checkstring(L, 2);
    const char* response = luaL_checkstring(L, 3);
    int flags = luaL_checkinteger(L, 4);
    int color = luaL_optinteger(L, 5, 0);
    // int wildcard = luaL_optinteger(L, 6, 0);  // Unused
    const char* sound_file = luaL_optstring(L, 7, "");
    const char* script = luaL_optstring(L, 8, "");
    int send_to = luaL_optinteger(L, 9, 0);
    int sequence = luaL_optinteger(L, 10, 100);

    QString qName = QString::fromUtf8(name);
    QString qMatch = QString::fromUtf8(match);

    // Validate and normalize trigger name
    qint32 nameStatus = validateObjectName(qName);
    if (nameStatus != eOK) {
        lua_pushnumber(L, nameStatus);
        return 1;
    }

    // Check if trigger already exists (check appropriate map based on context)
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* currentPlugin = plugin(L);
    bool replace = (flags & eReplace) != 0;
    if (currentPlugin) {
        auto it = currentPlugin->m_TriggerMap.find(qName);
        if (it != currentPlugin->m_TriggerMap.end()) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Cannot replace a trigger whose script is currently executing (use-after-free guard)
            if (it->second->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            // Delete existing trigger for replacement
            currentPlugin->m_TriggerMap.erase(it);
        }
    } else {
        Trigger* existing = pDoc->getTrigger(qName);
        if (existing) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Cannot replace a trigger whose script is currently executing (use-after-free guard)
            if (existing->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            (void)pDoc->deleteTrigger(qName); // safe: already checked executing_script
        }
    }

    // Cannot have empty match
    if (qMatch.isEmpty()) {
        return luaReturnError(L, eTriggerCannotBeEmpty);
    }

    // Validate sequence range (0-10000)
    if (sequence < 0 || sequence > 10000) {
        return luaReturnError(L, eTriggerSequenceOutOfRange);
    }

    // Validate send_to range (0 to eSendToLast-1)
    if (send_to < 0 || send_to >= eSendToLast) {
        return luaReturnError(L, eTriggerSendToInvalid);
    }

    // Create trigger
    auto trigger = std::make_unique<Trigger>();
    trigger->label = qName;
    trigger->internal_name = qName;
    trigger->trigger = qMatch;
    trigger->contents = QString::fromUtf8(response);
    trigger->enabled = (flags & eEnabled) != 0;
    trigger->omit_from_output = (flags & eAliasOmitFromOutput) != 0;
    trigger->omit_from_log = (flags & eOmitFromLogFile) != 0;
    // Note: keep_evaluating defaults to true (MUSHclient behavior)
    // The eKeepEvaluating flag is only used to explicitly request keeping evaluation
    // When not set, we keep the default (true) rather than forcing false
    if (flags & eKeepEvaluating) {
        trigger->keep_evaluating = true;
    }
    // else: keep the default from Trigger constructor (true)
    trigger->use_regexp = (flags & eAliasRegularExpression) != 0;
    trigger->ignore_case = (flags & eIgnoreAliasCase) != 0;
    trigger->expand_variables = (flags & eExpandVariables) != 0;
    trigger->temporary = (flags & eTemporary) != 0;
    trigger->one_shot = (flags & eAliasOneShot) != 0;
    trigger->colour = color;
    trigger->sound_to_play = QString::fromUtf8(sound_file);
    trigger->procedure = QString::fromUtf8(script);
    trigger->send_to = static_cast<SendTo>(send_to);
    trigger->sequence = sequence;

    // Add to appropriate trigger map (plugin or world)
    if (currentPlugin) {
        trigger->owningPlugin = currentPlugin;
        currentPlugin->m_TriggerMap[qName] = std::move(trigger);
        // Rebuild trigger array for matching
        currentPlugin->m_TriggerArray.clear();
        for (const auto& [name, t] : currentPlugin->m_TriggerMap) {
            currentPlugin->m_TriggerArray.push_back(t.get());
        }
    } else {
        if (!pDoc->addTrigger(qName, std::move(trigger)).has_value()) {
            return luaReturnError(L, eTriggerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteTrigger(name)
 *
 * Permanently removes a trigger from the world. The trigger will no longer
 * match incoming text after deletion.
 *
 * @param name (string) Name of the trigger to delete
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eTriggerNotFound: No trigger with this name exists
 *
 * @example
 * -- Remove a trigger when no longer needed
 * DeleteTrigger("low_health")
 *
 * @see AddTrigger, DeleteTriggerGroup, DeleteTemporaryTriggers, IsTrigger
 */
int L_DeleteTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    if (!pDoc->deleteTrigger(qName).has_value()) {
        return luaReturnError(L, eTriggerNotFound);
    }

    return luaReturnOK(L);
}

/**
 * world.IsTrigger(name)
 *
 * Checks whether a trigger with the given name exists in the current world.
 *
 * @param name (string) Name of the trigger to check
 *
 * @return (number) Error code:
 *   - eOK (0): Trigger exists
 *   - eTriggerNotFound: No trigger with this name
 *
 * @example
 * if IsTrigger("combat") == eOK then
 *     Note("Combat trigger is defined")
 * else
 *     Note("Combat trigger not found")
 * end
 *
 * @see AddTrigger, GetTrigger, GetTriggerList
 */
int L_IsTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Trigger* trigger = pDoc->getTrigger(qName);

    lua_pushnumber(L, trigger ? eOK : eTriggerNotFound);
    return 1;
}

/**
 * world.GetTrigger(name)
 *
 * Retrieves complete details about a trigger including its pattern,
 * response text, flags, colors, and script. Returns multiple values.
 *
 * @param name (string) Name of the trigger to retrieve
 *
 * @return (number, string, string, number, number, number, string, string) Multiple values:
 *   1. Error code (eOK on success, eTriggerNotFound if not found)
 *   2. Match pattern (the regex or text pattern)
 *   3. Response text (what gets sent when matched)
 *   4. Flags (bitwise OR of trigger flags)
 *   5. Colour index (-1 for no change)
 *   6. Clipboard wildcard number
 *   7. Sound file path
 *   8. Script function name
 *
 * @example
 * local code, match, response, flags, color, wc, sound, script = GetTrigger("combat")
 * if code == eOK then
 *     Note("Pattern: " .. match)
 *     Note("Response: " .. response)
 * end
 *
 * @see AddTrigger, GetTriggerInfo, GetTriggerOption, IsTrigger
 */
int L_GetTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Trigger* trigger = pDoc->getTrigger(qName);

    if (!trigger) {
        return luaReturnError(L, eTriggerNotFound);
    }

    // Build flags
    int flags = 0;
    if (trigger->ignore_case)
        flags |= eIgnoreCase;
    if (trigger->omit_from_output)
        flags |= eOmitFromOutput;
    if (trigger->keep_evaluating)
        flags |= eKeepEvaluating;
    if (trigger->omit_from_log)
        flags |= eOmitFromLog;
    if (trigger->enabled)
        flags |= eEnabled;
    if (trigger->use_regexp)
        flags |= eTriggerRegularExpression;
    if (trigger->lowercase_wildcard)
        flags |= eLowercaseWildcard;
    if (trigger->one_shot)
        flags |= eTriggerOneShot;

    // Return: error_code, match, response, flags, colour, wildcard, sound, script
    lua_pushnumber(L, eOK);
    lua_pushstring(L, trigger->trigger.toUtf8().constData());
    lua_pushstring(L, trigger->contents.toUtf8().constData());
    lua_pushnumber(L, flags);
    lua_pushnumber(L, trigger->colour);
    lua_pushnumber(L, trigger->clipboard_arg);
    lua_pushstring(L, trigger->sound_to_play.toUtf8().constData());
    lua_pushstring(L, trigger->procedure.toUtf8().constData());

    return 8;
}

/**
 * world.EnableTrigger(name, enabled)
 *
 * Enables or disables a trigger without deleting it. Disabled triggers
 * remain in memory but won't match incoming text until re-enabled.
 *
 * @param name (string) Name of the trigger to enable/disable
 * @param enabled (boolean) True to enable, false to disable
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eTriggerNotFound: No trigger with this name exists
 *
 * @example
 * -- Disable combat triggers when not fighting
 * EnableTrigger("auto_attack", false)
 *
 * @example
 * -- Re-enable when combat starts
 * EnableTrigger("auto_attack", true)
 *
 * @see AddTrigger, EnableTriggerGroup, GetTriggerInfo
 */
int L_EnableTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qName = QString::fromUtf8(name);
    Trigger* trigger = pDoc->getTrigger(qName);

    if (!trigger) {
        return luaReturnError(L, eTriggerNotFound);
    }

    trigger->enabled = enabled;
    return luaReturnOK(L);
}

/**
 * world.GetTriggerInfo(name, info_type)
 *
 * Gets specific information about a trigger using numeric info type codes.
 * MUSHclient-compatible function for querying individual trigger properties.
 *
 * Info types:
 * - 1: Trigger pattern (string)
 * - 2: Response/send text (string)
 * - 3: Sound file path (string)
 * - 4: Script procedure name (string)
 * - 5: Omit from log (boolean)
 * - 6: Omit from output (boolean)
 * - 7: Keep evaluating (boolean)
 * - 8: Enabled (boolean)
 * - 9: Is regexp (boolean)
 * - 10: Ignore case (boolean)
 * - 11: Repeat on same line (boolean)
 * - 12: Play sound if inactive (boolean)
 * - 13: Expand variables (boolean)
 * - 14: Clipboard wildcard argument (number)
 * - 15: Send-to destination (number)
 * - 16: Sequence number (number)
 * - 17: Match style (number)
 * - 18: New style (number)
 * - 19: Colour index (number)
 * - 20: Invocation count (number)
 * - 21: Times matched (number)
 * - 22: When last matched (Unix timestamp)
 * - 23: Is temporary (boolean)
 * - 24: Is included (boolean)
 * - 25: Lowercase wildcard (boolean)
 * - 26: Group name (string)
 * - 27: Variable name (string)
 * - 28: User option (number)
 * - 29: Other foreground color (number)
 * - 30: Other background color (number)
 * - 31: Regexp match count (number)
 * - 32: Last matching string (string)
 * - 33: Currently executing script (boolean)
 * - 34: Has script (boolean)
 * - 35: Regexp error code (number)
 * - 36: One-shot (boolean)
 * - 37: Regexp execution time (number)
 * - 38: Regexp match attempts (number)
 * - 101-109: Wildcards 1-9 (string)
 * - 110: Wildcard 0 / entire match (string)
 *
 * @param name (string) Name of the trigger
 * @param info_type (number) Type of information to retrieve (1-38, 101-110)
 *
 * @return (varies) Requested info, or nil if trigger not found
 *
 * @example
 * -- Check if trigger is enabled
 * local enabled = GetTriggerInfo("combat", 8)
 * if enabled then
 *     Note("Combat trigger is enabled")
 * end
 *
 * @example
 * -- Get wildcard from last match
 * local target = GetTriggerInfo("mob_enters", 101)
 *
 * @see GetTrigger, GetTriggerOption, SetTriggerOption
 */
int L_GetTriggerInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    int info_type = luaL_checkinteger(L, 2);

    QString qName = QString::fromUtf8(name);
    Trigger* trigger = pDoc->getTrigger(qName);

    if (!trigger) {
        lua_pushnil(L);
        return 1;
    }

    switch (info_type) {
        case 1: // trigger pattern
        {
            QByteArray ba = trigger->trigger.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 2: // contents (send text)
        {
            QByteArray ba = trigger->contents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 3: // sound_to_play
        {
            QByteArray ba = trigger->sound_to_play.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // procedure (script name)
        {
            QByteArray ba = trigger->procedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // omit_from_log
            lua_pushboolean(L, trigger->omit_from_log);
            break;
        case 6: // omit_from_output
            lua_pushboolean(L, trigger->omit_from_output);
            break;
        case 7: // keep_evaluating
            lua_pushboolean(L, trigger->keep_evaluating);
            break;
        case 8: // enabled
            lua_pushboolean(L, trigger->enabled);
            break;
        case 9: // use_regexp
            lua_pushboolean(L, trigger->use_regexp);
            break;
        case 10: // ignore_case
            lua_pushboolean(L, trigger->ignore_case);
            break;
        case 11: // repeat
            lua_pushboolean(L, trigger->repeat);
            break;
        case 12: // sound_if_inactive
            lua_pushboolean(L, trigger->sound_if_inactive);
            break;
        case 13: // expand_variables
            lua_pushboolean(L, trigger->expand_variables);
            break;
        case 14: // clipboard_arg
            lua_pushnumber(L, trigger->clipboard_arg);
            break;
        case 15: // send_to
            lua_pushnumber(L, static_cast<int>(trigger->send_to));
            break;
        case 16: // sequence
            lua_pushnumber(L, trigger->sequence);
            break;
        case 17: // match_type
            lua_pushnumber(L, trigger->match_type);
            break;
        case 18: // style
            lua_pushnumber(L, trigger->style);
            break;
        case 19: // colour
            lua_pushnumber(L, trigger->colour);
            break;
        case 20: // invocation_count
            lua_pushnumber(L, trigger->invocation_count);
            break;
        case 21: // matched
            lua_pushnumber(L, trigger->matched);
            break;
        case 22: // when_matched
            if (trigger->when_matched.isValid()) {
                // Return as Unix timestamp (seconds since epoch)
                lua_pushnumber(L, trigger->when_matched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L); // Return nil if never matched
            }
            break;
        case 23: // temporary
            lua_pushboolean(L, trigger->temporary);
            break;
        case 24: // included
            lua_pushboolean(L, trigger->included);
            break;
        case 25: // lowercase_wildcard
            lua_pushboolean(L, trigger->lowercase_wildcard);
            break;
        case 26: // group
        {
            QByteArray ba = trigger->group.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 27: // variable
        {
            QByteArray ba = trigger->variable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 28: // user_option
            lua_pushnumber(L, trigger->user_option);
            break;
        case 29: // other_foreground
            lua_pushnumber(L, trigger->other_foreground);
            break;
        case 30: // other_background
            lua_pushnumber(L, trigger->other_background);
            break;
        case 31: // regexp match count
            if (trigger->regexp) {
                // Count of wildcard matches (how many wildcards were captured)
                lua_pushnumber(L, trigger->wildcards.size());
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 32: // last matching string
            if (!trigger->wildcards.isEmpty()) {
                // wildcards[0] is the entire match
                QByteArray ba = trigger->wildcards[0].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 33: // executing_script
            lua_pushboolean(L, trigger->executing_script);
            break;
        case 34: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, trigger->dispid != -1);
            break;
        case 35: // regexp execution error
            // We don't track regexp errors in Qt (always 0)
            lua_pushnumber(L, 0);
            break;
        case 36: // one_shot
            lua_pushboolean(L, trigger->one_shot);
            break;
        case 37: // regexp execution time
            // We don't track execution time (always 0)
            lua_pushnumber(L, 0);
            break;
        case 38: // regexp match attempts
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
            if (wildcard_index < trigger->wildcards.size()) {
                QByteArray ba = trigger->wildcards[wildcard_index].toUtf8();
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
 * world.GetTriggerList()
 *
 * Returns an array of all trigger names defined in the current world.
 * Useful for iterating over all triggers or checking what triggers exist.
 *
 * @return (table) Array of trigger name strings, or empty table if no triggers
 *
 * @example
 * local triggers = GetTriggerList()
 * Note("Found " .. #triggers .. " triggers:")
 * for i, name in ipairs(triggers) do
 *     Note("  " .. name)
 * end
 *
 * @see GetTrigger, GetTriggerInfo, IsTrigger, GetPluginTriggerList
 */
int L_GetTriggerList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);
    int i = 1;
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        QByteArray ba = name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/**
 * world.GetPluginTriggerList(pluginID)
 *
 * Returns an array of all trigger names defined in a specific plugin.
 * Allows inspection of triggers from other plugins.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 *
 * @return (table) Array of trigger name strings, or empty table if plugin not found
 *
 * @example
 * local triggers = GetPluginTriggerList("abc12345-1234-1234-1234-123456789012")
 * for i, name in ipairs(triggers) do
 *     Note("Plugin trigger: " .. name)
 * end
 *
 * @see GetTriggerList, GetPluginTriggerInfo, GetPluginTriggerOption, GetPluginList
 */
int L_GetPluginTriggerList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    lua_newtable(L);

    if (plugin) {
        int index = 1;
        for (const auto& [name, triggerPtr] : plugin->m_TriggerMap) {
            lua_pushstring(L, name.toUtf8().constData());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

/**
 * world.GetPluginTriggerInfo(pluginID, triggerName, infoType)
 *
 * Gets specific information about a trigger in another plugin.
 * Uses the same info type codes as GetTriggerInfo.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 * @param triggerName (string) Name of the trigger in the plugin
 * @param infoType (number) Type of information to retrieve (see GetTriggerInfo)
 *
 * @return (varies) Requested info, or nil if plugin/trigger not found
 *
 * @example
 * -- Check if a plugin's trigger is enabled
 * local enabled = GetPluginTriggerInfo(pluginID, "combat", 8)
 *
 * @see GetTriggerInfo, GetPluginTriggerList, GetPluginTriggerOption
 */
int L_GetPluginTriggerInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* triggerName = luaL_checkstring(L, 2);
    int infoType = luaL_checkinteger(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find trigger in plugin's map
    QString tName = QString::fromUtf8(triggerName);
    Trigger* trigger = nullptr;
    auto it = plugin->m_TriggerMap.find(tName);
    if (it != plugin->m_TriggerMap.end()) {
        trigger = it->second.get();
    }

    // Get trigger info (reuse exact GetTriggerInfo logic)
    if (!trigger) {
        lua_pushnil(L);
        pDoc->m_CurrentPlugin = savedPlugin;
        return 1;
    }

    switch (infoType) {
        case 1: // trigger pattern
        {
            QByteArray ba = trigger->trigger.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 2: // contents (send text)
        {
            QByteArray ba = trigger->contents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 3: // sound_to_play
        {
            QByteArray ba = trigger->sound_to_play.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 4: // procedure (script name)
        {
            QByteArray ba = trigger->procedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // omit_from_log
            lua_pushboolean(L, trigger->omit_from_log);
            break;
        case 6: // omit_from_output
            lua_pushboolean(L, trigger->omit_from_output);
            break;
        case 7: // keep_evaluating
            lua_pushboolean(L, trigger->keep_evaluating);
            break;
        case 8: // enabled
            lua_pushboolean(L, trigger->enabled);
            break;
        case 9: // use_regexp
            lua_pushboolean(L, trigger->use_regexp);
            break;
        case 10: // ignore_case
            lua_pushboolean(L, trigger->ignore_case);
            break;
        case 11: // repeat
            lua_pushboolean(L, trigger->repeat);
            break;
        case 12: // sound_if_inactive
            lua_pushboolean(L, trigger->sound_if_inactive);
            break;
        case 13: // expand_variables
            lua_pushboolean(L, trigger->expand_variables);
            break;
        case 14: // clipboard_arg
            lua_pushnumber(L, trigger->clipboard_arg);
            break;
        case 15: // send_to
            lua_pushnumber(L, static_cast<int>(trigger->send_to));
            break;
        case 16: // sequence
            lua_pushnumber(L, trigger->sequence);
            break;
        case 17: // match_type
            lua_pushnumber(L, trigger->match_type);
            break;
        case 18: // style
            lua_pushnumber(L, trigger->style);
            break;
        case 19: // colour
            lua_pushnumber(L, trigger->colour);
            break;
        case 20: // invocation_count
            lua_pushnumber(L, trigger->invocation_count);
            break;
        case 21: // matched
            lua_pushnumber(L, trigger->matched);
            break;
        case 22: // when_matched
            if (trigger->when_matched.isValid()) {
                lua_pushnumber(L, trigger->when_matched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 23: // temporary
            lua_pushboolean(L, trigger->temporary);
            break;
        case 24: // included
            lua_pushboolean(L, trigger->included);
            break;
        case 25: // lowercase_wildcard
            lua_pushboolean(L, trigger->lowercase_wildcard);
            break;
        case 26: // group
        {
            QByteArray ba = trigger->group.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 27: // variable
        {
            QByteArray ba = trigger->variable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 28: // user_option
            lua_pushnumber(L, trigger->user_option);
            break;
        case 29: // other_foreground
            lua_pushnumber(L, trigger->other_foreground);
            break;
        case 30: // other_background
            lua_pushnumber(L, trigger->other_background);
            break;
        case 31: // regexp match count
            if (trigger->regexp) {
                lua_pushnumber(L, trigger->wildcards.size());
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 32: // last matching string
            if (!trigger->wildcards.isEmpty()) {
                QByteArray ba = trigger->wildcards[0].toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 33: // executing_script
            lua_pushboolean(L, trigger->executing_script);
            break;
        case 34: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, trigger->dispid != -1);
            break;
        case 35: // regexp execution error
            lua_pushnumber(L, 0);
            break;
        case 36: // one_shot
            lua_pushboolean(L, trigger->one_shot);
            break;
        case 37: // regexp execution time
            lua_pushnumber(L, 0);
            break;
        case 38: // regexp match attempts
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
            if (wildcard_index < trigger->wildcards.size()) {
                QByteArray ba = trigger->wildcards[wildcard_index].toUtf8();
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
 * world.GetPluginTriggerOption(pluginID, triggerName, optionName)
 *
 * Gets an option value for a trigger in another plugin.
 * Uses the same option names as GetTriggerOption.
 *
 * @param pluginID (string) Plugin ID (GUID format)
 * @param triggerName (string) Name of the trigger in the plugin
 * @param optionName (string) Option name (see GetTriggerOption for valid names)
 *
 * @return (varies) Option value, or nil if plugin/trigger not found
 *
 * @example
 * local seq = GetPluginTriggerOption(pluginID, "combat", "sequence")
 *
 * @see GetTriggerOption, GetPluginTriggerList, GetPluginTriggerInfo
 */
int L_GetPluginTriggerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* triggerName = luaL_checkstring(L, 2);
    const char* optionName = luaL_checkstring(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find trigger
    QString tName = QString::fromUtf8(triggerName);
    Trigger* trigger = nullptr;
    auto it = plugin->m_TriggerMap.find(tName);
    if (it != plugin->m_TriggerMap.end()) {
        trigger = it->second.get();
    }

    if (trigger) {
        QString option = QString::fromUtf8(optionName);
        if (option == "enabled") {
            lua_pushboolean(L, trigger->enabled);
        } else if (option == "keep_evaluating") {
            lua_pushboolean(L, trigger->keep_evaluating);
        } else if (option == "sequence") {
            lua_pushnumber(L, trigger->sequence);
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
 * world.StopEvaluatingTriggers(all_plugins)
 *
 * Stops evaluating triggers for the current line of MUD output.
 * Call this from within a trigger script to prevent other triggers
 * from matching the same line.
 *
 * @param all_plugins (boolean) If true, stops triggers in all plugins;
 *   if false/omitted, only stops triggers in the current plugin (optional)
 *
 * @return (none) No return value
 *
 * @example
 * -- In a trigger script, stop other triggers from matching
 * function OnImportantLine(name, line, wildcards)
 *     -- Process this line exclusively
 *     Note("Got important line: " .. line)
 *     StopEvaluatingTriggers()  -- No other triggers will match
 * end
 *
 * @example
 * -- Stop all plugins from evaluating triggers
 * StopEvaluatingTriggers(true)
 *
 * @see AddTrigger, EnableTrigger, EnableTriggerGroup
 */
int L_StopEvaluatingTriggers(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Optional parameter - defaults to false
    bool all_plugins = lua_toboolean(L, 1);

    // Enum values from doc.h
    // eKeepEvaluatingTriggers = 0
    // eStopEvaluatingTriggers = 1
    // eStopEvaluatingTriggersInAllPlugins = 2
    pDoc->m_iStopTriggerEvaluation = all_plugins ? 2 : 1;

    return 0;
}

/**
 * world.EnableTriggerGroup(group_name, enabled)
 *
 * Enables or disables all triggers that belong to a named group.
 * Groups provide a way to organize related triggers and control them together.
 *
 * @param group_name (string) Name of the trigger group
 * @param enabled (boolean) True to enable all, false to disable all (optional, defaults to true)
 *
 * @return (number) Count of triggers affected
 *
 * @example
 * -- Disable all combat triggers
 * local count = EnableTriggerGroup("combat", false)
 * Note("Disabled " .. count .. " combat triggers")
 *
 * @example
 * -- Enable all triggers in the "healing" group
 * EnableTriggerGroup("healing", true)
 *
 * @see EnableTrigger, DeleteTriggerGroup, SetTriggerOption
 */
int L_EnableTriggerGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* groupName = luaL_checkstring(L, 1);
    bool enabled = lua_isnone(L, 2) ? true : lua_toboolean(L, 2);

    QString qGroupName = QString::fromUtf8(groupName);

    // Empty group name affects nothing
    if (qGroupName.isEmpty()) {
        lua_pushnumber(L, 0);
        return 1;
    }

    long count = 0;

    // Iterate through all triggers
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();
        if (trigger && trigger->group == qGroupName) {
            trigger->enabled = enabled;
            count++;
        }
    }

    // If in plugin context, also check plugin triggers
    if (pDoc->m_CurrentPlugin) {
        for (const auto& [name, triggerPtr] : pDoc->m_CurrentPlugin->m_TriggerMap) {
            Trigger* trigger = triggerPtr.get();
            if (trigger && trigger->group == qGroupName) {
                trigger->enabled = enabled;
                count++;
            }
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.GetTriggerOption(trigger_name, option_name)
 *
 * Gets an option value for a trigger using named option strings.
 * More readable alternative to GetTriggerInfo's numeric codes.
 *
 * Numeric options: clipboard_arg, colour_change_type, custom_colour,
 *   lines_to_match, match_style, new_style, other_text_colour,
 *   other_back_colour, send_to, sequence, user
 * Boolean options: enabled, expand_variables, ignore_case, keep_evaluating,
 *   multi_line, omit_from_log, omit_from_output, regexp, repeat,
 *   sound_if_inactive, lowercase_wildcard, temporary, one_shot
 * String options: group, match, script, sound, send, variable
 *
 * @param trigger_name (string) Name of the trigger
 * @param option_name (string) Name of the option (case-insensitive)
 *
 * @return (varies) Option value, or nil if trigger/option not found
 *
 * @example
 * -- Get the sequence number of a trigger
 * local seq = GetTriggerOption("combat", "sequence")
 *
 * @example
 * -- Check if trigger uses regexp
 * if GetTriggerOption("mob_enters", "regexp") then
 *     Note("Trigger uses regular expressions")
 * end
 *
 * @see SetTriggerOption, GetTriggerInfo, GetTrigger
 */
int L_GetTriggerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* triggerName = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(triggerName);
    QString qOption = QString::fromUtf8(optionName).toLower().trimmed();

    Trigger* trigger = pDoc->getTrigger(qName);
    if (!trigger) {
        lua_pushnil(L);
        return 1;
    }

    // Numeric options
    if (qOption == "clipboard_arg") {
        lua_pushnumber(L, trigger->clipboard_arg);
    } else if (qOption == "colour_change_type") {
        lua_pushnumber(L, static_cast<int>(trigger->colour_change_type));
    } else if (qOption == "custom_colour") {
        lua_pushnumber(L, trigger->colour);
    } else if (qOption == "lines_to_match") {
        lua_pushnumber(L, trigger->lines_to_match);
    } else if (qOption == "match_style") {
        lua_pushnumber(L, trigger->match_type);
    } else if (qOption == "new_style") {
        lua_pushnumber(L, trigger->style);
    } else if (qOption == "other_text_colour") {
        lua_pushnumber(L, trigger->other_foreground);
    } else if (qOption == "other_back_colour") {
        lua_pushnumber(L, trigger->other_background);
    } else if (qOption == "send_to") {
        lua_pushnumber(L, static_cast<int>(trigger->send_to));
    } else if (qOption == "sequence") {
        lua_pushnumber(L, trigger->sequence);
    } else if (qOption == "user") {
        lua_pushnumber(L, trigger->user_option);
    }
    // Boolean options
    else if (qOption == "enabled") {
        lua_pushboolean(L, trigger->enabled);
    } else if (qOption == "expand_variables") {
        lua_pushboolean(L, trigger->expand_variables);
    } else if (qOption == "ignore_case") {
        lua_pushboolean(L, trigger->ignore_case);
    } else if (qOption == "keep_evaluating") {
        lua_pushboolean(L, trigger->keep_evaluating);
    } else if (qOption == "multi_line") {
        lua_pushboolean(L, trigger->multi_line);
    } else if (qOption == "omit_from_log") {
        lua_pushboolean(L, trigger->omit_from_log);
    } else if (qOption == "omit_from_output") {
        lua_pushboolean(L, trigger->omit_from_output);
    } else if (qOption == "regexp") {
        lua_pushboolean(L, trigger->use_regexp);
    } else if (qOption == "repeat") {
        lua_pushboolean(L, trigger->repeat);
    } else if (qOption == "sound_if_inactive") {
        lua_pushboolean(L, trigger->sound_if_inactive);
    } else if (qOption == "lowercase_wildcard") {
        lua_pushboolean(L, trigger->lowercase_wildcard);
    } else if (qOption == "temporary") {
        lua_pushboolean(L, trigger->temporary);
    } else if (qOption == "one_shot") {
        lua_pushboolean(L, trigger->one_shot);
    }
    // String options
    else if (qOption == "group") {
        QByteArray ba = trigger->group.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "match") {
        QByteArray ba = trigger->trigger.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "script") {
        QByteArray ba = trigger->procedure.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "sound") {
        QByteArray ba = trigger->sound_to_play.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "send") {
        QByteArray ba = trigger->contents.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "variable") {
        QByteArray ba = trigger->variable.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/**
 * world.SetTriggerOption(trigger_name, option_name, value)
 *
 * Sets an option value for a trigger using named option strings.
 * Changes take effect immediately for subsequent trigger matching.
 *
 * Numeric options: clipboard_arg, colour_change_type, custom_colour,
 *   lines_to_match, match_style, new_style, other_text_colour,
 *   other_back_colour, send_to, sequence, user
 * Boolean options: enabled, expand_variables, ignore_case, keep_evaluating,
 *   multi_line, omit_from_log, omit_from_output, repeat, sound_if_inactive,
 *   lowercase_wildcard, temporary, one_shot
 * String options: group, match, script, sound, send, variable
 *
 * Note: The "regexp" option cannot be changed after creation.
 *
 * @param trigger_name (string) Name of the trigger
 * @param option_name (string) Name of the option (case-insensitive)
 * @param value (varies) New value for the option
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eTriggerNotFound: No trigger with this name
 *   - eTriggerCannotBeEmpty: Tried to set empty match pattern
 *   - eUnknownOption: Invalid option name
 *   - ePluginCannotSetOption: Option cannot be modified (regexp)
 *
 * @example
 * -- Change trigger sequence for priority
 * SetTriggerOption("combat", "sequence", 50)
 *
 * @example
 * -- Update the response text
 * SetTriggerOption("combat", "send", "flee")
 *
 * @see GetTriggerOption, EnableTrigger, AddTrigger
 */
int L_SetTriggerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* triggerName = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(triggerName);
    QString qOption = QString::fromUtf8(optionName).toLower().trimmed();

    Trigger* trigger = pDoc->getTrigger(qName);
    if (!trigger) {
        return luaReturnError(L, eTriggerNotFound);
    }

    // Numeric options
    if (qOption == "clipboard_arg" || qOption == "colour_change_type" ||
        qOption == "custom_colour" || qOption == "lines_to_match" || qOption == "match_style" ||
        qOption == "new_style" || qOption == "other_text_colour" ||
        qOption == "other_back_colour" || qOption == "send_to" || qOption == "sequence" ||
        qOption == "user") {
        long value = luaL_checknumber(L, 3);

        if (qOption == "clipboard_arg") {
            trigger->clipboard_arg = value;
        } else if (qOption == "colour_change_type") {
            trigger->colour_change_type = static_cast<ColourChangeType>(value);
        } else if (qOption == "custom_colour") {
            trigger->colour = value;
        } else if (qOption == "lines_to_match") {
            trigger->lines_to_match = value;
        } else if (qOption == "match_style") {
            trigger->match_type = static_cast<quint16>(value);
        } else if (qOption == "new_style") {
            trigger->style = value;
        } else if (qOption == "other_text_colour") {
            trigger->other_foreground = value;
        } else if (qOption == "other_back_colour") {
            trigger->other_background = value;
        } else if (qOption == "send_to") {
            trigger->send_to = static_cast<SendTo>(value);
        } else if (qOption == "sequence") {
            trigger->sequence = value;
            // TODO: Re-sort triggers
        } else if (qOption == "user") {
            trigger->user_option = value;
        }
    }
    // Boolean options
    else if (qOption == "enabled" || qOption == "expand_variables" || qOption == "ignore_case" ||
             qOption == "keep_evaluating" || qOption == "multi_line" ||
             qOption == "omit_from_log" || qOption == "omit_from_output" || qOption == "regexp" ||
             qOption == "repeat" || qOption == "sound_if_inactive" ||
             qOption == "lowercase_wildcard" || qOption == "temporary" || qOption == "one_shot") {
        bool value = lua_toboolean(L, 3);

        if (qOption == "enabled") {
            trigger->enabled = value;
        } else if (qOption == "expand_variables") {
            trigger->expand_variables = value;
        } else if (qOption == "ignore_case") {
            trigger->ignore_case = value;
            (void)trigger->compileRegexp(); // Recompile with new case sensitivity
        } else if (qOption == "keep_evaluating") {
            trigger->keep_evaluating = value;
        } else if (qOption == "multi_line") {
            trigger->multi_line = value;
            (void)trigger->compileRegexp(); // Recompile with new multiline setting
        } else if (qOption == "omit_from_log") {
            trigger->omit_from_log = value;
        } else if (qOption == "omit_from_output") {
            trigger->omit_from_output = value;
        } else if (qOption == "regexp") {
            return luaReturnError(L, ePluginCannotSetOption); // Cannot write
        } else if (qOption == "repeat") {
            trigger->repeat = value;
        } else if (qOption == "sound_if_inactive") {
            trigger->sound_if_inactive = value;
        } else if (qOption == "lowercase_wildcard") {
            trigger->lowercase_wildcard = value;
        } else if (qOption == "temporary") {
            trigger->temporary = value;
        } else if (qOption == "one_shot") {
            trigger->one_shot = value;
        }
    }
    // String options
    else if (qOption == "group" || qOption == "match" || qOption == "script" ||
             qOption == "sound" || qOption == "send" || qOption == "variable") {
        const char* value = luaL_checkstring(L, 3);
        QString qValue = QString::fromUtf8(value);

        if (qOption == "group") {
            trigger->group = qValue;
        } else if (qOption == "match") {
            if (qValue.isEmpty()) {
                return luaReturnError(L, eTriggerCannotBeEmpty);
            }
            trigger->trigger = qValue;
            (void)trigger->compileRegexp(); // Recompile with new pattern
        } else if (qOption == "script") {
            trigger->procedure = qValue;
            // TODO: Update dispid
        } else if (qOption == "sound") {
            trigger->sound_to_play = qValue;
        } else if (qOption == "send") {
            trigger->contents = qValue;
        } else if (qOption == "variable") {
            trigger->variable = qValue;
        }
    } else {
        return luaReturnError(L, eUnknownOption);
    }

    return luaReturnOK(L);
}

/**
 * world.AddTriggerEx(name, match, response, flags, color, wildcard, sound_file, script, send_to,
 * sequence)
 *
 * Extended version of AddTrigger with all parameters required.
 * Provides explicit control over all trigger options.
 *
 * Flag values: Same as AddTrigger, plus:
 * - eLowercaseWildcard (4096): Convert wildcards to lowercase
 * - eTriggerOneShot (32768): Delete after first match
 *
 * @param name (string) Unique trigger identifier
 * @param match (string) Pattern to match against MUD output
 * @param response (string) Text to send when triggered
 * @param flags (number) Bitwise OR of flag constants
 * @param color (number) Custom color index for matched text
 * @param wildcard (number) Wildcard number to copy to clipboard (0-10)
 * @param sound_file (string) Sound file path to play on match
 * @param script (string) Script function name to call
 * @param send_to (number) Send destination 0-14
 * @param sequence (number) Evaluation order 0-10000
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eTriggerAlreadyExists: Trigger with this name exists
 *   - eTriggerCannotBeEmpty: Match pattern is empty
 *   - eTriggerSequenceOutOfRange: Sequence not 0-10000
 *   - eTriggerSendToInvalid: Invalid send_to value
 *   - eBadRegularExpression: Invalid regex pattern
 *
 * @example
 * -- Full trigger with all options
 * AddTriggerEx("mob_kill", "^You killed (\\w+)$", "loot corpse",
 *     eEnabled + eTriggerRegularExpression, -1, 1, "kill.wav",
 *     "OnKill", 0, 100)
 *
 * @see AddTrigger, DeleteTrigger, GetTrigger
 */
int L_AddTriggerEx(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* match = luaL_checkstring(L, 2);
    const char* response = luaL_checkstring(L, 3);
    int flags = luaL_checkinteger(L, 4);
    int color = luaL_checkinteger(L, 5);
    int wildcard = luaL_checkinteger(L, 6);
    const char* sound_file = luaL_checkstring(L, 7);
    const char* script = luaL_checkstring(L, 8);
    int send_to = luaL_checkinteger(L, 9);
    int sequence = luaL_checkinteger(L, 10);

    QString qName = QString::fromUtf8(name);
    QString qMatch = QString::fromUtf8(match);

    // Check if trigger already exists (check appropriate map based on context)
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* currentPlugin = plugin(L);
    bool replace = (flags & eReplace) != 0;
    if (currentPlugin) {
        auto it = currentPlugin->m_TriggerMap.find(qName);
        if (it != currentPlugin->m_TriggerMap.end()) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Cannot replace a trigger whose script is currently executing (use-after-free guard)
            if (it->second->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            // Delete existing trigger for replacement
            currentPlugin->m_TriggerMap.erase(it);
        }
    } else {
        Trigger* existing = pDoc->getTrigger(qName);
        if (existing) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Cannot replace a trigger whose script is currently executing (use-after-free guard)
            if (existing->executing_script) {
                return luaReturnError(L, eItemInUse);
            }
            (void)pDoc->deleteTrigger(qName); // safe: already checked executing_script
        }
    }

    // Cannot have empty match
    if (qMatch.isEmpty()) {
        return luaReturnError(L, eTriggerCannotBeEmpty);
    }

    // Check sequence
    if (sequence < 0 || sequence > 10000) {
        return luaReturnError(L, eTriggerSequenceOutOfRange);
    }

    // Check send_to
    if (send_to < 0 || send_to >= eSendToLast) {
        return luaReturnError(L, eTriggerSendToInvalid);
    }

    // Create trigger
    auto trigger = std::make_unique<Trigger>();
    trigger->label = qName;
    trigger->internal_name = qName;
    trigger->trigger = qMatch;
    trigger->contents = QString::fromUtf8(response);
    trigger->enabled = (flags & eEnabled) != 0;
    trigger->omit_from_output = (flags & eOmitFromOutput) != 0;
    trigger->omit_from_log = (flags & eOmitFromLog) != 0;
    // Note: keep_evaluating defaults to true (MUSHclient behavior)
    // The eKeepEvaluating flag is only used to explicitly request keeping evaluation
    // When not set, we keep the default (true) rather than forcing false
    if (flags & eKeepEvaluating) {
        trigger->keep_evaluating = true;
    }
    // else: keep the default from Trigger constructor (true)
    trigger->use_regexp = (flags & eTriggerRegularExpression) != 0;
    trigger->ignore_case = (flags & eIgnoreCase) != 0;
    trigger->expand_variables = (flags & eExpandVariables) != 0;
    trigger->temporary = (flags & eTemporary) != 0;
    trigger->lowercase_wildcard = (flags & eLowercaseWildcard) != 0;
    trigger->one_shot = (flags & eTriggerOneShot) != 0;
    trigger->colour = color;
    trigger->clipboard_arg = wildcard;
    trigger->sound_to_play = QString::fromUtf8(sound_file);
    trigger->procedure = QString::fromUtf8(script);
    trigger->send_to = static_cast<SendTo>(send_to);
    trigger->sequence = sequence;
    trigger->variable = qName; // kludge from original

    // Compile regexp
    if (!trigger->compileRegexp().has_value()) {
        // unique_ptr will automatically delete on scope exit
        return luaReturnError(L, eBadRegularExpression);
    }

    // Add to appropriate trigger map (plugin or world)
    if (currentPlugin) {
        trigger->owningPlugin = currentPlugin;
        currentPlugin->m_TriggerMap[qName] = std::move(trigger);
        // Rebuild trigger array for matching
        currentPlugin->m_TriggerArray.clear();
        for (const auto& [name, t] : currentPlugin->m_TriggerMap) {
            currentPlugin->m_TriggerArray.push_back(t.get());
        }
    } else {
        if (!pDoc->addTrigger(qName, std::move(trigger)).has_value()) {
            return luaReturnError(L, eTriggerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteTriggerGroup(groupName)
 *
 * Permanently deletes all triggers that belong to a named group.
 * Useful for cleaning up related triggers together.
 *
 * @param groupName (string) Name of the trigger group
 *
 * @return (number) Count of triggers deleted
 *
 * @example
 * -- Remove all combat triggers
 * local count = DeleteTriggerGroup("combat")
 * Note("Deleted " .. count .. " combat triggers")
 *
 * @see DeleteTrigger, DeleteTemporaryTriggers, EnableTriggerGroup
 */
int L_DeleteTriggerGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);

    QString qGroupName = QString::fromUtf8(groupName);
    QStringList toDelete;

    // Find all triggers in this group
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        if (triggerPtr->group == qGroupName) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        (void)pDoc->deleteTrigger(name); // intentional: bulk group/temporary delete
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.DeleteTemporaryTriggers()
 *
 * Deletes all triggers that were created with the eTemporary flag.
 * Temporary triggers are normally deleted when the world closes, but
 * this allows manual cleanup at any time.
 *
 * @return (number) Count of triggers deleted
 *
 * @example
 * -- Clean up temporary triggers
 * local count = DeleteTemporaryTriggers()
 * Note("Removed " .. count .. " temporary triggers")
 *
 * @see DeleteTrigger, DeleteTriggerGroup, AddTrigger
 */
int L_DeleteTemporaryTriggers(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList toDelete;

    // Find all temporary triggers
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        if (triggerPtr->temporary) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        (void)pDoc->deleteTrigger(name); // intentional: bulk group/temporary delete
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * register_trigger_functions - Register trigger API functions
 *
 * Registers all trigger-related functions in the worldlib table.
 *
 * @param worldlib Array of luaL_Reg to add functions to
 * @param index Current index in the worldlib array
 * @return Updated index
 */
void register_trigger_functions(luaL_Reg* worldlib, int& index)
{
    worldlib[index++] = {"AddTrigger", L_AddTrigger};
    worldlib[index++] = {"AddTriggerEx", L_AddTriggerEx};
    worldlib[index++] = {"DeleteTrigger", L_DeleteTrigger};
    worldlib[index++] = {"DeleteTriggerGroup", L_DeleteTriggerGroup};
    worldlib[index++] = {"DeleteTemporaryTriggers", L_DeleteTemporaryTriggers};
    worldlib[index++] = {"EnableTrigger", L_EnableTrigger};
    worldlib[index++] = {"EnableTriggerGroup", L_EnableTriggerGroup};
    worldlib[index++] = {"GetTrigger", L_GetTrigger};
    worldlib[index++] = {"GetTriggerInfo", L_GetTriggerInfo};
    worldlib[index++] = {"GetTriggerList", L_GetTriggerList};
    worldlib[index++] = {"GetTriggerOption", L_GetTriggerOption};
    worldlib[index++] = {"IsTrigger", L_IsTrigger};
    worldlib[index++] = {"SetTriggerOption", L_SetTriggerOption};
    worldlib[index++] = {"StopEvaluatingTriggers", L_StopEvaluatingTriggers};
    worldlib[index++] = {"GetPluginTriggerList", L_GetPluginTriggerList};
    worldlib[index++] = {"GetPluginTriggerInfo", L_GetPluginTriggerInfo};
    worldlib[index++] = {"GetPluginTriggerOption", L_GetPluginTriggerOption};
}
