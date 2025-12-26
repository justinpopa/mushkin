/**
 * test_database_lua_api_gtest.cpp - GoogleTest version
 * Database Lua API Test
 *
 * Tests all database API functions:
 * - world.DatabaseGetField
 * - world.DatabaseError
 * - world.DatabaseChanges
 * - world.DatabaseTotalChanges
 * - world.DatabaseLastInsertRowid
 * - world.DatabaseInfo
 * - world.DatabaseList
 */

#include "lua_api_test_fixture.h"

// Test DatabaseGetField
TEST_F(LuaApiTest, DatabaseGetField)
{
    lua_getglobal(L, "test_database_get_field");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_get_field should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_get_field should succeed";
}

// Test DatabaseError
TEST_F(LuaApiTest, DatabaseError)
{
    lua_getglobal(L, "test_database_error");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_error should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_error should succeed";
}

// Test DatabaseChanges
TEST_F(LuaApiTest, DatabaseChanges)
{
    lua_getglobal(L, "test_database_changes");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_changes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_changes should succeed";
}

// Test DatabaseTotalChanges
TEST_F(LuaApiTest, DatabaseTotalChanges)
{
    lua_getglobal(L, "test_database_total_changes");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_total_changes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_total_changes should succeed";
}

// Test DatabaseLastInsertRowid
TEST_F(LuaApiTest, DatabaseLastInsertRowid)
{
    lua_getglobal(L, "test_database_last_insert_rowid");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_last_insert_rowid should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_last_insert_rowid should succeed";
}

// Test DatabaseInfo
TEST_F(LuaApiTest, DatabaseInfo)
{
    lua_getglobal(L, "test_database_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_info should succeed";
}

// Test DatabaseList
TEST_F(LuaApiTest, DatabaseList)
{
    lua_getglobal(L, "test_database_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_database_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_database_list should succeed";
}

int main(int argc, char** argv)
{
    // Initialize Qt application (required for WorldDocument)
    QGuiApplication app(argc, argv);

    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
