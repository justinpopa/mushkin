/**
 * global_options.h - Global Application Preferences
 *
 * Defines application-wide preferences using QSettings for platform-native storage:
 * - Windows: Registry at HKEY_CURRENT_USER\Software\Gammon Software Solutions\MUSHclient
 * - macOS: ~/Library/Preferences/com.mushkin.Mushkin.plist
 * - Linux: ~/.config/Mushkin/Mushkin.conf
 *
 * On Windows, uses MUSHclient paths for backwards compatibility with original MUSHclient.
 * On macOS/Linux, uses Mushkin-specific paths (no MUSHclient legacy on these platforms).
 *
 * Ported from: globalregistryoptions.cpp (original MUSHclient)
 *
 * Usage:
 *   GlobalOptions* opts = GlobalOptions::instance();
 *   bool autoConnect = opts->autoConnectWorlds();
 *   opts->setAutoConnectWorlds(false);
 *   opts->save();  // Persist to storage
 */

#ifndef GLOBAL_OPTIONS_H
#define GLOBAL_OPTIONS_H

#include <QString>

class QSettings;

// Theme mode enumeration
enum ThemeMode { ThemeLight = 0, ThemeDark = 1, ThemeSystem = 2 };

/**
 * GlobalOptions - Singleton class for application-wide preferences
 *
 * All options are loaded from QSettings on first access and cached.
 * Changes are not persisted until save() is called.
 */
class GlobalOptions {
  public:
    static GlobalOptions* instance();

    // Load all options from QSettings
    void load();

    // Save all options to QSettings
    void save();

    // Reset all options to defaults
    void resetToDefaults();

    // ========================================================================
    // BOOLEAN OPTIONS (stored as integers 0/1 for Registry compatibility)
    // ========================================================================

    // Window behavior
    bool allTypingToCommandWindow() const
    {
        return m_allTypingToCommandWindow;
    }
    void setAllTypingToCommandWindow(bool v)
    {
        m_allTypingToCommandWindow = v;
    }

    bool alwaysOnTop() const
    {
        return m_alwaysOnTop;
    }
    void setAlwaysOnTop(bool v)
    {
        m_alwaysOnTop = v;
    }

    bool openWorldsMaximized() const
    {
        return m_openWorldsMaximized;
    }
    void setOpenWorldsMaximized(bool v)
    {
        m_openWorldsMaximized = v;
    }

    // Logging
    bool appendToLogFiles() const
    {
        return m_appendToLogFiles;
    }
    void setAppendToLogFiles(bool v)
    {
        m_appendToLogFiles = v;
    }

    bool autoLogWorld() const
    {
        return m_autoLogWorld;
    }
    void setAutoLogWorld(bool v)
    {
        m_autoLogWorld = v;
    }

    // Connection
    bool autoConnectWorlds() const
    {
        return m_autoConnectWorlds;
    }
    void setAutoConnectWorlds(bool v)
    {
        m_autoConnectWorlds = v;
    }

    bool notifyIfCannotConnect() const
    {
        return m_notifyIfCannotConnect;
    }
    void setNotifyIfCannotConnect(bool v)
    {
        m_notifyIfCannotConnect = v;
    }

    bool notifyOnDisconnect() const
    {
        return m_notifyOnDisconnect;
    }
    void setNotifyOnDisconnect(bool v)
    {
        m_notifyOnDisconnect = v;
    }

    bool reconnectOnLinkFailure() const
    {
        return m_reconnectOnLinkFailure;
    }
    void setReconnectOnLinkFailure(bool v)
    {
        m_reconnectOnLinkFailure = v;
    }

    // Confirmation dialogs
    bool confirmBeforeClosingMushclient() const
    {
        return m_confirmBeforeClosingMushclient;
    }
    void setConfirmBeforeClosingMushclient(bool v)
    {
        m_confirmBeforeClosingMushclient = v;
    }

    bool confirmBeforeClosingWorld() const
    {
        return m_confirmBeforeClosingWorld;
    }
    void setConfirmBeforeClosingWorld(bool v)
    {
        m_confirmBeforeClosingWorld = v;
    }

    bool confirmBeforeSavingVariables() const
    {
        return m_confirmBeforeSavingVariables;
    }
    void setConfirmBeforeSavingVariables(bool v)
    {
        m_confirmBeforeSavingVariables = v;
    }

    bool confirmLogFileClose() const
    {
        return m_confirmLogFileClose;
    }
    void setConfirmLogFileClose(bool v)
    {
        m_confirmLogFileClose = v;
    }

