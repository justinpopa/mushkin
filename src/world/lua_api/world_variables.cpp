/**
 * world_variables.cpp - Variable Functions
 */

#include "lua_common.h"

/**
 * world.GetVariable(name)
 *
 * Gets a variable value.
 * When called from a plugin, gets from plugin's variable map.
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
 * Sets a variable value.
 * When called from a plugin, stores in plugin's variable map.
 *
 * @return eOK (0) on success, eInvalidObjectLabel (30008) if name is invalid
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
 * Deletes a variable.
 * When called from a plugin, deletes from plugin's variable map.
 *
 * @return eOK (0) on success, eInvalidObjectLabel (30008) if name is invalid,
 *         eVariableNotFound (30019) if variable doesn't exist
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
 * Gets list of all variable names.
 * When called from a plugin, gets from plugin's variable map.
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
