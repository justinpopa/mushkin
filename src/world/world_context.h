#ifndef WORLD_CONTEXT_H
#define WORLD_CONTEXT_H

/**
 * IWorldContext — abstract interface for subsystem decoupling
 *
 * Subsystems (AutomationRegistry, SoundManager, NotepadWidget) depend on
 * this interface instead of the concrete WorldDocument class. This enables
 * isolation testing and reduces coupling.
 *
 * WorldDocument implements this interface.
 *
 * Phase 1: AutomationRegistry, SoundManager, NotepadWidget
 * Phase 2 (future): ScriptEngine, ConnectionManager, TelnetParser, MXPEngine
 */

#include <deque>
#include <memory>
#include <vector>

#include <QString>

// Forward declarations
class Alias;
class IOutputView;
class Line;
class NotepadWidget;
class Plugin;
class Timer;
class Trigger;

class IWorldContext {
  public:
    virtual ~IWorldContext() = default;

    // ========== Plugin System ==========
    virtual void SendToAllPluginCallbacks(const QString& callbackName) = 0;
    virtual bool SendToAllPluginCallbacks(const QString& callbackName, const QString& arg,
                                          bool bStopOnFalse = false) = 0;
    virtual const std::vector<std::unique_ptr<Plugin>>& pluginList() const = 0;
    virtual Plugin* currentPlugin() const = 0;
    virtual void setCurrentPlugin(Plugin* plugin) = 0;

    // ========== Automation Execution ==========
    virtual void executeTrigger(Trigger* trigger, Line* line, const QString& matchedText) = 0;
    virtual void executeAlias(Alias* alias, const QString& command) = 0;
    virtual void executeTimer(Timer* timer, const QString& name) = 0;
    virtual void executePluginTimer(Plugin* plugin, Timer* timer, const QString& name) = 0;

    // ========== State Queries ==========
    virtual bool triggersEnabled() const = 0;
    virtual bool timersEnabled() const = 0;
    virtual bool isConnectedToMud() const = 0;
    virtual const std::deque<QString>& recentLines() const = 0;

    // ========== Logging ==========
    virtual void flushLogIfNeeded() = 0;

    // ========== Output ==========
    virtual IOutputView* activeOutputView() const = 0;

    // ========== Notepad ==========
    virtual void unregisterNotepad(NotepadWidget* notepad) = 0;
};

#endif // WORLD_CONTEXT_H
