#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include "../automation/script_language.h"
#include <QObject>
#include <QString>

// Forward declarations
class WorldDocument;
struct lua_State;

/**
 * ScriptEngine - Lua scripting engine wrapper
 *
 * Manages a Lua state for one world document, providing:
 * - Lua state initialization and cleanup
 * - Standard library loading
 * - Document pointer storage in Lua registry
 * - Script parsing and execution
 *
 * Each world has its own independent Lua state for isolation.
 *
 * Based on CScriptEngine from scripting.h and lua_scripting.cpp
 */
class ScriptEngine : public QObject {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument this engine belongs to
     * @param language Script language (currently only "Lua")
     * @param parent Parent QObject
     */
    explicit ScriptEngine(WorldDocument* doc, const QString& language, QObject* parent = nullptr);

    /**
     * Destructor - automatically closes Lua state
     */
    ~ScriptEngine() override;

    /**
     * Create script engine (calls openLua)
     * @return true if successful
     */
    bool createScriptEngine();

    /**
     * Disable scripting (calls closeLua)
     */
    void disableScripting();

    /**
     * Check if Lua is active
     * @return true if Lua state exists
     */
    bool isLua() const
    {
        return L != nullptr;
    }

    /**
     * Get the language name
     * @return "Lua"
     */
    QString language() const
    {
        return m_language;
    }

    /**
     * Open Lua state
     *
     * Creates lua_State, loads standard libraries, stores document pointer
     * in registry, and prepares for script execution.
     *
     * Based on CScriptEngine::OpenLua() from lua_scripting.cpp
     */
    void openLua();

    /**
     * Close Lua state
     *
     * Destroys lua_State and frees all Lua resources.
     *
     * Based on CScriptEngine::CloseLua() from lua_scripting.cpp
     */
    void closeLua();

    /**
     * Parse and execute Lua code
     *
     * Script Loading and Parsing
     *
     * Compiles and executes Lua code string. Handles both compile
     * and runtime errors gracefully.
     *
     * Based on CScriptEngine::ParseLua() from lua_scripting.cpp
     *
     * @param code Lua code to execute
     * @param name Name for error messages (e.g., "Script file", "Plugin")
     * @return true on error, false on success
     */
    bool parseLua(const QString& code, const QString& name);

    /**
     * Parse and execute script code in the specified language
     *
     * For YueScript, transpiles to Lua first then executes.
     * For Lua, calls parseLua() directly.
     *
     * @param code Script code to execute
     * @param name Name for error messages
     * @param lang Script language (Lua or YueScript)
     * @return true on error, false on success
     */
    bool parseScript(const QString& code, const QString& name, ScriptLanguage lang);

    /**
     * Transpile YueScript code to Lua
     *
     * Uses the yue.to_lua() function from the YueScript module to convert
     * YueScript source code to equivalent Lua code.
     *
     * @param yueCode YueScript source code
     * @param name Name for error messages
     * @return Transpiled Lua code, or empty string on error
     */
    QString transpileYueScript(const QString& yueCode, const QString& name);

    /**
     * Transpile Teal code to Lua
     *
     * Uses tl.gen() to convert Teal (typed Lua) source code to Lua.
     *
     * @param tealCode Teal source code
     * @param name Name for error messages
     * @return Transpiled Lua code, or empty string on error
     */
    QString transpileTeal(const QString& tealCode, const QString& name);

    /**
     * Transpile Fennel code to Lua
     *
     * Uses fennel.compileString() to convert Fennel (Lisp) source to Lua.
     *
     * @param fennelCode Fennel source code
     * @param name Name for error messages
     * @return Transpiled Lua code, or empty string on error
     */
    QString transpileFennel(const QString& fennelCode, const QString& name);

    /**
     * Transpile MoonScript code to Lua
     *
     * Uses moonscript.to_lua() to convert MoonScript source to Lua.
     *
     * @param moonCode MoonScript source code
     * @param name Name for error messages
     * @return Transpiled Lua code, or empty string on error
     */
    QString transpileMoonScript(const QString& moonCode, const QString& name);

    // ========== Lua Function Callbacks ==========

