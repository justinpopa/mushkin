#ifndef WORLD_DOCUMENT_H
#define WORLD_DOCUMENT_H

#include <QColor>
#include <QDateTime>
#include <QFile>              // std::unique_ptr<QFile> m_logfile
#include <QFileSystemWatcher> // QFileSystemWatcher* m_scriptFileWatcher
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
#include "output_formatter.h" // OutputFormatter companion object (owns note/colour/hyperlink output)
#include "telnet_parser.h"    // TelnetParser, Phase enum, telnet constants
#include "world_context.h"    // IWorldContext interface
#include "world_error.h"      // WorldError, WorldErrorType

#include "../automation/script_language.h" // ScriptLanguage enum
#include "../automation/sendto.h"          // SendTo enum
#include "../automation/variable.h"        // ArraysMap type
#include "../text/style.h" // Style flag bits (HILITE, UNDERLINE, BLINK, INVERSE, STRIKEOUT, COLOUR_*, COLOURTYPE, ACTIONTYPE, STYLE_BITS)

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
class MiniWindow;         // Off-screen drawing surface (miniwindow.h)
class SoundManager;       // Spatial audio state (sound_manager.h)
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

// DirectionInfo is defined in speedwalk_engine.h (included above)


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

class WorldDocument : public QObject, public IWorldContext {
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

    // ========== OutputFormatter (companion object) ==========
    // Owns note/colour/hyperlink output formatting logic.
    // Extracted from WorldDocument for composability.
    // All access via m_outputFormatter->method().
    std::unique_ptr<OutputFormatter> m_outputFormatter;

    // ========== Connection Settings ==========
    QString m_server;    // hostname or IP address
    QString m_mush_name; // world name
    QString m_name;      // player name
    QString m_password;  // player password
    quint16 m_port;      // port number (1-65535)
    bool m_connect_now;  // auto-connect flag (see enum above)

    // ========== Proxy Configuration ==========
    struct ProxyConfig {
        quint16 type = 0; // 0=None, 1=SOCKS5, 2=HTTP CONNECT
        QString server;   // proxy hostname
        quint16 port = 0; // proxy port (0 = disabled)
        QString username; // optional auth username
        QString password; // optional auth password
    } m_proxy;

    // ========== Display Configuration ==========
    struct DisplayConfig {
        // Core output display
        bool wrap = true;                        // word-wrap enabled
        bool timestamps = false;                 // show timestamps?
        bool match_width = false;                // match width?
        qint32 max_lines = 5000;                 // maximum scrollback lines
        quint16 wrap_column = 80;                // wrap column
        bool indent_paras = true;                // indent paragraphs?
        bool line_information = true;            // show line information?
        bool keep_commands_on_same_line = false; // keep commands on same line?

        // Text style display
        bool show_bold = true;      // show bold in fonts?
        bool show_italic = true;    // show italic?
        bool show_underline = true; // show underline?

        // Scrolling/freeze
        quint16 pixel_offset = 1;           // pixel offset from window edge
        bool auto_freeze = true;            // freeze if not at bottom?
        bool keep_freeze_at_bottom = false; // don't unfreeze at bottom?

        // Interaction
        bool double_click_inserts = false;        // double-click inserts word?
        bool copy_selection_to_clipboard = false; // auto-copy selection?
        bool auto_copy_in_html = false;           // auto-copy in HTML?

        // Spacing & encoding
        quint16 line_spacing = 0; // line spacing (0 = auto)
        bool utf8 = false;        // UTF-8 support?

        // Activity indicators
        bool flash_icon = false;                    // flash icon for activity?
        bool do_not_show_outstanding_lines = false; // hide outstanding lines?
    } m_display;

    // ========== Output Configuration ==========
    struct OutputConfig {
        // Font
        QString font_name = "Courier New"; // output font face name
        qint32 font_height = 12;           // font size in pixels
        qint32 font_weight = 400;          // bold/normal (400=normal, 700=bold)
        quint32 font_charset = 0;          // character set

