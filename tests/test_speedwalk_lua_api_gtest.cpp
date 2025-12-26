/**
 * test_speedwalk_lua_api_gtest.cpp - GoogleTest version
 * Speedwalk Lua API Test
 *
 * Tests speedwalk-related Lua API functions:
 * - world.EvaluateSpeedwalk - Evaluates a speedwalk string into individual commands
 * - world.ReverseSpeedwalk - Reverses a speedwalk string
 * - world.RemoveBacktracks - Removes backtracking from speedwalk paths
 * - world.SetSpeedWalkDelay/GetSpeedWalkDelay - Get/set speedwalk delay
 */

#include "lua_api_test_fixture.h"

// Test: EvaluateSpeedwalk
TEST_F(LuaApiTest, EvaluateSpeedwalk)
{
    lua_getglobal(L, "test_evaluate_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_evaluate_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_evaluate_speedwalk should succeed";
}

// Test: ReverseSpeedwalk
TEST_F(LuaApiTest, ReverseSpeedwalk)
{
    lua_getglobal(L, "test_reverse_speedwalk");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_reverse_speedwalk should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_reverse_speedwalk should succeed";
}

// Test: RemoveBacktracks
TEST_F(LuaApiTest, RemoveBacktracks)
{
    lua_getglobal(L, "test_remove_backtracks");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_remove_backtracks should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_remove_backtracks should succeed";
}

// Test: SpeedwalkDelay get/set
TEST_F(LuaApiTest, SpeedwalkDelay)
{
    lua_getglobal(L, "test_speedwalk_delay");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_speedwalk_delay should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_speedwalk_delay should succeed";
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
