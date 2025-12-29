#include "main_window.h"
#include "../automation/plugin.h" // For plugin callback constants
#include "../storage/database.h"
#include "../storage/global_options.h"
#include "../text/line.h"
#include "../world/notepad_widget.h"
#include "../world/world_document.h"
#include "dialogs/alias_list_dialog.h"
#include "dialogs/ascii_art_dialog.h"
#include "dialogs/command_history_dialog.h"
#include "dialogs/command_options_dialog.h"
#include "dialogs/confirm_preamble_dialog.h"
#include "dialogs/find_dialog.h"
#include "dialogs/generate_id_dialog.h"
#include "dialogs/generate_name_dialog.h"
#include "dialogs/global_change_dialog.h"
#include "dialogs/global_preferences_dialog.h"
#include "dialogs/go_to_line_dialog.h"
#include "dialogs/highlight_phrase_dialog.h"
#include "dialogs/immediate_dialog.h"
#include "dialogs/import_xml_dialog.h"
#include "dialogs/insert_unicode_dialog.h"
#include "dialogs/key_name_dialog.h"
#include "dialogs/map_dialog.h"
#include "dialogs/multiline_trigger_dialog.h"
#include "dialogs/plugin_dialog.h"
#include "dialogs/plugin_wizard.h"
#include "dialogs/progress_dialog.h"
#include "dialogs/quick_connect_dialog.h"
#include "dialogs/recall_search_dialog.h"
#include "dialogs/send_to_all_dialog.h"
#include "dialogs/shortcut_list_dialog.h"
#include "dialogs/tab_defaults_dialog.h"
#include "dialogs/text_attributes_dialog.h"
#include "dialogs/timer_list_dialog.h"
#include "dialogs/trigger_list_dialog.h"
#include "dialogs/world_properties_dialog.h"
#include "logging.h"
#include "views/input_view.h"
#include "views/output_view.h"
#include "views/world_widget.h"
#include "widgets/activity_window.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyleHints>
#include <QSvgRenderer>
#include <functional>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_mdiArea(nullptr), m_mainToolBar(nullptr), m_gameToolBar(nullptr),
      m_activityToolBar(nullptr), m_lastFoundLine(-1), m_lastFoundChar(-1), m_trayIcon(nullptr),
      m_trayMenu(nullptr)
{
    setWindowTitle("Mushkin");

    // Create MDI area as central widget
    m_mdiArea = new QMdiArea(this);
    m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(m_mdiArea);

    // Connect MDI area signals
    connect(m_mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenus);

    // Create UI components
    createMenus();
    createToolBars();
    createInfoBar();
    createStatusBar();
    setupSystemTray();

    // Create Activity Window (dockable list of all worlds)
    m_activityWindow = new ActivityWindow(this);
    addDockWidget(Qt::RightDockWidgetArea, m_activityWindow);
    m_activityWindow->hide(); // Hidden by default, shown via menu

    // Initialize recent file actions
    for (int i = 0; i < MaxRecentFiles; ++i) {
        QAction* action = new QAction(this);
        action->setVisible(false);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
        m_recentFileActions.append(action);
    }
    updateRecentFilesMenu();

    // Read saved window geometry
    readSettings();

    // Update menus initial state
    updateMenus();

    statusBar()->showMessage("Ready", 2000);

    // Open startup worlds after event loop starts
    // This handles both command-line arguments and WorldList preference
    QTimer::singleShot(0, this, &MainWindow::openStartupWorlds);
}

MainWindow::~MainWindow()
{
    // writeSettings is called in closeEvent
}

