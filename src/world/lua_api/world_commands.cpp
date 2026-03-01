/**
 * world_commands.cpp - Command Queue and Internal Command Functions
 *
 * Functions for queuing commands to be sent to the MUD with delays,
 * and for dispatching named internal world actions.
 */

#include "../accelerator_manager.h"
#include "../docommand_callbacks.h"
#include "../macro_keypad_compat.h"
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
    QString message = luaCheckQString(L, 1);
    bool echo = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

    qint32 result = pDoc->Queue(message, echo);
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
    // Direction commands
    {"North",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("north"));
         return eOK;
     }},
    {"South",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("south"));
         return eOK;
     }},
    {"East",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("east"));
         return eOK;
     }},
    {"West",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("west"));
         return eOK;
     }},
    {"Up",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("up"));
         return eOK;
     }},
    {"Down",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("down"));
         return eOK;
     }},
    {"Northeast",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("northeast"));
         return eOK;
     }},
    {"Southeast",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("southeast"));
         return eOK;
     }},
    {"Northwest",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("northwest"));
         return eOK;
     }},
    {"Southwest",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("southwest"));
         return eOK;
     }},
    // Action commands
    {"Look",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("look"));
         return eOK;
     }},
    {"Examine",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("examine"));
         return eOK;
     }},
    {"Take",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("take"));
         return eOK;
     }},
    {"Drop",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("drop"));
         return eOK;
     }},
    {"Say",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("say"));
         return eOK;
     }},
    {"Whisper",
     [](WorldDocument* pDoc) -> int {
         pDoc->Execute(QStringLiteral("whisper"));
         return eOK;
     }},
    // Alias for ResetTimers
    {"ResetAllTimers",
     [](WorldDocument* pDoc) -> int {
         pDoc->resetAllTimers();
         return eOK;
     }},
};

/**
 * Dispatch macro/keypad prefix commands by looking up the corresponding
 * accelerator binding in AcceleratorManager and executing its action.
 *
 * Returns eOK (0) on success, -1 if no prefix matched or no binding found.
 */
static int dispatchMacroKeypad(WorldDocument* pDoc, const char* name)
{
    if (!pDoc->m_acceleratorManager)
        return -1;

    const QString qname = QString::fromUtf8(name);

    // "Macro..." prefix — e.g. "MacroF1", "MacroCtrl+F2", "MacroShift+F3"
    if (qname.startsWith(QStringLiteral("Macro"))) {
        const QString suffix = qname.mid(5); // after "Macro"
        const QString keyString = MacroKeypadCompat::macroNameToKeyString(suffix);
        if (!keyString.isEmpty()) {
            const AcceleratorEntry* entry = pDoc->m_acceleratorManager->getAccelerator(keyString);
            if (entry) {
                pDoc->Execute(entry->action);
                return eOK;
            }
        }
        return -1;
    }

    // "CtrlKeypad..." prefix — e.g. "CtrlKeypad5"
    // Must be checked before "Keypad..." since it is a longer prefix.
    if (qname.startsWith(QStringLiteral("CtrlKeypad"))) {
        const QString suffix = qname.mid(10); // after "CtrlKeypad"
        const QString keyString =
            MacroKeypadCompat::keypadNameToKeyString(QStringLiteral("Ctrl+") + suffix);
        if (!keyString.isEmpty()) {
            const AcceleratorEntry* entry = pDoc->m_acceleratorManager->getAccelerator(keyString);
            if (entry) {
                pDoc->Execute(entry->action);
                return eOK;
            }
        }
        return -1;
    }

    // "Keypad..." prefix — e.g. "Keypad8", "KeypadDecimal"
    if (qname.startsWith(QStringLiteral("Keypad"))) {
        const QString suffix = qname.mid(6); // after "Keypad"
        const QString keyString = MacroKeypadCompat::keypadNameToKeyString(suffix);
        if (!keyString.isEmpty()) {
            const AcceleratorEntry* entry = pDoc->m_acceleratorManager->getAccelerator(keyString);
            if (entry) {
                pDoc->Execute(entry->action);
                return eOK;
            }
        }
        return -1;
    }

    // "Alt<Letter>" prefix — e.g. "AltA", "AltZ" (exactly 4 chars, last is uppercase letter)
    if (qname.startsWith(QStringLiteral("Alt")) && qname.size() == 4) {
        const QChar letter = qname.at(3);
        if (letter.isUpper()) {
            const QString keyString = QStringLiteral("Alt+") + letter;
            const AcceleratorEntry* entry = pDoc->m_acceleratorManager->getAccelerator(keyString);
            if (entry) {
                pDoc->Execute(entry->action);
                return eOK;
            }
        }
        return -1;
    }

    return -1;
}

