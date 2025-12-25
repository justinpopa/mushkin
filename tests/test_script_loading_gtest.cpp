/**
 * test_script_loading_gtest.cpp - GoogleTest version
 * Test script loading and parsing functionality
 *
 * Verifies:
 * 1. Valid scripts load and execute
 * 2. Syntax errors are caught and displayed
 * 3. Runtime errors are caught and displayed
 * 4. Error line context is shown
 * 5. parseLua() works for inline code
 * 6. loadScriptFile() works for file loading
 * 7. Timing statistics are tracked
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QDir>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for script loading tests
class ScriptLoadingTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();

        // Initialize basic state
        doc->m_mush_name = "Test World";
        doc->m_server = "test.mud.com";
        doc->m_port = 4000;
        doc->m_iConnectPhase = eConnectConnectedToMud;
        doc->m_bUTF_8 = true;

        // Initialize note() settings
        doc->m_bNotesInRGB = true;
        doc->m_iNoteColourFore = qRgb(255, 255, 255); // White
        doc->m_iNoteColourBack = qRgb(0, 0, 0);       // Black
        doc->m_iNoteStyle = 0;                        // No special styling

        // Create initial line (needed for note() to work)
        doc->m_currentLine = new Line(1, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_RGB;
        initialStyle->iForeColour = qRgb(192, 192, 192);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));

        // Set current style
        doc->m_iFlags = COLOUR_RGB;
        doc->m_iForeColour = qRgb(192, 192, 192);
        doc->m_iBackColour = qRgb(0, 0, 0);

        // Initialize script timing
        doc->m_iScriptTimeTaken = 0;

        // Ensure ScriptEngine is initialized
        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should be created";
        if (!doc->m_ScriptEngine->L) {
            ASSERT_TRUE(doc->m_ScriptEngine->createScriptEngine())
                << "Should initialize script engine";
        }

        // Get test directory path
        // Test Lua files are in the source tests/ directory
        testDir = QDir::currentPath() + "/tests";
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper method to reset Lua state between tests
    void resetLuaState()
    {
        doc->m_ScriptEngine->closeLua();
        doc->m_ScriptEngine->openLua();
    }

    WorldDocument* doc = nullptr;
    QString testDir;
};

// Test 1: Verify ScriptEngine exists and is ready
TEST_F(ScriptLoadingTest, ScriptEngineExists)
{
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should exist";
    ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should be initialized";
}

// Test 2: parseLua() with valid code
TEST_F(ScriptLoadingTest, ParseLuaValidCode)
{
    QString validCode = R"(
        world.Note("Hello from inline Lua!")
        test_value = 123
    )";

    bool error = doc->m_ScriptEngine->parseLua(validCode, "Inline test");
    EXPECT_FALSE(error) << "Valid code should execute without error";

    // Verify the variable was set
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "test_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 123) << "Global variable should be set correctly";
}

// Test 3: parseLua() with syntax error
TEST_F(ScriptLoadingTest, ParseLuaSyntaxError)
{
    QString syntaxErrorCode = R"(
        function broken()
            world.Note("Missing end")
        -- Missing 'end' here
    )";

    bool error = doc->m_ScriptEngine->parseLua(syntaxErrorCode, "Syntax error test");
    EXPECT_TRUE(error) << "Syntax error should be detected";
}

// Test 4: parseLua() with runtime error
TEST_F(ScriptLoadingTest, ParseLuaRuntimeError)
{
    QString runtimeErrorCode = R"(
        nonexistent_function()
    )";

    bool error = doc->m_ScriptEngine->parseLua(runtimeErrorCode, "Runtime error test");
    EXPECT_TRUE(error) << "Runtime error should be detected";
}

// Test 5: loadScriptFile() with valid script
TEST_F(ScriptLoadingTest, LoadValidScriptFile)
{
    doc->m_strScriptFilename = testDir + "/test_valid.lua";
    doc->loadScriptFile();

    // Check if functions were defined
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "OnWorldConnect");
    bool hasOnWorldConnect = lua_isfunction(L, -1);
    lua_pop(L, 1);

    EXPECT_TRUE(hasOnWorldConnect)
        << "OnWorldConnect function should be defined after loading valid script";
}

// Test 6: loadScriptFile() with syntax error
TEST_F(ScriptLoadingTest, LoadScriptFileWithSyntaxError)
{
    resetLuaState();

    doc->m_strScriptFilename = testDir + "/test_syntax_error.lua";

    // Should handle syntax error gracefully without crashing
    EXPECT_NO_THROW(doc->loadScriptFile()) << "Should handle syntax error gracefully";
}

// Test 7: loadScriptFile() with runtime error
TEST_F(ScriptLoadingTest, LoadScriptFileWithRuntimeError)
{
    resetLuaState();

    doc->m_strScriptFilename = testDir + "/test_runtime_error.lua";

    // Should handle runtime error gracefully without crashing
    EXPECT_NO_THROW(doc->loadScriptFile()) << "Should handle runtime error gracefully";
}

// Test 8: showErrorLines()
TEST_F(ScriptLoadingTest, ShowErrorLines)
{
    doc->m_strScriptFilename = testDir + "/test_syntax_error.lua";

    // Should execute without crashing
    EXPECT_NO_THROW(doc->showErrorLines(13)) << "showErrorLines() should execute without crashing";
}

// Test 9: Timing statistics
TEST_F(ScriptLoadingTest, TimingStatistics)
{
    // Load a script to trigger timing
    doc->m_strScriptFilename = testDir + "/test_valid.lua";
    doc->loadScriptFile();

    EXPECT_GT(doc->m_iScriptTimeTaken, 0) << "Script timing should be recorded";

    double msElapsed = doc->m_iScriptTimeTaken / 1000000.0;
    EXPECT_GE(msElapsed, 0.0) << "Elapsed time should be non-negative";
}

// Test 10: Stack is clean after operations
TEST_F(ScriptLoadingTest, LuaStackIsClean)
{
    // Perform some operations
    QString code = "x = 42";
    doc->m_ScriptEngine->parseLua(code, "Stack test");

    // Check stack is clean
    lua_State* L = doc->m_ScriptEngine->L;
    int stackTop = lua_gettop(L);
    EXPECT_EQ(stackTop, 0) << "Lua stack should be clean after operations";
}

// Test 11: LPeg library is available
TEST_F(ScriptLoadingTest, LpegLibraryAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check lpeg global exists
    lua_getglobal(L, "lpeg");
    EXPECT_TRUE(lua_istable(L, -1)) << "lpeg global should be a table";
    lua_pop(L, 1);

    // Check require("lpeg") works
    QString code = R"(
        local lpeg = require("lpeg")
        lpeg_test_result = lpeg and lpeg.P and lpeg.R and lpeg.match
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "LPeg require test");
    EXPECT_FALSE(error) << "require('lpeg') should succeed";

    // Verify lpeg functions are accessible
    lua_getglobal(L, "lpeg_test_result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "lpeg should have P, R, and match functions";
    lua_pop(L, 1);
}

// Test 12: LPeg pattern matching works
TEST_F(ScriptLoadingTest, LpegPatternMatching)
{
    QString code = R"(
        local P, R, C = lpeg.P, lpeg.R, lpeg.C
        -- Match a word and capture it
        local word = C(R("az", "AZ")^1)
        lpeg_capture_result = lpeg.match(word, "Hello")
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "LPeg pattern test");
    EXPECT_FALSE(error) << "LPeg pattern matching should work";

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "lpeg_capture_result");
    const char* result = lua_tostring(L, -1);
    EXPECT_STREQ(result, "Hello") << "LPeg should capture 'Hello'";
    lua_pop(L, 1);
}

// Test 13: re module is available
TEST_F(ScriptLoadingTest, ReModuleAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check re global exists
    lua_getglobal(L, "re");
    EXPECT_TRUE(lua_istable(L, -1)) << "re global should be a table";
    lua_pop(L, 1);

    // Check require("re") works
    QString code = R"(
        local re = require("re")
        re_test_result = re and re.match and re.find and re.gsub
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "re require test");
    EXPECT_FALSE(error) << "require('re') should succeed";

    // Verify re functions are accessible
    lua_getglobal(L, "re_test_result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "re should have match, find, and gsub functions";
    lua_pop(L, 1);
}

// Test 14: re module pattern matching works
TEST_F(ScriptLoadingTest, RePatternMatching)
{
    // Note: re patterns use lpeg syntax, not Lua patterns
    // - Literal strings must be quoted: "hello" or 'hello'
    // - %a is predefined character class for alpha
    // - {pattern} captures the match
    QString code = R"(
        -- Test re.match - captures one or more alphabetic characters
        re_match_result = re.match("hello world", "{%a+}")
        -- Test re.find - find position of literal "world" (quoted in re syntax)
        re_find_start, re_find_end = re.find("hello world", "'world'")
        -- Test re.gsub - replace words with X
        re_gsub_result = re.gsub("hello world", "{%a+}", "X")
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "re pattern test");
    EXPECT_FALSE(error) << "re pattern matching should work";

    lua_State* L = doc->m_ScriptEngine->L;

    // Check re.match result
    lua_getglobal(L, "re_match_result");
    const char* matchResult = lua_tostring(L, -1);
    EXPECT_STREQ(matchResult, "hello") << "re.match should return 'hello'";
    lua_pop(L, 1);

    // Check re.find result
    lua_getglobal(L, "re_find_start");
    int findStart = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_getglobal(L, "re_find_end");
    int findEnd = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(findStart, 7) << "re.find should find 'world' starting at position 7";
    EXPECT_EQ(findEnd, 11) << "re.find should find 'world' ending at position 11";

    // Check re.gsub result
    lua_getglobal(L, "re_gsub_result");
    const char* gsubResult = lua_tostring(L, -1);
    EXPECT_STREQ(gsubResult, "X X") << "re.gsub should replace words with X";
    lua_pop(L, 1);
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
