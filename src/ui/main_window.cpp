#include "main_window.h"
#include "../automation/plugin.h" // For plugin callback constants
#include "../storage/database.h"
#include "../utils/app_paths.h"
#include "../storage/global_options.h"
#include "../text/line.h"
#include "../world/notepad_widget.h"
#include "../world/world_document.h"
#include "dialogs/alias_list_dialog.h"
#include "dialogs/ascii_art_dialog.h"
#include "dialogs/command_history_dialog.h"
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
#include "dialogs/debug_world_input_dialog.h"
#include "dialogs/map_comment_dialog.h"
#include "dialogs/map_dialog.h"
#include "dialogs/multiline_trigger_dialog.h"
#include "dialogs/plugin_dialog.h"
#include "dialogs/plugin_wizard.h"
#include "dialogs/progress_dialog.h"
#include "dialogs/quick_connect_dialog.h"
#include "dialogs/recall_search_dialog.h"
#include "dialogs/send_to_all_dialog.h"
#include "dialogs/text_attributes_dialog.h"
#include "dialogs/timer_list_dialog.h"
#include "dialogs/trigger_list_dialog.h"
#include "dialogs/world_properties_dialog.h"
#include "preferences/unified_preferences_dialog.h"
#include "logging.h"
#include "views/input_view.h"
#include "views/output_view.h"
#include "views/world_widget.h"
#include "widgets/activity_window.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
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
#include <QHBoxLayout>
#include <QFrame>
#include <QToolButton>
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

    // Create MDI area
    m_mdiArea = new QMdiArea(this);
    m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

#ifdef Q_OS_MACOS
    // On macOS, wrap MDI area in a container with minimized bar at bottom
    // since frameless MDI subwindows don't show a visible minimized representation
    QWidget* centralContainer = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_mdiArea, 1);

    // Create minimized bar container (hidden until windows are minimized)
    m_minimizedBarContainer = new QWidget(centralContainer);
    m_minimizedBarContainer->setFixedHeight(28);
    m_minimizedBarContainer->setStyleSheet(
        "QWidget { background: #2a2a2a; border-top: 1px solid #444; }");
    m_minimizedBarLayout = new QHBoxLayout(m_minimizedBarContainer);
    m_minimizedBarLayout->setContentsMargins(4, 2, 4, 2);
    m_minimizedBarLayout->setSpacing(4);
    m_minimizedBarLayout->addStretch();
    m_minimizedBarContainer->hide();

    centralLayout->addWidget(m_minimizedBarContainer);
    setCentralWidget(centralContainer);
#else
    setCentralWidget(m_mdiArea);