void MainWindow::createMenus()
{
    // File Menu
    m_fileMenu = menuBar()->addMenu("&File");

    m_newAction = m_fileMenu->addAction("&New World...");
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip("Create a new world connection");
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newWorld);

    m_openAction = m_fileMenu->addAction("&Open...");
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip("Open an existing world file");
    connect(m_openAction, &QAction::triggered, this, qOverload<>(&MainWindow::openWorld));

    m_quickConnectAction = m_fileMenu->addAction("&Quick Connect...");
    m_quickConnectAction->setStatusTip("Quickly connect to a MUD server");
    connect(m_quickConnectAction, &QAction::triggered, this, &MainWindow::quickConnect);

    m_fileMenu->addSeparator();

    m_closeAction = m_fileMenu->addAction("&Close");
    m_closeAction->setShortcut(QKeySequence::Close);
    m_closeAction->setStatusTip("Close the current world");
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::closeWorld);

    m_fileMenu->addSeparator();

    m_saveAction = m_fileMenu->addAction("&Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip("Save the current world");
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveWorld);

    m_saveAsAction = m_fileMenu->addAction("Save &As...");
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip("Save the current world with a new name");
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveWorldAs);

    m_saveSelectionAction = m_fileMenu->addAction("Save Se&lection...");
    m_saveSelectionAction->setStatusTip("Save selected text to a file");
    connect(m_saveSelectionAction, &QAction::triggered, this, &MainWindow::saveSelection);

    m_fileMenu->addSeparator();

    m_worldPropertiesAction = m_fileMenu->addAction("World &Properties...");
    m_worldPropertiesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    m_worldPropertiesAction->setStatusTip("Configure world settings");
    connect(m_worldPropertiesAction, &QAction::triggered, this, &MainWindow::worldProperties);

    m_configurePluginsAction = m_fileMenu->addAction("Pl&ugins...");
    m_configurePluginsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_configurePluginsAction->setStatusTip("Manage plugins for the active world");
    connect(m_configurePluginsAction, &QAction::triggered, this, &MainWindow::configurePlugins);

    m_pluginWizardAction = m_fileMenu->addAction("Plugin &Wizard...");
    m_pluginWizardAction->setStatusTip("Create a new plugin from world items");
    connect(m_pluginWizardAction, &QAction::triggered, this, &MainWindow::pluginWizard);

    m_fileMenu->addSeparator();

    m_logSessionAction = m_fileMenu->addAction("&Log Session");
    m_logSessionAction->setCheckable(true);
    m_logSessionAction->setStatusTip("Toggle session logging to file");
    connect(m_logSessionAction, &QAction::triggered, this, &MainWindow::toggleLogSession);

    m_fileMenu->addSeparator();

    m_importXmlAction = m_fileMenu->addAction("&Import XML...");
    m_importXmlAction->setStatusTip("Import triggers, aliases, and other settings from XML");
    connect(m_importXmlAction, &QAction::triggered, this, &MainWindow::importXml);

    m_fileMenu->addSeparator();

    // Recent Files submenu
    m_recentFilesMenu = m_fileMenu->addMenu("Recent &Files");
    for (QAction* action : m_recentFileActions) {
        m_recentFilesMenu->addAction(action);
    }

    m_fileMenu->addSeparator();

    m_exitAction = m_fileMenu->addAction("E&xit");
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exitApplication);

    // Edit Menu
    m_editMenu = menuBar()->addMenu("&Edit");

    m_copyAction = m_editMenu->addAction("&Copy");
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip("Copy selected text");
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copy);

    m_copyAsHtmlAction = m_editMenu->addAction("Copy as &HTML");
    m_copyAsHtmlAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    m_copyAsHtmlAction->setStatusTip("Copy selected text with colors and formatting as HTML");
    connect(m_copyAsHtmlAction, &QAction::triggered, this, &MainWindow::copyAsHtml);

    m_pasteAction = m_editMenu->addAction("&Paste");
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip("Paste text");
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::paste);

    m_pasteToMudAction = m_editMenu->addAction("Paste to &MUD");
    m_pasteToMudAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    m_pasteToMudAction->setStatusTip("Paste clipboard text directly to the MUD");
    connect(m_pasteToMudAction, &QAction::triggered, this, &MainWindow::pasteToMud);

    m_selectAllAction = m_editMenu->addAction("Select &All");
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setStatusTip("Select all text");
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::selectAll);

    m_editMenu->addSeparator();

    m_findAction = m_editMenu->addAction("&Find...");
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setStatusTip("Find text in the output");
    connect(m_findAction, &QAction::triggered, this, &MainWindow::find);

    m_findNextAction = m_editMenu->addAction("Find &Next");
    m_findNextAction->setShortcut(QKeySequence::FindNext); // F3
    m_findNextAction->setStatusTip("Find next occurrence");
    connect(m_findNextAction, &QAction::triggered, this, &MainWindow::findNext);

    m_editMenu->addSeparator();

    m_insertDateTimeAction = m_editMenu->addAction("Insert &Date/Time");
    m_insertDateTimeAction->setStatusTip("Insert current date and time");
    connect(m_insertDateTimeAction, &QAction::triggered, this, &MainWindow::insertDateTime);

    m_wordCountAction = m_editMenu->addAction("&Word Count...");
    m_wordCountAction->setStatusTip("Count words in selected text");
    connect(m_wordCountAction, &QAction::triggered, this, &MainWindow::wordCount);

    m_editMenu->addSeparator();

    m_preferencesAction = m_editMenu->addAction("&Preferences...");
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    m_preferencesAction->setStatusTip("Configure application preferences");
    connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::preferences);

    m_editMenu->addSeparator();

    m_generateNameAction = m_editMenu->addAction("Generate Character &Name...");
    m_generateNameAction->setStatusTip("Generate a random fantasy character name");
    connect(m_generateNameAction, &QAction::triggered, this, &MainWindow::generateCharacterName);

    m_generateIdAction = m_editMenu->addAction("Generate Unique &ID...");
    m_generateIdAction->setStatusTip("Generate a unique identifier for plugins");
    connect(m_generateIdAction, &QAction::triggered, this, &MainWindow::generateUniqueID);

    m_editMenu->addSeparator();

    m_insertUnicodeAction = m_editMenu->addAction("Insert &Unicode...");
    m_insertUnicodeAction->setStatusTip("Insert a Unicode character");
    connect(m_insertUnicodeAction, &QAction::triggered, this, &MainWindow::insertUnicode);

    m_sendToAllAction = m_editMenu->addAction("Send to &All Worlds...");
    m_sendToAllAction->setStatusTip("Send text to all open worlds");
    connect(m_sendToAllAction, &QAction::triggered, this, &MainWindow::sendToAll);

    m_asciiArtAction = m_editMenu->addAction("ASC&II Art...");
    m_asciiArtAction->setStatusTip("Create ASCII art text");
    connect(m_asciiArtAction, &QAction::triggered, this, &MainWindow::asciiArt);

    m_highlightPhraseAction = m_editMenu->addAction("&Highlight Phrase...");
    m_highlightPhraseAction->setStatusTip("Highlight text in output");
    connect(m_highlightPhraseAction, &QAction::triggered, this, &MainWindow::highlightPhrase);

    m_editMenu->addSeparator();

    m_goToMatchingBraceAction = m_editMenu->addAction("Go to &Matching Brace");
    m_goToMatchingBraceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight));
    m_goToMatchingBraceAction->setStatusTip("Jump to matching bracket, brace, or parenthesis");
    connect(m_goToMatchingBraceAction, &QAction::triggered, this, &MainWindow::goToMatchingBrace);

    m_selectToMatchingBraceAction = m_editMenu->addAction("Select to Matchin&g Brace");
    m_selectToMatchingBraceAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketRight));
    m_selectToMatchingBraceAction->setStatusTip(
        "Select text to matching bracket, brace, or parenthesis");
    connect(m_selectToMatchingBraceAction, &QAction::triggered, this,
            &MainWindow::selectToMatchingBrace);

    // Input Menu (matches original MUSHclient structure)
    m_inputMenu = menuBar()->addMenu("&Input");

    m_activateInputAreaAction = m_inputMenu->addAction("&Activate Input Area");
    m_activateInputAreaAction->setShortcut(QKeySequence(Qt::Key_Tab));
    m_activateInputAreaAction->setStatusTip("Set focus to the input field (Tab or Escape)");
    m_activateInputAreaAction->setMenuRole(QAction::NoRole);
    connect(m_activateInputAreaAction, &QAction::triggered, this, &MainWindow::activateInputArea);

    m_inputMenu->addSeparator();

    m_previousCommandAction = m_inputMenu->addAction("&Previous Command");
    m_previousCommandAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    m_previousCommandAction->setStatusTip("Recall previous command from history");
    m_previousCommandAction->setMenuRole(QAction::NoRole);
    connect(m_previousCommandAction, &QAction::triggered, this, &MainWindow::previousCommand);

    m_nextCommandAction = m_inputMenu->addAction("&Next Command");
    m_nextCommandAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    m_nextCommandAction->setStatusTip("Recall next command from history");
    m_nextCommandAction->setMenuRole(QAction::NoRole);
    connect(m_nextCommandAction, &QAction::triggered, this, &MainWindow::nextCommand);

    m_repeatLastCommandAction = m_inputMenu->addAction("&Repeat Last Command");
    m_repeatLastCommandAction->setStatusTip("Execute the most recent command again");
    m_repeatLastCommandAction->setMenuRole(QAction::NoRole);
    connect(m_repeatLastCommandAction, &QAction::triggered, this, &MainWindow::repeatLastCommand);

    m_inputMenu->addSeparator();

    m_clearCommandHistoryAction = m_inputMenu->addAction("C&lear Command History");
    m_clearCommandHistoryAction->setStatusTip("Clear all command history");
    m_clearCommandHistoryAction->setMenuRole(QAction::NoRole);
    connect(m_clearCommandHistoryAction, &QAction::triggered, this,
            &MainWindow::clearCommandHistory);

    m_commandHistoryAction = m_inputMenu->addAction("Command &History...");
    m_commandHistoryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    m_commandHistoryAction->setStatusTip("View and manage command history");
    m_commandHistoryAction->setMenuRole(QAction::NoRole);
    connect(m_commandHistoryAction, &QAction::triggered, this, &MainWindow::showCommandHistory);

    m_inputMenu->addSeparator();

    m_globalChangeAction = m_inputMenu->addAction("&Global Change...");
    m_globalChangeAction->setStatusTip("Search and replace text globally");
    m_globalChangeAction->setMenuRole(QAction::NoRole);
    connect(m_globalChangeAction, &QAction::triggered, this, &MainWindow::globalChange);

    m_discardQueueAction = m_inputMenu->addAction("&Discard Queued Commands");
    m_discardQueueAction->setStatusTip("Clear all pending queued commands");
    m_discardQueueAction->setMenuRole(QAction::NoRole);
    connect(m_discardQueueAction, &QAction::triggered, this, &MainWindow::discardQueuedCommands);

    m_keyNameAction = m_inputMenu->addAction("&Key Name...");
    m_keyNameAction->setStatusTip("Display the name of a key press");
    m_keyNameAction->setMenuRole(QAction::NoRole);
    connect(m_keyNameAction, &QAction::triggered, this, &MainWindow::showKeyName);

    // Connection Menu (matches original MUSHclient structure)
    QMenu* connectionMenu = menuBar()->addMenu("Connecti&on");

    m_connectAction = connectionMenu->addAction("&Connect");
    m_connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    m_connectAction->setStatusTip("Connect to the MUD server");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::connectToMud);

    m_disconnectAction = connectionMenu->addAction("&Disconnect");
    m_disconnectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    m_disconnectAction->setStatusTip("Disconnect from the MUD server");
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::disconnectFromMud);

    connectionMenu->addSeparator();

    m_autoConnectAction = connectionMenu->addAction("&Auto Connect");
    m_autoConnectAction->setCheckable(true);
    m_autoConnectAction->setStatusTip("Automatically connect when opening worlds");
    m_autoConnectAction->setMenuRole(QAction::NoRole);
    connect(m_autoConnectAction, &QAction::triggered, this, &MainWindow::toggleAutoConnect);

    m_reconnectOnDisconnectAction = connectionMenu->addAction("&Reconnect on Disconnect");
    m_reconnectOnDisconnectAction->setCheckable(true);
    m_reconnectOnDisconnectAction->setStatusTip("Automatically reconnect when disconnected");
    m_reconnectOnDisconnectAction->setMenuRole(QAction::NoRole);
    connect(m_reconnectOnDisconnectAction, &QAction::triggered, this,
            &MainWindow::toggleReconnectOnDisconnect);

    connectionMenu->addSeparator();

    m_connectToAllAction = connectionMenu->addAction("Connect to All &Open Worlds");
    m_connectToAllAction->setStatusTip("Connect to all open but disconnected worlds");
    connect(m_connectToAllAction, &QAction::triggered, this, &MainWindow::connectToAllOpenWorlds);

    m_connectToStartupListAction = connectionMenu->addAction("Connect to Worlds in &Startup List");
    m_connectToStartupListAction->setStatusTip(
        "Open and connect to all worlds in the startup list");
    connect(m_connectToStartupListAction, &QAction::triggered, this,
            &MainWindow::connectToStartupList);

    // Set initial state from database
    Database* db = Database::instance();
    bool autoConnect = db->getPreferenceInt("AutoConnectWorlds", 0) != 0;
    m_autoConnectAction->setChecked(autoConnect);
    bool reconnectOnDisconnect = db->getPreferenceInt("ReconnectOnDisconnect", 0) != 0;
    m_reconnectOnDisconnectAction->setChecked(reconnectOnDisconnect);

    // Game Menu
    m_gameMenu = menuBar()->addMenu("&Game");

    m_reloadScriptFileAction = m_gameMenu->addAction("&Reload Script File");
    m_reloadScriptFileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_reloadScriptFileAction->setStatusTip("Reload the script file for the active world");
    m_reloadScriptFileAction->setMenuRole(QAction::NoRole);
    connect(m_reloadScriptFileAction, &QAction::triggered, this, &MainWindow::reloadScriptFile);

    m_autoSayAction = m_gameMenu->addAction("Auto-&Say");
    m_autoSayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    m_autoSayAction->setCheckable(true);
    m_autoSayAction->setChecked(false); // Default to disabled
    m_autoSayAction->setStatusTip("Automatically prepend 'say ' to all commands");
    m_autoSayAction->setMenuRole(QAction::NoRole);
    connect(m_autoSayAction, &QAction::triggered, this, &MainWindow::toggleAutoSay);

    m_gameMenu->addSeparator();

    // Movement commands (matches original MUSHclient Game menu)
    QAction* northAction = m_gameMenu->addAction("&North");
    northAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_N));
    northAction->setStatusTip("Go North");
    connect(northAction, &QAction::triggered, this, [this]() { sendGameCommand("north"); });

    QAction* southAction = m_gameMenu->addAction("&South");
    southAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_S));
    southAction->setStatusTip("Go South");
    connect(southAction, &QAction::triggered, this, [this]() { sendGameCommand("south"); });

    QAction* eastAction = m_gameMenu->addAction("&East");
    eastAction->setStatusTip("Go East");
    connect(eastAction, &QAction::triggered, this, [this]() { sendGameCommand("east"); });

    QAction* westAction = m_gameMenu->addAction("&West");
    westAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_W));
    westAction->setStatusTip("Go West");
    connect(westAction, &QAction::triggered, this, [this]() { sendGameCommand("west"); });

    QAction* upAction = m_gameMenu->addAction("&Up");
    upAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_U));
    upAction->setStatusTip("Go Up");
    connect(upAction, &QAction::triggered, this, [this]() { sendGameCommand("up"); });

    QAction* downAction = m_gameMenu->addAction("&Down");
    downAction->setStatusTip("Go Down");
    connect(downAction, &QAction::triggered, this, [this]() { sendGameCommand("down"); });

    m_gameMenu->addSeparator();

    QAction* lookAction = m_gameMenu->addAction("&Look");
    lookAction->setStatusTip("Look around");
    connect(lookAction, &QAction::triggered, this, [this]() { sendGameCommand("look"); });

    QAction* examineAction = m_gameMenu->addAction("E&xamine");
    examineAction->setStatusTip("Examine");
    connect(examineAction, &QAction::triggered, this, [this]() { sendGameCommand("examine"); });

    m_gameMenu->addSeparator();

    // Social commands (REPLACE_COMMAND - put text in input for user to complete)
    QAction* sayAction = m_gameMenu->addAction("Sa&y");
    sayAction->setStatusTip("Say something (puts 'say ' in command input)");
    connect(sayAction, &QAction::triggered, this, [this]() {
        QMdiSubWindow* sub = m_mdiArea->activeSubWindow();
        if (WorldWidget* ww = sub ? qobject_cast<WorldWidget*>(sub->widget()) : nullptr) {
            ww->inputView()->setText("say ");
            ww->inputView()->setFocus();
        }
    });

    QAction* whisperAction = m_gameMenu->addAction("W&hisper");
    whisperAction->setStatusTip("Whisper to someone (puts 'whisper ' in command input)");
    connect(whisperAction, &QAction::triggered, this, [this]() {
        QMdiSubWindow* sub = m_mdiArea->activeSubWindow();
        if (WorldWidget* ww = sub ? qobject_cast<WorldWidget*>(sub->widget()) : nullptr) {
            ww->inputView()->setText("whisper ");
            ww->inputView()->setFocus();
        }
    });

    QAction* pageAction = m_gameMenu->addAction("Pa&ge");
    pageAction->setStatusTip("Page someone (puts 'page ' in command input)");
    connect(pageAction, &QAction::triggered, this, [this]() {
        QMdiSubWindow* sub = m_mdiArea->activeSubWindow();
        if (WorldWidget* ww = sub ? qobject_cast<WorldWidget*>(sub->widget()) : nullptr) {
            ww->inputView()->setText("page ");
            ww->inputView()->setFocus();
        }
    });

    m_gameMenu->addSeparator();

    // Status commands (SEND_NOW - send immediately)
    QAction* whoAction = m_gameMenu->addAction("&Who");
    whoAction->setStatusTip("Show who is connected");
    connect(whoAction, &QAction::triggered, this, [this]() { sendGameCommand("WHO"); });

    QAction* doingAction = m_gameMenu->addAction("Doin&g");
    doingAction->setStatusTip("Show what people are doing");
    connect(doingAction, &QAction::triggered, this, [this]() { sendGameCommand("DOING"); });

    m_gameMenu->addSeparator();

    // Configuration submenu (matches original MUSHclient structure)
    QMenu* configureMenu = new QMenu("C&onfigure", this);
    configureMenu->menuAction()->setMenuRole(QAction::NoRole); // Prevent macOS from hiding submenu
    configureMenu->menuAction()->setVisible(true);             // Explicitly set visible for macOS

    // Configuration items (always enabled - no longer disabled when no world is open)
    m_configureTriggersAction = configureMenu->addAction("Triggers...");
    m_configureTriggersAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_8));
    m_configureTriggersAction->setStatusTip("Configure triggers for the active world");
    m_configureTriggersAction->setMenuRole(QAction::NoRole);
    connect(m_configureTriggersAction, &QAction::triggered, this, &MainWindow::configureTriggers);

    m_configureAliasesAction = configureMenu->addAction("Aliases...");
    m_configureAliasesAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_9));
    m_configureAliasesAction->setStatusTip("Configure aliases for the active world");
    m_configureAliasesAction->setMenuRole(QAction::NoRole);
    connect(m_configureAliasesAction, &QAction::triggered, this, &MainWindow::configureAliases);

    m_configureTimersAction = configureMenu->addAction("Timers...");
    m_configureTimersAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_0));
    m_configureTimersAction->setStatusTip("Configure timers for the active world");
    m_configureTimersAction->setMenuRole(QAction::NoRole);
    connect(m_configureTimersAction, &QAction::triggered, this, &MainWindow::configureTimers);

    m_configureShortcutsAction = configureMenu->addAction("Shortcuts...");
    m_configureShortcutsAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_K));
    m_configureShortcutsAction->setStatusTip("Configure keyboard shortcuts for the active world");
    m_configureShortcutsAction->setMenuRole(QAction::NoRole);
    connect(m_configureShortcutsAction, &QAction::triggered, this, &MainWindow::configureShortcuts);

    m_gameMenu->addMenu(configureMenu);

    m_gameMenu->addSeparator();

    m_immediateScriptAction = m_gameMenu->addAction("&Immediate Script...");
    m_immediateScriptAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_immediateScriptAction->setStatusTip("Execute Lua script immediately");
    m_immediateScriptAction->setMenuRole(QAction::NoRole);
    connect(m_immediateScriptAction, &QAction::triggered, this, &MainWindow::immediateScript);

    m_commandOptionsAction = m_gameMenu->addAction("Command &Options...");
    m_commandOptionsAction->setStatusTip("Configure command processing options");
    m_commandOptionsAction->setMenuRole(QAction::NoRole);
    connect(m_commandOptionsAction, &QAction::triggered, this, &MainWindow::commandOptions);

    m_tabDefaultsAction = m_gameMenu->addAction("Tab Com&pletion...");
    m_tabDefaultsAction->setStatusTip("Configure tab completion defaults");
    m_tabDefaultsAction->setMenuRole(QAction::NoRole);
    connect(m_tabDefaultsAction, &QAction::triggered, this, &MainWindow::tabDefaults);

    m_gameMenu->addSeparator();

    m_sendFileAction = m_gameMenu->addAction("Send &File...");
    m_sendFileAction->setStatusTip("Send a text file to the MUD");
    m_sendFileAction->setMenuRole(QAction::NoRole);
    connect(m_sendFileAction, &QAction::triggered, this, &MainWindow::sendFile);

    m_gameMenu->addSeparator();

    m_mapperAction = m_gameMenu->addAction("&Mapper...");
    m_mapperAction->setStatusTip("Open the mapper window");
    m_mapperAction->setMenuRole(QAction::NoRole);
    connect(m_mapperAction, &QAction::triggered, this, &MainWindow::showMapper);

    // Display Menu (matches original MUSHclient structure)
    m_displayMenu = menuBar()->addMenu("&Display");

    m_startAction = m_displayMenu->addAction("&Start");
    m_startAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Home));
    m_startAction->setStatusTip("Scroll to start of output");
    connect(m_startAction, &QAction::triggered, this, &MainWindow::scrollToStart);

    m_pageUpAction = m_displayMenu->addAction("Page &Up");
    m_pageUpAction->setShortcut(QKeySequence(Qt::Key_PageUp));
    m_pageUpAction->setStatusTip("Scroll up one page");
    connect(m_pageUpAction, &QAction::triggered, this, &MainWindow::scrollPageUp);

    m_pageDownAction = m_displayMenu->addAction("Page &Down");
    m_pageDownAction->setShortcut(QKeySequence(Qt::Key_PageDown));
    m_pageDownAction->setStatusTip("Scroll down one page");
    connect(m_pageDownAction, &QAction::triggered, this, &MainWindow::scrollPageDown);

    m_endAction = m_displayMenu->addAction("&End");
    m_endAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_End));
    m_endAction->setStatusTip("Scroll to end of output");
    connect(m_endAction, &QAction::triggered, this, &MainWindow::scrollToEnd);

    m_displayMenu->addSeparator();

    m_lineUpAction = m_displayMenu->addAction("Line U&p");
    m_lineUpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Up));
    m_lineUpAction->setStatusTip("Scroll up one line");
    connect(m_lineUpAction, &QAction::triggered, this, &MainWindow::scrollLineUp);

    m_lineDownAction = m_displayMenu->addAction("Line Do&wn");
    m_lineDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Down));
    m_lineDownAction->setStatusTip("Scroll down one line");
    connect(m_lineDownAction, &QAction::triggered, this, &MainWindow::scrollLineDown);

    m_displayMenu->addSeparator();

    m_clearOutputAction = m_displayMenu->addAction("&Clear Output");
    m_clearOutputAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    m_clearOutputAction->setStatusTip("Clear all output text");
    connect(m_clearOutputAction, &QAction::triggered, this, &MainWindow::clearOutput);

    m_commandEchoAction = m_displayMenu->addAction("Command &Echo");
    m_commandEchoAction->setCheckable(true);
    m_commandEchoAction->setChecked(true); // Default to enabled
    m_commandEchoAction->setStatusTip("Toggle command echo in output");
    connect(m_commandEchoAction, &QAction::triggered, this, &MainWindow::toggleCommandEcho);

    m_freezeOutputAction = m_displayMenu->addAction("&Freeze Output");
    m_freezeOutputAction->setCheckable(true);
    m_freezeOutputAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    m_freezeOutputAction->setStatusTip("Pause output scrolling");
    connect(m_freezeOutputAction, &QAction::triggered, this, &MainWindow::toggleFreezeOutput);

    m_displayMenu->addSeparator();

    m_goToLineAction = m_displayMenu->addAction("&Go to Line...");
    m_goToLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    m_goToLineAction->setStatusTip("Navigate to a specific line in output");
    connect(m_goToLineAction, &QAction::triggered, this, &MainWindow::goToLine);

    m_goToUrlAction = m_displayMenu->addAction("Go to &URL");
    m_goToUrlAction->setStatusTip("Open selected text as URL in browser");
    connect(m_goToUrlAction, &QAction::triggered, this, &MainWindow::goToUrl);

    m_sendMailToAction = m_displayMenu->addAction("Send &Mail To...");
    m_sendMailToAction->setStatusTip("Send email to selected address");
    connect(m_sendMailToAction, &QAction::triggered, this, &MainWindow::sendMailTo);

    m_displayMenu->addSeparator();

    m_bookmarkSelectionAction = m_displayMenu->addAction("&Bookmark Selection");
    m_bookmarkSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    m_bookmarkSelectionAction->setStatusTip("Toggle bookmark on current line");
    connect(m_bookmarkSelectionAction, &QAction::triggered, this, &MainWindow::bookmarkSelection);

    m_goToBookmarkAction = m_displayMenu->addAction("Go to Boo&kmark");
    m_goToBookmarkAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_goToBookmarkAction->setStatusTip("Jump to next bookmarked line");
    connect(m_goToBookmarkAction, &QAction::triggered, this, &MainWindow::goToBookmark);

    m_displayMenu->addSeparator();

    m_activityListAction = m_displayMenu->addAction("&Activity List...");
    m_activityListAction->setStatusTip("Show list of worlds with activity");
    connect(m_activityListAction, &QAction::triggered, this, &MainWindow::activityList);

    m_textAttributesAction = m_displayMenu->addAction("Text &Attributes...");
    m_textAttributesAction->setStatusTip("Configure text formatting attributes");
    connect(m_textAttributesAction, &QAction::triggered, this, &MainWindow::textAttributes);

    m_multilineTriggerAction = m_displayMenu->addAction("&Multi-line Trigger...");
    m_multilineTriggerAction->setStatusTip("Configure multi-line trigger patterns");
    connect(m_multilineTriggerAction, &QAction::triggered, this, &MainWindow::multilineTrigger);

    // Convert Menu (text transformations for notepad windows)
    m_convertMenu = menuBar()->addMenu("Con&vert");

    m_convertUppercaseAction = m_convertMenu->addAction("&Uppercase");
    m_convertUppercaseAction->setStatusTip("Convert selected text to UPPERCASE");
    connect(m_convertUppercaseAction, &QAction::triggered, this, &MainWindow::convertUppercase);

    m_convertLowercaseAction = m_convertMenu->addAction("&Lowercase");
    m_convertLowercaseAction->setStatusTip("Convert selected text to lowercase");
    connect(m_convertLowercaseAction, &QAction::triggered, this, &MainWindow::convertLowercase);

    m_convertMenu->addSeparator();

    m_convertUnixToDosAction = m_convertMenu->addAction("Unix to &DOS");
    m_convertUnixToDosAction->setStatusTip("Convert Unix line endings (LF) to DOS (CR+LF)");
    connect(m_convertUnixToDosAction, &QAction::triggered, this, &MainWindow::convertUnixToDos);

    m_convertDosToUnixAction = m_convertMenu->addAction("DOS to Uni&x");
    m_convertDosToUnixAction->setStatusTip("Convert DOS line endings (CR+LF) to Unix (LF)");
    connect(m_convertDosToUnixAction, &QAction::triggered, this, &MainWindow::convertDosToUnix);

    m_convertMacToDosAction = m_convertMenu->addAction("&Mac to DOS");
    m_convertMacToDosAction->setStatusTip("Convert Mac line endings (CR) to DOS (CR+LF)");
    connect(m_convertMacToDosAction, &QAction::triggered, this, &MainWindow::convertMacToDos);

    m_convertDosToMacAction = m_convertMenu->addAction("DOS to Ma&c");
    m_convertDosToMacAction->setStatusTip("Convert DOS line endings (CR+LF) to Mac (CR)");
    connect(m_convertDosToMacAction, &QAction::triggered, this, &MainWindow::convertDosToMac);

    m_convertMenu->addSeparator();

    m_convertBase64EncodeAction = m_convertMenu->addAction("Base64 &Encode");
    m_convertBase64EncodeAction->setStatusTip("Encode selected text as Base64");
    connect(m_convertBase64EncodeAction, &QAction::triggered, this,
            &MainWindow::convertBase64Encode);

    m_convertBase64DecodeAction = m_convertMenu->addAction("Base64 Deco&de");
    m_convertBase64DecodeAction->setStatusTip("Decode Base64 text");
    connect(m_convertBase64DecodeAction, &QAction::triggered, this,
            &MainWindow::convertBase64Decode);

    m_convertMenu->addSeparator();

    m_convertHtmlEncodeAction = m_convertMenu->addAction("HTML &Special Encode");
    m_convertHtmlEncodeAction->setStatusTip("Convert special characters to HTML entities");
    connect(m_convertHtmlEncodeAction, &QAction::triggered, this, &MainWindow::convertHtmlEncode);

    m_convertHtmlDecodeAction = m_convertMenu->addAction("HTML Special Decode");
    m_convertHtmlDecodeAction->setStatusTip("Convert HTML entities to characters");
    connect(m_convertHtmlDecodeAction, &QAction::triggered, this, &MainWindow::convertHtmlDecode);

    m_convertMenu->addSeparator();

    m_convertQuoteLinesAction = m_convertMenu->addAction("&Quote Lines...");
    m_convertQuoteLinesAction->setStatusTip("Add a prefix to each line");
    connect(m_convertQuoteLinesAction, &QAction::triggered, this, &MainWindow::convertQuoteLines);

    m_convertRemoveExtraBlanksAction = m_convertMenu->addAction("&Remove Extra Blanks");
    m_convertRemoveExtraBlanksAction->setStatusTip("Remove extra whitespace from text");
    connect(m_convertRemoveExtraBlanksAction, &QAction::triggered, this,
            &MainWindow::convertRemoveExtraBlanks);

    m_convertWrapLinesAction = m_convertMenu->addAction("&Wrap Lines");
    m_convertWrapLinesAction->setStatusTip("Remove line breaks to create continuous text");
    connect(m_convertWrapLinesAction, &QAction::triggered, this, &MainWindow::convertWrapLines);

    // View Menu
    m_viewMenu = menuBar()->addMenu("&View");

    // Toolbar visibility toggles (will be connected after toolbars are created)
    m_mainToolBarAction = m_viewMenu->addAction("&Main Toolbar");
    m_mainToolBarAction->setCheckable(true);
    m_mainToolBarAction->setChecked(true);
    m_mainToolBarAction->setStatusTip("Show or hide the main toolbar");

    m_gameToolBarAction = m_viewMenu->addAction("&Game Toolbar");
    m_gameToolBarAction->setCheckable(true);
    m_gameToolBarAction->setChecked(true);
    m_gameToolBarAction->setStatusTip("Show or hide the game toolbar");

    m_activityToolBarAction = m_viewMenu->addAction("&Activity Toolbar");
    m_activityToolBarAction->setCheckable(true);
    m_activityToolBarAction->setChecked(true);
    m_activityToolBarAction->setStatusTip("Show or hide the activity toolbar");

    m_infoBarAction = m_viewMenu->addAction("&Info Bar");
    m_infoBarAction->setCheckable(true);
    m_infoBarAction->setChecked(false); // Hidden by default
    m_infoBarAction->setStatusTip("Show or hide the info bar");

    m_viewMenu->addSeparator();

    m_resetToolbarsAction = m_viewMenu->addAction("&Reset Toolbars");
    m_resetToolbarsAction->setStatusTip("Reset all toolbars to their default positions");
    connect(m_resetToolbarsAction, &QAction::triggered, this, &MainWindow::resetToolbars);

    m_viewMenu->addSeparator();

    m_tabbedViewAction = m_viewMenu->addAction("&Tabbed Windows");
    m_tabbedViewAction->setCheckable(true);
    m_tabbedViewAction->setChecked(false);
    m_tabbedViewAction->setStatusTip("Toggle between tabbed and windowed view");
    connect(m_tabbedViewAction, &QAction::triggered, this, &MainWindow::toggleTabbedView);

    m_alwaysOnTopAction = m_viewMenu->addAction("&Always On Top");
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setChecked(false);
    m_alwaysOnTopAction->setStatusTip("Keep window above all other windows");
    connect(m_alwaysOnTopAction, &QAction::triggered, this, &MainWindow::toggleAlwaysOnTop);

    m_fullScreenAction = m_viewMenu->addAction("&Full Screen");
    m_fullScreenAction->setCheckable(true);
    m_fullScreenAction->setChecked(false);
    m_fullScreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullScreenAction->setStatusTip("Toggle full screen mode");
    connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    m_viewMenu->addSeparator();

    m_recallAction = m_viewMenu->addAction("&Recall...");
    m_recallAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_recallAction->setStatusTip("Search and recall buffer contents");
    connect(m_recallAction, &QAction::triggered, this, &MainWindow::recall);

    // Window Menu
    m_windowMenu = menuBar()->addMenu("&Window");

    m_cascadeAction = m_windowMenu->addAction("&Cascade");
    m_cascadeAction->setStatusTip("Cascade all windows");
    connect(m_cascadeAction, &QAction::triggered, m_mdiArea, &QMdiArea::cascadeSubWindows);

    m_tileHorizontallyAction = m_windowMenu->addAction("Tile &Horizontally");
    m_tileHorizontallyAction->setStatusTip("Tile all windows horizontally");
    connect(m_tileHorizontallyAction, &QAction::triggered, this, &MainWindow::tileHorizontally);

    m_tileVerticallyAction = m_windowMenu->addAction("Tile &Vertically");
    m_tileVerticallyAction->setStatusTip("Tile all windows vertically");
    connect(m_tileVerticallyAction, &QAction::triggered, this, &MainWindow::tileVertically);

    m_windowMenu->addSeparator();

    m_closeAllAction = m_windowMenu->addAction("Close &All");
    m_closeAllAction->setStatusTip("Close all windows");
    connect(m_closeAllAction, &QAction::triggered, this, &MainWindow::closeAllWindows);

    m_windowMenu->addSeparator();

    // We'll manually populate the window menu in Qt 6
    // (Qt 5's QMdiArea::setWindowMenu is not available in Qt 6)
    connect(m_windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    // Help Menu
    m_helpMenu = menuBar()->addMenu("&Help");

    m_helpAction = m_helpMenu->addAction("&Contents");
    m_helpAction->setShortcut(QKeySequence::HelpContents);
    m_helpAction->setStatusTip("Show help contents");
    connect(m_helpAction, &QAction::triggered, this, &MainWindow::showHelp);

    m_helpMenu->addSeparator();

    m_aboutAction = m_helpMenu->addAction("&About");
    m_aboutAction->setStatusTip("About Mushkin");
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::createToolBars()
{
    // === Main Toolbar - File/Edit operations ===
    m_mainToolBar = addToolBar("Main Toolbar");
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setMovable(true);

    // Add actions to toolbar (icons set by updateToolbarIcons based on theme)
    m_mainToolBar->addAction(m_newAction);
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_saveAction);

    m_mainToolBar->addSeparator();

    // Connection actions
    m_mainToolBar->addAction(m_connectAction);
    m_mainToolBar->addAction(m_disconnectAction);

    m_mainToolBar->addSeparator();

    // Edit actions
    m_mainToolBar->addAction(m_copyAction);
    m_mainToolBar->addAction(m_pasteAction);

    m_mainToolBar->addSeparator();

    // Find action
    m_mainToolBar->addAction(m_findAction);

    // Connect visibility toggle
    connect(m_mainToolBarAction, &QAction::toggled, m_mainToolBar, &QToolBar::setVisible);
    connect(m_mainToolBar, &QToolBar::visibilityChanged, m_mainToolBarAction, &QAction::setChecked);

    // Create game and activity toolbars
    createGameToolBar();
    createActivityToolBar();

    // Apply initial toolbar appearance preferences
    applyToolbarPreferences();

    // Apply theme and update icons
    applyTheme();
}

