// world_sendto.cpp
// SendTo() - Central routing function for trigger/alias/timer actions
//
// Port of: doc.cpp (CMUSHclientDoc::SendTo)
//
// This function routes text to various destinations based on iWhere parameter.
// Used by triggers, aliases, and timers to perform their actions.
//
// Destinations:
// - eSendToWorld: Send to MUD
// - eSendToCommand: Put in command input field
// - eSendToOutput: Display in output window
// - eSendToStatus: Set status line message
// - eSendToNotepad: Create/append/replace notepad
// - eSendToLogFile: Write to log file
// - eSendToVariable: Set a variable
// - eSendToExecute: Re-parse as command (can trigger aliases)
// - eSendToSpeedwalk: Expand speedwalk and send to MUD
// - eSendToScript: Execute as script
// - eSendToCommandQueue: Add to command queue
// - eSendImmediate: Send immediately (bypass queue)
// - eSendToScriptAfterOmit: Execute script after omitting from output

#include "../automation/plugin.h"          // For plugin context detection
#include "../automation/script_language.h" // For ScriptLanguage enum
#include "../automation/sendto.h"
#include "logging.h"
#include "notepad_widget.h"
#include "script_engine.h"
#include "speedwalk_engine.h"
#include "view_interfaces.h"
#include "world_document.h"
#include <QDebug>

/**
 * SendTo - Route text to various destinations
 *
 * Port of: doc.cpp (CMUSHclientDoc::SendTo)
 *
 * Central routing function used by triggers, aliases, and timers.
 * Sends text to different destinations based on iWhere parameter.
 *
 * @param iWhere Where to send (eSendToWorld, eSendToOutput, etc.)
 * @param strSendText Text to send
 * @param omit_from_output If true, omit from output buffer
 * @param omit_from_log If true, omit from log file
 * @param strDescription Description for notepad/script (e.g., "trigger blah")
 * @param variable Variable name (for eSendToVariable)
 * @param strOutput [OUT] Accumulated output text (for eSendToOutput)
 */
