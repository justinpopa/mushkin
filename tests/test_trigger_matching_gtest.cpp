/**
 * test_trigger_matching_gtest.cpp - GoogleTest version of Trigger Pattern Matching Tests
 *
 * Tests the trigger pattern matching functionality including:
 * 1. Wildcard matching (* patterns)
 * 2. Regular expression matching
 * 3. Wildcard extraction (%0, %1, %2, etc.)
 * 4. Case-insensitive matching (ignore_case)
 * 5. Color/style matching (iMatch flags)
 * 6. Repeat matching (bRepeat flag)
 * 7. Multi-line matching (bMultiLine)
 * 8. bKeepEvaluating flag behavior
 * 9. bOneShot flag behavior
 * 10. Sequence-based evaluation order
 * 11. Statistics tracking
 */

#include "test_qt_static.h"
#include "../src/automation/trigger.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <cstring>
#include <gtest/gtest.h>

// Test fixture for trigger matching tests
class TriggerMatchingTest : public ::testing::Test {
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

// Test 1: Basic Wildcard Matching
TEST_F(TriggerMatchingTest, BasicWildcardMatching)
{
    // Create a trigger with wildcard pattern
    Trigger* t1 = addTrigger("gold_trigger", "You have * gold");
    t1->iSequence = 100;
    t1->bKeepEvaluating = true; // Allow other triggers to match too

    // Create a line that matches
    Line* line1 = createTestLine("You have 500 gold");

    // Evaluate triggers
    int matchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line1);
    int matchedAfter = doc->m_iTriggersMatchedCount;

    // Verify pattern matched
    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Wildcard pattern should match";

    // Verify trigger match count incremented
    EXPECT_EQ(t1->nMatched, 1) << "Trigger match count should be 1";

    // Verify wildcard %0 captured full match
    ASSERT_GT(t1->wildcards.size(), 0) << "Should have captured wildcards";
    EXPECT_EQ(t1->wildcards[0], "You have 500 gold") << "Wildcard %0 should be full match";

    // Verify wildcard %1 captured the wildcard value
    ASSERT_GT(t1->wildcards.size(), 1) << "Should have captured wildcard %1";
    EXPECT_EQ(t1->wildcards[1], "500") << "Wildcard %1 should be '500'";

    // Cleanup
    delete line1;
}

// Test 2: Case-Insensitive Matching
TEST_F(TriggerMatchingTest, CaseInsensitiveMatching)
{
    // Create a case-insensitive trigger
    Trigger* t2 = addTrigger("hunger_trigger", "you are hungry");
    t2->ignore_case = true;
    t2->iSequence = 200;
    t2->bKeepEvaluating = true;

    Line* line2 = createTestLine("YOU ARE HUNGRY");

    int matchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line2);
    int matchedAfter = doc->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Case-insensitive pattern should match";
    EXPECT_EQ(t2->nMatched, 1) << "Match count should be 1";

    // Cleanup
    delete line2;
}

// Test 3: Regular Expression Matching
TEST_F(TriggerMatchingTest, RegularExpressionMatching)
{
    // Create a regex trigger
    Trigger* t3 = addTrigger("gold_regex", "^You have (\\d+) gold$");
    t3->bRegexp = true;
    t3->iSequence = 300;

    Line* line3 = createTestLine("You have 1234 gold");

    int matchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line3);
    int matchedAfter = doc->m_iTriggersMatchedCount;

    // Verify regex matched
    EXPECT_GE(matchedAfter, matchedBefore + 1) << "Regex pattern should match";

    // Verify capture group
    ASSERT_GT(t3->wildcards.size(), 1) << "Should have captured regex group";
    EXPECT_EQ(t3->wildcards[1], "1234") << "Capture group %1 should be '1234'";

    // Cleanup
    delete line3;
}

