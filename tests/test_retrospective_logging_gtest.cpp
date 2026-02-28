/**
 * test_retrospective_logging_gtest.cpp - GoogleTest version of Retrospective Logging Test
 *
 * Tests that opening a log file mid-session writes all buffered lines
 * with LOG_LINE flag, and that raw mode skips formatting.
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>

// Test fixture for retrospective logging tests
// Provides common setup/teardown and helper methods
class RetrospectiveLoggingTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = std::make_unique<WorldDocument>();
    }

    void TearDown() override
    {
        // Clean up any test log files
        for (const QString& file : tempLogFiles) {
            QFile::remove(file);
        }
        tempLogFiles.clear();
    }

    // Helper method to read file contents
    QString readFile(const QString& filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        return stream.readAll();
    }

    // Helper method to get a temp log file path and register it for cleanup
    QString getTempLogFile(const QString& name)
    {
        QString path = QDir::temp().filePath(name);
        QFile::remove(path); // Ensure clean state
        tempLogFiles.append(path);
        return path;
    }

    // Helper method to create a line with text
    std::unique_ptr<Line> createLine(int lineNumber, const char* text, int flags,
                                     QDateTime time = QDateTime::currentDateTime())
    {
        auto line = std::make_unique<Line>(lineNumber, 80, flags & (COMMENT | USER_INPUT),
                                           qRgb(255, 255, 255), qRgb(0, 0, 0), false);
        int len = static_cast<int>(strlen(text));
        line->textBuffer.resize(len);
        memcpy(line->textBuffer.data(), text, len);
        line->textBuffer.push_back('\0');
        line->flags = flags;
        line->hard_return = true;
        line->m_theTime = time;
        return line;
    }

    std::unique_ptr<WorldDocument> doc;
    QStringList tempLogFiles;
};

// Test 1: Retrospective Logging - Write Buffered Lines
// Creates buffered lines with LOG_LINE flag, then opens log.
// Verifies that all flagged lines are written to the log.
TEST_F(RetrospectiveLoggingTest, WriteBufferedLines)
{
    doc->m_logging.log_html = false;
    doc->m_logging.log_raw = false;
    doc->m_logging.log_output = true;
    doc->m_logging.log_notes = true;
    doc->m_logging.log_input = true;
    doc->m_logging.line_preamble_output = "[OUT] ";
    doc->m_logging.line_preamble_notes = "[NOTE] ";
    doc->m_logging.line_preamble_input = "[IN] ";
    doc->m_logging.line_postamble_output = "";
    doc->m_logging.line_postamble_notes = "";
    doc->m_logging.line_postamble_input = "";

    // Create 3 buffered lines with LOG_LINE flag set
    doc->m_lineList.push_back(createLine(1, "First line from MUD", LOG_LINE));
    doc->m_lineList.push_back(createLine(2, "A note from script", COMMENT | LOG_LINE));
    doc->m_lineList.push_back(createLine(3, "look", USER_INPUT | LOG_LINE));

    // Create one line WITHOUT LOG_LINE flag (should not be logged)
    doc->m_lineList.push_back(createLine(4, "Password line omitted by trigger", 0));

    // Now open log - should trigger retrospective logging
    QString logFile = getTempLogFile("test_retrospective.log");

    qint32 result = doc->OpenLog(logFile, false);
    EXPECT_EQ(result, 0) << "OpenLog should succeed";

    doc->CloseLog();

    // Read log file and verify contents
    QString content = readFile(logFile);

    EXPECT_TRUE(content.contains("[OUT] First line from MUD")) << "MUD output should be logged";
    EXPECT_TRUE(content.contains("[NOTE] A note from script")) << "Note should be logged";
    EXPECT_TRUE(content.contains("[IN] look")) << "User input should be logged";
    EXPECT_FALSE(content.contains("Password line omitted"))
        << "Line without LOG_LINE should NOT be logged";

    // Verify line count
    int lineCount = content.count('\n');
    EXPECT_GE(lineCount, 3) << "Log should have at least 3 lines";
    EXPECT_LE(lineCount, 4)
        << "Log should have at most 4 lines (3 logged lines + maybe extra newline)";

    // Note: Don't manually delete lines - WorldDocument destructor will handle them
    // since they were added to m_lineList
}

// Test 2: Raw Logging Mode
// Verifies that raw mode skips preambles, postambles, and HTML escaping.
TEST_F(RetrospectiveLoggingTest, RawLoggingMode)
{
    doc->m_logging.log_html = false;
    doc->m_logging.log_raw = true; // Raw mode enabled
    doc->m_logging.log_output = true;
    doc->m_logging.line_preamble_output = "[OUT] ";       // Should be ignored
    doc->m_logging.line_postamble_output = " [END]";      // Should be ignored
    doc->m_logging.file_preamble = "==== Log Start ===="; // Should be ignored
    doc->m_logging.file_postamble = "==== Log End ====";  // Should be ignored

    QString logFile = getTempLogFile("test_raw_logging.log");

    qint32 result = doc->OpenLog(logFile, false);
    EXPECT_EQ(result, 0) << "OpenLog should succeed";

    // Log a line with special characters
    auto lineOwner = createLine(1, "<html> & \"special\" chars", 0);
    Line* line = lineOwner.get();

    doc->m_currentLine = std::move(lineOwner);
    doc->logCompletedLine(line);

    doc->CloseLog();

    // Read log file
    QString content = readFile(logFile);

    // In raw mode, should have exact text without preambles/postambles
    EXPECT_TRUE(content.contains("<html> & \"special\" chars")) << "Raw text should be preserved";
    EXPECT_FALSE(content.contains("[OUT]")) << "No preamble in raw mode";
    EXPECT_FALSE(content.contains("[END]")) << "No postamble in raw mode";
    EXPECT_FALSE(content.contains("==== Log Start ====")) << "No file preamble in raw mode";
    EXPECT_FALSE(content.contains("==== Log End ====")) << "No file postamble in raw mode";
    EXPECT_FALSE(content.contains("&lt;")) << "No HTML escaping in raw mode";

    // Release m_currentLine (unique_ptr reset handles cleanup)
    doc->m_currentLine.reset();
}

// Test 3: Retrospective Logging with HTML Mode
// Verifies that retrospective logging respects HTML mode.
TEST_F(RetrospectiveLoggingTest, RetrospectiveLoggingWithHTML)
{
    doc->m_logging.log_html = true;
    doc->m_logging.log_in_colour = false; // HTML without colors
    doc->m_logging.log_raw = false;
    doc->m_logging.log_output = true;
    doc->m_logging.line_preamble_output = "";
    doc->m_logging.line_postamble_output = "";

    // Create line with HTML special characters
    doc->m_lineList.push_back(createLine(1, "<script>alert('XSS')</script>", LOG_LINE));

    QString logFile = getTempLogFile("test_retrospective_html.log");

    doc->OpenLog(logFile, false);
    doc->CloseLog();

    QString content = readFile(logFile);

    // Should have HTML-escaped content
    EXPECT_TRUE(content.contains("&lt;script&gt;"))
        << "HTML should be escaped in retrospective log";
    EXPECT_FALSE(content.contains("<script>")) << "No raw script tag should be present";

    // Note: Don't delete line - WorldDocument destructor handles it
}

// Test 4: Empty Buffer Retrospective Logging
// Verifies that opening log with empty buffer doesn't crash.
TEST_F(RetrospectiveLoggingTest, EmptyBufferRetrospectiveLogging)
{
    doc->m_logging.log_html = false;
    doc->m_logging.log_raw = false;
    doc->m_logging.log_output = true;

    // No lines in buffer
    EXPECT_TRUE(doc->m_lineList.empty()) << "Buffer should be empty";

    QString logFile = getTempLogFile("test_empty_buffer.log");

    qint32 result = doc->OpenLog(logFile, false);
    EXPECT_EQ(result, 0) << "OpenLog should succeed with empty buffer";

    doc->CloseLog();

    QString content = readFile(logFile);

    // Should only have file preamble/postamble (if any)
    EXPECT_TRUE(content.isEmpty() || content.trimmed().isEmpty())
        << "Log should be empty or nearly empty";
}

// Test 5: Mixed Line Types Retrospective Logging
// Tests that different line types (output, notes, input) use correct preambles.
TEST_F(RetrospectiveLoggingTest, MixedLineTypesRetrospectiveLogging)
{
    doc->m_logging.log_html = false;
    doc->m_logging.log_raw = false;
    doc->m_logging.log_output = true;
    doc->m_logging.log_notes = true;
    doc->m_logging.log_input = true;
    doc->m_logging.line_preamble_output = "[OUT] ";
    doc->m_logging.line_preamble_notes = "[NOTE] ";
    doc->m_logging.line_preamble_input = "[CMD] ";
    doc->m_logging.line_postamble_output = "";
    doc->m_logging.line_postamble_notes = "";
    doc->m_logging.line_postamble_input = "";

    // Output line, Note line, Input line
    doc->m_lineList.push_back(createLine(1, "MUD says hello", LOG_LINE));
    doc->m_lineList.push_back(createLine(2, "Script note", COMMENT | LOG_LINE));
    doc->m_lineList.push_back(createLine(3, "kill orc", USER_INPUT | LOG_LINE));

    QString logFile = getTempLogFile("test_mixed_types.log");

    doc->OpenLog(logFile, false);
    doc->CloseLog();

    QString content = readFile(logFile);

    // Verify each line type has correct preamble
    EXPECT_TRUE(content.contains("[OUT] MUD says hello"))
        << "Output line should have [OUT] preamble";
    EXPECT_TRUE(content.contains("[NOTE] Script note")) << "Note line should have [NOTE] preamble";
    EXPECT_TRUE(content.contains("[CMD] kill orc")) << "Input line should have [CMD] preamble";

    // Verify order
    int outPos = content.indexOf("[OUT]");
    int notePos = content.indexOf("[NOTE]");
    int cmdPos = content.indexOf("[CMD]");

    EXPECT_LT(outPos, notePos) << "Output line should come before note line";
    EXPECT_LT(notePos, cmdPos) << "Note line should come before input line";

    // Note: Don't delete lines - WorldDocument destructor handles them
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
