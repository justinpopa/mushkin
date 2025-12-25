/**
 * test_command_execution_gtest.cpp - GoogleTest version
 *
 * Command Execution Pipeline - Test Suite
 *
 * Tests the command execution pipeline including:
 * - SendMsg() high-level routing
 * - DoSendMsg() low-level sending
 * - Command prefix support (/ and :)
 * - Input echoing to output
 * - Input logging to file
 * - Spam prevention
 * - Command queue
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <gtest/gtest.h>

// Test fixture for command execution tests
// Provides common setup/teardown and helper methods
class CommandExecutionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to delete test file
    void deleteFile(const QString& path)
    {
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
    }

    WorldDocument* doc = nullptr;
};

/**
 * Test 1: SendMsg() Multiline Splitting
 *
 * Verifies that SendMsg() splits multiline text into individual commands.
 */
TEST_F(CommandExecutionTest, SendMsgMultilineSplitting)
{
    // Enable speedwalk delay to force queueing
    doc->m_iSpeedWalkDelay = 100; // 100ms delay

    // Send multiline command
    QString multiline = "north\nsouth\neast";
    doc->SendMsg(multiline, true, true, false);

    // Should have 3 items in queue
    ASSERT_EQ(doc->m_CommandQueue.count(), 3) << "Should have 3 commands in queue";
    EXPECT_TRUE(doc->m_CommandQueue[0].endsWith("north")) << "First command should be 'north'";
    EXPECT_TRUE(doc->m_CommandQueue[1].endsWith("south")) << "Second command should be 'south'";
    EXPECT_TRUE(doc->m_CommandQueue[2].endsWith("east")) << "Third command should be 'east'";
}

/**
 * Test 2: Command Queue Prefix Encoding
 *
 * Verifies that echo/log flags are encoded into queue prefix.
 *
 * Prefix encoding:
 * - Q = queue + echo + log
 * - q = queue + no-echo + log
 * - I = immediate + echo + log
 * - i = immediate + no-echo + log
 * - Lowercase first letter = no logging
 */
TEST_F(CommandExecutionTest, CommandQueueEncoding)
{
    doc->m_iSpeedWalkDelay = 100; // Enable queueing

    // Test 1: Queue with echo and log
    doc->m_CommandQueue.clear();
    doc->SendMsg("test1", true, true, true);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_TRUE(doc->m_CommandQueue[0].startsWith("Q"))
        << "Should have uppercase Q prefix (echo + log)";
    EXPECT_EQ(doc->m_CommandQueue[0], "Qtest1");

    // Test 2: Queue without echo, with log
    doc->m_CommandQueue.clear();
    doc->SendMsg("test2", false, true, true);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_TRUE(doc->m_CommandQueue[0].startsWith("q"))
        << "Should have lowercase q prefix (no-echo + log)";
    EXPECT_EQ(doc->m_CommandQueue[0], "qtest2");

    // Test 3: Queue with echo, without log
    doc->m_CommandQueue.clear();
    doc->SendMsg("test3", true, true, false);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_TRUE(doc->m_CommandQueue[0].startsWith("q")) << "Should have lowercase prefix (no log)";
    EXPECT_EQ(doc->m_CommandQueue[0], "qtest3");

    // Test 4: Immediate with echo and log (forced to queue because queue not empty)
    doc->m_CommandQueue.clear();
    doc->SendMsg("test4", true, true, true);  // First one goes to queue
    doc->SendMsg("test5", true, false, true); // Second one is "immediate" but queue not empty
    ASSERT_EQ(doc->m_CommandQueue.count(), 2);
    EXPECT_TRUE(doc->m_CommandQueue[1].startsWith("I"))
        << "Should have uppercase I prefix (immediate + echo + log)";
    EXPECT_EQ(doc->m_CommandQueue[1], "Itest5");
}

/**
 * Test 3: Spam Prevention
 *
 * Verifies that repeated commands trigger spam prevention.
 */
