/**
 * test_misc_lua_api_gtest.cpp - GoogleTest version
 * Miscellaneous Lua API Tests
 *
 * Tests miscellaneous Lua API functions that don't fit into other specific test files:
 * - Utility functions (Base64, Hash, Trim, GUID, etc.)
 * - String functions (EditDistance, Replace, Metaphone, etc.)
 * - Text/display functions (Tell, ANSI, ColourNote, etc.)
 * - Script control (Execute, Queue, Trace, Debug, etc.)
 * - System functions (ChangeDir, ExportXML, etc.)
 * - Option functions (GetSetOption, GetAlphaOption, etc.)
 * - Info bar functions (Info, InfoColour, etc.)
 * - Plugin functions (GetPluginList, PluginSupports, etc.)
 * - Connection status and Send functions
 * - UI functions (disabled - require GUI)
 * - And various other miscellaneous functions
 */

#include "lua_api_test_fixture.h"

// Test 3: sendto constant table
TEST_F(LuaApiTest, SendtoTable)
{
    lua_getglobal(L, "sendto");
    ASSERT_TRUE(lua_istable(L, -1)) << "sendto should be a table";

    lua_getfield(L, -1, "Script");
    int script_sendto = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(script_sendto, 12) << "sendto.Script should be 12";
    lua_pop(L, 1); // pop sendto table
}

// Test 14: Verify group functions exist
TEST_F(LuaApiTest, GroupFunctionsExist)
{
    lua_getglobal(L, "test_group_functions_exist");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_group_functions_exist should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_group_functions_exist should succeed";
}

// ========== Utility Function Tests ==========

// Test 39: Base64Encode and Base64Decode
TEST_F(LuaApiTest, Base64)
{
    lua_getglobal(L, "test_base64");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_base64 should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_base64 should succeed";
}

// Test: Base64 Comprehensive
TEST_F(LuaApiTest, Base64Comprehensive)
{
    lua_getglobal(L, "test_base64_comprehensive");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_base64_comprehensive should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_base64_comprehensive should succeed";
}

// Test 40: Hash (SHA-256)
TEST_F(LuaApiTest, Hash)
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
TEST_F(LuaApiTest, Trim)
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
TEST_F(LuaApiTest, CreateGUID)
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
TEST_F(LuaApiTest, GetUniqueNumber)
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
TEST_F(LuaApiTest, GetUniqueID)
{
    lua_getglobal(L, "test_get_unique_id");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_unique_id should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_unique_id should succeed";
}

// ========== Option Tests ==========

