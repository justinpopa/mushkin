// xml_serialization.cpp
// Port XML Serialization (Load/Save World Files)
//
// Implements XML save/load for MUSHclient world files (.mcl)
// Maintains backward compatibility with original MFC version
//
// Ported from: xml/xml_serialize.cpp (original MUSHclient)

#include "xml_serialization.h"
#include "../storage/database.h"
#include "config_options.h"
#include "logging.h"
#include "plugin.h" // For Plugin member access
#include "world_document.h"
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm> // for std::sort

namespace {

/**
 * Resolve plugin path - matches original MUSHclient logic
 *
 * Original logic (xml_load_world.cpp):
 * - First handles special placeholders ($PLUGINSDEFAULTDIR, $WORLDDIR, etc.)
 * - For relative paths, prepends m_strPluginsDirectory (plugins directory)
 *
 * Search order:
 * 1. If absolute path, use directly
 * 2. Handle $PLUGINSDEFAULTDIR, $WORLDDIR placeholders
 * 3. Relative to PluginsDirectory preference (primary for plugins)
 * 4. Relative to working directory's worlds/plugins/
 * 5. Relative to world file directory (fallback)
 */
QString resolvePluginPath(const QString& pluginPath, const QString& worldFilePath)
{
    // Convert Windows backslashes to forward slashes for cross-platform compatibility
    QString path = pluginPath;
    path.replace('\\', '/');

    // Get world file directory for $WORLDDIR replacement
    QFileInfo worldFileInfo(worldFilePath);
    QString worldDir = worldFileInfo.absolutePath();

    // Get plugins directory
    Database* db = Database::instance();
    QString pluginsDir = db->getPreference("PluginsDirectory", "./worlds/plugins/");

    // Convert Windows backslashes in plugins directory path
    pluginsDir.replace('\\', '/');

    // If pluginsDir is relative, resolve against application directory
    // (where the binary is located, not current working directory)
    if (!QDir::isAbsolutePath(pluginsDir)) {
        pluginsDir = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(pluginsDir);
    }

    // Clean up the path (removes . and .., normalizes slashes)
    pluginsDir = QDir::cleanPath(pluginsDir);

    // Handle special placeholders (like original MUSHclient)
    path.replace("$PLUGINSDEFAULTDIR", QDir(pluginsDir).absolutePath());
    path.replace("$WORLDDIR", worldDir);
    path.replace("$PROGRAMDIR", QCoreApplication::applicationDirPath());

    // If absolute after placeholder replacement, use directly
    if (QDir::isAbsolutePath(path)) {
        return path;
    }

    // Check if path starts with relative indicators (../, ./, etc.)
    bool hasRelativePrefix = path.startsWith("../") || path.startsWith("..\\") ||
                             path.startsWith("./") || path.startsWith(".\\");

    if (hasRelativePrefix) {
        // Resolve relative to world file directory
        return QDir(worldDir).absoluteFilePath(path);
    }

    // For simple relative paths, try plugins directory first (matches original)
    QString fullPath = QDir(pluginsDir).absoluteFilePath(path);
    if (QFile::exists(fullPath)) {
        return fullPath;
    }

    // Fallback: try world file directory
    fullPath = QDir(worldDir).absoluteFilePath(path);
    if (QFile::exists(fullPath)) {
        return fullPath;
    }

    // Not found - return plugins directory path (original behavior)
    return QDir(pluginsDir).absoluteFilePath(path);
}

} // anonymous namespace

