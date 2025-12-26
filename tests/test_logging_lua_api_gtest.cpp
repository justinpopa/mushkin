/**
 * test_logging_lua_api_gtest.cpp - GoogleTest version
 * Logging Lua API Tests
 *
 * Tests all logging-related Lua API functions:
 * - OpenLog, CloseLog - Open and close log files
 * - WriteLog - Write text to log file
 * - FlushLog - Flush log buffer to disk
 * - IsLogOpen - Check if log is currently open
 * - LogInput - Enable/disable input logging
 * - LogOutput - Enable/disable output logging
 * - LogNotes - Enable/disable notes logging
 * - LogSend - Enable/disable send logging
 */

#include "lua_api_test_fixture.h"
#include <QFile>

// ========== Logging Config ==========

// Test 148: LogInput
TEST_F(LuaApiTest, LogInput)
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
TEST_F(LuaApiTest, LogOutput)
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
TEST_F(LuaApiTest, LogNotes)
{
    lua_getglobal(L, "test_log_notes");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_log_notes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_log_notes should succeed";
}

// ========== Logging Functions ==========

// Test 175: OpenLog and CloseLog
TEST_F(LuaApiTest, OpenCloseLog)
{
    lua_getglobal(L, "test_open_close_log");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_open_close_log should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_open_close_log should succeed";

    // Clean up test file
    QFile::remove("test_log.txt");
}

// Test 176: WriteLog
TEST_F(LuaApiTest, WriteLog)
{
    lua_getglobal(L, "test_write_log");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_write_log should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_write_log should succeed";

    // Clean up test file
    QFile::remove("test_log_write.txt");
}

// Test 177: FlushLog
TEST_F(LuaApiTest, FlushLog)
{
    lua_getglobal(L, "test_flush_log");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_flush_log should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_flush_log should succeed";

    // Clean up test file
    QFile::remove("test_log_flush.txt");
}

// Test 178: IsLogOpen
TEST_F(LuaApiTest, IsLogOpen)
{
    lua_getglobal(L, "test_is_log_open");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_is_log_open should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_is_log_open should succeed";

    // Clean up test file
    QFile::remove("test_log_status.txt");
}

// Test 179: OpenLog with append mode
TEST_F(LuaApiTest, OpenLogAppend)
{
    lua_getglobal(L, "test_open_log_append");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_open_log_append should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_open_log_append should succeed";

    // Clean up test file
    QFile::remove("test_log_append.txt");
}

// Test 180: LogSend
TEST_F(LuaApiTest, LogSend)
{
    lua_getglobal(L, "test_log_send");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_log_send should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_log_send should succeed";
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QGuiApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
