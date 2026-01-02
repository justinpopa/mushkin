/**
 * test_sendto_integration_gtest.cpp
 *
 * Tests for SendTo destination routing integrations:
 * - eSendToVariable: Set variables
 * - eSendToLogFile: Write to log file
 * - eSendToExecute: Re-execute commands with alias expansion
 */

#include "test_qt_static.h"
#include "../src/automation/alias.h"
#include "../src/automation/sendto.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>

class SendToIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        doc->setWorldName("TestWorld");
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc;
};

/**
 * Test eSendToVariable - Set a variable via SendTo
 */
TEST_F(SendToIntegrationTest, SendToVariableSetsVariable)
{
    QString output;

    // Send to variable
    doc->sendTo(eSendToVariable,
                "test_value", // value to set
                false,        // bOmitFromOutput
                false,        // bOmitFromLog
                "",           // strDescription
                "test_var",   // variable name
                output);

    // Verify variable was set
    QString value = doc->getVariable("test_var");
    EXPECT_EQ(value, "test_value");
}

/**
 * Test eSendToVariable with empty variable name (should not crash)
 */
TEST_F(SendToIntegrationTest, SendToVariableWithEmptyNameDoesNothing)
{
    QString output;

    // Send to variable with empty name
    doc->sendTo(eSendToVariable, "test_value", false, false, "",
                "", // empty variable name
                output);

    // Should not crash, just do nothing
    SUCCEED();
}

/**
 * Test eSendToExecute - Re-parse command through alias system
 */
TEST_F(SendToIntegrationTest, SendToExecuteTriggersAliases)
{
    // Create an alias that sets a variable via contents (not script)
    auto alias = std::make_unique<Alias>();
    alias->strLabel = "test_alias";
    alias->name = "testalias";
    alias->contents = "dummy";              // Send something to world
    alias->iSendTo = eSendToVariable;       // Send to variable instead
    alias->strVariable = "alias_triggered"; // Variable name
    alias->bEnabled = true;
    alias->iSequence = 100;

    doc->addAlias("test_alias", std::move(alias));

    QString output;

    // Execute command via SendTo - should expand to setting the variable
    doc->sendTo(eSendToExecute,
                "testalias", // This should match the alias
                false, false, "", "", output);

    // Verify alias was triggered and set the variable
    QString value = doc->getVariable("alias_triggered");
    EXPECT_EQ(value, "dummy");
}

/**
 * Test eSendToLogFile - Write to log file
 *
 * This test verifies that SendTo can write to the log file.
 * Since WriteToLog checks if logging is enabled internally,
 * we just verify it doesn't crash when called.
 */
TEST_F(SendToIntegrationTest, SendToLogFileDoesNotCrash)
{
    QString output;

    // Send to log file (logging not enabled, but should not crash)
    doc->sendTo(eSendToLogFile, "test log entry", false, false, "", "", output);

    // Should complete without crashing
    SUCCEED();
}

/**
 * Test eSendToLogFile with logging enabled
 */
TEST_F(SendToIntegrationTest, SendToLogFileWritesToLog)
{
    // Create temporary directory for log file
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QString logPath = tempDir.path() + "/test.log";

    // Enable logging
    doc->OpenLog(logPath, false);

    QString output;

    // Write to log via SendTo
    doc->sendTo(eSendToLogFile, "Test log entry from SendTo", false, false, "", "", output);

    // Close log to flush
    doc->CloseLog();

    // Verify log file was created and contains our text
    QFile logFile(logPath);
    ASSERT_TRUE(logFile.exists());
    ASSERT_TRUE(logFile.open(QIODevice::ReadOnly));

    QString logContents = QString::fromUtf8(logFile.readAll());
    EXPECT_TRUE(logContents.contains("Test log entry from SendTo"));

    logFile.close();
}

/**
 * Test eSendToOutput - Append to output string
 *
 * This was already implemented, but verify it still works
 */
TEST_F(SendToIntegrationTest, SendToOutputAppendsToString)
{
    QString output;

    doc->sendTo(eSendToOutput, "First line", false, false, "", "", output);

    doc->sendTo(eSendToOutput, "Second line", false, false, "", "", output);

    EXPECT_TRUE(output.contains("First line"));
    EXPECT_TRUE(output.contains("Second line"));
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
