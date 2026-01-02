/**
 * test_tab_completion_gtest.cpp - GoogleTest version
 *
 * Tab Completion - Test Suite (IDENTICAL behavior)
 *
 * Tests for handleTabCompletion() function in InputView.
 *
 * Based on sendvw.cpp:2036-2140 (OnKeysTab) in original MUSHclient.
 *
 * IMPORTANT: Tab completion searches OUTPUT BUFFER (m_lineList), NOT command history!
 * This matches the original MUSHclient behavior exactly.
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "test_qt_static.h"
#include "../src/text/line.h"
#include "../src/ui/views/input_view.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <cstring>
#include <gtest/gtest.h>

// Test fixture for tab completion tests
// Provides common setup/teardown and helper methods
class TabCompletionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        input = new InputView(doc);
    }

    void TearDown() override
    {
        delete input;
        delete doc;
    }

    /**
     * Helper function: Add text to output buffer (simulates MUD output)
     *
     * Creates a Line object and adds it to doc.m_lineList so tab completion
     * can search it.
     */
    void addLineToOutputBuffer(const QString& text)
    {
        // Create line with simple defaults
        Line* line = new Line(doc->m_lineList.count(), // lineNumber
                              80,                      // wrapColumn
                              0,                       // lineFlags
                              qRgb(255, 255, 255),     // foreColour (white)
                              qRgb(0, 0, 0),           // backColour (black)
                              false                    // isUnicode
        );

        // Copy text into line's buffer
        QByteArray textBytes = text.toUtf8();
        int len = textBytes.length();
        line->textBuffer.resize(len);
        memcpy(line->text(), textBytes.constData(), len);
        line->textBuffer.push_back('\0');

        // Add to output buffer
        doc->m_lineList.append(line);
    }

    WorldDocument* doc = nullptr;
    InputView* input = nullptr;
};

/**
 * Test 1: Simple completion from output buffer
 * Output buffer: ["kill archer", "look"]
 * Input: "kill ar" + Tab
 * Expected: "kill archer"
 */
TEST_F(TabCompletionTest, SimpleCompletion)
{
    // Add MUD output to buffer (tab completion searches this)
    addLineToOutputBuffer("kill archer");
    addLineToOutputBuffer("look");

    // Type partial command
    input->setText("kill ar");
    input->setCursorPosition(7); // After "ar"

    // Trigger tab completion
    input->handleTabCompletion();

    // Should complete to "archer" (no trailing space by default, matching original MUSHclient)
    EXPECT_EQ(input->text(), "kill archer") << "Simple completion should work";
}

/**
 * Test 2: Completion at start of line
 * Output buffer: ["north", "look"]
 * Input: "nor" + Tab
 * Expected: "north"
 */
TEST_F(TabCompletionTest, CompletionAtStart)
{
    addLineToOutputBuffer("north");
    addLineToOutputBuffer("look");

    input->setText("nor");
    input->setCursorPosition(3);

    input->handleTabCompletion();

    EXPECT_EQ(input->text(), "north") << "Completion at start of line should work";
}

/**
 * Test 3: Completion in middle of line
 * Output buffer: ["attack goblin warrior"]
 * Input: "attack gob|" + Tab (| = cursor)
 * Expected: "attack goblin"
 */
TEST_F(TabCompletionTest, CompletionInMiddle)
{
    addLineToOutputBuffer("attack goblin warrior");

    input->setText("attack gob");
    input->setCursorPosition(10); // After "gob"

    input->handleTabCompletion();

    EXPECT_EQ(input->text(), "attack goblin") << "Completion in middle of line should work";
}

/**
 * Test 4: Multiple matches - picks FIRST found (not alphabetical!)
 * Output buffer: ["kill axeman", "kill assassin", "kill archer"]
 * Input: "kill a" + Tab
 * Expected: "kill archer" (FIRST FOUND searching backwards from tail)
 */
TEST_F(TabCompletionTest, MultipleMatches)
{
    // Add in this order - search goes backwards from tail
    addLineToOutputBuffer("kill axeman");
    addLineToOutputBuffer("kill assassin");
    addLineToOutputBuffer("kill archer");

    input->setText("kill a");
    input->setCursorPosition(6);

    input->handleTabCompletion();

    // Should pick "archer" (FIRST match found searching backwards)
    EXPECT_EQ(input->text(), "kill archer") << "Multiple matches should pick first found";
}

/**
 * Test 5: No match - no change
 * Output buffer: ["north", "south"]
 * Input: "xyz" + Tab
 * Expected: "xyz" (no change)
 */
TEST_F(TabCompletionTest, NoMatch)
{
    addLineToOutputBuffer("north");
    addLineToOutputBuffer("south");

    input->setText("xyz");
    input->setCursorPosition(3);

    input->handleTabCompletion();

    // Should not change
    EXPECT_EQ(input->text(), "xyz") << "No match should leave input unchanged";
}

/**
 * Test 6: Case-insensitive matching
 * Output buffer: ["Kill Archer"]
 * Input: "kill ar" + Tab
 * Expected: "kill archer" (lowercase if m_bLowerCaseTabCompletion=true)
 */
TEST_F(TabCompletionTest, CaseInsensitive)
{
    addLineToOutputBuffer("Kill Archer");
    doc->m_bLowerCaseTabCompletion = true;

    input->setText("kill ar");
    input->setCursorPosition(7);

    input->handleTabCompletion();

    // Should match and convert to lowercase
    EXPECT_EQ(input->text(), "kill archer") << "Case-insensitive matching should work";
}

