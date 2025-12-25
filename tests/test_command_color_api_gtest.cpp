/**
 * test_command_color_api_gtest.cpp - GoogleTest version
 * Command Window and Color Lua API Test
 *
 * Tests command window and color API functions:
 * - world.GetCommand, SetCommand, SetCommandSelection
 * - world.SetCustomColourName
 * - world.GetUdpPort
 */

#include "../src/ui/views/input_view.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for command/color API tests
class CommandColorAPITest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;

        // Load test script file (relative to build/tests directory)
        if (luaL_dofile(L, "tests/test_api.lua") != 0) {
            const char* error = lua_tostring(L, -1);
            lua_pop(L, 1);
            FAIL() << "Could not load test script: " << error;
        }
    }

    void TearDown() override
    {
        delete inputView;
        delete doc;
    }

    // Helper to call a Lua test function
    void callLuaTest(const char* functionName)
    {
        lua_getglobal(L, functionName);
        int pcall_result = lua_pcall(L, 0, 1, 0);

        ASSERT_EQ(pcall_result, 0)
            << functionName
            << " should not error: " << (pcall_result != 0 ? lua_tostring(L, -1) : "");

        int result = lua_tointeger(L, -1);
        lua_pop(L, 1);

        EXPECT_EQ(result, 0) << functionName << " should succeed";
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
    InputView* inputView = nullptr;
};

// ========== Command Window Tests ==========

// Test GetCommand returns empty string when no InputView
TEST_F(CommandColorAPITest, GetCommandNoInputView)
{
    callLuaTest("test_get_command");

    // Verify C++ side: should return empty string
    QString cmd = doc->GetCommand();
    EXPECT_EQ(cmd, "");
}

// Test GetCommand with InputView
TEST_F(CommandColorAPITest, GetCommandWithInputView)
{
    // Create InputView and set it
    inputView = new InputView(doc, nullptr);
    doc->setActiveInputView(inputView);

    // Set some text
    inputView->setText("hello world");

    // Call Lua test
    callLuaTest("test_get_command");

    // Verify GetCommand returns the text
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetCommand");
    lua_pcall(L, 0, 1, 0);
    const char* cmd = lua_tostring(L, -1);
    EXPECT_STREQ(cmd, "hello world");
    lua_pop(L, 2); // pop result and world table
}

// Test SetCommand on empty input
TEST_F(CommandColorAPITest, SetCommandValid)
{
    // Create InputView (empty by default)
    inputView = new InputView(doc, nullptr);
    doc->setActiveInputView(inputView);

    // Call Lua test (should succeed)
    callLuaTest("test_set_command_valid");

    // Verify C++ side: text should be set
    EXPECT_EQ(inputView->text(), "test command");
}

// Test SetCommand when input not empty (should fail with eCommandNotEmpty)
TEST_F(CommandColorAPITest, SetCommandNotEmpty)
{
    // Create InputView with existing text
    inputView = new InputView(doc, nullptr);
    inputView->setText("existing text");
    doc->setActiveInputView(inputView);

    // Call Lua test (should return eCommandNotEmpty)
    callLuaTest("test_set_command_not_empty");

    // Verify C++ side: text should NOT have changed
    EXPECT_EQ(inputView->text(), "existing text");
}

// Test SetCommandSelection
TEST_F(CommandColorAPITest, SetCommandSelection)
{
    // Create InputView with text
    inputView = new InputView(doc, nullptr);
    inputView->setText("test command");
    doc->setActiveInputView(inputView);

    // Call Lua test (selects chars 1-4 "test", 1-based)
    callLuaTest("test_set_command_selection");

    // Verify C++ side: selection should be set
    EXPECT_TRUE(inputView->hasSelectedText());
    EXPECT_EQ(inputView->selectedText(), "test");
}

