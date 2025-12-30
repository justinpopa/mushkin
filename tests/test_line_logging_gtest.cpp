/**
 * test_line_logging_gtest.cpp - GoogleTest version of Line-Level Logging Integration Test
 *
 * Tests automatic line-level logging integrated into the line processing pipeline.
 * Verifies that our Qt implementation matches the original MUSHclient behavior.
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/color_utils.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <gtest/gtest.h>

// Test fixture for line logging tests
// Provides common setup/teardown and helper methods
class LineLoggingTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = new WorldDocument();

        // Initialize default logging settings
        doc->m_bLogOutput = false;
        doc->m_bLogNotes = false;
        doc->m_log_input = false;
        doc->m_bOmitCurrentLineFromLog = false;
    }

    void TearDown() override
    {
        // Clean up log files
        cleanupLogFile("test_output.log");
        cleanupLogFile("test_notes.log");
        cleanupLogFile("test_input.log");
        cleanupLogFile("test_omit.log");
        cleanupLogFile("test_selective.log");
        cleanupLogFile("test_preamble.log");
        cleanupLogFile("test_html_escape.log");
        cleanupLogFile("test_html_color.log");

        delete doc;
    }

    // Helper method to create a line with text
    Line* createLine(quint16 flags, const char* text)
    {
        Line* line = new Line(1, 80, flags, 0xFFFFFF, 0x000000, false);
        int len = strlen(text);
        line->textBuffer.resize(len);
        memcpy(line->text(), text, len);
        line->textBuffer.push_back('\0');
        line->hard_return = true;
        line->m_theTime = QDateTime::currentDateTime();

        return line;
    }

    // Helper method to add a style to a line
    void addStyle(Line* line, int length, quint16 flags = 0, QRgb foreColor = 0xFFFFFF,
                  QRgb backColor = 0x000000)
    {
        auto style = std::make_unique<Style>();
        style->iLength = length;
        style->iFlags = flags;
        style->iForeColour = foreColor;
        style->iBackColour = backColor;
        line->styleList.push_back(std::move(style));
    }

    // Helper method to read log file content
    QString readLogFile(const QString& filename)
    {
        QFile logFile(filename);
        if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }

        QTextStream in(&logFile);
        QString content = in.readAll();
        logFile.close();

        return content;
    }

    // Helper method to clean up log files
    void cleanupLogFile(const QString& filename)
    {
        if (QFile::exists(filename)) {
            QFile::remove(filename);
        }
    }

    WorldDocument* doc = nullptr;
};

/**
 * Test 1: Log normal MUD output line
 *
 * Expected behavior (from ProcessPreviousLine.cpp:306-351):
 * - If m_bLogOutput is true, log the line
 * - Set LOG_LINE flag for retrospective logging
 * - Write with m_strLogLinePreambleOutput and m_strLogLinePostambleOutput
 */
