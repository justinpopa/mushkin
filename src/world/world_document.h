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
#include <QRect>
#include <QRgb>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>     // Timer evaluation loop
#include <QVector>    // TriggerArray, AliasArray
#include <deque>      // For m_recentLines (multi-line triggers)
#include <functional> // For std::function (progress callback)
#include <memory>     // For std::unique_ptr

#include "../automation/variable.h" // ArraysMap type
#include "miniwindow.h"             // MiniWindow (off-screen drawing surface)
#include "mxp_types.h"              // MXP data structures
#include <QJsonArray>               // GMCP JSON parsing
#include <QJsonObject>              // GMCP JSON parsing
#include <QJsonValue>               // GMCP JSON parsing
#include <zlib.h>                   // For MCCP compression

// Forward declarations
class QAudioEngine;   // Spatial audio engine
class QAudioListener; // Spatial audio listener
class QSpatialSound;  // Positioned sound source
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
class OutputView;         // for miniwindow signal connection
class InputView;          // Command input widget
class NotepadWidget;      // Notepad window
class RemoteAccessServer; // Remote access server for telnet connections
class AcceleratorManager; // Keyboard accelerator management

// SQLite forward declarations (for Lua database functions)
struct sqlite3;
struct sqlite3_stmt;


// Constants
#define MAX_CUSTOM 16       // maximum custom colours
#define MACRO_COUNT 12      // F1-F12 function keys
#define KEYPAD_MAX_ITEMS 30 // keypad items

// xterm 256-color palette (defined in world_protocol.cpp)
extern QRgb xterm_256_colours[256];
// Sound buffers: Match original MUSHclient's 10 buffers for compatibility
#define MAX_SOUND_BUFFERS 10 // maximum simultaneous sounds

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
};

// Database error codes (negative to avoid conflicting with SQLite error codes)
// Based on methods_database.cpp
const qint32 DATABASE_ERROR_ID_NOT_FOUND = -1;            // Unknown database ID
const qint32 DATABASE_ERROR_NOT_OPEN = -2;                // Database not opened
const qint32 DATABASE_ERROR_HAVE_PREPARED_STATEMENT = -3; // Already have a prepared statement
const qint32 DATABASE_ERROR_NO_PREPARED_STATEMENT = -4;   // No prepared statement yet
const qint32 DATABASE_ERROR_NO_VALID_ROW = -5;            // Have not stepped to valid row
const qint32 DATABASE_ERROR_DATABASE_ALREADY_EXISTS =
    -6;                                               // Already have a database of that disk name
const qint32 DATABASE_ERROR_COLUMN_OUT_OF_RANGE = -7; // Requested column out of range

// Sound Buffer Structure
// Uses Qt Spatial Audio for stereo panning support
struct SoundBuffer {
    QSpatialSound* spatialSound; // Positioned sound source
    bool isPlaying;              // Currently playing?
    bool isLooping;              // Loop playback?
    QString filename;            // Current file

    SoundBuffer() : spatialSound(nullptr), isPlaying(false), isLooping(false)
    {
    }
};

// Flag bits for m_iFlags1
#define FLAGS1_ArrowRecallsPartial 0x0001
#define FLAGS1_CtrlZGoesToEndOfBuffer 0x0002
#define FLAGS1_CtrlPGoesToPreviousCommand 0x0004
#define FLAGS1_CtrlNGoesToNextCommand 0x0008
#define FLAGS1_HyperlinkAddsToCommandHistory 0x0010
#define FLAGS1_EchoHyperlinkInOutputWindow 0x0020
#define FLAGS1_AutoWrapWindowWidth 0x0040
#define FLAGS1_NAWS 0x0080
#define FLAGS1_Pueblo 0x0100
#define FLAGS1_NoEchoOff 0x0200
#define FLAGS1_UseCustomLinkColour 0x0400
#define FLAGS1_MudCanChangeLinkColour 0x0800
#define FLAGS1_UnderlineHyperlinks 0x1000
#define FLAGS1_MudCanRemoveUnderline 0x2000

// Flag bits for m_iFlags2
#define FLAGS2_AlternativeInverse 0x0001
#define FLAGS2_ShowConnectDisconnect 0x0002
#define FLAGS2_IgnoreMXPcolourChanges 0x0004
#define FLAGS2_Custom16isDefaultColour 0x0008
#define FLAGS2_LogInColour 0x0010
#define FLAGS2_LogRaw 0x0020

// Auto-connect values
enum { eNoAutoConnect, eConnectMUSH, eConnectAndGoIntoGame };

// MXP usage values
enum { eMXP_Off, eMXP_Query, eMXP_On };

// Connection phase values (doc.h)
enum {
    eConnectNotConnected,  // 0: not connected and not attempting connection
    eConnectMudNameLookup, // 1: finding address of MUD
    // Proxy states removed
    eConnectConnectingToMud = 3, // 3: connecting to MUD
    eConnectConnectedToMud = 8,  // 8: connected, we can play now
    eConnectDisconnecting = 9,   // 9: in process of disconnecting, don't attempt to reconnect
};