// Test: GetSetOption
TEST_F(LuaApiTest, GetSetOption)
{
    lua_getglobal(L, "test_get_set_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_option should succeed";
}

// Test: GetOptionUnknown
TEST_F(LuaApiTest, GetOptionUnknown)
{
    lua_getglobal(L, "test_get_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_option_unknown should succeed";
}

// Test: SetOptionUnknown
TEST_F(LuaApiTest, SetOptionUnknown)
{
    lua_getglobal(L, "test_set_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_option_unknown should succeed";
}

// Test: OptionBoolean
TEST_F(LuaApiTest, OptionBoolean)
{
    lua_getglobal(L, "test_option_boolean");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_option_boolean should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_option_boolean should succeed";
}

// Test: GetSetAlphaOption
TEST_F(LuaApiTest, GetSetAlphaOption)
{
    lua_getglobal(L, "test_get_set_alpha_option");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_set_alpha_option should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_set_alpha_option should succeed";
}

// Test: GetAlphaOptionUnknown
TEST_F(LuaApiTest, GetAlphaOptionUnknown)
{
    lua_getglobal(L, "test_get_alpha_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alpha_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alpha_option_unknown should succeed";
}

// Test: SetAlphaOptionUnknown
TEST_F(LuaApiTest, SetAlphaOptionUnknown)
{
    lua_getglobal(L, "test_set_alpha_option_unknown");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_set_alpha_option_unknown should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_set_alpha_option_unknown should succeed";
}

// Test: GetOptionList
TEST_F(LuaApiTest, GetOptionList)
{
    lua_getglobal(L, "test_get_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_option_list should succeed";
}

// Test: GetAlphaOptionList
TEST_F(LuaApiTest, GetAlphaOptionList)
{
    lua_getglobal(L, "test_get_alpha_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_alpha_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_alpha_option_list should succeed";
}

// ========== Additional Utility Function Tests ==========

// Test 69: EditDistance (Levenshtein distance)
TEST_F(LuaApiTest, EditDistance)
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
TEST_F(LuaApiTest, Replace)
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
TEST_F(LuaApiTest, Metaphone)
{
    lua_getglobal(L, "test_metaphone");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_metaphone should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_metaphone should succeed";
}

// ========== String Utility Tests ==========

// Test 84: StripANSI
TEST_F(LuaApiTest, StripANSI)
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
TEST_F(LuaApiTest, FixupEscapeSequences)
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
TEST_F(LuaApiTest, FixupHTML)
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
TEST_F(LuaApiTest, MakeRegularExpression)
{
    lua_getglobal(L, "test_make_regular_expression");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_make_regular_expression should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_make_regular_expression should succeed";
}

// ========== Notes API ==========

// Test 101: GetNotes and SetNotes
TEST_F(LuaApiTest, GetSetNotes)
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
TEST_F(LuaApiTest, GetLineCount)
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
TEST_F(LuaApiTest, GetLinesInBufferCount)
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
TEST_F(LuaApiTest, Tell)
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
TEST_F(LuaApiTest, ANSI)
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

// ========== Colour Output Functions ==========

// Test 155: ColourNote
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

// Test 156: ColourTell
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

// Test 157: AnsiNote
TEST_F(LuaApiTest, AnsiNote)
{
    lua_getglobal(L, "test_ansi_note");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_ansi_note should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_ansi_note should succeed";
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

// ========== Echo Functions ==========

// Test 111: EchoInput
TEST_F(LuaApiTest, EchoInput)
{
    lua_getglobal(L, "test_echo_input");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_echo_input should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_echo_input should succeed";
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

// ========== Trace/Debug Functions ==========

// Test 121: Trace
TEST_F(LuaApiTest, Trace)
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
TEST_F(LuaApiTest, TraceOut)
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
TEST_F(LuaApiTest, Debug)
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
TEST_F(LuaApiTest, SetGetTrace)
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

// ========== Group Functions ==========

// Test 128: DeleteGroup
TEST_F(LuaApiTest, DeleteGroup)
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
TEST_F(LuaApiTest, EnableGroup)
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
TEST_F(LuaApiTest, GetLineInfo)
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
TEST_F(LuaApiTest, GetRecentLines)
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

// ========== Plugin Functions ==========

// Test 135: GetPluginList
TEST_F(LuaApiTest, GetPluginList)
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
TEST_F(LuaApiTest, GetPluginID)
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
TEST_F(LuaApiTest, GetPluginName)
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
TEST_F(LuaApiTest, PluginSupports)
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
TEST_F(LuaApiTest, DISABLED_Clipboard)
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
TEST_F(LuaApiTest, DISABLED_GetMainWindowPosition)
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
TEST_F(LuaApiTest, DISABLED_SetStatus)
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
TEST_F(LuaApiTest, DISABLED_Repaint)
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
TEST_F(LuaApiTest, DISABLED_Redraw)
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
TEST_F(LuaApiTest, DISABLED_NotepadFunctions)
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
TEST_F(LuaApiTest, SoundFunctions)
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
TEST_F(LuaApiTest, GetGlobalOption)
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
TEST_F(LuaApiTest, GetGlobalOptionList)
{
    lua_getglobal(L, "test_get_global_option_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_global_option_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_global_option_list should succeed";
}

// ========== Misc Functions ==========

// Test 168: TranslateDebug
TEST_F(LuaApiTest, TranslateDebug)
{
    lua_getglobal(L, "test_translate_debug");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_translate_debug should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_translate_debug should succeed";
}

// Test 170: SetChanged
TEST_F(LuaApiTest, SetChanged)
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
TEST_F(LuaApiTest, ChangeDir)
{
    lua_getglobal(L, "test_change_dir");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_change_dir should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_change_dir should succeed";
}

// ========== Export XML ==========

// Test 158: ExportXML
TEST_F(LuaApiTest, ExportXML)
{
    lua_getglobal(L, "test_export_xml");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_export_xml should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_export_xml should succeed";
}

// ========== Info Bar Functions ==========

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

// Test Hyperlink
TEST_F(LuaApiTest, Hyperlink)
{
    lua_getglobal(L, "test_hyperlink");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_hyperlink should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_hyperlink should succeed";

    // Note: Hyperlink output goes to the output window, so we can't easily verify
    // the output here. The test mainly ensures the function doesn't crash.
}

// ========== UI Accelerator Functions (DISABLED - require GUI) ==========

// Test 199: Accelerator - DISABLED: requires GUI keyboard shortcuts
TEST_F(LuaApiTest, DISABLED_LuaAccelerator)
{
    lua_getglobal(L, "test_accelerator");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_accelerator should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_accelerator should succeed";
}

// Test 200: AcceleratorList - DISABLED: requires GUI keyboard shortcuts
TEST_F(LuaApiTest, DISABLED_LuaAcceleratorList)
{
    lua_getglobal(L, "test_accelerator_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_accelerator_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_accelerator_list should succeed";
}

// Test 201: AcceleratorTo - DISABLED: requires GUI keyboard shortcuts
TEST_F(LuaApiTest, DISABLED_LuaAcceleratorTo)
{
    lua_getglobal(L, "test_accelerator_to");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_accelerator_to should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_accelerator_to should succeed";
}

// Test 202: Activate - DISABLED: requires GUI window activation
TEST_F(LuaApiTest, DISABLED_LuaActivate)
{
    lua_getglobal(L, "test_activate");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_activate should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_activate should succeed";
}

// Test 203: ActivateClient - DISABLED: requires GUI window activation
TEST_F(LuaApiTest, DISABLED_LuaActivateClient)
{
    lua_getglobal(L, "test_activate_client");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_activate_client should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_activate_client should succeed";
}

// ========== Spell Check Functions (DISABLED - deprecated stubs) ==========

// Test: SpellCheck
TEST_F(LuaApiTest, DISABLED_LuaSpellCheck)
{
    lua_getglobal(L, "test_spell_check");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_spell_check should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_spell_check should succeed";
}

// Test: SpellCheckDlg
TEST_F(LuaApiTest, DISABLED_LuaSpellCheckDlg)
{
    lua_getglobal(L, "test_spell_check_dlg");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_spell_check_dlg should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_spell_check_dlg should succeed";
}

// Test: SpellCheckCommand
TEST_F(LuaApiTest, DISABLED_LuaSpellCheckCommand)
{
    lua_getglobal(L, "test_spell_check_command");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_spell_check_command should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_spell_check_command should succeed";
}

// Test: AddSpellCheckWord
TEST_F(LuaApiTest, DISABLED_LuaAddSpellCheckWord)
{
    lua_getglobal(L, "test_add_spell_check_word");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_spell_check_word should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_spell_check_word should succeed";
}

// ========== UI Background Functions (DISABLED - require UI) ==========

// Test: SetBackgroundColour
TEST_F(LuaApiTest, DISABLED_LuaSetBackgroundColour)
{
    lua_getglobal(L, "test_set_background_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_background_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_background_colour should succeed";
}

// Test: SetBackgroundImage
TEST_F(LuaApiTest, DISABLED_LuaSetBackgroundImage)
{
    lua_getglobal(L, "test_set_background_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_background_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_background_image should succeed";
}

// Test: SetCursor
TEST_F(LuaApiTest, DISABLED_LuaSetCursor)
{
    lua_getglobal(L, "test_set_cursor");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_cursor should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_cursor should succeed";
}

// Test: SetForegroundImage
TEST_F(LuaApiTest, DISABLED_LuaSetForegroundImage)
{
    lua_getglobal(L, "test_set_foreground_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_foreground_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_foreground_image should succeed";
}

// Test: SetFrameBackgroundColour
TEST_F(LuaApiTest, DISABLED_LuaSetFrameBackgroundColour)
{
    lua_getglobal(L, "test_set_frame_background_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_frame_background_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_frame_background_colour should succeed";
}

// Test: ShowInfoBar
TEST_F(LuaApiTest, DISABLED_LuaShowInfoBar)
{
    lua_getglobal(L, "test_show_info_bar");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_show_info_bar should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_show_info_bar should succeed";
}

// Test: TextRectangle
TEST_F(LuaApiTest, DISABLED_LuaTextRectangle)
{
    lua_getglobal(L, "test_text_rectangle");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_text_rectangle should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_text_rectangle should succeed";
}

// Test: Menu
TEST_F(LuaApiTest, DISABLED_LuaMenu)
{
    lua_getglobal(L, "test_menu");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_menu should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_menu should succeed";
}

// ========== UI Display Functions Tests (DISABLED - require UI) ==========

// Test: AddFont
TEST_F(LuaApiTest, DISABLED_LuaAddFont)
{
    lua_getglobal(L, "test_add_font");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_add_font should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_add_font should succeed";
}

// Test: FlashIcon
TEST_F(LuaApiTest, DISABLED_LuaFlashIcon)
{
    lua_getglobal(L, "test_flash_icon");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_flash_icon should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_flash_icon should succeed";
}

// Test: GetDeviceCaps
TEST_F(LuaApiTest, DISABLED_LuaGetDeviceCaps)
{
    lua_getglobal(L, "test_get_device_caps");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_device_caps should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_device_caps should succeed";
}

// Test: GetSysColor
TEST_F(LuaApiTest, DISABLED_LuaGetSysColor)
{
    lua_getglobal(L, "test_get_sys_color");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_sys_color should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_sys_color should succeed";
}

// Test: GetSystemMetrics
TEST_F(LuaApiTest, DISABLED_LuaGetSystemMetrics)
{
    lua_getglobal(L, "test_get_system_metrics");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_system_metrics should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_system_metrics should succeed";
}

// Test: OpenBrowser
TEST_F(LuaApiTest, DISABLED_LuaOpenBrowser)
{
    lua_getglobal(L, "test_open_browser");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_open_browser should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_open_browser should succeed";
}

// Test: Pause
TEST_F(LuaApiTest, DISABLED_LuaPause)
{
    lua_getglobal(L, "test_pause");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_pause should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_pause should succeed";
}

// Test: PickColour
TEST_F(LuaApiTest, DISABLED_LuaPickColour)
{
    lua_getglobal(L, "test_pick_colour");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_pick_colour should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_pick_colour should succeed";
}

// ========== Network Functions Tests ==========

TEST_F(LuaApiTest, SendNilParameter)
{
    lua_getglobal(L, "test_send_nil_parameter");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_nil_parameter should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_nil_parameter should succeed";
}

TEST_F(LuaApiTest, SendNoEchoNilParameter)
{
    lua_getglobal(L, "test_send_no_echo_nil_parameter");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_no_echo_nil_parameter should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_no_echo_nil_parameter should succeed";
}

TEST_F(LuaApiTest, SendPktNilParameter)
{
    lua_getglobal(L, "test_send_pkt_nil_parameter");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_pkt_nil_parameter should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_pkt_nil_parameter should succeed";
}

TEST_F(LuaApiTest, SendEmptyString)
{
    lua_getglobal(L, "test_send_empty_string");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_empty_string should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_empty_string should succeed";
}

TEST_F(LuaApiTest, SendNoEchoEmptyString)
{
    lua_getglobal(L, "test_send_no_echo_empty_string");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_no_echo_empty_string should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_no_echo_empty_string should succeed";
}

TEST_F(LuaApiTest, SendPktEmptyString)
{
    lua_getglobal(L, "test_send_pkt_empty_string");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_pkt_empty_string should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_pkt_empty_string should succeed";
}

TEST_F(LuaApiTest, SendPktBinaryData)
{
    lua_getglobal(L, "test_send_pkt_binary_data");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_pkt_binary_data should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_pkt_binary_data should succeed";
}

// ========== Encoding and Math Functions Tests ==========

// Test: MtRand
TEST_F(LuaApiTest, MtRand)
{
    lua_getglobal(L, "test_mt_rand");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_mt_rand should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_mt_rand should succeed";
}

// ========== UI Window Position Tests (DISABLED - Require UI) ==========

// Test 204: GetWorldWindowPosition (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_GetWorldWindowPosition)
{
    lua_getglobal(L, "test_get_world_window_position");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_world_window_position should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_world_window_position should succeed";
}

// Test 205: GetWorldWindowPositionX (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_GetWorldWindowPositionX)
{
    lua_getglobal(L, "test_get_world_window_position_x");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_get_world_window_position_x should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_get_world_window_position_x should succeed";
}

// Test 206: MoveMainWindow (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_MoveMainWindow)
{
    lua_getglobal(L, "test_move_main_window");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_move_main_window should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_move_main_window should succeed";
}

// Test 207: MoveWorldWindow (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_MoveWorldWindow)
{
    lua_getglobal(L, "test_move_world_window");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_move_world_window should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_move_world_window should succeed";
}

// Test 208: MoveWorldWindowX (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_MoveWorldWindowX)
{
    lua_getglobal(L, "test_move_world_window_x");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_move_world_window_x should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_move_world_window_x should succeed";
}

// Test 209: SetWorldWindowStatus (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_SetWorldWindowStatus)
{
    lua_getglobal(L, "test_set_world_window_status");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_world_window_status should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_world_window_status should succeed";
}

// Test 210: SetToolBarPosition (DISABLED - requires UI)
TEST_F(LuaApiTest, DISABLED_SetToolBarPosition)
{
    lua_getglobal(L, "test_set_tool_bar_position");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_set_tool_bar_position should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_set_tool_bar_position should succeed";
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
