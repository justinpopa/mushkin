#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QAction>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QPointer>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>

class ActivityWindow;
class WorldDocument;

/**
 * MainWindow - The main application window with MDI support
 *
 * This is the top-level window for MUSHclient Qt. It provides:
 * - MDI (Multi-Document Interface) for managing multiple world windows
 * - Menu bar with File, Edit, Window, Help menus
 * - Toolbar with common actions
 * - Status bar for messages
 * - Window geometry persistence (saves/restores position and size)
 * - Recent files management
 *
 * MFC Equivalent: CMainFrame (mainfrm.cpp)
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /**
     * Open a world file by path
     * Called from main() for command-line arguments
     */
    void openWorld(const QString& filename);

    /**
     * Queue multiple world files to open on startup
     */
    void queueWorldFiles(const QStringList& files);

    /**
     * Set toolbar position (for Lua API)
     * @param which 1=main, 2=game, 3=activity
     * @param floating true to float, false to dock
     * @param side 1=top, 2=bottom, 3=left, 4=right
     * @param top Top position for floating
     * @param left Left position for floating
     * @return 0 on success, error code on failure
     */
    int setToolBarPosition(int which, bool floating, int side, int top, int left);

    /**
     * Get toolbar info (for Lua API)
     * @param which 1=main, 2=game, 3=activity, 4=infobar
     * @param infoType 0=height, 1=width
     * @return Dimension in pixels, or 0 if invalid
     */
    int getToolBarInfo(int which, int infoType);

    /**
     * Info Bar Lua API methods
     */
    void showInfoBar(bool visible);
    void infoBarAppend(const QString& text);
    void infoBarClear();
    void infoBarSetColor(const QColor& color);
    void infoBarSetFont(const QString& fontName, int size, int style);
    void infoBarSetBackground(const QColor& color);

  protected:
    /**
     * Save window geometry when closing
     */
    void closeEvent(QCloseEvent* event) override;

    /**
     * Handle window state changes (minimize to tray)
     */
    void changeEvent(QEvent* event) override;

    /**
     * Handle clicks on status bar indicators
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

  public slots:
    /**
     * Apply theme based on preferences (light/dark/system)
     * Called when theme preference changes
     */
    void applyTheme();

  private slots:
    // File menu actions
    void newWorld();
    void openWorld();
    void openStartupList();
    void closeWorld();
    void saveWorld();
    void saveWorldAs();
    void worldProperties();
    void globalPreferences();
    void toggleLogSession();
    void reloadDefaults();
    void saveSelection();
    void openRecentFile();
    void quickConnect();
    void importXml();
    void exitApplication();

    // Edit menu actions
    void undo();
    void cut();
    void copy();
    void copyAsHtml();
    void paste();
    void pasteToWorld();
    void recallLastWord();
    void selectAll();
    void spellCheck();
    void generateCharacterName();
    void reloadNamesFile();
    void generateUniqueID();
    void openNotepad();
    void flipToNotepad();
    void colourPicker();
    void debugPackets();
    void goToMatchingBrace();
    void selectToMatchingBrace();

    // Input menu actions
    void activateInputArea();
    void previousCommand();
    void nextCommand();
    void repeatLastCommand();
    void clearCommandHistory();
    void showCommandHistory();
    void globalChange();
    void discardQueuedCommands();
    void showKeyName();

    // Connection menu actions
    void connectToMud();
    void disconnectFromMud();
    void toggleAutoConnect();
    void toggleReconnectOnDisconnect();
    void connectToAllOpenWorlds();
    void connectToStartupList();

    // Game menu actions
    void reloadScriptFile();
    void toggleAutoSay();
    void toggleWrapOutput();
    void testTrigger();
    void minimizeToTray();
    void toggleTrace();
    void editScriptFile();
    void resetAllTimers();
    void sendToAllWorlds();
    void doMapperSpecial();
    void addMapperComment();
    // Configure submenu actions
    void configureAll();
    void configureConnection();
    void configureLogging();
    void configureInfo();
    void configureOutput();
    void configureMxp();
    void configureColours();
    void configureCommands();
    void configureKeypad();
    void configureMacros();
    void configureAutoSay();
    void configurePaste();
    void configureScripting();
    void configureVariables();
    void configureTimers();
    void configureTriggers();
    void configureAliases();
    void configurePlugins();
    void pluginWizard();
    void immediateScript();
    void sendFile();
    void showMapper();

    // Display menu actions
    void find();
    void findNext();
    void findForward();
    void findBackward();
    void recallText();
    void highlightPhrase();
    void scrollToStart();
    void scrollPageUp();
    void scrollPageDown();
    void scrollToEnd();
    void scrollLineUp();
    void scrollLineDown();
    void clearOutput();
    void stopSound();
    void toggleCommandEcho();
    void toggleFreezeOutput();
    void goToLine();
    void goToUrl();
    void sendMailTo();
    void bookmarkSelection();
    void goToBookmark();
    void activityList();
    void textAttributes();
    void multilineTrigger();

    // Convert menu actions (text transformations for notepad windows)
    void convertUppercase();
    void convertLowercase();
    void convertUnixToDos();
    void convertDosToUnix();
    void convertMacToDos();
    void convertDosToMac();
    void convertBase64Encode();
    void convertBase64Decode();
    void convertHtmlEncode();
    void convertHtmlDecode();
    void convertQuoteLines();
    void convertRemoveExtraBlanks();
    void convertWrapLines();

    // View menu actions
    void toggleStatusBar(bool visible);
    void toggleAlwaysOnTop(bool enabled);
    void toggleFullScreen(bool enabled);
    void resetToolbars();

    // Window menu actions
    void cascade();
    void tileHorizontally();
    void tileVertically();
    void arrangeIcons();
    void newWindow();
    void closeAllNotepadWindows();

    // Help menu actions
    void showHelp();
    void openBugReports();
    void openDocumentation();
    void openWebPage();
    void about();

    // MDI area signals
    void updateMenus();
    void updateWindowMenu();

    // Notepad window creation
    void createNotepadWindow(class NotepadWidget* notepad);

    // Status bar updates
    void updateStatusIndicators();
    void onFreezeStateChanged(bool frozen, int lineCount);
    void onConnectionStateChanged(bool connected);
    void updateTimeIndicator();

    // Status bar click handlers
    void onFreezeIndicatorClicked();
    void onTimeIndicatorClicked();
    void onLinesIndicatorClicked();
    void onWorldNameIndicatorClicked();

    // System tray
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