// Action source values (Lua callbacks)
// These tell scripts what triggered the current code execution
// Based on doc.h action source enum
enum ActionSource {
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
enum HistoryStatus {
    eAtTop,    // At the top (oldest command) of history
    eInMiddle, // Somewhere in the middle of history
    eAtBottom  // At the bottom (newest, or ready for new command)
};

// Script reload option values (for m_nReloadOption)
// Based on doc.h script reload enum
enum ScriptReloadOption {
    eReloadConfirm = 0, // Show dialog asking user when script file changes
    eReloadAlways = 1,  // Automatically reload script when file changes
    eReloadNever = 2    // Ignore script file changes
};

// ========== Telnet Protocol Constants (RFC 854) ==========

// Telnet commands
#define IAC 255               // 0xFF - Interpret As Command
#define DONT 254              // 0xFE - Don't use this option
#define DO 253                // 0xFD - Please use this option
#define WONT 252              // 0xFC - I won't use this option
#define WILL 251              // 0xFB - I will use this option
#define SB 250                // 0xFA - Subnegotiation begin
#define GO_AHEAD 249          // 0xF9 - Go Ahead
#define ERASE_LINE 248        // 0xF8 - Erase Line
#define ERASE_CHARACTER 247   // 0xF7 - Erase Character
#define ARE_YOU_THERE 246     // 0xF6 - Are You There
#define ABORT_OUTPUT 245      // 0xF5 - Abort Output
#define INTERRUPT_PROCESS 244 // 0xF4 - Interrupt Process
#define BREAK 243             // 0xF3 - Break
#define DATA_MARK 242         // 0xF2 - Data Mark
#define NOP 241               // 0xF1 - No Operation
#define SE 240                // 0xF0 - Subnegotiation End
#define EOR 239               // 0xEF - End Of Record

// Telnet options (TELOPT)
#define TELOPT_ECHO 1           // Echo
#define TELOPT_SGA 3            // Suppress Go Ahead (SGA)
#define TELOPT_TERMINAL_TYPE 24 // Terminal Type
#define TELOPT_NAWS 31          // Negotiate About Window Size
#define TELOPT_CHARSET 42       // Character Set
#define TELOPT_COMPRESS 85      // MCCP v1 (Mud Client Compression Protocol)
#define TELOPT_COMPRESS2 86     // MCCP v2
#define TELOPT_MSP 90           // MSP (MUD Sound Protocol)
#define TELOPT_MXP 91           // MUD eXtension Protocol
#define TELOPT_ZMP 93           // ZMP (Zenith MUD Protocol)
#define TELOPT_MUD_SPECIFIC 102 // Aardwolf telnet option
#define TELOPT_ATCP 200         // ATCP (Achaea Telnet Client Protocol)
#define TELOPT_GMCP 201         // GMCP (Generic Mud Communication Protocol)

// Telnet subnegotiation opcodes
#define WILL_END_OF_RECORD 25 // Will send End Of Record

// MCCP constants
#define COMPRESS_BUFFER_LENGTH 20000

// MXP line security modes (ESC[<n>z) - doc.h
// IMPORTANT: These values must match the ANSI escape code values!
#define eMXP_open 0        // Open mode (all tags allowed)
#define eMXP_secure 1      // Secure mode (limited tags)
#define eMXP_locked 2      // Locked mode (no tags)
#define eMXP_reset 3       // MXP reset (close all open tags)
#define eMXP_secure_once 4 // Next tag is secure only
#define eMXP_perm_open 5   // Permanent open mode
#define eMXP_perm_secure 6 // Permanent secure mode
#define eMXP_perm_locked 7 // Permanent locked mode
// Room and welcome modes (10-19)
#define eMXP_room_name 10        // Line is room name
#define eMXP_room_description 11 // Line is room description
#define eMXP_room_exits 12       // Line is room exits
#define eMXP_welcome 19          // Welcome text

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

// ANSI formatting codes
#define ANSI_RESET 0
#define ANSI_BOLD 1
#define ANSI_BLINK 3
#define ANSI_UNDERLINE 4
#define ANSI_SLOW_BLINK 5
#define ANSI_FAST_BLINK 6
#define ANSI_INVERSE 7
#define ANSI_STRIKEOUT 9

// Cancel codes (new in MUSHclient 3.27)
#define ANSI_CANCEL_BOLD 22
#define ANSI_CANCEL_BLINK 23
#define ANSI_CANCEL_UNDERLINE 24
#define ANSI_CANCEL_SLOW_BLINK 25
#define ANSI_CANCEL_INVERSE 27
#define ANSI_CANCEL_STRIKEOUT 29

// Foreground colors (30-37)
#define ANSI_TEXT_BLACK 30
#define ANSI_TEXT_RED 31
#define ANSI_TEXT_GREEN 32
#define ANSI_TEXT_YELLOW 33
#define ANSI_TEXT_BLUE 34
#define ANSI_TEXT_MAGENTA 35
#define ANSI_TEXT_CYAN 36
#define ANSI_TEXT_WHITE 37
#define ANSI_TEXT_256_COLOUR 38 // Extended colors (256-color, 24-bit)
#define ANSI_SET_FOREGROUND_DEFAULT 39

// Background colors (40-47)
#define ANSI_BACK_BLACK 40
#define ANSI_BACK_RED 41
#define ANSI_BACK_GREEN 42
#define ANSI_BACK_YELLOW 43
#define ANSI_BACK_BLUE 44
#define ANSI_BACK_MAGENTA 45
#define ANSI_BACK_CYAN 46
#define ANSI_BACK_WHITE 47
#define ANSI_BACK_256_COLOUR 48 // Extended colors (256-color, 24-bit)
#define ANSI_SET_BACKGROUND_DEFAULT 49

// ========== Style Flag Bits (OtherTypes.h) ==========

// Style flags (stored in CStyle::iFlags)
#define HILITE 0x0001    // bold
#define UNDERLINE 0x0002 // underline
#define BLINK 0x0004     // italic (blink repurposed for italic)
#define INVERSE 0x0008   // inverted (swap fore/back)
#define STRIKEOUT 0x0020 // strikethrough

// Color type flags (which color mode)
#define COLOUR_ANSI 0x0000     // ANSI color from ANSI table (0-7)
#define COLOUR_CUSTOM 0x0100   // Custom color from custom table
#define COLOUR_RGB 0x0200      // RGB color in iForeColour/iBackColour
#define COLOUR_RESERVED 0x0300 // Reserved

#define COLOURTYPE 0x0300 // Mask for color type bits
#define ACTIONTYPE 0x0C00 // Mask for action type bits
#define STYLE_BITS 0x0FFF // Mask for all style bits (everything except START_TAG)

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

// Telnet Phase State Machine (doc.h)
// These are the states for parsing the incoming telnet/ANSI stream
enum Phase {
    NONE = 0,                // Normal text
    HAVE_ESC,                // Just received ESC (0x1B)
    DOING_CODE,              // Processing ANSI escape sequence ESC[...m
    HAVE_IAC,                // Just received IAC (0xFF)
    HAVE_WILL,               // Got IAC WILL
    HAVE_WONT,               // Got IAC WONT
    HAVE_DO,                 // Got IAC DO
    HAVE_DONT,               // Got IAC DONT
    HAVE_SB,                 // Got IAC SB (subnegotiation begin)
    HAVE_SUBNEGOTIATION,     // In subnegotiation, collecting data
    HAVE_SUBNEGOTIATION_IAC, // Got IAC inside subnegotiation
    HAVE_COMPRESS,           // Got IAC SB COMPRESS (MCCP v1)
    HAVE_COMPRESS_WILL,      // Got IAC SB COMPRESS WILL (MCCP v1)

    // 256-color support (ESC[38;5;<n>m or ESC[48;5;<n>m)
    HAVE_FOREGROUND_256_START,  // Got ESC[38;
    HAVE_FOREGROUND_256_FINISH, // Got ESC[38;5;
    HAVE_BACKGROUND_256_START,  // Got ESC[48;
    HAVE_BACKGROUND_256_FINISH, // Got ESC[48;5;

    // 24-bit true color support (ESC[38;2;<r>;<g>;<b>m)
    HAVE_FOREGROUND_24B_FINISH,  // Got ESC[38;2; (waiting for R)
    HAVE_FOREGROUND_24BR_FINISH, // Got ESC[38;2;<r>; (waiting for G)
    HAVE_FOREGROUND_24BG_FINISH, // Got ESC[38;2;<r>;<g>; (waiting for B)
    HAVE_FOREGROUND_24BB_FINISH, // Got ESC[38;2;<r>;<g>;<b>; (complete)
    HAVE_BACKGROUND_24B_FINISH,  // Got ESC[48;2; (waiting for R)
    HAVE_BACKGROUND_24BR_FINISH, // Got ESC[48;2;<r>; (waiting for G)
    HAVE_BACKGROUND_24BG_FINISH, // Got ESC[48;2;<r>;<g>; (waiting for B)
    HAVE_BACKGROUND_24BB_FINISH, // Got ESC[48;2;<r>;<g>;<b>; (complete)

    // UTF-8 multibyte character handling
    HAVE_UTF8_CHARACTER, // Receiving UTF-8 continuation bytes

