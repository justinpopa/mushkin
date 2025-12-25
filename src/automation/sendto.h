// sendto.h
// SendTo Enum - Defines where trigger/alias/timer actions are routed

#ifndef SENDTO_H
#define SENDTO_H

#include <QString>

/**
 * SendTo - Destination for trigger/alias/timer actions
 *
 * Based on OtherTypes.h from original MUSHclient
 *
 * These values are stored in world files and used by Lua scripts,
 * so the numeric values must remain stable for backwards compatibility.
 */
enum SendTo {
    eSendToWorld = 0,       // 0: Send to MUD (network socket)
    eSendToCommand,         // 1: Put in command input field
    eSendToOutput,          // 2: Display in output window (note)
    eSendToStatus,          // 3: Put in status line
    eSendToNotepad,         // 4: Create new notepad window
    eAppendToNotepad,       // 5: Append to existing notepad
    eSendToLogFile,         // 6: Write to log file
    eReplaceNotepad,        // 7: Replace notepad contents
    eSendToCommandQueue,    // 8: Add to command queue
    eSendToVariable,        // 9: Set a variable
    eSendToExecute,         // 10: Re-parse as command (can trigger aliases)
    eSendToSpeedwalk,       // 11: Expand speedwalk and send to MUD
    eSendToScript,          // 12: Execute as Lua script
    eSendImmediate,         // 13: Send to MUD immediately (bypass queue)
    eSendToScriptAfterOmit, // 14: Execute script after omitting from output

// Optional destinations (compile-time)
#ifdef PANE
    eSendToPane, // 15: Send to pane window (if enabled)
#endif

    eSendToLast // Must be last (used for validation)
};

/** Convert SendTo enum to human-readable display name for UI */
inline QString sendToDisplayName(int sendTo)
{
    switch (sendTo) {
        case eSendToWorld:
            return QStringLiteral("World");
        case eSendToCommand:
            return QStringLiteral("Command");
        case eSendToOutput:
            return QStringLiteral("Output");
        case eSendToStatus:
            return QStringLiteral("Status");
        case eSendToNotepad:
            return QStringLiteral("Notepad");
        case eAppendToNotepad:
            return QStringLiteral("Append");
        case eSendToLogFile:
            return QStringLiteral("Log");
        case eReplaceNotepad:
            return QStringLiteral("Replace");
        case eSendToCommandQueue:
            return QStringLiteral("Queue");
        case eSendToVariable:
            return QStringLiteral("Variable");
        case eSendToExecute:
            return QStringLiteral("Execute");
        case eSendToSpeedwalk:
            return QStringLiteral("Speedwalk");
        case eSendToScript:
            return QStringLiteral("Script");
        case eSendImmediate:
            return QStringLiteral("Immediate");
        case eSendToScriptAfterOmit:
            return QStringLiteral("Script+Omit");
        default:
            return QString::number(sendTo);
    }
}

#endif // SENDTO_H
