/**
 * automation_registry.cpp — AutomationRegistry companion object implementation
 *
 * Owns trigger/alias/timer storage and provides registry operations and the
 * evaluation pipeline (evaluateTriggers, evaluateAliases, checkTimerList).
 *
 * Execution callbacks (executeTrigger, executeAlias, executeTimer) remain on
 * WorldDocument and are called through the m_doc back-reference.
 */

#include "automation_registry.h"

#include "../automation/alias.h"
#include "../automation/plugin.h"
#include "../automation/timer.h"
#include "../automation/trigger.h"
#include "../text/line.h"
#include "../text/style.h"
#include "logging.h"
#include "world_document.h"
#include "world_error.h"

#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <QTime>
#include <algorithm>
#include <expected>

// ============================================================
// Constructor
// ============================================================

AutomationRegistry::AutomationRegistry(WorldDocument& doc) : m_doc(doc)
{
}

// ============================================================
// Trigger Registry
// ============================================================

std::expected<void, WorldError> AutomationRegistry::addTrigger(const QString& name,
                                                               std::unique_ptr<Trigger> trigger)
{
    if (m_TriggerMap.find(name) != m_TriggerMap.end()) {
        qWarning() << "Trigger already exists with name:" << name;
        return std::unexpected(WorldError{WorldErrorType::AlreadyExists,
                                          QString("Trigger already exists: %1").arg(name)});
    }

    trigger->internal_name = name;
    Trigger* rawPtr = trigger.get();
    m_TriggerMap[name] = std::move(trigger);
    m_TriggerArray.append(rawPtr);
    m_triggersNeedSorting = true;

    qCDebug(lcWorld) << "Added trigger:" << name << "sequence:" << rawPtr->sequence;
    return {};
}

std::expected<void, WorldError> AutomationRegistry::deleteTrigger(const QString& name)
{
    auto it = m_TriggerMap.find(name);
    if (it == m_TriggerMap.end()) {
        qWarning() << "Trigger not found:" << name;
        return std::unexpected(
            WorldError{WorldErrorType::NotFound, QString("Trigger not found: %1").arg(name)});
    }

    Trigger* trigger = it->second.get();
    if (trigger->executing_script) {
        qCDebug(lcWorld) << "Cannot delete trigger while script is executing:" << name;
        return std::unexpected(WorldError{WorldErrorType::ScriptExecuting,
                                          QString("Trigger script is executing: %1").arg(name)});
    }

    m_TriggerArray.removeOne(trigger);
    m_TriggerMap.erase(it);
    m_triggersNeedSorting = true;

    qCDebug(lcWorld) << "Deleted trigger:" << name;
    return {};
}

Trigger* AutomationRegistry::getTrigger(const QString& name)
{
    auto it = m_TriggerMap.find(name);
    return (it != m_TriggerMap.end()) ? it->second.get() : nullptr;
}

// ============================================================
// Alias Registry
// ============================================================

std::expected<void, WorldError> AutomationRegistry::addAlias(const QString& name,
                                                             std::unique_ptr<Alias> alias)
{
    if (m_AliasMap.find(name) != m_AliasMap.end()) {
        qWarning() << "Alias already exists with name:" << name;
        return std::unexpected(WorldError{WorldErrorType::AlreadyExists,
                                          QString("Alias already exists: %1").arg(name)});
    }

    alias->internal_name = name;
    Alias* rawPtr = alias.get();
    m_AliasMap[name] = std::move(alias);
    m_AliasArray.append(rawPtr);
    m_aliasesNeedSorting = true;

    qCDebug(lcWorld) << "Added alias:" << name << "sequence:" << rawPtr->sequence;
    return {};
}

std::expected<void, WorldError> AutomationRegistry::deleteAlias(const QString& name)
{
    auto it = m_AliasMap.find(name);
    if (it == m_AliasMap.end()) {
        qWarning() << "Alias not found:" << name;
        return std::unexpected(
            WorldError{WorldErrorType::NotFound, QString("Alias not found: %1").arg(name)});
    }

    Alias* alias = it->second.get();
    if (alias->executing_script) {
        qCDebug(lcWorld) << "Cannot delete alias while script is executing:" << name;
        return std::unexpected(WorldError{WorldErrorType::ScriptExecuting,
                                          QString("Alias script is executing: %1").arg(name)});
    }

    m_AliasArray.removeOne(alias);
    m_AliasMap.erase(it);
    m_aliasesNeedSorting = true;

    qCDebug(lcWorld) << "Deleted alias:" << name;
    return {};
}

