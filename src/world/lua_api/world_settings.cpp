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
#include "../../world/config_options.h"
#include "../../world/world_document.h"
#include "../lua_dialog_callbacks.h"
#include "../script_engine.h"
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFontDatabase>
#include <QHostInfo>
#include <QLocale>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QSysInfo>
#include <sqlite3.h>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "logging.h"
#include "lua_common.h"

// Application start time (set when first accessed)
static QDateTime s_applicationStartTime = QDateTime::currentDateTime();

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
 * Gets information about the world.
 *
 * Info types:
 *   1 = World name
 *   2 = World address (host:port)
 *   3 = World IP
 *   20 = Connected (boolean)
 *
 * @param type Info type number
 * @return Requested information
 */
int L_GetInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int type = luaL_checkinteger(L, 1);

    switch (type) {
        case 1: // Server address (hostname/IP)
        {
            QByteArray ba = pDoc->m_server.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 2: // World name
        {
            QByteArray ba = pDoc->m_mush_name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 3: // Character name
        {
            QByteArray ba = pDoc->m_name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 4: // Logging file preamble
        {
            QByteArray ba = pDoc->m_file_preamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 5: // Logging file postamble
        {
            QByteArray ba = pDoc->m_file_postamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 6: // Logging line preamble
        {
            QByteArray ba = pDoc->m_line_preamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 7: // Logging line postamble
        {
            QByteArray ba = pDoc->m_line_postamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 8: // Notes
        {
            QByteArray ba = pDoc->m_notes.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 9: // New activity sound
        {
            QByteArray ba = pDoc->m_new_activity_sound.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 10: // Script editor
        {
            QByteArray ba = pDoc->m_strScriptEditor.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 11: // Log file preamble
        {
            QByteArray ba = pDoc->m_strLogFilePreamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 12: // Log file postamble
        {
            QByteArray ba = pDoc->m_strLogFilePostamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 13: // Log line preamble (input)
        {
            QByteArray ba = pDoc->m_strLogLinePreambleInput.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 14: // Log line preamble (notes)
        {
            QByteArray ba = pDoc->m_strLogLinePreambleNotes.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 15: // Log line preamble (output)
        {
            QByteArray ba = pDoc->m_strLogLinePreambleOutput.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 16: // Log line postamble (input)
        {
            QByteArray ba = pDoc->m_strLogLinePostambleInput.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 17: // Log line postamble (notes)
        {
            QByteArray ba = pDoc->m_strLogLinePostambleNotes.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 18: // Log line postamble (output)
        {
            QByteArray ba = pDoc->m_strLogLinePostambleOutput.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 19: // Speed walk filler
        {
            QByteArray ba = pDoc->m_strSpeedWalkFiller.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 20: // Output font name
        {
            QByteArray ba = pDoc->m_font_name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 21: // Speed walk prefix
        {
            QByteArray ba = pDoc->m_speed_walk_prefix.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 22: // Connect text
        {
            QByteArray ba = pDoc->m_connect_text.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 23: // Input font name
        {
            QByteArray ba = pDoc->m_input_font_name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 24: // Paste postamble
        {
            QByteArray ba = pDoc->m_paste_postamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 25: // Paste preamble
        {
            QByteArray ba = pDoc->m_paste_preamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 26: // Paste line postamble
        {
            QByteArray ba = pDoc->m_pasteline_postamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 27: // Paste line preamble
        {
            QByteArray ba = pDoc->m_pasteline_preamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 28: // Script language
        {
            QByteArray ba = pDoc->m_strLanguage.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 29: // OnWorldOpen callback
        {
            QByteArray ba = pDoc->m_strWorldOpen.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 30: // OnWorldClose callback
        {
            QByteArray ba = pDoc->m_strWorldClose.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 31: // OnWorldConnect callback
        {
            QByteArray ba = pDoc->m_strWorldConnect.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 32: // OnWorldDisconnect callback
        {
            QByteArray ba = pDoc->m_strWorldDisconnect.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 33: // OnWorldGetFocus callback
        {
            QByteArray ba = pDoc->m_strWorldGetFocus.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 34: // OnWorldLoseFocus callback
        {
            QByteArray ba = pDoc->m_strWorldLoseFocus.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 35: // Script filename
        {
            QByteArray ba = pDoc->m_strScriptFilename.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 36: // Script prefix
        {
            QByteArray ba = pDoc->m_strScriptPrefix.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 37: // Auto-say string
        {
            QByteArray ba = pDoc->m_strAutoSayString.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 38: // Override prefix
        {
            QByteArray ba = pDoc->m_strOverridePrefix.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 39: // Tab completion defaults
        {
            QByteArray ba = pDoc->m_strTabCompletionDefaults.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 40: // Auto-log filename
        {
            QByteArray ba = pDoc->m_strAutoLogFileName.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 41: // Recall line preamble
        {
            QByteArray ba = pDoc->m_strRecallLinePreamble.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 42: // Terminal identification
        {
            QByteArray ba = pDoc->m_strTerminalIdentification.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 43: // Mapping failure message
        {
            QByteArray ba = pDoc->m_strMappingFailure.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 44: // OnMXP_Start callback
        {
            QByteArray ba = pDoc->m_strOnMXP_Start.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 45: // OnMXP_Stop callback
        {
            QByteArray ba = pDoc->m_strOnMXP_Stop.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 46: // OnMXP_Error callback
        {
            QByteArray ba = pDoc->m_strOnMXP_Error.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 47: // OnMXP_OpenTag callback
        {
            QByteArray ba = pDoc->m_strOnMXP_OpenTag.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 48: // OnMXP_CloseTag callback
        {
            QByteArray ba = pDoc->m_strOnMXP_CloseTag.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 49: // OnMXP_SetVariable callback
        {
            QByteArray ba = pDoc->m_strOnMXP_SetVariable.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 50: // Beep sound
        {
            QByteArray ba = pDoc->m_strBeepSound.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 51: // Log filename
        {
            QByteArray ba = pDoc->m_logfile_name.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 52: // Last immediate expression
        {
            QByteArray ba = pDoc->m_strLastImmediateExpression.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 53: // Status message
        {
            QByteArray ba = pDoc->m_strStatusMessage.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 54: // World file path (GetPathName)
        {
            QByteArray ba = pDoc->m_strWorldFilePath.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 55: // Window title (GetTitle)
        {
            QByteArray ba = pDoc->m_strWindowTitle.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 56: // MUSHclient executable path
        {
            QString appPath = QCoreApplication::applicationFilePath();
            QByteArray ba = appPath.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 57: // Default world file directory
        {
            GlobalOptions* opts = GlobalOptions::instance();
            QString dir = opts->defaultWorldFileDirectory();
            if (dir.isEmpty()) {
                dir = QCoreApplication::applicationDirPath() + "/worlds/";
            }
            QByteArray ba = dir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 58: // Default log file directory
        {
            GlobalOptions* opts = GlobalOptions::instance();
            QString dir = opts->defaultLogFileDirectory();
            if (dir.isEmpty()) {
                dir = QCoreApplication::applicationDirPath() + "/logs/";
            }
            QByteArray ba = dir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 59: // Scripts directory (executable directory)
        {
            QString appDir = QCoreApplication::applicationDirPath();
            if (!appDir.isEmpty() && !appDir.endsWith("/")) {
                appDir += "/";
            }
            QByteArray ba = appDir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 60: // Plugins directory (global)
        {
            // Return global plugins directory, resolved to absolute path
            Database* db = Database::instance();
            QString pluginsDir = db->getPreference("PluginsDirectory", "./worlds/plugins/");
            pluginsDir.replace('\\', '/');

            // If relative, resolve against application directory
            if (!QDir::isAbsolutePath(pluginsDir)) {
                QString appDir = QCoreApplication::applicationDirPath();
                pluginsDir = QDir(appDir).absoluteFilePath(pluginsDir);
            }
            pluginsDir = QDir::cleanPath(pluginsDir);

            // Ensure trailing slash for path concatenation
            if (!pluginsDir.endsWith('/')) {
                pluginsDir += '/';
            }

            QByteArray ba = pluginsDir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 61: // IP address from socket connection
        {
            QString peerAddr;
            if (pDoc->m_pSocket) {
                peerAddr = pDoc->m_pSocket->peerAddress();
            }
            QByteArray ba = peerAddr.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 62: // Proxy server (removed in original)
        {
            lua_pushstring(L, "");
        } break;

        case 63: // Hostname
        {
            QString hostname = QHostInfo::localHostName();
            QByteArray ba = hostname.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 64: // Current directory with trailing slash
        {
            QString currentDir = QDir::currentPath();
            if (!currentDir.isEmpty() && !currentDir.endsWith("/")) {
                currentDir += "/";
            }
            QByteArray ba = currentDir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 65: // OnWorldSave callback
        {
            QByteArray ba = pDoc->m_strWorldSave.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 66: // MUSHclient application directory
        {
            // Return the directory containing the application executable
            // Based on: ExtractDirectory(App.m_strMUSHclientFileName)
            QString appDir = QCoreApplication::applicationDirPath();
            // Ensure trailing slash for compatibility
            if (!appDir.isEmpty() && !appDir.endsWith("/")) {
                appDir += "/";
            }
            QByteArray ba = appDir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 67: // World file directory
        {
            QString dir;
            if (!pDoc->m_strWorldFilePath.isEmpty()) {
                QFileInfo fi(pDoc->m_strWorldFilePath);
                dir = fi.absolutePath();
                if (!dir.endsWith("/")) {
                    dir += "/";
                }
            }
            QByteArray ba = dir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 68: // Working directory
        {
            QString dir = QDir::currentPath();
            if (!dir.endsWith("/")) {
                dir += "/";
            }
            QByteArray ba = dir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 69: // Translator file
        {
            // TODO: Add translator file config
            lua_pushstring(L, "");
        } break;

        case 70: // Locale
        {
            QString locale = QLocale::system().name();
            QByteArray ba = locale.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 71: // Fixed pitch font
        {
            // TODO: Add fixed pitch font config
            lua_pushstring(L, "");
        } break;

        case 72: // Version
        {
            lua_pushstring(L, MUSHKIN_VERSION);
        } break;

        case 73: // Build date/time
        {
            QString buildDateTime = QString("%1 %2").arg(__DATE__).arg(__TIME__);
            QByteArray ba = buildDateTime.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 74: // Sounds directory
        {
            QString soundsDir = QCoreApplication::applicationDirPath();
            if (!soundsDir.isEmpty() && !soundsDir.endsWith("/")) {
                soundsDir += "/";
            }
            soundsDir += "sounds/";
            QByteArray ba = soundsDir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 75: // IAC subnegotiation data
        {
            lua_pushlstring(L, pDoc->m_IAC_subnegotiation_data.constData(),
                            pDoc->m_IAC_subnegotiation_data.size());
        } break;

        case 76: // Special font (first one)
        {
            // TODO: Add special fonts support
            lua_pushstring(L, "");
        } break;

        case 77: // OS version
        {
            QString osVersion = QSysInfo::prettyProductName();
            QByteArray ba = osVersion.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 78: // Foreground image
        {
            QByteArray ba = pDoc->m_strForegroundImageName.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 79: // Background image
        {
            QByteArray ba = pDoc->m_strBackgroundImageName.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 80: // PNG library version
        {
            // TODO: Get PNG version if using libpng
            lua_pushstring(L, "");
        } break;

        case 81: // PNG header version
        {
            // TODO: Get PNG header version
            lua_pushstring(L, "");
        } break;

        case 82: // Preferences database name
        {
            Database* db = Database::instance();
            QString path = db->databasePath();
            QByteArray ba = path.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 83: // SQLite version
        {
            lua_pushstring(L, sqlite3_libversion());
        } break;

        case 84: // File browsing directory
        {
            // TODO: Track file browsing directory
            lua_pushstring(L, "");
        } break;

        case 85: // Default state files directory
        {
            GlobalOptions* opts = GlobalOptions::instance();
            QString dir = opts->stateFilesDirectory();
            if (dir.isEmpty()) {
                dir = QCoreApplication::applicationDirPath() + "/worlds/plugins/state/";
            }
            QByteArray ba = dir.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 86: // Word under menu
        {
            QByteArray ba = pDoc->m_strWordUnderMenu.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 87: // Last command sent
        {
            QByteArray ba = pDoc->m_strLastCommandSent.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 88: // Window title (individual world)
        {
            QByteArray ba = pDoc->m_strWindowTitle.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 89: // Main window title
        {
            QByteArray ba = pDoc->m_strMainWindowTitle.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        // Boolean flags (101-125)
        case 101: // No echo (IAC WILL ECHO received)
            lua_pushboolean(L, pDoc->m_bNoEcho);
            break;

        case 102: // Debug incoming packets
            lua_pushboolean(L, pDoc->m_bDebugIncomingPackets);
            break;

        case 103: // Compress (MCCP active)
            lua_pushboolean(L, pDoc->m_bCompress);
            break;

        case 104: // MXP active
            lua_pushboolean(L, pDoc->m_bMXP);
            break;

        case 105: // Pueblo active
            lua_pushboolean(L, pDoc->m_bPuebloActive);
            break;

        case 106: // Not connected
            lua_pushboolean(L, pDoc->m_iConnectPhase != eConnectConnectedToMud);
            break;

        case 107: // Connecting (not disconnected and not connected)
            lua_pushboolean(L, pDoc->m_iConnectPhase != eConnectNotConnected &&
                                   pDoc->m_iConnectPhase != eConnectConnectedToMud);
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
        {
            lua_pushboolean(L, pDoc->isModified());
        } break;

        case 112: // Mapping enabled
        {
            // TODO: Add mapping support flag
            lua_pushboolean(L, false);
        } break;

        case 113: // Window open (has active views)
            lua_pushboolean(L, pDoc->m_pActiveOutputView != nullptr);
            break;

        case 114: // Current view frozen
        {
            // TODO: Add view freeze state
            lua_pushboolean(L, false);
        } break;

        case 115: // Translator Lua available
        {
            // TODO: Add translator support
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
            // TODO: Track full screen state
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
            lua_pushinteger(L, pDoc->m_nTotalLinesSent);
            break;

        case 204: // Input packet count
            lua_pushinteger(L, pDoc->m_iInputPacketCount);
            break;

        case 205: // Output packet count
            lua_pushinteger(L, pDoc->m_iOutputPacketCount);
            break;

        case 206: // Total uncompressed bytes (MCCP)
            lua_pushinteger(L, pDoc->m_nTotalUncompressed);
            break;

        case 207: // Total compressed bytes (MCCP)
            lua_pushinteger(L, pDoc->m_nTotalCompressed);
            break;

        case 208: // MCCP type (0=none, 1=v1, 2=v2)
            lua_pushinteger(L, pDoc->m_iMCCP_type);
            break;

        case 209: // MXP errors
            lua_pushinteger(L, pDoc->m_iMXPerrors);
            break;

        case 210: // MXP tags processed
            lua_pushinteger(L, pDoc->m_iMXPtags);
            break;

        case 211: // MXP entities processed
            lua_pushinteger(L, pDoc->m_iMXPentities);
            break;

        case 212: // Output font height
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_font_height);
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
            lua_pushinteger(L, pDoc->m_nBytesIn);
            break;

        case 217: // Bytes sent
            lua_pushinteger(L, pDoc->m_nBytesOut);
            break;

        case 218: // Count of variables
            lua_pushinteger(L, pDoc->m_VariableMap.size());
            break;

        case 219: // Count of triggers
            lua_pushinteger(L, pDoc->m_TriggerMap.size());
            break;

        case 220: // Count of timers
            lua_pushinteger(L, pDoc->m_TimerMap.size());
            break;

        case 221: // Count of aliases
            lua_pushinteger(L, pDoc->m_AliasMap.size());
            break;

        case 222: // Count of queued commands
            // TODO: Add queued commands list
            lua_pushinteger(L, 0);
            break;

        case 223: // Count of mapper items
            // TODO: Add mapper support (m_strMapList)
            lua_pushinteger(L, 0);
            break;

        case 224: // Count of lines in output window
            // Based on: methods_info.cpp
            lua_pushinteger(L, pDoc->m_lineList.count());
            break;

        case 225: // Count of custom MXP elements
            lua_pushinteger(L, pDoc->m_customElementMap.size());
            break;

        case 226: // Count of custom MXP entities
            lua_pushinteger(L, pDoc->m_customEntityMap.size());
            break;

        case 227: // Connection phase
            lua_pushinteger(L, pDoc->m_iConnectPhase);
            break;

        case 228: // IP address (as integer)
        {
            quint32 ipInt = 0;
            if (pDoc->m_pSocket) {
                QString addr = pDoc->m_pSocket->peerAddress();
                if (!addr.isEmpty()) {
                    QHostAddress hostAddr(addr);
                    // toIPv4Address returns in network order (big-endian)
                    if (hostAddr.protocol() == QAbstractSocket::IPv4Protocol) {
                        ipInt = hostAddr.toIPv4Address();
                    }
                }
            }
            lua_pushinteger(L, ipInt);
        } break;

        case 229: // Proxy (always 0 - proxy support removed)
            lua_pushinteger(L, 0);
            break;

        case 230: // Execution depth (script call depth)
            lua_pushinteger(L, pDoc->m_iExecutionDepth);
            break;

        case 231: // Log file size
            if (pDoc->m_logfile && pDoc->m_logfile->isOpen()) {
                lua_pushinteger(L, pDoc->m_logfile->size());
            } else {
                lua_pushinteger(L, 0);
            }
            break;

        case 232: // High-resolution timer (seconds since epoch)
            lua_pushnumber(L, QDateTime::currentMSecsSinceEpoch() / 1000.0);
            break;

        case 233: // Time taken doing triggers (seconds)
            // TODO: Add trigger timing support
            lua_pushnumber(L, 0.0);
            break;

        case 234: // Time taken doing aliases (seconds)
            // TODO: Add alias timing support
            lua_pushnumber(L, 0.0);
            break;

        case 235: // Number of world windows open
            // Always 1 in Qt version (single world per window)
            lua_pushinteger(L, 1);
            break;

        case 236: // Command selection start column
            // TODO: Add input selection tracking
            lua_pushinteger(L, 0);
            break;

        case 237: // Command selection end column
            // TODO: Add input selection tracking
            lua_pushinteger(L, 0);
            break;

        case 238: // Window placement flags
            // TODO: Add window state tracking
            lua_pushinteger(L, 0);
            break;

        case 239: // Current action source (trigger/alias/timer/etc)
            lua_pushinteger(L, pDoc->m_iCurrentActionSource);
            break;

        case 240: // Average character width
            lua_pushinteger(L, pDoc->m_FontWidth);
            break;

        case 241: // Character height
            lua_pushinteger(L, pDoc->m_font_height);
            break;

        case 242: // UTF-8 error count
            lua_pushinteger(L, pDoc->m_iUTF8ErrorCount);
            break;

        case 243: // Fixed pitch font size
            // TODO: Add global font size config
            lua_pushinteger(L, 10);
            break;

        case 244: // Triggers evaluated count
            lua_pushinteger(L, pDoc->m_iTriggersEvaluatedCount);
            break;

        case 245: // Triggers matched count
            lua_pushinteger(L, pDoc->m_iTriggersMatchedCount);
            break;

        case 246: // Aliases evaluated count
            lua_pushinteger(L, pDoc->m_iAliasesEvaluatedCount);
            break;

        case 247: // Aliases matched count
            lua_pushinteger(L, pDoc->m_iAliasesMatchedCount);
            break;

        case 248: // Timers fired count
            lua_pushinteger(L, pDoc->m_iTimersFiredCount);
            break;

        case 249: // Main window client height
            // TODO: Add main window size tracking
            lua_pushinteger(L, 0);
            break;

        case 250: // Main window client width
            // TODO: Add main window size tracking
            lua_pushinteger(L, 0);
            break;

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
            // TODO: Add status bar tracking
            lua_pushinteger(L, 0);
            break;

        case 260: // Status bar width
            // TODO: Add status bar tracking
            lua_pushinteger(L, 0);
            break;

        case 261: // World window non-client height
            // TODO: Add window frame tracking
            lua_pushinteger(L, 0);
            break;

        case 262: // World window non-client width
            // TODO: Add window frame tracking
            lua_pushinteger(L, 0);
            break;

        case 263: // World window client height
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->height());
            }
            break;

        case 264: // World window client width
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->width());
            }
            break;

        case 265: // OS major version
        {
            QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
            lua_pushinteger(L, osVer.majorVersion());
        } break;

        case 266: // OS minor version
        {
            QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
            lua_pushinteger(L, osVer.minorVersion());
        } break;

        case 267: // OS build number
        {
            QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
            // microVersion() returns patch/build number
            lua_pushinteger(L, osVer.microVersion());
        } break;

        case 268: // OS platform ID
        {
            // Windows platform IDs: 0=Win32s, 1=Win9x, 2=NT-based
            // For cross-platform: 2=Windows, 3=macOS, 4=Linux
            int platformId = 0;
#ifdef Q_OS_WIN
            platformId = 2; // VER_PLATFORM_WIN32_NT
#elif defined(Q_OS_MACOS)
            platformId = 3; // macOS (custom)
#elif defined(Q_OS_LINUX)
            platformId = 4; // Linux (custom)
#endif
            lua_pushinteger(L, platformId);
        } break;

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
                lua_pushinteger(L, pDoc->m_pActiveOutputView->height());
            }
            // Note: returns nil if view doesn't exist - matches original behavior
            break;

        case 281: // Output window client width
            // Based on: methods_info.cpp
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->width());
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
            lua_pushinteger(L, pDoc->m_iTriggersMatchedThisSessionCount);
            break;

        case 287: // Aliases matched this session
            lua_pushinteger(L, pDoc->m_iAliasesMatchedThisSessionCount);
            break;

        case 288: // Timers fired this session
            lua_pushinteger(L, pDoc->m_iTimersFiredThisSessionCount);
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

        case 294: // Scroll bar max position
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->getMaxScrollPosition());
            } else {
                lua_pushinteger(L, 0);
            }
            break;

        case 295: // Scroll bar page size (visible lines)
            if (pDoc->m_pActiveOutputView) {
                lua_pushinteger(L, pDoc->m_pActiveOutputView->getVisibleLines());
            } else {
                lua_pushinteger(L, 0);
            }
            break;

        case 296: // Output window scroll bar position
            // Based on: methods_info.cpp
            if (pDoc->m_pActiveOutputView) {
                // Return current scroll position in pixels (Y coordinate)
                lua_pushinteger(L, pDoc->m_pActiveOutputView->getScrollPositionPixels());
            } else {
                lua_pushnil(L);
            }
            break;

        case 297: // Horizontal scroll bar position
            // TODO: Add horizontal scroll bar tracking
            lua_pushinteger(L, 0);
            break;

        case 298: // Horizontal scroll bar max position
            // TODO: Add horizontal scroll bar tracking
            lua_pushinteger(L, 0);
            break;

        case 299: // Horizontal scroll bar page size
            // TODO: Add horizontal scroll bar tracking
            lua_pushinteger(L, 0);
            break;

        case 300: // Commands in history buffer
            lua_pushinteger(L, pDoc->m_commandHistory.count());
            break;

        case 301: // Number of sent packets
            // TODO: Distinguish between sent packets and sent lines
            lua_pushinteger(L, pDoc->m_nTotalLinesSent);
            break;

        case 302: // Connect time (seconds since connected)
            if (pDoc->m_tConnectTime.isValid()) {
                lua_pushnumber(L, pDoc->m_tConnectTime.secsTo(QDateTime::currentDateTime()));
            } else {
                lua_pushnumber(L, 0.0);
            }
            break;

        case 303: // Number of MXP elements (custom/user-defined)
            lua_pushinteger(L, pDoc->m_customElementMap.size());
            break;

        case 304: // Locale
        {
            QString locale = QLocale::system().name();
            QByteArray ba = locale.toUtf8();
            lua_pushlstring(L, ba.constData(), ba.length());
        } break;

        case 305: // Client start time (when application started)
        {
            // OLE date format: days since December 30, 1899 (including fractional days)
            // Convert: OLE = (Unix timestamp / 86400.0) + 25569.0
            // 25569 = days between Dec 30, 1899 and Jan 1, 1970
            double oleDate = (s_applicationStartTime.toSecsSinceEpoch() / 86400.0) + 25569.0;
            lua_pushnumber(L, oleDate);
        } break;

        case 306: // World start time (when world connected/started)
        {
            // OLE date format for compatibility with original MUSHclient
            if (pDoc->m_whenWorldStarted.isValid()) {
                double oleDate = (pDoc->m_whenWorldStarted.toSecsSinceEpoch() / 86400.0) + 25569.0;
                lua_pushnumber(L, oleDate);
            } else {
                lua_pushnumber(L, 0.0);
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
 * world.GetWorldName()
 *
 * Gets the world name.
 *
 * @return World name
 */
int L_GetWorldName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QByteArray ba = pDoc->m_mush_name.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
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

    const char* optionName = luaL_checkstring(L, 1);

    // Convert boolean to 0 or 1, nil to 0
    double value;
    if (lua_isboolean(L, 2)) {
        value = lua_toboolean(L, 2) ? 1 : 0;
    } else if (lua_isnil(L, 2)) {
        value = 0;
    } else {
        value = luaL_checknumber(L, 2);
    }

    // Find the option in OptionsTable
    int optionIndex = -1;
    QString optionNameStr = QString::fromUtf8(optionName);
    for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
        if (optionNameStr.compare(OptionsTable[i].pName, Qt::CaseInsensitive) == 0) {
            optionIndex = i;
            break;
        }
    }

    if (optionIndex == -1) {
        return luaReturnError(L, eUnknownOption);
    }

    // Check if plugin is allowed to write this option
    if (pDoc->m_CurrentPlugin && (OptionsTable[optionIndex].iFlags & OPT_PLUGIN_CANNOT_WRITE)) {
        return luaReturnError(L, ePluginCannotSetOption);
    }

    // Set the option value based on its offset and length
    const tConfigurationNumericOption& opt = OptionsTable[optionIndex];
    char* basePtr = reinterpret_cast<char*>(pDoc);
    char* fieldPtr = basePtr + opt.iOffset;

    // Clamp value to min/max if specified
    if (opt.iMinimum != 0 || opt.iMaximum != 0) {
        if (value < opt.iMinimum)
            value = opt.iMinimum;
        if (value > opt.iMaximum)
            value = opt.iMaximum;
    }

    // Write value based on field length
    switch (opt.iLength) {
        case 1:
            *reinterpret_cast<qint8*>(fieldPtr) = static_cast<qint8>(value);
            break;
        case 2:
            *reinterpret_cast<qint16*>(fieldPtr) = static_cast<qint16>(value);
            break;
        case 4:
            *reinterpret_cast<qint32*>(fieldPtr) = static_cast<qint32>(value);
            break;
        case 8:
            if (opt.iFlags & OPT_DOUBLE) {
                *reinterpret_cast<double*>(fieldPtr) = value;
            } else {
                *reinterpret_cast<qint64*>(fieldPtr) = static_cast<qint64>(value);
            }
            break;
    }

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

    // Search numeric options table first
    for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
        const tConfigurationNumericOption& opt = OptionsTable[i];

        if (strcmp(opt.pName, optionName) == 0) {
            // Calculate pointer to field in document
            const char* basePtr = reinterpret_cast<const char*>(pDoc);
            const char* fieldPtr = basePtr + opt.iOffset;

            double value = 0.0;

            // Read value based on field length
            switch (opt.iLength) {
                case 1: // unsigned char or bool
                    if (opt.iMaximum == 0.0 && opt.iMinimum == 0.0) {
                        // Boolean option
                        value = *reinterpret_cast<const bool*>(fieldPtr) ? 1.0 : 0.0;
                    } else {
                        value = *reinterpret_cast<const quint8*>(fieldPtr);
                    }
                    break;

                case 2: // quint16
                    value = *reinterpret_cast<const quint16*>(fieldPtr);
                    break;

                case 4: // quint32 or int
                    if (opt.iFlags & OPT_DOUBLE) {
                        // Actually a float (should be rare)
                        value = *reinterpret_cast<const float*>(fieldPtr);
                    } else {
                        value = *reinterpret_cast<const qint32*>(fieldPtr);
                    }
                    break;

                case 8: // double or qint64
                    if (opt.iFlags & OPT_DOUBLE) {
                        value = *reinterpret_cast<const double*>(fieldPtr);
                    } else {
                        value = *reinterpret_cast<const qint64*>(fieldPtr);
                    }
                    break;

                default:
                    lua_pushnil(L);
                    return 1;
            }

            lua_pushnumber(L, value);
            return 1;
        }
    }

    // Search alpha (string) options table
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

        if (strcmp(opt.pName, optionName) == 0) {
            // Calculate pointer to field in document
            const char* basePtr = reinterpret_cast<const char*>(pDoc);
            const char* fieldPtr = basePtr + opt.iOffset;

            // Read QString value
            const QString* strPtr = reinterpret_cast<const QString*>(fieldPtr);
            lua_pushstring(L, strPtr->toUtf8().constData());
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
    const char* optionName = luaL_checkstring(L, 1);

    // Normalize option name: lowercase, trimmed (matches original)
    QString normalizedName = QString::fromUtf8(optionName).toLower().trimmed();

    // Search alpha (string) options table only
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

        if (normalizedName == opt.pName) {
            // Check if plugin is allowed to read this option
            if (pDoc->m_CurrentPlugin && (opt.iFlags & OPT_PLUGIN_CANNOT_READ)) {
                lua_pushnil(L);
                return 1;
            }

            // Calculate pointer to field in document
            const char* basePtr = reinterpret_cast<const char*>(pDoc);
            const char* fieldPtr = basePtr + opt.iOffset;

            // Read QString value
            const QString* strPtr = reinterpret_cast<const QString*>(fieldPtr);
            lua_pushstring(L, strPtr->toUtf8().constData());
            return 1;
        }
    }

    // Option not found
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
    const char* optionName = luaL_checkstring(L, 1);
    const char* value = luaL_checkstring(L, 2);

    // Normalize option name: lowercase, trimmed (matches original)
    QString normalizedName = QString::fromUtf8(optionName).toLower().trimmed();
    QString strValue = QString::fromUtf8(value);

    // Search alpha (string) options table
    for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
        const tConfigurationAlphaOption& opt = AlphaOptionsTable[i];

        if (normalizedName == opt.pName) {
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
                    pDoc->m_enable_command_stack = false;
                    return luaReturnError(L, eOptionOutOfRange);
                }
                QChar ch = strValue.at(0);
                if (!ch.isPrint() || ch.isSpace()) {
                    pDoc->m_enable_command_stack = false;
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

            // Calculate pointer to field in document
            char* basePtr = reinterpret_cast<char*>(pDoc);
            char* fieldPtr = basePtr + opt.iOffset;

            // Write QString value
            QString* strPtr = reinterpret_cast<QString*>(fieldPtr);
            *strPtr = strValue;

            // Handle update flags if needed
            // TODO: Handle OPT_UPDATE_VIEWS, OPT_UPDATE_INPUT_FONT, etc.

            return luaReturnOK(L);
        }
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
    const char* text = luaL_checkstring(L, 1);

    pDoc->m_strStatusMessage = QString::fromUtf8(text);
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

    qint32 left = luaL_checkinteger(L, 1);
    qint32 top = luaL_checkinteger(L, 2);
    qint32 right = luaL_checkinteger(L, 3);
    qint32 bottom = luaL_checkinteger(L, 4);
    qint32 borderOffset = luaL_checkinteger(L, 5);
    QRgb borderColour = luaL_checkinteger(L, 6);
    qint32 borderWidth = luaL_checkinteger(L, 7);
    QRgb outsideFillColour = luaL_checkinteger(L, 8);
    qint32 outsideFillStyle = luaL_checkinteger(L, 9);

    qCDebug(lcScript) << "TextRectangle called: rect=(" << left << top << right << bottom
                      << ") border=" << borderOffset << borderWidth;

    // Store configuration
    // NOTE: Lua passes left, top, right, bottom (coordinates)
    // but QRect(int, int, int, int) expects x, y, width, height
    // Use QPoint constructor to create from coordinates
    pDoc->m_TextRectangle = QRect(QPoint(left, top), QPoint(right, bottom));
    pDoc->m_TextRectangleBorderOffset = borderOffset;
    pDoc->m_TextRectangleBorderColour = borderColour;
    pDoc->m_TextRectangleBorderWidth = borderWidth;
    pDoc->m_TextRectangleOutsideFillColour = outsideFillColour;
    pDoc->m_TextRectangleOutsideFillStyle = outsideFillStyle;

    // Notify OutputView that text rectangle config changed
    // (OutputView will recalculate metrics and update m_computedTextRectangle)
    emit pDoc->textRectangleConfigChanged();

    // Trigger redraw
    pDoc->Repaint();

    lua_pushinteger(L, 0); // eOK
    return 1;
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
    const char* filename = luaL_optstring(L, 1, "");
    qint32 mode = luaL_optinteger(L, 2, 0);

    // Validate mode (matches original)
    if (mode < 0 || mode > 13) {
        lua_pushinteger(L, eBadParameter);
        return 1;
    }

    // Store the image path and mode
    pDoc->m_strBackgroundImageName = QString::fromUtf8(filename);
    pDoc->m_iBackgroundMode = mode;

    // Tell OutputView to reload the image via callback (avoids ui module dependency)
    auto callback = ViewUpdateCallbacks::getReloadBackgroundImageCallback();
    if (callback) {
        callback(pDoc);
    }

    lua_pushinteger(L, eOK);
    return 1;
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
int L_GetCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString command = pDoc->GetCommand();
    QByteArray ba = command.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

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
    lua_pushinteger(L, 0); // eOK
    return 1;
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
    lua_pushinteger(L, 0); // eOK
    return 1;
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
            lua_pushinteger(L, eBadParameter);
            return 1;
    }

    // Set cursor on the output view (if it exists)
    if (pDoc && pDoc->m_pActiveOutputView) {
        pDoc->m_pActiveOutputView->setCursor(QCursor(qtCursor));
    }

    lua_pushinteger(L, eOK);
    return 1;
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
    lua_pushinteger(L, result);
    return 1;
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
    qint32 first = luaL_checkinteger(L, 1);
    qint32 last = luaL_checkinteger(L, 2);

    qint32 result = pDoc->SetCommandSelection(first, last);
    lua_pushinteger(L, result);
    return 1;
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

    lua_pushcfunction(L, L_GetWorldName);
    lua_setfield(L, -2, "GetWorldName");

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
