/**
 * test_command_lua_api_gtest.cpp - GoogleTest version
 * Command-related Lua API Test
 *
 * Tests command-related Lua API functions:
 * - DoAfter, DoAfterNote, DoAfterSpecial, DoAfterSpeedWalk
 * - Execute, Simulate
 * - GetCommandList, DeleteCommandHistory, PushCommand, SelectCommand
 * - Selection, Queue
 * - GetCommand/SetCommand (disabled - requires GUI)
 */

#include "lua_api_test_fixture.h"

// Test 106: Simulate
TEST_F(LuaApiTest, Simulate)
{
    lua_getglobal(L, "test_simulate");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_simulate should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_simulate should succeed";
}

// ========== Command Line Functions ==========

// Test 109: GetCommand and SetCommand - DISABLED: requires GUI input widget
TEST_F(LuaApiTest, DISABLED_GetSetCommand)
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
TEST_F(LuaApiTest, GetCommandList)
{
    lua_getglobal(L, "test_get_command_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_command_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_command_list should succeed";
}

// ========== DoAfter Functions ==========

// Test 116: DoAfter
TEST_F(LuaApiTest, DoAfter)
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
TEST_F(LuaApiTest, DoAfterNote)
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
TEST_F(LuaApiTest, DoAfterSpecial)
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
TEST_F(LuaApiTest, Queue)
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
TEST_F(LuaApiTest, PushCommand)
{
    lua_getglobal(L, "test_push_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_push_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_push_command should succeed";
}

// ========== Execute Function ==========

// Test 125: Execute
TEST_F(LuaApiTest, Execute)
{
    lua_getglobal(L, "test_execute");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_execute should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_execute should succeed";
}

// ========== Line Info Functions ==========

// Test 132: Selection Functions
TEST_F(LuaApiTest, Selection)
{
    lua_getglobal(L, "test_selection");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_selection should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_selection should succeed";
}

// ========== Command History ==========

// Test 153: DeleteCommandHistory
TEST_F(LuaApiTest, DeleteCommandHistory)
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
TEST_F(LuaApiTest, SelectCommand)
{
    lua_getglobal(L, "test_select_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_select_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_select_command should succeed";
}

// ========== Misc Functions ==========

// Test 169: DoAfterSpeedWalk
TEST_F(LuaApiTest, DoAfterSpeedWalk)
{
    lua_getglobal(L, "test_do_after_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_do_after_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_do_after_speedwalk should succeed";
}

// Main function
int main(int argc, char** argv)
{
    // Qt application is required for WorldDocument
    QGuiApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