    bool triggerRemoveCheck() const
    {
        return m_triggerRemoveCheck;
    }
    void setTriggerRemoveCheck(bool v)
    {
        m_triggerRemoveCheck = v;
    }

    // UI options
    bool autoExpandConfig() const
    {
        return m_autoExpandConfig;
    }
    void setAutoExpandConfig(bool v)
    {
        m_autoExpandConfig = v;
    }

    bool showGridLinesInListViews() const
    {
        return m_showGridLinesInListViews;
    }
    void setShowGridLinesInListViews(bool v)
    {
        m_showGridLinesInListViews = v;
    }

    bool smoothScrolling() const
    {
        return m_smoothScrolling;
    }
    void setSmoothScrolling(bool v)
    {
        m_smoothScrolling = v;
    }

    bool smootherScrolling() const
    {
        return m_smootherScrolling;
    }
    void setSmootherScrolling(bool v)
    {
        m_smootherScrolling = v;
    }

    // Scripting
    bool enablePackageLibrary() const
    {
        return m_enablePackageLibrary;
    }
    void setEnablePackageLibrary(bool v)
    {
        m_enablePackageLibrary = v;
    }

    bool errorNotificationToOutputWindow() const
    {
        return m_errorNotificationToOutputWindow;
    }
    void setErrorNotificationToOutputWindow(bool v)
    {
        m_errorNotificationToOutputWindow = v;
    }

    // Editor options
    bool fixedFontForEditing() const
    {
        return m_fixedFontForEditing;
    }
    void setFixedFontForEditing(bool v)
    {
        m_fixedFontForEditing = v;
    }

    bool notepadWordWrap() const
    {
        return m_notepadWordWrap;
    }
    void setNotepadWordWrap(bool v)
    {
        m_notepadWordWrap = v;
    }

    bool tabInsertsTab() const
    {
        return m_tabInsertsTab;
    }
    void setTabInsertsTab(bool v)
    {
        m_tabInsertsTab = v;
    }

    // Regex
    bool regexpMatchEmpty() const
    {
        return m_regexpMatchEmpty;
    }
    void setRegexpMatchEmpty(bool v)
    {
        m_regexpMatchEmpty = v;
    }

    // F1 behavior
    bool f1Macro() const
    {
        return m_f1Macro;
    }
    void setF1Macro(bool v)
    {
        m_f1Macro = v;
    }

    // ========================================================================
    // INTEGER OPTIONS
    // ========================================================================

    // Fonts
    int defaultInputFontHeight() const
    {
        return m_defaultInputFontHeight;
    }
    void setDefaultInputFontHeight(int v)
    {
        m_defaultInputFontHeight = v;
    }

    int defaultInputFontWeight() const
    {
        return m_defaultInputFontWeight;
    }
    void setDefaultInputFontWeight(int v)
    {
        m_defaultInputFontWeight = v;
    }

    int defaultInputFontItalic() const
    {
        return m_defaultInputFontItalic;
    }
    void setDefaultInputFontItalic(int v)
    {
        m_defaultInputFontItalic = v;
    }

    int defaultOutputFontHeight() const
    {
        return m_defaultOutputFontHeight;
    }
    void setDefaultOutputFontHeight(int v)
    {
        m_defaultOutputFontHeight = v;
    }

    int fixedPitchFontSize() const
    {
        return m_fixedPitchFontSize;
    }
    void setFixedPitchFontSize(int v)
    {
        m_fixedPitchFontSize = v;
    }

    // Colors
    int notepadBackColour() const
    {
        return m_notepadBackColour;
    }
    void setNotepadBackColour(int v)
    {
        m_notepadBackColour = v;
    }

    int notepadTextColour() const
    {
        return m_notepadTextColour;
    }
    void setNotepadTextColour(int v)
    {
        m_notepadTextColour = v;
    }

    // Printing
    int printerFontSize() const
    {
        return m_printerFontSize;
    }
    void setPrinterFontSize(int v)
    {
        m_printerFontSize = v;
    }

    int printerLeftMargin() const
    {
        return m_printerLeftMargin;
    }
    void setPrinterLeftMargin(int v)
    {
        m_printerLeftMargin = v;
    }

    int printerTopMargin() const
    {
        return m_printerTopMargin;
    }
    void setPrinterTopMargin(int v)
    {
        m_printerTopMargin = v;
    }

