#ifndef AUTOMATION_REGISTRY_H
#define AUTOMATION_REGISTRY_H

#include <QMap>
#include <QString>
#include <QVector>
#include <expected>
#include <map>
#include <memory>

#include "../automation/alias.h"
#include "../automation/timer.h"
#include "../automation/trigger.h"
#include "world_error.h"

class WorldDocument;
class Line;
class Plugin;

/**
 * AutomationRegistry — companion object that owns trigger/alias/timer storage
 * and provides the registry operations and evaluation pipeline.
 *
 * WorldDocument owns this via std::unique_ptr<AutomationRegistry>.
 * Execution methods (executeTrigger, executeAlias, executeTimer, sendTo) stay on
 * WorldDocument because they are tightly coupled to its output/script/sendTo
 * infrastructure.
 *
 * Pattern: same back-pointer callback design as TelnetParser, MXPEngine, SoundManager.
 */
class AutomationRegistry {
  public:
    explicit AutomationRegistry(WorldDocument& doc);
    ~AutomationRegistry() = default;

    // Non-copyable, non-movable (holds a WorldDocument&)
    AutomationRegistry(const AutomationRegistry&) = delete;
    AutomationRegistry& operator=(const AutomationRegistry&) = delete;

    // ========== Trigger Storage (public — companion-object pattern) ==========
    std::map<QString, std::unique_ptr<Trigger>> m_TriggerMap;
    QVector<Trigger*> m_TriggerArray; // sorted by sequence, non-owning
    bool m_triggersNeedSorting = false;

    // ========== Alias Storage ==========
    std::map<QString, std::unique_ptr<Alias>> m_AliasMap;
    QVector<Alias*> m_AliasArray; // sorted by sequence, non-owning
    bool m_aliasesNeedSorting = false;

    // ========== Timer Storage ==========
    std::map<QString, std::unique_ptr<Timer>> m_TimerMap;
    QMap<Timer*, QString> m_TimerRevMap; // reverse lookup for unlabelled timers

    // ========== Statistics ==========
    qint32 m_iTriggersEvaluatedCount = 0;
    qint32 m_iTriggersMatchedCount = 0;
    qint32 m_iAliasesEvaluatedCount = 0;
    qint32 m_iAliasesMatchedCount = 0;
    qint32 m_iTimersFiredCount = 0;
    qint32 m_iTriggersMatchedThisSessionCount = 0;
    qint32 m_iAliasesMatchedThisSessionCount = 0;
    qint32 m_iTimersFiredThisSessionCount = 0;
    double m_trigger_time_elapsed = 0.0; // seconds spent evaluating triggers
    double m_alias_time_elapsed = 0.0;   // seconds spent evaluating aliases

    // ========== Trigger Registry ==========
    [[nodiscard]] std::expected<void, WorldError> addTrigger(const QString& name,
                                                             std::unique_ptr<Trigger> trigger);
    [[nodiscard]] std::expected<void, WorldError> deleteTrigger(const QString& name);
    Trigger* getTrigger(const QString& name);

    // ========== Alias Registry ==========
    [[nodiscard]] std::expected<void, WorldError> addAlias(const QString& name,
                                                           std::unique_ptr<Alias> alias);
    [[nodiscard]] std::expected<void, WorldError> deleteAlias(const QString& name);
    Alias* getAlias(const QString& name);

    // ========== Timer Registry ==========
    [[nodiscard]] std::expected<void, WorldError> addTimer(const QString& name,
                                                           std::unique_ptr<Timer> timer);
    [[nodiscard]] std::expected<void, WorldError> deleteTimer(const QString& name);
    Timer* getTimer(const QString& name);

    // ========== Trigger Evaluation Pipeline ==========
    void evaluateTriggers(Line* line);
    void rebuildTriggerArray();

    // ========== Alias Evaluation Pipeline ==========
    bool evaluateAliases(const QString& command);
    void rebuildAliasArray();

    // ========== Timer Management ==========
    void resetOneTimer(Timer* timer);
    void resetAllTimers();
    void checkTimers();
    void checkTimerList();
    void checkPluginTimerList(Plugin* plugin);

  private:
    WorldDocument& m_doc;
};

#endif // AUTOMATION_REGISTRY_H
