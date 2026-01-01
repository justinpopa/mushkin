/**
 * test_utility_string_functions_gtest.cpp - GoogleTest version
 * Utility String Functions Test
 *
 * Tests for StripANSI, FixupEscapeSequences, FixupHTML, and MakeRegularExpression
 */

#include "test_qt_static.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture
class UtilityStringFunctionsTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// ========== StripANSI Tests ==========

TEST_F(UtilityStringFunctionsTest, StripANSIExists)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "StripANSI");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "world.StripANSI should exist";
    lua_pop(L, 2);
}

TEST_F(UtilityStringFunctionsTest, StripANSIBasicColorCode)
{
    const char* code = "return world.StripANSI('\\27[31mRed text\\27[0m')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Red text");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, StripANSIMultipleCodes)
{
    const char* code = "return world.StripANSI('\\27[1;31mBold Red\\27[0m Normal')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Bold Red Normal");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, StripANSINoEscapes)
{
    const char* code = "return world.StripANSI('Plain text')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Plain text");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, StripANSIComplexMUDOutput)
{
    const char* code =
        "return world.StripANSI('\\27[32mHP:\\27[0m 100/100 \\27[34mMP:\\27[0m 50/50')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "HP: 100/100 MP: 50/50");
    lua_pop(L, 1);
}

// ========== FixupEscapeSequences Tests ==========

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesExists)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "FixupEscapeSequences");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "world.FixupEscapeSequences should exist";
    lua_pop(L, 2);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesNewline)
{
    const char* code = R"(return world.FixupEscapeSequences('Line1\nLine2'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Line1\nLine2");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesTab)
{
    const char* code = R"(return world.FixupEscapeSequences('Col1\tCol2'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Col1\tCol2");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesBackslash)
{
    const char* code = R"(return world.FixupEscapeSequences('Path\\\\to\\\\file'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Path\\to\\file");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesHexEscape)
{
    const char* code = R"(return world.FixupEscapeSequences('ASCII \x41 is A'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "ASCII A is A");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesAllBasic)
{
    const char* code = R"(
        local result = world.FixupEscapeSequences('\\a\\b\\f\\n\\r\\t\\v')
        -- Check that we got 7 special characters
        return #result == 7
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupEscapeSequencesQuotes)
{
    const char* code = R"(return world.FixupEscapeSequences("Say \\'hello\\' and \\\"hi\\\""))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Say 'hello' and \"hi\"");
    lua_pop(L, 1);
}

// ========== FixupHTML Tests ==========

TEST_F(UtilityStringFunctionsTest, FixupHTMLExists)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "FixupHTML");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "world.FixupHTML should exist";
    lua_pop(L, 2);
}

TEST_F(UtilityStringFunctionsTest, FixupHTMLLessThan)
{
    const char* code = "return world.FixupHTML('<tag>')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "&lt;tag&gt;");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupHTMLAmpersand)
{
    const char* code = "return world.FixupHTML('Tom & Jerry')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Tom &amp; Jerry");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupHTMLQuote)
{
    const char* code = R"(return world.FixupHTML('Say "hello"'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Say &quot;hello&quot;");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupHTMLAllSpecialChars)
{
    const char* code = R"(return world.FixupHTML('<div>"A & B"</div>'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "&lt;div&gt;&quot;A &amp; B&quot;&lt;/div&gt;");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, FixupHTMLNoSpecialChars)
{
    const char* code = "return world.FixupHTML('Plain text')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Plain text");
    lua_pop(L, 1);
}

// ========== MakeRegularExpression Tests ==========

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionExists)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "MakeRegularExpression");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "world.MakeRegularExpression should exist";
    lua_pop(L, 2);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionSimple)
{
    const char* code = "return world.MakeRegularExpression('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "^hello$");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionEscapesSpecialChars)
{
    const char* code = "return world.MakeRegularExpression('2 + 2 = 4')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "^2 \\+ 2 \\= 4$");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionParentheses)
{
    const char* code = "return world.MakeRegularExpression('(test)')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "^\\(test\\)$");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionDot)
{
    const char* code = "return world.MakeRegularExpression('file.txt')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "^file\\.txt$");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionBrackets)
{
    const char* code = "return world.MakeRegularExpression('array[5]')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "^array\\[5\\]$");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, MakeRegularExpressionNewline)
{
    // Test that actual newline characters in the input get escaped as \n
    const char* code = "return world.MakeRegularExpression('Line1\\nLine2')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // The result should have \n (two chars: backslash and n) for the newline
    EXPECT_STREQ(lua_tostring(L, -1), "^Line1\\nLine2$");
    lua_pop(L, 1);
}

// ========== Integration Tests ==========

TEST_F(UtilityStringFunctionsTest, IntegrationStripAndFixup)
{
    const char* code = R"(
        local colored = '\27[32mHello\27[0m'
        local stripped = world.StripANSI(colored)
        return world.FixupHTML(stripped)
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Hello");
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, IntegrationEscapeAndRegex)
{
    const char* code = R"(
        local escaped = world.FixupEscapeSequences('Hello\nWorld')
        -- Check that newline is there
        return escaped:find('\n') ~= nil
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilityStringFunctionsTest, ErrorHandling)
{
    // Test calling with wrong number of arguments
    const char* code = R"(
        local ok, err = pcall(function()
            world.StripANSI()  -- Missing argument
        end)
        return not ok  -- Should fail
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "Should error with missing argument";
    lua_pop(L, 1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
