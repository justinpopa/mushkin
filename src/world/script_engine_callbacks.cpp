/**
 * script_engine_callbacks.cpp - Lua Function Callbacks
 *
 * Implements callback functionality for invoking Lua functions in response
 * to world events.
 *
 * Based on lua_scripting.cpp and Utilities.cpp from original MUSHclient.
 */

#include "../world/world_document.h"
#include "script_engine.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QStringList>

#include "logging.h"
#include "color_utils.h"

// Lua headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Forward declare callLuaWithTraceBack (implemented in script_engine.cpp)
extern int callLuaWithTraceBack(lua_State* L, int nArgs, int nResults);

// Declare luaError for use in executeLua
static void luaError(lua_State* L, const QString& event, const QString& procedure,
                     const QString& type, const QString& reason, WorldDocument* doc);

// ========== Lua Function Callbacks - Helper Functions ==========

/**
 * GetNestedFunction - Navigate nested tables to find function
 *
 * Lua Function Callbacks
 * Based on GetNestedFunction() from Utilities.cpp
 *
 * Navigates through dotted names (e.g., "utils.reload" → utils["reload"])
 * to find a function in nested tables. Leaves the function on the stack
 * if found.
 *
 * Stack behavior:
 * - On entry: Empty (cleared by caller)
 * - On success: Function is on top of stack
 * - On failure: Nil or error value on top of stack
 *
 * @param L Lua state
 * @param sName Function name (may contain dots for table navigation)
 * @param bRaiseError Show error message if function not found
 * @return true if function found and on stack, false otherwise
 */
bool GetNestedFunction(lua_State* L, const QString& sName, bool bRaiseError)
{
    // Split name by dots (e.g., "utils.reload" → ["utils", "reload"])
    QStringList parts = sName.split('.', Qt::SkipEmptyParts);

    // Start with global table
    lua_pushvalue(L, LUA_GLOBALSINDEX);

    QString sPrev = "_G";
    QString sItem;

    // Navigate through tables
    for (const QString& part : parts) {
        sItem = part;

        // Check if current value is a table
        if (!lua_istable(L, -1)) {
            break; // Not a table, exit loop
        }

        // Get next element from table
        QByteArray ba = part.toUtf8();
        lua_getfield(L, -1, ba.constData());

        // Remove parent table (leave new value on stack)
        lua_remove(L, -2);

        sPrev = part;
    }

    // Check if we got a function
    bool bResult = lua_isfunction(L, -1);

    // Handle error cases
    if (!bResult) {
        if (bRaiseError) {
            qWarning() << QString("Cannot find the function '%1' - item '%2' is %3")
                              .arg(sName)
                              .arg(sItem)
                              .arg(lua_typename(L, lua_type(L, -1)));
        }
        return false;
    }

    return true; // Found it, function is on stack
}

/**
 * FindLuaFunction - Check if a function exists in Lua
 *
 * Lua Function Callbacks
 * Based on FindLuaFunction() from Utilities.cpp
 *
 * Calls GetNestedFunction() to find the function, then clears the stack.
 * This is used to check for function existence without calling it.
 *
 * @param L Lua state
 * @param sName Function name (may be dotted like "string.gsub")
 * @return true if function exists, false otherwise
 */
bool FindLuaFunction(lua_State* L, const QString& sName)
{
    bool bResult = GetNestedFunction(L, sName, false);
    lua_settop(L, 0); // Clear stack (pop function or error)
    return bResult;
}

// ========== Lua Function Callbacks - ScriptEngine Methods ==========

/**
 * getLuaDispid - Check if function exists in Lua
 *
 * Lua Function Callbacks
 * Based on CScriptEngine::GetLuaDispid() from lua_scripting.cpp
 *
 * For Lua, DISPID is just a flag to track function existence:
 * - 1 = function exists
 * - DISPID_UNKNOWN = function doesn't exist
 *
 * This is used to avoid repeated lookups of non-existent functions.
 *
 * @param strName Function name (supports dotted names)
 * @return 1 if function exists, DISPID_UNKNOWN if not
 */
qint32 ScriptEngine::getLuaDispid(const QString& strName)
{
    bool hasFunction = L && FindLuaFunction(L, strName);
    qCDebug(lcScript) << "getLuaDispid:" << strName << "- found:" << hasFunction;
    return hasFunction ? 1 : DISPID_UNKNOWN;
}

// ========== Error Display (with procedure/type/reason) ==========

/**
 * luaError - Display Lua error with context
 *
 * version that accepts procedure, type, and reason for context.
 *
 * @param L Lua state (error message on top of stack)
 * @param event Event description ("Compile error", "Run-time error")
 * @param procedure Function name that failed
 * @param type Type string ("world", "trigger", "alias")
 * @param reason Description of what was being done
 * @param doc WorldDocument for display
 */
