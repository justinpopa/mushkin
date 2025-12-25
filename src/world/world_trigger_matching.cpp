/**
 * world_trigger_matching.cpp - Trigger Pattern Matching Engine
 *
 * Implements trigger pattern matching against incoming MUD lines.
 * Handles:
 * - Simple wildcard matching (* patterns)
 * - Regular expression matching
 * - Color/style matching
 * - Multi-line matching
 * - Repeat matching (multiple matches on same line)
 */

#include "../automation/plugin.h"
#include "../automation/trigger.h"
#include "../text/line.h"
#include "../text/style.h"
#include "logging.h"
#include "world_document.h"
#include <QDebug>
#include <QRegularExpression>

/**
 * wildcardToRegex - Convert wildcard pattern to regex
 *
 * Converts simple * wildcard patterns to QRegularExpression.
 * Example: "You have * gold" → "^You have (.*?) gold$"
 *
 * @param pattern The wildcard pattern (e.g., "You have * gold")
 * @param ignoreCase If true, create case-insensitive regex
 * @return QRegularExpression ready for matching
 */
static QRegularExpression wildcardToRegex(const QString& pattern, bool ignoreCase)
{
    // Escape all regex special characters EXCEPT *
    QString escaped = QRegularExpression::escape(pattern);

    // Replace escaped asterisks with non-greedy capture group
    // QRegularExpression::escape() turns * into \\*
    escaped.replace("\\*", "(.*?)");

    // Anchor to start and end of line
    QString fullPattern = "^" + escaped + "$";

    // Set case sensitivity
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignoreCase) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    return QRegularExpression(fullPattern, options);
}

/**
 * matchStyle - Check if line matches color/style requirements
 *
 * Examines the line's style runs to see if they match the trigger's
 * color and style requirements (iMatch and iStyle fields).
 *
 * @param trigger The trigger with style requirements
 * @param line The line to check
 * @return true if style requirements met (or no requirements), false otherwise
 */
static bool matchStyle(Trigger* trigger, Line* line)
{
    // If no style matching required, always pass
    if (trigger->iMatch == 0 && trigger->iStyle == 0) {
        return true;
    }

    // iMatch bit flags (from OtherTypes.h):
    // 0x0080 = TRIGGER_MATCH_TEXT (match text color)
    // 0x0800 = TRIGGER_MATCH_BACK (match background color)
    // 0x1000 = TRIGGER_MATCH_HILITE (match bold)
    // 0x2000 = TRIGGER_MATCH_UNDERLINE (match underline)
    // 0x4000 = TRIGGER_MATCH_BLINK (match blink/italic)
    // 0x8000 = TRIGGER_MATCH_INVERSE (match inverse)

#define TRIGGER_MATCH_TEXT 0x0080
#define TRIGGER_MATCH_BACK 0x0800
#define TRIGGER_MATCH_HILITE 0x1000
#define TRIGGER_MATCH_UNDERLINE 0x2000
#define TRIGGER_MATCH_BLINK 0x4000
#define TRIGGER_MATCH_INVERSE 0x8000

    // Check each style run in the line
    for (const auto& style : line->styleList) {
        bool matches = true;

        // Check text color
        if (trigger->iMatch & TRIGGER_MATCH_TEXT) {
            // For now, simple RGB comparison with iOtherForeground
            // TODO: Support ANSI color indices (trigger->colour field)
            if (trigger->iOtherForeground != 0) {
                if (style->iForeColour != trigger->iOtherForeground) {
                    matches = false;
                }
            }
        }

        // Check background color
        if (trigger->iMatch & TRIGGER_MATCH_BACK) {
            if (trigger->iOtherBackground != 0) {
                if (style->iBackColour != trigger->iOtherBackground) {
                    matches = false;
                }
            }
        }

        // Check style flags
        if (trigger->iMatch & TRIGGER_MATCH_HILITE) {
            if (!(style->iFlags & HILITE)) {
                matches = false;
            }
        }

        if (trigger->iMatch & TRIGGER_MATCH_UNDERLINE) {
            if (!(style->iFlags & UNDERLINE)) {
                matches = false;
            }
        }

        if (trigger->iMatch & TRIGGER_MATCH_BLINK) {
            if (!(style->iFlags & BLINK)) {
                matches = false;
            }
        }

        if (trigger->iMatch & TRIGGER_MATCH_INVERSE) {
            if (!(style->iFlags & INVERSE)) {
                matches = false;
            }
        }

        // If this style run matches, we're done
        if (matches) {
            return true;
        }
    }

    // No style run matched the requirements
    return false;
}