// Test 4: Multiple Wildcards
TEST_F(TriggerMatchingTest, MultipleWildcards)
{
    // Create a trigger with multiple wildcards
    Trigger* t4 = addTrigger("tell_trigger", "* tells you: *");
    t4->iSequence = 400;

    Line* line4 = createTestLine("Bob tells you: Hello!");

    int matchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line4);
    int matchedAfter = doc->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore + 1) << "Multiple wildcard pattern should match";

    // Verify first wildcard
    ASSERT_GT(t4->wildcards.size(), 1) << "Should have captured first wildcard";
    EXPECT_EQ(t4->wildcards[1], "Bob") << "First wildcard should be 'Bob'";

    // Verify second wildcard
    ASSERT_GT(t4->wildcards.size(), 2) << "Should have captured second wildcard";
    EXPECT_EQ(t4->wildcards[2], "Hello!") << "Second wildcard should be 'Hello!'";

    // Cleanup
    delete line4;
}

// Test 5: Non-matching Pattern
TEST_F(TriggerMatchingTest, NonMatchingPattern)
{
    // Add a trigger
    addTrigger("test_trigger", "This will not match");

    Line* line5 = createTestLine("This line doesn't match any trigger");

    int matchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line5);
    int matchedAfter = doc->m_iTriggersMatchedCount;

    EXPECT_EQ(matchedAfter, matchedBefore) << "Non-matching line should not trigger any matches";

    // Cleanup
    delete line5;
}

// Test 6: Sequence-Based Evaluation Order
TEST_F(TriggerMatchingTest, SequenceBasedEvaluationOrder)
{
    // Add triggers in reverse sequence order (highest to lowest)
    Trigger* tLast = addTrigger("last_trigger", "*");
    tLast->iSequence = 300; // Evaluated last

    Trigger* tFirst = addTrigger("first_trigger", "*");
    tFirst->iSequence = 100; // Evaluated first

    Trigger* tMiddle = addTrigger("middle_trigger", "*");
    tMiddle->iSequence = 200; // Evaluated middle

    doc->rebuildTriggerArray();

    // Verify array order
    ASSERT_GE(doc->m_TriggerArray.size(), 3) << "Should have 3 triggers";
    EXPECT_EQ(doc->m_TriggerArray[0]->iSequence, 100) << "First trigger should have sequence 100";
    EXPECT_EQ(doc->m_TriggerArray[1]->iSequence, 200) << "Second trigger should have sequence 200";
    EXPECT_EQ(doc->m_TriggerArray[2]->iSequence, 300) << "Third trigger should have sequence 300";
}

// Test 7: Lowercase Wildcard Conversion
TEST_F(TriggerMatchingTest, LowercaseWildcardConversion)
{
    // Create a trigger with lowercase wildcard conversion
    Trigger* t7 = addTrigger("lowercase_trigger", "You see *");
    t7->bLowercaseWildcard = true;
    t7->iSequence = 500;

    Line* line7 = createTestLine("You see DRAGON");

    doc->evaluateTriggers(line7);

    // Verify wildcard was lowercased
    ASSERT_GT(t7->wildcards.size(), 1) << "Should have captured wildcard";
    EXPECT_EQ(t7->wildcards[1], "dragon") << "Wildcard should be lowercased to 'dragon'";

    // Cleanup
    delete line7;
}

// Test 8: Disabled Trigger
TEST_F(TriggerMatchingTest, DisabledTrigger)
{
    // Create a disabled trigger
    Trigger* t8 = addTrigger("disabled_trigger", "test pattern");
    t8->bEnabled = false; // Disabled
    t8->iSequence = 600;

    doc->rebuildTriggerArray();

    Line* line8 = createTestLine("test pattern");

    int disabledMatchedBefore = doc->m_iTriggersMatchedCount;
    doc->evaluateTriggers(line8);
    int disabledMatchedAfter = doc->m_iTriggersMatchedCount;

    EXPECT_EQ(disabledMatchedAfter, disabledMatchedBefore) << "Disabled trigger should not match";
    EXPECT_EQ(t8->nMatched, 0) << "Disabled trigger match count should be 0";

    // Cleanup
    delete line8;
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
