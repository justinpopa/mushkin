/**
 * world_commands.cpp - Command Queue Functions
 */

#include "lua_common.h"

/**
 * world.Queue(message, echo)
 *
 * Queues a command to be sent to the MUD.
 * The command will be sent according to the speedwalk delay setting.
 *
 * @param message Command text to queue
 * @param echo Whether to echo the command when sent (optional, defaults to true)
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_Queue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* message = luaL_checkstring(L, 1);
    bool echo = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

    qint32 result = pDoc->Queue(QString::fromUtf8(message), echo);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.DiscardQueue()
 *
 * Clears all queued commands.
 *
 * @return Number of commands that were discarded
 */
int L_DiscardQueue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 count = pDoc->DiscardQueue();
    lua_pushnumber(L, count);
    return 1;
}

// ========== Registration ==========

void register_world_command_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"Queue", L_Queue};
    *ptr++ = {"DiscardQueue", L_DiscardQueue};
}
