// plugin.h
// Plugin Data Structure and Loading
//
// Port of CPlugin from plugins.h
// Plugins extend MUSHclient with isolated Lua scripts, triggers, aliases, and timers

#ifndef PLUGIN_H
#define PLUGIN_H

#include "variable.h"
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

// Forward declarations
class WorldDocument;
class ScriptEngine;
class Trigger;
class Alias;
class Timer;

/**
 * Plugin - Isolated extension package with own Lua state and automation
 *
 * Based on CPlugin from plugins.h
 *
 * Each plugin has:
 * - Metadata (name, author, description, version, ID)
 * - Isolated Lua script engine (separate lua_State)
 * - Own collections (triggers, aliases, timers, variables)
 * - Callbacks for world events (connect, line received, etc.)
 * - Sequence for evaluation order (negative = before world, positive = after)
 * - State persistence (variables saved to .state file)
 *
 * Plugins are loaded from XML files and managed by WorldDocument.
 * They run independently - changes to plugin triggers don't affect world triggers.
 *
 * Stories:
 * - 9.1: Data structure, loading, enable/disable
 * - 9.2: Callback system (OnPluginLineReceived, etc.)
 * - 9.3: Trigger/alias/timer evaluation integration
 * - 9.4: Plugin management API (LoadPlugin, CallPlugin, etc.)
 * - 9.5: State persistence
 * - 9.6: World serialization
 */
class Plugin : public QObject {
    Q_OBJECT

  public:
    /**
     * Constructor - initializes all fields to defaults
     * @param doc Parent WorldDocument
     */
    explicit Plugin(WorldDocument* doc, QObject* parent = nullptr);

    /**
     * Destructor - cleans up script engine and collections
     */
    ~Plugin() override;

    // ========== Metadata Fields (XML Attributes) ==========

    QString m_strName;        // Plugin name (e.g., "Health Bar")
    QString m_strAuthor;      // Plugin author
    QString m_strPurpose;     // Short description (1 line)
    QString m_strDescription; // Long description (multiline)
    QString m_strID;          // Unique GUID (e.g., "{12345678-1234-1234-1234-123456789012}")

    double m_dVersion;         // Plugin version (e.g., 1.5)
    double m_dRequiredVersion; // Required MUSHclient version (e.g., 4.0)

    QDateTime m_tDateWritten;   // When plugin was written
    QDateTime m_tDateModified;  // When plugin was last modified
    QDateTime m_tDateInstalled; // When plugin was installed in this world

    // ========== Script Fields ==========

    QString m_strScript;    // Lua code from <script> tags
    QString m_strLanguage;  // Script language (always "Lua")
    QString m_strSource;    // Full path to plugin XML file
    QString m_strDirectory; // Directory containing plugin file

    // ========== Status/Behavior Fields ==========

    bool m_bEnabled;           // Is plugin active?
    bool m_bSaveState;         // Save variables to .state file?
    qint16 m_iSequence;        // Evaluation order (negative = before world, positive = after)
    qint32 m_iLoadOrder;       // Order plugins were loaded (for dependencies)
    qint64 m_iScriptTimeTaken; // Time spent executing plugin scripts (µs)

    // ========== Plugin Collections (Isolated from world) ==========

    // Triggers: own set of triggers with independent matching
    std::map<QString, std::unique_ptr<Trigger>> m_TriggerMap; // name → unique_ptr<Trigger>
    QVector<Trigger*> m_TriggerArray;                         // sorted by sequence (non-owning)
    bool m_triggersNeedSorting;                               // rebuild array flag

    // Aliases: own set of aliases with independent matching
    std::map<QString, std::unique_ptr<Alias>> m_AliasMap; // name → unique_ptr<Alias>
    QVector<Alias*> m_AliasArray;                         // sorted by sequence (non-owning)
    bool m_aliasesNeedSorting;                            // rebuild array flag

    // Timers: own set of timers with independent firing
    std::map<QString, std::unique_ptr<Timer>> m_TimerMap; // name → unique_ptr<Timer>
    QMap<Timer*, QString> m_TimerRevMap; // reverse map for unlabelled timers (non-owning)

    // Variables: own set of script variables (key-value pairs)
    // Plugin state persistence
    VariableMap m_VariableMap; // name → Variable*

    // Arrays: nested map for Lua table persistence
    // Plugin state persistence
    // Outer map: array name → inner map of key → value
    ArraysMap m_Arrays; // array name → (key → value)

    // ========== Script Engine (Isolated) ==========

    std::unique_ptr<ScriptEngine> m_ScriptEngine; // Isolated Lua state for this plugin

