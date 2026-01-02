/**
 * test_trigger_execution_gtest.cpp - GoogleTest version of Trigger Execution Tests
 *
 * Tests trigger execution functionality including:
 * 1. Wildcard replacement (%0, %1, %2)
 * 2. Send to world
 * 3. Send to output (notes)
 * 4. Color changing
 * 5. One-shot triggers
 * 6. Script execution (Lua callbacks with wildcards)
 */

#include "test_qt_static.h"
#include "../src/automation/trigger.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <cstring>
#include <gtest/gtest.h>

// Lua headers for script execution tests
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for trigger execution tests
class TriggerExecutionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to create a test line with text
    Line* createTestLine(const char* text, qint32 lineNum = 1)
    {
        Line* line = new Line(lineNum,             // line number
                              80,                  // wrap column
                              0,                   // line flags (normal MUD output)
                              qRgb(255, 255, 255), // foreground (white)
                              qRgb(0, 0, 0),       // background (black)
                              true                 // UTF-8 mode
        );

        // Set the text
        int len = strlen(text);
        line->textBuffer.resize(len);
        memcpy(line->text(), text, len);
        line->textBuffer.push_back('\0');
        // memcpy already done above

        // Add a default style covering the entire line
        auto style = std::make_unique<Style>();
        style->iLength = line->len();
        style->iFlags = 0;
        style->iForeColour = qRgb(255, 255, 255);
        style->iBackColour = qRgb(0, 0, 0);
        style->pAction = nullptr;
        line->styleList.push_back(std::move(style));

        return line;
    }

    // Helper to add and enable a trigger
    Trigger* addTrigger(const QString& label, const QString& pattern)
    {
        Trigger* trigger = new Trigger();
        trigger->trigger = pattern;
        trigger->bEnabled = true;
        trigger->strLabel = label;
        trigger->strInternalName = label;

        doc->addTrigger(label, std::unique_ptr<Trigger>(trigger));
        doc->rebuildTriggerArray();

        return trigger;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Wildcard replacement in trigger contents
TEST_F(TriggerExecutionTest, WildcardReplacementInContents)
{
    // Create a trigger that sends text with wildcards
    Trigger* t = addTrigger("gold_notify", "You have * gold");
    t->contents = "Gold amount: %1 pieces"; // Will replace %1 with captured wildcard
    t->iSendTo = 2;                         // eSendToOutput (note)
    t->iSequence = 100;

    // Create a line that matches
    Line* line = createTestLine("You have 500 gold");

    // Evaluate triggers (should execute and replace %1 with "500")
    doc->evaluateTriggers(line);

    // Verify trigger executed
    EXPECT_EQ(t->nMatched, 1) << "Trigger should have executed once";

    // Verify wildcards were captured
    ASSERT_GT(t->wildcards.size(), 1) << "Should have captured wildcards";
    EXPECT_EQ(t->wildcards[1], "500") << "Wildcard %1 should be '500'";

    // Cleanup
    delete line;
}

// Test 2: Color changing
TEST_F(TriggerExecutionTest, ColorChanging)
{
    // Create a trigger that changes line color
    Trigger* t = addTrigger("warning_color", "Warning: *");
    t->iOtherForeground = qRgb(255, 0, 0); // Red foreground
    t->iColourChangeType = 1;              // TRIGGER_COLOUR_CHANGE_FOREGROUND
    t->iSequence = 200;

    Line* line = createTestLine("Warning: Low health");

    // Evaluate trigger
    doc->evaluateTriggers(line);

    // Verify color was changed
    ASSERT_GT(line->styleList.size(), 0) << "Line should have style";
    Style* style = line->styleList[0].get();
    EXPECT_EQ(style->iForeColour, qRgb(255, 0, 0)) << "Line color should be red";

    // Cleanup
    delete line;
}

// Test 3: One-shot trigger (deletes after first match)
TEST_F(TriggerExecutionTest, OneShotTrigger)
{
    // Create a one-shot trigger
    Trigger* t = addTrigger("level_up", "You level up!");
    t->bOneShot = true; // Delete after first match
    t->iSequence = 300;

    // Verify trigger exists
    ASSERT_NE(doc->getTrigger("level_up"), nullptr) << "One-shot trigger should be created";

    Line* line = createTestLine("You level up!");

    // Evaluate triggers (should execute and delete)
    doc->evaluateTriggers(line);

    // Verify trigger was deleted
    EXPECT_EQ(doc->getTrigger("level_up"), nullptr)
        << "One-shot trigger should be deleted after firing";

    // Cleanup
    delete line;
}

// Test 4: Multiple wildcards in contents
TEST_F(TriggerExecutionTest, MultipleWildcardsInContents)
{
    // Create a trigger that captures multiple wildcards
    Trigger* t = addTrigger("tell_format", "* tells you: *");
    t->contents = "Message from %1: %2";
    t->iSendTo = 2; // eSendToOutput
    t->iSequence = 400;

    Line* line = createTestLine("Alice tells you: Hello!");

    doc->evaluateTriggers(line);

    // Verify multiple wildcards were captured
    ASSERT_GT(t->wildcards.size(), 2) << "Should have captured 2 wildcards";
    EXPECT_EQ(t->wildcards[1], "Alice") << "First wildcard should be 'Alice'";
    EXPECT_EQ(t->wildcards[2], "Hello!") << "Second wildcard should be 'Hello!'";

    // Cleanup
    delete line;
}

// Test 5: Script execution (Lua callbacks with wildcards)
TEST_F(TriggerExecutionTest, ScriptExecution)
{
    // Define a Lua function that will be called by the trigger
    QString luaScript = R"(
-- Global variables to track callback
trigger_was_called = false
trigger_name_received = ""
trigger_line_received = ""
trigger_wildcard_count = 0
trigger_wildcard_1 = ""

-- Function that trigger will call
function on_health_trigger(name, line)
    trigger_was_called = true
    trigger_name_received = name
    trigger_line_received = line

    -- Count wildcards (0-indexed: wildcards[0] = full match)
    trigger_wildcard_count = 0
    for i = 0, 10 do
        if wildcards[i] ~= nil then
            trigger_wildcard_count = trigger_wildcard_count + 1
            if i == 1 then
                trigger_wildcard_1 = wildcards[1]
            end
        end
    end

    -- Send a note to verify script executed
    world.Note("Script called! HP: " .. wildcards[1])
end
)";

    // Load the Lua script
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    // Create a trigger that calls the Lua function
    Trigger* t = addTrigger("health_trigger", "Your health is *%");
    t->strProcedure = "on_health_trigger"; // Lua function to call
    t->iSendTo = 12;                       // eSendToScript
    t->iSequence = 500;

    Line* line = createTestLine("Your health is 75%");

    // Evaluate triggers (should call Lua function)
    doc->evaluateTriggers(line);

    // Verify the Lua function was called by checking global variables
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";
    ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should be available";
    lua_State* L = doc->m_ScriptEngine->L;

    // Check trigger_was_called
    lua_getglobal(L, "trigger_was_called");
    bool wasCalled = lua_toboolean(L, -1);
    lua_pop(L, 1);
    EXPECT_TRUE(wasCalled) << "Lua function should have been called";

    // Check trigger_name_received
    lua_getglobal(L, "trigger_name_received");
    QString nameReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(nameReceived, "health_trigger") << "Trigger name should be passed correctly";

    // Check trigger_line_received
    lua_getglobal(L, "trigger_line_received");
    QString lineReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(lineReceived, "Your health is 75%") << "Matched line should be passed correctly";

    // Check wildcard count (should be 2: wildcards[0] = full match, wildcards[1] = "75")
    lua_getglobal(L, "trigger_wildcard_count");
    int wildcardCount = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(wildcardCount, 2) << "Should have 2 wildcards";

    // Check wildcard[1] value
    lua_getglobal(L, "trigger_wildcard_1");
    QString wildcard1 = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(wildcard1, "75") << "wildcards[1] should be '75'";

    // Verify invocation count incremented
    EXPECT_EQ(t->nInvocationCount, 1) << "Invocation count should be incremented";

    // Cleanup
    delete line;
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects like WorldDocument)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
