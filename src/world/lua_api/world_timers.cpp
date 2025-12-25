/**
 * world_timers.cpp - Timer-related Lua API Functions
 *
 * This file implements the Lua C bindings for timer-related functions.
 * Functions are registered in the "world" table and callable from Lua scripts.
 *
 * Extracted from lua_methods.cpp
 */

#include "../../automation/plugin.h"
#include "../../automation/sendto.h"
#include "../../automation/timer.h"
#include "../../world/world_document.h"
#include "../script_engine.h"
#include <QDateTime>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "lua_common.h"

// ========== Timer Functions ==========

/**
 * world.AddTimer(name, hour, minute, second, text, flags, scriptName)
 *
 * Creates a new timer.
 *
 * @param name Timer label (must be unique)
 * @param hour Hour component (0-23 for at-time, or interval hours)
 * @param minute Minute component (0-59)
 * @param second Second component (0-59.999...)
 * @param text Text to send when timer fires
 * @param flags Timer flags (eTimerEnabled, eTimerAtTime, etc.)
 * @param scriptName Optional script function to call (default: "")
 * @return error code (eOK=0 on success)
 */
int L_AddTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    int hour = luaL_checkinteger(L, 2);
    int minute = luaL_checkinteger(L, 3);
    double second = luaL_checknumber(L, 4);
    const char* text = luaL_checkstring(L, 5);
    int flags = luaL_checkinteger(L, 6);
    const char* scriptName = luaL_optstring(L, 7, "");

    QString qName = QString::fromUtf8(name);

    // Validate and normalize timer name
    qint32 nameStatus = validateObjectName(qName);
    if (nameStatus != eOK) {
        lua_pushnumber(L, nameStatus);
        return 1;
    }

    // Check if timer already exists (check appropriate map based on context)
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        if (currentPlugin->m_TimerMap.find(qName) != currentPlugin->m_TimerMap.end()) {
            // If Replace flag is set, delete the old timer first
            if (flags & eTimerReplace) {
                currentPlugin->m_TimerRevMap.remove(currentPlugin->m_TimerMap[qName].get());
                currentPlugin->m_TimerMap.erase(qName);
            } else {
                return luaReturnError(L, eTimerAlreadyExists);
            }
        }
    } else {
        if (pDoc->getTimer(qName)) {
            // If Replace flag is set, delete the old timer first
            if (flags & eTimerReplace) {
                pDoc->deleteTimer(qName);
            } else {
                return luaReturnError(L, eTimerAlreadyExists);
            }
        }
    }

    // Validate time values
    if (hour < 0 || hour > 23) {
        return luaReturnError(L, eTimeInvalid);
    }
    if (minute < 0 || minute > 59) {
        return luaReturnError(L, eTimeInvalid);
    }
    if (second < 0.0 || second >= 60.0) {
        return luaReturnError(L, eTimeInvalid);
    }

    // Can't have zero time for interval timers (would fire continuously)
    if (!(flags & eTimerAtTime)) {
        if (hour == 0 && minute == 0 && second <= 0.0) {
            return luaReturnError(L, eTimeInvalid);
        }
    }

    // Create timer
    auto timer = std::make_unique<Timer>();
    timer->strLabel = qName;
    timer->bEnabled = (flags & eTimerEnabled) != 0;
    timer->bOneShot = (flags & eTimerOneShot) != 0;
    timer->bTemporary = (flags & eTimerTemporary) != 0;
    timer->bActiveWhenClosed = (flags & eTimerActiveWhenClosed) != 0;
    timer->strContents = QString::fromUtf8(text);
    timer->strProcedure = QString::fromUtf8(scriptName);

    // Set timer type and timing fields
    if (flags & eTimerAtTime) {
        // At-time timer: fire at specific time each day
        timer->iType = Timer::eAtTime;
        timer->iAtHour = hour;
        timer->iAtMinute = minute;
        timer->fAtSecond = second;
    } else {
        // Interval timer: fire every N time
        timer->iType = Timer::eInterval;
        timer->iEveryHour = hour;
        timer->iEveryMinute = minute;
        timer->fEverySecond = second;
    }

    // Set SendTo based on flags
    if (flags & eTimerSpeedWalk) {
        timer->iSendTo = eSendToSpeedwalk;
    } else if (flags & eTimerNote) {
        timer->iSendTo = eSendToOutput;
    } else {
        timer->iSendTo = eSendToWorld;
    }

    // Calculate when the timer should first fire (based on ResetOneTimer)
    // Only calculate if timer is enabled
    if (timer->bEnabled) {
        QDateTime now = QDateTime::currentDateTime();
        timer->tWhenFired = now;

        if (timer->iType == Timer::eAtTime) {
            // At-time timer: set to today at the specified time
            QDate today = now.date();
            QTime targetTime(timer->iAtHour, timer->iAtMinute, (int)timer->fAtSecond,
                             (int)((timer->fAtSecond - (int)timer->fAtSecond) * 1000));
            timer->tFireTime = QDateTime(today, targetTime);

            // If this time has passed today, move to tomorrow
            if (timer->tFireTime < now) {
                timer->tFireTime = timer->tFireTime.addDays(1);
            }
        } else {
            // Interval timer: fire after the interval from now
            qint64 intervalSecs =
                timer->iEveryHour * 3600 + timer->iEveryMinute * 60 + (qint64)timer->fEverySecond;
            timer->tFireTime = now.addSecs(intervalSecs);

            // Subtract offset (if any)
            timer->tFireTime =
                timer->tFireTime.addSecs(-(timer->iOffsetHour * 3600 + timer->iOffsetMinute * 60 +
                                           (qint64)timer->fOffsetSecond));
        }
    }

    // Add to appropriate timer map (plugin or world)
    // When called from plugin, timer must execute in plugin's Lua state
    if (currentPlugin) {
        Timer* rawTimer = timer.get();
        currentPlugin->m_TimerMap[qName] = std::move(timer);
        currentPlugin->m_TimerRevMap[rawTimer] = qName;
        // Fire time already calculated above when timer was created
    } else {
        // Add to world's timer map
        if (!pDoc->addTimer(qName, std::move(timer))) {
            // addTimer returns false if timer already exists (shouldn't happen - we checked above)
            return luaReturnError(L, eTimerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DeleteTimer(name)
 *
 * Deletes a timer.
 *
 * @param name Timer label
 * @return error code (eOK=0 on success)
 */
int L_DeleteTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it == currentPlugin->m_TimerMap.end()) {
            return luaReturnError(L, eTimerNotFound);
        }
        currentPlugin->m_TimerRevMap.remove(it->second.get());
        currentPlugin->m_TimerMap.erase(it);
    } else {
        if (!pDoc->deleteTimer(qName)) {
            return luaReturnError(L, eTimerNotFound);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.EnableTimer(name, enabled)
 *
 * Enables or disables a timer.
 *
 * @param name Timer label
 * @param enabled true to enable, false to disable
 * @return error code (eOK=0 on success)
 */
int L_EnableTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qName = QString::fromUtf8(name);
    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        return luaReturnError(L, eTimerNotFound);
    }

    timer->bEnabled = enabled;
    return luaReturnOK(L);
}

/**
 * world.IsTimer(name)
 *
 * Checks if a timer exists.
 *
 * @param name Timer label
 * @return error code (eOK=0 if exists, eTimerNotFound if not)
 */
int L_IsTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        lua_pushnumber(L, eTimerNotFound);
    } else {
        lua_pushnumber(L, eOK);
    }

    return 1;
}