    // ========== Callback Tracking ==========

    // Maps callback name (e.g., "OnPluginLineReceived") to dispatch ID
    // DISPID: 1 = function exists, DISPID_UNKNOWN = doesn't exist or errored
    // Cached to avoid repeated Lua state lookups
    QMap<QString, qint32> m_PluginCallbacks; // callback name → DISPID

    // ========== Plugin Communication ==========

    QString m_strCallingPluginID; // ID of plugin that called us (for CallPlugin)

    // ========== Runtime Flags ==========

    bool m_bGlobal;          // Is this a global plugin (loaded from prefs)?
    bool m_bExecutingScript; // Currently executing script (prevents deletion)
    bool m_bSavingStateNow;  // Currently saving state (prevent recursion)

    // ========== Back-pointer to World ==========

    WorldDocument* m_pDoc; // Parent world document (not owned)

    // ========== Helper Methods ==========

    /**
     * Get trigger array sorted by sequence
     * @return Reference to m_TriggerArray
     */
    QVector<Trigger*>& GetTriggerArray();

    /**
     * Get alias array sorted by sequence
     * @return Reference to m_AliasArray
     */
    QVector<Alias*>& GetAliasArray();

    /**
     * Get timer map
     * @return Reference to m_TimerMap
     */
    std::map<QString, std::unique_ptr<Timer>>& GetTimerMap();

    /**
     * Rebuild trigger array sorted by sequence
     */
    void rebuildTriggerArray();

    /**
     * Rebuild alias array sorted by sequence
     */
    void rebuildAliasArray();

    /**
     * Get plugin dispatch ID - check if callback function exists
     *
     * Plugin Callbacks
     *
     * Checks if the plugin's Lua state has a function with the given name.
     * Results are cached in m_PluginCallbacks to avoid repeated lookups.
     *
     * @param callbackName Function name (e.g., "OnPluginLineReceived")
     * @return 1 if function exists, DISPID_UNKNOWN if not
     */
    qint32 GetPluginDispid(const QString& callbackName);

    /**
     * Execute plugin callback with no parameters
     *
     * Plugin Callbacks
     *
     * @param callbackName Function name
     * @return true to continue, false to stop propagation
     */
    bool ExecutePluginScript(const QString& callbackName);

    /**
     * Execute plugin callback with string parameter
     *
     * Plugin Callbacks
     *
     * @param callbackName Function name
     * @param arg String parameter
     * @return true to continue, false to stop propagation
     */
    bool ExecutePluginScript(const QString& callbackName, const QString& arg);

    /**
     * Execute plugin callback with int + string parameters
     *
     * Plugin Callbacks
     *
     * @param callbackName Function name
     * @param arg1 Integer parameter
     * @param arg2 String parameter
     * @return true to continue, false to stop propagation
     */
    bool ExecutePluginScript(const QString& callbackName, qint32 arg1, const QString& arg2);

    /**
     * Execute plugin callback with int + int + string parameters
     *
     * Plugin Callbacks
     *
     * Used by telnet subnegotiation callbacks (option, suboption, data)
     *
     * @param callbackName Function name
     * @param arg1 First integer parameter
     * @param arg2 Second integer parameter
     * @param arg3 String parameter
     * @return true to continue, false to stop propagation
     */
    bool ExecutePluginScript(const QString& callbackName, qint32 arg1, qint32 arg2,
                             const QString& arg3);

    /**
     * Execute plugin callback with int + string + string + string parameters
     *
     * Plugin Communication
     *
     * Used by OnPluginBroadcast callback (message, senderID, senderName, text)
     *
     * @param callbackName Function name
     * @param arg1 Integer parameter (message ID)
     * @param arg2 First string parameter (sender ID)
     * @param arg3 Second string parameter (sender name)
     * @param arg4 Third string parameter (text)
     * @return true to continue, false to stop propagation
     */
    bool ExecutePluginScript(const QString& callbackName, qint32 arg1, const QString& arg2,
                             const QString& arg3, const QString& arg4);

    /**
     * Save plugin state to .state file
     *
     * Plugin State Saving
     *
     * Writes plugin variables and arrays to <plugin_id>.state file.
     * Calls OnPluginSaveState callback before saving.
     *
     * @return true on success
     */
    bool SaveState();

    /**
     * Load plugin state from .state file
     *
     * Plugin State Saving
     *
     * Reads variables and arrays from <plugin_id>.state file.
     *
     * @return true on success
     */
    bool LoadState();

    // ========== Accessor Methods ==========

