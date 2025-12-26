/**
 * test_ui_lua_api_gtest.cpp - UI Lua API Tests
 *
 * Tests for UI-related functions via Lua API:
 * - Input: PasteCommand, SetCommandWindowHeight, SetInputFont, SetOutputFont, ShiftTabCompleteItem
 * - Sound: PlaySound, GetSoundStatus
 * - Save/State: Save, SaveState, SetMainTitle, SetScroll, SetTitle, ResetIP
 * - Note styles: NoteStyle, NoteHr, GetStyleInfo
 * - Notepad: ActivateNotepad, SaveNotepad
 */

#include "lua_api_test_fixture.h"

// ========== UI Input Functions Tests ==========

TEST_F(LuaApiTest, PasteCommand)
{
    lua_getglobal(L, "test_paste_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_paste_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_paste_command should succeed";
}

TEST_F(LuaApiTest, SetCommandWindowHeight)
{
    lua_getglobal(L, "test_set_command_window_height");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_command_window_height should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_command_window_height should succeed";
}

TEST_F(LuaApiTest, SetInputFont)
{
    lua_getglobal(L, "test_set_input_font");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_input_font should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_input_font should succeed";
}

TEST_F(LuaApiTest, SetOutputFont)
{
    lua_getglobal(L, "test_set_output_font");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_output_font should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_output_font should succeed";
}

TEST_F(LuaApiTest, ShiftTabCompleteItem)
{
    lua_getglobal(L, "test_shift_tab_complete_item");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_shift_tab_complete_item should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_shift_tab_complete_item should succeed";
}

// ========== UI Sound and Save Functions Tests ==========

TEST_F(LuaApiTest, PlaySound)
{
    lua_getglobal(L, "test_play_sound");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_play_sound should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_play_sound should succeed";
}

TEST_F(LuaApiTest, GetSoundStatus)
{
    lua_getglobal(L, "test_get_sound_status");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_sound_status should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_sound_status should succeed";
}

TEST_F(LuaApiTest, DISABLED_Save)
{
    lua_getglobal(L, "test_save");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_save should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_save should succeed";
}

TEST_F(LuaApiTest, DISABLED_SaveState)
{
    lua_getglobal(L, "test_save_state");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_save_state should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_save_state should succeed";
}

TEST_F(LuaApiTest, DISABLED_SetMainTitle)
{
    lua_getglobal(L, "test_set_main_title");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_main_title should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_main_title should succeed";
}

TEST_F(LuaApiTest, DISABLED_SetScroll)
{
    lua_getglobal(L, "test_set_scroll");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_scroll should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_scroll should succeed";
}

TEST_F(LuaApiTest, DISABLED_SetTitle)
{
    lua_getglobal(L, "test_set_title");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_title should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_title should succeed";
}

TEST_F(LuaApiTest, DISABLED_ResetIP)
{
    lua_getglobal(L, "test_reset_ip");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_reset_ip should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_reset_ip should succeed";
}

// ========== UI Note Style Functions Tests ==========

TEST_F(LuaApiTest, NoteStyle)
{
    lua_getglobal(L, "test_note_style");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_note_style should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_note_style should succeed";
}

TEST_F(LuaApiTest, DISABLED_NoteHr)
{
    lua_getglobal(L, "test_note_hr");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_note_hr should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_note_hr should succeed";
}

TEST_F(LuaApiTest, DISABLED_GetStyleInfo)
{
    lua_getglobal(L, "test_get_style_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_style_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_style_info should succeed";
}

// ========== UI Notepad Functions Tests ==========

TEST_F(LuaApiTest, DISABLED_ActivateNotepad)
{
    lua_getglobal(L, "test_activate_notepad");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_activate_notepad should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_activate_notepad should succeed";
}

TEST_F(LuaApiTest, DISABLED_SaveNotepad)
{
    lua_getglobal(L, "test_save_notepad");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_save_notepad should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_save_notepad should succeed";
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