// Test SetCommandSelection with -1 (end of text)
TEST_F(CommandColorAPITest, SetCommandSelectionEnd)
{
    // Create InputView with text
    inputView = new InputView(doc, nullptr);
    inputView->setText("test command");
    doc->setActiveInputView(inputView);

    // Call Lua test (selects from position 6 to end "command", 1-based)
    callLuaTest("test_set_command_selection_end");

    // Verify C++ side: selection should be set
    EXPECT_TRUE(inputView->hasSelectedText());
    EXPECT_EQ(inputView->selectedText(), "command");
}

// ========== Color Function Tests ==========

// Test SetCustomColourName with valid input
TEST_F(CommandColorAPITest, SetCustomColourNameValid)
{
    callLuaTest("test_set_custom_colour_name_valid");

    // Verify C++ side: color name should be set
    EXPECT_EQ(doc->m_strCustomColourName[0], "MyRed");
}

// Test SetCustomColourName with out of range index
TEST_F(CommandColorAPITest, SetCustomColourNameOutOfRange)
{
    callLuaTest("test_set_custom_colour_name_out_of_range");

    // No side effects to verify (function returns error)
}

// Test SetCustomColourName with empty name
TEST_F(CommandColorAPITest, SetCustomColourNameEmpty)
{
    callLuaTest("test_set_custom_colour_name_empty");

    // No side effects to verify (function returns error)
}

// Test SetCustomColourName with name too long
TEST_F(CommandColorAPITest, SetCustomColourNameTooLong)
{
    callLuaTest("test_set_custom_colour_name_too_long");

    // No side effects to verify (function returns error)
}

// Test SetCustomColourName with max length name (30 chars)
TEST_F(CommandColorAPITest, SetCustomColourNameMaxLength)
{
    callLuaTest("test_set_custom_colour_name_max_length");

    // Verify C++ side: 30-char name should be set
    EXPECT_EQ(doc->m_strCustomColourName[0], "123456789012345678901234567890");
    EXPECT_EQ(doc->m_strCustomColourName[0].length(), 30);
}

// Test SetCustomColourName with different color indices
TEST_F(CommandColorAPITest, SetCustomColourNameDifferentValues)
{
    callLuaTest("test_set_custom_colour_name_different_values");

    // Verify C++ side: all three colors should be set
    EXPECT_EQ(doc->m_strCustomColourName[0], "Color1");
    EXPECT_EQ(doc->m_strCustomColourName[1], "Color2");
    EXPECT_EQ(doc->m_strCustomColourName[15], "Color16");
}

// Test SetCustomColourName marks document as modified
TEST_F(CommandColorAPITest, SetCustomColourNameModifiesDocument)
{
    // Reset modified flag
    doc->m_bModified = false;

    // Set color name
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "SetCustomColourName");
    lua_pushinteger(L, 1);
    lua_pushstring(L, "TestColor");
    lua_pcall(L, 2, 1, 0);
    lua_pop(L, 2); // pop result and world table

    // Verify document is marked as modified
    EXPECT_TRUE(doc->m_bModified);
}

// Test SetCustomColourName doesn't modify if name unchanged
TEST_F(CommandColorAPITest, SetCustomColourNameNoChangeIfSame)
{
    // Set initial name
    doc->m_strCustomColourName[0] = "TestColor";
    doc->m_bModified = false;

    // Set same name again
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "SetCustomColourName");
    lua_pushinteger(L, 1);
    lua_pushstring(L, "TestColor");
    lua_pcall(L, 2, 1, 0);
    lua_pop(L, 2); // pop result and world table

    // Document should NOT be marked as modified (name didn't change)
    EXPECT_FALSE(doc->m_bModified);
}

// ========== Utility Function Tests ==========

// Test GetUdpPort (deprecated, always returns 0)
TEST_F(CommandColorAPITest, GetUdpPort)
{
    callLuaTest("test_get_udp_port");

    // No C++ side effects to verify (function always returns 0)
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for QWidget types like InputView)
    QApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