    // MXP (MUD eXtension Protocol) modes
    HAVE_MXP_ELEMENT, // Collecting MXP element <...>
    HAVE_MXP_COMMENT, // Collecting MXP comment <!--...-->
    HAVE_MXP_QUOTE,   // Inside quoted string in MXP element
    HAVE_MXP_ENTITY,  // Collecting MXP entity &...;

    // MXP special collection modes
    HAVE_MXP_ROOM_NAME,        // Parsing room name
    HAVE_MXP_ROOM_DESCRIPTION, // Parsing room description
    HAVE_MXP_ROOM_EXITS,       // Parsing room exits
    HAVE_MXP_WELCOME,          // Parsing welcome text
};

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
    ~WorldDocument();

    // Public member variables (for direct port compatibility)
    WorldSocket* m_pSocket;
    std::unique_ptr<RemoteAccessServer> m_pRemoteServer; // Remote access server (runtime only)

    // ========== Connection Settings ==========
    QString m_server;      // hostname or IP address
    QString m_mush_name;   // world name
    QString m_name;        // player name
    QString m_password;    // player password
    quint16 m_port;        // port number (1-65535)
    quint16 m_connect_now; // auto-connect flag (see enum above)

    // ========== Display Settings ==========
    QString m_font_name;    // output font face name
    qint32 m_font_height;   // font size in pixels
    qint32 m_font_weight;   // bold/normal (400=normal, 700=bold)
    quint32 m_font_charset; // character set
    quint16 m_wrap;         // wrap width in characters
    quint16 m_timestamps;   // show timestamps?
    quint16 m_match_width;  // match width?

    // ========== Colors ==========
    // ANSI colors 0-7 normal intensity
    QRgb m_normalcolour[8];
    // ANSI colors 0-7 bold/bright
    QRgb m_boldcolour[8];
    // Custom foreground colors
    QRgb m_customtext[MAX_CUSTOM];
    // Custom background colors
    QRgb m_customback[MAX_CUSTOM];
    // Custom color names
    QString m_strCustomColourName[255];

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
    quint16 m_enable_aliases;
    quint16 m_enable_triggers;
    quint16 m_bEnableTimers;

    // ========== Trigger/Alias/Timer Collections (SAVED TO DISK) ==========
    // These hold all user-created triggers, aliases, and timers
    // → Triggers and Aliases Data Structures
    std::map<QString, std::unique_ptr<Alias>>
        m_AliasMap;               // Map of aliases (name → unique_ptr<Alias>)
    QVector<Alias*> m_AliasArray; // Aliases sorted by sequence (non-owning)
    std::map<QString, std::unique_ptr<Trigger>>
        m_TriggerMap;                 // Map of triggers (name → unique_ptr<Trigger>)
    QVector<Trigger*> m_TriggerArray; // Triggers sorted by sequence (non-owning)
    // → Timer Data Structure
    std::map<QString, std::unique_ptr<Timer>>
        m_TimerMap;                      // Map of timers (name → unique_ptr<Timer>)
    QMap<Timer*, QString> m_TimerRevMap; // Reverse map (Timer* → name) for unlabelled timers
    bool m_triggersNeedSorting;          // Rebuild trigger array on next evaluation
    bool m_aliasesNeedSorting;           // Rebuild alias array on next evaluation

    // ========== Input Handling ==========
    quint16 m_display_my_input;
    quint16 m_echo_colour;
    quint16 m_bEscapeDeletesInput;
    quint16 m_bArrowsChangeHistory;
    quint16 m_bConfirmOnPaste;

    // ========== Command History ==========
    QStringList m_commandHistory;   // List of previous commands
    qint32 m_maxCommandHistory;     // Max history size (default 20)
    qint32 m_historyPosition;       // Current position when browsing
    bool m_bFilterDuplicates;       // Remove duplicate commands from history
    QString m_last_command;         // Last command sent (for consecutive duplicate check)
    HistoryStatus m_iHistoryStatus; // Current position status (eAtTop, eInMiddle, eAtBottom)

    // ========== Sound ==========
    quint16 m_enable_beeps;
    quint16 m_enable_trigger_sounds;
    QString m_new_activity_sound;
    QString m_strBeepSound;

    // ========== Macros (Function Keys) ==========
    QString m_macros[MACRO_COUNT];     // text for F1-F12
    quint16 m_macro_type[MACRO_COUNT]; // send types
    QString m_macro_name[MACRO_COUNT]; // macro names

    // ========== Numeric Keypad ==========
    QString m_keypad[KEYPAD_MAX_ITEMS]; // keypad strings
    quint16 m_keypad_enable;            // keypad enabled?

    // ========== Speed Walking ==========
    quint16 m_enable_speed_walk;
    QString m_speed_walk_prefix;
    QString m_strSpeedWalkFiller;
    quint16 m_iSpeedWalkDelay; // delay in ms

    // ========== Command Stack ==========
    quint16 m_enable_command_stack;
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
    quint16 m_bEnableScripts;          // scripting enabled?
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
    quint16 m_iUseMXP;              // MXP usage (see enum)
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
    quint16 m_indent_paras;
    quint16 m_bSaveWorldAutomatically;
    quint16 m_bLineInformation;
    quint16 m_bStartPaused;
    quint16 m_iNoteTextColour;
    quint16 m_bKeepCommandsOnSameLine;

    // ========== Auto-say Settings ==========
    QString m_strAutoSayString;
    quint16 m_bEnableAutoSay;
    quint16 m_bExcludeMacros;
    quint16 m_bExcludeNonAlpha;
    QString m_strOverridePrefix;
    quint16 m_bConfirmBeforeReplacingTyping;
    quint16 m_bReEvaluateAutoSay;

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
    qint32 m_nNormalPrintStyle[8]; // print style for normal colors
    qint32 m_nBoldPrintStyle[8];   // print style for bold colors

    // ========== Display Options (Version 9+) ==========
    quint16 m_bShowBold;               // show bold in fonts?
    quint16 m_bShowItalic;             // show italic?
    quint16 m_bShowUnderline;          // show underline?
    quint16 m_bAltArrowRecallsPartial; // alt+up recalls partial?
    quint16 m_iPixelOffset;            // pixel offset from window edge
    quint16 m_bAutoFreeze;             // freeze if not at bottom?
    quint16 m_bKeepFreezeAtBottom;     // don't unfreeze at bottom?
    quint16 m_bAutoRepeat;             // auto repeat last command?
    quint16 m_bDisableCompression;     // disable MCCP?
    quint16 m_bLowerCaseTabCompletion; // tab complete in lower case?
    quint16 m_bDoubleClickInserts;     // double-click inserts word?
    quint16 m_bDoubleClickSends;       // double-click sends to MUD?
    quint16 m_bConfirmOnSend;          // confirm preamble/postamble?
    quint16 m_bTranslateGerman;        // translate German chars?

