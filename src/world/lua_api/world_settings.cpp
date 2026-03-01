/**
 * world_settings.cpp - World Settings and Info Lua API Functions
 *
 * Miniwindow System - Settings Module
 *
 * This file implements world settings, info, and UI control functions.
 * Extracted from lua_methods.cpp for better code organization.
 */

#include "../../automation/plugin.h"
#include "../../network/world_socket.h"
#include "../../storage/database.h"
#include "../../storage/global_options.h"
#include "../../utils/app_paths.h"
#include "../../world/config_options.h"
#include "../../world/view_interfaces.h"
#include "../../world/world_document.h"
#include "../lua_dialog_callbacks.h"
#include "../script_engine.h"
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QSysInfo>
#include <algorithm>
#include <sqlite3.h>
#include <string>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "logging.h"
#include "lua_common.h"

// ========== Font Management ==========

/**
 * world.AddFont(pathname)
 *
 * Adds a font file to the application's font database.
 *
 * @param pathname Path to font file (TTF, OTF, etc.)
 * @return Error code (eOK on success, eFileNotFound if font couldn't be loaded)
 *
 * Example: AddFont("fonts/Dina.ttf")
 */
int L_AddFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* pathname = luaL_checkstring(L, 1);

    // Check for empty path
    if (!pathname || pathname[0] == '\0') {
        return luaReturnError(L, eBadParameter);
    }

    QString fontPath = QString::fromUtf8(pathname);

    // Try to load the font using QFontDatabase
    int fontId = QFontDatabase::addApplicationFont(fontPath);

    if (fontId == -1) {
        // Font couldn't be loaded
        return luaReturnError(L, eFileNotFound);
    }

    // Success - font is now available
    // QFontDatabase automatically makes it available to the application
    return luaReturnOK(L);
}

// ========== World Info Functions ==========

/**
 * world.GetInfo(type)
 *
 * Returns information about the current world, connection, or client.
 *
 * String info types (1-89):
 * - 1: Server address (hostname/IP)
 * - 2: World name
 * - 3: Character name
 * - 4: Logging file preamble
 * - 5: Logging file postamble
 * - 6: Logging line preamble
 * - 7: Logging line postamble
 * - 8: Notes
 * - 9: New activity sound
 * - 10: Script editor
 * - 11: Log file preamble
 * - 12: Log file postamble
 * - 13: Log line preamble (input)
 * - 14: Log line preamble (notes)
 * - 15: Log line preamble (output)
 * - 16: Log line postamble (input)
 * - 17: Log line postamble (notes)
 * - 18: Log line postamble (output)
 * - 19: Speed walk filler
 * - 20: Output font name
 * - 21: Speed walk prefix
 * - 22: Connect text
 * - 23: Input font name
 * - 24: Paste postamble
 * - 25: Paste preamble
 * - 26: Paste line postamble
 * - 27: Paste line preamble
 * - 28: Script language
 * - 29: OnWorldOpen callback
 * - 30: OnWorldClose callback
 * - 31: OnWorldConnect callback
 * - 32: OnWorldDisconnect callback
 * - 33: OnWorldGetFocus callback
 * - 34: OnWorldLoseFocus callback
 * - 35: Script filename
 * - 36: Script prefix
 * - 37: Auto-say string
 * - 38: Override prefix
 * - 39: Tab completion defaults
 * - 40: Auto-log filename
 * - 41: Recall line preamble
 * - 42: Terminal identification
 * - 43: Mapping failure message
 * - 44: OnMXP_Start callback
 * - 45: OnMXP_Stop callback
 * - 46: OnMXP_Error callback
 * - 47: OnMXP_OpenTag callback
 * - 48: OnMXP_CloseTag callback
 * - 49: OnMXP_SetVariable callback
 * - 50: Beep sound
 * - 51: Log filename
 * - 52: Last immediate expression
 * - 53: Status message
 * - 54: World file path
 * - 55: Window title
 * - 56: Application executable path
 * - 57: Default world file directory
 * - 58: Default log file directory
 * - 59: Scripts directory
 * - 60: Plugins directory (global)
 * - 61: IP address from socket connection
 * - 62: Proxy server (removed)
 * - 63: Hostname
 * - 64: Current directory with trailing slash
 * - 65: OnWorldSave callback
 * - 66: Application directory
 * - 67: World file directory
 * - 68: Working directory
 * - 69: Translator file
 * - 70: Locale
 * - 71: Fixed pitch font
 * - 72: Version string
 * - 73: Build date/time
 * - 74: Sounds directory
 * - 75: IAC subnegotiation data
 * - 76: Special font
 * - 77: OS version
 * - 78: Foreground image
 * - 79: Background image
 * - 80: PNG library version
 * - 81: PNG header version
 * - 82: Preferences database name
 * - 83: SQLite version
 * - 84: File browsing directory
 * - 85: Default state files directory
 * - 86: Word under menu
 * - 87: Last command sent
 * - 88: Window title (individual world)
 * - 89: Main window title
 *
 * Boolean flags (101-125):
 * - 101: No echo (IAC WILL ECHO received)
 * - 102: Debug incoming packets
 * - 103: Compress (MCCP active)
 * - 104: MXP active
 * - 105: Pueblo active
 * - 106: Not connected
 * - 107: Connecting
 * - 108: Disconnect OK
 * - 109: Trace enabled
 * - 110: Script file changed
 * - 111: World modified
 * - 112: Mapping enabled
 * - 113: Window open
 * - 114: Current view frozen
 * - 115: Translator Lua available
 * - 118: Variables changed
 * - 119: Script engine exists
 * - 120: Scroll bar wanted
 * - 121: Performance counter available
 * - 122: SQLite threadsafe
 * - 123: Doing simulate
 * - 124: Line omitted from output
 * - 125: Full screen mode
 *
 * Counts and dimensions (201-310):
 * - 201: Total lines received
 * - 202: New lines (unread)
 * - 203: Total lines sent
 * - 204: Input packet count
 * - 205: Output packet count
 * - 206: Total uncompressed bytes (MCCP)
 * - 207: Total compressed bytes (MCCP)
 * - 208: MCCP type (0=none, 1=v1, 2=v2)
 * - 209: MXP errors
 * - 210: MXP tags processed
 * - 211: MXP entities processed
 * - 212: Output font height
 * - 213: Output font width
 * - 214: Input font height
 * - 215: Input font width
 * - 216: Bytes received
 * - 217: Bytes sent
 * - 218: Count of variables
 * - 219: Count of triggers
 * - 220: Count of timers
 * - 221: Count of aliases
 * - 222: Count of queued commands
 * - 223: Count of mapper items
 * - 224: Count of lines in output window
 * - 225: Count of custom MXP elements
 * - 226: Count of custom MXP entities
 * - 227: Connection phase
 * - 228: IP address (as integer)
 * - 229: Proxy type (always 0)
 * - 230: Execution depth (script call depth)
 * - 231: Log file size
 * - 232: High-resolution timer (seconds since epoch)
 * - 233: Time doing triggers (seconds)
 * - 234: Time doing aliases (seconds)
 * - 235: Number of world windows open
 * - 236: Command selection start column
 * - 237: Command selection end column
 * - 238: Window placement flags
 * - 239: Current action source
 * - 240: Average character width
 * - 241: Character height
 * - 242: UTF-8 error count
 * - 243: Fixed pitch font size
 * - 244: Triggers evaluated count
 * - 245: Triggers matched count
 * - 246: Aliases evaluated count
 * - 247: Aliases matched count
 * - 248: Timers fired count
 * - 249: Main window client height
 * - 250: Main window client width
 * - 251: Main toolbar height
 * - 252: Main toolbar width
 * - 253: Game toolbar height
 * - 254: Game toolbar width
 * - 255: Activity toolbar height
 * - 256: Activity toolbar width
 * - 257: Info bar height
 * - 258: Info bar width
 * - 259: Status bar height
 * - 260: Status bar width
 * - 261: World window non-client height
 * - 262: World window non-client width
 * - 263: World window client height
 * - 264: World window client width
 * - 265: OS major version
 * - 266: OS minor version
 * - 267: OS build number
 * - 268: OS platform ID
 * - 269: Foreground mode
 * - 270: Background mode
 * - 271: Background colour
 * - 272: Text rectangle left
 * - 273: Text rectangle top
 * - 274: Text rectangle right
 * - 275: Text rectangle bottom
 * - 276: Text rectangle border offset
 * - 277: Text rectangle border width
 * - 278: Text rectangle outside fill colour
 * - 279: Text rectangle outside fill style
 * - 280: Output window client height
 * - 281: Output window client width
 * - 282: Text rectangle border colour
 * - 283: Last mouse X position
 * - 284: Last mouse Y position
 * - 285: Is output window available
 * - 286: Triggers matched this session
 * - 287: Aliases matched this session
 * - 288: Timers fired this session
 * - 289: Last line with IAC GA
 * - 290: Actual text rectangle left
 * - 291: Actual text rectangle top
 * - 292: Actual text rectangle right
 * - 293: Actual text rectangle bottom
 * - 294: Scroll bar max position
 * - 295: Scroll bar page size
 * - 296: Output window scroll bar position
 * - 297: Horizontal scroll bar position
 * - 298: Horizontal scroll bar max position
 * - 299: Horizontal scroll bar page size
 * - 300: Commands in history buffer
 * - 301: Number of sent packets
 * - 302: Connect time (seconds since connected)
 * - 303: Number of MXP elements
 * - 304: Locale
 * - 305: Client start time
 * - 306: World start time
 * - 310: Newlines received count
 *
 * @param type (number) Info type number (1-310)
 *
 * @return (varies) String, number, or nil depending on the info type
 *
 * @example
 * -- Get world name
 * local name = GetInfo(2)
 *
 * -- Check if connected
 * if GetInfo(106) == false then
 *     Note("Connected to " .. GetInfo(1))
 * end
 *
 * -- Get connection duration
 * local seconds = GetInfo(302)
 * Note("Connected for " .. seconds .. " seconds")
 *
 * @see utils.infotypes, GetOption, GetAlphaOption
 */
