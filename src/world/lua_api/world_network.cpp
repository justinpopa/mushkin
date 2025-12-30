/**
 * world_network.cpp - Network Functions
 *
 * Functions for sending data to the MUD server and managing connections.
 */

#include "lua_common.h"

/**
 * world.Send(text)
 *
 * Sends text to the MUD as if typed by the user. The text is processed through
 * aliases and added to the command queue for paced sending.
 *
 * @param text (string) Text to send to the MUD
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * Send("look")
 * Send("say Hello, world!")
 *
 * @see SendImmediate, SendNoEcho, SendSpecial
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
 * Initiates a connection to the MUD server using the world's configured
 * host and port. The connection is asynchronous; use OnConnect callback
 * or IsConnected() to check when connected.
 *
 * @return (number) Error code:
 *   - eOK (0): Connection initiated
 *   - eWorldOpen (30001): Already connected
 *
 * @example
 * if not IsConnected() then
 *     Connect()
 *     Note("Connecting...")
 * end
 *
 * @see Disconnect, IsConnected
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
 * Disconnects from the MUD server. The OnDisconnect callback will be called
 * when the disconnection is complete.
 *
 * @return (number) Error code:
 *   - eOK (0): Disconnect initiated
 *   - eWorldClosed (30002): Already disconnected
 *
 * @example
 * Disconnect()
 * Note("Disconnecting from server...")
 *
 * @see Connect, IsConnected
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
 * Checks whether the client is currently connected to the MUD server.
 *
 * @return (boolean) True if connected, false otherwise
 *
 * @example
 * if IsConnected() then
 *     Send("quit")
 * else
 *     Note("Not connected")
 * end
 *
 * @see Connect, Disconnect
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
 * Sends text to the MUD immediately, bypassing the command queue. Use this
 * when you need to send something urgently without waiting for queued commands.
 * Echoing and logging follow the world's display settings.
 *
 * @param text (string) Text to send to the MUD
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Send urgent command immediately
 * SendImmediate("flee")
 *
 * @see Send, SendNoEcho, SendSpecial
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
 * Sends text to the MUD silently - no echo to output, no queueing, and no
 * logging. Useful for sending sensitive data like passwords.
 *
 * @param text (string) Text to send to the MUD
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Send password without echoing
 * SendNoEcho(password)
 *
 * @see Send, SendImmediate, SendSpecial
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
 * Sends text to the MUD and adds it to the command history for later recall.
 * The command can be retrieved with up-arrow. Uses world's echo setting,
 * bypasses queue, and doesn't log.
 *
 * @param text (string) Text to send to the MUD
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Send and remember for history
 * SendPush("cast 'fireball' dragon")
 *
 * @see Send, SendSpecial
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
 * Sends text to the MUD with full control over all send options. This is the
 * most flexible send function, allowing precise control over echoing, queueing,
 * logging, and command history.
 *
 * @param text (string) Text to send to the MUD
 * @param echo (boolean) True to echo to output window
 * @param queue (boolean) True to use command queue, false for immediate
 * @param log (boolean) True to log to log file
 * @param history (boolean) True to add to command history
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Send immediately, echo to output, don't log or add to history
 * SendSpecial("look", true, false, false, false)
 *
 * @example
 * -- Queue command, log it, but don't echo or add to history
 * SendSpecial("tell admin " .. msg, false, true, true, false)
 *
 * @see Send, SendImmediate, SendNoEcho, SendPush
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
