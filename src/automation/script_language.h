#ifndef SCRIPT_LANGUAGE_H
#define SCRIPT_LANGUAGE_H

#include <QString>

/**
 * ScriptLanguage - Scripting language enum for triggers, aliases, timers, and plugins
 *
 * Mushkin supports multiple scripting languages. YueScript is transpiled to Lua
 * at runtime using the yue.to_lua() function.
 */
enum class ScriptLanguage {
    Lua = 0,       // Standard Lua (default)
    YueScript = 1  // YueScript (transpiled to Lua)
};

/**
 * Convert ScriptLanguage enum to string for XML serialization
 *
 * @param lang Script language enum value
 * @return String representation ("Lua" or "YueScript")
 */
inline QString scriptLanguageToString(ScriptLanguage lang)
{
    switch (lang) {
        case ScriptLanguage::YueScript:
            return QStringLiteral("YueScript");
        default:
            return QStringLiteral("Lua");
    }
}

/**
 * Parse string to ScriptLanguage enum (backward compatible)
 *
 * Unknown or empty strings default to Lua for backward compatibility
 * with existing world files and plugins.
 *
 * @param str String to parse (case insensitive)
 * @return ScriptLanguage enum value
 */
inline ScriptLanguage scriptLanguageFromString(const QString& str)
{
    if (str.compare(QLatin1String("YueScript"), Qt::CaseInsensitive) == 0 ||
        str.compare(QLatin1String("Yue"), Qt::CaseInsensitive) == 0) {
        return ScriptLanguage::YueScript;
    }
    return ScriptLanguage::Lua;
}

/**
 * Get display name for script language (for UI)
 *
 * @param lang Script language enum value
 * @return Human-readable display name
 */
inline QString scriptLanguageDisplayName(ScriptLanguage lang)
{
    switch (lang) {
        case ScriptLanguage::YueScript:
            return QStringLiteral("YueScript");
        default:
            return QStringLiteral("Lua");
    }
}

#endif // SCRIPT_LANGUAGE_H
