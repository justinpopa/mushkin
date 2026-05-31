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

#include "../src/automation/trigger.h"
#include "fixtures/world_fixtures.h"
#include <cstring>

// Test fixture for trigger execution tests
class TriggerExecutionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    void TearDown() override
    {
    }

    // Helper to create a test line with text
    std::unique_ptr<Line> createTestLine(const char* text, qint32 lineNum = 1)
    {
        auto line = std::make_unique<Line>(lineNum,             // line number
                                           80,                  // wrap column
                                           0,                   // line flags (normal MUD output)
                                           qRgb(255, 255, 255), // foreground (white)
                                           qRgb(0, 0, 0),       // background (black)
                                           true                 // UTF-8 mode
        );

        // Set the text
        int len = strlen(text);
        line->textBuffer.resize(len);
        memcpy(line->textBuffer.data(), text, len);
        line->textBuffer.push_back('\0');

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
        auto trigger = std::make_unique<Trigger>();
        trigger->trigger = pattern;
        trigger->enabled = true;
        trigger->label = label;
        trigger->internal_name = label;
        Trigger* raw = trigger.get();

        auto addResult = doc->addTrigger(label, std::move(trigger));
        EXPECT_TRUE(addResult.has_value());
        doc->rebuildTriggerArray();

        return raw;
    }

    std::unique_ptr<WorldDocument> doc;
};

// Test 1: Wildcard replacement in trigger contents
TEST_F(TriggerExecutionTest, WildcardReplacementInContents)
{
    // Create a trigger that sends text with wildcards
    Trigger* t = addTrigger("gold_notify", "You have * gold");
    t->contents = "Gold amount: %1 pieces"; // Will replace %1 with captured wildcard
    t->send_to = eSendToOutput;
    t->sequence = 100;

    // Create a line that matches
    auto line = createTestLine("You have 500 gold");

    // Evaluate triggers (should execute and replace %1 with "500")
    doc->evaluateTriggers(line.get());

    // Verify trigger executed
    EXPECT_EQ(t->matched, 1) << "Trigger should have executed once";

    // Verify wildcards were captured
    ASSERT_GT(t->wildcards.size(), 1) << "Should have captured wildcards";
    EXPECT_EQ(t->wildcards[1], "500") << "Wildcard %1 should be '500'";
}

// Test 2: Color changing
TEST_F(TriggerExecutionTest, ColorChanging)
{
    // Create a trigger that changes line color
    Trigger* t = addTrigger("warning_color", "Warning: *");
    t->other_foreground = qRgb(255, 0, 0); // Red foreground
    t->colour_change_type = ColourChangeType::Foreground;
    t->sequence = 200;

    auto line = createTestLine("Warning: Low health");

    // Evaluate trigger
    doc->evaluateTriggers(line.get());

    // Verify color was changed
    ASSERT_GT(line->styleList.size(), 0) << "Line should have style";
    Style* style = line->styleList[0].get();
    EXPECT_EQ(style->iForeColour, qRgb(255, 0, 0)) << "Line color should be red";
}

// Test 3: One-shot trigger (deletes after first match)
TEST_F(TriggerExecutionTest, OneShotTrigger)
{
    // Create a one-shot trigger
    Trigger* t = addTrigger("level_up", "You level up!");
    t->one_shot = true; // Delete after first match
    t->sequence = 300;

    // Verify trigger exists
    ASSERT_NE(doc->getTrigger("level_up"), nullptr) << "One-shot trigger should be created";

    auto line = createTestLine("You level up!");

    // Evaluate triggers (should execute and delete)
    doc->evaluateTriggers(line.get());

    // Verify trigger was deleted
    EXPECT_EQ(doc->getTrigger("level_up"), nullptr)
        << "One-shot trigger should be deleted after firing";
}

// Test 4: Multiple wildcards in contents
TEST_F(TriggerExecutionTest, MultipleWildcardsInContents)
{
    // Create a trigger that captures multiple wildcards
    Trigger* t = addTrigger("tell_format", "* tells you: *");
    t->contents = "Message from %1: %2";
    t->send_to = eSendToOutput;
    t->sequence = 400;

    auto line = createTestLine("Alice tells you: Hello!");

    doc->evaluateTriggers(line.get());

    // Verify multiple wildcards were captured
    ASSERT_GT(t->wildcards.size(), 2) << "Should have captured 2 wildcards";
    EXPECT_EQ(t->wildcards[1], "Alice") << "First wildcard should be 'Alice'";
    EXPECT_EQ(t->wildcards[2], "Hello!") << "Second wildcard should be 'Hello!'";
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
    t->procedure = "on_health_trigger"; // Lua function to call
    t->send_to = eSendToScript;
    t->sequence = 500;

    auto line = createTestLine("Your health is 75%");

    // Evaluate triggers (should call Lua function)
    doc->evaluateTriggers(line.get());

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
    EXPECT_EQ(t->invocation_count, 1) << "Invocation count should be incremented";
}

// Test: %% produces a single literal percent sign (M3 — original evaluate.cpp:1268-1278)
TEST_F(TriggerExecutionTest, DoublePercentEscape)
{
    QVector<QString> wildcards = {"whole", "val"};
    QMap<QString, QString> named;
    QString result = doc->replaceWildcards("100%% done", wildcards, "trig", named);
    EXPECT_EQ(result, "100% done") << "%% should produce a single %";
}

