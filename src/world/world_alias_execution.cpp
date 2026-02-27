/**
 * world_alias_execution.cpp - Alias Execution
 *
 * Alias matching/evaluation pipeline (evaluateAliases, rebuildAliasArray) has moved
 * to AutomationRegistry (automation_registry.cpp).
 *
 * This file retains executeAlias() and executeAliasScript() which remain on WorldDocument
 * because they are tightly coupled to its output/script/sendTo infrastructure.
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
 * executeAlias - Execute an alias's action
 *
 * Called when an alias matches. Performs all alias actions:
 * - Echoes command if requested
 * - Replaces wildcards in contents
 * - Expands variables (if expand_variables)
 * - Executes action based on send_to
 * - Calls Lua script (if procedure set)
 * - Deletes alias (if one_shot)
 * - Adds to command history (if !omit_from_command_history)
 *
 * Note: Statistics (matched, when_matched) are already updated by Alias::match()
 *
 * @param alias The alias that matched
 * @param command The user's command
 */
void WorldDocument::executeAlias(Alias* alias, const QString& command)
{
    // Echo the alias/command?
    if (alias->echo_alias) {
        // Display the command in output
        note(command);
    }

    // Prepare contents (send text)
    QString contents = alias->contents;

    // Replace wildcards (%0, %1, %2, etc.)
    contents = replaceWildcards(contents, alias->wildcards);

    // Expand variables (@variablename → value)
    if (alias->expand_variables) {
        contents = expandVariables(contents);
    }

    // Execute action using central SendTo() function
    // Refactored to use sendTo() instead of inline switch statement
    QString strExtraOutput; // Accumulates eSendToOutput text
    QString aliasDescription =
        QString("Alias: %1").arg(alias->label.isEmpty() ? alias->internal_name : alias->label);

    sendTo(alias->send_to, contents, alias->omit_from_output, alias->omit_from_log,
           aliasDescription, alias->variable, strExtraOutput, alias->scriptLanguage);

    // Display any output that was accumulated
    if (!strExtraOutput.isEmpty()) {
        note(strExtraOutput);
    }

    // Call Lua script if procedure is defined
    // Original MUSHclient calls the script function if procedure is set,
    // regardless of send_to value
    if (!alias->procedure.isEmpty()) {
        executeAliasScript(alias, command);
    }

    // Add to command history (unless omitted)
    if (!alias->omit_from_command_history) {
        // addToCommandHistory() handles duplicate filtering
        // Note: We're passing the original command, not the alias contents
        // This matches original MUSHclient behavior
        addToCommandHistory(command);
    }

    // One-shot: delete alias after firing
    if (alias->one_shot) {
        qCDebug(lcWorld) << "Deleting one-shot alias:" << alias->label;
        (void)deleteAlias(alias->internal_name); // intentional: one-shot, fired and done
    }

    qCDebug(lcWorld) << "Alias executed:" << alias->label << "matched:" << alias->matched
                     << "times";
}

/**
 * executeAliasScript - Execute Lua script callback for an alias
 *
 * Alias Script Execution
 * Based on CMUSHclientDoc::ExecuteAliasScript()
 *
 * Calls the Lua function specified in alias->procedure with parameters:
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
    if (alias->procedure.isEmpty()) {
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
        qDebug() << "executeAliasScript: No script engine for alias" << alias->procedure;
        return;
    }

    // Check if function exists
    // dispid caching: DISPID_UNKNOWN = check needed, 1 = exists (cached)
    // We re-check each time it's DISPID_UNKNOWN (allows function to be defined later)
    if (alias->dispid == DISPID_UNKNOWN) {
        alias->dispid = scriptEngine->getLuaDispid(alias->procedure);

        if (alias->dispid == DISPID_UNKNOWN) {
            qDebug() << "executeAliasScript: Function not found:" << alias->procedure;
            return; // Function doesn't exist, skip it
        }
    }

    // Get Lua state
    lua_State* L = scriptEngine->L;
    if (!L) {
        return;
    }

    // Parameter 1: Alias name (use label if set, otherwise internal name)
    QString aliasName = alias->label.isEmpty() ? alias->internal_name : alias->label;

    // Prevent deletion during script execution
    alias->executing_script = true;

    // Clear stack
    lua_settop(L, 0);

    // Find the function
    lua_getglobal(L, alias->procedure.toUtf8().constData());
    if (!lua_isfunction(L, -1)) {
        qDebug() << "executeAliasScript: Function not found:" << alias->procedure;
        lua_pop(L, 1);
        alias->executing_script = false;
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
        qWarning() << "Alias script error in" << alias->procedure << ":" << errorMsg;
        lua_pop(L, 1);
        alias->dispid = DISPID_UNKNOWN;
    } else {
        alias->invocation_count++;
    }

    lua_settop(L, 0); // Clear stack

    // Allow deletion again
    alias->executing_script = false;

    qCDebug(lcWorld) << "Alias script executed:" << alias->procedure
                     << "invocations:" << alias->invocation_count;
}

// rebuildAliasArray moved to AutomationRegistry::rebuildAliasArray().
// WorldDocument::rebuildAliasArray() is an inline forwarding wrapper in world_document.h.