namespace XmlSerialization {

// ============================================================================
// ENCODING DETECTION
// ============================================================================

bool IsArchiveXML(QFile& file)
{
    // Save current position
    qint64 originalPos = file.pos();

    // Read first 500 bytes for signature detection
    file.seek(0);
    QByteArray buffer = file.read(500);

    // Restore file position
    file.seek(originalPos);

    if (buffer.isEmpty()) {
        return false;
    }

    // Check for UTF-8 BOM (EF BB BF)
    if (buffer.size() >= 3 && (unsigned char)buffer[0] == 0xEF &&
        (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF) {
        qCDebug(lcWorld) << "Detected UTF-8 BOM";
    }

    // Check for UTF-16LE BOM (FF FE)
    if (buffer.size() >= 2 && (unsigned char)buffer[0] == 0xFF &&
        (unsigned char)buffer[1] == 0xFE) {
        qCDebug(lcWorld) << "Detected UTF-16LE BOM";
    }

    // Convert to string for signature search
    QString content = QString::fromUtf8(buffer);

    // Look for XML signatures
    // Note: Using case-insensitive contains for robustness
    QStringList signatures = {"<?xml",  "<!--",      "<!DOCTYPE", "<muclient",
                              "<world", "<triggers", "<aliases",  "<timers"};

    for (const QString& sig : signatures) {
        if (content.contains(sig, Qt::CaseInsensitive)) {
            qCDebug(lcWorld) << "Found XML signature:" << sig;
            return true;
        }
    }

    return false;
}

// ============================================================================
// SAVE WORLD TO XML
// ============================================================================

bool SaveWorldXML(WorldDocument* doc, const QString& filename)
{
    if (!doc) {
        qWarning() << "SaveWorldXML: null document";
        return false;
    }

    // Notify plugins that world is about to be saved
    // This allows plugins to save their state before the world file is written
    doc->SendToAllPluginCallbacks(ON_PLUGIN_WORLD_SAVE);

    // Atomic save: write to temp file, then rename
    // This prevents corruption if app crashes mid-save
    QString tempFilename = filename + ".tmp";
    QString backupFilename = filename + ".bak";

    QFile file(tempFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "SaveWorldXML: cannot open temp file for writing:" << tempFilename;
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    // Write XML declaration
    writer.writeStartDocument("1.0");

    // Write DOCTYPE
    writer.writeDTD("<!DOCTYPE muclient>");

    // Write root element
    writer.writeStartElement("muclient");

    // Write world element
    writer.writeStartElement("world");

    // ========================================================================
    // SAVE NUMERIC OPTIONS
    // ========================================================================

    for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
        const tConfigurationNumericOption& opt = OptionsTable[i];

        // Calculate pointer to field in document
        const char* basePtr = reinterpret_cast<const char*>(doc);
        const char* fieldPtr = basePtr + opt.iOffset;

        double value = 0.0;

        // Read value based on field length
        switch (opt.iLength) {
            case 1: // unsigned char or bool
                if (opt.iMaximum == 0 && opt.iMinimum == 0) {
                    // Boolean
                    value = *reinterpret_cast<const bool*>(fieldPtr) ? 1.0 : 0.0;
                } else {
                    value = *reinterpret_cast<const unsigned char*>(fieldPtr);
                }
                break;

            case 2: // unsigned short or qint16
                value = *reinterpret_cast<const quint16*>(fieldPtr);
                break;

            case 4: // qint32, quint32, or float
                if (opt.iFlags & OPT_DOUBLE) {
                    value = *reinterpret_cast<const float*>(fieldPtr);
                } else if (opt.iFlags & OPT_RGB_COLOUR) {
                    // RGB color stored as QRgb (quint32)
                    QRgb rgb = *reinterpret_cast<const QRgb*>(fieldPtr);
                    value = rgb;
                } else {
                    value = *reinterpret_cast<const qint32*>(fieldPtr);
                }
                break;

            case 8: // qint64 or double
                if (opt.iFlags & OPT_DOUBLE) {
                    value = *reinterpret_cast<const double*>(fieldPtr);
                } else {
                    value = *reinterpret_cast<const qint64*>(fieldPtr);
                }
                break;

            default:
                qWarning() << "Unknown field length" << opt.iLength << "for option" << opt.pName;
                continue;
        }

        // Handle custom color indexing (add 1 when saving)
        if (opt.iFlags & OPT_CUSTOM_COLOUR) {
            value += 1.0;
        }

        // Write as XML attribute
        // Boolean values: write as "y" or "n"
        if (opt.iMaximum == 0.0 && opt.iMinimum == 0.0) {
            writer.writeAttribute(opt.pName, value != 0.0 ? "y" : "n");
        } else {
            writer.writeAttribute(opt.pName, QString::number(value, 'g', 15));
        }
    }

    // ========================================================================
    // SAVE STRING OPTIONS (ATTRIBUTES FIRST)
    // ========================================================================
    // Important: All attributes must be written before any child elements!

    // First pass: Write single-line string options as attributes
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

        // Calculate pointer to field in document
        const char* basePtr = reinterpret_cast<const char*>(doc);
        const char* fieldPtr = basePtr + opt.iOffset;

        // Read QString value
        const QString* strPtr = reinterpret_cast<const QString*>(fieldPtr);
        QString value = *strPtr;

        // Skip multi-line options in this pass
        if ((opt.iFlags & OPT_MULTLINE) || value.contains('\n')) {
            continue;
        }

        // Handle password encoding (base64)
        if (opt.iFlags & OPT_PASSWORD) {
            if (!value.isEmpty()) {
                QByteArray encoded = value.toUtf8().toBase64();
                value = QString::fromLatin1(encoded);
            }
        }

        // Single-line: write as attribute
        writer.writeAttribute(opt.pName, value);
    }

    // Second pass: Write multi-line string options as child elements
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

        // Calculate pointer to field in document
        const char* basePtr = reinterpret_cast<const char*>(doc);
        const char* fieldPtr = basePtr + opt.iOffset;

        // Read QString value
        const QString* strPtr = reinterpret_cast<const QString*>(fieldPtr);
        QString value = *strPtr;

        // Only process multi-line options in this pass
        if (!((opt.iFlags & OPT_MULTLINE) || value.contains('\n'))) {
            continue;
        }

        // Handle password encoding (base64)
        if (opt.iFlags & OPT_PASSWORD) {
            if (!value.isEmpty()) {
                QByteArray encoded = value.toUtf8().toBase64();
                value = QString::fromLatin1(encoded);
            }
        }

        // Multi-line: write as child element with CDATA
        writer.writeStartElement(opt.pName);
        writer.writeCDATA(value);
        writer.writeEndElement();
    }

