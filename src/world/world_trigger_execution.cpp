/**
 * world_trigger_execution.cpp - Trigger Execution and Actions
 *
 * Implements what happens when a trigger matches:
 * - Wildcard replacement (%1, %2, etc.)
 * - Variable expansion (@var@)
 * - Send to various destinations (world, output, variable, etc.)
 * - Color changing
 * - Omit from output/log
 * - One-shot deletion
 * - Script execution (Lua callbacks)
 */

#include "../automation/plugin.h"
#include "../automation/sendto.h"
#include "../automation/trigger.h"
#include "../text/line.h"
#include "../text/style.h"
#include "script_engine.h"
#include "world_document.h"
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>

#include "logging.h"

// Lua headers for manual table creation
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

/**
 * replaceWildcards - Replace wildcard placeholders with captured text
 *
 * Replaces %0, %1, %2, ... %99 with the captured wildcards from pattern matching.
 * %0 is the entire match, %1 is first capture group, etc.
 *
 * @param text The text containing wildcard placeholders
 * @param wildcards Vector of captured wildcard strings
 * @return Text with wildcards replaced
 */
QString WorldDocument::replaceWildcards(const QString& text, const QVector<QString>& wildcards)
{
    QString result = text;

    // Replace %0, %1, %2, ... %99
    // Process in reverse order (99 down to 0) to avoid replacing %1 in %10
    for (int i = qMin(wildcards.size() - 1, 99); i >= 0; --i) {
        if (i < wildcards.size()) {
            QString placeholder = QString("%%1").arg(i);
            result.replace(placeholder, wildcards[i]);
        }
    }

    return result;
}

/**
 * changeLineColors - Change colors of matched line
 *
 * Modifies the style runs in the line to change colors based on trigger settings.
 * Handles:
 * - iOtherForeground: RGB foreground color
 * - iOtherBackground: RGB background color
 * - iColourChangeType: Which colors to change (both, foreground only, background only)
 *
 * **KNOWN LIMITATION :**
 * This implementation colors the ENTIRE line, not just the matched portion.
 *
 * Original MUSHclient (ProcessPreviousLine.cpp, 356 lines):
 * - Finds style runs containing matched text (columns iStartCol to iEndCol)
 * - Splits style runs at match boundaries (creates 3 runs: before, match, after)
 * - Colors only the matched portion
 * - Handles RGB, custom colors, and ANSI colors
 * - Updates both Line styles AND StyledLine (for scripts)
 * - Repeats for bRepeat triggers
 *
 * Our simplified implementation (36 lines):
 * - Changes ALL style runs in the entire line
 * - RGB colors only (trigger->colour not used)
 * - No style run splitting
 * - No StyledLine updating
 *
 * **Example of limitation:**
 * - Line: "You have 500 gold"
 * - Trigger: "* gold" with red foreground
 * - Expected: Only "500 gold" turns red
 * - Actual: Entire line "You have 500 gold" turns red
 *
 * **TODO:** Implement partial line coloring when we have:
 * - Column tracking (which columns matched)
 * - Style run splitting logic
 * - Display update integration
 *
 * This simplified implementation is adequate for many use cases where
 * users want to color entire lines (e.g., "Warning: *" → red line).
 *
 * @param trigger The trigger with color settings
 * @param line The line to recolor
 */
void WorldDocument::changeLineColors(Trigger* trigger, Line* line)
{
    // No color change requested
    if (trigger->iOtherForeground == 0 && trigger->iOtherBackground == 0) {
        return;
    }

    // Modify all style runs in the line
    // TODO: Only modify style runs in matched portion (iStartCol to iEndCol)
    for (const auto& style : line->styleList) {
        switch (trigger->iColourChangeType) {
            case TRIGGER_COLOUR_CHANGE_BOTH:
                if (trigger->iOtherForeground != 0) {
                    style->iForeColour = trigger->iOtherForeground;
                }
                if (trigger->iOtherBackground != 0) {
                    style->iBackColour = trigger->iOtherBackground;
                }
                break;

            case TRIGGER_COLOUR_CHANGE_FOREGROUND:
                if (trigger->iOtherForeground != 0) {
                    style->iForeColour = trigger->iOtherForeground;
                }
                break;

            case TRIGGER_COLOUR_CHANGE_BACKGROUND:
                if (trigger->iOtherBackground != 0) {
                    style->iBackColour = trigger->iOtherBackground;
                }
                break;
        }
    }

    // TODO: Update the display to show the color change
    // updateLine(line->m_nLineNumber);
}

