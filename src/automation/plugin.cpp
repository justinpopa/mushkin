/**
 * plugin.cpp - Plugin Data Structure and Loading
 *
 * Based on CPlugin from plugins.h/plugins.cpp
 */

#include "plugin.h"
#include "../storage/global_options.h"
#include "../world/script_engine.h"
#include "../world/world_document.h"
#include "alias.h"
#include "logging.h"
#include "timer.h"
#include "trigger.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// Default plugin sequence (plugins.h)
#define DEFAULT_PLUGIN_SEQUENCE 5000

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

// ========== Plugin Callback Constants ==========

// Lifecycle callbacks
const QString ON_PLUGIN_INSTALL = "OnPluginInstall";
const QString ON_PLUGIN_CLOSE = "OnPluginClose";
const QString ON_PLUGIN_ENABLE = "OnPluginEnable";
const QString ON_PLUGIN_DISABLE = "OnPluginDisable";

// Connection callbacks
const QString ON_PLUGIN_CONNECT = "OnPluginConnect";
const QString ON_PLUGIN_DISCONNECT = "OnPluginDisconnect";

// Data callbacks
const QString ON_PLUGIN_LINE_RECEIVED = "OnPluginLineReceived";
const QString ON_PLUGIN_PARTIAL_LINE = "OnPluginPartialLine";
const QString ON_PLUGIN_PACKET_RECEIVED = "OnPluginPacketReceived";

// Send callbacks
const QString ON_PLUGIN_SEND = "OnPluginSend";
const QString ON_PLUGIN_SENT = "OnPluginSent";

// Command callbacks
const QString ON_PLUGIN_COMMAND = "OnPluginCommand";
const QString ON_PLUGIN_COMMAND_ENTERED = "OnPluginCommandEntered";
const QString ON_PLUGIN_COMMAND_CHANGED = "OnPluginCommandChanged";
const QString ON_PLUGIN_TABCOMPLETE = "OnPluginTabComplete";

// Telnet callbacks
const QString ON_PLUGIN_TELNET_OPTION = "OnPluginTelnetOption";
const QString ON_PLUGIN_TELNET_REQUEST = "OnPluginTelnetRequest";
const QString ON_PLUGIN_TELNET_SUBNEGOTIATION = "OnPluginTelnetSubnegotiation";
const QString ON_PLUGIN_IAC_GA = "OnPlugin_IAC_GA";

// MXP callbacks
const QString ON_PLUGIN_MXP_START = "OnPluginMXPstart";
const QString ON_PLUGIN_MXP_STOP = "OnPluginMXPstop";
const QString ON_PLUGIN_MXP_OPEN_TAG = "OnPluginMXPopenTag";
const QString ON_PLUGIN_MXP_CLOSE_TAG = "OnPluginMXPcloseTag";

// UI callbacks
const QString ON_PLUGIN_GET_FOCUS = "OnPluginGetFocus";
const QString ON_PLUGIN_LOSE_FOCUS = "OnPluginLoseFocus";
const QString ON_PLUGIN_TICK = "OnPluginTick";
const QString ON_PLUGIN_WORLD_OUTPUT_RESIZED = "OnPluginWorldOutputResized";
const QString ON_PLUGIN_MOUSE_MOVED = "OnPluginMouseMoved";
const QString ON_PLUGIN_SCREENDRAW = "OnPluginScreendraw";
const QString ON_PLUGIN_SELECTION_CHANGED = "OnPluginSelectionChanged";
const QString ON_PLUGIN_DRAW_OUTPUT_WINDOW = "OnPluginDrawOutputWindow";

// Debug callbacks
const QString ON_PLUGIN_TRACE = "OnPluginTrace";
const QString ON_PLUGIN_PACKET_DEBUG = "OnPluginPacketDebug";

// State callbacks
const QString ON_PLUGIN_SAVE_STATE = "OnPluginSaveState";
const QString ON_PLUGIN_WORLD_SAVE = "OnPluginWorldSave";

// Communication callbacks
const QString ON_PLUGIN_BROADCAST = "OnPluginBroadcast";
const QString ON_PLUGIN_LIST_CHANGED = "OnPluginListChanged";

// Sound callbacks
const QString ON_PLUGIN_PLAY_SOUND = "OnPluginPlaySound";

