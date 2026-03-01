/**
 * world_ui_control.cpp - Window Positioning, Fonts, Appearance, Accelerators, Clipboard, and Dialog
 * Functions
 */

#include "../accelerator_manager.h"
#include "../lua_dialog_callbacks.h"
#include "../view_interfaces.h"
#include "lua_common.h"
#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>

/**
 * world.AcceleratorTo(key_string, script, send_to)
 *
 * Registers a keyboard accelerator (hotkey) that executes a script.
 * The script is routed to the specified destination type.
 *
 * @param key_string (string) Key combination (e.g., "Ctrl+A", "F1", "PageUp")
 *   Format: [Ctrl+][Alt+][Shift+]Key
 *   Valid keys: A-Z, 0-9, F1-F12, PageUp, PageDown, Home, End, Insert, Delete
 * @param script (string) Script or command to execute when key is pressed
 * @param send_to (number) Destination for the script:
 *   - sendto.world (0): Send to MUD
 *   - sendto.command (1): Put in command window
 *   - sendto.output (2): Display in output
 *   - sendto.status (3): Show in status line
 *   - sendto.notepad (4): Send to notepad
 *   - sendto.script (12): Execute as Lua script
 *   - sendto.scriptaliasafteraliases (14): Execute after alias processing
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eBadParameter (30): Invalid key string
 *
 * @example
 * -- Run a Lua function when F5 is pressed
 * AcceleratorTo("F5", "myHealFunction()", sendto.script)
 *
 * -- Send "look" to MUD when Ctrl+L is pressed
 * AcceleratorTo("Ctrl+L", "look", sendto.world)
 *
 * -- Complex key combination
 * AcceleratorTo("Ctrl+Shift+S", "saveState()", sendto.script)
 *
 * @see Accelerator, AcceleratorList
 */
int L_AcceleratorTo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString key_string = luaCheckQString(L, 1);
    QString script = luaCheckQString(L, 2);
    int send_to = luaL_checkinteger(L, 3);

    // Get current plugin ID if running from a plugin
    QString pluginId;
    if (pDoc->m_CurrentPlugin) {
        pluginId = pDoc->m_CurrentPlugin->id();
    }

    // Register the accelerator
    int result = pDoc->m_acceleratorManager->addAccelerator(key_string, script, send_to, pluginId);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.Accelerator(key_string, send_string)
 *
 * Registers a keyboard accelerator that executes a command.
 * Convenience wrapper for AcceleratorTo with sendto.execute (12).
 * The command is executed as if typed by the user.
 *
 * @param key_string (string) Key combination (e.g., "Ctrl+A", "F1", "PageUp")
 * @param send_string (string) Command to execute. Empty string removes the accelerator.
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eBadParameter (30): Invalid key string
 *
 * @example
 * -- Quick direction keys
 * Accelerator("Numpad8", "north")
 * Accelerator("Numpad2", "south")
 * Accelerator("Numpad4", "west")
 * Accelerator("Numpad6", "east")
 *
 * -- Action shortcuts
 * Accelerator("F1", "look")
 * Accelerator("F2", "inventory")
 * Accelerator("F3", "score")
 *
 * -- Remove an accelerator
 * Accelerator("F1", "")
 *
 * @see AcceleratorTo, AcceleratorList
 */