#endif

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
    // File Menu (matches original MUSHclient structure)
    m_fileMenu = menuBar()->addMenu("&File");

    m_newAction = m_fileMenu->addAction("&New World...");
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip("Create a new world connection");
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newWorld);

    m_openAction = m_fileMenu->addAction("&Open World...");
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip("Open an existing world file");
    connect(m_openAction, &QAction::triggered, this, qOverload<>(&MainWindow::openWorld));

    m_openStartupListAction = m_fileMenu->addAction("Open Worlds In &Startup List");
    m_openStartupListAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O));
    m_openStartupListAction->setStatusTip("Open all worlds in the startup list");
    connect(m_openStartupListAction, &QAction::triggered, this, &MainWindow::openStartupList);

    m_closeAction = m_fileMenu->addAction("&Close World");
    m_closeAction->setShortcut(QKeySequence::Close);
    m_closeAction->setStatusTip("Close the current world");
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::closeWorld);

    m_importXmlAction = m_fileMenu->addAction("&Import...");
    m_importXmlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_I));
    m_importXmlAction->setStatusTip("Import triggers, aliases, and other settings from XML");
    connect(m_importXmlAction, &QAction::triggered, this, &MainWindow::importXml);

    m_configurePluginsAction = m_fileMenu->addAction("Pl&ugins...");
    m_configurePluginsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_configurePluginsAction->setStatusTip("Manage plugins for the active world");
    connect(m_configurePluginsAction, &QAction::triggered, this, &MainWindow::configurePlugins);

    m_pluginWizardAction = m_fileMenu->addAction("Plugin &Wizard...");
    m_pluginWizardAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_P));
    m_pluginWizardAction->setStatusTip("Create a new plugin from world items");
    connect(m_pluginWizardAction, &QAction::triggered, this, &MainWindow::pluginWizard);

    m_saveAction = m_fileMenu->addAction("&Save World Details");
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip("Save the current world");
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveWorld);

    m_saveAsAction = m_fileMenu->addAction("Save World Details &As...");
    m_saveAsAction->setStatusTip("Save the current world with a new name");
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveWorldAs);

    m_saveSelectionAction = m_fileMenu->addAction("Save Se&lection...");
    m_saveSelectionAction->setStatusTip("Save selected text to a file");
    connect(m_saveSelectionAction, &QAction::triggered, this, &MainWindow::saveSelection);

    m_fileMenu->addSeparator();

    m_globalPreferencesAction = m_fileMenu->addAction("&Global Preferences...");
    m_globalPreferencesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G));
    m_globalPreferencesAction->setStatusTip("Configure global application settings");
    connect(m_globalPreferencesAction, &QAction::triggered, this, &MainWindow::globalPreferences);

    m_logSessionAction = m_fileMenu->addAction("&Log Session...");
    m_logSessionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_J));
    m_logSessionAction->setCheckable(true);
    m_logSessionAction->setStatusTip("Toggle session logging to file");
    connect(m_logSessionAction, &QAction::triggered, this, &MainWindow::toggleLogSession);

    m_reloadDefaultsAction = m_fileMenu->addAction("&Reload Defaults");
    m_reloadDefaultsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R));
    m_reloadDefaultsAction->setStatusTip("Reload default settings for the current world");
    connect(m_reloadDefaultsAction, &QAction::triggered, this, &MainWindow::reloadDefaults);

    m_worldPropertiesAction = m_fileMenu->addAction("World &Properties...");
    m_worldPropertiesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
    m_worldPropertiesAction->setStatusTip("Configure world settings");
    connect(m_worldPropertiesAction, &QAction::triggered, this, &MainWindow::worldProperties);

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

    // Edit Menu (matches original MUSHclient structure)
    m_editMenu = menuBar()->addMenu("&Edit");

    m_undoAction = m_editMenu->addAction("&Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip("Undo last action");
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);

    m_editMenu->addSeparator();

    m_cutAction = m_editMenu->addAction("Cu&t");
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setStatusTip("Cut selected text");
    connect(m_cutAction, &QAction::triggered, this, &MainWindow::cut);

    m_copyAction = m_editMenu->addAction("&Copy");
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip("Copy selected text");
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copy);

    m_copyAsHtmlAction = m_editMenu->addAction("Copy as HTML");
    m_copyAsHtmlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
    m_copyAsHtmlAction->setStatusTip("Copy selected text with colors and formatting as HTML");
    connect(m_copyAsHtmlAction, &QAction::triggered, this, &MainWindow::copyAsHtml);

    m_pasteAction = m_editMenu->addAction("&Paste");
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip("Paste text");
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::paste);

    m_editMenu->addSeparator();

    m_pasteToWorldAction = m_editMenu->addAction("Paste To &World...");
    m_pasteToWorldAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    m_pasteToWorldAction->setStatusTip("Paste clipboard text directly to the MUD");
    connect(m_pasteToWorldAction, &QAction::triggered, this, &MainWindow::pasteToWorld);

    m_recallLastWordAction = m_editMenu->addAction("Recall Last Word");
    m_recallLastWordAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backspace));
    m_recallLastWordAction->setStatusTip("Recall last word typed");
    connect(m_recallLastWordAction, &QAction::triggered, this, &MainWindow::recallLastWord);

    m_editMenu->addSeparator();

    m_selectAllAction = m_editMenu->addAction("Select &All");
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setStatusTip("Select all text");
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::selectAll);

    m_spellCheckAction = m_editMenu->addAction("Sp&ell Check");
    m_spellCheckAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    m_spellCheckAction->setStatusTip("Check spelling of text");
    connect(m_spellCheckAction, &QAction::triggered, this, &MainWindow::spellCheck);

    m_editMenu->addSeparator();

    m_generateNameAction = m_editMenu->addAction("&Generate Character Name...");
    m_generateNameAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_N));
    m_generateNameAction->setStatusTip("Generate a random fantasy character name");
    connect(m_generateNameAction, &QAction::triggered, this, &MainWindow::generateCharacterName);

    m_reloadNamesFileAction = m_editMenu->addAction("&Reload Names File...");
    m_reloadNamesFileAction->setStatusTip("Reload the character names file");
    connect(m_reloadNamesFileAction, &QAction::triggered, this, &MainWindow::reloadNamesFile);

    m_generateIdAction = m_editMenu->addAction("Generate Unique &ID...");
    m_generateIdAction->setStatusTip("Generate a unique identifier for plugins");
    connect(m_generateIdAction, &QAction::triggered, this, &MainWindow::generateUniqueID);

    m_editMenu->addSeparator();

    m_notepadAction = m_editMenu->addAction("&Notepad");
    m_notepadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));
    m_notepadAction->setStatusTip("Open the notepad window");
    connect(m_notepadAction, &QAction::triggered, this, &MainWindow::openNotepad);

    m_flipToNotepadAction = m_editMenu->addAction("&Flip To Notepad");
    m_flipToNotepadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Space));
    m_flipToNotepadAction->setStatusTip("Switch to notepad window");
    connect(m_flipToNotepadAction, &QAction::triggered, this, &MainWindow::flipToNotepad);

    m_editMenu->addSeparator();

    m_colourPickerAction = m_editMenu->addAction("Colour Picker");
    m_colourPickerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));
    m_colourPickerAction->setStatusTip("Open colour picker dialog");
    connect(m_colourPickerAction, &QAction::triggered, this, &MainWindow::colourPicker);

    m_debugPacketsAction = m_editMenu->addAction("Debug Packets");
    m_debugPacketsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F11));
    m_debugPacketsAction->setStatusTip("Debug incoming packets");
    connect(m_debugPacketsAction, &QAction::triggered, this, &MainWindow::debugPackets);

    m_editMenu->addSeparator();

    m_goToMatchingBraceAction = m_editMenu->addAction("Go To &Matching Brace");
    m_goToMatchingBraceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    m_goToMatchingBraceAction->setStatusTip("Jump to matching bracket, brace, or parenthesis");
    connect(m_goToMatchingBraceAction, &QAction::triggered, this, &MainWindow::goToMatchingBrace);

    m_selectToMatchingBraceAction = m_editMenu->addAction("Select To Matching &Brace");
    m_selectToMatchingBraceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    m_selectToMatchingBraceAction->setStatusTip(
        "Select text to matching bracket, brace, or parenthesis");
    connect(m_selectToMatchingBraceAction, &QAction::triggered, this,
            &MainWindow::selectToMatchingBrace);

    // Input Menu (matches original MUSHclient structure)
    m_inputMenu = menuBar()->addMenu("&Input");

    m_activateInputAreaAction = m_inputMenu->addAction("Activate &Input Area");
    m_activateInputAreaAction->setShortcut(QKeySequence(Qt::Key_Tab));
    m_activateInputAreaAction->setStatusTip("Set focus to the input field");
    m_activateInputAreaAction->setMenuRole(QAction::NoRole);
    connect(m_activateInputAreaAction, &QAction::triggered, this, &MainWindow::activateInputArea);

    m_inputMenu->addSeparator();

    m_nextCommandAction = m_inputMenu->addAction("&Next Command");
    m_nextCommandAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    m_nextCommandAction->setStatusTip("Recall next command from history");
    m_nextCommandAction->setMenuRole(QAction::NoRole);
    connect(m_nextCommandAction, &QAction::triggered, this, &MainWindow::nextCommand);

    m_previousCommandAction = m_inputMenu->addAction("&Previous Command");
    m_previousCommandAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    m_previousCommandAction->setStatusTip("Recall previous command from history");
    m_previousCommandAction->setMenuRole(QAction::NoRole);
    connect(m_previousCommandAction, &QAction::triggered, this, &MainWindow::previousCommand);

    m_repeatLastCommandAction = m_inputMenu->addAction("&Repeat Last Command");
    m_repeatLastCommandAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_repeatLastCommandAction->setStatusTip("Execute the most recent command again");
    m_repeatLastCommandAction->setMenuRole(QAction::NoRole);
    connect(m_repeatLastCommandAction, &QAction::triggered, this, &MainWindow::repeatLastCommand);

    QAction* quitWorldAction = m_inputMenu->addAction("&Quit From This World...");
    quitWorldAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_Q));
    quitWorldAction->setStatusTip("Close the current world");
    quitWorldAction->setMenuRole(QAction::NoRole);
    connect(quitWorldAction, &QAction::triggered, this, &MainWindow::closeWorld);

    m_inputMenu->addSeparator();

    m_commandHistoryAction = m_inputMenu->addAction("Command &History...");
    m_commandHistoryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    m_commandHistoryAction->setStatusTip("View and manage command history");
    m_commandHistoryAction->setMenuRole(QAction::NoRole);
    connect(m_commandHistoryAction, &QAction::triggered, this, &MainWindow::showCommandHistory);

    m_clearCommandHistoryAction = m_inputMenu->addAction("&Clear Command History...");
    m_clearCommandHistoryAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_D));
    m_clearCommandHistoryAction->setStatusTip("Clear all command history");
    m_clearCommandHistoryAction->setMenuRole(QAction::NoRole);
    connect(m_clearCommandHistoryAction, &QAction::triggered, this,
            &MainWindow::clearCommandHistory);

    m_discardQueueAction = m_inputMenu->addAction("&Discard 0 Queued Commands");
    m_discardQueueAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    m_discardQueueAction->setStatusTip("Clear all pending queued commands");
    m_discardQueueAction->setMenuRole(QAction::NoRole);
    m_discardQueueAction->setEnabled(false); // Disabled initially, updated by updateMenus()
    connect(m_discardQueueAction, &QAction::triggered, this, &MainWindow::discardQueuedCommands);

    m_inputMenu->addSeparator();

    // Auto Say toggle (also configurable via Game → Configure → Auto Say)
    QAction* autoSayAction = m_inputMenu->addAction("&Auto Say");
    autoSayAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_A));
    autoSayAction->setCheckable(true);
    autoSayAction->setStatusTip("Toggle auto-say mode");
    autoSayAction->setMenuRole(QAction::NoRole);
    connect(autoSayAction, &QAction::triggered, this, &MainWindow::toggleAutoSay);

    // Send File (also in Game menu)
    QAction* sendFileAction = m_inputMenu->addAction("&Send File...");
    sendFileAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_O));
    sendFileAction->setStatusTip("Send a text file to the MUD");
    sendFileAction->setMenuRole(QAction::NoRole);
    connect(sendFileAction, &QAction::triggered, this, &MainWindow::sendFile);

    m_globalChangeAction = m_inputMenu->addAction("Global Change...");
    m_globalChangeAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_G));
    m_globalChangeAction->setStatusTip("Search and replace text globally");
    m_globalChangeAction->setMenuRole(QAction::NoRole);
    connect(m_globalChangeAction, &QAction::triggered, this, &MainWindow::globalChange);

    m_keyNameAction = m_inputMenu->addAction("&Key Name...");
    m_keyNameAction->setStatusTip("Display the name of a key press");
    m_keyNameAction->setMenuRole(QAction::NoRole);
    connect(m_keyNameAction, &QAction::triggered, this, &MainWindow::showKeyName);

    // Connection Menu (matches original MUSHclient structure)
    m_connectionMenu = menuBar()->addMenu("Connecti&on");

    // Quick Connect first (also in File menu)
    QAction* quickConnectAction2 = m_connectionMenu->addAction("&Quick Connect...");
    quickConnectAction2->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_K));
    quickConnectAction2->setStatusTip("Quickly connect to a MUD server");
    connect(quickConnectAction2, &QAction::triggered, this, &MainWindow::quickConnect);

    m_connectionMenu->addSeparator();

    m_connectAction = m_connectionMenu->addAction("&Connect");
    m_connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    m_connectAction->setStatusTip("Connect to the MUD server");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::connectToMud);

    m_disconnectAction = m_connectionMenu->addAction("&Disconnect");
    m_disconnectAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_K));
    m_disconnectAction->setStatusTip("Disconnect from the MUD server");
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::disconnectFromMud);

    m_connectionMenu->addSeparator();

    m_autoConnectAction = m_connectionMenu->addAction("&Auto Connect");
    m_autoConnectAction->setCheckable(true);
    m_autoConnectAction->setStatusTip("Automatically connect when opening worlds");
    m_autoConnectAction->setMenuRole(QAction::NoRole);
    connect(m_autoConnectAction, &QAction::triggered, this, &MainWindow::toggleAutoConnect);

    m_reconnectOnDisconnectAction = m_connectionMenu->addAction("&Reconnect On Disconnect");
    m_reconnectOnDisconnectAction->setCheckable(true);
    m_reconnectOnDisconnectAction->setStatusTip("Automatically reconnect when disconnected");
    m_reconnectOnDisconnectAction->setMenuRole(QAction::NoRole);
    connect(m_reconnectOnDisconnectAction, &QAction::triggered, this,
            &MainWindow::toggleReconnectOnDisconnect);

    m_connectionMenu->addSeparator();

    m_connectToAllAction = m_connectionMenu->addAction("Connect to All Open &Worlds");
    m_connectToAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
    m_connectToAllAction->setStatusTip("Connect to all open but disconnected worlds");
    connect(m_connectToAllAction, &QAction::triggered, this, &MainWindow::connectToAllOpenWorlds);

    m_connectToStartupListAction = m_connectionMenu->addAction("Connect to Worlds &In Startup List");
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

    // Game Menu (matches original MUSHclient structure)
    m_gameMenu = menuBar()->addMenu("&Game");

    // Hidden Auto-Say action for state tracking (not in menu - config is in Configure submenu)
    m_autoSayAction = new QAction("Auto-Say", this);
    m_autoSayAction->setCheckable(true);
    m_autoSayAction->setChecked(false);
    connect(m_autoSayAction, &QAction::triggered, this, &MainWindow::toggleAutoSay);

    // Configuration submenu (first item in original)
    QMenu* configureMenu = new QMenu("C&onfigure", this);
    configureMenu->menuAction()->setMenuRole(QAction::NoRole); // Prevent macOS from hiding submenu
    configureMenu->menuAction()->setVisible(true);             // Explicitly set visible for macOS

    // All Configuration (opens unified preferences dialog)
    m_configureAllAction = configureMenu->addAction("&All Configuration...");
    m_configureAllAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
    m_configureAllAction->setStatusTip("Open world configuration dialog");
    m_configureAllAction->setMenuRole(QAction::NoRole);
    connect(m_configureAllAction, &QAction::triggered, this, &MainWindow::configureAll);

    configureMenu->addSeparator();

    // Connection settings (Alt+1 for address, Alt+2 for connecting - both go to Connection page)
    m_configureConnectionAction = configureMenu->addAction("MUD &Name/IP address...");
    m_configureConnectionAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_1));
    m_configureConnectionAction->setStatusTip("Configure MUD address and connection settings");
    m_configureConnectionAction->setMenuRole(QAction::NoRole);
    connect(m_configureConnectionAction, &QAction::triggered, this, &MainWindow::configureConnection);

    m_configureConnectingAction = configureMenu->addAction("C&onnecting...");
    m_configureConnectingAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_2));
    m_configureConnectingAction->setStatusTip("Configure connection name and password");
    m_configureConnectingAction->setMenuRole(QAction::NoRole);
    connect(m_configureConnectingAction, &QAction::triggered, this, &MainWindow::configureConnection);

    m_configureLoggingAction = configureMenu->addAction("&Logging...");
    m_configureLoggingAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_3));
    m_configureLoggingAction->setStatusTip("Configure session logging");
    m_configureLoggingAction->setMenuRole(QAction::NoRole);
    connect(m_configureLoggingAction, &QAction::triggered, this, &MainWindow::configureLogging);

    m_configureNotesAction = configureMenu->addAction("No&tes...");
    m_configureNotesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_4));
    m_configureNotesAction->setStatusTip("View and edit world notes");
    m_configureNotesAction->setMenuRole(QAction::NoRole);
    connect(m_configureNotesAction, &QAction::triggered, this, &MainWindow::configureInfo);

    configureMenu->addSeparator();

    // Appearance settings
    m_configureOutputAction = configureMenu->addAction("&Output...");
    m_configureOutputAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_5));
    m_configureOutputAction->setStatusTip("Configure output window appearance");
    m_configureOutputAction->setMenuRole(QAction::NoRole);
    connect(m_configureOutputAction, &QAction::triggered, this, &MainWindow::configureOutput);

    m_configureMxpAction = configureMenu->addAction("MXP / &Pueblo...");
    m_configureMxpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U));
    m_configureMxpAction->setStatusTip("Configure MXP and Pueblo protocol settings");
    m_configureMxpAction->setMenuRole(QAction::NoRole);
    connect(m_configureMxpAction, &QAction::triggered, this, &MainWindow::configureMxp);

    m_configureColoursAction = configureMenu->addAction("ANSI C&olours...");
    m_configureColoursAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_6));
    m_configureColoursAction->setStatusTip("Configure ANSI colours");
    m_configureColoursAction->setMenuRole(QAction::NoRole);
    connect(m_configureColoursAction, &QAction::triggered, this, &MainWindow::configureColours);

    m_configureCustomColoursAction = configureMenu->addAction("&Custom Colours...");
    m_configureCustomColoursAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_8));
    m_configureCustomColoursAction->setStatusTip("Configure custom colours");
    m_configureCustomColoursAction->setMenuRole(QAction::NoRole);
    connect(m_configureCustomColoursAction, &QAction::triggered, this, &MainWindow::configureColours);

    configureMenu->addSeparator();

    // Input settings
    m_configureCommandsAction = configureMenu->addAction("Co&mmands...");
    m_configureCommandsAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_0));
    m_configureCommandsAction->setStatusTip("Configure command input settings");
    m_configureCommandsAction->setMenuRole(QAction::NoRole);
    connect(m_configureCommandsAction, &QAction::triggered, this, &MainWindow::configureCommands);

    m_configureKeypadAction = configureMenu->addAction("&Keypad...");
    m_configureKeypadAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_1));
    m_configureKeypadAction->setStatusTip("Configure numeric keypad commands");
    m_configureKeypadAction->setMenuRole(QAction::NoRole);
    connect(m_configureKeypadAction, &QAction::triggered, this, &MainWindow::configureKeypad);

    m_configureMacrosAction = configureMenu->addAction("&Macros...");
    m_configureMacrosAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_2));
    m_configureMacrosAction->setStatusTip("Configure keyboard macros");
    m_configureMacrosAction->setMenuRole(QAction::NoRole);
    connect(m_configureMacrosAction, &QAction::triggered, this, &MainWindow::configureMacros);

    m_configureAutoSayAction = configureMenu->addAction("Auto &say...");
    m_configureAutoSayAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_3));
    m_configureAutoSayAction->setStatusTip("Configure auto-say settings");
    m_configureAutoSayAction->setMenuRole(QAction::NoRole);
    connect(m_configureAutoSayAction, &QAction::triggered, this, &MainWindow::configureAutoSay);

    configureMenu->addSeparator();

    // Paste/Send settings
    m_configurePasteAction = configureMenu->addAction("Pas&te to world...");
    m_configurePasteAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_4));
    m_configurePasteAction->setStatusTip("Configure paste to world settings");
    m_configurePasteAction->setMenuRole(QAction::NoRole);
    connect(m_configurePasteAction, &QAction::triggered, this, &MainWindow::configurePaste);

    m_configureSendFileAction = configureMenu->addAction("Sen&d file...");
    m_configureSendFileAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_5));
    m_configureSendFileAction->setStatusTip("Configure send file settings");
    m_configureSendFileAction->setMenuRole(QAction::NoRole);
    connect(m_configureSendFileAction, &QAction::triggered, this, &MainWindow::configurePaste);

    configureMenu->addSeparator();

    // Scripting settings
    m_configureScriptingAction = configureMenu->addAction("&Scripting...");
    m_configureScriptingAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_6));
    m_configureScriptingAction->setStatusTip("Configure script file settings");
    m_configureScriptingAction->setMenuRole(QAction::NoRole);
    connect(m_configureScriptingAction, &QAction::triggered, this, &MainWindow::configureScripting);

    m_configureVariablesAction = configureMenu->addAction("&Variables...");
    m_configureVariablesAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_7));
    m_configureVariablesAction->setStatusTip("View and edit script variables");
    m_configureVariablesAction->setMenuRole(QAction::NoRole);
    connect(m_configureVariablesAction, &QAction::triggered, this, &MainWindow::configureVariables);

    configureMenu->addSeparator();

    // Automation settings
    m_configureTimersAction = configureMenu->addAction("&Timers...");
    m_configureTimersAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_0));
    m_configureTimersAction->setStatusTip("Configure timers for the active world");
    m_configureTimersAction->setMenuRole(QAction::NoRole);
    connect(m_configureTimersAction, &QAction::triggered, this, &MainWindow::configureTimers);

    m_configureTriggersAction = configureMenu->addAction("Tri&ggers...");
    m_configureTriggersAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_8));
    m_configureTriggersAction->setStatusTip("Configure triggers for the active world");
    m_configureTriggersAction->setMenuRole(QAction::NoRole);
    connect(m_configureTriggersAction, &QAction::triggered, this, &MainWindow::configureTriggers);

    m_configureAliasesAction = configureMenu->addAction("&Aliases...");
    m_configureAliasesAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_9));
    m_configureAliasesAction->setStatusTip("Configure aliases for the active world");
    m_configureAliasesAction->setMenuRole(QAction::NoRole);
    connect(m_configureAliasesAction, &QAction::triggered, this, &MainWindow::configureAliases);

    configureMenu->addSeparator();

    m_configureInfoAction = configureMenu->addAction("&Info...");
    m_configureInfoAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_I));
    m_configureInfoAction->setStatusTip("View world information");
    m_configureInfoAction->setMenuRole(QAction::NoRole);
    connect(m_configureInfoAction, &QAction::triggered, this, &MainWindow::configureInfo);

    m_gameMenu->addMenu(configureMenu);

    m_wrapOutputAction = m_gameMenu->addAction("Wrap &Output");
    m_wrapOutputAction->setCheckable(true);
    m_wrapOutputAction->setChecked(true); // Default to enabled
    m_wrapOutputAction->setStatusTip("Toggle line wrapping in output window");
    m_wrapOutputAction->setMenuRole(QAction::NoRole);
    connect(m_wrapOutputAction, &QAction::triggered, this, &MainWindow::toggleWrapOutput);

    m_gameMenu->addSeparator();

    m_testTriggerAction = m_gameMenu->addAction("Test &Trigger...");
    m_testTriggerAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_F12));
    m_testTriggerAction->setStatusTip("Test triggers with simulated input");
    m_testTriggerAction->setMenuRole(QAction::NoRole);
    connect(m_testTriggerAction, &QAction::triggered, this, &MainWindow::testTrigger);

    m_gameMenu->addSeparator();

    m_minimizeProgramAction = m_gameMenu->addAction("Minimize &Program");
    m_minimizeProgramAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    m_minimizeProgramAction->setStatusTip("Minimize to system tray");
    m_minimizeProgramAction->setMenuRole(QAction::NoRole);
    connect(m_minimizeProgramAction, &QAction::triggered, this, &MainWindow::minimizeToTray);

    m_gameMenu->addSeparator();

    m_immediateScriptAction = m_gameMenu->addAction("&Immediate...");
    m_immediateScriptAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_immediateScriptAction->setStatusTip("Execute Lua script immediately");
    m_immediateScriptAction->setMenuRole(QAction::NoRole);
    connect(m_immediateScriptAction, &QAction::triggered, this, &MainWindow::immediateScript);

    m_editScriptFileAction = m_gameMenu->addAction("&Edit Script File...");
    m_editScriptFileAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_H));
    m_editScriptFileAction->setStatusTip("Open script file in external editor");
    m_editScriptFileAction->setMenuRole(QAction::NoRole);
    connect(m_editScriptFileAction, &QAction::triggered, this, &MainWindow::editScriptFile);

    m_reloadScriptFileAction = m_gameMenu->addAction("&Reload Script File");
    m_reloadScriptFileAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_R));
    m_reloadScriptFileAction->setStatusTip("Reload the script file for the active world");
    m_reloadScriptFileAction->setMenuRole(QAction::NoRole);
    connect(m_reloadScriptFileAction, &QAction::triggered, this, &MainWindow::reloadScriptFile);

    m_traceAction = m_gameMenu->addAction("Tr&ace");
    m_traceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T));
    m_traceAction->setCheckable(true);
    m_traceAction->setChecked(false);
    m_traceAction->setStatusTip("Toggle script tracing/debugging mode");
    m_traceAction->setMenuRole(QAction::NoRole);
    connect(m_traceAction, &QAction::triggered, this, &MainWindow::toggleTrace);

    m_gameMenu->addSeparator();

    m_resetAllTimersAction = m_gameMenu->addAction("Reset All Ti&mers");
    m_resetAllTimersAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_T));
    m_resetAllTimersAction->setStatusTip("Reset all timers to their starting values");
    m_resetAllTimersAction->setMenuRole(QAction::NoRole);
    connect(m_resetAllTimersAction, &QAction::triggered, this, &MainWindow::resetAllTimers);

    m_gameMenu->addSeparator();

    m_sendToAllWorldsAction = m_gameMenu->addAction("&Send To All Worlds...");
    m_sendToAllWorldsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    m_sendToAllWorldsAction->setStatusTip("Send text to all open worlds");
    m_sendToAllWorldsAction->setMenuRole(QAction::NoRole);
    connect(m_sendToAllWorldsAction, &QAction::triggered, this, &MainWindow::sendToAllWorlds);

    m_mapperAction = m_gameMenu->addAction("&Mapper");
    m_mapperAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M));
    m_mapperAction->setStatusTip("Open the mapper window");
    m_mapperAction->setMenuRole(QAction::NoRole);
    connect(m_mapperAction, &QAction::triggered, this, &MainWindow::showMapper);

    m_doMapperSpecialAction = m_gameMenu->addAction("&Do Mapper Special...");
    m_doMapperSpecialAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_D));
    m_doMapperSpecialAction->setStatusTip("Execute mapper special action");
    m_doMapperSpecialAction->setMenuRole(QAction::NoRole);
    connect(m_doMapperSpecialAction, &QAction::triggered, this, &MainWindow::doMapperSpecial);

    m_addMapperCommentAction = m_gameMenu->addAction("Add Mapper &Comment...");
    m_addMapperCommentAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_D));
    m_addMapperCommentAction->setStatusTip("Add a comment to the current mapper room");
    m_addMapperCommentAction->setMenuRole(QAction::NoRole);
    connect(m_addMapperCommentAction, &QAction::triggered, this, &MainWindow::addMapperComment);

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

    m_lineUpAction = m_displayMenu->addAction("Line Up");
    m_lineUpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Up));
    m_lineUpAction->setStatusTip("Scroll up one line");
    connect(m_lineUpAction, &QAction::triggered, this, &MainWindow::scrollLineUp);

    m_lineDownAction = m_displayMenu->addAction("Line Down");
    m_lineDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Down));
    m_lineDownAction->setStatusTip("Scroll down one line");
    connect(m_lineDownAction, &QAction::triggered, this, &MainWindow::scrollLineDown);

    m_displayMenu->addSeparator();

    m_activityListAction = m_displayMenu->addAction("&Activity List...");
    m_activityListAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_L));
    m_activityListAction->setStatusTip("Show list of worlds with activity");
    connect(m_activityListAction, &QAction::triggered, this, &MainWindow::activityList);

    m_freezeOutputAction = m_displayMenu->addAction("Pause &Output");
    m_freezeOutputAction->setCheckable(true);
    m_freezeOutputAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space));
    m_freezeOutputAction->setStatusTip("Pause output scrolling");
    connect(m_freezeOutputAction, &QAction::triggered, this, &MainWindow::toggleFreezeOutput);

    m_displayMenu->addSeparator();

    m_findAction = m_displayMenu->addAction("&Find...");
    m_findAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    m_findAction->setStatusTip("Search for text in output");
    connect(m_findAction, &QAction::triggered, this, &MainWindow::find);

    m_findNextAction = m_displayMenu->addAction("Find &Again");
    m_findNextAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_F));
    m_findNextAction->setStatusTip("Find next occurrence of search text");
    connect(m_findNextAction, &QAction::triggered, this, &MainWindow::findNext);

    m_findForwardAction = m_displayMenu->addAction("Find Again &Forwards");
    m_findForwardAction->setShortcut(QKeySequence(Qt::Key_F3));
    m_findForwardAction->setStatusTip("Find next occurrence (forward)");
    connect(m_findForwardAction, &QAction::triggered, this, &MainWindow::findForward);

    m_findBackwardAction = m_displayMenu->addAction("Find Again &Backwards");
    m_findBackwardAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3));
    m_findBackwardAction->setStatusTip("Find previous occurrence (backward)");
    connect(m_findBackwardAction, &QAction::triggered, this, &MainWindow::findBackward);

    m_recallTextAction = m_displayMenu->addAction("&Recall Text...");
    m_recallTextAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    m_recallTextAction->setStatusTip("Search and recall buffer contents");
    connect(m_recallTextAction, &QAction::triggered, this, &MainWindow::recallText);

    m_goToLineAction = m_displayMenu->addAction("Go to &Line...");
    m_goToLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
    m_goToLineAction->setStatusTip("Navigate to a specific line in output");
    connect(m_goToLineAction, &QAction::triggered, this, &MainWindow::goToLine);

    m_displayMenu->addSeparator();

    m_goToUrlAction = m_displayMenu->addAction("Go to &URL...");
    m_goToUrlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_J));
    m_goToUrlAction->setStatusTip("Open selected text as URL in browser");
    connect(m_goToUrlAction, &QAction::triggered, this, &MainWindow::goToUrl);

    m_sendMailToAction = m_displayMenu->addAction("Send &Mail To...");
    m_sendMailToAction->setStatusTip("Send email to selected address");
    connect(m_sendMailToAction, &QAction::triggered, this, &MainWindow::sendMailTo);

    m_displayMenu->addSeparator();

    m_clearOutputAction = m_displayMenu->addAction("&Clear Output Buffer...");
    m_clearOutputAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_C));
    m_clearOutputAction->setStatusTip("Clear all output text");
    connect(m_clearOutputAction, &QAction::triggered, this, &MainWindow::clearOutput);

    m_stopSoundAction = m_displayMenu->addAction("Stop Sound Playing");
    m_stopSoundAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B));
    m_stopSoundAction->setStatusTip("Stop any currently playing sound");
    connect(m_stopSoundAction, &QAction::triggered, this, &MainWindow::stopSound);

    m_displayMenu->addSeparator();

    m_bookmarkSelectionAction = m_displayMenu->addAction("&Bookmark Selection");
    m_bookmarkSelectionAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_B));
    m_bookmarkSelectionAction->setStatusTip("Toggle bookmark on current line");
    connect(m_bookmarkSelectionAction, &QAction::triggered, this, &MainWindow::bookmarkSelection);

    m_goToBookmarkAction = m_displayMenu->addAction("&Go To Bookmark");
    m_goToBookmarkAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    m_goToBookmarkAction->setStatusTip("Jump to next bookmarked line");
    connect(m_goToBookmarkAction, &QAction::triggered, this, &MainWindow::goToBookmark);

    m_highlightPhraseAction = m_displayMenu->addAction("&Highlight Word...");
    m_highlightPhraseAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_H));
    m_highlightPhraseAction->setStatusTip("Highlight matching words in output");
    connect(m_highlightPhraseAction, &QAction::triggered, this, &MainWindow::highlightPhrase);

    m_multilineTriggerAction = m_displayMenu->addAction("&Multi-line Trigger...");
    m_multilineTriggerAction->setStatusTip("Configure multi-line trigger patterns");
    connect(m_multilineTriggerAction, &QAction::triggered, this, &MainWindow::multilineTrigger);

    m_textAttributesAction = m_displayMenu->addAction("&Text Attributes...");
    m_textAttributesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    m_textAttributesAction->setStatusTip("Configure text formatting attributes");
    connect(m_textAttributesAction, &QAction::triggered, this, &MainWindow::textAttributes);

    m_displayMenu->addSeparator();

    m_commandEchoAction = m_displayMenu->addAction("No Command &Echo");
    m_commandEchoAction->setCheckable(true);
    m_commandEchoAction->setChecked(false); // Default: echo is enabled (not checked = echo on)
    m_commandEchoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E));
    m_commandEchoAction->setStatusTip("Toggle command echo in output");
    connect(m_commandEchoAction, &QAction::triggered, this, &MainWindow::toggleCommandEcho);

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

    // View Menu (matches original MUSHclient structure)
    m_viewMenu = menuBar()->addMenu("&View");

    // Toolbar visibility toggles (will be connected after toolbars are created)
    m_mainToolBarAction = m_viewMenu->addAction("&Toolbar");
    m_mainToolBarAction->setCheckable(true);
    m_mainToolBarAction->setChecked(true);
    m_mainToolBarAction->setStatusTip("Show or hide the main toolbar");

    m_gameToolBarAction = m_viewMenu->addAction("&World Toolbar");
    m_gameToolBarAction->setCheckable(true);
    m_gameToolBarAction->setChecked(true);
    m_gameToolBarAction->setStatusTip("Show or hide the world/game toolbar");

    m_activityToolBarAction = m_viewMenu->addAction("&Activity Toolbar");
    m_activityToolBarAction->setCheckable(true);
    m_activityToolBarAction->setChecked(true);
    m_activityToolBarAction->setStatusTip("Show or hide the activity toolbar");

    m_statusBarAction = m_viewMenu->addAction("&Status Bar");
    m_statusBarAction->setCheckable(true);
    m_statusBarAction->setChecked(true);
    m_statusBarAction->setStatusTip("Show or hide the status bar");
    connect(m_statusBarAction, &QAction::triggered, this, &MainWindow::toggleStatusBar);

    m_infoBarAction = m_viewMenu->addAction("&Info Bar");
    m_infoBarAction->setCheckable(true);
    m_infoBarAction->setChecked(false); // Hidden by default
    m_infoBarAction->setStatusTip("Show or hide the info bar");

    m_viewMenu->addSeparator();

    m_resetToolbarsAction = m_viewMenu->addAction("&Reset Toolbar Locations");
    m_resetToolbarsAction->setStatusTip("Reset all toolbars to their default positions");
    connect(m_resetToolbarsAction, &QAction::triggered, this, &MainWindow::resetToolbars);

    m_alwaysOnTopAction = m_viewMenu->addAction("Always &On Top");
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setChecked(false);
    m_alwaysOnTopAction->setStatusTip("Keep window above all other windows");
    connect(m_alwaysOnTopAction, &QAction::triggered, this, &MainWindow::toggleAlwaysOnTop);

    m_fullScreenAction = m_viewMenu->addAction("&Full Screen Mode");
    m_fullScreenAction->setCheckable(true);
    m_fullScreenAction->setChecked(false);
    m_fullScreenAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F));
    m_fullScreenAction->setStatusTip("Toggle full screen mode");
    connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    // Window Menu (matches original MUSHclient structure)
    m_windowMenu = menuBar()->addMenu("&Window");

    m_newWindowAction = m_windowMenu->addAction("&New Window");
    m_newWindowAction->setStatusTip("Open another window for the active document");
    connect(m_newWindowAction, &QAction::triggered, this, &MainWindow::newWindow);

    m_cascadeAction = m_windowMenu->addAction("&Cascade");
    m_cascadeAction->setStatusTip("Cascade all windows");
    connect(m_cascadeAction, &QAction::triggered, m_mdiArea, &QMdiArea::cascadeSubWindows);

    m_tileHorizontallyAction = m_windowMenu->addAction("Tile &Horizontally");
    m_tileHorizontallyAction->setStatusTip("Tile all windows horizontally");
    connect(m_tileHorizontallyAction, &QAction::triggered, this, &MainWindow::tileHorizontally);

    m_tileVerticallyAction = m_windowMenu->addAction("Tile &Vertically");
    m_tileVerticallyAction->setStatusTip("Tile all windows vertically");
    connect(m_tileVerticallyAction, &QAction::triggered, this, &MainWindow::tileVertically);

    m_arrangeIconsAction = m_windowMenu->addAction("&Arrange Icons");
    m_arrangeIconsAction->setStatusTip("Arrange minimized windows");
    connect(m_arrangeIconsAction, &QAction::triggered, this, &MainWindow::arrangeIcons);

    m_closeAllNotepadAction = m_windowMenu->addAction("Close A&ll Notepad Windows");
    m_closeAllNotepadAction->setStatusTip("Close all notepad windows");
    connect(m_closeAllNotepadAction, &QAction::triggered, this, &MainWindow::closeAllNotepadWindows);

    // We'll manually populate the window menu in Qt 6
    // (Qt 5's QMdiArea::setWindowMenu is not available in Qt 6)
    // Dynamic window list is added after a separator by updateWindowMenu
    connect(m_windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    // Help Menu (matches original MUSHclient structure)
    m_helpMenu = menuBar()->addMenu("&Help");

    m_helpAction = m_helpMenu->addAction("&Contents");
    m_helpAction->setShortcut(QKeySequence::HelpContents);
    m_helpAction->setStatusTip("Show help contents");
    connect(m_helpAction, &QAction::triggered, this, &MainWindow::showHelp);

    m_helpMenu->addSeparator();

    m_bugReportsAction = m_helpMenu->addAction("&Bug Reports...");
    m_bugReportsAction->setStatusTip("Report bugs or suggest features on GitHub");
    connect(m_bugReportsAction, &QAction::triggered, this, &MainWindow::openBugReports);

    m_documentationAction = m_helpMenu->addAction("&Documentation Web Page...");
    m_documentationAction->setStatusTip("Open the Mushkin documentation wiki");
    connect(m_documentationAction, &QAction::triggered, this, &MainWindow::openDocumentation);

    m_webPageAction = m_helpMenu->addAction("Mushkin &Web Page...");
    m_webPageAction->setStatusTip("Open the Mushkin GitHub repository");
    connect(m_webPageAction, &QAction::triggered, this, &MainWindow::openWebPage);

    m_helpMenu->addSeparator();

    m_aboutAction = m_helpMenu->addAction("&About");
    m_aboutAction->setStatusTip("About Mushkin");
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::about);

    // Reorder menus to match original MUSHclient:
    // File, Edit, View, Connection, Input, Display, Game, Window, Help
    // (Convert menu is removed - it's notepad-specific in original)
    menuBar()->clear();
    menuBar()->addMenu(m_fileMenu);
    menuBar()->addMenu(m_editMenu);
    menuBar()->addMenu(m_viewMenu);
    menuBar()->addMenu(m_connectionMenu);
    menuBar()->addMenu(m_inputMenu);
    menuBar()->addMenu(m_displayMenu);
    menuBar()->addMenu(m_gameMenu);
    menuBar()->addMenu(m_windowMenu);
    menuBar()->addMenu(m_helpMenu);
    // Note: m_convertMenu is intentionally not added - it's notepad-specific
}

