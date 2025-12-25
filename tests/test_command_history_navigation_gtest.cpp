/**
 * test_command_history_navigation_gtest.cpp - GoogleTest version
 *
 * Command History Navigation - Test Suite
 *
 * Tests the enhanced command history features:
 * - Consecutive duplicate filtering (m_last_command)
 * - History size limit (m_nHistoryLines)
 * - History status tracking (eAtTop, eInMiddle, eAtBottom)
 * - clearCommandHistory() method
 * - XML persistence
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/world/world_document.h"
#include "../src/world/xml_serialization.h"
#include <QCoreApplication>
#include <QFile>
#include <gtest/gtest.h>

// Test fixture for command history navigation tests
// Provides common setup/teardown and helper methods
class CommandHistoryNavigationTest : public ::testing::Test {
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
 * Test 1: Consecutive Duplicate Filtering
 *
 * Verifies that consecutive duplicates are skipped, but non-consecutive
 * duplicates are allowed.
 *
 * Based on sendvw.cpp:2411-2413: strCommand != m_last_command check
 */
TEST_F(CommandHistoryNavigationTest, ConsecutiveDuplicateFiltering)
{
    // Add first command
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_commandHistory.count(), 1);
    EXPECT_EQ(doc->m_commandHistory[0], "north");
    EXPECT_EQ(doc->m_last_command, "north");

    // Try to add same command (consecutive duplicate - should be skipped)
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_commandHistory.count(), 1) << "Still 1, consecutive duplicate not added";
    EXPECT_EQ(doc->m_last_command, "north");

    // Add different command
    doc->addToCommandHistory("south");
    EXPECT_EQ(doc->m_commandHistory.count(), 2);
    EXPECT_EQ(doc->m_commandHistory[1], "south");
    EXPECT_EQ(doc->m_last_command, "south");

    // Try consecutive duplicate again
    doc->addToCommandHistory("south");
    EXPECT_EQ(doc->m_commandHistory.count(), 2) << "Still 2, consecutive duplicate not added";

    // Add different command
    doc->addToCommandHistory("east");
    EXPECT_EQ(doc->m_commandHistory.count(), 3);

    // Now add "north" again (NOT consecutive - should be added)
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_commandHistory.count(), 4);
    EXPECT_EQ(doc->m_commandHistory[3], "north") << "Non-consecutive duplicate should be added";
}

/**
 * Test 2: History Size Limit (m_nHistoryLines)
 *
 * Verifies that history is trimmed when exceeding m_nHistoryLines.
 *
 * Based on sendvw.cpp:2415: if (m_inputcount >= pDoc->m_nHistoryLines)
 */
TEST_F(CommandHistoryNavigationTest, HistorySizeLimit)
{
    doc->m_nHistoryLines = 5; // Set small limit for testing

    // Add 5 commands (up to limit)
    doc->addToCommandHistory("cmd1");
    doc->addToCommandHistory("cmd2");
    doc->addToCommandHistory("cmd3");
    doc->addToCommandHistory("cmd4");
    doc->addToCommandHistory("cmd5");

    EXPECT_EQ(doc->m_commandHistory.count(), 5);
    EXPECT_EQ(doc->m_commandHistory[0], "cmd1");
    EXPECT_EQ(doc->m_commandHistory[4], "cmd5");

    // Add 6th command (should remove oldest)
    doc->addToCommandHistory("cmd6");
    EXPECT_EQ(doc->m_commandHistory.count(), 5) << "Still 5, size limit enforced";
    EXPECT_EQ(doc->m_commandHistory[0], "cmd2") << "cmd1 removed, oldest trimmed";
    EXPECT_EQ(doc->m_commandHistory[4], "cmd6") << "cmd6 added at end";

    // Add 7th command
    doc->addToCommandHistory("cmd7");
    EXPECT_EQ(doc->m_commandHistory.count(), 5);
    EXPECT_EQ(doc->m_commandHistory[0], "cmd3") << "cmd2 removed, oldest trimmed";
    EXPECT_EQ(doc->m_commandHistory[4], "cmd7");
}

/**
 * Test 3: History Status Tracking
 *
 * Verifies that m_iHistoryStatus is set correctly:
 * - eAtBottom when adding commands
 * - eAtBottom after clearing history
 *
 * Based on sendvw.cpp:2430: m_iHistoryStatus = eAtBottom
 */
TEST_F(CommandHistoryNavigationTest, HistoryStatusTracking)
{
    // Initial status should be eAtBottom
    EXPECT_EQ(doc->m_iHistoryStatus, eAtBottom);

    // Add command - status should be eAtBottom
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_iHistoryStatus, eAtBottom);
    EXPECT_EQ(doc->m_historyPosition, 1) << "At end after adding";

    // Add another - still eAtBottom
    doc->addToCommandHistory("south");
    EXPECT_EQ(doc->m_iHistoryStatus, eAtBottom);
    EXPECT_EQ(doc->m_historyPosition, 2);
}

