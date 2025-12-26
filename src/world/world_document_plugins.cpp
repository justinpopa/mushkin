/**
 * world_document_plugins.cpp - Plugin Management
 *
 * Implements plugin loading, finding, and enable/disable functionality
 */

#include "../automation/plugin.h"
#include "logging.h"
#include "miniwindow.h"
#include "script_engine.h"
#include "world_document.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <algorithm>

// ============================================================================
// Plugin Finding
// ============================================================================

/**
 * FindPluginByID - Find plugin by unique GUID
 *
 * @param pluginID Plugin GUID (e.g., "{12345678-1234-1234-1234-123456789012}")
 * @return Plugin pointer or nullptr if not found
 */
Plugin* WorldDocument::FindPluginByID(const QString& pluginID)
{
    if (pluginID.isEmpty()) {
        return nullptr;
    }

    for (const auto& plugin : m_PluginList) {
        if (plugin && plugin->m_strID.compare(pluginID, Qt::CaseInsensitive) == 0) {
            return plugin.get();
        }
    }

    return nullptr;
}

/**
 * FindPluginByName - Find plugin by name
 *
 * @param pluginName Plugin name (case-insensitive)
 * @return Plugin pointer or nullptr if not found
 */
Plugin* WorldDocument::FindPluginByName(const QString& pluginName)
{
    if (pluginName.isEmpty()) {
        return nullptr;
    }

    for (const auto& plugin : m_PluginList) {
        if (plugin && plugin->m_strName.compare(pluginName, Qt::CaseInsensitive) == 0) {
            return plugin.get();
        }
    }

    return nullptr;
}

/**
 * FindPluginByFilePath - Find plugin by source file path
 *
 * Matches original MUSHclient behavior of checking for duplicates by file path.
 * Comparison is done on the canonical (absolute) path to handle relative paths.
 *
 * @param filepath Plugin file path
 * @return Plugin pointer or nullptr if not found
 */
Plugin* WorldDocument::FindPluginByFilePath(const QString& filepath)
{
    if (filepath.isEmpty()) {
        return nullptr;
    }

    // Get canonical path for comparison
    QFileInfo fileInfo(filepath);
    QString canonicalPath = fileInfo.canonicalFilePath();
    if (canonicalPath.isEmpty()) {
        // File doesn't exist, use absolute path instead
        canonicalPath = fileInfo.absoluteFilePath();
    }

    for (const auto& plugin : m_PluginList) {
        if (!plugin) {
            continue;
        }

        QFileInfo pluginInfo(plugin->m_strSource);
        QString pluginCanonical = pluginInfo.canonicalFilePath();
        if (pluginCanonical.isEmpty()) {
            pluginCanonical = pluginInfo.absoluteFilePath();
        }

        if (canonicalPath.compare(pluginCanonical, Qt::CaseInsensitive) == 0) {
            return plugin.get();
        }
    }

    return nullptr;
}

/**
 * getPlugin - Get plugin by ID (alias for FindPluginByID)
 *
 * @param pluginID Plugin GUID
 * @return Plugin pointer or nullptr if not found
 */
Plugin* WorldDocument::getPlugin(const QString& pluginID)
{
    return FindPluginByID(pluginID);
}

// ============================================================================
// Plugin Loading
// ============================================================================

/**
 * LoadPlugin - Load plugin from XML file
 *
 * Plugin Data Structure and Loading
 *
 * Based on CMUSHclientDoc::LoadPlugin()
 *
 * Loads plugin XML file, parses metadata, script, and collections (triggers/aliases/timers),
 * creates isolated script engine, executes script, and adds to plugin list.
 *
 * @param filepath Full path to plugin XML file
 * @param errorMsg OUT: Error message if loading fails
 * @return Plugin pointer on success, nullptr on failure
 */
/**
 * preprocessPluginXml - Fix common MUSHclient XML issues
 *
 * MUSHclient's XML parser was more lenient than Qt's QXmlStreamReader.
 * This function fixes common issues:
 * 1. Multiple DOCTYPE declarations with entity definitions - expand entities inline
 * 2. Unescaped < in attribute values (regex named capture groups like (?<name>...))
 *
 * @param content Original XML content
 * @return Fixed XML content
 */
