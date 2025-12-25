// world_timers.cpp
// Timer Fire Time Calculation
// Timer Execution
//
// Port of timers.cpp from original MUSHclient
// Implements ResetOneTimer() and ResetAllTimers() which calculate when timers should fire.

#include "../automation/plugin.h" // For plugin timer support
#include "../automation/timer.h"
#include "logging.h"
#include "script_engine.h" // For executeTimerScript
#include "world_document.h"
#include <QDateTime>
#include <QDebug>
#include <QTime>

// ResetOneTimer - Calculate when a timer should next fire
// Port of: timers.cpp (CMUSHclientDoc::ResetOneTimer)
//
// For at-time timers (eAtTime):
//   - Set fire time to today at the specified hour:minute:second
//   - If that time has already passed, move to tomorrow
//
// For interval timers (eInterval):
//   - Fire time = now + interval - offset
//   - Offset shifts the timing boundaries
//     Example: every 5 minutes with 2 minute offset fires at , , , etc.
//
// Always updates tWhenFired to track when the timer was reset.
void WorldDocument::resetOneTimer(Timer* timer)
{
    if (!timer->bEnabled) {
        return; // Ignore disabled timers
    }

    QDateTime now = QDateTime::currentDateTime();

    // Track when timer was reset
    timer->tWhenFired = now;

    // For at-time timers: fire at specific time each day
    if (timer->iType == Timer::eAtTime) {
        // Extract milliseconds from fractional seconds
        // Example: 30.5 seconds = 30 seconds + 500 milliseconds
        int seconds = static_cast<int>(timer->fAtSecond);
        int milliseconds = static_cast<int>((timer->fAtSecond - seconds) * 1000);

        // Construct time for today at the specified time
        QTime fireTime(timer->iAtHour, timer->iAtMinute, seconds, milliseconds);
        timer->tFireTime = QDateTime(now.date(), fireTime);

        // If this time has already passed today, move to tomorrow
        if (timer->tFireTime < now) {
            timer->tFireTime = timer->tFireTime.addDays(1);
        }
    } else {
        // For interval timers: now + interval - offset
        // Convert hours/minutes to seconds for addSecs()
        qint64 intervalSecs = static_cast<qint64>(timer->iEveryHour) * 3600 +
                              static_cast<qint64>(timer->iEveryMinute) * 60 +
                              static_cast<qint64>(timer->fEverySecond);

        qint64 offsetSecs = static_cast<qint64>(timer->iOffsetHour) * 3600 +
                            static_cast<qint64>(timer->iOffsetMinute) * 60 +
                            static_cast<qint64>(timer->fOffsetSecond);

        // Fire time = now + (interval - offset)
        timer->tFireTime = now.addSecs(intervalSecs - offsetSecs);
    }
}

// ResetAllTimers - Reset all timers in the timer map
// Port of: timers.cpp (CMUSHclientDoc::ResetAllTimers)
//
// Iterates through all timers and recalculates their fire times.
// Called when timers are loaded from file or when user requests reset.
void WorldDocument::resetAllTimers()
{
    for (auto& [name, timer] : m_TimerMap) {
        resetOneTimer(timer.get());
    }
}

// ========== Timer Evaluation Loop ==========

