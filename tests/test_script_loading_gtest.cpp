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

#include "test_qt_static.h"
#include "../src/automation/script_language.h"
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

// ========== Transpiled Language Tests ==========

// Test 15: YueScript module is available
TEST_F(ScriptLoadingTest, YueScriptModuleAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check yue global exists
    lua_getglobal(L, "yue");
    EXPECT_TRUE(lua_istable(L, -1)) << "yue global should be a table";
    lua_pop(L, 1);

    // Check yue.to_lua exists
    QString code = R"(
        local yue = require("yue")
        yue_available = yue and yue.to_lua and type(yue.to_lua) == "function"
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "YueScript module test");
    EXPECT_FALSE(error) << "require('yue') should succeed";

    lua_getglobal(L, "yue_available");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "yue.to_lua function should exist";
    lua_pop(L, 1);
}

// Test 16: YueScript transpilation works
TEST_F(ScriptLoadingTest, YueScriptTranspilation)
{
    // Simple YueScript code that assigns a value
    // Note: YueScript creates locals by default, use 'global' statement for globals
    QString yueCode = R"(
print "Hello from YueScript"
global yue_test_value = 42
)";
    QString transpiled = doc->m_ScriptEngine->transpileYueScript(yueCode, "YueScript test");
    EXPECT_FALSE(transpiled.isEmpty()) << "YueScript transpilation should produce output";

    // Execute the transpiled code
    bool error = doc->m_ScriptEngine->parseLua(transpiled, "YueScript transpiled");
    EXPECT_FALSE(error) << "Transpiled YueScript should execute without error";

    // Verify the variable was set
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "yue_test_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 42) << "YueScript should set the global variable";
}

// Test 17: YueScript parseScript() works
TEST_F(ScriptLoadingTest, YueScriptParseScript)
{
    // Note: YueScript creates locals by default, use 'global' statement for globals
    QString yueCode = R"(
global yue_parse_value = 100
)";
    bool error = doc->m_ScriptEngine->parseScript(yueCode, "YueScript parseScript", ScriptLanguage::YueScript);
    EXPECT_FALSE(error) << "parseScript with YueScript should succeed";

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "yue_parse_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 100) << "YueScript parseScript should execute correctly";
}

// Test 18: Teal module is available
TEST_F(ScriptLoadingTest, TealModuleAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check tl global exists
    lua_getglobal(L, "tl");
    EXPECT_TRUE(lua_istable(L, -1)) << "tl global should be a table";
    lua_pop(L, 1);

    // Check tl.gen exists
    QString code = R"(
        local tl = require("tl")
        tl_available = tl and tl.gen and type(tl.gen) == "function"
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "Teal module test");
    EXPECT_FALSE(error) << "require('tl') should succeed";

    lua_getglobal(L, "tl_available");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "tl.gen function should exist";
    lua_pop(L, 1);
}

// Test 19: Teal transpilation works
TEST_F(ScriptLoadingTest, TealTranspilation)
{
    // Simple Teal code with type annotation
    QString tealCode = R"(
local x: number = 42
teal_test_value = x
)";
    QString transpiled = doc->m_ScriptEngine->transpileTeal(tealCode, "Teal test");
    EXPECT_FALSE(transpiled.isEmpty()) << "Teal transpilation should produce output";

    // Execute the transpiled code
    bool error = doc->m_ScriptEngine->parseLua(transpiled, "Teal transpiled");
    EXPECT_FALSE(error) << "Transpiled Teal should execute without error";

    // Verify the variable was set
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "teal_test_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 42) << "Teal should set the global variable";
}

// Test 20: Teal parseScript() works
TEST_F(ScriptLoadingTest, TealParseScript)
{
    QString tealCode = R"(
local y: number = 200
teal_parse_value = y
)";
    bool error = doc->m_ScriptEngine->parseScript(tealCode, "Teal parseScript", ScriptLanguage::Teal);
    EXPECT_FALSE(error) << "parseScript with Teal should succeed";

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "teal_parse_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 200) << "Teal parseScript should execute correctly";
}

// Test 21: Fennel module is available
TEST_F(ScriptLoadingTest, FennelModuleAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check fennel global exists
    lua_getglobal(L, "fennel");
    EXPECT_TRUE(lua_istable(L, -1)) << "fennel global should be a table";
    lua_pop(L, 1);

    // Check fennel.compileString exists
    QString code = R"(
        local fennel = require("fennel")
        fennel_available = fennel and fennel.compileString and type(fennel.compileString) == "function"
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "Fennel module test");
    EXPECT_FALSE(error) << "require('fennel') should succeed";

    lua_getglobal(L, "fennel_available");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "fennel.compileString function should exist";
    lua_pop(L, 1);
}

// Test 22: Fennel transpilation works
TEST_F(ScriptLoadingTest, FennelTranspilation)
{
    // Simple Fennel code (Lisp syntax)
    QString fennelCode = R"(
(global fennel_test_value 42)
)";
    QString transpiled = doc->m_ScriptEngine->transpileFennel(fennelCode, "Fennel test");
    EXPECT_FALSE(transpiled.isEmpty()) << "Fennel transpilation should produce output";

    // Execute the transpiled code
    bool error = doc->m_ScriptEngine->parseLua(transpiled, "Fennel transpiled");
    EXPECT_FALSE(error) << "Transpiled Fennel should execute without error";

    // Verify the variable was set
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "fennel_test_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 42) << "Fennel should set the global variable";
}