    // ========== Tab Completion ==========
    QString m_strTabCompletionDefaults; // initial words
    quint32 m_iTabCompletionLines;      // lines to search
    quint16 m_bTabCompletionSpace;      // insert space after word?
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
    quint16 m_bPasteCommentedSoftcode; // paste commented softcode?
    quint16 m_bFileCommentedSoftcode;  // send commented softcode?
    quint16 m_bFlashIcon;              // flash icon for activity?
    quint16 m_bArrowKeysWrap;          // arrow keys wrap history?
    quint16 m_bSpellCheckOnSend;       // spell check on send?
    qint32 m_nPasteDelay;              // paste delay (ms)
    qint32 m_nFileDelay;               // file send delay (ms)
    qint32 m_nPasteDelayPerLines;      // lines before delay
    qint32 m_nFileDelayPerLines;       // lines before delay

    // ========== Miscellaneous Options ==========
    qint32 m_nReloadOption;                // script reload option
    qint32 m_bUseDefaultOutputFont;        // use default output font?
    qint32 m_bSaveDeletedCommand;          // save deleted command?
    qint32 m_bTranslateBackslashSequences; // interpret \n \r etc.?
    qint32 m_bEditScriptWithNotepad;       // use built-in notepad?
    qint32 m_bWarnIfScriptingInactive;     // warn if can't invoke script?

    // ========== Sending Options ==========
    quint16 m_bWriteWorldNameToLog; // write world name to log?
    quint16 m_bSendEcho;            // echo sends?
    quint16 m_bPasteEcho;           // echo pastes?

    // ========== Default Options ==========
    quint16 m_bUseDefaultColours;
    quint16 m_bUseDefaultTriggers;
    quint16 m_bUseDefaultAliases;
    quint16 m_bUseDefaultMacros;
    quint16 m_bUseDefaultTimers;
    quint16 m_bUseDefaultInputFont;

    // ========== Terminal Settings ==========
    QString m_strTerminalIdentification; // telnet negotiation ID

    // ========== Mapping ==========
    QString m_strMappingFailure; // mapping failure message
    quint16 m_bMapFailureRegexp; // mapping failure is regexp?

    // ========== Flag Containers ==========
    quint16 m_iFlags1; // misc flags (see FLAGS1_* defines)
    quint16 m_iFlags2; // more flags (see FLAGS2_* defines)

    // ========== World ID ==========
    QString m_strWorldID; // unique GUID for this world

    // ========== More Options (Version 15+) ==========
    quint16 m_bAlwaysRecordCommandHistory; // record even if echo off?
    quint16 m_bCopySelectionToClipboard;   // auto-copy selection?
    quint16 m_bCarriageReturnClearsLine;   // \r clears line?
    quint16 m_bSendMXP_AFK_Response;       // reply to <afk>?
    quint16 m_bMudCanChangeOptions;        // server may recommend?
    quint16 m_bEnableSpamPrevention;       // spam prevention?
    quint16 m_iSpamLineCount;              // spam line threshold
    QString m_strSpamMessage;              // spam filler message

    quint16 m_bDoNotShowOutstandingLines; // hide outstanding lines?
    quint16 m_bDoNotTranslateIACtoIACIAC; // don't translate IAC?

    // ========== Clipboard and Display ==========
    quint16 m_bAutoCopyInHTML;      // auto-copy in HTML?
    quint16 m_iLineSpacing;         // line spacing (0 = auto)
    quint16 m_bUTF_8;               // UTF-8 support?
    quint16 m_bConvertGAtoNewline;  // convert IAC/GA to newline?
    quint32 m_iCurrentActionSource; // what caused current script?

    // ========== Filters ==========
    QString m_strTriggersFilter;  // Lua filter for triggers
    QString m_strAliasesFilter;   // Lua filter for aliases
    QString m_strTimersFilter;    // Lua filter for timers
    QString m_strVariablesFilter; // Lua filter for variables

    // ========== Script Errors ==========
    quint16 m_bScriptErrorsToOutputWindow; // show errors in output?
    quint16 m_bLogScriptErrors;            // log script errors?

    // ========== Command Window Auto-resize ==========
    quint16 m_bAutoResizeCommandWindow;        // auto-resize command window?
    QString m_strEditorWindowName;             // editor window name
    quint16 m_iAutoResizeMinimumLines;         // minimum lines
    quint16 m_iAutoResizeMaximumLines;         // maximum lines
    quint16 m_bDoNotAddMacrosToCommandHistory; // macros not in history?
    quint16 m_bSendKeepAlives;                 // use SO_KEEPALIVE?

    // ========== Default Trigger Settings ==========
    quint16 m_iDefaultTriggerSendTo;
    quint16 m_iDefaultTriggerSequence;
    quint16 m_bDefaultTriggerRegexp;
    quint16 m_bDefaultTriggerExpandVariables;
    quint16 m_bDefaultTriggerKeepEvaluating;
    quint16 m_bDefaultTriggerIgnoreCase;

    // ========== Default Alias Settings ==========
    quint16 m_iDefaultAliasSendTo;
    quint16 m_iDefaultAliasSequence;
    quint16 m_bDefaultAliasRegexp;
    quint16 m_bDefaultAliasExpandVariables;
    quint16 m_bDefaultAliasKeepEvaluating;
    quint16 m_bDefaultAliasIgnoreCase;

    // ========== Default Timer Settings ==========
    quint16 m_iDefaultTimerSendTo;

    // ========== Sound ==========
    quint16 m_bPlaySoundsInBackground; // use global sound buffer?

    // ========== HTML Logging ==========
    quint16 m_bLogHTML;       // convert HTML sequences?
    quint16 m_bUnpauseOnSend; // cancel pause on send?

    // ========== Logging Options ==========
    quint16 m_log_input;    // log player input?
    quint16 m_bLogOutput;   // log MUD output?
    quint16 m_bLogNotes;    // log notes?
    quint16 m_bLogInColour; // HTML logging in colour?
    quint16 m_bLogRaw;      // log raw input from MUD?

    // ========== Tree Views ==========
    quint16 m_bTreeviewTriggers; // show triggers in tree?
    quint16 m_bTreeviewAliases;  // show aliases in tree?
    quint16 m_bTreeviewTimers;   // show timers in tree?

    // ========== Input Wrapping ==========
    quint16 m_bAutoWrapInput; // match input wrap to output?

    // ========== Tooltips ==========
    quint32 m_iToolTipVisibleTime; // tooltip visible time (ms)
    quint32 m_iToolTipStartTime;   // tooltip delay time (ms)

    // ========== Save File Options ==========
    quint16 m_bOmitSavedDateFromSaveFiles; // don't write save date?

    // ========== Output Buffer Fading ==========
    quint16 m_iFadeOutputBufferAfterSeconds; // fade after N seconds
    quint16 m_FadeOutputOpacityPercent;      // fade opacity %
    quint16 m_FadeOutputSeconds;             // fade duration
    quint16 m_bCtrlBackspaceDeletesLastWord; // Ctrl+Backspace behavior?

    // ========== Remote Access Server Settings ==========
    quint16 m_bEnableRemoteAccess;    // enable remote access server?
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
    quint16 m_remove_channels1;
    quint16 m_remove_channels2;
    quint16 m_remove_pages;
    quint16 m_remove_whispers;
    quint16 m_remove_set;
    quint16 m_remove_mail;
    quint16 m_remove_game;