void WorldDocument::sendTo(SendTo iWhere, const QString& strSendText, bool omit_from_output,
                           bool omit_from_log, const QString& strDescription,
                           const QString& variable, QString& strOutput, ScriptLanguage scriptLang)
{
    // Empty send text does nothing for most destinations
    // Original: doc.cpp
    if (iWhere != eSendToNotepad && iWhere != eAppendToNotepad && iWhere != eReplaceNotepad &&
        iWhere != eSendToOutput && iWhere != eSendToLogFile && iWhere != eSendToVariable) {
        if (strSendText.isEmpty()) {
            return;
        }
    }

    switch (iWhere) {
        // ========== eSendToWorld: Send to MUD ==========
        // Original: doc.cpp
        case eSendToWorld:
            if (!strSendText.isEmpty()) {
                // TODO(feature): Honor omit_from_output and omit_from_log flags from trigger/alias.
                sendToMud(strSendText);
            }
            break;

        // ========== eSendToCommand: Put in command input field ==========
        // Original: doc.cpp
        case eSendToCommand:
            if (m_pActiveInputView) {
                m_pActiveInputView->setInputText(strSendText);
            }
            break;

        // ========== eSendToCommandQueue: Add to command queue ==========
        // Original: doc.cpp
        case eSendToCommandQueue:
            // TODO(feature): Command queue not yet implemented — eSendToCommandQueue is a no-op.
            qCDebug(lcWorld) << "SendTo: eSendToCommandQueue:" << strSendText;
            break;

        // ========== eSendToStatus: Set status line message ==========
        // Original: doc.cpp
        case eSendToStatus:
            m_strStatusMessage = strSendText;
            m_tStatusDisplayed = QDateTime::currentDateTime();
            break;

        // ========== eSendToNotepad: Create new notepad window ==========
        // Original: doc.cpp
        case eSendToNotepad:
            CreateNotepadWindow(strDescription, strSendText);
            break;

        // ========== eAppendToNotepad: Append to existing notepad ==========
        // Original: doc.cpp
        case eAppendToNotepad: {
            NotepadWidget* notepad = FindNotepad(strDescription);
            if (notepad) {
                notepad->AppendText(strSendText);
            } else {
                CreateNotepadWindow(strDescription, strSendText);
            }
            break;
        }

        // ========== eReplaceNotepad: Replace notepad contents ==========
        // Original: doc.cpp
        case eReplaceNotepad: {
            NotepadWidget* notepad = FindNotepad(strDescription);
            if (notepad) {
                notepad->ReplaceText(strSendText);
            } else {
                CreateNotepadWindow(strDescription, strSendText);
            }
            break;
        }

        // ========== eSendToOutput: Display in output window ==========
        // Original: doc.cpp
        case eSendToOutput:
            // Append to strOutput parameter (caller will display it)
            strOutput += strSendText;
            if (!strSendText.endsWith("\r\n") && !strSendText.endsWith("\n")) {
                strOutput += "\r\n"; // Add newline if necessary
            }
            break;

        // ========== eSendToLogFile: Write to log file ==========
        // Original: doc.cpp
        case eSendToLogFile:
            // Write text to log file (if logging is enabled)
            // WriteToLog checks m_logfile internally and handles all formatting
            WriteToLog(strSendText);
            break;

        // ========== eSendToVariable: Set a variable ==========
        // Original: doc.cpp
        case eSendToVariable:
            // Set variable using the variable system
            // variable contains the variable name, strSendText contains the value
            if (!variable.isEmpty()) {
                setVariable(variable, strSendText);
            }
            break;

        // ========== eSendToExecute: Re-parse as command ==========
        // Original: doc.cpp
        case eSendToExecute:
            // Re-parse the text as a command, which can trigger:
            // - Alias matching and expansion
            // - Command stacking (multiple commands separated by delimiter)
            // - Speedwalk expansion
            // Note: omit_from_log flag not currently honored by Execute()
            Execute(strSendText);
            break;

        // ========== eSendToSpeedwalk: Expand speedwalk and send to MUD ==========
        // Original: doc.cpp
        case eSendToSpeedwalk: {
            QString expanded = speedwalk::evaluate(strSendText, m_speedwalk.filler);
            if (!expanded.isEmpty()) {
                sendToMud(expanded);
            }
            break;
        }

        // ========== eSendToScript: Execute as script ==========
        // Original: doc.cpp
        case eSendToScript:
            // Execute script code in appropriate Lua state
            // If we're in plugin context, use plugin's Lua state, otherwise use world's
            {
                ScriptEngine* scriptEngine =
                    m_CurrentPlugin ? m_CurrentPlugin->scriptEngine() : m_ScriptEngine.get();
                if (scriptEngine) {
                    // Set flag to prevent DeleteLines being used during script
                    // (Original sets m_bInSendToScript = true)
                    // Use parseScript() which handles YueScript transpilation if needed
                    scriptEngine->parseScript(strSendText, strDescription, scriptLang);
                } else {
                    note("\x1b[37;41mSend-to-script cannot execute because scripting is not "
                         "enabled.\x1b[0m");
                }
            }
            break;

        // ========== eSendImmediate: Send immediately (bypass queue) ==========
        // Original: doc.cpp
        case eSendImmediate:
            // Sends immediately, bypassing any future command queue — same as eSendToWorld for now.
            if (!strSendText.isEmpty()) {
                sendToMud(strSendText);
            }
            break;

        // ========== eSendToScriptAfterOmit: Execute script after omitting from output ==========
        // Original: doc.cpp
        case eSendToScriptAfterOmit:
            // Same as eSendToScript but called after omit processing
            // If we're in plugin context, use plugin's Lua state, otherwise use world's
            {
                ScriptEngine* scriptEngine =
                    m_CurrentPlugin ? m_CurrentPlugin->scriptEngine() : m_ScriptEngine.get();
                if (scriptEngine) {
                    // Use parseScript() which handles YueScript transpilation if needed
                    scriptEngine->parseScript(strSendText, strDescription, scriptLang);
                } else {
                    note("\x1b[37;41mSend-to-script cannot execute because scripting is not "
                         "enabled.\x1b[0m");
                }
            }
            break;

        default:
            qCDebug(lcWorld) << "SendTo: Unknown destination:" << iWhere;
            break;
    }
}