Alias* AutomationRegistry::getAlias(const QString& name)
{
    auto it = m_AliasMap.find(name);
    return (it != m_AliasMap.end()) ? it->second.get() : nullptr;
}

// ============================================================
// Timer Registry
// ============================================================

std::expected<void, WorldError> AutomationRegistry::addTimer(const QString& name,
                                                             std::unique_ptr<Timer> timer)
{
    if (m_TimerMap.find(name) != m_TimerMap.end()) {
        return std::unexpected(WorldError{WorldErrorType::AlreadyExists,
                                          QString("Timer already exists: %1").arg(name)});
    }

    resetOneTimer(timer.get());
    m_TimerMap[name] = std::move(timer);
    return {};
}

std::expected<void, WorldError> AutomationRegistry::deleteTimer(const QString& name)
{
    auto it = m_TimerMap.find(name);
    if (it == m_TimerMap.end()) {
        return std::unexpected(
            WorldError{WorldErrorType::NotFound, QString("Timer not found: %1").arg(name)});
    }

    Timer* timer = it->second.get();
    if (timer && timer->executing_script) {
        return std::unexpected(WorldError{WorldErrorType::ScriptExecuting,
                                          QString("Timer script is executing: %1").arg(name)});
    }

    m_TimerMap.erase(it);
    return {};
}

Timer* AutomationRegistry::getTimer(const QString& name)
{
    auto it = m_TimerMap.find(name);
    return (it != m_TimerMap.end()) ? it->second.get() : nullptr;
}

// ============================================================
// Trigger Array Rebuild
// ============================================================

void AutomationRegistry::rebuildTriggerArray()
{
    m_TriggerArray.clear();
    for (const auto& [name, triggerPtr] : m_TriggerMap) {
        m_TriggerArray.append(triggerPtr.get());
    }
    std::sort(m_TriggerArray.begin(), m_TriggerArray.end(),
              [](const Trigger* a, const Trigger* b) { return a->sequence < b->sequence; });
    m_triggersNeedSorting = false;
}

// ============================================================
// Alias Array Rebuild
// ============================================================

void AutomationRegistry::rebuildAliasArray()
{
    m_AliasArray.clear();
    for (const auto& [name, aliasPtr] : m_AliasMap) {
        m_AliasArray.append(aliasPtr.get());
    }
    std::sort(m_AliasArray.begin(), m_AliasArray.end(),
              [](const Alias* a, const Alias* b) { return a->sequence < b->sequence; });
    m_aliasesNeedSorting = false;
}

// ============================================================
// Trigger Evaluation Pipeline
// (internals copied from world_trigger_matching.cpp)
// ============================================================

// --- static helpers (file-local) ---

static QRegularExpression wildcardToRegex(const QString& pattern, bool ignoreCase)
{
    QString escaped = QRegularExpression::escape(pattern);
    escaped.replace("\\*", "(.*?)");
    QString fullPattern = "^" + escaped + "$";
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignoreCase) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    return QRegularExpression(fullPattern, options);
}

// TRIGGER_MATCH_* constants are defined as inline constexpr in trigger.h

static bool matchStyle(Trigger* trigger, Line* line)
{
    if (trigger->match_type == 0 && trigger->style == 0) {
        return true;
    }
    for (const auto& style : line->styleList) {
        bool matches = true;
        if (trigger->match_type & TRIGGER_MATCH_TEXT) {
            if (trigger->other_foreground != 0) {
                if (style->iForeColour != trigger->other_foreground) {
                    matches = false;
                }
            }
        }
        if (trigger->match_type & TRIGGER_MATCH_BACK) {
            if (trigger->other_background != 0) {
                if (style->iBackColour != trigger->other_background) {
                    matches = false;
                }
            }
        }
        if (trigger->match_type & TRIGGER_MATCH_HILITE) {
            if (!(style->iFlags & HILITE)) {
                matches = false;
            }
        }
        if (trigger->match_type & TRIGGER_MATCH_UNDERLINE) {
            if (!(style->iFlags & UNDERLINE)) {
                matches = false;
            }
        }
        if (trigger->match_type & TRIGGER_MATCH_BLINK) {
            if (!(style->iFlags & BLINK)) {
                matches = false;
            }
        }
        if (trigger->match_type & TRIGGER_MATCH_INVERSE) {
            if (!(style->iFlags & INVERSE)) {
                matches = false;
            }
        }
        if (matches) {
            return true;
        }
    }
    return false;
}

