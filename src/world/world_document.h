#ifndef WORLD_DOCUMENT_H
#define WORLD_DOCUMENT_H

#include <QColor>
#include <QDateTime>
#include <QFile>              // Log file management
#include <QFileSystemWatcher> // Script file change monitoring
#include <QJsonDocument>      // GMCP JSON parsing
#include <QMap>               // TriggerMap, AliasMap
#include <QObject>
#include <QPoint>
#include <QPointer> // For QPointer<NotepadWidget> (null-safe observer)
#include <QRect>
#include <QRgb>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>        // Timer evaluation loop
#include <QVector>       // TriggerArray, AliasArray
#include <array>         // For std::array (fixed-size collections)
#include <deque>         // For m_recentLines (multi-line triggers)
#include <expected>      // For std::expected (fallible operations)
#include <functional>    // For std::function (progress callback)
#include <memory>        // For std::unique_ptr
#include <span>          // For std::span (buffer parameters)
#include <unordered_map> // For std::unordered_map (option snapshots)
#include <vector>        // For std::vector (line buffer)

#include "automation_registry.h" // AutomationRegistry companion object (owns automation storage)
#include "connection_manager.h"  // ConnectionManager companion object (owns connection state)
#include "mxp_engine.h"          // MXPEngine companion object (owns MXP protocol state)
#include "sound_manager.h"       // SoundManager companion object (owns spatial audio state)
#include "telnet_parser.h"       // TelnetParser, Phase enum, telnet constants
#include "world_error.h"         // WorldError, WorldErrorType

#include "../automation/script_language.h" // ScriptLanguage enum
#include "../automation/sendto.h"          // SendTo enum
#include "../automation/variable.h"        // ArraysMap type
#include "../text/style.h" // Style flag bits (HILITE, UNDERLINE, BLINK, INVERSE, STRIKEOUT, COLOUR_*, COLOURTYPE, ACTIONTYPE, STYLE_BITS)
#include "miniwindow.h"    // MiniWindow (off-screen drawing surface)
#include "mxp_types.h"     // MXP data structures (still used by some callers)
#include <QJsonArray>      // GMCP JSON parsing
#include <QJsonObject>     // GMCP JSON parsing
#include <QJsonValue>      // GMCP JSON parsing
#include <zlib.h>          // For MCCP compression

// Forward declarations
class WorldSocket;
class Line;
class Style;
class Action;
class ScriptEngine;
class Trigger;            //
class Alias;              //
class Timer;              //
class Plugin;             //
class Variable;           //
class IOutputView;        // Interface for output view (see view_interfaces.h)
class IInputView;         // Interface for input view (see view_interfaces.h)
class NotepadWidget;      // Notepad window
class RemoteAccessServer; // Remote access server for telnet connections
class AcceleratorManager; // Keyboard accelerator management

// SQLite forward declarations (for Lua database functions)
struct sqlite3;
struct sqlite3_stmt;


// Constants
inline constexpr int MAX_CUSTOM = 16;       // maximum custom colours
inline constexpr int MACRO_COUNT = 12;      // F1-F12 function keys
inline constexpr int KEYPAD_MAX_ITEMS = 30; // keypad items

// xterm 256-color palette (defined in world_protocol.cpp)
extern QRgb xterm_256_colours[256];

// Type definitions
typedef std::map<QString, std::unique_ptr<Variable>> VariableMap;

// Database structure (for Lua DatabaseOpen/DatabasePrepare/DatabaseStep functions)
// Based on tDatabase struct from methods_database.cpp
struct LuaDatabase {
    sqlite3* db;         // SQLite database handle
    sqlite3_stmt* pStmt; // Prepared statement (nullptr if none)
    bool bValidRow;      // Whether last Step() returned a valid row
    QString db_name;     // Filename/path of the database
    qint32 iColumns;     // Number of columns in prepared statement

    LuaDatabase() : db(nullptr), pStmt(nullptr), bValidRow(false), iColumns(0)
    {
    }

    // Non-copyable, non-movable: owns sqlite3* handle; destructor calls sqlite3_close.
    // Stored exclusively via std::unique_ptr<LuaDatabase> in m_DatabaseMap.
    LuaDatabase(const LuaDatabase&) = delete;
    LuaDatabase& operator=(const LuaDatabase&) = delete;
    LuaDatabase(LuaDatabase&&) = delete;
    LuaDatabase& operator=(LuaDatabase&&) = delete;

    // Defined out-of-line in world_database.cpp (requires complete sqlite3.h type)
    ~LuaDatabase();
};

// Database error codes (negative to avoid conflicting with SQLite error codes)
// Based on methods_database.cpp
enum class DatabaseError : qint32 {
    IdNotFound = -1,            // Unknown database ID
    NotOpen = -2,               // Database not opened
    HavePreparedStatement = -3, // Already have a prepared statement
    NoPreparedStatement = -4,   // No prepared statement yet
    NoValidRow = -5,            // Have not stepped to valid row
    DatabaseAlreadyExists = -6, // Already have a database of that disk name
    ColumnOutOfRange = -7,      // Requested column out of range
};

[[nodiscard]] inline constexpr auto to_underlying(DatabaseError e) noexcept
{
    return static_cast<std::underlying_type_t<DatabaseError>>(e);
}

// Flag bits for m_iFlags1
inline constexpr quint16 FLAGS1_ArrowRecallsPartial = 0x0001;
inline constexpr quint16 FLAGS1_CtrlZGoesToEndOfBuffer = 0x0002;
inline constexpr quint16 FLAGS1_CtrlPGoesToPreviousCommand = 0x0004;
inline constexpr quint16 FLAGS1_CtrlNGoesToNextCommand = 0x0008;
inline constexpr quint16 FLAGS1_HyperlinkAddsToCommandHistory = 0x0010;
inline constexpr quint16 FLAGS1_EchoHyperlinkInOutputWindow = 0x0020;
inline constexpr quint16 FLAGS1_AutoWrapWindowWidth = 0x0040;
inline constexpr quint16 FLAGS1_NAWS = 0x0080;
inline constexpr quint16 FLAGS1_Pueblo = 0x0100;
inline constexpr quint16 FLAGS1_NoEchoOff = 0x0200;
inline constexpr quint16 FLAGS1_UseCustomLinkColour = 0x0400;
inline constexpr quint16 FLAGS1_MudCanChangeLinkColour = 0x0800;
inline constexpr quint16 FLAGS1_UnderlineHyperlinks = 0x1000;
inline constexpr quint16 FLAGS1_MudCanRemoveUnderline = 0x2000;

// Flag bits for m_iFlags2
inline constexpr quint16 FLAGS2_AlternativeInverse = 0x0001;
inline constexpr quint16 FLAGS2_ShowConnectDisconnect = 0x0002;
inline constexpr quint16 FLAGS2_IgnoreMXPcolourChanges = 0x0004;
inline constexpr quint16 FLAGS2_Custom16isDefaultColour = 0x0008;
inline constexpr quint16 FLAGS2_LogInColour = 0x0010;
inline constexpr quint16 FLAGS2_LogRaw = 0x0020;

// Auto-connect values
enum class AutoConnect : int { eNoAutoConnect, eConnectMUSH, eConnectAndGoIntoGame };

// MXP usage values
enum class MXPMode : quint16 { eMXP_Off, eMXP_Query, eMXP_On };

// Connection phase values (doc.h)
// These are now inline constexpr aliases to connection_manager.h constants.
// Kept here for backward compatibility with all callers in world_*.cpp and lua_api/*.cpp.
inline constexpr qint32 eConnectNotConnected = CONNECT_NOT_CONNECTED;
inline constexpr qint32 eConnectMudNameLookup = CONNECT_MUD_NAME_LOOKUP;
inline constexpr qint32 eConnectConnectingToMud = CONNECT_CONNECTING_TO_MUD;
inline constexpr qint32 eConnectConnectedToMud = CONNECT_CONNECTED_TO_MUD;
inline constexpr qint32 eConnectDisconnecting = CONNECT_DISCONNECTING;

// Action source values (Lua callbacks)
// These tell scripts what triggered the current code execution
// Based on doc.h action source enum
enum class ActionSource : quint32 {
    eUnknownActionSource = 0, // Unknown/not set
    eUserAction,              // User typed a command
    eWorldAction,             // World event (connect, disconnect, open, close)
    eTriggerAction,           // Trigger fired
    eAliasAction,             // Alias matched
    eTimerAction,             // Timer fired
    ePluginAction,            // Plugin called function
    eLuaSandbox,              // Sandbox initialization
    eDontChangeAction = 9999  // Special: don't change m_iCurrentActionSource
};

// Command history position status (command history navigation)
// Based on sendvw.h in original MUSHclient
enum class HistoryStatus : int {
    eAtTop,    // At the top (oldest command) of history
    eInMiddle, // Somewhere in the middle of history
    eAtBottom  // At the bottom (newest, or ready for new command)
};

// Script reload option values (for m_nReloadOption)
// Based on doc.h script reload enum
enum class ScriptReloadOption : qint32 {
    eReloadConfirm = 0, // Show dialog asking user when script file changes
    eReloadAlways = 1,  // Automatically reload script when file changes
    eReloadNever = 2    // Ignore script file changes
};

// ========== Telnet Protocol Constants (RFC 854) ==========
// inline constexpr aliases to the values in telnet_parser.h (TELNET_* / TELOPT_*_OPT).
// Kept here for backward compatibility with callers in world_protocol.cpp, etc.
inline constexpr auto IAC = TELNET_IAC;
inline constexpr auto DONT = TELNET_DONT;
inline constexpr auto DO = TELNET_DO;
inline constexpr auto WONT = TELNET_WONT;
inline constexpr auto WILL = TELNET_WILL;
inline constexpr auto SB = TELNET_SB;
inline constexpr auto GO_AHEAD = TELNET_GO_AHEAD;
inline constexpr auto ERASE_LINE = TELNET_ERASE_LINE;
inline constexpr auto ERASE_CHARACTER = TELNET_ERASE_CHARACTER;
inline constexpr auto ARE_YOU_THERE = TELNET_ARE_YOU_THERE;
inline constexpr auto ABORT_OUTPUT = TELNET_ABORT_OUTPUT;
inline constexpr auto INTERRUPT_PROCESS = TELNET_INTERRUPT_PROCESS;
inline constexpr auto BREAK = TELNET_BREAK;
inline constexpr auto DATA_MARK = TELNET_DATA_MARK;
inline constexpr auto NOP = TELNET_NOP;
inline constexpr auto SE = TELNET_SE;
inline constexpr auto EOR = TELNET_EOR;

