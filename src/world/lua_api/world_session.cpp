/**
 * world_session.cpp - World Management, Trace/Debug, and Command Processing Functions
 */

#include "lua_common.h"

/**
 * world.Execute(command)
 *
 * Executes a command as if typed by the user. The command is processed
 * through the normal command pipeline including alias expansion and
 * command stacking (semicolon separation).
 *
 * Before execution, calls ON_PLUGIN_COMMAND callback for all plugins.
 * If any plugin returns false, the command is not sent.
 *
 * @param command (string) Command to execute
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *
 * @example
 * -- Execute a simple command
 * Execute("look")
 *
 * -- Execute multiple commands (if command stacking enabled)
 * Execute("north;look;inventory")
 *
 * -- Trigger an alias
 * Execute("heal self")  -- Will match alias patterns
 *
 * @see Send, SendNoEcho, DoCommand
 */
int L_Execute(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* command = luaL_checkstring(L, 1);
    QString str = QString::fromUtf8(command);

    // Call ON_PLUGIN_COMMAND callback with recursion guard
    if (!pDoc->m_bPluginProcessingCommand) {
        pDoc->m_bPluginProcessingCommand = true;
        bool shouldSend = pDoc->SendToAllPluginCallbacks(ON_PLUGIN_COMMAND, str, true);
        pDoc->m_bPluginProcessingCommand = false;

        if (!shouldSend) {
            lua_pushinteger(L, eOK);
            return 1;
        }
    }

    // Call Execute() to process command stacking, aliases, etc.
    // (not sendToMud which sends raw - would miss semicolon prefix handling)
    pDoc->Execute(str);
    lua_pushinteger(L, eOK);
    return 1;
}

/**
 * world.Activate()
 *
 * Activates (brings to front) the world's window.
 * Useful for drawing attention to important events.
 *
 * @example
 * -- Bring window to front on important event
 * function OnCombatStart()
 *     Activate()
 *     PlaySound("combat.wav")
 * end
 *
 * @see ActivateClient, FlashIcon
 */
int L_Activate(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    emit pDoc->activateWorldWindow();
    return 0;
}

/**
 * world.ActivateClient()
 *
 * Activates (brings to front) the main application window.
 * Similar to Activate() but focuses the entire application.
 *
 * @example
 * -- Alert user when they receive a tell
 * function OnTellReceived(sender, message)
 *     ActivateClient()
 *     FlashIcon()
 *     Note("Tell from " .. sender .. ": " .. message)
 * end
 *
 * @see Activate, FlashIcon
 */
int L_ActivateClient(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    emit pDoc->activateClientWindow();
    return 0;
}

/**
 * world.GetWorldID()
 *
 * Returns the unique identifier (GUID) for this world.
 * Each world has a unique ID that persists across sessions.
 *
 * @return (string) World ID as a GUID string
 *
 * @example
 * local id = GetWorldID()
 * Note("World ID: " .. id)
 *
 * -- Use for world-specific settings
 * SetVariable("world_" .. GetWorldID() .. "_setting", value)
 *
 * @see GetWorldList, GetWorldIdList
 */
int L_GetWorldID(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QByteArray id = pDoc->m_strWorldID.toUtf8();
    lua_pushlstring(L, id.constData(), id.length());
    return 1;
}

/**
 * world.GetWorldList()
 *
 * Returns a table of all open world names.
 * Note: In the Qt version, returns only the current world name.
 *
 * @return (table) Array of world names (1-indexed)
 *
 * @example
 * local worlds = GetWorldList()
 * for i, name in ipairs(worlds) do
 *     Note("World " .. i .. ": " .. name)
 * end
 *
 * @see GetWorldIdList, GetWorldID
 */
int L_GetWorldList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    // TODO(multi-world): Iterate over all open worlds, not just current.
    QByteArray name = pDoc->m_mush_name.toUtf8();
    lua_pushlstring(L, name.constData(), name.length());
    lua_rawseti(L, -2, 1);

    return 1;
}

