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
#include "color_utils.h"
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
 * Replaces %0-%9 with the captured wildcards from pattern matching.
 * %0 is the entire match, %1 is first capture group, etc.
 * %% produces a literal percent sign.
 * %N (or %n) is the item name. %C (or %c) is the clipboard contents.
 * %<name> is replaced with named capture groups.
 *
 * Original: evaluate.cpp FixSendText / FixupEscapeSequences
 *
 * @param text The text containing wildcard placeholders
 * @param wildcards Vector of captured wildcard strings (max 10: %0-%9)
 * @param itemName Name of the trigger/alias/timer (substituted for %N/%n)
 * @param namedWildcards Named capture groups
 * @param sendTo Destination: if eSendToScript/eSendToScriptAfterOmit, escape \ and " in values
 * @return Text with wildcards replaced
 */
QString WorldDocument::replaceWildcards(const QString& text, const QVector<QString>& wildcards,
                                        const QString& itemName,
                                        const QMap<QString, QString>& namedWildcards, SendTo sendTo)
{
    // Build result character-by-character to handle %% and single-digit-only %0-%9
    // (original: evaluate.cpp:1244-1420, FixSendText)
    bool scriptDest = (sendTo == eSendToScript || sendTo == eSendToScriptAfterOmit);

    // Helper to escape wildcard values for Lua script consumption
    // (original: evaluate.cpp:264-287, FixWildcard)
    auto fixWildcard = [&](const QString& value) -> QString {
        if (!scriptDest) {
            return value;
        }
        // Escape backslashes first, then double-quotes
        QString escaped = value;
        escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
        return escaped;
    };

    QString result;
    result.reserve(text.size());

    const QChar* p = text.constData();
    while (!p->isNull()) {
        if (*p != '%') {
            result.append(*p++);
            continue;
        }

        // p points at '%'; peek at next char
        const QChar next = *(p + 1);

        if (next.isNull()) {
            // Trailing % — copy it literally (original: pText++ and break)
            result.append('%');
            p++;
            continue;
        }

        if (next == '%') {
            // %% → single literal %
            // (original: evaluate.cpp:1268-1278)
            result.append('%');
            p += 2;
            continue;
        }

        if (next >= '0' && next <= '9') {
            // %0-%9: single-digit wildcards only (original: MAX_WILDCARDS = 10)
            // (original: evaluate.cpp:1284-1322)
            int idx = next.digitValue();
            if (idx < wildcards.size()) {
                result.append(fixWildcard(wildcards[idx]));
            }
            // If index out of range, nothing is substituted (original behavior)
            p += 2;
            continue;
        }

        if (next == '<') {
            // %<name> — named capture group
            // (original: evaluate.cpp:1324-1367)
            const QChar* nameStart = p + 2;
            const QChar* q = nameStart;
            while (!q->isNull() && *q != '>') {
                ++q;
            }
            if (*q == '>') {
                QString name(nameStart, q - nameStart);
                auto it = namedWildcards.constFind(name);
                if (it != namedWildcards.constEnd()) {
                    result.append(fixWildcard(it.value()));
                }
                p = q + 1; // skip past '>'
            } else {
                // No closing '>' — copy '%' literally
                result.append('%');
                p++;
            }
            continue;
        }

        if (next == 'C' || next == 'c') {
            // %C or %c — clipboard contents (original: evaluate.cpp:1373-1392, case 'C': case 'c':)
            QClipboard* clipboard = QGuiApplication::clipboard();
            QString clipText = clipboard ? clipboard->text() : QString();
            result.append(fixWildcard(clipText));
            p += 2;
            continue;
        }

        if (next == 'N' || next == 'n') {
            // %N or %n — item name (original: evaluate.cpp:1398-1412, case 'N': case 'n':)
            result.append(fixWildcard(itemName));
            p += 2;
            continue;
        }

        // Unknown %X — copy percent literally, leave X for next iteration
        result.append('%');
        p++;
    }

    return result;
}

