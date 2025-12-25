/**
 * global_options.cpp - Global Application Preferences Implementation
 *
 * Loads and saves global preferences using QSettings for platform-native storage.
 *
 * On Windows, this reads/writes to the same Registry location as original MUSHclient
 * for backwards compatibility:
 *   HKEY_CURRENT_USER\Software\Gammon Software Solutions\MUSHclient\Global prefs
 *
 * On macOS/Linux, uses Mushkin-specific paths (no MUSHclient legacy on these platforms):
 *   macOS: ~/Library/Preferences/com.mushkin.Mushkin.plist
 *   Linux: ~/.config/Mushkin/Mushkin.conf
 *
 * Ported from: globalregistryoptions.cpp (original MUSHclient)
 */

#include "global_options.h"
#include <QSettings>

// Windows: Match original MUSHclient for Registry compatibility
// macOS/Linux: Use Mushkin (no legacy to maintain on these platforms)
#ifdef Q_OS_WIN
static const char* SETTINGS_ORG = "Gammon Software Solutions";
static const char* SETTINGS_APP = "MUSHclient";
#else
static const char* SETTINGS_ORG = "Mushkin";
static const char* SETTINGS_APP = "Mushkin";
#endif
static const char* SETTINGS_GROUP = "Global prefs";

GlobalOptions* GlobalOptions::s_instance = nullptr;

GlobalOptions* GlobalOptions::instance()
{
    if (!s_instance) {
        s_instance = new GlobalOptions();
    }
    return s_instance;
}

GlobalOptions::GlobalOptions()
{
    // Options are initialized with defaults in the header
    // Call load() to populate from QSettings
}

QSettings* GlobalOptions::createSettings() const
{
    return new QSettings(SETTINGS_ORG, SETTINGS_APP);
}