/**
 * world.GetWorldIdList()
 *
 * Returns a table of all open world IDs.
 * Note: In the Qt version, returns only the current world ID.
 *
 * @return (table) Array of world ID strings (1-indexed)
 *
 * @example
 * local ids = GetWorldIdList()
 * for i, id in ipairs(ids) do
 *     Note("World ID " .. i .. ": " .. id)
 * end
 *
 * @see GetWorldList, GetWorldID
 */
int L_GetWorldIdList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    // TODO(multi-world): Iterate over all open worlds, not just current.
    QByteArray id = pDoc->m_strWorldID.toUtf8();
    lua_pushlstring(L, id.constData(), id.length());
    lua_rawseti(L, -2, 1);

    return 1;
}

// ========== World Management Functions ==========

/**
 * world.GetWorld(name)
 *
 * Returns the world object with the given name, or nil if not found.
 * Note: In the Qt version, only the current world is available.
 *
 * @param name (string) The world name to look up (case-insensitive)
 * @return (userdata|nil) World userdata, or nil if not found
 *
 * @example
 * local w = GetWorld("My MUD")
 * if w then
 *     Note("Found world: " .. w.name)
 * end
 *
 * @see GetWorldById, GetWorldList
 */
int L_GetWorld(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    if (pDoc->m_mush_name.compare(QString::fromUtf8(name), Qt::CaseInsensitive) == 0) {
        WorldDocument** ppDoc =
            static_cast<WorldDocument**>(lua_newuserdata(L, sizeof(WorldDocument*)));
        *ppDoc = pDoc;
        luaL_getmetatable(L, "mushclient");
        lua_setmetatable(L, -2);
        return 1;
    }

    return 0;
}

/**
 * world.GetWorldById(id)
 *
 * Returns the world object with the given ID, or nil if not found.
 * Note: In the Qt version, only the current world is available.
 *
 * @param id (string) The world ID to look up (case-insensitive)
 * @return (userdata|nil) World userdata, or nil if not found
 *
 * @example
 * local w = GetWorldById("abc123")
 * if w then
 *     Note("Found world by ID")
 * end
 *
 * @see GetWorld, GetWorldID, GetWorldIdList
 */
int L_GetWorldById(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* id = luaL_checkstring(L, 1);

    if (pDoc->m_strWorldID.compare(QString::fromUtf8(id), Qt::CaseInsensitive) == 0) {
        WorldDocument** ppDoc =
            static_cast<WorldDocument**>(lua_newuserdata(L, sizeof(WorldDocument*)));
        *ppDoc = pDoc;
        luaL_getmetatable(L, "mushclient");
        lua_setmetatable(L, -2);
        return 1;
    }

    return 0;
}

/**
 * world.Open(filename)
 *
 * Opens a world file.
 * Note: Not yet implemented in the Qt version.
 *
 * @param filename (string) Path to the world file (.mcl) to open
 * @return (boolean) true on success, false if not yet implemented
 *
 * @example
 * local ok = Open("/path/to/world.mcl")
 * if not ok then
 *     Note("Open not yet supported")
 * end
 *
 * @see GetWorld, GetWorldList
 */
int L_Open(lua_State* L)
{
    luaL_checkstring(L, 1); // filename — accepted but not yet used
    // TODO(ui): Implement via MainWindow callback to open world files programmatically.
    lua_pushboolean(L, 0);
    return 1;
}

/**
 * world.Reset()
 *
 * Resets the MXP parser state (soft reset).
 * Equivalent to the original MUSHclient Reset() function.
 *
 * @return (none)
 *
 * @example
 * Reset()  -- clear MXP parser state
 *
 * @see Connect, Disconnect
 */
int L_Reset(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->MXP_Off(false); // soft reset — resets parser state without fully disabling MXP
    return 0;
}

