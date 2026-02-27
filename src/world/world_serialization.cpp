// world_serialization.cpp
// XML Serialization Implementation for World Automation Elements
//
// Implements XML save/load for triggers, aliases, timers, and variables
// Port of: XML serialization from original MUSHclient
//
// Provides:
// - saveTriggersToXml() / loadTriggersFromXml() - Trigger persistence
// - saveAliasesToXml() / loadAliasesFromXml() - Alias persistence
// - saveTimersToXml() / loadTimersFromXml() - Timer persistence
// - saveVariablesToXml() / loadVariablesFromXml() - Variable persistence
// - Save_One_*_XML() - Individual element serialization for Plugin Wizard
//
// XML Format:
// - Uses QXmlStreamWriter/Reader for modern XML processing
// - Maintains backward compatibility with original MUSHclient .mcl files
// - Handles CDATA sections for script content
// - Color values stored as hex (#RRGGBB) or ANSI indices
//
// Based on xml_save_world.cpp and xml_load_world.cpp

#include "../automation/alias.h"    // For Alias
#include "../automation/plugin.h"   // For Plugin
#include "../automation/sendto.h"   // For eSendToWorld, eSendToCommand
#include "../automation/timer.h"    // For Timer
#include "../automation/trigger.h"  // For Trigger
#include "../automation/variable.h" // For Variable
#include "accelerator_manager.h"    // For AcceleratorEntry
#include "color_utils.h"            // For BGR/RGB conversion
#include "logging.h"                // For qCDebug(lcWorld)
#include "macro_keypad_compat.h"    // For macro/keypad format conversion
#include "world_document.h"
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// SAMECOLOUR constant from original MUSHclient
#define SAMECOLOUR 65535

// Style bit masks from original MUSHclient
#define HILITE 0x0001
#define UNDERLINE 0x0002
#define BLINK 0x0004 // italic
#define INVERSE 0x0008

// Trigger match bit masks (from original MUSHclient OtherTypes.h)
#define TRIGGER_MATCH_TEXT 0x0080 // high order of text nibble
#define TRIGGER_MATCH_BACK 0x0800 // high order of back nibble
#define TRIGGER_MATCH_HILITE 0x1000
#define TRIGGER_MATCH_UNDERLINE 0x2000
#define TRIGGER_MATCH_BLINK 0x4000 // italic
#define TRIGGER_MATCH_INVERSE 0x8000

// Helper function to convert BGR color to hex name (#RRGGBB for XML)
// Internal colors are stored in BGR format (MUSHclient COLORREF)
static QString colorToName(QRgb bgr)
{
    // Convert BGR to RGB for human-readable hex format
    QColor color = bgrToQColor(bgr);
    return QString("#%1%2%3")
        .arg(color.red(), 2, 16, QChar('0'))
        .arg(color.green(), 2, 16, QChar('0'))
        .arg(color.blue(), 2, 16, QChar('0'))
        .toUpper();
}

// Helper function to parse color name (#RRGGBB) to BGR
// Returns BGR format for internal storage
static QRgb nameToColor(const QString& name)
{
    if (name.startsWith("#")) {
        // Hex format: #RRGGBB - parse as RGB then convert to BGR
        bool ok;
        unsigned int rgb = name.mid(1).toUInt(&ok, 16);
        if (ok) {
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            return BGR(r, g, b);
        }
    }
    // Could add named color lookup here (red, blue, etc.)
    // For now, return black as default
    return BGR(0, 0, 0);
}

void WorldDocument::saveTriggersToXml(QXmlStreamWriter& xml)
{
    xml.writeStartElement("triggers");

    for (const auto& [name, triggerPtr] : m_automationRegistry->m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();

        // Skip temporary triggers
        if (trigger->temporary)
            continue;

        xml.writeStartElement("trigger");

        // Write attributes
        xml.writeAttribute("name", trigger->label);
        xml.writeAttribute("enabled", trigger->enabled ? "y" : "n");
        xml.writeAttribute("match", trigger->trigger);
        xml.writeAttribute("send_to", QString::number(trigger->send_to));
        xml.writeAttribute("sequence", QString::number(trigger->sequence));
        xml.writeAttribute("script", trigger->procedure);
        xml.writeAttribute("group", trigger->group);
        xml.writeAttribute("variable", trigger->variable);

        // Behavior flags
        xml.writeAttribute("omit_from_output", trigger->omit_from_output ? "y" : "n");
        xml.writeAttribute("omit_from_log", trigger->omit_from_log ? "y" : "n");
        xml.writeAttribute("keep_evaluating", trigger->keep_evaluating ? "y" : "n");
        xml.writeAttribute("regexp", trigger->use_regexp ? "y" : "n");
        xml.writeAttribute("ignore_case", trigger->ignore_case ? "y" : "n");
        xml.writeAttribute("repeat", trigger->repeat ? "y" : "n");
        xml.writeAttribute("expand_variables", trigger->expand_variables ? "y" : "n");
        xml.writeAttribute("one_shot", trigger->one_shot ? "y" : "n");
        xml.writeAttribute("lowercase_wildcard", trigger->lowercase_wildcard ? "y" : "n");

        // Multi-line matching
        xml.writeAttribute("multi_line", trigger->multi_line ? "y" : "n");
        xml.writeAttribute("lines_to_match", QString::number(trigger->lines_to_match));

        // Sound
        xml.writeAttribute("sound", trigger->sound_to_play);
        xml.writeAttribute("sound_if_inactive", trigger->sound_if_inactive ? "y" : "n");

        // Decompose style into individual make_* attributes
        if (trigger->style & HILITE)
            xml.writeAttribute("make_bold", "y");
        if (trigger->style & BLINK)
            xml.writeAttribute("make_italic", "y");
        if (trigger->style & UNDERLINE)
            xml.writeAttribute("make_underline", "y");

        // Decompose match_type into individual attributes
        // Extract ANSI color indices (0-15)
        int text_colour = (trigger->match_type >> 4) & 0x0F;
        int back_colour = (trigger->match_type >> 8) & 0x0F;
        if (text_colour != 0)
            xml.writeAttribute("text_colour", QString::number(text_colour));
        if (back_colour != 0)
            xml.writeAttribute("back_colour", QString::number(back_colour));

        // Extract style matching flags
        if (trigger->match_type & HILITE)
            xml.writeAttribute("bold", "y");
        if (trigger->match_type & INVERSE)
            xml.writeAttribute("inverse", "y");
        if (trigger->match_type & BLINK)
            xml.writeAttribute("italic", "y");
        if (trigger->match_type & TRIGGER_MATCH_TEXT)
            xml.writeAttribute("match_text_colour", "y");
        if (trigger->match_type & TRIGGER_MATCH_BACK)
            xml.writeAttribute("match_back_colour", "y");
        if (trigger->match_type & TRIGGER_MATCH_HILITE)
            xml.writeAttribute("match_bold", "y");
        if (trigger->match_type & TRIGGER_MATCH_INVERSE)
            xml.writeAttribute("match_inverse", "y");
        if (trigger->match_type & TRIGGER_MATCH_BLINK)
            xml.writeAttribute("match_italic", "y");
        if (trigger->match_type & TRIGGER_MATCH_UNDERLINE)
            xml.writeAttribute("match_underline", "y");

        // Custom color (add 1, skip SAMECOLOUR)
        if (trigger->colour != SAMECOLOUR)
            xml.writeAttribute("custom_colour", QString::number(trigger->colour + 1));

        xml.writeAttribute("colour_change_type", QString::number(trigger->colour_change_type));

        // RGB colors as names
        if (trigger->other_foreground != 0)
            xml.writeAttribute("other_text_colour", colorToName(trigger->other_foreground));
        if (trigger->other_background != 0)
            xml.writeAttribute("other_back_colour", colorToName(trigger->other_background));

        // Script language (only write if not Lua - the default)
        if (trigger->scriptLanguage != ScriptLanguage::Lua) {
            xml.writeAttribute("script_language", scriptLanguageToString(trigger->scriptLanguage));
        }

        // Other options
        xml.writeAttribute("clipboard_arg", QString::number(trigger->clipboard_arg));
        xml.writeAttribute("user", QString::number(trigger->user_option));

        // Contents as child element with CDATA
        if (!trigger->contents.isEmpty()) {
            xml.writeStartElement("send");
            xml.writeCDATA(trigger->contents);
            xml.writeEndElement(); // send
        }

        xml.writeEndElement(); // trigger
    }

    xml.writeEndElement(); // triggers
}