/**
 * Test 4: Clear Command History
 *
 * Verifies that clearCommandHistory() resets all state.
 *
 * Based on sendvw.cpp:2152-2159
 */
TEST_F(CommandHistoryNavigationTest, ClearHistory)
{
    // Add some commands
    doc->addToCommandHistory("north");
    doc->addToCommandHistory("south");
    doc->addToCommandHistory("east");

    EXPECT_EQ(doc->m_commandHistory.count(), 3);
    EXPECT_EQ(doc->m_last_command, "east");
    EXPECT_EQ(doc->m_historyPosition, 3);

    // Clear history
    doc->clearCommandHistory();

    // Verify everything is reset
    EXPECT_TRUE(doc->m_commandHistory.isEmpty()) << "History should be empty";
    EXPECT_TRUE(doc->m_last_command.isEmpty()) << "Last command should be empty";
    EXPECT_EQ(doc->m_historyPosition, 0) << "Position should be 0";
    EXPECT_EQ(doc->m_iHistoryStatus, eAtBottom) << "Status should be eAtBottom";

    // Should be able to add commands again
    doc->addToCommandHistory("west");
    EXPECT_EQ(doc->m_commandHistory.count(), 1);
    EXPECT_EQ(doc->m_commandHistory[0], "west");
}

/**
 * Test 5: Empty Command Filtering
 *
 * Verifies that empty/whitespace-only commands are not added to history.
 *
 * Based on sendvw.cpp:2411: if (!strCommand.IsEmpty())
 */
TEST_F(CommandHistoryNavigationTest, EmptyCommandFiltering)
{
    // Try to add empty string
    doc->addToCommandHistory("");
    EXPECT_TRUE(doc->m_commandHistory.isEmpty()) << "Empty string should not be added";

    // Try to add whitespace only
    doc->addToCommandHistory("   ");
    EXPECT_TRUE(doc->m_commandHistory.isEmpty()) << "Whitespace-only should not be added";

    // Try tabs and newlines
    doc->addToCommandHistory("\t\n  ");
    EXPECT_TRUE(doc->m_commandHistory.isEmpty()) << "Tabs/newlines only should not be added";

    // Add valid command
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_commandHistory.count(), 1);
}

/**
 * Test 6: XML Serialization
 *
 * Verifies that command history is saved and loaded from XML.
 *
 * spec: "Persist history to world file (XML serialization)"
 */
TEST_F(CommandHistoryNavigationTest, XmlSerialization)
{
    QString testFile = "test_history.mcl";
    deleteFile(testFile);

    // Create document and add history
    {
        WorldDocument doc1;
        doc1.m_mush_name = "Test World";

        doc1.addToCommandHistory("north");
        doc1.addToCommandHistory("south");
        doc1.addToCommandHistory("look");
        doc1.addToCommandHistory("inventory");

        ASSERT_EQ(doc1.m_commandHistory.count(), 4);

        // Save to XML
        bool saved = XmlSerialization::SaveWorldXML(&doc1, testFile);
        ASSERT_TRUE(saved) << "Should save successfully";
        ASSERT_TRUE(QFile::exists(testFile)) << "File should exist";
    }

    // Load from XML into new document
    {
        WorldDocument doc2;

        bool loaded = XmlSerialization::LoadWorldXML(&doc2, testFile);
        ASSERT_TRUE(loaded) << "Should load successfully";

        // Verify history was loaded
        EXPECT_EQ(doc2.m_commandHistory.count(), 4);
        EXPECT_EQ(doc2.m_commandHistory[0], "north");
        EXPECT_EQ(doc2.m_commandHistory[1], "south");
        EXPECT_EQ(doc2.m_commandHistory[2], "look");
        EXPECT_EQ(doc2.m_commandHistory[3], "inventory");

        // Verify position and status reset
        EXPECT_EQ(doc2.m_historyPosition, 4) << "Position should be at end";
        EXPECT_EQ(doc2.m_iHistoryStatus, eAtBottom) << "Status should be eAtBottom";
    }

    deleteFile(testFile);
}

/**
 * Test 7: History Position After Adding
 *
 * Verifies that m_historyPosition is set to end after adding.
 *
 * Based on sendvw.cpp:2429: m_HistoryPosition = NULL (end of list)
 */
TEST_F(CommandHistoryNavigationTest, HistoryPositionAfterAdding)
{
    // Initially at position 0 (empty)
    EXPECT_EQ(doc->m_historyPosition, 0);

    // Add command - should be at position 1 (after first command)
    doc->addToCommandHistory("north");
    EXPECT_EQ(doc->m_historyPosition, 1);

    // Add another - should be at position 2
    doc->addToCommandHistory("south");
    EXPECT_EQ(doc->m_historyPosition, 2);

    // Manually set position to middle
    doc->m_historyPosition = 1;

    // Add new command - should reset to end
    doc->addToCommandHistory("east");
    EXPECT_EQ(doc->m_historyPosition, 3) << "Position should reset to end after adding";
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