int L_GetInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int type = luaL_checkinteger(L, 1);

    switch (type) {
        case 1: // Server address (hostname/IP)
        {
            luaPushQString(L, pDoc->m_server);
        } break;

        case 2: // World name
        {
            luaPushQString(L, pDoc->m_mush_name);
        } break;

        case 3: // Character name
        {
            luaPushQString(L, pDoc->m_name);
        } break;

        case 4: // Logging file preamble
        {
            luaPushQString(L, pDoc->m_paste.file_preamble);
        } break;

        case 5: // Logging file postamble
        {
            luaPushQString(L, pDoc->m_paste.file_postamble);
        } break;

        case 6: // Logging line preamble
        {
            luaPushQString(L, pDoc->m_paste.line_preamble);
        } break;

        case 7: // Logging line postamble
        {
            luaPushQString(L, pDoc->m_paste.line_postamble);
        } break;

        case 8: // Notes
        {
            luaPushQString(L, pDoc->m_notes);
        } break;

        case 9: // New activity sound
        {
            luaPushQString(L, pDoc->m_sound.new_activity_sound);
        } break;

        case 10: // Script editor
        {
            luaPushQString(L, pDoc->m_scripting.editor);
        } break;

        case 11: // Log file preamble
        {
            luaPushQString(L, pDoc->m_logging.file_preamble);
        } break;

        case 12: // Log file postamble
        {
            luaPushQString(L, pDoc->m_logging.file_postamble);
        } break;

        case 13: // Log line preamble (input)
        {
            luaPushQString(L, pDoc->m_logging.line_preamble_input);
        } break;

        case 14: // Log line preamble (notes)
        {
            luaPushQString(L, pDoc->m_logging.line_preamble_notes);
        } break;

        case 15: // Log line preamble (output)
        {
            luaPushQString(L, pDoc->m_logging.line_preamble_output);
        } break;

        case 16: // Log line postamble (input)
        {
            luaPushQString(L, pDoc->m_logging.line_postamble_input);
        } break;

        case 17: // Log line postamble (notes)
        {
            luaPushQString(L, pDoc->m_logging.line_postamble_notes);
        } break;

        case 18: // Log line postamble (output)
        {
            luaPushQString(L, pDoc->m_logging.line_postamble_output);
        } break;

        case 19: // Speed walk filler
        {
            luaPushQString(L, pDoc->m_speedwalk.filler);
        } break;

        case 20: // Output font name
        {
            luaPushQString(L, pDoc->m_output.font_name);
        } break;

        case 21: // Speed walk prefix
        {
            luaPushQString(L, pDoc->m_speedwalk.prefix);
        } break;

        case 22: // Connect text
        {
            luaPushQString(L, pDoc->m_connect_text);
        } break;

        case 23: // Input font name
        {
            luaPushQString(L, pDoc->m_input.font_name);
        } break;

        case 24: // Paste postamble
        {
            luaPushQString(L, pDoc->m_paste.paste_postamble);
        } break;

        case 25: // Paste preamble
        {
            luaPushQString(L, pDoc->m_paste.paste_preamble);
        } break;

        case 26: // Paste line postamble
        {
            luaPushQString(L, pDoc->m_paste.pasteline_postamble);
        } break;

        case 27: // Paste line preamble
        {
            luaPushQString(L, pDoc->m_paste.pasteline_preamble);
        } break;

        case 28: // Script language
        {
            luaPushQString(L, pDoc->m_scripting.language);
        } break;

        case 29: // OnWorldOpen callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_open);
        } break;

        case 30: // OnWorldClose callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_close);
        } break;

        case 31: // OnWorldConnect callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_connect);
        } break;

        case 32: // OnWorldDisconnect callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_disconnect);
        } break;

        case 33: // OnWorldGetFocus callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_get_focus);
        } break;

        case 34: // OnWorldLoseFocus callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_lose_focus);
        } break;

        case 35: // Script filename
        {
            luaPushQString(L, pDoc->m_scripting.filename);
        } break;

        case 36: // Script prefix
        {
            luaPushQString(L, pDoc->m_scripting.prefix);
        } break;

        case 37: // Auto-say string
        {
            luaPushQString(L, pDoc->m_auto_say.say_string);
        } break;

        case 38: // Override prefix
        {
            luaPushQString(L, pDoc->m_auto_say.override_prefix);
        } break;

        case 39: // Tab completion defaults
        {
            luaPushQString(L, pDoc->m_command_window.tab_completion_defaults);
        } break;

        case 40: // Auto-log filename
        {
            luaPushQString(L, pDoc->m_logging.auto_log_file_name);
        } break;

        case 41: // Recall line preamble
        {
            luaPushQString(L, pDoc->m_output.recall_line_preamble);
        } break;

        case 42: // Terminal identification
        {
            luaPushQString(L, pDoc->m_strTerminalIdentification);
        } break;

        case 43: // Mapping failure message
        {
            luaPushQString(L, pDoc->m_mapping.failure_message);
        } break;

        case 44: // OnMXP_Start callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_Start);
        } break;

        case 45: // OnMXP_Stop callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_Stop);
        } break;

        case 46: // OnMXP_Error callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_Error);
        } break;

        case 47: // OnMXP_OpenTag callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_OpenTag);
        } break;

        case 48: // OnMXP_CloseTag callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_CloseTag);
        } break;

        case 49: // OnMXP_SetVariable callback
        {
            luaPushQString(L, pDoc->m_strOnMXP_SetVariable);
        } break;

        case 50: // Beep sound
        {
            luaPushQString(L, pDoc->m_sound.beep_sound);
        } break;

        case 51: // Log filename
        {
            luaPushQString(L, pDoc->m_logfile_name);
        } break;

        case 52: // Last immediate expression
        {
            luaPushQString(L, pDoc->m_strLastImmediateExpression);
        } break;

        case 53: // Status message
        {
            luaPushQString(L, pDoc->m_strStatusMessage);
        } break;

        case 54: // World file path (GetPathName)
        {
            luaPushQString(L, pDoc->m_strWorldFilePath);
        } break;

        case 55: // Window title (GetTitle)
        {
            luaPushQString(L, pDoc->m_strWindowTitle);
        } break;

        case 56: // MUSHclient executable path
        {
            luaPushQString(L, QCoreApplication::applicationFilePath());
        } break;

        case 57: // Default world file directory
        {
            luaPushQString(L, GlobalOptions::instance().defaultWorldFileDirectory());
        } break;

        case 58: // Default log file directory
        {
            luaPushQString(L, GlobalOptions::instance().defaultLogFileDirectory());
        } break;

        case 59: // Scripts directory (application directory)
        {
            QString appDir = AppPaths::getAppDirectory();
            if (!appDir.isEmpty() && !appDir.endsWith("/")) {
                appDir += "/";
            }
            luaPushQString(L, appDir);
        } break;

        case 60: // Plugins directory (global)
        {
            // Return global plugins directory, resolved to absolute path
            auto& db = Database::instance();
            QString pluginsDir = db.getPreference("PluginsDirectory", "./worlds/plugins/");
            pluginsDir.replace('\\', '/');

            // If relative, resolve against application directory
            if (!QDir::isAbsolutePath(pluginsDir)) {
                QString appDir = AppPaths::getAppDirectory();
                pluginsDir = QDir(appDir).absoluteFilePath(pluginsDir);
            }
            pluginsDir = QDir::cleanPath(pluginsDir);

            // Ensure trailing slash for path concatenation
            if (!pluginsDir.endsWith('/')) {
                pluginsDir += '/';
            }

            luaPushQString(L, pluginsDir);
        } break;

        case 61: // IP address from socket connection
        {
            if (pDoc->m_connectionManager->isConnected() && pDoc->m_connectionManager->m_pSocket) {
                luaPushQString(L, pDoc->m_connectionManager->m_pSocket->peerAddress());
            } else {
                lua_pushstring(L, "");
            }
        } break;

        case 62: // Proxy server
        {
            luaPushQString(L, pDoc->m_proxy.server);
        } break;

        case 63: // Hostname
        {
            luaPushQString(L, QHostInfo::localHostName());
        } break;

        case 64: // Current directory with trailing slash
        {
            QString currentDir = QDir::currentPath();
            if (!currentDir.isEmpty() && !currentDir.endsWith("/")) {
                currentDir += "/";
            }
            luaPushQString(L, currentDir);
        } break;

        case 65: // OnWorldSave callback
        {
            luaPushQString(L, pDoc->m_scripting.on_world_save);
        } break;

        case 66: // MUSHclient application directory
        {
            // Return the application directory (where user files like worlds/ are located)
            // On macOS .app bundles, this is the folder containing the .app bundle
            // Based on: ExtractDirectory(App.m_strMUSHclientFileName)
            QString appDir = AppPaths::getAppDirectory();
            // Ensure trailing slash for compatibility
            if (!appDir.isEmpty() && !appDir.endsWith("/")) {
                appDir += "/";
            }
            luaPushQString(L, appDir);
        } break;

        case 67: // World file directory
        {
            if (!pDoc->m_strWorldFilePath.isEmpty()) {
                QString dir = QFileInfo(pDoc->m_strWorldFilePath).path();
                if (!dir.endsWith("/")) {
                    dir += "/";
                }
                luaPushQString(L, dir);
            } else {
                lua_pushstring(L, "");
            }
        } break;

        case 68: // Working directory
        {
            luaPushQString(L, QDir::currentPath());
        } break;

        case 69: // Translator file
        {
            // Not applicable: MFC CDocument translator — no cross-platform equivalent.
            lua_pushstring(L, "");
        } break;

        case 70: // Locale
        {
            luaPushQString(L, QLocale::system().name());
        } break;

        case 71: // Fixed pitch font
        {
            luaPushQString(L, GlobalOptions::instance().fixedPitchFont());
        } break;

        case 72: // Version
        {
            // Reports compatibility version for plugin detection.
            lua_pushstring(L, "5.06-preview");
        } break;

        case 73: // Build date/time
        {
            luaPushQString(L, QString("%1 %2").arg(__DATE__).arg(__TIME__));
        } break;

        case 74: // Sounds directory
        {
            QString soundsDir = AppPaths::getAppDirectory();
            if (!soundsDir.isEmpty() && !soundsDir.endsWith("/")) {
                soundsDir += "/";
            }
            soundsDir += "sounds/";
            luaPushQString(L, soundsDir);
        } break;

        case 75: // IAC subnegotiation data
        {
            lua_pushlstring(L, pDoc->m_telnetParser->m_IAC_subnegotiation_data.constData(),
                            pDoc->m_telnetParser->m_IAC_subnegotiation_data.size());
        } break;

        case 76: // Special font (first one)
        {
            // Not applicable: Windows GDI special font enumeration. Use AddFont() to register
            // fonts.
            lua_pushstring(L, "");
        } break;

        case 77: // OS version
        {
            luaPushQString(L, QSysInfo::prettyProductName());
        } break;

        case 78: // Foreground image
        {
            luaPushQString(L, pDoc->m_strForegroundImageName);
        } break;

        case 79: // Background image
        {
            luaPushQString(L, pDoc->m_strBackgroundImageName);
        } break;

        case 80: // PNG library version
        {
            // Not applicable: Qt handles PNG internally — no standalone libpng to version.
            lua_pushstring(L, "");
        } break;

        case 81: // PNG header version
        {
            // Not applicable: Qt handles PNG internally — no standalone libpng to version.
            lua_pushstring(L, "");
        } break;

        case 82: // Preferences database name
        {
            // QSettings uses platform-native storage (plist on macOS, registry on Windows, conf on
            // Linux). Return the settings file path where available.
            luaPushQString(L, QSettings().fileName());
        } break;

        case 83: // SQLite version
        {
            lua_pushstring(L, sqlite3_libversion());
        } break;

        case 84: // File browsing directory
        {
            // Not applicable: Windows common file dialog directory tracking. Use GetInfo(68) for
            // CWD.
            lua_pushstring(L, "");
        } break;

        case 85: // Default state files directory
        {
            luaPushQString(L, GlobalOptions::instance().stateFilesDirectory());
        } break;

        case 86: // Word under menu
        {
            luaPushQString(L, pDoc->m_strWordUnderMenu);
        } break;

        case 87: // Last command sent
        {
            luaPushQString(L, pDoc->m_strLastCommandSent);
        } break;

        case 88: // Window title (individual world)
        {
            luaPushQString(L, pDoc->m_strWindowTitle);
        } break;

        case 89: // Main window title
        {
            luaPushQString(L, pDoc->m_strMainWindowTitle);
        } break;

        // Boolean flags (101-125)
        case 101: // No echo (IAC WILL ECHO received)
            lua_pushboolean(L, pDoc->m_telnetParser->m_bNoEcho);
            break;

        case 102: // Debug incoming packets
            lua_pushboolean(L, pDoc->m_bDebugIncomingPackets);
            break;

        case 103: // Compress (MCCP active)
            lua_pushboolean(L, pDoc->m_telnetParser->m_bCompress);
            break;

        case 104: // MXP active
            lua_pushboolean(L, pDoc->m_mxpEngine->m_bMXP);
            break;

        case 105: // Pueblo active
            lua_pushboolean(L, pDoc->m_mxpEngine->m_bPuebloActive);
            break;

        case 106: // Not connected
            lua_pushboolean(L, pDoc->connectPhase() != eConnectConnectedToMud);
            break;

        case 107: // Connecting (not disconnected and not connected)
            lua_pushboolean(L, pDoc->connectPhase() != eConnectNotConnected &&
                                   pDoc->connectPhase() != eConnectConnectedToMud);
            break;

        case 108: // Disconnect OK
            lua_pushboolean(L, pDoc->m_bDisconnectOK);
            break;

        case 109: // Trace enabled
            lua_pushboolean(L, pDoc->m_bTrace);
            break;

        case 110: // Script file changed
            lua_pushboolean(L, pDoc->m_bInScriptFileChanged);
            break;

        case 111: // World modified (IsModified)
            lua_pushboolean(L, pDoc->isModified());
            break;

        case 112: // Mapping enabled
            lua_pushboolean(L, pDoc->m_mapping.enabled);
            break;

        case 113: // Window open (has active views)
            lua_pushboolean(L, pDoc->m_pActiveOutputView != nullptr);
            break;

        case 114: // Current view frozen
        {
            bool frozen = pDoc->m_pActiveOutputView ? pDoc->m_pActiveOutputView->isFrozen() : false;
            lua_pushboolean(L, frozen);
        } break;

        case 115: // Translator Lua available
        {
            // Not applicable: MFC CDocument translator subsystem — no cross-platform equivalent.
            lua_pushboolean(L, false);
        } break;

        case 118: // Variables changed
            lua_pushboolean(L, pDoc->m_bVariablesChanged);
            break;

        case 119: // Script engine exists
            lua_pushboolean(L, pDoc->m_ScriptEngine != nullptr);
            break;

        case 120: // Scroll bar wanted
            lua_pushboolean(L, pDoc->m_bScrollBarWanted);
            break;

        case 121: // Performance counter available
        {
            // Always true on modern systems
            lua_pushboolean(L, true);
        } break;

        case 122: // SQLite threadsafe
            lua_pushboolean(L, sqlite3_threadsafe() != 0);
            break;

        case 123: // Doing simulate
            lua_pushboolean(L, pDoc->m_bDoingSimulate);
            break;

        case 124: // Line omitted from output
            lua_pushboolean(L, pDoc->m_bLineOmittedFromOutput);
            break;

        case 125: // Full screen mode
        {
            // TODO(ui): Requires MainWindow callback to check windowState() & Qt::WindowFullScreen.
            lua_pushboolean(L, false);
        } break;

        // Line and packet counts (201-205)
        case 201: // Total lines received
            lua_pushinteger(L, pDoc->m_total_lines);
            break;

        case 202: // New lines (unread)
            lua_pushinteger(L, pDoc->m_new_lines);
            break;

        case 203: // Total lines sent
            lua_pushinteger(L, pDoc->m_connectionManager->m_nTotalLinesSent);
            break;

        case 204: // Input packet count
            lua_pushinteger(L, pDoc->m_connectionManager->m_iInputPacketCount);
            break;

        case 205: // Output packet count
            lua_pushinteger(L, pDoc->m_connectionManager->m_iOutputPacketCount);
            break;

        case 206: // Total uncompressed bytes (MCCP)
            lua_pushinteger(L, pDoc->m_telnetParser->m_nTotalUncompressed);
            break;

        case 207: // Total compressed bytes (MCCP)
            lua_pushinteger(L, pDoc->m_telnetParser->m_nTotalCompressed);
            break;

        case 208: // MCCP type (0=none, 1=v1, 2=v2)
            lua_pushinteger(L, pDoc->m_telnetParser->m_iMCCP_type);
            break;

        case 209: // MXP errors
            lua_pushinteger(L, pDoc->m_mxpEngine->m_iMXPerrors);
            break;

        case 210: // MXP tags processed
            lua_pushinteger(L, pDoc->m_mxpEngine->m_iMXPtags);
            break;

        case 211: // MXP entities processed
            lua_pushinteger(L, pDoc->m_mxpEngine->m_iMXPentities);
            break;

        case 212: // Output font height
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_output.font_height);
            break;

        case 213: // Output font width
            lua_pushinteger(L, pDoc->m_FontWidth);
            break;

        case 214: // Input font height
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_InputFontHeight);
            break;

        case 215: // Input font width
            lua_pushinteger(L, pDoc->m_InputFontWidth);
            break;

        case 216: // Bytes received
            lua_pushinteger(L, pDoc->m_connectionManager->m_nBytesIn);
            break;

        case 217: // Bytes sent
            lua_pushinteger(L, pDoc->m_connectionManager->m_nBytesOut);
            break;

        case 218: // Count of variables
            lua_pushinteger(L, pDoc->m_VariableMap.size());
            break;

        case 219: // Count of triggers
            lua_pushinteger(L, pDoc->m_automationRegistry->m_TriggerMap.size());
            break;

        case 220: // Count of timers
            lua_pushinteger(L, pDoc->m_automationRegistry->m_TimerMap.size());
            break;

        case 221: // Count of aliases
            lua_pushinteger(L, pDoc->m_automationRegistry->m_AliasMap.size());
            break;

        case 222: // Count of queued commands
            // TODO(feature): Command queue not yet implemented. Return 0 until queue system exists.
            lua_pushinteger(L, 0);
            break;

        case 223: // Count of mapper items
            lua_pushinteger(L, static_cast<lua_Integer>(pDoc->m_mapList.size()));
            break;

        case 224: // Count of lines in output window
            // Based on: methods_info.cpp
            lua_pushinteger(L, static_cast<lua_Integer>(pDoc->m_lineList.size()));
            break;

        case 225: // Count of custom MXP elements
            lua_pushinteger(L,
                            static_cast<lua_Integer>(pDoc->m_mxpEngine->m_customElementMap.size()));
            break;

        case 226: // Count of custom MXP entities
            lua_pushinteger(L,
                            static_cast<lua_Integer>(pDoc->m_mxpEngine->m_customEntityMap.size()));
            break;

        case 227: // Connection phase
            lua_pushinteger(L, pDoc->connectPhase());
            break;

        case 228: // IP address as integer (host byte order)
        {
            if (pDoc->m_connectionManager->isConnected() && pDoc->m_connectionManager->m_pSocket) {
                QHostAddress addr(pDoc->m_connectionManager->m_pSocket->peerAddress());
                bool ok = false;
                quint32 ipv4 = addr.toIPv4Address(&ok);
                lua_pushinteger(L, ok ? static_cast<lua_Integer>(ipv4) : 0);
            } else {
                lua_pushinteger(L, 0);
            }
        } break;

        case 229: // Proxy type (0=None, 1=SOCKS5, 2=HTTP CONNECT)
            lua_pushinteger(L, pDoc->m_proxy.type);
            break;

        case 230: // Execution depth (script call depth)
            lua_pushinteger(L, pDoc->m_iExecutionDepth);
            break;

        case 231: // Log file size
            if (pDoc->m_logfile && pDoc->m_logfile->isOpen()) {
                lua_pushinteger(L, static_cast<lua_Integer>(pDoc->m_logfile->size()));
            } else {
                lua_pushinteger(L, 0);
            }
            break;

        case 232: // High-resolution timer (seconds since epoch)
            lua_pushnumber(L, QDateTime::currentMSecsSinceEpoch() / 1000.0);
            break;

        case 233: // Time taken doing triggers (seconds)
            lua_pushnumber(L, pDoc->m_automationRegistry->m_trigger_time_elapsed);
            break;

        case 234: // Time taken doing aliases (seconds)
            lua_pushnumber(L, pDoc->m_automationRegistry->m_alias_time_elapsed);
            break;

        case 235: // Number of world windows open
            // Always 1 in Qt version (single world per window)
            lua_pushinteger(L, 1);
            break;

        case 236: // Command selection start column
        {
            auto* inputView = pDoc->activeInputView();
            if (inputView && inputView->selectionStart() != inputView->selectionEnd()) {
                lua_pushinteger(L, inputView->selectionStart() + 1); // 1-based
            } else {
                lua_pushinteger(L, 0); // No selection
            }
            break;
        }

        case 237: // Command selection end column
        {
            auto* inputView = pDoc->activeInputView();
            if (inputView && inputView->selectionStart() != inputView->selectionEnd()) {
                lua_pushinteger(L, inputView->selectionEnd() + 1); // 1-based
            } else {
                lua_pushinteger(L, 0); // No selection
            }
            break;
        }

        case 238: // Window placement flags
            // Not applicable: Windows window placement flags (WPF_*). Use GetInfo(125) for
            // fullscreen.
            lua_pushinteger(L, 0);
            break;

        case 239: // Current action source (trigger/alias/timer/etc)
            lua_pushinteger(L, static_cast<quint32>(pDoc->m_iCurrentActionSource));
            break;

        case 240: // Average character width
            lua_pushinteger(L, pDoc->m_FontWidth);
            break;

        case 241: // Character height
            lua_pushinteger(L, pDoc->m_output.font_height);
            break;

        case 242: // UTF-8 error count
            lua_pushinteger(L, pDoc->m_iUTF8ErrorCount);
            break;

        case 243: // Fixed pitch font size
            lua_pushinteger(L, GlobalOptions::instance().fixedPitchFontSize());
            break;

        case 244: // Triggers evaluated count
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iTriggersEvaluatedCount);
            break;

        case 245: // Triggers matched count
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iTriggersMatchedCount);
            break;

        case 246: // Aliases evaluated count
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iAliasesEvaluatedCount);
            break;

        case 247: // Aliases matched count
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iAliasesMatchedCount);
            break;

        case 248: // Timers fired count
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iTimersFiredCount);
            break;

        case 249: // Main window client height
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(0, 0)); // which=0 (client area), infoType=0 (height)
            break;
        }

        case 250: // Main window client width
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(0, 1)); // which=0 (client area), infoType=1 (width)
            break;
        }

        case 251: // Main toolbar height
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(1, 0)); // which=1 (main), infoType=0 (height)
            break;
        }

        case 252: // Main toolbar width
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(1, 1)); // which=1 (main), infoType=1 (width)
            break;
        }

        case 253: // Game toolbar height
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(2, 0)); // which=2 (game), infoType=0 (height)
            break;
        }

        case 254: // Game toolbar width
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(2, 1)); // which=2 (game), infoType=1 (width)
            break;
        }

        case 255: // Activity toolbar height
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(3, 0)); // which=3 (activity), infoType=0 (height)
            break;
        }

        case 256: // Activity toolbar width
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(3, 1)); // which=3 (activity), infoType=1 (width)
            break;
        }

        case 257: // Info bar height
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(4, 0)); // which=4 (infobar), infoType=0 (height)
            break;
        }

        case 258: // Info bar width
        {
            auto callback = ToolbarCallbacks::getGetToolBarInfoCallback();
            lua_pushinteger(L, callback(4, 1)); // which=4 (infobar), infoType=1 (width)
            break;
        }

        case 259: // Status bar height
            // Not applicable: Status bar not used — info displayed in InfoBar dock (cases 257-258).
            lua_pushinteger(L, 0);
            break;

        case 260: // Status bar width
            // Not applicable: Status bar not used — info displayed in InfoBar dock (cases 257-258).
            lua_pushinteger(L, 0);
            break;

        case 261: // World window non-client height
            // Not applicable: Window frame (non-client area) dimensions are platform-dependent and
            // rarely used by plugins.
            lua_pushinteger(L, 0);
            break;

        case 262: // World window non-client width
            // Not applicable: Window frame (non-client area) dimensions are platform-dependent and
            // rarely used by plugins.
            lua_pushinteger(L, 0);
            break;

        case 263: // World window client height
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->viewHeight());
            }
            break;

        case 264: // World window client width
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->viewWidth());
            }
            break;

        case 265: // OS major version
        {
            QStringList parts = QSysInfo::kernelVersion().split('.');
            lua_pushinteger(L, parts.size() > 0 ? parts[0].toInt() : 0);
            break;
        }

        case 266: // OS minor version
        {
            QStringList parts = QSysInfo::kernelVersion().split('.');
            lua_pushinteger(L, parts.size() > 1 ? parts[1].toInt() : 0);
            break;
        }

        case 267: // OS build number
        {
            QStringList parts = QSysInfo::kernelVersion().split('.');
            lua_pushinteger(L, parts.size() > 2 ? parts[2].toInt() : 0);
            break;
        }

        case 268: // OS platform ID
            // Not applicable: Windows OSVERSIONINFO.dwPlatformId constant (always 2 on modern
            // Windows). Plugins checking this value expect 2; returning 0 signals non-Windows
            // platform.
            lua_pushinteger(L, 0);
            break;

        case 269: // Foreground mode
            lua_pushinteger(L, pDoc->m_iForegroundMode);
            break;

        case 270: // Background mode
            lua_pushinteger(L, pDoc->m_iBackgroundMode);
            break;

        case 271: // Background colour
            lua_pushinteger(L, pDoc->m_iBackgroundColour);
            break;

        case 272: // Text rectangle - left
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangle.left());
            break;

        case 273: // Text rectangle - top
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangle.top());
            break;

        case 274: // Text rectangle - right
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangle.right());
            break;

        case 275: // Text rectangle - bottom
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangle.bottom());
            break;

        case 276: // Text rectangle - border offset
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangleBorderOffset);
            break;

        case 277: // Text rectangle - border width
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_TextRectangleBorderWidth);
            break;

        case 278: // Text rectangle outside fill colour
            lua_pushinteger(L, pDoc->m_TextRectangleOutsideFillColour);
            break;

        case 279: // Text rectangle outside fill style
            lua_pushinteger(L, pDoc->m_TextRectangleOutsideFillStyle);
            break;

        case 280: // Output window client height
            // Based on: methods_info.cpp
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->viewHeight());
            }
            // Note: returns nil if view doesn't exist - matches original behavior
            break;

        case 281: // Output window client width
            // Based on: methods_info.cpp
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->viewWidth());
            }
            // Note: returns nil if view doesn't exist - matches original behavior
            break;

        case 282: // Text rectangle border colour
            lua_pushinteger(L, pDoc->m_TextRectangleBorderColour);
            break;

        case 283: // Last mouse X position
            lua_pushinteger(L, pDoc->m_lastMousePosition.x());
            break;

        case 284: // Last mouse Y position
            lua_pushinteger(L, pDoc->m_lastMousePosition.y());
            break;

        case 285: // Is output window available?
            lua_pushboolean(L, pDoc->m_currentLine != nullptr);
            break;

        case 286: // Triggers matched this session
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iTriggersMatchedThisSessionCount);
            break;

        case 287: // Aliases matched this session
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iAliasesMatchedThisSessionCount);
            break;

        case 288: // Timers fired this session
            lua_pushinteger(L, pDoc->m_automationRegistry->m_iTimersFiredThisSessionCount);
            break;

        case 289: // Last line with IAC GA
            lua_pushinteger(L, pDoc->m_last_line_with_IAC_GA);
            break;

        case 290: // Actual text rectangle - left (after normalization)
            // Based on: methods_info.cpp
            // Read from computed rectangle (updated by OutputView)
            lua_pushinteger(L, pDoc->m_computedTextRectangle.left());
            break;

        case 291: // Actual text rectangle - top (after normalization)
            // Based on: methods_info.cpp
            // Read from computed rectangle (updated by OutputView)
            lua_pushinteger(L, pDoc->m_computedTextRectangle.top());
            break;

        case 292: // Actual text rectangle - right (after normalization)
            // Based on: methods_info.cpp
            // Read from computed rectangle (updated by OutputView)
            lua_pushinteger(L, pDoc->m_computedTextRectangle.right());
            break;

        case 293: // Actual text rectangle - bottom (after normalization)
            // Based on: methods_info.cpp
            // Read from computed rectangle (updated by OutputView)
            lua_pushinteger(L, pDoc->m_computedTextRectangle.bottom());
            break;

        case 294: // State of keyboard modifier keys and mouse buttons
        {
            // Based on: GetKeyState(VK_*) bitmask in original.
            // Qt cannot distinguish L/R modifier keys or lock-key toggle states
            // portably. Bits 0x08-0x100 (L/R variants) and 0x200-0x8000 (lock
            // key states) are always 0 on non-Windows platforms.
            long result = 0;
            Qt::KeyboardModifiers mods = QGuiApplication::queryKeyboardModifiers();
            if (mods & Qt::ShiftModifier)
                result |= 0x01;
            if (mods & Qt::ControlModifier)
                result |= 0x02;
            if (mods & Qt::AltModifier)
                result |= 0x04;
            Qt::MouseButtons btns = QGuiApplication::mouseButtons();
            if (btns & Qt::LeftButton)
                result |= 0x10000;
            if (btns & Qt::RightButton)
                result |= 0x20000;
            if (btns & Qt::MiddleButton)
                result |= 0x40000;
            lua_pushinteger(L, static_cast<lua_Integer>(result));
        } break;

        case 295: // Times output window redrawn
            lua_pushinteger(L, static_cast<lua_Integer>(pDoc->m_iOutputWindowRedrawCount));
            break;

        case 296: // Output window scroll bar position
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->getScrollPositionPixels());
            } else {
                lua_pushnil(L);
            }
            break;

        case 297: // High-resolution timer frequency
            // Original: App.m_iCounterFrequency (QueryPerformanceFrequency).
            // Qt uses millisecond timers, so frequency is 1000.0.
            lua_pushnumber(L, 1000.0);
            break;

        case 298: // SQLite3 version number
            lua_pushinteger(L, static_cast<lua_Integer>(sqlite3_libversion_number()));
            break;

        case 299: // ANSI code page (UTF-8 = 65001)
            lua_pushinteger(L, 65001);
            break;

        case 300: // OEM code page (UTF-8 = 65001)
            lua_pushinteger(L, 65001);
            break;

        case 301: // Time connected (DATE \u2014 epoch seconds)
        {
            const QDateTime& ct = pDoc->m_connectionManager->m_tConnectTime;
            if (ct.isValid()) {
                lua_pushnumber(L, static_cast<lua_Number>(ct.toSecsSinceEpoch()));
            } else {
                lua_pushnil(L);
            }
        } break;

        case 302: // Time log file last flushed (DATE \u2014 epoch seconds)
        {
            if (pDoc->m_LastFlushTime.isValid()) {
                lua_pushnumber(L,
                               static_cast<lua_Number>(pDoc->m_LastFlushTime.toSecsSinceEpoch()));
            } else {
                lua_pushnil(L);
            }
        } break;

        case 303: // Script file modification time (DATE \u2014 epoch seconds)
        {
            if (pDoc->m_timeScriptFileMod.isValid()) {
                lua_pushnumber(
                    L, static_cast<lua_Number>(pDoc->m_timeScriptFileMod.toSecsSinceEpoch()));
            } else {
                lua_pushnil(L);
            }
        } break;

        case 304: // Current time (DATE \u2014 epoch seconds)
            lua_pushnumber(L, static_cast<lua_Number>(QDateTime::currentSecsSinceEpoch()));
            break;

        case 305: // Client start time (DATE \u2014 epoch seconds)
        {
            // Approximation: captures time at first call. Negligible drift from true app start.
            static const qint64 s_appStartSecs = QDateTime::currentSecsSinceEpoch();
            lua_pushnumber(L, static_cast<lua_Number>(s_appStartSecs));
        } break;

        case 306: // World start time (DATE \u2014 epoch seconds)
        {
            const QDateTime& ws = pDoc->m_connectionManager->m_whenWorldStarted;
            if (ws.isValid()) {
                lua_pushnumber(L, static_cast<lua_Number>(ws.toSecsSinceEpoch()));
            } else {
                lua_pushnil(L);
            }
        } break;

        case 310: // Newlines received count
            lua_pushinteger(L, pDoc->m_newlines_received);
            break;

        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * SetOption - Set a world option by name
 *
 * Lua signature: error_code = SetOption(option_name, value)
 *
 * @param option_name Name of the option to set (e.g., "wrap_column")
 * @param value New value (number or boolean)
 * @return Error code (eOK=0 on success, eUnknownOption if not found, ePluginCannotSetOption if
 * plugin cannot write)
 */
