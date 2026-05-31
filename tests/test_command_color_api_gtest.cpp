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
#include "fixtures/world_fixtures.h"

// Test fixture for command/color API tests
class CommandColorAPITest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();

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
    inputView = new InputView(doc.get(), nullptr);
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
    inputView = new InputView(doc.get(), nullptr);
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
    inputView = new InputView(doc.get(), nullptr);
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
    inputView = new InputView(doc.get(), nullptr);
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
    inputView = new InputView(doc.get(), nullptr);
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
    EXPECT_EQ(doc->m_colors.custom_colour_name[0], "MyRed");
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
    EXPECT_EQ(doc->m_colors.custom_colour_name[0], "123456789012345678901234567890");
    EXPECT_EQ(doc->m_colors.custom_colour_name[0].length(), 30);
}

// Test SetCustomColourName with different color indices
TEST_F(CommandColorAPITest, SetCustomColourNameDifferentValues)
{
    callLuaTest("test_set_custom_colour_name_different_values");

    // Verify C++ side: all three colors should be set
    EXPECT_EQ(doc->m_colors.custom_colour_name[0], "Color1");
    EXPECT_EQ(doc->m_colors.custom_colour_name[1], "Color2");
    EXPECT_EQ(doc->m_colors.custom_colour_name[15], "Color16");
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
    doc->m_colors.custom_colour_name[0] = "TestColor";
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

// ========== ColourNameToRGB Tests ==========

// Helper: call ColourNameToRGB(name) and return the lua_Integer it pushed.
static lua_Integer callColourNameToRGB(lua_State* L, const char* name)
{
    lua_getglobal(L, "ColourNameToRGB");
    lua_pushstring(L, name);
    EXPECT_EQ(lua_pcall(L, 1, 1, 0), 0)
        << "ColourNameToRGB error: " << (lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
    lua_Integer result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return result;
}

// Unknown color names must return -1 (matches original methods_colours.cpp:31-32),
// NOT 4294967295 (0xFFFFFFFF reinterpreted as unsigned on 64-bit). Plugins do
// `if ColourNameToRGB(x) == -1 then ...` and rely on the signed sentinel.
TEST_F(CommandColorAPITest, ColourNameToRGBUnknownReturnsMinusOne)
{
    EXPECT_EQ(callColourNameToRGB(L, "not_a_real_colour"), -1);
}

// Valid named colors must return their positive BGR value, unchanged by the
// sign-extension fix (top byte is zero, so they stay positive).
TEST_F(CommandColorAPITest, ColourNameToRGBKnownReturnsPositiveBGR)
{
    // red => BGR(255,0,0) => 0x000000FF
    EXPECT_EQ(callColourNameToRGB(L, "red"), 0x000000FF);
    // white => BGR(255,255,255) => 0x00FFFFFF, the largest valid value
    EXPECT_EQ(callColourNameToRGB(L, "white"), 0x00FFFFFF);
}

// Hex strings resolve to positive BGR values too (never the -1 sentinel).
TEST_F(CommandColorAPITest, ColourNameToRGBHexReturnsPositive)
{
    // "#0000FF" (HTML blue, RGB) => BGR(0,0,255) => 0x00FF0000
    EXPECT_EQ(callColourNameToRGB(L, "#0000FF"), 0x00FF0000);
}

// ========== Utility Function Tests ==========

// Test GetUdpPort (deprecated, always returns 0)
TEST_F(CommandColorAPITest, GetUdpPort)
{
    callLuaTest("test_get_udp_port");

    // No C++ side effects to verify (function always returns 0)
}

// ========== Font Metric GetInfo Tests ==========

// world.GetInfo() font metrics are derived from the active font on demand, not
// from a 0-initialized member that is never updated. Original returns
// tm.tmAveCharWidth / tm.tmHeight (doc.cpp:3258/3323/3324); Mushkin computes
// these from QFontMetrics. These cases require a GUI app (QFontMetrics needs
// QGuiApplication), which this binary provides via test_main_gui.
TEST_F(CommandColorAPITest, GetInfoFontMetricsNonZero)
{
    executeLua("output_width = world.GetInfo(213)"); // output font width
    executeLua("input_height = world.GetInfo(214)"); // input font height
    executeLua("input_width = world.GetInfo(215)");  // input font width
    executeLua("avg_width = world.GetInfo(240)");    // average char width

    int outputWidth = getGlobalInt("output_width");
    int inputHeight = getGlobalInt("input_height");
    int inputWidth = getGlobalInt("input_width");
    int avgWidth = getGlobalInt("avg_width");

    EXPECT_GT(outputWidth, 0) << "GetInfo(213) output font width must be non-zero";
    EXPECT_GT(inputHeight, 0) << "GetInfo(214) input font height must be non-zero";
    EXPECT_GT(inputWidth, 0) << "GetInfo(215) input font width must be non-zero";
    EXPECT_GT(avgWidth, 0) << "GetInfo(240) average char width must be non-zero";

    // GetInfo(240) average char width mirrors GetInfo(213) output font width.
    EXPECT_EQ(avgWidth, outputWidth)
        << "GetInfo(240) must equal GetInfo(213) (both are output tm.tmAveCharWidth)";
}
