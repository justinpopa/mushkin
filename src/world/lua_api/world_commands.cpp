/**
 * world_commands.cpp - Command Queue Functions
 *
 * Functions for queuing commands to be sent to the MUD with delays.
 */

#include "lua_common.h"

/**
 * world.Queue(message, echo)
 *
 * Queues a command to be sent to the MUD. Commands in the queue are sent
 * one at a time with a delay between them (controlled by speedwalk delay
 * setting). This is useful for sending multiple commands without flooding
 * the MUD.
 *
 * @param message (string) Command text to queue for sending
 * @param echo (boolean) Whether to echo the command to output when sent (optional, defaults to true)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Queue multiple commands with automatic pacing
 * Queue("north")
 * Queue("east")
 * Queue("open door")
 * Queue("north")
 *
 * @example
 * -- Queue a silent command (not echoed to output)
 * Queue("password123", false)
 *
 * @see DiscardQueue, Send, DoAfterSpecial
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
 * Clears all queued commands, preventing them from being sent. Use this to
 * cancel a queued speedwalk or command sequence when circumstances change
 * (e.g., entering combat, receiving an error message).
 *
 * @return (number) Count of commands that were discarded from the queue
 *
 * @example
 * -- Cancel queued commands when combat starts
 * function OnCombatStart()
 *     local discarded = DiscardQueue()
 *     if discarded > 0 then
 *         Note("Cancelled " .. discarded .. " queued commands")
 *     end
 * end
 *
 * @see Queue
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