int L_Accelerator(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString key_string = luaCheckQString(L, 1);
    QString send_string = luaCheckQString(L, 2);

    // Get current plugin ID if running from a plugin
    QString pluginId;
    if (pDoc->m_CurrentPlugin) {
        pluginId = pDoc->m_CurrentPlugin->id();
    }

    // Register the accelerator with eSendToExecute (12)
    int result = pDoc->m_acceleratorManager->addAccelerator(key_string, send_string,
                                                            12, // eSendToExecute
                                                            pluginId);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.AcceleratorList()
 *
 * Returns a table of all registered keyboard accelerators.
 * Includes accelerators from all plugins and the world itself.
 *
 * @return (table) Array of strings describing each accelerator:
 *   - Format: "Key = Command" for sendto.execute accelerators
 *   - Format: "Key = Command\t[sendto]" for other types
 *
 * @example
 * -- List all accelerators
 * local accel = AcceleratorList()
 * Note("Registered accelerators: " .. #accel)
 * for i, v in ipairs(accel) do
 *     Note("  " .. v)
 * end
 *
 * -- Output might show:
 * -- Registered accelerators: 4
 * --   F1 = look
 * --   F2 = inventory
 * --   Ctrl+F5 = doHealing()	[12]
 * --   PageUp = scroll up	[1]
 *
 * @see Accelerator, AcceleratorTo
 */
int L_AcceleratorList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    QVector<AcceleratorEntry> accelerators = pDoc->m_acceleratorManager->acceleratorList();

    lua_newtable(L);
    int index = 1;

    for (const AcceleratorEntry& entry : accelerators) {
        QString str = entry.keyString + " = " + entry.action;

        // Add sendto suffix if not eSendToExecute (12)
        if (entry.sendTo != 12) {
            str += QString("\t[%1]").arg(entry.sendTo);
        }

        luaPushQString(L, str);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * world.GetClipboard()
 *
 * Returns the current contents of the system clipboard as a string.
 * Only retrieves text content; other clipboard formats are ignored.
 *
 * @return (string) Clipboard text, or empty string if no text available
 *
 * @example
 * -- Get clipboard contents
 * local text = GetClipboard()
 * if text ~= "" then
 *     Note("Clipboard: " .. text)
 * else
 *     Note("Clipboard is empty")
 * end
 *
 * -- Execute clipboard contents as command
 * local cmd = GetClipboard()
 * if cmd ~= "" then
 *     Execute(cmd)
 * end
 *
 * @see SetClipboard
 */
int L_GetClipboard(lua_State* L)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text();
    luaPushQString(L, text);
    return 1;
}

/**
 * world.SetClipboard(text)
 *
 * Sets the system clipboard contents to the specified text.
 * Replaces any existing clipboard content.
 *
 * @param text (string) Text to copy to clipboard
 *
 * @example
 * -- Copy MUD output to clipboard
 * function OnTriggerMatch(name, line, wildcards)
 *     SetClipboard(line)
 *     Note("Line copied to clipboard!")
 * end
 *
 * -- Copy formatted data
 * local data = string.format("HP: %d/%d  MP: %d/%d", hp, maxhp, mp, maxmp)
 * SetClipboard(data)
 *
 * @see GetClipboard
 */
int L_SetClipboard(lua_State* L)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(luaCheckQString(L, 1));
    return 0;
}

/**
 * world.Menu(items, default)
 *
 * Displays a popup menu at the cursor position and returns the selected item.
 * Useful for creating context menus or option selection dialogs.
 *
 * @param items (string) Pipe-separated list of menu items
 *   Special formatting:
 *   - Use "-" for a separator line
 *   - Use "!" prefix for a checkmarked item (e.g., "!Option")
 * @param default (string, optional) Item to highlight initially
 *
 * @return (string) Selected item text (trimmed), or empty string if canceled
 *
 * @example
 * -- Simple menu
 * local choice = Menu("Attack|Cast Spell|Flee|Cancel")
 * if choice == "Attack" then
 *     Send("kill mob")
 * elseif choice == "Cast Spell" then
 *     Send("cast fireball mob")
 * end
 *
 * -- Menu with separators and checkmarks
 * local opts = "!Auto-attack|-|Enable triggers|Disable triggers|-|Cancel"
 * local selected = Menu(opts)
 *
 * -- Inventory item actions
 * function OnItemClick(item)
 *     local action = Menu("Take|Wield|Drop|-|Examine")
 *     if action ~= "" then
 *         Send(action:lower() .. " " .. item)
 *     end
 * end
 *
 * @see WindowMenu
 */
int L_Menu(lua_State* L)
{
    QString itemsStr = luaCheckQString(L, 1);
    QString defaultStr = luaOptQString(L, 2);

    // Must have at least one item
    if (itemsStr.trimmed().isEmpty()) {
        lua_pushstring(L, "");
        return 1;
    }

    // Parse menu string (pipe-separated items)
    QStringList itemList = itemsStr.split('|');

    // Create popup menu
    QMenu menu;
    QMap<QAction*, QString> actionMap; // Map actions to original item text

    for (const QString& item : itemList) {
        QString trimmedItem = item.trimmed();

        // Check for separator
        if (trimmedItem == "-") {
            menu.addSeparator();
            continue;
        }

        // Skip empty items
        if (trimmedItem.isEmpty()) {
            continue;
        }

        // Check for checkmark (item starts with "!")
        bool isChecked = trimmedItem.startsWith('!');
        QString displayText = isChecked ? trimmedItem.mid(1) : trimmedItem;

        // Add menu item
        QAction* action = menu.addAction(displayText);
        if (isChecked) {
            action->setCheckable(true);
            action->setChecked(true);
        }

        // Store original text (without "!" prefix) for return value
        actionMap[action] = displayText;

        // If this is the default item, select it
        if (!defaultStr.isEmpty() && displayText == defaultStr) {
            menu.setActiveAction(action);
        }
    }

    // Show menu at cursor position
    QAction* selectedAction = menu.exec(QCursor::pos());

    // Return selected item text (or empty string if canceled)
    if (selectedAction && actionMap.contains(selectedAction)) {
        luaPushQString(L, actionMap[selectedAction]);
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

/**
 * world.SpellCheckDlg(Text)
 *
 * Shows spell check dialog (deprecated - spell check removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param Text Text to spell check
 * @return Always returns empty table (spell check removed)
 */
int L_SpellCheckDlg(lua_State* L)
{
    Q_UNUSED(L);
    lua_newtable(L);
    return 1;
}

/**
 * world.FlashIcon()
 *
 * Flashes the application icon in the taskbar to get user attention.
 * Useful for alerting the user to important events when the window is minimized.
 *
 * @example
 * -- Alert on important tells
 * function OnTellReceived(sender, message)
 *     FlashIcon()
 *     PlaySound("tell.wav")
 * end
 *
 * -- Alert when combat ends
 * function OnCombatEnd()
 *     FlashIcon()
 * end
 *
 * @see Activate, ActivateClient
 */
int L_FlashIcon(lua_State* L)
{
    Q_UNUSED(L);
    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        mainWindow = QApplication::topLevelWidgets().value(0);
    }
    if (mainWindow) {
        QApplication::alert(mainWindow);
    }
    return 0;
}

/**
 * world.Redraw()
 *
 * Forces a redraw of all views and miniwindows.
 * Useful after making changes that need immediate visual update.
 *
 * @example
 * -- Update display after batch changes
 * WindowCreate("mywin", 0, 0, 200, 100, 1, 0, 0)
 * WindowRectOp("mywin", 2, 0, 0, 200, 100, 0xFF0000)
 * WindowText("mywin", "font", "Hello", 10, 10, 0, 0, 0xFFFFFF)
 * Redraw()  -- Force immediate display update
 *
 * @see Repaint
 */
int L_Redraw(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    emit pDoc->outputSettingsChanged();
    return 0;
}

/**
 * world.Pause(flag)
 *
 * Pauses or resumes output display (freeze mode).
 * When paused, new MUD output is buffered but not displayed.
 *
 * @param flag (boolean, optional) true to pause, false to resume. Default: true
 *
 * @example
 * -- Pause during intense spam
 * Pause(true)
 * DoAfterSpecial(5, "Pause(false)", sendto.script)  -- Resume in 5 seconds
 *
 * -- Toggle pause
 * local paused = GetOption("freeze") == 1
 * Pause(not paused)
 *
 * @see Redraw
 */
int L_Pause(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true (pause) if no argument provided - matches original optboolean(L, 1, 1)
    bool pause = lua_isnone(L, 1) ? true : lua_toboolean(L, 1);

    // Set freeze state via interface method
    if (pDoc->m_pActiveOutputView) {
        pDoc->m_pActiveOutputView->setFrozen(pause);
    }
    return 0;
}

/**
 * world.SetTitle(...)
 *
 * Sets the world window/tab title.
 * All arguments are concatenated to form the title.
 *
 * @param ... (string) Title parts (concatenated)
 *
 * @example
 * -- Simple title
 * SetTitle("My MUD - ", character_name)
 *
 * -- Show status in title
 * SetTitle(world_name, " - HP: ", hp, "/", maxhp)
 *
 * @see SetMainTitle
 */
int L_SetTitle(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString title = concatArgs(L);
    pDoc->m_strWindowTitle = title;
    emit pDoc->worldNameChanged(pDoc->m_strWindowTitle);
    return 0;
}

/**
 * world.SetMainTitle(...)
 *
 * Sets the main application window title.
 * All arguments are concatenated to form the title.
 *
 * @param ... (string) Title parts (concatenated)
 *
 * @example
 * -- Custom application title
 * SetMainTitle("Mushkin - ", character_name, " @ ", server_name)
 *
 * @see SetTitle
 */
int L_SetMainTitle(lua_State* L)
{
    QString title = concatArgs(L);
    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        mainWindow = QApplication::topLevelWidgets().value(0);
    }
    if (mainWindow) {
        mainWindow->setWindowTitle(title);
    }
    return 0;
}

/**
 * world.GetMainWindowPosition(useGetWindowRect)
 *
 * Gets the main window position and size.
 *
 * Based on scripting/lua_methods.cpp L_GetMainWindowPosition
 * Note: Lua version returns table, VBScript version returned string
 *
 * @param useGetWindowRect Optional, if true use screen coords (default false)
 * @return Table {left, top, width, height}
 */
int L_GetMainWindowPosition(lua_State* L)
{
    // Optional parameter for screen vs window coords (ignored in Qt, always window)
    Q_UNUSED(luaL_optinteger(L, 1, 0));

    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        mainWindow = QApplication::topLevelWidgets().value(0);
    }

    lua_newtable(L);
    if (mainWindow) {
        QRect geom = mainWindow->geometry();
        lua_pushinteger(L, geom.left());
        lua_setfield(L, -2, "left");
        lua_pushinteger(L, geom.top());
        lua_setfield(L, -2, "top");
        lua_pushinteger(L, geom.width());
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, geom.height());
        lua_setfield(L, -2, "height");
    }
    return 1;
}