static QString preprocessPluginXml(const QString& content)
{
    QString result = content;

    // Fix 1: Extract entity definitions and expand them inline
    // Pattern: <!ENTITY name "value" >
    // Using delimiter to avoid raw string confusion with quotes/parens
    static QRegularExpression entityDef(R"RE(<!ENTITY\s+(\w+)\s+"([^"]*)"\s*>)RE");
    QMap<QString, QString> entities;

    QRegularExpressionMatchIterator it = entityDef.globalMatch(result);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString name = match.captured(1);
        QString value = match.captured(2);
        entities[name] = value;
    }

    // Replace entity references with their values
    for (auto entityIt = entities.begin(); entityIt != entities.end(); ++entityIt) {
        QString entityRef = QString("&%1;").arg(entityIt.key());
        result.replace(entityRef, entityIt.value());
    }

    // Remove ALL DOCTYPE declarations (MUSHclient XML may have multiple)
    // First remove DOCTYPE with entity definitions: <!DOCTYPE muclient [ ... ]>
    static QRegularExpression doctypeWithEntities(R"(<!DOCTYPE\s+\w+\s*\[[\s\S]*?\]>)",
                                                  QRegularExpression::CaseInsensitiveOption);
    result.replace(doctypeWithEntities, "<!-- DOCTYPE with entities expanded -->");

    // Also remove simple DOCTYPE declarations: <!DOCTYPE muclient>
    static QRegularExpression doctypeSimple(R"(<!DOCTYPE\s+\w+\s*>)",
                                            QRegularExpression::CaseInsensitiveOption);
    result.replace(doctypeSimple, "<!-- DOCTYPE removed -->");

    // Fix 2: Escape < and > in regex patterns within attribute values ONLY
    // IMPORTANT: We must NOT modify CDATA sections as they pass through without entity decoding
    // This handles only attribute values:
    // - Named capture groups: (?<name>...) -> (?&lt;name&gt;...)
    // - Lookbehind assertions: (?<! and (?<= -> (?&lt;! and (?&lt;=
    // - Literal < in match patterns: match="<MAPSTART>" -> match="&lt;MAPSTART&gt;"

    // Process only the parts outside of CDATA sections
    // Split by CDATA markers, process non-CDATA parts, then rejoin
    static QRegularExpression cdataSplit(R"((<!\[CDATA\[[\s\S]*?\]\]>))");
    QStringList parts = result.split(cdataSplit, Qt::KeepEmptyParts);
    QRegularExpressionMatchIterator cdataMatches = cdataSplit.globalMatch(result);

    QString processed;
    int partIndex = 0;

    // Interleave processed parts with CDATA sections
    for (const QString& part : parts) {
        // Process this non-CDATA part
        QString fixedPart = part;

        // Handle named capture groups: (?<name>)
        static QRegularExpression namedCapture(R"(\(\?<(\w+)>)");
        fixedPart.replace(namedCapture, R"((?&lt;\1&gt;)");

        // Handle lookbehind assertions: (?<! and (?<=
        static QRegularExpression lookbehind(R"(\(\?<([!=]))");
        fixedPart.replace(lookbehind, R"((?&lt;\1)");

        // Handle < and > inside attribute values
        static QRegularExpression attrWithLt(R"((\w+="[^"]*)<([^"&][^"]*"))");
        while (fixedPart.contains(attrWithLt)) {
            fixedPart.replace(attrWithLt, R"(\1&lt;\2)");
        }
        static QRegularExpression attrWithGt(R"((\w+="[^"]*)>([^"]*"))");
        while (fixedPart.contains(attrWithGt)) {
            fixedPart.replace(attrWithGt, R"(\1&gt;\2)");
        }

        processed += fixedPart;

        // Add the next CDATA section if there is one
        if (cdataMatches.hasNext()) {
            processed += cdataMatches.next().captured(0);
        }
    }

    return processed;
}

Plugin* WorldDocument::LoadPlugin(const QString& filepath, QString& errorMsg)
{
    errorMsg.clear();

    // Check file exists
    QFileInfo fileInfo(filepath);
    if (!fileInfo.exists()) {
        errorMsg = QString("Plugin file not found: %1").arg(filepath);
        qWarning() << errorMsg;
        return nullptr;
    }

    // Open file
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMsg = QString("Cannot open plugin file: %1").arg(filepath);
        qWarning() << errorMsg;
        return nullptr;
    }

    // Read and preprocess XML content to fix MUSHclient-specific issues
    QString xmlContent = QString::fromUtf8(file.readAll());
    file.close();
    xmlContent = preprocessPluginXml(xmlContent);

    // Create plugin object
    auto plugin = std::make_unique<Plugin>(this, this);
    plugin->m_strSource = fileInfo.absoluteFilePath();
    plugin->m_strDirectory = fileInfo.absolutePath();
    plugin->m_iLoadOrder = m_PluginList.size(); // Load order = current count

    // Parse XML from preprocessed string
    QXmlStreamReader xml(xmlContent);

    // Find <muclient> root element
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("muclient")) {
            break;
        }
    }

    if (xml.atEnd() || xml.hasError()) {
        if (xml.hasError()) {
            errorMsg = QString("XML parse error in %1: %2").arg(filepath).arg(xml.errorString());
        } else {
            errorMsg = QString("Invalid plugin file (no <muclient> root): %1").arg(filepath);
        }
        qWarning() << errorMsg;
        return nullptr;
    }

    // Parse all elements under <muclient>
    // Structure: <muclient><plugin/><aliases/><triggers/><timers/><script/></muclient>
    bool foundPlugin = false;

    while (!xml.atEnd()) {
        xml.readNext();

        // Stop at end of <muclient>
        if (xml.isEndElement() && xml.name() == QLatin1String("muclient")) {
            break;
        }

        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            // ---- Parse <plugin> attributes ----
            if (elementName == "plugin") {
                foundPlugin = true;

                QXmlStreamAttributes attrs = xml.attributes();

                plugin->m_strName = attrs.value("name").toString();
                plugin->m_strAuthor = attrs.value("author").toString();
                plugin->m_strID = attrs.value("id").toString();
                plugin->m_strPurpose = attrs.value("purpose").toString();
                plugin->m_strLanguage = attrs.value("language").toString();
                plugin->m_dVersion = attrs.value("version").toDouble();
                plugin->m_dRequiredVersion = attrs.value("requires").toDouble();
                plugin->m_bSaveState = (attrs.value("save_state").toString().toLower() == "y");

                // Parse sequence (default 5000 if not specified)
                if (attrs.hasAttribute("sequence")) {
                    plugin->m_iSequence = attrs.value("sequence").toInt();
                }

                // Parse dates
                QString dateWritten = attrs.value("date_written").toString();
                if (!dateWritten.isEmpty()) {
                    plugin->m_tDateWritten = QDateTime::fromString(dateWritten, Qt::ISODate);
                }

                QString dateModified = attrs.value("date_modified").toString();
                if (!dateModified.isEmpty()) {
                    plugin->m_tDateModified = QDateTime::fromString(dateModified, Qt::ISODate);
                }

                // Read <plugin> children (like <description>, <script>, <triggers>, <aliases>,
                // <timers>)
                while (!xml.atEnd()) {
                    xml.readNext();

                    if (xml.isEndElement() && xml.name() == QLatin1String("plugin")) {
                        break;
                    }

                    if (xml.isStartElement()) {
                        if (xml.name() == QLatin1String("description")) {
                            plugin->m_strDescription = xml.readElementText();
                        } else if (xml.name() == QLatin1String("script")) {
                            plugin->m_strScript = xml.readElementText();
                        } else if (xml.name() == QLatin1String("triggers")) {
                            loadTriggersFromXml(xml, plugin.get());
                        } else if (xml.name() == QLatin1String("aliases")) {
                            loadAliasesFromXml(xml, plugin.get());
                        } else if (xml.name() == QLatin1String("timers")) {
                            loadTimersFromXml(xml, plugin.get());
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
            }

            // ---- Parse <script> (sibling of <plugin>) ----
            else if (elementName == "script") {
                plugin->m_strScript = xml.readElementText();
            }

            // ---- Parse <triggers> (sibling of <plugin>) ----
            else if (elementName == "triggers") {
                loadTriggersFromXml(xml, plugin.get());
            }

            // ---- Parse <aliases> (sibling of <plugin>) ----
            else if (elementName == "aliases") {
                loadAliasesFromXml(xml, plugin.get());
            }

            // ---- Parse <timers> (sibling of <plugin>) ----
            else if (elementName == "timers") {
                loadTimersFromXml(xml, plugin.get());
            }

            // ---- Parse <include name="file.lua"/> for Lua file includes ----
            else if (elementName == "include") {
                QXmlStreamAttributes attrs = xml.attributes();
                QString includeName = attrs.value("name").toString();

                // Skip if this is a plugin include (handled elsewhere)
                if (attrs.hasAttribute("plugin")) {
                    xml.skipCurrentElement();
                    continue;
                }

                if (!includeName.isEmpty() && includeName.endsWith(".lua", Qt::CaseInsensitive)) {
                    // Resolve path relative to plugin file
                    QFileInfo pluginFileInfo(filepath);
                    QString includePath = pluginFileInfo.absolutePath() + "/" + includeName;

                    QFile includeFile(includePath);
                    if (includeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString includeContent = QString::fromUtf8(includeFile.readAll());
                        includeFile.close();

                        // Check if it's XML-wrapped Lua (MUSHclient format)
                        if (includeContent.trimmed().startsWith("<?xml") ||
                            includeContent.trimmed().startsWith("<!DOCTYPE")) {
                            // Extract Lua code from CDATA section
                            static QRegularExpression cdataRegex(R"(<!?\[CDATA\[([\s\S]*?)\]\]>)");
                            QRegularExpressionMatch match = cdataRegex.match(includeContent);
                            if (match.hasMatch()) {
                                includeContent = match.captured(1);
                            } else {
                                qWarning() << "Include file" << includePath
                                           << "appears to be XML but has no CDATA section";
                                includeContent.clear();
                            }
                        }

                        if (!includeContent.isEmpty()) {
                            // Prepend to plugin script so constants are available
                            plugin->m_strScript = includeContent + "\n\n" + plugin->m_strScript;
                            qDebug() << "Loaded include file:" << includeName << "for plugin"
                                     << plugin->m_strName;
                        }
                    } else {
                        qWarning() << "Could not open include file:" << includePath;
                    }
                }
                xml.skipCurrentElement();
            }

            // ---- Skip other elements ----
            else {
                xml.skipCurrentElement();
            }
        }
    }

    if (!foundPlugin) {
        errorMsg = QString("No <plugin> element found in file: %1").arg(filepath);
        qWarning() << errorMsg;
        return nullptr;
    }

    if (xml.hasError()) {
        errorMsg = QString("XML parse error: %1").arg(xml.errorString());
        qWarning() << errorMsg;
        return nullptr;
    }

    // ================================================================
    // VALIDATE PLUGIN
    // ================================================================

    if (plugin->m_strName.isEmpty()) {
        errorMsg = "Plugin has no name";
        qWarning() << errorMsg;
        return nullptr;
    }

    if (plugin->m_strID.isEmpty()) {
        errorMsg = QString("Plugin '%1' has no ID (GUID required)").arg(plugin->m_strName);
        qWarning() << errorMsg;
        return nullptr;
    }

    // Check for duplicate plugin by file path (matches original MUSHclient behavior)
    // Original MUSHclient checks by file path, not plugin ID, and silently skips duplicates
    if (Plugin* existingPlugin = FindPluginByFilePath(filepath)) {
        qCDebug(lcPlugin) << "Plugin already loaded from:" << filepath << "- skipping";
        // Return existing plugin (not an error)
        errorMsg.clear();
        return existingPlugin;
    }

    // Check for duplicate plugin ID (different file, same ID) - reject
    if (Plugin* existingPlugin = FindPluginByID(plugin->m_strID)) {
        errorMsg = QString("Plugin '%1' ID %2 already installed from '%3'")
                       .arg(plugin->m_strName)
                       .arg(plugin->m_strID)
                       .arg(existingPlugin->m_strSource);
        qWarning() << errorMsg;
        return nullptr;
    }

    // ================================================================
    // ADD TO PLUGIN LIST (must be before script execution so GetPluginInfo can find it)
    // ================================================================

    m_PluginList.push_back(std::move(plugin));
    Plugin* pluginPtr = m_PluginList.back().get(); // Get raw pointer for subsequent use

    // ================================================================
    // INITIALIZE SCRIPT ENGINE
    // ================================================================

    // ================================================================
    // Load plugin state BEFORE script initialization
    // ================================================================
    // This must happen before script parsing so that GetVariable() calls
    // in the plugin's initialization code can access persisted variables
    pluginPtr->LoadState();

    qCDebug(lcPlugin) << "Plugin script initialization:";
    qCDebug(lcPlugin) << "  Language:" << pluginPtr->m_strLanguage;
    qCDebug(lcPlugin) << "  Script length:" << pluginPtr->m_strScript.length();
    qCDebug(lcPlugin) << "  Script (first 50 chars):" << pluginPtr->m_strScript.left(50);

    if (!pluginPtr->m_strLanguage.isEmpty() && !pluginPtr->m_strScript.isEmpty()) {
        pluginPtr->m_ScriptEngine =
            std::make_unique<ScriptEngine>(this, pluginPtr->m_strLanguage, pluginPtr);
        pluginPtr->m_ScriptEngine->openLua();

        // Store plugin pointer in Lua registry for reliable context lookup
        // This allows API functions to identify the plugin even after modal dialogs
        pluginPtr->m_ScriptEngine->setPlugin(pluginPtr);

        // Set plugin as current context for API calls
        Plugin* savedPlugin = m_CurrentPlugin;
        m_CurrentPlugin = pluginPtr;

        // Execute plugin script
        bool error = pluginPtr->m_ScriptEngine->parseLua(
            pluginPtr->m_strScript, QString("Plugin %1").arg(pluginPtr->m_strName));
        if (error) {
            errorMsg = QString("Script error in plugin '%1'").arg(pluginPtr->m_strName);
            qWarning() << errorMsg;
            // Don't fail - plugin may still be useful without script
        }

        // Restore previous plugin context
        m_CurrentPlugin = savedPlugin;
    }

    // ================================================================
    // SORT PLUGIN LIST BY SEQUENCE
    // ================================================================

    // Sort plugins by sequence (negative first, then positive)
    std::sort(m_PluginList.begin(), m_PluginList.end(),
              [](const std::unique_ptr<Plugin>& a, const std::unique_ptr<Plugin>& b) {
                  return a->m_iSequence < b->m_iSequence;
              });

    // ================================================================
    // CALL OnPluginInstall CALLBACK
    // ================================================================

    qCDebug(lcPlugin) << "Plugin loaded:" << pluginPtr->m_strName << "ID:" << pluginPtr->m_strID;
    qCDebug(lcPlugin) << "  ScriptEngine:" << (pluginPtr->m_ScriptEngine ? "exists" : "NULL");
    qCDebug(lcPlugin) << "  m_pActiveOutputView:" << (void*)m_pActiveOutputView
                      << "(WorldDocument:" << (void*)this << ")";
    if (pluginPtr->m_ScriptEngine) {
        qCDebug(lcPlugin) << "  Calling ExecutePluginScript(ON_PLUGIN_INSTALL)";
        // Set plugin as current context for API calls within OnPluginInstall
        Plugin* savedPlugin = m_CurrentPlugin;
        m_CurrentPlugin = pluginPtr;
        pluginPtr->ExecutePluginScript(ON_PLUGIN_INSTALL);
        m_CurrentPlugin = savedPlugin;
        qCDebug(lcPlugin) << "  ExecutePluginScript returned";
    } else {
        qCDebug(lcPlugin) << "  Skipping OnPluginInstall - no script engine";
    }

    // Notify other plugins that plugin list changed
    PluginListChanged();

    return pluginPtr;
}

// ============================================================================
// Plugin Enable/Disable
// ============================================================================

/**
 * EnablePlugin - Enable or disable a plugin
 *
 * Plugin Management
 *
 * Sets plugin's m_bEnabled flag and calls OnPluginEnable or OnPluginDisable callback.
 *
 * @param pluginID Plugin GUID
 * @param enabled true to enable, false to disable
 * @return true on success, false if plugin not found
 */
bool WorldDocument::EnablePlugin(const QString& pluginID, bool enabled)
{
    Plugin* plugin = FindPluginByID(pluginID);
    if (!plugin) {
        qWarning() << "EnablePlugin: plugin not found:" << pluginID;
        return false;
    }

    // Already in desired state?
    if (plugin->m_bEnabled == enabled) {
        return true;
    }

    plugin->m_bEnabled = enabled;

    // Call lifecycle callbacks
    if (enabled) {
        qCDebug(lcPlugin) << "Plugin enabled:" << plugin->m_strName;
        plugin->ExecutePluginScript(ON_PLUGIN_ENABLE);
    } else {
        qCDebug(lcPlugin) << "Plugin disabled:" << plugin->m_strName;
        plugin->ExecutePluginScript(ON_PLUGIN_DISABLE);
    }

    return true;
}

/**
 * UnloadPlugin - Unload and delete a plugin
 *
 * Plugin Management
 *
 * Removes plugin from list, calls OnPluginClose callback, and deletes the plugin object.
 *
 * @param pluginID Plugin GUID
 * @return true on success, false if plugin not found or currently executing
 */
bool WorldDocument::UnloadPlugin(const QString& pluginID)
{
    Plugin* plugin = FindPluginByID(pluginID);
    if (!plugin) {
        qWarning() << "UnloadPlugin: plugin not found:" << pluginID;
        return false;
    }

    // Safety: don't unload plugin that's currently executing
    if (plugin->m_bExecutingScript) {
        qWarning() << "UnloadPlugin: plugin is currently executing:" << plugin->m_strName;
        return false;
    }

    qCDebug(lcPlugin) << "Unloading plugin:" << plugin->m_strName;

    // Remove from list (unique_ptr will automatically delete the plugin)
    auto it =
        std::find_if(m_PluginList.begin(), m_PluginList.end(),
                     [plugin](const std::unique_ptr<Plugin>& p) { return p.get() == plugin; });
    if (it != m_PluginList.end()) {
        m_PluginList.erase(it); // OnPluginClose called in destructor
    }

    // Notify other plugins
    PluginListChanged();

    return true;
}

/**
 * PluginListChanged - Notify all plugins that plugin list has changed
 *
 * Plugin Management
 *
 * Calls OnPluginListChanged callback in all enabled plugins.
 * Uses recursion protection to prevent infinite loops.
 */
void WorldDocument::PluginListChanged()
{
    static bool bInPluginListChanged = false;

    // Prevent recursion
    if (bInPluginListChanged) {
        return;
    }

    bInPluginListChanged = true;

    // Notify all plugins that the plugin list has changed
    SendToAllPluginCallbacks(ON_PLUGIN_LIST_CHANGED);
    qCDebug(lcPlugin) << "PluginListChanged: notifying" << m_PluginList.size() << "plugins";

    bInPluginListChanged = false;
}

// ============================================================================
// Plugin Callbacks (Stubs for now)
// ============================================================================

/**
 * SendToAllPluginCallbacks - Call all plugins with no arguments
 *
 * Plugin Callbacks
 *
 * Iterates through all enabled plugins and calls the specified callback function.
 *
 * @param callbackName Callback function name (e.g., "OnPluginConnect")
 */
void WorldDocument::SendToAllPluginCallbacks(const QString& callbackName)
{
    Plugin* savedPlugin = m_CurrentPlugin;

    for (const auto& plugin : m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        m_CurrentPlugin = plugin.get();
        plugin->ExecutePluginScript(callbackName);
    }

    m_CurrentPlugin = savedPlugin;
}

/**
 * SendToAllPluginCallbacks - Call all plugins with string argument
 *
 * Plugin Callbacks
 *
 * Iterates through all enabled plugins and calls the specified callback function.
 * Stops iteration if a plugin returns false and bStopOnFalse is true.
 *
 * @param callbackName Callback function name
 * @param arg String argument
 * @param bStopOnFalse Stop if callback returns false
 * @return true if all callbacks returned true (or none existed), false otherwise
 */
bool WorldDocument::SendToAllPluginCallbacks(const QString& callbackName, const QString& arg,
                                             bool bStopOnFalse)
{
    Plugin* savedPlugin = m_CurrentPlugin;
    bool result = true;

    for (const auto& plugin : m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        m_CurrentPlugin = plugin.get();
        bool pluginResult = plugin->ExecutePluginScript(callbackName, arg);

        if (!pluginResult) {
            result = false;
            if (bStopOnFalse) {
                break;
            }
        }
    }

    m_CurrentPlugin = savedPlugin;
    return result;
}

/**
 * SendToAllPluginCallbacks - Call all plugins with int + string arguments
 *
 * Plugin Callbacks
 *
 * Iterates through all enabled plugins and calls the specified callback function.
 * Stops iteration if a plugin returns true and bStopOnTrue is true.
 *
 * Used by TELNET_REQUEST to allow plugins to handle telnet negotiations.
 *
 * @param callbackName Callback function name
 * @param arg1 Integer argument (e.g., telnet option number)
 * @param arg2 String argument (e.g., "WILL", "DO", "SENT_DO", etc.)
 * @param bStopOnTrue Stop if callback returns true (plugin handled it)
 * @return true if any callback returned true, false otherwise
 */
bool WorldDocument::SendToAllPluginCallbacks(const QString& callbackName, qint32 arg1,
                                             const QString& arg2, bool bStopOnTrue)
{
    Plugin* savedPlugin = m_CurrentPlugin;
    bool result = false;

    for (const auto& plugin : m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        m_CurrentPlugin = plugin.get();
        bool pluginResult = plugin->ExecutePluginScript(callbackName, arg1, arg2);

        if (pluginResult) {
            result = true;
            if (bStopOnTrue) {
                break;
            }
        }
    }

    m_CurrentPlugin = savedPlugin;
    return result;
}

/**
 * SendToAllPluginCallbacks - Call all plugins with int + int + string arguments
 *
 * Plugin Callbacks
 *
 * Used by ON_PLUGIN_MOUSE_MOVED (x, y, miniWindowId)
 *
 * @param callbackName Callback function name
 * @param arg1 First integer (e.g., x coordinate)
 * @param arg2 Second integer (e.g., y coordinate)
 * @param arg3 String argument (e.g., miniwindow ID)
 * @return true if all callbacks succeeded
 */
bool WorldDocument::SendToAllPluginCallbacks(const QString& callbackName, qint32 arg1, qint32 arg2,
                                             const QString& arg3)
{
    Plugin* savedPlugin = m_CurrentPlugin;
    bool result = true;

    for (const auto& plugin : m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        m_CurrentPlugin = plugin.get();
        bool pluginResult = plugin->ExecutePluginScript(callbackName, arg1, arg2, arg3);

        if (!pluginResult) {
            result = false;
        }
    }

    m_CurrentPlugin = savedPlugin;
    return result;
}

/**
 * SendToFirstPluginCallbacks - Call plugins until one returns true
 *
 * Plugin Callbacks
 *
 * Used by ON_PLUGIN_TRACE and ON_PLUGIN_PACKET_DEBUG where only one plugin
 * should handle the callback.
 *
 * @param callbackName Callback function name
 * @param arg String argument
 * @return true if any plugin returned true (handled it)
 */
bool WorldDocument::SendToFirstPluginCallbacks(const QString& callbackName, const QString& arg)
{
    Plugin* savedPlugin = m_CurrentPlugin;

    for (const auto& plugin : m_PluginList) {
        if (!plugin || !plugin->m_bEnabled) {
            continue;
        }

        m_CurrentPlugin = plugin.get();
        bool pluginResult = plugin->ExecutePluginScript(callbackName, arg);

        if (pluginResult) {
            m_CurrentPlugin = savedPlugin;
            return true; // First plugin handled it
        }
    }

    m_CurrentPlugin = savedPlugin;
    return false; // No plugin handled it
}

// ============================================================================
// Screendraw and Trace Callbacks
// ============================================================================

/**
 * Screendraw - Notify plugins when a line is drawn to the output window
 *
 * Screen Accessibility
 *
 * Based on CMUSHclientDoc::Screendraw() in doc.cpp
 *
 * Called when lines are displayed on screen, for accessibility/screen reader support.
 * Uses a static recursion guard to prevent infinite loops.
 *
 * @param iType Line type: 0 = MUD output, 1 = note (COMMENT), 2 = command (USER_INPUT)
 * @param iLog Whether the line should be logged
 * @param sText The text being displayed
 */
void WorldDocument::Screendraw(qint32 iType, bool iLog, const QString& sText)
{
    static bool bInScreendraw = false;

    // Don't recurse into infinite loops
    if (bInScreendraw) {
        return;
    }

    bInScreendraw = true;
    SendToAllPluginCallbacks(ON_PLUGIN_SCREENDRAW, iType, iLog ? 1 : 0, sText);
    bInScreendraw = false;
}

/**
 * Trace - Output trace message with plugin callback support
 *
 * Debug Tracing
 *
 * Based on CMUSHclientDoc::Trace() in methods_tracing.cpp
 *
 * If a plugin handles the trace message (returns true from OnPluginTrace),
 * the message is not displayed. Otherwise, it's shown as a note prefixed with "TRACE: ".
 *
 * Uses a recursion guard to prevent infinite loops where the trace handler
 * would itself trigger more traces.
 *
 * @param message The trace message to output
 */
void WorldDocument::Trace(const QString& message)
{
    // Do nothing if not tracing
    if (!m_bTrace) {
        return;
    }

    // Temporarily disable trace to prevent infinite loops
    // where the plugin trace handler triggers more traces
    m_bTrace = false;

    // See if a plugin will handle the trace message
    if (SendToFirstPluginCallbacks(ON_PLUGIN_TRACE, message)) {
        m_bTrace = true;
        return; // Plugin handled it, don't display
    }

    m_bTrace = true;

    // Add newline and prefix, then display as note
    QString fullMsg = QStringLiteral("TRACE: %1").arg(message);
    note(fullMsg);
}

// ============================================================================
// Plugin World Serialization
// ============================================================================

/**
 * savePluginsToXml - Save plugin list to world XML
 *
 * Plugin World Serialization
 *
 * Based on CMUSHclientDoc::Save_Plugins_XML()
 *
 * Saves references to installed plugins (filepath, enabled state) to the world file.
 * Does NOT save plugin contents (those are in the plugin XML files).
 * Global plugins (m_bGlobal) are not saved to world file.
 *
 * @param xml QXmlStreamWriter to write to
 */
void WorldDocument::savePluginsToXml(QXmlStreamWriter& xml)
{
    if (m_PluginList.empty()) {
        return; // No plugins to save
    }

    xml.writeComment(" plugins ");

    for (const auto& plugin : m_PluginList) {
        if (!plugin) {
            continue;
        }

        // Don't save global plugins (loaded from preferences, not world-specific)
        if (plugin->m_bGlobal) {
            continue;
        }

        // Write <include name="path/to/plugin.xml" plugin="y"/>
        xml.writeStartElement("include");
        xml.writeAttribute("name", plugin->m_strSource);
        xml.writeAttribute("plugin", "y");
        xml.writeEndElement(); // include
    }
}

/**
 * loadPluginsFromXml - Load plugins from world XML
 *
 * Plugin World Serialization
 *
 * Based on CMUSHclientDoc::Load_One_Include_XML()
 *
 * Reads <include> elements with plugin="y" attribute and loads each plugin.
 * Handles missing plugin files gracefully (warns and continues).
 * Sets enabled state and sequence from saved values.
 *
 * @param xml QXmlStreamReader positioned at start of section containing <include> elements
 */
void WorldDocument::loadPluginsFromXml(QXmlStreamReader& xml)
{
    int loadOrder = 0;

    // Read all <include> elements
    while (!xml.atEnd()) {
        xml.readNext();

        // Stop if we've exited the parent element
        if (xml.isEndElement()) {
            break;
        }

        // Look for <include plugin="y"> elements
        if (xml.isStartElement() && xml.name() == QLatin1String("include")) {
            QXmlStreamAttributes attrs = xml.attributes();

            // Check if this is a plugin include
            if (attrs.value("plugin").toString().toLower() != "y") {
                continue; // Not a plugin, skip
            }

            QString filepath = attrs.value("name").toString();
            if (filepath.isEmpty()) {
                qWarning() << "loadPluginsFromXml: <include> has no name attribute";
                continue;
            }

            // Load the plugin
            QString errorMsg;
            Plugin* plugin = LoadPlugin(filepath, errorMsg);

            if (!plugin) {
                qWarning() << "loadPluginsFromXml: Failed to load plugin:" << filepath;
                qWarning() << "  Error:" << errorMsg;
                // Continue loading other plugins
                continue;
            }

            // Set load order
            plugin->m_iLoadOrder = loadOrder++;

            qCDebug(lcPlugin) << "loadPluginsFromXml: Loaded plugin:" << plugin->m_strName << "from"
                              << filepath;
        }
    }

    // Sort plugins by sequence after loading all
    if (!m_PluginList.empty()) {
        std::sort(m_PluginList.begin(), m_PluginList.end(),
                  [](const std::unique_ptr<Plugin>& a, const std::unique_ptr<Plugin>& b) {
                      return a->m_iSequence < b->m_iSequence;
                  });

        qCDebug(lcPlugin) << "loadPluginsFromXml: Loaded" << m_PluginList.size() << "plugins";
    }
}

// ============================================================================
// MiniWindow Management
// ============================================================================

/**
 * WindowFont - Add font to miniwindow
 *
 * MiniWindow Management
 *
 * Based on CMUSHclientDoc::WindowFont() from methods_miniwindows.cpp
 *
 * Creates or updates a font in the miniwindow's font map.
 * The font is identified by fontId and can be used later with WindowText.
 *
 * @param windowName Name of miniwindow
 * @param fontId Font identifier (e.g., "f1")
 * @param fontName Font family name (e.g., "Courier New")
 * @param size Font size in points
 * @param bold Bold flag
 * @param italic Italic flag
 * @param underline Underline flag
 * @param strikeout Strikeout flag
 * @param charset Font charset (not used in Qt)
 * @param pitchAndFamily Font pitch and family (not used in Qt)
 * @return Error code (eOK on success, eNoSuchWindow if window not found)
 */
qint32 WorldDocument::WindowFont(const QString& windowName, const QString& fontId,
                                 const QString& fontName, double size, bool bold, bool italic,
                                 bool underline, bool strikeout, qint16 charset,
                                 qint16 pitchAndFamily)
{
    Q_UNUSED(charset);        // Qt handles charset automatically
    Q_UNUSED(pitchAndFamily); // Qt handles pitch/family automatically

    // Find miniwindow by name
    auto it = m_MiniWindowMap.find(windowName);
    if (it == m_MiniWindowMap.end() || !it->second) {
        return 30010; // eNoSuchWindow
    }

    MiniWindow* miniWindow = it->second.get();

    // Call MiniWindow::Font() to add/update the font
    return miniWindow->Font(fontId, fontName, size, bold, italic, underline, strikeout);
}
