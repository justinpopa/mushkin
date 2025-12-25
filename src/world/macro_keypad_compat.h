/**
 * @file macro_keypad_compat.h
 * @brief Compatibility layer for original MUSHclient macros and keypad XML format
 *
 * The original MUSHclient has three keyboard binding systems:
 * 1. Macros - 64 fixed slots for F-keys, Ctrl+Fn, Shift+Fn, Alt+letters, and named commands
 * 2. Keypad - 30 fixed slots for numpad keys (normal and Ctrl+ variants)
 * 3. Accelerators - Runtime-only, not saved to XML
 *
 * mushclient-qt unifies these into a single AcceleratorManager system.
 * This file provides bidirectional conversion between the formats.
 */

#ifndef MACRO_KEYPAD_COMPAT_H
#define MACRO_KEYPAD_COMPAT_H

#include <QHash>
#include <QKeySequence>
#include <QString>
#include <optional>

namespace MacroKeypadCompat {

// ============================================================================
// Macro Type Mapping (original type attribute <-> sendTo value)
// ============================================================================

// Original macro types from MUSHclient
enum MacroType {
    MacroTypeReplace = 0, // "replace" - replace command line text
    MacroTypeSendNow = 1, // "send_now" - send directly to world
    MacroTypeInsert = 2   // "insert" - append to command line
};

// Map macro type string to sendTo value
// "send_now" -> eSendToWorld (0)
// "replace"  -> eSendToCommand (1)
// "insert"   -> eSendToCommand (1) - no exact match, use command
int macroTypeToSendTo(const QString& macroType);

// Map sendTo value back to macro type string for saving
// Returns empty string if the sendTo doesn't map to a macro type
QString sendToToMacroType(int sendTo);

// ============================================================================
// Macro Name <-> Qt Key Sequence Mapping
// ============================================================================

struct MacroMapping {
    const char* name;           // Original name: "F1", "F2+Ctrl", "Alt+A", etc.
    Qt::Key key;                // Qt key code
    Qt::KeyboardModifiers mods; // Qt modifiers
};

// Get Qt key sequence string from original macro name
// e.g., "F1" -> "F1", "F2+Ctrl" -> "Ctrl+F2", "Alt+A" -> "Alt+A"
QString macroNameToKeyString(const QString& macroName);

// Get original macro name from Qt key sequence string
// Returns empty string if the key doesn't map to a known macro slot
QString keyStringToMacroName(const QString& keyString);

// Check if a key string corresponds to a known macro slot
bool isMacroKey(const QString& keyString);

// ============================================================================
// Keypad Name <-> Qt Key Sequence Mapping
// ============================================================================

// Get Qt key sequence string from original keypad name
// e.g., "8" -> "Num+8", "/" -> "Num+/", "Ctrl+5" -> "Ctrl+Num+5"
QString keypadNameToKeyString(const QString& keypadName);

// Get original keypad name from Qt key sequence string
// Returns empty string if the key doesn't map to a known keypad slot
QString keyStringToKeypadName(const QString& keyString);

// Check if a key string corresponds to a known keypad slot
bool isKeypadKey(const QString& keyString);

// ============================================================================
// Initialization
// ============================================================================

// Initialize the mapping tables (call once at startup)
void initMappingTables();

} // namespace MacroKeypadCompat

#endif // MACRO_KEYPAD_COMPAT_H