        // Echo
        quint16 echo_colour = 65535;                  // SAMECOLOUR
        bool echo_hyperlink_in_output_window = false; // echo hyperlinks in output?

        // Output line preambles (timestamps)
        QString preamble_output;                        // preamble for MUD output lines
        QString preamble_input;                         // preamble for input lines
        QString preamble_notes;                         // preamble for note lines
        QRgb preamble_output_text_colour = 0x00FFFFFFu; // text color - output (white BGR)
        QRgb preamble_output_back_colour = 0x00000000u; // back color - output (black)
        QRgb preamble_input_text_colour = 0x00800000u;  // text color - input (dark blue BGR)
        QRgb preamble_input_back_colour = 0x00000000u;  // back color - input (black)
        QRgb preamble_notes_text_colour = 0x000000FFu;  // text color - notes (blue BGR)
        QRgb preamble_notes_back_colour = 0x00000000u;  // back color - notes (black)

        // Recall window
        QString recall_line_preamble; // line preamble for recall

        // Output buffer fading
        quint16 fade_after_seconds = 0;    // fade after N seconds (0 = disabled)
        quint16 fade_opacity_percent = 20; // fade opacity %
        quint16 fade_seconds = 8;          // fade duration
    } m_output;

    // ========== Color Configuration ==========
    struct ColorConfig {
        // ANSI colors 0-7 normal intensity (initialized by initializeColors())
        std::array<QRgb, 8> normal_colour{};
        // ANSI colors 0-7 bold/bright (initialized by initializeColors())
        std::array<QRgb, 8> bold_colour{};
        // Custom foreground colors (initialized by initializeColors())
        std::array<QRgb, MAX_CUSTOM> custom_text{};
        // Custom background colors (initialized by initializeColors())
        std::array<QRgb, MAX_CUSTOM> custom_back{};
        // Custom color names (initialized by initializeColors())
        std::array<QString, 255> custom_colour_name{};
        // Hyperlink color
        QRgb hyperlink_colour =
            0x000080FFu; // Light blue - BGR(255,128,0) = RGB(0,128,255) like original
        // Note text color index (4 = cyan, 65535 = SAMECOLOUR)
        quint16 note_text_colour = 4;
        // Use default colors from parent world?
        bool use_default_colours = false;
    } m_colors;

    // ========== Input Configuration ==========
    struct InputConfig {
        // Colors
        QRgb text_colour = 0x000000u;       // input text color (BGR) — black
        QRgb background_colour = 0xFFFFFFu; // input background color (BGR) — white

        // Font
        qint32 font_height = 12;           // font size in pixels
        QString font_name = "Courier New"; // input font face name
        quint8 font_italic = 0;            // italic flag (0=normal, 1=italic)
        qint32 font_weight = 400;          // font weight (400=normal, 700=bold)
        quint32 font_charset = 0;          // character set

        // History
        qint32 history_lines = 1000; // command history size

        // Behavior flags
        bool escape_deletes_input = false;      // Escape clears input?
        bool arrows_change_history = true;      // arrow keys navigate history?
        qint32 save_deleted_command = 0;        // save command on Escape?
        bool alt_arrow_recalls_partial = false; // Alt+Up recalls partial match?
        bool auto_wrap = false;                 // match input wrap to output?
        bool send_echo = false;                 // echo sends?
        bool no_echo_off = false;               // ignore echo-off from server?
        bool enable_command_stack = false;      // split commands on stack char?
        QString command_stack_character = ";";  // character to split commands
        bool double_click_sends = false;        // double-click sends to MUD?
        bool arrow_recalls_partial = false;     // Up/Down recalls partial match?
    } m_input;

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

    // ========== Command History ==========
    QStringList m_commandHistory;   // List of previous commands
    qint32 m_maxCommandHistory;     // Max history size (default 20)
    qint32 m_historyPosition;       // Current position when browsing
    bool m_bFilterDuplicates;       // Remove duplicate commands from history
    QString m_last_command;         // Last command sent (for consecutive duplicate check)
    HistoryStatus m_iHistoryStatus; // Current position status (eAtTop, eInMiddle, eAtBottom)

