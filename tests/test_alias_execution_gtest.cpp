/**
 * test_alias_execution_gtest.cpp - GoogleTest version of Alias Execution Tests
 *
 * Tests alias execution functionality including:
 * 1. Wildcard pattern matching (e.g., "n*" matches "north")
 * 2. Wildcard capture (wildcards[0] = full match, wildcards[1+] = captures)
 * 3. Case sensitivity
 * 4. Send to world
 * 5. Send to execute (command chaining)
 * 6. One-shot aliases
 * 7. Keep evaluating flag
 * 8. Script execution (Lua callbacks with wildcards)
 */

#include "test_qt_static.h"
#include "../src/automation/alias.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Lua headers for script execution tests
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for alias execution tests
class AliasExecutionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to add and enable an alias
    Alias* addAlias(const QString& label, const QString& pattern)
    {
        auto alias = std::make_unique<Alias>();
        alias->name = pattern;
        alias->bEnabled = true;
        alias->strLabel = label;
        alias->strInternalName = label;

        Alias* aliasPtr = alias.get();
        doc->addAlias(label, std::move(alias));

        return aliasPtr;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Wildcard Pattern Matching - "n*" matches "north"
TEST_F(AliasExecutionTest, WildcardPatternMatching)
{
    Alias* a = addAlias("north_alias", "n*");
    a->contents = "walk north";
    a->bRegexp = false; // Wildcard mode
    a->bIgnoreCase = false;
    a->iSequence = 100;

    // Test matching "north"
    EXPECT_TRUE(a->match("north")) << "Pattern 'n*' should match 'north'";

    // Verify wildcards captured
    ASSERT_GT(a->wildcards.size(), 1) << "Should have captured wildcards";
    EXPECT_EQ(a->wildcards[1], "orth") << "wildcards[1] should be 'orth'";

    // Test matching "northeast"
    EXPECT_TRUE(a->match("northeast")) << "Pattern 'n*' should match 'northeast'";
    EXPECT_EQ(a->wildcards[1], "ortheast") << "wildcards[1] should be 'ortheast'";

    // Test non-matching
    EXPECT_FALSE(a->match("south")) << "Pattern 'n*' should not match 'south'";
}

// Test 2: Case Sensitivity - case-sensitive matching
TEST_F(AliasExecutionTest, CaseSensitiveMatching)
{
    Alias* a = addAlias("go_sensitive", "go*");
    a->contents = "walk %1";
    a->bRegexp = false;
    a->bIgnoreCase = false; // Case-sensitive
    a->iSequence = 200;

    // Should match "gonorth"
    EXPECT_TRUE(a->match("gonorth")) << "'go*' should match 'gonorth' (case-sensitive)";

    // Should NOT match "GONORTH" (different case)
    EXPECT_FALSE(a->match("GONORTH")) << "'go*' should not match 'GONORTH' (case-sensitive)";
}

// Test 3: Case Insensitivity - case-insensitive matching
TEST_F(AliasExecutionTest, CaseInsensitiveMatching)
{
    Alias* a = addAlias("go_insensitive", "GO*");
    a->contents = "walk %1";
    a->bRegexp = false;
    a->bIgnoreCase = true; // Case-insensitive
    a->iSequence = 300;

    // Should match "gonorth" (case-insensitive)
    EXPECT_TRUE(a->match("gonorth")) << "'GO*' should match 'gonorth' (case-insensitive)";

    // Should match "GONORTH" (case-insensitive)
    EXPECT_TRUE(a->match("GONORTH")) << "'GO*' should match 'GONORTH' (case-insensitive)";
}

// Test 4: Multiple Wildcards
TEST_F(AliasExecutionTest, MultipleWildcards)
{
    Alias* a = addAlias("tell_alias", "tell * *");
    a->contents = "say Telling %1: %2";
    a->bRegexp = false;
    a->bIgnoreCase = false;
    a->iSequence = 400;

    EXPECT_TRUE(a->match("tell Bob hello there"))
        << "Pattern 'tell * *' should match 'tell Bob hello there'";

    // Verify wildcards: wildcards[0] = full match, wildcards[1] = "Bob", wildcards[2] = "hello
    // there"
    ASSERT_GT(a->wildcards.size(), 2) << "Should have captured multiple wildcards";
    EXPECT_EQ(a->wildcards[1], "Bob") << "wildcards[1] should be 'Bob'";
    EXPECT_EQ(a->wildcards[2], "hello there") << "wildcards[2] should be 'hello there'";
}

// Test 5: Alias Execution - Send to World
TEST_F(AliasExecutionTest, SendToWorld)
{
    Alias* a = addAlias("n_alias", "n");
    a->contents = "north";
    a->iSendTo = 0; // eSendToWorld
    a->iSequence = 100;

    // Evaluate alias
    bool executed = doc->evaluateAliases("n");

    EXPECT_TRUE(executed) << "Alias should have executed";

    // Verify statistics updated
    EXPECT_EQ(a->nMatched, 1) << "Match count should be 1";
}

// Test 6: One-Shot Alias
TEST_F(AliasExecutionTest, OneShotAlias)
{
    Alias* a = addAlias("quickheal_alias", "quickheal");
    a->contents = "cast heal self";
    a->iSendTo = 0;     // eSendToWorld
    a->bOneShot = true; // Delete after first use

    // Verify alias exists
    ASSERT_NE(doc->getAlias("quickheal_alias"), nullptr) << "One-shot alias should be created";

    // Execute alias (should delete after execution)
    doc->evaluateAliases("quickheal");

    // Verify alias was deleted
    EXPECT_EQ(doc->getAlias("quickheal_alias"), nullptr)
        << "One-shot alias should be deleted after execution";
}

// Test 7: Keep Evaluating Flag - both aliases execute
TEST_F(AliasExecutionTest, KeepEvaluatingTrue)
{
    // First alias: matches "go north" but keeps evaluating
    Alias* a1 = addAlias("go_walk", "go *");
    a1->contents = "walk %1";
    a1->iSendTo = 0;
    a1->bKeepEvaluating = true; // Keep evaluating other aliases
    a1->iSequence = 100;

    // Second alias: also matches "go north", lower priority (higher sequence)
    Alias* a2 = addAlias("go_north_shortcut", "go north");
    a2->contents = "north";
    a2->iSendTo = 0;
    a2->bKeepEvaluating = false; // Stop after this one
    a2->iSequence = 200;

    // Evaluate aliases - both should execute due to bKeepEvaluating
    doc->evaluateAliases("go north");

    EXPECT_EQ(a1->nMatched, 1) << "First alias should execute";
    EXPECT_EQ(a2->nMatched, 1) << "Second alias should also execute (bKeepEvaluating = true)";
}

// Test 8: Keep Evaluating Flag - only first alias executes
TEST_F(AliasExecutionTest, KeepEvaluatingFalse)
{
    Alias* a1 = addAlias("test_first", "test *");
    a1->contents = "first %1";
    a1->iSendTo = 0;
    a1->bKeepEvaluating = false; // Stop after this one
    a1->iSequence = 100;

    Alias* a2 = addAlias("test_second", "test command");
    a2->contents = "second";
    a2->iSendTo = 0;
    a2->bKeepEvaluating = false;
    a2->iSequence = 200;

    doc->evaluateAliases("test command");

    EXPECT_EQ(a1->nMatched, 1) << "First alias should execute";
    EXPECT_EQ(a2->nMatched, 0) << "Second alias should not execute (bKeepEvaluating = false)";
}

// Test 9: Regular Expression Alias
TEST_F(AliasExecutionTest, RegularExpressionAlias)
{
    Alias* a = addAlias("north_regex", "^(n|north)$");
    a->contents = "walk north";
    a->bRegexp = true; // Regular expression mode
    a->bIgnoreCase = false;
    a->iSequence = 100;

    // Should match "n"
    EXPECT_TRUE(a->match("n")) << "Regex '^(n|north)$' should match 'n'";

    // Should match "north"
    EXPECT_TRUE(a->match("north")) << "Regex '^(n|north)$' should match 'north'";

    // Should NOT match "northeast"
    EXPECT_FALSE(a->match("northeast")) << "Regex '^(n|north)$' should not match 'northeast'";
}

// Test 10: Script Execution (Lua Callbacks)
TEST_F(AliasExecutionTest, ScriptExecution)
{
    // Define a Lua function that will be called by the alias
    QString luaScript = R"(
-- Global variables to track if function was called
alias_was_called = false
alias_name_received = ""
alias_command_received = ""
alias_wildcard_count = 0
alias_wildcard_1 = ""

-- Function that alias will call
function on_speedwalk_alias(name, command)
    alias_was_called = true
    alias_name_received = name
    alias_command_received = command

    -- Count wildcards (they're 0-indexed: wildcards[0] = full match)
    alias_wildcard_count = 0
    for i = 0, 10 do
        if wildcards[i] ~= nil then
            alias_wildcard_count = alias_wildcard_count + 1
            if i == 1 then
                alias_wildcard_1 = wildcards[1]
            end
        end
    end

    -- Send a note to verify script executed
    world.Note("Speedwalking: " .. wildcards[1])
end
)";

    // Load the Lua script
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    // Create an alias that calls the Lua function
    Alias* a = addAlias("speedwalk_alias", "run *");
    a->strProcedure = "on_speedwalk_alias"; // Lua function to call
    a->iSendTo = 12;                        // eSendToScript

    // Execute alias (should call Lua function)
    doc->evaluateAliases("run 3n2e");

    // Verify the Lua function was called by checking global variables
    ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should be available";

    lua_State* L = doc->m_ScriptEngine->L;

    // Check alias_was_called
    lua_getglobal(L, "alias_was_called");
    bool wasCalled = lua_toboolean(L, -1);
    lua_pop(L, 1);
    EXPECT_TRUE(wasCalled) << "Lua function should have been called";

    // Check alias_name_received
    lua_getglobal(L, "alias_name_received");
    QString nameReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(nameReceived, "speedwalk_alias") << "Alias name should be passed correctly";

    // Check alias_command_received
    lua_getglobal(L, "alias_command_received");
    QString commandReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(commandReceived, "run 3n2e") << "Matched command should be passed correctly";

    // Check wildcard count (should be 2: wildcards[0] = full match, wildcards[1] = "3n2e")
    lua_getglobal(L, "alias_wildcard_count");
    int wildcardCount = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(wildcardCount, 2) << "Should have 2 wildcards";

    // Check wildcard[1] value
    lua_getglobal(L, "alias_wildcard_1");
    QString wildcard1 = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(wildcard1, "3n2e") << "wildcards[1] should be '3n2e'";

    // Verify invocation count incremented
    EXPECT_EQ(a->nInvocationCount, 1) << "Invocation count should be 1";
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
