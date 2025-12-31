/**
 * world_alias_execution.cpp - Alias Matching and Execution
 *
 * Implements what happens when user types a command:
 * - Check aliases in sequence order
 * - Match against wildcards or regex patterns
 * - Capture wildcards (%1, %2, etc.)
 * - Execute actions (send to world, call script, etc.)
 * - Echo command if requested
 * - Add to command history
 */

#include "../automation/alias.h"
#include "../automation/plugin.h"
#include "../automation/sendto.h"
#include "script_engine.h"
#include "world_document.h"
#include <QDebug>

#include "logging.h"

// Lua headers for manual table creation
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

/**
 * evaluateOneAliasSequence - Helper to evaluate one alias sequence
 *
 * Plugin Evaluation Order
 * Helper function that evaluates a single alias array (world or plugin).
 *
 * @param aliasArray Array of aliases to evaluate
 * @param command The user's command
 * @param doc The world document
 * @param anyMatched Output: set to true if any alias matched
 * @return true if evaluation should stop (alias matched and !bKeepEvaluating)
 */
static bool evaluateOneAliasSequence(const QVector<Alias*>& aliasArray, const QString& command,
                                     WorldDocument* doc, bool& anyMatched)
{
    // Check each alias in sequence order
    for (Alias* alias : aliasArray) {
        // Skip disabled aliases
        if (!alias->bEnabled) {
            continue;
        }

        // Try to match alias pattern
        bool matched = alias->match(command);

        if (matched) {
            qDebug() << "Alias MATCHED:" << alias->strLabel << "pattern:" << alias->name
                     << "script:" << alias->strProcedure;

            // Track that at least one alias matched
            anyMatched = true;

            // Execute the alias
            doc->executeAlias(alias, command);

            // If alias doesn't want to keep evaluating, we're done
            if (!alias->bKeepEvaluating) {
                return true; // Stop evaluation
            }
        }
    }

    return false; // Continue evaluation
}

/**
 * evaluateAliases - Check command against all aliases
 *
 * Alias matching and execution
 * Plugin evaluation order (negative → world → positive)
 *
 * Called when user presses Enter in command window. Checks all enabled
 * aliases in sequence order. If an alias matches, executes it and optionally
 * stops evaluation (if bKeepEvaluating is false).
 *
 * Based on evaluate.cpp in original MUSHclient.
 * Evaluation order (suggested by Fiendish, added in v4.97):
 * 1. Plugins with negative sequence (< 0)
 * 2. World aliases
 * 3. Plugins with zero/positive sequence (>= 0)
 *
 * @param command User's typed command
 * @return true if alias handled the command, false if should send to MUD
 */
bool WorldDocument::evaluateAliases(const QString& command)
{
    // Empty command - nothing to do
    if (command.isEmpty()) {
        return false;
    }

    // Debug: show what command we're evaluating
    qDebug() << "evaluateAliases: command =" << command;

    // Rebuild alias array if needed
    if (m_aliasesNeedSorting) {
        rebuildAliasArray();
    }

    // Save current plugin context
    Plugin* savedPlugin = m_CurrentPlugin;
    m_CurrentPlugin = nullptr;

    bool stopEvaluation = false;
    bool anyMatched = false; // Track if ANY alias matched (regardless of bKeepEvaluating)

    // ========== Phase 1: Evaluate plugins with NEGATIVE sequence ==========
    // These plugins get to see the command BEFORE the world aliases
    for (const auto& plugin : m_PluginList) {
        if (plugin->m_iSequence >= 0) {
            // Stop when we hit non-negative sequence
            break;
        }

        if (!plugin->m_bEnabled) {
            continue;
        }

        // Rebuild plugin's alias array if needed
        if (plugin->m_aliasesNeedSorting) {
            plugin->rebuildAliasArray();
        }

        // Set current plugin context
        m_CurrentPlugin = plugin.get();

        // Evaluate this plugin's aliases
        stopEvaluation = evaluateOneAliasSequence(plugin->m_AliasArray, command, this, anyMatched);

        if (stopEvaluation) {
            m_CurrentPlugin = savedPlugin;
            return true; // Alias handled it and wants to stop
        }
    }

    // ========== Phase 2: Evaluate WORLD aliases ==========
    m_CurrentPlugin = nullptr;

    stopEvaluation = evaluateOneAliasSequence(m_AliasArray, command, this, anyMatched);

    if (stopEvaluation) {
        m_CurrentPlugin = savedPlugin;
        return true; // Alias handled it and wants to stop
    }

    // ========== Phase 3: Evaluate plugins with ZERO/POSITIVE sequence ==========
    // These plugins get to see the command AFTER the world aliases
    for (const auto& plugin : m_PluginList) {
        if (plugin->m_iSequence < 0) {
            // Skip negative sequence (already processed)
            continue;
        }

        if (!plugin->m_bEnabled) {
            continue;
        }

        // Rebuild plugin's alias array if needed
        if (plugin->m_aliasesNeedSorting) {
            plugin->rebuildAliasArray();
        }

        // Set current plugin context
        m_CurrentPlugin = plugin.get();

        // Evaluate this plugin's aliases
        stopEvaluation = evaluateOneAliasSequence(plugin->m_AliasArray, command, this, anyMatched);

        if (stopEvaluation) {
            m_CurrentPlugin = savedPlugin;
            return true; // Alias handled it and wants to stop
        }
    }

    // Restore plugin context
    m_CurrentPlugin = savedPlugin;

    // Return true if any alias matched, even if they all had bKeepEvaluating=true
    if (anyMatched) {
        qDebug() << "evaluateAliases: Alias(es) matched and handled command:" << command;
        return true;
    }

    qDebug() << "evaluateAliases: No alias matched, sending to MUD:" << command;
    return false; // No alias handled it, send to MUD
}