// Flat list of all command names accepted by DoCommand(), in declaration order.
static const char* const kAllCommandNames[] = {
    // Document-tier (original 9)
    "Connect",
    "Disconnect",
    "Save",
    "ReloadScriptFile",
    "ResetTimers",
    "Pause",
    "Unpause",
    "FreezeOutput",
    "UnfreezeOutput",
    // Document-tier (directions + actions + alias)
    "North",
    "South",
    "East",
    "West",
    "Up",
    "Down",
    "Northeast",
    "Southeast",
    "Northwest",
    "Southwest",
    "Look",
    "Examine",
    "Take",
    "Drop",
    "Say",
    "Whisper",
    "ResetAllTimers",
    // Macro F-keys (36)
    "MacroF1",
    "MacroF2",
    "MacroF3",
    "MacroF4",
    "MacroF5",
    "MacroF6",
    "MacroF7",
    "MacroF8",
    "MacroF9",
    "MacroF10",
    "MacroF11",
    "MacroF12",
    "MacroCtrl+F1",
    "MacroCtrl+F2",
    "MacroCtrl+F3",
    "MacroCtrl+F4",
    "MacroCtrl+F5",
    "MacroCtrl+F6",
    "MacroCtrl+F7",
    "MacroCtrl+F8",
    "MacroCtrl+F9",
    "MacroCtrl+F10",
    "MacroCtrl+F11",
    "MacroCtrl+F12",
    "MacroShift+F1",
    "MacroShift+F2",
    "MacroShift+F3",
    "MacroShift+F4",
    "MacroShift+F5",
    "MacroShift+F6",
    "MacroShift+F7",
    "MacroShift+F8",
    "MacroShift+F9",
    "MacroShift+F10",
    "MacroShift+F11",
    "MacroShift+F12",
    // Keypad (15)
    "Keypad0",
    "Keypad1",
    "Keypad2",
    "Keypad3",
    "Keypad4",
    "Keypad5",
    "Keypad6",
    "Keypad7",
    "Keypad8",
    "Keypad9",
    "KeypadDecimal",
    "KeypadDivide",
    "KeypadMultiply",
    "KeypadSubtract",
    "KeypadAdd",
    // Ctrl+Keypad (15)
    "CtrlKeypad0",
    "CtrlKeypad1",
    "CtrlKeypad2",
    "CtrlKeypad3",
    "CtrlKeypad4",
    "CtrlKeypad5",
    "CtrlKeypad6",
    "CtrlKeypad7",
    "CtrlKeypad8",
    "CtrlKeypad9",
    "CtrlKeypadDecimal",
    "CtrlKeypadDivide",
    "CtrlKeypadMultiply",
    "CtrlKeypadSubtract",
    "CtrlKeypadAdd",
    // Alt+Letter (17)
    "AltA",
    "AltB",
    "AltC",
    "AltD",
    "AltE",
    "AltF",
    "AltG",
    "AltH",
    "AltJ",
    "AltK",
    "AltM",
    "AltN",
    "AltO",
    "AltP",
    "AltQ",
    "AltT",
    "AltX",
    // UI commands (~113)
    "NewWorld",
    "OpenWorld",
    "CloseWorld",
    "SaveWorld",
    "SaveWorldAs",
    "WorldProperties",
    "GlobalPreferences",
    "LogSession",
    "ReloadDefaults",
    "ImportXml",
    "Exit",
    "Undo",
    "Cut",
    "Copy",
    "CopyAsHTML",
    "Paste",
    "PasteToWorld",
    "RecallLastWord",
    "SelectAll",
    "SpellCheck",
    "GenerateCharacterName",
    "ReloadNamesFile",
    "GenerateUniqueID",
    "ActivateInputArea",
    "PreviousCommand",
    "NextCommand",
    "RepeatLastCommand",
    "ClearCommandHistory",
    "CommandHistory",
    "GlobalChange",
    "DiscardQueuedCommands",
    "ShowKeyName",
    "ConnectToMud",
    "DisconnectFromMud",
    "AutoConnect",
    "ReconnectOnDisconnect",
    "ConnectToAllOpenWorlds",
    "ConnectToStartupList",
    "AutoSay",
    "WrapOutput",
    "TestTrigger",
    "MinimizeToTray",
    "Trace",
    "EditScriptFile",
    "SendToAllWorlds",
    "DoMapperSpecial",
    "AddMapperComment",
    "ImmediateScript",
    "Find",
    "FindNext",
    "FindForward",
    "FindBackward",
    "RecallText",
    "HighlightPhrase",
    "ScrollToStart",
    "ScrollPageUp",
    "ScrollPageDown",
    "ScrollToEnd",
    "ScrollLineUp",
    "ScrollLineDown",
    "ClearOutput",
    "StopSound",
    "CommandEcho",
    "GoToLine",
    "GoToUrl",
    "ConfigureAll",
    "ConfigureConnection",
    "ConfigureLogging",
    "ConfigureInfo",
    "ConfigureOutput",
    "ConfigureMxp",
    "ConfigureColours",
    "ConfigureCommands",
    "ConfigureKeypad",
    "ConfigureMacros",
    "ConfigureAutoSay",
    "ConfigurePaste",
    "ConfigureScripting",
    "ConfigureVariables",
    "ConfigureTimers",
    "ConfigureTriggers",
    "ConfigureAliases",
    "ConfigurePlugins",
    "PluginWizard",
    "SendFile",
    "ShowMapper",
    "SaveSelection",
    "CascadeWindows",
    "TileHorizontally",
    "TileVertically",
    "ArrangeIcons",
    "NewWindow",
    "CloseAllNotepadWindows",
    "OpenNotepad",
    "FlipToNotepad",
    "ColourPicker",
    "DebugPackets",
    "ResetToolbars",
    "ActivityList",
    "TextAttributes",
    "MultilineTrigger",
    "BookmarkSelection",
    "GoToBookmark",
    "SendMailTo",
    "Help",
    "BugReports",
    "Documentation",
    "WebPage",
    "About",
    "QuickConnect",
    "ConvertUppercase",
    "ConvertLowercase",
    "ConvertUnixToDos",
    "ConvertDosToUnix",
    "ConvertMacToDos",
    "ConvertDosToMac",
    "ConvertBase64Encode",
    "ConvertBase64Decode",
    "ConvertHtmlEncode",
    "ConvertHtmlDecode",
    "ConvertQuoteLines",
    "ConvertRemoveExtraBlanks",
    "ConvertWrapLines",
    "World1",
    "World2",
    "World3",
    "World4",
    "World5",
    "World6",
    "World7",
    "World8",
    "World9",
    "World10",
    "MinimizeWindow",
    "MaximizeWindow",
    "RestoreWindow",
    "AlwaysOnTop",
    "FullScreenMode",
    "ViewStatusBar",
    "ViewMainToolbar",
    "ViewGameToolbar",
    "ViewActivityToolbar",
    "ViewInfoBar",
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

    // 1. Document-tier table
    for (const CommandEntry& entry : kCommandTable) {
        if (std::strcmp(entry.name, command_name) == 0) {
            int result = entry.handler(pDoc);
            lua_pushnumber(L, result);
            return 1;
        }
    }

    // 2. Macro/keypad prefix matching
    int macroResult = dispatchMacroKeypad(pDoc, command_name);
    if (macroResult >= 0) {
        lua_pushnumber(L, macroResult);
        return 1;
    }

    // 3. UI callback (MainWindow dispatch)
    auto uiCallback = DoCommandCallbacks::get();
    if (uiCallback) {
        int uiResult = uiCallback(command_name);
        if (uiResult != 30054) { // eNoSuchCommand
            lua_pushnumber(L, uiResult);
            return 1;
        }
    }

    // 4. Unknown command
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
    for (const char* name : kAllCommandNames) {
        lua_pushstring(L, name);
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

/**
 * world.Help(topic)
 *
 * Show help for a topic. In the original MUSHclient this opened the help
 * viewer window to a specific topic page. Mushkin is a cross-platform
 * rewrite without a compiled help file, so this function is a stub that
 * returns eOK immediately.
 *
 * @param topic (string) Help topic to display (ignored in this implementation)
 *
 * @return (number) Error code:
 *   - eOK (0): Success (always)
 *
 * @example
 * Help("triggers")
 *
 * @see DoCommand, GetInternalCommandsList
 */
int L_Help(lua_State* L)
{
    // Stub: mushkin has no compiled help file. Accept the topic argument
    // silently and return eOK for plugin compatibility.
    (void)L;
    return luaReturnOK(L);
}