    // ========================================================================
    // TRIGGERS AND ALIASES
    // ========================================================================

    doc->saveTriggersToXml(writer);
    doc->saveAliasesToXml(writer);

    // ========================================================================
    // TIMERS
    // ========================================================================

    doc->saveTimersToXml(writer);

    // ========================================================================
    // VARIABLES
    // ========================================================================

    doc->saveVariablesToXml(writer);

    // ========================================================================
    // ACCELERATORS (User-defined keyboard shortcuts)
    // ========================================================================

    doc->saveAcceleratorsToXml(writer);

    // ========================================================================
    // PLUGINS
    // ========================================================================

    doc->savePluginsToXml(writer);

    // ========================================================================
    // COMMAND HISTORY
    // ========================================================================

    if (!doc->m_commandHistory.isEmpty()) {
        writer.writeStartElement("command_history");

        for (const QString& command : doc->m_commandHistory) {
            writer.writeTextElement("command", command);
        }

        writer.writeEndElement(); // command_history
    }

    // Close world element
    writer.writeEndElement(); // world

    // Close root element
    writer.writeEndElement(); // muclient

    // Finish document
    writer.writeEndDocument();

    file.close();

    if (writer.hasError()) {
        qWarning() << "SaveWorldXML: XML writer error";
        QFile::remove(tempFilename); // Clean up temp file
        return false;
    }

    // Atomic rename: backup existing file, then rename temp to final
    if (QFile::exists(filename)) {
        // Remove old backup if it exists
        QFile::remove(backupFilename);
        // Rename current file to backup
        if (!QFile::rename(filename, backupFilename)) {
            qWarning() << "SaveWorldXML: failed to create backup file";
            // Continue anyway - save is more important than backup
        }
    }