int L_SetOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Convert boolean to 0 or 1, nil to 0
    double value;
    if (lua_isboolean(L, 2)) {
        value = lua_toboolean(L, 2) ? 1 : 0;
    } else if (lua_isnil(L, 2)) {
        value = 0;
    } else {
        value = luaL_checknumber(L, 2);
    }

    // Find the option in OptionsTable (O(1) hash map lookup)
    QString optionNameStr = luaCheckQString(L, 1);
    std::string key = optionNameStr.toLower().trimmed().toStdString();
    auto it = getNumericOptionMap().find(key);
    if (it == getNumericOptionMap().end()) {
        return luaReturnError(L, eUnknownOption);
    }
    int optionIndex = it->second;

    // Check if plugin is allowed to write this option
    if (pDoc->m_CurrentPlugin && (OptionsTable[optionIndex].iFlags & OPT_PLUGIN_CANNOT_WRITE)) {
        return luaReturnError(L, ePluginCannotSetOption);
    }

    const tConfigurationNumericOption& opt = OptionsTable[optionIndex];

    // Clamp value to min/max if specified
    if (opt.iMinimum != 0 || opt.iMaximum != 0) {
        if (value < opt.iMinimum)
            value = opt.iMinimum;
        if (value > opt.iMaximum)
            value = opt.iMaximum;
    }

    opt.setter(*pDoc, value);

    return luaReturnOK(L);
}