// CheckTimers - Housekeeping and main timer check entry point
// Port of: timers.cpp (CMUSHclientDoc::CheckTimers)
//
// Called every second by QTimer. Performs various housekeeping tasks:
// - Updates status line (every 5 seconds)
// - Flushes log file (every 120 seconds)
// - Auto-reconnect if disconnected
// - Checks and fires ready timers
void WorldDocument::checkTimers()
{
    // Flush log file to disk every 2 minutes
    // Uses close/reopen pattern instead of flush() for more reliable disk write
    if (m_logfile && m_logfile->isOpen()) {
        QDateTime now = QDateTime::currentDateTime();
        qint64 elapsed = m_LastFlushTime.secsTo(now);

        // Flush every 120 seconds (2 minutes)
        if (elapsed > 120) {
            m_LastFlushTime = now;

            // Close and reopen in append mode
            // This ensures data is written to disk (more reliable than flush())
            QString savedFileName = m_logfile_name;
            m_logfile->close();

            QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
            if (!m_logfile->open(mode)) {
                qCDebug(lcWorld) << "CheckTimers: Failed to reopen log file" << savedFileName;
            }
        }
    }

    // Only check timers if enabled
    if (!m_bEnableTimers) {
        return;
    }

    // Check main timer list
    checkTimerList();

    // Check plugin timers (Plugin DoAfterSpecial support)
    for (const auto& plugin : m_PluginList) {
        if (plugin && plugin->enabled()) {
            checkPluginTimerList(plugin.get());
        }
    }

    // Notify plugins of timer tick (for plugins that need periodic updates)
    SendToAllPluginCallbacks(ON_PLUGIN_TICK);
}

// CheckTimerList - Find and execute all ready timers
// Port of: timers.cpp (CMUSHclientDoc::CheckTimerList)
//
// Two-pass approach prevents issues when scripts modify timer list:
// Pass 1: Build list of timer names that are ready to fire
// Pass 2: Look up and execute each timer (may have been deleted by script)
//
// Timer firing logic:
// - Skip disabled timers
// - Skip timers with bActiveWhenClosed=false when disconnected
// - Update fire time BEFORE execution (prevents drift for long scripts)
// - For one-shot: disable BEFORE execution (prevents re-fire during dialog)
// - Delete one-shot timers AFTER execution if they still exist
void WorldDocument::checkTimerList()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList firedTimers;

    // Pass 1: Build list of ready timers
    for (const auto& [name, timerPtr] : m_TimerMap) {
        Timer* timer = timerPtr.get();

        if (!timer->bEnabled) {
            continue;
        }

        // Respect bActiveWhenClosed: skip if not connected and timer requires connection
        if (!timer->bActiveWhenClosed && m_iConnectPhase != eConnectConnectedToMud) {
            continue;
        }

        // Check if fire time has arrived
        if (timer->tFireTime > now) {
            continue;
        }

        // Timer is ready to fire
        firedTimers.append(name);
    }

    // Pass 2: Execute fired timers
    for (const QString& name : firedTimers) {
        Timer* timer = getTimer(name);
        if (!timer) {
            continue; // Script deleted it
        }

        // Update statistics
        timer->nMatched++;
        timer->tWhenFired = now;
        m_iTimersFiredCount++;
        m_iTimersFiredThisSessionCount++; // Track session-specific count

        // Update fire time BEFORE execution (so long-running scripts don't cause drift)
        if (timer->iType == Timer::eAtTime) {
            // At-time timer: add 1 day
            timer->tFireTime = timer->tFireTime.addDays(1);
        } else {
            // Interval timer: add interval
            qint64 intervalSecs = static_cast<qint64>(timer->iEveryHour) * 3600 +
                                  static_cast<qint64>(timer->iEveryMinute) * 60 +
                                  static_cast<qint64>(timer->fEverySecond);
            timer->tFireTime = timer->tFireTime.addSecs(intervalSecs);
        }

        // If new fire time is still in the past (clock changed?), recalculate
        if (timer->tFireTime <= now) {
            resetOneTimer(timer);
        }

        // Disable one-shot timers BEFORE execution (prevents re-fire if script shows dialog)
        if (timer->bOneShot) {
            timer->bEnabled = false;
        }

        // Execute the timer
        executeTimer(timer, name);

        // Check if timer still exists (script may have deleted it)
        if (m_TimerMap.find(name) == m_TimerMap.end()) {
            continue;
        }

        // Delete one-shot timers after execution
        if (timer->bOneShot) {
            // Remove from reverse map before deleting
            m_TimerRevMap.remove(timer);
            // Remove from map using erase() to avoid copy issues (unique_ptr automatically deletes)
            auto it = m_TimerMap.find(name);
            if (it != m_TimerMap.end()) {
                m_TimerMap.erase(it);
            }
        }
    }
}

