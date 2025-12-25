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

// Helper function to convert RGB color to hex name
static QString colorToName(QRgb rgb)
{
    // For now, just return hex format #RRGGBB
    // Original MUSHclient would check color name map first
    return QString("#%1%2%3")
        .arg(qRed(rgb), 2, 16, QChar('0'))
        .arg(qGreen(rgb), 2, 16, QChar('0'))
        .arg(qBlue(rgb), 2, 16, QChar('0'))
        .toUpper();
}

// Helper function to parse color name to RGB
static QRgb nameToColor(const QString& name)
{
    if (name.startsWith("#")) {
        // Hex format: #RRGGBB
        bool ok;
        unsigned int rgb = name.mid(1).toUInt(&ok, 16);
        if (ok) {
            return qRgb((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }
    }
    // Could add named color lookup here (red, blue, etc.)
    // For now, return black as default
    return qRgb(0, 0, 0);
}

void WorldDocument::saveTriggersToXml(QXmlStreamWriter& xml)
{
    xml.writeStartElement("triggers");

    for (const auto& [name, triggerPtr] : m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();

        // Skip temporary triggers
        if (trigger->bTemporary)
            continue;

        xml.writeStartElement("trigger");

        // Write attributes
        xml.writeAttribute("name", trigger->strLabel);
        xml.writeAttribute("enabled", trigger->bEnabled ? "y" : "n");
        xml.writeAttribute("match", trigger->trigger);
        xml.writeAttribute("send_to", QString::number(trigger->iSendTo));
        xml.writeAttribute("sequence", QString::number(trigger->iSequence));
        xml.writeAttribute("script", trigger->strProcedure);
        xml.writeAttribute("group", trigger->strGroup);
        xml.writeAttribute("variable", trigger->strVariable);

        // Behavior flags
        xml.writeAttribute("omit_from_output", trigger->bOmitFromOutput ? "y" : "n");
        xml.writeAttribute("omit_from_log", trigger->omit_from_log ? "y" : "n");
        xml.writeAttribute("keep_evaluating", trigger->bKeepEvaluating ? "y" : "n");
        xml.writeAttribute("regexp", trigger->bRegexp ? "y" : "n");
        xml.writeAttribute("ignore_case", trigger->ignore_case ? "y" : "n");
        xml.writeAttribute("repeat", trigger->bRepeat ? "y" : "n");
        xml.writeAttribute("expand_variables", trigger->bExpandVariables ? "y" : "n");
        xml.writeAttribute("one_shot", trigger->bOneShot ? "y" : "n");
        xml.writeAttribute("lowercase_wildcard", trigger->bLowercaseWildcard ? "y" : "n");

        // Multi-line matching
        xml.writeAttribute("multi_line", trigger->bMultiLine ? "y" : "n");
        xml.writeAttribute("lines_to_match", QString::number(trigger->iLinesToMatch));

        // Sound
        xml.writeAttribute("sound", trigger->sound_to_play);
        xml.writeAttribute("sound_if_inactive", trigger->bSoundIfInactive ? "y" : "n");

        // Decompose iStyle into individual make_* attributes
        if (trigger->iStyle & HILITE)
            xml.writeAttribute("make_bold", "y");
        if (trigger->iStyle & BLINK)
            xml.writeAttribute("make_italic", "y");
        if (trigger->iStyle & UNDERLINE)
            xml.writeAttribute("make_underline", "y");

        // Decompose iMatch into individual attributes
        // Extract ANSI color indices (0-15)
        int text_colour = (trigger->iMatch >> 4) & 0x0F;
        int back_colour = (trigger->iMatch >> 8) & 0x0F;
        if (text_colour != 0)
            xml.writeAttribute("text_colour", QString::number(text_colour));
        if (back_colour != 0)
            xml.writeAttribute("back_colour", QString::number(back_colour));

        // Extract style matching flags
        if (trigger->iMatch & HILITE)
            xml.writeAttribute("bold", "y");
        if (trigger->iMatch & INVERSE)
            xml.writeAttribute("inverse", "y");
        if (trigger->iMatch & BLINK)
            xml.writeAttribute("italic", "y");
        if (trigger->iMatch & TRIGGER_MATCH_TEXT)
            xml.writeAttribute("match_text_colour", "y");
        if (trigger->iMatch & TRIGGER_MATCH_BACK)
            xml.writeAttribute("match_back_colour", "y");
        if (trigger->iMatch & TRIGGER_MATCH_HILITE)
            xml.writeAttribute("match_bold", "y");
        if (trigger->iMatch & TRIGGER_MATCH_INVERSE)
            xml.writeAttribute("match_inverse", "y");
        if (trigger->iMatch & TRIGGER_MATCH_BLINK)
            xml.writeAttribute("match_italic", "y");
        if (trigger->iMatch & TRIGGER_MATCH_UNDERLINE)
            xml.writeAttribute("match_underline", "y");

        // Custom color (add 1, skip SAMECOLOUR)
        if (trigger->colour != SAMECOLOUR)
            xml.writeAttribute("custom_colour", QString::number(trigger->colour + 1));

        xml.writeAttribute("colour_change_type", QString::number(trigger->iColourChangeType));

        // RGB colors as names
        if (trigger->iOtherForeground != 0)
            xml.writeAttribute("other_text_colour", colorToName(trigger->iOtherForeground));
        if (trigger->iOtherBackground != 0)
            xml.writeAttribute("other_back_colour", colorToName(trigger->iOtherBackground));

        // Other options
        xml.writeAttribute("clipboard_arg", QString::number(trigger->iClipboardArg));
        xml.writeAttribute("user", QString::number(trigger->iUserOption));

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

            trigger->strLabel = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            if (trigger->strLabel.isEmpty()) {
                // Use pattern as base for name (truncated if too long)
                QString pattern = attrs.value("match").toString();
                if (pattern.length() > 50) {
                    pattern = pattern.left(50) + "...";
                }
                // Make it unique by appending a counter
                trigger->strInternalName =
                    QString("trigger_%1_%2").arg(triggerCount).arg(qHash(pattern));
            } else {
                trigger->strInternalName = trigger->strLabel;
            }
            trigger->bEnabled = attrs.value("enabled").toString() == "y";
            trigger->trigger = attrs.value("match").toString();
            trigger->iSendTo = attrs.value("send_to").toInt();
            trigger->iSequence = attrs.value("sequence").toInt();
            trigger->strProcedure = attrs.value("script").toString();
            trigger->strGroup = attrs.value("group").toString();
            trigger->strVariable = attrs.value("variable").toString();

            // Behavior flags
            trigger->bOmitFromOutput = attrs.value("omit_from_output").toString() == "y";
            trigger->omit_from_log = attrs.value("omit_from_log").toString() == "y";
            // Only set to false if explicitly "n", otherwise keep default (true)
            if (attrs.hasAttribute("keep_evaluating")) {
                trigger->bKeepEvaluating = attrs.value("keep_evaluating").toString() != "n";
            }
            trigger->bRegexp = attrs.value("regexp").toString() == "y";
            trigger->ignore_case = attrs.value("ignore_case").toString() == "y";
            trigger->bRepeat = attrs.value("repeat").toString() == "y";
            trigger->bExpandVariables = attrs.value("expand_variables").toString() == "y";
            trigger->bOneShot = attrs.value("one_shot").toString() == "y";
            trigger->bLowercaseWildcard = attrs.value("lowercase_wildcard").toString() == "y";

            // Multi-line matching
            trigger->bMultiLine = attrs.value("multi_line").toString() == "y";
            trigger->iLinesToMatch = attrs.value("lines_to_match").toInt();

            // Sound
            trigger->sound_to_play = attrs.value("sound").toString();
            trigger->bSoundIfInactive = attrs.value("sound_if_inactive").toString() == "y";

            // Compose iStyle from individual make_* attributes
            trigger->iStyle = 0;
            if (attrs.value("make_bold").toString() == "y")
                trigger->iStyle |= HILITE;
            if (attrs.value("make_italic").toString() == "y")
                trigger->iStyle |= BLINK;
            if (attrs.value("make_underline").toString() == "y")
                trigger->iStyle |= UNDERLINE;

            // Compose iMatch from individual attributes
            trigger->iMatch = 0;

            // Pack ANSI color indices (0-15) into iMatch
            if (attrs.hasAttribute("text_colour")) {
                int text_colour = attrs.value("text_colour").toInt();
                trigger->iMatch |= (text_colour << 4);
            }
            if (attrs.hasAttribute("back_colour")) {
                int back_colour = attrs.value("back_colour").toInt();
                trigger->iMatch |= (back_colour << 8);
            }

            // Compose style matching flags
            if (attrs.value("bold").toString() == "y")
                trigger->iMatch |= HILITE;
            if (attrs.value("inverse").toString() == "y")
                trigger->iMatch |= INVERSE;
            if (attrs.value("italic").toString() == "y")
                trigger->iMatch |= BLINK;
            if (attrs.value("match_text_colour").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_TEXT;
            if (attrs.value("match_back_colour").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_BACK;
            if (attrs.value("match_bold").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_HILITE;
            if (attrs.value("match_inverse").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_INVERSE;
            if (attrs.value("match_italic").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_BLINK;
            if (attrs.value("match_underline").toString() == "y")
                trigger->iMatch |= TRIGGER_MATCH_UNDERLINE;

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
                trigger->iColourChangeType = attrs.value("colour_change_type").toInt();

            // RGB colors from names
            if (attrs.hasAttribute("other_text_colour"))
                trigger->iOtherForeground =
                    nameToColor(attrs.value("other_text_colour").toString());
            if (attrs.hasAttribute("other_back_colour"))
                trigger->iOtherBackground =
                    nameToColor(attrs.value("other_back_colour").toString());

            // Other options
            if (attrs.hasAttribute("clipboard_arg"))
                trigger->iClipboardArg = attrs.value("clipboard_arg").toInt();
            if (attrs.hasAttribute("user"))
                trigger->iUserOption = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    trigger->contents = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            // Compile regex if needed
            if (trigger->bRegexp) {
                trigger->compileRegexp();
            }

            // Add to plugin or world collections based on context
            if (plugin) {
                // Add to plugin's collections
                trigger->strInternalName =
                    trigger->strLabel.isEmpty()
                        ? QString("trigger_%1_%2").arg(triggerCount).arg(qHash(trigger->trigger))
                        : trigger->strLabel;

                QString triggerName = trigger->strInternalName;

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
                qCDebug(lcWorld) << "Added trigger to plugin:" << rawPtr->strInternalName
                                 << "sequence:" << rawPtr->iSequence;
            } else {
                // Add to world's collections (existing behavior)
                QString triggerName = trigger->strInternalName;
                addTrigger(triggerName, std::move(trigger));
                m_triggersNeedSorting = true;
            }
        }
    }
}

void WorldDocument::saveAliasesToXml(QXmlStreamWriter& xml)
{
    xml.writeStartElement("aliases");

    for (const auto& [name, aliasPtr] : m_AliasMap) {
        Alias* alias = aliasPtr.get();

        // Skip temporary aliases
        if (alias->bTemporary)
            continue;

        xml.writeStartElement("alias");

        // Write attributes
        xml.writeAttribute("name", alias->strLabel);
        xml.writeAttribute("enabled", alias->bEnabled ? "y" : "n");
        xml.writeAttribute("match", alias->name);
        xml.writeAttribute("send_to", QString::number(alias->iSendTo));
        xml.writeAttribute("sequence", QString::number(alias->iSequence));
        xml.writeAttribute("script", alias->strProcedure);
        xml.writeAttribute("group", alias->strGroup);
        xml.writeAttribute("variable", alias->strVariable);

        // Behavior flags
        xml.writeAttribute("omit_from_output", alias->bOmitFromOutput ? "y" : "n");
        xml.writeAttribute("omit_from_log", alias->bOmitFromLog ? "y" : "n");
        xml.writeAttribute("omit_from_command_history", alias->bOmitFromCommandHistory ? "y" : "n");
        xml.writeAttribute("keep_evaluating", alias->bKeepEvaluating ? "y" : "n");
        xml.writeAttribute("regexp", alias->bRegexp ? "y" : "n");
        xml.writeAttribute("ignore_case", alias->bIgnoreCase ? "y" : "n");
        xml.writeAttribute("expand_variables", alias->bExpandVariables ? "y" : "n");
        xml.writeAttribute("echo_alias", alias->bEchoAlias ? "y" : "n");
        xml.writeAttribute("one_shot", alias->bOneShot ? "y" : "n");
        xml.writeAttribute("menu", alias->bMenu ? "y" : "n");

        // Other options
        xml.writeAttribute("user", QString::number(alias->iUserOption));

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

            alias->strLabel = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            if (alias->strLabel.isEmpty()) {
                QString match = attrs.value("match").toString();
                if (match.length() > 50) {
                    match = match.left(50) + "...";
                }
                alias->strInternalName = QString("alias_%1_%2").arg(aliasCount).arg(qHash(match));
            } else {
                alias->strInternalName = alias->strLabel;
            }

            aliasCount++;
            alias->bEnabled = attrs.value("enabled").toString() == "y";
            alias->name = attrs.value("match").toString();
            alias->iSendTo = attrs.value("send_to").toInt();
            alias->iSequence = attrs.value("sequence").toInt();
            alias->strProcedure = attrs.value("script").toString();
            alias->strGroup = attrs.value("group").toString();
            alias->strVariable = attrs.value("variable").toString();

            // Behavior flags
            alias->bOmitFromOutput = attrs.value("omit_from_output").toString() == "y";
            alias->bOmitFromLog = attrs.value("omit_from_log").toString() == "y";
            alias->bOmitFromCommandHistory =
                attrs.value("omit_from_command_history").toString() == "y";
            // Only set to false if explicitly "n", otherwise keep default (true)
            if (attrs.hasAttribute("keep_evaluating")) {
                alias->bKeepEvaluating = attrs.value("keep_evaluating").toString() != "n";
            }
            alias->bRegexp = attrs.value("regexp").toString() == "y";
            alias->bIgnoreCase = attrs.value("ignore_case").toString() == "y";
            alias->bExpandVariables = attrs.value("expand_variables").toString() == "y";
            alias->bEchoAlias = attrs.value("echo_alias").toString() == "y";
            alias->bOneShot = attrs.value("one_shot").toString() == "y";
            alias->bMenu = attrs.value("menu").toString() == "y";

            // Other options
            if (attrs.hasAttribute("user"))
                alias->iUserOption = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    alias->contents = xml.readElementText();
                } else {
                    xml.skipCurrentElement();
                }
            }

            // Compile regex if needed
            if (alias->bRegexp) {
                alias->compileRegexp();
            }

            // Add to plugin or world collections based on context
            if (plugin) {
                // Add to plugin's collections
                QString internalName = alias->strInternalName;

                // Skip duplicates - don't overwrite existing aliases
                if (plugin->m_AliasMap.find(internalName) != plugin->m_AliasMap.end()) {
                    qCDebug(lcWorld) << "Skipping duplicate alias:" << internalName;
                    continue;
                }

                qint32 sequence = alias->iSequence;
                Alias* rawPtr = alias.get();
                plugin->m_AliasMap[internalName] = std::move(alias);
                plugin->m_AliasArray.append(rawPtr);
                plugin->m_aliasesNeedSorting = true;
                qCDebug(lcWorld) << "Added alias to plugin:" << internalName
                                 << "sequence:" << sequence;
            } else {
                // Add to world's collections (existing behavior)
                QString internalName = alias->strInternalName;
                addAlias(internalName, std::move(alias));
                m_aliasesNeedSorting = true;
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

    for (const auto& [name, timerPtr] : m_TimerMap) {
        Timer* timer = timerPtr.get();

        // Skip temporary timers (don't save to file)
        if (timer->bTemporary)
            continue;

        // Skip included timers (loaded from included file)
        if (timer->bIncluded)
            continue;

        xml.writeStartElement("timer");

        // Write basic attributes
        xml.writeAttribute("name", timer->strLabel);
        xml.writeAttribute("enabled", timer->bEnabled ? "y" : "n");
        xml.writeAttribute("send_to", QString::number(timer->iSendTo));
        xml.writeAttribute("script", timer->strProcedure);
        xml.writeAttribute("group", timer->strGroup);
        xml.writeAttribute("variable", timer->strVariable);

        // Timing configuration - use original MUSHclient format for compatibility
        // Original format uses at_time="y"/"n" instead of type="0"/"1"
        // and uses hour/minute/second (picking from at_* or every_* based on type)
        bool isAtTime = (timer->iType == Timer::eAtTime);
        xml.writeAttribute("at_time", isAtTime ? "y" : "n");

        // Write hour/minute/second from either at_* or every_* fields based on type
        if (isAtTime) {
            xml.writeAttribute("hour", QString::number(timer->iAtHour));
            xml.writeAttribute("minute", QString::number(timer->iAtMinute));
            xml.writeAttribute("second", QString::number(timer->fAtSecond, 'f', 4));
        } else {
            xml.writeAttribute("hour", QString::number(timer->iEveryHour));
            xml.writeAttribute("minute", QString::number(timer->iEveryMinute));
            xml.writeAttribute("second", QString::number(timer->fEverySecond, 'f', 4));
        }

        // Offset fields (always written)
        xml.writeAttribute("offset_hour", QString::number(timer->iOffsetHour));
        xml.writeAttribute("offset_minute", QString::number(timer->iOffsetMinute));
        xml.writeAttribute("offset_second", QString::number(timer->fOffsetSecond, 'f', 4));

        // Behavior flags
        xml.writeAttribute("one_shot", timer->bOneShot ? "y" : "n");
        // Use active_closed for original MUSHclient compatibility
        xml.writeAttribute("active_closed", timer->bActiveWhenClosed ? "y" : "n");
        xml.writeAttribute("omit_from_output", timer->bOmitFromOutput ? "y" : "n");
        xml.writeAttribute("omit_from_log", timer->bOmitFromLog ? "y" : "n");

        // Other options
        xml.writeAttribute("user", QString::number(timer->iUserOption));

        // Contents as child element with CDATA
        if (!timer->strContents.isEmpty()) {
            xml.writeStartElement("send");
            xml.writeCDATA(timer->strContents);
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

            timer->strLabel = attrs.value("name").toString();

            // Generate unique internal name if no name attribute exists
            QString internalName;
            if (timer->strLabel.isEmpty()) {
                // Use timestamp-based unique name for unlabelled timers
                internalName = QString("*timer%1").arg(timerCount, 10, 10, QChar('0'));
            } else {
                internalName = timer->strLabel;
            }

            timer->bEnabled = attrs.value("enabled").toString() == "y";
            timer->iSendTo = attrs.value("send_to").toInt();
            timer->strProcedure = attrs.value("script").toString();
            timer->strGroup = attrs.value("group").toString();
            timer->strVariable = attrs.value("variable").toString();

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
            timer->iType = isAtTime ? Timer::eAtTime : Timer::eInterval;

            // Read hour/minute/second from either simple names or specific names
            int hour = attrs.value("hour").toInt();
            int minute = attrs.value("minute").toInt();
            double second = attrs.value("second").toDouble();

            if (isAtTime) {
                // At-time timer: use hour/minute/second for at_* fields
                timer->iAtHour =
                    attrs.hasAttribute("at_hour") ? attrs.value("at_hour").toInt() : hour;
                timer->iAtMinute =
                    attrs.hasAttribute("at_minute") ? attrs.value("at_minute").toInt() : minute;
                timer->fAtSecond =
                    attrs.hasAttribute("at_second") ? attrs.value("at_second").toDouble() : second;
            } else {
                // Interval timer: use hour/minute/second for every_* fields
                timer->iEveryHour =
                    attrs.hasAttribute("every_hour") ? attrs.value("every_hour").toInt() : hour;
                timer->iEveryMinute = attrs.hasAttribute("every_minute")
                                          ? attrs.value("every_minute").toInt()
                                          : minute;
                timer->fEverySecond = attrs.hasAttribute("every_second")
                                          ? attrs.value("every_second").toDouble()
                                          : second;
            }

            // Offset fields
            timer->iOffsetHour = attrs.value("offset_hour").toInt();
            timer->iOffsetMinute = attrs.value("offset_minute").toInt();
            timer->fOffsetSecond = attrs.value("offset_second").toDouble();

            // Behavior flags
            timer->bOneShot = attrs.value("one_shot").toString() == "y";
            // active_closed is alternate name for active_when_closed
            timer->bActiveWhenClosed = (attrs.value("active_when_closed").toString() == "y") ||
                                       (attrs.value("active_closed").toString() == "y");
            timer->bOmitFromOutput = attrs.value("omit_from_output").toString() == "y";
            timer->bOmitFromLog = attrs.value("omit_from_log").toString() == "y";

            // Other options
            if (attrs.hasAttribute("user"))
                timer->iUserOption = attrs.value("user").toInt();

            // Read <send> child element
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("send")) {
                    timer->strContents = xml.readElementText();
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
                if (rawTimer->strLabel.isEmpty()) {
                    plugin->m_TimerRevMap.insert(rawTimer, internalName);
                }
                qCDebug(lcWorld) << "Added timer to plugin:" << internalName;
            } else {
                // Add to world's collections (transfer ownership via move)
                addTimer(internalName, std::move(timer));
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
        xml.writeAttribute("name", var->strLabel);
        xml.writeCharacters(var->strContents);
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
            var->strLabel = varName;
            var->strContents = contents;

            if (plugin) {
                plugin->m_VariableMap[var->strLabel] = std::move(var);
            } else {
                m_VariableMap[var->strLabel] = std::move(var);
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
    QString escapedName = variable->strLabel;
    escapedName.replace("&", "&amp;");
    escapedName.replace("<", "&lt;");
    escapedName.replace(">", "&gt;");
    escapedName.replace("\"", "&quot;");
    escapedName.replace("'", "&apos;");

    QString escapedContents = variable->strContents;
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
    out << QString("   name=\"%1\"\n").arg(escapeXml(trigger->strLabel));
    out << QString("   enabled=\"%1\"\n").arg(trigger->bEnabled ? "y" : "n");
    out << QString("   match=\"%1\"\n").arg(escapeXml(trigger->trigger));
    out << QString("   send_to=\"%1\"\n").arg(trigger->iSendTo);
    out << QString("   sequence=\"%1\"\n").arg(trigger->iSequence);

    if (!trigger->strProcedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(trigger->strProcedure));
    if (!trigger->strGroup.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(trigger->strGroup));
    if (!trigger->strVariable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(trigger->strVariable));

    // Behavior flags (only write if not default)
    if (trigger->bOmitFromOutput)
        out << "   omit_from_output=\"y\"\n";
    if (trigger->omit_from_log)
        out << "   omit_from_log=\"y\"\n";
    if (!trigger->bKeepEvaluating)
        out << "   keep_evaluating=\"n\"\n";
    if (trigger->bRegexp)
        out << "   regexp=\"y\"\n";
    if (trigger->ignore_case)
        out << "   ignore_case=\"y\"\n";
    if (trigger->bRepeat)
        out << "   repeat=\"y\"\n";
    if (trigger->bExpandVariables)
        out << "   expand_variables=\"y\"\n";
    if (trigger->bOneShot)
        out << "   one_shot=\"y\"\n";
    if (trigger->bLowercaseWildcard)
        out << "   lowercase_wildcard=\"y\"\n";

    // Multi-line matching
    if (trigger->bMultiLine) {
        out << "   multi_line=\"y\"\n";
        out << QString("   lines_to_match=\"%1\"\n").arg(trigger->iLinesToMatch);
    }

    // Sound
    if (!trigger->sound_to_play.isEmpty()) {
        out << QString("   sound=\"%1\"\n").arg(escapeXml(trigger->sound_to_play));
        if (trigger->bSoundIfInactive)
            out << "   sound_if_inactive=\"y\"\n";
    }

    // Style flags
    if (trigger->iStyle & HILITE)
        out << "   make_bold=\"y\"\n";
    if (trigger->iStyle & BLINK)
        out << "   make_italic=\"y\"\n";
    if (trigger->iStyle & UNDERLINE)
        out << "   make_underline=\"y\"\n";

    // Extract ANSI color indices from iMatch (bits 4-11)
    int text_colour = (trigger->iMatch >> 4) & 0x0F;
    int back_colour = (trigger->iMatch >> 8) & 0x0F;
    if (text_colour != 0)
        out << QString("   text_colour=\"%1\"\n").arg(text_colour);
    if (back_colour != 0)
        out << QString("   back_colour=\"%1\"\n").arg(back_colour);

    // Extract style matching flags from iMatch
    if (trigger->iMatch & HILITE)
        out << "   bold=\"y\"\n";
    if (trigger->iMatch & INVERSE)
        out << "   inverse=\"y\"\n";
    if (trigger->iMatch & BLINK)
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
    out << QString("   name=\"%1\"\n").arg(escapeXml(alias->strLabel));
    out << QString("   enabled=\"%1\"\n").arg(alias->bEnabled ? "y" : "n");
    out << QString("   match=\"%1\"\n").arg(escapeXml(alias->name));
    out << QString("   send_to=\"%1\"\n").arg(alias->iSendTo);
    out << QString("   sequence=\"%1\"\n").arg(alias->iSequence);

    if (!alias->strProcedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(alias->strProcedure));
    if (!alias->strGroup.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(alias->strGroup));
    if (!alias->strVariable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(alias->strVariable));

    // Behavior flags (only write if not default)
    if (alias->bOmitFromOutput)
        out << "   omit_from_output=\"y\"\n";
    if (alias->bOmitFromLog)
        out << "   omit_from_log=\"y\"\n";
    if (alias->bOmitFromCommandHistory)
        out << "   omit_from_command_history=\"y\"\n";
    if (!alias->bKeepEvaluating)
        out << "   keep_evaluating=\"n\"\n";
    if (alias->bRegexp)
        out << "   regexp=\"y\"\n";
    if (alias->bIgnoreCase)
        out << "   ignore_case=\"y\"\n";
    if (alias->bExpandVariables)
        out << "   expand_variables=\"y\"\n";
    if (alias->bEchoAlias)
        out << "   echo_alias=\"y\"\n";
    if (alias->bOneShot)
        out << "   one_shot=\"y\"\n";
    if (alias->bMenu)
        out << "   menu=\"y\"\n";

    // User option
    if (alias->iUserOption != 0)
        out << QString("   user=\"%1\"\n").arg(alias->iUserOption);

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
    out << QString("   name=\"%1\"\n").arg(escapeXml(timer->strLabel));
    out << QString("   enabled=\"%1\"\n").arg(timer->bEnabled ? "y" : "n");
    out << QString("   send_to=\"%1\"\n").arg(timer->iSendTo);

    if (!timer->strProcedure.isEmpty())
        out << QString("   script=\"%1\"\n").arg(escapeXml(timer->strProcedure));
    if (!timer->strGroup.isEmpty())
        out << QString("   group=\"%1\"\n").arg(escapeXml(timer->strGroup));
    if (!timer->strVariable.isEmpty())
        out << QString("   variable=\"%1\"\n").arg(escapeXml(timer->strVariable));

    // Timing configuration
    out << QString("   type=\"%1\"\n").arg(timer->iType);

    // At-time fields (for eAtTime timers)
    out << QString("   at_hour=\"%1\"\n").arg(timer->iAtHour);
    out << QString("   at_minute=\"%1\"\n").arg(timer->iAtMinute);
    out << QString("   at_second=\"%1\"\n").arg(timer->fAtSecond, 0, 'f', 4);

    // Interval fields (for eInterval timers)
    out << QString("   every_hour=\"%1\"\n").arg(timer->iEveryHour);
    out << QString("   every_minute=\"%1\"\n").arg(timer->iEveryMinute);
    out << QString("   every_second=\"%1\"\n").arg(timer->fEverySecond, 0, 'f', 4);

    // Offset fields (for interval timers)
    out << QString("   offset_hour=\"%1\"\n").arg(timer->iOffsetHour);
    out << QString("   offset_minute=\"%1\"\n").arg(timer->iOffsetMinute);
    out << QString("   offset_second=\"%1\"\n").arg(timer->fOffsetSecond, 0, 'f', 4);

    // Behavior flags (only write if not default)
    if (timer->bOneShot)
        out << "   one_shot=\"y\"\n";
    if (timer->bActiveWhenClosed)
        out << "   active_when_closed=\"y\"\n";
    if (timer->bOmitFromOutput)
        out << "   omit_from_output=\"y\"\n";
    if (timer->bOmitFromLog)
        out << "   omit_from_log=\"y\"\n";

    // User option
    if (timer->iUserOption != 0)
        out << QString("   user=\"%1\"\n").arg(timer->iUserOption);

    out << "  >\n";

    // Contents as <send> child element with CDATA
    if (!timer->strContents.isEmpty()) {
        out << "  <send><![CDATA[" << timer->strContents << "]]></send>\n";
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