    // ========== Runtime State Flags ==========
    bool m_bNAWS_wanted;          // server wants NAWS messages
    bool m_bCHARSET_wanted;       // server wants CHARSET messages
    bool m_bLoaded;               // true if loaded from disk
    bool m_bSelected;             // true if selected in Send To All Worlds
    bool m_bVariablesChanged;     // true if variables have changed
    bool m_bModified;             // true if document has unsaved changes
    bool m_bNoEcho;               // set if IAC WILL ECHO
    bool m_bDebugIncomingPackets; // display all incoming text?

    // ========== Statistics Counters ==========
    qint64 m_iInputPacketCount;        // count of packets received
    qint64 m_iOutputPacketCount;       // count of packets sent
    qint32 m_iUTF8ErrorCount;          // count of lines with bad UTF8
    qint32 m_iOutputWindowRedrawCount; // count of times output window redrawn

    // ========== UTF-8 Processing State ==========
    quint8 m_UTF8Sequence[8]; // collect up to 4 UTF8 bytes + null (expanded for safety)
    qint32 m_iUTF8BytesLeft;  // how many UTF8 bytes to go

    // ========== Trigger/Alias/Timer Statistics ==========
    qint32 m_iTriggersEvaluatedCount; // how many triggers evaluated
    qint32 m_iTriggersMatchedCount;   // how many triggers matched
    qint32 m_iAliasesEvaluatedCount;  // how many aliases evaluated
    qint32 m_iAliasesMatchedCount;    // how many aliases matched
    qint32 m_iTimersFiredCount;       // how many timers fired
    qint32 m_iTriggersMatchedThisSessionCount;
    qint32 m_iAliasesMatchedThisSessionCount;
    qint32 m_iTimersFiredThisSessionCount;

    // ========== UI State ==========
    qint32 m_last_prefs_page;         // last preferences page viewed
    quint16 m_bConfigEnableTimers;    // used during world config
    QString m_strLastSelectedTrigger; // last selected trigger
    QString m_strLastSelectedAlias;
    QString m_strLastSelectedTimer;
    QString m_strLastSelectedVariable;

    // ========== View Pointers (will be replaced with Qt equivalents) ==========
    class InputView* m_pActiveInputView; // Track active input widget
    OutputView* m_pActiveOutputView;     // OutputView for miniwindow signal connection

    // ========== Text Selection State ==========
    // Selection coordinates (0-based internally, converted to 1-based for Lua API)
    // -1 indicates no selection
    int m_selectionStartLine;
    int m_selectionStartChar;
    int m_selectionEndLine;
    int m_selectionEndChar;

    // ========== Line Buffer ==========
    QList<Line*> m_lineList;  // list of output buffer lines
    Line* m_currentLine;      // the line currently receiving
    QString m_strCurrentLine; // current line from MUD (no control codes)

    // ========== Multi-line Trigger Buffer ==========
    // Rolling buffer of recent line text for multi-line trigger matching
    // Based on original MUSHclient ProcessPreviousLine.cpp
    static const int MAX_RECENT_LINES = 200;
    std::deque<QString> m_recentLines;

    // ========== Actions List ==========
    // CActionList m_ActionList;
    void* m_ActionList; // placeholder - will be QList<Action*>

    // ========== Line Position Tracking ==========
    void* m_pLinePositions; // array of line positions
    qint32 m_total_lines;
    qint32 m_new_lines;           // lines they haven't read yet
    qint32 m_newlines_received;   // lines pushed into m_recentLines
    qint32 m_nTotalLinesSent;     // lines sent this connection
    qint32 m_nTotalLinesReceived; // lines received this connection
    qint32 m_last_line_with_IAC_GA;

    // ========== Timing Variables ==========
    QDateTime m_tConnectTime;               // when connected to world
    QDateTime m_tLastPlayerInput;           // last player input (for <afk>)
    qint64 m_tsConnectDuration;             // ms connected (use qint64 for QTimeSpan)
    QDateTime m_whenWorldStarted;           // when world document started
    qint64 m_whenWorldStartedHighPrecision; // high-res timestamp

    QDateTime m_tStatusTime;    // time of line mouse was over
    QPoint m_lastMousePosition; // where mouse last was

    qint32 m_view_number; // sequence in activity view

    // ========== Timer Evaluation (1-second check timer) ==========
    QTimer* m_timerCheckTimer = nullptr; // Periodic timer that calls checkTimers() every second

    // ========== Telnet Negotiation Phase ==========
    Phase m_phase;           // telnet negotiation phase (current parser state)
    qint32 m_ttype_sequence; // for MTTS (terminal type sequence)

    // ========== MCCP Compression ==========
    z_stream m_zCompress;    // zlib decompression state
    bool m_bCompress;        // true if decompressing
    bool m_bCompressInitOK;  // true if OK to decompress
    Bytef* m_CompressInput;  // input buffer (may not be needed)
    Bytef* m_CompressOutput; // output buffer for decompressed data
    qint64 m_nTotalUncompressed;
    qint64 m_nTotalCompressed;
    qint64 m_iCompressionTimeTaken; // time taken to decompress
    qint32 m_nCompressionOutputBufferSize;

    qint32 m_iMCCP_type; // 0=none, 1=v1, 2=v2
    bool m_bSupports_MCCP_2;

    // ========== Telnet Subnegotiation ==========
    qint32 m_subnegotiation_type;         // 0-255
    QByteArray m_IAC_subnegotiation_data; // last IAC SB c x IAC SE data

    // Telnet negotiation tracking arrays
    bool m_bClient_sent_IAC_DO[256];
    bool m_bClient_sent_IAC_DONT[256];
    bool m_bClient_sent_IAC_WILL[256];
    bool m_bClient_sent_IAC_WONT[256];
    bool m_bClient_got_IAC_DO[256];
    bool m_bClient_got_IAC_DONT[256];
    bool m_bClient_got_IAC_WILL[256];
    bool m_bClient_got_IAC_WONT[256];

    // ========== MSP (MUD Sound Protocol) ==========
    bool m_bMSP; // using MSP?

    // ========== ZMP (Zenith MUD Protocol) Runtime State ==========
    bool m_bZMP;             // ZMP negotiated and active
    QString m_strZMPpackage; // Current ZMP package name during collection

    // ========== ATCP (Achaea Telnet Client Protocol) Runtime State ==========
    bool m_bATCP; // ATCP negotiated and active

    // ========== MXP/Pueblo Runtime State ==========
    bool m_bMXP;                // using MXP now?
    bool m_bPuebloActive;       // Pueblo mode active
    QString m_iPuebloLevel;     // e.g., "1.0", "1.10"
    bool m_bPreMode;            // <PRE> tag active
    qint32 m_iMXP_mode;         // current tag security
    qint32 m_iMXP_defaultMode;  // default after newline
    qint32 m_iMXP_previousMode; // previous mode
    bool m_bInParagraph;        // discard newlines (wrap)
    bool m_bMXP_script;         // in script collection
    bool m_bSuppressNewline;    // newline doesn't start new line