/**
 * world.Trace()
 *
 * Returns the current trace setting.
 * Trace mode outputs detailed information about script execution.
 *
 * @return (boolean) true if tracing is enabled, false otherwise
 *
 * @example
 * if Trace() then
 *     Note("Tracing is currently enabled")
 * end
 *
 * @see SetTrace, GetTrace, TraceOut
 */
int L_Trace(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_bTrace);
    return 1;
}

/**
 * world.TraceOut(message)
 *
 * Outputs a message to the trace output (if tracing is enabled).
 * The message is routed through the ON_PLUGIN_TRACE callback,
 * allowing plugins to intercept and handle trace output.
 *
 * @param message (string) Message to output to trace
 *
 * @example
 * -- Debug output only shown when tracing
 * TraceOut("Processing line: " .. line)
 * TraceOut("Match found at position " .. pos)
 *
 * -- Conditional debug info
 * if debugging then
 *     SetTrace(true)
 *     TraceOut("Entering combat mode")
 * end
 *
 * @see Trace, SetTrace, GetTrace
 */
int L_TraceOut(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* message = luaL_checkstring(L, 1);

    // Use Trace() which has plugin callback support
    pDoc->Trace(QString::fromUtf8(message));

    return 0;
}

/**
 * world.Debug(command)
 *
 * Executes a debug command.
 * Note: This is a stub for compatibility - not implemented in Qt version.
 *
 * @param command (string) Debug command
 *
 * @return (string) Empty string (not implemented)
 *
 * @see Trace, TraceOut
 */
int L_Debug(lua_State* L)
{
    // Debug command - placeholder
    lua_pushstring(L, "");
    return 1;
}

/**
 * world.GetTrace()
 *
 * Returns the current trace setting.
 * Alias for Trace().
 *
 * @return (boolean) true if tracing is enabled, false otherwise
 *
 * @see Trace, SetTrace, TraceOut
 */
int L_GetTrace(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_bTrace);
    return 1;
}

/**
 * world.SetTrace(enable)
 *
 * Enables or disables trace mode.
 * Outputs "TRACE: Trace on" or "TRACE: Trace off" message when state changes.
 *
 * When trace is enabled, detailed script execution information is logged.
 *
 * @param enable (boolean) true to enable tracing, false to disable
 *
 * @example
 * -- Enable tracing for debugging
 * SetTrace(true)
 * -- ... run problematic code ...
 * SetTrace(false)
 *
 * -- Toggle trace mode
 * SetTrace(not Trace())
 *
 * @see Trace, GetTrace, TraceOut
 */
int L_SetTrace(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool enable = lua_toboolean(L, 1) != 0;

    // Only act if value is actually changing (matches original OnGameTrace behavior)
    if (enable != pDoc->m_bTrace) {
        if (pDoc->m_bTrace) {
            // Turning off - print message before disabling
            pDoc->note("TRACE: Trace off");
        }
        pDoc->m_bTrace = enable;
        if (enable) {
            // Turning on - print message after enabling
            pDoc->note("TRACE: Trace on");
        }
        // TODO(ui): Emit signal for trace menu checkmark update.
    }
    return 0;
}

/**
 * world.GetEchoInput()
 *
 * Returns whether input echoing is enabled.
 * When enabled, commands you type are displayed in the output window.
 *
 * @return (boolean) true if input echoing is enabled
 *
 * @example
 * if GetEchoInput() then
 *     Note("Input echo is ON")
 * end
 *
 * @see SetEchoInput
 */
int L_GetEchoInput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_display_my_input);
    return 1;
}

/**
 * world.SetEchoInput(enable)
 *
 * Enables or disables input echoing.
 * When enabled, commands you type are displayed in the output window.
 *
 * @param enable (boolean) true to enable input echo, false to disable
 *
 * @example
 * -- Disable echo for password entry
 * SetEchoInput(false)
 * Send(password)
 * SetEchoInput(true)
 *
 * @see GetEchoInput
 */
int L_SetEchoInput(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool enable = lua_toboolean(L, 1);
    pDoc->m_display_my_input = enable;
    return 0;
}

