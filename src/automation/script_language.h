#ifndef SCRIPT_LANGUAGE_H
#define SCRIPT_LANGUAGE_H

#include <QString>

/**
 * ScriptLanguage - Scripting language enum for triggers, aliases, timers, and plugins
 *
 * Mushkin supports multiple scripting languages that transpile to Lua at runtime:
 * - YueScript: Clean syntax (like CoffeeScript for Lua), MoonScript derivative
 * - MoonScript: Original clean syntax language (YueScript predecessor)
 * - Teal: Static typing (like TypeScript for Lua)
 * - Fennel: Lisp syntax that compiles to Lua
 */
enum class ScriptLanguage {
    Lua = 0,        // Standard Lua (default)
    YueScript = 1,  // YueScript (transpiled to Lua)
    Teal = 2,       // Teal (typed Lua, transpiled)
    Fennel = 3,     // Fennel (Lisp syntax, transpiled)
    MoonScript = 4  // MoonScript (transpiled to Lua)
};

/**
 * Convert ScriptLanguage enum to string for XML serialization
 *
 * @param lang Script language enum value
 * @return String representation ("Lua", "YueScript", "Teal", "Fennel", or "MoonScript")
 */
inline QString scriptLanguageToString(ScriptLanguage lang)
{
    switch (lang) {
        case ScriptLanguage::YueScript:
            return QStringLiteral("YueScript");
        case ScriptLanguage::Teal:
            return QStringLiteral("Teal");
        case ScriptLanguage::Fennel:
            return QStringLiteral("Fennel");
        case ScriptLanguage::MoonScript:
            return QStringLiteral("MoonScript");
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
    if (str.compare(QLatin1String("Teal"), Qt::CaseInsensitive) == 0 ||
        str.compare(QLatin1String("tl"), Qt::CaseInsensitive) == 0) {
        return ScriptLanguage::Teal;
    }
    if (str.compare(QLatin1String("Fennel"), Qt::CaseInsensitive) == 0 ||
        str.compare(QLatin1String("fnl"), Qt::CaseInsensitive) == 0) {
        return ScriptLanguage::Fennel;
    }
    if (str.compare(QLatin1String("MoonScript"), Qt::CaseInsensitive) == 0 ||
        str.compare(QLatin1String("moon"), Qt::CaseInsensitive) == 0) {
        return ScriptLanguage::MoonScript;
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
        case ScriptLanguage::Teal:
            return QStringLiteral("Teal");
        case ScriptLanguage::Fennel:
            return QStringLiteral("Fennel");
        case ScriptLanguage::MoonScript:
            return QStringLiteral("MoonScript");
        default:
            return QStringLiteral("Lua");
    }
}

#endif // SCRIPT_LANGUAGE_H