/**
 * matchTriggerPattern - Match trigger pattern against text
 *
 * Handles both wildcard and regex matching, extracting wildcards.
 *
 * @param trigger The trigger to match
 * @param text The text to match against
 * @return true if matched, false otherwise
 */
static bool matchTriggerPattern(Trigger* trigger, const QString& text)
{
    if (trigger->bRegexp) {
        // Regular expression matching
        if (!trigger->regexp) {
            // Compile regexp if not cached
            if (!trigger->compileRegexp()) {
                return false; // Invalid regex
            }
        }

        QRegularExpressionMatch match = trigger->regexp->match(text);
        if (!match.hasMatch()) {
            return false;
        }

        // Extract wildcards (captured groups)
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);

            // Apply lowercase conversion if requested (skip %0 - the whole match)
            if (trigger->bLowercaseWildcard && i > 0) {
                captured = captured.toLower();
            }

            trigger->wildcards[i] = captured;
        }

        // Extract named capture groups
        trigger->namedWildcards.clear();
        const QStringList namedGroups = trigger->regexp->namedCaptureGroups();
        for (const QString& name : namedGroups) {
            if (!name.isEmpty()) {
                QString captured = match.captured(name);
                if (trigger->bLowercaseWildcard) {
                    captured = captured.toLower();
                }
                trigger->namedWildcards[name] = captured;
            }
        }

        return true;

    } else {
        // Wildcard matching (convert * to regex)
        QRegularExpression re = wildcardToRegex(trigger->trigger, trigger->ignore_case);
        QRegularExpressionMatch match = re.match(text);

        if (!match.hasMatch()) {
            return false;
        }

        // Extract wildcards
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);

            // Apply lowercase conversion if requested
            if (trigger->bLowercaseWildcard && i > 0) {
                captured = captured.toLower();
            }

            trigger->wildcards[i] = captured;
        }

        return true;
    }
}

/**
 * matchTriggerWithRepeat - Match trigger with bRepeat support
 *
 * If bRepeat is true, keeps matching the same line multiple times.
 *
 * @param trigger The trigger to match
 * @param text The text to match against
 * @param line The line object (for style matching)
 * @param doc The world document (for trigger execution)
 * @return true if at least one match occurred
 */