inline constexpr auto TELOPT_ECHO = TELOPT_ECHO_OPT;
inline constexpr auto TELOPT_SGA = TELOPT_SGA_OPT;
inline constexpr auto TELOPT_TERMINAL_TYPE = TELOPT_TERMINAL_TYPE_OPT;
inline constexpr auto TELOPT_NAWS = TELOPT_NAWS_OPT;
inline constexpr auto TELOPT_CHARSET = TELOPT_CHARSET_OPT;
inline constexpr auto TELOPT_COMPRESS = TELOPT_COMPRESS_OPT;
inline constexpr auto TELOPT_COMPRESS2 = TELOPT_COMPRESS2_OPT;
inline constexpr auto TELOPT_MSP = TELOPT_MSP_OPT;
inline constexpr auto TELOPT_MXP = TELOPT_MXP_OPT;
inline constexpr auto TELOPT_ZMP = TELOPT_ZMP_OPT;
inline constexpr auto TELOPT_MUD_SPECIFIC = TELOPT_MUD_SPECIFIC_OPT;
inline constexpr auto TELOPT_ATCP = TELOPT_ATCP_OPT;
inline constexpr auto TELOPT_GMCP = TELOPT_GMCP_OPT;

inline constexpr auto WILL_END_OF_RECORD = TELNET_WILL_END_OF_RECORD;
inline constexpr auto COMPRESS_BUFFER_LENGTH = TELNET_COMPRESS_BUFFER_LENGTH;

// MXP line security modes (ESC[<n>z) - doc.h
// inline constexpr aliases to the values in mxp_engine.h (MXP_MODE_*).
// Kept here for backward compatibility with world_protocol.cpp, world_mxp.cpp, telnet_parser.cpp.
// IMPORTANT: These values must match the ANSI escape code values!
inline constexpr auto eMXP_open = MXP_MODE_OPEN;
inline constexpr auto eMXP_secure = MXP_MODE_SECURE;
inline constexpr auto eMXP_locked = MXP_MODE_LOCKED;
inline constexpr auto eMXP_reset = MXP_MODE_RESET;
inline constexpr auto eMXP_secure_once = MXP_MODE_SECURE_ONCE;
inline constexpr auto eMXP_perm_open = MXP_MODE_PERM_OPEN;
inline constexpr auto eMXP_perm_secure = MXP_MODE_PERM_SECURE;
inline constexpr auto eMXP_perm_locked = MXP_MODE_PERM_LOCKED;
// Room and welcome modes (10-19)
inline constexpr auto eMXP_room_name = MXP_MODE_ROOM_NAME;
inline constexpr auto eMXP_room_description = MXP_MODE_ROOM_DESCRIPTION;
inline constexpr auto eMXP_room_exits = MXP_MODE_ROOM_EXITS;
inline constexpr auto eMXP_welcome = MXP_MODE_WELCOME;

// ========== Speedwalk Direction Mapping ==========

/**
 * DirectionInfo - Information about a speedwalk direction
 *
 * Based on original MUSHclient MapDirectionsMap in mapping.cpp
 */
struct DirectionInfo {
    QString m_sDirectionToSend;  // Command to send (e.g., "north")
    QString m_sReverseDirection; // Reverse direction (e.g., for "n" it's "s")

    DirectionInfo(const QString& toSend = "", const QString& reverse = "")
        : m_sDirectionToSend(toSend), m_sReverseDirection(reverse)
    {
    }
};


// ========== ANSI Color/Style Constants (stdafx.h) ==========
// These are ANSI escape code parameter values (the numbers after ESC[), not bit flags.

// ANSI formatting codes
inline constexpr int ANSI_RESET = 0;
inline constexpr int ANSI_BOLD = 1;
inline constexpr int ANSI_BLINK = 3;
inline constexpr int ANSI_UNDERLINE = 4;
inline constexpr int ANSI_SLOW_BLINK = 5;
inline constexpr int ANSI_FAST_BLINK = 6;
inline constexpr int ANSI_INVERSE = 7;
inline constexpr int ANSI_STRIKEOUT = 9;

// Cancel codes (new in MUSHclient 3.27)
inline constexpr int ANSI_CANCEL_BOLD = 22;
inline constexpr int ANSI_CANCEL_BLINK = 23;
inline constexpr int ANSI_CANCEL_UNDERLINE = 24;
inline constexpr int ANSI_CANCEL_SLOW_BLINK = 25;
inline constexpr int ANSI_CANCEL_INVERSE = 27;
inline constexpr int ANSI_CANCEL_STRIKEOUT = 29;

// Foreground colors (30-37)
inline constexpr int ANSI_TEXT_BLACK = 30;
inline constexpr int ANSI_TEXT_RED = 31;
inline constexpr int ANSI_TEXT_GREEN = 32;
inline constexpr int ANSI_TEXT_YELLOW = 33;
inline constexpr int ANSI_TEXT_BLUE = 34;
inline constexpr int ANSI_TEXT_MAGENTA = 35;
inline constexpr int ANSI_TEXT_CYAN = 36;
inline constexpr int ANSI_TEXT_WHITE = 37;
inline constexpr int ANSI_TEXT_256_COLOUR = 38; // Extended colors (256-color, 24-bit)
inline constexpr int ANSI_SET_FOREGROUND_DEFAULT = 39;

// Background colors (40-47)
inline constexpr int ANSI_BACK_BLACK = 40;
inline constexpr int ANSI_BACK_RED = 41;
inline constexpr int ANSI_BACK_GREEN = 42;
inline constexpr int ANSI_BACK_YELLOW = 43;
inline constexpr int ANSI_BACK_BLUE = 44;
inline constexpr int ANSI_BACK_MAGENTA = 45;
inline constexpr int ANSI_BACK_CYAN = 46;
inline constexpr int ANSI_BACK_WHITE = 47;
inline constexpr int ANSI_BACK_256_COLOUR = 48; // Extended colors (256-color, 24-bit)
inline constexpr int ANSI_SET_BACKGROUND_DEFAULT = 49;

// ========== Style Flag Bits (OtherTypes.h) ==========
// Defined canonically in src/text/style.h (included above).
// HILITE, UNDERLINE, BLINK, INVERSE, STRIKEOUT, COLOUR_*, COLOURTYPE, ACTIONTYPE, STYLE_BITS
// are all available here via the style.h include.

// ========== Color Constants (stdafx.h) ==========

// ANSI color indices (0-7)
// Note: Use ANSI_ prefix to avoid collision with style.h macros (BLACK, WHITE)
enum {
    ANSI_BLACK = 0,
    ANSI_RED,
    ANSI_GREEN,
    ANSI_YELLOW,
    ANSI_BLUE,
    ANSI_MAGENTA,
    ANSI_CYAN,
    ANSI_WHITE
};

// Telnet Phase State Machine — defined in telnet_parser.h (included above).
// The Phase enum and all phase constants (NONE, HAVE_ESC, DOING_CODE, …) are available
// here because telnet_parser.h is included at the top of this file.

/**
 * WorldDocument - The core document class for a MUD connection
 *
 * This is a direct port of CMUSHclientDoc from the original MFC MUSHclient.
 * It holds ALL state for one world/MUD connection including:
 * - Connection settings (server, port, credentials)
 * - Display preferences (colors, fonts)
 * - Network state
 * - Text buffer
 * - Triggers, aliases, timers
 * - Lua scripting state
 *
 * Source: doc.h (original MUSHclient) in original MUSHclient
 *
 * IMPORTANT: This is intentionally kept as a FLAT structure for initial port.
 * Don't try to organize into sub-objects yet.
 */

class WorldDocument : public QObject {
    Q_OBJECT

  public:
    explicit WorldDocument(QObject* parent = nullptr);
    ~WorldDocument() override;

    // Public member variables (for direct port compatibility)

    // NOTE: m_pSocket is now stored inside m_connectionManager->m_pSocket.
    // For backward-compat with callers that access it directly, use the wrapper below.
    // The socket is still created with `new WorldSocket(this, this)` (Qt parent-child).
    std::unique_ptr<RemoteAccessServer> m_pRemoteServer; // Remote access server (runtime only)

    // ========== ConnectionManager (companion object) ==========
    // Owns connection state, network statistics, timing, and the command queue.
    // Created in WorldDocument constructor. Access via m_connectionManager->.
    //
    // Fields that previously lived directly on WorldDocument and are now in ConnectionManager:
    //   m_pSocket, m_iConnectPhase,
    //   m_nBytesIn, m_nBytesOut, m_iInputPacketCount, m_iOutputPacketCount,
    //   m_nTotalLinesSent, m_nTotalLinesReceived,
    //   m_tConnectTime, m_tsConnectDuration, m_whenWorldStarted,
    //   m_whenWorldStartedHighPrecision, m_CommandQueue.
    // All access via m_connectionManager->fieldName or m_connectionManager->method().
    std::unique_ptr<ConnectionManager> m_connectionManager;
    // NOTE: m_pSocket is now at m_connectionManager->m_pSocket.
    // Direct access to the socket in WorldDocument member functions goes through
    // m_connectionManager->m_pSocket (see world_document.cpp, world_protocol.cpp).

    // ========== Connection Settings ==========
    QString m_server;    // hostname or IP address
    QString m_mush_name; // world name
    QString m_name;      // player name
    QString m_password;  // player password
    quint16 m_port;      // port number (1-65535)
    bool m_connect_now;  // auto-connect flag (see enum above)

    // ========== Display Settings ==========
    QString m_font_name;    // output font face name
    qint32 m_font_height;   // font size in pixels
    qint32 m_font_weight;   // bold/normal (400=normal, 700=bold)
    quint32 m_font_charset; // character set
    bool m_wrap;            // word-wrap enabled
    bool m_timestamps;      // show timestamps?
    bool m_match_width;     // match width?

    // ========== Colors ==========
    // ANSI colors 0-7 normal intensity
    std::array<QRgb, 8> m_normalcolour;
    // ANSI colors 0-7 bold/bright
    std::array<QRgb, 8> m_boldcolour;
    // Custom foreground colors
    std::array<QRgb, MAX_CUSTOM> m_customtext;
    // Custom background colors
    std::array<QRgb, MAX_CUSTOM> m_customback;
    // Custom color names
    std::array<QString, 255> m_strCustomColourName;

