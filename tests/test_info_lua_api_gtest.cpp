/**
 * test_info_lua_api_gtest.cpp - GoogleTest version
 * Info and World Information Lua API Test
 *
 * Tests Lua API functions for world information and info bar display:
 * - GetInfo, GetInfoBoolean, GetInfoUnknown
 * - Info, InfoClear, InfoColour, InfoBackground, InfoFont
 * - GetWorldID, WorldName, WorldAddress, WorldPort
 * - Version, GetScriptTime, ErrorDesc
 */

#include "lua_api_test_fixture.h"

// Test 66: GetInfo generic types
TEST_F(LuaApiTest, GetInfo)
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
TEST_F(LuaApiTest, GetInfoBoolean)
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
TEST_F(LuaApiTest, GetInfoUnknown)
{
    lua_getglobal(L, "test_get_info_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_info_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_info_unknown should succeed";
}

// Test 72: Version
TEST_F(LuaApiTest, Version)
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
TEST_F(LuaApiTest, ErrorDesc)
{
    lua_getglobal(L, "test_error_desc");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_error_desc should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_error_desc should succeed";
}

// Test 97: GetWorldID
TEST_F(LuaApiTest, GetWorldID)
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
TEST_F(LuaApiTest, WorldName)
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
TEST_F(LuaApiTest, WorldAddress)
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
TEST_F(LuaApiTest, WorldPort)
{
    lua_getglobal(L, "test_world_port");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_world_port should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_world_port should succeed";
}

// Test 107: GetScriptTime
TEST_F(LuaApiTest, GetScriptTime)
{
    lua_getglobal(L, "test_get_script_time");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_script_time should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_script_time should succeed";
}

// ========== Info Bar Functions Tests ==========

// Test Info
TEST_F(LuaApiTest, Info)
{
    lua_getglobal(L, "test_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_info should succeed";

    // Verify info bar text was set (should be "Hello World")
    EXPECT_EQ(doc->m_infoBarText, QString("Hello World")) << "Info bar text should be 'Hello World'";
}

// Test InfoClear
TEST_F(LuaApiTest, InfoClear)
{
    // Set some info bar state first
    doc->m_infoBarText = "Test content";
    doc->m_infoBarTextColor = qRgb(255, 0, 0); // Red
    doc->m_infoBarBackColor = qRgb(0, 0, 255); // Blue
    doc->m_infoBarFontName = "Arial";
    doc->m_infoBarFontSize = 20;
    doc->m_infoBarFontStyle = 1; // Bold

    lua_getglobal(L, "test_info_clear");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_info_clear should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_info_clear should succeed";

    // Verify everything was reset to defaults
    EXPECT_TRUE(doc->m_infoBarText.isEmpty()) << "Info bar text should be empty";
    // Colors stored without alpha channel
    EXPECT_EQ(doc->m_infoBarTextColor, 0x000000u) << "Text color should be black";
    EXPECT_EQ(doc->m_infoBarBackColor, 0xFFFFFFu) << "Background color should be white";
    EXPECT_EQ(doc->m_infoBarFontName, QString("Courier New")) << "Font should be Courier New";
    EXPECT_EQ(doc->m_infoBarFontSize, 10) << "Font size should be 10";
    EXPECT_EQ(doc->m_infoBarFontStyle, 0) << "Font style should be 0 (normal)";
}

// Test InfoColour
TEST_F(LuaApiTest, InfoColour)
{
    lua_getglobal(L, "test_info_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_info_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_info_colour should succeed";

    // Verify last color set was navy - RGB(0, 0, 128) = BGR 0x00800000
    EXPECT_EQ(doc->m_infoBarTextColor, static_cast<QRgb>(0x00800000)) << "Text color should be navy";
}

// Test InfoBackground
TEST_F(LuaApiTest, InfoBackground)
{
    lua_getglobal(L, "test_info_background");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_info_background should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_info_background should succeed";

    // Verify last color set was magenta - RGB(255, 0, 255) = BGR 0x00FF00FF
    EXPECT_EQ(doc->m_infoBarBackColor, static_cast<QRgb>(0x00FF00FF)) << "Background color should be magenta";
}

// Test InfoFont
TEST_F(LuaApiTest, InfoFont)
{
    lua_getglobal(L, "test_info_font");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_info_font should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_info_font should succeed";

    // Verify last font settings (Arial, size 12)
    EXPECT_EQ(doc->m_infoBarFontName, QString("Arial")) << "Font should be Arial";
    EXPECT_EQ(doc->m_infoBarFontSize, 12) << "Font size should be 12";
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt GUI application (required for info bar operations)
    QGuiApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
