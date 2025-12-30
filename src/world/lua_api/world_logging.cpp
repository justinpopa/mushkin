/**
 * world_logging.cpp - Logging Functions
 *
 * Functions for logging MUD output and custom messages to files.
 */

#include "lua_common.h"

/**
 * world.OpenLog(filename, append)
 *
 * Opens a log file for writing. If a log file is already open, it is closed
 * first. The log file can be used to record MUD output, notes, and custom
 * messages for later review.
 *
 * @param filename (string) Path to the log file (optional, uses default if omitted)
 * @param append (boolean) If true, append to existing file; if false, overwrite (optional, defaults to false)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eLogFileNotOpen (30020): Failed to open file
 *
 * @example
 * -- Start logging to a new file
 * OpenLog("session.log", false)
 *
 * @example
 * -- Continue logging to existing file
 * OpenLog("combat.log", true)
 *
 * @see CloseLog, WriteLog, IsLogOpen
 */
int L_OpenLog(lua_State* L) {
    WorldDocument* pDoc = doc(L);

    QString filename;
    if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) {
        filename = QString::fromUtf8(luaL_checkstring(L, 1));
    }

    bool append = false;
    if (lua_gettop(L) >= 2) {
        append = lua_toboolean(L, 2);
    }

    qint32 result = pDoc->OpenLog(filename, append);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.CloseLog()
 *
 * Closes the currently open log file and flushes any buffered data to disk.
 * Safe to call even if no log file is open.
 *
 * @return (number) Error code:
 *   - eOK (0): Success (or no log was open)
 *
 * @example
 * -- Close the log when done
 * CloseLog()
 *
 * @see OpenLog, FlushLog, IsLogOpen
 */
int L_CloseLog(lua_State* L) {
    WorldDocument* pDoc = doc(L);

    qint32 result = pDoc->CloseLog();

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WriteLog(message)
 *
 * Writes a custom message to the currently open log file. The message is
 * written exactly as provided, without any automatic newlines or formatting.
 * Include "\n" in your message if you want a line break.
 *
 * @param message (string) Text to write to the log file
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eLogFileNotOpen (30020): No log file is open
 *
 * @example
 * WriteLog("=== Combat Started ===\n")
 * WriteLog("Target: " .. target_name .. "\n")
 *
 * @see OpenLog, FlushLog, CloseLog
 */
int L_WriteLog(lua_State* L) {
    WorldDocument* pDoc = doc(L);

    QString message = QString::fromUtf8(luaL_checkstring(L, 1));

    qint32 result = pDoc->WriteLog(message);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.FlushLog()
 *
 * Flushes any buffered log data to disk immediately. Normally log data is
 * buffered for performance; use this to ensure data is written before a
 * potential crash or when you need to read the log file from another program.
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eLogFileNotOpen (30020): No log file is open
 *
 * @example
 * -- Ensure critical event is saved
 * WriteLog("CRITICAL: Player died!\n")
 * FlushLog()
 *
 * @see OpenLog, WriteLog, CloseLog
 */
int L_FlushLog(lua_State* L) {
    WorldDocument* pDoc = doc(L);

    qint32 result = pDoc->FlushLog();

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.IsLogOpen()
 *
 * Checks whether a log file is currently open for writing.
 *
 * @return (boolean) True if a log file is open, false otherwise
 *
 * @example
 * if not IsLogOpen() then
 *     OpenLog("session.log")
 * end
 * WriteLog("This will work now\n")
 *
 * @see OpenLog, CloseLog
 */
int L_IsLogOpen(lua_State* L) {
    WorldDocument* pDoc = doc(L);

    bool isOpen = pDoc->IsLogOpen();

    lua_pushboolean(L, isOpen);
    return 1;
}

// ========== Registration ==========

void register_world_logging_functions(luaL_Reg* &ptr) {
    *ptr++ = {"OpenLog", L_OpenLog};
    *ptr++ = {"CloseLog", L_CloseLog};
    *ptr++ = {"WriteLog", L_WriteLog};
    *ptr++ = {"FlushLog", L_FlushLog};
    *ptr++ = {"IsLogOpen", L_IsLogOpen};
}