    // Rename temp file to final filename
    if (!QFile::rename(tempFilename, filename)) {
        qWarning() << "SaveWorldXML: failed to rename temp file to final";
        // Try to restore from backup
        if (QFile::exists(backupFilename)) {
            QFile::rename(backupFilename, filename);
        }
        return false;
    }

    qCDebug(lcWorld) << "SaveWorldXML: successfully saved to" << filename;
    return true;
}

// ============================================================================
// LOAD WORLD FROM XML
// ============================================================================

bool LoadWorldXML(WorldDocument* doc, const QString& filename)
{
    if (!doc) {
        qWarning() << "LoadWorldXML: null document";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "LoadWorldXML: cannot open file for reading:" << filename;
        return false;
    }

    // Check if file is actually XML
    if (!IsArchiveXML(file)) {
        qWarning() << "LoadWorldXML: file does not appear to be XML:" << filename;
        file.close();
        return false;
    }

    // Reset to beginning for reading
    file.seek(0);

    QXmlStreamReader reader(&file);

    // Skip to <muclient> root element
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QLatin1String("muclient")) {
            break;
        }
    }

    if (reader.atEnd() || reader.hasError()) {
        qWarning() << "LoadWorldXML: no <muclient> root element found";
        file.close();
        return false;
    }

    // Read <world> element
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement() && reader.name() == QLatin1String("world")) {
            // Process world attributes
            QXmlStreamAttributes attrs = reader.attributes();

            // ================================================================
            // LOAD NUMERIC OPTIONS
            // ================================================================

            for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
                const tConfigurationNumericOption& opt = OptionsTable[i];

                if (!attrs.hasAttribute(opt.pName)) {
                    continue; // Use default value (already initialized in constructor)
                }

                QString attrValue = attrs.value(opt.pName).toString();
                double value = 0.0;

                // Parse boolean values
                if (opt.iMaximum == 0.0 && opt.iMinimum == 0.0) {
                    // Boolean: "y", "n", "1", "0", "true", "false"
                    value = (attrValue == "y" || attrValue == "1" || attrValue.toLower() == "true")
                                ? 1.0
                                : 0.0;
                } else if (opt.iFlags & OPT_RGB_COLOUR) {
                    // RGB colors can be numeric or named (e.g., "white", "black", "red")
                    bool ok;
                    value = attrValue.toDouble(&ok);
                    if (!ok) {
                        // Try parsing as color name
                        QColor color(attrValue);
                        if (color.isValid()) {
                            // Convert to Windows COLORREF format (BGR)
                            value = color.red() | (color.green() << 8) | (color.blue() << 16);
                        }
                        // If invalid, value stays 0 and will be skipped below
                    }
                } else {
                    value = attrValue.toDouble();
                }

                // Handle custom color indexing (subtract 1 when loading)
                if (opt.iFlags & OPT_CUSTOM_COLOUR) {
                    value -= 1.0;
                }

                // Clamp to min/max if specified
                if (!(opt.iMaximum == 0.0 && opt.iMinimum == 0.0)) {
                    if (value < opt.iMinimum)
                        value = opt.iMinimum;
                    if (value > opt.iMaximum)
                        value = opt.iMaximum;
                }

                // Write value to document
                char* basePtr = reinterpret_cast<char*>(doc);
                char* fieldPtr = basePtr + opt.iOffset;

                switch (opt.iLength) {
                    case 1: // unsigned char or bool
                        if (opt.iMaximum == 0.0 && opt.iMinimum == 0.0) {
                            *reinterpret_cast<bool*>(fieldPtr) = (value != 0.0);
                        } else {
                            *reinterpret_cast<unsigned char*>(fieldPtr) =
                                static_cast<unsigned char>(value);
                        }
                        break;

                    case 2: // quint16
                        *reinterpret_cast<quint16*>(fieldPtr) = static_cast<quint16>(value);
                        break;

                    case 4: // qint32, quint32, float, or QRgb
                        if (opt.iFlags & OPT_DOUBLE) {
                            *reinterpret_cast<float*>(fieldPtr) = static_cast<float>(value);
                        } else if (opt.iFlags & OPT_RGB_COLOUR) {
                            // For RGB colors, 0 might mean "use default" in old MUSHclient files
                            // Only set if value is non-zero, otherwise keep the default
                            if (value != 0.0) {
                                *reinterpret_cast<QRgb*>(fieldPtr) = static_cast<QRgb>(value);
                            }
                        } else {
                            *reinterpret_cast<qint32*>(fieldPtr) = static_cast<qint32>(value);
                        }
                        break;

                    case 8: // qint64 or double
                        if (opt.iFlags & OPT_DOUBLE) {
                            *reinterpret_cast<double*>(fieldPtr) = value;
                        } else {
                            *reinterpret_cast<qint64*>(fieldPtr) = static_cast<qint64>(value);
                        }
                        break;
                }
            }

            // ================================================================
            // LOAD STRING OPTIONS (attributes)
            // ================================================================

            for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
                const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

                if (!attrs.hasAttribute(opt.pName)) {
                    continue; // Check for element form below
                }

                QString value = attrs.value(opt.pName).toString();

                // Handle password decoding (base64)
                if (opt.iFlags & OPT_PASSWORD) {
                    if (!value.isEmpty()) {
                        QByteArray decoded = QByteArray::fromBase64(value.toLatin1());
                        value = QString::fromUtf8(decoded);
                    }
                }

                // Write to document
                char* basePtr = reinterpret_cast<char*>(doc);
                char* fieldPtr = basePtr + opt.iOffset;
                QString* strPtr = reinterpret_cast<QString*>(fieldPtr);
                *strPtr = value;
            }

            // ================================================================
            // LOAD STRING OPTIONS (child elements for multiline)
            // ================================================================

            while (!reader.atEnd()) {
                reader.readNext();

                if (reader.isEndElement() && reader.name() == QLatin1String("world")) {
                    break; // End of world element
                }

                if (reader.isStartElement()) {
                    QString elementName = reader.name().toString();

                    // Load triggers and aliases
                    if (elementName == "triggers") {
                        doc->loadTriggersFromXml(reader);
                        continue;
                    }

                    if (elementName == "aliases") {
                        doc->loadAliasesFromXml(reader);
                        continue;
                    }

                    // Load timers
                    if (elementName == "timers") {
                        doc->loadTimersFromXml(reader);
                        continue;
                    }

                    // Load plugins from <include plugin="y"> elements
                    if (elementName == "include") {
                        QXmlStreamAttributes attrs = reader.attributes();
                        if (attrs.value("plugin").toString().toLower() == "y") {
                            QString pluginPath = attrs.value("name").toString();
                            if (!pluginPath.isEmpty()) {
                                QString fullPath = resolvePluginPath(pluginPath, filename);

                                QString errorMsg;
                                Plugin* plugin = doc->LoadPlugin(fullPath, errorMsg);
                                if (plugin) {
                                    qDebug() << "Loaded plugin:" << plugin->m_strName
                                             << "| Aliases:" << plugin->m_AliasMap.size()
                                             << "| Triggers:" << plugin->m_TriggerMap.size()
                                             << "| Enabled:" << plugin->m_bEnabled;
                                } else {
                                    qWarning() << "Failed to load plugin:" << pluginPath;
                                    qWarning() << "  Error:" << errorMsg;
                                }
                            }
                        }
                        reader.skipCurrentElement();
                        continue;
                    }

                    // Load command history
                    if (elementName == "command_history") {
                        doc->m_commandHistory.clear();

                        while (!reader.atEnd()) {
                            reader.readNext();

                            if (reader.isEndElement() &&
                                reader.name() == QLatin1String("command_history")) {
                                break;
                            }

                            if (reader.isStartElement() &&
                                reader.name() == QLatin1String("command")) {
                                QString command = reader.readElementText();
                                if (!command.isEmpty()) {
                                    doc->m_commandHistory.append(command);
                                }
                            }
                        }

                        // Reset history position to end
                        doc->m_historyPosition = doc->m_commandHistory.count();
                        doc->m_iHistoryStatus = eAtBottom;

                        qCDebug(lcWorld)
                            << "Loaded" << doc->m_commandHistory.count() << "commands from history";
                        continue;
                    }

                    // Load variables
                    if (elementName == "variables") {
                        doc->loadVariablesFromXml(reader);
                        continue;
                    }

                    // Load accelerators (user-defined keyboard shortcuts)
                    if (elementName == "accelerators") {
                        doc->loadAcceleratorsFromXml(reader);
                        continue;
                    }

                    // Load macros (original MUSHclient format -> accelerators)
                    if (elementName == "macros") {
                        doc->loadMacrosFromXml(reader);
                        continue;
                    }

                    // Load keypad (original MUSHclient format -> accelerators)
                    if (elementName == "keypad") {
                        doc->loadKeypadFromXml(reader);
                        continue;
                    }

                    // Check if this is a string option in element form
                    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
                        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

                        if (elementName == opt.pName) {
                            QString value = reader.readElementText();

                            // Handle password decoding
                            if (opt.iFlags & OPT_PASSWORD) {
                                if (!value.isEmpty()) {
                                    QByteArray decoded = QByteArray::fromBase64(value.toLatin1());
                                    value = QString::fromUtf8(decoded);
                                }
                            }

                            // Write to document
                            char* basePtr = reinterpret_cast<char*>(doc);
                            char* fieldPtr = basePtr + opt.iOffset;
                            QString* strPtr = reinterpret_cast<QString*>(fieldPtr);
                            *strPtr = value;
                            break;
                        }
                    }
                }
            }
        }

        // After processing </world>, check for sibling elements like <triggers> and <aliases>
        // Original MUSHclient format has these as siblings of <world>, not children
        if (reader.isStartElement()) {
            QString elementName = reader.name().toString();

            if (elementName == "triggers") {
                doc->loadTriggersFromXml(reader);
                continue;
            }

            if (elementName == "aliases") {
                doc->loadAliasesFromXml(reader);
                continue;
            }

            if (elementName == "timers") {
                doc->loadTimersFromXml(reader);
                continue;
            }

            // Load plugins from <include plugin="y"> elements at muclient level
            if (elementName == "include") {
                QXmlStreamAttributes includeAttrs = reader.attributes();
                if (includeAttrs.value("plugin").toString().toLower() == "y") {
                    QString pluginPath = includeAttrs.value("name").toString();
                    if (!pluginPath.isEmpty()) {
                        QString fullPath = resolvePluginPath(pluginPath, filename);

                        QString errorMsg;
                        Plugin* plugin = doc->LoadPlugin(fullPath, errorMsg);
                        if (plugin) {
                            qDebug() << "Loaded plugin:" << plugin->m_strName
                                     << "| Aliases:" << plugin->m_AliasMap.size()
                                     << "| Triggers:" << plugin->m_TriggerMap.size()
                                     << "| Enabled:" << plugin->m_bEnabled;
                        } else {
                            qWarning() << "Failed to load plugin:" << pluginPath;
                            qWarning() << "  Error:" << errorMsg;
                        }
                    }
                }
                reader.skipCurrentElement();
                continue;
            }

            // Skip other unknown elements
            reader.skipCurrentElement();
        }
    }

    file.close();

    if (reader.hasError()) {
        qWarning() << "LoadWorldXML: XML parse error:" << reader.errorString();
        return false;
    }

    // Sort plugins by evaluation order after loading
    if (!doc->m_PluginList.empty()) {
        std::sort(doc->m_PluginList.begin(), doc->m_PluginList.end(),
                  [](const std::unique_ptr<Plugin>& a, const std::unique_ptr<Plugin>& b) {
                      return a->m_iSequence < b->m_iSequence;
                  });
        qCDebug(lcWorld) << "LoadWorldXML: Sorted" << doc->m_PluginList.size()
                         << "plugins by sequence";
    }

    qCDebug(lcWorld) << "LoadWorldXML: successfully loaded from" << filename;
    return true;
}

