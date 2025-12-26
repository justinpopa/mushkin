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
 * Creates a new trigger.
 *
 * @param name Trigger label
 * @param match Pattern to match
 * @param response Text to send when triggered
 * @param flags Bit flags (enabled, regexp, etc.)
 * @param color Color to change text to (optional)
 * @param wildcard Unused (optional)
 * @param sound_file Sound file to play (optional)
 * @param script Script procedure to call (optional)
 * @param send_to Where to send response (optional, default 0)
 * @param sequence Sequence number (optional, default 100)
 * @return error code (eOK=0 on success)
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
        if (currentPlugin->m_TriggerMap.find(qName) != currentPlugin->m_TriggerMap.end()) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Delete existing trigger for replacement
            currentPlugin->m_TriggerMap.erase(qName);
        }
    } else {
        if (pDoc->getTrigger(qName)) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Delete existing trigger for replacement
            pDoc->deleteTrigger(qName);
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
    trigger->strLabel = qName;
    trigger->strInternalName = qName;
    trigger->trigger = qMatch;
    trigger->contents = QString::fromUtf8(response);
    trigger->bEnabled = (flags & eEnabled) != 0;
    trigger->bOmitFromOutput = (flags & eAliasOmitFromOutput) != 0;
    trigger->omit_from_log = (flags & eOmitFromLogFile) != 0;
    // Note: bKeepEvaluating defaults to true (MUSHclient behavior)
    // The eKeepEvaluating flag is only used to explicitly request keeping evaluation
    // When not set, we keep the default (true) rather than forcing false
    if (flags & eKeepEvaluating) {
        trigger->bKeepEvaluating = true;
    }
    // else: keep the default from Trigger constructor (true)
    trigger->bRegexp = (flags & eAliasRegularExpression) != 0;
    trigger->ignore_case = (flags & eIgnoreAliasCase) != 0;
    trigger->bExpandVariables = (flags & eExpandVariables) != 0;
    trigger->bTemporary = (flags & eTemporary) != 0;
    trigger->bOneShot = (flags & eAliasOneShot) != 0;
    trigger->colour = color;
    trigger->sound_to_play = QString::fromUtf8(sound_file);
    trigger->strProcedure = QString::fromUtf8(script);
    trigger->iSendTo = send_to;
    trigger->iSequence = sequence;

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
        if (!pDoc->addTrigger(qName, std::move(trigger))) {
            return luaReturnError(L, eTriggerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteTrigger(name)
 *
 * Deletes a trigger.
 *
 * @param name Trigger label
 * @return error code (eOK=0 on success)
 */
int L_DeleteTrigger(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    if (!pDoc->deleteTrigger(qName)) {
        return luaReturnError(L, eTriggerNotFound);
    }

    return luaReturnOK(L);
}

/**
 * world.IsTrigger(name)
 *
 * Checks if a trigger exists.
 *
 * @param name Trigger label
 * @return eOK if exists, eTriggerNotFound if not
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
 * Gets trigger details.
 *
 * @param name Trigger label
 * @return error_code, match_text, response_text, flags, colour, wildcard, sound_file, script_name
 *         Returns just error_code if trigger not found
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
    if (trigger->bOmitFromOutput)
        flags |= eOmitFromOutput;
    if (trigger->bKeepEvaluating)
        flags |= eKeepEvaluating;
    if (trigger->omit_from_log)
        flags |= eOmitFromLog;
    if (trigger->bEnabled)
        flags |= eEnabled;
    if (trigger->bRegexp)
        flags |= eTriggerRegularExpression;
    if (trigger->bLowercaseWildcard)
        flags |= eLowercaseWildcard;
    if (trigger->bOneShot)
        flags |= eTriggerOneShot;

    // Return: error_code, match, response, flags, colour, wildcard, sound, script
    lua_pushnumber(L, eOK);
    lua_pushstring(L, trigger->trigger.toUtf8().constData());
    lua_pushstring(L, trigger->contents.toUtf8().constData());
    lua_pushnumber(L, flags);
    lua_pushnumber(L, trigger->colour == -1 ? -1 : trigger->colour);
    lua_pushnumber(L, trigger->iClipboardArg);
    lua_pushstring(L, trigger->sound_to_play.toUtf8().constData());
    lua_pushstring(L, trigger->strProcedure.toUtf8().constData());

    return 8;
}

/**
 * world.EnableTrigger(name, enabled)
 *
 * Enables or disables a trigger.
 *
 * @param name Trigger label
 * @param enabled true to enable, false to disable
 * @return error code (eOK=0 on success)
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

    trigger->bEnabled = enabled;
    return luaReturnOK(L);
}

/**
 * world.GetTriggerInfo(name, info_type)
 *
 * Gets information about a trigger.
 * Matches original MUSHclient info type mappings.
 *
 * Info types:
 *   1 = trigger pattern         21 = times matched
 *   2 = contents (send text)    22 = when matched (date)
 *   3 = sound_to_play           23 = temporary
 *   4 = script (strProcedure)   24 = included
 *   5 = omit_from_log           25 = lowercase_wildcard
 *   6 = omit_from_output        26 = group
 *   7 = keep_evaluating         27 = variable
 *   8 = enabled                 28 = user option
 *   9 = regexp                  29 = other foreground
 *  10 = ignore_case             30 = other background
 *  11 = repeat                  31 = regexp match count
 *  12 = sound_if_inactive       32 = last matching string
 *  13 = expand_variables        33 = executing script
 *  14 = clipboard_arg           34 = has script (dispid)
 *  15 = send_to                 35 = regexp error
 *  16 = sequence                36 = one_shot
 *  17 = match style             37 = regexp time
 *  18 = new style               38 = regexp attempts
 *  19 = colour                  101-110 = wildcards
 *  20 = invocation count
 *
 * @param name Trigger label
 * @param info_type Type of information to get
 * @return Information value or nil if not found
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
        case 4: // strProcedure (script name)
        {
            QByteArray ba = trigger->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // omit_from_log
            lua_pushboolean(L, trigger->omit_from_log);
            break;
        case 6: // bOmitFromOutput
            lua_pushboolean(L, trigger->bOmitFromOutput);
            break;
        case 7: // bKeepEvaluating
            lua_pushboolean(L, trigger->bKeepEvaluating);
            break;
        case 8: // bEnabled
            lua_pushboolean(L, trigger->bEnabled);
            break;
        case 9: // bRegexp
            lua_pushboolean(L, trigger->bRegexp);
            break;
        case 10: // ignore_case
            lua_pushboolean(L, trigger->ignore_case);
            break;
        case 11: // bRepeat
            lua_pushboolean(L, trigger->bRepeat);
            break;
        case 12: // bSoundIfInactive
            lua_pushboolean(L, trigger->bSoundIfInactive);
            break;
        case 13: // bExpandVariables
            lua_pushboolean(L, trigger->bExpandVariables);
            break;
        case 14: // iClipboardArg
            lua_pushnumber(L, trigger->iClipboardArg);
            break;
        case 15: // iSendTo
            lua_pushnumber(L, trigger->iSendTo);
            break;
        case 16: // iSequence
            lua_pushnumber(L, trigger->iSequence);
            break;
        case 17: // iMatch
            lua_pushnumber(L, trigger->iMatch);
            break;
        case 18: // iStyle
            lua_pushnumber(L, trigger->iStyle);
            break;
        case 19: // colour
            lua_pushnumber(L, trigger->colour);
            break;
        case 20: // nInvocationCount
            lua_pushnumber(L, trigger->nInvocationCount);
            break;
        case 21: // nMatched
            lua_pushnumber(L, trigger->nMatched);
            break;
        case 22: // tWhenMatched
            if (trigger->tWhenMatched.isValid()) {
                // Return as Unix timestamp (seconds since epoch)
                lua_pushnumber(L, trigger->tWhenMatched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L); // Return nil if never matched
            }
            break;
        case 23: // bTemporary
            lua_pushboolean(L, trigger->bTemporary);
            break;
        case 24: // bIncluded
            lua_pushboolean(L, trigger->bIncluded);
            break;
        case 25: // bLowercaseWildcard
            lua_pushboolean(L, trigger->bLowercaseWildcard);
            break;
        case 26: // strGroup
        {
            QByteArray ba = trigger->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 27: // strVariable
        {
            QByteArray ba = trigger->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 28: // iUserOption
            lua_pushnumber(L, trigger->iUserOption);
            break;
        case 29: // iOtherForeground
            lua_pushnumber(L, trigger->iOtherForeground);
            break;
        case 30: // iOtherBackground
            lua_pushnumber(L, trigger->iOtherBackground);
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
        case 33: // bExecutingScript
            lua_pushboolean(L, trigger->bExecutingScript);
            break;
        case 34: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, trigger->dispid != -1);
            break;
        case 35: // regexp execution error
            // We don't track regexp errors in Qt (always 0)
            lua_pushnumber(L, 0);
            break;
        case 36: // bOneShot
            lua_pushboolean(L, trigger->bOneShot);
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
 * Gets a list of all trigger names.
 *
 * @return Table (array) of trigger names
 */
int L_GetTriggerList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);
    int i = 1;
    for (const auto& [name, triggerPtr] : pDoc->m_TriggerMap) {
        QByteArray ba = name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/**
 * GetPluginTriggerList - Get array of all trigger names in plugin
 *
 * Lua signature: triggers = GetPluginTriggerList(pluginID)
 *
 * Returns: Lua table (array) of trigger names
 *
 * Based on methods_plugins.cpp
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
 * GetPluginTriggerInfo - Get trigger info from plugin
 *
 * Lua signature: value = GetPluginTriggerInfo(pluginID, triggerName, infoType)
 *
 * Returns: Varies by infoType
 *
 * Uses GET_PLUGIN_STUFF pattern to access another plugin's trigger
 *
 * Based on methods_plugins.cpp
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
        case 4: // strProcedure (script name)
        {
            QByteArray ba = trigger->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // omit_from_log
            lua_pushboolean(L, trigger->omit_from_log);
            break;
        case 6: // bOmitFromOutput
            lua_pushboolean(L, trigger->bOmitFromOutput);
            break;
        case 7: // bKeepEvaluating
            lua_pushboolean(L, trigger->bKeepEvaluating);
            break;
        case 8: // bEnabled
            lua_pushboolean(L, trigger->bEnabled);
            break;
        case 9: // bRegexp
            lua_pushboolean(L, trigger->bRegexp);
            break;
        case 10: // ignore_case
            lua_pushboolean(L, trigger->ignore_case);
            break;
        case 11: // bRepeat
            lua_pushboolean(L, trigger->bRepeat);
            break;
        case 12: // bSoundIfInactive
            lua_pushboolean(L, trigger->bSoundIfInactive);
            break;
        case 13: // bExpandVariables
            lua_pushboolean(L, trigger->bExpandVariables);
            break;
        case 14: // iClipboardArg
            lua_pushnumber(L, trigger->iClipboardArg);
            break;
        case 15: // iSendTo
            lua_pushnumber(L, trigger->iSendTo);
            break;
        case 16: // iSequence
            lua_pushnumber(L, trigger->iSequence);
            break;
        case 17: // iMatch
            lua_pushnumber(L, trigger->iMatch);
            break;
        case 18: // iStyle
            lua_pushnumber(L, trigger->iStyle);
            break;
        case 19: // colour
            lua_pushnumber(L, trigger->colour);
            break;
        case 20: // nInvocationCount
            lua_pushnumber(L, trigger->nInvocationCount);
            break;
        case 21: // nMatched
            lua_pushnumber(L, trigger->nMatched);
            break;
        case 22: // tWhenMatched
            if (trigger->tWhenMatched.isValid()) {
                lua_pushnumber(L, trigger->tWhenMatched.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 23: // bTemporary
            lua_pushboolean(L, trigger->bTemporary);
            break;
        case 24: // bIncluded
            lua_pushboolean(L, trigger->bIncluded);
            break;
        case 25: // bLowercaseWildcard
            lua_pushboolean(L, trigger->bLowercaseWildcard);
            break;
        case 26: // strGroup
        {
            QByteArray ba = trigger->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 27: // strVariable
        {
            QByteArray ba = trigger->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 28: // iUserOption
            lua_pushnumber(L, trigger->iUserOption);
            break;
        case 29: // iOtherForeground
            lua_pushnumber(L, trigger->iOtherForeground);
            break;
        case 30: // iOtherBackground
            lua_pushnumber(L, trigger->iOtherBackground);
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
        case 33: // bExecutingScript
            lua_pushboolean(L, trigger->bExecutingScript);
            break;
        case 34: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, trigger->dispid != -1);
            break;
        case 35: // regexp execution error
            lua_pushnumber(L, 0);
            break;
        case 36: // bOneShot
            lua_pushboolean(L, trigger->bOneShot);
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
 * GetPluginTriggerOption - Get trigger option from plugin
 *
 * Lua signature: value = GetPluginTriggerOption(pluginID, triggerName, optionName)
 *
 * Returns: Varies by option
 *
 * Based on methods_plugins.cpp
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
            lua_pushboolean(L, trigger->bEnabled);
        } else if (option == "keep_evaluating") {
            lua_pushboolean(L, trigger->bKeepEvaluating);
        } else if (option == "sequence") {
            lua_pushnumber(L, trigger->iSequence);
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
 * Stops processing triggers for the current line.
 * Based on CMUSHclientDoc::StopEvaluatingTriggers() from methods_triggers.cpp
 *
 * @param all_plugins Optional boolean - if true, stops triggers in all plugins, not just current
 * @return no return value
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
 * Enables or disables all triggers in a group.
 * Based on CMUSHclientDoc::EnableTriggerGroup() from methods_triggers.cpp
 *
 * @param group_name Name of the trigger group
 * @param enabled Optional boolean - defaults to true
 * @return Number of triggers affected
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
    for (const auto& [name, triggerPtr] : pDoc->m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();
        if (trigger && trigger->strGroup == qGroupName) {
            trigger->bEnabled = enabled;
            count++;
        }
    }

    // If in plugin context, also check plugin triggers
    if (pDoc->m_CurrentPlugin) {
        for (const auto& [name, triggerPtr] : pDoc->m_CurrentPlugin->m_TriggerMap) {
            Trigger* trigger = triggerPtr.get();
            if (trigger && trigger->strGroup == qGroupName) {
                trigger->bEnabled = enabled;
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
 * Gets an option value for a trigger.
 * Based on CMUSHclientDoc::GetTriggerOption() from methods_triggers.cpp
 *
 * @param trigger_name Name of the trigger
 * @param option_name Name of the option to get
 * @return Option value or nil if not found
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
        lua_pushnumber(L, trigger->iClipboardArg);
    } else if (qOption == "colour_change_type") {
        lua_pushnumber(L, trigger->iColourChangeType);
    } else if (qOption == "custom_colour") {
        lua_pushnumber(L, trigger->colour);
    } else if (qOption == "lines_to_match") {
        lua_pushnumber(L, trigger->iLinesToMatch);
    } else if (qOption == "match_style") {
        lua_pushnumber(L, trigger->iMatch);
    } else if (qOption == "new_style") {
        lua_pushnumber(L, trigger->iStyle);
    } else if (qOption == "other_text_colour") {
        lua_pushnumber(L, trigger->iOtherForeground);
    } else if (qOption == "other_back_colour") {
        lua_pushnumber(L, trigger->iOtherBackground);
    } else if (qOption == "send_to") {
        lua_pushnumber(L, trigger->iSendTo);
    } else if (qOption == "sequence") {
        lua_pushnumber(L, trigger->iSequence);
    } else if (qOption == "user") {
        lua_pushnumber(L, trigger->iUserOption);
    }
    // Boolean options - return 0/1 to match original MUSHclient (uses SetUpVariantLong)
    else if (qOption == "enabled") {
        lua_pushnumber(L, trigger->bEnabled ? 1 : 0);
    } else if (qOption == "expand_variables") {
        lua_pushnumber(L, trigger->bExpandVariables ? 1 : 0);
    } else if (qOption == "ignore_case") {
        lua_pushnumber(L, trigger->ignore_case ? 1 : 0);
    } else if (qOption == "keep_evaluating") {
        lua_pushnumber(L, trigger->bKeepEvaluating ? 1 : 0);
    } else if (qOption == "multi_line") {
        lua_pushnumber(L, trigger->bMultiLine ? 1 : 0);
    } else if (qOption == "omit_from_log") {
        lua_pushnumber(L, trigger->omit_from_log ? 1 : 0);
    } else if (qOption == "omit_from_output") {
        lua_pushnumber(L, trigger->bOmitFromOutput ? 1 : 0);
    } else if (qOption == "regexp") {
        lua_pushnumber(L, trigger->bRegexp ? 1 : 0);
    } else if (qOption == "repeat") {
        lua_pushnumber(L, trigger->bRepeat ? 1 : 0);
    } else if (qOption == "sound_if_inactive") {
        lua_pushnumber(L, trigger->bSoundIfInactive ? 1 : 0);
    } else if (qOption == "lowercase_wildcard") {
        lua_pushnumber(L, trigger->bLowercaseWildcard ? 1 : 0);
    } else if (qOption == "temporary") {
        lua_pushnumber(L, trigger->bTemporary ? 1 : 0);
    } else if (qOption == "one_shot") {
        lua_pushnumber(L, trigger->bOneShot ? 1 : 0);
    }
    // String options
    else if (qOption == "group") {
        QByteArray ba = trigger->strGroup.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "match") {
        QByteArray ba = trigger->trigger.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "script") {
        QByteArray ba = trigger->strProcedure.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "sound") {
        QByteArray ba = trigger->sound_to_play.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "send") {
        QByteArray ba = trigger->contents.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "variable") {
        QByteArray ba = trigger->strVariable.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/**
 * world.SetTriggerOption(trigger_name, option_name, value)
 *
 * Sets an option value for a trigger.
 * Based on CMUSHclientDoc::SetTriggerOption() from methods_triggers.cpp
 *
 * @param trigger_name Name of the trigger
 * @param option_name Name of the option to set
 * @param value New value for the option
 * @return Error code (eOK=0 on success)
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
            trigger->iClipboardArg = value;
        } else if (qOption == "colour_change_type") {
            trigger->iColourChangeType = value;
        } else if (qOption == "custom_colour") {
            trigger->colour = value;
        } else if (qOption == "lines_to_match") {
            trigger->iLinesToMatch = value;
        } else if (qOption == "match_style") {
            trigger->iMatch = value;
        } else if (qOption == "new_style") {
            trigger->iStyle = value;
        } else if (qOption == "other_text_colour") {
            trigger->iOtherForeground = value;
        } else if (qOption == "other_back_colour") {
            trigger->iOtherBackground = value;
        } else if (qOption == "send_to") {
            trigger->iSendTo = value;
        } else if (qOption == "sequence") {
            trigger->iSequence = value;
            pDoc->m_triggersNeedSorting = true;
        } else if (qOption == "user") {
            trigger->iUserOption = value;
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
            trigger->bEnabled = value;
        } else if (qOption == "expand_variables") {
            trigger->bExpandVariables = value;
        } else if (qOption == "ignore_case") {
            trigger->ignore_case = value;
            trigger->compileRegexp(); // Recompile with new case sensitivity
        } else if (qOption == "keep_evaluating") {
            trigger->bKeepEvaluating = value;
        } else if (qOption == "multi_line") {
            trigger->bMultiLine = value;
            trigger->compileRegexp(); // Recompile with new multiline setting
        } else if (qOption == "omit_from_log") {
            trigger->omit_from_log = value;
        } else if (qOption == "omit_from_output") {
            trigger->bOmitFromOutput = value;
        } else if (qOption == "regexp") {
            return luaReturnError(L, ePluginCannotSetOption); // Cannot write
        } else if (qOption == "repeat") {
            trigger->bRepeat = value;
        } else if (qOption == "sound_if_inactive") {
            trigger->bSoundIfInactive = value;
        } else if (qOption == "lowercase_wildcard") {
            trigger->bLowercaseWildcard = value;
        } else if (qOption == "temporary") {
            trigger->bTemporary = value;
        } else if (qOption == "one_shot") {
            trigger->bOneShot = value;
        }
    }
    // String options
    else if (qOption == "group" || qOption == "match" || qOption == "script" ||
             qOption == "sound" || qOption == "send" || qOption == "variable") {
        const char* value = luaL_checkstring(L, 3);
        QString qValue = QString::fromUtf8(value);

        if (qOption == "group") {
            trigger->strGroup = qValue;
        } else if (qOption == "match") {
            if (qValue.isEmpty()) {
                return luaReturnError(L, eTriggerCannotBeEmpty);
            }
            trigger->trigger = qValue;
            trigger->compileRegexp(); // Recompile with new pattern
        } else if (qOption == "script") {
            trigger->strProcedure = qValue;
            trigger->dispid = DISPID_UNKNOWN; // Reset so it gets re-looked up
        } else if (qOption == "sound") {
            trigger->sound_to_play = qValue;
        } else if (qOption == "send") {
            trigger->contents = qValue;
        } else if (qOption == "variable") {
            trigger->strVariable = qValue;
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
 * Extended version of AddTrigger with more parameters.
 * Based on CMUSHclientDoc::AddTriggerEx() from methods_triggers.cpp
 *
 * @param name Trigger label
 * @param match Pattern to match
 * @param response Text to send when triggered
 * @param flags Bit flags (enabled, regexp, etc.)
 * @param color Color to change text to
 * @param wildcard Wildcard to copy to clipboard (0-10)
 * @param sound_file Sound file to play
 * @param script Script procedure to call
 * @param send_to Where to send response
 * @param sequence Sequence number
 * @return error code (eOK=0 on success)
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
        if (currentPlugin->m_TriggerMap.find(qName) != currentPlugin->m_TriggerMap.end()) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Delete existing trigger for replacement
            currentPlugin->m_TriggerMap.erase(qName);
        }
    } else {
        if (pDoc->getTrigger(qName)) {
            if (!replace) {
                return luaReturnError(L, eTriggerAlreadyExists);
            }
            // Delete existing trigger for replacement
            pDoc->deleteTrigger(qName);
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
    trigger->strLabel = qName;
    trigger->strInternalName = qName;
    trigger->trigger = qMatch;
    trigger->contents = QString::fromUtf8(response);
    trigger->bEnabled = (flags & eEnabled) != 0;
    trigger->bOmitFromOutput = (flags & eOmitFromOutput) != 0;
    trigger->omit_from_log = (flags & eOmitFromLog) != 0;
    // Note: bKeepEvaluating defaults to true (MUSHclient behavior)
    // The eKeepEvaluating flag is only used to explicitly request keeping evaluation
    // When not set, we keep the default (true) rather than forcing false
    if (flags & eKeepEvaluating) {
        trigger->bKeepEvaluating = true;
    }
    // else: keep the default from Trigger constructor (true)
    trigger->bRegexp = (flags & eTriggerRegularExpression) != 0;
    trigger->ignore_case = (flags & eIgnoreCase) != 0;
    trigger->bExpandVariables = (flags & eExpandVariables) != 0;
    trigger->bTemporary = (flags & eTemporary) != 0;
    trigger->bLowercaseWildcard = (flags & eLowercaseWildcard) != 0;
    trigger->bOneShot = (flags & eTriggerOneShot) != 0;
    trigger->colour = color;
    trigger->iClipboardArg = wildcard;
    trigger->sound_to_play = QString::fromUtf8(sound_file);
    trigger->strProcedure = QString::fromUtf8(script);
    trigger->iSendTo = send_to;
    trigger->iSequence = sequence;
    trigger->strVariable = qName; // kludge from original

    // Compile regexp
    if (!trigger->compileRegexp()) {
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
        if (!pDoc->addTrigger(qName, std::move(trigger))) {
            return luaReturnError(L, eTriggerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteTriggerGroup(groupName)
 *
 * Deletes all triggers in a group.
 *
 * @param groupName Trigger group name
 * @return Count of triggers deleted
 */
int L_DeleteTriggerGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);

    QString qGroupName = QString::fromUtf8(groupName);
    QStringList toDelete;

    // Find all triggers in this group
    for (const auto& [name, triggerPtr] : pDoc->m_TriggerMap) {
        if (triggerPtr->strGroup == qGroupName) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteTrigger(name);
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.DeleteTemporaryTriggers()
 *
 * Deletes all temporary triggers.
 *
 * @return Count of triggers deleted
 */
int L_DeleteTemporaryTriggers(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList toDelete;

    // Find all temporary triggers
    for (const auto& [name, triggerPtr] : pDoc->m_TriggerMap) {
        if (triggerPtr->bTemporary) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteTrigger(name);
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