static bool matchTriggerPattern(Trigger* trigger, const QString& text)
{
    if (trigger->use_regexp) {
        if (!trigger->regexp) {
            if (!trigger->compileRegexp().has_value()) {
                return false;
            }
        }
        QRegularExpressionMatch match = trigger->regexp->match(text);
        if (!match.hasMatch()) {
            return false;
        }
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);
            if (trigger->lowercase_wildcard && i > 0) {
                captured = captured.toLower();
            }
            trigger->wildcards[i] = captured;
        }
        trigger->namedWildcards.clear();
        const QStringList namedGroups = trigger->regexp->namedCaptureGroups();
        for (const QString& name : namedGroups) {
            if (!name.isEmpty()) {
                QString captured = match.captured(name);
                if (trigger->lowercase_wildcard) {
                    captured = captured.toLower();
                }
                trigger->namedWildcards[name] = captured;
            }
        }
        return true;
    } else {
        QRegularExpression re = wildcardToRegex(trigger->trigger, trigger->ignore_case);
        QRegularExpressionMatch match = re.match(text);
        if (!match.hasMatch()) {
            return false;
        }
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);
            if (trigger->lowercase_wildcard && i > 0) {
                captured = captured.toLower();
            }
            trigger->wildcards[i] = captured;
        }
        return true;
    }
}

static bool matchTriggerWithRepeat(Trigger* trigger, const QString& text, Line* line,
                                   WorldDocument* doc)
{
    if (!matchStyle(trigger, line)) {
        return false;
    }
    if (!trigger->repeat) {
        if (matchTriggerPattern(trigger, text)) {
            doc->executeTrigger(trigger, line, text);
            return true;
        }
        return false;
    }

    bool anyMatch = false;
    int offset = 0;
    QRegularExpression re;
    if (trigger->use_regexp) {
        if (!trigger->regexp) {
            if (!trigger->compileRegexp().has_value()) {
                return false;
            }
        }
        re = *trigger->regexp;
    } else {
        re = wildcardToRegex(trigger->trigger, trigger->ignore_case);
    }

    while (offset < text.length()) {
        QRegularExpressionMatch match = re.match(text, offset);
        if (!match.hasMatch()) {
            break;
        }
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);
            if (trigger->lowercase_wildcard && i > 0) {
                captured = captured.toLower();
            }
            trigger->wildcards[i] = captured;
        }
        doc->executeTrigger(trigger, line, text);
        anyMatch = true;
        offset = match.capturedEnd();
        if (offset <= match.capturedStart()) {
            offset++;
        }
    }
    return anyMatch;
}

// Helper: evaluate one sorted trigger array (world or plugin).
// Returns true if evaluation should stop.
static bool evaluateOneTriggerSequence(const QVector<Trigger*>& triggerArray, Line* line,
                                       const QString& lineText, WorldDocument* doc,
                                       AutomationRegistry* registry, QString& oneShotToDelete)
{
    for (Trigger* trigger : triggerArray) {
        if (!trigger->enabled) {
            continue;
        }

        bool matched = false;
        if (trigger->multi_line && trigger->lines_to_match > 1) {
            QString multiLineText;
            int bufferSize = static_cast<int>(doc->m_recentLines.size());
            int startPos = bufferSize - trigger->lines_to_match;
            if (startPos < 0) {
                startPos = 0;
            }
            for (int i = startPos; i < bufferSize; ++i) {
                multiLineText += doc->m_recentLines[static_cast<size_t>(i)];
                multiLineText += '\n';
            }
            matched = matchTriggerWithRepeat(trigger, multiLineText, line, doc);
        } else {
            matched = matchTriggerWithRepeat(trigger, lineText, line, doc);
        }

        if (matched) {
            registry->m_iTriggersMatchedCount++;
            registry->m_iTriggersMatchedThisSessionCount++;

            if (trigger->one_shot) {
                oneShotToDelete = trigger->internal_name;
                return true;
            }
            if (!trigger->keep_evaluating) {
                return true;
            }
        }
    }
    return false;
}

