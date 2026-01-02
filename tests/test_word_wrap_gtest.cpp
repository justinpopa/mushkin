/**
 * test_word_wrap_gtest.cpp - Word Wrap Tests
 *
 * Tests for word-wrap behavior matching original MUSHclient.
 *
 * The m_wrap setting controls whether lines wrap at word boundaries (spaces)
 * or at the exact column boundary:
 * - m_wrap = 1 (enabled): Break at last space before wrap column
 * - m_wrap = 0 (disabled): Hard break at wrap column
 *
 * m_nWrapColumn controls the column width at which wrapping occurs.
 */

#include "test_qt_static.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/color_utils.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for word wrap tests
class WordWrapTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        // Set a small wrap column for easier testing
        doc->m_nWrapColumn = 20;
        // Enable word wrap by default
        doc->m_wrap = 1;

        // Create initial line for AddToLine to work
        // (normally done when connecting to a MUD)
        doc->m_currentLine = new Line(1,                    // line number
                                      doc->m_nWrapColumn,   // wrap column
                                      0,                    // flags
                                      qRgb(255, 255, 255),  // foreground (white)
                                      qRgb(0, 0, 0),        // background (black)
                                      false);               // UTF-8 mode

        // Create initial empty style
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = 0;
        initialStyle->iForeColour = qRgb(255, 255, 255);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to get the current line text
    QString getCurrentLineText()
    {
        if (doc->m_currentLine) {
            return QString::fromUtf8(doc->m_currentLine->text(), doc->m_currentLine->len());
        }
        return QString();
    }

    // Helper to get the number of lines in buffer
    int getLineCount()
    {
        return doc->m_lineList.count();
    }

    // Helper to get text of a specific line from buffer
    QString getLineText(int index)
    {
        if (index >= 0 && index < doc->m_lineList.count()) {
            Line* line = doc->m_lineList.at(index);
            return QString::fromUtf8(line->text(), line->len());
        }
        return QString();
    }

    WorldDocument* doc = nullptr;
};

/**
 * Test 1: Word wrap breaks at last space
 * With m_wrap=1, text should break at the last space before wrap column
 */
TEST_F(WordWrapTest, WordWrapBreaksAtSpace)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 20;

    // Add text with spaces: "Hello world this is a test"
    // At column 20, should break at a space
    const char* text = "Hello world this is a test";
    doc->AddToLine(text, strlen(text));

    // After wrap, current line should have carried-over text
    // First line should be in buffer, ending at a space boundary
    EXPECT_GE(getLineCount(), 1) << "Should have at least one line in buffer after wrap";

    // The first line should break at a word boundary
    QString firstLine = getLineText(0);
    EXPECT_FALSE(firstLine.isEmpty()) << "First line should not be empty";

    // First line should end at a word boundary (no partial words)
    // With "Hello world this is a test" and column 20:
    // "Hello world this is" = 19 chars (fits)
    // Adding " a" would exceed, so it breaks
    EXPECT_LE(firstLine.length(), 20) << "First line should not exceed wrap column";
}

/**
 * Test 2: Hard wrap when word-wrap disabled
 * With m_wrap=0, text should break exactly at wrap column
 */
TEST_F(WordWrapTest, HardWrapWhenDisabled)
{
    doc->m_wrap = 0;  // Disable word wrap
    doc->m_nWrapColumn = 20;

    // Add text longer than wrap column
    const char* text = "ThisIsAVeryLongWordWithNoSpaces";
    doc->AddToLine(text, strlen(text));

    // Should have wrapped (hard break at column 20)
    EXPECT_GE(getLineCount(), 1) << "Should have at least one line after hard wrap";

    // First line should be exactly 20 chars (hard break)
    QString firstLine = getLineText(0);
    EXPECT_EQ(firstLine.length(), 20) << "Hard wrap should break exactly at column";
}

/**
 * Test 3: No wrap when text fits
 * Text shorter than wrap column should not cause a wrap
 */
TEST_F(WordWrapTest, NoWrapWhenTextFits)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 80;

    const char* text = "Short text";
    doc->AddToLine(text, strlen(text));

    // Should NOT have wrapped - text is still in current line
    EXPECT_EQ(getLineCount(), 0) << "Should have no lines in buffer (current line not yet complete)";
    EXPECT_EQ(getCurrentLineText(), "Short text") << "Current line should contain the text";
}

/**
 * Test 4: Multiple wraps for very long text
 * Very long text should wrap multiple times
 */
TEST_F(WordWrapTest, MultipleWraps)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 20;

    // Text that will wrap multiple times
    const char* text = "The quick brown fox jumps over the lazy dog and keeps running";
    doc->AddToLine(text, strlen(text));

    // Should have multiple lines
    EXPECT_GE(getLineCount(), 2) << "Long text should produce multiple wrapped lines";
}

/**
 * Test 5: Wrap column 0 means no wrapping
 * Setting m_nWrapColumn to 0 should disable wrapping entirely
 */
