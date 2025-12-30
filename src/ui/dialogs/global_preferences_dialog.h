#ifndef GLOBAL_PREFERENCES_DIALOG_H
#define GLOBAL_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QFont>
#include <QPlainTextEdit>

// Forward declarations
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QLabel;
class QRadioButton;

/**
 * GlobalPreferencesDialog - Application-wide preferences
 *
 * Provides a sidebar-style interface for configuring global settings
 * that apply to ALL worlds (not per-world settings like WorldPropertiesDialog).
 *
 * Settings stored in: mushclient_prefs.sqlite database
 * Accessed via: Edit → Preferences (Ctrl+,)
 *
 * Categories:
 * - General: Auto-connect, confirmations, defaults
 * - Output: Default output font, colors, wrap column, buffer size
 * - Logging: Log directory, format, auto-enable
 * - Scripting: Script language, timeout, error handling
 *
 * Future expansion: MXP, Timers, Plugins, Printer
 */
class GlobalPreferencesDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit GlobalPreferencesDialog(QWidget* parent = nullptr);
    ~GlobalPreferencesDialog() override;

  private slots:
    /**
     * OK button clicked - save and close
     */
    void onOkClicked();

    /**
     * Cancel button clicked - discard changes and close
     */
    void onCancelClicked();

    /**
     * Apply button clicked - save without closing
     */
    void onApplyClicked();

    /**
     * Category selected in sidebar
     */
    void onCategoryChanged(int index);

  private:
    // Setup methods
    void setupUi();
    void setupSidebar();
    void createPages();

    // Page creation (matching MUSHclient tabs)
    QWidget* createWorldsPage();     // Worlds tab (P1)
    QWidget* createGeneralPage();    // General tab (P2)
    QWidget* createDefaultsPage();   // Defaults tab (P9)
    QWidget* createNotepadPage();    // Notepad tab (P10)
    QWidget* createPluginsPage();    // Plugins tab (P12)
    QWidget* createLuaScriptsPage(); // Lua Scripts tab (P13)
    QWidget* createClosingPage();    // Closing tab (P3)
    QWidget* createLoggingPage();    // Logging tab (P5)
    QWidget* createTimersPage();     // Timers tab (P6)
    QWidget* createActivityPage();   // Activity tab (P7)
    QWidget* createTrayIconPage();   // Tray Icon tab (P11)

    // Data methods
    void loadSettings();
    void saveSettings();
    void applySettings();

    // Helper methods
    void updateColorButton(QPushButton* button, QRgb color);
    void onColorButtonClicked(QPushButton* button, QRgb* colorStorage);

    // Member variables - Main UI
    QListWidget* m_categoryList;    // Left sidebar
    QStackedWidget* m_contentStack; // Right content area
    QDialogButtonBox* m_buttonBox;

    // === Worlds Page ===
    QListWidget* m_worldList;
    QPushButton* m_addWorld;
    QPushButton* m_removeWorld;
    QPushButton* m_moveWorldUp;
    QPushButton* m_moveWorldDown;
    QPushButton* m_addCurrentWorld;
    QLineEdit* m_worldDirectory;
    QPushButton* m_browseWorldDir;

    // === General Page ===
    // Theme section
    QComboBox* m_themeMode;

    // Worlds section
    QCheckBox* m_autoConnectWorlds;
    QCheckBox* m_reconnectOnDisconnect;
    QCheckBox* m_openWorldsMaximized;
    QCheckBox* m_notifyIfCannotConnect;
    QCheckBox* m_notifyOnDisconnect;

    // Behavior section
    QCheckBox* m_allTypingToCommandWindow;
    QCheckBox* m_altKeyActivatesMenu;
    QCheckBox* m_fixedFontForEditing;
    QCheckBox* m_f1Macro;
    QCheckBox* m_regexpMatchEmpty;
    QCheckBox* m_triggerRemoveCheck;
    QCheckBox* m_errorNotificationToOutput;

    // Tab completion / Word delimiters
    QLineEdit* m_wordDelimiters;
    QLineEdit* m_wordDelimitersDblClick;

    // Display
    QComboBox* m_windowTabsStyle;
    QLineEdit* m_localeCode;
    QCheckBox* m_showGridLinesInListViews;
    QCheckBox* m_flatToolbars;

    // === Defaults Page ===
    // Fonts
    QPushButton* m_defaultOutputFontButton;
    QLabel* m_defaultOutputFontLabel;
    QFont m_defaultOutputFont;

    QPushButton* m_defaultInputFontButton;
    QLabel* m_defaultInputFontLabel;
    QFont m_defaultInputFont;

    QPushButton* m_fixedPitchFontButton;
    QLabel* m_fixedPitchFontLabel;
    QFont m_fixedPitchFont;

    // Default import files
    QLineEdit* m_defaultAliasesFile;
    QLineEdit* m_defaultTriggersFile;
    QLineEdit* m_defaultTimersFile;
    QLineEdit* m_defaultMacrosFile;
    QLineEdit* m_defaultColoursFile;
    QPushButton* m_browseAliasesFile;
    QPushButton* m_browseTriggersFile;
    QPushButton* m_browseTimersFile;
    QPushButton* m_browseMacrosFile;
    QPushButton* m_browseColoursFile;

    // Display settings
    QCheckBox* m_bleedBackground;
    QCheckBox* m_colourGradientConfig;
    QCheckBox* m_autoExpandConfig;
    QCheckBox* m_smoothScrolling;
    QCheckBox* m_smootherScrolling;

    // === Notepad Page ===
    QCheckBox* m_notepadWordWrap;
    QPushButton* m_notepadFontButton;
    QLabel* m_notepadFontLabel;
    QFont m_notepadFont;
    QPushButton* m_notepadBackColourButton;
    QPushButton* m_notepadTextColourButton;
    QRgb m_notepadBackColour;
    QRgb m_notepadTextColour;
    QLineEdit* m_notepadQuoteString;
    QCheckBox* m_tabInsertsTab;

    // Paren matching flags
    QCheckBox* m_parenMatchNestBraces;
    QCheckBox* m_parenMatchBackslashEscapes;
    QCheckBox* m_parenMatchPercentEscapes;
    QCheckBox* m_parenMatchSingleQuotes;
    QCheckBox* m_parenMatchDoubleQuotes;
    QCheckBox* m_parenMatchEscapeSingleQuotes;
    QCheckBox* m_parenMatchEscapeDoubleQuotes;

    // === Plugins Page ===
    QLineEdit* m_pluginsDirectory;
    QLineEdit* m_stateFilesDirectory;
    QPushButton* m_browsePluginsDir;
    QPushButton* m_browseStateFilesDir;
    QListWidget* m_pluginList;
    QPushButton* m_addPlugin;
    QPushButton* m_removePlugin;
    QPushButton* m_movePluginUp;
    QPushButton* m_movePluginDown;

    // === Lua Scripts Page ===
    QPlainTextEdit* m_luaScript;
    QCheckBox* m_enablePackageLibrary;

    // === Closing Page ===
    QCheckBox* m_confirmBeforeClosingMushclient;
    QCheckBox* m_confirmBeforeClosingWorld;
    QCheckBox* m_confirmBeforeClosingMXPdebug;
    QCheckBox* m_confirmBeforeSavingVariables;

    // === Logging Page ===
    QLineEdit* m_logDirectory;
    QPushButton* m_browseLogDir;
    QCheckBox* m_autoLogWorld;
    QCheckBox* m_appendToLogFiles;
    QCheckBox* m_confirmLogFileClose;

    // === Timers Page ===
    QSpinBox* m_timerInterval;

    // === Activity Page ===
    QCheckBox* m_openActivityWindow;
    QSpinBox* m_activityRefreshInterval;
    QRadioButton* m_refreshOnActivity;
    QRadioButton* m_refreshPeriodically;
    QRadioButton* m_refreshBoth;
    QComboBox* m_activityButtonBarStyle;

    // === Tray Icon Page ===
    QComboBox* m_iconPlacement;
    QRadioButton* m_useMushclientIcon;
    QRadioButton* m_useCustomIcon;
    QLineEdit* m_customIconFile;
    QPushButton* m_browseIconFile;
};

#endif // GLOBAL_PREFERENCES_DIALOG_H