// CheckPluginTimerList - Find and execute all ready plugin timers
// Similar to checkTimerList() but operates on a plugin's timer map
// and executes scripts in the plugin's Lua state
void WorldDocument::checkPluginTimerList(Plugin* plugin)
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList firedTimers;

    // Pass 1: Build list of ready timers
    for (const auto& [name, timerPtr] : plugin->m_TimerMap) {
        Timer* timer = timerPtr.get();

        if (!timer->bEnabled) {
            continue;
        }

        // Respect bActiveWhenClosed: skip if not connected and timer requires connection
        if (!timer->bActiveWhenClosed && m_iConnectPhase != eConnectConnectedToMud) {
            continue;
        }

        // Check if fire time has arrived
        if (timer->tFireTime > now) {
            continue;
        }

        // Timer is ready to fire
        firedTimers.append(name);
    }

    // Pass 2: Execute fired timers
    for (const QString& name : firedTimers) {
        auto it = plugin->m_TimerMap.find(name);
        if (it == plugin->m_TimerMap.end()) {
            continue; // Script deleted it
        }
        Timer* timer = it->second.get();

        // Update statistics
        timer->nMatched++;
        timer->tWhenFired = now;
        m_iTimersFiredCount++;
        m_iTimersFiredThisSessionCount++;

        // Update fire time BEFORE execution
        if (timer->iType == Timer::eAtTime) {
            timer->tFireTime = timer->tFireTime.addDays(1);
        } else {
            qint64 intervalSecs = static_cast<qint64>(timer->iEveryHour) * 3600 +
                                  static_cast<qint64>(timer->iEveryMinute) * 60 +
                                  static_cast<qint64>(timer->fEverySecond);
            timer->tFireTime = timer->tFireTime.addSecs(intervalSecs);
        }

        // If new fire time is still in the past, recalculate
        if (timer->tFireTime <= now) {
            resetOneTimer(timer);
        }

        // Disable one-shot timers BEFORE execution
        if (timer->bOneShot) {
            timer->bEnabled = false;
        }

        // Execute the timer in plugin's context
        executePluginTimer(plugin, timer, name);

        // Check if timer still exists
        auto checkIt = plugin->m_TimerMap.find(name);
        if (checkIt == plugin->m_TimerMap.end()) {
            continue;
        }

        // Delete one-shot timers after execution
        if (timer->bOneShot) {
            plugin->m_TimerRevMap.remove(timer);
            plugin->m_TimerMap.erase(checkIt); // unique_ptr automatically deletes
        }
    }
}

// ========== Timer Execution ==========

// ExecuteTimer - Execute a fired timer
// Port of: timers.cpp (timer execution logic in CheckTimerList)
//
// Executes timer actions when timer fires:
// - Calls SendTo() to route timer contents
// - Displays output (if eSendToOutput was used)
// - Calls Lua callback if strProcedure is set
// - Prevents deletion during script execution (bExecutingScript flag)
//
// @param timer The timer that fired
// @param name Timer name (from map key)
void WorldDocument::executeTimer(Timer* timer, const QString& name)
{
    QString strExtraOutput; // Accumulates eSendToOutput text

    // Prevent deletion during execution
    timer->bExecutingScript = true;

    // Set action source so scripts know they're being called from a timer
    m_iCurrentActionSource = eTimerAction;

    // Build description for SendTo
    QString strDescription =
        QString("Timer: %1").arg(timer->strLabel.isEmpty() ? name : timer->strLabel);

    // Call SendTo() to route timer contents to destination
    sendTo(timer->iSendTo, timer->strContents, timer->bOmitFromOutput, timer->bOmitFromLog,
           strDescription, timer->strVariable, strExtraOutput);

    // Reset action source
    m_iCurrentActionSource = eUnknownActionSource;

    // Allow deletion again
    timer->bExecutingScript = false;

    // Display any output that was accumulated
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Execute Lua callback if specified
    if (!timer->strProcedure.isEmpty()) {
        executeTimerScript(timer, name);
    }
}