    // MXP formatting state
    bool m_bMXP_nobr;           // <nobr> - non-breaking mode active
    bool m_bMXP_preformatted;   // <pre> - preformatted text mode
    bool m_bMXP_centered;       // <center> - centered text mode
    QString m_strMXP_link;      // current <send> or <a> href
    QString m_strMXP_hint;      // current link hint/tooltip
    bool m_bMXP_link_prompt;    // <send prompt> - show prompt before sending
    qint32 m_iMXP_list_depth;   // current list nesting depth
    qint32 m_iMXP_list_counter; // counter for ordered lists

    qint32 m_iListMode;  // what sort of list displaying
    qint32 m_iListCount; // for ordered list

    QString m_strMXPstring;      // currently collecting MXP string
    QString m_strMXPtagcontents; // stuff inside tag
    char m_cMXPquoteTerminator;  // ' or "

    // MXP Data Structures
    AtomicElementMap m_atomicElementMap; // Built-in MXP elements
    CustomElementMap m_customElementMap; // User-defined elements
    MXPEntityMap m_entityMap;            // Standard HTML entities
    MXPEntityMap m_customEntityMap;      // User-defined entities
    ActiveTagList m_activeTagList;       // Stack of unclosed tags
    MXPGaugeMap m_gaugeMap;              // Track gauges and stats by entity name

    char m_cLastChar; // last incoming character
    qint32 m_iLastOutstandingTagCount;
    QString m_strPuebloMD5; // Pueblo hash string

    // ========== MXP Statistics ==========
    qint64 m_iMXPerrors;
    qint64 m_iMXPtags;
    qint64 m_iMXPentities;

    // ========== ANSI State ==========
    qint32 m_code;        // current ANSI code
    qint32 m_lastGoTo;    // last line we went to
    bool m_bWorldClosing; // world is closing?

    // Current style (from MUD)
    quint16 m_iFlags;                        // style flags
    QRgb m_iForeColour;                      // foreground color
    QRgb m_iBackColour;                      // background color
    std::shared_ptr<Action> m_currentAction; // current hyperlink action (nullptr if no action)

    // Notes style
    bool m_bNotesInRGB; // notes in RGB mode?
    QRgb m_iNoteColourFore;
    QRgb m_iNoteColourBack;
    quint16 m_iNoteStyle; // HILITE, UNDERLINE, etc.

    // ========== Logging ==========
    QFile* m_logfile;          // Current open log file (nullptr if not logging)
    QString m_logfile_name;    // Resolved log filename
    QDateTime m_LastFlushTime; // Last time log was flushed to disk

    // ========== Fonts (will use QFont) ==========
    void* m_font[16]; // placeholder for QFont* array
    qint32 m_FontHeight;
    qint32 m_FontWidth;
    void* m_input_font; // placeholder for QFont*
    qint32 m_InputFontHeight;
    qint32 m_InputFontWidth;

    // ========== Byte Counters ==========
    qint64 m_nBytesIn;
    qint64 m_nBytesOut;

    // ========== Sockets (will use Qt network classes) ==========
    void* m_sockAddr;       // SOCKADDR_IN placeholder
    void* m_hNameLookup;    // HANDLE placeholder
    void* m_pGetHostStruct; // buffer placeholder
    qint32 m_iConnectPhase; // connection phase enum

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
    void* m_pThread;                // HANDLE placeholder
    void* m_eventScriptFileChanged; // CEvent placeholder
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
    qint32 m_dispidOnMXP_Start;
    qint32 m_dispidOnMXP_Stop;
    qint32 m_dispidOnMXP_OpenTag;
    qint32 m_dispidOnMXP_CloseTag;
    qint32 m_dispidOnMXP_SetVariable;
    qint32 m_dispidOnMXP_Error;

    // ========== Plugin State ==========
    bool m_bPluginProcessesOpenTag;
    bool m_bPluginProcessesCloseTag;
    bool m_bPluginProcessesSetVariable;
    bool m_bPluginProcessesSetEntity;
    bool m_bPluginProcessesError;

    // ========== Find Info Structures (will use Qt classes) ==========
    void* m_DisplayFindInfo; // CFindInfo placeholder
    void* m_RecallFindInfo;
    void* m_TriggersFindInfo;
    void* m_AliasesFindInfo;
    void* m_MacrosFindInfo;
    void* m_TimersFindInfo;
    void* m_VariablesFindInfo;
    void* m_NotesFindInfo;

    // Recall flags
    bool m_bRecallCommands;
    bool m_bRecallOutput;
    bool m_bRecallNotes;

    // ========== Document ID ==========
    qint64 m_iUniqueDocumentNumber;

    // ========== Mapping ==========
    void* m_strMapList;       // CStringList placeholder
    void* m_MapFailureRegexp; // t_regexp* placeholder
    QString m_strSpecialForwards;
    QString m_strSpecialBackwards;
    void* m_pTimerWnd;          // CTimerWnd* placeholder
    QStringList m_CommandQueue; // Command queue for speedwalking/delays
    bool m_bShowingMapperStatus;
    void* m_strIncludeFileList;        // CStringList placeholder
    void* m_strCurrentIncludeFileList; // CStringList placeholder

    // ========== Configuration Arrays ==========
    void* m_NumericConfiguration; // array placeholder
    void* m_AlphaConfiguration;   // array placeholder

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

    // ========== Arrays for scripting ==========
    // Named arrays for Lua ArrayCreate/ArrayGet/ArraySet etc.
    // Each array is a map of key->value pairs, organized by array name
    // Uses ArraysMap (QMap<QString, QMap<QString, QString>>) for consistency with Plugin
    ArraysMap m_Arrays;

    // ========== Special Fonts ==========
    void* m_strSpecialFontName; // ci_set placeholder

    // ========== Background Image ==========
    QString m_strBackgroundImageName;
    void* m_BackgroundBitmap; // CBitmap placeholder
    qint32 m_iBackgroundMode;
    QRgb m_iBackgroundColour;

    // ========== Foreground Image ==========
    QString m_strForegroundImageName;
    void* m_ForegroundBitmap; // CBitmap placeholder
    qint32 m_iForegroundMode;

    // ========== MiniWindows ==========
    std::map<QString, std::unique_ptr<MiniWindow>>
        m_MiniWindowMap;                 // Map of miniwindows (name → MiniWindow)
    QVector<QString> m_MiniWindowsOrder; // Z-order sorted list of miniwindow names

    // ========== Databases ==========
    void* m_Databases; // tDatabaseMap placeholder

    // ========== Text Rectangle ==========
    QRect m_TextRectangle; // Configuration (raw coordinates, may be negative for offsets)
    qint32 m_TextRectangleBorderOffset;
    qint32 m_TextRectangleBorderColour;
    qint32 m_TextRectangleBorderWidth;
    qint32 m_TextRectangleOutsideFillColour;
    qint32 m_TextRectangleOutsideFillStyle;
    QRect m_computedTextRectangle; // Computed/normalized rectangle (updated by OutputView)