static void luaError(lua_State* L, const QString& event, const QString& procedure,
                     const QString& type, const QString& reason, WorldDocument* doc)
{
    // Get error message from Lua stack
    QString errorMsg;
    if (lua_isstring(L, -1)) {
        errorMsg = QString::fromUtf8(lua_tostring(L, -1));
    } else {
        errorMsg = "<unknown error>";
    }
    lua_settop(L, 0); // Clear stack

    // Build context string
    QString context;
    if (!procedure.isEmpty()) {
        context = QString("Function/Sub: %1 called by %2\nReason: %3").arg(procedure, type, reason);
    } else {
        context = "Immediate execution";
    }

    // Display error
    qWarning() << "=== Lua Error ===" << event;
    qWarning() << "  Context:" << context;
    qWarning() << "  Message:" << errorMsg;

    // Display in output window - orange error text on black background
    if (doc) {
        doc->colourNote(BGR(255, 140, 0), BGR(0, 0, 0), QString("=== %1 ===").arg(event));
        if (!context.isEmpty()) {
            doc->colourNote(BGR(255, 140, 0), BGR(0, 0, 0), context);
        }
        doc->colourNote(BGR(255, 140, 0), BGR(0, 0, 0), errorMsg);
    }
}

/**
 * executeLua - Execute Lua callback function with parameters
 *
 * Lua Function Callbacks
 * Based on CScriptEngine::ExecuteLua() from lua_scripting.cpp
 *
 * This is the main callback executor. It:
 * 1. Checks if function exists (via dispid)
 * 2. Finds the function in Lua state
 * 3. Pushes parameters onto stack
 * 4. Sets action source (tells scripts why code is running)
 * 5. Calls function with traceback
 * 6. Extracts return value
 * 7. Updates timing and invocation statistics
 *
 * @param dispid IN/OUT: Function exists flag (1 = exists, DISPID_UNKNOWN = doesn't exist)
 * @param szProcedure Function name (e.g., "OnWorldConnect")
 * @param iReason Action source enum (eWorldAction, eTriggerAction, etc.)
 * @param szType Type string ("world", "trigger", "alias")
 * @param szReason Description (e.g., "world connect")
 * @param nparams List of number parameters to pass to function
 * @param sparams List of string parameters to pass to function
 * @param nInvocationCount IN/OUT: Invocation counter (incremented on success)
 * @param result OUT: Boolean result from function (optional)
 * @return true on error, false on success
 */
bool ScriptEngine::executeLua(qint32& dispid, const QString& szProcedure, unsigned short iReason,
                              const QString& szType, const QString& szReason,
                              QList<double>& nparams, QList<QString>& sparams,
                              long& nInvocationCount, bool* result)
{
    // Safety check
    if (!L) {
        return false; // No Lua state, not really an error
    }

    // Don't call if previously failed
    if (dispid == DISPID_UNKNOWN) {
        return false; // Skip it
    }

    // Clear stack
    lua_settop(L, 0);

    // Trace (skip noisy callbacks)
    if (szProcedure != "OnPluginDrawOutputWindow" && szProcedure != "OnPluginTick") {
        qCDebug(lcScript) << QString("Executing %1 script \"%2\"").arg(szType, szProcedure);
    }

    // Time the call
    QElapsedTimer timer;
    timer.start();

    // Find the function
    if (!GetNestedFunction(L, szProcedure, true)) {
        dispid = DISPID_UNKNOWN; // Mark as missing
        return true;             // Error
    }

    int paramCount = 0;

    // Push number parameters
    for (double n : nparams) {
        lua_pushnumber(L, n);
        paramCount++;
    }

    // Push string parameters
    for (const QString& s : sparams) {
        QByteArray ba = s.toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        paramCount++;
    }

    // Save old action source and note style
    unsigned short oldActionSource = m_doc->m_iCurrentActionSource;
    quint16 oldNoteStyle = m_doc->m_iNoteStyle;

    // Set action source (tells scripts what triggered this call)
    if (iReason != eDontChangeAction) {
        m_doc->m_iCurrentActionSource = iReason;
    }
    m_doc->m_iNoteStyle = 0; // NORMAL

    // Call function with traceback
    int error = callLuaWithTraceBack(L, paramCount, LUA_MULTRET);

    // Restore old state
    if (iReason != eDontChangeAction) {
        m_doc->m_iCurrentActionSource = oldActionSource;
    }
    m_doc->m_iNoteStyle = oldNoteStyle;

    if (error) {
        dispid = DISPID_UNKNOWN; // Mark as failed, don't call again
        luaError(L, "Run-time error", szProcedure, szType, szReason, m_doc);
        return true; // Error
    }

    nInvocationCount++; // Count successful calls

    // Update timing statistics
    if (m_doc) {
        qint64 elapsed = timer.nsecsElapsed();
        m_doc->m_iScriptTimeTaken += elapsed;
    }

    // Get return value (if wanted)
    if (result) {
        *result = true; // Default to true

        if (lua_gettop(L) > 0) {
            if (lua_isboolean(L, 1)) {
                *result = lua_toboolean(L, 1);
            } else {
                // Lua treats 0 as true, so check number
                *result = lua_tonumber(L, 1) != 0;
            }
        }
    }

    lua_settop(L, 0); // Clear stack

    return false; // Success
}