void MainWindow::createGameToolBar()
{
    // === Game Toolbar - Direction buttons and common commands ===
    m_gameToolBar = addToolBar("Game Toolbar");
    m_gameToolBar->setObjectName("GameToolBar");
    m_gameToolBar->setMovable(true);

    // Use a slightly larger font for direction buttons
    QFont dirFont = m_gameToolBar->font();
    dirFont.setBold(true);
    dirFont.setPointSize(dirFont.pointSize() + 1);

    // Direction buttons using text (like original MUSHclient)
    m_gameNorthAction = m_gameToolBar->addAction("N");
    m_gameNorthAction->setToolTip("Go North");
    m_gameNorthAction->setFont(dirFont);
    connect(m_gameNorthAction, &QAction::triggered, this, [this]() { sendGameCommand("north"); });

    m_gameSouthAction = m_gameToolBar->addAction("S");
    m_gameSouthAction->setToolTip("Go South");
    m_gameSouthAction->setFont(dirFont);
    connect(m_gameSouthAction, &QAction::triggered, this, [this]() { sendGameCommand("south"); });

    m_gameEastAction = m_gameToolBar->addAction("E");
    m_gameEastAction->setToolTip("Go East");
    m_gameEastAction->setFont(dirFont);
    connect(m_gameEastAction, &QAction::triggered, this, [this]() { sendGameCommand("east"); });

    m_gameWestAction = m_gameToolBar->addAction("W");
    m_gameWestAction->setToolTip("Go West");
    m_gameWestAction->setFont(dirFont);
    connect(m_gameWestAction, &QAction::triggered, this, [this]() { sendGameCommand("west"); });

    m_gameToolBar->addSeparator();

    m_gameUpAction = m_gameToolBar->addAction("U");
    m_gameUpAction->setToolTip("Go Up");
    m_gameUpAction->setFont(dirFont);
    connect(m_gameUpAction, &QAction::triggered, this, [this]() { sendGameCommand("up"); });

    m_gameDownAction = m_gameToolBar->addAction("D");
    m_gameDownAction->setToolTip("Go Down");
    m_gameDownAction->setFont(dirFont);
    connect(m_gameDownAction, &QAction::triggered, this, [this]() { sendGameCommand("down"); });

    m_gameToolBar->addSeparator();

    // Common commands
    m_gameLookAction = m_gameToolBar->addAction("Look");
    m_gameLookAction->setToolTip("Look around");
    connect(m_gameLookAction, &QAction::triggered, this, [this]() { sendGameCommand("look"); });

    m_gameExamineAction = m_gameToolBar->addAction("Exam");
    m_gameExamineAction->setToolTip("Examine");
    connect(m_gameExamineAction, &QAction::triggered, this,
            [this]() { sendGameCommand("examine"); });

    m_gameWhoAction = m_gameToolBar->addAction("Who");
    m_gameWhoAction->setToolTip("Who is online");
    connect(m_gameWhoAction, &QAction::triggered, this, [this]() { sendGameCommand("who"); });

    // Connect visibility toggle
    connect(m_gameToolBarAction, &QAction::toggled, m_gameToolBar, &QToolBar::setVisible);
    connect(m_gameToolBar, &QToolBar::visibilityChanged, m_gameToolBarAction, &QAction::setChecked);
}

void MainWindow::createActivityToolBar()
{
    // === Activity Toolbar - World switching ===
    m_activityToolBar = addToolBar("Activity Toolbar");
    m_activityToolBar->setObjectName("ActivityToolBar");
    m_activityToolBar->setMovable(true);

    // This toolbar will be dynamically populated with connected worlds
    // For now, add a placeholder label
    QLabel* activityLabel = new QLabel(" Worlds: ", this);
    m_activityToolBar->addWidget(activityLabel);

    // TODO: Add dynamic world buttons when worlds connect/disconnect

    // Connect visibility toggle
    connect(m_activityToolBarAction, &QAction::toggled, m_activityToolBar, &QToolBar::setVisible);
    connect(m_activityToolBar, &QToolBar::visibilityChanged, m_activityToolBarAction,
            &QAction::setChecked);
}

void MainWindow::createInfoBar()
{
    // Create dock widget for info bar
    m_infoBarDock = new QDockWidget("Info", this);
    m_infoBarDock->setObjectName("InfoBarDock");
    m_infoBarDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);

    // Create text edit widget
    m_infoBarText = new QTextEdit(this);
    m_infoBarText->setReadOnly(true);
    m_infoBarText->setMaximumHeight(60); // Keep it compact like original

    m_infoBarDock->setWidget(m_infoBarText);
    addDockWidget(Qt::BottomDockWidgetArea, m_infoBarDock);

    // Hidden by default (like original MUSHclient)
    m_infoBarDock->hide();

    // Connect visibility toggle
    connect(m_infoBarAction, &QAction::toggled, m_infoBarDock, &QDockWidget::setVisible);
    connect(m_infoBarDock, &QDockWidget::visibilityChanged, m_infoBarAction, &QAction::setChecked);
}

void MainWindow::showInfoBar(bool visible)
{
    if (m_infoBarDock) {
        m_infoBarDock->setVisible(visible);
    }
}

void MainWindow::infoBarAppend(const QString& text)
{
    if (m_infoBarText) {
        QTextCursor cursor = m_infoBarText->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
        m_infoBarText->setTextCursor(cursor);
    }
}

void MainWindow::infoBarClear()
{
    if (m_infoBarText) {
        m_infoBarText->clear();
    }
}

void MainWindow::infoBarSetColor(const QColor& color)
{
    if (m_infoBarText) {
        m_infoBarText->setTextColor(color);
    }
}

void MainWindow::infoBarSetFont(const QString& fontName, int size, int style)
{
    if (!m_infoBarText)
        return;

    QTextCharFormat format;

    if (!fontName.isEmpty()) {
        format.setFontFamilies({fontName});
    }

    if (size > 0) {
        format.setFontPointSize(size);
    }

    // Style bits: 1=bold, 2=italic, 4=underline, 8=strikeout
    format.setFontWeight((style & 1) ? QFont::Bold : QFont::Normal);
    format.setFontItalic((style & 2) != 0);
    format.setFontUnderline((style & 4) != 0);
    format.setFontStrikeOut((style & 8) != 0);

    m_infoBarText->mergeCurrentCharFormat(format);
}

void MainWindow::infoBarSetBackground(const QColor& color)
{
    if (m_infoBarText) {
        QPalette palette = m_infoBarText->palette();
        palette.setColor(QPalette::Base, color);
        m_infoBarText->setPalette(palette);
    }
}

void MainWindow::applyToolbarPreferences()
{
    Database* db = Database::instance();

    // Apply flat toolbar style
    bool flatToolbars = db->getPreferenceInt("FlatToolbars", 1) != 0;
    QString toolbarStyle;
    if (flatToolbars) {
        // Flat style - no button borders
        toolbarStyle = "QToolBar { border: none; } "
                       "QToolButton { border: none; padding: 3px; } "
                       "QToolButton:hover { background: palette(highlight); }";
    } else {
        // Normal style - clear any custom style
        toolbarStyle.clear();
    }
    m_mainToolBar->setStyleSheet(toolbarStyle);
    m_gameToolBar->setStyleSheet(toolbarStyle);
    m_activityToolBar->setStyleSheet(toolbarStyle);

    // Apply activity button bar style
    // Style values: 0-5 correspond to different Qt::ToolButtonStyle options
    int buttonStyle = db->getPreferenceInt("ActivityButtonBarStyle", 0);
    Qt::ToolButtonStyle tbStyle;
    switch (buttonStyle) {
        case 1:
            tbStyle = Qt::ToolButtonTextOnly;
            break;
        case 2:
            tbStyle = Qt::ToolButtonTextBesideIcon;
            break;
        case 3:
            tbStyle = Qt::ToolButtonTextUnderIcon;
            break;
        case 4:
            tbStyle = Qt::ToolButtonFollowStyle;
            break;
        case 5:
            tbStyle = Qt::ToolButtonIconOnly;
            break;
        default:
            tbStyle = Qt::ToolButtonIconOnly; // Style 0 = icons only
            break;
    }
    m_activityToolBar->setToolButtonStyle(tbStyle);
}