void MainWindow::createToolBars()
{
    // === Main Toolbar - File/Edit operations (matches original MUSHclient) ===
    m_mainToolBar = addToolBar("Toolbar");
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setMovable(true);

    // File operations (icons set by updateToolbarIcons based on theme)
    m_mainToolBar->addAction(m_newAction);

    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_saveAction);
    // Print World button (stub - not yet implemented)
    // m_mainToolBar->addAction(m_printWorldAction);

    m_mainToolBar->addSeparator();

    // Notepad
    m_mainToolBar->addAction(m_notepadAction);

    m_mainToolBar->addSeparator();

    // Edit actions
    m_mainToolBar->addAction(m_cutAction);
    m_mainToolBar->addAction(m_copyAction);
    m_mainToolBar->addAction(m_pasteAction);

    m_mainToolBar->addSeparator();

    // About and Help
    m_mainToolBar->addAction(m_aboutAction);
    m_mainToolBar->addAction(m_helpAction);

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
    // === World Toolbar - World configuration controls (matches original MUSHclient) ===
    m_gameToolBar = addToolBar("World Toolbar");
    m_gameToolBar->setObjectName("GameToolBar");
    m_gameToolBar->setMovable(true);

    // Wrap Lines toggle and Log Session toggle
    m_gameToolBar->addAction(m_wrapOutputAction);
    m_gameToolBar->addAction(m_logSessionAction);

    m_gameToolBar->addSeparator();

    // Connection controls
    m_gameToolBar->addAction(m_connectAction);
    m_gameToolBar->addAction(m_disconnectAction);

    m_gameToolBar->addSeparator();

    // World configuration buttons
    m_gameToolBar->addAction(m_configureAllAction);
    m_gameToolBar->addAction(m_configureTriggersAction);
    m_gameToolBar->addAction(m_configureAliasesAction);
    m_gameToolBar->addAction(m_configureTimersAction);
    m_gameToolBar->addAction(m_configureOutputAction);
    m_gameToolBar->addAction(m_configureCommandsAction);
    m_gameToolBar->addAction(m_configureScriptingAction);
    m_gameToolBar->addAction(m_configureNotesAction);
    m_gameToolBar->addAction(m_configureVariablesAction);

    m_gameToolBar->addSeparator();

    // Timer and script controls
    m_gameToolBar->addAction(m_resetAllTimersAction);
    m_gameToolBar->addAction(m_reloadScriptFileAction);

    m_gameToolBar->addSeparator();

    // Toggle actions
    m_gameToolBar->addAction(m_autoSayAction);
    m_gameToolBar->addAction(m_freezeOutputAction);

    m_gameToolBar->addSeparator();

    // Find controls
    m_gameToolBar->addAction(m_findAction);
    m_gameToolBar->addAction(m_findForwardAction);
    m_gameToolBar->addAction(m_findBackwardAction);

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
    // Main toolbar icons
    m_newAction->setIcon(loadThemedIcon("file-plus"));
    m_openAction->setIcon(loadThemedIcon("folder-open"));
    m_saveAction->setIcon(loadThemedIcon("device-floppy"));
    m_notepadAction->setIcon(loadThemedIcon("notebook"));
    m_cutAction->setIcon(loadThemedIcon("scissors"));
    m_copyAction->setIcon(loadThemedIcon("copy"));
    m_pasteAction->setIcon(loadThemedIcon("clipboard"));
    m_aboutAction->setIcon(loadThemedIcon("info-circle"));
    m_helpAction->setIcon(loadThemedIcon("help-circle"));

    // World toolbar icons
    m_wrapOutputAction->setIcon(loadThemedIcon("text-wrap"));
    m_logSessionAction->setIcon(loadThemedIcon("file-text"));
    m_connectAction->setIcon(loadThemedIcon("plug-connected"));
    m_disconnectAction->setIcon(loadThemedIcon("plug-x"));
    m_configureAllAction->setIcon(loadThemedIcon("settings"));
    m_configureTriggersAction->setIcon(loadThemedIcon("bolt"));
    m_configureAliasesAction->setIcon(loadThemedIcon("at"));
    m_configureTimersAction->setIcon(loadThemedIcon("clock"));
    m_configureOutputAction->setIcon(loadThemedIcon("terminal"));
    m_configureCommandsAction->setIcon(loadThemedIcon("command"));
    m_configureScriptingAction->setIcon(loadThemedIcon("code"));
    m_configureNotesAction->setIcon(loadThemedIcon("notes"));
    m_configureVariablesAction->setIcon(loadThemedIcon("braces"));
    m_resetAllTimersAction->setIcon(loadThemedIcon("refresh"));
    m_reloadScriptFileAction->setIcon(loadThemedIcon("refresh"));
    m_autoSayAction->setIcon(loadThemedIcon("message"));
    m_freezeOutputAction->setIcon(loadThemedIcon("player-pause"));
    m_findAction->setIcon(loadThemedIcon("search"));
    m_findForwardAction->setIcon(loadThemedIcon("arrow-right"));
    m_findBackwardAction->setIcon(loadThemedIcon("arrow-left"));
}

