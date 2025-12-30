/**
 * world_variables.cpp - Variable Functions
 *
 * Variables provide persistent key-value storage that survives across sessions.
 * When called from a plugin, variables are scoped to that plugin's namespace.
 * When called from world script, variables are stored in the world's global namespace.
 */

#include "lua_common.h"

/**
 * world.GetVariable(name)
 *
 * Retrieves the value of a stored variable.
 *
 * @param name (string) The variable name to look up
 *
 * @return (string) The variable's value if it exists
 * @return (nil) If the variable does not exist or has no value
 *
 * @example
 * local hp = GetVariable("current_hp")
 * if hp then
 *     print("HP: " .. hp)
 * else
 *     print("HP not set")
 * end
 *
 * @see SetVariable, DeleteVariable, GetVariableList
 */
int L_GetVariable(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    // Use plugin(L) to get the plugin from Lua registry
    Plugin* currentPlugin = plugin(L);
    QString value;

    if (currentPlugin) {
        // Get from plugin's variable map
        auto it = currentPlugin->m_VariableMap.find(qName);
        if (it != currentPlugin->m_VariableMap.end()) {
            value = it->second->strContents;
        }
    } else {
        value = pDoc->getVariable(qName);
    }

    if (value.isEmpty()) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, value.toUtf8().constData());
    }

    return 1;
}

/**
 * world.SetVariable(name, value)
 *
 * Stores a value in a named variable. Creates the variable if it doesn't exist,
 * or updates the existing value if it does.
 *
 * Variable names must be valid identifiers (alphanumeric and underscore, not
 * starting with a digit). Leading/trailing whitespace is trimmed automatically.
 *
 * @param name (string) The variable name (must be a valid identifier)
 * @param value (string) The value to store
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eInvalidObjectLabel (30008): Invalid variable name
 *
 * @example
 * SetVariable("player_name", "Gandalf")
 * SetVariable("current_hp", "100")  -- Note: values are always strings
 *
 * @see GetVariable, DeleteVariable, GetVariableList
 */
int L_SetVariable(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* value = luaL_checkstring(L, 2);

    QString qName = QString::fromUtf8(name);

    // Validate and trim the name
    qint32 status = validateObjectName(qName);
    if (status != eOK) {
        lua_pushnumber(L, status);
        return 1;
    }

    // Use plugin(L) to get the plugin from Lua registry
    Plugin* currentPlugin = plugin(L);
    qint32 result;

    if (currentPlugin) {
        // Store in plugin's variable map
        auto it = currentPlugin->m_VariableMap.find(qName);
        if (it != currentPlugin->m_VariableMap.end()) {
            // Update existing variable
            it->second->strContents = QString::fromUtf8(value);
        } else {
            // Create new variable
            auto var = std::make_unique<Variable>();
            var->strLabel = qName;
            var->strContents = QString::fromUtf8(value);
            currentPlugin->m_VariableMap[qName] = std::move(var);
        }
        result = eOK;
    } else {
        result = pDoc->setVariable(qName, QString::fromUtf8(value));
    }

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.DeleteVariable(name)
 *
 * Removes a variable from storage. The variable will no longer exist after
 * this call, and GetVariable will return nil for it.
 *
 * @param name (string) The variable name to delete
 *
 * @return (number) Error code:
 *   - eOK (0): Successfully deleted
 *   - eInvalidObjectLabel (30008): Invalid variable name
 *   - eVariableNotFound (30019): Variable does not exist
 *
 * @example
 * local result = DeleteVariable("temp_data")
 * if result == 0 then
 *     print("Variable deleted")
 * elseif result == 30019 then
 *     print("Variable didn't exist")
 * end
 *
 * @see GetVariable, SetVariable, GetVariableList
 */
int L_DeleteVariable(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString qName = QString::fromUtf8(name);

    // Validate and trim the name
    qint32 status = validateObjectName(qName);
    if (status != eOK) {
        lua_pushnumber(L, status);
        return 1;
    }

    // Use plugin(L) to get the plugin from Lua registry
    Plugin* currentPlugin = plugin(L);
    qint32 result;

    if (currentPlugin) {
        // Delete from plugin's variable map
        auto it = currentPlugin->m_VariableMap.find(qName);
        if (it != currentPlugin->m_VariableMap.end()) {
            currentPlugin->m_VariableMap.erase(it);
            result = eOK;
        } else {
            result = eVariableNotFound;
        }
    } else {
        result = pDoc->deleteVariable(qName);
    }

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.GetVariableList()
 *
 * Returns a list of all variable names in the current scope. Useful for
 * iterating over all stored variables or debugging.
 *
 * @return (table) An array of variable names (strings), indexed 1 to n.
 *   Returns an empty table if no variables exist.
 *
 * @example
 * local vars = GetVariableList()
 * for i, name in ipairs(vars) do
 *     print(name .. " = " .. (GetVariable(name) or "nil"))
 * end
 *
 * @example
 * -- Check if any variables exist
 * if #GetVariableList() == 0 then
 *     print("No variables stored")
 * end
 *
 * @see GetVariable, SetVariable, DeleteVariable
 */
int L_GetVariableList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Use plugin(L) to get the plugin from Lua registry
    Plugin* currentPlugin = plugin(L);
    QStringList names;

    if (currentPlugin) {
        // Get from plugin's variable map
        for (const auto& [name, value] : currentPlugin->m_VariableMap) {
            names.append(name);
        }
    } else {
        names = pDoc->getVariableList();
    }

    lua_newtable(L);
    for (int i = 0; i < names.size(); i++) {
        lua_pushstring(L, names[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

// ========== Registration ==========

void register_world_variables_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"GetVariable", L_GetVariable};
    *ptr++ = {"SetVariable", L_SetVariable};
    *ptr++ = {"DeleteVariable", L_DeleteVariable};
    *ptr++ = {"GetVariableList", L_GetVariableList};
}