/**
 * GetOption - Get a world option by name
 *
 * Lua signature: value = GetOption(option_name)
 *
 * @param option_name Name of the option to get (e.g., "wrap_column")
 * @return Option value, or nil if option not found
 *
 * Example: enable_triggers = GetOption("enable_triggers")
 */
int L_GetOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* optionName = luaL_checkstring(L, 1);

    // Build lowercase key for O(1) lookup
    std::string key(optionName);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    // Search numeric options table first (O(1))
    {
        auto it = getNumericOptionMap().find(key);
        if (it != getNumericOptionMap().end()) {
            lua_pushnumber(L, OptionsTable[it->second].getter(*pDoc));
            return 1;
        }
    }

    // Search alpha (string) options table (O(1))
    {
        auto it = getAlphaOptionMap().find(key);
        if (it != getAlphaOptionMap().end()) {
            luaPushQString(L, AlphaOptionsTable[it->second].getter(*pDoc));
            return 1;
        }
    }

    // Option not found
    lua_pushnil(L);
    return 1;
}

/**
 * GetAlphaOption - Get a string option by name
 *
 * Lua signature: value = GetAlphaOption(option_name)
 *
 * @param option_name Name of the string option to get (e.g., "script_prefix")
 * @return Option value string, or nil if option not found or plugin cannot read
 *
 * Example: prefix = GetAlphaOption("script_prefix")
 */