/**
 * Constructor - Initialize all fields to defaults
 *
 * Based on CPlugin::CPlugin() from plugins.cpp
 */
Plugin::Plugin(WorldDocument* doc, QObject* parent) : QObject(parent), m_pDoc(doc)
{
    initializeDefaults();
}

/**
 * Destructor - Clean up script engine and collections
 *
 * Based on CPlugin::~CPlugin() from plugins.cpp
 */
Plugin::~Plugin()
{
    // Call OnPluginClose callback before cleanup
    // Note: We need to temporarily set this as the current plugin
    // Only do this if m_pDoc is valid (not during WorldDocument destruction)
    if (m_pDoc) {
        Plugin* pSavedPlugin = m_pDoc->m_CurrentPlugin;
        m_pDoc->m_CurrentPlugin = this;
        ExecutePluginScript(ON_PLUGIN_CLOSE);
        m_pDoc->m_CurrentPlugin = pSavedPlugin;

        // Save state before destruction
        SaveState();
    }

    // Delete all triggers (unique_ptr handles deletion automatically)
    m_TriggerMap.clear();
    m_TriggerArray.clear();

    // Delete all aliases (unique_ptr handles deletion automatically)
    m_AliasMap.clear();
    m_AliasArray.clear();

    // Delete all timers (unique_ptr handles deletion automatically)
    m_TimerMap.clear();
    m_TimerRevMap.clear();

    // Delete variables (unique_ptr handles deletion automatically)
    m_VariableMap.clear();

    // Arrays are QMap<QString, QString> - no pointers to delete
    m_Arrays.clear();

    // Script engine (unique_ptr handles deletion automatically)
    m_ScriptEngine.reset();
}

/**
 * Initialize all fields to default values
 *
 * Based on CPlugin constructor from plugins.cpp
 */
void Plugin::initializeDefaults()
{
    // Metadata
    m_strName.clear();
    m_strAuthor.clear();
    m_strPurpose.clear();
    m_strDescription.clear();
    m_strID.clear();

    m_dVersion = 0.0;
    m_dRequiredVersion = 0.0;

    m_tDateWritten = QDateTime();
    m_tDateModified = QDateTime();
    m_tDateInstalled = QDateTime::currentDateTime();

    // Script
    m_strScript.clear();
    m_strLanguage = "Lua";
    m_strSource.clear();
    m_strDirectory.clear();

    // Status/Behavior
    m_bEnabled = true;
    m_bSaveState = false;
    m_iSequence = DEFAULT_PLUGIN_SEQUENCE;
    m_iLoadOrder = 0;
    m_iScriptTimeTaken = 0;

    // Collections
    m_triggersNeedSorting = false;
    m_aliasesNeedSorting = false;

    // Communication
    m_strCallingPluginID.clear();

    // Runtime flags
    m_bGlobal = false;
    m_bExecutingScript = false;
    m_bSavingStateNow = false;
}

/**
 * Get trigger array sorted by sequence
 *
 * Rebuilds array if needed (m_triggersNeedSorting flag is set)
 *
 * @return Reference to m_TriggerArray
 */
QVector<Trigger*>& Plugin::GetTriggerArray()
{
    if (m_triggersNeedSorting) {
        rebuildTriggerArray();
    }
    return m_TriggerArray;
}

/**
 * Get alias array sorted by sequence
 *
 * Rebuilds array if needed (m_aliasesNeedSorting flag is set)
 *
 * @return Reference to m_AliasArray
 */
QVector<Alias*>& Plugin::GetAliasArray()
{
    if (m_aliasesNeedSorting) {
        rebuildAliasArray();
    }
    return m_AliasArray;
}

/**
 * Get timer map
 *
 * @return Reference to m_TimerMap
 */
std::map<QString, std::unique_ptr<Timer>>& Plugin::GetTimerMap()
{
    return m_TimerMap;
}

/**
 * Rebuild trigger array sorted by sequence
 *
 * Based on similar logic in WorldDocument::rebuildTriggerArray()
 */
