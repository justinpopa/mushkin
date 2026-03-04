/**
 * global_options.cpp - Global Application Preferences Implementation
 *
 * Loads and saves global preferences using the SQLite database (mushclient_prefs.sqlite).
 * This matches the original MUSHclient's storage mechanism, providing drop-in compatibility
 * with existing MUSHclient preference databases.
 *
 * Ported from: globalregistryoptions.cpp (original MUSHclient)
 */

#include "global_options.h"
#include "database.h"

GlobalOptions& GlobalOptions::instance()
{
    static GlobalOptions instance;
    return instance;
}

GlobalOptions::GlobalOptions()
{
}

void GlobalOptions::load()
{
    auto& db = Database::instance();

    m_allTypingToCommandWindow = db.getPreferenceInt("AllTypingToCommandWindow", 1) != 0;
    m_alwaysOnTop = db.getPreferenceInt("AlwaysOnTop", 0) != 0;
    m_openWorldsMaximized = db.getPreferenceInt("OpenWorldsMaximised", 0) != 0;
    m_appendToLogFiles = db.getPreferenceInt("AppendToLogFiles", 0) != 0;
    m_autoLogWorld = db.getPreferenceInt("AutoLogWorld", 0) != 0;
    m_autoConnectWorlds = db.getPreferenceInt("AutoConnectWorlds", 1) != 0;
    m_notifyIfCannotConnect = db.getPreferenceInt("NotifyIfCannotConnect", 1) != 0;
    m_notifyOnDisconnect = db.getPreferenceInt("NotifyOnDisconnect", 1) != 0;
    m_reconnectOnLinkFailure = db.getPreferenceInt("ReconnectOnLinkFailure", 0) != 0;
    m_confirmBeforeClosingMushclient =
        db.getPreferenceInt("ConfirmBeforeClosingMushclient", 0) != 0;
    m_confirmBeforeClosingWorld = db.getPreferenceInt("ConfirmBeforeClosingWorld", 1) != 0;
    m_confirmBeforeSavingVariables = db.getPreferenceInt("ConfirmBeforeSavingVariables", 1) != 0;
    m_confirmLogFileClose = db.getPreferenceInt("ConfirmLogFileClose", 1) != 0;
    m_triggerRemoveCheck = db.getPreferenceInt("TriggerRemoveCheck", 1) != 0;
    m_autoExpandConfig = db.getPreferenceInt("AutoExpandConfig", 1) != 0;
    m_showGridLinesInListViews = db.getPreferenceInt("ShowGridLinesInListViews", 1) != 0;
    m_smoothScrolling = db.getPreferenceInt("SmoothScrolling", 0) != 0;
    m_smootherScrolling = db.getPreferenceInt("SmootherScrolling", 0) != 0;
    m_enablePackageLibrary = db.getPreferenceInt("AllowLoadingDlls", 1) != 0;
    m_errorNotificationToOutputWindow =
        db.getPreferenceInt("ErrorNotificationToOutputWindow", 1) != 0;
    m_fixedFontForEditing = db.getPreferenceInt("FixedFontForEditing", 1) != 0;
    m_notepadWordWrap = db.getPreferenceInt("NotepadWordWrap", 1) != 0;
    m_tabInsertsTab = db.getPreferenceInt("TabInsertsTabInMultiLineDialogs", 0) != 0;
    m_regexpMatchEmpty = db.getPreferenceInt("RegexpMatchEmpty", 1) != 0;
    m_f1Macro = db.getPreferenceInt("F1macro", 0) != 0;
    m_disableKeyboardMenuActivation = db.getPreferenceInt("DisableKeyboardMenuActivation", 0) != 0;
    m_colourGradientConfig = db.getPreferenceInt("ColourGradientConfig", 1) != 0;
    m_bleedBackground = db.getPreferenceInt("BleedBackground", 0) != 0;
    m_flatToolbars = db.getPreferenceInt("FlatToolbars", 1) != 0;
    m_openActivityWindow = db.getPreferenceInt("OpenActivityWindow", 0) != 0;
    m_confirmBeforeClosingMXPdebug = db.getPreferenceInt("ConfirmBeforeClosingMXPdebug", 0) != 0;

    m_defaultInputFontHeight = db.getPreferenceInt("DefaultInputFontHeight", 9);
    m_defaultInputFontWeight = db.getPreferenceInt("DefaultInputFontWeight", 400);
    m_defaultInputFontItalic = db.getPreferenceInt("DefaultInputFontItalic", 0);
    m_defaultOutputFontHeight = db.getPreferenceInt("DefaultOutputFontHeight", 9);
    m_fixedPitchFontSize = db.getPreferenceInt("FixedPitchFontSize", 9);
    m_notepadBackColour = db.getPreferenceInt("NotepadBackColour", 0);
    m_notepadTextColour = db.getPreferenceInt("NotepadTextColour", 0);
    m_printerFontSize = db.getPreferenceInt("PrinterFontSize", 10);
    m_printerLeftMargin = db.getPreferenceInt("PrinterLeftMargin", 15);
    m_printerTopMargin = db.getPreferenceInt("PrinterTopMargin", 15);
    m_printerLinesPerPage = db.getPreferenceInt("PrinterLinesPerPage", 60);
    m_timerInterval = db.getPreferenceInt("TimerInterval", 0);
    m_activityWindowRefreshInterval = db.getPreferenceInt("ActivityWindowRefreshInterval", 15);
    m_activityWindowRefreshType = db.getPreferenceInt("ActivityWindowRefreshType", 2);
    m_windowTabsStyle = db.getPreferenceInt("WindowTabsStyle", 0);
    m_iconPlacement = db.getPreferenceInt("IconPlacement", 0);
    m_trayIcon = db.getPreferenceInt("TrayIcon", 0);
    m_themeMode = db.getPreferenceInt("ThemeMode", ThemeSystem);
    m_activityButtonBarStyle = db.getPreferenceInt("ActivityButtonBarStyle", 0);
    m_parenMatchFlags = db.getPreferenceInt("ParenMatchFlags", 0x0061);
    m_notepadFontHeight = db.getPreferenceInt("NotepadFontHeight", 10);

    m_defaultLogFileDirectory = db.getPreference("DefaultLogFileDirectory", "./logs/");
    m_defaultWorldFileDirectory = db.getPreference("DefaultWorldFileDirectory", "./worlds/");
    m_pluginsDirectory = db.getPreference("PluginsDirectory", "./worlds/plugins/");
    m_stateFilesDirectory = db.getPreference("StateFilesDirectory", "./worlds/plugins/state/");
    m_defaultTriggersFile = db.getPreference("DefaultTriggersFile", "");
    m_defaultAliasesFile = db.getPreference("DefaultAliasesFile", "");
    m_defaultTimersFile = db.getPreference("DefaultTimersFile", "");
    m_defaultMacrosFile = db.getPreference("DefaultMacrosFile", "");
    m_defaultColoursFile = db.getPreference("DefaultColoursFile", "");
    m_defaultInputFont = db.getPreference("DefaultInputFont", "Courier New");
    m_defaultOutputFont = db.getPreference("DefaultOutputFont", "Courier New");
    m_fixedPitchFont = db.getPreference("FixedPitchFont", "Courier New");
    m_printerFont = db.getPreference("PrinterFont", "Courier");
    m_notepadQuoteString = db.getPreference("NotepadQuoteString", "> ");
    m_wordDelimiters = db.getPreference("WordDelimiters", ".,()[]\"'");
    m_wordDelimitersDblClick = db.getPreference("WordDelimitersDblClick", ".,()[]\"'");
    m_luaScript = db.getPreference("LuaScript", "");
    m_locale = db.getPreference("Locale", "EN");
    m_trayIconFileName = db.getPreference("TrayIconFileName", "");
    m_notepadFontName = db.getPreference("NotepadFont", "Courier");
    m_worldList = db.getPreference("WorldList", "").split('*', Qt::SkipEmptyParts);
    m_globalPluginList = db.getPreference("PluginList", "").split('\n', Qt::SkipEmptyParts);

    m_loaded = true;
}