    // ========== Input Colors and Font ==========
    QRgb m_input_text_colour;
    QRgb m_input_background_colour;
    qint32 m_input_font_height;
    QString m_input_font_name;
    quint8 m_input_font_italic;
    qint32 m_input_font_weight;
    quint32 m_input_font_charset;

    // ========== Output Buffer Settings ==========
    qint32 m_maxlines;      // maximum scrollback lines
    qint32 m_nHistoryLines; // command history size
    quint16 m_nWrapColumn;  // wrap column

    // ========== Triggers, Aliases, Timers - Enable Flags ==========
    bool m_enable_aliases;
    bool m_enable_triggers;
    bool m_bEnableTimers;

    // ========== AutomationRegistry (companion object) ==========
    // Owns all trigger/alias/timer storage and evaluation pipeline.
    // Created in WorldDocument constructor. Access via m_automationRegistry->.
    //
    // Fields that previously lived directly on WorldDocument and are now in AutomationRegistry:
    //   m_TriggerMap, m_TriggerArray, m_triggersNeedSorting,
    //   m_AliasMap, m_AliasArray, m_aliasesNeedSorting,
    //   m_TimerMap, m_TimerRevMap,
    //   m_iTriggersEvaluatedCount, m_iTriggersMatchedCount,
    //   m_iAliasesEvaluatedCount, m_iAliasesMatchedCount,
    //   m_iTimersFiredCount, m_iTriggersMatchedThisSessionCount,
    //   m_iAliasesMatchedThisSessionCount, m_iTimersFiredThisSessionCount.
    // All access via m_automationRegistry->fieldName or m_automationRegistry->method().
    //
    // For backward-compat with callers that access the maps directly, use:
    //   m_automationRegistry->m_TriggerMap  (instead of m_TriggerMap)
    //   m_automationRegistry->m_AliasMap    (instead of m_AliasMap)
    //   m_automationRegistry->m_TimerMap    (instead of m_TimerMap)
    //   m_automationRegistry->m_TimerRevMap (instead of m_TimerRevMap)
    std::unique_ptr<AutomationRegistry> m_automationRegistry;

    // ========== Input Handling ==========
    bool m_display_my_input;
    quint16 m_echo_colour;
    bool m_bEscapeDeletesInput;
    bool m_bArrowsChangeHistory;
    bool m_bConfirmOnPaste;

    // ========== Command History ==========
    QStringList m_commandHistory;   // List of previous commands
    qint32 m_maxCommandHistory;     // Max history size (default 20)
    qint32 m_historyPosition;       // Current position when browsing
    bool m_bFilterDuplicates;       // Remove duplicate commands from history
    QString m_last_command;         // Last command sent (for consecutive duplicate check)
    HistoryStatus m_iHistoryStatus; // Current position status (eAtTop, eInMiddle, eAtBottom)

    // ========== Sound ==========
    bool m_enable_beeps;
    bool m_enable_trigger_sounds;
    QString m_new_activity_sound;
    QString m_strBeepSound;

    // ========== Macros (Function Keys) ==========
    std::array<QString, MACRO_COUNT> m_macros;     // text for F1-F12
    std::array<quint16, MACRO_COUNT> m_macro_type; // send types
    std::array<QString, MACRO_COUNT> m_macro_name; // macro names

    // ========== Numeric Keypad ==========
    std::array<QString, KEYPAD_MAX_ITEMS> m_keypad; // keypad strings
    bool m_keypad_enable;                           // keypad enabled?

    // ========== Speed Walking ==========
    bool m_enable_speed_walk;
    QString m_speed_walk_prefix;
    QString m_strSpeedWalkFiller;
    quint16 m_iSpeedWalkDelay; // delay in ms

    // ========== Command Stack ==========
    bool m_enable_command_stack;
    QString m_strCommandStackCharacter;

    // ========== Connection Text ==========
    QString m_connect_text;

    // ========== File Handling ==========
    QString m_file_postamble;
    QString m_file_preamble;
    QString m_line_postamble;
    QString m_line_preamble;
    QString m_strLogFilePreamble;

    // ========== Paste Settings ==========
    QString m_paste_postamble;
    QString m_paste_preamble;
    QString m_pasteline_postamble;
    QString m_pasteline_preamble;

    // ========== World Notes ==========
    QString m_notes;

    // ========== Scripting ==========
    QString m_strLanguage;             // script language (e.g., "Lua")
    bool m_bEnableScripts;             // scripting enabled?
    QString m_strScriptFilename;       // script file path
    QString m_strScriptPrefix;         // script invocation prefix
    QString m_strScriptEditor;         // editor path
    QString m_strScriptEditorArgument; // editor arguments

    // ========== Script Event Handlers ==========
    QString m_strWorldOpen;       // handler on world open
    QString m_strWorldClose;      // handler on world close
    QString m_strWorldSave;       // handler on world save
    QString m_strWorldConnect;    // handler on connect
    QString m_strWorldDisconnect; // handler on disconnect
    QString m_strWorldGetFocus;   // handler on focus gain
    QString m_strWorldLoseFocus;  // handler on focus loss

    // ========== MXP (MUD Extension Protocol) ==========
    MXPMode m_iUseMXP;              // MXP usage (see enum)
    quint16 m_iMXPdebugLevel;       // MXP debug level
    QString m_strOnMXP_Start;       // MXP starting
    QString m_strOnMXP_Stop;        // MXP stopping
    QString m_strOnMXP_Error;       // MXP error
    QString m_strOnMXP_OpenTag;     // MXP tag open
    QString m_strOnMXP_CloseTag;    // MXP tag close
    QString m_strOnMXP_SetVariable; // MXP variable set

    // ========== Hyperlinks ==========
    QRgb m_iHyperlinkColour; // hyperlink color

    // ========== Miscellaneous Flags ==========
    bool m_indent_paras;
    bool m_bSaveWorldAutomatically;
    bool m_bLineInformation;
    bool m_bStartPaused;
    quint16 m_iNoteTextColour;
    bool m_bKeepCommandsOnSameLine;

    // ========== Auto-say Settings ==========
    QString m_strAutoSayString;
    bool m_bEnableAutoSay;
    bool m_bExcludeMacros;
    bool m_bExcludeNonAlpha;
    QString m_strOverridePrefix;
    bool m_bConfirmBeforeReplacingTyping;
    bool m_bReEvaluateAutoSay;

    // ========== Script Variables Collection (SAVED TO DISK) ==========
    // Holds all script variables (key-value pairs)
    // → Variable System Integration
    VariableMap m_VariableMap; // Map of variables (name → Variable*)

    // ========== Lua Database Collection (NOT saved to disk) ==========
    // Holds all Lua-accessible databases (opened via DatabaseOpen)
    // These are runtime-only and not persisted to world files
    std::map<QString, std::unique_ptr<LuaDatabase>>
        m_DatabaseMap; // Map of databases (name → LuaDatabase*)

    // ========== Print Styles ==========
    std::array<qint32, 8> m_nNormalPrintStyle; // print style for normal colors
    std::array<qint32, 8> m_nBoldPrintStyle;   // print style for bold colors

    // ========== Display Options (Version 9+) ==========
    bool m_bShowBold;               // show bold in fonts?
    bool m_bShowItalic;             // show italic?
    bool m_bShowUnderline;          // show underline?
    bool m_bAltArrowRecallsPartial; // alt+up recalls partial?
    quint16 m_iPixelOffset;         // pixel offset from window edge
    bool m_bAutoFreeze;             // freeze if not at bottom?
    bool m_bKeepFreezeAtBottom;     // don't unfreeze at bottom?
    bool m_bAutoRepeat;             // auto repeat last command?
    bool m_bDisableCompression;     // disable MCCP?
    bool m_bLowerCaseTabCompletion; // tab complete in lower case?
    bool m_bDoubleClickInserts;     // double-click inserts word?
    bool m_bDoubleClickSends;       // double-click sends to MUD?
    bool m_bConfirmOnSend;          // confirm preamble/postamble?
    bool m_bTranslateGerman;        // translate German chars?

    // ========== Tab Completion ==========
    QString m_strTabCompletionDefaults; // initial words
    quint32 m_iTabCompletionLines;      // lines to search
    bool m_bTabCompletionSpace;         // insert space after word?
    QString m_strWordDelimiters;        // word delimiters for tab completion

    // Shift+Tab completion (Lua API controlled)
    QSet<QString> m_ExtraShiftTabCompleteItems; // extra items for Shift+Tab menu
    bool m_bTabCompleteFunctions;               // show Lua functions in Shift+Tab menu

    // ========== Auto Logging ==========
    QString m_strAutoLogFileName;        // auto-log filename
    QString m_strLogLinePreambleOutput;  // log preamble for output
    QString m_strLogLinePreambleInput;   // log preamble for input
    QString m_strLogLinePreambleNotes;   // log preamble for notes
    QString m_strLogFilePostamble;       // log file postamble
    QString m_strLogLinePostambleOutput; // log postamble for output
    QString m_strLogLinePostambleInput;  // log postamble for input
    QString m_strLogLinePostambleNotes;  // log postamble for notes

    // ========== Output Line Preambles ==========
    QString m_strOutputLinePreambleOutput;     // output preamble for MUD output
    QString m_strOutputLinePreambleInput;      // output preamble for input
    QString m_strOutputLinePreambleNotes;      // output preamble for notes
    QRgb m_OutputLinePreambleOutputTextColour; // text color - output
    QRgb m_OutputLinePreambleOutputBackColour; // back color - output
    QRgb m_OutputLinePreambleInputTextColour;  // text color - input
    QRgb m_OutputLinePreambleInputBackColour;  // back color - input
    QRgb m_OutputLinePreambleNotesTextColour;  // text color - notes
    QRgb m_OutputLinePreambleNotesBackColour;  // back color - notes

    // ========== Recall Window ==========
    QString m_strRecallLinePreamble; // line preamble for recall