/**
 * world.GetWorldWindowPosition()
 *
 * Gets the world window position and size.
 * In the Qt version, returns the main window position since worlds are tabs.
 *
 * Based on methods_info.cpp GetWorldWindowPosition
 *
 * @return Table {left, top, width, height}
 */
int L_GetWorldWindowPosition(lua_State* L)
{
    Q_UNUSED(L);
    // In Qt version, worlds are tabs in main window, so return main window position
    return L_GetMainWindowPosition(L);
}

/**
 * world.MoveMainWindow(left, top, width, height)
 *
 * Moves and resizes the main application window.
 *
 * Based on methods_output.cpp MoveMainWindow
 */
int L_MoveMainWindow(lua_State* L)
{
    int left = luaL_checkinteger(L, 1);
    int top = luaL_checkinteger(L, 2);
    int width = luaL_checkinteger(L, 3);
    int height = luaL_checkinteger(L, 4);

    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        mainWindow = QApplication::topLevelWidgets().value(0);
    }
    if (mainWindow) {
        mainWindow->setGeometry(left, top, width, height);
    }
    return 0;
}

/**
 * world.MoveWorldWindow(left, top, width, height)
 *
 * Moves and resizes the world window.
 * In the Qt version, moves the main window since worlds are tabs.
 *
 * Based on methods_output.cpp MoveWorldWindow
 */
