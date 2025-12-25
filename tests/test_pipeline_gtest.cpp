/**
 * test_pipeline_gtest.cpp - GoogleTest version of Pipeline Test
 *
 * This tests the complete data flow:
 * Socket → ReceiveMsg() → ProcessIncomingByte() → AddToLine() → StartNewLine() → Display
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for pipeline tests
// Provides common setup/teardown and helper methods
class PipelineTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = new WorldDocument();

        // Initialize connection state (normally done by OnConnect)
        doc->m_phase = NONE;
        doc->m_bUTF_8 = false; // ASCII mode for simplicity

        // Initialize document style state
        doc->m_iFlags = COLOUR_ANSI;
        doc->m_iForeColour = WHITE;
        doc->m_iBackColour = BLACK;

        // Create initial line
        doc->m_currentLine = new Line(1, 80, COLOUR_ANSI, WHITE, BLACK, false);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_ANSI;
        initialStyle->iForeColour = WHITE;
        initialStyle->iBackColour = BLACK;
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper method to process a string byte by byte
    void processString(const char* str)
    {
        for (int i = 0; str[i]; i++) {
            doc->ProcessIncomingByte((unsigned char)str[i]);
        }
    }

    // Helper method to process a byte array
    void processBytes(const unsigned char* bytes)
    {
        for (int i = 0; bytes[i]; i++) {
            doc->ProcessIncomingByte(bytes[i]);
        }
    }

    // Get text from a line
    QString getLineText(int index) const
    {
        if (index >= doc->m_lineList.count()) {
            return QString();
        }
        Line* line = doc->m_lineList[index];
        return QString::fromUtf8(line->text(), line->len());
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Simple ASCII text processing
TEST_F(PipelineTest, SimpleASCIIText)
{
    // Process "Hello\n"
    processString("Hello\n");

    // Verify line was added
    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    QString text = getLineText(0);

    // Verify text content
    EXPECT_EQ(text, "Hello") << "Line text should be 'Hello'";
    EXPECT_EQ(line->len(), 5) << "Line length should be 5";
    EXPECT_TRUE(line->hard_return) << "Line should have hard return set";
    EXPECT_GT(line->styleList.size(), 0) << "Line should have at least one style";
}

// Test 2: ANSI colored text
TEST_F(PipelineTest, ANSIColoredText)
{
    // Process "\x1b[31mRed\x1b[0m\n" (red "Red", then reset)
    const unsigned char ansiRed[] = {0x1B, '[',  '3', '1', 'm', 'R',  'e',
                                     'd',  0x1B, '[', '0', 'm', '\n', 0};
    processBytes(ansiRed);

    // Verify line was added
    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    QString text = getLineText(0);

    // Verify text (ANSI codes should be processed, not included in text)
    EXPECT_EQ(text, "Red") << "Line text should be 'Red' (ANSI codes stripped)";
    EXPECT_GT(line->styleList.size(), 0) << "Line should have style information";

    // Note: Full ANSI parsing is tested elsewhere, we're just verifying the pipeline works
}

// Test 3: Multiple lines
TEST_F(PipelineTest, MultipleLines)
{
    // Process "Line1\nLine2\nLine3\n"
    processString("Line1\nLine2\nLine3\n");

    // Verify all lines were added
    ASSERT_EQ(doc->m_lineList.count(), 3) << "Expected 3 lines in buffer";

    // Verify each line's content
    EXPECT_EQ(getLineText(0), "Line1") << "First line should be 'Line1'";
    EXPECT_EQ(getLineText(1), "Line2") << "Second line should be 'Line2'";
    EXPECT_EQ(getLineText(2), "Line3") << "Third line should be 'Line3'";

    // Verify all have hard returns
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(doc->m_lineList[i]->hard_return) << "Line " << i << " should have hard return";
    }
}

// Test 4: UTF-8 text (multibyte sequences)
TEST_F(PipelineTest, UTF8Text)
{
    // Enable UTF-8 mode
    doc->m_bUTF_8 = true;

    // Process "Café\n" = 0x43 0x61 0x66 0xC3 0xA9 0x0A
    const unsigned char utf8Cafe[] = {0x43, 0x61, 0x66, 0xC3, 0xA9, 0x0A, 0};
    processBytes(utf8Cafe);

    // Verify line was added
    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    QString text = getLineText(0);

    // Verify UTF-8 was handled correctly
    EXPECT_EQ(text, "Café") << "UTF-8 text should be decoded correctly";
}

// Test 5: Empty line (just newline)
TEST_F(PipelineTest, EmptyLine)
{
    processString("\n");

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line for empty input with newline";

    Line* line = doc->m_lineList[0];
    EXPECT_EQ(line->len(), 0) << "Empty line should have length 0";
    EXPECT_TRUE(line->hard_return) << "Empty line should still have hard return";
}

// Test 6: Line without newline (incomplete line)
TEST_F(PipelineTest, IncompleteLineStaysInBuffer)
{
    processString("Hello"); // No newline

    // Line should still be in current line buffer, not in line list
    EXPECT_EQ(doc->m_lineList.count(), 0) << "Incomplete line should not be added to line list yet";

    // Now send newline to complete it
    processString("\n");

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Line should be completed after newline";
    EXPECT_EQ(getLineText(0), "Hello") << "Completed line should have correct text";
}

// Test 7: Mixed ASCII and ANSI
TEST_F(PipelineTest, MixedASCIIAndANSI)
{
    // Process "Normal \x1b[1mBold\x1b[0m Text\n"
    const unsigned char mixed[] = {'N', 'o', 'r', 'm', 'a',  'l',  ' ', 0x1B,
                                   '[', '1', 'm',                            // Bold on
                                   'B', 'o', 'l', 'd', 0x1B, '[',  '0', 'm', // Reset
                                   ' ', 'T', 'e', 'x', 't',  '\n', 0};
    processBytes(mixed);

    ASSERT_EQ(doc->m_lineList.count(), 1);
    EXPECT_EQ(getLineText(0), "Normal Bold Text") << "Mixed text should be processed correctly";
}

// Test 8: 256-color ANSI sequences (foreground)
TEST_F(PipelineTest, ANSI256ColorForeground)
{
    // Process "\x1b[38;5;196mRed256\n" (xterm color 196 = bright red)
    const unsigned char ansi256[] = {0x1B, '[', '3', '8', ';', '5', ';', '1',  '9', '6',
                                     'm',  'R', 'e', 'd', '2', '5', '6', '\n', 0};
    processBytes(ansi256);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    QString text = getLineText(0);

    EXPECT_EQ(text, "Red256") << "Line text should be 'Red256'";
    EXPECT_GT(line->styleList.size(), 0) << "Line should have style information";

    // Find the style that actually has text (iLength > 0)
    const Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }
    ASSERT_NE(style, nullptr) << "Should have at least one style with length > 0";

    // Verify the style has COLOUR_RGB flag set
    EXPECT_EQ((style->iFlags & COLOURTYPE), COLOUR_RGB) << "256-color should use COLOUR_RGB mode";

    // Verify foreground color is from xterm palette (color 196 = 0xFF0000)
    EXPECT_EQ(style->iForeColour, qRgb(0xFF, 0x00, 0x00))
        << "xterm color 196 should be bright red (0xFF0000)";
}

// Test 9: 256-color ANSI sequences (background)
TEST_F(PipelineTest, ANSI256ColorBackground)
{
    // Process "\x1b[48;5;21mBlue BG\n" (xterm color 21 = dark blue)
    const unsigned char ansi256bg[] = {0x1B, '[', '4', '8', ';', '5', ';', '2',  '1', 'm',
                                       'B',  'l', 'u', 'e', ' ', 'B', 'G', '\n', 0};
    processBytes(ansi256bg);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";
    Line* line = doc->m_lineList[0];

    EXPECT_EQ(getLineText(0), "Blue BG") << "Line text should be 'Blue BG'";

    // Find the style that actually has text (iLength > 0)
    const Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }
    ASSERT_NE(style, nullptr) << "Should have at least one style with length > 0";

    EXPECT_EQ((style->iFlags & COLOURTYPE), COLOUR_RGB) << "256-color should use COLOUR_RGB mode";

    // xterm color 21 = 0x0000FF (pure blue)
    EXPECT_EQ(style->iBackColour, qRgb(0x00, 0x00, 0xFF))
        << "xterm color 21 should be blue (0x0000FF)";
}

// Test 10: TrueColor/24-bit RGB ANSI sequences (foreground)
TEST_F(PipelineTest, TrueColorForeground)
{
    // Process "\x1b[38;2;255;128;0mOrange\n" (RGB: 255, 128, 0)
    const unsigned char trueColor[] = {0x1B, '[', '3', '8', ';', '2', ';', // ESC[38;2;
                                       '2',  '5', '5', ';',                // R=255
                                       '1',  '2', '8', ';',                // G=128
                                       '0',  'm',                          // B=0
                                       'O',  'r', 'a', 'n', 'g', 'e', '\n', 0};
    processBytes(trueColor);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    QString text = getLineText(0);

    EXPECT_EQ(text, "Orange") << "Line text should be 'Orange'";
    EXPECT_GT(line->styleList.size(), 0) << "Line should have style information";

    // Find the style that actually has text (iLength > 0)
    const Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }
    ASSERT_NE(style, nullptr) << "Should have at least one style with length > 0";

    EXPECT_EQ((style->iFlags & COLOURTYPE), COLOUR_RGB) << "TrueColor should use COLOUR_RGB mode";

    EXPECT_EQ(style->iForeColour, qRgb(255, 128, 0))
        << "TrueColor foreground should be RGB(255, 128, 0)";
}

// Test 11: TrueColor/24-bit RGB ANSI sequences (background)
TEST_F(PipelineTest, TrueColorBackground)
{
    // Process "\x1b[48;2;64;0;128mPurple BG\n" (RGB: 64, 0, 128)
    const unsigned char trueColorBg[] = {0x1B, '[', '4', '8', ';', '2', ';', // ESC[48;2;
                                         '6',  '4', ';',                     // R=64
                                         '0',  ';',                          // G=0
                                         '1',  '2', '8', 'm',                // B=128
                                         'P',  'u', 'r', 'p', 'l', 'e', ' ', 'B', 'G', '\n', 0};
    processBytes(trueColorBg);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    EXPECT_EQ(getLineText(0), "Purple BG") << "Line text should be 'Purple BG'";

    // Find the style that actually has text (iLength > 0)
    const Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }
    ASSERT_NE(style, nullptr) << "Should have at least one style with length > 0";

    EXPECT_EQ((style->iFlags & COLOURTYPE), COLOUR_RGB) << "TrueColor should use COLOUR_RGB mode";

    EXPECT_EQ(style->iBackColour, qRgb(64, 0, 128))
        << "TrueColor background should be RGB(64, 0, 128)";
}

// Test 12: TrueColor with both foreground and background
TEST_F(PipelineTest, TrueColorBothForeAndBack)
{
    // Process "\x1b[38;2;255;255;0m\x1b[48;2;0;0;128mYellow on Navy\n"
    const unsigned char trueColorBoth[] = {0x1B, '[', '3', '8', ';', '2', ';', // ESC[38;2;
                                           '2',  '5', '5', ';',                // R=255
                                           '2',  '5', '5', ';',                // G=255
                                           '0',  'm',                          // B=0
                                           0x1B, '[', '4', '8', ';', '2', ';', // ESC[48;2;
                                           '0',  ';',                          // R=0
                                           '0',  ';',                          // G=0
                                           '1',  '2', '8', 'm',                // B=128
                                           'Y',  'e', 'l', 'l', 'o', 'w', ' ',  'o',
                                           'n',  ' ', 'N', 'a', 'v', 'y', '\n', 0};
    processBytes(trueColorBoth);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    EXPECT_EQ(getLineText(0), "Yellow on Navy") << "Line text should be 'Yellow on Navy'";

    // Find the style that actually has text (iLength > 0)
    const Style* style = nullptr;
    for (const auto& s : line->styleList) {
        if (s->iLength > 0) {
            style = s.get();
            break;
        }
    }
    ASSERT_NE(style, nullptr) << "Should have at least one style with length > 0";

    EXPECT_EQ((style->iFlags & COLOURTYPE), COLOUR_RGB) << "Should use COLOUR_RGB mode";

    EXPECT_EQ(style->iForeColour, qRgb(255, 255, 0))
        << "Foreground should be yellow RGB(255, 255, 0)";
    EXPECT_EQ(style->iBackColour, qRgb(0, 0, 128)) << "Background should be navy RGB(0, 0, 128)";
}

// Test 13: TrueColor gradient (multiple colors on same line)
TEST_F(PipelineTest, TrueColorGradient)
{
    // Test multiple TrueColor sequences in one line
    // "\x1b[38;2;255;0;0mR\x1b[38;2;0;255;0mG\x1b[38;2;0;0;255mB\n"
    const unsigned char gradient[] = {
        0x1B, '[', '3', '8', ';', '2', ';', '2', '5', '5', ';', '0', ';', '0', 'm', 'R', // Red R
        0x1B, '[', '3', '8', ';', '2', ';', '0', ';', '2', '5', '5', ';', '0', 'm', 'G', // Green G
        0x1B, '[', '3', '8', ';', '2', ';', '0', ';', '0', ';', '2', '5', '5', 'm', 'B', // Blue B
        '\n', 0};
    processBytes(gradient);

    ASSERT_EQ(doc->m_lineList.count(), 1) << "Expected 1 line in buffer";

    Line* line = doc->m_lineList[0];
    EXPECT_EQ(getLineText(0), "RGB") << "Line text should be 'RGB'";

    // Should have multiple styles (one for each color change)
    EXPECT_GE(line->styleList.size(), 3) << "Should have at least 3 styles for color changes";
}

// Main function required for GoogleTest
// Note: QCoreApplication must be created before any Qt objects
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects like WorldDocument)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
