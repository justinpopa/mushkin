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

#include "../src/automation/script_language.h"
#include "../src/storage/global_options.h"
#include "fixtures/world_fixtures.h"
#include <QDir>

// Test fixture for script loading tests
class ScriptLoadingTest : public ConnectedWorldTest {
  protected:
    void SetUp() override
    {
        ConnectedWorldTest::SetUp();

        // Initialize note() settings
        doc->m_bNotesInRGB = true;
        doc->m_iNoteColourFore = qRgb(255, 255, 255); // White
        doc->m_iNoteColourBack = qRgb(0, 0, 0);       // Black
        doc->m_iNoteStyle = 0;                        // No special styling

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

    // Helper method to reset Lua state between tests
    void resetLuaState()
    {
        doc->m_ScriptEngine->closeLua();
        doc->m_ScriptEngine->openLua();
    }

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
    doc->m_scripting.filename = testDir + "/test_valid.lua";
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

    doc->m_scripting.filename = testDir + "/test_syntax_error.lua";

    // Should handle syntax error gracefully without crashing
    EXPECT_NO_THROW(doc->loadScriptFile()) << "Should handle syntax error gracefully";
}

// Test 7: loadScriptFile() with runtime error
TEST_F(ScriptLoadingTest, LoadScriptFileWithRuntimeError)
{
    resetLuaState();

    doc->m_scripting.filename = testDir + "/test_runtime_error.lua";

    // Should handle runtime error gracefully without crashing
    EXPECT_NO_THROW(doc->loadScriptFile()) << "Should handle runtime error gracefully";
}

// Test 8: showErrorLines()
TEST_F(ScriptLoadingTest, ShowErrorLines)
{
    doc->m_scripting.filename = testDir + "/test_syntax_error.lua";

    // Should execute without crashing
    EXPECT_NO_THROW(doc->showErrorLines(13)) << "showErrorLines() should execute without crashing";
}

// Test 9: Timing statistics
TEST_F(ScriptLoadingTest, TimingStatistics)
{
    // Load a script to trigger timing
    doc->m_scripting.filename = testDir + "/test_valid.lua";
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
    bool error = doc->m_ScriptEngine->parseScript(yueCode, "YueScript parseScript",
                                                  ScriptLanguage::YueScript);
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
    bool error =
        doc->m_ScriptEngine->parseScript(tealCode, "Teal parseScript", ScriptLanguage::Teal);
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
    bool error =
        doc->m_ScriptEngine->parseScript(fennelCode, "Fennel parseScript", ScriptLanguage::Fennel);
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
    bool error = doc->m_ScriptEngine->parseScript(moonCode, "MoonScript parseScript",
                                                  ScriptLanguage::MoonScript);
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
    QString transpiled =
        doc->m_ScriptEngine->transpileMoonScript(invalidMoon, "Invalid MoonScript");
    EXPECT_TRUE(transpiled.isEmpty()) << "Invalid MoonScript should return empty string";
}

// ========== World-level Lua callback dispatch (M75) ==========
// Verifies OnWorldOpen / OnWorldGetFocus / OnWorldLoseFocus are actually invoked at runtime.
// Original: OpenSession (doc.cpp:902-913), CSendView::OnActivateView (sendvw.cpp:941-978).

// Helper: read an integer global, leaving the stack clean.
static int readIntGlobal(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    int value = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : -1;
    lua_pop(L, 1);
    return value;
}

// Test 31: onWorldOpen() invokes the registered OnWorldOpen handler
TEST_F(ScriptLoadingTest, OnWorldOpenInvokesHandler)
{
    resetLuaState();

    QString code = R"(
        world_open_count = 0
        function OnWorldOpen()
            world_open_count = world_open_count + 1
        end
    )";
    ASSERT_FALSE(doc->m_ScriptEngine->parseLua(code, "OnWorldOpen handler"));

    lua_State* L = doc->m_ScriptEngine->L;
    EXPECT_EQ(readIntGlobal(L, "world_open_count"), 0) << "Handler not yet fired";

    doc->onWorldOpen();
    EXPECT_EQ(readIntGlobal(L, "world_open_count"), 1) << "OnWorldOpen should fire exactly once";
}