/**
 * changeLineColors - Change colors of matched line
 *
 * Modifies the style runs in the line to change colors based on trigger settings.
 * Handles:
 * - other_foreground: RGB foreground color
 * - other_background: RGB background color
 * - colour_change_type: Which colors to change (both, foreground only, background only)
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
 * - Repeats for repeat triggers
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
    if (trigger->other_foreground == 0 && trigger->other_background == 0) {
        return;
    }

    // Modify all style runs in the line
    // TODO(feature): Restrict color change to matched portion (iStartCol to iEndCol) instead of
    // full line.
    for (const auto& style : line->styleList) {
        switch (trigger->colour_change_type) {
            case ColourChangeType::Both:
                if (trigger->other_foreground != 0) {
                    style->iForeColour = trigger->other_foreground;
                }
                if (trigger->other_background != 0) {
                    style->iBackColour = trigger->other_background;
                }
                break;

            case ColourChangeType::Foreground:
                if (trigger->other_foreground != 0) {
                    style->iForeColour = trigger->other_foreground;
                }
                break;

            case ColourChangeType::Background:
                if (trigger->other_background != 0) {
                    style->iBackColour = trigger->other_background;
                }
                break;
        }
    }

    // TODO(feature): Request output view repaint after trigger color change is applied.
    // updateLine(line->m_nLineNumber);
}

/**
 * executeTrigger - Execute a trigger's action
 *
 * Called when a trigger matches. Performs all trigger actions:
 * - Updates statistics (matched, when_matched)
 * - Replaces wildcards in contents
 * - Expands variables (if expand_variables)
 * - Copies wildcard to clipboard (if clipboard_arg set)
 * - Changes line colors
 * - Omits from output/log
 * - Executes action based on send_to
 * - Calls Lua script (if procedure set)
 * - Deletes trigger (if one_shot)
 *
 * @param trigger The trigger that matched
 * @param line The matched line
 * @param matchedText The text that matched
 */
