/**
 * test_variable_lua_api_gtest.cpp
 * Variable Management Lua API Tests
 *
 * Tests for variable-related Lua API functions:
 * - world.SetVariable - Set a world variable value
 * - world.GetVariable - Retrieve a world variable value
 * - world.GetVariableList - Get list of all variables
 * - world.DeleteVariable - Remove a variable
 *
 * These tests verify variable storage, retrieval, listing, and deletion,
 * including edge cases like non-existent variables and empty string values.
 */

#include "lua_api_test_fixture.h"

// Test 33: SetVariable and GetVariable
TEST_F(LuaApiTest, SetGetVariable)
{
    lua_getglobal(L, "test_set_get_variable");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_get_variable should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_get_variable should succeed";
}

// Test 34: GetVariable for non-existent variable
TEST_F(LuaApiTest, GetVariableNotFound)
{
    lua_getglobal(L, "test_get_variable_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_variable_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_variable_not_found should succeed";
}

// Test 35: GetVariableList
TEST_F(LuaApiTest, GetVariableList)
{
    lua_getglobal(L, "test_get_variable_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_variable_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_variable_list should succeed";
}

// Test 36: DeleteVariable
TEST_F(LuaApiTest, DeleteVariable)
{
    lua_getglobal(L, "test_delete_variable");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_variable should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_variable should succeed";
}

// Test 37: DeleteVariable for non-existent variable
TEST_F(LuaApiTest, DeleteVariableNotFound)
{
    lua_getglobal(L, "test_delete_variable_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_variable_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_variable_not_found should succeed";
}

// Test 38: Variable with empty string value
TEST_F(LuaApiTest, VariableEmptyString)
{
    lua_getglobal(L, "test_variable_empty_string");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_variable_empty_string should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_variable_empty_string should succeed";
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
