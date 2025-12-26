/**
 * test_color_lua_api_gtest.cpp - Color API Tests
 *
 * Tests for color-related Lua API functions:
 * - ColourNameToRGB, RGBColourToName
 * - NormalColour, BoldColour, CustomColour
 * - AdjustColour, BlendPixel, FilterPixel
 * - ColourNote, ColourTell
 * - NoteColourRGB, NoteColourName, GetNoteColour
 */

#include "lua_api_test_fixture.h"

// Test 1: ColourNameToRGB basic colors
TEST_F(LuaApiTest, ColourNameToRGBBasic)
{
    lua_getglobal(L, "test_colour_name_to_rgb_basic");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_basic should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_basic should succeed";
}

// Test 2: ColourNameToRGB case insensitivity
TEST_F(LuaApiTest, ColourNameToRGBCase)
{
    lua_getglobal(L, "test_colour_name_to_rgb_case");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_case should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_case should succeed";
}

// Test 3: ColourNameToRGB extended colors
TEST_F(LuaApiTest, ColourNameToRGBExtended)
{
    lua_getglobal(L, "test_colour_name_to_rgb_extended");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_extended should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_extended should succeed";
}

// Test 4: ColourNameToRGB hex values
TEST_F(LuaApiTest, ColourNameToRGBHex)
{
    lua_getglobal(L, "test_colour_name_to_rgb_hex");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_name_to_rgb_hex should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_name_to_rgb_hex should succeed";
}

// Test 5: RGBColourToName
TEST_F(LuaApiTest, RGBColourToName)
{
    lua_getglobal(L, "test_rgb_colour_to_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_rgb_colour_to_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_rgb_colour_to_name should succeed";
}

// Test 6: RGBColourToName unknown color
TEST_F(LuaApiTest, RGBColourToNameUnknown)
{
    lua_getglobal(L, "test_rgb_colour_to_name_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_rgb_colour_to_name_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_rgb_colour_to_name_unknown should succeed";
}

// Test 7: Colour roundtrip
TEST_F(LuaApiTest, ColourRoundtrip)
{
    lua_getglobal(L, "test_colour_roundtrip");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_colour_roundtrip should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_colour_roundtrip should succeed";
}

// Test 8: Normal colour get/set
TEST_F(LuaApiTest, NormalColour)
{
    lua_getglobal(L, "test_normal_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_normal_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_normal_colour should succeed";
}

// Test 9: Bold colour get/set
TEST_F(LuaApiTest, BoldColour)
{
    lua_getglobal(L, "test_bold_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_bold_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_bold_colour should succeed";
}

// Test 10: Custom colour text get/set
TEST_F(LuaApiTest, CustomColourText)
{
    lua_getglobal(L, "test_custom_colour_text");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_custom_colour_text should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_custom_colour_text should succeed";
}

// Test 11: Custom colour background get/set
TEST_F(LuaApiTest, CustomColourBackground)
{
    lua_getglobal(L, "test_custom_colour_background");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_custom_colour_background should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_custom_colour_background should succeed";
}

// Test 12: AdjustColour
TEST_F(LuaApiTest, AdjustColour)
{
    lua_getglobal(L, "test_adjust_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_adjust_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_adjust_colour should succeed";
}

// Test 13: ColourNote
TEST_F(LuaApiTest, ColourNote)
{
    lua_getglobal(L, "test_colour_note");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_colour_note should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_colour_note should succeed";
}

// Test 14: ColourTell
TEST_F(LuaApiTest, ColourTell)
{
    lua_getglobal(L, "test_colour_tell");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_colour_tell should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_colour_tell should succeed";
}

// Test 15: NoteColourRGB
TEST_F(LuaApiTest, NoteColourRGB)
{
    lua_getglobal(L, "test_note_colour_rgb");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_note_colour_rgb should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_note_colour_rgb should succeed";
}

// Test 16: NoteColourName
TEST_F(LuaApiTest, NoteColourName)
{
    lua_getglobal(L, "test_note_colour_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_note_colour_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_note_colour_name should succeed";
}

// Test 17: GetNoteColourFore and GetNoteColourBack in RGB mode
TEST_F(LuaApiTest, GetNoteColourRGBMode)
{
    lua_getglobal(L, "test_get_note_colour_rgb_mode");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_note_colour_rgb_mode should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_note_colour_rgb_mode should succeed";
}

// Test 18: GetNoteColour alias
TEST_F(LuaApiTest, GetNoteColourAlias)
{
    lua_getglobal(L, "test_get_note_colour_alias");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_note_colour_alias should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_note_colour_alias should succeed";
}

// Test 19: NoteColour alpha masking
TEST_F(LuaApiTest, NoteColourAlphaMasking)
{
    lua_getglobal(L, "test_note_colour_alpha_masking");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_note_colour_alpha_masking should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_note_colour_alpha_masking should succeed";
}

// Test 20: BlendPixel
TEST_F(LuaApiTest, BlendPixel)
{
    lua_getglobal(L, "test_blend_pixel");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_blend_pixel should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_blend_pixel should succeed";
}

// Test 21: FilterPixel
TEST_F(LuaApiTest, FilterPixel)
{
    lua_getglobal(L, "test_filter_pixel");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_filter_pixel should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_filter_pixel should succeed";
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