/**
 * executeTrigger - Execute a trigger's action
 *
 * Called when a trigger matches. Performs all trigger actions:
 * - Updates statistics (nMatched, tWhenMatched)
 * - Replaces wildcards in contents
 * - Expands variables (if bExpandVariables)
 * - Copies wildcard to clipboard (if iClipboardArg set)
 * - Changes line colors
 * - Omits from output/log
 * - Executes action based on iSendTo
 * - Calls Lua script (if strProcedure set)
 * - Deletes trigger (if bOneShot)
 *
 * @param trigger The trigger that matched
 * @param line The matched line
 * @param matchedText The text that matched
 */
void WorldDocument::executeTrigger(Trigger* trigger, Line* line, const QString& matchedText)
{
    // Update statistics
    trigger->nMatched++;
    trigger->tWhenMatched = QDateTime::currentDateTime();

    // Prepare contents (send text)
    QString contents = trigger->contents;

    // Replace wildcards (%0, %1, %2, etc.)
    contents = replaceWildcards(contents, trigger->wildcards);

    // Expand variables (@variablename → value)
    if (trigger->bExpandVariables) {
        contents = expandVariables(contents);
    }

    // Copy wildcard to clipboard
    if (trigger->iClipboardArg > 0 && trigger->iClipboardArg <= trigger->wildcards.size()) {
        QString wildcard = trigger->wildcards[trigger->iClipboardArg - 1];
        QGuiApplication::clipboard()->setText(wildcard);
        qCDebug(lcWorld) << "Copied wildcard" << trigger->iClipboardArg
                         << "to clipboard:" << wildcard;
    }

    // Change line colors
    changeLineColors(trigger, line);

    // Omit from output
    if (trigger->bOmitFromOutput) {
        // TODO: Actually delete the line from m_lineList (like original does)
        // The original removes omitted lines but preserves notes/player input
        qCDebug(lcWorld) << "Trigger omit from output (not yet implemented)";
    }

    // Omit from log
    // Mark line to not be logged if trigger has omit_from_log set
    if (trigger->omit_from_log) {
        m_bOmitCurrentLineFromLog = true;
        qCDebug(lcWorld) << "Trigger omit from log: set m_bOmitCurrentLineFromLog flag";
    }

    // Play sound
    if (!trigger->sound_to_play.isEmpty()) {
        // Check bSoundIfInactive flag - only play if window is inactive, or flag is not set
        if (!trigger->bSoundIfInactive || !IsWindowActive()) {
            PlaySoundFile(trigger->sound_to_play);
            qCDebug(lcWorld) << "Trigger playing sound:" << trigger->sound_to_play;
        } else {
            qCDebug(lcWorld) << "Trigger sound skipped (window is active):"
                             << trigger->sound_to_play;
        }
    }

    // Execute action using central SendTo() function
    // Refactored to use sendTo() instead of inline switch statement
    QString strExtraOutput; // Accumulates eSendToOutput text
    QString triggerDescription =
        QString("Trigger: %1")
            .arg(trigger->strLabel.isEmpty() ? trigger->strInternalName : trigger->strLabel);

    sendTo(trigger->iSendTo, contents, trigger->bOmitFromOutput, trigger->omit_from_log,
           triggerDescription, trigger->strVariable, strExtraOutput);

    // Display any output that was accumulated
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Call Lua script if needed (sendTo handles eSendToScript, but for triggers
    // we also need to call executeTriggerScript for eSendToScriptAfterOmit)
    if (!trigger->strProcedure.isEmpty() &&
        (trigger->iSendTo == eSendToScript || trigger->iSendTo == eSendToScriptAfterOmit)) {
        executeTriggerScript(trigger, matchedText);
    }

    qCDebug(lcWorld) << "Trigger executed:" << trigger->strLabel << "matched:" << trigger->nMatched
                     << "times";
}

/**
 * executeTriggerScript - Execute Lua script callback for a trigger
 *
 * Trigger Script Execution
 * Based on CMUSHclientDoc::ExecuteTriggerScript() from doc.cpp
 *
 * Calls the Lua function specified in trigger->strProcedure with parameters:
 * 1. Trigger name (string)
 * 2. Matched line (string)
 * 3. Wildcards (table) - indexed 0..N where 0 is full match, 1+ are capture groups
 * 4. TriggerStyleRuns (table) - TODO styled line information
 *
 * @param trigger The trigger that matched
 * @param matchedText The text that matched the trigger
 */
void WorldDocument::executeTriggerScript(Trigger* trigger, const QString& matchedText)
{
    // Safety check - need a procedure name
    if (trigger->strProcedure.isEmpty()) {
        return;
    }

    // Determine which script engine to use:
    // - If trigger belongs to a plugin, use the plugin's script engine
    // - Otherwise, use the world's script engine
    ScriptEngine* engine = nullptr;
    if (trigger->owningPlugin && trigger->owningPlugin->m_ScriptEngine) {
        engine = trigger->owningPlugin->m_ScriptEngine.get();
    } else if (m_ScriptEngine) {
        engine = m_ScriptEngine.get();
    }

    // Safety check - need a script engine
    if (!engine || !engine->isLua()) {
        return;
    }

    // Check if function exists
    // dispid caching: DISPID_UNKNOWN = check needed, 1 = exists (cached)
    // We re-check each time it's DISPID_UNKNOWN (allows function to be defined later)
    if (trigger->dispid == DISPID_UNKNOWN) {
        trigger->dispid = engine->getLuaDispid(trigger->strProcedure);

        if (trigger->dispid == DISPID_UNKNOWN) {
            return; // Function doesn't exist, skip it
        }
    }

    // Get Lua state
    lua_State* L = engine->L;
    if (!L) {
        return;
    }

    // MUSHclient trigger callback signature:
    // function callback(trigger_name, matched_line, wildcards, style_runs)
    //
    // We need to call the Lua function directly (not via executeLua) because
    // wildcards needs to be passed as a table parameter, not just strings.

    // Parameter 1: Trigger name (use label if set, otherwise internal name)
    QString triggerName =
        trigger->strLabel.isEmpty() ? trigger->strInternalName : trigger->strLabel;

    // Save stack top for cleanup
    int stackTop = lua_gettop(L);

    // Create and set global wildcards table (for scripts that access it as a global)
    // This matches MUSHclient behavior where wildcards is available globally
    lua_newtable(L);
    for (int i = 0; i < trigger->wildcards.size(); ++i) {
        QString wildcard = trigger->wildcards[i];
        if (trigger->bLowercaseWildcard && i > 0) {
            wildcard = wildcard.toLower();
        }
        lua_pushinteger(L, i);
        QByteArray ba = wildcard.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_settable(L, -3);
    }
    // Add named capture groups to global wildcards table
    for (auto it = trigger->namedWildcards.constBegin(); it != trigger->namedWildcards.constEnd();
         ++it) {
        QString value = trigger->bLowercaseWildcard ? it.value().toLower() : it.value();
        QByteArray keyBytes = it.key().toUtf8();
        lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
        QByteArray valueBytes = value.toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_settable(L, -3);
    }
    lua_setglobal(L, "wildcards");

    // Push the function onto the stack
    lua_getglobal(L, trigger->strProcedure.toUtf8().constData());
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, stackTop);
        trigger->dispid = DISPID_UNKNOWN;
        return;
    }

    // Create and set global wildcards table (for scripts that access it as a global)
    // This matches MUSHclient behavior where wildcards is available both as a global
    // and as a function parameter
    lua_newtable(L);
    for (int i = 0; i < trigger->wildcards.size(); ++i) {
        QString wildcard = trigger->wildcards[i];
        if (trigger->bLowercaseWildcard && i > 0) {
            wildcard = wildcard.toLower();
        }
        lua_pushinteger(L, i);
        QByteArray ba = wildcard.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_settable(L, -3);
    }
    for (auto it = trigger->namedWildcards.constBegin(); it != trigger->namedWildcards.constEnd();
         ++it) {
        QString value = it.value();
        if (trigger->bLowercaseWildcard) {
            value = value.toLower();
        }
        QByteArray nameBytes = it.key().toUtf8();
        lua_pushlstring(L, nameBytes.constData(), nameBytes.length());
        QByteArray valueBytes = value.toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_settable(L, -3);
    }
    lua_setglobal(L, "wildcards");

    // Argument 1: Trigger name (string)
    QByteArray nameBytes = triggerName.toUtf8();
    lua_pushlstring(L, nameBytes.constData(), nameBytes.length());

    // Argument 2: Matched line (string)
    QByteArray lineBytes = matchedText.toUtf8();
    lua_pushlstring(L, lineBytes.constData(), lineBytes.length());

    // Argument 3: Wildcards table (also passed as function parameter)
    lua_newtable(L);

    // Add wildcards: wildcards[0] = full match, wildcards[1..N] = capture groups
    for (int i = 0; i < trigger->wildcards.size(); ++i) {
        QString wildcard = trigger->wildcards[i];

        // Apply lowercase conversion if requested (skip %0 - the whole match)
        if (trigger->bLowercaseWildcard && i > 0) {
            wildcard = wildcard.toLower();
        }

        // Push index (Lua uses 1-based indexing, but we keep 0-based for consistency with %0)
        lua_pushinteger(L, i);

        // Push value
        QByteArray ba = wildcard.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());

        // Set table[i] = wildcard
        lua_settable(L, -3);
    }

    // Add named capture groups: wildcards.name = value
    for (auto it = trigger->namedWildcards.constBegin(); it != trigger->namedWildcards.constEnd();
         ++it) {
        const QString& name = it.key();
        QString value = it.value();

        // Apply lowercase conversion if requested
        if (trigger->bLowercaseWildcard) {
            value = value.toLower();
        }

        // Push name as string key
        QByteArray keyBytes = name.toUtf8();
        lua_pushlstring(L, keyBytes.constData(), keyBytes.length());

        // Push value
        QByteArray valueBytes = value.toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());

        // Set table[name] = value
        lua_settable(L, -3);
    }

    // Argument 4: TriggerStyleRuns table (empty for now)
    lua_newtable(L);

    // Prevent deletion during script execution
    trigger->bExecutingScript = true;

    // Call the function with 4 arguments, 0 results
    int callResult = lua_pcall(L, 4, 0, 0);

    bool error = (callResult != 0);
    if (error) {
        // Get error message
        const char* errMsg = lua_tostring(L, -1);
        QString errorStr = errMsg ? QString::fromUtf8(errMsg) : "Unknown error";

        // Log the error
        QString strReason = QString("processing trigger \"%1\" when matching line: \"%2\"")
                                .arg(triggerName, matchedText);
        qWarning() << "=== Lua Error ===" << "\"Run-time error\"";
        qWarning() << "  Context:"
                   << QString("\"Function/Sub: %1 called by trigger\\nReason: %2\"")
                          .arg(trigger->strProcedure, strReason);
        qWarning() << "  Message:" << QString("\"%1\"").arg(errorStr);

        lua_pop(L, 1); // Pop error message
    }

    // Update invocation count
    trigger->nInvocationCount++;

    // Allow deletion again
    trigger->bExecutingScript = false;

    // If function failed, mark it as DISPID_UNKNOWN so we don't keep trying
    if (error) {
        trigger->dispid = DISPID_UNKNOWN;
        qDebug() << "TRIGGER SCRIPT ERROR:" << trigger->strProcedure;
    } else {
        // DEBUG: Log successful script execution for command_executed
        if (trigger->strProcedure == "command_executed") {
            qDebug() << "command_executed script ran successfully, invocations:"
                     << trigger->nInvocationCount;
        }
    }

    qCDebug(lcWorld) << "Trigger script executed:" << trigger->strProcedure
                     << "invocations:" << trigger->nInvocationCount;
}