void GlobalOptions::save()
{
    auto& db = Database::instance();

    db.setPreferenceInt("AllTypingToCommandWindow", m_allTypingToCommandWindow ? 1 : 0);
    db.setPreferenceInt("AlwaysOnTop", m_alwaysOnTop ? 1 : 0);
    db.setPreferenceInt("OpenWorldsMaximised", m_openWorldsMaximized ? 1 : 0);
    db.setPreferenceInt("AppendToLogFiles", m_appendToLogFiles ? 1 : 0);
    db.setPreferenceInt("AutoLogWorld", m_autoLogWorld ? 1 : 0);
    db.setPreferenceInt("AutoConnectWorlds", m_autoConnectWorlds ? 1 : 0);
    db.setPreferenceInt("NotifyIfCannotConnect", m_notifyIfCannotConnect ? 1 : 0);
    db.setPreferenceInt("NotifyOnDisconnect", m_notifyOnDisconnect ? 1 : 0);
    db.setPreferenceInt("ReconnectOnLinkFailure", m_reconnectOnLinkFailure ? 1 : 0);
    db.setPreferenceInt("ConfirmBeforeClosingMushclient", m_confirmBeforeClosingMushclient ? 1 : 0);
    db.setPreferenceInt("ConfirmBeforeClosingWorld", m_confirmBeforeClosingWorld ? 1 : 0);
    db.setPreferenceInt("ConfirmBeforeSavingVariables", m_confirmBeforeSavingVariables ? 1 : 0);
    db.setPreferenceInt("ConfirmLogFileClose", m_confirmLogFileClose ? 1 : 0);
    db.setPreferenceInt("TriggerRemoveCheck", m_triggerRemoveCheck ? 1 : 0);
    db.setPreferenceInt("AutoExpandConfig", m_autoExpandConfig ? 1 : 0);
    db.setPreferenceInt("ShowGridLinesInListViews", m_showGridLinesInListViews ? 1 : 0);
    db.setPreferenceInt("SmoothScrolling", m_smoothScrolling ? 1 : 0);
    db.setPreferenceInt("SmootherScrolling", m_smootherScrolling ? 1 : 0);
    db.setPreferenceInt("AllowLoadingDlls", m_enablePackageLibrary ? 1 : 0);
    db.setPreferenceInt("ErrorNotificationToOutputWindow",
                        m_errorNotificationToOutputWindow ? 1 : 0);
    db.setPreferenceInt("FixedFontForEditing", m_fixedFontForEditing ? 1 : 0);
    db.setPreferenceInt("NotepadWordWrap", m_notepadWordWrap ? 1 : 0);
    db.setPreferenceInt("TabInsertsTabInMultiLineDialogs", m_tabInsertsTab ? 1 : 0);
    db.setPreferenceInt("RegexpMatchEmpty", m_regexpMatchEmpty ? 1 : 0);
    db.setPreferenceInt("F1macro", m_f1Macro ? 1 : 0);
    db.setPreferenceInt("DisableKeyboardMenuActivation", m_disableKeyboardMenuActivation ? 1 : 0);
    db.setPreferenceInt("ColourGradientConfig", m_colourGradientConfig ? 1 : 0);
    db.setPreferenceInt("BleedBackground", m_bleedBackground ? 1 : 0);
    db.setPreferenceInt("FlatToolbars", m_flatToolbars ? 1 : 0);
    db.setPreferenceInt("OpenActivityWindow", m_openActivityWindow ? 1 : 0);
    db.setPreferenceInt("ConfirmBeforeClosingMXPdebug", m_confirmBeforeClosingMXPdebug ? 1 : 0);

    db.setPreferenceInt("DefaultInputFontHeight", m_defaultInputFontHeight);
    db.setPreferenceInt("DefaultInputFontWeight", m_defaultInputFontWeight);
    db.setPreferenceInt("DefaultInputFontItalic", m_defaultInputFontItalic);
    db.setPreferenceInt("DefaultOutputFontHeight", m_defaultOutputFontHeight);
    db.setPreferenceInt("FixedPitchFontSize", m_fixedPitchFontSize);
    db.setPreferenceInt("NotepadBackColour", m_notepadBackColour);
    db.setPreferenceInt("NotepadTextColour", m_notepadTextColour);
    db.setPreferenceInt("PrinterFontSize", m_printerFontSize);
    db.setPreferenceInt("PrinterLeftMargin", m_printerLeftMargin);
    db.setPreferenceInt("PrinterTopMargin", m_printerTopMargin);
    db.setPreferenceInt("PrinterLinesPerPage", m_printerLinesPerPage);
    db.setPreferenceInt("TimerInterval", m_timerInterval);
    db.setPreferenceInt("ActivityWindowRefreshInterval", m_activityWindowRefreshInterval);
    db.setPreferenceInt("ActivityWindowRefreshType", m_activityWindowRefreshType);
    db.setPreferenceInt("WindowTabsStyle", m_windowTabsStyle);
    db.setPreferenceInt("IconPlacement", m_iconPlacement);
    db.setPreferenceInt("TrayIcon", m_trayIcon);
    db.setPreferenceInt("ThemeMode", m_themeMode);
    db.setPreferenceInt("ActivityButtonBarStyle", m_activityButtonBarStyle);
    db.setPreferenceInt("ParenMatchFlags", m_parenMatchFlags);
    db.setPreferenceInt("NotepadFontHeight", m_notepadFontHeight);

    db.setPreference("DefaultLogFileDirectory", m_defaultLogFileDirectory);
    db.setPreference("DefaultWorldFileDirectory", m_defaultWorldFileDirectory);
    db.setPreference("PluginsDirectory", m_pluginsDirectory);
    db.setPreference("StateFilesDirectory", m_stateFilesDirectory);
    db.setPreference("DefaultTriggersFile", m_defaultTriggersFile);
    db.setPreference("DefaultAliasesFile", m_defaultAliasesFile);
    db.setPreference("DefaultTimersFile", m_defaultTimersFile);
    db.setPreference("DefaultMacrosFile", m_defaultMacrosFile);
    db.setPreference("DefaultColoursFile", m_defaultColoursFile);
    db.setPreference("DefaultInputFont", m_defaultInputFont);
    db.setPreference("DefaultOutputFont", m_defaultOutputFont);
    db.setPreference("FixedPitchFont", m_fixedPitchFont);
    db.setPreference("PrinterFont", m_printerFont);
    db.setPreference("NotepadQuoteString", m_notepadQuoteString);
    db.setPreference("WordDelimiters", m_wordDelimiters);
    db.setPreference("WordDelimitersDblClick", m_wordDelimitersDblClick);
    db.setPreference("LuaScript", m_luaScript);
    db.setPreference("Locale", m_locale);
    db.setPreference("TrayIconFileName", m_trayIconFileName);
    db.setPreference("NotepadFont", m_notepadFontName);
    db.setPreference("WorldList", m_worldList.join('*'));
    db.setPreference("PluginList", m_globalPluginList.join('\n'));
}

void GlobalOptions::resetToDefaults()
{
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
    m_disableKeyboardMenuActivation = false;
    m_colourGradientConfig = true;
    m_bleedBackground = false;
    m_flatToolbars = true;
    m_openActivityWindow = false;
    m_confirmBeforeClosingMXPdebug = false;

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
    m_activityWindowRefreshType = 2; // eRefreshBoth
    m_windowTabsStyle = 0;
    m_iconPlacement = 0;
    m_trayIcon = 0;
    m_themeMode = ThemeSystem;
    m_activityButtonBarStyle = 0;
    m_parenMatchFlags = 0x0061;
    m_notepadFontHeight = 10;

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
    m_notepadFontName = "Courier";
    m_worldList.clear();
    m_globalPluginList.clear();
}
