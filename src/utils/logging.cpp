#include "logging.h"

// Define logging categories with default levels
// Third parameter sets the default threshold:
//   QtDebugMsg    = Debug and above (very verbose)
//   QtInfoMsg     = Info and above
//   QtWarningMsg  = Warnings and critical only (quiet for production)
//   QtCriticalMsg = Only critical messages

// Core systems - default to warnings only (quiet)
Q_LOGGING_CATEGORY(lcWorld, "mushkin.world", QtWarningMsg)
Q_LOGGING_CATEGORY(lcNetwork, "mushkin.network", QtWarningMsg)
Q_LOGGING_CATEGORY(lcScript, "mushkin.script", QtWarningMsg)
Q_LOGGING_CATEGORY(lcAutomation, "mushkin.automation", QtWarningMsg)

// UI systems - default to warnings only (quiet)
Q_LOGGING_CATEGORY(lcUI, "mushkin.ui", QtWarningMsg)
Q_LOGGING_CATEGORY(lcDialog, "mushkin.ui.dialog", QtWarningMsg)

// Subsystems - default to warnings only (quiet)
Q_LOGGING_CATEGORY(lcPlugin, "mushkin.plugin", QtWarningMsg)
Q_LOGGING_CATEGORY(lcMiniwindow, "mushkin.miniwindow", QtWarningMsg)
Q_LOGGING_CATEGORY(lcText, "mushkin.text", QtWarningMsg)
Q_LOGGING_CATEGORY(lcLogging, "mushkin.logging", QtWarningMsg)
Q_LOGGING_CATEGORY(lcStorage, "mushkin.storage", QtWarningMsg)
Q_LOGGING_CATEGORY(lcTest, "mushkin.test", QtWarningMsg)
Q_LOGGING_CATEGORY(lcMXP, "mushkin.mxp", QtWarningMsg)
