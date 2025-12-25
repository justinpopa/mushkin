/**
 * test_lua_basics_gtest.cpp - GoogleTest version
 * Test basic Lua functionality
 *
 * Tests tostring(), pairs(), ipairs() to verify core Lua functions work
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for Lua basics tests
class LuaBasicsTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;
        ASSERT_NE(L, nullptr) << "Lua state should be available";
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test 0: Simple assignment (no functions)
TEST_F(LuaBasicsTest, SimpleAssignment)
{
    int result = luaL_dostring(L, "x = 42");
    EXPECT_EQ(result, 0) << "Simple assignment should succeed";

    lua_getglobal(L, "x");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 42) << "Variable x should be 42";
}

// Test 0.5: Check if tostring exists
TEST_F(LuaBasicsTest, TostringExists)
{
    lua_getglobal(L, "tostring");
    int type = lua_type(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(type, LUA_TFUNCTION) << "tostring should be a function";
}

// Test 1: Call tostring() directly
TEST_F(LuaBasicsTest, TostringDirectCall)
{
    // Load and execute: y = tostring(99)
    ASSERT_EQ(luaL_loadstring(L, "y = tostring(99)"), 0) << "Code should load successfully";
    ASSERT_EQ(lua_pcall(L, 0, 0, 0), 0) << "Code should execute successfully";

    // Check result
    lua_getglobal(L, "y");
    const char* strresult = lua_tostring(L, -1);
    lua_pop(L, 1);

    EXPECT_STREQ(strresult, "99") << "tostring(99) should return '99'";
}

// Test 2: ipairs() on Lua-created table
TEST_F(LuaBasicsTest, IpairsOnLuaTable)
{
    const char* code = R"(
        t = {10, 20, 30}
        sum = 0
        for i,v in ipairs(t) do
            sum = sum + v
        end
    )";

    ASSERT_EQ(luaL_dostring(L, code), 0) << "ipairs code should execute";

    lua_getglobal(L, "sum");
    int sum = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(sum, 60) << "Sum via ipairs should be 60 (10+20+30)";
}

// Test 3: pairs() on Lua-created table
TEST_F(LuaBasicsTest, PairsOnLuaTable)
{
    const char* code = R"(
        t = {a=1, b=2, c=3}
        count = 0
        for k,v in pairs(t) do
            count = count + 1
        end
    )";

    ASSERT_EQ(luaL_dostring(L, code), 0) << "pairs code should execute";

    lua_getglobal(L, "count");
    int count = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(count, 3) << "Count via pairs should be 3";
}

// Test 4: ipairs() on C-created table
TEST_F(LuaBasicsTest, IpairsOnCTable)
{
    // Create table from C
    lua_newtable(L);
    lua_pushnumber(L, 10);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, 20);
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, 30);
    lua_rawseti(L, -2, 3);
    lua_setglobal(L, "ctable");

    const char* code = R"(
        csum = 0
        for i,v in ipairs(ctable) do
            csum = csum + v
        end
    )";

    ASSERT_EQ(luaL_dostring(L, code), 0) << "ipairs on C table should execute";

    lua_getglobal(L, "csum");
    int csum = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(csum, 60) << "Sum via ipairs on C table should be 60";
}

// Test 5: tostring() in string concatenation
TEST_F(LuaBasicsTest, TostringInConcatenation)
{
    ASSERT_EQ(luaL_dostring(L, "msg = 'Value is: ' .. tostring(123)"), 0)
        << "Concatenation with tostring should execute";

    lua_getglobal(L, "msg");
    const char* msg = lua_tostring(L, -1);
    lua_pop(L, 1);

    EXPECT_STREQ(msg, "Value is: 123") << "Concatenation should produce correct string";
}

// Test 6: tostring() with function return value
TEST_F(LuaBasicsTest, TostringWithFunctionReturn)
{
    const char* code = R"(
        function getnum()
            return 42
        end
        result = tostring(getnum())
    )";

    ASSERT_EQ(luaL_dostring(L, code), 0) << "Function with tostring should execute";

    lua_getglobal(L, "result");
    const char* funcresult = lua_tostring(L, -1);
    lua_pop(L, 1);

    EXPECT_STREQ(funcresult, "42") << "tostring on function return should be '42'";
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects like WorldDocument)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