    // ========== Sound Configuration ==========
    struct SoundConfig {
        bool enable_beeps = true;          // enable beep on ^G?
        bool enable_trigger_sounds = true; // enable trigger sound actions?
        QString new_activity_sound;        // sound played on new activity
        QString beep_sound;                // sound file for beep (^G)
        bool play_in_background = false;   // use global sound buffer?
        bool use_msp = false;              // Use MSP (MUD Sound Protocol)
    } m_sound;

    // ========== Macros (Function Keys) ==========
    std::array<QString, MACRO_COUNT> m_macros;     // text for F1-F12
    std::array<quint16, MACRO_COUNT> m_macro_type; // send types
    std::array<QString, MACRO_COUNT> m_macro_name; // macro names

    // ========== Numeric Keypad ==========
    std::array<QString, KEYPAD_MAX_ITEMS> m_keypad; // keypad strings
    bool m_keypad_enable;                           // keypad enabled?

    // ========== Speed Walking ==========
    struct SpeedwalkConfig {
        bool enabled = false; // enable speedwalk?
        QString prefix;       // speedwalk prefix (e.g., "#")
        QString filler;       // filler command ('F' token)
        quint16 delay = 0;    // delay between commands (ms)
    } m_speedwalk;

    // ========== Connection Text ==========
    QString m_connect_text;

    // ========== Paste/Send File Configuration ==========
    struct PasteSendConfig {
        // Paste text framing
        QString paste_preamble;      // sent before pasted block
        QString paste_postamble;     // sent after pasted block
        QString pasteline_preamble;  // sent before each pasted line
        QString pasteline_postamble; // sent after each pasted line

        // Send-file text framing
        QString file_preamble;  // sent before file block
        QString file_postamble; // sent after file block
        QString line_preamble;  // sent before each file line
        QString line_postamble; // sent after each file line

        // Paste behavior
        bool paste_commented_softcode = false; // strip softcode comments when pasting?
        bool paste_echo = false;               // echo pasted lines?
        bool confirm_on_paste = true;          // confirm before paste?
        qint32 paste_delay = 0;                // delay between paste lines (ms)
        qint32 paste_delay_per_lines = 1;      // lines between paste delays

        // Send-file behavior
        bool file_commented_softcode = false; // strip softcode comments when sending file?
        qint32 file_delay = 0;                // delay between file lines (ms)
        qint32 file_delay_per_lines = 1;      // lines between file delays
    } m_paste;

    // ========== World Notes ==========
    QString m_notes;

    // ========== Scripting Configuration ==========
    struct ScriptConfig {
        // Core settings
        QString language = "Lua"; // script language (e.g., "Lua")
        bool enabled = true;      // scripting enabled?
        QString filename;         // script file path
        QString prefix = "/";     // script invocation prefix
        QString editor;           // editor path
        QString editor_argument;  // editor arguments

        // Event handlers
        QString on_world_open;       // handler on world open
        QString on_world_close;      // handler on world close
        QString on_world_save;       // handler on world save
        QString on_world_connect;    // handler on connect
        QString on_world_disconnect; // handler on disconnect
        QString on_world_get_focus;  // handler on focus gain
        QString on_world_lose_focus; // handler on focus loss

        // Reload & error handling
        ScriptReloadOption reload_option = ScriptReloadOption::eReloadConfirm; // reload behavior
        bool errors_to_output_window = false; // show errors in output?

        // Execution options (kept as qint32 for integer-valued serialization compat)
        qint32 edit_with_notepad = 1; // use built-in notepad?
        qint32 warn_if_inactive = 1;  // warn if can't invoke script?

        // Misc
        bool tab_complete_functions = true; // show Lua functions in Shift+Tab menu

        // Filters
        QString triggers_filter;  // Lua filter for triggers
        QString aliases_filter;   // Lua filter for aliases
        QString timers_filter;    // Lua filter for timers
        QString variables_filter; // Lua filter for variables
    } m_scripting;