/**
 * world.GetTimer(name)
 *
 * Gets timer details.
 *
 * @param name Timer label
 * @return error_code, hour, minute, second, response_text, flags, script_name
 *         Returns just error_code if timer not found
 */
int L_GetTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        return luaReturnError(L, eTimerNotFound);
    }

    // Get time values (depends on timer type)
    int hour, minute;
    double second;
    if (timer->iType == Timer::eAtTime) {
        hour = timer->iAtHour;
        minute = timer->iAtMinute;
        second = timer->fAtSecond;
    } else {
        hour = timer->iEveryHour;
        minute = timer->iEveryMinute;
        second = timer->fEverySecond;
    }

    // Build flags
    int flags = 0;
    if (timer->bEnabled)
        flags |= eTimerEnabled;
    if (timer->bOneShot)
        flags |= eTimerOneShot;
    if (timer->iSendTo == eSendToSpeedwalk)
        flags |= eTimerSpeedWalk;
    if (timer->iSendTo == eSendToOutput)
        flags |= eTimerNote;
    if (timer->bActiveWhenClosed)
        flags |= eTimerActiveWhenClosed;

    // Return: error_code, hour, minute, second, response, flags, script
    lua_pushnumber(L, eOK);
    lua_pushnumber(L, hour);
    lua_pushnumber(L, minute);
    lua_pushnumber(L, second);
    lua_pushstring(L, timer->strContents.toUtf8().constData());
    lua_pushnumber(L, flags);
    lua_pushstring(L, timer->strProcedure.toUtf8().constData());

    return 7;
}