void MainWindow::applyTheme()
{
    Database* db = Database::instance();
    int mode = db->getPreferenceInt("ThemeMode", ThemeSystem);

    qDebug() << "applyTheme: mode =" << mode << "(0=Light, 1=Dark, 2=System)";

    // Use Qt 6's color scheme API to properly change appearance on macOS
    Qt::ColorScheme scheme;
    if (mode == ThemeLight) {
        scheme = Qt::ColorScheme::Light;
    } else if (mode == ThemeDark) {
        scheme = Qt::ColorScheme::Dark;
    } else {
        scheme = Qt::ColorScheme::Unknown; // System/auto
    }

    // Set the color scheme - this properly changes appearance on macOS
    QGuiApplication::styleHints()->setColorScheme(scheme);

    qDebug() << "applyTheme: set color scheme to" << static_cast<int>(scheme);

    // Update toolbar icons to match the new theme
    updateToolbarIcons();
}

QIcon MainWindow::loadThemedIcon(const QString& name)
{
    // Read SVG from resources
    QFile file(QString(":/icons/%1").arg(name));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load icon:" << name;
        return QIcon();
    }
    QString svg = QString::fromUtf8(file.readAll());
    file.close();

    // Determine icon color based on effective color scheme
    Database* db = Database::instance();
    int mode = db->getPreferenceInt("ThemeMode", ThemeSystem);

    bool useDark = false;
    if (mode == ThemeDark) {
        useDark = true;
    } else if (mode == ThemeSystem) {
        // Check system preference
        auto colorScheme = QGuiApplication::styleHints()->colorScheme();
        useDark = (colorScheme == Qt::ColorScheme::Dark);
    }
    // ThemeLight: useDark stays false

    // Use light icons on dark backgrounds, dark icons on light backgrounds
    QColor color = useDark ? QColor(255, 255, 255) : QColor(51, 51, 51);

    // Replace stroke color
    svg.replace("currentColor", color.name());

    // Create icon from modified SVG
    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    return QIcon(pixmap);
}

void MainWindow::updateToolbarIcons()
{
    // Update main toolbar action icons
    m_newAction->setIcon(loadThemedIcon("file-plus"));
    m_openAction->setIcon(loadThemedIcon("folder-open"));
    m_saveAction->setIcon(loadThemedIcon("device-floppy"));
    m_connectAction->setIcon(loadThemedIcon("player-play"));
    m_disconnectAction->setIcon(loadThemedIcon("player-stop"));
    m_copyAction->setIcon(loadThemedIcon("copy"));
    m_pasteAction->setIcon(loadThemedIcon("clipboard"));
    m_findAction->setIcon(loadThemedIcon("search"));
}

void MainWindow::sendGameCommand(const QString& command)
{
    // Get the active world widget and send the command
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc || doc->m_iConnectPhase != eConnectConnectedToMud) {
        statusBar()->showMessage("Not connected", 2000);
        return;
    }

    // Send the command
    doc->sendToMud(command);
}

void MainWindow::createStatusBar()
{
    // Create permanent status indicators (right side of status bar)

    // Lines indicator - shows line count
    m_linesIndicator = new QLabel(this);
    m_linesIndicator->setMinimumWidth(80);
    m_linesIndicator->setAlignment(Qt::AlignCenter);
    m_linesIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(m_linesIndicator);

    // Connection indicator - shows connected/closed status
    m_connectionIndicator = new QLabel(this);
    m_connectionIndicator->setMinimumWidth(80);
    m_connectionIndicator->setAlignment(Qt::AlignCenter);
    m_connectionIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(m_connectionIndicator);

    // Freeze indicator - shows PAUSE/MORE when output is frozen
    m_freezeIndicator = new QLabel(this);
    m_freezeIndicator->setMinimumWidth(60);
    m_freezeIndicator->setAlignment(Qt::AlignCenter);
    m_freezeIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(m_freezeIndicator);

    // Initial status message
    statusBar()->showMessage("Ready");
}

void MainWindow::readSettings()
{
    QSettings settings("Gammon", "MUSHclient");

    // Restore window geometry
    QByteArray geometry = settings.value("mainWindow/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        // Default size if no saved geometry
        resize(1024, 768);
    }

    // Restore window state (toolbars, dock widgets, etc.)
    QByteArray state = settings.value("mainWindow/state").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    // Restore tabbed view preference
    bool tabbedView = settings.value("mainWindow/tabbedView", false).toBool();
    m_tabbedViewAction->setChecked(tabbedView);
    toggleTabbedView(tabbedView);
}

void MainWindow::writeSettings()
{
    QSettings settings("Gammon", "MUSHclient");

    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/state", saveState());
    settings.setValue("mainWindow/tabbedView", m_tabbedViewAction->isChecked());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Check if any worlds have unsaved changes
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    QStringList unsavedWorlds;

    for (QMdiSubWindow* window : windows) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(window->widget());
        if (worldWidget && worldWidget->isModified()) {
            unsavedWorlds.append(worldWidget->worldName());
        }
    }

    if (!unsavedWorlds.isEmpty()) {
        QString message;
        if (unsavedWorlds.size() == 1) {
            message = QString("The world '%1' has unsaved changes.\n\n"
                              "Do you want to save before closing?")
                          .arg(unsavedWorlds.first());
        } else {
            message = QString("The following worlds have unsaved changes:\n\n%1\n\n"
                              "Do you want to save before closing?")
                          .arg(unsavedWorlds.join("\n"));
        }

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes", message,
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

        if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }

        if (reply == QMessageBox::Save) {
            // Save all modified worlds
            for (QMdiSubWindow* window : windows) {
                WorldWidget* worldWidget = qobject_cast<WorldWidget*>(window->widget());
                if (worldWidget && worldWidget->isModified()) {
                    QString filename = worldWidget->filename();
                    if (!filename.isEmpty()) {
                        worldWidget->saveToFile(filename);
                    }
                    // New worlds without a filename will be discarded
                    // (user would need to use Save As explicitly)
                }
            }
        }
    }

    writeSettings();

    // Close all MDI subwindows
    m_mdiArea->closeAllSubWindows();

    // Hide tray icon before closing
    if (m_trayIcon) {
        m_trayIcon->hide();
    }

    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // Check if minimizing and tray is enabled
        if (isMinimized() && m_trayIcon && m_trayIcon->isVisible()) {
            Database* db = Database::instance();
            int iconPlacement = db->getPreferenceInt("IconPlacement", 0);

            // If system tray only (1), hide from taskbar when minimized
            if (iconPlacement == 1) {
                QTimer::singleShot(0, this, [this]() {
                    hide();
                    m_trayIcon->showMessage("Mushkin", "Application minimized to system tray",
                                            QSystemTrayIcon::Information, 2000);
                });
            }
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupSystemTray()
{
    // Check if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    Database* db = Database::instance();
    int iconPlacement = db->getPreferenceInt("IconPlacement", 0);

    // 0 = taskbar only (no tray icon)
    // 1 = system tray only
    // 2 = both taskbar and tray
    if (iconPlacement == 0) {
        return;
    }

    // Create tray icon
    m_trayIcon = new QSystemTrayIcon(this);

    // Set icon - check for custom icon first
    int trayIconType = db->getPreferenceInt("TrayIcon", 0);
    QIcon trayIcon;

    if (trayIconType == 10) {
        QString customIconPath = db->getPreference("TrayIconFileName", "");
        if (!customIconPath.isEmpty() && QFile::exists(customIconPath)) {
            trayIcon = QIcon(customIconPath);
        }
    }

    // Fallback to window icon, then to a standard icon if none set
    if (trayIcon.isNull()) {
        trayIcon = windowIcon();
    }
    if (trayIcon.isNull()) {
        // Use Qt standard application icon as fallback
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    m_trayIcon->setIcon(trayIcon);

    m_trayIcon->setToolTip("Mushkin");

    // Create right-click context menu (matches original: About, Exit)
    m_trayMenu = new QMenu(this);

    QAction* aboutAction = m_trayMenu->addAction("About...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);

    m_trayMenu->addSeparator();

    QAction* exitAction = m_trayMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApplication);

    m_trayIcon->setContextMenu(m_trayMenu);

    // Connect activation signal
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);

    // Show the tray icon
    m_trayIcon->show();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger: // Left-click - show world list popup
        {
            // Build popup menu with list of open worlds (matches original)
            QMenu worldMenu;
            QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();

            if (windows.isEmpty()) {
                QAction* noWorlds = worldMenu.addAction("(no worlds open)");
                noWorlds->setEnabled(false);
            } else {
                for (QMdiSubWindow* window : windows) {
                    WorldWidget* ww = qobject_cast<WorldWidget*>(window->widget());
                    if (ww && ww->document()) {
                        QString name = ww->document()->worldName();
                        if (name.isEmpty())
                            name = "Untitled";
                        QAction* action = worldMenu.addAction(name);
                        connect(action, &QAction::triggered, this, [this, window]() {
                            show();
                            showNormal();
                            activateWindow();
                            raise();
                            m_mdiArea->setActiveSubWindow(window);
                        });
                    }
                }
            }

            worldMenu.addSeparator();
            QAction* showAction = worldMenu.addAction("Show Mushkin");
            connect(showAction, &QAction::triggered, this, [this]() {
                show();
                showNormal();
                activateWindow();
                raise();
            });

            worldMenu.exec(QCursor::pos());
            break;
        }
        case QSystemTrayIcon::DoubleClick: // Double-click - show/restore window
            show();
            showNormal();
            activateWindow();
            raise();
            break;
        default:
            break;
    }
}

void MainWindow::updateMenus()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    bool hasActiveWorld = (activeSubWindow != nullptr);

    // Handle focus change callbacks
    WorldDocument* currentDoc = nullptr;
    if (hasActiveWorld) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
        if (worldWidget) {
            currentDoc = worldWidget->document();
        }
    }

    // If focus changed to a different world, send callbacks
    if (currentDoc != m_lastFocusedWorld) {
        // Send LOSEFOCUS to previous world (if it still exists)
        if (m_lastFocusedWorld && !m_lastFocusedWorld->m_bWorldClosing) {
            m_lastFocusedWorld->SendToAllPluginCallbacks(ON_PLUGIN_LOSE_FOCUS);
        }

        // Send GETFOCUS to new world
        if (currentDoc && !currentDoc->m_bWorldClosing) {
            currentDoc->SendToAllPluginCallbacks(ON_PLUGIN_GET_FOCUS);
        }

        m_lastFocusedWorld = currentDoc;
    }

    // Enable/disable actions based on whether there's an active world
    m_closeAction->setEnabled(hasActiveWorld);
    m_saveAction->setEnabled(hasActiveWorld);
    m_saveAsAction->setEnabled(hasActiveWorld);
    m_saveSelectionAction->setEnabled(hasActiveWorld);
    m_worldPropertiesAction->setEnabled(hasActiveWorld);
    m_configurePluginsAction->setEnabled(hasActiveWorld);
    m_pluginWizardAction->setEnabled(hasActiveWorld);
    m_logSessionAction->setEnabled(hasActiveWorld);
    m_copyAction->setEnabled(hasActiveWorld);
    m_copyAsHtmlAction->setEnabled(hasActiveWorld);
    m_pasteAction->setEnabled(hasActiveWorld);
    m_selectAllAction->setEnabled(hasActiveWorld);
    m_findAction->setEnabled(hasActiveWorld);
    m_findNextAction->setEnabled(hasActiveWorld && !m_lastSearchText.isEmpty());
    m_recallAction->setEnabled(hasActiveWorld);
    m_insertDateTimeAction->setEnabled(hasActiveWorld);
    m_wordCountAction->setEnabled(hasActiveWorld);

    // Display menu actions
    m_clearOutputAction->setEnabled(hasActiveWorld);

    // Game menu actions - check connection state and log state
    bool isConnected = false;
    bool isLogOpen = false;
    bool isAutoSayEnabled = false;
    if (hasActiveWorld) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
        if (worldWidget) {
            isConnected = worldWidget->isConnected();
            isLogOpen = worldWidget->document()->IsLogOpen();
            isAutoSayEnabled = worldWidget->document()->m_bEnableAutoSay != 0;
        }
    }
    m_connectAction->setEnabled(hasActiveWorld && !isConnected);
    m_disconnectAction->setEnabled(hasActiveWorld && isConnected);
    m_reloadScriptFileAction->setEnabled(hasActiveWorld);
    m_autoSayAction->setEnabled(hasActiveWorld);

    // Connection menu - startup list action enabled only if startup list exists
    Database* db = Database::instance();
    QString startupList = db->getPreference("WorldList", "");
    m_connectToStartupListAction->setEnabled(!startupList.isEmpty());

    // Update Log Session checked state
    m_logSessionAction->setChecked(isLogOpen);

    // Update Auto-Say checked state
    m_autoSayAction->setChecked(isAutoSayEnabled);

    // Configure Triggers/Aliases/Timers are always enabled (matches original MUSHclient)
    // This also fixes the macOS native menu bar bug where disabled items stay disabled

    // Input menu actions - require an active world
    m_activateInputAreaAction->setEnabled(hasActiveWorld);
    m_previousCommandAction->setEnabled(hasActiveWorld);
    m_nextCommandAction->setEnabled(hasActiveWorld);
    m_repeatLastCommandAction->setEnabled(hasActiveWorld);
    m_clearCommandHistoryAction->setEnabled(hasActiveWorld);
    m_commandHistoryAction->setEnabled(hasActiveWorld);

    // Window menu actions
    bool hasWorlds = !m_mdiArea->subWindowList().isEmpty();
    m_cascadeAction->setEnabled(hasWorlds);
    m_tileHorizontallyAction->setEnabled(hasWorlds);
    m_tileVerticallyAction->setEnabled(hasWorlds);
    m_closeAllAction->setEnabled(hasWorlds);

    // Update status bar indicators
    updateStatusIndicators();
}

void MainWindow::updateWindowMenu()
{
    // This is called when the Window menu is about to show
    // Manually populate with list of open worlds (Qt 6 doesn't have setWindowMenu)

    // Remove any dynamically added actions (world list)
    // Keep the fixed menu items (Cascade, Tile, etc.)
    QList<QAction*> actions = m_windowMenu->actions();
    for (QAction* action : actions) {
        if (action->data().toString() == "world_window") {
            m_windowMenu->removeAction(action);
            delete action;
        }
    }

    // Add current subwindows to menu
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    if (!windows.isEmpty()) {
        for (int i = 0; i < windows.size(); ++i) {
            QMdiSubWindow* window = windows[i];
            QString text = QString("&%1 %2").arg(i + 1).arg(window->windowTitle());

            QAction* action = m_windowMenu->addAction(text);
            action->setData("world_window");
            action->setCheckable(true);
            action->setChecked(window == m_mdiArea->activeSubWindow());

            connect(action, &QAction::triggered,
                    [this, window]() { m_mdiArea->setActiveSubWindow(window); });
        }
    }
}

void MainWindow::updateRecentFilesMenu()
{
    Database* db = Database::instance();
    QStringList recentFiles = db->getRecentFiles();

    int numRecentFiles = qMin(recentFiles.size(), MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString fileName = recentFiles[i];
        QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(fileName).fileName());

        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(fileName);
        m_recentFileActions[i]->setVisible(true);
    }

    for (int i = numRecentFiles; i < MaxRecentFiles; ++i) {
        m_recentFileActions[i]->setVisible(false);
    }

    m_recentFilesMenu->setEnabled(numRecentFiles > 0);
}

void MainWindow::addRecentFile(const QString& filename)
{
    Database* db = Database::instance();
    db->addRecentFile(filename);
    updateRecentFilesMenu();
}

void MainWindow::queueWorldFiles(const QStringList& files)
{
    m_queuedWorldFiles = files;
}

