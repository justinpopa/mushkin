/**
 * test_timer_api_gtest.cpp - Timer API Tests
 *
 * Tests for timer management functions:
 * - AddTimer, DeleteTimer, EnableTimer, ResetTimer
 * - GetTimerInfo, GetTimer
 * - EnableTimerGroup, DeleteTimerGroup
 * - TimerOption, SetTimerOption
 * - ResetTimers
 */

#include "lua_api_test_fixture.h"

// Test 1: timer_flag constant table
TEST_F(LuaApiTest, TimerFlagTable)
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

// Test 2: AddTimer API
TEST_F(LuaApiTest, AddTimer)
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

// Test 3: GetTimerInfo API
TEST_F(LuaApiTest, GetTimerInfo)
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

// Test 4: EnableTimer API
TEST_F(LuaApiTest, EnableTimer)
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

// Test 5: GetTimerOption and SetTimerOption
TEST_F(LuaApiTest, TimerOption)
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

// Test 6: EnableTimerGroup API
TEST_F(LuaApiTest, EnableTimerGroup)
{
    lua_getglobal(L, "test_enable_timer_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_timer_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_timer_group should succeed";
}

// Test 7: EnableTimerGroup with empty group
TEST_F(LuaApiTest, EnableTimerGroupEmpty)
{
    lua_getglobal(L, "test_enable_timer_group_empty");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_enable_timer_group_empty should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_enable_timer_group_empty should succeed";
}

// Test 8: ResetTimer API
TEST_F(LuaApiTest, ResetTimer)
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

// Test 9: GetTimerOption for non-existent timer
TEST_F(LuaApiTest, TimerOptionNotFound)
{
    lua_getglobal(L, "test_timer_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_timer_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_timer_option_not_found should succeed";
}

// Test 10: SetTimerOption for non-existent timer
TEST_F(LuaApiTest, SetTimerOptionNotFound)
{
    lua_getglobal(L, "test_set_timer_option_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_timer_option_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_timer_option_not_found should succeed";
}

// Test 11: DeleteTimer API
TEST_F(LuaApiTest, DeleteTimer)
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

// Test 12: DeleteTimer for non-existent timer
TEST_F(LuaApiTest, DeleteTimerNotFound)
{
    lua_getglobal(L, "test_delete_timer_not_found");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_timer_not_found should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_timer_not_found should succeed";
}

// Test 13: IsTimer function
TEST_F(LuaApiTest, IsTimer)
{
    lua_getglobal(L, "test_is_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_is_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_is_timer should succeed";
}

// Test 14: GetTimer function
TEST_F(LuaApiTest, GetTimer)
{
    lua_getglobal(L, "test_get_timer");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_timer should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_timer should succeed";
}

// Test 15: DeleteTimerGroup function
TEST_F(LuaApiTest, DeleteTimerGroup)
{
    lua_getglobal(L, "test_delete_timer_group");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_timer_group should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_timer_group should succeed";
}

// Test 16: DeleteTemporaryTimers function
TEST_F(LuaApiTest, DeleteTemporaryTimers)
{
    lua_getglobal(L, "test_delete_temporary_timers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_delete_temporary_timers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_delete_temporary_timers should succeed";
}

// Test 17: ResetTimers function
TEST_F(LuaApiTest, ResetTimers)
{
    lua_getglobal(L, "test_reset_timers");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_reset_timers should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_reset_timers should succeed";
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