/**
 * Test 7: No space after completion if disabled
 * Output buffer: ["north"]
 * Input: "nor" + Tab
 * m_bTabCompletionSpace = false
 * Expected: "north" (no space)
 */
TEST_F(TabCompletionTest, NoSpaceAfterCompletion)
{
    addLineToOutputBuffer("north");
    doc->m_bTabCompletionSpace = false;

    input->setText("nor");
    input->setCursorPosition(3);

    input->handleTabCompletion();

    // Should not have trailing space
    EXPECT_EQ(input->text(), "north") << "Should not have space when disabled";
}

/**
 * Test 8: Completion with default list
 * m_strTabCompletionDefaults = "fireball lightning heal"
 * Output buffer: (empty)
 * Input: "fire" + Tab
 * Expected: "fireball"
 */
TEST_F(TabCompletionTest, DefaultCompletionList)
{
    doc->m_strTabCompletionDefaults = "fireball lightning heal";

    input->setText("fire");
    input->setCursorPosition(4);

    input->handleTabCompletion();

    EXPECT_EQ(input->text(), "fireball") << "Default completion list should work";
}

/**
 * Test 9: Empty output buffer - no completion
 * Output buffer: (empty)
 * Input: "north" + Tab
 * Expected: "north" (no change)
 */
TEST_F(TabCompletionTest, EmptyHistory)
{
    // No output buffer lines

    input->setText("north");
    input->setCursorPosition(5);

    input->handleTabCompletion();

    // Should not change
    EXPECT_EQ(input->text(), "north") << "Empty output buffer should be handled correctly";
}

/**
 * Test 10: Duplicate words - first match wins
 * Output buffer: ["kill orc", "kill orc", "kill orc"]
 * Input: "or" + Tab
 * Expected: "orc" (first match found)
 */
TEST_F(TabCompletionTest, DuplicateRemoval)
{
    addLineToOutputBuffer("kill orc");
    addLineToOutputBuffer("kill orc");
    addLineToOutputBuffer("kill orc");

    input->setText("or");
    input->setCursorPosition(2);

    input->handleTabCompletion();

    // Should complete to "orc" (first match found)
    EXPECT_EQ(input->text(), "orc") << "First match should win with duplicates";
}

/**
 * Test 11: Only complete if match is longer than prefix
 * Output buffer: ["a", "ab", "abc"]
 * Input: "abc" + Tab
 * Expected: "abc" (no change, no match longer than "abc")
 */
TEST_F(TabCompletionTest, MatchMustBeLonger)
{
    addLineToOutputBuffer("a");
    addLineToOutputBuffer("ab");
    addLineToOutputBuffer("abc");

    input->setText("abc");
    input->setCursorPosition(3);

    input->handleTabCompletion();

    // Should not change (no match longer than "abc")
    EXPECT_EQ(input->text(), "abc") << "Match must be longer than prefix";
}

/**
 * Test 12: Cursor positioning after completion
 * Output buffer: ["north"]
 * Input: "nor" + Tab
 * Expected: cursor at position 5 (after "north")
 */
TEST_F(TabCompletionTest, CursorPositioning)
{
    addLineToOutputBuffer("north");

    input->setText("nor");
    input->setCursorPosition(3);

    input->handleTabCompletion();

    EXPECT_EQ(input->text(), "north") << "Text should be completed";
    EXPECT_EQ(input->cursorPosition(), 5) << "Cursor should be positioned after 'north'";
}

/**
 * Test 13: Completion preserves text after cursor
 * Output buffer: ["northern"]
 * Input: "nor| path" (| = cursor) + Tab
 * Expected: "northern path" (completes "nor" to "northern")
 */
TEST_F(TabCompletionTest, PreservesTextAfterCursor)
{
    addLineToOutputBuffer("northern");

    input->setText("nor path");
    input->setCursorPosition(3); // After "nor"

    input->handleTabCompletion();

    // Should complete "nor" to "northern" and keep " path"
    EXPECT_EQ(input->text(), "northern path") << "Text after cursor should be preserved";
}

/**
 * Test 14: Multiple words in line - completes individual words
 * Output buffer: ["kill archer bronze"]
 * Input: "bron" + Tab
 * Expected: "bronze"
 */
TEST_F(TabCompletionTest, MultipleWordsInCommand)
{
    addLineToOutputBuffer("kill archer bronze");

    input->setText("bron");
    input->setCursorPosition(4);

    input->handleTabCompletion();

    EXPECT_EQ(input->text(), "bronze") << "Should complete individual words from multi-word lines";
}

/**
 * Test 15: Completion at cursor position (not at end)
 * Output buffer: ["archer"]
 * Input: "ar| south" (| = cursor at position 2) + Tab
 * Expected: "archer south"
 */
TEST_F(TabCompletionTest, CompletionNotAtEnd)
{
    addLineToOutputBuffer("archer");

    input->setText("ar south");
    input->setCursorPosition(2); // After "ar"

    input->handleTabCompletion();

    // Should complete "ar" to "archer" and keep " south"
    EXPECT_EQ(input->text(), "archer south") << "Completion should work when cursor not at end";
}

// Main function required for GoogleTest
// Note: QApplication must be created before any Qt objects
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects like WorldDocument and InputView)
    QApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
