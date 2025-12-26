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

// Test 14: Verify group functions exist
TEST_F(TriggerAliasAPITest, GroupFunctionsExist)
{
    lua_getglobal(L, "test_group_functions_exist");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_group_functions_exist should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_group_functions_exist should succeed";
}

// Test 15: EnableTriggerGroup API
TEST_F(TriggerAliasAPITest, EnableTriggerGroup)
{
    lua_getglobal(L, "test_enable_trigger_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_trigger_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_trigger_group should succeed";
}

// Test 16: EnableAliasGroup API
TEST_F(TriggerAliasAPITest, EnableAliasGroup)
{
    lua_getglobal(L, "test_enable_alias_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_alias_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_alias_group should succeed";
}

// Test 16: EnableTriggerGroup with empty group
TEST_F(TriggerAliasAPITest, EnableTriggerGroupEmpty)
{
    lua_getglobal(L, "test_enable_trigger_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_trigger_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_trigger_group_empty should succeed";
}

// Test 17: EnableAliasGroup with empty group
TEST_F(TriggerAliasAPITest, EnableAliasGroupEmpty)
{
    lua_getglobal(L, "test_enable_alias_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_alias_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_alias_group_empty should succeed";
}

// Test 18: GetTriggerOption and SetTriggerOption
TEST_F(TriggerAliasAPITest, TriggerOption)
{
    lua_getglobal(L, "test_trigger_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_trigger_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_trigger_option should succeed";
}

// Test 19: GetTriggerOption for non-existent trigger
TEST_F(TriggerAliasAPITest, TriggerOptionNotFound)
{
    lua_getglobal(L, "test_trigger_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_trigger_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_trigger_option_not_found should succeed";
}

// Test 20: SetTriggerOption for non-existent trigger
TEST_F(TriggerAliasAPITest, SetTriggerOptionNotFound)
{
    lua_getglobal(L, "test_set_trigger_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_trigger_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_trigger_option_not_found should succeed";
}

// ========== Timer API Tests ==========

// Test 21: timer_flag constant table
TEST_F(TriggerAliasAPITest, TimerFlagTable)
{
    lua_getglobal(L, "timer_flag");
    ASSERT_TRUE(lua_istable(L, -1)) << "timer_flag should be a table";

    lua_getfield(L, -1, "Enabled");
    int enabled_flag = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(enabled_flag, 1) << "timer_flag.Enabled should be 1";

    lua_getfield(L, -1, "OneShot");
    int oneshot_flag = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(oneshot_flag, 4) << "timer_flag.OneShot should be 4";
    lua_pop(L, 1); // pop timer_flag table
}

// Test 22: AddTimer API
TEST_F(TriggerAliasAPITest, AddTimer)
{
    lua_getglobal(L, "test_add_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_add_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_add_timer should succeed";

    // Verify timer exists in document
    EXPECT_NE(doc->getTimer("test_timer"), nullptr) << "Timer should exist in document";
}

// Test 23: GetTimerInfo API
TEST_F(TriggerAliasAPITest, GetTimerInfo)
{
    // First add the timer
    lua_getglobal(L, "test_add_timer");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test GetTimerInfo
    lua_getglobal(L, "test_get_timer_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_timer_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_timer_info should succeed";
}

// Test 24: EnableTimer API
TEST_F(TriggerAliasAPITest, EnableTimer)
{
    // First add the timer
    lua_getglobal(L, "test_add_timer");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test EnableTimer
    lua_getglobal(L, "test_enable_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_timer should succeed";
}

// Test 25: GetTimerOption and SetTimerOption
TEST_F(TriggerAliasAPITest, TimerOption)
{
    // First add the timer
    lua_getglobal(L, "test_add_timer");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    lua_getglobal(L, "test_timer_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_timer_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_timer_option should succeed";
}

// Test 26: EnableTimerGroup API
TEST_F(TriggerAliasAPITest, EnableTimerGroup)
{
    lua_getglobal(L, "test_enable_timer_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_timer_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_timer_group should succeed";
}

// Test 27: EnableTimerGroup with empty group
TEST_F(TriggerAliasAPITest, EnableTimerGroupEmpty)
{
    lua_getglobal(L, "test_enable_timer_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_timer_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_timer_group_empty should succeed";
}

// Test 28: ResetTimer API
TEST_F(TriggerAliasAPITest, ResetTimer)
{
    // First add the timer
    lua_getglobal(L, "test_add_timer");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    lua_getglobal(L, "test_reset_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_reset_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_reset_timer should succeed";
}

// Test 29: GetTimerOption for non-existent timer
TEST_F(TriggerAliasAPITest, TimerOptionNotFound)
{
    lua_getglobal(L, "test_timer_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_timer_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_timer_option_not_found should succeed";
}

// Test 30: SetTimerOption for non-existent timer
TEST_F(TriggerAliasAPITest, SetTimerOptionNotFound)
{
    lua_getglobal(L, "test_set_timer_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_timer_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_timer_option_not_found should succeed";
}

// Test 31: DeleteTimer API
TEST_F(TriggerAliasAPITest, DeleteTimer)
{
    // First add the timer
    lua_getglobal(L, "test_add_timer");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0);
    lua_pop(L, 1);

    // Now test DeleteTimer
    lua_getglobal(L, "test_delete_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_timer should succeed";

    // Verify timer was deleted from document
    EXPECT_EQ(doc->getTimer("test_timer"), nullptr) << "Timer should be deleted from document";
}

// Test 32: DeleteTimer for non-existent timer
TEST_F(TriggerAliasAPITest, DeleteTimerNotFound)
{
    lua_getglobal(L, "test_delete_timer_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_timer_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_timer_not_found should succeed";
}

// ========== Variable API Tests ==========

// Test 33: SetVariable and GetVariable
TEST_F(TriggerAliasAPITest, SetGetVariable)
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
TEST_F(TriggerAliasAPITest, GetVariableNotFound)
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
TEST_F(TriggerAliasAPITest, GetVariableList)
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
TEST_F(TriggerAliasAPITest, DeleteVariable)
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
TEST_F(TriggerAliasAPITest, DeleteVariableNotFound)
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
TEST_F(TriggerAliasAPITest, VariableEmptyString)
{
    lua_getglobal(L, "test_variable_empty_string");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_variable_empty_string should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_variable_empty_string should succeed";
}

// ========== Utility Function Tests ==========

// Test 39: Base64Encode and Base64Decode
TEST_F(TriggerAliasAPITest, Base64)
{
    lua_getglobal(L, "test_base64");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_base64 should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_base64 should succeed";
}

// Test 40: Hash (SHA-256)
TEST_F(TriggerAliasAPITest, Hash)
{
    lua_getglobal(L, "test_hash");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_hash should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_hash should succeed";
}

// Test 41: Trim
TEST_F(TriggerAliasAPITest, Trim)
{
    lua_getglobal(L, "test_trim");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_trim should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_trim should succeed";
}

// Test 42: CreateGUID
TEST_F(TriggerAliasAPITest, CreateGUID)
{
    lua_getglobal(L, "test_create_guid");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_create_guid should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_create_guid should succeed";
}

// Test 43: GetUniqueNumber
TEST_F(TriggerAliasAPITest, GetUniqueNumber)
{
    lua_getglobal(L, "test_get_unique_number");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_unique_number should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_unique_number should succeed";
}

// Test 44: GetUniqueID
TEST_F(TriggerAliasAPITest, GetUniqueID)
{
    lua_getglobal(L, "test_get_unique_id");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_unique_id should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_unique_id should succeed";
}

// ========== Color Utility Tests ==========

// Test 45: ColourNameToRGB basic colors
TEST_F(TriggerAliasAPITest, ColourNameToRGBBasic)
{
    lua_getglobal(L, "test_colour_name_to_rgb_basic");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_basic should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_basic should succeed";
}

// Test 46: ColourNameToRGB case insensitivity
TEST_F(TriggerAliasAPITest, ColourNameToRGBCase)
{
    lua_getglobal(L, "test_colour_name_to_rgb_case");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_case should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_case should succeed";
}

// Test 47: ColourNameToRGB extended colors
TEST_F(TriggerAliasAPITest, ColourNameToRGBExtended)
{
    lua_getglobal(L, "test_colour_name_to_rgb_extended");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_extended should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_extended should succeed";
}

// Test 48: ColourNameToRGB hex values
TEST_F(TriggerAliasAPITest, ColourNameToRGBHex)
{
    lua_getglobal(L, "test_colour_name_to_rgb_hex");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_hex should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_hex should succeed";
}

// Test 49: RGBColourToName
TEST_F(TriggerAliasAPITest, RGBColourToName)
{
    lua_getglobal(L, "test_rgb_colour_to_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_rgb_colour_to_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_rgb_colour_to_name should succeed";
}

// Test 50: RGBColourToName unknown color
TEST_F(TriggerAliasAPITest, RGBColourToNameUnknown)
{
    lua_getglobal(L, "test_rgb_colour_to_name_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_rgb_colour_to_name_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_rgb_colour_to_name_unknown should succeed";
}

// Test 51: Colour roundtrip
TEST_F(TriggerAliasAPITest, ColourRoundtrip)
{
    lua_getglobal(L, "test_colour_roundtrip");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_roundtrip should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_roundtrip should succeed";
}

// Test 52: Normal colour get/set
TEST_F(TriggerAliasAPITest, NormalColour)
{
    lua_getglobal(L, "test_normal_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_normal_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_normal_colour should succeed";
}

// Test 53: Bold colour get/set
TEST_F(TriggerAliasAPITest, BoldColour)
{
    lua_getglobal(L, "test_bold_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_bold_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_bold_colour should succeed";
}

// Test 54: Custom colour text get/set
TEST_F(TriggerAliasAPITest, CustomColourText)
{
    lua_getglobal(L, "test_custom_colour_text");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_custom_colour_text should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_custom_colour_text should succeed";
}

// Test 55: Custom colour background get/set
TEST_F(TriggerAliasAPITest, CustomColourBackground)
{
    lua_getglobal(L, "test_custom_colour_background");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_custom_colour_background should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_custom_colour_background should succeed";
}

// Test 56: AdjustColour
TEST_F(TriggerAliasAPITest, AdjustColour)
{
    lua_getglobal(L, "test_adjust_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_adjust_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_adjust_colour should succeed";
}

// ========== Options API Tests ==========

// Test 57: GetOption and SetOption
TEST_F(TriggerAliasAPITest, GetSetOption)
{
    lua_getglobal(L, "test_get_set_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_option should succeed";
}

// Test 58: GetOption unknown
TEST_F(TriggerAliasAPITest, GetOptionUnknown)
{
    lua_getglobal(L, "test_get_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_option_unknown should succeed";
}

// Test 59: SetOption unknown
TEST_F(TriggerAliasAPITest, SetOptionUnknown)
{
    lua_getglobal(L, "test_set_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_option_unknown should succeed";
}

// Test 60: Boolean option
TEST_F(TriggerAliasAPITest, OptionBoolean)
{
    lua_getglobal(L, "test_option_boolean");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_option_boolean should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_option_boolean should succeed";
}

// Test 61: GetAlphaOption and SetAlphaOption
TEST_F(TriggerAliasAPITest, GetSetAlphaOption)
{
    lua_getglobal(L, "test_get_set_alpha_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_alpha_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_alpha_option should succeed";
}

// Test 62: GetAlphaOption unknown
TEST_F(TriggerAliasAPITest, GetAlphaOptionUnknown)
{
    lua_getglobal(L, "test_get_alpha_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alpha_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alpha_option_unknown should succeed";
}

// Test 63: SetAlphaOption unknown
TEST_F(TriggerAliasAPITest, SetAlphaOptionUnknown)
{
    lua_getglobal(L, "test_set_alpha_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_alpha_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_alpha_option_unknown should succeed";
}

// Test 64: GetOptionList
TEST_F(TriggerAliasAPITest, GetOptionList)
{
    lua_getglobal(L, "test_get_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_option_list should succeed";
}

// Test 65: GetAlphaOptionList
TEST_F(TriggerAliasAPITest, GetAlphaOptionList)
{
    lua_getglobal(L, "test_get_alpha_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alpha_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alpha_option_list should succeed";
}

// Test 66: GetInfo
TEST_F(TriggerAliasAPITest, GetInfo)
{
    lua_getglobal(L, "test_get_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_info should succeed";
}

// Test 67: GetInfo boolean types
TEST_F(TriggerAliasAPITest, GetInfoBoolean)
{
    lua_getglobal(L, "test_get_info_boolean");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_info_boolean should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_info_boolean should succeed";
}

// Test 68: GetInfo unknown type
TEST_F(TriggerAliasAPITest, GetInfoUnknown)
{
    lua_getglobal(L, "test_get_info_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_info_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_info_unknown should succeed";
}

// ========== Additional Utility Function Tests ==========

// Test 69: EditDistance (Levenshtein distance)
TEST_F(TriggerAliasAPITest, EditDistance)
{
    lua_getglobal(L, "test_edit_distance");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_edit_distance should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_edit_distance should succeed";
}

// Test 70: Replace function
TEST_F(TriggerAliasAPITest, Replace)
{
    lua_getglobal(L, "test_replace");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_replace should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_replace should succeed";
}

// Test 71: Metaphone phonetic encoding
TEST_F(TriggerAliasAPITest, Metaphone)
{
    lua_getglobal(L, "test_metaphone");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_metaphone should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_metaphone should succeed";
}

// Test 72: Version function
TEST_F(TriggerAliasAPITest, Version)
{
    lua_getglobal(L, "test_version");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_version should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_version should succeed";
}

// Test 73: ErrorDesc function
TEST_F(TriggerAliasAPITest, ErrorDesc)
{
    lua_getglobal(L, "test_error_desc");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_error_desc should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_error_desc should succeed";
}

// Test 74: IsTrigger function
TEST_F(TriggerAliasAPITest, IsTrigger)
{
    lua_getglobal(L, "test_is_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_trigger should succeed";
}

// Test 75: IsAlias function
TEST_F(TriggerAliasAPITest, IsAlias)
{
    lua_getglobal(L, "test_is_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_alias should succeed";
}

// Test 76: IsTimer function
TEST_F(TriggerAliasAPITest, IsTimer)
{
    lua_getglobal(L, "test_is_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_timer should succeed";
}

// Test 77: GetTrigger function
TEST_F(TriggerAliasAPITest, GetTrigger)
{
    lua_getglobal(L, "test_get_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_trigger should succeed";
}

// Test 78: GetAlias function
TEST_F(TriggerAliasAPITest, GetAlias)
{
    lua_getglobal(L, "test_get_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alias should succeed";
}

// Test 79: GetTimer function
TEST_F(TriggerAliasAPITest, GetTimer)
{
    lua_getglobal(L, "test_get_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_timer should succeed";
}

// ========== Speedwalk Function Tests ==========

// Test 80: EvaluateSpeedwalk
TEST_F(TriggerAliasAPITest, EvaluateSpeedwalk)
{
    lua_getglobal(L, "test_evaluate_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_evaluate_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_evaluate_speedwalk should succeed";
}

// Test 81: ReverseSpeedwalk
TEST_F(TriggerAliasAPITest, ReverseSpeedwalk)
{
    lua_getglobal(L, "test_reverse_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_reverse_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_reverse_speedwalk should succeed";
}

// Test 82: RemoveBacktracks
TEST_F(TriggerAliasAPITest, RemoveBacktracks)
{
    lua_getglobal(L, "test_remove_backtracks");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_remove_backtracks should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_remove_backtracks should succeed";
}

// Test 83: SpeedwalkDelay get/set
TEST_F(TriggerAliasAPITest, SpeedwalkDelay)
{
    lua_getglobal(L, "test_speedwalk_delay");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_speedwalk_delay should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_speedwalk_delay should succeed";
}

// ========== String Utility Tests ==========

// Test 84: StripANSI
TEST_F(TriggerAliasAPITest, StripANSI)
{
    lua_getglobal(L, "test_strip_ansi");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_strip_ansi should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_strip_ansi should succeed";
}

// Test 85: FixupEscapeSequences
TEST_F(TriggerAliasAPITest, FixupEscapeSequences)
{
    lua_getglobal(L, "test_fixup_escape_sequences");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_fixup_escape_sequences should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_fixup_escape_sequences should succeed";
}

// Test 86: FixupHTML
TEST_F(TriggerAliasAPITest, FixupHTML)
{
    lua_getglobal(L, "test_fixup_html");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_fixup_html should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_fixup_html should succeed";
}

// Test 87: MakeRegularExpression
TEST_F(TriggerAliasAPITest, MakeRegularExpression)
{
    lua_getglobal(L, "test_make_regular_expression");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_make_regular_expression should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_make_regular_expression should succeed";
}

// ========== Alias Options API Tests ==========

// Test 88: Alias Option get/set
TEST_F(TriggerAliasAPITest, AliasOption)
{
    lua_getglobal(L, "test_alias_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_alias_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_alias_option should succeed";
}

// Test 89: AliasOption not found
TEST_F(TriggerAliasAPITest, AliasOptionNotFound)
{
    lua_getglobal(L, "test_alias_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_alias_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_alias_option_not_found should succeed";
}

// Test 90: SetAliasOption not found
TEST_F(TriggerAliasAPITest, SetAliasOptionNotFound)
{
    lua_getglobal(L, "test_set_alias_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_alias_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_alias_option_not_found should succeed";
}

// ========== Delete Group Functions ==========

// Test 91: DeleteTriggerGroup
TEST_F(TriggerAliasAPITest, DeleteTriggerGroup)
{
    lua_getglobal(L, "test_delete_trigger_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_trigger_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_trigger_group should succeed";
}

// Test 92: DeleteAliasGroup
TEST_F(TriggerAliasAPITest, DeleteAliasGroup)
{
    lua_getglobal(L, "test_delete_alias_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_alias_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_alias_group should succeed";
}

// Test 93: DeleteTimerGroup
TEST_F(TriggerAliasAPITest, DeleteTimerGroup)
{
    lua_getglobal(L, "test_delete_timer_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_timer_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_timer_group should succeed";
}

// ========== Delete Temporary Functions ==========

// Test 94: DeleteTemporaryTriggers
TEST_F(TriggerAliasAPITest, DeleteTemporaryTriggers)
{
    lua_getglobal(L, "test_delete_temporary_triggers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_triggers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_triggers should succeed";
}

// Test 95: DeleteTemporaryAliases
TEST_F(TriggerAliasAPITest, DeleteTemporaryAliases)
{
    lua_getglobal(L, "test_delete_temporary_aliases");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_aliases should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_aliases should succeed";
}

// Test 96: DeleteTemporaryTimers
TEST_F(TriggerAliasAPITest, DeleteTemporaryTimers)
{
    lua_getglobal(L, "test_delete_temporary_timers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_timers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_timers should succeed";
}

// ========== World Info Functions ==========

// Test 97: GetWorldID
TEST_F(TriggerAliasAPITest, GetWorldID)
{
    lua_getglobal(L, "test_get_world_id");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_world_id should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_world_id should succeed";
}

// Test 98: WorldName
TEST_F(TriggerAliasAPITest, WorldName)
{
    lua_getglobal(L, "test_world_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_world_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_world_name should succeed";
}

// Test 99: WorldAddress
TEST_F(TriggerAliasAPITest, WorldAddress)
{
    lua_getglobal(L, "test_world_address");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_world_address should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_world_address should succeed";
}

// Test 100: WorldPort
TEST_F(TriggerAliasAPITest, WorldPort)
{
    lua_getglobal(L, "test_world_port");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_world_port should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_world_port should succeed";
}

// ========== Notes API ==========

// Test 101: GetNotes and SetNotes
TEST_F(TriggerAliasAPITest, GetSetNotes)
{
    lua_getglobal(L, "test_get_set_notes");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_notes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_notes should succeed";
}

// ========== Line Buffer Functions ==========

// Test 102: GetLineCount
TEST_F(TriggerAliasAPITest, GetLineCount)
{
    lua_getglobal(L, "test_get_line_count");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_line_count should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_line_count should succeed";
}

// Test 103: GetLinesInBufferCount
TEST_F(TriggerAliasAPITest, GetLinesInBufferCount)
{
    lua_getglobal(L, "test_get_lines_in_buffer_count");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_lines_in_buffer_count should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_lines_in_buffer_count should succeed";
}

// ========== Output Functions ==========

// Test 104: Tell
TEST_F(TriggerAliasAPITest, Tell)
{
    lua_getglobal(L, "test_tell");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_tell should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_tell should succeed";
}

// Test 105: ANSI
TEST_F(TriggerAliasAPITest, ANSI)
{
    lua_getglobal(L, "test_ansi");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_ansi should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_ansi should succeed";
}

// Test 106: Simulate
TEST_F(TriggerAliasAPITest, Simulate)
{
    lua_getglobal(L, "test_simulate");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_simulate should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_simulate should succeed";
}

// ========== Script Info Functions ==========

// Test 107: GetScriptTime
TEST_F(TriggerAliasAPITest, GetScriptTime)
{
    lua_getglobal(L, "test_get_script_time");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_script_time should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_script_time should succeed";
}

// Test 108: GetFrame - DISABLED: requires GUI, returns nil in headless mode
TEST_F(TriggerAliasAPITest, DISABLED_GetFrame)
{
    lua_getglobal(L, "test_get_frame");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_frame should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_frame should succeed";
}

// ========== Command Line Functions ==========

// Test 109: GetCommand and SetCommand - DISABLED: requires GUI input widget
TEST_F(TriggerAliasAPITest, DISABLED_GetSetCommand)
{
    lua_getglobal(L, "test_get_set_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_command should succeed";
}

// Test 110: GetCommandList
TEST_F(TriggerAliasAPITest, GetCommandList)
{
    lua_getglobal(L, "test_get_command_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_command_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_command_list should succeed";
}

// ========== Echo Functions ==========

// Test 111: EchoInput
TEST_F(TriggerAliasAPITest, EchoInput)
{
    lua_getglobal(L, "test_echo_input");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_echo_input should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_echo_input should succeed";
}

// ========== Connection Info ==========

// Test 112: GetConnectDuration
TEST_F(TriggerAliasAPITest, GetConnectDuration)
{
    lua_getglobal(L, "test_get_connect_duration");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_connect_duration should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_connect_duration should succeed";
}

// Test 113: GetReceivedBytes
TEST_F(TriggerAliasAPITest, GetReceivedBytes)
{
    lua_getglobal(L, "test_get_received_bytes");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_received_bytes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_received_bytes should succeed";
}

// Test 114: GetSentBytes
TEST_F(TriggerAliasAPITest, GetSentBytes)
{
    lua_getglobal(L, "test_get_sent_bytes");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_sent_bytes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_sent_bytes should succeed";
}

// ========== DoAfter Functions ==========

// Test 116: DoAfter
TEST_F(TriggerAliasAPITest, DoAfter)
{
    lua_getglobal(L, "test_do_after");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_do_after should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_do_after should succeed";
}

// Test 117: DoAfterNote
TEST_F(TriggerAliasAPITest, DoAfterNote)
{
    lua_getglobal(L, "test_do_after_note");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_do_after_note should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_do_after_note should succeed";
}

// Test 118: DoAfterSpecial
TEST_F(TriggerAliasAPITest, DoAfterSpecial)
{
    lua_getglobal(L, "test_do_after_special");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_do_after_special should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_do_after_special should succeed";
}

// ========== Queue Functions ==========

// Test 119: Queue, GetQueue, DiscardQueue
TEST_F(TriggerAliasAPITest, Queue)
{
    lua_getglobal(L, "test_queue");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_queue should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_queue should succeed";
}

// Test 120: PushCommand
TEST_F(TriggerAliasAPITest, PushCommand)
{
    lua_getglobal(L, "test_push_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_push_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_push_command should succeed";
}

// ========== Trace/Debug Functions ==========

// Test 121: Trace
TEST_F(TriggerAliasAPITest, Trace)
{
    lua_getglobal(L, "test_trace");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_trace should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_trace should succeed";
}

// Test 122: TraceOut
TEST_F(TriggerAliasAPITest, TraceOut)
{
    lua_getglobal(L, "test_trace_out");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_trace_out should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_trace_out should succeed";
}

// Test 123: Debug
TEST_F(TriggerAliasAPITest, Debug)
{
    lua_getglobal(L, "test_debug");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_debug should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_debug should succeed";
}

// Test 124: SetTrace and GetTrace
TEST_F(TriggerAliasAPITest, SetGetTrace)
{
    lua_getglobal(L, "test_set_get_trace");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_get_trace should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_get_trace should succeed";
}

// ========== Execute Function ==========

// Test 125: Execute
TEST_F(TriggerAliasAPITest, Execute)
{
    lua_getglobal(L, "test_execute");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_execute should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_execute should succeed";
}

// Test 126: StopEvaluatingTriggers
TEST_F(TriggerAliasAPITest, StopEvaluatingTriggers)
{
    lua_getglobal(L, "test_stop_evaluating_triggers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_stop_evaluating_triggers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_stop_evaluating_triggers should succeed";
}

// Test 127: ResetTimers
TEST_F(TriggerAliasAPITest, ResetTimers)
{
    lua_getglobal(L, "test_reset_timers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_reset_timers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_reset_timers should succeed";
}

// ========== Group Functions ==========

// Test 128: DeleteGroup
TEST_F(TriggerAliasAPITest, DeleteGroup)
{
    lua_getglobal(L, "test_delete_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_group should succeed";
}

// Test 129: EnableGroup
TEST_F(TriggerAliasAPITest, EnableGroup)
{
    lua_getglobal(L, "test_enable_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_group should succeed";
}

// ========== Line Info Functions ==========

// Test 130: GetLineInfo
TEST_F(TriggerAliasAPITest, GetLineInfo)
{
    lua_getglobal(L, "test_get_line_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_line_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_line_info should succeed";
}

// Test 131: GetRecentLines
TEST_F(TriggerAliasAPITest, GetRecentLines)
{
    lua_getglobal(L, "test_get_recent_lines");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_recent_lines should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_recent_lines should succeed";
}

// Test 132: Selection Functions
TEST_F(TriggerAliasAPITest, Selection)
{
    lua_getglobal(L, "test_selection");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_selection should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_selection should succeed";
}

// ========== Host Info Functions ==========

// Test 133: GetHostAddress
TEST_F(TriggerAliasAPITest, GetHostAddress)
{
    lua_getglobal(L, "test_get_host_address");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_host_address should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_host_address should succeed";
}

// Test 134: GetHostName
TEST_F(TriggerAliasAPITest, GetHostName)
{
    lua_getglobal(L, "test_get_host_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_host_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_host_name should succeed";
}

// ========== Plugin Functions ==========

// Test 135: GetPluginList
TEST_F(TriggerAliasAPITest, GetPluginList)
{
    lua_getglobal(L, "test_get_plugin_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_plugin_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_plugin_list should succeed";
}

// Test 136: GetPluginID
TEST_F(TriggerAliasAPITest, GetPluginID)
{
    lua_getglobal(L, "test_get_plugin_id");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_plugin_id should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_plugin_id should succeed";
}

// Test 137: GetPluginName
TEST_F(TriggerAliasAPITest, GetPluginName)
{
    lua_getglobal(L, "test_get_plugin_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_plugin_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_plugin_name should succeed";
}

// Test 138: PluginSupports
TEST_F(TriggerAliasAPITest, PluginSupports)
{
    lua_getglobal(L, "test_plugin_supports");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_plugin_supports should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_plugin_supports should succeed";
}

// ========== UI Functions (DISABLED - require GUI) ==========

// Test 139: Clipboard - DISABLED: requires GUI clipboard access
TEST_F(TriggerAliasAPITest, DISABLED_Clipboard)
{
    lua_getglobal(L, "test_clipboard");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_clipboard should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_clipboard should succeed";
}

// Test 140: GetMainWindowPosition - DISABLED: requires GUI window
TEST_F(TriggerAliasAPITest, DISABLED_GetMainWindowPosition)
{
    lua_getglobal(L, "test_get_main_window_position");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_main_window_position should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_main_window_position should succeed";
}

// Test 141: SetStatus - DISABLED: requires GUI status bar
TEST_F(TriggerAliasAPITest, DISABLED_SetStatus)
{
    lua_getglobal(L, "test_set_status");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_status should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_status should succeed";
}

// Test 142: Repaint - DISABLED: requires GUI window
TEST_F(TriggerAliasAPITest, DISABLED_Repaint)
{
    lua_getglobal(L, "test_repaint");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_repaint should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_repaint should succeed";
}

// Test 143: Redraw - DISABLED: requires GUI window
TEST_F(TriggerAliasAPITest, DISABLED_Redraw)
{
    lua_getglobal(L, "test_redraw");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_redraw should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_redraw should succeed";
}

// Test 144: Notepad Functions - DISABLED: requires GUI notepad windows
TEST_F(TriggerAliasAPITest, DISABLED_NotepadFunctions)
{
    lua_getglobal(L, "test_notepad_functions");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_notepad_functions should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_notepad_functions should succeed";
}

// Test 145: Sound Functions
TEST_F(TriggerAliasAPITest, SoundFunctions)
{
    lua_getglobal(L, "test_sound_functions");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_sound_functions should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_sound_functions should succeed";
}

// ========== Global Options ==========

// Test 146: GetGlobalOption
TEST_F(TriggerAliasAPITest, GetGlobalOption)
{
    lua_getglobal(L, "test_get_global_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_global_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_global_option should succeed";
}

// Test 147: GetGlobalOptionList
TEST_F(TriggerAliasAPITest, GetGlobalOptionList)
{
    lua_getglobal(L, "test_get_global_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_global_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_global_option_list should succeed";
}

// ========== Logging Config ==========

// Test 148: LogInput
TEST_F(TriggerAliasAPITest, LogInput)
{
    lua_getglobal(L, "test_log_input");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_log_input should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_log_input should succeed";
}

// Test 149: LogOutput
TEST_F(TriggerAliasAPITest, LogOutput)
{
    lua_getglobal(L, "test_log_output");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_log_output should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_log_output should succeed";
}

// Test 150: LogNotes
TEST_F(TriggerAliasAPITest, LogNotes)
{
    lua_getglobal(L, "test_log_notes");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_log_notes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_log_notes should succeed";
}

// ========== Wildcard Functions ==========

// Test 151: GetTriggerWildcard
TEST_F(TriggerAliasAPITest, GetTriggerWildcard)
{
    lua_getglobal(L, "test_get_trigger_wildcard");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_trigger_wildcard should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_trigger_wildcard should succeed";
}

// Test 152: GetAliasWildcard
TEST_F(TriggerAliasAPITest, GetAliasWildcard)
{
    lua_getglobal(L, "test_get_alias_wildcard");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_alias_wildcard should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_alias_wildcard should succeed";
}

// ========== Command History ==========

// Test 153: DeleteCommandHistory
TEST_F(TriggerAliasAPITest, DeleteCommandHistory)
{
    lua_getglobal(L, "test_delete_command_history");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_delete_command_history should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_delete_command_history should succeed";
}

// Test 154: SelectCommand
TEST_F(TriggerAliasAPITest, SelectCommand)
{
    lua_getglobal(L, "test_select_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_select_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_select_command should succeed";
}

// ========== Colour Output Functions ==========

// Test 155: ColourNote
TEST_F(TriggerAliasAPITest, ColourNote)
{
    lua_getglobal(L, "test_colour_note");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_colour_note should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_colour_note should succeed";
}

// Test 156: ColourTell
TEST_F(TriggerAliasAPITest, ColourTell)
{
    lua_getglobal(L, "test_colour_tell");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_colour_tell should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_colour_tell should succeed";
}

// Test 157: AnsiNote
TEST_F(TriggerAliasAPITest, AnsiNote)
{
    lua_getglobal(L, "test_ansi_note");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_ansi_note should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_ansi_note should succeed";
}

// ========== Export XML ==========

// Test 158: ExportXML
TEST_F(TriggerAliasAPITest, ExportXML)
{
    lua_getglobal(L, "test_export_xml");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_export_xml should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_export_xml should succeed";
}

// ========== Extended Miniwindow Tests ==========

// Test 159: WindowArc
TEST_F(TriggerAliasAPITest, WindowArc)
{
    lua_getglobal(L, "test_window_arc");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_arc should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_arc should succeed";
}

// Test 160: WindowBezier
TEST_F(TriggerAliasAPITest, WindowBezier)
{
    lua_getglobal(L, "test_window_bezier");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_bezier should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_bezier should succeed";
}

// Test 161: WindowPolygon
TEST_F(TriggerAliasAPITest, WindowPolygon)
{
    lua_getglobal(L, "test_window_polygon");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_polygon should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_polygon should succeed";
}

// Test 162: WindowGradient
TEST_F(TriggerAliasAPITest, WindowGradient)
{
    lua_getglobal(L, "test_window_gradient");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_gradient should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_gradient should succeed";
}

// Test 163: WindowHotspots
TEST_F(TriggerAliasAPITest, WindowHotspots)
{
    lua_getglobal(L, "test_window_hotspots");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_hotspots should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_hotspots should succeed";
}

// Test 164: WindowHotspotInfo
TEST_F(TriggerAliasAPITest, WindowHotspotInfo)
{
    lua_getglobal(L, "test_window_hotspot_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_hotspot_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_hotspot_info should succeed";
}

// Test 165: WindowImages
TEST_F(TriggerAliasAPITest, WindowImages)
{
    lua_getglobal(L, "test_window_images");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_images should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_images should succeed";
}

// Test 166: WindowPositionResize
TEST_F(TriggerAliasAPITest, WindowPositionResize)
{
    lua_getglobal(L, "test_window_position_resize");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_position_resize should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_position_resize should succeed";
}

// Test 167: WindowZOrder
TEST_F(TriggerAliasAPITest, WindowZOrder)
{
    lua_getglobal(L, "test_window_zorder");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_zorder should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_zorder should succeed";
}

// ========== Misc Functions ==========

// Test 168: TranslateDebug
TEST_F(TriggerAliasAPITest, TranslateDebug)
{
    lua_getglobal(L, "test_translate_debug");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_translate_debug should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_translate_debug should succeed";
}

// Test 169: DoAfterSpeedWalk
TEST_F(TriggerAliasAPITest, DoAfterSpeedWalk)
{
    lua_getglobal(L, "test_do_after_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_do_after_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_do_after_speedwalk should succeed";
}

// Test 170: SetChanged
TEST_F(TriggerAliasAPITest, SetChanged)
{
    lua_getglobal(L, "test_set_changed");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_changed should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_changed should succeed";
}

// Test 171: ChangeDir
TEST_F(TriggerAliasAPITest, ChangeDir)
{
    lua_getglobal(L, "test_change_dir");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_change_dir should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_change_dir should succeed";
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