void WorldDocument::loadTriggersFromXml(QXmlStreamReader& xml, Plugin* plugin)
{
    qCDebug(lcWorld) << "loadTriggersFromXml: Starting to load triggers"
                     << (plugin ? "for plugin" : "for world");
    int triggerCount = 0;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("triggers")) {
            qCDebug(lcWorld) << "loadTriggersFromXml: Finished loading" << triggerCount
                             << "triggers";
            break;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("trigger")) {
            triggerCount++;
            qCDebug(lcWorld) << "loadTriggersFromXml: Found trigger #" << triggerCount;
            auto trigger = std::make_unique<Trigger>();

            // Read attributes
            QXmlStreamAttributes attrs = xml.attributes();

            trigger->label = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            if (trigger->label.isEmpty()) {
                // Use pattern as base for name (truncated if too long)
                QString pattern = attrs.value("match").toString();
                if (pattern.length() > 50) {
                    pattern = pattern.left(50) + "...";
                }
                // Make it unique by appending a counter
                trigger->internal_name =
                    QString("trigger_%1_%2").arg(triggerCount).arg(qHash(pattern));
            } else {
                trigger->internal_name = trigger->label;
            }
            trigger->enabled = attrs.value("enabled").toString() == "y";
            trigger->trigger = attrs.value("match").toString();
            trigger->send_to = attrs.value("send_to").toInt();
            trigger->sequence = attrs.value("sequence").toInt();
            trigger->procedure = attrs.value("script").toString();
            trigger->group = attrs.value("group").toString();
            trigger->variable = attrs.value("variable").toString();

            // Behavior flags
            trigger->omit_from_output = attrs.value("omit_from_output").toString() == "y";
            trigger->omit_from_log = attrs.value("omit_from_log").toString() == "y";
            // Only set to false if explicitly "n", otherwise keep default (true)
            if (attrs.hasAttribute("keep_evaluating")) {
                trigger->keep_evaluating = attrs.value("keep_evaluating").toString() != "n";
            }
            trigger->use_regexp = attrs.value("regexp").toString() == "y";
            trigger->ignore_case = attrs.value("ignore_case").toString() == "y";
            trigger->repeat = attrs.value("repeat").toString() == "y";
            trigger->expand_variables = attrs.value("expand_variables").toString() == "y";
            trigger->one_shot = attrs.value("one_shot").toString() == "y";
            trigger->lowercase_wildcard = attrs.value("lowercase_wildcard").toString() == "y";

            // Multi-line matching
            trigger->multi_line = attrs.value("multi_line").toString() == "y";
            trigger->lines_to_match = attrs.value("lines_to_match").toInt();

            // Sound
            trigger->sound_to_play = attrs.value("sound").toString();
            trigger->sound_if_inactive = attrs.value("sound_if_inactive").toString() == "y";

            // Compose style from individual make_* attributes
            trigger->style = 0;
            if (attrs.value("make_bold").toString() == "y")
                trigger->style |= HILITE;
            if (attrs.value("make_italic").toString() == "y")
                trigger->style |= BLINK;
            if (attrs.value("make_underline").toString() == "y")
                trigger->style |= UNDERLINE;

            // Compose match_type from individual attributes
            trigger->match_type = 0;

            // Pack ANSI color indices (0-15) into match_type
            if (attrs.hasAttribute("text_colour")) {
                int text_colour = attrs.value("text_colour").toInt();
                trigger->match_type |= (text_colour << 4);
            }
            if (attrs.hasAttribute("back_colour")) {
                int back_colour = attrs.value("back_colour").toInt();
                trigger->match_type |= (back_colour << 8);
            }

            // Compose style matching flags
            if (attrs.value("bold").toString() == "y")
                trigger->match_type |= HILITE;
            if (attrs.value("inverse").toString() == "y")
                trigger->match_type |= INVERSE;
            if (attrs.value("italic").toString() == "y")
                trigger->match_type |= BLINK;
            if (attrs.value("match_text_colour").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_TEXT;
            if (attrs.value("match_back_colour").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_BACK;
            if (attrs.value("match_bold").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_HILITE;
            if (attrs.value("match_inverse").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_INVERSE;
            if (attrs.value("match_italic").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_BLINK;
            if (attrs.value("match_underline").toString() == "y")
                trigger->match_type |= TRIGGER_MATCH_UNDERLINE;

            // Custom color (decrement by 1, handle SAMECOLOUR)
            if (attrs.hasAttribute("custom_colour")) {
                int custom_colour = attrs.value("custom_colour").toInt();
                if (custom_colour == 0)
                    trigger->colour = SAMECOLOUR;
                else
                    trigger->colour = custom_colour - 1; // Make zero-relative
            } else {
                trigger->colour = SAMECOLOUR;
            }

            if (attrs.hasAttribute("colour_change_type"))
                trigger->colour_change_type = attrs.value("colour_change_type").toInt();

            // RGB colors from names
            if (attrs.hasAttribute("other_text_colour"))
                trigger->other_foreground =
                    nameToColor(attrs.value("other_text_colour").toString());
            if (attrs.hasAttribute("other_back_colour"))
                trigger->other_background =
                    nameToColor(attrs.value("other_back_colour").toString());

            // Script language (defaults to Lua if not specified)
            if (attrs.hasAttribute("script_language")) {
                trigger->scriptLanguage =
                    scriptLanguageFromString(attrs.value("script_language").toString());
            }

            // Other options
            if (attrs.hasAttribute("clipboard_arg"))
                trigger->clipboard_arg = attrs.value("clipboard_arg").toInt();
            if (attrs.hasAttribute("user"))
                trigger->user_option = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    trigger->contents = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            // Compile regex if needed
            if (trigger->use_regexp) {
                (void)trigger->compileRegexp();
            }

            // Add to plugin or world collections based on context
            if (plugin) {
                // Add to plugin's collections
                trigger->internal_name =
                    trigger->label.isEmpty()
                        ? QString("trigger_%1_%2").arg(triggerCount).arg(qHash(trigger->trigger))
                        : trigger->label;

                QString triggerName = trigger->internal_name;

                // Skip duplicates - don't overwrite existing triggers
                if (plugin->m_TriggerMap.find(triggerName) != plugin->m_TriggerMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate trigger:" << triggerName;
                    continue;
                }

                trigger->owningPlugin = plugin;
                Trigger* rawPtr = trigger.get();
                plugin->m_TriggerMap[triggerName] = std::move(trigger);
                plugin->m_TriggerArray.append(rawPtr);
                plugin->m_triggersNeedSorting = true;
                qCDebug(lcWorld) << "Added trigger to plugin:" << rawPtr->internal_name
                                 << "sequence:" << rawPtr->sequence;
            } else {
                // Add to world's collections (existing behavior)
                QString triggerName = trigger->internal_name;
                (void)addTrigger(triggerName,
                                 std::move(trigger)); // intentional: deserialization load
                m_automationRegistry->m_triggersNeedSorting = true;
            }
        }
    }
}

void WorldDocument::saveAliasesToXml(QXmlStreamWriter& xml)
{
    xml.writeStartElement("aliases");

    for (const auto& [name, aliasPtr] : m_automationRegistry->m_AliasMap) {
        Alias* alias = aliasPtr.get();

        // Skip temporary aliases
        if (alias->temporary)
            continue;

        xml.writeStartElement("alias");

        // Write attributes
        xml.writeAttribute("name", alias->label);
        xml.writeAttribute("enabled", alias->enabled ? "y" : "n");
        xml.writeAttribute("match", alias->name);
        xml.writeAttribute("send_to", QString::number(alias->send_to));
        xml.writeAttribute("sequence", QString::number(alias->sequence));
        xml.writeAttribute("script", alias->procedure);
        xml.writeAttribute("group", alias->group);
        xml.writeAttribute("variable", alias->variable);

        // Behavior flags
        xml.writeAttribute("omit_from_output", alias->omit_from_output ? "y" : "n");
        xml.writeAttribute("omit_from_log", alias->omit_from_log ? "y" : "n");
        xml.writeAttribute("omit_from_command_history",
                           alias->omit_from_command_history ? "y" : "n");
        xml.writeAttribute("keep_evaluating", alias->keep_evaluating ? "y" : "n");
        xml.writeAttribute("regexp", alias->use_regexp ? "y" : "n");
        xml.writeAttribute("ignore_case", alias->ignore_case ? "y" : "n");
        xml.writeAttribute("expand_variables", alias->expand_variables ? "y" : "n");
        xml.writeAttribute("echo_alias", alias->echo_alias ? "y" : "n");
        xml.writeAttribute("one_shot", alias->one_shot ? "y" : "n");
        xml.writeAttribute("menu", alias->menu ? "y" : "n");

        // Script language (only write if not Lua - the default)
        if (alias->scriptLanguage != ScriptLanguage::Lua) {
            xml.writeAttribute("script_language", scriptLanguageToString(alias->scriptLanguage));
        }

        // Other options
        xml.writeAttribute("user", QString::number(alias->user_option));

        // Contents as child element with CDATA
        if (!alias->contents.isEmpty()) {
            xml.writeStartElement("send");
            xml.writeCDATA(alias->contents);
            xml.writeEndElement(); // send
        }

        xml.writeEndElement(); // alias
    }

    xml.writeEndElement(); // aliases
}

void WorldDocument::loadAliasesFromXml(QXmlStreamReader& xml, Plugin* plugin)
{
    int aliasCount = 0; // Counter for generating unique names

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("aliases"))
            break;

        if (xml.isStartElement() && xml.name() == QLatin1String("alias")) {
            auto alias = std::make_unique<Alias>();

            // Read attributes
            QXmlStreamAttributes attrs = xml.attributes();

            alias->label = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            if (alias->label.isEmpty()) {
                QString match = attrs.value("match").toString();
                if (match.length() > 50) {
                    match = match.left(50) + "...";
                }
                alias->internal_name = QString("alias_%1_%2").arg(aliasCount).arg(qHash(match));
            } else {
                alias->internal_name = alias->label;
            }

            aliasCount++;
            alias->enabled = attrs.value("enabled").toString() == "y";
            alias->name = attrs.value("match").toString();
            alias->send_to = attrs.value("send_to").toInt();
            alias->sequence = attrs.value("sequence").toInt();
            alias->procedure = attrs.value("script").toString();
            alias->group = attrs.value("group").toString();
            alias->variable = attrs.value("variable").toString();

            // Behavior flags
            alias->omit_from_output = attrs.value("omit_from_output").toString() == "y";
            alias->omit_from_log = attrs.value("omit_from_log").toString() == "y";
            alias->omit_from_command_history =
                attrs.value("omit_from_command_history").toString() == "y";
            // Only set to false if explicitly "n", otherwise keep default (true)
            if (attrs.hasAttribute("keep_evaluating")) {
                alias->keep_evaluating = attrs.value("keep_evaluating").toString() != "n";
            }
            alias->use_regexp = attrs.value("regexp").toString() == "y";
            alias->ignore_case = attrs.value("ignore_case").toString() == "y";
            alias->expand_variables = attrs.value("expand_variables").toString() == "y";
            alias->echo_alias = attrs.value("echo_alias").toString() == "y";
            alias->one_shot = attrs.value("one_shot").toString() == "y";
            alias->menu = attrs.value("menu").toString() == "y";

            // Script language (defaults to Lua if not specified)
            if (attrs.hasAttribute("script_language")) {
                alias->scriptLanguage =
                    scriptLanguageFromString(attrs.value("script_language").toString());
            }

            // Other options
            if (attrs.hasAttribute("user"))
                alias->user_option = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    alias->contents = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            // Compile regex if needed
            if (alias->use_regexp) {
                (void)alias->compileRegexp();
            }

            // Add to plugin or world collections based on context
            if (plugin) {
                // Add to plugin's collections
                QString internalName = alias->internal_name;

                // Skip duplicates - don't overwrite existing aliases
                if (plugin->m_AliasMap.find(internalName) != plugin->m_AliasMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate alias:" << internalName;
                    continue;
                }

                qint32 sequence = alias->sequence;
                Alias* rawPtr = alias.get();
                plugin->m_AliasMap[internalName] = std::move(alias);
                plugin->m_AliasArray.append(rawPtr);
                plugin->m_aliasesNeedSorting = true;
                qCDebug(lcWorld) << "Added alias to plugin:" << internalName
                                 << "sequence:" << sequence;
            } else {
                // Add to world's collections (existing behavior)
                QString internalName = alias->internal_name;
                (void)addAlias(internalName, std::move(alias)); // intentional: deserialization load
                m_automationRegistry->m_aliasesNeedSorting = true;
            }
        }
    }
}

// ========================================================================
// Timer XML Serialization
// ========================================================================

void WorldDocument::saveTimersToXml(QXmlStreamWriter& xml)
{
    xml.writeStartElement("timers");

    for (const auto& [name, timerPtr] : m_automationRegistry->m_TimerMap) {
        Timer* timer = timerPtr.get();

        // Skip temporary timers (don't save to file)
        if (timer->temporary)
            continue;

        // Skip included timers (loaded from included file)
        if (timer->included)
            continue;

        xml.writeStartElement("timer");

        // Write basic attributes
        xml.writeAttribute("name", timer->label);
        xml.writeAttribute("enabled", timer->enabled ? "y" : "n");
        xml.writeAttribute("send_to", QString::number(timer->send_to));
        xml.writeAttribute("script", timer->procedure);
        xml.writeAttribute("group", timer->group);
        xml.writeAttribute("variable", timer->variable);

        // Timing configuration - use original MUSHclient format for compatibility
        // Original format uses at_time="y"/"n" instead of type="0"/"1"
        // and uses hour/minute/second (picking from at_* or every_* based on type)
        bool isAtTime = (timer->type == Timer::eAtTime);
        xml.writeAttribute("at_time", isAtTime ? "y" : "n");

        // Write hour/minute/second from either at_* or every_* fields based on type
        if (isAtTime) {
            xml.writeAttribute("hour", QString::number(timer->at_hour));
            xml.writeAttribute("minute", QString::number(timer->at_minute));
            xml.writeAttribute("second", QString::number(timer->at_second, 'f', 4));
        } else {
            xml.writeAttribute("hour", QString::number(timer->every_hour));
            xml.writeAttribute("minute", QString::number(timer->every_minute));
            xml.writeAttribute("second", QString::number(timer->every_second, 'f', 4));
        }

        // Offset fields (always written)
        xml.writeAttribute("offset_hour", QString::number(timer->offset_hour));
        xml.writeAttribute("offset_minute", QString::number(timer->offset_minute));
        xml.writeAttribute("offset_second", QString::number(timer->offset_second, 'f', 4));

        // Behavior flags
        xml.writeAttribute("one_shot", timer->one_shot ? "y" : "n");
        // Use active_closed for original MUSHclient compatibility
        xml.writeAttribute("active_closed", timer->active_when_closed ? "y" : "n");
        xml.writeAttribute("omit_from_output", timer->omit_from_output ? "y" : "n");
        xml.writeAttribute("omit_from_log", timer->omit_from_log ? "y" : "n");

        // Script language (only write if not Lua - the default)
        if (timer->scriptLanguage != ScriptLanguage::Lua) {
            xml.writeAttribute("script_language", scriptLanguageToString(timer->scriptLanguage));
        }

        // Other options
        xml.writeAttribute("user", QString::number(timer->user_option));

        // Contents as child element with CDATA
        if (!timer->contents.isEmpty()) {
            xml.writeStartElement("send");
            xml.writeCDATA(timer->contents);
            xml.writeEndElement(); // send
        }

        xml.writeEndElement(); // timer
    }

    xml.writeEndElement(); // timers
}

void WorldDocument::loadTimersFromXml(QXmlStreamReader& xml, Plugin* plugin)
{
    qCDebug(lcWorld) << "loadTimersFromXml: Starting to load timers"
                     << (plugin ? "for plugin" : "for world");
    int timerCount = 0;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("timers")) {
            qCDebug(lcWorld) << "loadTimersFromXml: Finished loading" << timerCount << "timers";
            break;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("timer")) {
            timerCount++;
            qCDebug(lcWorld) << "loadTimersFromXml: Found timer #" << timerCount;
            auto timer = std::make_unique<Timer>();

            // Read attributes
            QXmlStreamAttributes attrs = xml.attributes();

            timer->label = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            QString internalName;
            if (timer->label.isEmpty()) {
                // Use timestamp-based unique name for unlabelled timers
                internalName = QString("*timer%1").arg(timerCount, 10, 10, QChar('0'));
            } else {
                internalName = timer->label;
            }

            timer->enabled = attrs.value("enabled").toString() == "y";
            timer->send_to = attrs.value("send_to").toInt();
            timer->procedure = attrs.value("script").toString();
            timer->group = attrs.value("group").toString();
            timer->variable = attrs.value("variable").toString();

            // Timing configuration
            // Check for type attribute first (mushclient-qt format), then at_time (original format)
            bool isAtTime;
            if (attrs.hasAttribute("type")) {
                // New format: type="0" (eAtTime) or type="1" (eInterval)
                isAtTime = (attrs.value("type").toInt() == Timer::eAtTime);
            } else {
                // Original format: at_time="y" or "n"
                isAtTime = (attrs.value("at_time").toString() == "y");
            }
            timer->type = isAtTime ? Timer::eAtTime : Timer::eInterval;

            // Read hour/minute/second from either simple names or specific names
            int hour = attrs.value("hour").toInt();
            int minute = attrs.value("minute").toInt();
            double second = attrs.value("second").toDouble();

            if (isAtTime) {
                // At-time timer: use hour/minute/second for at_* fields
                timer->at_hour =
                    attrs.hasAttribute("at_hour") ? attrs.value("at_hour").toInt() : hour;
                timer->at_minute =
                    attrs.hasAttribute("at_minute") ? attrs.value("at_minute").toInt() : minute;
                timer->at_second =
                    attrs.hasAttribute("at_second") ? attrs.value("at_second").toDouble() : second;
            } else {
                // Interval timer: use hour/minute/second for every_* fields
                timer->every_hour =
                    attrs.hasAttribute("every_hour") ? attrs.value("every_hour").toInt() : hour;
                timer->every_minute = attrs.hasAttribute("every_minute")
                                          ? attrs.value("every_minute").toInt()
                                          : minute;
                timer->every_second = attrs.hasAttribute("every_second")
                                          ? attrs.value("every_second").toDouble()
                                          : second;
            }

            // Offset fields
            timer->offset_hour = attrs.value("offset_hour").toInt();
            timer->offset_minute = attrs.value("offset_minute").toInt();
            timer->offset_second = attrs.value("offset_second").toDouble();

            // Behavior flags
            timer->one_shot = attrs.value("one_shot").toString() == "y";
            // active_closed is alternate name for active_when_closed
            timer->active_when_closed = (attrs.value("active_when_closed").toString() == "y") ||
                                        (attrs.value("active_closed").toString() == "y");
            timer->omit_from_output = attrs.value("omit_from_output").toString() == "y";
            timer->omit_from_log = attrs.value("omit_from_log").toString() == "y";

            // Script language (defaults to Lua if not specified)
            if (attrs.hasAttribute("script_language")) {
                timer->scriptLanguage =
                    scriptLanguageFromString(attrs.value("script_language").toString());
            }

            // Other options
            if (attrs.hasAttribute("user"))
                timer->user_option = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    timer->contents = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            // Calculate when timer should next fire
            resetOneTimer(timer.get());

            // Add to plugin or world collections based on context
            if (plugin) {
                // Skip duplicates - don't overwrite existing timers
                if (plugin->m_TimerMap.find(internalName) != plugin->m_TimerMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate timer:" << internalName;
                    continue;
                }

                // Add to plugin's collections
                Timer* rawTimer = timer.get(); // Get raw pointer before moving
                plugin->m_TimerMap[internalName] = std::move(timer);
                if (rawTimer->label.isEmpty()) {
                    plugin->m_TimerRevMap.insert(rawTimer, internalName);
                }
                qCDebug(lcWorld) << "Added timer to plugin:" << internalName;
            } else {
                // Add to world's collections (transfer ownership via move)
                (void)addTimer(internalName, std::move(timer)); // intentional: deserialization load
            }
        }
    }
}