int L_GetAlphaOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Normalize option name: lowercase, trimmed (matches original)
    std::string key = luaCheckQString(L, 1).toLower().trimmed().toStdString();

    // Search alpha (string) options table only (O(1))
    auto it = getAlphaOptionMap().find(key);
    if (it != getAlphaOptionMap().end()) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[it->second];
        // Check if plugin is allowed to read this option
        if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
            lua_pushnil(L);
            return 1;
        }
        luaPushQString(L, opt.getter(*pDoc));
        return 1;
    }

    // Option not found
    lua_pushnil(L);
    return 1;
}

/**
 * GetCurrentValue - Get current value of any option by name
 *
 * Lua signature: value = GetCurrentValue(option_name)
 * Unified wrapper that searches both numeric and alpha tables.
 * Enforces OPT_PLUGIN_CANNOT_READ on both tables.
 */
int L_GetCurrentValue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    std::string key = luaCheckQString(L, 1).toLower().trimmed().toStdString();

    // Search numeric options table first (O(1))
    {
        auto it = getNumericOptionMap().find(key);
        if (it != getNumericOptionMap().end()) {
            const tConfigurationNumericOption& opt = OptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            lua_pushnumber(L, opt.getter(*pDoc));
            return 1;
        }
    }

    // Search alpha (string) options table (O(1))
    {
        auto it = getAlphaOptionMap().find(key);
        if (it != getAlphaOptionMap().end()) {
            const tConfigurationAlphaOption& opt = AlphaOptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            luaPushQString(L, opt.getter(*pDoc));
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

/**
 * GetDefaultValue - Get the default value of any option by name
 *
 * Lua signature: value = GetDefaultValue(option_name)
 * Returns the hardcoded default from the options table metadata.
 * Enforces OPT_PLUGIN_CANNOT_READ on both tables.
 */
int L_GetDefaultValue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    std::string key = luaCheckQString(L, 1).toLower().trimmed().toStdString();

    // Search numeric options table first (O(1))
    {
        auto it = getNumericOptionMap().find(key);
        if (it != getNumericOptionMap().end()) {
            const tConfigurationNumericOption& opt = OptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            lua_pushnumber(L, opt.iDefault);
            return 1;
        }
    }

    // Search alpha (string) options table (O(1))
    {
        auto it = getAlphaOptionMap().find(key);
        if (it != getAlphaOptionMap().end()) {
            const tConfigurationAlphaOption& opt = AlphaOptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            if (opt.sDefault) {
                lua_pushstring(L, opt.sDefault);
            } else {
                lua_pushstring(L, "");
            }
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

/**
 * GetLoadedValue - Get the value of an option as it was when last loaded from disk
 *
 * Lua signature: value = GetLoadedValue(option_name)
 * Returns the snapshot taken after XML load completed.
 * Enforces OPT_PLUGIN_CANNOT_READ on both tables.
 */
int L_GetLoadedValue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    std::string key = luaCheckQString(L, 1).toLower().trimmed().toStdString();
    QString normalizedName = QString::fromStdString(key);

    // Search numeric options table first (O(1))
    {
        auto it = getNumericOptionMap().find(key);
        if (it != getNumericOptionMap().end()) {
            const tConfigurationNumericOption& opt = OptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            auto loadIt = pDoc->m_loadedNumericOptions.find(normalizedName);
            if (loadIt != pDoc->m_loadedNumericOptions.end()) {
                lua_pushnumber(L, loadIt->second);
            } else {
                // No loaded snapshot available (world not loaded from file)
                lua_pushnil(L);
            }
            return 1;
        }
    }

    // Search alpha (string) options table (O(1))
    {
        auto it = getAlphaOptionMap().find(key);
        if (it != getAlphaOptionMap().end()) {
            const tConfigurationAlphaOption& opt = AlphaOptionsTable[it->second];
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }
            auto loadIt = pDoc->m_loadedAlphaOptions.find(normalizedName);
            if (loadIt != pDoc->m_loadedAlphaOptions.end()) {
                luaPushQString(L, loadIt->second);
            } else {
                lua_pushnil(L);
            }
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

/**
 * SetAlphaOption - Set a string option by name
 *
 * Lua signature: result = SetAlphaOption(option_name, value)
 *
 * @param option_name Name of the string option to set (e.g., "script_prefix")
 * @param value New string value
 * @return eOK on success, eUnknownOption if not found, ePluginCannotSetOption if denied
 *
 * Example: SetAlphaOption("script_prefix", "lua_")
 */
int L_SetAlphaOption(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Normalize option name: lowercase, trimmed (matches original)
    auto [rawName, strValue] = luaArgs<QString, QString>(L);
    std::string key = rawName.toLower().trimmed().toStdString();

    // Search alpha (string) options table (O(1))
    auto it = getAlphaOptionMap().find(key);
    if (it != getAlphaOptionMap().end()) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[it->second];

        // Check if this option can be written at all
        if (opt.iFlags & OPT_CANNOT_WRITE) {
            return luaReturnError(L, eOptionOutOfRange);
        }

        // Check if plugin is allowed to write this option
        if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_WRITE)) {
            return luaReturnError(L, ePluginCannotSetOption);
        }

        // Special validation for command stack character
        if (opt.iFlags & OPT_COMMAND_STACK) {
            if (strValue.length() > 1) {
                return luaReturnError(L, eOptionOutOfRange);
            }
            if (strValue.isEmpty()) {
                // Disable command stack
                pDoc->m_input.enable_command_stack = false;
                return luaReturnError(L, eOptionOutOfRange);
            }
            QChar ch = strValue.at(0);
            if (!ch.isPrint() || ch.isSpace()) {
                pDoc->m_input.enable_command_stack = false;
                return luaReturnError(L, eOptionOutOfRange);
            }
        }

        // Special validation for world ID
        if (opt.iFlags & OPT_WORLD_ID) {
            if (!strValue.isEmpty()) {
                if (strValue.length() != 24) { // PLUGIN_UNIQUE_ID_LENGTH
                    return luaReturnError(L, eOptionOutOfRange);
                }
                // Ensure all hex characters
                static QRegularExpression hexRegex("^[0-9a-fA-F]+$");
                if (!hexRegex.match(strValue).hasMatch()) {
                    return luaReturnError(L, eOptionOutOfRange);
                }
                strValue = strValue.toLower();
            }
        }

        // Strip newlines from non-multiline options
        if (!(opt.iFlags & OPT_MULTLINE)) {
            strValue.remove('\n');
            strValue.remove('\r');
        }

        opt.setter(*pDoc, strValue);

        // TODO(ui): Trigger UI refresh callbacks (view repaint, font reload) when options
        // change.

        return luaReturnOK(L);
    }

    // Option not found
    return luaReturnError(L, eUnknownOption);
}

/**
 * SetStatus - Set the status bar text
 *
 * Lua signature: SetStatus(text)
 *
 * @param text Status text
 */
int L_SetStatus(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_strStatusMessage = luaCheckQString(L, 1);
    pDoc->m_tStatusDisplayed = QDateTime::currentDateTime();

    return 0;
}

/**
 * Repaint - Trigger a UI repaint
 *
 * Lua signature: Repaint()
 *
 * Forces an immediate repaint of the active output view.
 * Used by plugins to refresh miniwindows after updates.
 */
int L_Repaint(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->Repaint();
    return 0;
}

/**
 * TextRectangle - Configure the text rectangle for text display
 *
 * Text Rectangle Architecture
 *
 * Based on CMUSHclientDoc::TextRectangle() from methods_output.cpp
 *
 * Sets up a bounded rectangle within the output window where text is displayed.
 * Miniwindows are positioned in the margins outside this rectangle.
 *
 * Negative values for Right and Bottom are treated as offsets from the window edges:
 * - Right <= 0: means "Right = window_width + Right"
 * - Bottom <= 0: means "Bottom = window_height + Bottom"
 *
 * Example: TextRectangle(7, 278, -10, -12, ...)
 *          Left margin: 7 pixels
 *          Top margin: 278 pixels
 *          Right margin: 10 pixels from right edge
 *          Bottom margin: 12 pixels from bottom edge
 *
 * @param Left Left coordinate
 * @param Top Top coordinate
 * @param Right Right coordinate (or negative offset from right edge)
 * @param Bottom Bottom coordinate (or negative offset from bottom edge)
 * @param BorderOffset Space between text and border
 * @param BorderColour Border color (ARGB)
 * @param BorderWidth Border thickness in pixels
 * @param OutsideFillColour Margin fill color (ARGB)
 * @param OutsideFillStyle Margin fill style (0=solid, 1=null, 2+=patterns)
 * @return 0 (eOK) on success
 */
int L_TextRectangle(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    auto [left, top, right, bottom, borderOffset, borderColour, borderWidth, outsideFillColour,
          outsideFillStyle] = luaArgs<int, int, int, int, int, int, int, int, int>(L);

    qCDebug(lcScript) << "TextRectangle called: rect=(" << left << top << right << bottom
                      << ") border=" << borderOffset << borderWidth;

    // Store configuration
    // NOTE: Lua passes left, top, right, bottom (coordinates)
    // but QRect(int, int, int, int) expects x, y, width, height
    // Use QPoint constructor to create from coordinates
    pDoc->m_TextRectangle = QRect(QPoint(left, top), QPoint(right, bottom));
    pDoc->m_TextRectangleBorderOffset = borderOffset;
    pDoc->m_TextRectangleBorderColour = static_cast<QRgb>(borderColour);
    pDoc->m_TextRectangleBorderWidth = borderWidth;
    pDoc->m_TextRectangleOutsideFillColour = static_cast<QRgb>(outsideFillColour);
    pDoc->m_TextRectangleOutsideFillStyle = outsideFillStyle;

    // Notify OutputView that text rectangle config changed
    // (OutputView will recalculate metrics and update m_computedTextRectangle)
    emit pDoc->textRectangleConfigChanged();

    // Trigger redraw
    pDoc->Repaint();

    return luaReturn(L, eOK);
}

/**
 * SetBackgroundImage - Set background image
 *
 * Background/Foreground Image Support
 *
 * Sets a background image that is drawn behind all other content.
 * SetBackgroundImage
 *
 * @param filename Path to image file (or empty string to clear)
 * @param mode Display mode (0-3=stretch variants, 4-12=position, 13=tile)
 * @return Error code (eOK on success, eBadParameter for invalid mode)
 */
int L_SetBackgroundImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 mode = luaL_optinteger(L, 2, 0);

    // Validate mode (matches original)
    if (mode < 0 || mode > 13) {
        return luaReturn(L, eBadParameter);
    }

    // Store the image path and mode
    pDoc->m_strBackgroundImageName = luaOptQString(L, 1);
    pDoc->m_iBackgroundMode = mode;

    // Tell OutputView to reload the image via interface method
    if (pDoc->m_pActiveOutputView) {
        pDoc->m_pActiveOutputView->reloadBackgroundImage();
    }

    return luaReturn(L, eOK);
}

/**
 * world.GetCommand()
 *
 * Gets the current text in the command input window.
 *
 * Based on methods_commands.cpp
 *
 * @return Current command text
 */
LUA_DOC_GETTER(L_GetCommand, pDoc->GetCommand())

/**
 * SetCommandWindowHeight - Set command window height (stub implementation)
 *
 * TODO: Implement command window height control
 * For now, just return success to allow aard_layout plugin to load
 */
int L_SetCommandWindowHeight(lua_State* L)
{
    // qint32 height = luaL_checkinteger(L, 1);
    // qDebug() << "SetCommandWindowHeight called (stub): height=" << height;
    return luaReturn(L, eOK);
}

/**
 * SetScroll(position, visible)
 *
 * Scrollbar Management (STUB IMPLEMENTATION)
 * Sets the output window scroll position and scrollbar visibility.
 *
 * Based on: CMUSHclientDoc::SetScroll from methods_output.cpp
 *
 * @param position Scroll position in pixels (-1=no change, -2=special)
 * @param visible Scrollbar visibility (true=visible, false=hidden)
 * @return eOK (success)
 */
int L_SetScroll(lua_State* L)
{
    // qint32 position = luaL_checkinteger(L, 1);
    // bool visible = lua_toboolean(L, 2);
    // qDebug() << "SetScroll called (stub): position=" << position << ", visible=" << visible;
    return luaReturn(L, eOK);
}

/**
 * SetCursor - Set the mouse cursor shape
 *
 * Lua signature: code = SetCursor(cursor_type)
 *
 * Cursor types (from methods_output.cpp):
 *  -1 = No cursor (hide cursor)
 *   0 = Arrow (normal)
 *   1 = Hand (pointing hand)
 *   2 = I-beam (text cursor)
 *   3 = Cross
 *   4 = Wait (hourglass)
 *   5 = Up arrow
 *   6 = Size NW-SE
 *   7 = Size NE-SW
 *   8 = Size E-W (horizontal)
 *   9 = Size N-S (vertical)
 *  10 = Size all (four-way arrows)
 *  11 = No (forbidden - circle with slash)
 *  12 = Help (question mark)
 *
 * @param cursor_type Cursor type code (-1 to 12)
 * @return Error code (eOK on success, eBadParameter if invalid cursor type)
 */
int L_SetCursor(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_Integer cursorType = luaL_checkinteger(L, 1);

    // Map MUSHclient cursor IDs to Qt::CursorShape
    // Based on methods_output.cpp
    Qt::CursorShape qtCursor;
    switch (cursorType) {
        case -1: // No cursor (hide cursor)
            qtCursor = Qt::BlankCursor;
            break;
        case 0: // Arrow (normal)
            qtCursor = Qt::ArrowCursor;
            break;
        case 1: // Hand (pointing hand)
            qtCursor = Qt::PointingHandCursor;
            break;
        case 2: // I-beam (text cursor)
            qtCursor = Qt::IBeamCursor;
            break;
        case 3: // Cross
            qtCursor = Qt::CrossCursor;
            break;
        case 4: // Wait (hourglass)
            qtCursor = Qt::WaitCursor;
            break;
        case 5: // Up arrow
            qtCursor = Qt::UpArrowCursor;
            break;
        case 6: // Size NW-SE
            qtCursor = Qt::SizeFDiagCursor;
            break;
        case 7: // Size NE-SW
            qtCursor = Qt::SizeBDiagCursor;
            break;
        case 8: // Size E-W (horizontal)
            qtCursor = Qt::SizeHorCursor;
            break;
        case 9: // Size N-S (vertical)
            qtCursor = Qt::SizeVerCursor;
            break;
        case 10: // Size all (four-way arrows)
            qtCursor = Qt::SizeAllCursor;
            break;
        case 11: // No (forbidden - circle with slash)
            qtCursor = Qt::ForbiddenCursor;
            break;
        case 12: // Help (question mark)
            qtCursor = Qt::WhatsThisCursor;
            break;
        default:
            // Invalid cursor type - return error
            return luaReturn(L, eBadParameter);
    }

    // Set cursor on the output view (if it exists)
    if (pDoc && pDoc->m_pActiveOutputView) {
        pDoc->m_pActiveOutputView->setViewCursor(QCursor(qtCursor));
    }

    return luaReturn(L, eOK);
}

/**
 * world.SetCommand(text)
 *
 * Sets the text in the command input window.
 * Returns eCommandNotEmpty if the input field is not empty.
 *
 * Based on methods_commands.cpp
 *
 * @param text Text to set in command input
 * @return 0 (eOK) on success, 30011 (eCommandNotEmpty) if input not empty
 */
int L_SetCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);
    QString qText = QString::fromUtf8(text, len);

    qint32 result = pDoc->SetCommand(qText);
    return luaReturn(L, result);
}

