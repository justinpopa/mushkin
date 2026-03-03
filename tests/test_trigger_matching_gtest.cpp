/**
 * test_trigger_matching_gtest.cpp - GoogleTest version of Trigger Pattern Matching Tests
 *
 * Tests the trigger pattern matching functionality including:
 * 1. Wildcard matching (* patterns)
 * 2. Regular expression matching
 * 3. Wildcard extraction (%0, %1, %2, etc.)
 * 4. Case-insensitive matching (ignore_case)
 * 5. Color/style matching (match_type flags)
 * 6. Repeat matching (repeat flag)
 * 7. Multi-line matching (multi_line)
 * 8. keep_evaluating flag behavior
 * 9. one_shot flag behavior
 * 10. Sequence-based evaluation order
 * 11. Statistics tracking
 */

#include "../src/automation/trigger.h"
#include "fixtures/world_fixtures.h"
#include <cstring>

// Test fixture for trigger matching tests
class TriggerMatchingTest : public ::testing::Test {
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
        style->iLength = len;
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

// Test 1: Basic Wildcard Matching
TEST_F(TriggerMatchingTest, BasicWildcardMatching)
{
    // Create a trigger with wildcard pattern
    Trigger* t1 = addTrigger("gold_trigger", "You have * gold");
    t1->sequence = 100;
    t1->keep_evaluating = true; // Allow other triggers to match too

    // Create a line that matches
    auto line1 = createTestLine("You have 500 gold");

    // Evaluate triggers
    int matchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line1.get());
    int matchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    // Verify pattern matched
    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Wildcard pattern should match";

    // Verify trigger match count incremented
    EXPECT_EQ(t1->matched, 1) << "Trigger match count should be 1";

    // Verify wildcard %0 captured full match
    ASSERT_GT(t1->wildcards.size(), 0) << "Should have captured wildcards";
    EXPECT_EQ(t1->wildcards[0], "You have 500 gold") << "Wildcard %0 should be full match";

    // Verify wildcard %1 captured the wildcard value
    ASSERT_GT(t1->wildcards.size(), 1) << "Should have captured wildcard %1";
    EXPECT_EQ(t1->wildcards[1], "500") << "Wildcard %1 should be '500'";
}

// Test 2: Case-Insensitive Matching
TEST_F(TriggerMatchingTest, CaseInsensitiveMatching)
{
    // Create a case-insensitive trigger
    Trigger* t2 = addTrigger("hunger_trigger", "you are hungry");
    t2->ignore_case = true;
    t2->sequence = 200;
    t2->keep_evaluating = true;

    auto line2 = createTestLine("YOU ARE HUNGRY");

    int matchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line2.get());
    int matchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Case-insensitive pattern should match";
    EXPECT_EQ(t2->matched, 1) << "Match count should be 1";
}

// Test 3: Regular Expression Matching
TEST_F(TriggerMatchingTest, RegularExpressionMatching)
{
    // Create a regex trigger
    Trigger* t3 = addTrigger("gold_regex", "^You have (\\d+) gold$");
    t3->use_regexp = true;
    t3->sequence = 300;

    auto line3 = createTestLine("You have 1234 gold");

    int matchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line3.get());
    int matchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    // Verify regex matched
    EXPECT_GE(matchedAfter, matchedBefore + 1) << "Regex pattern should match";

    // Verify capture group
    ASSERT_GT(t3->wildcards.size(), 1) << "Should have captured regex group";
    EXPECT_EQ(t3->wildcards[1], "1234") << "Capture group %1 should be '1234'";
}

// Test 4: Multiple Wildcards
TEST_F(TriggerMatchingTest, MultipleWildcards)
{
    // Create a trigger with multiple wildcards
    Trigger* t4 = addTrigger("tell_trigger", "* tells you: *");
    t4->sequence = 400;

    auto line4 = createTestLine("Bob tells you: Hello!");

    int matchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line4.get());
    int matchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Multiple wildcard pattern should match";

    // Verify first wildcard
    ASSERT_GT(t4->wildcards.size(), 1) << "Should have captured first wildcard";
    EXPECT_EQ(t4->wildcards[1], "Bob") << "First wildcard should be 'Bob'";

    // Verify second wildcard
    ASSERT_GT(t4->wildcards.size(), 2) << "Should have captured second wildcard";
    EXPECT_EQ(t4->wildcards[2], "Hello!") << "Second wildcard should be 'Hello!'";
}

// Test 5: Non-matching Pattern
TEST_F(TriggerMatchingTest, NonMatchingPattern)
{
    // Add a trigger
    addTrigger("test_trigger", "This will not match");

    auto line5 = createTestLine("This line doesn't match any trigger");

    int matchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line5.get());
    int matchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore) << "Non-matching line should not trigger any matches";
}

// Test 6: Sequence-Based Evaluation Order
TEST_F(TriggerMatchingTest, SequenceBasedEvaluationOrder)
{
    // Add triggers in reverse sequence order (highest to lowest)
    Trigger* tLast = addTrigger("last_trigger", "*");
    tLast->sequence = 300; // Evaluated last

    Trigger* tFirst = addTrigger("first_trigger", "*");
    tFirst->sequence = 100; // Evaluated first

    Trigger* tMiddle = addTrigger("middle_trigger", "*");
    tMiddle->sequence = 200; // Evaluated middle

    doc->rebuildTriggerArray();

    // Verify array order
    ASSERT_GE(doc->m_automationRegistry->m_TriggerArray.size(), 3) << "Should have 3 triggers";
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[0]->sequence, 100)
        << "First trigger should have sequence 100";
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[1]->sequence, 200)
        << "Second trigger should have sequence 200";
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[2]->sequence, 300)
        << "Third trigger should have sequence 300";
}

// Test 7: Lowercase Wildcard Conversion
TEST_F(TriggerMatchingTest, LowercaseWildcardConversion)
{
    // Create a trigger with lowercase wildcard conversion
    Trigger* t7 = addTrigger("lowercase_trigger", "You see *");
    t7->lowercase_wildcard = true;
    t7->sequence = 500;

    auto line7 = createTestLine("You see DRAGON");

    doc->evaluateTriggers(line7.get());

    // Verify wildcard was lowercased
    ASSERT_GT(t7->wildcards.size(), 1) << "Should have captured wildcard";
    EXPECT_EQ(t7->wildcards[1], "dragon") << "Wildcard should be lowercased to 'dragon'";
}

// Test 8: Disabled Trigger
TEST_F(TriggerMatchingTest, DisabledTrigger)
{
    // Create a disabled trigger
    Trigger* t8 = addTrigger("disabled_trigger", "test pattern");
    t8->enabled = false; // Disabled
    t8->sequence = 600;

    doc->rebuildTriggerArray();

    auto line8 = createTestLine("test pattern");

    int disabledMatchedBefore = doc->m_automationRegistry->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line8.get());
    int disabledMatchedAfter = doc->m_automationRegistry->m_iTriggersMatchedCount;

    EXPECT_EQ(disabledMatchedAfter, disabledMatchedBefore) << "Disabled trigger should not match";
    EXPECT_EQ(t8->matched, 0) << "Disabled trigger match count should be 0";
}