// ========== Variable XML Serialization ==========

/**
 * saveVariablesToXml - Save all variables to XML
 *
 * Writes <variables> section containing all world variables.
 * Format: <variable name="varname">value</variable>
 *
 * Based on xml_save_world.cpp
 */
void WorldDocument::saveVariablesToXml(QXmlStreamWriter& xml)
{
    if (m_VariableMap.empty())
        return;

    xml.writeStartElement("variables");

    for (const auto& [name, var] : m_VariableMap) {
        xml.writeStartElement("variable");
        xml.writeAttribute("name", var->label);
        xml.writeCharacters(var->contents);
        xml.writeEndElement(); // variable
    }

    xml.writeEndElement(); // variables
}

/**
 * loadVariablesFromXml - Load variables from XML
 *
 * Reads <variables> section and creates Variable objects.
 * If plugin is specified, loads into plugin's VariableMap instead of world's.
 *
 * Based on xml_load_world.cpp
 *
 * @param xml XML reader positioned at <variables> start element
 * @param plugin If non-null, load into plugin's VariableMap instead of world's
 */
void WorldDocument::loadVariablesFromXml(QXmlStreamReader& xml, Plugin* plugin)
{
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("variables"))
            break;

        if (xml.isStartElement() && xml.name() == QLatin1String("variable")) {
            QString name = xml.attributes().value("name").toString();
            QString contents = xml.readElementText();

            QString varName = name.toLower(); // Ensure lowercase

            // Skip duplicates - don't overwrite existing variables
            if (plugin) {
                if (plugin->m_VariableMap.find(varName) != plugin->m_VariableMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate variable:" << varName;
                    continue;
                }
            } else {
                if (m_VariableMap.find(varName) != m_VariableMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate variable:" << varName;
                    continue;
                }
            }

            auto var = std::make_unique<Variable>();
            var->label = varName;
            var->contents = contents;

            if (plugin) {
                plugin->m_VariableMap[var->label] = std::move(var);
            } else {
                m_VariableMap[var->label] = std::move(var);
            }
        }
    }
}