    /**
     * Get Lua DISPID - Check if function exists in Lua
     *
     * Returns 1 if function exists, DISPID_UNKNOWN if not.
     * For Lua, DISPID is just a flag to track function existence.
     *
     * Based on CScriptEngine::GetLuaDispid() from lua_scripting.cpp
     *
     * @param strName Function name (supports dotted names like "utils.reload")
     * @return 1 if function exists, DISPID_UNKNOWN (-1) otherwise
     */
    qint32 getLuaDispid(const QString& strName);

    /**
     * Execute Lua function with parameters
     *
     * Calls a Lua function with the given parameters, handling errors gracefully.
     * Updates timing statistics and invocation counters.
     *
     * Based on CScriptEngine::ExecuteLua() from lua_scripting.cpp
     *
     * @param dispid IN/OUT: Function exists flag (set to DISPID_UNKNOWN on error)
     * @param szProcedure Function name (e.g., "OnWorldConnect")
     * @param iReason Action source (eWorldAction, eTriggerAction, etc.)
     * @param szType Type string ("world", "trigger", etc.)
     * @param szReason Description (e.g., "world connect")
     * @param nparams List of number parameters
     * @param sparams List of string parameters
     * @param nInvocationCount IN/OUT: Invocation counter
     * @param result OUT: Boolean result from function (optional)
     * @return true on error, false on success
     */
    bool executeLua(qint32& dispid, const QString& szProcedure, unsigned short iReason,
                    const QString& szType, const QString& szReason, QList<double>& nparams,
                    QList<QString>& sparams, long& nInvocationCount, bool* result = nullptr);

    // Public Lua state (accessed by API functions)
    lua_State* L;

    /**
     * Set plugin pointer for this script engine
     *
     * For plugin script engines, this stores the plugin pointer
     * in the Lua registry so API functions can retrieve it.
     *
     * @param plugin Plugin this engine belongs to
     */
    void setPlugin(class Plugin* plugin);

  private:
    WorldDocument* m_doc;   // Back-pointer to world (not owned)
    class Plugin* m_plugin; // Plugin this engine belongs to (null for world)
    QString m_language;     // "Lua"
};

// Registry key for document pointer
#define DOCUMENT_STATE "mushclient.document"

// Registry key for plugin pointer (for plugin Lua states)
#define PLUGIN_STATE "mushclient.plugin"

// DISPID constants for script callbacks
// For Lua, DISPID is just a flag: 1 = exists, DISPID_UNKNOWN = doesn't exist
#define DISPID_UNKNOWN (-1)

// ========== Lua API Registration ==========

// Forward declaration (defined in lua_api/lua_registration.cpp)
int RegisterLuaRoutines(lua_State* L);

/**
 * luaopen_utils - Register utils module functions
 *
 * Creates the global "utils" table and registers utility functions.
 *
 * @param L Lua state
 */
void luaopen_utils(lua_State* L);

// ========== Lua Helper Functions ==========

/**
 * callLuaWithTraceBack - Call Lua function with debug traceback
 *
 * Calls lua_pcall with debug.traceback as error handler for better errors.
 * Used by parseLua() and executeLua().
 *
 * @param L Lua state
 * @param nArgs Number of arguments on stack
 * @param nResults Number of expected results
 * @return Error code (0 = success, non-zero = error)
 */
int callLuaWithTraceBack(lua_State* L, int nArgs, int nResults);

/**
 * FindLuaFunction - Check if a function exists in Lua
 *
 * Handles dotted names (e.g., "string.gsub", "utils.reload").
 * Clears the stack before returning.
 *
 * Based on FindLuaFunction() from Utilities.cpp
 *
 * @param L Lua state
 * @param sName Function name
 * @return true if function exists
 */
bool FindLuaFunction(lua_State* L, const QString& sName);

/**
 * GetNestedFunction - Navigate nested tables to find function
 *
 * Leaves function on stack if found. Handles dotted names.
 * Does NOT clear the stack (caller must do that).
 *
 * Based on GetNestedFunction() from Utilities.cpp
 *
 * @param L Lua state
 * @param sName Function name (may be dotted)
 * @param bRaiseError Show error dialog if function not found
 * @return true if function found and left on stack
 */
bool GetNestedFunction(lua_State* L, const QString& sName, bool bRaiseError = false);

#endif // SCRIPT_ENGINE_H
