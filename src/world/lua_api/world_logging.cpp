/**
 * world_logging.cpp - Logging Functions
 */

#include "lua_common.h"

/**
 * world.OpenLog(filename, append)
 *
 * Opens a log file for writing.
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
 * Closes the currently open log file.
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
 * Writes a message to the log file.
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
 * Flushes the log file to disk.
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
 * Checks if a log file is currently open.
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