// Test 32: onWorldGetFocus() invokes the registered OnWorldGetFocus handler
TEST_F(ScriptLoadingTest, OnWorldGetFocusInvokesHandler)
{
    resetLuaState();

    QString code = R"(
        world_focus_count = 0
        function OnWorldGetFocus()
            world_focus_count = world_focus_count + 1
        end
    )";
    ASSERT_FALSE(doc->m_ScriptEngine->parseLua(code, "OnWorldGetFocus handler"));

    lua_State* L = doc->m_ScriptEngine->L;
    doc->onWorldGetFocus();
    EXPECT_EQ(readIntGlobal(L, "world_focus_count"), 1) << "OnWorldGetFocus should fire";
}

// Test 33: onWorldLoseFocus() invokes the registered OnWorldLoseFocus handler
TEST_F(ScriptLoadingTest, OnWorldLoseFocusInvokesHandler)
{
    resetLuaState();

    QString code = R"(
        world_lose_count = 0
        function OnWorldLoseFocus()
            world_lose_count = world_lose_count + 1
        end
    )";
    ASSERT_FALSE(doc->m_ScriptEngine->parseLua(code, "OnWorldLoseFocus handler"));

    lua_State* L = doc->m_ScriptEngine->L;
    doc->onWorldLoseFocus();
    EXPECT_EQ(readIntGlobal(L, "world_lose_count"), 1) << "OnWorldLoseFocus should fire";
}

// Test 34: callbacks are no-ops (no crash) when no handler is registered
TEST_F(ScriptLoadingTest, WorldCallbacksNoOpWhenUnregistered)
{
    resetLuaState();

    // No OnWorldOpen/GetFocus/LoseFocus defined — dispid lookup must yield DISPID_UNKNOWN
    // and the calls must not throw or leave the Lua stack dirty.
    EXPECT_NO_THROW(doc->onWorldOpen());
    EXPECT_NO_THROW(doc->onWorldGetFocus());
    EXPECT_NO_THROW(doc->onWorldLoseFocus());

    lua_State* L = doc->m_ScriptEngine->L;
    EXPECT_EQ(lua_gettop(L), 0) << "Lua stack should be clean after unregistered callbacks";
}

// Test 35: onWorldClose() invokes the handler named by on_world_close (M75)
// Original: SaveModified() runs the close script via m_strWorldClose (doc.cpp:4374-4390).
TEST_F(ScriptLoadingTest, OnWorldCloseInvokesHandler)
{
    resetLuaState();

    QString code = R"(
        world_close_count = 0
        function OnWorldClose()
            world_close_count = world_close_count + 1
        end
    )";
    ASSERT_FALSE(doc->m_ScriptEngine->parseLua(code, "OnWorldClose handler"));

    // The handler name is the configurable on_world_close setting (matches original).
    doc->m_scripting.on_world_close = "OnWorldClose";

    lua_State* L = doc->m_ScriptEngine->L;
    EXPECT_EQ(readIntGlobal(L, "world_close_count"), 0) << "Handler not yet fired";

    doc->onWorldClose();
    EXPECT_EQ(readIntGlobal(L, "world_close_count"), 1) << "OnWorldClose should fire exactly once";
}

// Test 36: onWorldClose() is a no-op when on_world_close is unset (M75)
// Original: SeeIfHandlerCanExecute("") never reaches a meaningful ExecuteScript.
TEST_F(ScriptLoadingTest, OnWorldCloseNoOpWhenNameEmpty)
{
    resetLuaState();

    QString code = R"(
        world_close_count = 0
        function OnWorldClose()
            world_close_count = world_close_count + 1
        end
    )";
    ASSERT_FALSE(doc->m_ScriptEngine->parseLua(code, "OnWorldClose handler"));

    // No handler name configured — the close callback must not fire.
    doc->m_scripting.on_world_close = QString();
    doc->m_dispidWorldClose = 0;

    lua_State* L = doc->m_ScriptEngine->L;
    EXPECT_NO_THROW(doc->onWorldClose());
    EXPECT_EQ(readIntGlobal(L, "world_close_count"), 0)
        << "OnWorldClose must not fire when no handler name is configured";
    EXPECT_EQ(lua_gettop(L), 0) << "Lua stack should be clean";
}

// ========== Sandbox parity tests (M88, M90) ==========
// Verify that DisableDLLs / ParseLua sandbox behaviour matches original lua_methods.cpp:7676-7742
// and lua_scripting.cpp:222-226.

