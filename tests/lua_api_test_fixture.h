/**
 * lua_api_test_fixture.h - Shared test fixture for Lua API tests
 *
 * Provides a common test fixture that:
 * - Creates a WorldDocument with Lua scripting engine
 * - Loads tests/test_api.lua containing test helper functions
 * - Cleans up after each test
 *
 * Usage:
 *   #include "lua_api_test_fixture.h"
 *
 *   TEST_F(LuaApiTest, MyTest) {
 *       lua_getglobal(L, "test_my_function");
 *       int pcall_result = lua_pcall(L, 0, 1, 0);
 *       ASSERT_EQ(pcall_result, 0);
 *       int result = lua_tointeger(L, -1);
 *       lua_pop(L, 1);
 *       EXPECT_EQ(result, 0);
 *   }
 */

#ifndef LUA_API_TEST_FIXTURE_H
#define LUA_API_TEST_FIXTURE_H

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QGuiApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

/**
 * Base test fixture for Lua API tests.
 *
 * Sets up a WorldDocument with Lua scripting and loads test_api.lua.
 * Provides access to the Lua state via the L member variable.
 */
class LuaApiTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;

        // Load test script file (relative to build/tests directory)
        if (luaL_dofile(L, "tests/test_api.lua") != 0) {
            const char* error = lua_tostring(L, -1);
            lua_pop(L, 1);
            FAIL() << "Could not load test script: " << error;
        }
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

#endif // LUA_API_TEST_FIXTURE_H