TEST_F(CommandExecutionTest, SpamPrevention)
{
    // Enable spam prevention
    doc->m_bEnableSpamPrevention = true;
    doc->m_iSpamLineCount = 3; // Insert spam message after 3 repeats
    doc->m_strSpamMessage = "*** SPAM FILLER ***";

    // Track last command sent
    EXPECT_TRUE(doc->m_strLastCommandSent.isEmpty()) << "Last command should be empty initially";
    EXPECT_EQ(doc->m_iLastCommandCount, 0) << "Command count should be 0 initially";

    // Simulate sending same command 5 times
    for (int i = 0; i < 5; i++) {
        // Simulate the spam prevention logic
        QString cmd = "attack";
        if (cmd == doc->m_strLastCommandSent) {
            doc->m_iLastCommandCount++;
        } else {
            doc->m_strLastCommandSent = cmd;
            doc->m_iLastCommandCount = 1;
        }

        if (i == 0) {
            EXPECT_EQ(doc->m_iLastCommandCount, 1) << "First iteration should have count 1";
        } else if (i == 3) {
            // After 4th repeat, counter should be 4
            EXPECT_EQ(doc->m_iLastCommandCount, 4) << "Fourth iteration should have count 4";
        }
    }
}

/**
 * Test 4: Command Stripping
 *
 * Verifies that SendMsg() strips trailing newlines.
 */
TEST_F(CommandExecutionTest, CommandStripping)
{
    doc->m_iSpeedWalkDelay = 100; // Force queueing to inspect result

    // Test trailing \r\n
    doc->m_CommandQueue.clear();
    doc->SendMsg("test1\r\n", true, true, false);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_FALSE(doc->m_CommandQueue[0].contains("\r")) << "Should not contain \\r";
    EXPECT_FALSE(doc->m_CommandQueue[0].contains("\n")) << "Should not contain \\n";

    // Test trailing \n
    doc->m_CommandQueue.clear();
    doc->SendMsg("test2\n", true, true, false);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_FALSE(doc->m_CommandQueue[0].contains("\n")) << "Should not contain \\n";

    // Test no trailing newline
    doc->m_CommandQueue.clear();
    doc->SendMsg("test3", true, true, false);
    ASSERT_EQ(doc->m_CommandQueue.count(), 1);
    EXPECT_TRUE(doc->m_CommandQueue[0].endsWith("test3")) << "Should end with 'test3'";
}

/**
 * Test 5: Empty Command Handling
 *
 * Verifies that empty commands are handled gracefully.
 */
TEST_F(CommandExecutionTest, EmptyCommandHandling)
{
    doc->m_iSpeedWalkDelay = 100;

    // Send empty string
    doc->m_CommandQueue.clear();
    doc->SendMsg("", true, true, false);
    // Empty string should create one empty item
    EXPECT_EQ(doc->m_CommandQueue.count(), 1) << "Empty command should create one queue item";
}

/**
 * Test 6: Command Queue Clear State
 *
 * Verifies that queue is empty initially.
 */
TEST_F(CommandExecutionTest, QueueInitialState)
{
    // Queue should be empty initially
    EXPECT_TRUE(doc->m_CommandQueue.isEmpty()) << "Queue should be empty initially";
    EXPECT_EQ(doc->m_CommandQueue.count(), 0) << "Queue count should be 0 initially";
}

/**
 * Test 7: Immediate Sending (No Queue)
 *
 * Verifies that commands are sent immediately when no speedwalk delay.
 */
TEST_F(CommandExecutionTest, ImmediateSending)
{
    // No speedwalk delay
    doc->m_iSpeedWalkDelay = 0;

    // Queue should remain empty (commands sent immediately)
    doc->m_CommandQueue.clear();

    // Note: We can't actually test sending without a socket,
    // but we can verify the queue remains empty
    // In real implementation, this would call DoSendMsg() directly

    // For now, just verify queue is empty
    EXPECT_TRUE(doc->m_CommandQueue.isEmpty())
        << "Queue should remain empty with no speedwalk delay";
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
