// world_variables.cpp
// Variable Management Implementation
//
// Implements variable storage, retrieval, and expansion for Mushkin
// Port of: Variable management from original MUSHclient
//
// Provides:
// - getVariable() - Retrieve variable value by name
// - setVariable() - Create or update a variable
// - deleteVariable() - Remove a variable
// - getVariableList() - Get list of all variable names
// - expandVariables() - Expand @variable references in text
//
// Variable System:
// - Case-insensitive storage (all names lowercase)
// - @variable syntax for expansion in triggers/aliases
// - @@ escapes to literal @
// - @!variable disables regex escaping
//
// Based on doc.cpp and evaluate.cpp

#include "../automation/plugin.h"   // For Plugin (arrays context)
#include "../automation/variable.h" // For Variable
#include "../utils/error_codes.h"
#include "logging.h" // For qCDebug(lcWorld)
#include "world_document.h"

// ========== Variable Management ==========

/**
 * getVariable - Retrieve variable value by name
 *
 * Variables are case-insensitive (stored as lowercase).
 * Returns empty QString if variable doesn't exist.
 *
 * GetVariable()
 */
QString WorldDocument::getVariable(const QString& name) const
{
    // Case-insensitive lookup
    QString lowerName = name.toLower();
    const VariableMap& varMap = getVariableMap();

    auto it = varMap.find(lowerName);
    if (it != varMap.end()) {
        return it->second->contents;
    }

    return QString(); // Return empty if not found
}

/**
 * setVariable - Create or update a variable
 *
 * Variables are case-insensitive (stored as lowercase).
 * Creates new variable if doesn't exist, updates if it does.
 * Increments update_number for change tracking.
 *
 * The map key is lowercased for case-insensitive lookup, but the label
 * preserves the original case as supplied by the caller (matching the
 * original MUSHclient behaviour where strLabel = VariableName, not the
 * lower-cased key).
 *
 * SetVariable()
 *
 * @return 0 (eOK) on success
 */
qint32 WorldDocument::setVariable(const QString& name, const QString& value)
{
    // Map key is lowercase for case-insensitive lookup
    QString lowerName = name.toLower();
    VariableMap& varMap = getVariableMap();

    auto it = varMap.find(lowerName);
    Variable* var = nullptr;

    if (it != varMap.end()) {
        // Update existing variable; preserve original case of the new name
        var = it->second.get();
        var->label = name; // update label to reflect caller-supplied case
    } else {
        // Create new variable; label preserves original case (key is lowercase)
        auto newVar = std::make_unique<Variable>();
        newVar->label = name; // original case, not lowerName
        var = newVar.get();
        varMap[lowerName] = std::move(newVar);
    }

    var->contents = value;
    var->update_number++;
    m_bVariablesChanged = true;

    return 0; // eOK
}

/**
 * deleteVariable - Remove a variable by name
 *
 * Variables are case-insensitive (stored as lowercase).
 * Frees memory for the variable.
 *
 * DeleteVariable()
 *
 * @return 0 (eOK) if deleted, 30003 (eVariableNotFound) if not found
 */
qint32 WorldDocument::deleteVariable(const QString& name)
{
    // Case-insensitive lookup
    QString lowerName = name.toLower();
    VariableMap& varMap = getVariableMap();

    auto it = varMap.find(lowerName);
    if (it == varMap.end()) {
        return eVariableNotFound;
    }

    varMap.erase(it); // unique_ptr automatically deletes
    m_bVariablesChanged = true;

    return 0; // eOK
}

/**
 * getVariableList - Get list of all variable names
 *
 * Returns QStringList of all variable names (lowercase).
 *
 * GetVariableList()
 */
QStringList WorldDocument::getVariableList() const
{
    QStringList names;
    const VariableMap& varMap = getVariableMap();
    for (const auto& [name, var] : varMap) {
        names.append(name);
    }
    return names;
}

/**
 * expandVariables - Expand @variable references in text
 *
 * Replaces @variablename with variable contents from current context's variable map.
 * Supports special syntax:
 *   @variable  - Replace with variable value
 *   @@         - Literal @ (escape)
 *   @!variable - Variable without regex escaping (advanced)
 *
 * Variable names: [a-zA-Z0-9_]+ (case-insensitive)
 * If the variable name is empty or the variable does not exist, the @reference
 * is silently dropped (matching original MUSHclient FixSendText behaviour —
 * "non-existent and empty variables will be silently dropped").
 *
 * FixSendText()
 *
 * @param text      Text to expand
 * @param escapeRegex If true, escape special chars in variable values
 * @param isRegexp  True when the trigger match text is a regexp pattern.
 *                  When false and escapeRegex is true, asterisks ('*') are
 *                  dropped from variable values instead of being escaped
 *                  (original non-regexp trigger match-text behaviour).
 * @return Expanded text
 */