    // ========== Paste/File Options ==========
    bool m_bPasteCommentedSoftcode; // paste commented softcode?
    bool m_bFileCommentedSoftcode;  // send commented softcode?
    bool m_bFlashIcon;              // flash icon for activity?
    bool m_bArrowKeysWrap;          // arrow keys wrap history?
    bool m_bSpellCheckOnSend;       // spell check on send?
    qint32 m_nPasteDelay;           // paste delay (ms)
    qint32 m_nFileDelay;            // file send delay (ms)
    qint32 m_nPasteDelayPerLines;   // lines before delay
    qint32 m_nFileDelayPerLines;    // lines before delay

    // ========== Miscellaneous Options ==========
    ScriptReloadOption m_nReloadOption;    // script reload option
    qint32 m_bUseDefaultOutputFont;        // use default output font?
    qint32 m_bSaveDeletedCommand;          // save deleted command?
    qint32 m_bTranslateBackslashSequences; // interpret \n \r etc.?
    qint32 m_bEditScriptWithNotepad;       // use built-in notepad?
    qint32 m_bWarnIfScriptingInactive;     // warn if can't invoke script?

    // ========== Sending Options ==========
    bool m_bWriteWorldNameToLog; // write world name to log?
    bool m_bSendEcho;            // echo sends?
    bool m_bPasteEcho;           // echo pastes?

    // ========== Default Options ==========
    bool m_bUseDefaultColours;
    bool m_bUseDefaultTriggers;
    bool m_bUseDefaultAliases;
    bool m_bUseDefaultMacros;
    bool m_bUseDefaultTimers;
    bool m_bUseDefaultInputFont;

    // ========== Terminal Settings ==========
    QString m_strTerminalIdentification; // telnet negotiation ID

    // ========== Mapping ==========
    QString m_strMappingFailure; // mapping failure message
    bool m_bMapFailureRegexp;    // mapping failure is regexp?

    // ========== Flag Containers ==========
    quint16 m_iFlags1; // misc flags (see FLAGS1_* defines)
    quint16 m_iFlags2; // more flags (see FLAGS2_* defines)

    // ========== World ID ==========
    QString m_strWorldID; // unique GUID for this world

    // ========== More Options (Version 15+) ==========
    bool m_bAlwaysRecordCommandHistory; // record even if echo off?
    bool m_bCopySelectionToClipboard;   // auto-copy selection?
    bool m_bCarriageReturnClearsLine;   // \r clears line?
    bool m_bSendMXP_AFK_Response;       // reply to <afk>?
    bool m_bMudCanChangeOptions;        // server may recommend?
    bool m_bEnableSpamPrevention;       // spam prevention?
    quint16 m_iSpamLineCount;           // spam line threshold
    QString m_strSpamMessage;           // spam filler message

    bool m_bDoNotShowOutstandingLines; // hide outstanding lines?
    bool m_bDoNotTranslateIACtoIACIAC; // don't translate IAC?

    // ========== Clipboard and Display ==========
    bool m_bAutoCopyInHTML;              // auto-copy in HTML?
    quint16 m_iLineSpacing;              // line spacing (0 = auto)
    bool m_bUTF_8;                       // UTF-8 support?
    bool m_bConvertGAtoNewline;          // convert IAC/GA to newline?
    ActionSource m_iCurrentActionSource; // what caused current script?

    // ========== Filters ==========
    QString m_strTriggersFilter;  // Lua filter for triggers
    QString m_strAliasesFilter;   // Lua filter for aliases
    QString m_strTimersFilter;    // Lua filter for timers
    QString m_strVariablesFilter; // Lua filter for variables

    // ========== Script Errors ==========
    bool m_bScriptErrorsToOutputWindow; // show errors in output?
    bool m_bLogScriptErrors;            // log script errors?

    // ========== Command Window Auto-resize ==========
    bool m_bAutoResizeCommandWindow;        // auto-resize command window?
    QString m_strEditorWindowName;          // editor window name
    quint16 m_iAutoResizeMinimumLines;      // minimum lines
    quint16 m_iAutoResizeMaximumLines;      // maximum lines
    bool m_bDoNotAddMacrosToCommandHistory; // macros not in history?
    bool m_bSendKeepAlives;                 // use SO_KEEPALIVE?

    // ========== Default Trigger Settings ==========
    quint16 m_iDefaultTriggerSendTo;
    quint16 m_iDefaultTriggerSequence;
    bool m_bDefaultTriggerRegexp;
    bool m_bDefaultTriggerExpandVariables;
    bool m_bDefaultTriggerKeepEvaluating;
    bool m_bDefaultTriggerIgnoreCase;

    // ========== Default Alias Settings ==========
    quint16 m_iDefaultAliasSendTo;
    quint16 m_iDefaultAliasSequence;
    bool m_bDefaultAliasRegexp;
    bool m_bDefaultAliasExpandVariables;
    bool m_bDefaultAliasKeepEvaluating;
    bool m_bDefaultAliasIgnoreCase;

    // ========== Default Timer Settings ==========
    quint16 m_iDefaultTimerSendTo;

    // ========== Sound ==========
    bool m_bPlaySoundsInBackground; // use global sound buffer?

    // ========== HTML Logging ==========
    bool m_bLogHTML;       // convert HTML sequences?
    bool m_bUnpauseOnSend; // cancel pause on send?

    // ========== Logging Options ==========
    bool m_log_input;        // log player input?
    bool m_bLogOutput;       // log MUD output?
    bool m_bLogNotes;        // log notes?
    bool m_bLogInColour;     // HTML logging in colour?
    bool m_bLogRaw;          // log raw input from MUD?
    int m_nLogLines;         // max output lines to include in log (0 = unlimited)
    bool m_bAppendToLogFile; // append vs overwrite when opening log file

    // ========== Tree Views ==========
    bool m_bTreeviewTriggers; // show triggers in tree?
    bool m_bTreeviewAliases;  // show aliases in tree?
    bool m_bTreeviewTimers;   // show timers in tree?

    // ========== Input Wrapping ==========
    bool m_bAutoWrapInput; // match input wrap to output?

    // ========== Tooltips ==========
    quint32 m_iToolTipVisibleTime; // tooltip visible time (ms)
    quint32 m_iToolTipStartTime;   // tooltip delay time (ms)

    // ========== Save File Options ==========
    bool m_bOmitSavedDateFromSaveFiles; // don't write save date?

    // ========== Output Buffer Fading ==========
    quint16 m_iFadeOutputBufferAfterSeconds; // fade after N seconds
    quint16 m_FadeOutputOpacityPercent;      // fade opacity %
    quint16 m_FadeOutputSeconds;             // fade duration
    bool m_bCtrlBackspaceDeletesLastWord;    // Ctrl+Backspace behavior?

    // ========== Remote Access Server Settings ==========
    bool m_bEnableRemoteAccess;       // enable remote access server?
    quint16 m_iRemotePort;            // port to listen on (0 = disabled)
    QString m_strRemotePassword;      // password for authentication (required)
    quint16 m_iRemoteScrollbackLines; // lines to send on connect (default 100)
    quint16 m_iRemoteMaxClients;      // max simultaneous clients (default 5)
    quint16 m_iRemoteLockoutAttempts; // failed attempts before lockout (default 3)
    quint16 m_iRemoteLockoutSeconds;  // lockout duration (default 300)

    // ========== RUNTIME STATE VARIABLES (Not saved to disk) ==========

    // ========== Deprecated/Legacy (pre-version 11) ==========
    quint16 m_page_colour;
    quint16 m_whisper_colour;
    quint16 m_mail_colour;
    quint16 m_game_colour;
    bool m_remove_channels1;
    bool m_remove_channels2;
    bool m_remove_pages;
    bool m_remove_whispers;
    bool m_remove_set;
    bool m_remove_mail;
    bool m_remove_game;

    // ========== Runtime State Flags ==========
    // NOTE: m_bNAWS_wanted, m_bCHARSET_wanted, m_bNoEcho are now reference members
    // that delegate to m_telnetParser (see TelnetParser companion object section below).
    bool m_bLoaded;               // true if loaded from disk
    bool m_bSelected;             // true if selected in Send To All Worlds
    bool m_bVariablesChanged;     // true if variables have changed
    bool m_bModified;             // true if document has unsaved changes
    bool m_bDebugIncomingPackets; // display all incoming text?

    // ========== Statistics Counters ==========
    // NOTE: m_iInputPacketCount, m_iOutputPacketCount moved to ConnectionManager.
    // Access via m_connectionManager->m_iInputPacketCount,
    // m_connectionManager->m_iOutputPacketCount.
    qint32 m_iUTF8ErrorCount;          // count of lines with bad UTF8
    qint32 m_iOutputWindowRedrawCount; // count of times output window redrawn

    // ========== UTF-8 Processing State ==========
    std::array<quint8, 8> m_UTF8Sequence; // collect up to 4 UTF8 bytes + null (expanded for safety)
    qint32 m_iUTF8BytesLeft;              // how many UTF8 bytes to go

    // ========== Trigger/Alias/Timer Statistics ==========
    // NOTE: These statistics have moved to AutomationRegistry.
    // Access via m_automationRegistry->m_iTriggersEvaluatedCount, etc.

    // ========== UI State ==========
    qint32 m_last_prefs_page;         // last preferences page viewed
    bool m_bConfigEnableTimers;       // used during world config
    QString m_strLastSelectedTrigger; // last selected trigger
    QString m_strLastSelectedAlias;
    QString m_strLastSelectedTimer;
    QString m_strLastSelectedVariable;

    // ========== View Pointers ==========
    // Non-owning: lifetime managed by the parent QWidget (WorldWidget).
    // IInputView/IOutputView are pure abstract interfaces (not QObject-derived),
    // so QPointer cannot be used. Callers must not access these after WorldWidget
    // has been destroyed.
    IInputView* m_pActiveInputView;
    IOutputView* m_pActiveOutputView;

    // ========== Text Selection State ==========
    // Selection coordinates (0-based internally, converted to 1-based for Lua API)
    // -1 indicates no selection
    int m_selectionStartLine;
    int m_selectionStartChar;
    int m_selectionEndLine;
    int m_selectionEndChar;

    // ========== Line Buffer ==========
    std::vector<std::unique_ptr<Line>> m_lineList; // list of output buffer lines
    std::unique_ptr<Line> m_currentLine;           // the line currently receiving (owns the line)
    QString m_strCurrentLine;                      // current line from MUD (no control codes)

    // ========== Multi-line Trigger Buffer ==========
    // Rolling buffer of recent line text for multi-line trigger matching
    // Based on original MUSHclient ProcessPreviousLine.cpp
    static const int MAX_RECENT_LINES = 200;
    std::deque<QString> m_recentLines;

