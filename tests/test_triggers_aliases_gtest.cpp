/**
 * test_triggers_aliases_gtest.cpp - GoogleTest version
 * Test Trigger and Alias data structures
 *
 * These tests verify:
 * 1. Trigger class creation with default values
 * 2. Alias class creation with default values
 * 3. Adding triggers/aliases to WorldDocument (map and array)
 * 4. Retrieving triggers/aliases by name
 * 5. Deleting triggers/aliases
 * 6. Sequence-based automatic sorting
 * 7. Wildcard vector allocation (MAX_WILDCARDS)
 * 8. Duplicate name prevention
 * 9. Equality operators
 * 10. Internal name tracking
 * 11. Field value preservation
 */

#include "../src/automation/alias.h"
#include "../src/automation/trigger.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>
#include <memory>

// Test fixture for trigger/alias tests
class TriggersAliasesTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    void TearDown() override
    {
    }

    std::unique_ptr<WorldDocument> doc;
};

// Test 1: Trigger creation with defaults
TEST_F(TriggersAliasesTest, TriggerDefaults)
{
    auto trigger = std::make_unique<Trigger>();

    EXPECT_TRUE(trigger->enabled) << "enabled should default to true";
    EXPECT_EQ(trigger->sequence, 100) << "sequence should default to 100";
    EXPECT_EQ(trigger->wildcards.size(), MAX_WILDCARDS)
        << "Wildcard vector should be allocated with MAX_WILDCARDS";
    EXPECT_EQ(trigger->regexp, nullptr) << "regexp pointer should be nullptr";
    EXPECT_EQ(trigger->send_to, eSendToWorld) << "send_to should default to eSendToWorld (0)";
    EXPECT_EQ(trigger->dispid, -1) << "dispid should default to DISPID_UNKNOWN (-1)";
    EXPECT_EQ(trigger->matched, 0) << "matched should be initialized to 0";
    EXPECT_EQ(trigger->invocation_count, 0) << "invocation_count should be initialized to 0";
}

// Test 2: Alias creation with defaults
TEST_F(TriggersAliasesTest, AliasDefaults)
{
    auto alias = std::make_unique<Alias>();

    EXPECT_TRUE(alias->enabled) << "enabled should default to true";
    EXPECT_EQ(alias->sequence, 100) << "sequence should default to 100";
    EXPECT_EQ(alias->wildcards.size(), MAX_WILDCARDS)
        << "Wildcard vector should be allocated with MAX_WILDCARDS";
    EXPECT_EQ(alias->regexp, nullptr) << "regexp pointer should be nullptr";
    EXPECT_EQ(alias->dispid, -1) << "dispid should default to DISPID_UNKNOWN (-1)";
}

// Test 3: Add trigger to WorldDocument
TEST_F(TriggersAliasesTest, AddTrigger)
{
    auto t1Owned = std::make_unique<Trigger>();
    t1Owned->trigger = "You have * hit points";
    t1Owned->label = "hp_trigger";
    t1Owned->sequence = 100;
    Trigger* t1 = t1Owned.get();

    auto addResult = doc->addTrigger("hp_trigger", std::move(t1Owned));

    EXPECT_TRUE(addResult.has_value()) << "addTrigger() should succeed";
    EXPECT_TRUE(doc->m_automationRegistry->m_TriggerMap.find("hp_trigger") !=
                doc->m_automationRegistry->m_TriggerMap.end())
        << "Trigger should be in m_TriggerMap";
    EXPECT_TRUE(doc->m_automationRegistry->m_TriggerArray.contains(t1))
        << "Trigger should be in m_TriggerArray";
    EXPECT_EQ(t1->internal_name, "hp_trigger") << "internal_name should be set";
}