    QString name() const
    {
        return m_strName;
    }
    QString id() const
    {
        return m_strID;
    }
    bool enabled() const
    {
        return m_bEnabled;
    }
    ScriptEngine* scriptEngine() const
    {
        return m_ScriptEngine.get();
    }
    QString callingPluginID() const
    {
        return m_strCallingPluginID;
    }
    void setCallingPluginID(const QString& id)
    {
        m_strCallingPluginID = id;
    }

  private:
    void initializeDefaults();
};

// ========== Plugin Callback Constants ==========

// Lifecycle callbacks
extern const QString ON_PLUGIN_INSTALL; // "OnPluginInstall"
extern const QString ON_PLUGIN_CLOSE;   // "OnPluginClose"
extern const QString ON_PLUGIN_ENABLE;  // "OnPluginEnable"
extern const QString ON_PLUGIN_DISABLE; // "OnPluginDisable"

// Connection callbacks
extern const QString ON_PLUGIN_CONNECT;    // "OnPluginConnect"
extern const QString ON_PLUGIN_DISCONNECT; // "OnPluginDisconnect"

// Data callbacks
extern const QString ON_PLUGIN_LINE_RECEIVED;   // "OnPluginLineReceived" (most important)
extern const QString ON_PLUGIN_PARTIAL_LINE;    // "OnPluginPartialLine"
extern const QString ON_PLUGIN_PACKET_RECEIVED; // "OnPluginPacketReceived"

// Send callbacks
extern const QString ON_PLUGIN_SEND; // "OnPluginSend"
extern const QString ON_PLUGIN_SENT; // "OnPluginSent"

// Command callbacks
extern const QString ON_PLUGIN_COMMAND;         // "OnPluginCommand"
extern const QString ON_PLUGIN_COMMAND_ENTERED; // "OnPluginCommandEntered"
extern const QString ON_PLUGIN_COMMAND_CHANGED; // "OnPluginCommandChanged"
extern const QString ON_PLUGIN_TABCOMPLETE;     // "OnPluginTabComplete"

// Telnet callbacks
extern const QString ON_PLUGIN_TELNET_OPTION;         // "OnPluginTelnetOption"
extern const QString ON_PLUGIN_TELNET_REQUEST;        // "OnPluginTelnetRequest"
extern const QString ON_PLUGIN_TELNET_SUBNEGOTIATION; // "OnPluginTelnetSubnegotiation"
extern const QString ON_PLUGIN_IAC_GA;                // "OnPlugin_IAC_GA"

// MXP callbacks
extern const QString ON_PLUGIN_MXP_START;     // "OnPluginMXPstart"
extern const QString ON_PLUGIN_MXP_STOP;      // "OnPluginMXPstop"
extern const QString ON_PLUGIN_MXP_OPEN_TAG;  // "OnPluginMXPopenTag"
extern const QString ON_PLUGIN_MXP_CLOSE_TAG; // "OnPluginMXPcloseTag"

// UI callbacks
extern const QString ON_PLUGIN_GET_FOCUS;            // "OnPluginGetFocus"
extern const QString ON_PLUGIN_LOSE_FOCUS;           // "OnPluginLoseFocus"
extern const QString ON_PLUGIN_TICK;                 // "OnPluginTick" (every second)
extern const QString ON_PLUGIN_WORLD_OUTPUT_RESIZED; // "OnPluginWorldOutputResized"
extern const QString ON_PLUGIN_MOUSE_MOVED;          // "OnPluginMouseMoved"
extern const QString ON_PLUGIN_SCREENDRAW;           // "OnPluginScreendraw"
extern const QString ON_PLUGIN_SELECTION_CHANGED;    // "OnPluginSelectionChanged"
extern const QString ON_PLUGIN_DRAW_OUTPUT_WINDOW;   // "OnPluginDrawOutputWindow"

// Debug callbacks
extern const QString ON_PLUGIN_TRACE;        // "OnPluginTrace"
extern const QString ON_PLUGIN_PACKET_DEBUG; // "OnPluginPacketDebug"

// State callbacks
extern const QString ON_PLUGIN_SAVE_STATE; // "OnPluginSaveState"
extern const QString ON_PLUGIN_WORLD_SAVE; // "OnPluginWorldSave"

// Communication callbacks
extern const QString ON_PLUGIN_BROADCAST;    // "OnPluginBroadcast"
extern const QString ON_PLUGIN_LIST_CHANGED; // "OnPluginListChanged"

// Sound callbacks
extern const QString ON_PLUGIN_PLAY_SOUND; // "OnPluginPlaySound"

#endif // PLUGIN_H