/**
 * executeAlias - Execute an alias's action
 *
 * Called when an alias matches. Performs all alias actions:
 * - Echoes command if requested
 * - Replaces wildcards in contents
 * - Expands variables (if bExpandVariables)
 * - Executes action based on iSendTo
 * - Calls Lua script (if strProcedure set)
 * - Deletes alias (if bOneShot)
 * - Adds to command history (if !bOmitFromCommandHistory)
 *
 * Note: Statistics (nMatched, tWhenMatched) are already updated by Alias::match()
 *
 * @param alias The alias that matched
 * @param command The user's command
 */
void WorldDocument::executeAlias(Alias* alias, const QString& command)
{
    // Echo the alias/command?
    if (alias->bEchoAlias) {
        // Display the command in output
        note(command);
    }

    // Prepare contents (send text)
    QString contents = alias->contents;

    // Replace wildcards (%0, %1, %2, etc.)
    contents = replaceWildcards(contents, alias->wildcards);

    // Expand variables (@variablename → value)
    if (alias->bExpandVariables) {
        contents = expandVariables(contents);
    }

    // Execute action using central SendTo() function
    // Refactored to use sendTo() instead of inline switch statement
    QString strExtraOutput; // Accumulates eSendToOutput text
    QString aliasDescription =
        QString("Alias: %1")
            .arg(alias->strLabel.isEmpty() ? alias->strInternalName : alias->strLabel);

    sendTo(alias->iSendTo, contents, alias->bOmitFromOutput, alias->bOmitFromLog, aliasDescription,
           alias->strVariable, strExtraOutput, alias->scriptLanguage);

    // Display any output that was accumulated
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Call Lua script if procedure is defined
    // Original MUSHclient calls the script function if strProcedure is set,
    // regardless of iSendTo value
    if (!alias->strProcedure.isEmpty()) {
        executeAliasScript(alias, command);
    }

    // Add to command history (unless omitted)
    if (!alias->bOmitFromCommandHistory) {
        // addToCommandHistory() handles duplicate filtering
        // Note: We're passing the original command, not the alias contents
        // This matches original MUSHclient behavior
        addToCommandHistory(command);
    }

    // One-shot: delete alias after firing
    if (alias->bOneShot) {
        qCDebug(lcWorld) << "Deleting one-shot alias:" << alias->strLabel;
        deleteAlias(alias->strInternalName);
    }

    qCDebug(lcWorld) << "Alias executed:" << alias->strLabel << "matched:" << alias->nMatched
                     << "times";
}

/**
 * executeAliasScript - Execute Lua script callback for an alias
 *
 * Alias Script Execution
 * Based on CMUSHclientDoc::ExecuteAliasScript()
 *
 * Calls the Lua function specified in alias->strProcedure with parameters:
 * 1. Alias name (string)
 * 2. Matched command (string)
 * 3. Wildcards (table) - indexed 0..N where 0 is full match, 1+ are capture groups
 *
 * @param alias The alias that matched
 * @param command The command that matched the alias
 */