// Test 4: Get trigger by name
TEST_F(TriggersAliasesTest, GetTrigger)
{
    auto t1Owned = std::make_unique<Trigger>();
    t1Owned->label = "test_trigger";
    Trigger* t1 = t1Owned.get();
    EXPECT_TRUE(doc->addTrigger("test_trigger", std::move(t1Owned)).has_value());

    Trigger* retrieved = doc->getTrigger("test_trigger");
    EXPECT_EQ(retrieved, t1) << "getTrigger() should return correct trigger";

    Trigger* nonexistent = doc->getTrigger("nonexistent_trigger");
    EXPECT_EQ(nonexistent, nullptr) << "getTrigger() should return nullptr for nonexistent trigger";
}

// Test 5: Duplicate trigger name prevention
TEST_F(TriggersAliasesTest, DuplicateTriggerPrevention)
{
    auto t1Owned = std::make_unique<Trigger>();
    t1Owned->label = "duplicate_trigger";
    EXPECT_TRUE(doc->addTrigger("duplicate_trigger", std::move(t1Owned)).has_value());

    auto duplicateResult = doc->addTrigger("duplicate_trigger", std::make_unique<Trigger>());

    EXPECT_FALSE(duplicateResult.has_value()) << "addTrigger() should reject duplicate names";
    EXPECT_EQ(duplicateResult.error().type, WorldErrorType::AlreadyExists);
    // duplicate is automatically deleted by unique_ptr if not added
}

// Test 6: Sequence-based trigger sorting
TEST_F(TriggersAliasesTest, TriggerSequenceSorting)
{
    auto t1Owned = std::make_unique<Trigger>();
    t1Owned->label = "hp_trigger";
    t1Owned->sequence = 100;
    Trigger* t1 = t1Owned.get();
    EXPECT_TRUE(doc->addTrigger("hp_trigger", std::move(t1Owned)).has_value());

    auto t2Owned = std::make_unique<Trigger>();
    t2Owned->label = "hunger_trigger";
    t2Owned->sequence = 50; // Lower sequence = earlier
    Trigger* t2 = t2Owned.get();
    EXPECT_TRUE(doc->addTrigger("hunger_trigger", std::move(t2Owned)).has_value());

    auto t3Owned = std::make_unique<Trigger>();
    t3Owned->label = "thirst_trigger";
    t3Owned->sequence = 200; // Higher sequence = later
    Trigger* t3 = t3Owned.get();
    EXPECT_TRUE(doc->addTrigger("thirst_trigger", std::move(t3Owned)).has_value());

    ASSERT_EQ(doc->m_automationRegistry->m_TriggerArray.size(), 3)
        << "All 3 triggers should be in array";

    // Triggers use lazy sorting, so rebuild array before checking order
    doc->rebuildTriggerArray();

    // Array should be sorted: t2 (50), t1 (100), t3 (200)
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[0], t2)
        << "First trigger should have lowest sequence";
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[1], t1)
        << "Second trigger should have middle sequence";
    EXPECT_EQ(doc->m_automationRegistry->m_TriggerArray[2], t3)
        << "Third trigger should have highest sequence";
}

// Test 7: Delete trigger
TEST_F(TriggersAliasesTest, DeleteTrigger)
{
    auto t1Owned = std::make_unique<Trigger>();
    t1Owned->label = "temp_trigger";
    EXPECT_TRUE(doc->addTrigger("temp_trigger", std::move(t1Owned)).has_value());

    auto deleteResult = doc->deleteTrigger("temp_trigger");

    EXPECT_TRUE(deleteResult.has_value()) << "deleteTrigger() should succeed";
    EXPECT_FALSE(doc->m_automationRegistry->m_TriggerMap.find("temp_trigger") !=
                 doc->m_automationRegistry->m_TriggerMap.end())
        << "Trigger should be removed from m_TriggerMap";
    // Note: Can't check m_TriggerArray.contains(t1) after delete - t1 is now deleted

    // Verify we can't delete it again
    auto deleteAgain = doc->deleteTrigger("temp_trigger");
    EXPECT_FALSE(deleteAgain.has_value())
        << "deleteTrigger() should fail for already-deleted trigger";
    EXPECT_EQ(deleteAgain.error().type, WorldErrorType::NotFound);
}