    // ========== Line Position Tracking ==========
    qint32 m_total_lines;
    qint32 m_new_lines;         // lines they haven't read yet
    qint32 m_newlines_received; // lines pushed into m_recentLines
    // NOTE: m_nTotalLinesSent, m_nTotalLinesReceived moved to ConnectionManager.
    // Access via m_connectionManager->m_nTotalLinesSent,
    // m_connectionManager->m_nTotalLinesReceived.
    qint32 m_last_line_with_IAC_GA;

    // ========== Timing Variables ==========
    // NOTE: m_tConnectTime, m_tsConnectDuration, m_whenWorldStarted,
    //       m_whenWorldStartedHighPrecision moved to ConnectionManager.
    // Access via m_connectionManager->m_tConnectTime, etc.
    QDateTime m_tLastPlayerInput; // last player input (for <afk>)

    QDateTime m_tStatusTime;    // time of line mouse was over
    QPoint m_lastMousePosition; // where mouse last was

    qint32 m_view_number; // sequence in activity view

    // ========== Timer Evaluation (1-second check timer) ==========
    // Qt parent-child owned (parent = this in constructor); do NOT manually delete.
    QTimer* m_timerCheckTimer = nullptr; // Periodic timer that calls checkTimers() every second

    // ========== TelnetParser (companion object) ==========
    // Owns all telnet protocol state: Phase, compression, IAC tracking, subnegotiation.
    // Created in WorldDocument constructor. Access via m_telnetParser->.
    //
    // Fields that previously lived directly on WorldDocument and are now in TelnetParser:
    //   m_phase, m_ttype_sequence, m_zlibStream (ZlibStream RAII), m_bCompress,
    //   m_CompressInput, m_CompressOutput, m_nTotalUncompressed, m_nTotalCompressed,
    //   m_iCompressionTimeTaken, m_iMCCP_type, m_bSupports_MCCP_2,
    //   m_subnegotiation_type, m_IAC_subnegotiation_data,
    //   m_bClient_sent_IAC_DO/DONT/WILL/WONT, m_bClient_got_IAC_DO/DONT/WILL/WONT,
    //   m_bNAWS_wanted, m_bCHARSET_wanted, m_bNoEcho, m_bATCP, m_bZMP, m_bMSP,
    //   m_nCount_IAC_DO/DONT/WILL/WONT/SB.
    // All access via m_telnetParser->fieldName.
    std::unique_ptr<TelnetParser> m_telnetParser;

    // ========== MXPEngine (companion object) ==========
    // Owns all MXP protocol state: mode, tags, entities, data structures, statistics.
    // Created in WorldDocument constructor. Access via m_mxpEngine->.
    //
    // Fields that previously lived directly on WorldDocument and are now in MXPEngine:
    //   m_bMXP, m_bPuebloActive, m_iPuebloLevel, m_bPreMode,
    //   m_iMXP_mode, m_iMXP_defaultMode, m_iMXP_previousMode,
    //   m_bInParagraph, m_bMXP_script, m_bSuppressNewline,
    //   m_bMXP_nobr, m_bMXP_preformatted, m_bMXP_centered,
    //   m_strMXP_link, m_strMXP_hint, m_bMXP_link_prompt,
    //   m_iMXP_list_depth, m_iMXP_list_counter, m_iListMode, m_iListCount,
    //   m_strMXPstring, m_strMXPtagcontents, m_cMXPquoteTerminator,
    //   m_atomicElementMap, m_customElementMap, m_entityMap, m_customEntityMap,
    //   m_activeTagList, m_gaugeMap,
    //   m_strPuebloMD5, m_iMXPerrors, m_iMXPtags, m_iMXPentities,
    //   m_dispidOnMXP_Start/Stop/OpenTag/CloseTag/SetVariable/Error.
    // All access via m_mxpEngine->fieldName or m_mxpEngine->method().
    std::unique_ptr<MXPEngine> m_mxpEngine;

    // ZMP package name stays on WorldDocument (it's ZMP state, not pure telnet negotiation)
    QString m_strZMPpackage; // Current ZMP package name during collection

    char m_cLastChar;   // last incoming character
    qint32 m_lastSpace; // position of last space in current line (for word-wrap), -1 if none
    qint32 m_iLastOutstandingTagCount;

    // ========== ANSI State ==========
    qint32 m_code;        // current ANSI code
    qint32 m_lastGoTo;    // last line we went to
    bool m_bWorldClosing; // world is closing?

    // Current style (from MUD)
    quint16 m_iFlags;   // style flags
    QRgb m_iForeColour; // foreground color
    QRgb m_iBackColour; // background color
    // shared_ptr is intentional: m_currentAction is copied into each Style created while the
    // hyperlink is active. All resulting Style objects co-own the same Action; none is the unique
    // owner. unique_ptr would require transferring ownership on every AddToLine() call and would
    // leave m_currentAction empty — breaking multi-style hyperlink runs.
    std::shared_ptr<Action> m_currentAction; // current hyperlink action (nullptr if none)

    // Notes style
    bool m_bNotesInRGB; // notes in RGB mode?
    QRgb m_iNoteColourFore;
    QRgb m_iNoteColourBack;
    quint16 m_iNoteStyle; // HILITE, UNDERLINE, etc.

    // ========== Logging ==========
    std::unique_ptr<QFile> m_logfile; // Current open log file (nullptr if not logging)
    QString m_logfile_name;           // Resolved log filename
    QDateTime m_LastFlushTime;        // Last time log was flushed to disk

    // ========== Fonts ==========
    qint32 m_FontHeight;
    qint32 m_FontWidth;
    qint32 m_InputFontHeight;
    qint32 m_InputFontWidth;

    // ========== Byte Counters ==========
    // NOTE: m_nBytesIn, m_nBytesOut moved to ConnectionManager.
    // Access via m_connectionManager->m_nBytesIn, m_connectionManager->m_nBytesOut.

    // NOTE: m_iConnectPhase moved to ConnectionManager.
    // Access via m_connectionManager->m_iConnectPhase.

    // ========== Scripting Engine ==========
    std::unique_ptr<ScriptEngine> m_ScriptEngine; // Lua scripting engine
    bool m_bSyntaxErrorOnly;
    bool m_bDisconnectOK;
    bool m_bTrace;
    bool m_bInSendToScript;
    qint64 m_iScriptTimeTaken; // time taken to execute scripts

    // Tab completion - moved to Tab Completion section (line ~608)

    QString m_strLastImmediateExpression;

    // Script file monitoring
    bool m_bInScriptFileChanged;
    QDateTime m_timeScriptFileMod;

    QString m_strStatusMessage; // status message
    QDateTime m_tStatusDisplayed;
    QString m_strScript; // script as read from file

    // ========== Info Bar (Script-controllable status area) ==========
    QString m_infoBarText;     // Text displayed in info bar
    bool m_infoBarVisible;     // Whether info bar is visible
    QRgb m_infoBarTextColor;   // Text color
    QRgb m_infoBarBackColor;   // Background color
    QString m_infoBarFontName; // Font name
    int m_infoBarFontSize;     // Font size in points
    int m_infoBarFontStyle;    // Font style (0=normal, 1=bold, 2=italic, 3=bold+italic)

    // ========== Script Handler DISPIDs (Lua flags) ==========
    qint32 m_dispidWorldOpen;
    qint32 m_dispidWorldClose;
    qint32 m_dispidWorldSave;
    qint32 m_dispidWorldConnect;
    qint32 m_dispidWorldDisconnect;
    qint32 m_dispidWorldGetFocus;
    qint32 m_dispidWorldLoseFocus;
    // NOTE: MXP DISPIDs (m_dispidOnMXP_Start/Stop/OpenTag/CloseTag/SetVariable/Error)
    // moved to MXPEngine. Access via m_mxpEngine->m_dispidOnMXP_*.

    // ========== Plugin State ==========
    bool m_bPluginProcessesOpenTag;
    bool m_bPluginProcessesCloseTag;
    bool m_bPluginProcessesSetVariable;
    bool m_bPluginProcessesSetEntity;
    bool m_bPluginProcessesError;

    // Recall flags
    bool m_bRecallCommands;
    bool m_bRecallOutput;
    bool m_bRecallNotes;

    // ========== Document ID ==========
    qint64 m_iUniqueDocumentNumber;

    // ========== Mapping ==========
    QString m_strSpecialForwards;
    QString m_strSpecialBackwards;
    // NOTE: m_CommandQueue moved to ConnectionManager.
    // Access via m_connectionManager->m_CommandQueue.
    bool m_bShowingMapperStatus;

    // Mapper state (Lua API: AddToMapper, GetMappingString, etc.)
    QStringList m_mapList;                   // Ordered list of map entries (directions/comments)
    bool m_bMapping = false;                 // Whether mapping is active
    bool m_bRemoveMapReverses = true;        // Auto-cancel opposite directions
    QMap<QRgb, QRgb> m_colourTranslationMap; // Color substitution map for display

    // ========== Plugins ==========
    std::vector<std::unique_ptr<Plugin>> m_PluginList; // List of installed plugins
    Plugin* m_CurrentPlugin; // Currently executing plugin (nullptr = world context)
    bool m_bPluginProcessingCommand;
    bool m_bPluginProcessingSend;
    bool m_bPluginProcessingSent;
    QString m_strLastCommandSent;
    qint32 m_iLastCommandCount;
    qint32 m_iExecutionDepth;
    bool m_bOmitFromCommandHistory;

    // ========== Option snapshots (for GetLoadedValue API) ==========
    // Populated after XML load completes; reflects values at load time.
    std::unordered_map<QString, double> m_loadedNumericOptions;
    std::unordered_map<QString, QString> m_loadedAlphaOptions;

    // ========== Arrays for scripting ==========
    // Named arrays for Lua ArrayCreate/ArrayGet/ArraySet etc.
    // Each array is a map of key->value pairs, organized by array name
    // Uses ArraysMap (QMap<QString, QMap<QString, QString>>) for consistency with Plugin
    ArraysMap m_Arrays;

    // ========== Background Image ==========
    QString m_strBackgroundImageName;
    qint32 m_iBackgroundMode;
    QRgb m_iBackgroundColour;