// ExecuteTimerScript - Execute Lua script callback for a timer
// Port of: timers.cpp (Lua timer script execution)
//
// Calls the Lua function specified in timer->strProcedure with parameters:
// 1. Timer name (string)
//
// @param timer The timer that fired
// @param name Timer name (from map key, used if strLabel is empty)
void WorldDocument::executeTimerScript(Timer* timer, const QString& name)
{
    // Safety check - need a script engine
    if (!m_ScriptEngine || !m_ScriptEngine->isLua()) {
        return;
    }

    // Safety check - need a procedure name
    if (timer->strProcedure.isEmpty()) {
        return;
    }

    // Check if function exists
    // dispid caching: DISPID_UNKNOWN = check needed, 1 = exists (cached)
    // We re-check each time it's DISPID_UNKNOWN (allows function to be defined later)
    qint32 dispid = timer->dispid.toInt();
    if (dispid == DISPID_UNKNOWN) {
        dispid = m_ScriptEngine->getLuaDispid(timer->strProcedure);
        timer->dispid = dispid;

        if (dispid == DISPID_UNKNOWN) {
            return; // Function doesn't exist, skip it
        }
    }

    // Prepare parameters for executeLua
    QList<double> nparams; // No numeric parameters
    QList<QString> sparams;

    // Parameter 1: Timer name (use label if set, otherwise internal name)
    QString timerName = timer->strLabel.isEmpty() ? name : timer->strLabel;
    sparams.append(timerName);

    // Prevent deletion during script execution
    timer->bExecutingScript = true;

    // Build description strings
    QString strType = "timer";
    QString strReason = QString("processing timer \"%1\"").arg(timerName);

    // Convert nInvocationCount to long for executeLua (expects long&)
    long invocationCount = timer->nInvocationCount;

    // Execute Lua function
    bool error = m_ScriptEngine->executeLua(dispid,              // IN/OUT: function exists flag
                                            timer->strProcedure, // function name
                                            eTimerAction,        // action source
                                            strType,             // "timer"
                                            strReason,           // description
                                            nparams,             // no numeric params
                                            sparams,             // timer name
                                            invocationCount      // IN/OUT: invocation counter
    );

    // Copy back the updated values
    timer->dispid = dispid;
    timer->nInvocationCount = invocationCount;

    // Allow deletion again
    timer->bExecutingScript = false;

    // If function failed, mark it as DISPID_UNKNOWN so we don't keep trying
    if (error) {
        timer->dispid = DISPID_UNKNOWN;
    }

    qCDebug(lcWorld) << "Timer script executed:" << timer->strProcedure
                     << "invocations:" << timer->nInvocationCount;
}

// ========== Timer Management Functions ==========

// addTimer - Add a timer to the map
// Similar to addTrigger/addAlias implementations
// Takes ownership of the timer via unique_ptr
bool WorldDocument::addTimer(const QString& name, std::unique_ptr<Timer> timer)
{
    if (m_TimerMap.find(name) != m_TimerMap.end()) {
        return false; // Timer with this name already exists
    }

    // Calculate the initial fire time
    calculateNextFireTime(timer.get());

    // Add to map (takes ownership)
    m_TimerMap[name] = std::move(timer);

    return true;
}

// deleteTimer - Delete a timer by name
bool WorldDocument::deleteTimer(const QString& name)
{
    auto it = m_TimerMap.find(name);
    if (it == m_TimerMap.end()) {
        return false; // Timer doesn't exist
    }

    Timer* timer = it->second.get();

    // Don't delete if currently executing
    if (timer && timer->bExecutingScript) {
        return false;
    }

    // Remove from map (unique_ptr automatically deletes)
    m_TimerMap.erase(it);

    return true;
}