// Test 8: Add alias to WorldDocument
TEST_F(TriggersAliasesTest, AddAlias)
{
    auto a1 = std::make_unique<Alias>();
    a1->name = "^n$";
    a1->contents = "north";
    a1->label = "north_alias";
    a1->sequence = 100;
    a1->use_regexp = true;

    Alias* a1Ptr = a1.get();
    auto aliasAddResult = doc->addAlias("north_alias", std::move(a1));

    EXPECT_TRUE(aliasAddResult.has_value()) << "addAlias() should succeed";
    EXPECT_NE(doc->m_automationRegistry->m_AliasMap.find("north_alias"),
              doc->m_automationRegistry->m_AliasMap.end())
        << "Alias should be in m_AliasMap";
    EXPECT_TRUE(doc->m_automationRegistry->m_AliasArray.contains(a1Ptr))
        << "Alias should be in m_AliasArray";
    EXPECT_EQ(a1Ptr->internal_name, "north_alias") << "internal_name should be set";
}

// Test 9: Get alias by name
TEST_F(TriggersAliasesTest, GetAlias)
{
    auto a1 = std::make_unique<Alias>();
    a1->label = "test_alias";
    Alias* a1Ptr = a1.get();
    EXPECT_TRUE(doc->addAlias("test_alias", std::move(a1)).has_value());

    Alias* retrievedAlias = doc->getAlias("test_alias");
    EXPECT_EQ(retrievedAlias, a1Ptr) << "getAlias() should return correct alias";

    Alias* nonexistentAlias = doc->getAlias("nonexistent_alias");
    EXPECT_EQ(nonexistentAlias, nullptr)
        << "getAlias() should return nullptr for nonexistent alias";
}

// Test 10: Delete alias
TEST_F(TriggersAliasesTest, DeleteAlias)
{
    auto a1 = std::make_unique<Alias>();
    a1->label = "temp_alias";
    Alias* a1Ptr = a1.get();
    EXPECT_TRUE(doc->addAlias("temp_alias", std::move(a1)).has_value());

    auto aliasDeleteResult = doc->deleteAlias("temp_alias");

    EXPECT_TRUE(aliasDeleteResult.has_value()) << "deleteAlias() should succeed";
    EXPECT_EQ(doc->m_automationRegistry->m_AliasMap.find("temp_alias"),
              doc->m_automationRegistry->m_AliasMap.end())
        << "Alias should be removed from m_AliasMap";
    EXPECT_FALSE(doc->m_automationRegistry->m_AliasArray.contains(a1Ptr))
        << "Alias should be removed from m_AliasArray";
}

// Test 11: Trigger equality operator
TEST_F(TriggersAliasesTest, TriggerEquality)
{
    auto eq1 = std::make_unique<Trigger>();
    eq1->trigger = "test pattern";
    eq1->contents = "test contents";
    eq1->sequence = 50;
    eq1->enabled = true;

    auto eq2 = std::make_unique<Trigger>();
    eq2->trigger = "test pattern";
    eq2->contents = "test contents";
    eq2->sequence = 50;
    eq2->enabled = true;

    EXPECT_TRUE(*eq1 == *eq2) << "Identical triggers should be equal";

    eq2->sequence = 100; // Change one field
    EXPECT_FALSE(*eq1 == *eq2) << "Different triggers should not be equal";
}

// Test 12: Alias equality operator
TEST_F(TriggersAliasesTest, AliasEquality)
{
    auto aeq1 = std::make_unique<Alias>();
    aeq1->name = "test";
    aeq1->contents = "test command";
    aeq1->sequence = 50;

    auto aeq2 = std::make_unique<Alias>();
    aeq2->name = "test";
    aeq2->contents = "test command";
    aeq2->sequence = 50;

    EXPECT_TRUE(*aeq1 == *aeq2) << "Identical aliases should be equal";

    aeq2->name = "different"; // Change one field
    EXPECT_FALSE(*aeq1 == *aeq2) << "Different aliases should not be equal";
}

