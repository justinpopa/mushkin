// world_timers.cpp
// Timer Execution
//
// The timer registry (addTimer/deleteTimer/getTimer), fire-time calculation
// (resetOneTimer/resetAllTimers/calculateNextFireTime), and evaluation loop
// (checkTimers/checkTimerList/checkPluginTimerList) have been moved to
// AutomationRegistry (automation_registry.cpp).
//
// WorldDocument forwards all those calls via inline wrappers in world_document.h.
//
// This file retains executeTimer(), executeTimerScript(), executePluginTimer(),
// and executePluginTimerScript() which stay on WorldDocument because they are
// tightly coupled to its output/script/sendTo infrastructure.

#include "../automation/plugin.h" // For plugin timer support
#include "../automation/timer.h"
#include "logging.h"
#include "script_engine.h"
#include "world_document.h"
#include <QDateTime>
#include <QDebug>
#include <QTime>

// ========== Timer Execution ==========

// ExecuteTimer - Execute a fired timer
void WorldDocument::executeTimer(Timer* timer, const QString& name)
{
    QString strExtraOutput;

    timer->executing_script = true;
    m_iCurrentActionSource = eTimerAction;

    QString strDescription = QString("Timer: %1").arg(timer->label.isEmpty() ? name : timer->label);

    sendTo(timer->send_to, timer->contents, timer->omit_from_output, timer->omit_from_log,
           strDescription, timer->variable, strExtraOutput, timer->scriptLanguage);

    m_iCurrentActionSource = eUnknownActionSource;
    timer->executing_script = false;

    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    if (!timer->procedure.isEmpty()) {
        executeTimerScript(timer, name);
    }
}

// ExecuteTimerScript - Execute Lua script callback for a timer
void WorldDocument::executeTimerScript(Timer* timer, const QString& name)
{
    if (!m_ScriptEngine || !m_ScriptEngine->isLua()) {
        return;
    }
    if (timer->procedure.isEmpty()) {
        return;
    }

    qint32 dispid = timer->dispid.toInt();
    if (dispid == DISPID_UNKNOWN) {
        dispid = m_ScriptEngine->getLuaDispid(timer->procedure);
        timer->dispid = dispid;
        if (dispid == DISPID_UNKNOWN) {
            return;
        }
    }

    QList<double> nparams;
    QList<QString> sparams;
    QString timerName = timer->label.isEmpty() ? name : timer->label;
    sparams.append(timerName);

    timer->executing_script = true;

    QString strType = "timer";
    QString strReason = QString("processing timer \"%1\"").arg(timerName);
    long invocationCount = timer->invocation_count;

    bool error = m_ScriptEngine->executeLua(dispid, timer->procedure, eTimerAction, strType,
                                            strReason, nparams, sparams, invocationCount);

    timer->dispid = dispid;
    timer->invocation_count = invocationCount;
    timer->executing_script = false;

    if (error) {
        timer->dispid = DISPID_UNKNOWN;
    }

    qCDebug(lcWorld) << "Timer script executed:" << timer->procedure
                     << "invocations:" << timer->invocation_count;
}

// ========== Plugin Timer Execution ==========

void WorldDocument::executePluginTimer(Plugin* plugin, Timer* timer, const QString& name)
{
    QString strExtraOutput;

    timer->executing_script = true;
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = plugin;
    m_iCurrentActionSource = eTimerAction;

    QString strDescription = QString("Timer: %1").arg(timer->label.isEmpty() ? name : timer->label);

    sendTo(timer->send_to, timer->contents, timer->omit_from_output, timer->omit_from_log,
           strDescription, timer->variable, strExtraOutput, timer->scriptLanguage);

    m_iCurrentActionSource = eUnknownActionSource;
    m_CurrentPlugin = savedPlugin;
    timer->executing_script = false;

    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    if (!timer->procedure.isEmpty()) {
        executePluginTimerScript(plugin, timer, name);
    }
}

void WorldDocument::executePluginTimerScript(Plugin* plugin, Timer* timer, const QString& name)
{
    ScriptEngine* scriptEngine = plugin->scriptEngine();
    if (!scriptEngine || !scriptEngine->isLua()) {
        return;
    }
    if (timer->procedure.isEmpty()) {
        return;
    }

    qint32 dispid = timer->dispid.toInt();
    if (dispid == DISPID_UNKNOWN) {
        dispid = scriptEngine->getLuaDispid(timer->procedure);
        timer->dispid = dispid;
        if (dispid == DISPID_UNKNOWN) {
            return;
        }
    }

    QList<double> nparams;
    QList<QString> sparams;
    QString timerName = timer->label.isEmpty() ? name : timer->label;
    sparams.append(timerName);

    timer->executing_script = true;
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = plugin;

    QString strType = "timer";
    QString strReason = QString("processing timer \"%1\"").arg(timerName);
    long invocationCount = timer->invocation_count;

    bool error = scriptEngine->executeLua(dispid, timer->procedure, eTimerAction, strType,
                                          strReason, nparams, sparams, invocationCount);

    timer->dispid = dispid;
    timer->invocation_count = invocationCount;
    m_CurrentPlugin = savedPlugin;
    timer->executing_script = false;

    if (error) {
        timer->dispid = DISPID_UNKNOWN;
    }

    qCDebug(lcWorld) << "Plugin timer script executed:" << timer->procedure
                     << "plugin:" << plugin->name() << "invocations:" << timer->invocation_count;
}