void GlobalOptions::load()
{
    QSettings* settings = createSettings();
    settings->beginGroup(SETTINGS_GROUP);

    // ========================================================================
    // LOAD BOOLEAN OPTIONS (stored as integers for Registry compatibility)
    // ========================================================================
    m_allTypingToCommandWindow = settings->value("AllTypingToCommandWindow", 1).toInt() != 0;
    m_alwaysOnTop = settings->value("AlwaysOnTop", 0).toInt() != 0;
    m_openWorldsMaximized = settings->value("OpenWorldsMaximised", 0).toInt() != 0;
    m_appendToLogFiles = settings->value("AppendToLogFiles", 0).toInt() != 0;
    m_autoLogWorld = settings->value("AutoLogWorld", 0).toInt() != 0;
    m_autoConnectWorlds = settings->value("AutoConnectWorlds", 1).toInt() != 0;
    m_notifyIfCannotConnect = settings->value("NotifyIfCannotConnect", 1).toInt() != 0;
    m_notifyOnDisconnect = settings->value("NotifyOnDisconnect", 1).toInt() != 0;
    m_reconnectOnLinkFailure = settings->value("ReconnectOnLinkFailure", 0).toInt() != 0;
    m_confirmBeforeClosingMushclient =
        settings->value("ConfirmBeforeClosingMushclient", 0).toInt() != 0;
    m_confirmBeforeClosingWorld = settings->value("ConfirmBeforeClosingWorld", 1).toInt() != 0;
    m_confirmBeforeSavingVariables =
        settings->value("ConfirmBeforeSavingVariables", 1).toInt() != 0;
    m_confirmLogFileClose = settings->value("ConfirmLogFileClose", 1).toInt() != 0;
    m_triggerRemoveCheck = settings->value("TriggerRemoveCheck", 1).toInt() != 0;
    m_autoExpandConfig = settings->value("AutoExpandConfig", 1).toInt() != 0;
    m_showGridLinesInListViews = settings->value("ShowGridLinesInListViews", 1).toInt() != 0;
    m_smoothScrolling = settings->value("SmoothScrolling", 0).toInt() != 0;
    m_smootherScrolling = settings->value("SmootherScrolling", 0).toInt() != 0;
    m_enablePackageLibrary = settings->value("AllowLoadingDlls", 1).toInt() != 0;
    m_errorNotificationToOutputWindow =
        settings->value("ErrorNotificationToOutputWindow", 1).toInt() != 0;
    m_fixedFontForEditing = settings->value("FixedFontForEditing", 1).toInt() != 0;
    m_notepadWordWrap = settings->value("NotepadWordWrap", 1).toInt() != 0;
    m_tabInsertsTab = settings->value("TabInsertsTabInMultiLineDialogs", 0).toInt() != 0;
    m_regexpMatchEmpty = settings->value("RegexpMatchEmpty", 1).toInt() != 0;
    m_f1Macro = settings->value("F1macro", 0).toInt() != 0;

    // ========================================================================
    // LOAD INTEGER OPTIONS
    // ========================================================================
    m_defaultInputFontHeight = settings->value("DefaultInputFontHeight", 9).toInt();
    m_defaultInputFontWeight = settings->value("DefaultInputFontWeight", 400).toInt();
    m_defaultInputFontItalic = settings->value("DefaultInputFontItalic", 0).toInt();
    m_defaultOutputFontHeight = settings->value("DefaultOutputFontHeight", 9).toInt();
    m_fixedPitchFontSize = settings->value("FixedPitchFontSize", 9).toInt();
    m_notepadBackColour = settings->value("NotepadBackColour", 0).toInt();
    m_notepadTextColour = settings->value("NotepadTextColour", 0).toInt();
    m_printerFontSize = settings->value("PrinterFontSize", 10).toInt();
    m_printerLeftMargin = settings->value("PrinterLeftMargin", 15).toInt();
    m_printerTopMargin = settings->value("PrinterTopMargin", 15).toInt();
    m_printerLinesPerPage = settings->value("PrinterLinesPerPage", 60).toInt();
    m_timerInterval = settings->value("TimerInterval", 0).toInt();
    m_activityWindowRefreshInterval = settings->value("ActivityWindowRefreshInterval", 15).toInt();
    m_activityWindowRefreshType = settings->value("ActivityWindowRefreshType", 0).toInt();
    m_windowTabsStyle = settings->value("WindowTabsStyle", 0).toInt();
    m_iconPlacement = settings->value("IconPlacement", 0).toInt();
    m_trayIcon = settings->value("TrayIcon", 0).toInt();

    // ========================================================================
    // LOAD STRING OPTIONS
    // ========================================================================
    m_defaultLogFileDirectory = settings->value("DefaultLogFileDirectory", "./logs/").toString();
    m_defaultWorldFileDirectory =
        settings->value("DefaultWorldFileDirectory", "./worlds/").toString();
    m_pluginsDirectory = settings->value("PluginsDirectory", "./worlds/plugins/").toString();
    m_stateFilesDirectory =
        settings->value("StateFilesDirectory", "./worlds/plugins/state/").toString();
    m_defaultTriggersFile = settings->value("DefaultTriggersFile", "").toString();
    m_defaultAliasesFile = settings->value("DefaultAliasesFile", "").toString();
    m_defaultTimersFile = settings->value("DefaultTimersFile", "").toString();
    m_defaultMacrosFile = settings->value("DefaultMacrosFile", "").toString();
    m_defaultColoursFile = settings->value("DefaultColoursFile", "").toString();
    m_defaultInputFont = settings->value("DefaultInputFont", "Courier New").toString();
    m_defaultOutputFont = settings->value("DefaultOutputFont", "Courier New").toString();
    m_fixedPitchFont = settings->value("FixedPitchFont", "Courier New").toString();
    m_printerFont = settings->value("PrinterFont", "Courier").toString();
    m_notepadQuoteString = settings->value("NotepadQuoteString", "> ").toString();
    m_wordDelimiters = settings->value("WordDelimiters", ".,()[]\"'").toString();
    m_wordDelimitersDblClick = settings->value("WordDelimitersDblClick", ".,()[]\"'").toString();
    m_luaScript = settings->value("LuaScript", "").toString();
    m_locale = settings->value("Locale", "EN").toString();
    m_trayIconFileName = settings->value("TrayIconFileName", "").toString();

    settings->endGroup();
    delete settings;

    m_loaded = true;
}