    // ========== MXP (MUD Extension Protocol) ==========
    MXPMode m_iUseMXP;              // MXP usage (see enum)
    quint16 m_iMXPdebugLevel;       // MXP debug level
    QString m_strOnMXP_Start;       // MXP starting
    QString m_strOnMXP_Stop;        // MXP stopping
    QString m_strOnMXP_Error;       // MXP error
    QString m_strOnMXP_OpenTag;     // MXP tag open
    QString m_strOnMXP_CloseTag;    // MXP tag close
    QString m_strOnMXP_SetVariable; // MXP variable set

    // ========== Miscellaneous Flags ==========
    bool m_bSaveWorldAutomatically;
    bool m_bStartPaused;

    // ========== Auto-say Settings ==========
    struct AutoSayConfig {
        QString say_string = "say ";          // string prepended to commands
        QString override_prefix = "-";        // prefix to bypass auto-say
        bool enabled = false;                 // auto-say mode enabled?
        bool exclude_macros = false;          // skip macro/accelerator keys?
        bool exclude_non_alpha = false;       // skip commands not starting with a letter?
        bool confirm_before_replacing = true; // confirm before replacing typed text?
        bool re_evaluate = false;             // re-evaluate after alias expansion?
    } m_auto_say;

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
    bool m_bAutoRepeat;         // auto repeat last command?
    bool m_bDisableCompression; // disable MCCP?
    bool m_bConfirmOnSend;      // confirm preamble/postamble?
    bool m_bTranslateGerman;    // translate German chars?

    // ========== Command Window Configuration ==========
    struct CommandWindowConfig {
        // Auto-resize
        bool auto_resize = false;   // auto-resize command window?
        quint16 minimum_lines = 1;  // minimum lines when auto-resizing
        quint16 maximum_lines = 20; // maximum lines when auto-resizing

        // History behavior
        bool no_macros_in_history = false; // macros not added to history?

        // Tab completion
        bool lower_case_tab_completion = false; // tab complete in lower case?
        QString tab_completion_defaults;        // initial words
        quint32 tab_completion_lines = 200;     // lines to search
        bool tab_completion_space = false;      // insert space after word?
        QString word_delimiters = "-._~!@#$%^&*()+=[]{}\\|;:'\",<>?/"; // word delimiters

        // Miscellaneous sending
        bool arrow_keys_wrap = false;     // arrow keys wrap history?
        bool spell_check_on_send = false; // spell check on send?
    } m_command_window;

    // Shift+Tab completion (Lua API controlled)
    QSet<QString> m_ExtraShiftTabCompleteItems; // extra items for Shift+Tab menu
    // tab_complete_functions moved to m_scripting.tab_complete_functions

    // ========== Auto Logging ==========
    // (fields moved to LoggingConfig m_logging below)
    // Note: output preambles and recall preamble moved to OutputConfig m_output

    // ========== Miscellaneous Options ==========
    // m_nReloadOption moved to m_scripting.reload_option
    // m_bEditScriptWithNotepad moved to m_scripting.edit_with_notepad
    // m_bWarnIfScriptingInactive moved to m_scripting.warn_if_inactive
    qint32 m_bUseDefaultOutputFont;        // use default output font?
    qint32 m_bTranslateBackslashSequences; // interpret \n \r etc.?

    // ========== Default Options ==========
    bool m_bUseDefaultTriggers;
    bool m_bUseDefaultAliases;
    bool m_bUseDefaultMacros;
    bool m_bUseDefaultTimers;
    bool m_bUseDefaultInputFont;

    // ========== Terminal Settings ==========
    QString m_strTerminalIdentification; // telnet negotiation ID

    // ========== Flag Containers ==========
    quint16 m_iFlags1; // misc flags (see FLAGS1_* defines)
    quint16 m_iFlags2; // more flags (see FLAGS2_* defines)

    // ========== World ID ==========
    QString m_strWorldID; // unique GUID for this world

    // ========== More Options (Version 15+) ==========
    bool m_bAlwaysRecordCommandHistory; // record even if echo off?
    bool m_bSendMXP_AFK_Response;       // reply to <afk>?
    bool m_bMudCanChangeOptions;        // server may recommend?