    int printerLinesPerPage() const
    {
        return m_printerLinesPerPage;
    }
    void setPrinterLinesPerPage(int v)
    {
        m_printerLinesPerPage = v;
    }

    // Timers
    int timerInterval() const
    {
        return m_timerInterval;
    }
    void setTimerInterval(int v)
    {
        m_timerInterval = v;
    }

    // Activity window
    int activityWindowRefreshInterval() const
    {
        return m_activityWindowRefreshInterval;
    }
    void setActivityWindowRefreshInterval(int v)
    {
        m_activityWindowRefreshInterval = v;
    }

    int activityWindowRefreshType() const
    {
        return m_activityWindowRefreshType;
    }
    void setActivityWindowRefreshType(int v)
    {
        m_activityWindowRefreshType = v;
    }

    // UI styles
    int windowTabsStyle() const
    {
        return m_windowTabsStyle;
    }
    void setWindowTabsStyle(int v)
    {
        m_windowTabsStyle = v;
    }

    int iconPlacement() const
    {
        return m_iconPlacement;
    }
    void setIconPlacement(int v)
    {
        m_iconPlacement = v;
    }

    int trayIcon() const
    {
        return m_trayIcon;
    }
    void setTrayIcon(int v)
    {
        m_trayIcon = v;
    }

    // Theme (0=Light, 1=Dark, 2=System)
    int themeMode() const
    {
        return m_themeMode;
    }
    void setThemeMode(int v)
    {
        m_themeMode = v;
    }

    // ========================================================================
    // STRING OPTIONS
    // ========================================================================

    // Directories
    QString defaultLogFileDirectory() const
    {
        return m_defaultLogFileDirectory;
    }
    void setDefaultLogFileDirectory(const QString& v)
    {
        m_defaultLogFileDirectory = v;
    }

    QString defaultWorldFileDirectory() const
    {
        return m_defaultWorldFileDirectory;
    }
    void setDefaultWorldFileDirectory(const QString& v)
    {
        m_defaultWorldFileDirectory = v;
    }

    QString pluginsDirectory() const
    {
        return m_pluginsDirectory;
    }
    void setPluginsDirectory(const QString& v)
    {
        m_pluginsDirectory = v;
    }

    QString stateFilesDirectory() const
    {
        return m_stateFilesDirectory;
    }
    void setStateFilesDirectory(const QString& v)
    {
        m_stateFilesDirectory = v;
    }

    // Default files
    QString defaultTriggersFile() const
    {
        return m_defaultTriggersFile;
    }
    void setDefaultTriggersFile(const QString& v)
    {
        m_defaultTriggersFile = v;
    }

    QString defaultAliasesFile() const
    {
        return m_defaultAliasesFile;
    }
    void setDefaultAliasesFile(const QString& v)
    {
        m_defaultAliasesFile = v;
    }

    QString defaultTimersFile() const
    {
        return m_defaultTimersFile;
    }
    void setDefaultTimersFile(const QString& v)
    {
        m_defaultTimersFile = v;
    }

    QString defaultMacrosFile() const
    {
        return m_defaultMacrosFile;
    }
    void setDefaultMacrosFile(const QString& v)
    {
        m_defaultMacrosFile = v;
    }

    QString defaultColoursFile() const
    {
        return m_defaultColoursFile;
    }
    void setDefaultColoursFile(const QString& v)
    {
        m_defaultColoursFile = v;
    }

    // Fonts
    QString defaultInputFont() const
    {
        return m_defaultInputFont;
    }
    void setDefaultInputFont(const QString& v)
    {
        m_defaultInputFont = v;
    }

    QString defaultOutputFont() const
    {
        return m_defaultOutputFont;
    }
    void setDefaultOutputFont(const QString& v)
    {
        m_defaultOutputFont = v;
    }

    QString fixedPitchFont() const
    {
        return m_fixedPitchFont;
    }
    void setFixedPitchFont(const QString& v)
    {
        m_fixedPitchFont = v;
    }

    QString printerFont() const
    {
        return m_printerFont;
    }
    void setPrinterFont(const QString& v)
    {
        m_printerFont = v;
    }

    // Editor
    QString notepadQuoteString() const
    {
        return m_notepadQuoteString;
    }
    void setNotepadQuoteString(const QString& v)
    {
        m_notepadQuoteString = v;
    }

    // Word delimiters
    QString wordDelimiters() const
    {
        return m_wordDelimiters;
    }
    void setWordDelimiters(const QString& v)
    {
        m_wordDelimiters = v;
    }