#ifdef Q_OS_MACOS
    // Minimized bar management (macOS only)
    void onSubWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
    void updateMinimizedBar();
#endif

  private:
    /**
     * Open worlds queued from command line or startup list
     * Called after window is shown using QTimer::singleShot
     */
    void openStartupWorlds();

    /**
     * Create the menu bar with all menus
     */
    void createMenus();

    /**
     * Find matching brace position from current cursor in text
     * Returns -1 if no match found
     */
    int findMatchingBracePosition(const QString& text, int cursorPos);

    /**
     * Create the toolbars (main, game, activity)
     */
    void createToolBars();

    /**
     * Create the game toolbar with direction buttons
     */
    void createGameToolBar();

    /**
     * Create the activity toolbar for world switching
     */
    void createActivityToolBar();

    /**
     * Apply toolbar appearance preferences (flat style, button style)
     */
    void applyToolbarPreferences();

    /**
     * Update toolbar icons based on current theme
     */
    void updateToolbarIcons();

    /**
     * Load an SVG icon and colorize it based on current palette
     */
    QIcon loadThemedIcon(const QString& name);

    /**
     * Create the info bar (dockable bar with rich text display)
     */
    void createInfoBar();

    /**
     * Create the status bar
     */
    void createStatusBar();

    /**
     * Read window geometry from settings
     */
    void readSettings();

    /**
     * Write window geometry to settings
     */
    void writeSettings();

    /**
     * Update recent files menu
     */
    void updateRecentFilesMenu();

    /**
     * Add a file to recent files list
     */
    void addRecentFile(const QString& filename);

    // UI Components
    QMdiArea* m_mdiArea;

#ifdef Q_OS_MACOS
    // Minimized bar (macOS only - frameless MDI windows need custom minimized representation)
    QWidget* m_minimizedBarContainer;
    QHBoxLayout* m_minimizedBarLayout;
    QMap<QMdiSubWindow*, QWidget*> m_minimizedBars; // Maps subwindow to its minimized bar widget
