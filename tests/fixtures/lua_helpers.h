/**
 * lua_helpers.h - Standalone Lua test helper functions
 *
 * Free functions in the test_helpers namespace for use by tests
 * that need Lua utilities without inheriting from LuaWorldTest.
 * Most tests should prefer the methods on LuaWorldTest instead.
 */

#pragma once

#include <QString>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace test_helpers {

inline void executeLua(lua_State* L, const char* code)
{
    ASSERT_EQ(luaL_loadstring(L, code), 0) << "Lua code should compile: " << code;
    ASSERT_EQ(lua_pcall(L, 0, 0, 0), 0) << "Lua code should execute: " << code;
}

inline QString getGlobalString(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    const char* str = lua_tostring(L, -1);
    QString result = str ? QString::fromUtf8(str) : QString();
    lua_pop(L, 1);
    return result;
}

inline double getGlobalNumber(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    double result = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return result;
}

inline bool functionExists(lua_State* L, const char* tableName, const char* funcName)
{
    lua_getglobal(L, tableName);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    lua_getfield(L, -1, funcName);
    bool exists = lua_isfunction(L, -1);
    lua_pop(L, 2);
    return exists;
}

} // namespace test_helpers