void MainWindow::openStartupWorlds()
{
    // Collect all world files to open:
    // 1. From WorldList preference (matches original MUSHclient behavior)
    // 2. From command-line arguments (m_queuedWorldFiles)

    QStringList worldsToOpen;

    // Read WorldList preference (asterisk-separated paths)
    // Matches original MUSHclient.cpp
    Database* db = Database::instance();
    QString worldList = db->getPreference("WorldList", "");

    if (!worldList.isEmpty()) {
        // Split by asterisk delimiter
        QStringList worldListFiles = worldList.split('*', Qt::SkipEmptyParts);
        for (const QString& path : worldListFiles) {
            QString trimmedPath = path.trimmed();
            if (trimmedPath.isEmpty()) {
                continue;
            }

            // Convert Windows backslashes to forward slashes for cross-platform compatibility
            trimmedPath.replace('\\', '/');

            // Resolve relative paths relative to the application directory
            // (e.g., "./worlds/Aardwolf.mcl" or "worlds/Aardwolf.mcl")
            if (!QDir::isAbsolutePath(trimmedPath)) {
                trimmedPath = QCoreApplication::applicationDirPath() + "/" + trimmedPath;
            }

            // Clean up the path (resolve . and ..)
            trimmedPath = QDir::cleanPath(trimmedPath);

            if (QFile::exists(trimmedPath)) {
                worldsToOpen.append(trimmedPath);
            } else {
                qCWarning(lcUI) << "Startup world not found:" << trimmedPath;
            }
        }
    }

    // Add command-line queued files
    for (const QString& path : m_queuedWorldFiles) {
        if (!path.isEmpty() && QFile::exists(path)) {
            worldsToOpen.append(path);
        }
    }

    // Open each world file
    for (const QString& worldPath : worldsToOpen) {
        qCDebug(lcUI) << "Opening startup world:" << worldPath;
        openWorld(worldPath);
    }

    // Clear queued files after processing
    m_queuedWorldFiles.clear();
}

// File menu slots
void MainWindow::newWorld()
{
    // Create world widget with default settings
    WorldWidget* worldWidget = new WorldWidget();

    // Wrap in MDI subwindow
    QMdiSubWindow* subWindow = m_mdiArea->addSubWindow(worldWidget);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(worldWidget->worldName());

    // Remove the system menu (that extra menu bar inside the MDI area)
    subWindow->setSystemMenu(nullptr);

    // Connect window title changes
    connect(worldWidget, &WorldWidget::windowTitleChanged, subWindow,
            &QMdiSubWindow::setWindowTitle);

    // Connect connection state changes to update menus
    connect(worldWidget, &WorldWidget::connectedChanged, this, &MainWindow::updateMenus);

    // Connect notepad creation requests to create MDI subwindows
    connect(worldWidget, &WorldWidget::notepadRequested, this, &MainWindow::createNotepadWindow);

    // Connect to subwindow's destroyed signal to save geometry
    connect(subWindow, &QObject::destroyed, [worldWidget, subWindow]() {
        if (!worldWidget || worldWidget->worldName().isEmpty()) {
            return;
        }

        // Save current window geometry to database
        Database* db = Database::instance();
        QRect geometry = subWindow->geometry();
        db->saveWindowGeometry(worldWidget->worldName(), geometry);
    });

    // Show the subwindow
    subWindow->show();

    // New worlds get default size (no saved geometry yet)
    subWindow->resize(800, 600);

    // Update menus
    updateMenus();

    statusBar()->showMessage("New world created", 2000);
}

void MainWindow::openWorld()
{
    // Use configured world directory (matches original MUSHclient behavior)
    QString startDir = GlobalOptions::instance()->defaultWorldFileDirectory();

    QString filename = QFileDialog::getOpenFileName(
        this, "Open World File", startDir, "MUSHclient World Files (*.mcl);;All Files (*)");

    if (!filename.isEmpty()) {
        openWorld(filename);
    }
}

void MainWindow::openWorld(const QString& filename)
{
    statusBar()->showMessage(QString("Opening %1...").arg(filename));

    // Create world widget
    WorldWidget* worldWidget = new WorldWidget();

    // Load world file
    if (!worldWidget->loadFromFile(filename)) {
        delete worldWidget;
        QMessageBox::critical(this, "Error",
                              QString("Failed to load world file:\n%1").arg(filename));
        statusBar()->showMessage("Failed to load world file", 3000);
        return;
    }

    // Add to recent files
    addRecentFile(filename);

    // Wrap in MDI subwindow
    QMdiSubWindow* subWindow = m_mdiArea->addSubWindow(worldWidget);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(worldWidget->worldName());

    // Remove the system menu (that extra menu bar inside the MDI area)
    subWindow->setSystemMenu(nullptr);

    // Connect window title changes
    connect(worldWidget, &WorldWidget::windowTitleChanged, subWindow,
            &QMdiSubWindow::setWindowTitle);

    // Connect connection state changes to update menus
    connect(worldWidget, &WorldWidget::connectedChanged, this, &MainWindow::updateMenus);

    // Connect notepad creation requests to create MDI subwindows
    connect(worldWidget, &WorldWidget::notepadRequested, this, &MainWindow::createNotepadWindow);

    // Connect to subwindow's destroyed signal to save geometry
    // This ensures we save before the window is deleted
    connect(subWindow, &QObject::destroyed, [worldWidget, subWindow]() {
        if (!worldWidget || worldWidget->worldName().isEmpty()) {
            return;
        }

        // Save current window geometry to database
        Database* db = Database::instance();
        QRect geometry = subWindow->geometry();
        db->saveWindowGeometry(worldWidget->worldName(), geometry);
    });

    // Show the subwindow
    subWindow->show();

    // Try to restore saved window geometry from database
    // Matches original MUSHclient winplace.cpp behavior
    Database* db = Database::instance();
    QRect savedGeometry;
    if (db->loadWindowGeometry(worldWidget->worldName(), savedGeometry)) {
        subWindow->setGeometry(savedGeometry);
        qCDebug(lcUI) << "Restored window geometry for" << worldWidget->worldName() << ":"
                      << savedGeometry;
    } else {
        // No saved geometry - use reasonable default size
        subWindow->resize(800, 600);
    }

    // Update menus
    updateMenus();

    // Check if auto-connect is enabled (matches original MUSHclient doc.cpp)
    bool autoConnect = db->getPreferenceInt("AutoConnectWorlds", 0) != 0;
    if (autoConnect) {
        worldWidget->connectToMud();
        statusBar()->showMessage(
            QString("Opened %1 - Auto-connecting...").arg(worldWidget->worldName()), 3000);
    } else {
        statusBar()->showMessage(QString("Opened %1").arg(worldWidget->worldName()), 3000);
    }
}

void MainWindow::closeWorld()
{
    if (m_mdiArea->activeSubWindow()) {
        m_mdiArea->activeSubWindow()->close();
    }
}

void MainWindow::saveWorld()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Check if world has existing filename
    QString filename = worldWidget->filename();

    // If no filename (new world), prompt for one (Save As behavior)
    if (filename.isEmpty()) {
        // Use configured world directory (matches original MUSHclient behavior)
        QString startDir = GlobalOptions::instance()->defaultWorldFileDirectory();
        filename = QFileDialog::getSaveFileName(
            this, "Save World File", startDir, "MUSHclient World Files (*.mcl);;All Files (*)");

        if (filename.isEmpty()) {
            return; // User cancelled
        }
    }

    // Save to file
    if (worldWidget->saveToFile(filename)) {
        addRecentFile(filename);
        statusBar()->showMessage(QString("Saved %1").arg(worldWidget->worldName()), 3000);
    } else {
        QMessageBox::critical(this, "Error",
                              QString("Failed to save world file:\n%1").arg(filename));
        statusBar()->showMessage("Failed to save world file", 3000);
    }
}

void MainWindow::saveWorldAs()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Use configured world directory (matches original MUSHclient behavior)
    QString startDir = GlobalOptions::instance()->defaultWorldFileDirectory();
    QString filename = QFileDialog::getSaveFileName(
        this, "Save World File As", startDir, "MUSHclient World Files (*.mcl);;All Files (*)");

    if (filename.isEmpty()) {
        return;
    }

    if (worldWidget->saveToFile(filename)) {
        addRecentFile(filename);
        activeSubWindow->setWindowTitle(worldWidget->worldName());
        statusBar()->showMessage(
            QString("Saved %1 as %2").arg(worldWidget->worldName()).arg(filename), 3000);
    } else {
        QMessageBox::critical(this, "Error",
                              QString("Failed to save world file:\n%1").arg(filename));
        statusBar()->showMessage("Failed to save world file", 3000);
    }
}

void MainWindow::worldProperties()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open World Properties dialog
    WorldPropertiesDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("World properties updated", 2000);
}

void MainWindow::toggleLogSession()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Toggle log state
    if (doc->IsLogOpen()) {
        // Close the log
        qint32 result = doc->CloseLog();
        if (result == 0) {
            statusBar()->showMessage("Log session closed", 2000);
        } else {
            QMessageBox::warning(this, "Log Error", "Failed to close log file.");
            statusBar()->showMessage("Failed to close log", 2000);
        }
    } else {
        // Open the log - prompt for filename
        QString defaultName = QString("%1_%2.log")
                                  .arg(worldWidget->worldName())
                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        // Use configured log directory (matches original MUSHclient behavior)
        QString logDir = GlobalOptions::instance()->defaultLogFileDirectory();
        QString filename = QFileDialog::getSaveFileName(
            this, "Save Log File", logDir + "/" + defaultName,
            "Log Files (*.log *.txt);;All Files (*)");

        if (filename.isEmpty()) {
            // User cancelled - uncheck the menu item
            m_logSessionAction->setChecked(false);
            return;
        }

        // Open the log (append = true by default)
        qint32 result = doc->OpenLog(filename, true);
        if (result == 0) {
            statusBar()->showMessage(QString("Logging to %1").arg(QFileInfo(filename).fileName()),
                                     3000);
        } else {
            QMessageBox::critical(this, "Log Error",
                                  QString("Failed to open log file:\n%1").arg(filename));
            statusBar()->showMessage("Failed to open log", 2000);
        }
    }

    // Update menus to reflect new log state
    updateMenus();
}

void MainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filename = action->data().toString();
        openWorld(filename);
    }
}

void MainWindow::exitApplication()
{
    close();
}

// Edit menu slots
void MainWindow::copy()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Get the output view and copy selected text
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->copyToClipboard();
        statusBar()->showMessage("Copied to clipboard", 2000);
    }
}

void MainWindow::copyAsHtml()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Get the output view and copy selected text as HTML
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->copyAsHtml();
        statusBar()->showMessage("Copied as HTML to clipboard", 2000);
    }
}

void MainWindow::paste()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Get the input view from the world widget and paste
    // InputView is a QLineEdit, which has built-in paste() support
    if (worldWidget->document()) {
        worldWidget->activateInputArea(); // Focus the input first

        // Get the input view directly
        InputView* inputView = worldWidget->inputView();
        if (inputView) {
            inputView->paste();
            statusBar()->showMessage("Pasted from clipboard", 2000);
        }
    }
}

void MainWindow::pasteToMud()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    WorldDocument* doc = worldWidget->document();

    // Get clipboard contents
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text();

    if (text.isEmpty()) {
        statusBar()->showMessage("Clipboard is empty", 2000);
        return;
    }

    // Get settings from confirm dialog or use defaults
    QString preamble = doc->m_paste_preamble;
    QString postamble = doc->m_paste_postamble;
    QString linePreamble = doc->m_pasteline_preamble;
    QString linePostamble = doc->m_pasteline_postamble;
    bool commentedSoftcode = doc->m_bPasteCommentedSoftcode != 0;
    int lineDelay = doc->m_nPasteDelay;
    int lineDelayPerLines = doc->m_nPasteDelayPerLines;
    bool echo = doc->m_bPasteEcho != 0;
    int lineCount = text.count('\n') + 1;

    // Check if confirmation is needed
    if (doc->m_bConfirmOnPaste) {
        ConfirmPreambleDialog dlg(this);
        dlg.setPasteMessage(QString("About to send: %1 characters, %2 lines to %3")
                                .arg(text.length())
                                .arg(lineCount)
                                .arg(doc->m_mush_name));
        dlg.setPreamble(preamble);
        dlg.setPostamble(postamble);
        dlg.setLinePreamble(linePreamble);
        dlg.setLinePostamble(linePostamble);
        dlg.setCommentedSoftcode(commentedSoftcode);
        dlg.setLineDelay(lineDelay);
        dlg.setLineDelayPerLines(lineDelayPerLines);
        dlg.setEcho(echo);

        if (dlg.exec() != QDialog::Accepted) {
            statusBar()->showMessage("Paste cancelled", 2000);
            return;
        }

        // Use values from dialog (user may have modified them)
        preamble = dlg.preamble();
        postamble = dlg.postamble();
        linePreamble = dlg.linePreamble();
        linePostamble = dlg.linePostamble();
        commentedSoftcode = dlg.commentedSoftcode();
        lineDelay = dlg.lineDelay();
        lineDelayPerLines = dlg.lineDelayPerLines();
        echo = dlg.echo();
    }

    // Create progress dialog
    ProgressDialog progressDlg("Pasting to MUD", this);
    progressDlg.setRange(0, lineCount);
    progressDlg.setMessage(
        QString("Sending %1 lines to %2...").arg(lineCount).arg(doc->m_mush_name));
    progressDlg.setCancelable(true);
    progressDlg.show();

    // Progress callback
    auto progressCallback = [&progressDlg](int current, int total) -> bool {
        progressDlg.setProgress(current);
        progressDlg.setMessage(QString("Sending line %1 of %2...").arg(current + 1).arg(total));
        return !progressDlg.wasCanceled();
    };

    // Send the text
    bool completed =
        doc->sendTextToMud(text, preamble, linePreamble, linePostamble, postamble,
                           commentedSoftcode, lineDelay, lineDelayPerLines, echo, progressCallback);

    progressDlg.close();

    if (completed) {
        statusBar()->showMessage("Pasted to MUD", 2000);
    } else {
        statusBar()->showMessage("Paste cancelled", 2000);
    }
}

void MainWindow::selectAll()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Get the output view and select all text
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->selectAll();
        statusBar()->showMessage("All text selected", 2000);
    }
}

void MainWindow::find()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Find dialog
    FindDialog dialog(worldWidget->document(), worldWidget);
    if (dialog.exec() == QDialog::Accepted) {
        // Store last search parameters and position for Find Next
        m_lastSearchText = dialog.lastSearchText();
        m_lastSearchMatchCase = dialog.lastMatchCase();
        m_lastSearchUseRegex = dialog.lastUseRegex();
        m_lastSearchForward = dialog.lastSearchForward();
        m_lastFoundLine = dialog.lastFoundLine();
        m_lastFoundChar = dialog.lastFoundChar();

        // Update Find Next menu state
        updateMenus();
    }
}

void MainWindow::findNext()
{
    if (m_lastSearchText.isEmpty()) {
        // No previous search - open find dialog
        find();
        return;
    }

    // Perform search with stored parameters
    if (!performSearch()) {
        statusBar()->showMessage(QString("Cannot find \"%1\"").arg(m_lastSearchText), 3000);
    }
}

void MainWindow::preferences()
{
    // Open Global Preferences dialog
    GlobalPreferencesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        statusBar()->showMessage("Preferences saved", 2000);
        // Apply toolbar preferences that may have changed
        applyToolbarPreferences();
    } else {
        statusBar()->showMessage("Preferences cancelled", 2000);
    }
}

void MainWindow::recall()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Open Recall Search dialog
    RecallSearchDialog dialog(doc, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // Perform the search
    QString result = doc->RecallText(
        dialog.searchText(), dialog.matchCase(), dialog.useRegex(), dialog.includeOutput(),
        dialog.includeCommands(), dialog.includeNotes(), dialog.lineCount(), dialog.linePreamble());

    if (result.isEmpty()) {
        QMessageBox::information(
            this, "Recall", QString("No lines found matching \"%1\"").arg(dialog.searchText()));
        return;
    }

    // Create notepad window with results
    QString title = QString("Recall: %1").arg(dialog.searchText());
    doc->SendToNotepad(title, result);
    doc->ActivateNotepad(title);

    statusBar()->showMessage(QString("Recall completed: %1 characters found").arg(result.length()),
                             3000);
}

void MainWindow::generateCharacterName()
{
    GenerateNameDialog dialog(this);
    dialog.exec();
}

void MainWindow::generateUniqueID()
{
    GenerateIDDialog dialog(this);
    dialog.exec();
}

// Input menu slots
void MainWindow::activateInputArea()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Activate the input area
    worldWidget->activateInputArea();
}

void MainWindow::previousCommand()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->previousCommand();
}

void MainWindow::nextCommand()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->nextCommand();
}

void MainWindow::repeatLastCommand()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->repeatLastCommand();
    statusBar()->showMessage("Repeated last command", 2000);
}