// getTimer - Get a timer by name
Timer* WorldDocument::getTimer(const QString& name)
{
    auto it = m_TimerMap.find(name);
    return (it != m_TimerMap.end()) ? it->second.get() : nullptr;
}

// calculateNextFireTime - Calculate when a timer should next fire
// This is an alias for resetOneTimer to match the API naming in lua_methods.cpp
void WorldDocument::calculateNextFireTime(Timer* timer)
{
    resetOneTimer(timer);
}

// ========== Plugin Timer Execution (Plugin DoAfterSpecial) ==========

// ExecutePluginTimer - Execute a plugin timer in the plugin's Lua state
// Similar to executeTimer() but uses plugin's script engine for eSendToScript
void WorldDocument::executePluginTimer(Plugin* plugin, Timer* timer, const QString& name)
{
    QString strExtraOutput;

    // Prevent deletion during execution
    timer->bExecutingScript = true;

    // Set current plugin context
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = plugin;

    // Set action source
    m_iCurrentActionSource = eTimerAction;

    // Build description for SendTo
    QString strDescription =
        QString("Timer: %1").arg(timer->strLabel.isEmpty() ? name : timer->strLabel);

    // Call SendTo() - for eSendToScript, sendTo will use the current plugin's Lua state
    sendTo(timer->iSendTo, timer->strContents, timer->bOmitFromOutput, timer->bOmitFromLog,
           strDescription, timer->strVariable, strExtraOutput);

    // Reset action source and plugin context
    m_iCurrentActionSource = eUnknownActionSource;
    m_CurrentPlugin = savedPlugin;

    // Allow deletion again
    timer->bExecutingScript = false;

    // Display any output
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Execute Lua callback if specified (uses plugin's Lua state)
    if (!timer->strProcedure.isEmpty()) {
        executePluginTimerScript(plugin, timer, name);
    }
}

// ExecutePluginTimerScript - Execute Lua callback in plugin's Lua state
// Similar to executeTimerScript() but uses plugin's script engine
void WorldDocument::executePluginTimerScript(Plugin* plugin, Timer* timer, const QString& name)
{
    // Safety check - need a script engine
    ScriptEngine* scriptEngine = plugin->scriptEngine();
    if (!scriptEngine || !scriptEngine->isLua()) {
        return;
    }

    // Safety check - need a procedure name
    if (timer->strProcedure.isEmpty()) {
        return;
    }

    // Check if function exists
    qint32 dispid = timer->dispid.toInt();
    if (dispid == DISPID_UNKNOWN) {
        dispid = scriptEngine->getLuaDispid(timer->strProcedure);
        timer->dispid = dispid;

        if (dispid == DISPID_UNKNOWN) {
            return; // Function doesn't exist
        }
    }

    // Prepare parameters
    QList<double> nparams;
    QList<QString> sparams;

    QString timerName = timer->strLabel.isEmpty() ? name : timer->strLabel;
    sparams.append(timerName);

    // Prevent deletion during script execution
    timer->bExecutingScript = true;

    // Set current plugin context
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = plugin;

    // Build description strings
    QString strType = "timer";
    QString strReason = QString("processing timer \"%1\"").arg(timerName);

    long invocationCount = timer->nInvocationCount;

    // Execute Lua function in plugin's Lua state
    bool error = scriptEngine->executeLua(dispid, timer->strProcedure, eTimerAction, strType,
                                          strReason, nparams, sparams, invocationCount);

    // Copy back updated values
    timer->dispid = dispid;
    timer->nInvocationCount = invocationCount;

    // Restore context
    m_CurrentPlugin = savedPlugin;

    // Allow deletion again
    timer->bExecutingScript = false;

    // If function failed, mark it as DISPID_UNKNOWN
    if (error) {
        timer->dispid = DISPID_UNKNOWN;
    }

    qCDebug(lcWorld) << "Plugin timer script executed:" << timer->strProcedure
                     << "plugin:" << plugin->name() << "invocations:" << timer->nInvocationCount;
}