/**
 * Save_One_Variable_XML - Save single variable for Plugin Wizard
 *
 * Writes a single <variable> element as raw XML text.
 * Used by Plugin Wizard to generate plugin XML files.
 *
 * Save_One_Variable_XML()
 *
 * @param out Text stream for writing XML
 * @param variable Variable to save
 */
void WorldDocument::Save_One_Variable_XML(QTextStream& out, Variable* variable)
{
    // Simple XML escaping
    QString escapedName = variable->label;
    escapedName.replace("&", "&amp;");
    escapedName.replace("<", "&lt;");
    escapedName.replace(">", "&gt;");
    escapedName.replace("\"", "&quot;");
    escapedName.replace("'", "&apos;");

    QString escapedContents = variable->contents;
    escapedContents.replace("&", "&amp;");
    escapedContents.replace("<", "&lt;");
    escapedContents.replace(">", "&gt;");

    out << QString("  <variable name=\"%1\">%2</variable>\n").arg(escapedName).arg(escapedContents);
}

/**
 * Save_One_Trigger_XML - Save single trigger for Plugin Wizard
 *
 * Writes a single <trigger> element as raw XML text.
 * Used by Plugin Wizard to generate plugin XML files.
 *
 * Save_One_Trigger_XML()
 *
 * @param out Text stream for writing XML
 * @param trigger Trigger to save
 */