void MainWindow::clearCommandHistory()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Confirm before clearing
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Command History", "Are you sure you want to clear all command history?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        worldWidget->clearCommandHistory();
        statusBar()->showMessage("Command history cleared", 2000);
    }
}

void MainWindow::showCommandHistory()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Command History Dialog
    CommandHistoryDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Command history closed", 2000);
}

void MainWindow::globalChange()
{
    // Global Change does search/replace on the command input text
    // Based on CSendView::OnInputGlobalchange() from sendvw.cpp

    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    InputView* inputView = worldWidget->inputView();
    if (!inputView) {
        return;
    }

    // Get current text and selection
    QString currentText = inputView->text();
    if (currentText.isEmpty()) {
        statusBar()->showMessage("No text in command input", 2000);
        return;
    }

    GlobalChangeDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString findText = dialog.findText();
    QString replaceText = dialog.replaceText();

    if (findText.isEmpty()) {
        return;
    }

    // Handle escape sequences like original: \n -> newline, \t -> tab, \\ -> backslash
    QString findProcessed = findText;
    findProcessed.replace("\\\\", "\x01");
    findProcessed.replace("\\n", "\n");
    findProcessed.replace("\\t", "\t");
    findProcessed.replace("\x01", "\\");

    QString replaceProcessed = replaceText;
    replaceProcessed.replace("\\\\", "\x01");
    replaceProcessed.replace("\\n", "\n");
    replaceProcessed.replace("\\t", "\t");
    replaceProcessed.replace("\x01", "\\");

    // Get selection - if text selected, only replace in selection
    QTextCursor cursor = inputView->textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    int selLength = selEnd - selStart;

    QString textToProcess;
    if (selLength > 0) {
        textToProcess = cursor.selectedText();
    } else {
        textToProcess = currentText;
        selStart = 0;
        selLength = currentText.length();
    }

    // Perform replacement
    QString newText = textToProcess.replace(findProcessed, replaceProcessed);

    if (newText == textToProcess) {
        statusBar()->showMessage(QString("No replacements made for '%1'").arg(findText), 2000);
        return;
    }

    // Replace in input view
    if (selLength > 0 && selLength < currentText.length()) {
        // Replace only selection - reselect the text
        cursor.setPosition(selStart);
        cursor.setPosition(selStart + selLength, QTextCursor::KeepAnchor);
        inputView->setTextCursor(cursor);
        inputView->textCursor().insertText(newText);
    } else {
        // Replace all
        inputView->selectAll();
        inputView->textCursor().insertText(newText);
    }

    statusBar()->showMessage("Global change completed", 2000);
}

void MainWindow::discardQueuedCommands()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    qint32 count = doc->DiscardQueue();

    QString message = count > 0 ? QString("Discarded %1 queued command(s)").arg(count)
                                : "No queued commands to discard";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::showKeyName()
{
    KeyNameDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString keyName = dialog.keyName();
        if (!keyName.isEmpty()) {
            statusBar()->showMessage(QString("Key: %1").arg(keyName), 3000);
        }
    }
}

// Game menu slots
void MainWindow::connectToMud()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->connectToMud();
    statusBar()->showMessage("Connecting...", 2000);
}

void MainWindow::disconnectFromMud()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->disconnectFromMud();
    statusBar()->showMessage("Disconnecting...", 2000);
}

void MainWindow::toggleAutoConnect()
{
    // Toggle the global AutoConnectWorlds preference
    Database* db = Database::instance();
    bool currentValue = db->getPreferenceInt("AutoConnectWorlds", 0) != 0;
    bool newValue = !currentValue;

    db->setPreferenceInt("AutoConnectWorlds", newValue ? 1 : 0);
    m_autoConnectAction->setChecked(newValue);

    QString message = newValue ? "Auto-connect enabled" : "Auto-connect disabled";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::toggleReconnectOnDisconnect()
{
    // Toggle the global ReconnectOnDisconnect preference
    Database* db = Database::instance();
    bool currentValue = db->getPreferenceInt("ReconnectOnDisconnect", 0) != 0;
    bool newValue = !currentValue;

    db->setPreferenceInt("ReconnectOnDisconnect", newValue ? 1 : 0);
    m_reconnectOnDisconnectAction->setChecked(newValue);

    QString message =
        newValue ? "Reconnect on disconnect enabled" : "Reconnect on disconnect disabled";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::connectToAllOpenWorlds()
{
    // Connect to all open but disconnected worlds
    int connectedCount = 0;
    for (QMdiSubWindow* window : m_mdiArea->subWindowList()) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(window->widget());
        if (worldWidget && !worldWidget->isConnected()) {
            worldWidget->connectToMud();
            connectedCount++;
        }
    }

    QString message = connectedCount > 0 ? QString("Connecting to %1 world(s)").arg(connectedCount)
                                         : "No disconnected worlds to connect";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::connectToStartupList()
{
    // Open and connect to all worlds in the startup list
    Database* db = Database::instance();
    QString startupList = db->getPreference("WorldList", "");

    if (startupList.isEmpty()) {
        statusBar()->showMessage("No worlds in startup list", 2000);
        return;
    }

    // Split by newlines or semicolons (common separators)
    QStringList worldFiles = startupList.split(QRegularExpression("[\\n;]"), Qt::SkipEmptyParts);

    int openedCount = 0;
    for (const QString& worldFile : worldFiles) {
        QString trimmed = worldFile.trimmed();
        if (!trimmed.isEmpty() && QFile::exists(trimmed)) {
            openWorld(trimmed);
            openedCount++;
        }
    }

    QString message = openedCount > 0
                          ? QString("Opened %1 world(s) from startup list").arg(openedCount)
                          : "No valid worlds in startup list";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::reloadScriptFile()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Check if a script file is configured
    if (doc->m_strScriptFilename.isEmpty()) {
        QMessageBox::information(
            this, "No Script File",
            "No script file is configured for this world.\n\n"
            "To set a script file, go to File → World Properties → Scripting tab.");
        statusBar()->showMessage("No script file configured", 2000);
        return;
    }

    // Reload the script file
    doc->loadScriptFile();
    statusBar()->showMessage(
        QString("Script file reloaded: %1").arg(QFileInfo(doc->m_strScriptFilename).fileName()),
        3000);
}

/**
 * Toggle Auto-Say Mode
 *
 * Based on issue #96: Auto-Say mode for role-playing
 *
 * When enabled, automatically prepends the auto-say string (default: "say ")
 * to all commands unless they start with the override prefix.
 *
 * Example:
 * - Auto-Say enabled, command "Hello!" → sends "say Hello!"
 * - Auto-Say enabled with override "/", command "/north" → sends "north"
 */
void MainWindow::toggleAutoSay()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Toggle Auto-Say setting
    bool newValue = m_autoSayAction->isChecked();
    doc->m_bEnableAutoSay = newValue ? 1 : 0;

    QString message = newValue ? "Auto-Say mode enabled" : "Auto-Say mode disabled";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::configureTriggers()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Trigger List Dialog
    TriggerListDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Trigger configuration closed", 2000);
}

void MainWindow::configureAliases()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Alias List Dialog
    AliasListDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Alias configuration closed", 2000);
}

void MainWindow::configureTimers()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Timer List Dialog
    TimerListDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Timer configuration closed", 2000);
}

void MainWindow::configureShortcuts()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Shortcut List Dialog
    ShortcutListDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Shortcut configuration closed", 2000);
}

void MainWindow::configurePlugins()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Plugin Dialog
    PluginDialog dialog(worldWidget->document(), this);
    dialog.exec();

    statusBar()->showMessage("Plugin configuration closed", 2000);
}

void MainWindow::pluginWizard()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Plugin Wizard
    PluginWizard wizard(worldWidget->document(), this);
    if (wizard.exec() == QDialog::Accepted) {
        statusBar()->showMessage("Plugin created successfully", 3000);
    } else {
        statusBar()->showMessage("Plugin wizard cancelled", 2000);
    }
}

// Display menu slots
void MainWindow::scrollToStart()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollToTop();
    }
}

void MainWindow::scrollPageUp()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollPageUp();
    }
}

void MainWindow::scrollPageDown()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollPageDown();
    }
}

void MainWindow::scrollToEnd()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollToBottom();
    }
}

void MainWindow::scrollLineUp()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollLineUp();
    }
}

void MainWindow::scrollLineDown()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Call scroll method directly
    OutputView* outputView = worldWidget->outputView();
    if (outputView) {
        outputView->scrollLineDown();
    }
}

// Window menu slots
void MainWindow::cascade()
{
    m_mdiArea->cascadeSubWindows();
}

void MainWindow::tileHorizontally()
{
    // Qt doesn't have built-in horizontal tiling, so we implement it
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    if (windows.isEmpty()) {
        return;
    }

    int heightPerWindow = m_mdiArea->height() / windows.size();
    int y = 0;

    for (QMdiSubWindow* window : windows) {
        window->showNormal();
        window->setGeometry(0, y, m_mdiArea->width(), heightPerWindow);
        y += heightPerWindow;
    }
}

void MainWindow::tileVertically()
{
    // Use Qt's built-in tile method (tiles vertically by default)
    m_mdiArea->tileSubWindows();
}

void MainWindow::closeAllWindows()
{
    m_mdiArea->closeAllSubWindows();
}

void MainWindow::toggleTabbedView(bool enabled)
{
    if (enabled) {
        m_mdiArea->setViewMode(QMdiArea::TabbedView);
        m_mdiArea->setTabsClosable(true);
        m_mdiArea->setTabsMovable(true);
    } else {
        m_mdiArea->setViewMode(QMdiArea::SubWindowView);
    }
}

void MainWindow::toggleAlwaysOnTop(bool enabled)
{
    // Toggle WindowStaysOnTopHint flag
    Qt::WindowFlags flags = windowFlags();
    if (enabled) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);

    // setWindowFlags hides the window, so we need to show it again
    show();

    QString message = enabled ? "Always on top enabled" : "Always on top disabled";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::toggleFullScreen(bool enabled)
{
    if (enabled) {
        showFullScreen();
    } else {
        showNormal();
    }

    QString message = enabled ? "Full screen mode" : "Windowed mode";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::resetToolbars()
{
    // Reset all toolbars to default positions (docked at top, visible)

    // Remove toolbars from current positions
    removeToolBar(m_mainToolBar);
    removeToolBar(m_gameToolBar);
    removeToolBar(m_activityToolBar);

    // Add them back at the top in order
    addToolBar(Qt::TopToolBarArea, m_mainToolBar);
    addToolBar(Qt::TopToolBarArea, m_gameToolBar);
    addToolBar(Qt::TopToolBarArea, m_activityToolBar);

    // Make them all visible
    m_mainToolBar->setVisible(true);
    m_gameToolBar->setVisible(true);
    m_activityToolBar->setVisible(true);

    // Reset InfoBar to bottom, hidden (default state)
    if (m_infoBarDock) {
        m_infoBarDock->setFloating(false);
        addDockWidget(Qt::BottomDockWidgetArea, m_infoBarDock);
        m_infoBarDock->hide();
    }

    // Update menu checkboxes
    m_mainToolBarAction->setChecked(true);
    m_gameToolBarAction->setChecked(true);
    m_activityToolBarAction->setChecked(true);
    m_infoBarAction->setChecked(false);

    statusBar()->showMessage("Toolbars reset to default positions", 2000);
}

int MainWindow::setToolBarPosition(int which, bool floating, int side, int top, int left)
{
    // Handle InfoBar (which=4) separately since it's a QDockWidget
    if (which == 4) {
        if (!m_infoBarDock) {
            return 30; // eBadParameter
        }

        if (floating) {
            m_infoBarDock->setFloating(true);
            m_infoBarDock->move(left, top);
        } else {
            Qt::DockWidgetArea area;
            switch (side) {
                case 1:
                    area = Qt::TopDockWidgetArea;
                    break;
                case 2:
                    area = Qt::BottomDockWidgetArea;
                    break;
                default:
                    area = Qt::BottomDockWidgetArea;
                    break;
            }
            m_infoBarDock->setFloating(false);
            addDockWidget(area, m_infoBarDock);
        }
        m_infoBarDock->setVisible(true);
        return 0; // eOK
    }

    // Get the toolbar based on which parameter
    QToolBar* toolbar = nullptr;
    switch (which) {
        case 1:
            toolbar = m_mainToolBar;
            break;
        case 2:
            toolbar = m_gameToolBar;
            break;
        case 3:
            toolbar = m_activityToolBar;
            break;
        default:
            return 30; // eBadParameter
    }

    if (!toolbar) {
        return 30; // eBadParameter
    }

    if (floating) {
        // Float the toolbar at the specified position
        removeToolBar(toolbar);
        addToolBar(toolbar);
        toolbar->setFloatable(true);
        toolbar->move(left, top);
        toolbar->setVisible(true);
    } else {
        // Dock the toolbar to the specified side
        Qt::ToolBarArea area;
        switch (side) {
            case 1:
                area = Qt::TopToolBarArea;
                break;
            case 2:
                area = Qt::BottomToolBarArea;
                break;
            case 3:
                area = Qt::LeftToolBarArea;
                break;
            case 4:
                area = Qt::RightToolBarArea;
                break;
            default:
                area = Qt::TopToolBarArea;
                break;
        }
        removeToolBar(toolbar);
        addToolBar(area, toolbar);
        toolbar->setVisible(true);
    }

    return 0; // eOK
}

int MainWindow::getToolBarInfo(int which, int infoType)
{
    // Handle InfoBar (which=4) separately since it's a QDockWidget
    if (which == 4) {
        if (!m_infoBarDock || !m_infoBarDock->isVisible()) {
            return 0;
        }
        if (infoType == 0) {
            return m_infoBarDock->height();
        } else if (infoType == 1) {
            return m_infoBarDock->width();
        }
        return 0;
    }

    // Get the toolbar based on which parameter
    QToolBar* toolbar = nullptr;
    switch (which) {
        case 1:
            toolbar = m_mainToolBar;
            break;
        case 2:
            toolbar = m_gameToolBar;
            break;
        case 3:
            toolbar = m_activityToolBar;
            break;
        default:
            return 0; // Invalid toolbar
    }

    if (!toolbar || !toolbar->isVisible()) {
        return 0;
    }

    if (infoType == 0) {
        return toolbar->height();
    } else if (infoType == 1) {
        return toolbar->width();
    }

    return 0;
}

// Help menu slots
void MainWindow::showHelp()
{
    QMessageBox::information(this, "Help",
                             "Mushkin Help\n\n"
                             "Online documentation:\n"
                             "https://www.gammon.com.au/mushclient\n\n"
                             "Mushkin is a cross-platform MUD client based on MUSHclient.");
}

void MainWindow::about()
{
    QMessageBox::about(
        this, "About Mushkin",
        "<h2>Mushkin</h2>"
        "<p><b>Version 5.0.0</b></p>"
        "<p>Cross-platform MUD client built with Qt 6</p>"
        "<p>Based on MUSHclient by Nick Gammon</p>"
        "<p><a href='https://www.gammon.com.au/mushclient'>www.gammon.com.au/mushclient</a></p>"
        "<hr>"
        "<p>A streamlined port maintaining compatibility "
        "with existing world files and plugins.</p>");
}

// Additional Edit/File/Display menu slots
void MainWindow::saveSelection()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    OutputView* outputView = worldWidget->outputView();
    if (!outputView) {
        return;
    }

    // Get selected text
    QString selectedText = outputView->getSelectedText();
    if (selectedText.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select some text first.");
        return;
    }

    // Prompt for filename
    QString filename = QFileDialog::getSaveFileName(
        this, "Save Selection", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Text Files (*.txt);;All Files (*)");

    if (filename.isEmpty()) {
        return; // User cancelled
    }

    // Save to file
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << selectedText;
        file.close();
        statusBar()->showMessage(
            QString("Selection saved to %1").arg(QFileInfo(filename).fileName()), 3000);
    } else {
        QMessageBox::critical(this, "Error", QString("Failed to save file:\n%1").arg(filename));
        statusBar()->showMessage("Failed to save selection", 2000);
    }
}

void MainWindow::insertDateTime()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Get current date/time formatted nicely
    QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // Insert into input field
    InputView* inputView = worldWidget->inputView();
    if (inputView) {
        inputView->insertPlainText(dateTime);
        statusBar()->showMessage("Date/time inserted", 2000);
    }
}