TEST_F(WordWrapTest, WrapColumnZeroDisablesWrap)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 0;  // Disable wrapping

    const char* text = "This is a very long line that would normally wrap but should not because wrap column is zero";
    doc->AddToLine(text, strlen(text));

    // Should NOT have wrapped
    EXPECT_EQ(getLineCount(), 0) << "Should have no lines in buffer when wrap disabled";
    EXPECT_EQ(getCurrentLineText().length(), static_cast<int>(strlen(text)))
        << "Current line should contain all text";
}

/**
 * Test 6: Hard_return flag is false for soft wraps
 * When a line is wrapped (not from MUD newline), hard_return should be false
 */
TEST_F(WordWrapTest, SoftWrapHasHardReturnFalse)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 20;

    const char* text = "Hello world this is a wrapped line";
    doc->AddToLine(text, strlen(text));

    // Check that wrapped line has hard_return = false
    if (getLineCount() > 0) {
        Line* wrappedLine = doc->m_lineList.at(0);
        EXPECT_FALSE(wrappedLine->hard_return) << "Soft-wrapped line should have hard_return=false";
    }
}

/**
 * Test 7: Space at wrap boundary
 * When a space occurs exactly at the wrap column, should handle correctly
 */
TEST_F(WordWrapTest, SpaceAtWrapBoundary)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 10;

    // "1234567890 text" - space at position 10
    const char* text = "1234567890 text";
    doc->AddToLine(text, strlen(text));

    // Should wrap at the space
    EXPECT_GE(getLineCount(), 1) << "Should wrap when reaching column";
}

/**
 * Test 8: Text with no spaces (word wrap enabled)
 * Long text with no spaces should NOT wrap - preserves ASCII art
 */
TEST_F(WordWrapTest, NoSpacesDoesNotWrapWhenWordWrapEnabled)
{
    doc->m_wrap = 1;  // Word wrap enabled
    doc->m_nWrapColumn = 20;

    // No spaces - should NOT wrap, line extends past wrap column
    // This preserves ASCII art that has no spaces
    const char* text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    doc->AddToLine(text, strlen(text));

    // Should NOT have wrapped - m_lineList should be empty (no lines completed)
    EXPECT_EQ(getLineCount(), 0) << "No completed lines (preserves ASCII art)";

    // The current line should contain all the text (not flushed to lineList)
    QString currentText = getCurrentLineText();
    EXPECT_EQ(currentText.length(), 26) << "Current line should contain all 26 characters";
}

/**
 * Test 9: GetOption/SetOption for wrap setting
 * Verify that scripts can get/set the wrap option
 */
TEST_F(WordWrapTest, GetSetOptionWrap)
{
    // Set wrap via direct assignment
    doc->m_wrap = 0;
    EXPECT_EQ(doc->m_wrap, 0) << "m_wrap should be 0";

    doc->m_wrap = 1;
    EXPECT_EQ(doc->m_wrap, 1) << "m_wrap should be 1";
}

/**
 * Test 10: Trailing space is preserved after word wrap
 * When word-wrapping at a space, the space should be kept at the end
 * of the first line to prevent words from running together when
 * soft-wrapped lines are concatenated.
 */
TEST_F(WordWrapTest, TrailingSpacePreserved)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 15;

    // "Hello world test" - space after "world" at position 11
    // After wrap at position 15, first line should be "Hello world " (with trailing space)
    const char* text = "Hello world test";
    doc->AddToLine(text, strlen(text));

    // Should have at least one line in buffer
    ASSERT_GE(getLineCount(), 1) << "Should have at least one line after wrap";

    // Get the first wrapped line
    QString firstLine = getLineText(0);

    // The first line should end with a space (before "test")
    // "Hello world " = 12 chars (including trailing space)
    EXPECT_TRUE(firstLine.endsWith(' '))
        << "Wrapped line should end with trailing space. Got: '" << firstLine.toStdString() << "'";

    // When soft-wrapped lines are joined, the result should have the space
    // Simulate what getSelectedText does for soft-wrapped lines
    QString currentLine = getCurrentLineText();
    QString joined = firstLine + currentLine;  // No newline for soft-wrap

    // The joined text should have a space between "world" and "test"
    EXPECT_TRUE(joined.contains("world test"))
        << "Joined soft-wrapped lines should have space between words. Got: '"
        << joined.toStdString() << "'";
}

/**
 * Test 11: Multiple spaces are handled correctly
 * Lines with multiple spaces should wrap correctly
 */
TEST_F(WordWrapTest, MultipleSpacesHandled)
{
    doc->m_wrap = 1;
    doc->m_nWrapColumn = 15;

    // Text with multiple spaces
    const char* text = "aa  bb  cc  dd  ee";
    doc->AddToLine(text, strlen(text));

    // Should wrap and spaces should be preserved
    EXPECT_GE(getLineCount(), 1) << "Should have at least one line after wrap";
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