void WorldDocument::Save_One_Trigger_XML(QTextStream& out, Trigger* trigger)
{
    // Helper lambda for XML attribute escaping
    auto escapeXml = [](const QString& str) -> QString {
        QString escaped = str;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        escaped.replace("\"", "&quot;");
        escaped.replace("'", "&apos;");
        return escaped;
    };

    // Start trigger element with attributes
    out << "  <trigger\n";
    out << QString("   name=\"%1\"\n").arg(escapeXml(trigger->label));
    out << QString("   enabled=\"%1\"\n").arg(trigger->enabled ? "y" : "n");
    out << QString("   match=\"%1\"\n").arg(escapeXml(trigger->trigger));
    out << QString("   send_to=\"%1\"\n").arg(trigger->send_to);
    out << QString("   sequence=\"%1\"\n").arg(trigger->sequence);

    if (!trigger->procedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(trigger->procedure));
    if (trigger->scriptLanguage != ScriptLanguage::Lua)
        out << QString("   script_language=\"%1\"\n")
                   .arg(scriptLanguageToString(trigger->scriptLanguage));
    if (!trigger->group.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(trigger->group));
    if (!trigger->variable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(trigger->variable));

    // Behavior flags (only write if not default)
    if (trigger->omit_from_output)
        out << "   omit_from_output=\"y\"\n";
    if (trigger->omit_from_log)
        out << "   omit_from_log=\"y\"\n";
    if (!trigger->keep_evaluating)
        out << "   keep_evaluating=\"n\"\n";
    if (trigger->use_regexp)
        out << "   regexp=\"y\"\n";
    if (trigger->ignore_case)
        out << "   ignore_case=\"y\"\n";
    if (trigger->repeat)
        out << "   repeat=\"y\"\n";
    if (trigger->expand_variables)
        out << "   expand_variables=\"y\"\n";
    if (trigger->one_shot)
        out << "   one_shot=\"y\"\n";
    if (trigger->lowercase_wildcard)
        out << "   lowercase_wildcard=\"y\"\n";

    // Multi-line matching
    if (trigger->multi_line) {
        out << "   multi_line=\"y\"\n";
        out << QString("   lines_to_match=\"%1\"\n").arg(trigger->lines_to_match);
    }

    // Sound
    if (!trigger->sound_to_play.isEmpty()) {
        out << QString("   sound=\"%1\"\n").arg(escapeXml(trigger->sound_to_play));
        if (trigger->sound_if_inactive)
            out << "   sound_if_inactive=\"y\"\n";
    }

    // Style flags
    if (trigger->style & HILITE)
        out << "   make_bold=\"y\"\n";
    if (trigger->style & BLINK)
        out << "   make_italic=\"y\"\n";
    if (trigger->style & UNDERLINE)
        out << "   make_underline=\"y\"\n";

    // Extract ANSI color indices from match_type (bits 4-11)
    int text_colour = (trigger->match_type >> 4) & 0x0F;
    int back_colour = (trigger->match_type >> 8) & 0x0F;
    if (text_colour != 0)
        out << QString("   text_colour=\"%1\"\n").arg(text_colour);
    if (back_colour != 0)
        out << QString("   back_colour=\"%1\"\n").arg(back_colour);

    // Extract style matching flags from match_type
    if (trigger->match_type & HILITE)
        out << "   bold=\"y\"\n";
    if (trigger->match_type & INVERSE)
        out << "   inverse=\"y\"\n";
    if (trigger->match_type & BLINK)
        out << "   italic=\"y\"\n";

    // TODO: Add TRIGGER_MATCH_* flag definitions if needed

    out << "  >\n";

    // Contents as <send> child element with CDATA
    if (!trigger->contents.isEmpty()) {
        out << "  <send><![CDATA[" << trigger->contents << "]]></send>\n";
    }

    out << "  </trigger>\n";
}