int L_MoveWorldWindow(lua_State* L)
{
    // In Qt version, worlds are tabs in main window, so move main window
    return L_MoveMainWindow(L);
}

/**
 * world.SetBackgroundColour(colour)
 *
 * Sets the output window background color.
 *
 * Based on methods_output.cpp SetBackgroundColour
 *
 * @param colour BGR color value
 * @return Previous background color
 */
int L_SetBackgroundColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int colour = luaL_checkinteger(L, 1);

    int oldColour = pDoc->m_iBackgroundColour;
    pDoc->m_iBackgroundColour = colour;
    emit pDoc->outputSettingsChanged();

    lua_pushinteger(L, oldColour);
    return 1;
}

/**
 * world.SetOutputFont(fontName, pointSize)
 *
 * Sets the output window font.
 *
 * Based on methods_output.cpp SetOutputFont
 *
 * @param fontName Font family name
 * @param pointSize Font size in points (converted to pixels)
 */
int L_SetOutputFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int pointSize = luaL_checkinteger(L, 2);

    pDoc->m_output.font_name = luaCheckQString(L, 1);
    pDoc->m_output.font_height = pointSize; // Store as provided
    emit pDoc->outputSettingsChanged();
    return 0;
}

/**
 * world.SetInputFont(fontName, pointSize, weight, italic)
 *
 * Sets the command input font.
 *
 * Based on methods_output.cpp SetInputFont
 *
 * @param fontName Font family name
 * @param pointSize Font size in points
 * @param weight Font weight (e.g., 400=normal, 700=bold)
 * @param italic Italic flag (optional, default 0)
 */
int L_SetInputFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int pointSize = luaL_checkinteger(L, 2);
    int weight = luaL_checkinteger(L, 3);
    int italic = luaL_optinteger(L, 4, 0);

    pDoc->m_input.font_name = luaCheckQString(L, 1);
    pDoc->m_input.font_height = pointSize;
    pDoc->m_input.font_weight = weight;
    pDoc->m_input.font_italic = italic ? 1 : 0;
    emit pDoc->inputSettingsChanged();
    return 0;
}