void Plugin::rebuildTriggerArray()
{
    m_TriggerArray.clear();
    m_TriggerArray.reserve(m_TriggerMap.size());

    // Copy all triggers from map to array
    for (const auto& [name, triggerPtr] : m_TriggerMap) {
        m_TriggerArray.append(triggerPtr.get());
    }

    // Sort by sequence (lower sequence = earlier evaluation)
    std::sort(m_TriggerArray.begin(), m_TriggerArray.end(),
              [](const Trigger* a, const Trigger* b) { return a->iSequence < b->iSequence; });

    m_triggersNeedSorting = false;
}

/**
 * Rebuild alias array sorted by sequence
 *
 * Based on similar logic in WorldDocument::rebuildAliasArray()
 */
void Plugin::rebuildAliasArray()
{
    m_AliasArray.clear();
    m_AliasArray.reserve(m_AliasMap.size());

    // Copy all aliases from map to array
    for (const auto& [name, aliasPtr] : m_AliasMap) {
        m_AliasArray.append(aliasPtr.get());
    }

    // Sort by sequence (lower sequence = earlier evaluation)
    std::sort(m_AliasArray.begin(), m_AliasArray.end(),
              [](const Alias* a, const Alias* b) { return a->iSequence < b->iSequence; });

    m_aliasesNeedSorting = false;
}

/**
 * Get plugin dispatch ID - check if callback function exists
 *
 * Plugin Callbacks
 *
 * Based on CPlugin::GetPluginDispid() from plugins.cpp
 *
 * Checks if the plugin's Lua state has a function with the given name.
 * Results are cached in m_PluginCallbacks to avoid repeated lookups.
 *
 * @param callbackName Function name (e.g., "OnPluginLineReceived")
 * @return 1 if function exists, DISPID_UNKNOWN if not
 */
qint32 Plugin::GetPluginDispid(const QString& callbackName)
{
    // Check cache first
    if (m_PluginCallbacks.contains(callbackName)) {
        return m_PluginCallbacks[callbackName];
    }

    // Check if function exists in script engine
    qint32 dispid = DISPID_UNKNOWN;
    if (m_ScriptEngine) {
        dispid = m_ScriptEngine->getLuaDispid(callbackName);
    }

    // Cache result
    m_PluginCallbacks[callbackName] = dispid;

    return dispid;
}

/**
 * Execute plugin callback with no parameters
 *
 * Plugin Callbacks
 *
 * Based on CPlugin::ExecutePluginScript() from plugins.cpp
 *
 * @param callbackName Function name
 * @return true to continue, false to stop propagation
 */
bool Plugin::ExecutePluginScript(const QString& callbackName)
{
    qint32 dispid = GetPluginDispid(callbackName);
    if (dispid == DISPID_UNKNOWN || !m_ScriptEngine) {
        return true; // No callback, continue
    }

    // Build parameter lists (empty)
    QList<double> nparams;
    QList<QString> sparams;

    // Execution context strings
    QString strType = QString("Plugin %1").arg(m_strName);
    QString strReason = QString("Executing plugin %1 sub %2").arg(m_strName, callbackName);

    // Call Lua
    long nInvocationCount = 0;
    bool result = true; // Default: continue propagation

    bool bError =
        m_ScriptEngine->executeLua(dispid, callbackName, eDontChangeAction, strType, strReason,
                                   nparams, sparams, nInvocationCount, &result);

    // If error, mark callback as invalid and continue
    if (bError) {
        qWarning() << "Plugin" << m_strName << "callback" << callbackName << "failed";
        m_PluginCallbacks[callbackName] = DISPID_UNKNOWN;
        return true;
    }

    return result;
}

/**
 * Execute plugin callback with string parameter
 *
 * Plugin Callbacks
 *
 * Based on CPlugin::ExecutePluginScript() from plugins.cpp
 *
 * @param callbackName Function name
 * @param arg String parameter
 * @return true to continue, false to stop propagation
 */
