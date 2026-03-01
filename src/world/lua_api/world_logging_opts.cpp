/**
 * world_logging_opts.cpp - Logging Get/Set and Speedwalk API Functions
 */

#include "lua_common.h"

#include "../speedwalk_engine.h"

// Forward declarations for functions defined in this file that call each other
static int L_GetEchoInput_impl(lua_State* L);
static int L_SetEchoInput_impl(lua_State* L);
static int L_GetSpeedWalkDelay_impl(lua_State* L);
static int L_SetSpeedWalkDelay_impl(lua_State* L);
static int L_GetLogInput_impl(lua_State* L);
static int L_SetLogInput_impl(lua_State* L);
static int L_GetLogNotes_impl(lua_State* L);
static int L_SetLogNotes_impl(lua_State* L);
static int L_GetLogOutput_impl(lua_State* L);
static int L_SetLogOutput_impl(lua_State* L);

/**
 * world.GetLogInput()
 *
 * Returns whether input logging is enabled.
 * When enabled, commands you send are written to the log file.
 *
 * @return (boolean) true if input logging is enabled
 *
 * @example
 * if GetLogInput() then
 *     Note("Your commands are being logged")
 * end
 *
 * @see SetLogInput, GetLogOutput, GetLogNotes
 */
int L_GetLogInput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_logging.log_input);
    return 1;
}

/**
 * world.SetLogInput(enable)
 *
 * Enables or disables input logging.
 * When enabled, commands you send are written to the log file.
 *
 * @param enable (boolean, optional) true to enable, false to disable. Default: true
 *
 * @example
 * -- Enable logging of commands
 * SetLogInput(true)
 *
 * -- Disable command logging for privacy
 * SetLogInput(false)
 * Send(password)
 * SetLogInput(true)
 *
 * @see GetLogInput, SetLogOutput, SetLogNotes
 */
int L_SetLogInput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true if no argument provided (matches original optboolean behavior)
    bool enable = lua_isnone(L, 1) ? true : lua_toboolean(L, 1);
    pDoc->m_logging.log_input = enable;
    return 0;
}

/**
 * world.GetLogNotes()
 *
 * Returns whether notes logging is enabled.
 * Notes are text from Note(), ColourNote(), etc. script functions.
 *
 * @return (boolean) true if notes logging is enabled
 *
 * @example
 * if GetLogNotes() then
 *     Note("Script notes are being logged")
 * end
 *
 * @see SetLogNotes, GetLogInput, GetLogOutput
 */
int L_GetLogNotes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_logging.log_notes);
    return 1;
}

/**
 * world.SetLogNotes(enable)
 *
 * Enables or disables notes logging.
 * When enabled, Note(), ColourNote(), etc. output is written to the log file.
 *
 * @param enable (boolean, optional) true to enable, false to disable. Default: true
 *
 * @example
 * -- Include script output in log
 * SetLogNotes(true)
 *
 * -- Exclude script output from log
 * SetLogNotes(false)
 *
 * @see GetLogNotes, SetLogInput, SetLogOutput
 */
int L_SetLogNotes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true if no argument provided (matches original optboolean behavior)
    bool enable = lua_isnone(L, 1) ? true : lua_toboolean(L, 1);
    pDoc->m_logging.log_notes = enable;
    return 0;
}

/**
 * world.GetLogOutput()
 *
 * Returns whether MUD output logging is enabled.
 * When enabled, lines received from the MUD are written to the log file.
 *
 * @return (boolean) true if output logging is enabled
 *
 * @example
 * if GetLogOutput() then
 *     Note("MUD output is being logged")
 * end
 *
 * @see SetLogOutput, GetLogInput, GetLogNotes
 */
int L_GetLogOutput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_logging.log_output);
    return 1;
}

/**
 * world.SetLogOutput(enable)
 *
 * Enables or disables MUD output logging.
 * When enabled, lines received from the MUD are written to the log file.
 *
 * @param enable (boolean, optional) true to enable, false to disable. Default: true
 *
 * @example
 * -- Enable logging of MUD output
 * SetLogOutput(true)
 *
 * -- Temporarily disable output logging
 * SetLogOutput(false)
 * -- ... spam section ...
 * SetLogOutput(true)
 *
 * @see GetLogOutput, SetLogInput, SetLogNotes
 */
int L_SetLogOutput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true if no argument provided (matches original optboolean behavior)
    bool enable = lua_isnone(L, 1) ? true : lua_toboolean(L, 1);
    pDoc->m_logging.log_output = enable;
    return 0;
}

// ========== Bare-name compatibility aliases (dual get/set dispatch) ==========