void MainWindow::wordCount()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    OutputView* outputView = worldWidget->outputView();
    WorldDocument* doc = worldWidget->document();
    if (!outputView || !doc) {
        return;
    }

    // Get text - either selection or entire document
    QString text;
    bool isSelection = false;

    if (outputView->hasSelection()) {
        text = outputView->getSelectedText();
        isSelection = true;
    } else {
        // Get all text from document (matches original MUSHclient behavior)
        QStringList lines;
        for (Line* line : doc->m_lineList) {
            if (line && line->len() > 0) {
                lines.append(QString::fromUtf8(line->text(), line->len()));
            }
        }
        text = lines.join('\n');
    }

    if (text.isEmpty()) {
        QMessageBox::information(this, "Word Count", "No text to count.");
        return;
    }

    // Count using original MUSHclient algorithm (TextView.cpp)
    int lineCount = 0;
    int wordCount = 0;
    int charCount = text.length();

    // Count newlines
    for (int i = 0; i < text.length(); i++) {
        if (text[i] == '\n') {
            lineCount++;
        }
        // Count words: space followed by non-space
        if (i > 0 && text[i - 1].isSpace() && !text[i].isSpace()) {
            wordCount++;
        }
    }

    // Unless zero length, must have one line in it
    if (!text.isEmpty()) {
        lineCount++;
        // If first character is not a space, that counts as our first word
        if (!text[0].isSpace()) {
            wordCount++;
        }
    }

    // Build message with proper pluralization
    QString scope = isSelection ? "selection" : "document";
    QString linePlural = (lineCount == 1) ? "" : "s";
    QString wordPlural = (wordCount == 1) ? "" : "s";
    QString charPlural = (charCount == 1) ? "" : "s";

    QString message = QString("The %1 contains %2 line%3, %4 word%5, %6 character%7")
                          .arg(scope)
                          .arg(lineCount)
                          .arg(linePlural)
                          .arg(wordCount)
                          .arg(wordPlural)
                          .arg(charCount)
                          .arg(charPlural);

    QMessageBox::information(this, "Word Count", message);

    statusBar()->showMessage(QString("%1 words, %2 characters").arg(wordCount).arg(charCount),
                             3000);
}

void MainWindow::clearOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Confirm before clearing
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Output", "Are you sure you want to clear all output text?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Clear the line list
        doc->m_lineList.clear();

        // Clear current incomplete line if any
        if (doc->m_currentLine) {
            delete doc->m_currentLine;
            doc->m_currentLine = nullptr;
        }

        // Trigger redraw
        OutputView* outputView = worldWidget->outputView();
        if (outputView) {
            outputView->update();
        }

        statusBar()->showMessage("Output cleared", 2000);
    }
}

void MainWindow::toggleCommandEcho()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    if (!doc) {
        return;
    }

    // Toggle the echo command flag
    bool newValue = m_commandEchoAction->isChecked();
    doc->m_display_my_input = newValue ? 1 : 0;

    QString message = newValue ? "Command echo enabled" : "Command echo disabled";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::toggleFreezeOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    bool newValue = m_freezeOutputAction->isChecked();
    worldWidget->outputView()->setFrozen(newValue);

    QString message = newValue ? "Output frozen" : "Output unfrozen";
    statusBar()->showMessage(message, 2000);
}

void MainWindow::goToLine()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    // Get total lines from the world document's line list
    int totalLines = worldWidget->document()->m_lineList.count();
    if (totalLines == 0) {
        statusBar()->showMessage("No lines in output", 2000);
        return;
    }

    GoToLineDialog dlg(totalLines, 1, this);
    if (dlg.exec() == QDialog::Accepted) {
        int lineNumber = dlg.lineNumber();
        if (lineNumber > 0 && lineNumber <= totalLines && worldWidget->outputView()) {
            worldWidget->outputView()->scrollToLine(lineNumber - 1); // 0-based index
            statusBar()->showMessage(QString("Jumped to line %1").arg(lineNumber), 2000);
        }
    }
}

void MainWindow::goToUrl()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    QString selection = worldWidget->outputView()->getSelectedText().trimmed();
    if (selection.isEmpty()) {
        statusBar()->showMessage("No URL selected", 2000);
        return;
    }

    if (selection.length() > 512) {
        statusBar()->showMessage("URL too long", 2000);
        return;
    }

    // Add http:// if no protocol specified
    QString url = selection;
    if (!url.startsWith("http://", Qt::CaseInsensitive) &&
        !url.startsWith("https://", Qt::CaseInsensitive) &&
        !url.startsWith("ftp://", Qt::CaseInsensitive)) {
        url = "http://" + url;
    }

    QDesktopServices::openUrl(QUrl(url));
    statusBar()->showMessage(QString("Opening URL: %1").arg(selection), 2000);
}

void MainWindow::sendMailTo()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    QString selection = worldWidget->outputView()->getSelectedText().trimmed();
    if (selection.isEmpty()) {
        statusBar()->showMessage("No email address selected", 2000);
        return;
    }

    // Strip mailto: if already present
    QString email = selection;
    if (email.startsWith("mailto:", Qt::CaseInsensitive)) {
        email = email.mid(7);
    }

    QDesktopServices::openUrl(QUrl("mailto:" + email));
    statusBar()->showMessage(QString("Opening mail to: %1").arg(email), 2000);
}

void MainWindow::bookmarkSelection()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document() || !worldWidget->outputView()) {
        return;
    }

    int lineIndex = worldWidget->outputView()->getSelectionStartLine();
    if (lineIndex < 0) {
        // No selection - use last visible line
        lineIndex = worldWidget->document()->m_lineList.count() - 1;
    }

    if (lineIndex < 0 || lineIndex >= worldWidget->document()->m_lineList.count()) {
        statusBar()->showMessage("No line to bookmark", 2000);
        return;
    }

    Line* pLine = worldWidget->document()->m_lineList[lineIndex];
    pLine->flags ^= BOOKMARK; // Toggle bookmark

    QString message = (pLine->flags & BOOKMARK)
                          ? QString("Line %1 bookmarked").arg(lineIndex + 1)
                          : QString("Line %1 bookmark removed").arg(lineIndex + 1);
    statusBar()->showMessage(message, 2000);
    worldWidget->outputView()->update();
}

void MainWindow::goToBookmark()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document() || !worldWidget->outputView()) {
        return;
    }

    QList<Line*>& lines = worldWidget->document()->m_lineList;
    if (lines.isEmpty()) {
        return;
    }

    // Start from current selection or beginning
    int startLine = worldWidget->outputView()->getSelectionStartLine();
    if (startLine < 0)
        startLine = 0;

    // Search for next bookmark (wrap around)
    int searchStart = (startLine + 1) % lines.count();
    int current = searchStart;
    do {
        if (lines[current]->flags & BOOKMARK) {
            worldWidget->outputView()->scrollToLine(current);
            statusBar()->showMessage(QString("Jumped to bookmark at line %1").arg(current + 1),
                                     2000);
            return;
        }
        current = (current + 1) % lines.count();
    } while (current != searchStart);

    statusBar()->showMessage("No bookmarks found", 2000);
}

void MainWindow::activityList()
{
    // Toggle Activity Window visibility
    if (m_activityWindow->isVisible()) {
        m_activityWindow->hide();
    } else {
        m_activityWindow->refresh();
        m_activityWindow->show();
        m_activityWindow->raise();
    }
}

void MainWindow::textAttributes()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    TextAttributesDialog dlg(this);
    dlg.exec();
}

void MainWindow::multilineTrigger()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    MultilineTriggerDialog dlg(this);
    dlg.exec();
}

// ============================================================================
// Convert Menu Actions
// Text transformations for notepad windows (based on original TextView.cpp)
// ============================================================================

/**
 * Helper to get active notepad and its selection
 * Returns the notepad's QTextEdit, or nullptr if no notepad is active
 */
static QTextEdit* getActiveNotepadTextEdit(QMdiArea* mdiArea)
{
    QMdiSubWindow* activeSubWindow = mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return nullptr;
    }

    NotepadWidget* notepad = qobject_cast<NotepadWidget*>(activeSubWindow->widget());
    if (!notepad) {
        return nullptr;
    }

    return notepad->m_pTextEdit;
}

/**
 * Helper to transform text in active notepad
 * If text is selected, transforms selection; otherwise transforms all text
 */
static void transformNotepadText(QMdiArea* mdiArea,
                                 std::function<QString(const QString&)> transform,
                                 QStatusBar* statusBar, const QString& actionName)
{
    QTextEdit* textEdit = getActiveNotepadTextEdit(mdiArea);
    if (!textEdit) {
        if (statusBar) {
            statusBar->showMessage("No active notepad window", 2000);
        }
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    QString text;
    bool hasSelection = cursor.hasSelection();

    if (hasSelection) {
        text = cursor.selectedText();
        // QTextEdit uses paragraph separators (U+2029) instead of newlines
        text.replace(QChar::ParagraphSeparator, '\n');
    } else {
        text = textEdit->toPlainText();
        cursor.select(QTextCursor::Document);
    }

    QString transformed = transform(text);

    cursor.insertText(transformed);

    if (statusBar) {
        statusBar->showMessage(actionName + " completed", 2000);
    }
}

void MainWindow::convertUppercase()
{
    transformNotepadText(
        m_mdiArea, [](const QString& text) { return text.toUpper(); }, statusBar(), "Uppercase");
}

void MainWindow::convertLowercase()
{
    transformNotepadText(
        m_mdiArea, [](const QString& text) { return text.toLower(); }, statusBar(), "Lowercase");
}

void MainWindow::convertUnixToDos()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            // First normalize to LF only, then convert to CRLF
            result.replace("\r\n", "\n");
            result.replace("\r", "\n");
            result.replace("\n", "\r\n");
            return result;
        },
        statusBar(), "Unix to DOS");
}

void MainWindow::convertDosToUnix()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            result.replace("\r\n", "\n");
            result.replace("\r", "\n");
            return result;
        },
        statusBar(), "DOS to Unix");
}

void MainWindow::convertMacToDos()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            // First convert CRLF to LF, then CR to CRLF
            result.replace("\r\n", "\n");
            result.replace("\r", "\r\n");
            result.replace("\n", "\r\n");
            return result;
        },
        statusBar(), "Mac to DOS");
}

void MainWindow::convertDosToMac()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            result.replace("\r\n", "\r");
            result.replace("\n", "\r");
            return result;
        },
        statusBar(), "DOS to Mac");
}

void MainWindow::convertBase64Encode()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) { return QString::fromLatin1(text.toUtf8().toBase64()); },
        statusBar(), "Base64 Encode");
}

void MainWindow::convertBase64Decode()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QByteArray decoded = QByteArray::fromBase64(text.toLatin1());
            return QString::fromUtf8(decoded);
        },
        statusBar(), "Base64 Decode");
}

void MainWindow::convertHtmlEncode()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result;
            result.reserve(text.size() * 1.1);
            for (const QChar& ch : text) {
                if (ch == '<') {
                    result += "&lt;";
                } else if (ch == '>') {
                    result += "&gt;";
                } else if (ch == '&') {
                    result += "&amp;";
                } else if (ch == '"') {
                    result += "&quot;";
                } else if (ch == '\'') {
                    result += "&#39;";
                } else {
                    result += ch;
                }
            }
            return result;
        },
        statusBar(), "HTML Encode");
}

void MainWindow::convertHtmlDecode()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            result.replace("&lt;", "<");
            result.replace("&gt;", ">");
            result.replace("&quot;", "\"");
            result.replace("&#39;", "'");
            result.replace("&apos;", "'");
            result.replace("&amp;", "&"); // Must be last
            return result;
        },
        statusBar(), "HTML Decode");
}

void MainWindow::convertQuoteLines()
{
    QTextEdit* textEdit = getActiveNotepadTextEdit(m_mdiArea);
    if (!textEdit) {
        statusBar()->showMessage("No active notepad window", 2000);
        return;
    }

    bool ok;
    QString prefix = QInputDialog::getText(
        this, "Quote Lines", "Enter prefix for each line:", QLineEdit::Normal, "> ", &ok);
    if (!ok) {
        return;
    }

    transformNotepadText(
        m_mdiArea,
        [&prefix](const QString& text) {
            QStringList lines = text.split('\n');
            for (QString& line : lines) {
                line = prefix + line;
            }
            return lines.join('\n');
        },
        statusBar(), "Quote Lines");
}

void MainWindow::convertRemoveExtraBlanks()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            // Replace multiple spaces with single space
            static QRegularExpression multiSpace("  +");
            result.replace(multiSpace, " ");
            // Remove trailing whitespace from lines
            static QRegularExpression trailingWs("[ \\t]+\\n");
            result.replace(trailingWs, "\n");
            // Remove leading whitespace from lines
            static QRegularExpression leadingWs("\\n[ \\t]+");
            result.replace(leadingWs, "\n");
            return result.trimmed();
        },
        statusBar(), "Remove Extra Blanks");
}

void MainWindow::convertWrapLines()
{
    transformNotepadText(
        m_mdiArea,
        [](const QString& text) {
            QString result = text;
            // Replace newlines with spaces (keeping paragraph breaks as double newlines)
            result.replace("\r\n", "\n");
            result.replace("\r", "\n");
            result.replace("\n\n", "\x01"); // Preserve paragraph breaks
            result.replace("\n", " ");
            result.replace("\x01", "\n\n");
            // Clean up multiple spaces
            static QRegularExpression multiSpace("  +");
            result.replace(multiSpace, " ");
            return result;
        },
        statusBar(), "Wrap Lines");
}

// Helper method to perform search with stored parameters
bool MainWindow::performSearch()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return false;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return false;
    }

    WorldDocument* doc = worldWidget->document();
    OutputView* outputView = worldWidget->outputView();
    if (!doc || !outputView) {
        return false;
    }

    if (doc->m_lineList.isEmpty()) {
        return false;
    }

    Qt::CaseSensitivity cs = m_lastSearchMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;

    // Start from position after last find
    int startLine = (m_lastFoundLine >= 0) ? m_lastFoundLine : 0;
    int startChar = (m_lastFoundChar >= 0) ? m_lastFoundChar + 1 : 0;

    // Search forward from current position
    for (int i = startLine; i < doc->m_lineList.count(); i++) {
        Line* pLine = doc->m_lineList[i];
        if (!pLine || pLine->len() == 0)
            continue;

        QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

        int index = -1;
        if (m_lastSearchUseRegex) {
            QRegularExpression re(m_lastSearchText,
                                  m_lastSearchMatchCase
                                      ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(lineText, startChar);
            if (match.hasMatch()) {
                index = match.capturedStart();
            }
        } else {
            index = lineText.indexOf(m_lastSearchText, startChar, cs);
        }

        if (index != -1) {
            // Found!
            m_lastFoundLine = i;
            m_lastFoundChar = index;

            // Highlight the result
            outputView->selectTextAt(i, index, m_lastSearchText.length());
            statusBar()->showMessage("Found", 2000);
            return true;
        }

        startChar = 0; // Reset for next lines
    }

    // Not found - wrap around to beginning
    for (int i = 0; i < startLine; i++) {
        Line* pLine = doc->m_lineList[i];
        if (!pLine || pLine->len() == 0)
            continue;

        QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

        int index = -1;
        if (m_lastSearchUseRegex) {
            QRegularExpression re(m_lastSearchText,
                                  m_lastSearchMatchCase
                                      ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(lineText);
            if (match.hasMatch()) {
                index = match.capturedStart();
            }
        } else {
            index = lineText.indexOf(m_lastSearchText, 0, cs);
        }

        if (index != -1) {
            // Found!
            m_lastFoundLine = i;
            m_lastFoundChar = index;

            // Highlight the result
            outputView->selectTextAt(i, index, m_lastSearchText.length());
            statusBar()->showMessage("Found (wrapped)", 2000);
            return true;
        }
    }

    return false;
}

/**
 * createNotepadWindow - Wrap notepad widget in MDI subwindow
 *
 * This slot is called when a world's WorldDocument creates a notepad.
 * The notepad widget is wrapped in a QMdiSubWindow and added to the MDI area.
 *
 * @param notepad NotepadWidget to wrap (created by WorldDocument)
 */
void MainWindow::createNotepadWindow(NotepadWidget* notepad)
{
    if (!notepad) {
        return;
    }

    // Wrap in MDI subwindow
    QMdiSubWindow* subWindow = m_mdiArea->addSubWindow(notepad);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(notepad->m_strTitle);

    // Remove the system menu (that extra menu bar inside the MDI area)
    subWindow->setSystemMenu(nullptr);

    // Store reference to MDI subwindow in notepad
    notepad->m_pMdiSubWindow = subWindow;

    // Show the window
    subWindow->show();
}

// ============================================================================
// NEW DIALOG MENU ACTIONS
// ============================================================================

void MainWindow::quickConnect()
{
    QuickConnectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // TODO: Create temporary world and connect with the provided settings
        statusBar()->showMessage("Quick connect: feature in development", 3000);
    }
}

