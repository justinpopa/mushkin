/**
 * test_trigger_api_gtest.cpp - Trigger API Tests
 *
 * Tests for trigger management functions:
 * - AddTrigger, DeleteTrigger, EnableTrigger
 * - GetTriggerInfo, GetTriggerList, GetTrigger
 * - EnableTriggerGroup, DeleteTriggerGroup
 * - TriggerOption, SetTriggerOption
 * - AddTriggerEx, StopEvaluatingTriggers
 * - ImportXML (for triggers)
 */

#include "lua_api_test_fixture.h"

// Test 1: trigger_flag constant table
TEST_F(LuaApiTest, TriggerFlagTable)
{
    lua_getglobal(L, "trigger_flag");
    ASSERT_TRUE(lua_istable(L, -1)) << "trigger_flag should be a table";

    lua_getfield(L, -1, "Enabled");
    int enabled_flag = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(enabled_flag, 1) << "trigger_flag.Enabled should be 1";
    lua_pop(L, 1); // pop trigger_flag table
}

// Test 4: AddTrigger API
TEST_F(LuaApiTest, AddTrigger)
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
TEST_F(LuaApiTest, GetTriggerInfo)
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
TEST_F(LuaApiTest, GetTriggerList)
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
TEST_F(LuaApiTest, EnableTrigger)
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
TEST_F(LuaApiTest, DeleteTrigger)
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

// Test 15: EnableTriggerGroup API
TEST_F(LuaApiTest, EnableTriggerGroup)
{
    lua_getglobal(L, "test_enable_trigger_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_trigger_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_trigger_group should succeed";
}

// Test 16: EnableTriggerGroup with empty group
TEST_F(LuaApiTest, EnableTriggerGroupEmpty)
{
    lua_getglobal(L, "test_enable_trigger_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_trigger_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_trigger_group_empty should succeed";
}

// Test 18: GetTriggerOption and SetTriggerOption
TEST_F(LuaApiTest, TriggerOption)
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
TEST_F(LuaApiTest, TriggerOptionNotFound)
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
TEST_F(LuaApiTest, SetTriggerOptionNotFound)
{
    lua_getglobal(L, "test_set_trigger_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_trigger_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_trigger_option_not_found should succeed";
}

// Test 74: IsTrigger function
TEST_F(LuaApiTest, IsTrigger)
{
    lua_getglobal(L, "test_is_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_trigger should succeed";
}

// Test 77: GetTrigger function
TEST_F(LuaApiTest, GetTrigger)
{
    lua_getglobal(L, "test_get_trigger");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_trigger should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_trigger should succeed";
}

// Test 91: DeleteTriggerGroup
TEST_F(LuaApiTest, DeleteTriggerGroup)
{
    lua_getglobal(L, "test_delete_trigger_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_trigger_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_trigger_group should succeed";
}

// Test 94: DeleteTemporaryTriggers
TEST_F(LuaApiTest, DeleteTemporaryTriggers)
{
    lua_getglobal(L, "test_delete_temporary_triggers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_triggers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_triggers should succeed";
}

// Test 151: GetTriggerWildcard
TEST_F(LuaApiTest, GetTriggerWildcard)
{
    lua_getglobal(L, "test_get_trigger_wildcard");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_trigger_wildcard should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_trigger_wildcard should succeed";
}

// Test 126: StopEvaluatingTriggers
TEST_F(LuaApiTest, StopEvaluatingTriggers)
{
    lua_getglobal(L, "test_stop_evaluating_triggers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_stop_evaluating_triggers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_stop_evaluating_triggers should succeed";
}

// Test: ImportXML
TEST_F(LuaApiTest, ImportXML)
{
    lua_getglobal(L, "test_import_xml");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_import_xml should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_import_xml should succeed";
}

// Test: ImportXMLMultiple
TEST_F(LuaApiTest, ImportXMLMultiple)
{
    lua_getglobal(L, "test_import_xml_multiple");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_import_xml_multiple should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_import_xml_multiple should succeed";
}

// Test: ImportXMLInvalid
TEST_F(LuaApiTest, ImportXMLInvalid)
{
    lua_getglobal(L, "test_import_xml_invalid");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_import_xml_invalid should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_import_xml_invalid should succeed";
}

// Test: AddTriggerEx
TEST_F(LuaApiTest, AddTriggerEx)
{
    lua_getglobal(L, "test_add_trigger_ex");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex should succeed";
}

// Test: AddTriggerExFlags
TEST_F(LuaApiTest, AddTriggerExFlags)
{
    lua_getglobal(L, "test_add_trigger_ex_flags");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex_flags should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex_flags should succeed";
}

// Test: AddTriggerExScript
TEST_F(LuaApiTest, AddTriggerExScript)
{
    lua_getglobal(L, "test_add_trigger_ex_script");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex_script should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex_script should succeed";
}

// Test: AddTriggerExRegexp
TEST_F(LuaApiTest, AddTriggerExRegexp)
{
    lua_getglobal(L, "test_add_trigger_ex_regexp");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex_regexp should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex_regexp should succeed";
}

// Test: AddTriggerExEmptyMatch
TEST_F(LuaApiTest, AddTriggerExEmptyMatch)
{
    lua_getglobal(L, "test_add_trigger_ex_empty_match");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex_empty_match should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex_empty_match should succeed";
}

// Test: AddTriggerExInvalidSequence
TEST_F(LuaApiTest, AddTriggerExInvalidSequence)
{
    lua_getglobal(L, "test_add_trigger_ex_invalid_sequence");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_trigger_ex_invalid_sequence should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_trigger_ex_invalid_sequence should succeed";
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