void MainWindow::createStatusBar()
{
    // Create permanent status indicators (right side of status bar)
    // Order matches original MUSHclient: Freeze | World Name | Time | Lines | Log

    // Freeze indicator - shows CLOSED/PAUSE/MORE
    m_freezeIndicator = new QLabel(this);
    m_freezeIndicator->setMinimumWidth(50);
    m_freezeIndicator->setAlignment(Qt::AlignCenter);
    m_freezeIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_freezeIndicator->setToolTip("Double-click to toggle freeze");
    m_freezeIndicator->installEventFilter(this);
    statusBar()->addPermanentWidget(m_freezeIndicator);

    // World name indicator
    m_worldNameIndicator = new QLabel(this);
    m_worldNameIndicator->setMinimumWidth(120);
    m_worldNameIndicator->setAlignment(Qt::AlignCenter);
    m_worldNameIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_worldNameIndicator->setToolTip("Click to switch worlds");
    m_worldNameIndicator->installEventFilter(this);
    statusBar()->addPermanentWidget(m_worldNameIndicator);

    // Time indicator - shows connected time (H:MM:SS)
    m_timeIndicator = new QLabel(this);
    m_timeIndicator->setMinimumWidth(70);
    m_timeIndicator->setAlignment(Qt::AlignCenter);
    m_timeIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_timeIndicator->setToolTip("Double-click to reset connected time");
    m_timeIndicator->installEventFilter(this);
    statusBar()->addPermanentWidget(m_timeIndicator);

    // Lines indicator - shows line count
    m_linesIndicator = new QLabel(this);
    m_linesIndicator->setMinimumWidth(80);
    m_linesIndicator->setAlignment(Qt::AlignCenter);
    m_linesIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_linesIndicator->setToolTip("Double-click to go to line");
    m_linesIndicator->installEventFilter(this);
    statusBar()->addPermanentWidget(m_linesIndicator);

    // Log indicator - shows LOG when logging is active
    m_logIndicator = new QLabel(this);
    m_logIndicator->setMinimumWidth(40);
    m_logIndicator->setAlignment(Qt::AlignCenter);
    m_logIndicator->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_logIndicator->setToolTip("Logging status");
    statusBar()->addPermanentWidget(m_logIndicator);

    // Timer for updating the connected time indicator
    m_statusBarTimer = new QTimer(this);
    connect(m_statusBarTimer, &QTimer::timeout, this, &MainWindow::updateTimeIndicator);
    m_statusBarTimer->start(1000);

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

    // Restore status bar visibility
    bool statusBarVisible = settings.value("mainWindow/statusBar", true).toBool();
    m_statusBarAction->setChecked(statusBarVisible);
    statusBar()->setVisible(statusBarVisible);
}

