// variable.h
// Variable data structure for plugin state persistence
//
// Port of CVariable from OtherTypes.h
// Simple key-value storage for plugin variables

#ifndef VARIABLE_H
#define VARIABLE_H

#include <QMap>
#include <QString>
#include <map>
#include <memory>

/**
 * Variable - Simple key-value storage for scripts
 *
 * Based on CVariable from OtherTypes.h
 *
 * Variables store persistent script state:
 * - Player stats (hp, mana, gold)
 * - Configuration (preferences, thresholds)
 * - State tracking (quest progress, cooldowns)
 * - Cross-script communication
 *
 * Variables persist:
 * - During session (in memory)
 * - Across sessions (saved to .state file)
 * - Accessible from all scripts in same context (world or plugin)
 *
 * Stories:
 * - 9.5: State persistence (SaveState/LoadState)
 * - 4.2: Variable API (GetVariable, SetVariable, DeleteVariable)
 */
class Variable {
  public:
    /**
     * Constructor - initializes to defaults
     */
    Variable() : nUpdateNumber(0), bSelected(false)
    {
    }

    // ========== Fields ==========

    QString strLabel;    // Variable name (key)
    QString strContents; // Variable value (always string)

    // ========== Runtime Fields ==========

    qint64 nUpdateNumber; // For detecting update clashes
    bool bSelected;       // If true, selected for use in a plugin
};

// Type definitions
typedef std::map<QString, std::unique_ptr<Variable>> VariableMap;

// Arrays: nested map for Lua table persistence
// Outer map: array name → inner map
// Inner map: key → value (both strings)
typedef QMap<QString, QMap<QString, QString>> ArraysMap;

#endif // VARIABLE_H