/**
 * Save_One_Alias_XML - Save single alias for Plugin Wizard
 *
 * Writes a single <alias> element as raw XML text.
 * Used by Plugin Wizard to generate plugin XML files.
 *
 * Save_One_Alias_XML()
 *
 * @param out Text stream for writing XML
 * @param alias Alias to save
 */
void WorldDocument::Save_One_Alias_XML(QTextStream& out, Alias* alias)
{
    // Helper lambda for XML attribute escaping
    auto escapeXml = [](const QString& str) -> QString {
        QString escaped = str;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        escaped.replace("\"", "&quot;");
        escaped.replace("'", "&apos;");
        return escaped;
    };

    // Start alias element with attributes
    out << "  <alias\n";
    out << QString("   name=\"%1\"\n").arg(escapeXml(alias->label));
    out << QString("   enabled=\"%1\"\n").arg(alias->enabled ? "y" : "n");
    out << QString("   match=\"%1\"\n").arg(escapeXml(alias->name));
    out << QString("   send_to=\"%1\"\n").arg(alias->send_to);
    out << QString("   sequence=\"%1\"\n").arg(alias->sequence);

    if (!alias->procedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(alias->procedure));
    if (alias->scriptLanguage != ScriptLanguage::Lua)
        out << QString("   script_language=\"%1\"\n")
                   .arg(scriptLanguageToString(alias->scriptLanguage));
    if (!alias->group.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(alias->group));
    if (!alias->variable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(alias->variable));

    // Behavior flags (only write if not default)
    if (alias->omit_from_output)
        out << "   omit_from_output=\"y\"\n";
    if (alias->omit_from_log)
        out << "   omit_from_log=\"y\"\n";
    if (alias->omit_from_command_history)
        out << "   omit_from_command_history=\"y\"\n";
    if (!alias->keep_evaluating)
        out << "   keep_evaluating=\"n\"\n";
    if (alias->use_regexp)
        out << "   regexp=\"y\"\n";
    if (alias->ignore_case)
        out << "   ignore_case=\"y\"\n";
    if (alias->expand_variables)
        out << "   expand_variables=\"y\"\n";
    if (alias->echo_alias)
        out << "   echo_alias=\"y\"\n";
    if (alias->one_shot)
        out << "   one_shot=\"y\"\n";
    if (alias->menu)
        out << "   menu=\"y\"\n";

    // User option
    if (alias->user_option != 0)
        out << QString("   user=\"%1\"\n").arg(alias->user_option);

    out << "  >\n";

    // Contents as <send> child element with CDATA
    if (!alias->contents.isEmpty()) {
        out << "  <send><![CDATA[" << alias->contents << "]]></send>\n";
    }

    out << "  </alias>\n";
}