/**
 * world.GetTimerInfo(name, info_type)
 *
 * Gets information about a timer.
 *
 * Info types:
 *   1 = hour                    15 = group
 *   2 = minute                  16 = variable
 *   3 = second                  17 = user option
 *   4 = contents (send text)    18 = executing script
 *   5 = script (strProcedure)   19 = has script (dispid)
 *   6 = omit_from_log           20 = invocation count
 *   7 = enabled                 21 = times matched
 *   8 = at_time                 22 = when matched (date)
 *   9 = one_shot                23 = send_to
 *  10 = temporary               24 = active_when_closed
 *  11 = interval_hour           25 = time_to_next_fire
 *  12 = interval_minute         26 = at_time_string
 *  13 = interval_second
 *  14 = sequence
 *
 * @param name Timer label
 * @param info_type Type of information to get
 * @return Information value or nil if not found
 */
int L_GetTimerInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    int info_type = luaL_checkinteger(L, 2);

    QString qName = QString::fromUtf8(name);
    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        lua_pushnil(L);
        return 1;
    }

    switch (info_type) {
        case 1: // hour (at-time hour or interval hour)
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->iAtHour);
            } else {
                lua_pushnumber(L, timer->iEveryHour);
            }
            break;
        case 2: // minute
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->iAtMinute);
            } else {
                lua_pushnumber(L, timer->iEveryMinute);
            }
            break;
        case 3: // second
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->fAtSecond);
            } else {
                lua_pushnumber(L, timer->fEverySecond);
            }
            break;
        case 4: // contents (send text)
        {
            QByteArray ba = timer->strContents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // strProcedure (script name)
        {
            QByteArray ba = timer->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 6: // bOmitFromLog
            lua_pushboolean(L, timer->bOmitFromLog);
            break;
        case 7: // bEnabled
            lua_pushboolean(L, timer->bEnabled);
            break;
        case 8: // bAtTime
            lua_pushboolean(L, timer->iType == Timer::eAtTime);
            break;
        case 9: // bOneShot
            lua_pushboolean(L, timer->bOneShot);
            break;
        case 10: // bTemporary
            lua_pushboolean(L, timer->bTemporary);
            break;
        case 11: // interval hour (0 for at-time timers)
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->iEveryHour);
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 12: // interval minute
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->iEveryMinute);
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 13: // interval second
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->fEverySecond);
            } else {
                lua_pushnumber(L, 0.0);
            }
            break;
        case 14: // sequence (use nCreateSequence)
            lua_pushnumber(L, timer->nCreateSequence);
            break;
        case 15: // strGroup
        {
            QByteArray ba = timer->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 16: // strVariable
        {
            QByteArray ba = timer->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // iUserOption
            lua_pushnumber(L, timer->iUserOption);
            break;
        case 18: // bExecutingScript
            lua_pushboolean(L, timer->bExecutingScript);
            break;
        case 19: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, timer->dispid != -1);
            break;
        case 20: // nInvocationCount
            lua_pushnumber(L, timer->nInvocationCount);
            break;
        case 21: // nMatched
            lua_pushnumber(L, timer->nMatched);
            break;
        case 22: // tWhenFired
            if (timer->tWhenFired.isValid()) {
                lua_pushnumber(L, timer->tWhenFired.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 23: // iSendTo
            lua_pushnumber(L, timer->iSendTo);
            break;
        case 24: // bActiveWhenClosed
            lua_pushboolean(L, timer->bActiveWhenClosed);
            break;
        case 25: // time to next fire (in seconds)
            if (timer->tFireTime.isValid()) {
                qint64 msecs = QDateTime::currentDateTime().msecsTo(timer->tFireTime);
                lua_pushnumber(L, msecs / 1000.0);
            } else {
                lua_pushnumber(L, 0.0);
            }
            break;
        case 26: // at_time_string (formatted time)
            if (timer->iType == Timer::eAtTime) {
                QString timeStr = QString("%1:%2:%3")
                                      .arg(timer->iAtHour, 2, 10, QChar('0'))
                                      .arg(timer->iAtMinute, 2, 10, QChar('0'))
                                      .arg(timer->fAtSecond, 5, 'f', 2, QChar('0'));
                QByteArray ba = timeStr.toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;

        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.GetTimerList()
 *
 * Gets a list of all timer names.
 *
 * @return Table (array) of timer names
 */
int L_GetTimerList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);
    int i = 1;
    for (const auto& [name, timer] : pDoc->m_TimerMap) {
        QByteArray ba = name.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/**
 * world.ResetTimer(name)
 *
 * Resets a timer to fire again from now.
 *
 * @param name Timer label
 * @return error code (eOK=0 on success)
 */
int L_ResetTimer(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);
    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        return luaReturnError(L, eTimerNotFound);
    }

    // Reset the timer by recalculating its fire time
    pDoc->calculateNextFireTime(timer);

    return luaReturnOK(L);
}

/**
 * world.ResetTimers()
 *
 * Resets all timers to fire again from now.
 */
int L_ResetTimers(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Iterate through all timers and reset each one
    for (const auto& [name, timer] : pDoc->m_TimerMap) {
        pDoc->calculateNextFireTime(timer.get());
    }

    return 0;
}

/**
 * world.DoAfter(seconds, text)
 *
 * Creates a temporary one-shot timer that sends text after a delay.
 *
 * @param seconds Delay in seconds
 * @param text Text to send to MUD
 * @return error code (eOK=0 on success)
 */
int L_DoAfter(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    double seconds = luaL_checknumber(L, 1);
    const char* text = luaL_checkstring(L, 2);

    if (seconds <= 0.0) {
        return luaReturnError(L, eTimeInvalid);
    }

    // Generate unique name: "doafter_<timestamp>_<counter>"
    static quint64 counter = 0;
    QString name = QString("doafter_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++counter);

    // Create temporary one-shot interval timer
    auto timer = std::make_unique<Timer>();
    timer->strLabel = name;
    timer->iType = Timer::eInterval;
    timer->iEveryHour = 0;
    timer->iEveryMinute = 0;
    timer->fEverySecond = seconds;
    timer->strContents = QString::fromUtf8(text);
    timer->bEnabled = true;
    timer->bOneShot = true; // Delete after firing
    timer->bTemporary = true;
    timer->bActiveWhenClosed = true;
    timer->iSendTo = eSendToWorld;

    // Add to document (transfer ownership)
    if (!pDoc->addTimer(name, std::move(timer))) {
        return luaReturnError(L, eTimerAlreadyExists);
    }

    return luaReturnOK(L);
}

/**
 * DoAfterSpecial(seconds, text, sendto)
 *
 * Creates a temporary one-shot timer with a specified destination.
 *
 * Timer System
 * Based on DoAfterSpecial() from methods_timers.cpp
 *
 * @param seconds Delay in seconds (0.1 to 86399)
 * @param text Text/code to execute
 * @param sendto Destination (0-14): 0=world, 10=script, etc.
 * @return error code (eOK=0 on success)
 */
int L_DoAfterSpecial(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    double seconds = luaL_checknumber(L, 1);
    const char* text = luaL_checkstring(L, 2);
    int sendto = luaL_checknumber(L, 3);

    if (seconds < 0.1 || seconds > 86399.0) {
        return luaReturnError(L, eTimeInvalid);
    }

    if (sendto < 0 || sendto > 14) {
        return luaReturnError(L, eBadParameter);
    }

    // Generate unique name: "doafterspecial_<timestamp>_<counter>"
    static quint64 counter = 0;
    QString name =
        QString("doafterspecial_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++counter);

    // Create temporary one-shot interval timer
    auto timer = std::make_unique<Timer>();
    timer->strLabel = name;
    timer->iType = Timer::eInterval;
    timer->iEveryHour = 0;
    timer->iEveryMinute = 0;
    timer->fEverySecond = seconds;
    timer->strContents = QString::fromUtf8(text);
    timer->bEnabled = true;
    timer->bOneShot = true; // Delete after firing
    timer->bTemporary = true;
    timer->bActiveWhenClosed = true;
    timer->iSendTo = sendto;

    // Add to appropriate timer map (plugin or world)
    // When called from plugin with eSendToScript, timer must execute in plugin's Lua state
    // Use plugin(L) to get the plugin from Lua registry - this is reliable even after modal dialogs
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        Timer* rawTimer = timer.get(); // Get raw pointer before moving
        currentPlugin->m_TimerMap[name] = std::move(timer);
        currentPlugin->m_TimerRevMap[rawTimer] = name;
        // Initialize fire time for plugin timer
        pDoc->resetOneTimer(rawTimer);
    } else {
        if (!pDoc->addTimer(name, std::move(timer))) {
            return luaReturnError(L, eTimerAlreadyExists);
        }
    }

    return luaReturnOK(L);
}

/**
 * world.DoAfterNote(seconds, text)
 *
 * Creates a temporary one-shot timer that displays a note after a delay.
 *
 * @param seconds Delay in seconds
 * @param text Text to display as note
 * @return error code (eOK=0 on success)
 */
int L_DoAfterNote(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    double seconds = luaL_checknumber(L, 1);
    const char* text = luaL_checkstring(L, 2);

    if (seconds <= 0.0) {
        return luaReturnError(L, eTimeInvalid);
    }

    // Generate unique name
    static quint64 counter = 0;
    QString name =
        QString("doafternote_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++counter);

    // Create temporary one-shot interval timer
    auto timer = std::make_unique<Timer>();
    timer->strLabel = name;
    timer->iType = Timer::eInterval;
    timer->iEveryHour = 0;
    timer->iEveryMinute = 0;
    timer->fEverySecond = seconds;
    timer->strContents = QString::fromUtf8(text);
    timer->bEnabled = true;
    timer->bOneShot = true;
    timer->bTemporary = true;
    timer->bActiveWhenClosed = true;
    timer->iSendTo = eSendToOutput; // Send to output (note)

    if (!pDoc->addTimer(name, std::move(timer))) {
        return luaReturnError(L, eTimerAlreadyExists);
    }

    return luaReturnOK(L);
}

/**
 * world.DoAfterSpeedWalk(seconds, text)
 *
 * Creates a temporary one-shot timer that executes a speedwalk after a delay.
 *
 * @param seconds Delay in seconds
 * @param text Speedwalk string
 * @return error code (eOK=0 on success)
 */
int L_DoAfterSpeedWalk(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    double seconds = luaL_checknumber(L, 1);
    const char* text = luaL_checkstring(L, 2);

    if (seconds <= 0.0) {
        return luaReturnError(L, eTimeInvalid);
    }

    // Generate unique name
    static quint64 counter = 0;
    QString name =
        QString("doafterspeedwalk_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++counter);

    // Create temporary one-shot interval timer
    auto timer = std::make_unique<Timer>();
    timer->strLabel = name;
    timer->iType = Timer::eInterval;
    timer->iEveryHour = 0;
    timer->iEveryMinute = 0;
    timer->fEverySecond = seconds;
    timer->strContents = QString::fromUtf8(text);
    timer->bEnabled = true;
    timer->bOneShot = true;
    timer->bTemporary = true;
    timer->bActiveWhenClosed = true;
    timer->iSendTo = eSendToSpeedwalk;

    if (!pDoc->addTimer(name, std::move(timer))) {
        return luaReturnError(L, eTimerAlreadyExists);
    }

    return luaReturnOK(L);
}

/**
 * world.EnableTimerGroup(groupName, enabled)
 *
 * Enables or disables all timers in a group.
 *
 * @param groupName Timer group name
 * @param enabled true to enable, false to disable
 * @return Count of timers affected
 */
int L_EnableTimerGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);
    bool enabled = lua_toboolean(L, 2);

    QString qGroupName = QString::fromUtf8(groupName);
    int count = 0;

    // Iterate through all timers
    for (const auto& [name, timerPtr] : pDoc->m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (timer->strGroup == qGroupName) {
            timer->bEnabled = enabled;
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.DeleteTimerGroup(groupName)
 *
 * Deletes all timers in a group.
 *
 * @param groupName Timer group name
 * @return Count of timers deleted
 */
int L_DeleteTimerGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* groupName = luaL_checkstring(L, 1);

    QString qGroupName = QString::fromUtf8(groupName);
    QStringList toDelete;

    // Find all timers in this group
    for (const auto& [name, timerPtr] : pDoc->m_TimerMap) {
        if (timerPtr->strGroup == qGroupName) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteTimer(name);
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.DeleteTemporaryTimers()
 *
 * Deletes all temporary timers.
 *
 * @return Count of timers deleted
 */
int L_DeleteTemporaryTimers(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList toDelete;

    // Find all temporary timers
    for (const auto& [name, timerPtr] : pDoc->m_TimerMap) {
        if (timerPtr->bTemporary) {
            toDelete.append(name);
        }
    }

    // Delete them
    for (const QString& name : toDelete) {
        pDoc->deleteTimer(name);
    }

    lua_pushnumber(L, toDelete.size());
    return 1;
}

/**
 * world.GetTimerOption(name, optionName)
 *
 * Gets a timer option by name (alternative to GetTimerInfo).
 *
 * @param name Timer label
 * @param optionName Option name (e.g., "hour", "minute", "enabled")
 * @return Option value or nil if not found
 */
int L_GetTimerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(name);
    QString qOption = QString::fromUtf8(optionName).toLower();

    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        lua_pushnil(L);
        return 1;
    }

    // Map option names to values
    if (qOption == "hour") {
        if (timer->iType == Timer::eAtTime) {
            lua_pushnumber(L, timer->iAtHour);
        } else {
            lua_pushnumber(L, timer->iEveryHour);
        }
    } else if (qOption == "minute") {
        if (timer->iType == Timer::eAtTime) {
            lua_pushnumber(L, timer->iAtMinute);
        } else {
            lua_pushnumber(L, timer->iEveryMinute);
        }
    } else if (qOption == "second") {
        if (timer->iType == Timer::eAtTime) {
            lua_pushnumber(L, timer->fAtSecond);
        } else {
            lua_pushnumber(L, timer->fEverySecond);
        }
    } else if (qOption == "enabled") {
        lua_pushboolean(L, timer->bEnabled);
    } else if (qOption == "at_time") {
        lua_pushboolean(L, timer->iType == Timer::eAtTime);
    } else if (qOption == "one_shot") {
        lua_pushboolean(L, timer->bOneShot);
    } else if (qOption == "temporary") {
        lua_pushboolean(L, timer->bTemporary);
    } else if (qOption == "active_when_closed") {
        lua_pushboolean(L, timer->bActiveWhenClosed);
    } else if (qOption == "send_to") {
        lua_pushnumber(L, timer->iSendTo);
    } else if (qOption == "script") {
        QByteArray ba = timer->strProcedure.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "send") {
        QByteArray ba = timer->strContents.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "group") {
        QByteArray ba = timer->strGroup.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else if (qOption == "offset_hour") {
        lua_pushnumber(L, timer->iOffsetHour);
    } else if (qOption == "offset_minute") {
        lua_pushnumber(L, timer->iOffsetMinute);
    } else if (qOption == "offset_second") {
        lua_pushnumber(L, timer->fOffsetSecond);
    } else if (qOption == "user") {
        lua_pushnumber(L, timer->iUserOption);
    } else if (qOption == "omit_from_output") {
        lua_pushboolean(L, timer->bOmitFromOutput);
    } else if (qOption == "omit_from_log") {
        lua_pushboolean(L, timer->bOmitFromLog);
    } else if (qOption == "variable") {
        QByteArray ba = timer->strVariable.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

/**
 * world.SetTimerOption(name, optionName, value)
 *
 * Sets a timer option by name.
 *
 * @param name Timer label
 * @param optionName Option name (e.g., "hour", "minute", "enabled")
 * @param value New value
 * @return error code (eOK=0 on success)
 */
int L_SetTimerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* optionName = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(name);
    QString qOption = QString::fromUtf8(optionName).toLower();

    Timer* timer = nullptr;

    // Check appropriate timer map based on context
    Plugin* currentPlugin = plugin(L);
    if (currentPlugin) {
        auto it = currentPlugin->m_TimerMap.find(qName);
        if (it != currentPlugin->m_TimerMap.end()) {
            timer = it->second.get();
        }
    } else {
        timer = pDoc->getTimer(qName);
    }

    if (!timer) {
        return luaReturnError(L, eTimerNotFound);
    }

    // Set the option based on name
    if (qOption == "hour") {
        int value = luaL_checkinteger(L, 3);
        if (value < 0 || value > 23) {
            return luaReturnError(L, eTimeInvalid);
        }
        if (timer->iType == Timer::eAtTime) {
            timer->iAtHour = value;
        } else {
            timer->iEveryHour = value;
        }
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "minute") {
        int value = luaL_checkinteger(L, 3);
        if (value < 0 || value > 59) {
            return luaReturnError(L, eTimeInvalid);
        }
        if (timer->iType == Timer::eAtTime) {
            timer->iAtMinute = value;
        } else {
            timer->iEveryMinute = value;
        }
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "second") {
        double value = luaL_checknumber(L, 3);
        if (value < 0.0 || value >= 60.0) {
            return luaReturnError(L, eTimeInvalid);
        }
        if (timer->iType == Timer::eAtTime) {
            timer->fAtSecond = value;
        } else {
            timer->fEverySecond = value;
        }
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "enabled") {
        timer->bEnabled = lua_toboolean(L, 3);
    } else if (qOption == "at_time") {
        bool isAtTime = lua_toboolean(L, 3);
        timer->iType = isAtTime ? Timer::eAtTime : Timer::eInterval;
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "one_shot") {
        timer->bOneShot = lua_toboolean(L, 3);
    } else if (qOption == "temporary") {
        timer->bTemporary = lua_toboolean(L, 3);
    } else if (qOption == "active_when_closed") {
        timer->bActiveWhenClosed = lua_toboolean(L, 3);
    } else if (qOption == "send_to") {
        timer->iSendTo = luaL_checkinteger(L, 3);
    } else if (qOption == "script") {
        const char* value = luaL_checkstring(L, 3);
        timer->strProcedure = QString::fromUtf8(value);
    } else if (qOption == "send") {
        const char* value = luaL_checkstring(L, 3);
        timer->strContents = QString::fromUtf8(value);
    } else if (qOption == "group") {
        const char* value = luaL_checkstring(L, 3);
        timer->strGroup = QString::fromUtf8(value);
    } else if (qOption == "offset_hour") {
        int value = luaL_checkinteger(L, 3);
        if (value < 0 || value > 23) {
            return luaReturnError(L, eTimeInvalid);
        }
        timer->iOffsetHour = value;
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "offset_minute") {
        int value = luaL_checkinteger(L, 3);
        if (value < 0 || value > 59) {
            return luaReturnError(L, eTimeInvalid);
        }
        timer->iOffsetMinute = value;
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "offset_second") {
        double value = luaL_checknumber(L, 3);
        if (value < 0.0 || value >= 60.0) {
            return luaReturnError(L, eTimeInvalid);
        }
        timer->fOffsetSecond = value;
        pDoc->calculateNextFireTime(timer);
    } else if (qOption == "user") {
        timer->iUserOption = luaL_checkinteger(L, 3);
    } else if (qOption == "omit_from_output") {
        timer->bOmitFromOutput = lua_toboolean(L, 3);
    } else if (qOption == "omit_from_log") {
        timer->bOmitFromLog = lua_toboolean(L, 3);
    } else if (qOption == "variable") {
        const char* value = luaL_checkstring(L, 3);
        timer->strVariable = QString::fromUtf8(value);
    } else {
        return luaReturnError(L, eOK); // Unknown option, but don't fail
    }

    return luaReturnOK(L);
}

/**
 * GetPluginTimerList - Get list of timer names from plugin
 *
 * Lua signature: list = GetPluginTimerList(pluginID)
 *
 * Returns: Table of timer names (or empty table if plugin not found)
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginTimerList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    lua_newtable(L);

    if (plugin) {
        int index = 1;
        for (const auto& [name, timerPtr] : plugin->m_TimerMap) {
            lua_pushstring(L, name.toUtf8().constData());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

/**
 * GetPluginTimerInfo - Get timer info from plugin
 *
 * Lua signature: value = GetPluginTimerInfo(pluginID, timerName, infoType)
 *
 * Returns: Varies by infoType
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginTimerInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* timerName = luaL_checkstring(L, 2);
    int infoType = luaL_checkinteger(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find timer in plugin's map
    QString tName = QString::fromUtf8(timerName);
    Timer* timer = nullptr;
    auto it = plugin->m_TimerMap.find(tName);
    if (it != plugin->m_TimerMap.end()) {
        timer = it->second.get();
    }

    if (!timer) {
        lua_pushnil(L);
        pDoc->m_CurrentPlugin = savedPlugin;
        return 1;
    }

    // Return timer info using same cases as GetTimerInfo
    switch (infoType) {
        case 1: // hour (at-time hour or interval hour)
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->iAtHour);
            } else {
                lua_pushnumber(L, timer->iEveryHour);
            }
            break;
        case 2: // minute
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->iAtMinute);
            } else {
                lua_pushnumber(L, timer->iEveryMinute);
            }
            break;
        case 3: // second
            if (timer->iType == Timer::eAtTime) {
                lua_pushnumber(L, timer->fAtSecond);
            } else {
                lua_pushnumber(L, timer->fEverySecond);
            }
            break;
        case 4: // contents (send text)
        {
            QByteArray ba = timer->strContents.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 5: // strProcedure (script name)
        {
            QByteArray ba = timer->strProcedure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 6: // bOmitFromLog
            lua_pushboolean(L, timer->bOmitFromLog);
            break;
        case 7: // bEnabled
            lua_pushboolean(L, timer->bEnabled);
            break;
        case 8: // bAtTime
            lua_pushboolean(L, timer->iType == Timer::eAtTime);
            break;
        case 9: // bOneShot
            lua_pushboolean(L, timer->bOneShot);
            break;
        case 10: // bTemporary
            lua_pushboolean(L, timer->bTemporary);
            break;
        case 11: // interval hour (0 for at-time timers)
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->iEveryHour);
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 12: // interval minute
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->iEveryMinute);
            } else {
                lua_pushnumber(L, 0);
            }
            break;
        case 13: // interval second
            if (timer->iType == Timer::eInterval) {
                lua_pushnumber(L, timer->fEverySecond);
            } else {
                lua_pushnumber(L, 0.0);
            }
            break;
        case 14: // sequence (use nCreateSequence)
            lua_pushnumber(L, timer->nCreateSequence);
            break;
        case 15: // strGroup
        {
            QByteArray ba = timer->strGroup.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 16: // strVariable
        {
            QByteArray ba = timer->strVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;
        case 17: // iUserOption
            lua_pushnumber(L, timer->iUserOption);
            break;
        case 18: // bExecutingScript
            lua_pushboolean(L, timer->bExecutingScript);
            break;
        case 19: // has script (dispid != DISPID_UNKNOWN)
            lua_pushboolean(L, timer->dispid != -1);
            break;
        case 20: // nInvocationCount
            lua_pushnumber(L, timer->nInvocationCount);
            break;
        case 21: // nMatched
            lua_pushnumber(L, timer->nMatched);
            break;
        case 22: // tWhenFired
            if (timer->tWhenFired.isValid()) {
                lua_pushnumber(L, timer->tWhenFired.toSecsSinceEpoch());
            } else {
                lua_pushnil(L);
            }
            break;
        case 23: // iSendTo
            lua_pushnumber(L, timer->iSendTo);
            break;
        case 24: // bActiveWhenClosed
            lua_pushboolean(L, timer->bActiveWhenClosed);
            break;
        case 25: // time to next fire (in seconds)
            if (timer->tFireTime.isValid()) {
                qint64 msecs = QDateTime::currentDateTime().msecsTo(timer->tFireTime);
                lua_pushnumber(L, msecs / 1000.0);
            } else {
                lua_pushnumber(L, 0.0);
            }
            break;
        case 26: // at_time_string (formatted time)
            if (timer->iType == Timer::eAtTime) {
                QString timeStr = QString("%1:%2:%3")
                                      .arg(timer->iAtHour, 2, 10, QChar('0'))
                                      .arg(timer->iAtMinute, 2, 10, QChar('0'))
                                      .arg(timer->fAtSecond, 5, 'f', 2, QChar('0'));
                QByteArray ba = timeStr.toUtf8();
                lua_pushlstring(L, ba.constData(), ba.length());
            } else {
                lua_pushstring(L, "");
            }
            break;

        default:
            lua_pushnil(L);
            break;
    }

    // Restore context
    pDoc->m_CurrentPlugin = savedPlugin;

    return 1;
}

/**
 * GetPluginTimerOption - Get timer option from plugin
 *
 * Lua signature: value = GetPluginTimerOption(pluginID, timerName, optionName)
 *
 * Returns: Varies by option
 *
 * Based on methods_plugins.cpp
 */
int L_GetPluginTimerOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* pluginID = luaL_checkstring(L, 1);
    const char* timerName = luaL_checkstring(L, 2);
    const char* optionName = luaL_checkstring(L, 3);

    Plugin* plugin = pDoc->FindPluginByID(QString::fromUtf8(pluginID));

    if (!plugin) {
        lua_pushnil(L);
        return 1;
    }

    // Switch to plugin context
    Plugin* savedPlugin = pDoc->m_CurrentPlugin;
    pDoc->m_CurrentPlugin = plugin;

    // Find timer
    QString tName = QString::fromUtf8(timerName);
    Timer* timer = nullptr;
    auto it = plugin->m_TimerMap.find(tName);
    if (it != plugin->m_TimerMap.end()) {
        timer = it->second.get();
    }

    if (timer) {
        QString option = QString::fromUtf8(optionName);
        if (option == "enabled") {
            lua_pushboolean(L, timer->bEnabled);
        } else if (option == "at_time") {
            lua_pushboolean(L, timer->iType == Timer::eAtTime);
        } else if (option == "one_shot") {
            lua_pushboolean(L, timer->bOneShot);
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

// ========== Registration Function ==========

/**
 * register_timer_functions - Register all timer-related Lua functions
 *
 * This function is called from register_world_api() in lua_methods.cpp
 * to register timer functions in the luaL_Reg worldlib[] array.
 *
 * Note: The actual registration happens in lua_methods.cpp where the
 * luaL_Reg array is defined. This file only contains the implementations.
 */
void register_timer_functions(lua_State* L)
{
    // This is a placeholder function.
    // The actual registration is done in lua_methods.cpp via the luaL_Reg worldlib[] array.
    // Functions from this file are included in that array.
    (void)L; // Unused parameter
}