    // ========== Foreground Image ==========
    QString m_strForegroundImageName;
    qint32 m_iForegroundMode;

    // ========== MiniWindows ==========
    std::map<QString, std::unique_ptr<MiniWindow>>
        m_MiniWindowMap;                 // Map of miniwindows (name → MiniWindow)
    QVector<QString> m_MiniWindowsOrder; // Z-order sorted list of miniwindow names

    // ========== Text Rectangle ==========
    QRect m_TextRectangle; // Configuration (raw coordinates, may be negative for offsets)
    qint32 m_TextRectangleBorderOffset;
    qint32 m_TextRectangleBorderColour;
    qint32 m_TextRectangleBorderWidth;
    qint32 m_TextRectangleOutsideFillColour;
    qint32 m_TextRectangleOutsideFillStyle;
    QRect m_computedTextRectangle; // Computed/normalized rectangle (updated by OutputView)

    // ========== Sound System ==========
    // SoundManager owns all spatial audio state (engine, listener, 10 buffers).
    // Lazy-initialized on first playSound() call; nullptr until then.
    std::unique_ptr<SoundManager> m_soundManager;

    // ========== Notepad Windows ==========
    // Non-owning: NotepadWidget is a QWidget; Qt parent-child manages lifetime.
    // QPointer<NotepadWidget> auto-nulls if the widget is destroyed before removal.
    QList<QPointer<NotepadWidget>> m_notepadList;

    // ========== Accelerators ==========
    // Qt parent-child owned (parent = this in constructor); do NOT manually delete.
    AcceleratorManager* m_acceleratorManager;

    bool m_bNotesNotWantedNow;
    bool m_bDoingSimulate;
    bool m_bLineOmittedFromOutput;
    bool m_bOmitCurrentLineFromLog; // Trigger omit_from_log flag
    bool m_bScrollBarWanted;

    // NOTE: IAC Counters (m_nCount_IAC_DO/DONT/WILL/WONT/SB) moved to TelnetParser.
    // Access via m_telnetParser->m_nCount_IAC_*.

    // ========== UI State Strings ==========
    QString m_strWordUnderMenu;
    QString m_strWindowTitle;
    QString m_strMainWindowTitle;
    QString m_strWorldFilePath; // Full path to .mcl file (set by WorldWidget on load/save)

    // ========== Timing for fading ==========
    QDateTime m_timeFadeCancelled;
    QDateTime m_timeLastWindowDraw;

    // ========== Trigger Evaluation Control ==========
    quint16 m_iStopTriggerEvaluation;

    // ========== Pane Windows (optional) ==========
    // void* m_PaneMap;  // only if PANE is defined

    // ========== Unpacked flags from m_iFlags1 ==========
    bool m_bArrowRecallsPartial;
    bool m_bCtrlZGoesToEndOfBuffer;
    bool m_bCtrlPGoesToPreviousCommand;
    bool m_bCtrlNGoesToNextCommand;
    bool m_bHyperlinkAddsToCommandHistory;
    bool m_bEchoHyperlinkInOutputWindow;
    bool m_bAutoWrapWindowWidth;
    bool m_bNAWS;      // Negotiate About Window Size
    bool m_bUseZMP;    // Use ZMP (Zenith MUD Protocol)
    bool m_bUseATCP;   // Use ATCP (Achaea Telnet Client Protocol)
    bool m_bUseMSP;    // Use MSP (MUD Sound Protocol)
    bool m_bPueblo;    // Allow Pueblo
    bool m_bNoEchoOff; // ignore echo off
    bool m_bUseCustomLinkColour;
    bool m_bMudCanChangeLinkColour;
    bool m_bUnderlineHyperlinks;
    bool m_bMudCanRemoveUnderline;

    // ========== Unpacked flags from m_iFlags2 ==========
    bool m_bAlternativeInverse;
    bool m_bShowConnectDisconnect;
    bool m_bIgnoreMXPcolourChanges;
    bool m_bCustom16isDefaultColour;

    // Accessors for world name
    QString worldName() const
    {
        return m_mush_name;
    }
    void setWorldName(const QString& name)
    {
        m_mush_name = name;
    }

    // Document modification tracking
    bool isModified() const
    {
        return m_bModified;
    }
    void setModified(bool modified)
    {
        m_bModified = modified;
    }

    // Network event handlers
    void ReceiveMsg(); // Called when socket has data to read

    // Forwarding wrappers for WorldSocket callbacks (socket calls these on WorldDocument;
    // implementation now lives in ConnectionManager).
    void OnConnect(int errorCode)
    {
        m_connectionManager->onConnect(errorCode);
    }
    void OnConnectionDisconnect()
    {
        m_connectionManager->onConnectionDisconnect();
    }

    // Connection control methods (forwarding wrappers → ConnectionManager)
    void connectToMud()
    {
        m_connectionManager->connectToMud();
    }
    void disconnectFromMud()
    {
        m_connectionManager->disconnectFromMud();
    }
    void sendToMud(const QString& text); // Send text to MUD (stays on WorldDocument)

    // Connection time methods (for status bar) — forwarding wrappers → ConnectionManager
    [[nodiscard]] qint64 connectedTime() const
    {
        return m_connectionManager->connectedTime();
    }
    void resetConnectedTime()
    {
        m_connectionManager->resetConnectedTime();
    }

    // Logging status (for status bar)
    bool isLogging() const
    {
        return m_logfile != nullptr;
    }

    // ========== Telnet State Machine ==========

    // Main byte processor - routes incoming bytes to phase handlers.
    // Telnet-specific phases (HAVE_ESC, HAVE_IAC, HAVE_WILL, etc.) delegate to
    // m_telnetParser->Phase_*(). MXP phases (HAVE_MXP_ELEMENT etc.) delegate to
    // m_mxpEngine->Phase_MXP_*().
    void ProcessIncomingByte(unsigned char c);

    // MXP phase handler forwarding (called from ProcessIncomingByte, delegate to m_mxpEngine)
    void Phase_MXP_ELEMENT(unsigned char c)
    {
        m_mxpEngine->Phase_MXP_ELEMENT(c);
    }
    void Phase_MXP_COMMENT(unsigned char c)
    {
        m_mxpEngine->Phase_MXP_COMMENT(c);
    }
    void Phase_MXP_QUOTE(unsigned char c)
    {
        m_mxpEngine->Phase_MXP_QUOTE(c);
    }
    void Phase_MXP_ENTITY(unsigned char c)
    {
        m_mxpEngine->Phase_MXP_ENTITY(c);
    }

    // Support methods
    void SendPacket(std::span<const unsigned char> data); // Send raw bytes
    void OutputBadUTF8characters();                       // Fallback for invalid UTF-8

    // ========== ANSI Parser ==========
    void InterpretANSIcode(int code);    // Process ANSI color/style codes
    void Interpret256ANSIcode(int code); // Process 256-color and 24-bit codes

    // ========== Style Management ==========
    Style* AddStyle(quint16 iFlags, QRgb iForeColour, QRgb iBackColour, quint16 iLength,
                    const std::shared_ptr<Action>& pAction);
    void RememberStyle(const Style* pStyle);
    void GetStyleRGB(const Style* pOldStyle, QRgb& iForeColour, QRgb& iBackColour);

    // ========== MXP — forwarding wrappers to m_mxpEngine ==========
    // These keep the existing call sites in telnet_parser.cpp and
    // world_protocol.cpp compiling without modification.
    void MXP_On()
    {
        m_mxpEngine->MXP_On();
    }
    void MXP_Off(bool force = false)
    {
        m_mxpEngine->MXP_Off(force);
    }
    void MXP_mode_change(int mode)
    {
        m_mxpEngine->MXP_mode_change(mode);
    }
    void CleanupMXP()
    {
        m_mxpEngine->CleanupMXP();
    }
    bool MXP_Open() const
    {
        return m_mxpEngine->MXP_Open();
    }
    bool MXP_Secure() const
    {
        return m_mxpEngine->MXP_Secure();
    }

    // NOTE: SendWindowSizes moved to TelnetParser::sendWindowSizes(int width).
    // NOTE: MXP implementation methods (MXP_StartTag, MXP_EndTag, ParseMXPTag, etc.)
    //       are now on MXPEngine. Access via m_mxpEngine->.

    // ========== Line Buffer Management ==========
    void AddLineToBuffer(std::unique_ptr<Line> line); // Add line to buffer with size limiting
    void AddToLine(const char* str, int len = -1);    // Add text to current line
    void AddToLine(unsigned char c);                  // Add single character to current line
    void handleLineWrap();                            // Handle line wrapping when column exceeded
    void adjustStylesForTruncation(qint32 newLength); // Adjust styles when truncating line
    void StartNewLine(bool bNewLine,
                      unsigned char iFlags); // Complete current line and start new one

    // URL Detection and Linkification
    void DetectAndLinkifyURLs(Line* line); // Detect URLs and convert to hyperlinks
    void SplitStyleForURL(Line* line, size_t styleIdx,
                          int urlStart, // Split style to create URL hyperlink
                          int urlLength, const QString& url);

    // ========== Command Window Helpers ==========
    void setActiveInputView(IInputView* inputView);
    void setActiveOutputView(IOutputView* outputView);
    IInputView* activeInputView() const
    {
        return m_pActiveInputView;
    }
    IOutputView* activeOutputView() const
    {
        return m_pActiveOutputView;
    }

    QString GetCommand() const;
    qint32 SetCommand(const QString& text);
    qint32 SetCommandSelection(qint32 first, qint32 last);
    void SelectCommand();
    QString PushCommand();

    // Forwarding wrappers → ConnectionManager
    [[nodiscard]] QStringList GetCommandQueue() const
    {
        return m_connectionManager->getCommandQueue();
    }
    qint32 Queue(const QString& message, bool echo)
    {
        // Lua API boundary: convert std::expected<void, WorldError> → legacy integer error code.
        auto result = m_connectionManager->queue(message, echo);
        if (result) {
            return 0; // eOK
        }
        switch (result.error().type) {
            case WorldErrorType::NotConnected:
                return 30002; // eWorldClosed
            case WorldErrorType::ItemInUse:
                return 30063; // eItemInUse
            default:
                return 30000; // eUnknownError
        }
    }
    qint32 DiscardQueue()
    {
        return m_connectionManager->discardQueue();
    }