void MainWindow::writeSettings()
{
    QSettings settings("Gammon", "MUSHclient");

    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/state", saveState());
    settings.setValue("mainWindow/statusBar", m_statusBarAction->isChecked());
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

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // Handle double-clicks on status bar indicators
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (watched == m_freezeIndicator) {
            onFreezeIndicatorClicked();
            return true;
        } else if (watched == m_timeIndicator) {
            onTimeIndicatorClicked();
            return true;
        } else if (watched == m_linesIndicator) {
            onLinesIndicatorClicked();
            return true;
        }
    }
    // Handle single clicks on world name indicator
    else if (event->type() == QEvent::MouseButtonPress) {
        if (watched == m_worldNameIndicator) {
            onWorldNameIndicatorClicked();
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
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

    // Update Wrap Output checked state from document's m_wrap setting
    if (hasActiveWorld) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
        if (worldWidget && worldWidget->document()) {
            m_wrapOutputAction->setChecked(worldWidget->document()->m_wrap != 0);
        }
    }

    // Configure Triggers/Aliases/Timers are always enabled (matches original MUSHclient)
    // This also fixes the macOS native menu bar bug where disabled items stay disabled

    // Input menu actions - require an active world
    m_activateInputAreaAction->setEnabled(hasActiveWorld);
    m_previousCommandAction->setEnabled(hasActiveWorld);
    m_nextCommandAction->setEnabled(hasActiveWorld);
    m_repeatLastCommandAction->setEnabled(hasActiveWorld);
    m_clearCommandHistoryAction->setEnabled(hasActiveWorld);
    m_commandHistoryAction->setEnabled(hasActiveWorld);

    // Discard Queued Commands - show count and disable when empty
    int queuedCount = 0;
    if (currentDoc) {
        queuedCount = currentDoc->GetCommandQueue().size();
    }
    m_discardQueueAction->setText(tr("&Discard %1 Queued Commands").arg(queuedCount));
    m_discardQueueAction->setEnabled(hasActiveWorld && queuedCount > 0);

    // Window menu actions
    bool hasWorlds = !m_mdiArea->subWindowList().isEmpty();
    m_cascadeAction->setEnabled(hasWorlds);
    m_tileHorizontallyAction->setEnabled(hasWorlds);
    m_tileVerticallyAction->setEnabled(hasWorlds);
    m_arrangeIconsAction->setEnabled(hasWorlds);
    m_newWindowAction->setEnabled(hasWorlds);
    m_closeAllNotepadAction->setEnabled(hasWorlds);

    // Update status bar indicators
    updateStatusIndicators();
}