// Test: multi-digit %10 is NOT substituted (original MAX_WILDCARDS = 10, single-digit only)
// (M3 — original: evaluate.cpp:1284 — case '0': ... case '9': only)
TEST_F(TriggerExecutionTest, MultiDigitWildcardNotSubstituted)
{
    QVector<QString> wildcards;
    for (int i = 0; i <= 10; ++i)
        wildcards.append(QString("w%1").arg(i));
    QMap<QString, QString> named;
    QString result = doc->replaceWildcards("%10 test", wildcards, "trig", named);
    // %1 should be replaced, then "0 test" remains (not %10 as a unit)
    EXPECT_EQ(result, "w1" + QString("0 test"))
        << "%10 should be parsed as %1 followed by literal '0', not a two-digit wildcard";
}

// Test: lowercase %n is handled (M3 — original evaluate.cpp:1398-1399, case 'N': case 'n':)
TEST_F(TriggerExecutionTest, LowercasePercentN)
{
    QVector<QString> wildcards;
    QMap<QString, QString> named;
    QString result = doc->replaceWildcards("name=%n", wildcards, "MyTrigger", named);
    EXPECT_EQ(result, "name=MyTrigger") << "%n (lowercase) should substitute item name";
}

// Test: %N (uppercase) also works
TEST_F(TriggerExecutionTest, UppercasePercentN)
{
    QVector<QString> wildcards;
    QMap<QString, QString> named;
    QString result = doc->replaceWildcards("name=%N", wildcards, "MyTrigger", named);
    EXPECT_EQ(result, "name=MyTrigger") << "%N should substitute item name";
}

// Test: FixWildcard escaping for script destinations (M3 — original evaluate.cpp:277-284)
// When send_to is eSendToScript, backslashes and double-quotes in wildcard values are escaped.
TEST_F(TriggerExecutionTest, WildcardScriptEscaping)
{
    QVector<QString> wildcards = {"whole", R"(say "hello\nworld")"};
    QMap<QString, QString> named;
    // Non-script destination: no escaping
    QString plain = doc->replaceWildcards("%1", wildcards, "trig", named, eSendToWorld);
    EXPECT_EQ(plain, R"(say "hello\nworld")") << "Non-script: no escaping";

    // Script destination: backslash → \\, double-quote → \"
    QString scripted = doc->replaceWildcards("%1", wildcards, "trig", named, eSendToScript);
    EXPECT_EQ(scripted, R"(say \"hello\\nworld\")") << "Script dest: should escape \\ and \"";
}

// Test: trigger with non-script send_to still fires Lua callback if procedure is set
// (M89 — original: ProcessPreviousLine.cpp:1326-1327 adds to triggerList unconditionally)
TEST_F(TriggerExecutionTest, ScriptCallbackFiredForNonScriptSendTo)
{
    QString luaScript = R"(
callback_was_called = false
function on_world_trigger(name, line, wildcards, styles)
    callback_was_called = true
end
)";
    ASSERT_NE(doc->m_ScriptEngine, nullptr);
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Trigger* t = addTrigger("world_send_trigger", "test line");
    t->procedure = "on_world_trigger";
    t->send_to = eSendToWorld; // NOT eSendToScript — callback should still fire
    t->sequence = 600;

    auto line = createTestLine("test line");
    doc->evaluateTriggers(line.get());

    lua_State* L = doc->m_ScriptEngine->L;
    ASSERT_NE(L, nullptr);

    lua_getglobal(L, "callback_was_called");
    bool called = lua_toboolean(L, -1);
    lua_pop(L, 1);
    EXPECT_TRUE(called) << "Lua callback should fire even when send_to != eSendToScript";
}

// Test: style flags in trigger callback use '& 7' mask, not TEXT_STYLE (M187 LOW)
// (original: ProcessPreviousLine.cpp:228 — "pStyle->iFlags & 7")
TEST_F(TriggerExecutionTest, StyleRunsFlagsMaskedToSevenBits)
{
    QString luaScript = R"(
style_flags_received = -1
function on_style_trigger(name, line, wildcards, styles)
    if styles and styles[1] then
        style_flags_received = styles[1].style
    end
end
)";
    ASSERT_NE(doc->m_ScriptEngine, nullptr);
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Trigger* t = addTrigger("style_trigger", "styled line");
    t->procedure = "on_style_trigger";
    t->send_to = eSendToScript;
    t->sequence = 700;

    auto line = createTestLine("styled line");
    // Set INVERSE (0x08) and STRIKEOUT (0x20) flags on the style — these should be stripped
    ASSERT_FALSE(line->styleList.empty());
    line->styleList[0]->iFlags = HILITE | INVERSE | STRIKEOUT; // 0x01 | 0x08 | 0x20 = 0x29

    doc->evaluateTriggers(line.get());

    lua_State* L = doc->m_ScriptEngine->L;
    ASSERT_NE(L, nullptr);

    lua_getglobal(L, "style_flags_received");
    int flags = lua_tointeger(L, -1);
    lua_pop(L, 1);

    // Original masks with & 7, so only bits 0-2 (HILITE, UNDERLINE, BLINK) survive
    EXPECT_EQ(flags, HILITE & 7)
        << "Style flags should be masked to bits 0-2 only (HILITE|UNDERLINE|BLINK)";
    EXPECT_EQ(flags & INVERSE, 0) << "INVERSE bit (0x08) should not appear in style runs";
    EXPECT_EQ(flags & STRIKEOUT, 0) << "STRIKEOUT bit (0x20) should not appear in style runs";
}