void GlobalOptions::save()
{
    QSettings* settings = createSettings();
    settings->beginGroup(SETTINGS_GROUP);

    // ========================================================================
    // SAVE BOOLEAN OPTIONS (as integers for Registry compatibility)
    // ========================================================================
    settings->setValue("AllTypingToCommandWindow", m_allTypingToCommandWindow ? 1 : 0);
    settings->setValue("AlwaysOnTop", m_alwaysOnTop ? 1 : 0);
    settings->setValue("OpenWorldsMaximised", m_openWorldsMaximized ? 1 : 0);
    settings->setValue("AppendToLogFiles", m_appendToLogFiles ? 1 : 0);
    settings->setValue("AutoLogWorld", m_autoLogWorld ? 1 : 0);
    settings->setValue("AutoConnectWorlds", m_autoConnectWorlds ? 1 : 0);
    settings->setValue("NotifyIfCannotConnect", m_notifyIfCannotConnect ? 1 : 0);
    settings->setValue("NotifyOnDisconnect", m_notifyOnDisconnect ? 1 : 0);
    settings->setValue("ReconnectOnLinkFailure", m_reconnectOnLinkFailure ? 1 : 0);
    settings->setValue("ConfirmBeforeClosingMushclient", m_confirmBeforeClosingMushclient ? 1 : 0);
    settings->setValue("ConfirmBeforeClosingWorld", m_confirmBeforeClosingWorld ? 1 : 0);
    settings->setValue("ConfirmBeforeSavingVariables", m_confirmBeforeSavingVariables ? 1 : 0);
    settings->setValue("ConfirmLogFileClose", m_confirmLogFileClose ? 1 : 0);
    settings->setValue("TriggerRemoveCheck", m_triggerRemoveCheck ? 1 : 0);
    settings->setValue("AutoExpandConfig", m_autoExpandConfig ? 1 : 0);
    settings->setValue("ShowGridLinesInListViews", m_showGridLinesInListViews ? 1 : 0);
    settings->setValue("SmoothScrolling", m_smoothScrolling ? 1 : 0);
    settings->setValue("SmootherScrolling", m_smootherScrolling ? 1 : 0);
    settings->setValue("AllowLoadingDlls", m_enablePackageLibrary ? 1 : 0);
    settings->setValue("ErrorNotificationToOutputWindow",
                       m_errorNotificationToOutputWindow ? 1 : 0);
    settings->setValue("FixedFontForEditing", m_fixedFontForEditing ? 1 : 0);
    settings->setValue("NotepadWordWrap", m_notepadWordWrap ? 1 : 0);
    settings->setValue("TabInsertsTabInMultiLineDialogs", m_tabInsertsTab ? 1 : 0);
    settings->setValue("RegexpMatchEmpty", m_regexpMatchEmpty ? 1 : 0);
    settings->setValue("F1macro", m_f1Macro ? 1 : 0);

    // ========================================================================
    // SAVE INTEGER OPTIONS
    // ========================================================================
    settings->setValue("DefaultInputFontHeight", m_defaultInputFontHeight);
    settings->setValue("DefaultInputFontWeight", m_defaultInputFontWeight);
    settings->setValue("DefaultInputFontItalic", m_defaultInputFontItalic);
    settings->setValue("DefaultOutputFontHeight", m_defaultOutputFontHeight);
    settings->setValue("FixedPitchFontSize", m_fixedPitchFontSize);
    settings->setValue("NotepadBackColour", m_notepadBackColour);
    settings->setValue("NotepadTextColour", m_notepadTextColour);
    settings->setValue("PrinterFontSize", m_printerFontSize);
    settings->setValue("PrinterLeftMargin", m_printerLeftMargin);
    settings->setValue("PrinterTopMargin", m_printerTopMargin);
    settings->setValue("PrinterLinesPerPage", m_printerLinesPerPage);
    settings->setValue("TimerInterval", m_timerInterval);
    settings->setValue("ActivityWindowRefreshInterval", m_activityWindowRefreshInterval);
    settings->setValue("ActivityWindowRefreshType", m_activityWindowRefreshType);
    settings->setValue("WindowTabsStyle", m_windowTabsStyle);
    settings->setValue("IconPlacement", m_iconPlacement);
    settings->setValue("TrayIcon", m_trayIcon);

    // ========================================================================
    // SAVE STRING OPTIONS
    // ========================================================================
    settings->setValue("DefaultLogFileDirectory", m_defaultLogFileDirectory);
    settings->setValue("DefaultWorldFileDirectory", m_defaultWorldFileDirectory);
    settings->setValue("PluginsDirectory", m_pluginsDirectory);
    settings->setValue("StateFilesDirectory", m_stateFilesDirectory);
    settings->setValue("DefaultTriggersFile", m_defaultTriggersFile);
    settings->setValue("DefaultAliasesFile", m_defaultAliasesFile);
    settings->setValue("DefaultTimersFile", m_defaultTimersFile);
    settings->setValue("DefaultMacrosFile", m_defaultMacrosFile);
    settings->setValue("DefaultColoursFile", m_defaultColoursFile);
    settings->setValue("DefaultInputFont", m_defaultInputFont);
    settings->setValue("DefaultOutputFont", m_defaultOutputFont);
    settings->setValue("FixedPitchFont", m_fixedPitchFont);
    settings->setValue("PrinterFont", m_printerFont);
    settings->setValue("NotepadQuoteString", m_notepadQuoteString);
    settings->setValue("WordDelimiters", m_wordDelimiters);
    settings->setValue("WordDelimitersDblClick", m_wordDelimitersDblClick);
    settings->setValue("LuaScript", m_luaScript);
    settings->setValue("Locale", m_locale);
    settings->setValue("TrayIconFileName", m_trayIconFileName);

    settings->endGroup();
    settings->sync(); // Ensure changes are written immediately
    delete settings;
}