/**
 * world.PasteCommand(text)
 *
 * Pastes text into the command input at the current cursor position.
 * Useful for inserting generated text or completions into the command line.
 *
 * @param text (string) Text to insert into command input
 *
 * @return (string) Text that was replaced (empty if no selection)
 *
 * @example
 * -- Insert target name
 * PasteCommand("goblin")
 *
 * -- Auto-complete from history
 * function CompleteFromHistory(partial)
 *     local history = GetCommandList(100)
 *     for i, cmd in ipairs(history) do
 *         if cmd:sub(1, #partial) == partial then
 *             PasteCommand(cmd:sub(#partial + 1))
 *             break
 *         end
 *     end
 * end
 *
 * @see GetCommand, SetCommand, SelectCommand
 */
int L_PasteCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);

    // Emit signal to paste text into command input
    emit pDoc->pasteToCommand(QString::fromUtf8(text));

    lua_pushstring(L, "");
    return 1;
}

/**
 * world.GetCommandList(count)
 *
 * Returns a table of recent commands from history.
 * Commands are returned oldest to newest.
 *
 * @param count (number, optional) Maximum number of commands to return.
 *   Default: 0 (all commands)
 *
 * @return (table) Array of command strings (1-indexed)
 *
 * @example
 * -- Get last 10 commands
 * local history = GetCommandList(10)
 * for i, cmd in ipairs(history) do
 *     Note(i .. ": " .. cmd)
 * end
 *
 * -- Find last attack command
 * local history = GetCommandList()
 * for i = #history, 1, -1 do
 *     if history[i]:match("^attack ") then
 *         Note("Last attack: " .. history[i])
 *         break
 *     end
 * end
 *
 * @see DeleteCommandHistory, PushCommand
 */
int L_GetCommandList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int count = luaL_optinteger(L, 1, 0);

    const QStringList& history = pDoc->m_commandHistory;
    int total = history.size();

    if (count <= 0 || count > total) {
        count = total;
    }

    lua_createtable(L, count, 0);

    // Return most recent commands first
    int tableIndex = 1;
    for (int i = total - count; i < total; i++) {
        lua_pushstring(L, history.at(i).toUtf8().constData());
        lua_rawseti(L, -2, tableIndex++);
    }

    return 1;
}

/**
 * world.SelectCommand()
 *
 * Selects all text in the command input window.
 * The selected text can then be replaced by typing or using PasteCommand.
 *
 * @example
 * -- Select all and prepare for replacement
 * SelectCommand()
 * PasteCommand("new command")
 *
 * -- Copy command to clipboard
 * SelectCommand()
 * local cmd = GetCommand()
 * SetClipboard(cmd)
 *
 * @see GetCommand, SetCommand, PasteCommand
 */
int L_SelectCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->SelectCommand();
    return 0;
}

/**
 * world.GetQueue()
 *
 * Returns a table of queued commands waiting to be sent.
 * Commands may be queued from speedwalking, pacing, or other sources.
 *
 * @return (table) Array of command strings in queue (1-indexed)
 *
 * @example
 * -- Show queue status
 * local queue = GetQueue()
 * Note("Commands in queue: " .. #queue)
 * for i, cmd in ipairs(queue) do
 *     Note("  " .. i .. ": " .. cmd)
 * end
 *
 * -- Check if queue is empty before adding more
 * if #GetQueue() == 0 then
 *     DoAfterSpecial(1, "check_status()", sendto.script)
 * end
 *
 * @see DoAfter, DoAfterSpecial, Speedwalk
 */
int L_GetQueue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QStringList queue = pDoc->GetCommandQueue();

    lua_createtable(L, queue.size(), 0);

    int tableIndex = 1;
    for (const QString& cmd : queue) {
        lua_pushstring(L, cmd.toUtf8().constData());
        lua_rawseti(L, -2, tableIndex++);
    }

    return 1;
}
