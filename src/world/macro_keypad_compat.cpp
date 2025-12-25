/**
 * @file macro_keypad_compat.cpp
 * @brief Implementation of macro/keypad compatibility layer
 */

#include "macro_keypad_compat.h"
#include "../automation/sendto.h"
#include <QHash>

namespace MacroKeypadCompat {

// ============================================================================
// Static Mapping Tables
// ============================================================================

// Hash tables for fast lookup (initialized once)
static QHash<QString, QString> s_macroNameToKey;  // "F2+Ctrl" -> "Ctrl+F2"
static QHash<QString, QString> s_keyToMacroName;  // "Ctrl+F2" -> "F2+Ctrl"
static QHash<QString, QString> s_keypadNameToKey; // "8" -> "Num+8"
static QHash<QString, QString> s_keyToKeypadName; // "Num+8" -> "8"
static bool s_initialized = false;

// Original macro names from MUSHclient (in order of MAC_* enum)
// Note: First 15 are "command" names, not actual key bindings
static const char* s_macroDescriptions[] = {
    // Command names (indices 0-14) - these don't have fixed key bindings
    "up", "down", "north", "south", "east", "west", "examine", "look", "page", "say", "whisper",
    "doing", "who", "drop", "take",
    // Function keys (indices 15-24)
    "F2", "F3", "F4", "F5", "F7", "F8", "F9", "F10", "F11", "F12",
    // Shift+Function keys (indices 25-35)
    "F2+Shift", "F3+Shift", "F4+Shift", "F5+Shift", "F6+Shift", "F7+Shift", "F8+Shift", "F9+Shift",
    "F10+Shift", "F11+Shift", "F12+Shift",
    // Ctrl+Function keys (indices 36-44)
    "F2+Ctrl", "F3+Ctrl", "F5+Ctrl", "F7+Ctrl", "F8+Ctrl", "F9+Ctrl", "F10+Ctrl", "F11+Ctrl",
    "F12+Ctrl",
    // More command names (indices 45-46)
    "logout", "quit",
    // Alt+Letter keys (indices 47-63)
    "Alt+A", "Alt+B", "Alt+J", "Alt+K", "Alt+L", "Alt+M", "Alt+N", "Alt+O", "Alt+P", "Alt+Q",
    "Alt+R", "Alt+S", "Alt+T", "Alt+U", "Alt+X", "Alt+Y", "Alt+Z",
    // Added in v3.44 (indices 64-68)
    "F1", "F1+Ctrl", "F1+Shift", "F6", "F6+Ctrl"};

// Keypad names from MUSHclient
static const char* s_keypadNames[] = {
    // Normal numpad keys (indices 0-14)
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "/", "*", "-", "+",
    // Ctrl+numpad keys (indices 15-29)
    "Ctrl+0", "Ctrl+1", "Ctrl+2", "Ctrl+3", "Ctrl+4", "Ctrl+5", "Ctrl+6", "Ctrl+7", "Ctrl+8",
    "Ctrl+9", "Ctrl+.", "Ctrl+/", "Ctrl+*", "Ctrl+-", "Ctrl++"};

// ============================================================================
// Macro Type Conversion
// ============================================================================

int macroTypeToSendTo(const QString& macroType)
{
    if (macroType == "send_now") {
        return eSendToWorld; // 0 - send directly to MUD
    } else if (macroType == "replace") {
        return eSendToCommand; // 1 - put in command line
    } else if (macroType == "insert") {
        return eSendToCommand; // 1 - best approximation (no append mode)
    }
    return eSendToWorld; // Default
}

QString sendToToMacroType(int sendTo)
{
    switch (sendTo) {
        case eSendToWorld:
            return "send_now";
        case eSendToCommand:
            return "replace"; // Could be "insert" but we default to replace
        default:
            return QString(); // Not a macro-compatible sendTo
    }
}

// ============================================================================
// Macro Name Conversion Helpers
// ============================================================================

// Convert original format "F2+Ctrl" to Qt format "Ctrl+F2"
static QString originalToQtFormat(const QString& original)
{
    // Handle "Fn+Modifier" format -> "Modifier+Fn"
    if (original.contains("+")) {
        QStringList parts = original.split("+");
        if (parts.size() == 2) {
            QString key = parts[0];
            QString mod = parts[1];
            // Check if first part is the key (F1-F12)
            if (key.startsWith("F") && key.mid(1).toInt() > 0) {
                return mod + "+" + key;
            }
            // Already in correct format (Alt+A, Ctrl+0)
            return original;
        }
    }
    return original;
}

// Convert Qt format "Ctrl+F2" to original format "F2+Ctrl"
static QString qtToOriginalFormat(const QString& qtFormat)
{
    if (qtFormat.contains("+")) {
        QStringList parts = qtFormat.split("+");
        if (parts.size() == 2) {
            QString first = parts[0];
            QString second = parts[1];
            // Check if second part is F-key
            if (second.startsWith("F") && second.mid(1).toInt() > 0) {
                // Ctrl+F2 -> F2+Ctrl, Shift+F2 -> F2+Shift
                return second + "+" + first;
            }
        }
    }
    return qtFormat;
}

// ============================================================================
// Initialization
// ============================================================================

void initMappingTables()
{
    if (s_initialized)
        return;

    // Build macro mappings
    for (size_t i = 0; i < sizeof(s_macroDescriptions) / sizeof(s_macroDescriptions[0]); i++) {
        QString origName = QString::fromLatin1(s_macroDescriptions[i]);
        QString qtKey = originalToQtFormat(origName);

        // Skip command names that aren't actual key bindings
        // (up, down, north, south, east, west, examine, look, page, say,
        //  whisper, doing, who, drop, take, logout, quit)
        if (!origName.startsWith("F") && !origName.startsWith("Alt+")) {
            // These are named command slots, not key bindings
            // We still add them to allow loading, but they map to themselves
            s_macroNameToKey.insert(origName, origName);
            s_keyToMacroName.insert(origName, origName);
            continue;
        }

        s_macroNameToKey.insert(origName, qtKey);
        s_keyToMacroName.insert(qtKey, origName);

        // Also add lowercase version for case-insensitive matching
        s_macroNameToKey.insert(origName.toLower(), qtKey);
    }

    // Build keypad mappings
    // Qt uses different names for numpad keys
    static const struct {
        const char* origName;
        const char* qtName;
    } keypadMappings[] = {// Normal numpad
                          {"0", "Num+0"},
                          {"1", "Num+1"},
                          {"2", "Num+2"},
                          {"3", "Num+3"},
                          {"4", "Num+4"},
                          {"5", "Num+5"},
                          {"6", "Num+6"},
                          {"7", "Num+7"},
                          {"8", "Num+8"},
                          {"9", "Num+9"},
                          {".", "Num+."},
                          {"/", "Num+/"},
                          {"*", "Num+*"},
                          {"-", "Num+-"},
                          {"+", "Num++"},
                          // Ctrl+numpad
                          {"Ctrl+0", "Ctrl+Num+0"},
                          {"Ctrl+1", "Ctrl+Num+1"},
                          {"Ctrl+2", "Ctrl+Num+2"},
                          {"Ctrl+3", "Ctrl+Num+3"},
                          {"Ctrl+4", "Ctrl+Num+4"},
                          {"Ctrl+5", "Ctrl+Num+5"},
                          {"Ctrl+6", "Ctrl+Num+6"},
                          {"Ctrl+7", "Ctrl+Num+7"},
                          {"Ctrl+8", "Ctrl+Num+8"},
                          {"Ctrl+9", "Ctrl+Num+9"},
                          {"Ctrl+.", "Ctrl+Num+."},
                          {"Ctrl+/", "Ctrl+Num+/"},
                          {"Ctrl+*", "Ctrl+Num+*"},
                          {"Ctrl+-", "Ctrl+Num+-"},
                          {"Ctrl++", "Ctrl+Num++"}};

    for (const auto& mapping : keypadMappings) {
        QString origName = QString::fromLatin1(mapping.origName);
        QString qtKey = QString::fromLatin1(mapping.qtName);
        s_keypadNameToKey.insert(origName, qtKey);
        s_keyToKeypadName.insert(qtKey, origName);
    }

    s_initialized = true;
}

// ============================================================================
// Macro Name <-> Key String Conversion
// ============================================================================

QString macroNameToKeyString(const QString& macroName)
{
    initMappingTables();
    return s_macroNameToKey.value(macroName, QString());
}

QString keyStringToMacroName(const QString& keyString)
{
    initMappingTables();

    // First try direct lookup
    QString result = s_keyToMacroName.value(keyString);
    if (!result.isEmpty()) {
        return result;
    }

    // Try converting to original format
    QString origFormat = qtToOriginalFormat(keyString);
    return s_keyToMacroName.value(origFormat, QString());
}

bool isMacroKey(const QString& keyString)
{
    return !keyStringToMacroName(keyString).isEmpty();
}

// ============================================================================
// Keypad Name <-> Key String Conversion
// ============================================================================

QString keypadNameToKeyString(const QString& keypadName)
{
    initMappingTables();
    return s_keypadNameToKey.value(keypadName, QString());
}

QString keyStringToKeypadName(const QString& keyString)
{
    initMappingTables();
    return s_keyToKeypadName.value(keyString, QString());
}

bool isKeypadKey(const QString& keyString)
{
    return !keyStringToKeypadName(keyString).isEmpty();
}

} // namespace MacroKeypadCompat