void GlobalOptions::resetToDefaults()
{
    // Boolean options
    m_allTypingToCommandWindow = true;
    m_alwaysOnTop = false;
    m_openWorldsMaximized = false;
    m_appendToLogFiles = false;
    m_autoLogWorld = false;
    m_autoConnectWorlds = true;
    m_notifyIfCannotConnect = true;
    m_notifyOnDisconnect = true;
    m_reconnectOnLinkFailure = false;
    m_confirmBeforeClosingMushclient = false;
    m_confirmBeforeClosingWorld = true;
    m_confirmBeforeSavingVariables = true;
    m_confirmLogFileClose = true;
    m_triggerRemoveCheck = true;
    m_autoExpandConfig = true;
    m_showGridLinesInListViews = true;
    m_smoothScrolling = false;
    m_smootherScrolling = false;
    m_enablePackageLibrary = true;
    m_errorNotificationToOutputWindow = true;
    m_fixedFontForEditing = true;
    m_notepadWordWrap = true;
    m_tabInsertsTab = false;
    m_regexpMatchEmpty = true;
    m_f1Macro = false;

    // Integer options
    m_defaultInputFontHeight = 9;
    m_defaultInputFontWeight = 400;
    m_defaultInputFontItalic = 0;
    m_defaultOutputFontHeight = 9;
    m_fixedPitchFontSize = 9;
    m_notepadBackColour = 0;
    m_notepadTextColour = 0;
    m_printerFontSize = 10;
    m_printerLeftMargin = 15;
    m_printerTopMargin = 15;
    m_printerLinesPerPage = 60;
    m_timerInterval = 0;
    m_activityWindowRefreshInterval = 15;
    m_activityWindowRefreshType = 0;
    m_windowTabsStyle = 0;
    m_iconPlacement = 0;
    m_trayIcon = 0;

    // String options
    m_defaultLogFileDirectory = "./logs/";
    m_defaultWorldFileDirectory = "./worlds/";
    m_pluginsDirectory = "./worlds/plugins/";
    m_stateFilesDirectory = "./worlds/plugins/state/";
    m_defaultTriggersFile.clear();
    m_defaultAliasesFile.clear();
    m_defaultTimersFile.clear();
    m_defaultMacrosFile.clear();
    m_defaultColoursFile.clear();
    m_defaultInputFont = "Courier New";
    m_defaultOutputFont = "Courier New";
    m_fixedPitchFont = "Courier New";
    m_printerFont = "Courier";
    m_notepadQuoteString = "> ";
    m_wordDelimiters = ".,()[]\"'";
    m_wordDelimitersDblClick = ".,()[]\"'";
    m_luaScript.clear();
    m_locale = "EN";
    m_trayIconFileName.clear();
}