    // ========== Sound System ==========
    // Qt Spatial Audio for stereo panning support
    QAudioEngine* m_audioEngine = nullptr;         // Single spatial audio engine
    QAudioListener* m_audioListener = nullptr;     // Single listener at origin
    SoundBuffer m_soundBuffers[MAX_SOUND_BUFFERS]; // Sound buffer array (10 buffers)

    // ========== Notepad Windows ==========
    QList<NotepadWidget*> m_notepadList; // List of notepad windows (non-owning)

    // ========== Accelerators ==========
    AcceleratorManager* m_acceleratorManager; // Keyboard shortcut management

    // ========== Color Translation ==========
    void* m_ColourTranslationMap; // map<COLORREF,COLORREF> placeholder

    // ========== Outstanding Lines ==========
    void* m_OutstandingLines; // list<CPaneStyle> placeholder
    bool m_bNotesNotWantedNow;
    bool m_bDoingSimulate;
    bool m_bLineOmittedFromOutput;
    bool m_bOmitCurrentLineFromLog; // Trigger omit_from_log flag
    bool m_bScrollBarWanted;

    // ========== IAC Counters ==========
    qint32 m_nCount_IAC_DO;
    qint32 m_nCount_IAC_DONT;
    qint32 m_nCount_IAC_WILL;
    qint32 m_nCount_IAC_WONT;
    qint32 m_nCount_IAC_SB;

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
    void ReceiveMsg();             // Called when socket has data to read
    void OnConnect(int errorCode); // Called when connection succeeds or fails

    // Connection control methods
    void connectToMud();                 // Initiate connection to MUD
    void disconnectFromMud();            // Disconnect from MUD
    void sendToMud(const QString& text); // Send text to MUD

    // ========== Telnet State Machine ==========

    // Main byte processor - routes incoming bytes to phase handlers
    void ProcessIncomingByte(unsigned char c);

    // Phase handlers (telnet_phases.cpp)
    void Phase_ESC(unsigned char c);                // Handle ESC character
    void Phase_ANSI(unsigned char c);               // Parse ANSI escape sequences
    void Phase_IAC(unsigned char& c);               // Handle IAC commands (note: reference!)
    void Phase_WILL(unsigned char c);               // Handle IAC WILL
    void Phase_WONT(unsigned char c);               // Handle IAC WONT
    void Phase_DO(unsigned char c);                 // Handle IAC DO
    void Phase_DONT(unsigned char c);               // Handle IAC DONT
    void Phase_SB(unsigned char c);                 // Start subnegotiation
    void Phase_SUBNEGOTIATION(unsigned char c);     // Collect subnegotiation data
    void Phase_SUBNEGOTIATION_IAC(unsigned char c); // Handle IAC within subnegotiation
    void Phase_UTF8(unsigned char c);               // Accumulate UTF-8 bytes
    void Phase_COMPRESS(unsigned char c);           // MCCP v1 start
    void Phase_COMPRESS_WILL(unsigned char c);      // MCCP v1 activation
    void Phase_MXP_ELEMENT(unsigned char c);        // Collect MXP element
    void Phase_MXP_COMMENT(unsigned char c);        // Collect MXP comment
    void Phase_MXP_QUOTE(unsigned char c);          // Collect quoted string in MXP
    void Phase_MXP_ENTITY(unsigned char c);         // Collect MXP entity

    // IAC negotiation methods (with loop prevention)
    void Send_IAC_DO(unsigned char c);
    void Send_IAC_DONT(unsigned char c);
    void Send_IAC_WILL(unsigned char c);
    void Send_IAC_WONT(unsigned char c);

    // Protocol-specific handlers (subnegotiation handlers)
    void Handle_TELOPT_COMPRESS2();     // MCCP v2
    void Handle_TELOPT_MXP();           // MXP on command
    void Handle_TELOPT_CHARSET();       // Character set negotiation
    void Handle_TELOPT_TERMINAL_TYPE(); // Terminal type/MTTS
    void Handle_TELOPT_ZMP();           // ZMP (Zenith MUD Protocol)
    void Handle_TELOPT_ATCP();          // ATCP (Achaea Telnet Client Protocol)
    void Handle_TELOPT_MSP();           // MSP (MUD Sound Protocol)

    // Support methods
    void SendPacket(const unsigned char* data, qint64 len);        // Send raw bytes
    bool Handle_Telnet_Request(int iNumber, const QString& sType); // Query plugins
    void Handle_IAC_GA();                                          // Handle Go Ahead
    void OutputBadUTF8characters();                                // Fallback for invalid UTF-8

    // ========== ANSI Parser ==========
    void InterpretANSIcode(int code);    // Process ANSI color/style codes
    void Interpret256ANSIcode(int code); // Process 256-color and 24-bit codes

    // ========== Style Management ==========
    Style* AddStyle(quint16 iFlags, QRgb iForeColour, QRgb iBackColour, quint16 iLength,
                    const std::shared_ptr<Action>& pAction);
    void RememberStyle(const Style* pStyle);
    void GetStyleRGB(const Style* pOldStyle, QRgb& iForeColour, QRgb& iBackColour);

    // ========== MXP and NAWS ==========
    void MXP_On();                    // MXP activation
    void MXP_Off(bool force = false); // MXP deactivation
    void MXP_mode_change(int mode);   // MXP mode changes

    // MXP Initialization
    void InitializeMXPElements(); // Load built-in MXP elements
    void InitializeMXPEntities(); // Load standard HTML entities
    void CleanupMXP();            // Free MXP resources

    // MXP Parsing
    void MXP_collected_element();                  // Process collected <element>
    void MXP_collected_entity();                   // Process collected &entity;
    void MXP_StartTag(const QString& tagString);   // Process opening tag
    void MXP_EndTag(const QString& tagName);       // Process closing tag
    void MXP_Definition(const QString& defString); // Process <!ELEMENT> or <!ENTITY>

    // MXP Element Management
    void MXP_DefineElement(const QString& defString);          // Define custom element
    void MXP_DefineEntity(const QString& defString);           // Define custom entity
    AtomicElement* MXP_FindAtomicElement(const QString& name); // Lookup built-in element
    CustomElement* MXP_FindCustomElement(const QString& name); // Lookup custom element

    // MXP Entity System
    QString MXP_GetEntity(const QString& entityName); // Resolve &entity; to text

    // MXP Execution
    void MXP_ExecuteAction(AtomicElement* elem,
                           const MXPArgumentList& args); // Execute element action
    void MXP_EndAction(int action);                      // End element action
    QRgb MXP_GetColor(const QString& colorSpec);         // Resolve color name or #RRGGBB

    // MXP Argument Parsing
    void ParseMXPTag(const QString& tagString, QString& tagName, MXPArgumentList& args);
    QString GetMXPArgument(const MXPArgumentList& args, const QString& name);
    QString MXP_GetArgument(const QString& name,
                            const MXPArgumentList& args); // Wrapper for GetMXPArgument
    bool MXP_HasArgument(const QString& name,
                         const MXPArgumentList& args); // Check if argument exists

    // MXP Tag Stack Management
    void MXP_CloseOpenTags();                  // Close all unclosed tags
    void MXP_CloseTag(const QString& tagName); // Close specific tag