/**
 * Save_One_Timer_XML - Save single timer for Plugin Wizard
 *
 * Writes a single <timer> element as raw XML text.
 * Used by Plugin Wizard to generate plugin XML files.
 *
 * Save_One_Timer_XML()
 *
 * @param out Text stream for writing XML
 * @param timer Timer to save
 */
void WorldDocument::Save_One_Timer_XML(QTextStream& out, Timer* timer)
{
    // Helper lambda for XML attribute escaping
    auto escapeXml = [](const QString& str) -> QString {
        QString escaped = str;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        escaped.replace("\"", "&quot;");
        escaped.replace("'", "&apos;");
        return escaped;
    };

    // Start timer element with attributes
    out << "  <timer\n";
    out << QString("   name=\"%1\"\n").arg(escapeXml(timer->label));
    out << QString("   enabled=\"%1\"\n").arg(timer->enabled ? "y" : "n");
    out << QString("   send_to=\"%1\"\n").arg(timer->send_to);

    if (!timer->procedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(timer->procedure));
    if (timer->scriptLanguage != ScriptLanguage::Lua)
        out << QString("   script_language=\"%1\"\n")
                   .arg(scriptLanguageToString(timer->scriptLanguage));
    if (!timer->group.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(timer->group));
    if (!timer->variable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(timer->variable));

    // Timing configuration
    out << QString("   type=\"%1\"\n").arg(timer->type);

    // At-time fields (for eAtTime timers)
    out << QString("   at_hour=\"%1\"\n").arg(timer->at_hour);
    out << QString("   at_minute=\"%1\"\n").arg(timer->at_minute);
    out << QString("   at_second=\"%1\"\n").arg(timer->at_second, 0, 'f', 4);

    // Interval fields (for eInterval timers)
    out << QString("   every_hour=\"%1\"\n").arg(timer->every_hour);
    out << QString("   every_minute=\"%1\"\n").arg(timer->every_minute);
    out << QString("   every_second=\"%1\"\n").arg(timer->every_second, 0, 'f', 4);

    // Offset fields (for interval timers)
    out << QString("   offset_hour=\"%1\"\n").arg(timer->offset_hour);
    out << QString("   offset_minute=\"%1\"\n").arg(timer->offset_minute);
    out << QString("   offset_second=\"%1\"\n").arg(timer->offset_second, 0, 'f', 4);

    // Behavior flags (only write if not default)
    if (timer->one_shot)
        out << "   one_shot=\"y\"\n";
    if (timer->active_when_closed)
        out << "   active_when_closed=\"y\"\n";
    if (timer->omit_from_output)
        out << "   omit_from_output=\"y\"\n";
    if (timer->omit_from_log)
        out << "   omit_from_log=\"y\"\n";

    // User option
    if (timer->user_option != 0)
        out << QString("   user=\"%1\"\n").arg(timer->user_option);

    out << "  >\n";

    // Contents as <send> child element with CDATA
    if (!timer->contents.isEmpty()) {
        out << "  <send><![CDATA[" << timer->contents << "]]></send>\n";
    }

    out << "  </timer>\n";
}

// ============================================================================
// ACCELERATOR SERIALIZATION (User-defined keyboard shortcuts)
// ============================================================================

void WorldDocument::saveAcceleratorsToXml(QXmlStreamWriter& xml)
{
    // Get user-defined accelerators only (not script/plugin runtime ones)
    QVector<AcceleratorEntry> keyBindings = m_acceleratorManager->keyBindingList();

    // Filter out keys that are saved as macros or keypad (for bidirectional compatibility)
    // Those are saved separately in saveMacrosToXml() and saveKeypadToXml()
    QVector<AcceleratorEntry> otherAccels;
    for (const auto& entry : keyBindings) {
        if (!MacroKeypadCompat::isMacroKey(entry.keyString) &&
            !MacroKeypadCompat::isKeypadKey(entry.keyString)) {
            otherAccels.append(entry);
        }
    }

    if (otherAccels.isEmpty()) {
        return; // Don't write empty element
    }

    xml.writeStartElement("accelerators");

    for (const auto& entry : otherAccels) {
        xml.writeStartElement("accelerator");

        xml.writeAttribute("key", entry.keyString);
        xml.writeAttribute("action", entry.action);
        xml.writeAttribute("sendto", QString::number(entry.sendTo));
        xml.writeAttribute("enabled", entry.enabled ? "y" : "n");

        xml.writeEndElement(); // accelerator
    }

    xml.writeEndElement(); // accelerators
}

void WorldDocument::loadAcceleratorsFromXml(QXmlStreamReader& xml)
{
    // Expect: currently positioned at <accelerators>

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("accelerators")) {
            break;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("accelerator")) {
            QXmlStreamAttributes attrs = xml.attributes();

            QString keyString = attrs.value("key").toString();
            QString action = attrs.value("action").toString();
            int sendTo = attrs.value("sendto").toString().toInt();
            bool enabled = attrs.value("enabled").toString() != "n";

            if (!keyString.isEmpty() && !action.isEmpty()) {
                // Add as user accelerator
                m_acceleratorManager->addKeyBinding(keyString, action, sendTo);

                // Set enabled state
                if (!enabled) {
                    m_acceleratorManager->setAcceleratorEnabled(keyString, false);
                }
            }
        }
    }
}