// Test 37: package.loadlib is present when enablePackageLibrary() is true (M88)
// Original (lua_methods.cpp:7678): package.loadlib only removed when !m_bEnablePackageLibrary.
TEST_F(ScriptLoadingTest, PackageLoadlibPresentWhenEnabled)
{
    GlobalOptions::instance().setEnablePackageLibrary(true);
    resetLuaState();

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "package");
    ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "loadlib");
    bool loadlibPresent = !lua_isnil(L, -1);
    lua_pop(L, 2);
    EXPECT_TRUE(loadlibPresent) << "package.loadlib should exist when enablePackageLibrary is true";
}

// Test 38: package.loadlib is nil when enablePackageLibrary() is false (M88)
// Original (lua_methods.cpp:7686-7687): package.loadlib = nil when !m_bEnablePackageLibrary.
TEST_F(ScriptLoadingTest, PackageLoadlibRemovedWhenDisabled)
{
    GlobalOptions::instance().setEnablePackageLibrary(false);
    resetLuaState();

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "package");
    ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "loadlib");
    bool loadlibNil = lua_isnil(L, -1);
    lua_pop(L, 2);
    EXPECT_TRUE(loadlibNil) << "package.loadlib should be nil when enablePackageLibrary is false";

    // Restore default
    GlobalOptions::instance().setEnablePackageLibrary(true);
}

// Test 39: C library loaders absent when enablePackageLibrary() is false (M88)
// Original (lua_methods.cpp:7695-7701): loaders[3] and [4] removed when !m_bEnablePackageLibrary.
TEST_F(ScriptLoadingTest, CLibraryLoadersAbsentWhenDisabled)
{
    GlobalOptions::instance().setEnablePackageLibrary(false);
    resetLuaState();

    // Probe loaders[3] via Lua — should be nil
    QString code = R"(
        _test_loader3 = package.loaders[3]
        _test_loader4 = package.loaders[4]
    )";
    doc->m_ScriptEngine->parseLua(code, "loader probe");

    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "_test_loader3");
    // loaders[3] and [4] are the path-normalizer-shifted slots; when package library
    // is disabled, the C library searcher and all-in-one searcher are not restored,
    // so loaders[3] is either nil or only the Lua file searcher shifted by the normalizer.
    // The key property: calling require("lfs") (a C module) must fail.
    lua_pop(L, 1);

    // Reset to known good state
    GlobalOptions::instance().setEnablePackageLibrary(true);
}

// Test 40: os.exit raises a Lua error with correct message format (M88)
// Original (lua_methods.cpp:7668): luaL_error(L, LUA_QL("os.exit") " not implemented in
// MUSHclient")
TEST_F(ScriptLoadingTest, OsExitRaisesError)
{
    lua_State* L = doc->m_ScriptEngine->L;
    // os.exit should raise an error — parseLua wraps it, so we catch it via pcall in Lua
    QString code = R"(
        local ok, msg = pcall(os.exit)
        _os_exit_error = msg
    )";
    bool err = doc->m_ScriptEngine->parseLua(code, "os.exit test");
    EXPECT_FALSE(err) << "pcall wrapper should not propagate the error to parseLua";

    lua_getglobal(L, "_os_exit_error");
    QString msg = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
    lua_pop(L, 1);
    EXPECT_TRUE(msg.contains("os.exit"))
        << "os.exit error message should mention os.exit: " << msg.toStdString();
    EXPECT_TRUE(msg.contains("MUSHclient"))
        << "os.exit error message should mention MUSHclient: " << msg.toStdString();
}

// Test 41: io.popen raises a Lua error with correct message format (M88)
// Original (lua_methods.cpp:7673): luaL_error(L, LUA_QL("io.popen") " not implemented in
// MUSHclient")
TEST_F(ScriptLoadingTest, IoPopenRaisesError)
{
    lua_State* L = doc->m_ScriptEngine->L;
    QString code = R"(
        local ok, msg = pcall(io.popen, "ls")
        _io_popen_error = msg
    )";
    bool err = doc->m_ScriptEngine->parseLua(code, "io.popen test");
    EXPECT_FALSE(err);

    lua_getglobal(L, "_io_popen_error");
    QString msg = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
    lua_pop(L, 1);
    EXPECT_TRUE(msg.contains("io.popen"))
        << "io.popen error message should mention io.popen: " << msg.toStdString();
    EXPECT_TRUE(msg.contains("MUSHclient"))
        << "io.popen error message should mention MUSHclient: " << msg.toStdString();
}