    // ========== Clipboard and Display ==========
    ActionSource m_iCurrentActionSource; // what caused current script?

    // ========== Filters ==========
    // Moved to m_scripting: triggers_filter, aliases_filter, timers_filter, variables_filter

    // ========== Script Errors ==========
    // Moved to m_scripting.errors_to_output_window

    // ========== Editor Window ==========
    QString m_strEditorWindowName; // editor window name
    bool m_bSendKeepAlives;        // use SO_KEEPALIVE?

    // ========== Automation Defaults Configuration ==========
    struct AutomationDefaultsConfig {
        // Default trigger settings
        quint16 trigger_send_to = 0;           // default send-to for new triggers
        quint16 trigger_sequence = 100;        // default sequence for new triggers
        bool trigger_regexp = false;           // default regexp for new triggers?
        bool trigger_expand_variables = false; // default expand vars for new triggers?
        bool trigger_keep_evaluating = false;  // default keep evaluating for new triggers?
        bool trigger_ignore_case = false;      // default ignore case for new triggers?

        // Default alias settings
        quint16 alias_send_to = 0;           // default send-to for new aliases
        quint16 alias_sequence = 100;        // default sequence for new aliases
        bool alias_regexp = false;           // default regexp for new aliases?
        bool alias_expand_variables = false; // default expand vars for new aliases?
        bool alias_keep_evaluating = false;  // default keep evaluating for new aliases?
        bool alias_ignore_case = false;      // default ignore case for new aliases?

        // Default timer settings
        quint16 timer_send_to = 0; // default send-to for new timers
    } m_automation_defaults;

    // ========== HTML Logging ==========
    bool m_bUnpauseOnSend; // cancel pause on send?

    // ========== Logging Configuration ==========
    struct LoggingConfig {
        // Log file settings
        QString auto_log_file_name;    // auto-log filename
        QString file_preamble;         // log file preamble
        QString file_postamble;        // log file postamble
        QString line_preamble_output;  // log preamble for output
        QString line_preamble_input;   // log preamble for input
        QString line_preamble_notes;   // log preamble for notes
        QString line_postamble_output; // log postamble for output
        QString line_postamble_input;  // log postamble for input
        QString line_postamble_notes;  // log postamble for notes

        // Logging flags
        bool log_html = false;          // convert HTML sequences?
        bool log_input = false;         // log player input?
        bool log_output = false;        // log MUD output?
        bool log_notes = false;         // log notes?
        bool log_in_colour = false;     // HTML logging in colour?
        bool log_raw = false;           // log raw input from MUD?
        bool write_world_name = false;  // write world name to log?
        bool log_script_errors = false; // log script errors?
        bool append_to_log_file = true; // append vs overwrite
        int log_lines = 0;              // max lines (0 = unlimited)
    } m_logging;

    // ========== Tree Views ==========
    bool m_bTreeviewTriggers; // show triggers in tree?
    bool m_bTreeviewAliases;  // show aliases in tree?
    bool m_bTreeviewTimers;   // show timers in tree?

    // ========== Tooltips ==========
    quint32 m_iToolTipVisibleTime; // tooltip visible time (ms)
    quint32 m_iToolTipStartTime;   // tooltip delay time (ms)

    // ========== Save File Options ==========
    bool m_bOmitSavedDateFromSaveFiles; // don't write save date?

    // Note: output buffer fading fields moved to OutputConfig m_output
    bool m_bCtrlBackspaceDeletesLastWord; // Ctrl+Backspace behavior?

    // ========== Remote Access Server Settings ==========
    struct RemoteAccessConfig {
        bool enabled = false;           // enable remote access server?
        quint16 port = 0;               // port to listen on (0 = disabled)
        QString password;               // password for authentication (required)
        quint16 scrollback_lines = 100; // lines to send on connect
        quint16 max_clients = 5;        // max simultaneous clients
        quint16 lockout_attempts = 3;   // failed attempts before lockout
        quint16 lockout_seconds = 300;  // lockout duration
    } m_remote;