void AutomationRegistry::evaluateTriggers(Line* line)
{
    if (!line || line->len() == 0) {
        return;
    }

    QString lineText = QString::fromUtf8(line->text().data(), line->text().size());

    if (!m_doc.SendToAllPluginCallbacks(ON_PLUGIN_LINE_RECEIVED, lineText, true)) {
        return;
    }

    if (m_triggersNeedSorting) {
        rebuildTriggerArray();
    }

    if (!m_doc.m_enable_triggers) {
        return;
    }

    m_iTriggersEvaluatedCount += m_TriggerArray.size();

    QString oneShotToDelete;
    Plugin* savedPlugin = m_doc.m_CurrentPlugin;
    m_doc.m_CurrentPlugin = nullptr;
    bool stopEvaluation = false;

    // Phase 1: plugins with negative sequence
    for (const auto& plugin : m_doc.m_PluginList) {
        if (plugin->m_iSequence >= 0) {
            break;
        }
        if (!plugin->m_bEnabled) {
            continue;
        }
        if (plugin->m_triggersNeedSorting) {
            plugin->rebuildTriggerArray();
        }
        m_doc.m_CurrentPlugin = plugin.get();
        m_iTriggersEvaluatedCount += plugin->m_TriggerArray.size();
        stopEvaluation = evaluateOneTriggerSequence(plugin->m_TriggerArray, line, lineText, &m_doc,
                                                    this, oneShotToDelete);
        if (stopEvaluation) {
            if (!oneShotToDelete.isEmpty()) {
                plugin->m_TriggerMap.erase(oneShotToDelete);
                plugin->m_triggersNeedSorting = true;
            }
            m_doc.m_CurrentPlugin = savedPlugin;
            return;
        }
    }

    // Phase 2: world triggers
    m_doc.m_CurrentPlugin = nullptr;
    stopEvaluation =
        evaluateOneTriggerSequence(m_TriggerArray, line, lineText, &m_doc, this, oneShotToDelete);
    if (stopEvaluation) {
        if (!oneShotToDelete.isEmpty()) {
            qCDebug(lcWorld) << "Deleting one-shot world trigger:" << oneShotToDelete;
            (void)deleteTrigger(oneShotToDelete);
        }
        m_doc.m_CurrentPlugin = savedPlugin;
        return;
    }

    // Phase 3: plugins with zero/positive sequence
    for (const auto& plugin : m_doc.m_PluginList) {
        if (plugin->m_iSequence < 0) {
            continue;
        }
        if (!plugin->m_bEnabled) {
            continue;
        }
        if (plugin->m_triggersNeedSorting) {
            plugin->rebuildTriggerArray();
        }
        m_doc.m_CurrentPlugin = plugin.get();
        m_iTriggersEvaluatedCount += plugin->m_TriggerArray.size();
        stopEvaluation = evaluateOneTriggerSequence(plugin->m_TriggerArray, line, lineText, &m_doc,
                                                    this, oneShotToDelete);
        if (stopEvaluation) {
            if (!oneShotToDelete.isEmpty()) {
                plugin->m_TriggerMap.erase(oneShotToDelete);
                plugin->m_triggersNeedSorting = true;
            }
            m_doc.m_CurrentPlugin = savedPlugin;
            return;
        }
    }

    m_doc.m_CurrentPlugin = savedPlugin;
}

// ============================================================
// Alias Evaluation Pipeline
// (internals copied from world_alias_execution.cpp)
// ============================================================

static bool evaluateOneAliasSequence(const QVector<Alias*>& aliasArray, const QString& command,
                                     WorldDocument* doc, bool& anyMatched)
{
    for (Alias* alias : aliasArray) {
        if (!alias->enabled) {
            continue;
        }
        bool matched = alias->match(command);
        if (matched) {
            qDebug() << "Alias MATCHED:" << alias->label << "pattern:" << alias->name
                     << "script:" << alias->procedure;
            anyMatched = true;
            doc->executeAlias(alias, command);
            if (!alias->keep_evaluating) {
                return true;
            }
        }
    }
    return false;
}