void MainWindow::updateWindowMenu()
{
    // This is called when the Window menu is about to show
    // Manually populate with list of open worlds (Qt 6 doesn't have setWindowMenu)

    // Remove any dynamically added actions (world list and separator)
    // Keep the fixed menu items (Cascade, Tile, etc.)
    QList<QAction*> actions = m_windowMenu->actions();
    for (QAction* action : actions) {
        if (action->data().toString() == "world_window" ||
            action->data().toString() == "window_separator") {
            m_windowMenu->removeAction(action);
            delete action;
        }
    }

    // Add current subwindows to menu
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    if (!windows.isEmpty()) {
        // Add separator before window list
        QAction* separator = m_windowMenu->addSeparator();
        separator->setData("window_separator");

        for (int i = 0; i < windows.size(); ++i) {
            QMdiSubWindow* window = windows[i];
            QString text = QString("&%1 %2").arg(i + 1).arg(window->windowTitle());

            // Mark minimized windows in the list
            if (window->isMinimized()) {
                text += tr(" [Minimized]");
            }

            QAction* action = m_windowMenu->addAction(text);
            action->setData("world_window");
            action->setCheckable(true);
            action->setChecked(window == m_mdiArea->activeSubWindow());

            connect(action, &QAction::triggered, [this, window]() {
                // Restore if minimized, then activate
                if (window->isMinimized()) {
                    window->showNormal();
                }
                m_mdiArea->setActiveSubWindow(window);
            });
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
                trimmedPath = AppPaths::getAppDirectory() + "/" + trimmedPath;
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

#ifdef Q_OS_MACOS
    // On macOS, use frameless subwindow - WorldWidget has its own custom title bar
    subWindow->setWindowFlags(Qt::FramelessWindowHint);
    // Connect to window state changes to update minimized bar
    connect(subWindow, &QMdiSubWindow::windowStateChanged, this, &MainWindow::onSubWindowStateChanged);
    // Connect to window state changes to update WorldWidget frame (hide border when maximized)
    connect(subWindow, &QMdiSubWindow::windowStateChanged, worldWidget,
            [worldWidget](Qt::WindowStates, Qt::WindowStates newState) {
                worldWidget->updateFrameForWindowState(newState);
            });
#endif

    // Connect window title changes
    connect(worldWidget, &WorldWidget::windowTitleChanged, subWindow,
            &QMdiSubWindow::setWindowTitle);

    // Connect connection state changes to update menus
    connect(worldWidget, &WorldWidget::connectedChanged, this, &MainWindow::updateMenus);

    // Connect notepad creation requests to create MDI subwindows
    connect(worldWidget, &WorldWidget::notepadRequested, this, &MainWindow::createNotepadWindow);

    // Connect to subwindow's destroyed signal to save geometry (newWorld)
    connect(subWindow, &QObject::destroyed, [this, worldWidget, subWindow]() {
#ifdef Q_OS_MACOS
        // Remove from minimized bars if it exists
        if (m_minimizedBars.contains(subWindow)) {
            QWidget* bar = m_minimizedBars.take(subWindow);
            m_minimizedBarLayout->removeWidget(bar);
            delete bar;
            updateMinimizedBar();
        }
#endif
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

#ifdef Q_OS_MACOS
    // On macOS, use frameless subwindow - WorldWidget has its own custom title bar
    subWindow->setWindowFlags(Qt::FramelessWindowHint);
    // Connect to window state changes to update minimized bar
    connect(subWindow, &QMdiSubWindow::windowStateChanged, this, &MainWindow::onSubWindowStateChanged);
    // Connect to window state changes to update WorldWidget frame (hide border when maximized)
    connect(subWindow, &QMdiSubWindow::windowStateChanged, worldWidget,
            [worldWidget](Qt::WindowStates, Qt::WindowStates newState) {
                worldWidget->updateFrameForWindowState(newState);
            });
#endif

    // Connect window title changes
    connect(worldWidget, &WorldWidget::windowTitleChanged, subWindow,
            &QMdiSubWindow::setWindowTitle);

    // Connect connection state changes to update menus
    connect(worldWidget, &WorldWidget::connectedChanged, this, &MainWindow::updateMenus);

    // Connect notepad creation requests to create MDI subwindows
    connect(worldWidget, &WorldWidget::notepadRequested, this, &MainWindow::createNotepadWindow);

    // Connect to subwindow's destroyed signal to save geometry
    // This ensures we save before the window is deleted
    connect(subWindow, &QObject::destroyed, [this, worldWidget, subWindow]() {
#ifdef Q_OS_MACOS
        // Remove from minimized bars if it exists
        if (m_minimizedBars.contains(subWindow)) {
            QWidget* bar = m_minimizedBars.take(subWindow);
            m_minimizedBarLayout->removeWidget(bar);
            delete bar;
            updateMinimizedBar();
        }
#endif
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

void MainWindow::openStartupList()
{
    // Open all worlds in the startup list
    // TODO: Implement startup list functionality
    QMessageBox::information(this, "Open Startup List",
                             "Opening worlds from startup list is not yet implemented.");
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

    // Open Unified Preferences dialog to Connection page
    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Connection, this);
    dialog.exec();
}

void MainWindow::globalPreferences()
{
    // TODO: Implement global preferences dialog
    QMessageBox::information(this, "Global Preferences",
                             "Global preferences dialog is not yet implemented.");
}

void MainWindow::reloadDefaults()
{
    // TODO: Implement reload defaults functionality
    QMessageBox::information(this, "Reload Defaults",
                             "Reload defaults is not yet implemented.");
}

void MainWindow::toggleLogSession()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        if (result != 0) {
            QMessageBox::warning(this, "Log Error", "Failed to close log file.");
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
void MainWindow::undo()
{
    // Get focus widget and try to undo
    QWidget* focusWidget = QApplication::focusWidget();
    if (auto* textEdit = qobject_cast<QTextEdit*>(focusWidget)) {
        textEdit->undo();
    } else if (auto* lineEdit = qobject_cast<QLineEdit*>(focusWidget)) {
        lineEdit->undo();
    }
}

void MainWindow::cut()
{
    // Get focus widget and try to cut
    QWidget* focusWidget = QApplication::focusWidget();
    if (auto* textEdit = qobject_cast<QTextEdit*>(focusWidget)) {
        textEdit->cut();
    } else if (auto* lineEdit = qobject_cast<QLineEdit*>(focusWidget)) {
        lineEdit->cut();
    }
}

void MainWindow::copy()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
    }
}

void MainWindow::copyAsHtml()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
    }
}

void MainWindow::paste()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        }
    }
}

void MainWindow::pasteToWorld()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
    } else {
    }
}

void MainWindow::selectAll()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
    }
}

void MainWindow::spellCheck()
{
    // TODO: Implement spell check functionality
}

void MainWindow::reloadNamesFile()
{
    // TODO: Implement reload names file functionality
}

void MainWindow::openNotepad()
{
    // TODO: Implement open notepad functionality
}

void MainWindow::flipToNotepad()
{
    // TODO: Implement flip to notepad functionality
}

void MainWindow::colourPicker()
{
    QColor color = QColorDialog::getColor(Qt::white, this, "Choose Colour");
    if (color.isValid()) {
        // Copy color info to clipboard
        QString colorInfo = QString("RGB: %1, %2, %3\nHex: %4")
                                .arg(color.red())
                                .arg(color.green())
                                .arg(color.blue())
                                .arg(color.name());
        QGuiApplication::clipboard()->setText(colorInfo);
    }
}

void MainWindow::debugPackets()
{
    // TODO: Implement debug packets window
}

void MainWindow::find()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

void MainWindow::findForward()
{
    if (m_lastSearchText.isEmpty()) {
        // No previous search - open find dialog
        find();
        return;
    }

    // Perform forward search
    if (!performSearch()) {
        statusBar()->showMessage(QString("Cannot find \"%1\"").arg(m_lastSearchText), 3000);
    }
}

void MainWindow::findBackward()
{
    if (m_lastSearchText.isEmpty()) {
        // No previous search - open find dialog
        find();
        return;
    }

    // Perform backward search
    if (!performSearchBackward()) {
        statusBar()->showMessage(QString("Cannot find \"%1\"").arg(m_lastSearchText), 3000);
    }
}

void MainWindow::recallLastWord()
{
    // Recall last word from input - essentially undo last word deletion
    // TODO: Implement recall last word functionality
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
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    worldWidget->repeatLastCommand();
}

void MainWindow::clearCommandHistory()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
    }
}

void MainWindow::showCommandHistory()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Command History Dialog
    CommandHistoryDialog dialog(worldWidget->document(), this);
    dialog.exec();

}

void MainWindow::globalChange()
{
    // Global Change does search/replace on the command input text
    // Based on CSendView::OnInputGlobalchange() from sendvw.cpp

    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

}

void MainWindow::discardQueuedCommands()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    WorldDocument* doc = worldWidget->document();
    doc->DiscardQueue();
}

void MainWindow::showKeyName()
{
    KeyNameDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString keyName = dialog.keyName();
        if (!keyName.isEmpty()) {
        }
    }
}

// Game menu slots
void MainWindow::connectToMud()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

}