    // MXP Mode Helpers
    bool MXP_Open() const;   // Check if in open mode
    bool MXP_Secure() const; // Check if in secure/locked mode

    void SendWindowSizes(int width); // NAWS

    // ========== Line Buffer Management ==========
    void AddLineToBuffer(Line* line);              // Add line to buffer with size limiting
    void AddToLine(const char* str, int len = -1); // Add text to current line
    void AddToLine(unsigned char c);               // Add single character to current line
    void StartNewLine(bool bNewLine,
                      unsigned char iFlags); // Complete current line and start new one

    // URL Detection and Linkification
    void DetectAndLinkifyURLs(Line* line); // Detect URLs and convert to hyperlinks
    void SplitStyleForURL(Line* line, size_t styleIdx,
                          int urlStart, // Split style to create URL hyperlink
                          int urlLength, const QString& url);

    // ========== Command Window Helpers ==========
    void setActiveInputView(InputView* inputView);
    InputView* activeInputView() const
    {
        return m_pActiveInputView;
    }

    QString GetCommand() const;
    qint32 SetCommand(const QString& text);
    qint32 SetCommandSelection(qint32 first, qint32 last);
    void SelectCommand();
    QString PushCommand();

    QStringList GetCommandQueue() const;
    qint32 Queue(const QString& message, bool echo);
    qint32 DiscardQueue();

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

    // ========== Lua Function Callbacks ==========
    void onWorldConnect();    // Call OnWorldConnect() if exists
    void onWorldDisconnect(); // Call OnWorldDisconnect() if exists

    // ========== Trigger and Alias Management ==========
    bool addTrigger(const QString& name,
                    std::unique_ptr<Trigger> trigger); // Add trigger to map and array
    bool deleteTrigger(const QString& name);           // Delete trigger by name
    Trigger* getTrigger(const QString& name);          // Get trigger by name
    bool addAlias(const QString& name,
                  std::unique_ptr<Alias> alias); // Add alias to map and array
    bool deleteAlias(const QString& name);       // Delete alias by name
    Alias* getAlias(const QString& name);        // Get alias by name

    // ========== Timer Management ==========
    bool addTimer(const QString& name, std::unique_ptr<Timer> timer); // Add timer to map
    bool deleteTimer(const QString& name);                            // Delete timer by name
    Timer* getTimer(const QString& name);                             // Get timer by name
    void calculateNextFireTime(Timer* timer); // Calculate when timer should next fire

    // ========== Trigger Pattern Matching ==========
    void evaluateTriggers(Line* line); // Evaluate triggers against completed line
    void rebuildTriggerArray();        // Rebuild trigger array sorted by sequence

    // ========== Trigger Execution and Actions ==========
    void executeTrigger(Trigger* trigger, Line* line,
                        const QString& matchedText); // Execute trigger action
    void executeTriggerScript(Trigger* trigger,
                              const QString& matchedText); // Execute Lua script callback
    QString replaceWildcards(const QString& text,
                             const QVector<QString>& wildcards); // Replace %1, %2, etc.
    void changeLineColors(Trigger* trigger, Line* line);         // Change matched line colors

    // ========== Alias Matching and Execution ==========
    bool evaluateAliases(const QString& command); // Check command against all aliases
    void rebuildAliasArray();                     // Rebuild alias array sorted by sequence
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
    void resetOneTimer(Timer* timer); // Calculate when timer should next fire
    void resetAllTimers();            // Reset all timers in m_TimerMap

    // ========== Timer Evaluation Loop ==========
    void checkTimers();    // Housekeeping and timer check (called every second)
    void checkTimerList(); // Find and execute ready timers

    // ========== Timer Execution ==========
    void executeTimer(Timer* timer, const QString& name);       // Execute a fired timer
    void executeTimerScript(Timer* timer, const QString& name); // Execute Lua script callback

    // ========== Plugin Timer Execution ==========
    void checkPluginTimerList(Plugin* plugin); // Find and execute ready plugin timers
    void executePluginTimer(Plugin* plugin, Timer* timer,
                            const QString& name); // Execute plugin timer
    void executePluginTimerScript(Plugin* plugin, Timer* timer,
                                  const QString& name); // Execute plugin timer script

    // ========== SendTo() - Central Action Routing ==========
    void sendTo(quint16 iWhere, const QString& strSendText, bool bOmitFromOutput, bool bOmitFromLog,
                const QString& strDescription, const QString& strVariable, QString& strOutput);

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
    // iType: 0 = MUD output, 1 = note (COMMENT), 2 = user input (USER_INPUT)
    // iLog: whether line should be logged
    void Screendraw(qint32 iType, bool iLog, const QString& sText);

    // ========== Trace Output ==========
    // Output trace message to display (if trace enabled)
    // If a plugin handles the trace, message is not displayed
    void Trace(const QString& message);

    // ========== Other stub methods ==========
    bool InitZlib(z_stream& zInfo); // MCCP
    void OnConnectionDisconnect();  // Disconnect handling
    void Repaint();                 // Trigger UI repaint (for miniwindows etc.)

    // ========== Sound System ==========
    void InitializeSoundSystem();               // Initialize all sound buffers
    void CleanupSoundSystem();                  // Stop and delete all sound buffers
    bool IsSoundBufferAvailable(qint16 buffer); // Check if buffer is free
    void ReleaseInactiveSoundBuffers();         // Update buffer states (release stopped buffers)
    qint32 PlaySound(qint16 buffer, const QString& filename, bool loop, double volume,
                     double pan);                // Play sound in specific buffer
    qint32 StopSound(qint16 buffer);             // Stop sound in buffer (0 = all)
    qint32 GetSoundStatus(qint16 buffer);        // Query status of sound buffer (1-based index)
    bool PlaySoundFile(const QString& filename); // Play sound in first available buffer
    void PlayMSPSound(const QString& filename, const QString& url, bool loop, double volume,
                      qint16 buffer);                 // Play MSP sound, downloading if necessary
    QString ResolveFilePath(const QString& filename); // Resolve relative/absolute sound file paths
    bool IsWindowActive();                            // Check if main window has focus

    // ========== Notepad Management ==========
    void RegisterNotepad(NotepadWidget* notepad);     // Register notepad with world
    void UnregisterNotepad(NotepadWidget* notepad);   // Unregister notepad from world
    NotepadWidget* FindNotepad(const QString& title); // Find notepad by title (case-insensitive)
    NotepadWidget* CreateNotepadWindow(const QString& title,
                                       const QString& contents); // Create new notepad window

    // ========== Notepad Operations (Lua API) ==========
    bool SendToNotepad(const QString& title,
                       const QString& contents); // Create/replace notepad text
    bool AppendToNotepad(const QString& title, const QString& contents); // Append to notepad
    bool ReplaceNotepad(const QString& title, const QString& contents);  // Replace notepad text
    bool ActivateNotepad(const QString& title); // Activate (raise/focus) notepad window
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
    bool MoveNotepadWindow(const QString& title, qint32 left, qint32 top, qint32 width,
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
    QFileSystemWatcher* m_scriptFileWatcher; // Watches script file for changes
};

#endif // WORLD_DOCUMENT_H