// Test 13: Multiple aliases with sorting
TEST_F(TriggersAliasesTest, AliasSequenceSorting)
{
    auto south = std::make_unique<Alias>();
    south->name = "s";
    south->contents = "south";
    south->sequence = 200;

    auto west = std::make_unique<Alias>();
    west->name = "w";
    west->contents = "west";
    west->sequence = 50;

    auto east = std::make_unique<Alias>();
    east->name = "e";
    east->contents = "east";
    east->sequence = 100;

    Alias* southPtr = south.get();
    Alias* westPtr = west.get();
    Alias* eastPtr = east.get();

    EXPECT_TRUE(doc->addAlias("south_alias", std::move(south)).has_value());
    EXPECT_TRUE(doc->addAlias("west_alias", std::move(west)).has_value());
    EXPECT_TRUE(doc->addAlias("east_alias", std::move(east)).has_value());

    // Force rebuild of alias array (lazy sorting)
    doc->rebuildAliasArray();

    ASSERT_EQ(doc->m_automationRegistry->m_AliasArray.size(), 3)
        << "All 3 aliases should be in array";

    // Should be sorted: west(50), east(100), south(200)
    EXPECT_EQ(doc->m_automationRegistry->m_AliasArray[0], westPtr)
        << "First alias should have lowest sequence";
    EXPECT_EQ(doc->m_automationRegistry->m_AliasArray[1], eastPtr)
        << "Second alias should have middle sequence";
    EXPECT_EQ(doc->m_automationRegistry->m_AliasArray[2], southPtr)
        << "Third alias should have highest sequence";
}

// Test 14: Field value preservation
TEST_F(TriggersAliasesTest, TriggerFieldPreservation)
{
    auto detailedOwned = std::make_unique<Trigger>();
    detailedOwned->trigger = "You have (*) gold";
    detailedOwned->contents = "say I have %1 gold!";
    detailedOwned->label = "gold_trigger";
    detailedOwned->procedure = "on_gold_change";
    detailedOwned->sequence = 75;
    detailedOwned->enabled = false;
    detailedOwned->use_regexp = true;
    detailedOwned->keep_evaluating = true;
    detailedOwned->omit_from_output = true;
    detailedOwned->colour = 5;
    detailedOwned->send_to = eSendToScript;
    detailedOwned->group = "Currency";
    detailedOwned->user_option = 42;

    EXPECT_TRUE(doc->addTrigger("gold_trigger", std::move(detailedOwned)).has_value());

    Trigger* verified = doc->getTrigger("gold_trigger");

    EXPECT_EQ(verified->trigger, "You have (*) gold") << "trigger field should be preserved";
    EXPECT_EQ(verified->contents, "say I have %1 gold!") << "contents field should be preserved";
    EXPECT_EQ(verified->label, "gold_trigger") << "label field should be preserved";
    EXPECT_EQ(verified->procedure, "on_gold_change") << "procedure field should be preserved";
    EXPECT_EQ(verified->sequence, 75) << "sequence field should be preserved";
    EXPECT_FALSE(verified->enabled) << "enabled field should be preserved";
    EXPECT_TRUE(verified->use_regexp) << "use_regexp field should be preserved";
    EXPECT_TRUE(verified->keep_evaluating) << "keep_evaluating field should be preserved";
    EXPECT_TRUE(verified->omit_from_output) << "omit_from_output field should be preserved";
    EXPECT_EQ(verified->colour, 5) << "colour field should be preserved";
    EXPECT_EQ(verified->send_to, eSendToScript) << "send_to field should be preserved";
    EXPECT_EQ(verified->group, "Currency") << "group field should be preserved";
    EXPECT_EQ(verified->user_option, 42) << "user_option field should be preserved";
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