static bool matchTriggerWithRepeat(Trigger* trigger, const QString& text, Line* line,
                                   WorldDocument* doc)
{
    // Check style requirements first (applies to entire line)
    if (!matchStyle(trigger, line)) {
        return false;
    }

    if (!trigger->bRepeat) {
        // Single match only
        if (matchTriggerPattern(trigger, text)) {
            // Execute trigger action
            doc->executeTrigger(trigger, line, text);
            return true;
        }
        return false;
    }

    // Repeat matching - match multiple times on same line
    bool anyMatch = false;
    int offset = 0;

    // Build regex for repeated matching
    QRegularExpression re;
    if (trigger->bRegexp) {
        if (!trigger->regexp) {
            if (!trigger->compileRegexp()) {
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

        // Extract wildcards for this match
        trigger->wildcards.clear();
        trigger->wildcards.resize(match.lastCapturedIndex() + 1);
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            QString captured = match.captured(i);
            if (trigger->bLowercaseWildcard && i > 0) {
                captured = captured.toLower();
            }
            trigger->wildcards[i] = captured;
        }

        // Execute trigger action
        doc->executeTrigger(trigger, line, text);
        anyMatch = true;

        // Move offset past this match
        offset = match.capturedEnd();
        if (offset <= match.capturedStart()) {
            // Prevent infinite loop on zero-width matches
            offset++;
        }
    }

    return anyMatch;
}

/**
 * evaluateOneTriggerSequence - Helper to evaluate one trigger sequence
 *
 * Plugin Evaluation Order
 * Helper function that evaluates a single trigger array (world or plugin).
 * Returns true if evaluation should stop (one-shot or !bKeepEvaluating).
 *
 * @param triggerArray Array of triggers to evaluate
 * @param line The completed line to check triggers against
 * @param lineText QString version of line text
 * @param oneShotToDelete Output parameter for one-shot trigger name
 * @return true if evaluation should stop
 */
static bool evaluateOneTriggerSequence(const QVector<Trigger*>& triggerArray, Line* line,
                                       const QString& lineText, WorldDocument* doc,
                                       QString& oneShotToDelete)
{
    // Check each trigger in sequence order
    for (Trigger* trigger : triggerArray) {
        // Skip disabled triggers
        if (!trigger->bEnabled) {
            continue;
        }

        bool matched = false;

        // Handle multi-line matching
        if (trigger->bMultiLine && trigger->iLinesToMatch > 1) {
            // Assemble multi-line match text from recent lines buffer
            // Based on evaluate.cpp in original MUSHclient
            QString multiLineText;
            int bufferSize = static_cast<int>(doc->m_recentLines.size());
            int startPos = bufferSize - trigger->iLinesToMatch;
            if (startPos < 0) {
                startPos = 0;
            }

            for (int i = startPos; i < bufferSize; ++i) {
                multiLineText += doc->m_recentLines[static_cast<size_t>(i)];
                multiLineText += '\n'; // Multi-line triggers always end lines with newline
            }

            matched = matchTriggerWithRepeat(trigger, multiLineText, line, doc);

        } else {
            // Single-line matching
            matched = matchTriggerWithRepeat(trigger, lineText, line, doc);
        }

        if (matched) {
            doc->m_iTriggersMatchedCount++;
            doc->m_iTriggersMatchedThisSessionCount++;

            // One-shot: mark for deletion after iteration completes
            if (trigger->bOneShot) {
                oneShotToDelete = trigger->strInternalName;
                // One-shot triggers also stop evaluation
                return true;
            }

            // Stop evaluating if this trigger doesn't allow keep evaluating
            if (!trigger->bKeepEvaluating) {
                return true;
            }
        }
    }

    return false; // Continue evaluation
}

/**
 * evaluateTriggers - Evaluate all enabled triggers against a line
 *
 * Main trigger evaluation method
 * Plugin evaluation order (negative → world → positive)
 *
 * Called from StartNewLine() after a line is completed.
 *
 * Based on ProcessPreviousLine.cpp in original MUSHclient.
 * Evaluation order (suggested by Fiendish, added in v4.97):
 * 1. Plugins with negative sequence (< 0)
 * 2. World triggers
 * 3. Plugins with zero/positive sequence (>= 0)
 *
 * @param line The completed line to check triggers against
 */
void WorldDocument::evaluateTriggers(Line* line)
{
    // Sanity check
    if (!line || line->len() == 0) {
        return;
    }

    // Convert line text to QString (needed for callback and trigger matching)
    QString lineText = QString::fromUtf8(line->text(), line->len());

    // Call ON_PLUGIN_LINE_RECEIVED for all plugins
    // If any plugin returns false, stop processing this line
    if (!SendToAllPluginCallbacks(ON_PLUGIN_LINE_RECEIVED, lineText, true)) {
        return;
    }

    // Rebuild trigger array if needed (triggers added/deleted/sequence changed)
    if (m_triggersNeedSorting) {
        rebuildTriggerArray();
    }

    // Skip if triggers disabled
    if (!m_enable_triggers) {
        return;
    }

    // Track statistics
    m_iTriggersEvaluatedCount += m_TriggerArray.size();

    // Track one-shot trigger to delete (must defer until after iteration)
    QString oneShotToDelete;

    // Save current plugin context
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = nullptr;

    bool stopEvaluation = false;

    // ========== Phase 1: Evaluate plugins with NEGATIVE sequence ==========
    // These plugins get to see the line BEFORE the world triggers
    for (const auto& plugin : m_PluginList) {
        if (plugin->m_iSequence >= 0) {
            // Stop when we hit non-negative sequence (list should be sorted, but be safe)
            break;
        }

        if (!plugin->m_bEnabled) {
            continue;
        }

        // Rebuild plugin's trigger array if needed
        if (plugin->m_triggersNeedSorting) {
            plugin->rebuildTriggerArray();
        }

        // Set current plugin context
        m_CurrentPlugin = plugin.get();

        // Evaluate this plugin's triggers
        m_iTriggersEvaluatedCount += plugin->m_TriggerArray.size();
        stopEvaluation = evaluateOneTriggerSequence(plugin->m_TriggerArray, line, lineText, this,
                                                    oneShotToDelete);

        if (stopEvaluation) {
            // Delete one-shot from plugin
            if (!oneShotToDelete.isEmpty()) {
                plugin->m_TriggerMap.erase(oneShotToDelete);
                plugin->m_triggersNeedSorting = true;
            }
            m_CurrentPlugin = savedPlugin;
            return;
        }
    }

    // ========== Phase 2: Evaluate WORLD triggers ==========
    m_CurrentPlugin = nullptr;

    stopEvaluation =
        evaluateOneTriggerSequence(m_TriggerArray, line, lineText, this, oneShotToDelete);

    if (stopEvaluation) {
        // Delete one-shot from world
        if (!oneShotToDelete.isEmpty()) {
            qCDebug(lcWorld) << "Deleting one-shot world trigger:" << oneShotToDelete;
            deleteTrigger(oneShotToDelete);
        }
        m_CurrentPlugin = savedPlugin;
        return;
    }

    // ========== Phase 3: Evaluate plugins with ZERO/POSITIVE sequence ==========
    // These plugins get to see the line AFTER the world triggers
    for (const auto& plugin : m_PluginList) {
        if (plugin->m_iSequence < 0) {
            // Skip negative sequence (already processed)
            continue;
        }

        if (!plugin->m_bEnabled) {
            continue;
        }

        // Rebuild plugin's trigger array if needed
        if (plugin->m_triggersNeedSorting) {
            plugin->rebuildTriggerArray();
        }

        // Set current plugin context
        m_CurrentPlugin = plugin.get();

        // Evaluate this plugin's triggers
        m_iTriggersEvaluatedCount += plugin->m_TriggerArray.size();
        stopEvaluation = evaluateOneTriggerSequence(plugin->m_TriggerArray, line, lineText, this,
                                                    oneShotToDelete);

        if (stopEvaluation) {
            // Delete one-shot from plugin
            if (!oneShotToDelete.isEmpty()) {
                plugin->m_TriggerMap.erase(oneShotToDelete);
                plugin->m_triggersNeedSorting = true;
            }
            m_CurrentPlugin = savedPlugin;
            return;
        }
    }

    // Restore plugin context
    m_CurrentPlugin = savedPlugin;
}

/**
 * rebuildTriggerArray - Rebuild trigger array sorted by sequence
 *
 * Called when triggers are added/deleted or sequence numbers change.
 * Rebuilds m_TriggerArray from m_TriggerMap, sorted by iSequence.
 */
void WorldDocument::rebuildTriggerArray()
{
    m_TriggerArray.clear();

    // Add all triggers from map to array
    for (const auto& [name, triggerPtr] : m_TriggerMap) {
        m_TriggerArray.append(triggerPtr.get());
    }

    // Sort by sequence (lower = earlier)
    std::sort(m_TriggerArray.begin(), m_TriggerArray.end(),
              [](const Trigger* a, const Trigger* b) { return a->iSequence < b->iSequence; });

    m_triggersNeedSorting = false;
}