/**
 * world.EchoInput([enable])
 *
 * Compatibility alias for the MUSHclient COM property EchoInput.
 * With no arguments acts as a getter (returns current echo state).
 * With one argument acts as a setter.
 *
 * @param enable (boolean, optional) true to enable, false to disable
 *
 * @return (boolean|nothing) Current echo state when getting; nothing when setting
 *
 * @see GetEchoInput, SetEchoInput
 */
int L_EchoInput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (lua_gettop(L) >= 1) {
        bool enable = lua_toboolean(L, 1);
        pDoc->m_display_my_input = enable;
        return 0;
    }
    lua_pushboolean(L, pDoc->m_display_my_input);
    return 1;
}

/**
 * world.SpeedWalkDelay([delay])
 *
 * Compatibility alias for the MUSHclient COM property SpeedWalkDelay.
 * With no arguments acts as a getter (returns current delay in ms).
 * With one argument acts as a setter.
 *
 * @param delay (number, optional) Delay in milliseconds between speedwalk commands
 *
 * @return (number|nothing) Current delay when getting; nothing when setting
 *
 * @see GetSpeedWalkDelay, SetSpeedWalkDelay
 */
int L_SpeedWalkDelay(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (lua_gettop(L) >= 1) {
        int delay = luaL_checkinteger(L, 1);
        pDoc->m_speedwalk.delay = delay;
        return 0;
    }
    lua_pushinteger(L, pDoc->m_speedwalk.delay);
    return 1;
}

/**
 * world.LogInput([enable])
 *
 * Compatibility alias for the MUSHclient COM property LogInput.
 * With no arguments acts as a getter; with one argument acts as a setter.
 *
 * @param enable (boolean, optional) true to enable, false to disable
 *
 * @return (boolean|nothing) Current state when getting; nothing when setting
 *
 * @see GetLogInput, SetLogInput
 */
int L_LogInput(lua_State* L)
{
    if (lua_gettop(L) >= 1)
        return L_SetLogInput(L);
    return L_GetLogInput(L);
}

/**
 * world.LogNotes([enable])
 *
 * Compatibility alias for the MUSHclient COM property LogNotes.
 * With no arguments acts as a getter; with one argument acts as a setter.
 *
 * @param enable (boolean, optional) true to enable, false to disable
 *
 * @return (boolean|nothing) Current state when getting; nothing when setting
 *
 * @see GetLogNotes, SetLogNotes
 */
int L_LogNotes(lua_State* L)
{
    if (lua_gettop(L) >= 1)
        return L_SetLogNotes(L);
    return L_GetLogNotes(L);
}

/**
 * world.LogOutput([enable])
 *
 * Compatibility alias for the MUSHclient COM property LogOutput.
 * With no arguments acts as a getter; with one argument acts as a setter.
 *
 * @param enable (boolean, optional) true to enable, false to disable
 *
 * @return (boolean|nothing) Current state when getting; nothing when setting
 *
 * @see GetLogOutput, SetLogOutput
 */
int L_LogOutput(lua_State* L)
{
    if (lua_gettop(L) >= 1)
        return L_SetLogOutput(L);
    return L_GetLogOutput(L);
}

/**
 * world.LogSend(message, ...)
 *
 * Sends a message to the MUD and logs it regardless of log_input setting.
 * Useful when you want to ensure specific important commands are always logged.
 * Multiple arguments are concatenated together.
 *
 * @param message (string) Message(s) to send and log (concatenated)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eWorldClosed (30002): Not connected to MUD
 *   - eItemInUse (30063): Plugin is processing sent text
 *
 * @example
 * -- Always log important commands even if input logging is off
 * LogSend("say I need help!")
 *
 * -- Log a command with values
 * LogSend("deposit ", gold_amount, " gold")
 *
 * @see Send, SetLogInput
 */
int L_LogSend(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Check if connected
    if (pDoc->m_connectionManager->m_iConnectPhase != eConnectConnectedToMud) {
        return luaReturnError(L, eWorldClosed);
    }

    // Check if plugin is processing sent text
    if (pDoc->m_bPluginProcessingSent) {
        return luaReturnError(L, eItemInUse);
    }

    // Concatenate all arguments (matches original concatArgs behavior)
    QString text = concatArgs(L);

    // Send the message
    pDoc->sendToMud(text);

    // Log the command unconditionally (regardless of m_log_input setting)
    if (pDoc->IsLogOpen()) {
        pDoc->logCommand(text);
    }

    return luaReturnOK(L);
}

/**
 * world.GetSpeedWalkDelay()
 *
 * Returns the speedwalk delay in milliseconds.
 * This is the delay between sending each command during speedwalk.
 *
 * @return (number) Delay in milliseconds between speedwalk commands
 *
 * @example
 * local delay = GetSpeedWalkDelay()
 * Note("Speedwalk delay: " .. delay .. "ms")
 *
 * @see SetSpeedWalkDelay, Speedwalk
 */