QString WorldDocument::expandVariables(const QString& text, bool escapeRegex, bool isRegexp) const
{
    QString result;
    const QChar* pText = text.constData();
    const QChar* pStart = pText;
    const VariableMap& varMap = getVariableMap();

    while (!pText->isNull()) {
        if (*pText == '@') {
            // Copy everything up to the @
            result.append(QString(pStart, pText - pStart));
            pText++; // Skip the @

            // @@ becomes literal @
            if (*pText == '@') {
                result.append('@');
                pText++;
                pStart = pText;
                continue;
            }

            // Check for @! prefix (disables escaping)
            bool shouldEscape = escapeRegex;
            if (*pText == '!') {
                shouldEscape = false;
                pText++;
            }

            // Extract variable name
            const QChar* pNameStart = pText;
            while (!pText->isNull() && (pText->isLetterOrNumber() || *pText == '_')) {
                pText++;
            }

            QString varName(pNameStart, pText - pNameStart);

            if (varName.isEmpty()) {
                // @ not followed by valid variable name — silently drop
                // (original: "@ must be followed by a variable name" / silent drop)
            } else {
                // Look up variable (case-insensitive)
                QString lowerName = varName.toLower();
                auto it = varMap.find(lowerName);
                if (it != varMap.end()) {
                    QString value = it->second->contents;

                    // Escape / filter variable value characters if needed
                    if (shouldEscape) {
                        QString escaped;
                        for (const QChar& ch : value) {
                            // Skip non-printable characters
                            if (ch < ' ')
                                continue;

                            if (isRegexp) {
                                // Regexp trigger: escape all metacharacters
                                // so they are treated as literals in the pattern
                                if (ch == '\\' || ch == '^' || ch == '$' || ch == '.' ||
                                    ch == '|' || ch == '?' || ch == '*' || ch == '+' || ch == '(' ||
                                    ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}') {
                                    escaped.append('\\');
                                }
                                escaped.append(ch);
                            } else {
                                // Non-regexp trigger: drop asterisks, copy everything else
                                // (original: "copy all except asterisks")
                                if (ch != '*')
                                    escaped.append(ch);
                            }
                        }
                        result.append(escaped);
                    } else {
                        // No escaping - just copy the value
                        result.append(value);
                    }
                }
                // else: variable not found — silently drop @variable
                // (original: "Variable '%s' is not defined" with bThrowExceptions,
                //  or silent drop when bThrowExceptions=false)
            }

            pStart = pText;
        } else {
            pText++;
        }
    }

    // Append any remaining text
    result.append(QString(pStart, pText - pStart));
    return result;
}

// ========== Array Management (plugin-aware) ==========

/**
 * getArrayMap - Get the arrays map for the current context
 *
 * Returns the plugin's arrays if executing in plugin context,
 * otherwise returns the world's arrays.
 *
 * GetArrayMap()
 */
ArraysMap& WorldDocument::getArrayMap()
{
    if (m_CurrentPlugin) {
        return m_CurrentPlugin->m_Arrays;
    }
    return m_Arrays;
}

/**
 * getArrayMap (const) - Get the arrays map for the current context (const version)
 */
const ArraysMap& WorldDocument::getArrayMap() const
{
    if (m_CurrentPlugin) {
        return m_CurrentPlugin->m_Arrays;
    }
    return m_Arrays;
}

// ========== Variable Management (plugin-aware) ==========

/**
 * getVariableMap - Get the variable map for the current context
 *
 * Returns the plugin's variables if executing in plugin context,
 * otherwise returns the world's variables.
 *
 * GetVariableMap()
 */
VariableMap& WorldDocument::getVariableMap()
{
    if (m_CurrentPlugin) {
        return m_CurrentPlugin->m_VariableMap;
    }
    return m_VariableMap;
}

/**
 * getVariableMap (const) - Get the variable map for the current context (const version)
 */
const VariableMap& WorldDocument::getVariableMap() const
{
    if (m_CurrentPlugin) {
        return m_CurrentPlugin->m_VariableMap;
    }
    return m_VariableMap;
}