    qint32 Send(const QString& message);
    qint32 SendImmediate(const QString& message);
    qint32 SendNoEcho(const QString& message);
    qint32 SendPush(const QString& message);
    qint32 SendSpecial(const QString& message, bool echo, bool queue, bool log, bool history);
    qint32 LogSend(const QString& message);

    QString GetClipboard() const;
    void SetClipboard(const QString& text);

    // ========== Custom Colours ==========
    qint32 SetCustomColourName(qint16 whichColour, const QString& name);

    // ========== Text Selection ==========
    void setSelection(int startLine, int startChar, int endLine,
                      int endChar);       // Update selection state (called by OutputView)
    void clearSelection();                // Clear selection state
    qint32 GetSelectionStartLine() const; // Get selection start line (1-based), 0 if no selection
    qint32 GetSelectionEndLine() const;   // Get selection end line (1-based), 0 if no selection
    qint32
    GetSelectionStartColumn() const;      // Get selection start column (1-based), 0 if no selection
    qint32 GetSelectionEndColumn() const; // Get selection end column (1-based), 0 if no selection

    // ========== Command History ==========
    void addToCommandHistory(const QString& command); // Add command to history
    void clearCommandHistory();                       // Clear all command history

    // ========== Script Output Methods ==========

    void note(const QString& text); // Display note in output window
    void colourNote(QRgb foreColor, QRgb backColor, const QString& text); // Display colored note
    void colourTell(QRgb foreColor, QRgb backColor,
                    const QString& text); // Display colored text without newline (optional)
    void hyperlink(const QString& action, const QString& text, const QString& hint, QRgb foreColor,
                   QRgb backColor, bool isUrl); // Display clickable hyperlink
    void simulate(const QString& text);         // Process text as if received from MUD
    void noteHr();                              // Display horizontal rule

    // ========== Script Loading and Parsing ==========
    void loadScriptFile();               // Load and execute script file
    void showErrorLines(int lineNumber); // Display error context around line
    void setupScriptFileWatcher();       // Set up file watcher for script file changes

    // NOTE: onWorldConnect() and onWorldDisconnect() moved to ConnectionManager (private).
    // They are called from ConnectionManager::onConnect() and onConnectionDisconnect().

    // ========== Trigger and Alias Management ==========
    // Forwarding wrappers — implementation delegates to m_automationRegistry.
    [[nodiscard]] std::expected<void, WorldError> addTrigger(const QString& name,
                                                             std::unique_ptr<Trigger> trigger)
    {
        return m_automationRegistry->addTrigger(name, std::move(trigger));
    }
    [[nodiscard]] std::expected<void, WorldError> deleteTrigger(const QString& name)
    {
        return m_automationRegistry->deleteTrigger(name);
    }
    Trigger* getTrigger(const QString& name)
    {
        return m_automationRegistry->getTrigger(name);
    }
    [[nodiscard]] std::expected<void, WorldError> addAlias(const QString& name,
                                                           std::unique_ptr<Alias> alias)
    {
        return m_automationRegistry->addAlias(name, std::move(alias));
    }
    [[nodiscard]] std::expected<void, WorldError> deleteAlias(const QString& name)
    {
        return m_automationRegistry->deleteAlias(name);
    }
    Alias* getAlias(const QString& name)
    {
        return m_automationRegistry->getAlias(name);
    }

    // ========== Timer Management ==========
    [[nodiscard]] std::expected<void, WorldError> addTimer(const QString& name,
                                                           std::unique_ptr<Timer> timer)
    {
        return m_automationRegistry->addTimer(name, std::move(timer));
    }
    [[nodiscard]] std::expected<void, WorldError> deleteTimer(const QString& name)
    {
        return m_automationRegistry->deleteTimer(name);
    }
    Timer* getTimer(const QString& name)
    {
        return m_automationRegistry->getTimer(name);
    }
    void calculateNextFireTime(Timer* timer)
    {
        m_automationRegistry->resetOneTimer(timer);
    }

    // ========== Trigger Pattern Matching ==========
    void evaluateTriggers(Line* line)
    {
        m_automationRegistry->evaluateTriggers(line);
    }
    void rebuildTriggerArray()
    {
        m_automationRegistry->rebuildTriggerArray();
    }

    // ========== Trigger Execution and Actions ==========
    void executeTrigger(Trigger* trigger, Line* line,
                        const QString& matchedText); // Execute trigger action
    void executeTriggerScript(Trigger* trigger,
                              const QString& matchedText); // Execute Lua script callback
    QString replaceWildcards(const QString& text,
                             const QVector<QString>& wildcards); // Replace %1, %2, etc.
    void changeLineColors(Trigger* trigger, Line* line);         // Change matched line colors

    // ========== Alias Matching and Execution ==========
    bool evaluateAliases(const QString& command)
    {
        return m_automationRegistry->evaluateAliases(command);
    }
    void rebuildAliasArray()
    {
        m_automationRegistry->rebuildAliasArray();
    }
    void executeAlias(Alias* alias, const QString& command);       // Execute alias action
    void executeAliasScript(Alias* alias, const QString& command); // Execute Lua script callback

    // ========== Trigger/Alias XML Serialization ==========
    void saveTriggersToXml(class QXmlStreamWriter& xml); // Save triggers to XML
    void
    loadTriggersFromXml(class QXmlStreamReader& xml,
                        class Plugin* plugin = nullptr); // Load triggers from XML (plugin context)
    void saveAliasesToXml(class QXmlStreamWriter& xml);  // Save aliases to XML
    void
    loadAliasesFromXml(class QXmlStreamReader& xml,
                       class Plugin* plugin = nullptr); // Load aliases from XML (plugin context)

    // ========== Timer XML Serialization ==========
    void saveTimersToXml(class QXmlStreamWriter& xml); // Save timers to XML
    void loadTimersFromXml(class QXmlStreamReader& xml,
                           class Plugin* plugin = nullptr); // Load timers from XML (plugin context)

    // ========== Single-Item XML Serialization (for Plugin Wizard) ==========
    void Save_One_Trigger_XML(class QTextStream& out,
                              class Trigger* trigger); // Save single trigger as raw XML
    void Save_One_Alias_XML(class QTextStream& out,
                            class Alias* alias); // Save single alias as raw XML
    void Save_One_Timer_XML(class QTextStream& out,
                            class Timer* timer); // Save single timer as raw XML
    void Save_One_Variable_XML(class QTextStream& out,
                               class Variable* variable); // Save single variable as raw XML

    // ========== Variable Management ==========
    QString getVariable(const QString& name) const; // Get variable value by name
    qint32 setVariable(const QString& name,
                       const QString& value);   // Set variable (create or update)
    qint32 deleteVariable(const QString& name); // Delete variable by name
    QStringList getVariableList() const;        // Get list of all variable names
    QString expandVariables(const QString& text,
                            bool escapeRegex = true) const; // Expand @variable references in text

    // ========== Array Management (plugin-aware) ==========
    ArraysMap& getArrayMap();                  // Get arrays map (respects plugin context)
    const ArraysMap& getArrayMap() const;      // Get arrays map (const version)
    VariableMap& getVariableMap();             // Get variable map (respects plugin context)
    const VariableMap& getVariableMap() const; // Get variable map (const version)

    // ========== Variable XML Serialization ==========
    void saveVariablesToXml(class QXmlStreamWriter& xml); // Save variables to XML
    void loadVariablesFromXml(
        class QXmlStreamReader& xml,
        class Plugin* plugin = nullptr); // Load variables from XML (plugin context)

    // ========== Accelerator XML Serialization ==========
    void saveAcceleratorsToXml(class QXmlStreamWriter& xml);   // Save user accelerators to XML
    void loadAcceleratorsFromXml(class QXmlStreamReader& xml); // Load user accelerators from XML

    // ========== Macro/Keypad Compatibility (Original MUSHclient format) ==========
    void loadMacrosFromXml(class QXmlStreamReader& xml); // Load <macros> from original format
    void loadKeypadFromXml(class QXmlStreamReader& xml); // Load <keypad> from original format
    void saveMacrosToXml(class QXmlStreamWriter& xml);   // Save macro-compatible keys as <macros>
    void saveKeypadToXml(class QXmlStreamWriter& xml);   // Save keypad-compatible keys as <keypad>

    // ========== Plugin World Serialization ==========
    void savePluginsToXml(class QXmlStreamWriter& xml);   // Save plugin list to XML
    void loadPluginsFromXml(class QXmlStreamReader& xml); // Load plugins from XML

    // ========== Timer Fire Time Calculation ==========
    void resetOneTimer(Timer* timer)
    {
        m_automationRegistry->resetOneTimer(timer);
    }
    void resetAllTimers()
    {
        m_automationRegistry->resetAllTimers();
    }

    // ========== Timer Evaluation Loop ==========
    void checkTimers()
    {
        m_automationRegistry->checkTimers();
    }
    void checkTimerList()
    {
        m_automationRegistry->checkTimerList();
    }

    // ========== Timer Execution ==========
    void executeTimer(Timer* timer, const QString& name);       // Execute a fired timer
    void executeTimerScript(Timer* timer, const QString& name); // Execute Lua script callback

    // ========== Plugin Timer Execution ==========
    void checkPluginTimerList(Plugin* plugin)
    {
        m_automationRegistry->checkPluginTimerList(plugin);
    }
    void executePluginTimer(Plugin* plugin, Timer* timer,
                            const QString& name); // Execute plugin timer
    void executePluginTimerScript(Plugin* plugin, Timer* timer,
                                  const QString& name); // Execute plugin timer script

    // ========== SendTo() - Central Action Routing ==========
    void sendTo(SendTo iWhere, const QString& strSendText, bool omit_from_output,
                bool omit_from_log, const QString& strDescription, const QString& variable,
                QString& strOutput, ScriptLanguage scriptLang = ScriptLanguage::Lua);

    // ========== Command Execution Pipeline ==========
    void SendMsg(const QString& text, bool bEcho, bool bQueue,
                 bool bLog);                                    // High-level command routing
    void DoSendMsg(const QString& text, bool bEcho, bool bLog); // Low-level command sending
    void logCommand(const QString& text);                       // Log command to input log

    // ========== Paste/Send File Helper ==========
    /**
     * Progress callback for sendTextToMud
     * Called periodically during send operation
     * @param current Current line being sent (0-based)
     * @param total Total lines to send
     * @return true to continue, false to cancel
     */
    using ProgressCallback = std::function<bool(int current, int total)>;