// ============================================================================
// IMPORT XML FROM STRING
// ============================================================================

int ImportXML(WorldDocument* doc, const QString& xmlString, int flags)
{
    if (!doc) {
        qWarning() << "ImportXML: null document";
        return -1;
    }

    if (xmlString.isEmpty()) {
        qWarning() << "ImportXML: empty XML string";
        return -1;
    }

    // Convert QString to QByteArray for QXmlStreamReader
    QByteArray xmlData = xmlString.toUtf8();

    // Check if string contains XML signatures
    QString xmlLower = xmlString.left(500).toLower();
    QStringList signatures = {"<?xml",  "<!--",      "<!doctype", "<muclient",
                              "<world", "<triggers", "<aliases",  "<timers"};
    bool foundSignature = false;
    for (const QString& sig : signatures) {
        if (xmlLower.contains(sig)) {
            foundSignature = true;
            break;
        }
    }

    if (!foundSignature) {
        qWarning() << "ImportXML: string does not appear to be XML";
        return -1;
    }

    // Determine what to import based on flags
    bool importTriggers = (flags & XML_TRIGGERS) != 0;
    bool importAliases = (flags & XML_ALIASES) != 0;
    bool importTimers = (flags & XML_TIMERS) != 0;
    bool importVariables = (flags & XML_VARIABLES) != 0;
    bool importMacros = (flags & XML_MACROS) != 0; // Accelerators

    // Track counts before import
    int triggersBefore = doc->m_TriggerMap.size();
    int aliasesBefore = doc->m_AliasMap.size();
    int timersBefore = doc->m_TimerMap.size();
    int variablesBefore = doc->m_VariableMap.size();

    QXmlStreamReader reader(xmlData);

    // Skip to <muclient> root element (if present)
    bool inMuclient = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QLatin1String("muclient")) {
            inMuclient = true;
            break;
        }
        // If we find triggers/aliases/timers/variables before muclient, that's ok too
        if (reader.isStartElement()) {
            QString name = reader.name().toString();
            if (name == "triggers" || name == "aliases" || name == "timers" ||
                name == "variables" || name == "world") {
                // Don't skip - we'll process this below
                inMuclient = false;
                break;
            }
        }
    }

    // Process elements
    while (!reader.atEnd()) {
        if (inMuclient) {
            reader.readNext();
        }

        if (reader.isEndElement() && reader.name() == QLatin1String("muclient")) {
            break; // End of muclient root
        }

        if (reader.isStartElement()) {
            QString elementName = reader.name().toString();

            // Load triggers (if flag set)
            if (elementName == "triggers") {
                if (importTriggers) {
                    doc->loadTriggersFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load aliases (if flag set)
            if (elementName == "aliases") {
                if (importAliases) {
                    doc->loadAliasesFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load timers (if flag set)
            if (elementName == "timers") {
                if (importTimers) {
                    doc->loadTimersFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load variables (if flag set)
            if (elementName == "variables") {
                if (importVariables) {
                    doc->loadVariablesFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load accelerators (if macros flag set)
            if (elementName == "accelerators") {
                if (importMacros) {
                    doc->loadAcceleratorsFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load macros (original MUSHclient format, if macros flag set)
            if (elementName == "macros") {
                if (importMacros) {
                    doc->loadMacrosFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // Load keypad (original MUSHclient format, if macros flag set)
            if (elementName == "keypad") {
                if (importMacros) {
                    doc->loadKeypadFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
                continue;
            }

            // If we encounter <world>, process its children
            if (elementName == "world") {
                // Skip world attributes, only process child elements
                while (!reader.atEnd()) {
                    reader.readNext();

                    if (reader.isEndElement() && reader.name() == QLatin1String("world")) {
                        break;
                    }

                    if (reader.isStartElement()) {
                        QString childName = reader.name().toString();

                        if (childName == "triggers" && importTriggers) {
                            doc->loadTriggersFromXml(reader);
                        } else if (childName == "aliases" && importAliases) {
                            doc->loadAliasesFromXml(reader);
                        } else if (childName == "timers" && importTimers) {
                            doc->loadTimersFromXml(reader);
                        } else if (childName == "variables" && importVariables) {
                            doc->loadVariablesFromXml(reader);
                        } else if (childName == "accelerators" && importMacros) {
                            doc->loadAcceleratorsFromXml(reader);
                        } else if (childName == "macros" && importMacros) {
                            doc->loadMacrosFromXml(reader);
                        } else if (childName == "keypad" && importMacros) {
                            doc->loadKeypadFromXml(reader);
                        } else {
                            reader.skipCurrentElement();
                        }
                    }
                }
                continue;
            }

            // Skip other unknown elements
            reader.skipCurrentElement();
        }

        if (!inMuclient) {
            reader.readNext();
        }
    }

    if (reader.hasError()) {
        qWarning() << "ImportXML: XML parse error:" << reader.errorString();
        return -1;
    }

    // Calculate counts of imported items
    int triggersAfter = doc->m_TriggerMap.size();
    int aliasesAfter = doc->m_AliasMap.size();
    int timersAfter = doc->m_TimerMap.size();
    int variablesAfter = doc->m_VariableMap.size();

    int totalImported = (triggersAfter - triggersBefore) + (aliasesAfter - aliasesBefore) +
                        (timersAfter - timersBefore) + (variablesAfter - variablesBefore);

    qCDebug(lcWorld) << "ImportXML: imported" << totalImported << "items ("
                     << (triggersAfter - triggersBefore) << "triggers,"
                     << (aliasesAfter - aliasesBefore) << "aliases," << (timersAfter - timersBefore)
                     << "timers," << (variablesAfter - variablesBefore) << "variables)";

    return totalImported;
}

// ============================================================================
// EXPORT XML TO STRING
// ============================================================================

QString ExportXML(WorldDocument* doc, int flags, const QString& comment)
{
    if (!doc) {
        qWarning() << "ExportXML: null document";
        return QString();
    }

    QString xmlOutput;
    QXmlStreamWriter writer(&xmlOutput);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument("1.0");
    writer.writeDTD("<!DOCTYPE muclient>");

    // Write comment if provided
    if (!comment.isEmpty()) {
        writer.writeComment(" " + comment + " ");
    }

    writer.writeStartElement("muclient");

    // Export triggers
    if ((flags & XML_TRIGGERS) && !doc->m_TriggerMap.empty()) {
        doc->saveTriggersToXml(writer);
    }

    // Export aliases
    if ((flags & XML_ALIASES) && !doc->m_AliasMap.empty()) {
        doc->saveAliasesToXml(writer);
    }

    // Export timers
    if ((flags & XML_TIMERS) && !doc->m_TimerMap.empty()) {
        doc->saveTimersToXml(writer);
    }

    // Export variables
    if ((flags & XML_VARIABLES) && !doc->m_VariableMap.empty()) {
        doc->saveVariablesToXml(writer);
    }

    // Export macros/keypad/accelerators (all under XML_MACROS flag)
    // For bidirectional compatibility with original MUSHclient:
    // - Keys that map to original macro slots -> save as <macros>
    // - Keys that map to original keypad slots -> save as <keypad>
    // - Everything else -> save as <accelerators>
    if (flags & XML_MACROS) {
        doc->saveMacrosToXml(writer);
        doc->saveKeypadToXml(writer);
        doc->saveAcceleratorsToXml(writer);
    }

    writer.writeEndElement(); // muclient
    writer.writeEndDocument();

    int totalExported = 0;
    if (flags & XML_TRIGGERS)
        totalExported += doc->m_TriggerMap.size();
    if (flags & XML_ALIASES)
        totalExported += doc->m_AliasMap.size();
    if (flags & XML_TIMERS)
        totalExported += doc->m_TimerMap.size();
    if (flags & XML_VARIABLES)
        totalExported += doc->m_VariableMap.size();

    qCDebug(lcWorld) << "ExportXML: exported" << totalExported << "items";

    return xmlOutput;
}

} // namespace XmlSerialization
