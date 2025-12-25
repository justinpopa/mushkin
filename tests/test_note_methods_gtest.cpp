/**
 * test_note_methods_gtest.cpp - GoogleTest version
 * Test script output methods: note(), colourNote(), and colourTell()
 *
 * These tests verify:
 * 1. note() displays text with COMMENT flag
 * 2. colourNote() displays text with specified colors and COMMENT flag
 * 3. colourTell() displays text without newline
 * 4. Style state is properly saved/restored
 * 5. m_bNotesNotWantedNow suppression works
 * 6. Unicode text handling
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for note methods tests
class NoteMethodsTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();

        // Initialize basic state
        doc->m_phase = NONE;
        doc->m_bUTF_8 = true; // UTF-8 mode
        doc->m_bNotesInRGB = true;
        doc->m_iNoteColourFore = qRgb(255, 255, 255); // White
        doc->m_iNoteColourBack = qRgb(0, 0, 0);       // Black
        doc->m_iNoteStyle = 0;                        // No special styling

        // Create initial line (simulating active connection)
        doc->m_currentLine = new Line(1, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_RGB;
        initialStyle->iForeColour = qRgb(192, 192, 192);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));

        // Set current style (simulating MUD output in progress)
        doc->m_iFlags = COLOUR_RGB | HILITE;  // Bold text
        doc->m_iForeColour = qRgb(255, 0, 0); // Red
        doc->m_iBackColour = qRgb(0, 0, 0);   // Black
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Basic note() functionality
TEST_F(NoteMethodsTest, BasicNote)
{
    doc->note("This is a test note");

    // Verify the note was added
    ASSERT_GE(doc->m_lineList.count(), 1) << "Note should be added to buffer";

    Line* line = doc->m_lineList.last();
    QString text = QString::fromUtf8(line->text(), line->len());

    // Check COMMENT flag
    EXPECT_TRUE(line->flags & COMMENT) << "COMMENT flag should be set";

    // Check text
    EXPECT_EQ(text, "This is a test note") << "Text should match";

    // Check hard return
    EXPECT_TRUE(line->hard_return) << "Hard return should be set (newline completed)";

    // Check that a new line was created for continued output
    EXPECT_NE(doc->m_currentLine, line)
        << "New current line should be created for continued output";

    // Verify style was restored
    EXPECT_EQ(doc->m_iFlags, COLOUR_RGB | HILITE) << "Flags should be restored";
    EXPECT_EQ(doc->m_iForeColour, qRgb(255, 0, 0)) << "Foreground color should be restored to red";
    EXPECT_EQ(doc->m_iBackColour, qRgb(0, 0, 0)) << "Background color should be restored to black";
}

// Test 2: colourNote() with custom colors
TEST_F(NoteMethodsTest, ColourNoteWithCustomColors)
{
    QRgb greenColor = qRgb(0, 255, 0); // Bright green
    QRgb blueColor = qRgb(0, 0, 255);  // Blue background

    doc->colourNote(greenColor, blueColor, "Green text on blue background");

    // Verify the colored note
    ASSERT_GE(doc->m_lineList.count(), 1) << "Colored note should be added";

    Line* line = doc->m_lineList.last();
    QString text = QString::fromUtf8(line->text(), line->len());

    // Check COMMENT flag
    EXPECT_TRUE(line->flags & COMMENT) << "COMMENT flag should be set";

    // Check text
    EXPECT_EQ(text, "Green text on blue background") << "Text should match";

    // Check style colors
    // Note: Line may have initial empty style (length=0), we want the style with actual text
    Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }

    ASSERT_NE(style, nullptr) << "Line should have at least one style with content";

    // Check for RGB color mode
    EXPECT_TRUE(style->iFlags & COLOUR_RGB) << "RGB color mode should be used";

    // Check foreground color
    EXPECT_EQ(style->iForeColour, greenColor) << "Foreground color should be green";

    // Check background color
    EXPECT_EQ(style->iBackColour, blueColor) << "Background color should be blue";

    // Verify style was restored
    EXPECT_EQ(doc->m_iFlags, COLOUR_RGB | HILITE) << "Flags should be restored after colourNote()";
    EXPECT_EQ(doc->m_iForeColour, qRgb(255, 0, 0)) << "Foreground color should be restored";
    EXPECT_EQ(doc->m_iBackColour, qRgb(0, 0, 0)) << "Background color should be restored";
}

// Test 3: colourTell() without newline
TEST_F(NoteMethodsTest, ColourTellWithoutNewline)
{
    int lineCountBefore = doc->m_lineList.count();

    QRgb yellowColor = qRgb(255, 255, 0); // Yellow
    QRgb blackColor = qRgb(0, 0, 0);      // Black

    doc->colourTell(yellowColor, blackColor, "Part 1 ");
    doc->colourTell(qRgb(255, 0, 255), blackColor, "Part 2 "); // Magenta
    doc->colourTell(qRgb(0, 255, 255), blackColor, "Part 3");  // Cyan

    // Verify no new lines were added yet
    EXPECT_EQ(doc->m_lineList.count(), lineCountBefore)
        << "No lines should be added yet (text on current line)";

    // Check current line has the text
    ASSERT_NE(doc->m_currentLine, nullptr) << "Current line should exist";

    QString text = QString::fromUtf8(doc->m_currentLine->text(), doc->m_currentLine->len());

    EXPECT_TRUE(text.contains("Part 1")) << "Part 1 should be in current line";
    EXPECT_TRUE(text.contains("Part 2")) << "Part 2 should be in current line";
    EXPECT_TRUE(text.contains("Part 3")) << "Part 3 should be in current line";

    // Check we have multiple styles (one for each colourTell call)
    EXPECT_GE(doc->m_currentLine->styleList.size(), 3) << "Should have at least 3 styles";

    // Now complete the line with a note
    doc->note(""); // Empty note to complete the line

    // Verify the line was completed
    EXPECT_GT(doc->m_lineList.count(), lineCountBefore) << "Line should be completed";

    Line* completedLine = doc->m_lineList[lineCountBefore];
    QString completedText = QString::fromUtf8(completedLine->text(), completedLine->len());

    EXPECT_TRUE(completedText.contains("Part 1")) << "Part 1 should be in completed line";
    EXPECT_TRUE(completedText.contains("Part 2")) << "Part 2 should be in completed line";
    EXPECT_TRUE(completedText.contains("Part 3")) << "Part 3 should be in completed line";
}

// Test 4: m_bNotesNotWantedNow suppression
TEST_F(NoteMethodsTest, NotesSuppression)
{
    doc->m_bNotesNotWantedNow = true; // Disable notes
    int lineCountBeforeSuppressed = doc->m_lineList.count();

    doc->note("This should not appear");

    EXPECT_EQ(doc->m_lineList.count(), lineCountBeforeSuppressed)
        << "Note should be suppressed when m_bNotesNotWantedNow = true";

    doc->m_bNotesNotWantedNow = false; // Re-enable notes
}

// Test 5: Unicode text in notes
TEST_F(NoteMethodsTest, UnicodeTextHandling)
{
    int lineCountBefore = doc->m_lineList.count();

    doc->note("Unicode test: Café ☕ 你好 ");

    ASSERT_GT(doc->m_lineList.count(), lineCountBefore) << "Unicode note should be added";

    Line* line = doc->m_lineList.last();
    QString text = QString::fromUtf8(line->text(), line->len());

    EXPECT_TRUE(text.contains("Café")) << "Café should be preserved";
    EXPECT_TRUE(text.contains("☕")) << "Coffee emoji should be preserved";
    EXPECT_TRUE(text.contains("你好")) << "Chinese characters should be preserved";
    EXPECT_TRUE(text.contains("")) << "Emoji should be preserved";
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
