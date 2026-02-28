/**
 * world_commands.cpp - Command Queue and Internal Command Functions
 *
 * Functions for queuing commands to be sent to the MUD with delays,
 * and for dispatching named internal world actions.
 */

#include "../view_interfaces.h"
#include "../xml_serialization.h"
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
 * @param echo (boolean) Whether to echo the command to output when sent (optional, defaults to
 * true)
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

// ========== Internal Command Dispatch ==========

namespace {

struct CommandEntry {
    const char* name;
    int (*handler)(WorldDocument*);
};

static const CommandEntry kCommandTable[] = {
    {"Connect",
     [](WorldDocument* pDoc) -> int {
         if (pDoc->m_connectionManager->m_iConnectPhase != CONNECT_NOT_CONNECTED)
             return eWorldOpen;
         pDoc->connectToMud();
         return eOK;
     }},
    {"Disconnect",
     [](WorldDocument* pDoc) -> int {
         qint32 phase = pDoc->m_connectionManager->m_iConnectPhase;
         if (phase == CONNECT_NOT_CONNECTED || phase == CONNECT_DISCONNECTING)
             return eWorldClosed;
         pDoc->disconnectFromMud();
         return eOK;
     }},
    {"Save",
     [](WorldDocument* pDoc) -> int {
         // Silently save to the current path; no UI dialog on empty path.
         bool success = XmlSerialization::SaveWorldXML(pDoc, pDoc->m_strWorldFilePath);
         return success ? eOK : eCouldNotOpenFile;
     }},
    {"ReloadScriptFile",
     [](WorldDocument* pDoc) -> int {
         pDoc->loadScriptFile();
         return eOK;
     }},
    {"ResetTimers",
     [](WorldDocument* pDoc) -> int {
         pDoc->resetAllTimers();
         return eOK;
     }},
    {"Pause",
     [](WorldDocument* pDoc) -> int {
         if (pDoc->m_pActiveOutputView)
             pDoc->m_pActiveOutputView->setFrozen(true);
         return eOK;
     }},
    {"Unpause",
     [](WorldDocument* pDoc) -> int {
         if (pDoc->m_pActiveOutputView)
             pDoc->m_pActiveOutputView->setFrozen(false);
         return eOK;
     }},
    {"FreezeOutput",
     [](WorldDocument* pDoc) -> int {
         if (pDoc->m_pActiveOutputView)
             pDoc->m_pActiveOutputView->setFrozen(true);
         return eOK;
     }},
    {"UnfreezeOutput",
     [](WorldDocument* pDoc) -> int {
         if (pDoc->m_pActiveOutputView)
             pDoc->m_pActiveOutputView->setFrozen(false);
         return eOK;
     }},
};

} // namespace

/**
 * world.DoCommand(command_name)
 *
 * Executes a named internal world command. This provides Lua scripts with
 * access to built-in world actions that do not have dedicated Lua functions.
 * Command names are case-sensitive and match the original MUSHclient API.
 *
 * @param command_name (string) Name of the command to execute
 *
 * @return (number) Error code:
 *   - eOK (0): Command executed successfully
 *   - eWorldOpen (30001): Connect attempted but world is already connected
 *   - eWorldClosed (30002): Disconnect attempted but world is not connected
 *   - eCouldNotOpenFile (30013): Save failed (no path or write error)
 *   - eNoSuchCommand (30054): Unknown command name
 *
 * @example
 * -- Save the world file from a trigger
 * DoCommand("Save")
 *
 * @example
 * -- Freeze output when combat starts
 * function OnCombatStart()
 *     DoCommand("FreezeOutput")
 * end
 *
 * @see GetInternalCommandsList
 */
int L_DoCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* command_name = luaL_checkstring(L, 1);

    for (const CommandEntry& entry : kCommandTable) {
        if (std::strcmp(entry.name, command_name) == 0) {
            int result = entry.handler(pDoc);
            lua_pushnumber(L, result);
            return 1;
        }
    }

    return luaReturnError(L, eNoSuchCommand);
}

/**
 * world.GetInternalCommandsList()
 *
 * Returns a Lua table (array) listing all command names accepted by DoCommand().
 * The list is in declaration order and includes alias names (e.g., both
 * "Pause" and "FreezeOutput" appear).
 *
 * @return (table) Array of command name strings
 *
 * @example
 * -- Print all available internal commands
 * local commands = GetInternalCommandsList()
 * for i, name in ipairs(commands) do
 *     Note(i .. ": " .. name)
 * end
 *
 * @see DoCommand
 */
int L_GetInternalCommandsList(lua_State* L)
{
    lua_newtable(L);
    int idx = 1;
    for (const CommandEntry& entry : kCommandTable) {
        lua_pushstring(L, entry.name);
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

// ========== Registration ==========

void register_world_command_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"Queue", L_Queue};
    *ptr++ = {"DiscardQueue", L_DiscardQueue};
    *ptr++ = {"DoCommand", L_DoCommand};
    *ptr++ = {"GetInternalCommandsList", L_GetInternalCommandsList};
}