    // ========== Spam Prevention / Protocol Behavior Configuration ==========
    struct SpamPreventionConfig {
        bool enabled = false;                     // spam prevention?
        quint16 line_count = 20;                  // spam line threshold
        QString message = "look";                 // spam filler message
        bool carriage_return_clears_line = false; // \r clears line?
        bool convert_ga_to_newline = false;       // convert IAC/GA to newline?
        bool do_not_translate_iac = false;        // don't translate IAC?
    } m_spam;

    // ========== Mapping Configuration ==========
    struct MappingConfig {
        bool enabled = false;        // mapping active?
        bool remove_reverses = true; // auto-cancel opposite directions?
        QString failure_message = "Alas, you cannot go that way."; // movement failure text
        bool failure_regexp = false;                               // failure_message is regexp?
        QString special_forwards;                                  // recorded forward directions
        QString special_backwards;                                 // recorded backward directions
    } m_mapping;

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
    // NOTE: m_CommandQueue moved to ConnectionManager.
    // Access via m_connectionManager->m_CommandQueue.
    bool m_bShowingMapperStatus;

    // Mapper state (Lua API: AddToMapper, GetMappingString, etc.)
    QStringList m_mapList;                   // Ordered list of map entries (directions/comments)
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
    bool m_bCtrlZGoesToEndOfBuffer;
    bool m_bCtrlPGoesToPreviousCommand;
    bool m_bCtrlNGoesToNextCommand;
    bool m_bHyperlinkAddsToCommandHistory;
    // echo_hyperlink_in_output_window moved to m_output.echo_hyperlink_in_output_window
    bool m_bAutoWrapWindowWidth;
    bool m_bNAWS;    // Negotiate About Window Size
    bool m_bUseZMP;  // Use ZMP (Zenith MUD Protocol)
    bool m_bUseATCP; // Use ATCP (Achaea Telnet Client Protocol)
    bool m_bPueblo;  // Allow Pueblo
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

    // ConnectionManager state accessors
    [[nodiscard]] qint32 connectPhase() const
    {
        return m_connectionManager->m_iConnectPhase;
    }
    [[nodiscard]] qint64 bytesIn() const
    {
        return m_connectionManager->m_nBytesIn;
    }
    [[nodiscard]] qint64 bytesOut() const
    {
        return m_connectionManager->m_nBytesOut;
    }
    [[nodiscard]] qint32 totalLinesSent() const
    {
        return m_connectionManager->m_nTotalLinesSent;
    }
    [[nodiscard]] qint64 inputPacketCount() const
    {
        return m_connectionManager->m_iInputPacketCount;
    }
    [[nodiscard]] qint64 outputPacketCount() const
    {
        return m_connectionManager->m_iOutputPacketCount;
    }

    // TelnetParser state accessors
    [[nodiscard]] bool isEchoOff() const
    {
        return m_telnetParser->m_bNoEcho;
    }
    [[nodiscard]] bool isCompressing() const
    {
        return m_telnetParser->m_bCompress;
    }
    [[nodiscard]] qint32 mccpType() const
    {
        return m_telnetParser->m_iMCCP_type;
    }
    [[nodiscard]] qint64 totalUncompressed() const
    {
        return m_telnetParser->m_nTotalUncompressed;
    }
    [[nodiscard]] qint64 totalCompressed() const
    {
        return m_telnetParser->m_nTotalCompressed;
    }

    // MXPEngine state accessors
    [[nodiscard]] bool isMXPActive() const
    {
        return m_mxpEngine->m_bMXP;
    }
    [[nodiscard]] bool isPuebloActive() const
    {
        return m_mxpEngine->m_bPuebloActive;
    }
    [[nodiscard]] qint64 mxpErrorCount() const
    {
        return m_mxpEngine->m_iMXPerrors;
    }
    [[nodiscard]] qint64 mxpTagCount() const
    {
        return m_mxpEngine->m_iMXPtags;
    }
    [[nodiscard]] qint64 mxpEntityCount() const
    {
        return m_mxpEngine->m_iMXPentities;
    }