#endif

    // Toolbars
    QToolBar* m_mainToolBar;
    QToolBar* m_gameToolBar;
    QToolBar* m_activityToolBar;

    // Info Bar (dockable bar with rich text display)
    QDockWidget* m_infoBarDock;
    QTextEdit* m_infoBarText;

    // Activity Window (dockable list of all open worlds)
    ActivityWindow* m_activityWindow;


    // Status bar indicators (order matches original MUSHclient: Freeze, Name, Time, Lines, Log)
    QLabel* m_freezeIndicator;
    QLabel* m_worldNameIndicator;
    QLabel* m_timeIndicator;
    QLabel* m_linesIndicator;
    QLabel* m_logIndicator;
    QTimer* m_statusBarTimer;  // Updates time indicator every second

    // Currently tracked world for status updates
    QPointer<class WorldWidget> m_trackedWorld;

    // Last focused world for focus change callbacks
    QPointer<class WorldDocument> m_lastFocusedWorld;

    // File menu and actions
    QMenu* m_fileMenu;
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_openStartupListAction;
    QAction* m_closeAction;
    QAction* m_importXmlAction;
    // m_configurePluginsAction and m_pluginWizardAction declared in Game menu section
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_saveSelectionAction;
    QAction* m_globalPreferencesAction;
    QAction* m_logSessionAction;
    QAction* m_reloadDefaultsAction;
    QAction* m_worldPropertiesAction;
    QMenu* m_recentFilesMenu;
    QAction* m_exitAction;

    // Edit menu and actions
    QMenu* m_editMenu;
    QAction* m_undoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_copyAsHtmlAction;
    QAction* m_pasteAction;
    QAction* m_pasteToWorldAction;
    QAction* m_recallLastWordAction;
    QAction* m_selectAllAction;
    QAction* m_spellCheckAction;
    QAction* m_generateNameAction;
    QAction* m_reloadNamesFileAction;
    QAction* m_generateIdAction;
    QAction* m_notepadAction;
    QAction* m_flipToNotepadAction;
    QAction* m_colourPickerAction;
    QAction* m_debugPacketsAction;
    QAction* m_goToMatchingBraceAction;
    QAction* m_selectToMatchingBraceAction;

    // Input menu and actions
    QMenu* m_inputMenu;
    QAction* m_activateInputAreaAction;
    QAction* m_previousCommandAction;
    QAction* m_nextCommandAction;
    QAction* m_repeatLastCommandAction;
    QAction* m_clearCommandHistoryAction;
    QAction* m_commandHistoryAction;
    QAction* m_globalChangeAction;
    QAction* m_discardQueueAction;
    QAction* m_keyNameAction;

    // Connection menu and actions
    QMenu* m_connectionMenu;
    QAction* m_connectAction;
    QAction* m_disconnectAction;
    QAction* m_autoConnectAction;
    QAction* m_reconnectOnDisconnectAction;
    QAction* m_connectToAllAction;
    QAction* m_connectToStartupListAction;

    // Game menu and actions
    QMenu* m_gameMenu;
    QAction* m_reloadScriptFileAction;
    QAction* m_autoSayAction;
    QAction* m_wrapOutputAction;
    QAction* m_testTriggerAction;
    QAction* m_minimizeProgramAction;
    QAction* m_traceAction;
    QAction* m_editScriptFileAction;
    QAction* m_resetAllTimersAction;
    QAction* m_sendToAllWorldsAction;
    QAction* m_doMapperSpecialAction;
    QAction* m_addMapperCommentAction;
    // Configure submenu actions
    QAction* m_configureAllAction;
    QAction* m_configureConnectionAction;
    QAction* m_configureConnectingAction;
    QAction* m_configureLoggingAction;
    QAction* m_configureNotesAction;
    QAction* m_configureOutputAction;
    QAction* m_configureMxpAction;
    QAction* m_configureColoursAction;
    QAction* m_configureCustomColoursAction;
    QAction* m_configureCommandsAction;
    QAction* m_configureKeypadAction;
    QAction* m_configureMacrosAction;
    QAction* m_configureAutoSayAction;
    QAction* m_configurePasteAction;
    QAction* m_configureSendFileAction;
    QAction* m_configureScriptingAction;
    QAction* m_configureVariablesAction;
    QAction* m_configureTimersAction;
    QAction* m_configureTriggersAction;
    QAction* m_configureAliasesAction;
    QAction* m_configureInfoAction;
    QAction* m_configurePluginsAction;
    QAction* m_pluginWizardAction;
    QAction* m_immediateScriptAction;
    QAction* m_sendFileAction;
    QAction* m_mapperAction;

    // Display menu and actions (scrolling, output control)
    QMenu* m_displayMenu;
    QAction* m_startAction;
    QAction* m_pageUpAction;
    QAction* m_pageDownAction;
    QAction* m_endAction;
    QAction* m_lineUpAction;
    QAction* m_lineDownAction;
    QAction* m_activityListAction;
    QAction* m_freezeOutputAction;
    QAction* m_findAction;
    QAction* m_findNextAction;
    QAction* m_findForwardAction;
    QAction* m_findBackwardAction;
    QAction* m_recallTextAction;
    QAction* m_goToLineAction;
    QAction* m_goToUrlAction;
    QAction* m_sendMailToAction;
    QAction* m_clearOutputAction;
    QAction* m_stopSoundAction;
    QAction* m_bookmarkSelectionAction;
    QAction* m_goToBookmarkAction;
    QAction* m_highlightPhraseAction;
    QAction* m_multilineTriggerAction;
    QAction* m_textAttributesAction;
    QAction* m_commandEchoAction;

    // Convert menu and actions (text transformations for notepad windows)
    QMenu* m_convertMenu;
    QAction* m_convertUppercaseAction;
    QAction* m_convertLowercaseAction;
    QAction* m_convertUnixToDosAction;
    QAction* m_convertDosToUnixAction;
    QAction* m_convertMacToDosAction;
    QAction* m_convertDosToMacAction;
    QAction* m_convertBase64EncodeAction;
    QAction* m_convertBase64DecodeAction;
    QAction* m_convertHtmlEncodeAction;
    QAction* m_convertHtmlDecodeAction;
    QAction* m_convertQuoteLinesAction;
    QAction* m_convertRemoveExtraBlanksAction;
    QAction* m_convertWrapLinesAction;

    // View menu and actions
    QMenu* m_viewMenu;
    QAction* m_mainToolBarAction;
    QAction* m_gameToolBarAction;
    QAction* m_activityToolBarAction;
    QAction* m_statusBarAction;
    QAction* m_infoBarAction;
    QAction* m_resetToolbarsAction;
    QAction* m_alwaysOnTopAction;
    QAction* m_fullScreenAction;

    // Window menu and actions
    QMenu* m_windowMenu;
    QAction* m_cascadeAction;
    QAction* m_tileHorizontallyAction;
    QAction* m_tileVerticallyAction;
    QAction* m_arrangeIconsAction;
    QAction* m_newWindowAction;
    QAction* m_closeAllNotepadAction;

    // Help menu and actions
    QMenu* m_helpMenu;
    QAction* m_helpAction;
    QAction* m_bugReportsAction;
    QAction* m_documentationAction;
    QAction* m_webPageAction;
    QAction* m_aboutAction;

    // Recent files
    static constexpr int MaxRecentFiles = 10;
    QList<QAction*> m_recentFileActions;

    // World files queued to open on startup
    QStringList m_queuedWorldFiles;

    // Find dialog state
    QString m_lastSearchText;
    bool m_lastSearchMatchCase;
    bool m_lastSearchUseRegex;
    bool m_lastSearchForward;
    int m_lastFoundLine;
    int m_lastFoundChar;

    /**
     * performSearch - Execute search with current parameters (forward)
     * @return true if found, false if not found
     */
    bool performSearch();

    /**
     * performSearchBackward - Execute search with current parameters (backward)
     * @return true if found, false if not found
     */
    bool performSearchBackward();

    /**
     * Setup system tray icon based on preferences
     */
    void setupSystemTray();

    // System tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
};

#endif // MAIN_WINDOW_H
