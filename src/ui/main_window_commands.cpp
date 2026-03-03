// main_window_commands.cpp — MainWindow::executeDoCommand dispatch table
//
// Dispatches MUSHclient-compatible command names to MainWindow private slots
// and QAction toggles. Part of the DoCommand() Lua API.
//
// The dispatch table is a static local inside the member function so that
// the lambdas have access to MainWindow's private members.

#include "main_window.h"

#include <QMdiSubWindow>
#include <functional>
#include <string_view>
#include <unordered_map>

// Error codes (must stay in sync with lua_common.h)
static constexpr int eOK = 0;
static constexpr int eNoSuchCommand = 30054;

int MainWindow::executeDoCommand(const char* name)
{
    using Handler = std::function<int(MainWindow*)>;
    static const std::unordered_map<std::string_view, Handler> table = {

        // ----------------------------------------------------------------
        // File / World
        // ----------------------------------------------------------------
        {"NewWorld",          [](MainWindow* w) { w->newWorld();           return eOK; }},
        {"OpenWorld",         [](MainWindow* w) { w->openWorld();          return eOK; }},
        {"CloseWorld",        [](MainWindow* w) { w->closeWorld();         return eOK; }},
        {"SaveWorld",         [](MainWindow* w) { w->saveWorld();          return eOK; }},
        {"SaveWorldAs",       [](MainWindow* w) { w->saveWorldAs();        return eOK; }},
        {"WorldProperties",   [](MainWindow* w) { w->worldProperties();    return eOK; }},
        {"GlobalPreferences", [](MainWindow* w) { w->globalPreferences();  return eOK; }},
        {"LogSession",        [](MainWindow* w) { w->toggleLogSession();   return eOK; }},
        {"ReloadDefaults",    [](MainWindow* w) { w->reloadDefaults();     return eOK; }},
        {"ImportXml",         [](MainWindow* w) { w->importXml();          return eOK; }},
        {"Exit",              [](MainWindow* w) { w->exitApplication();    return eOK; }},

        // ----------------------------------------------------------------
        // Edit
        // ----------------------------------------------------------------
        {"Undo",                 [](MainWindow* w) { w->undo();                  return eOK; }},
        {"Cut",                  [](MainWindow* w) { w->cut();                   return eOK; }},
        {"Copy",                 [](MainWindow* w) { w->copy();                  return eOK; }},
        {"CopyAsHTML",           [](MainWindow* w) { w->copyAsHtml();            return eOK; }},
        {"Paste",                [](MainWindow* w) { w->paste();                 return eOK; }},
        {"PasteToWorld",         [](MainWindow* w) { w->pasteToWorld();          return eOK; }},
        {"RecallLastWord",       [](MainWindow* w) { w->recallLastWord();        return eOK; }},
        {"SelectAll",            [](MainWindow* w) { w->selectAll();             return eOK; }},
        {"SpellCheck",           [](MainWindow* w) { w->spellCheck();            return eOK; }},
        {"GenerateCharacterName",[](MainWindow* w) { w->generateCharacterName(); return eOK; }},
        {"ReloadNamesFile",      [](MainWindow* w) { w->reloadNamesFile();       return eOK; }},
        {"GenerateUniqueID",     [](MainWindow* w) { w->generateUniqueID();      return eOK; }},

        // ----------------------------------------------------------------
        // Input / Commands
        // ----------------------------------------------------------------
        {"ActivateInputArea",      [](MainWindow* w) { w->activateInputArea();      return eOK; }},
        {"PreviousCommand",        [](MainWindow* w) { w->previousCommand();        return eOK; }},
        {"NextCommand",            [](MainWindow* w) { w->nextCommand();            return eOK; }},
        {"RepeatLastCommand",      [](MainWindow* w) { w->repeatLastCommand();      return eOK; }},
        {"ClearCommandHistory",    [](MainWindow* w) { w->clearCommandHistory();    return eOK; }},
        {"CommandHistory",         [](MainWindow* w) { w->showCommandHistory();     return eOK; }},
        {"GlobalChange",           [](MainWindow* w) { w->globalChange();           return eOK; }},
        {"DiscardQueuedCommands",  [](MainWindow* w) { w->discardQueuedCommands();  return eOK; }},
        {"ShowKeyName",            [](MainWindow* w) { w->showKeyName();            return eOK; }},

        // ----------------------------------------------------------------
        // Connection
        // ----------------------------------------------------------------
        {"ConnectToMud",             [](MainWindow* w) { w->connectToMud();                return eOK; }},
        {"DisconnectFromMud",        [](MainWindow* w) { w->disconnectFromMud();           return eOK; }},
        {"AutoConnect",              [](MainWindow* w) { w->toggleAutoConnect();           return eOK; }},
        {"ReconnectOnDisconnect",    [](MainWindow* w) { w->toggleReconnectOnDisconnect(); return eOK; }},
        {"ConnectToAllOpenWorlds",   [](MainWindow* w) { w->connectToAllOpenWorlds();      return eOK; }},
        {"ConnectToStartupList",     [](MainWindow* w) { w->connectToStartupList();        return eOK; }},

        // ----------------------------------------------------------------
        // Game / Script
        // ----------------------------------------------------------------
        {"ReloadScriptFile",  [](MainWindow* w) { w->reloadScriptFile();  return eOK; }},
        {"AutoSay",           [](MainWindow* w) { w->toggleAutoSay();     return eOK; }},
        {"WrapOutput",        [](MainWindow* w) { w->toggleWrapOutput();  return eOK; }},
        {"TestTrigger",       [](MainWindow* w) { w->testTrigger();       return eOK; }},
        {"MinimizeToTray",    [](MainWindow* w) { w->minimizeToTray();    return eOK; }},
        {"Trace",             [](MainWindow* w) { w->toggleTrace();       return eOK; }},
        {"EditScriptFile",    [](MainWindow* w) { w->editScriptFile();    return eOK; }},
        {"ResetAllTimers",    [](MainWindow* w) { w->resetAllTimers();    return eOK; }},
        {"SendToAllWorlds",   [](MainWindow* w) { w->sendToAllWorlds();   return eOK; }},
        {"DoMapperSpecial",   [](MainWindow* w) { w->doMapperSpecial();   return eOK; }},
        {"AddMapperComment",  [](MainWindow* w) { w->addMapperComment();  return eOK; }},
        {"ImmediateScript",   [](MainWindow* w) { w->immediateScript();   return eOK; }},

        // ----------------------------------------------------------------
        // Display
        // ----------------------------------------------------------------
        {"Find",              [](MainWindow* w) { w->find();              return eOK; }},
        {"FindNext",          [](MainWindow* w) { w->findNext();          return eOK; }},
        {"FindForward",       [](MainWindow* w) { w->findForward();       return eOK; }},
        {"FindBackward",      [](MainWindow* w) { w->findBackward();      return eOK; }},
        {"RecallText",        [](MainWindow* w) { w->recallText();        return eOK; }},
        {"HighlightPhrase",   [](MainWindow* w) { w->highlightPhrase();   return eOK; }},
        {"ScrollToStart",     [](MainWindow* w) { w->scrollToStart();     return eOK; }},
        {"ScrollPageUp",      [](MainWindow* w) { w->scrollPageUp();      return eOK; }},
        {"ScrollPageDown",    [](MainWindow* w) { w->scrollPageDown();    return eOK; }},
        {"ScrollToEnd",       [](MainWindow* w) { w->scrollToEnd();       return eOK; }},
        {"ScrollLineUp",      [](MainWindow* w) { w->scrollLineUp();      return eOK; }},
        {"ScrollLineDown",    [](MainWindow* w) { w->scrollLineDown();    return eOK; }},
        {"ClearOutput",       [](MainWindow* w) { w->clearOutput();       return eOK; }},
        {"StopSound",         [](MainWindow* w) { w->stopSound();         return eOK; }},
        {"CommandEcho",       [](MainWindow* w) { w->toggleCommandEcho(); return eOK; }},
        {"FreezeOutput",      [](MainWindow* w) { w->toggleFreezeOutput();return eOK; }},
        {"GoToLine",          [](MainWindow* w) { w->goToLine();          return eOK; }},
        {"GoToUrl",           [](MainWindow* w) { w->goToUrl();           return eOK; }},

        // ----------------------------------------------------------------
        // Configure
        // ----------------------------------------------------------------
        {"ConfigureAll",         [](MainWindow* w) { w->configureAll();         return eOK; }},
        {"ConfigureConnection",  [](MainWindow* w) { w->configureConnection();  return eOK; }},
        {"ConfigureLogging",     [](MainWindow* w) { w->configureLogging();     return eOK; }},
        {"ConfigureInfo",        [](MainWindow* w) { w->configureInfo();        return eOK; }},
        {"ConfigureOutput",      [](MainWindow* w) { w->configureOutput();      return eOK; }},
        {"ConfigureMxp",         [](MainWindow* w) { w->configureMxp();         return eOK; }},
        {"ConfigureColours",     [](MainWindow* w) { w->configureColours();     return eOK; }},
        {"ConfigureCommands",    [](MainWindow* w) { w->configureCommands();    return eOK; }},
        {"ConfigureKeypad",      [](MainWindow* w) { w->configureKeypad();      return eOK; }},
        {"ConfigureMacros",      [](MainWindow* w) { w->configureMacros();      return eOK; }},
        {"ConfigureAutoSay",     [](MainWindow* w) { w->configureAutoSay();     return eOK; }},
        {"ConfigurePaste",       [](MainWindow* w) { w->configurePaste();       return eOK; }},
        {"ConfigureScripting",   [](MainWindow* w) { w->configureScripting();   return eOK; }},
        {"ConfigureVariables",   [](MainWindow* w) { w->configureVariables();   return eOK; }},
        {"ConfigureTimers",      [](MainWindow* w) { w->configureTimers();      return eOK; }},
        {"ConfigureTriggers",    [](MainWindow* w) { w->configureTriggers();    return eOK; }},
        {"ConfigureAliases",     [](MainWindow* w) { w->configureAliases();     return eOK; }},
        {"ConfigurePlugins",     [](MainWindow* w) { w->configurePlugins();     return eOK; }},
        {"PluginWizard",         [](MainWindow* w) { w->pluginWizard();         return eOK; }},
        {"SendFile",             [](MainWindow* w) { w->sendFile();             return eOK; }},
        {"ShowMapper",           [](MainWindow* w) { w->showMapper();           return eOK; }},
        {"SaveSelection",        [](MainWindow* w) { w->saveSelection();        return eOK; }},

        // ----------------------------------------------------------------
        // View / Window management
        // ----------------------------------------------------------------
        {"CascadeWindows",          [](MainWindow* w) { w->cascade();                  return eOK; }},
        {"TileHorizontally",        [](MainWindow* w) { w->tileHorizontally();         return eOK; }},
        {"TileVertically",          [](MainWindow* w) { w->tileVertically();           return eOK; }},
        {"ArrangeIcons",            [](MainWindow* w) { w->arrangeIcons();             return eOK; }},
        {"NewWindow",               [](MainWindow* w) { w->newWindow();                return eOK; }},
        {"CloseAllNotepadWindows",  [](MainWindow* w) { w->closeAllNotepadWindows();   return eOK; }},
        {"OpenNotepad",             [](MainWindow* w) { w->openNotepad();              return eOK; }},
        {"FlipToNotepad",           [](MainWindow* w) { w->flipToNotepad();            return eOK; }},
        {"ColourPicker",            [](MainWindow* w) { w->colourPicker();             return eOK; }},
        {"DebugPackets",            [](MainWindow* w) { w->debugPackets();             return eOK; }},
        {"ResetToolbars",           [](MainWindow* w) { w->resetToolbars();            return eOK; }},
        {"ActivityList",            [](MainWindow* w) { w->activityList();             return eOK; }},
        {"TextAttributes",          [](MainWindow* w) { w->textAttributes();           return eOK; }},
        {"MultilineTrigger",        [](MainWindow* w) { w->multilineTrigger();         return eOK; }},
        {"BookmarkSelection",       [](MainWindow* w) { w->bookmarkSelection();        return eOK; }},
        {"GoToBookmark",            [](MainWindow* w) { w->goToBookmark();             return eOK; }},
        {"SendMailTo",              [](MainWindow* w) { w->sendMailTo();               return eOK; }},

        // ----------------------------------------------------------------
        // Help
        // ----------------------------------------------------------------
        {"Help",          [](MainWindow* w) { w->showHelp();           return eOK; }},
        {"BugReports",    [](MainWindow* w) { w->openBugReports();     return eOK; }},
        {"Documentation", [](MainWindow* w) { w->openDocumentation();  return eOK; }},
        {"WebPage",       [](MainWindow* w) { w->openWebPage();        return eOK; }},
        {"About",         [](MainWindow* w) { w->about();              return eOK; }},
        {"QuickConnect",  [](MainWindow* w) { w->quickConnect();       return eOK; }},

        // ----------------------------------------------------------------
        // Text conversions
        // ----------------------------------------------------------------
        {"ConvertUppercase",         [](MainWindow* w) { w->convertUppercase();         return eOK; }},
        {"ConvertLowercase",         [](MainWindow* w) { w->convertLowercase();         return eOK; }},
        {"ConvertUnixToDos",         [](MainWindow* w) { w->convertUnixToDos();         return eOK; }},
        {"ConvertDosToUnix",         [](MainWindow* w) { w->convertDosToUnix();         return eOK; }},
        {"ConvertMacToDos",          [](MainWindow* w) { w->convertMacToDos();          return eOK; }},
        {"ConvertDosToMac",          [](MainWindow* w) { w->convertDosToMac();          return eOK; }},
        {"ConvertBase64Encode",      [](MainWindow* w) { w->convertBase64Encode();      return eOK; }},
        {"ConvertBase64Decode",      [](MainWindow* w) { w->convertBase64Decode();      return eOK; }},
        {"ConvertHtmlEncode",        [](MainWindow* w) { w->convertHtmlEncode();        return eOK; }},
        {"ConvertHtmlDecode",        [](MainWindow* w) { w->convertHtmlDecode();        return eOK; }},
        {"ConvertQuoteLines",        [](MainWindow* w) { w->convertQuoteLines();        return eOK; }},
        {"ConvertRemoveExtraBlanks", [](MainWindow* w) { w->convertRemoveExtraBlanks(); return eOK; }},
        {"ConvertWrapLines",         [](MainWindow* w) { w->convertWrapLines();         return eOK; }},

        // ----------------------------------------------------------------
        // MDI window switching: World1 through World10
        // ----------------------------------------------------------------
        {"World1",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 1) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(0));
            return eOK;
        }},
        {"World2",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 2) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(1));
            return eOK;
        }},
        {"World3",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 3) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(2));
            return eOK;
        }},
        {"World4",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 4) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(3));
            return eOK;
        }},
        {"World5",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 5) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(4));
            return eOK;
        }},
        {"World6",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 6) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(5));
            return eOK;
        }},
        {"World7",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 7) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(6));
            return eOK;
        }},
        {"World8",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 8) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(7));
            return eOK;
        }},
        {"World9",  [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 9) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(8));
            return eOK;
        }},
        {"World10", [](MainWindow* w) -> int {
            const auto list = w->m_mdiArea->subWindowList();
            if (list.size() < 10) return eNoSuchCommand;
            w->m_mdiArea->setActiveSubWindow(list.at(9));
            return eOK;
        }},

        // ----------------------------------------------------------------
        // MDI operations on the active subwindow (null-safe)
        // ----------------------------------------------------------------
        {"MinimizeWindow", [](MainWindow* w) -> int {
            QMdiSubWindow* sub = w->m_mdiArea->activeSubWindow();
            if (sub) sub->showMinimized();
            return eOK;
        }},
        {"MaximizeWindow", [](MainWindow* w) -> int {
            QMdiSubWindow* sub = w->m_mdiArea->activeSubWindow();
            if (sub) sub->showMaximized();
            return eOK;
        }},
        {"RestoreWindow", [](MainWindow* w) -> int {
            QMdiSubWindow* sub = w->m_mdiArea->activeSubWindow();
            if (sub) sub->showNormal();
            return eOK;
        }},

        // ----------------------------------------------------------------
        // Toggle view items (QAction::trigger() flips checkable state)
        // ----------------------------------------------------------------
        {"AlwaysOnTop",          [](MainWindow* w) { w->m_alwaysOnTopAction->trigger();       return eOK; }},
        {"FullScreenMode",       [](MainWindow* w) { w->m_fullScreenAction->trigger();        return eOK; }},
        {"ViewStatusBar",        [](MainWindow* w) { w->m_statusBarAction->trigger();         return eOK; }},
        {"ViewMainToolbar",      [](MainWindow* w) { w->m_mainToolBarAction->trigger();       return eOK; }},
        {"ViewGameToolbar",      [](MainWindow* w) { w->m_gameToolBarAction->trigger();       return eOK; }},
        {"ViewActivityToolbar",  [](MainWindow* w) { w->m_activityToolBarAction->trigger();   return eOK; }},
        {"ViewInfoBar",          [](MainWindow* w) { w->m_infoBarAction->trigger();           return eOK; }},
    };

    const auto it = table.find(std::string_view{name});
    if (it == table.end()) {
        return eNoSuchCommand;
    }
    return it->second(this);
}