/**
 * world.SetWorldWindowStatus(status)
 *
 * Sets the world window state (normal, minimized, maximized).
 *
 * Based on methods_output.cpp SetWorldWindowStatus
 *
 * @param status 1=normal, 2=minimized, 3=maximized
 */
int L_SetWorldWindowStatus(lua_State* L)
{
    int status = luaL_checkinteger(L, 1);

    QWidget* mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        mainWindow = QApplication::topLevelWidgets().value(0);
    }
    if (mainWindow) {
        switch (status) {
            case 1: // Normal
                mainWindow->showNormal();
                break;
            case 2: // Minimized
                mainWindow->showMinimized();
                break;
            case 3: // Maximized
                mainWindow->showMaximized();
                break;
        }
    }
    return 0;
}

/**
 * world.GetWorldWindowPositionX(which)
 *
 * Gets the position of a specific world window.
 * In Qt we only have one world window per document, so this is the same as GetWorldWindowPosition.
 *
 * Based on methods_info.cpp GetWorldWindowPositionX
 *
 * @param which Window number (ignored, always returns first)
 * @return Table {left, top, width, height}
 */
int L_GetWorldWindowPositionX(lua_State* L)
{
    Q_UNUSED(L);
    // Same as GetWorldWindowPosition - we only have one world window
    return L_GetWorldWindowPosition(L);
}

/**
 * world.MoveWorldWindowX(left, top, width, height, which)
 *
 * Moves a specific world window.
 * In Qt we only have one world window per document.
 *
 * Based on methods_output.cpp MoveWorldWindowX
 */
int L_MoveWorldWindowX(lua_State* L)
{
    // Same as MoveWorldWindow - we only have one world window
    return L_MoveWorldWindow(L);
}

/**
 * world.SetForegroundImage(fileName, mode)
 *
 * Sets a foreground image overlay that is drawn on top of everything.
 *
 * Background/Foreground Image Support
 * Based on methods_output.cpp SetForegroundImage
 *
 * @param fileName Image file path (or empty string to clear)
 * @param mode Display mode (0-3=stretch variants, 4-12=position, 13=tile)
 * @return eOK on success, eBadParameter for invalid mode
 */
int L_SetForegroundImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 mode = luaL_optinteger(L, 2, 0);

    // Validate mode (matches original)
    if (mode < 0 || mode > 13) {
        lua_pushinteger(L, eBadParameter);
        return 1;
    }

    // Store the image path and mode
    pDoc->m_strForegroundImageName = luaOptQString(L, 1);
    pDoc->m_iForegroundMode = mode;

    // Tell OutputView to reload the image via interface method
    if (pDoc->m_pActiveOutputView) {
        pDoc->m_pActiveOutputView->reloadForegroundImage();
    }

    lua_pushinteger(L, eOK);
    return 1;
}

/**
 * world.SetFrameBackgroundColour(colour)
 *
 * Sets the frame background color.
 * Stub - uses SetBackgroundColour instead in Qt.
 *
 * @param colour BGR color value
 * @return Previous color
 */
int L_SetFrameBackgroundColour(lua_State* L)
{
    // Same as SetBackgroundColour in Qt
    return L_SetBackgroundColour(L);
}

/**
 * world.SetToolBarPosition(which, float, side, top, left)
 *
 * Sets toolbar position.
 *
 * Based on methods_output.cpp SetToolBarPosition
 *
 * @param which 1=main toolbar, 2=game toolbar, 3=activity toolbar
 * @param float true to float the toolbar, false to dock it
 * @param side For docking: 1=top, 2=bottom, 3=left, 4=right
 *             For floating: 1=use top param, 3=use left param
 * @param top Top position (for floating)
 * @param left Left position (for floating)
 * @return eOK on success, eBadParameter on invalid toolbar
 */
int L_SetToolBarPosition(lua_State* L)
{
    int which = luaL_optinteger(L, 1, 1);
    bool floating = lua_toboolean(L, 2);
    int side = luaL_optinteger(L, 3, 1);
    int top = luaL_optinteger(L, 4, 0);
    int left = luaL_optinteger(L, 5, 0);

    // Validate which parameter (1-4: main, game, activity, infobar)
    if (which < 1 || which > 4) {
        lua_pushinteger(L, 30); // eBadParameter
        return 1;
    }

    auto callback = ToolbarCallbacks::getSetToolBarPositionCallback();
    int result = callback(which, floating, side, top, left);

    lua_pushinteger(L, result);
    return 1;
}