    QString wordDelimitersDblClick() const
    {
        return m_wordDelimitersDblClick;
    }
    void setWordDelimitersDblClick(const QString& v)
    {
        m_wordDelimitersDblClick = v;
    }

    // Scripting
    QString luaScript() const
    {
        return m_luaScript;
    }
    void setLuaScript(const QString& v)
    {
        m_luaScript = v;
    }

    // Locale
    QString locale() const
    {
        return m_locale;
    }
    void setLocale(const QString& v)
    {
        m_locale = v;
    }

    // Tray icon
    QString trayIconFileName() const
    {
        return m_trayIconFileName;
    }
    void setTrayIconFileName(const QString& v)
    {
        m_trayIconFileName = v;
    }

  private:
    GlobalOptions();
    ~GlobalOptions() = default;

    // Prevent copying
    GlobalOptions(const GlobalOptions&) = delete;
    GlobalOptions& operator=(const GlobalOptions&) = delete;

    // Helper to get QSettings configured for MUSHclient Registry path
    QSettings* createSettings() const;

    static GlobalOptions* s_instance;
    bool m_loaded = false;

    // ========================================================================
    // BOOLEAN MEMBERS (with defaults)
    // ========================================================================
    bool m_allTypingToCommandWindow = true;
    bool m_alwaysOnTop = false;
    bool m_openWorldsMaximized = false;
    bool m_appendToLogFiles = false;
    bool m_autoLogWorld = false;
    bool m_autoConnectWorlds = true;
    bool m_notifyIfCannotConnect = true;
    bool m_notifyOnDisconnect = true;
    bool m_reconnectOnLinkFailure = false;
    bool m_confirmBeforeClosingMushclient = false;
    bool m_confirmBeforeClosingWorld = true;
    bool m_confirmBeforeSavingVariables = true;
    bool m_confirmLogFileClose = true;
    bool m_triggerRemoveCheck = true;
    bool m_autoExpandConfig = true;
    bool m_showGridLinesInListViews = true;
    bool m_smoothScrolling = false;
    bool m_smootherScrolling = false;
    bool m_enablePackageLibrary = true;
    bool m_errorNotificationToOutputWindow = true;
    bool m_fixedFontForEditing = true;
    bool m_notepadWordWrap = true;
    bool m_tabInsertsTab = false;
    bool m_regexpMatchEmpty = true;
    bool m_f1Macro = false;

    // ========================================================================
    // INTEGER MEMBERS (with defaults)
    // ========================================================================
    int m_defaultInputFontHeight = 9;
    int m_defaultInputFontWeight = 400; // FW_NORMAL
    int m_defaultInputFontItalic = 0;
    int m_defaultOutputFontHeight = 9;
    int m_fixedPitchFontSize = 9;
    int m_notepadBackColour = 0;
    int m_notepadTextColour = 0;
    int m_printerFontSize = 10;
    int m_printerLeftMargin = 15;
    int m_printerTopMargin = 15;
    int m_printerLinesPerPage = 60;
    int m_timerInterval = 0;
    int m_activityWindowRefreshInterval = 15;
    int m_activityWindowRefreshType = 0; // eRefreshBoth
    int m_windowTabsStyle = 0;
    int m_iconPlacement = 0; // ICON_PLACEMENT_TASKBAR
    int m_trayIcon = 0;
    int m_themeMode = ThemeSystem; // Default to system theme

    // ========================================================================
    // STRING MEMBERS (with defaults)
    // ========================================================================
    QString m_defaultLogFileDirectory = "./logs/";
    QString m_defaultWorldFileDirectory = "./worlds/";
    QString m_pluginsDirectory = "./worlds/plugins/";
    QString m_stateFilesDirectory = "./worlds/plugins/state/";
    QString m_defaultTriggersFile;
    QString m_defaultAliasesFile;
    QString m_defaultTimersFile;
    QString m_defaultMacrosFile;
    QString m_defaultColoursFile;
    QString m_defaultInputFont = "Courier New";
    QString m_defaultOutputFont = "Courier New";
    QString m_fixedPitchFont = "Courier New";
    QString m_printerFont = "Courier";
    QString m_notepadQuoteString = "> ";
    QString m_wordDelimiters = ".,()[]\"'";
    QString m_wordDelimitersDblClick = ".,()[]\"'";
    QString m_luaScript;
    QString m_locale = "EN";
    QString m_trayIconFileName;
};

#endif // GLOBAL_OPTIONS_H