void MainWindow::importXml()
{
    // Get active world
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        QMessageBox::warning(this, "Import XML", "Please open a world first.");
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        QMessageBox::warning(this, "Import XML", "No active world document.");
        return;
    }

    ImportXmlDialog dlg(worldWidget->document(), this);
    if (dlg.exec() == QDialog::Accepted) {
        // The ImportXmlDialog handles file selection and import internally
        statusBar()->showMessage("XML import completed", 3000);
    }
}

void MainWindow::insertUnicode()
{
    InsertUnicodeDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString unicodeChar = dlg.character();
        if (!unicodeChar.isEmpty()) {
            // Get active world's input view and insert the character
            QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
            if (activeSubWindow) {
                WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
                if (worldWidget) {
                    worldWidget->inputView()->insertPlainText(unicodeChar);
                }
            }
        }
    }
}

void MainWindow::sendToAll()
{
    // Collect all open world names
    QStringList worldNames;
    for (QMdiSubWindow* subWindow : m_mdiArea->subWindowList()) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(subWindow->widget());
        if (worldWidget && worldWidget->document()) {
            worldNames.append(worldWidget->document()->m_mush_name);
        }
    }

    if (worldNames.isEmpty()) {
        QMessageBox::information(this, "Send to All", "No worlds are currently open.");
        return;
    }

    SendToAllDialog dlg(this);
    dlg.setWorlds(worldNames);
    if (dlg.exec() == QDialog::Accepted) {
        QString textToSend = dlg.sendText();
        QStringList selectedWorlds = dlg.selectedWorlds();
        // Send text to selected worlds
        for (QMdiSubWindow* subWindow : m_mdiArea->subWindowList()) {
            WorldWidget* worldWidget = qobject_cast<WorldWidget*>(subWindow->widget());
            if (worldWidget && worldWidget->document()) {
                if (selectedWorlds.contains(worldWidget->document()->m_mush_name)) {
                    worldWidget->document()->sendToMud(textToSend);
                }
            }
        }
    }
}

void MainWindow::asciiArt()
{
    AsciiArtDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString art = dlg.generatedArt();
        if (!art.isEmpty()) {
            // Get active world's input view and insert the art
            QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
            if (activeSubWindow) {
                WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
                if (worldWidget) {
                    worldWidget->inputView()->insertPlainText(art);
                }
            }
        }
    }
}

void MainWindow::highlightPhrase()
{
    HighlightPhraseDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // Get the phrase settings and apply them to the active world
        QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
        if (activeSubWindow) {
            WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
            if (worldWidget && worldWidget->document()) {
                // TODO: Apply highlight settings to the world document
                QString phrase = dlg.text();
                if (!phrase.isEmpty()) {
                    statusBar()->showMessage(
                        QString("Highlight phrase '%1' configured").arg(phrase), 3000);
                }
            }
        }
    }
}

int MainWindow::findMatchingBracePosition(const QString& text, int cursorPos)
{
    if (text.isEmpty() || cursorPos < 0 || cursorPos > text.length())
        return -1;

    // Check character at cursor position or just before it
    int checkPos = cursorPos;
    if (checkPos == text.length())
        checkPos--;
    else if (checkPos > 0) {
        // Check if character at cursor is not a brace, try one before
        QChar c = text[checkPos];
        if (c != '{' && c != '}' && c != '[' && c != ']' && c != '(' && c != ')')
            checkPos--;
    }

    if (checkPos < 0)
        return -1;

    QChar startChar = text[checkPos];
    QChar matchChar;
    int direction = 0;

    // Determine direction and matching character
    if (startChar == '{') {
        matchChar = '}';
        direction = 1;
    } else if (startChar == '}') {
        matchChar = '{';
        direction = -1;
    } else if (startChar == '[') {
        matchChar = ']';
        direction = 1;
    } else if (startChar == ']') {
        matchChar = '[';
        direction = -1;
    } else if (startChar == '(') {
        matchChar = ')';
        direction = 1;
    } else if (startChar == ')') {
        matchChar = '(';
        direction = -1;
    } else {
        return -1; // Not on a brace
    }

    // Search for matching brace
    int level = 1;
    int pos = checkPos + direction;

    while (pos >= 0 && pos < text.length()) {
        QChar c = text[pos];
        if (c == startChar)
            level++;
        else if (c == matchChar)
            level--;

        if (level == 0)
            return pos;

        pos += direction;
    }

    return -1; // No match found
}

void MainWindow::goToMatchingBrace()
{
    // Get the focused widget
    QWidget* focusWidget = QApplication::focusWidget();

    // Try QPlainTextEdit first (notepad, script editors)
    if (QPlainTextEdit* plainEdit = qobject_cast<QPlainTextEdit*>(focusWidget)) {
        QString text = plainEdit->toPlainText();
        int cursorPos = plainEdit->textCursor().position();
        int matchPos = findMatchingBracePosition(text, cursorPos);
        if (matchPos < 0) {
            QApplication::beep();
            return;
        }
        QTextCursor cursor = plainEdit->textCursor();
        cursor.setPosition(matchPos);
        plainEdit->setTextCursor(cursor);
        return;
    }

    // Try QLineEdit (command input)
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(focusWidget)) {
        QString text = lineEdit->text();
        int cursorPos = lineEdit->cursorPosition();
        int matchPos = findMatchingBracePosition(text, cursorPos);
        if (matchPos < 0) {
            QApplication::beep();
            return;
        }
        lineEdit->setCursorPosition(matchPos);
        return;
    }

    // No suitable editor found
    QApplication::beep();
}

void MainWindow::selectToMatchingBrace()
{
    // Get the focused widget
    QWidget* focusWidget = QApplication::focusWidget();

    // Try QPlainTextEdit first (notepad, script editors)
    if (QPlainTextEdit* plainEdit = qobject_cast<QPlainTextEdit*>(focusWidget)) {
        QString text = plainEdit->toPlainText();
        int cursorPos = plainEdit->textCursor().position();
        int matchPos = findMatchingBracePosition(text, cursorPos);
        if (matchPos < 0) {
            QApplication::beep();
            return;
        }
        QTextCursor cursor = plainEdit->textCursor();
        int startPos = cursor.position();
        if (matchPos > startPos) {
            cursor.setPosition(startPos);
            cursor.setPosition(matchPos + 1, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(startPos);
            cursor.setPosition(matchPos, QTextCursor::KeepAnchor);
        }
        plainEdit->setTextCursor(cursor);
        return;
    }

    // Try QLineEdit (command input)
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(focusWidget)) {
        QString text = lineEdit->text();
        int cursorPos = lineEdit->cursorPosition();
        int matchPos = findMatchingBracePosition(text, cursorPos);
        if (matchPos < 0) {
            QApplication::beep();
            return;
        }
        int startPos = cursorPos;
        if (matchPos > startPos) {
            lineEdit->setSelection(startPos, matchPos - startPos + 1);
        } else {
            lineEdit->setSelection(matchPos, startPos - matchPos + 1);
        }
        return;
    }

    // No suitable editor found
    QApplication::beep();
}

void MainWindow::immediateScript()
{
    // Get active world
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        QMessageBox::warning(this, "Immediate Script", "Please open a world first.");
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        QMessageBox::warning(this, "Immediate Script", "No active world document.");
        return;
    }

    ImmediateDialog dlg(worldWidget->document(), this);
    dlg.exec();
}

void MainWindow::commandOptions()
{
    // Get active world
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        QMessageBox::warning(this, "Command Options", "Please open a world first.");
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        QMessageBox::warning(this, "Command Options", "No active world document.");
        return;
    }

    CommandOptionsDialog dlg(worldWidget->document(), this);
    if (dlg.exec() == QDialog::Accepted) {
        // Settings are saved by the dialog
    }
}

void MainWindow::tabDefaults()
{
    // Get active world
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        QMessageBox::warning(this, "Tab Completion", "Please open a world first.");
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        QMessageBox::warning(this, "Tab Completion", "No active world document.");
        return;
    }

    TabDefaultsDialog dlg(worldWidget->document(), this);
    dlg.exec();
}

void MainWindow::sendFile()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        statusBar()->showMessage("No active world", 2000);
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    WorldDocument* doc = worldWidget->document();

    // Open file dialog
    QString fileName = QFileDialog::getOpenFileName(
        this, QString("File to send to %1").arg(doc->m_mush_name), QString(),
        "MUD files (*.mud *.mush);;Text files (*.txt);;All files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    // Read file contents
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", QString("Unable to open file: %1").arg(fileName));
        return;
    }

    QString text = QString::fromUtf8(file.readAll());
    file.close();

    if (text.isEmpty()) {
        statusBar()->showMessage("File is empty", 2000);
        return;
    }

    // Get settings from confirm dialog or use defaults
    QString preamble = doc->m_file_preamble;
    QString postamble = doc->m_file_postamble;
    QString linePreamble = doc->m_line_preamble;
    QString linePostamble = doc->m_line_postamble;
    bool commentedSoftcode = doc->m_bFileCommentedSoftcode != 0;
    int lineDelay = doc->m_nFileDelay;
    int lineDelayPerLines = doc->m_nFileDelayPerLines;
    bool echo = doc->m_bSendEcho != 0;
    int lineCount = text.count('\n') + 1;
    QString shortFileName = QFileInfo(fileName).fileName();

    // Check if confirmation is needed
    if (doc->m_bConfirmOnSend) {
        ConfirmPreambleDialog dlg(this);
        dlg.setPasteMessage(QString("About to send: %1 characters, %2 lines from %3 to %4")
                                .arg(text.length())
                                .arg(lineCount)
                                .arg(shortFileName)
                                .arg(doc->m_mush_name));
        dlg.setPreamble(preamble);
        dlg.setPostamble(postamble);
        dlg.setLinePreamble(linePreamble);
        dlg.setLinePostamble(linePostamble);
        dlg.setCommentedSoftcode(commentedSoftcode);
        dlg.setLineDelay(lineDelay);
        dlg.setLineDelayPerLines(lineDelayPerLines);
        dlg.setEcho(echo);

        if (dlg.exec() != QDialog::Accepted) {
            statusBar()->showMessage("Send file cancelled", 2000);
            return;
        }

        // Use values from dialog (user may have modified them)
        preamble = dlg.preamble();
        postamble = dlg.postamble();
        linePreamble = dlg.linePreamble();
        linePostamble = dlg.linePostamble();
        commentedSoftcode = dlg.commentedSoftcode();
        lineDelay = dlg.lineDelay();
        lineDelayPerLines = dlg.lineDelayPerLines();
        echo = dlg.echo();
    }

    // Create progress dialog
    ProgressDialog progressDlg("Sending File", this);
    progressDlg.setRange(0, lineCount);
    progressDlg.setMessage(QString("Sending %1 (%2 lines) to %3...")
                               .arg(shortFileName)
                               .arg(lineCount)
                               .arg(doc->m_mush_name));
    progressDlg.setCancelable(true);
    progressDlg.show();

    // Progress callback
    auto progressCallback = [&progressDlg, lineCount](int current, int total) -> bool {
        progressDlg.setProgress(current);
        progressDlg.setMessage(QString("Sending line %1 of %2...").arg(current + 1).arg(total));
        return !progressDlg.wasCanceled();
    };

    // Send the text
    bool completed =
        doc->sendTextToMud(text, preamble, linePreamble, linePostamble, postamble,
                           commentedSoftcode, lineDelay, lineDelayPerLines, echo, progressCallback);

    progressDlg.close();

    if (completed) {
        statusBar()->showMessage(QString("Sent file: %1").arg(shortFileName), 2000);
    } else {
        statusBar()->showMessage("Send file cancelled", 2000);
    }
}

void MainWindow::showMapper()
{
    // Get active world
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        QMessageBox::warning(this, "Mapper", "Please open a world first.");
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        QMessageBox::warning(this, "Mapper", "No active world document.");
        return;
    }

    MapDialog dlg(worldWidget->document(), this);
    dlg.exec();
}

// Status bar indicator updates

void MainWindow::updateStatusIndicators()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    WorldWidget* worldWidget = nullptr;

    if (activeSubWindow) {
        worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    }

    // Disconnect from previous world's signals
    if (m_trackedWorld && m_trackedWorld != worldWidget) {
        if (OutputView* outputView = m_trackedWorld->outputView()) {
            disconnect(outputView, &OutputView::freezeStateChanged, this,
                       &MainWindow::onFreezeStateChanged);
        }
        disconnect(m_trackedWorld, &WorldWidget::connectedChanged, this,
                   &MainWindow::onConnectionStateChanged);
    }

    // Connect to new world's signals
    if (worldWidget && worldWidget != m_trackedWorld) {
        if (OutputView* outputView = worldWidget->outputView()) {
            connect(outputView, &OutputView::freezeStateChanged, this,
                    &MainWindow::onFreezeStateChanged);
        }
        connect(worldWidget, &WorldWidget::connectedChanged, this,
                &MainWindow::onConnectionStateChanged);
    }

    m_trackedWorld = worldWidget;

    // Update indicators based on current state
    if (!worldWidget) {
        // No active world
        m_freezeIndicator->setText("");
        m_connectionIndicator->setText("");
        m_linesIndicator->setText("");
        return;
    }

    // Connection indicator
    bool isConnected = worldWidget->isConnected();
    m_connectionIndicator->setText(isConnected ? "" : "CLOSED");

    // Freeze indicator
    OutputView* outputView = worldWidget->outputView();
    if (outputView && outputView->isFrozen()) {
        int pendingLines = outputView->frozenLineCount();
        if (pendingLines > 0) {
            // There are lines waiting - show MORE (inverted style)
            m_freezeIndicator->setText("MORE");
            m_freezeIndicator->setStyleSheet(
                "QLabel { background-color: #000000; color: #FFFFFF; }");
        } else {
            // Frozen but at end of buffer - show PAUSE
            m_freezeIndicator->setText("PAUSE");
            m_freezeIndicator->setStyleSheet("");
        }
    } else {
        m_freezeIndicator->setText("");
        m_freezeIndicator->setStyleSheet("");
    }

    // Lines indicator - show line count in output buffer
    if (WorldDocument* doc = worldWidget->document()) {
        int lineCount = doc->m_lineList.count();
        m_linesIndicator->setText(QString("%1 lines").arg(lineCount));
    } else {
        m_linesIndicator->setText("");
    }
}

void MainWindow::onFreezeStateChanged(bool frozen, int lineCount)
{
    Q_UNUSED(frozen);
    Q_UNUSED(lineCount);

    // Update the freeze indicator immediately
    if (!m_trackedWorld) {
        return;
    }

    OutputView* outputView = m_trackedWorld->outputView();
    if (!outputView) {
        return;
    }

    if (outputView->isFrozen()) {
        int pendingLines = outputView->frozenLineCount();
        if (pendingLines > 0) {
            // There are lines waiting - show MORE (inverted style)
            m_freezeIndicator->setText("MORE");
            m_freezeIndicator->setStyleSheet(
                "QLabel { background-color: #000000; color: #FFFFFF; }");
        } else {
            // Frozen but at end of buffer - show PAUSE
            m_freezeIndicator->setText("PAUSE");
            m_freezeIndicator->setStyleSheet("");
        }
    } else {
        m_freezeIndicator->setText("");
        m_freezeIndicator->setStyleSheet("");
    }
}

void MainWindow::onConnectionStateChanged(bool connected)
{
    // Update connection indicator
    m_connectionIndicator->setText(connected ? "" : "CLOSED");
}