bool AutomationRegistry::evaluateAliases(const QString& command)
{
    if (command.isEmpty()) {
        return false;
    }

    qDebug() << "evaluateAliases: command =" << command;

    if (m_aliasesNeedSorting) {
        rebuildAliasArray();
    }

    Plugin* savedPlugin = m_doc.m_CurrentPlugin;
    m_doc.m_CurrentPlugin = nullptr;
    bool stopEvaluation = false;
    bool anyMatched = false;

    // Phase 1: plugins with negative sequence
    for (const auto& plugin : m_doc.m_PluginList) {
        if (plugin->m_iSequence >= 0) {
            break;
        }
        if (!plugin->m_bEnabled) {
            continue;
        }
        if (plugin->m_aliasesNeedSorting) {
            plugin->rebuildAliasArray();
        }
        m_doc.m_CurrentPlugin = plugin.get();
        stopEvaluation =
            evaluateOneAliasSequence(plugin->m_AliasArray, command, &m_doc, anyMatched);
        if (stopEvaluation) {
            m_doc.m_CurrentPlugin = savedPlugin;
            return true;
        }
    }

    // Phase 2: world aliases
    m_doc.m_CurrentPlugin = nullptr;
    stopEvaluation = evaluateOneAliasSequence(m_AliasArray, command, &m_doc, anyMatched);
    if (stopEvaluation) {
        m_doc.m_CurrentPlugin = savedPlugin;
        return true;
    }

    // Phase 3: plugins with zero/positive sequence
    for (const auto& plugin : m_doc.m_PluginList) {
        if (plugin->m_iSequence < 0) {
            continue;
        }
        if (!plugin->m_bEnabled) {
            continue;
        }
        if (plugin->m_aliasesNeedSorting) {
            plugin->rebuildAliasArray();
        }
        m_doc.m_CurrentPlugin = plugin.get();
        stopEvaluation =
            evaluateOneAliasSequence(plugin->m_AliasArray, command, &m_doc, anyMatched);
        if (stopEvaluation) {
            m_doc.m_CurrentPlugin = savedPlugin;
            return true;
        }
    }

    m_doc.m_CurrentPlugin = savedPlugin;

    if (anyMatched) {
        qDebug() << "evaluateAliases: Alias(es) matched and handled command:" << command;
        return true;
    }
    qDebug() << "evaluateAliases: No alias matched, sending to MUD:" << command;
    return false;
}

// ============================================================
// Timer Management
// (internals copied from world_timers.cpp)
// ============================================================

void AutomationRegistry::resetOneTimer(Timer* timer)
{
    if (!timer->enabled) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    timer->when_fired = now;

    if (timer->type == Timer::TimerType::AtTime) {
        int seconds = static_cast<int>(timer->at_second);
        int milliseconds = static_cast<int>((timer->at_second - seconds) * 1000);
        QTime fireTime(timer->at_hour, timer->at_minute, seconds, milliseconds);
        timer->fire_time = QDateTime(now.date(), fireTime);
        if (timer->fire_time < now) {
            timer->fire_time = timer->fire_time.addDays(1);
        }
    } else {
        // Compute interval and offset in milliseconds to preserve fractional seconds.
        // every_second / offset_second are double (e.g. 0.5 = 500 ms); truncating them
        // to qint64 before multiplying would silently drop sub-second precision.
        qint64 intervalMs = static_cast<qint64>(timer->every_hour) * 3600000LL +
                            static_cast<qint64>(timer->every_minute) * 60000LL +
                            static_cast<qint64>(timer->every_second * 1000.0);
        qint64 offsetMs = static_cast<qint64>(timer->offset_hour) * 3600000LL +
                          static_cast<qint64>(timer->offset_minute) * 60000LL +
                          static_cast<qint64>(timer->offset_second * 1000.0);
        // The offset shortens the initial delay so that subsequent firings land on the
        // phase (offset mod interval) within each period.  Guard against a mis-configured
        // offset that equals or exceeds the interval: clamp to at least 1 ms so we never
        // schedule a fire in the past (which would cause a busy-loop on every tick).
        qint64 delayMs = intervalMs - offsetMs;
        if (delayMs < 1) {
            delayMs = intervalMs > 0 ? intervalMs : 1000LL;
        }
        timer->fire_time = now.addMSecs(delayMs);
    }
}

void AutomationRegistry::resetAllTimers()
{
    for (auto& [name, timer] : m_TimerMap) {
        resetOneTimer(timer.get());
    }
}

