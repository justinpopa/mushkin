/**
 * world_network.cpp - Network Functions
 */

#include "lua_common.h"

/**
 * world.Send(text)
 *
 * Sends text to the MUD.
 *
 * @param text Text to send to the MUD
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_Send(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);

    // Check if connected
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text (cannot change what we're sending)
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Send the message
    pDoc->sendToMud(QString::fromUtf8(text));

    return luaReturnOK(L);
}

/**
 * world.Connect()
 *
 * Initiates connection to the MUD.
 *
 * @return Error code:
 *   - eOK (0): Successfully initiated connection
 *   - eWorldOpen (30001): Already connected
 */
int L_Connect(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Check if already connected
    if (pDoc->m_iConnectPhase != eConnectNotConnected) {
        return luaReturnError(L, eWorldOpen);
    }

    pDoc->connectToMud();
    return luaReturnOK(L);
}

/**
 * world.Disconnect()
 *
 * Disconnects from the MUD.
 *
 * @return Error code:
 *   - eOK (0): Successfully initiated disconnect
 *   - eWorldClosed (30002): Already disconnected or disconnecting
 */
int L_Disconnect(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Check if already disconnected or disconnecting
    if (pDoc->m_iConnectPhase == eConnectNotConnected ||
        pDoc->m_iConnectPhase == eConnectDisconnecting) {
        return luaReturnError(L, eWorldClosed);
    }

    pDoc->disconnectFromMud();
    return luaReturnOK(L);
}

/**
 * world.IsConnected()
 *
 * Checks if connected to the MUD.
 *
 * @return true if connected, false otherwise
 */
int L_IsConnected(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool connected = (pDoc->m_iConnectPhase == eConnectConnectedToMud);
    lua_pushboolean(L, connected);
    return 1;
}

/**
 * world.SendImmediate(text)
 *
 * Sends text to the MUD immediately, bypassing the command queue.
 * Uses the world's display and log settings for echo/log behavior.
 *
 * @param text Text to send to the MUD
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_SendImmediate(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);

    // Check if connected
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Send immediately (bypasses queue) using world's display/log settings
    pDoc->DoSendMsg(QString::fromUtf8(text), pDoc->m_display_my_input, pDoc->m_log_input);

    return luaReturnOK(L);
}

/**
 * world.SendNoEcho(text)
 *
 * Sends text to the MUD without echoing to output, without queueing, and without logging.
 *
 * @param text Text to send to the MUD
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_SendNoEcho(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);

    // Check if connected
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Send with no echo, no queue, no log
    pDoc->SendMsg(QString::fromUtf8(text), false, false, false);

    return luaReturnOK(L);
}

/**
 * world.SendPush(text)
 *
 * Sends text to the MUD and adds it to the command history (for recall).
 * Uses the world's display setting for echo, doesn't queue or log.
 *
 * @param text Text to send to the MUD
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_SendPush(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);

    // Check if connected
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Send using world's display setting, no queue, no log
    pDoc->SendMsg(QString::fromUtf8(text), pDoc->m_display_my_input, false, false);

    // Add to command history for recall
    pDoc->addToCommandHistory(QString::fromUtf8(text));

    return luaReturnOK(L);
}

/**
 * world.SendSpecial(text, echo, queue, log, history)
 *
 * Sends text to the MUD with full control over echoing, queueing, logging, and history.
 *
 * @param text Text to send to the MUD
 * @param echo Whether to echo the text to the output window
 * @param queue Whether to queue the command (vs send immediately)
 * @param log Whether to log the command to the log file
 * @param history Whether to add the command to command history
 * @return Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 */
int L_SendSpecial(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);
    bool echo = lua_toboolean(L, 2);
    bool queue = lua_toboolean(L, 3);
    bool log = lua_toboolean(L, 4);
    bool history = lua_toboolean(L, 5);

    // Check if connected
    if (pDoc->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Send with specified options
    pDoc->SendMsg(QString::fromUtf8(text), echo, queue, log);

    // Add to history if requested
    if (history) {
        pDoc->addToCommandHistory(QString::fromUtf8(text));
    }

    return luaReturnOK(L);
}

// ========== Registration ==========

void register_world_network_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"Send", L_Send};
    *ptr++ = {"SendImmediate", L_SendImmediate};
    *ptr++ = {"SendNoEcho", L_SendNoEcho};
    *ptr++ = {"SendPush", L_SendPush};
    *ptr++ = {"SendSpecial", L_SendSpecial};
    *ptr++ = {"Connect", L_Connect};
    *ptr++ = {"Disconnect", L_Disconnect};
    *ptr++ = {"IsConnected", L_IsConnected};
}