    /**
     * Send multi-line text to MUD with preamble, postamble, delays, etc.
     * Used by "Paste to MUD" and "Send File" features.
     *
     * @param text The text to send (can be multi-line)
     * @param preamble Text sent before all content
     * @param linePreamble Text prepended to each line
     * @param linePostamble Text appended to each line
     * @param postamble Text sent after all content
     * @param commentedSoftcode Strip leading # from lines
     * @param lineDelay Delay in ms between lines
     * @param delayPerLines Apply delay every N lines
     * @param echo Echo sent lines to output
     * @param progressCallback Optional callback for progress updates (nullptr to skip)
     * @return true if send was completed (not cancelled)
     */
    bool sendTextToMud(const QString& text, const QString& preamble, const QString& linePreamble,
                       const QString& linePostamble, const QString& postamble,
                       bool commentedSoftcode, int lineDelay, int delayPerLines, bool echo,
                       ProgressCallback progressCallback = nullptr);

    // ========== Command Stacking ==========
    void Execute(const QString& command); // Process command with stacking support

    // ========== Speed Walking ==========
    QString DoEvaluateSpeedwalk(const QString& speedWalkString); // Parse speedwalk notation
    QString DoReverseSpeedwalk(const QString& speedWalkString);  // Reverse a speedwalk
    QString RemoveBacktracks(const QString& speedWalkString);    // Remove redundant movements

    // ========== Log File Management ==========
    qint32 OpenLog(const QString& filename, bool append); // Open log file for writing
    qint32 CloseLog();                                    // Close log file
    void WriteToLog(const QString& text);    // Internal: write text to log (no newline)
    qint32 WriteLog(const QString& message); // API: write message to log (add newline if missing)
    qint32 FlushLog();                       // Flush log to disk
    bool IsLogOpen() const;                  // Check if log file is open
    QString FormatTime(const QDateTime& dt, const QString& pattern,
                       bool forHTML);           // Expand time codes in string
    QString FixHTMLString(const QString& text); // Escape HTML special characters
    void LogLineInHTMLcolour(Line* line);       // Write styled line as HTML
    void logCompletedLine(Line* line);          // Log completed line with preamble/postamble
    void writeRetrospectiveLog();               // Write all buffered lines with LOG_LINE flag

    // ========== Plugin Management ==========
    Plugin* FindPluginByID(const QString& pluginID);       // Find plugin by GUID
    Plugin* FindPluginByName(const QString& pluginName);   // Find plugin by name
    Plugin* FindPluginByFilePath(const QString& filepath); // Find plugin by source file path
    Plugin* LoadPlugin(const QString& filepath, QString& errorMsg); // Load plugin from XML file
    bool UnloadPlugin(const QString& pluginID);                     // Unload and delete plugin
    bool EnablePlugin(const QString& pluginID, bool enabled);       // Enable/disable plugin
    void PluginListChanged();                   // Notify plugins that list changed
    Plugin* getPlugin(const QString& pluginID); // Get plugin by ID (alias for FindPluginByID)
    Plugin* getCurrentPlugin() const
    {
        return m_CurrentPlugin;
    } // Get currently executing plugin
    void setCurrentPlugin(Plugin* plugin)
    {
        m_CurrentPlugin = plugin;
    } // Set currently executing plugin

    // ========== MiniWindow Management ==========
    qint32 WindowFont(const QString& windowName, const QString& fontId, const QString& fontName,
                      double size, bool bold, bool italic, bool underline, bool strikeout,
                      qint16 charset, qint16 pitchAndFamily); // Add font to miniwindow

    // ========== Plugin Callbacks ==========
    void SendToAllPluginCallbacks(const QString& callbackName); // Call all plugins (no args)
    bool SendToAllPluginCallbacks(const QString& callbackName, const QString& arg,
                                  bool bStopOnFalse = false); // Call all plugins (1 arg)
    bool SendToAllPluginCallbacks(const QString& callbackName, qint32 arg1, const QString& arg2,
                                  bool bStopOnTrue = false); // Call all plugins (int + string)
    bool SendToAllPluginCallbacks(const QString& callbackName, qint32 arg1, qint32 arg2,
                                  const QString& arg3); // Call all plugins (int + int + string)

    // First-match callbacks (stop on first true response)
    bool SendToFirstPluginCallbacks(const QString& callbackName,
                                    const QString& arg); // Stop on first true

    // ========== Screen Draw Callback ==========
    // Called when lines are displayed on screen for accessibility/screen reader support
    // type: 0 = MUD output, 1 = note (COMMENT), 2 = user input (USER_INPUT)
    // iLog: whether line should be logged
    void Screendraw(qint32 type, bool iLog, const QString& sText);

    // ========== Trace Output ==========
    // Output trace message to display (if trace enabled)
    // If a plugin handles the trace, message is not displayed
    void Trace(const QString& message);

    // ========== Other stub methods ==========
    // NOTE: InitZlib moved to ZlibStream::init() inside TelnetParser (telnet_parser.h/cpp).
    // NOTE: OnConnectionDisconnect() moved to ConnectionManager::onConnectionDisconnect().
    //       The forwarding wrapper above (WorldDocument::OnConnectionDisconnect) calls it.
    void Repaint(); // Trigger UI repaint (for miniwindows etc.)

    // ========== Sound System ==========
    // All sound functionality is delegated to m_soundManager (SoundManager companion object).
    // The methods below are thin forwarding wrappers kept for source compatibility with
    // callers in world_trigger_execution.cpp, telnet_parser.cpp, and Lua API bindings.
    qint32 PlaySound(qint16 buffer, const QString& filename, bool loop, double volume, double pan);
    qint32 StopSound(qint16 buffer);
    bool PlaySoundFile(const QString& filename);
    qint32 GetSoundStatus(qint16 buffer);
    void PlayMSPSound(const QString& filename, const QString& url, bool loop, double volume,
                      qint16 buffer);
    bool IsWindowActive();

    // ========== Notepad Management ==========
    void RegisterNotepad(NotepadWidget* notepad);     // Register notepad with world
    void UnregisterNotepad(NotepadWidget* notepad);   // Unregister notepad from world
    NotepadWidget* FindNotepad(const QString& title); // Find notepad by title (case-insensitive)
    NotepadWidget* CreateNotepadWindow(const QString& title,
                                       const QString& contents); // Create new notepad window

    // ========== Notepad Operations (Lua API) ==========
    std::expected<void, WorldError>
    SendToNotepad(const QString& title,
                  const QString& contents); // Create/replace notepad text
    std::expected<void, WorldError> AppendToNotepad(const QString& title,
                                                    const QString& contents); // Append to notepad
    std::expected<void, WorldError>
    ReplaceNotepad(const QString& title,
                   const QString& contents); // Replace notepad text (must exist)
    std::expected<void, WorldError>
    ActivateNotepad(const QString& title); // Activate (raise/focus) notepad window
    qint32 CloseNotepad(const QString& title, bool querySave); // Close notepad (with save prompt)
    qint32 SaveNotepad(const QString& title, const QString& filename,
                       bool replaceExisting);          // Save notepad to file
    QStringList GetNotepadList(bool includeAllWorlds); // Get list of notepad titles
    qint32 NotepadFont(const QString& title, const QString& name, qint32 size, qint32 style,
                       qint32 charset); // Set notepad font
    qint32 NotepadColour(const QString& title, const QString& textColour,
                         const QString& backColour);               // Set notepad colors
    qint32 NotepadReadOnly(const QString& title, bool readOnly);   // Set read-only mode
    qint32 NotepadSaveMethod(const QString& title, qint32 method); // Set auto-save method
    std::expected<void, WorldError> MoveNotepadWindow(const QString& title, qint32 left, qint32 top,
                                                      qint32 width,
                                                      qint32 height); // Move/resize notepad window
    QString GetNotepadWindowPosition(
        const QString& title); // Get notepad window position as "left,top,width,height"

    // ========== Flag Management ==========
    void packFlags();   // Pack individual bool members back into m_iFlags1/m_iFlags2
    void unpackFlags(); // Unpack m_iFlags1/m_iFlags2 into individual bool members

    // ========== Recall/Buffer Search ==========
    /**
     * RecallText - Search buffer and return matching lines
     *
     * Searches through the output buffer for lines matching the search text.
     * Can filter by line type (output/commands/notes) and use regex matching.
     *
     * @param searchText Text or regex pattern to search for
     * @param matchCase If true, search is case-sensitive
     * @param useRegex If true, treat searchText as regex pattern
     * @param includeOutput If true, include normal MUD output lines
     * @param includeCommands If true, include user command lines
     * @param includeNotes If true, include script note/comment lines
     * @param lineCount Number of lines to search (0 = all lines)
     * @param linePreamble Optional timestamp format to prepend to each line
     * @return QString containing all matching lines (with preambles if specified)
     */
    QString RecallText(const QString& searchText, bool matchCase, bool useRegex, bool includeOutput,
                       bool includeCommands, bool includeNotes, int lineCount,
                       const QString& linePreamble);

  signals:
    void worldNameChanged(const QString& name);
    void connectionStateChanged(bool connected);
    void linesAdded();            // Emitted when new lines added to buffer
    void incompleteLine();        // Emitted when we have an incomplete line (prompt) to display
    void outputSettingsChanged(); // Emitted when output font/colors change
    void inputSettingsChanged();  // Emitted when input font/colors change
    void miniwindowCreated(MiniWindow* win);     // Emitted when miniwindow created
    void notepadCreated(NotepadWidget* notepad); // Emitted when notepad created
    void textRectangleConfigChanged();           // Emitted when text rectangle config changes
    void pasteToCommand(const QString& text);    // Emitted to paste text into command input
    void activateWorldWindow();                  // Emitted to bring world window to front
    void activateClientWindow();                 // Emitted to bring main window to front
    void infoBarChanged();                       // Emitted when info bar text/style changes

  private slots:
    void onScriptFileChanged(const QString& path); // Handle script file change notification

  private:
    void initializeDefaults();
    void initializeColors();

    // Script file monitoring
    // Manually owned (NOT Qt parent-child; created without parent to avoid double-free).
    // Deleted in setupScriptFileWatcher() on reset and in ~WorldDocument() on final cleanup.
    QFileSystemWatcher* m_scriptFileWatcher;
};

#endif // WORLD_DOCUMENT_H