bool Plugin::ExecutePluginScript(const QString& callbackName, const QString& arg)
{
    qint32 dispid = GetPluginDispid(callbackName);
    if (dispid == DISPID_UNKNOWN || !m_ScriptEngine) {
        return true; // No callback, continue
    }

    // Build parameter lists (one string)
    QList<double> nparams;
    QList<QString> sparams;
    sparams.append(arg);

    // Execution context strings
    QString strType = QString("Plugin %1").arg(m_strName);
    QString strReason = QString("Executing plugin %1 sub %2").arg(m_strName, callbackName);

    // Call Lua
    long nInvocationCount = 0;
    bool result = true; // Default: continue propagation

    bool bError =
        m_ScriptEngine->executeLua(dispid, callbackName, eDontChangeAction, strType, strReason,
                                   nparams, sparams, nInvocationCount, &result);

    // If error, mark callback as invalid and continue
    if (bError) {
        m_PluginCallbacks[callbackName] = DISPID_UNKNOWN;
        return true;
    }

    return result;
}

/**
 * Execute plugin callback with int + string parameters
 *
 * Plugin Callbacks
 *
 * Based on CPlugin::ExecutePluginScript() from plugins.cpp
 *
 * @param callbackName Function name
 * @param arg1 Integer parameter
 * @param arg2 String parameter
 * @return true to continue, false to stop propagation
 */
bool Plugin::ExecutePluginScript(const QString& callbackName, qint32 arg1, const QString& arg2)
{
    qint32 dispid = GetPluginDispid(callbackName);
    if (dispid == DISPID_UNKNOWN || !m_ScriptEngine) {
        qCDebug(lcAutomation) << "ExecutePluginScript:" << m_strName << callbackName
                              << "- No callback (dispid=" << dispid << ")";
        return true; // No callback, continue
    }

    qCDebug(lcAutomation) << "ExecutePluginScript:" << m_strName << callbackName << "arg1=" << arg1
                          << "arg2=" << arg2;

    // Build parameter lists (one number, one string)
    QList<double> nparams;
    QList<QString> sparams;
    nparams.append(static_cast<double>(arg1));
    sparams.append(arg2);

    // Execution context strings
    QString strType = QString("Plugin %1").arg(m_strName);
    QString strReason = QString("Executing plugin %1 sub %2").arg(m_strName, callbackName);

    // Call Lua
    long nInvocationCount = 0;
    bool result = true; // Default: continue propagation

    bool bError =
        m_ScriptEngine->executeLua(dispid, callbackName, eDontChangeAction, strType, strReason,
                                   nparams, sparams, nInvocationCount, &result);

    // If error, mark callback as invalid and continue
    if (bError) {
        qWarning() << "ExecutePluginScript ERROR:" << m_strName << callbackName;
        m_PluginCallbacks[callbackName] = DISPID_UNKNOWN;
        return true;
    }
    return result;
}

/**
 * Execute plugin callback with int + int + string parameters
 *
 * Plugin Callbacks
 *
 * Based on CPlugin::ExecutePluginScript() from plugins.cpp
 *
 * Used by telnet callbacks like OnPluginTelnetSubnegotiation(option, suboption, data)
 *
 * @param callbackName Function name
 * @param arg1 First integer parameter
 * @param arg2 Second integer parameter
 * @param arg3 String parameter
 * @return true to continue, false to stop propagation
 */