TEST_F(LineLoggingTest, LogOutputLine)
{
    doc->m_bLogOutput = true; // Enable output logging
    doc->m_bLogNotes = false;
    doc->m_log_input = false;
    doc->m_strLogLinePreambleOutput = "";
    doc->m_strLogLinePostambleOutput = "";

    // Open log file
    qint32 result = doc->OpenLog("test_output.log", false);
    ASSERT_EQ(result, 0) << "OpenLog should succeed";

    // Create a normal MUD output line (no COMMENT or USER_INPUT flags)
    Line* line = createLine(0, "You swing at the goblin!");
    addStyle(line, line->len());

    // Log the line
    doc->logCompletedLine(line);

    // Verify LOG_LINE flag was set
    EXPECT_TRUE((line->flags & LOG_LINE) != 0) << "LOG_LINE flag should be set on output line";

    // Close and check log file
    doc->CloseLog();

    // Read log file and verify content
    QString content = readLogFile("test_output.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";
    EXPECT_TRUE(content.contains("You swing at the goblin!")) << "Log should contain line text";

    delete line;
}

/**
 * Test 2: Log world.Note() line
 *
 * Expected behavior (from ProcessPreviousLine.cpp:253-300):
 * - If (flags & COMMENT) and m_bLogNotes, log the line
 * - Set LOG_LINE flag
 * - Write with m_strLogLinePreambleNotes and m_strLogLinePostambleNotes
 */
TEST_F(LineLoggingTest, LogNoteLine)
{
    doc->m_bLogOutput = false;
    doc->m_bLogNotes = true; // Enable notes logging
    doc->m_log_input = false;
    doc->m_strLogLinePreambleNotes = "[NOTE] ";
    doc->m_strLogLinePostambleNotes = "";

    doc->OpenLog("test_notes.log", false);

    // Create a note line (COMMENT flag)
    Line* line = createLine(COMMENT, "This is a note from script");
    addStyle(line, line->len());

    // Log the line
    doc->logCompletedLine(line);

    EXPECT_TRUE((line->flags & LOG_LINE) != 0) << "LOG_LINE flag should be set on note line";

    doc->CloseLog();

    // Verify log content
    QString content = readLogFile("test_notes.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";
    EXPECT_TRUE(content.contains("[NOTE]")) << "Log should contain note preamble";
    EXPECT_TRUE(content.contains("This is a note from script")) << "Log should contain note text";

    delete line;
}

/**
 * Test 3: Log user input line
 *
 * Expected behavior:
 * - If (flags & USER_INPUT) and m_log_input, log the line
 * - Set LOG_LINE flag
 * - Write with m_strLogLinePreambleInput and m_strLogLinePostambleInput
 */
TEST_F(LineLoggingTest, LogInputLine)
{
    doc->m_bLogOutput = false;
    doc->m_bLogNotes = false;
    doc->m_log_input = true; // Enable input logging
    doc->m_strLogLinePreambleInput = "> ";
    doc->m_strLogLinePostambleInput = "";

    doc->OpenLog("test_input.log", false);

    // Create a user input line (USER_INPUT flag)
    Line* line = createLine(USER_INPUT, "north");
    addStyle(line, line->len());

    doc->logCompletedLine(line);

    EXPECT_TRUE((line->flags & LOG_LINE) != 0) << "LOG_LINE flag should be set on input line";

    doc->CloseLog();

    QString content = readLogFile("test_input.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";
    EXPECT_TRUE(content.contains("> north")) << "Log should contain input with preamble";

    delete line;
}

/**
 * Test 4: Trigger omit_from_log flag
 *
 * Expected behavior (from ProcessPreviousLine.cpp:1071-1072):
 * - When trigger has omit_from_log, set bNoLog = true
 * - Line should NOT be logged
 * - LOG_LINE flag may still be set (for retrospective if triggers change)
 */
TEST_F(LineLoggingTest, TriggerOmitFromLog)
{
    doc->m_bLogOutput = true; // Logging enabled
    doc->m_strLogLinePreambleOutput = "";
    doc->m_strLogLinePostambleOutput = "";

    doc->OpenLog("test_omit.log", false);

    // Create a normal line
    Line* line = createLine(0, "Password: secret123");
    addStyle(line, line->len());

    // Simulate trigger setting omit flag
    doc->m_bOmitCurrentLineFromLog = true;

    // Log the line
    doc->logCompletedLine(line);

    // LOG_LINE flag should NOT be set when omitted
    EXPECT_FALSE((line->flags & LOG_LINE) != 0) << "LOG_LINE flag should NOT be set when omitted";

    doc->CloseLog();

    // Verify log file does NOT contain the password
    QString content = readLogFile("test_omit.log");
    EXPECT_FALSE(content.contains("secret123")) << "Log should NOT contain omitted line";

    delete line;
}

/**
 * Test 5: LOG_LINE flag for retrospective logging
 *
 * Expected behavior:
 * - Lines that should be logged get LOG_LINE flag set
 * - Lines that should not be logged do NOT get LOG_LINE flag
 * - This allows retrospective logging when log opens mid-session
 */
TEST_F(LineLoggingTest, LOG_LINEFlag)
{
    doc->m_bLogOutput = true;
    doc->m_bLogNotes = false;
    doc->m_log_input = false;

    // NO log file open - testing flag setting without writing

    // Test 1: Output line with logging enabled
    Line* line1 = createLine(0, "test");
    // line1->len is now a method - use textBuffer directly: 10;

    doc->logCompletedLine(line1);
    EXPECT_TRUE((line1->flags & LOG_LINE) != 0) << "LOG_LINE should be set when logging enabled";

    // Test 2: Output line with logging disabled
    doc->m_bLogOutput = false;
    Line* line2 = createLine(0, "test");
    // line2->len is now a method - use textBuffer directly: 10;

    doc->logCompletedLine(line2);
    EXPECT_FALSE((line2->flags & LOG_LINE) != 0)
        << "LOG_LINE should NOT be set when logging disabled";

    // Test 3: Note line with notes logging disabled
    Line* line3 = createLine(COMMENT, "test");
    // line3->len is now a method - use textBuffer directly: 10;

    doc->logCompletedLine(line3);
    EXPECT_FALSE((line3->flags & LOG_LINE) != 0)
        << "LOG_LINE should NOT be set for notes when disabled";

    delete line1;
    delete line2;
    delete line3;
}

/**
 * Test 6: Selective logging flags
 *
 * Expected behavior:
 * - m_bLogOutput controls MUD output logging
 * - m_bLogNotes controls world.Note() logging
 * - m_log_input controls user input logging
 * - Each can be enabled/disabled independently
 */
TEST_F(LineLoggingTest, SelectiveLogging)
{
    // Test: Only log notes, not output or input
    doc->m_bLogOutput = false;
    doc->m_bLogNotes = true;
    doc->m_log_input = false;
    doc->m_strLogLinePreambleNotes = "";
    doc->m_strLogLinePostambleNotes = "";

    doc->OpenLog("test_selective.log", false);

    // Output line - should NOT be logged
    Line* outputLine = createLine(0, "MUD output");
    addStyle(outputLine, outputLine->len());
    doc->logCompletedLine(outputLine);

    // Note line - SHOULD be logged
    Line* noteLine = createLine(COMMENT, "Script note");
    addStyle(noteLine, noteLine->len());
    doc->logCompletedLine(noteLine);

    doc->CloseLog();

    // Verify selective logging worked
    QString content = readLogFile("test_selective.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";
    EXPECT_FALSE(content.contains("MUD output")) << "Output should NOT be logged when disabled";
    EXPECT_TRUE(content.contains("Script note")) << "Note should be logged when enabled";

    delete outputLine;
    delete noteLine;
}

/**
 * Test 7: Preamble and postamble expansion
 *
 * Expected behavior:
 * - %n is replaced with newline
 * - Time codes like %Y, %m, %d are expanded using line's m_theTime
 * - Uses FormatTime() for expansion
 */
TEST_F(LineLoggingTest, PreamblePostamble)
{
    doc->m_bLogOutput = true;

    // Set preamble with time codes
    doc->m_strLogLinePreambleOutput = "[%H:%M:%S] ";
    doc->m_strLogLinePostambleOutput = "%n"; // Newline

    doc->OpenLog("test_preamble.log", false);

    Line* line = createLine(0, "Test message");
    addStyle(line, line->len());

    // Set specific time for testing
    line->m_theTime = QDateTime(QDate(2025, 10, 11), QTime(14, 30, 45));

    doc->logCompletedLine(line);
    doc->CloseLog();

    // Verify time formatting
    QString content = readLogFile("test_preamble.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";
    EXPECT_TRUE(content.contains("[14:30:45]")) << "Time codes should be expanded correctly";
    EXPECT_TRUE(content.contains("Test message")) << "Message content should be present";

    delete line;
}

/**
 * Test 8: HTML Escaping
 *
 * Expected behavior:
 * - FixHTMLString() escapes & → &amp;
 * - Escapes < → &lt;
 * - Escapes > → &gt;
 * - Escapes " → &quot;
 * - & must be escaped FIRST to avoid double-escaping
 */
TEST_F(LineLoggingTest, HTMLEscaping)
{
    doc->m_bLogOutput = true;
    doc->m_bLogHTML = true;      // Enable HTML mode
    doc->m_bLogInColour = false; // Without colors (uses FixHTMLString)
    doc->m_strLogLinePreambleOutput = "";
    doc->m_strLogLinePostambleOutput = "";

    doc->OpenLog("test_html_escape.log", false);

    // Create a line with HTML special characters
    Line* line = createLine(0, "<script>alert(\"XSS\")</script> & more");

    auto style = std::make_unique<Style>();
    style->iLength = line->len();
    style->iFlags = COLOUR_RGB; // RGB color mode
    style->iForeColour = 0xFFFFFF;
    style->iBackColour = 0x000000;
    line->styleList.push_back(std::move(style));

    doc->logCompletedLine(line);
    doc->CloseLog();

    // Verify HTML escaping
    QString content = readLogFile("test_html_escape.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";

    // Verify each escape
    EXPECT_TRUE(content.contains("&lt;script&gt;")) << "< and > should be escaped correctly";
    EXPECT_TRUE(content.contains("alert(&quot;XSS&quot;)")) << "Quotes should be escaped correctly";
    EXPECT_TRUE(content.contains("&amp; more")) << "Ampersand should be escaped correctly";

    // Verify no double-escaping (& was escaped first)
    EXPECT_FALSE(content.contains("&amp;lt;")) << "No double-escaping of <";
    EXPECT_FALSE(content.contains("&amp;gt;")) << "No double-escaping of >";

    // Verify actual script tags are NOT in output (would be dangerous!)
    EXPECT_FALSE(content.contains("<script>")) << "No actual <script> tag";
    EXPECT_FALSE(content.contains("</script>")) << "No actual </script> tag";

    delete line;
}

/**
 * Test 9: HTML Color Logging
 *
 * Expected behavior:
 * - LogLineInHTMLcolour() generates <font> tags for foreground colors
 * - Generates <span> tags for background colors (only if not black)
 * - Wraps underlined text in <u> tags
 * - Properly closes all tags at end
 * - Text content is still HTML-escaped
 */
TEST_F(LineLoggingTest, HTMLColorLogging)
{
    doc->m_bLogOutput = true;
    doc->m_bLogHTML = true;     // Enable HTML mode
    doc->m_bLogInColour = true; // WITH colors (uses LogLineInHTMLcolour)
    doc->m_strLogLinePreambleOutput = "";
    doc->m_strLogLinePostambleOutput = "";

    doc->OpenLog("test_html_color.log", false);

    // Create a line with multiple colored segments and special characters
    Line* line = createLine(0, "Red <text> on black, Yellow on Blue");

    // Style 1: "Red <text> on black," - red foreground, black background
    auto style1 = std::make_unique<Style>();
    style1->iLength = 20; // "Red <text> on black,"
    style1->iFlags = COLOUR_RGB;
    style1->iForeColour = BGR(255, 0, 0); // Red (stored as BGR/COLORREF)
    style1->iBackColour = BGR(0, 0, 0);   // Black
    line->styleList.push_back(std::move(style1));

    // Style 2: " Yellow on Blue" - yellow foreground, blue background, underlined
    auto style2 = std::make_unique<Style>();
    style2->iLength = 16; // " Yellow on Blue"
    style2->iFlags = COLOUR_RGB | UNDERLINE;
    style2->iForeColour = BGR(255, 255, 0); // Yellow (stored as BGR/COLORREF)
    style2->iBackColour = BGR(0, 0, 255);   // Blue (not black, so span needed)
    line->styleList.push_back(std::move(style2));

    doc->logCompletedLine(line);
    doc->CloseLog();

    // Verify HTML color formatting
    QString content = readLogFile("test_html_color.log");
    ASSERT_FALSE(content.isEmpty()) << "Should be able to read log file";

    // Test 1: Font tags for foreground colors
    bool hasRedFont = content.contains("<font color=\"#ff0000\">") ||
                      content.contains("<font color=\"#FF0000\">");
    EXPECT_TRUE(hasRedFont) << "Red font tag should be present";

    bool hasYellowFont = content.contains("<font color=\"#ffff00\">") ||
                         content.contains("<font color=\"#FFFF00\">");
    EXPECT_TRUE(hasYellowFont) << "Yellow font tag should be present";

    // Test 2: Span tags for non-black background
    bool hasBlueBackground =
        content.contains("background:#0000ff") || content.contains("background:#0000FF");
    EXPECT_TRUE(hasBlueBackground) << "Blue background in span tag should be present";

    // Test 3: Underline tags
    EXPECT_TRUE(content.contains("<u>")) << "Underline opening tag should be present";
    EXPECT_TRUE(content.contains("</u>")) << "Underline closing tag should be present";

    // Test 4: HTML escaping still works in color mode
    EXPECT_TRUE(content.contains("&lt;text&gt;"))
        << "HTML characters should be escaped in color mode";

    // Test 5: All tags are properly closed
    int fontOpen = content.count("<font");
    int fontClose = content.count("</font>");
    EXPECT_EQ(fontOpen, fontClose)
        << "Font tags should be balanced (open=" << fontOpen << ", close=" << fontClose << ")";

    int spanOpen = content.count("<span");
    int spanClose = content.count("</span>");
    EXPECT_EQ(spanOpen, spanClose)
        << "Span tags should be balanced (open=" << spanOpen << ", close=" << spanClose << ")";

    // Test 6: Content is preserved
    EXPECT_TRUE(content.contains("Red") && content.contains("Yellow"))
        << "Text content should be preserved";

    delete line;
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