void WorldDocument::executeTrigger(Trigger* trigger, Line* line, const QString& matchedText)
{
    // Update statistics
    trigger->matched++;
    trigger->when_matched = QDateTime::currentDateTime();

    // Prepare contents (send text)
    QString contents = trigger->contents;

    // Replace wildcards (%0-%9, %N, %C, %<name>, %%) — pass send_to so script destinations get
    // FixWildcard escaping (original: evaluate.cpp FixSendText calls FixWildcard per wildcard)
    contents = replaceWildcards(contents, trigger->wildcards, trigger->label,
                                trigger->namedWildcards, trigger->send_to);

    // Expand variables (@variablename → value) without regex-escaping
    // (original: evaluate.cpp:472-482 passes bExpandVariables but never escapes the send-text path)
    if (trigger->expand_variables) {
        contents = expandVariables(contents, false);
    }

    // Copy wildcard to clipboard
    if (trigger->clipboard_arg > 0 && trigger->clipboard_arg <= trigger->wildcards.size()) {
        QString wildcard = trigger->wildcards[trigger->clipboard_arg - 1];
        QGuiApplication::clipboard()->setText(wildcard);
        qCDebug(lcWorld) << "Copied wildcard" << trigger->clipboard_arg
                         << "to clipboard:" << wildcard;
    }

    // Change line colors
    changeLineColors(trigger, line);

    // Omit from output (original: ProcessPreviousLine.cpp:1323-1324)
    // Sets flag checked by StartNewLine to skip adding line to output buffer
    if (trigger->omit_from_output && !trigger->multi_line) {
        m_bLineOmittedFromOutput = true;
    }

    // Omit from log — only for single-line triggers
    // (original: ProcessPreviousLine.cpp:1320-1321 checks !bMultiLine)
    if (trigger->omit_from_log && !trigger->multi_line) {
        m_bOmitCurrentLineFromLog = true;
        qCDebug(lcWorld) << "Trigger omit from log: set m_bOmitCurrentLineFromLog flag";
    }

    // Play sound (original: ProcessPreviousLine.cpp:985 checks m_enable_trigger_sounds)
    if (!trigger->sound_to_play.isEmpty() && m_sound.enable_trigger_sounds) {
        // Check sound_if_inactive flag - only play if window is inactive, or flag is not set
        if (!trigger->sound_if_inactive || !IsWindowActive()) {
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
            .arg(trigger->label.isEmpty() ? trigger->internal_name : trigger->label);

    // Set action source around sendTo so any callbacks see eTriggerFired
    // (original: ProcessPreviousLine.cpp:1046-1055 — sets before SendTo, restores after)
    trigger->executing_script = true;
    m_iCurrentActionSource = ActionSource::eTriggerFired;
    sendTo(trigger->send_to, contents, trigger->omit_from_output, trigger->omit_from_log,
           triggerDescription, trigger->variable, strExtraOutput, trigger->scriptLanguage);
    m_iCurrentActionSource = ActionSource::eUnknownActionSource;
    trigger->executing_script = false;

    // Display any output that was accumulated
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Call Lua script callback if the trigger has a procedure name, regardless of send_to
    // (original: ProcessPreviousLine.cpp:1326-1327 — adds to triggerList unconditionally
    // whenever strProcedure is non-empty; ExecuteTriggerScript called for all in triggerList)
    if (!trigger->procedure.isEmpty()) {
        executeTriggerScript(trigger, line, matchedText);
    }

    qCDebug(lcWorld) << "Trigger executed:" << trigger->label << "matched:" << trigger->matched
                     << "times";
}

/**
 * executeTriggerScript - Execute Lua script callback for a trigger
 *
 * Trigger Script Execution
 * Based on CMUSHclientDoc::ExecuteTriggerScript() from doc.cpp
 *
 * Calls the Lua function specified in trigger->procedure with parameters:
 * 1. Trigger name (string)
 * 2. Matched line (string)
 * 3. Wildcards (table) - indexed 0..N where 0 is full match, 1+ are capture groups
 * 4. TriggerStyleRuns (table) - styled line information (text, length, colours, style)
 *
 * @param trigger The trigger that matched
 * @param line The matched line (for style runs, may be nullptr)
 * @param matchedText The text that matched the trigger
 */
void WorldDocument::executeTriggerScript(Trigger* trigger, Line* line, const QString& matchedText)
{
    // Safety check - need a procedure name
    if (trigger->procedure.isEmpty()) {
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
        trigger->dispid = engine->getLuaDispid(trigger->procedure);

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
    QString triggerName = trigger->label.isEmpty() ? trigger->internal_name : trigger->label;

    // Save stack top for cleanup
    int stackTop = lua_gettop(L);

    // Push the function onto the stack — support dotted names like "utils.my_trigger"
    // (original: lua_scripting.cpp:490 uses GetNestedFunction)
    QByteArray procName = trigger->procedure.toUtf8();
    if (trigger->procedure.contains('.')) {
        // Dotted name: split and traverse tables
        QStringList parts = trigger->procedure.split('.');
        lua_getglobal(L, parts[0].toUtf8().constData());
        for (int i = 1; i < parts.size(); ++i) {
            if (!lua_istable(L, -1)) {
                lua_settop(L, stackTop);
                trigger->dispid = DISPID_UNKNOWN;
                return;
            }
            lua_getfield(L, -1, parts[i].toUtf8().constData());
            lua_remove(L, -2); // remove parent table
        }
    } else {
        lua_getglobal(L, procName.constData());
    }
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
        if (trigger->lowercase_wildcard && i > 0) {
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
        if (trigger->lowercase_wildcard) {
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
        if (trigger->lowercase_wildcard && i > 0) {
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
        if (trigger->lowercase_wildcard) {
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

    // Argument 4: TriggerStyleRuns table
    // Build style runs from the matched line (original: lua_scripting.cpp:599-619)
    // Each entry is a sub-table with: text, length, textcolour, backcolour, style
    lua_newtable(L);
    if (line) {
        int styleIndex = 1;
        int textOffset = 0;
        for (const auto& style : line->styleList) {
            lua_newtable(L);

            // Extract text for this style run
            int runLen = style->iLength;
            QString runText;
            if (textOffset + runLen <= line->len()) {
                runText = QString::fromUtf8(line->textBuffer.data() + textOffset, runLen);
            }
            textOffset += runLen;

            // "text" = the text content of this style run
            QByteArray textBytes = runText.toUtf8();
            lua_pushlstring(L, textBytes.constData(), textBytes.length());
            lua_setfield(L, -2, "text");

            // "length" = length of the text
            lua_pushinteger(L, runText.length());
            lua_setfield(L, -2, "length");

            // "textcolour" / "backcolour" = resolved RGB (not raw ANSI index)
            // (original: ProcessPreviousLine.cpp:222-228 calls GetStyleRGB before CPaneStyle)
            QRgb cText = 0;
            QRgb cBack = 0;
            GetStyleRGB(style.get(), cText, cBack);
            lua_pushinteger(L, static_cast<lua_Integer>(cText & 0x00FFFFFF));
            lua_setfield(L, -2, "textcolour");
            lua_pushinteger(L, static_cast<lua_Integer>(cBack & 0x00FFFFFF));
            lua_setfield(L, -2, "backcolour");

            // "style" = style flags masked to HILITE|UNDERLINE|BLINK only (original: iFlags & 7)
            // (original: ProcessPreviousLine.cpp:228 — "pStyle->iFlags & 7")
            lua_pushinteger(L, style->iFlags & 7);
            lua_setfield(L, -2, "style");

            lua_rawseti(L, -2, styleIndex++);
        }
    }

    // M185: Save and reset note style before callback, restore after
    // (original: lua_scripting.cpp:487-488 saves iOldStyle and sets to NORMAL)
    quint16 oldNoteStyle = m_iNoteStyle;
    m_iNoteStyle = 0; // NORMAL

    // Prevent deletion during script execution (original: doc.cpp:2571-2583)
    trigger->executing_script = true;

    // Set action source for the duration of the Lua callback
    // (original: lua_scripting.cpp:621-627 — sets iReason before CallLuaWithTraceBack,
    // restores to eUnknownActionSource immediately after)
    m_iCurrentActionSource = ActionSource::eTriggerFired;

    // Call with traceback error handler for readable stack traces
    // (original: lua_scripting.cpp:624 uses CallLuaWithTraceBack)
    int callResult = callLuaWithTraceBack(L, 4, 0);

    m_iCurrentActionSource = ActionSource::eUnknownActionSource;

    // Allow deletion again
    trigger->executing_script = false;

    // M185: Restore note style after callback
    m_iNoteStyle = oldNoteStyle;

    bool error = (callResult != 0);
    if (error) {
        // Get error message
        const char* errMsg = lua_tostring(L, -1);
        QString errorStr = errMsg ? QString::fromUtf8(errMsg) : "Unknown error";

        // Display error in output window matching original LuaError() output format
        // (original: lua_scripting.cpp:394-397 — strEvent, m_strRaisedBy, m_strCalledBy,
        // m_strDescription all sent to ColourNote with SCRIPTERRORFORECOLOUR = "orangered")
        constexpr QRgb orangeRed = BGR(255, 69, 0); // SCRIPTERRORFORECOLOUR
        constexpr QRgb black = BGR(0, 0, 0);        // SCRIPTERRORBACKCOLOUR

        // Line 1: event type ("Run-time error" — original: strEvent)
        colourNote(orangeRed, black, QString("Run-time error in Lua"));

        // Line 2: raised-by — "World: name" or "Plugin: name (called from world: ...)"
        // (original: lua_scripting.cpp:349-359 — dlg.m_strRaisedBy)
        QString strRaisedBy;
        if (m_CurrentPlugin) {
            strRaisedBy = QString("Plugin: %1 (called from world: %2)")
                              .arg(m_CurrentPlugin->m_strName, m_mush_name);
        } else {
            strRaisedBy = QString("World: %1").arg(m_mush_name);
        }
        colourNote(orangeRed, black, strRaisedBy);

        // Line 3: called-by context ("processing trigger X when matching line Y")
        // (original: lua_scripting.cpp:396 — dlg.m_strCalledBy)
        QString strReason = QString("processing trigger \"%1\" when matching line: \"%2\"")
                                .arg(triggerName, matchedText);
        colourNote(orangeRed, black, strReason);

        // Line 4: error description
        // (original: lua_scripting.cpp:397 — dlg.m_strDescription)
        colourNote(orangeRed, black, errorStr);

        lua_pop(L, 1); // Pop error message
    }

    // If function failed, mark it as DISPID_UNKNOWN so we don't keep trying
    if (error) {
        trigger->dispid = DISPID_UNKNOWN;
        qCDebug(lcWorld) << "Trigger script error:" << trigger->procedure;
    } else {
        // M186: Only increment invocation_count on success
        // (original: lua_scripting.cpp:736 increments after error check)
        trigger->invocation_count++;
    }

    qCDebug(lcWorld) << "Trigger script executed:" << trigger->procedure
                     << "invocations:" << trigger->invocation_count;
}
