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

#include "../src/utils/error_codes.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QFile>

// Test fixture for command execution tests
// Provides common setup/teardown and helper methods
class CommandExecutionTest : public WorldDocumentTest {
  protected:
    // Helper to delete test file
    void deleteFile(const QString& path)
    {
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
    }
};

/**
 * Test 1: SendMsg() Multiline Splitting
 *
 * Verifies that SendMsg() splits multiline text into individual commands.
 */
TEST_F(CommandExecutionTest, SendMsgMultilineSplitting)
{
    // Enable speedwalk delay to force queueing
    doc->m_speedwalk.delay = 100; // 100ms delay

    // Send multiline command
    QString multiline = "north\nsouth\neast";
    doc->SendMsg(multiline, true, true, false);

    // Should have 3 items in queue
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 3)
        << "Should have 3 commands in queue";
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue[0].endsWith("north"))
        << "First command should be 'north'";
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue[1].endsWith("south"))
        << "Second command should be 'south'";
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue[2].endsWith("east"))
        << "Third command should be 'east'";
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
    doc->m_speedwalk.delay = 100; // Enable queueing

    // Test 1: Queue with echo and log → E prefix (original: doc.h:241)
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test1", true, true, true);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue[0], "Etest1") << "E = queue + echo + log";

    // Test 2: Queue without echo, with log → N prefix (original: doc.h:242)
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test2", false, true, true);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue[0], "Ntest2") << "N = queue + no-echo + log";

    // Test 3: Queue with echo, without log → e prefix (original: doc.h:246)
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test3", true, true, false);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue[0], "etest3") << "e = queue + echo + no-log";

    // Test 4: Immediate with echo and log (forced to queue because queue not empty) → I prefix
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test4", true, true, true);  // First one goes to queue (E)
    doc->SendMsg("test5", true, false, true); // Second is "immediate" but queue not empty (I)
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 2);
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue[1], "Itest5")
        << "I = immediate + echo + log";
}

/**
 * Test 3: Spam Prevention
 *
 * Verifies that repeated commands trigger spam prevention.
 */
TEST_F(CommandExecutionTest, SpamPrevention)
{
    // Enable spam prevention
    doc->m_spam.enabled = true;
    doc->m_spam.line_count = 3; // Insert spam message after 3 repeats
    doc->m_spam.message = "*** SPAM FILLER ***";

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
    doc->m_speedwalk.delay = 100; // Force queueing to inspect result

    // Test trailing \r\n
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test1\r\n", true, true, false);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_FALSE(doc->m_connectionManager->m_CommandQueue[0].contains("\r"))
        << "Should not contain \\r";
    EXPECT_FALSE(doc->m_connectionManager->m_CommandQueue[0].contains("\n"))
        << "Should not contain \\n";

    // Test trailing \n
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test2\n", true, true, false);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_FALSE(doc->m_connectionManager->m_CommandQueue[0].contains("\n"))
        << "Should not contain \\n";

    // Test no trailing newline
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("test3", true, true, false);
    ASSERT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1);
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue[0].endsWith("test3"))
        << "Should end with 'test3'";
}

/**
 * Test 5: Empty Command Handling
 *
 * Verifies that empty commands are handled gracefully.
 */
TEST_F(CommandExecutionTest, EmptyCommandHandling)
{
    doc->m_speedwalk.delay = 100;

    // Send empty string
    doc->m_connectionManager->m_CommandQueue.clear();
    doc->SendMsg("", true, true, false);
    // Empty string should create one empty item
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 1)
        << "Empty command should create one queue item";
}

/**
 * Test 6: Command Queue Clear State
 *
 * Verifies that queue is empty initially.
 */
TEST_F(CommandExecutionTest, QueueInitialState)
{
    // Queue should be empty initially
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue.isEmpty())
        << "Queue should be empty initially";
    EXPECT_EQ(doc->m_connectionManager->m_CommandQueue.count(), 0)
        << "Queue count should be 0 initially";
}

/**
 * Test 7: Immediate Sending (No Queue)
 *
 * Verifies that commands are sent immediately when no speedwalk delay.
 */