bool Plugin::ExecutePluginScript(const QString& callbackName, qint32 arg1, qint32 arg2,
                                 const QString& arg3)
{
    qint32 dispid = GetPluginDispid(callbackName);
    if (dispid == DISPID_UNKNOWN || !m_ScriptEngine) {
        return true; // No callback, continue
    }

    // Build parameter lists (two numbers, one string)
    QList<double> nparams;
    QList<QString> sparams;
    nparams.append(static_cast<double>(arg1));
    nparams.append(static_cast<double>(arg2));
    sparams.append(arg3);

    // Execution context strings
    QString strType = QString("Plugin %1").arg(m_strName);
    QString strReason = QString("Executing plugin %1 sub %2").arg(m_strName, callbackName);

    // Call Lua
    long nInvocationCount = 0;
    bool result = true; // Default: continue propagation

    bool bError =
        m_ScriptEngine->executeLua(dispid, callbackName, eDontChangeAction, strType, strReason,
                                   nparams, sparams, nInvocationCount, &result);

    // If error, mark callback as invalid and continue
    if (bError) {
        m_PluginCallbacks[callbackName] = DISPID_UNKNOWN;
        return true;
    }

    return result;
}

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
bool Plugin::ExecutePluginScript(const QString& callbackName, qint32 arg1, const QString& arg2,
                                 const QString& arg3, const QString& arg4)
{
    qint32 dispid = GetPluginDispid(callbackName);
    if (dispid == DISPID_UNKNOWN || !m_ScriptEngine) {
        qCDebug(lcAutomation) << "ExecutePluginScript(" << m_strName << "):" << callbackName
                              << "- No callback found (dispid=" << dispid << ")";
        return true; // No callback, continue
    }

    qCDebug(lcAutomation) << "ExecutePluginScript(" << m_strName << "):" << callbackName
                          << "- Executing callback with" << arg1 << arg2 << arg3 << arg4;

    // Build parameter lists (one number, three strings)
    QList<double> nparams;
    QList<QString> sparams;
    nparams.append(static_cast<double>(arg1));
    sparams.append(arg2);
    sparams.append(arg3);
    sparams.append(arg4);

    // Execution context strings
    QString strType = QString("Plugin %1").arg(m_strName);
    QString strReason = QString("Executing plugin %1 sub %2").arg(m_strName, callbackName);

    // Call Lua
    long nInvocationCount = 0;
    bool result = true; // Default: continue propagation

    bool bError =
        m_ScriptEngine->executeLua(dispid, callbackName, eDontChangeAction, strType, strReason,
                                   nparams, sparams, nInvocationCount, &result);

    // If error, mark callback as invalid and continue
    if (bError) {
        m_PluginCallbacks[callbackName] = DISPID_UNKNOWN;
        return true;
    }

    return result;
}

/**
 * Save plugin state to state file
 *
 * Based on CPlugin::SaveState() from plugins.cpp
 *
 * Writes plugin variables and arrays to state file.
 * Uses format: {StateFilesDir}/{WorldID}-{PluginID}-state.xml
 * This matches original MUSHclient for compatibility and per-world isolation.
 * Calls OnPluginSaveState callback before saving.
 *
 * @return true on success
 */
bool Plugin::SaveState()
{
    // Check if state saving is enabled
    if (!m_bSaveState) {
        return true; // Not saving state
    }

    // Prevent infinite recursion
    if (m_bSavingStateNow) {
        return true;
    }

    // Check if we have an ID (required for filename)
    if (m_strID.isEmpty()) {
        qCDebug(lcAutomation) << "Plugin::SaveState() - no ID for plugin:" << m_strName;
        return false;
    }

    // Need a state files directory
    QString stateDir = GlobalOptions::instance()->stateFilesDirectory();
    if (stateDir.isEmpty()) {
        qCDebug(lcAutomation) << "Plugin::SaveState() - no state files directory configured";
        return false;
    }

    // Need a world ID for per-world isolation
    if (m_pDoc->m_strWorldID.isEmpty()) {
        qCDebug(lcAutomation) << "Plugin::SaveState() - no world ID for plugin:" << m_strName;
        return false;
    }

    // Set saving flag
    m_bSavingStateNow = true;

    // Save current plugin context and set to this plugin
    Plugin* savedPlugin = m_pDoc->m_CurrentPlugin;
    m_pDoc->m_CurrentPlugin = this;

    // Call OnPluginSaveState callback before saving
    ExecutePluginScript(ON_PLUGIN_SAVE_STATE);

    // Restore plugin context
    m_pDoc->m_CurrentPlugin = savedPlugin;

    // Ensure state files directory exists
    QDir dir(stateDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCDebug(lcAutomation) << "Plugin::SaveState() - failed to create state directory:"
                                  << stateDir;
            m_bSavingStateNow = false;
            return false;
        }
    }

    // Build state filename: {stateDir}/{worldID}-{pluginID}-state.xml
    // This matches original MUSHclient format for compatibility
    QString stateFilePath = stateDir + "/" + m_pDoc->m_strWorldID + "-" + m_strID + "-state.xml";

    // Open file for writing
    QFile file(stateFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCDebug(lcAutomation) << "Plugin::SaveState() - failed to open file for writing:"
                              << stateFilePath;
        m_bSavingStateNow = false;
        return false;
    }

    // Write XML
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    // XML prolog and root
    xml.writeStartDocument();
    xml.writeComment(QString("Plugin state saved. Plugin: \"%1\"").arg(m_strName));
    xml.writeStartElement("muclient");

    // Write variables section
    if (!m_VariableMap.empty()) {
        xml.writeStartElement("variables");

        // Variables are already sorted by name (std::map maintains key order)
        for (const auto& [key, var] : m_VariableMap) {
            xml.writeStartElement("variable");
            xml.writeAttribute("name", var->strLabel);
            xml.writeCharacters(var->strContents);
            xml.writeEndElement(); // variable
        }

        xml.writeEndElement(); // variables
    }

    // Write arrays section
    if (!m_Arrays.isEmpty()) {
        xml.writeStartElement("arrays");

        // Sort array names for consistent output
        QStringList arrayNames = m_Arrays.keys();
        arrayNames.sort();

        for (const QString& arrayName : arrayNames) {
            const QMap<QString, QString>& arrayMap = m_Arrays[arrayName];

            xml.writeStartElement("array");
            xml.writeAttribute("name", arrayName);

            // Sort keys within each array
            QStringList itemKeys = arrayMap.keys();
            itemKeys.sort();

            for (const QString& itemKey : itemKeys) {
                xml.writeStartElement("item");
                xml.writeAttribute("key", itemKey);
                xml.writeCharacters(arrayMap[itemKey]);
                xml.writeEndElement(); // item
            }

            xml.writeEndElement(); // array
        }

        xml.writeEndElement(); // arrays
    }

    xml.writeEndElement(); // muclient
    xml.writeEndDocument();

    // Ensure all data is flushed to disk before closing
    file.flush();
    file.close();

    // Clear saving flag
    m_bSavingStateNow = false;

    return true;
}

