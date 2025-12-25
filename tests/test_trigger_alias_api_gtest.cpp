/**
 * test_trigger_alias_api_gtest.cpp - GoogleTest version
 * Trigger/Alias Lua API Test
 *
 * Tests all trigger and alias API functions:
 * - world.AddTrigger, DeleteTrigger, EnableTrigger, GetTriggerInfo, GetTriggerList
 * - world.AddAlias, DeleteAlias, EnableAlias, GetAliasInfo, GetAliasList
 * - trigger_flag, alias_flag, sendto constant tables
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

// Test fixture for trigger/alias API tests
class TriggerAliasAPITest : public ::testing::Test {
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

// Test 1: trigger_flag constant table
TEST_F(TriggerAliasAPITest, TriggerFlagTable)
{
    lua_getglobal(L, "trigger_flag");
    ASSERT_TRUE(lua_istable(L, -1)) << "trigger_flag should be a table";

    lua_getfield(L, -1, "Enabled");
    int enabled_flag = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(enabled_flag, 1) << "trigger_flag.Enabled should be 1";
    lua_pop(L, 1); // pop trigger_flag table
}

// Test 2: alias_flag constant table
TEST_F(TriggerAliasAPITest, AliasFlagTable)
{
    lua_getglobal(L, "alias_flag");
    ASSERT_TRUE(lua_istable(L, -1)) << "alias_flag should be a table";
    lua_pop(L, 1);
}

// Test 3: sendto constant table
TEST_F(TriggerAliasAPITest, SendtoTable)
{
    lua_getglobal(L, "sendto");
    ASSERT_TRUE(lua_istable(L, -1)) << "sendto should be a table";

    lua_getfield(L, -1, "Script");
    int script_sendto = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(script_sendto, 12) << "sendto.Script should be 12";
    lua_pop(L, 1); // pop sendto table
}

// Test 4: AddTrigger API
TEST_F(TriggerAliasAPITest, AddTrigger)
{
    lua_getglobal(L, "test_add_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_add_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_add_trigger should succeed";

    // Verify trigger was added to document
    EXPECT_NE(doc->getTrigger("test_trigger"), nullptr) << "Trigger should be added to document";
}

// Test 5: GetTriggerInfo API
TEST_F(TriggerAliasAPITest, GetTriggerInfo)
{
    // First add the trigger
    lua_getglobal(L, "test_add_trigger");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test GetTriggerInfo
    lua_getglobal(L, "test_get_trigger_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_trigger_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_trigger_info should succeed";
}

// Test 6: GetTriggerList API
TEST_F(TriggerAliasAPITest, GetTriggerList)
{
    // First add the trigger
    lua_getglobal(L, "test_add_trigger");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test GetTriggerList
    lua_getglobal(L, "test_get_trigger_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_trigger_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_trigger_list should succeed";
}

// Test 7: EnableTrigger API
TEST_F(TriggerAliasAPITest, EnableTrigger)
{
    // First add the trigger
    lua_getglobal(L, "test_add_trigger");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test EnableTrigger
    lua_getglobal(L, "test_enable_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_trigger should succeed";
}

// Test 8: DeleteTrigger API
TEST_F(TriggerAliasAPITest, DeleteTrigger)
{
    // First add the trigger
    lua_getglobal(L, "test_add_trigger");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test DeleteTrigger
    lua_getglobal(L, "test_delete_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_trigger should succeed";

    // Verify trigger was deleted from document
    EXPECT_EQ(doc->getTrigger("test_trigger"), nullptr)
        << "Trigger should be deleted from document";
}

// Test 9: AddAlias API
TEST_F(TriggerAliasAPITest, AddAlias)
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

// Test 10: GetAliasInfo API
TEST_F(TriggerAliasAPITest, GetAliasInfo)
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

// Test 11: GetAliasList API
TEST_F(TriggerAliasAPITest, GetAliasList)
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

// Test 12: EnableAlias API
TEST_F(TriggerAliasAPITest, EnableAlias)
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

// Test 13: DeleteAlias API
TEST_F(TriggerAliasAPITest, DeleteAlias)
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

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