/**
 * world.SetCommandSelection(first, last)
 *
 * Sets the selection in the command input window.
 * Parameters are 1-based (use -1 for end of text).
 *
 * Based on methods_commands.cpp
 *
 * @param first Start position (1-based)
 * @param last End position (1-based, -1 for end of text)
 * @return 0 (eOK)
 */
int L_SetCommandSelection(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    auto [first, last] = luaArgs<int, int>(L);

    qint32 result = pDoc->SetCommandSelection(first, last);
    return luaReturn(L, result);
}

/**
 * GetAlphaOptionList - Get list of all string option names
 *
 * Lua signature: list = GetAlphaOptionList()
 *
 * @return Table array containing all string (alpha) option names
 *
 * Example: for i, name in ipairs(GetAlphaOptionList()) do print(name) end
 */
int L_GetAlphaOptionList(lua_State* L)
{
    lua_newtable(L);

    int index = 1;
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        lua_pushstring(L, AlphaOptionsTable[i].pName);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * GetOptionList - Get list of all numeric option names
 *
 * Lua signature: list = GetOptionList()
 *
 * @return Table array containing all numeric option names
 *
 * Example: for i, name in ipairs(GetOptionList()) do print(name) end
 */
int L_GetOptionList(lua_State* L)
{
    lua_newtable(L);

    int index = 1;
    for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
        lua_pushstring(L, OptionsTable[i].pName);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

// ========== Registration ==========

/**
 * register_setting_functions - Register all world settings and info API functions
 *
 * Called from RegisterLuaRoutines() in lua_methods.cpp
 *
 * @param L Lua state
 */
void register_setting_functions(lua_State* L)
{
    // Register functions directly in the already-created "world" table
    lua_getglobal(L, "world");

    // Font management
    lua_pushcfunction(L, L_AddFont);
    lua_setfield(L, -2, "AddFont");

    // World info functions
    lua_pushcfunction(L, L_GetInfo);
    lua_setfield(L, -2, "GetInfo");

    lua_pushcfunction(L, L_SetOption);
    lua_setfield(L, -2, "SetOption");

    lua_pushcfunction(L, L_GetOption);
    lua_setfield(L, -2, "GetOption");

    lua_pushcfunction(L, L_SetStatus);
    lua_setfield(L, -2, "SetStatus");

    // UI control functions
    lua_pushcfunction(L, L_Repaint);
    lua_setfield(L, -2, "Repaint");

    lua_pushcfunction(L, L_TextRectangle);
    lua_setfield(L, -2, "TextRectangle");

    lua_pushcfunction(L, L_SetBackgroundImage);
    lua_setfield(L, -2, "SetBackgroundImage");

    lua_pushcfunction(L, L_GetCommand);
    lua_setfield(L, -2, "GetCommand");

    lua_pushcfunction(L, L_SetCommand);
    lua_setfield(L, -2, "SetCommand");

    lua_pushcfunction(L, L_SetCommandSelection);
    lua_setfield(L, -2, "SetCommandSelection");

    lua_pushcfunction(L, L_SetCommandWindowHeight);
    lua_setfield(L, -2, "SetCommandWindowHeight");

    lua_pushcfunction(L, L_SetScroll);
    lua_setfield(L, -2, "SetScroll");

    lua_pushcfunction(L, L_SetCursor);
    lua_setfield(L, -2, "SetCursor");

    lua_pop(L, 1); // Pop "world" table
}