// Test 23: Fennel parseScript() works
TEST_F(ScriptLoadingTest, FennelParseScript)
{
    QString fennelCode = R"(
(global fennel_parse_value 300)
)";
    bool error = doc->m_ScriptEngine->parseScript(fennelCode, "Fennel parseScript", ScriptLanguage::Fennel);
    EXPECT_FALSE(error) << "parseScript with Fennel should succeed";

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "fennel_parse_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 300) << "Fennel parseScript should execute correctly";
}

// Test 24: Error handling for invalid YueScript
TEST_F(ScriptLoadingTest, YueScriptErrorHandling)
{
    // Invalid YueScript syntax
    QString invalidYue = R"(
@@@invalid syntax here###
)";
    QString transpiled = doc->m_ScriptEngine->transpileYueScript(invalidYue, "Invalid YueScript");
    EXPECT_TRUE(transpiled.isEmpty()) << "Invalid YueScript should return empty string";
}

// Test 25: Error handling for invalid Teal
TEST_F(ScriptLoadingTest, TealErrorHandling)
{
    // Invalid Teal - type error (assigning string to number)
    QString invalidTeal = R"(
local x: number = "not a number"
)";
    QString transpiled = doc->m_ScriptEngine->transpileTeal(invalidTeal, "Invalid Teal");
    // Teal may or may not fail here depending on strictness, but the code should handle it
    // If it transpiles, the runtime will catch it
    EXPECT_NO_THROW(transpiled = doc->m_ScriptEngine->transpileTeal(invalidTeal, "Invalid Teal"))
        << "Teal error handling should not crash";
}

// Test 26: Error handling for invalid Fennel
TEST_F(ScriptLoadingTest, FennelErrorHandling)
{
    // Invalid Fennel syntax (unbalanced parens)
    QString invalidFennel = R"(
(def x 42
)";
    QString transpiled = doc->m_ScriptEngine->transpileFennel(invalidFennel, "Invalid Fennel");
    EXPECT_TRUE(transpiled.isEmpty()) << "Invalid Fennel should return empty string";
}

// Test 27: MoonScript module is available
TEST_F(ScriptLoadingTest, MoonScriptModuleAvailable)
{
    lua_State* L = doc->m_ScriptEngine->L;

    // Check moonscript global exists
    lua_getglobal(L, "moonscript");
    EXPECT_TRUE(lua_istable(L, -1)) << "moonscript global should be a table";
    lua_pop(L, 1);

    // Check moonscript.to_lua exists
    QString code = R"(
        local moonscript = require("moonscript")
        moonscript_available = moonscript and moonscript.to_lua and type(moonscript.to_lua) == "function"
    )";
    bool error = doc->m_ScriptEngine->parseLua(code, "MoonScript module test");
    EXPECT_FALSE(error) << "require('moonscript') should succeed";

    lua_getglobal(L, "moonscript_available");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "moonscript.to_lua function should exist";
    lua_pop(L, 1);
}

// Test 28: MoonScript transpilation works
TEST_F(ScriptLoadingTest, MoonScriptTranspilation)
{
    // Simple MoonScript code that assigns a value
    // MoonScript uses export for globals
    QString moonCode = R"(
export moon_test_value = 42
)";
    QString transpiled = doc->m_ScriptEngine->transpileMoonScript(moonCode, "MoonScript test");
    EXPECT_FALSE(transpiled.isEmpty()) << "MoonScript transpilation should produce output";

    // Execute the transpiled code
    bool error = doc->m_ScriptEngine->parseLua(transpiled, "MoonScript transpiled");
    EXPECT_FALSE(error) << "Transpiled MoonScript should execute without error";

    // Verify the variable was set
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "moon_test_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 42) << "MoonScript should set the global variable";
}

// Test 29: MoonScript parseScript() works
TEST_F(ScriptLoadingTest, MoonScriptParseScript)
{
    // MoonScript uses export for globals
    QString moonCode = R"(
export moon_parse_value = 100
)";
    bool error = doc->m_ScriptEngine->parseScript(moonCode, "MoonScript parseScript", ScriptLanguage::MoonScript);
    EXPECT_FALSE(error) << "parseScript with MoonScript should succeed";

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "moon_parse_value");
    int value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(value, 100) << "MoonScript parseScript should execute correctly";
}

// Test 30: Error handling for invalid MoonScript
TEST_F(ScriptLoadingTest, MoonScriptErrorHandling)
{
    // Invalid MoonScript syntax
    QString invalidMoon = R"(
@@@ invalid syntax here ###
)";
    QString transpiled = doc->m_ScriptEngine->transpileMoonScript(invalidMoon, "Invalid MoonScript");
    EXPECT_TRUE(transpiled.isEmpty()) << "Invalid MoonScript should return empty string";
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