void WorldDocument::executeAliasScript(Alias* alias, const QString& command)
{
    // Safety check - need a procedure name
    if (alias->strProcedure.isEmpty()) {
        return;
    }

    // Determine which script engine to use:
    // - For plugin aliases, use the plugin's script engine
    // - For world aliases, use the world's script engine
    ScriptEngine* scriptEngine = nullptr;
    if (m_CurrentPlugin && m_CurrentPlugin->m_ScriptEngine) {
        scriptEngine = m_CurrentPlugin->m_ScriptEngine.get();
    } else if (m_ScriptEngine) {
        scriptEngine = m_ScriptEngine.get();
    }

    if (!scriptEngine || !scriptEngine->isLua()) {
        qDebug() << "executeAliasScript: No script engine for alias" << alias->strProcedure;
        return;
    }

    // Check if function exists
    // dispid caching: DISPID_UNKNOWN = check needed, 1 = exists (cached)
    // We re-check each time it's DISPID_UNKNOWN (allows function to be defined later)
    if (alias->dispid == DISPID_UNKNOWN) {
        alias->dispid = scriptEngine->getLuaDispid(alias->strProcedure);

        if (alias->dispid == DISPID_UNKNOWN) {
            qDebug() << "executeAliasScript: Function not found:" << alias->strProcedure;
            return; // Function doesn't exist, skip it
        }
    }

    // Get Lua state
    lua_State* L = scriptEngine->L;
    if (!L) {
        return;
    }

    // Parameter 1: Alias name (use label if set, otherwise internal name)
    QString aliasName = alias->strLabel.isEmpty() ? alias->strInternalName : alias->strLabel;

    // Prevent deletion during script execution
    alias->bExecutingScript = true;

    // Clear stack
    lua_settop(L, 0);

    // Find the function
    lua_getglobal(L, alias->strProcedure.toUtf8().constData());
    if (!lua_isfunction(L, -1)) {
        qDebug() << "executeAliasScript: Function not found:" << alias->strProcedure;
        lua_pop(L, 1);
        alias->bExecutingScript = false;
        alias->dispid = DISPID_UNKNOWN;
        return;
    }

    // Push parameter 1: Alias name
    QByteArray nameBytes = aliasName.toUtf8();
    lua_pushlstring(L, nameBytes.constData(), nameBytes.length());

    // Push parameter 2: Matched command
    QByteArray cmdBytes = command.toUtf8();
    lua_pushlstring(L, cmdBytes.constData(), cmdBytes.length());

    // Create and set global wildcards table (for scripts that access it as a global)
    lua_newtable(L);
    // Add numeric wildcards
    for (int i = 0; i < alias->wildcards.size(); ++i) {
        QString wildcard = alias->wildcards[i];
        lua_pushinteger(L, i);
        QByteArray ba = wildcard.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_settable(L, -3);
    }
    // Add named capture groups
    for (auto it = alias->namedWildcards.constBegin(); it != alias->namedWildcards.constEnd();
         ++it) {
        QByteArray nameBytes = it.key().toUtf8();
        lua_pushlstring(L, nameBytes.constData(), nameBytes.length());
        QByteArray valueBytes = it.value().toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_settable(L, -3);
    }
    lua_setglobal(L, "wildcards");

    // Push parameter 3: Wildcards table (for function parameter)
    lua_newtable(L);
    // Add numeric wildcards
    for (int i = 0; i < alias->wildcards.size(); ++i) {
        QString wildcard = alias->wildcards[i];
        lua_pushinteger(L, i);
        QByteArray ba = wildcard.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_settable(L, -3);
    }
    // Add named capture groups
    for (auto it = alias->namedWildcards.constBegin(); it != alias->namedWildcards.constEnd();
         ++it) {
        QByteArray nameBytes = it.key().toUtf8();
        lua_pushlstring(L, nameBytes.constData(), nameBytes.length());
        QByteArray valueBytes = it.value().toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_settable(L, -3);
    }

    // Save old action source
    unsigned short oldActionSource = m_iCurrentActionSource;
    m_iCurrentActionSource = eAliasAction;

    // Call function with 3 parameters (name, line, wildcards)
    int error = lua_pcall(L, 3, 0, 0);

    // Restore action source
    m_iCurrentActionSource = oldActionSource;

    if (error) {
        QString errorMsg = QString::fromUtf8(lua_tostring(L, -1));
        qWarning() << "Alias script error in" << alias->strProcedure << ":" << errorMsg;
        lua_pop(L, 1);
        alias->dispid = DISPID_UNKNOWN;
    } else {
        alias->nInvocationCount++;
    }

    lua_settop(L, 0); // Clear stack

    // Allow deletion again
    alias->bExecutingScript = false;

    qCDebug(lcWorld) << "Alias script executed:" << alias->strProcedure
                     << "invocations:" << alias->nInvocationCount;
}

/**
 * rebuildAliasArray - Rebuild alias array sorted by sequence
 *
 * Plugin Evaluation Order
 * Called when aliases are added/deleted or sequence numbers change.
 * Rebuilds m_AliasArray from m_AliasMap, sorted by iSequence.
 */
void WorldDocument::rebuildAliasArray()
{
    m_AliasArray.clear();

    // Add all aliases from map to array
    for (const auto& [name, aliasPtr] : m_AliasMap) {
        m_AliasArray.append(aliasPtr.get());
    }

    // Sort by sequence (lower = earlier)
    std::sort(m_AliasArray.begin(), m_AliasArray.end(),
              [](const Alias* a, const Alias* b) { return a->iSequence < b->iSequence; });

    m_aliasesNeedSorting = false;
}