// ============================================================================
// Macro/Keypad Compatibility - Load Functions
// ============================================================================

void WorldDocument::loadMacrosFromXml(QXmlStreamReader& xml)
{
    // Expect: currently positioned at <macros>
    // Original format:
    // <macros>
    //   <macro name="F1" type="replace">
    //     <send>look</send>
    //   </macro>
    // </macros>

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("macros")) {
            break;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("macro")) {
            QXmlStreamAttributes attrs = xml.attributes();

            QString macroName = attrs.value("name").toString();
            QString macroType = attrs.value("type").toString();
            QString sendText;

            // Read <send> child element
            while (!xml.atEnd() && !xml.hasError()) {
                xml.readNext();

                if (xml.isEndElement() && xml.name() == QLatin1String("macro")) {
                    break;
                }

                if (xml.isStartElement() && xml.name() == QLatin1String("send")) {
                    sendText = xml.readElementText();
                }
            }

            if (!macroName.isEmpty() && !sendText.isEmpty()) {
                // Convert macro name to Qt key string
                QString keyString = MacroKeypadCompat::macroNameToKeyString(macroName);
                if (keyString.isEmpty()) {
                    // Fallback: use the name as-is for command macros
                    keyString = macroName;
                }

                // Convert macro type to sendTo value
                int sendTo = MacroKeypadCompat::macroTypeToSendTo(macroType);

                // Add to accelerator manager
                m_acceleratorManager->addKeyBinding(keyString, sendText, sendTo);

                qCDebug(lcWorld) << "Loaded macro:" << macroName << "-> key:" << keyString
                                 << "action:" << sendText << "sendTo:" << sendTo;
            }
        }
    }
}

void WorldDocument::loadKeypadFromXml(QXmlStreamReader& xml)
{
    // Expect: currently positioned at <keypad>
    // Original format:
    // <keypad>
    //   <key name="8">
    //     <send>north</send>
    //   </key>
    // </keypad>

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isEndElement() && xml.name() == QLatin1String("keypad")) {
            break;
        }

        if (xml.isStartElement() && xml.name() == QLatin1String("key")) {
            QXmlStreamAttributes attrs = xml.attributes();

            QString keypadName = attrs.value("name").toString();
            QString sendText;

            // Read <send> child element
            while (!xml.atEnd() && !xml.hasError()) {
                xml.readNext();

                if (xml.isEndElement() && xml.name() == QLatin1String("key")) {
                    break;
                }

                if (xml.isStartElement() && xml.name() == QLatin1String("send")) {
                    sendText = xml.readElementText();
                }
            }

            if (!keypadName.isEmpty() && !sendText.isEmpty()) {
                // Convert keypad name to Qt key string
                QString keyString = MacroKeypadCompat::keypadNameToKeyString(keypadName);
                if (keyString.isEmpty()) {
                    qCWarning(lcWorld) << "Unknown keypad key:" << keypadName;
                    continue;
                }

                // Keypad always sends directly to world (send_now behavior)
                int sendTo = eSendToWorld;

                // Add to accelerator manager
                m_acceleratorManager->addKeyBinding(keyString, sendText, sendTo);

                qCDebug(lcWorld) << "Loaded keypad:" << keypadName << "-> key:" << keyString
                                 << "action:" << sendText;
            }
        }
    }
}

// ============================================================================
// Macro/Keypad Compatibility - Save Functions
// ============================================================================

void WorldDocument::saveMacrosToXml(QXmlStreamWriter& xml)
{
    // Get all user accelerators
    QVector<AcceleratorEntry> keyBindings = m_acceleratorManager->keyBindingList();

    // Filter to macro-compatible keys only
    QVector<AcceleratorEntry> macroAccels;
    for (const auto& entry : keyBindings) {
        if (MacroKeypadCompat::isMacroKey(entry.keyString)) {
            macroAccels.append(entry);
        }
    }

    if (macroAccels.isEmpty()) {
        return; // Don't write empty element
    }

    xml.writeStartElement("macros");

    for (const auto& entry : macroAccels) {
        QString macroName = MacroKeypadCompat::keyStringToMacroName(entry.keyString);
        QString macroType = MacroKeypadCompat::sendToToMacroType(entry.sendTo);

        if (macroName.isEmpty() || macroType.isEmpty()) {
            continue;
        }

        xml.writeStartElement("macro");
        xml.writeAttribute("name", macroName);
        xml.writeAttribute("type", macroType);

        // Write <send> element with CDATA if needed
        if (entry.action.contains('\n') || entry.action.contains('<') ||
            entry.action.contains('>') || entry.action.contains('&')) {
            xml.writeStartElement("send");
            xml.writeCDATA(entry.action);
            xml.writeEndElement();
        } else {
            xml.writeTextElement("send", entry.action);
        }

        xml.writeEndElement(); // macro
    }

    xml.writeEndElement(); // macros
}

void WorldDocument::saveKeypadToXml(QXmlStreamWriter& xml)
{
    // Get all user accelerators
    QVector<AcceleratorEntry> keyBindings = m_acceleratorManager->keyBindingList();

    // Filter to keypad-compatible keys only
    QVector<AcceleratorEntry> keypadAccels;
    for (const auto& entry : keyBindings) {
        if (MacroKeypadCompat::isKeypadKey(entry.keyString)) {
            keypadAccels.append(entry);
        }
    }

    if (keypadAccels.isEmpty()) {
        return; // Don't write empty element
    }

    xml.writeStartElement("keypad");

    for (const auto& entry : keypadAccels) {
        QString keypadName = MacroKeypadCompat::keyStringToKeypadName(entry.keyString);

        if (keypadName.isEmpty()) {
            continue;
        }

        xml.writeStartElement("key");
        xml.writeAttribute("name", keypadName);

        // Write <send> element
        if (entry.action.contains('\n') || entry.action.contains('<') ||
            entry.action.contains('>') || entry.action.contains('&')) {
            xml.writeStartElement("send");
            xml.writeCDATA(entry.action);
            xml.writeEndElement();
        } else {
            xml.writeTextElement("send", entry.action);
        }

        xml.writeEndElement(); // key
    }

    xml.writeEndElement(); // keypad
}