/**
 * Load plugin state from state file
 *
 * Reads variables and arrays from state file.
 * Uses format: {StateFilesDir}/{WorldID}-{PluginID}-state.xml
 * This matches original MUSHclient for compatibility and per-world isolation.
 *
 * @return true on success
 */
bool Plugin::LoadState()
{
    // Check if we have an ID (required for filename)
    if (m_strID.isEmpty()) {
        return true; // No state to load
    }

    // Need a state files directory
    QString stateDir = GlobalOptions::instance()->stateFilesDirectory();
    if (stateDir.isEmpty()) {
        return true; // No state directory configured
    }

    // Need a world ID for per-world isolation
    if (m_pDoc->m_strWorldID.isEmpty()) {
        return true; // No world ID, can't load state
    }

    // Build state filename: {stateDir}/{worldID}-{pluginID}-state.xml
    // This matches original MUSHclient format for compatibility
    QString stateFilePath = stateDir + "/" + m_pDoc->m_strWorldID + "-" + m_strID + "-state.xml";

    // Check if state file exists
    QFile file(stateFilePath);
    if (!file.exists()) {
        return true; // No state file yet, this is OK
    }

    // Open file for reading
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(lcAutomation) << "Plugin::LoadState() - failed to open file for reading:"
                              << stateFilePath;
        return false;
    }

    // Read XML
    QXmlStreamReader xml(&file);

    // Navigate to root element
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("muclient")) {
            // Process muclient children
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("variables")) {
                    // Read variables
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("variable")) {
                            QString name = xml.attributes().value("name").toString();
                            QString value = xml.readElementText();

                            // Create variable and add to map
                            auto var = std::make_unique<Variable>();
                            var->strLabel = name;
                            var->strContents = value;
                            m_VariableMap[name] = std::move(var);
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else if (xml.name() == QLatin1String("arrays")) {
                    // Read arrays
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("array")) {
                            QString arrayName = xml.attributes().value("name").toString();

                            // Create inner map for this array
                            QMap<QString, QString> arrayMap;

                            // Read items
                            while (xml.readNextStartElement()) {
                                if (xml.name() == QLatin1String("item")) {
                                    QString key = xml.attributes().value("key").toString();
                                    QString value = xml.readElementText();
                                    arrayMap[key] = value;
                                } else {
                                    xml.skipCurrentElement();
                                }
                            }

                            // Add array to map
                            m_Arrays[arrayName] = arrayMap;
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else {
                    xml.skipCurrentElement();
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    // Check for XML errors
    if (xml.hasError()) {
        qCDebug(lcAutomation) << "Plugin::LoadState() - XML error:" << xml.errorString();
        file.close();
        return false;
    }

    file.close();
    return true;
}