void MainWindow::toggleReconnectOnDisconnect()
{
    // Toggle the global ReconnectOnDisconnect preference
    Database* db = Database::instance();
    bool currentValue = db->getPreferenceInt("ReconnectOnDisconnect", 0) != 0;
    bool newValue = !currentValue;

    db->setPreferenceInt("ReconnectOnDisconnect", newValue ? 1 : 0);
    m_reconnectOnDisconnectAction->setChecked(newValue);

}

void MainWindow::connectToAllOpenWorlds()
{
    // Connect to all open but disconnected worlds
    for (QMdiSubWindow* window : m_mdiArea->subWindowList()) {
        WorldWidget* worldWidget = qobject_cast<WorldWidget*>(window->widget());
        if (worldWidget && !worldWidget->isConnected()) {
            worldWidget->connectToMud();
        }
    }
}

void MainWindow::connectToStartupList()
{
    // Open and connect to all worlds in the startup list
    Database* db = Database::instance();
    QString startupList = db->getPreference("WorldList", "");

    if (startupList.isEmpty()) {
        return;
    }

    // Split by newlines or semicolons (common separators)
    QStringList worldFiles = startupList.split(QRegularExpression("[\\n;]"), Qt::SkipEmptyParts);

    for (const QString& worldFile : worldFiles) {
        QString trimmed = worldFile.trimmed();
        if (!trimmed.isEmpty() && QFile::exists(trimmed)) {
            openWorld(trimmed);
        }
    }
}

void MainWindow::reloadScriptFile()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        return;
    }

    // Reload the script file
    doc->loadScriptFile();
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

}

void MainWindow::toggleWrapOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Toggle m_wrap (word-wrap at spaces enabled/disabled)
    // m_wrap is a boolean (0=off, non-zero=on), separate from m_nWrapColumn (the wrap column width)
    // This matches original MUSHclient behavior from doc.cpp OnGameWraplines()
    doc->m_wrap = m_wrapOutputAction->isChecked() ? 1 : 0;
}

void MainWindow::minimizeToTray()
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        m_trayIcon->showMessage("Mushkin", "Application minimized to system tray",
                                 QSystemTrayIcon::Information, 2000);
    } else {
        // No tray icon, just minimize normally
        showMinimized();
    }
}

void MainWindow::toggleTrace()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Toggle trace mode
    bool traceEnabled = m_traceAction->isChecked();
    doc->m_bTrace = traceEnabled ? 1 : 0;

}

void MainWindow::testTrigger()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Open debug world input dialog to test triggers
    DebugWorldInputDialog dialog(this);
    dialog.setWindowTitle("Test Trigger");
    if (dialog.exec() == QDialog::Accepted) {
        QString testInput = dialog.text();
        if (!testInput.isEmpty()) {
            // Simulate the test input as if received from MUD (processes through triggers)
            doc->simulate(testInput);
        }
    }
}

void MainWindow::editScriptFile()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        return;
    }

    // Open the script file in the system's default text editor
    QUrl fileUrl = QUrl::fromLocalFile(doc->m_strScriptFilename);
    if (!QDesktopServices::openUrl(fileUrl)) {
        QMessageBox::warning(
            this, "Cannot Open File",
            QString("Could not open script file:\n%1\n\n"
                    "No application is associated with this file type.")
                .arg(doc->m_strScriptFilename));
    }
}

void MainWindow::resetAllTimers()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Reset all timers
    doc->resetAllTimers();
}

void MainWindow::sendToAllWorlds()
{
    // Get list of all open world names and documents
    QStringList worldNames;
    QList<WorldDocument*> openWorlds;
    for (QMdiSubWindow* subWindow : m_mdiArea->subWindowList()) {
        if (WorldWidget* ww = qobject_cast<WorldWidget*>(subWindow->widget())) {
            if (WorldDocument* doc = ww->document()) {
                openWorlds.append(doc);
                worldNames.append(doc->worldName());
            }
        }
    }

    if (openWorlds.isEmpty()) {
        QMessageBox::information(this, "Send To All Worlds", "No worlds are currently open.");
        return;
    }

    SendToAllDialog dialog(this);
    dialog.setWorlds(worldNames);
    if (dialog.exec() == QDialog::Accepted) {
        QString text = dialog.sendText();
        QStringList selectedWorlds = dialog.selectedWorlds();
        bool echo = dialog.echo();

        // Send to each selected world
        for (int i = 0; i < openWorlds.size(); ++i) {
            if (selectedWorlds.contains(worldNames[i])) {
                if (echo) {
                    openWorlds[i]->note(QString("> %1").arg(text));
                }
                openWorlds[i]->sendToMud(text + "\n");
            }
        }
    }
}

void MainWindow::doMapperSpecial()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Open mapper special dialog
    MapDialog dialog(doc, this);
    dialog.exec();
}

void MainWindow::addMapperComment()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    // Open mapper comment dialog
    MapCommentDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString comment = dialog.comment();
        if (!comment.isEmpty()) {
            // TODO: Add comment to current mapper location when mapper is fully implemented
        }
    }
}

void MainWindow::configureTriggers()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Unified Preferences dialog to Triggers page
    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Triggers, this);
    dialog.exec();

}

void MainWindow::configureAliases()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Unified Preferences dialog to Aliases page
    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Aliases, this);
    dialog.exec();

}

void MainWindow::configureTimers()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Unified Preferences dialog to Timers page
    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Timers, this);
    dialog.exec();

}

void MainWindow::configureAll()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Unified Preferences dialog to Connection page (first page)
    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Connection, this);
    dialog.exec();
}

void MainWindow::configureConnection()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Connection, this);
    dialog.exec();
}

void MainWindow::configureLogging()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Logging, this);
    dialog.exec();
}

void MainWindow::configureInfo()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Info, this);
    dialog.exec();
}

void MainWindow::configureOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Output, this);
    dialog.exec();
}

void MainWindow::configureMxp()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::MXP, this);
    dialog.exec();
}

void MainWindow::configureColours()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Colors, this);
    dialog.exec();
}

void MainWindow::configureCommands()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Commands, this);
    dialog.exec();
}

void MainWindow::configureKeypad()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Keypad, this);
    dialog.exec();
}

void MainWindow::configureMacros()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Macros, this);
    dialog.exec();
}

void MainWindow::configureAutoSay()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::AutoSay, this);
    dialog.exec();
}

void MainWindow::configurePaste()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::PasteSend, this);
    dialog.exec();
}

void MainWindow::configureScripting()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Scripting, this);
    dialog.exec();
}

void MainWindow::configureVariables()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    UnifiedPreferencesDialog dialog(worldWidget->document(),
                                     UnifiedPreferencesDialog::Page::Variables, this);
    dialog.exec();
}

void MainWindow::configurePlugins()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Plugin Dialog
    PluginDialog dialog(worldWidget->document(), this);
    dialog.exec();

}

void MainWindow::pluginWizard()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget) {
        return;
    }

    // Open Plugin Wizard
    PluginWizard wizard(worldWidget->document(), this);
    if (wizard.exec() == QDialog::Accepted) {
    } else {
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

void MainWindow::arrangeIcons()
{
    // Arrange minimized MDI subwindows (icons) at the bottom of the MDI area
    // Note: With frameless windows, this may have limited effect
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    int x = 0;
    int y = m_mdiArea->height() - 30; // Position at bottom
    for (QMdiSubWindow* window : windows) {
        if (window->isMinimized()) {
            window->move(x, y);
            x += 160; // Width for minimized window icon
            if (x + 160 > m_mdiArea->width()) {
                x = 0;
                y -= 30; // Move up a row
            }
        }
    }
}

void MainWindow::closeAllNotepadWindows()
{
    // Only close notepad windows, not world windows
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    for (QMdiSubWindow* window : windows) {
        // Check if this is a notepad window (not a WorldWidget)
        if (qobject_cast<WorldWidget*>(window->widget()) == nullptr) {
            window->close();
        }
    }
}

void MainWindow::newWindow()
{
    // Open another view of the active world document
    // For now, just show a message - full implementation would create a second view
    QMessageBox::information(this, "New Window",
                             "Opening additional views of the same world is not yet implemented.");
}

void MainWindow::toggleStatusBar(bool visible)
{
    statusBar()->setVisible(visible);
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

}

void MainWindow::toggleFullScreen(bool enabled)
{
    if (enabled) {
        showFullScreen();
    } else {
        showNormal();
    }

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
    QDesktopServices::openUrl(QUrl("https://github.com/justinpopa/mushkin/wiki"));
}

void MainWindow::openBugReports()
{
    QDesktopServices::openUrl(QUrl("https://github.com/justinpopa/mushkin/issues"));
}

void MainWindow::openDocumentation()
{
    QDesktopServices::openUrl(QUrl("https://github.com/justinpopa/mushkin/wiki"));
}

void MainWindow::openWebPage()
{
    QDesktopServices::openUrl(QUrl("https://github.com/justinpopa/mushkin"));
}

void MainWindow::about()
{
    QMessageBox::about(
        this, "About Mushkin",
        QString("<h2>Mushkin</h2>"
                "<p><b>Version %1</b></p>"
                "<p>Cross-platform MUD client built with Qt 6</p>"
                "<hr>"
                "<p>A ground-up rewrite inspired by MUSHclient, "
                "preserving its behavior and compatibility across multiple platforms.</p>"
                "<p>Original MUSHclient by Nick Gammon:<br>"
                "<a href='https://www.gammon.com.au/mushclient'>www.gammon.com.au/mushclient</a></p>")
            .arg(QCoreApplication::applicationVersion()));
}

// Additional Edit/File/Display menu slots
void MainWindow::saveSelection()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

void MainWindow::clearOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

    }
}

void MainWindow::recallText()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

}

void MainWindow::stopSound()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    // Stop all sounds for this world (buffer 0 = all)
    worldWidget->document()->StopSound(0);
}

void MainWindow::toggleCommandEcho()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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

}

void MainWindow::toggleFreezeOutput()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    bool newValue = m_freezeOutputAction->isChecked();
    worldWidget->outputView()->setFrozen(newValue);

}

void MainWindow::goToLine()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->document()) {
        return;
    }

    // Get total lines from the world document's line list
    int totalLines = worldWidget->document()->m_lineList.count();
    if (totalLines == 0) {
        return;
    }

    GoToLineDialog dlg(totalLines, 1, this);
    if (dlg.exec() == QDialog::Accepted) {
        int lineNumber = dlg.lineNumber();
        if (lineNumber > 0 && lineNumber <= totalLines && worldWidget->outputView()) {
            worldWidget->outputView()->scrollToLine(lineNumber - 1); // 0-based index
        }
    }
}

