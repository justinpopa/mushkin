/**
 * test_alias_integration_gtest.cpp - GoogleTest version of Alias Integration Tests
 *
 * Tests complete alias integration from WorldDocument to command processing:
 * 1. Alias evaluation intercepts commands
 * 2. Commands without aliases go to MUD
 * 3. Alias execution handles all actions correctly
 * 4. Command history respects omit_from_command_history
 */

#include "../src/automation/alias.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"

// Test fixture for alias integration tests
class AliasIntegrationTest : public WorldDocumentTest {
  protected:
    // Helper to add and enable an alias
    Alias* addAlias(const QString& label, const QString& pattern)
    {
        auto alias = std::make_unique<Alias>();
        alias->name = pattern;
        alias->enabled = true;
        alias->label = label;
        alias->send_to = eSendToWorld;
        alias->sequence = 100;

        Alias* aliasPtr = alias.get();
        auto addResult = doc->addAlias(label, std::move(alias));
        EXPECT_TRUE(addResult.has_value());

        return aliasPtr;
    }
};

// Test 1: Alias Intercepts Command (Prevents MUD Send)
TEST_F(AliasIntegrationTest, AliasInterceptsCommand)
{
    // Create a simple north alias
    Alias* a = addAlias("north_shortcut", "n");
    a->contents = "north";

    // Simulate user typing "n" (this is what WorldWidget::sendCommand() does)
    QString command = "n";

    // Call evaluateAliases (like WorldWidget::sendCommand() does)
    bool aliasHandled = doc->evaluateAliases(command);

    // Verify alias intercepted command
    EXPECT_TRUE(aliasHandled) << "Alias should intercept command 'n'";

    // Verify alias was executed
    EXPECT_EQ(a->matched, 1) << "Alias should be executed once";
}

// Test 2: Non-Matching Command Goes to MUD
TEST_F(AliasIntegrationTest, NonMatchingCommandGoesToMUD)
{
    // Create alias that won't match
    Alias* a = addAlias("north_alias", "north");
    a->contents = "walk north";

    // Try command that doesn't match
    QString nonMatchingCommand = "south";
    bool aliasHandled = doc->evaluateAliases(nonMatchingCommand);

    // Verify no alias matched
    EXPECT_FALSE(aliasHandled) << "No alias should match 'south'";

    // Verify alias was NOT executed
    EXPECT_EQ(a->matched, 0) << "Alias should not be executed";
}

// Test 3: Command History - omit_from_command_history = false (default)
TEST_F(AliasIntegrationTest, CommandHistoryIncludesCommand)
{
    // Alias with omit_from_command_history = false (default)
    Alias* a = addAlias("heal_alias", "heal");
    a->contents = "cast heal self";
    a->omit_from_command_history = false; // Should add to history

    // Clear command history
    doc->m_commandHistory.clear();

    // Execute alias
    doc->evaluateAliases("heal");

    // Check if added to history
    EXPECT_TRUE(doc->m_commandHistory.contains("heal"))
        << "Command should be added to history (omit_from_command_history = false)";
}

// Test 4: Command History - omit_from_command_history = true
TEST_F(AliasIntegrationTest, CommandHistoryOmitsCommand)
{
    // Alias with omit_from_command_history = true
    Alias* a = addAlias("secret_alias", "secret");
    a->contents = "say secret password";
    a->omit_from_command_history = true; // Should NOT add to history

    // Clear command history
    doc->m_commandHistory.clear();

    // Execute alias
    doc->evaluateAliases("secret");

    // Check if NOT in history
    EXPECT_FALSE(doc->m_commandHistory.contains("secret"))
        << "Command should not be added to history (omit_from_command_history = true)";
}

// Test 5: Wildcard Alias End-to-End
TEST_F(AliasIntegrationTest, WildcardAliasEndToEnd)
{
    // Create wildcard alias
    Alias* a = addAlias("tell_alias", "tell * *");
    a->contents = "say Telling %1: %2";

    // Execute wildcard alias
    bool handled = doc->evaluateAliases("tell Bob hello world");

    // Verify alias matched
    EXPECT_TRUE(handled) << "Wildcard alias should match 'tell Bob hello world'";

    // Verify wildcards captured
    ASSERT_GT(a->wildcards.size(), 2) << "Should have captured wildcards";
    EXPECT_EQ(a->wildcards[1], "Bob") << "wildcards[1] should be 'Bob'";
    EXPECT_EQ(a->wildcards[2], "hello world") << "wildcards[2] should be 'hello world'";
}

// Test 6: Multiple Aliases - Priority/Sequence
TEST_F(AliasIntegrationTest, AliasPriorityBySequence)
{
    // Higher priority (lower sequence number) should execute first
    Alias* a1 = addAlias("high_priority", "go *");
    a1->contents = "walk %1";
    a1->sequence = 50; // Higher priority
    a1->keep_evaluating = false;

    Alias* a2 = addAlias("low_priority", "go *");
    a2->contents = "run %1";
    a2->sequence = 150; // Lower priority

    // Execute
    doc->evaluateAliases("go north");

    // Higher priority should execute
    EXPECT_EQ(a1->matched, 1) << "Higher priority alias should execute";
    // Lower priority should not execute (keep_evaluating = false)
    EXPECT_EQ(a2->matched, 0) << "Lower priority alias should not execute";
}

// Test 7: Disabled Alias Does Not Execute
TEST_F(AliasIntegrationTest, DisabledAliasDoesNotExecute)
{
    // Create disabled alias
    Alias* a = addAlias("disabled_alias", "test");
    a->contents = "say testing";
    a->enabled = false; // Disabled

    // Execute
    bool handled = doc->evaluateAliases("test");

    // Verify alias did not handle command
    EXPECT_FALSE(handled) << "Disabled alias should not handle command";
    EXPECT_EQ(a->matched, 0) << "Disabled alias should not execute";
}

// Test 8: Exact Match Takes Precedence Over Wildcard
TEST_F(AliasIntegrationTest, ExactMatchPrecedence)
{
    // Wildcard pattern (lower priority)
    Alias* a1 = addAlias("wildcard_alias", "look *");
    a1->contents = "examine %1";
    a1->sequence = 100;
    a1->keep_evaluating = false;

    // Exact match (higher priority)
    Alias* a2 = addAlias("exact_alias", "look");
    a2->contents = "glance";
    a2->sequence = 50; // Higher priority

    // Execute exact match
    doc->evaluateAliases("look");

    // Exact match should execute
    EXPECT_EQ(a2->matched, 1) << "Exact match should execute";
    // Wildcard should not execute
    EXPECT_EQ(a1->matched, 0) << "Wildcard match should not execute";
}
