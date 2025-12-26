/**
 * test_alias_api_gtest.cpp - Alias API Tests
 *
 * Tests for alias management functions:
 * - AddAlias, DeleteAlias, EnableAlias
 * - GetAliasInfo, GetAliasList, GetAlias
 * - EnableAliasGroup, DeleteAliasGroup
 * - AliasOption, SetAliasOption
 */

#include "lua_api_test_fixture.h"

// Test 1: alias_flag constant table
TEST_F(LuaApiTest, AliasFlagTable)
{
    lua_getglobal(L, "alias_flag");
    ASSERT_TRUE(lua_istable(L, -1)) << "alias_flag should be a table";
    lua_pop(L, 1);
}

// Test 2: AddAlias API
TEST_F(LuaApiTest, AddAlias)
{
    lua_getglobal(L, "test_add_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_add_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_add_alias should succeed";

    // Verify alias was added to document
    EXPECT_NE(doc->getAlias("test_alias"), nullptr) << "Alias should be added to document";
}

// Test 3: GetAliasInfo API
TEST_F(LuaApiTest, GetAliasInfo)
{
    // First add the alias
    lua_getglobal(L, "test_add_alias");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test GetAliasInfo
    lua_getglobal(L, "test_get_alias_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alias_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alias_info should succeed";
}

// Test 4: GetAliasList API
TEST_F(LuaApiTest, GetAliasList)
{
    // First add the alias
    lua_getglobal(L, "test_add_alias");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test GetAliasList
    lua_getglobal(L, "test_get_alias_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alias_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alias_list should succeed";
}

// Test 5: EnableAlias API
TEST_F(LuaApiTest, EnableAlias)
{
    // First add the alias
    lua_getglobal(L, "test_add_alias");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test EnableAlias
    lua_getglobal(L, "test_enable_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_alias should succeed";
}

// Test 6: DeleteAlias API
TEST_F(LuaApiTest, DeleteAlias)
{
    // First add the alias
    lua_getglobal(L, "test_add_alias");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test DeleteAlias
    lua_getglobal(L, "test_delete_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_alias should succeed";

    // Verify alias was deleted from document
    EXPECT_EQ(doc->getAlias("test_alias"), nullptr) << "Alias should be deleted from document";
}

// Test 7: EnableAliasGroup API
TEST_F(LuaApiTest, EnableAliasGroup)
{
    lua_getglobal(L, "test_enable_alias_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_alias_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_alias_group should succeed";
}

// Test 8: EnableAliasGroup with empty group
TEST_F(LuaApiTest, EnableAliasGroupEmpty)
{
    lua_getglobal(L, "test_enable_alias_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_alias_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_alias_group_empty should succeed";
}

// Test 9: Alias Option get/set
TEST_F(LuaApiTest, AliasOption)
{
    lua_getglobal(L, "test_alias_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_alias_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_alias_option should succeed";
}

// Test 10: AliasOption not found
TEST_F(LuaApiTest, AliasOptionNotFound)
{
    lua_getglobal(L, "test_alias_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_alias_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_alias_option_not_found should succeed";
}

// Test 11: SetAliasOption not found
TEST_F(LuaApiTest, SetAliasOptionNotFound)
{
    lua_getglobal(L, "test_set_alias_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_alias_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_alias_option_not_found should succeed";
}

// Test 12: IsAlias function
TEST_F(LuaApiTest, IsAlias)
{
    lua_getglobal(L, "test_is_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_alias should succeed";
}

// Test 13: GetAlias function
TEST_F(LuaApiTest, GetAlias)
{
    lua_getglobal(L, "test_get_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alias should succeed";
}

// Test 14: DeleteAliasGroup
TEST_F(LuaApiTest, DeleteAliasGroup)
{
    lua_getglobal(L, "test_delete_alias_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_alias_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_alias_group should succeed";
}

// Test 15: DeleteTemporaryAliases
TEST_F(LuaApiTest, DeleteTemporaryAliases)
{
    lua_getglobal(L, "test_delete_temporary_aliases");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_aliases should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_aliases should succeed";
}

// Test 16: GetAliasWildcard
TEST_F(LuaApiTest, GetAliasWildcard)
{
    lua_getglobal(L, "test_get_alias_wildcard");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_alias_wildcard should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_alias_wildcard should succeed";
}

int main(int argc, char** argv)
{
    // Initialize Qt GUI application (required for Lua API operations)
    QGuiApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