int L_GetSpeedWalkDelay(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushinteger(L, pDoc->m_speedwalk.delay);
    return 1;
}

/**
 * world.SetSpeedWalkDelay(delay)
 *
 * Sets the speedwalk delay in milliseconds.
 * Lower values make speedwalk faster, higher values add more delay.
 *
 * @param delay (number) Delay in milliseconds between speedwalk commands
 *
 * @example
 * -- Fast speedwalk
 * SetSpeedWalkDelay(100)
 *
 * -- Slow, cautious speedwalk
 * SetSpeedWalkDelay(1000)
 *
 * -- Adjust based on lag
 * if GetInfo(248) > 500 then  -- If lag is high
 *     SetSpeedWalkDelay(2000)
 * end
 *
 * @see GetSpeedWalkDelay, Speedwalk
 */
int L_SetSpeedWalkDelay(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int delay = luaL_checkinteger(L, 1);
    pDoc->m_speedwalk.delay = delay;
    // Not applicable: Windows MFC timer list window refresh. Timer changes take effect on next
    // check.
    return 0;
}

/**
 * world.EvaluateSpeedwalk(speedwalk)
 *
 * Parses speedwalk notation and expands it to individual movement commands.
 * Speedwalk notation uses shorthand for multiple directional commands.
 *
 * Format: [count]direction repeated, e.g., "3n2e" means "n n n e e"
 *
 * Direction codes: n(orth), s(outh), e(ast), w(est),
 *                  u(p), d(own), ne, nw, se, sw
 *
 * @param speedwalk (string) Speedwalk notation string
 *
 * @return (string) Newline-separated movement commands, or error starting with "*"
 *
 * @example
 * local expanded = EvaluateSpeedwalk("3n2e")
 * -- Returns: "north\nnorth\nnorth\neast\neast\n"
 *
 * local path = EvaluateSpeedwalk("n3e2su")
 * -- Returns: "north\neast\neast\neast\nsouth\nsouth\nup\n"
 *
 * -- Check for errors
 * local result = EvaluateSpeedwalk("3x")  -- Invalid direction
 * if result:sub(1,1) == "*" then
 *     Note("Error: " .. result)
 * end
 *
 * @see ReverseSpeedwalk, RemoveBacktracks, Speedwalk
 */
int L_EvaluateSpeedwalk(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* speedwalk = luaL_checkstring(L, 1);
    QString result = speedwalk::evaluate(QString::fromUtf8(speedwalk), pDoc->m_speedwalk.filler);
    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}

/**
 * world.ReverseSpeedwalk(speedwalk)
 *
 * Reverses a speedwalk string to create the return path.
 * Each direction is reversed (n→s, e→w, u→d, etc.) and the order is flipped.
 *
 * @param speedwalk (string) Speedwalk notation string
 *
 * @return (string) Reversed speedwalk string, or error starting with "*"
 *
 * @example
 * local back = ReverseSpeedwalk("3n2e")
 * -- Returns: "2w3s" (2 west, 3 south)
 *
 * -- Store path and return path
 * local path_to = "3neu"
 * local path_back = ReverseSpeedwalk(path_to)  -- "dsw3"
 *
 * @see EvaluateSpeedwalk, RemoveBacktracks, Speedwalk
 */
int L_ReverseSpeedwalk(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* speedwalk = luaL_checkstring(L, 1);
    QString result = speedwalk::reverse(QString::fromUtf8(speedwalk));
    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}

/**
 * world.RemoveBacktracks(speedwalk)
 *
 * Removes redundant back-and-forth movements from a speedwalk string.
 * Opposite directions that cancel each other out are removed.
 *
 * @param speedwalk (string) Speedwalk notation string
 *
 * @return (string) Optimized speedwalk string, or error starting with "*"
 *
 * @example
 * local optimized = RemoveBacktracks("nsew")
 * -- Returns: "" (north-south and east-west cancel out)
 *
 * local optimized = RemoveBacktracks("3n2sne")
 * -- Returns: "nne" (3n-2s = 1n, plus ne)
 *
 * -- Optimize recorded paths
 * local path = recorded_movements
 * path = RemoveBacktracks(path)
 * Note("Optimized path: " .. path)
 *
 * @see EvaluateSpeedwalk, ReverseSpeedwalk, Speedwalk
 */
int L_RemoveBacktracks(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* speedwalk = luaL_checkstring(L, 1);
    QString result =
        speedwalk::removeBacktracks(QString::fromUtf8(speedwalk), pDoc->m_speedwalk.filler);
    lua_pushstring(L, result.toUtf8().constData());
    return 1;
}