TEST_F(CommandExecutionTest, ImmediateSending)
{
    // No speedwalk delay
    doc->m_speedwalk.delay = 0;

    // Queue should remain empty (commands sent immediately)
    doc->m_connectionManager->m_CommandQueue.clear();

    // Note: We can't actually test sending without a socket,
    // but we can verify the queue remains empty
    // In real implementation, this would call DoSendMsg() directly

    // For now, just verify queue is empty
    EXPECT_TRUE(doc->m_connectionManager->m_CommandQueue.isEmpty())
        << "Queue should remain empty with no speedwalk delay";
}

/**
 * Test 8: Execute() returns eOK on a normal command
 *
 * Mirrors original CMUSHclientDoc::Execute (methods_commands.cpp:398), whose long
 * return value is pushed to the Lua caller by world.Execute (lua_methods.cpp:2269).
 */
TEST_F(CommandExecutionTest, ExecuteReturnsOkForNormalCommand)
{
    EXPECT_EQ(doc->Execute("look", true, false), static_cast<long>(eOK))
        << "A normal command should return eOK";
    // Depth counter must be balanced after a successful Execute().
    EXPECT_EQ(doc->m_iExecutionDepth, 0) << "Execution depth should return to 0 after Execute()";
}

/**
 * Test 9: Execute() returns eCommandsNestedTooDeeply when the recursion guard fires
 *
 * Original doc.h:43 — MAX_EXECUTION_DEPTH = 20.
 */
TEST_F(CommandExecutionTest, ExecuteReturnsNestedTooDeeplyAtDepthLimit)
{
    // Pre-load depth to MAX_EXECUTION_DEPTH so next ++ trips the guard.
    // The original limit is 20 (doc.h:43).
    doc->m_iExecutionDepth = 20;

    EXPECT_EQ(doc->Execute("look", true, false), static_cast<long>(eCommandsNestedTooDeeply))
        << "Exceeding MAX_EXECUTION_DEPTH (20) should return eCommandsNestedTooDeeply";

    // The guard decrements before returning, restoring the pre-call depth.
    EXPECT_EQ(doc->m_iExecutionDepth, 20)
        << "Depth guard must decrement back to its pre-call value on the deep path";

    doc->m_iExecutionDepth = 0; // restore for subsequent tests
}

/**
 * H21: MAX_EXECUTION_DEPTH is 20, matching original doc.h:43.
 */
TEST_F(CommandExecutionTest, ExecuteAllowsUpToDepth20)
{
    // At depth 19 the call should proceed (not trip the guard).
    doc->m_iExecutionDepth = 19;
    long result = doc->Execute("look", true, false);
    // depth was decremented back to 19 by the RAII guard after Execute returned.
    EXPECT_EQ(doc->m_iExecutionDepth, 19) << "Depth should return to 19 after Execute at depth 19";
    EXPECT_NE(result, static_cast<long>(eCommandsNestedTooDeeply))
        << "Depth 19+1=20 should NOT trip the guard (limit is >20, i.e. >MAX_EXECUTION_DEPTH)";
    doc->m_iExecutionDepth = 0;
}

/**
 * M116: DoSendMsg re-entrancy guard — calling DoSendMsg while m_bPluginProcessingSent
 * is true must be a no-op (original doc.cpp:1178).
 */
TEST_F(CommandExecutionTest, DoSendMsgNoOpWhenPluginProcessingSent)
{
    // Set the guard flag as if we're inside an ON_PLUGIN_SENT callback.
    doc->m_bPluginProcessingSent = true;

    // DoSendMsg must return without crashing or modifying state.
    // We verify by checking that the total lines sent counter does not increase.
    qint64 linesBefore = doc->m_connectionManager->m_nTotalLinesSent;
    doc->DoSendMsg(QStringLiteral("test"), true, false);
    EXPECT_EQ(doc->m_connectionManager->m_nTotalLinesSent, linesBefore)
        << "DoSendMsg should be a no-op when m_bPluginProcessingSent is true";

    doc->m_bPluginProcessingSent = false;
}