    // AutomationRegistry statistics accessors
    [[nodiscard]] qint32 triggersEvaluatedCount() const
    {
        return m_automationRegistry->m_iTriggersEvaluatedCount;
    }
    [[nodiscard]] qint32 triggersMatchedCount() const
    {
        return m_automationRegistry->m_iTriggersMatchedCount;
    }
    [[nodiscard]] qint32 aliasesEvaluatedCount() const
    {
        return m_automationRegistry->m_iAliasesEvaluatedCount;
    }
    [[nodiscard]] qint32 aliasesMatchedCount() const
    {
        return m_automationRegistry->m_iAliasesMatchedCount;
    }
    [[nodiscard]] qint32 timersFiredCount() const
    {
        return m_automationRegistry->m_iTimersFiredCount;
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
    IOutputView* activeOutputView() const override
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

    // Forwarding wrappers — implementation lives in OutputFormatter (output_formatter.cpp)
    void note(const QString& text)
    {
        m_outputFormatter->note(text);
    }
    void colourNote(QRgb foreColor, QRgb backColor, const QString& text)
    {
        m_outputFormatter->colourNote(foreColor, backColor, text);
    }
    void colourTell(QRgb foreColor, QRgb backColor, const QString& text)
    {
        m_outputFormatter->colourTell(foreColor, backColor, text);
    }
    void hyperlink(const QString& action, const QString& text, const QString& hint, QRgb foreColor,
                   QRgb backColor, bool isUrl)
    {
        m_outputFormatter->hyperlink(action, text, hint, foreColor, backColor, isUrl);
    }
    void simulate(const QString& text)
    {
        m_outputFormatter->simulate(text);
    }
    void noteHr()
    {
        m_outputFormatter->noteHr();
    }

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
                        const QString& matchedText) override; // Execute trigger action
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
    void executeAlias(Alias* alias, const QString& command) override; // Execute alias action
    void executeAliasScript(Alias* alias, const QString& command);    // Execute Lua script callback

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
    void executeTimer(Timer* timer, const QString& name) override; // Execute a fired timer
    void executeTimerScript(Timer* timer, const QString& name);    // Execute Lua script callback

    // ========== Plugin Timer Execution ==========
    void checkPluginTimerList(Plugin* plugin)
    {
        m_automationRegistry->checkPluginTimerList(plugin);
    }
    void executePluginTimer(Plugin* plugin, Timer* timer,
                            const QString& name) override; // Execute plugin timer
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
    void setCurrentPlugin(Plugin* plugin) override
    {
        m_CurrentPlugin = plugin;
    } // Set currently executing plugin

    // ========== MiniWindow Management ==========
    qint32 WindowFont(const QString& windowName, const QString& fontId, const QString& fontName,
                      double size, bool bold, bool italic, bool underline, bool strikeout,
                      qint16 charset, qint16 pitchAndFamily); // Add font to miniwindow

    // ========== Plugin Callbacks ==========
    void
    SendToAllPluginCallbacks(const QString& callbackName) override; // Call all plugins (no args)
    bool SendToAllPluginCallbacks(const QString& callbackName, const QString& arg,
                                  bool bStopOnFalse = false) override; // Call all plugins (1 arg)
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

    // ========== IWorldContext Implementation ==========
    const std::vector<std::unique_ptr<Plugin>>& pluginList() const override
    {
        return m_PluginList;
    }
    Plugin* currentPlugin() const override
    {
        return m_CurrentPlugin;
    }
    bool triggersEnabled() const override
    {
        return m_enable_triggers;
    }
    bool timersEnabled() const override
    {
        return m_bEnableTimers;
    }
    const std::deque<QString>& recentLines() const override
    {
        return m_recentLines;
    }
    void unregisterNotepad(NotepadWidget* notepad) override
    {
        UnregisterNotepad(notepad);
    }
    bool isConnectedToMud() const override;
    void flushLogIfNeeded() override;

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