void AutomationRegistry::checkTimers()
{
    // Flush log file to disk every 2 minutes
    if (m_doc.m_logfile && m_doc.m_logfile->isOpen()) {
        QDateTime now = QDateTime::currentDateTime();
        qint64 elapsed = m_doc.m_LastFlushTime.secsTo(now);
        if (elapsed > 120) {
            m_doc.m_LastFlushTime = now;
            QString savedFileName = m_doc.m_logfile_name;
            m_doc.m_logfile->close();
            QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
            if (!m_doc.m_logfile->open(mode)) {
                qCDebug(lcWorld) << "CheckTimers: Failed to reopen log file" << savedFileName;
            }
        }
    }

    if (!m_doc.m_bEnableTimers) {
        return;
    }

    checkTimerList();

    for (const auto& plugin : m_doc.m_PluginList) {
        if (plugin && plugin->enabled()) {
            checkPluginTimerList(plugin.get());
        }
    }

    m_doc.SendToAllPluginCallbacks(ON_PLUGIN_TICK);
}

void AutomationRegistry::checkTimerList()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList firedTimers;

    for (const auto& [name, timerPtr] : m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (!timer->enabled) {
            continue;
        }
        if (!timer->active_when_closed &&
            m_doc.m_connectionManager->m_iConnectPhase != eConnectConnectedToMud) {
            continue;
        }
        if (timer->fire_time > now) {
            continue;
        }
        firedTimers.append(name);
    }

    for (const QString& name : firedTimers) {
        Timer* timer = getTimer(name);
        if (!timer) {
            continue;
        }

        timer->matched++;
        timer->when_fired = now;
        m_iTimersFiredCount++;
        m_iTimersFiredThisSessionCount++;

        if (timer->type == Timer::TimerType::AtTime) {
            timer->fire_time = timer->fire_time.addDays(1);
        } else {
            qint64 intervalMs = static_cast<qint64>(timer->every_hour) * 3600000LL +
                                static_cast<qint64>(timer->every_minute) * 60000LL +
                                static_cast<qint64>(timer->every_second * 1000.0);
            if (intervalMs < 1) {
                intervalMs = 1000LL; // guard: degenerate 0-interval treated as 1 s
            }
            timer->fire_time = timer->fire_time.addMSecs(intervalMs);
        }

        if (timer->fire_time <= now) {
            resetOneTimer(timer);
        }

        if (timer->one_shot) {
            timer->enabled = false;
        }

        m_doc.executeTimer(timer, name);

        if (m_TimerMap.find(name) == m_TimerMap.end()) {
            continue;
        }

        if (timer->one_shot) {
            m_TimerRevMap.remove(timer);
            auto it = m_TimerMap.find(name);
            if (it != m_TimerMap.end()) {
                m_TimerMap.erase(it);
            }
        }
    }
}

void AutomationRegistry::checkPluginTimerList(Plugin* plugin)
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList firedTimers;

    for (const auto& [name, timerPtr] : plugin->m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (!timer->enabled) {
            continue;
        }
        if (!timer->active_when_closed &&
            m_doc.m_connectionManager->m_iConnectPhase != eConnectConnectedToMud) {
            continue;
        }
        if (timer->fire_time > now) {
            continue;
        }
        firedTimers.append(name);
    }

    for (const QString& name : firedTimers) {
        auto it = plugin->m_TimerMap.find(name);
        if (it == plugin->m_TimerMap.end()) {
            continue;
        }
        Timer* timer = it->second.get();

        timer->matched++;
        timer->when_fired = now;
        m_iTimersFiredCount++;
        m_iTimersFiredThisSessionCount++;

        if (timer->type == Timer::TimerType::AtTime) {
            timer->fire_time = timer->fire_time.addDays(1);
        } else {
            qint64 intervalMs = static_cast<qint64>(timer->every_hour) * 3600000LL +
                                static_cast<qint64>(timer->every_minute) * 60000LL +
                                static_cast<qint64>(timer->every_second * 1000.0);
            if (intervalMs < 1) {
                intervalMs = 1000LL; // guard: degenerate 0-interval treated as 1 s
            }
            timer->fire_time = timer->fire_time.addMSecs(intervalMs);
        }

        if (timer->fire_time <= now) {
            resetOneTimer(timer);
        }

        if (timer->one_shot) {
            timer->enabled = false;
        }

        m_doc.executePluginTimer(plugin, timer, name);

        auto checkIt = plugin->m_TimerMap.find(name);
        if (checkIt == plugin->m_TimerMap.end()) {
            continue;
        }

        if (timer->one_shot) {
            plugin->m_TimerRevMap.remove(timer);
            plugin->m_TimerMap.erase(checkIt);
        }
    }
}