void MainWindow::goToUrl()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    QString selection = worldWidget->outputView()->getSelectedText().trimmed();
    if (selection.isEmpty()) {
        return;
    }

    if (selection.length() > 512) {
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
}

void MainWindow::sendMailTo()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
        return;
    }

    WorldWidget* worldWidget = qobject_cast<WorldWidget*>(activeSubWindow->widget());
    if (!worldWidget || !worldWidget->outputView()) {
        return;
    }

    QString selection = worldWidget->outputView()->getSelectedText().trimmed();
    if (selection.isEmpty()) {
        return;
    }

    // Strip mailto: if already present
    QString email = selection;
    if (email.startsWith("mailto:", Qt::CaseInsensitive)) {
        email = email.mid(7);
    }

    QDesktopServices::openUrl(QUrl("mailto:" + email));
}

void MainWindow::bookmarkSelection()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        return;
    }

    Line* pLine = worldWidget->document()->m_lineList[lineIndex];
    pLine->flags ^= BOOKMARK; // Toggle bookmark
    worldWidget->outputView()->update();
}

void MainWindow::goToBookmark()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
            return;
        }
        current = (current + 1) % lines.count();
    } while (current != searchStart);

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

// Helper method to perform backward search with stored parameters
bool MainWindow::performSearchBackward()
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

    // Start from position before last find
    int startLine = (m_lastFoundLine >= 0) ? m_lastFoundLine : doc->m_lineList.count() - 1;
    int startChar = (m_lastFoundChar > 0) ? m_lastFoundChar - 1 : -1;

    // Search backward from current position
    for (int i = startLine; i >= 0; i--) {
        Line* pLine = doc->m_lineList[i];
        if (!pLine || pLine->len() == 0)
            continue;

        QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

        int searchTo = (i == startLine && startChar >= 0) ? startChar : lineText.length();
        int index = -1;

        if (m_lastSearchUseRegex) {
            QRegularExpression re(m_lastSearchText,
                                  m_lastSearchMatchCase
                                      ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption);
            // Search backwards by finding all matches up to searchTo
            int lastMatch = -1;
            QRegularExpressionMatchIterator it = re.globalMatch(lineText);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                if (match.capturedStart() < searchTo) {
                    lastMatch = match.capturedStart();
                } else {
                    break;
                }
            }
            index = lastMatch;
        } else {
            // Find last occurrence before searchTo
            index = lineText.lastIndexOf(m_lastSearchText, searchTo - 1, cs);
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
    }

    // Not found - wrap around to end
    for (int i = doc->m_lineList.count() - 1; i > startLine; i--) {
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
            int lastMatch = -1;
            QRegularExpressionMatchIterator it = re.globalMatch(lineText);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                lastMatch = match.capturedStart();
            }
            index = lastMatch;
        } else {
            index = lineText.lastIndexOf(m_lastSearchText, -1, cs);
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

#ifdef Q_OS_MACOS
    // On macOS, use frameless subwindow for consistency with world windows
    subWindow->setWindowFlags(Qt::FramelessWindowHint);
    // Connect to window state changes to update minimized bar
    connect(subWindow, &QMdiSubWindow::windowStateChanged, this, &MainWindow::onSubWindowStateChanged);
    // Clean up minimized bar when destroyed
    connect(subWindow, &QObject::destroyed, [this, subWindow]() {
        if (m_minimizedBars.contains(subWindow)) {
            QWidget* bar = m_minimizedBars.take(subWindow);
            m_minimizedBarLayout->removeWidget(bar);
            delete bar;
            updateMinimizedBar();
        }
    });
#endif

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

void MainWindow::sendFile()
{
    QMdiSubWindow* activeSubWindow = m_mdiArea->activeSubWindow();
    if (!activeSubWindow) {
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
        // No active world - clear all indicators
        m_freezeIndicator->setText("");
        m_freezeIndicator->setStyleSheet("");
        m_worldNameIndicator->setText("");
        m_timeIndicator->setText("");
        m_linesIndicator->setText("");
        m_logIndicator->setText("");
        return;
    }

    // Freeze/Connection indicator (combined like original MUSHclient)
    bool isConnected = worldWidget->isConnected();
    if (!isConnected) {
        m_freezeIndicator->setText("CLOSED");
        m_freezeIndicator->setStyleSheet("");
    } else {
        OutputView* outputView = worldWidget->outputView();
        if (outputView && outputView->isFrozen()) {
            if (!outputView->isAtBottom()) {
                // Frozen and NOT at bottom - show MORE (inverted style)
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

    // World name indicator
    m_worldNameIndicator->setText(worldWidget->worldName());

    // Time indicator
    updateTimeIndicator();

    // Lines indicator
    if (WorldDocument* doc = worldWidget->document()) {
        int lineCount = doc->m_lineList.count();
        m_linesIndicator->setText(QString("%1 lines").arg(lineCount));
    } else {
        m_linesIndicator->setText("");
    }

    // Log indicator
    if (WorldDocument* doc = worldWidget->document()) {
        m_logIndicator->setText(doc->isLogging() ? "LOG" : "");
    } else {
        m_logIndicator->setText("");
    }
}

void MainWindow::onFreezeStateChanged(bool frozen, bool atBottom)
{
    // Update the freeze indicator immediately
    if (!m_trackedWorld) {
        return;
    }

    if (frozen) {
        if (!atBottom) {
            // Frozen and NOT at bottom - show MORE (inverted style like original)
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
    // Update freeze indicator - shows CLOSED when disconnected
    if (!connected) {
        m_freezeIndicator->setText("CLOSED");
        m_freezeIndicator->setStyleSheet("");
    } else {
        // Let onFreezeStateChanged handle the rest when connected
        if (m_trackedWorld) {
            if (OutputView* outputView = m_trackedWorld->outputView()) {
                onFreezeStateChanged(outputView->isFrozen(), outputView->isAtBottom());
            }
        }
    }
}

void MainWindow::updateTimeIndicator()
{
    if (!m_trackedWorld) {
        m_timeIndicator->setText("");
        return;
    }

    WorldDocument* doc = m_trackedWorld->document();
    if (!doc) {
        m_timeIndicator->setText("");
        return;
    }

    qint64 secs = doc->connectedTime();
    if (secs < 0) {
        m_timeIndicator->setText("");
        return;
    }

    // Format as H:MM:SS (matching original MUSHclient)
    int hours = secs / 3600;
    int mins = (secs % 3600) / 60;
    int s = secs % 60;
    m_timeIndicator->setText(QString("%1:%2:%3")
                                 .arg(hours)
                                 .arg(mins, 2, 10, QChar('0'))
                                 .arg(s, 2, 10, QChar('0')));
}

void MainWindow::onFreezeIndicatorClicked()
{
    // Toggle freeze on double-click
    if (!m_trackedWorld) {
        return;
    }

    OutputView* outputView = m_trackedWorld->outputView();
    if (outputView) {
        outputView->setFrozen(!outputView->isFrozen());
    }
}

void MainWindow::onTimeIndicatorClicked()
{
    // Reset connected time on double-click
    if (!m_trackedWorld) {
        return;
    }

    WorldDocument* doc = m_trackedWorld->document();
    if (doc) {
        doc->resetConnectedTime();
        updateTimeIndicator();
    }
}

void MainWindow::onLinesIndicatorClicked()
{
    // Open Go To Line dialog on double-click
    goToLine();
}

void MainWindow::onWorldNameIndicatorClicked()
{
    // Show activity window on click
    m_activityWindow->show();
    m_activityWindow->raise();
}

#ifdef Q_OS_MACOS
void MainWindow::onSubWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
{
    Q_UNUSED(oldState);

    QMdiSubWindow* subWindow = qobject_cast<QMdiSubWindow*>(sender());
    if (!subWindow) {
        return;
    }

    bool wasMinimized = m_minimizedBars.contains(subWindow);
    bool isMinimized = (newState & Qt::WindowMinimized);

    if (isMinimized && !wasMinimized) {
        // Window was just minimized - create a bar for it
        QWidget* bar = new QWidget(m_minimizedBarContainer);
        bar->setFixedHeight(24);
        bar->setStyleSheet(
            "QWidget { background: #3a3a3a; border: 1px solid #555; border-radius: 3px; }");

        QHBoxLayout* barLayout = new QHBoxLayout(bar);
        barLayout->setContentsMargins(6, 2, 4, 2);
        barLayout->setSpacing(4);

        // Window title
        QLabel* titleLabel = new QLabel(subWindow->windowTitle(), bar);
        titleLabel->setStyleSheet("QLabel { color: #ccc; border: none; background: transparent; }");
        titleLabel->setMaximumWidth(150);
        barLayout->addWidget(titleLabel);

        // Restore button
        QToolButton* restoreBtn = new QToolButton(bar);
        restoreBtn->setText("□");
        restoreBtn->setFixedSize(18, 18);
        restoreBtn->setStyleSheet(
            "QToolButton { border: 1px solid #666; border-radius: 2px; background: #444; color: #aaa; }"
            "QToolButton:hover { background: #555; }");
        connect(restoreBtn, &QToolButton::clicked, [subWindow]() {
            subWindow->showNormal();
        });
        barLayout->addWidget(restoreBtn);

        // Close button
        QToolButton* closeBtn = new QToolButton(bar);
        closeBtn->setText("×");
        closeBtn->setFixedSize(18, 18);
        closeBtn->setStyleSheet(
            "QToolButton { border: 1px solid #666; border-radius: 2px; background: #444; color: #aaa; }"
            "QToolButton:hover { background: #744; color: #fcc; }");
        connect(closeBtn, &QToolButton::clicked, [subWindow]() {
            subWindow->close();
        });
        barLayout->addWidget(closeBtn);

        // Insert before the stretch
        m_minimizedBarLayout->insertWidget(m_minimizedBarLayout->count() - 1, bar);
        m_minimizedBars.insert(subWindow, bar);

    } else if (!isMinimized && wasMinimized) {
        // Window was just restored - remove its bar
        QWidget* bar = m_minimizedBars.take(subWindow);
        if (bar) {
            m_minimizedBarLayout->removeWidget(bar);
            delete bar;
        }
    }

    updateMinimizedBar();
}

void MainWindow::updateMinimizedBar()
{
    // Show/hide the container based on whether there are any minimized windows
    bool hasMinimized = !m_minimizedBars.isEmpty();
    m_minimizedBarContainer->setVisible(hasMinimized);
}
#endif
