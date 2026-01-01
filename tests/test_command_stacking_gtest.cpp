/**
 * test_command_stacking_gtest.cpp - GoogleTest version
 *
 * Command Stacking - Test Suite
 *
 * Tests for Execute() function with command stacking enabled/disabled.
 *
 * Based on methods_commands.cpp:311-334 in original MUSHclient.
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "test_qt_static.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for command stacking tests
// Provides common setup/teardown and helper methods
class CommandStackingTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
};

/**
 * Test 1: Basic command stacking
 * Input: "north;south;east"
 * Expected: 3 commands sent (checked via command history)
 */
TEST_F(CommandStackingTest, BasicStacking)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false; // Disable aliases for simplicity

    doc->Execute("north;south;east");

    // Should have 3 commands in history
    ASSERT_EQ(doc->m_commandHistory.size(), 3) << "Should have 3 commands in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north") << "First command should be 'north'";
    EXPECT_EQ(doc->m_commandHistory[1], "south") << "Second command should be 'south'";
    EXPECT_EQ(doc->m_commandHistory[2], "east") << "Third command should be 'east'";
}

/**
 * Test 2: Escape sequence (double delimiter)
 * Input: "say Hello;;there"
 * Expected: 1 command "say Hello;there"
 */
TEST_F(CommandStackingTest, EscapeSequence)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("say Hello;;there");

    // Should have 1 command with semicolon preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 1) << "Should have 1 command in history";
    EXPECT_EQ(doc->m_commandHistory[0], "say Hello;there")
        << "Escape sequence should convert double semicolon to single";
}

/**
 * Test 3: Leading delimiter bypass
 * Input: ";north;south"
 * Expected: 1 command "north;south" (literal semicolon preserved)
 */
TEST_F(CommandStackingTest, LeadingDelimiterBypass)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute(";north;south");

    // Should have 1 command with semicolon preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 1) << "Should have 1 command in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north;south")
        << "Leading delimiter should bypass stacking";
}

/**
 * Test 4: Whitespace preservation
 * Input: "north ; south"
 * Expected: 2 commands "north " and " south" (spaces preserved)
 */
TEST_F(CommandStackingTest, WhitespacePreservation)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("north ; south");

    // Should have 2 commands with whitespace preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 2) << "Should have 2 commands in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north ") << "First command should preserve trailing space";
    EXPECT_EQ(doc->m_commandHistory[1], " south") << "Second command should preserve leading space";
}

/**
 * Test 5: Stacking disabled
 * Input: "north;south;east"
 * Expected: 1 command "north;south;east" (literal semicolons)
 */
TEST_F(CommandStackingTest, StackingDisabled)
{
    doc->m_enable_command_stack = false; // Disabled!
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("north;south;east");

    // Should have 1 command with semicolons preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 1) << "Should have 1 command in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north;south;east")
        << "When stacking disabled, semicolons should be preserved";
}

/**
 * Test 6: Empty commands between delimiters
 * Input: "north;;south" (with stacking disabled to test raw behavior)
 * Expected: 1 command "north;;south"
 */
TEST_F(CommandStackingTest, EmptyCommandsDisabled)
{
    doc->m_enable_command_stack = false;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("north;;south");

    // Should have 1 command with double semicolons preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 1) << "Should have 1 command in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north;;south")
        << "Double semicolons should be preserved when stacking disabled";
}

/**
 * Test 7: Empty command at end
 * Input: "north;south;"
 * Expected: 3 commands sent, but only 2 in history (empty commands not recorded)
 */
TEST_F(CommandStackingTest, EmptyCommandAtEnd)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("north;south;");

    // Should have 2 commands in history (empty command sent but not recorded)
    ASSERT_EQ(doc->m_commandHistory.size(), 2)
        << "Empty commands should be sent but not recorded in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north") << "First command should be 'north'";
    EXPECT_EQ(doc->m_commandHistory[1], "south") << "Second command should be 'south'";
}

/**
 * Test 8: Complex escape sequence
 * Input: "say ;;;test"
 * Expected: 2 commands "say ;" and "test"
 */
TEST_F(CommandStackingTest, ComplexEscape)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = ";";
    doc->m_enable_aliases = false;

    doc->Execute("say ;;;test");

    // Should have 2 commands: "say ;" and "test"
    ASSERT_EQ(doc->m_commandHistory.size(), 2) << "Should have 2 commands in history";
    EXPECT_EQ(doc->m_commandHistory[0], "say ;")
        << "First command should be 'say ;' (double semicolon escaped to single)";
    EXPECT_EQ(doc->m_commandHistory[1], "test") << "Second command should be 'test'";
}

/**
 * Test 9: Custom delimiter
 * Input: "north|south|east"
 * Expected: 3 commands with "|" as delimiter
 */
TEST_F(CommandStackingTest, CustomDelimiter)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = "|"; // Custom delimiter
    doc->m_enable_aliases = false;

    doc->Execute("north|south|east");

    // Should have 3 commands
    ASSERT_EQ(doc->m_commandHistory.size(), 3) << "Should have 3 commands in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north") << "First command should be 'north'";
    EXPECT_EQ(doc->m_commandHistory[1], "south") << "Second command should be 'south'";
    EXPECT_EQ(doc->m_commandHistory[2], "east") << "Third command should be 'east'";
}

/**
 * Test 10: Leading delimiter with custom delimiter
 * Input: "|north|south"
 * Expected: 1 command "north|south" (bypass stacking)
 */
TEST_F(CommandStackingTest, CustomDelimiterBypass)
{
    doc->m_enable_command_stack = true;
    doc->m_strCommandStackCharacter = "|";
    doc->m_enable_aliases = false;

    doc->Execute("|north|south");

    // Should have 1 command with delimiter preserved
    ASSERT_EQ(doc->m_commandHistory.size(), 1) << "Should have 1 command in history";
    EXPECT_EQ(doc->m_commandHistory[0], "north|south")
        << "Leading delimiter should bypass stacking with custom delimiter";
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
