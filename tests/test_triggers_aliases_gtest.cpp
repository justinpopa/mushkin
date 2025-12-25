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

// Test fixture for trigger/alias tests
class TriggersAliasesTest : public ::testing::Test {
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

// Test 1: Trigger creation with defaults
TEST_F(TriggersAliasesTest, TriggerDefaults)
{
    Trigger* trigger = new Trigger();

    EXPECT_TRUE(trigger->bEnabled) << "bEnabled should default to true";
    EXPECT_EQ(trigger->iSequence, 100) << "iSequence should default to 100";
    EXPECT_EQ(trigger->wildcards.size(), MAX_WILDCARDS)
        << "Wildcard vector should be allocated with MAX_WILDCARDS";
    EXPECT_EQ(trigger->regexp, nullptr) << "regexp pointer should be nullptr";
    EXPECT_EQ(trigger->iSendTo, 0) << "iSendTo should default to eSendToWorld (0)";
    EXPECT_EQ(trigger->dispid, -1) << "dispid should default to DISPID_UNKNOWN (-1)";
    EXPECT_EQ(trigger->nMatched, 0) << "nMatched should be initialized to 0";
    EXPECT_EQ(trigger->nInvocationCount, 0) << "nInvocationCount should be initialized to 0";

    delete trigger;
}

// Test 2: Alias creation with defaults
TEST_F(TriggersAliasesTest, AliasDefaults)
{
    auto alias = std::make_unique<Alias>();

    EXPECT_TRUE(alias->bEnabled) << "bEnabled should default to true";
    EXPECT_EQ(alias->iSequence, 100) << "iSequence should default to 100";
    EXPECT_EQ(alias->wildcards.size(), MAX_WILDCARDS)
        << "Wildcard vector should be allocated with MAX_WILDCARDS";
    EXPECT_EQ(alias->regexp, nullptr) << "regexp pointer should be nullptr";
    EXPECT_EQ(alias->dispid, -1) << "dispid should default to DISPID_UNKNOWN (-1)";
}

// Test 3: Add trigger to WorldDocument
TEST_F(TriggersAliasesTest, AddTrigger)
{
    Trigger* t1 = new Trigger();
    t1->trigger = "You have * hit points";
    t1->strLabel = "hp_trigger";
    t1->iSequence = 100;

    bool addResult = doc->addTrigger("hp_trigger", std::unique_ptr<Trigger>(t1));

    EXPECT_TRUE(addResult) << "addTrigger() should return true";
    EXPECT_TRUE(doc->m_TriggerMap.find("hp_trigger") != doc->m_TriggerMap.end())
        << "Trigger should be in m_TriggerMap";
    EXPECT_TRUE(doc->m_TriggerArray.contains(t1)) << "Trigger should be in m_TriggerArray";
    EXPECT_EQ(t1->strInternalName, "hp_trigger") << "strInternalName should be set";
}

// Test 4: Get trigger by name
TEST_F(TriggersAliasesTest, GetTrigger)
{
    Trigger* t1 = new Trigger();
    t1->strLabel = "test_trigger";
    doc->addTrigger("test_trigger", std::unique_ptr<Trigger>(t1));

    Trigger* retrieved = doc->getTrigger("test_trigger");
    EXPECT_EQ(retrieved, t1) << "getTrigger() should return correct trigger";

    Trigger* nonexistent = doc->getTrigger("nonexistent_trigger");
    EXPECT_EQ(nonexistent, nullptr) << "getTrigger() should return nullptr for nonexistent trigger";
}

// Test 5: Duplicate trigger name prevention
TEST_F(TriggersAliasesTest, DuplicateTriggerPrevention)
{
    Trigger* t1 = new Trigger();
    t1->strLabel = "duplicate_trigger";
    doc->addTrigger("duplicate_trigger", std::unique_ptr<Trigger>(t1));

    Trigger* duplicate = new Trigger();
    bool duplicateResult =
        doc->addTrigger("duplicate_trigger", std::unique_ptr<Trigger>(duplicate));

    EXPECT_FALSE(duplicateResult) << "addTrigger() should reject duplicate names";
    // duplicate is automatically deleted by unique_ptr if not added
}

// Test 6: Sequence-based trigger sorting
TEST_F(TriggersAliasesTest, TriggerSequenceSorting)
{
    Trigger* t1 = new Trigger();
    t1->strLabel = "hp_trigger";
    t1->iSequence = 100;
    doc->addTrigger("hp_trigger", std::unique_ptr<Trigger>(t1));

    Trigger* t2 = new Trigger();
    t2->strLabel = "hunger_trigger";
    t2->iSequence = 50; // Lower sequence = earlier
    doc->addTrigger("hunger_trigger", std::unique_ptr<Trigger>(t2));

    Trigger* t3 = new Trigger();
    t3->strLabel = "thirst_trigger";
    t3->iSequence = 200; // Higher sequence = later
    doc->addTrigger("thirst_trigger", std::unique_ptr<Trigger>(t3));

    ASSERT_EQ(doc->m_TriggerArray.size(), 3) << "All 3 triggers should be in array";

    // Triggers use lazy sorting, so rebuild array before checking order
    doc->rebuildTriggerArray();

    // Array should be sorted: t2 (50), t1 (100), t3 (200)
    EXPECT_EQ(doc->m_TriggerArray[0], t2) << "First trigger should have lowest sequence";
    EXPECT_EQ(doc->m_TriggerArray[1], t1) << "Second trigger should have middle sequence";
    EXPECT_EQ(doc->m_TriggerArray[2], t3) << "Third trigger should have highest sequence";
}

// Test 7: Delete trigger
TEST_F(TriggersAliasesTest, DeleteTrigger)
{
    Trigger* t1 = new Trigger();
    t1->strLabel = "temp_trigger";
    doc->addTrigger("temp_trigger", std::unique_ptr<Trigger>(t1));

    bool deleteResult = doc->deleteTrigger("temp_trigger");

    EXPECT_TRUE(deleteResult) << "deleteTrigger() should return true";
    EXPECT_FALSE(doc->m_TriggerMap.find("temp_trigger") != doc->m_TriggerMap.end())
        << "Trigger should be removed from m_TriggerMap";
    // Note: Can't check m_TriggerArray.contains(t1) after delete - t1 is now deleted

    // Verify we can't delete it again
    bool deleteAgain = doc->deleteTrigger("temp_trigger");
    EXPECT_FALSE(deleteAgain) << "deleteTrigger() should return false for already-deleted trigger";
}

// Test 8: Add alias to WorldDocument
TEST_F(TriggersAliasesTest, AddAlias)
{
    auto a1 = std::make_unique<Alias>();
    a1->name = "^n$";
    a1->contents = "north";
    a1->strLabel = "north_alias";
    a1->iSequence = 100;
    a1->bRegexp = true;

    Alias* a1Ptr = a1.get();
    bool aliasAddResult = doc->addAlias("north_alias", std::move(a1));

    EXPECT_TRUE(aliasAddResult) << "addAlias() should return true";
    EXPECT_NE(doc->m_AliasMap.find("north_alias"), doc->m_AliasMap.end())
        << "Alias should be in m_AliasMap";
    EXPECT_TRUE(doc->m_AliasArray.contains(a1Ptr)) << "Alias should be in m_AliasArray";
    EXPECT_EQ(a1Ptr->strInternalName, "north_alias") << "strInternalName should be set";
}

// Test 9: Get alias by name
TEST_F(TriggersAliasesTest, GetAlias)
{
    auto a1 = std::make_unique<Alias>();
    a1->strLabel = "test_alias";
    Alias* a1Ptr = a1.get();
    doc->addAlias("test_alias", std::move(a1));

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
    a1->strLabel = "temp_alias";
    Alias* a1Ptr = a1.get();
    doc->addAlias("temp_alias", std::move(a1));

    bool aliasDeleteResult = doc->deleteAlias("temp_alias");

    EXPECT_TRUE(aliasDeleteResult) << "deleteAlias() should return true";
    EXPECT_EQ(doc->m_AliasMap.find("temp_alias"), doc->m_AliasMap.end())
        << "Alias should be removed from m_AliasMap";
    EXPECT_FALSE(doc->m_AliasArray.contains(a1Ptr)) << "Alias should be removed from m_AliasArray";
}

// Test 11: Trigger equality operator
TEST_F(TriggersAliasesTest, TriggerEquality)
{
    Trigger* eq1 = new Trigger();
    eq1->trigger = "test pattern";
    eq1->contents = "test contents";
    eq1->iSequence = 50;
    eq1->bEnabled = true;

    Trigger* eq2 = new Trigger();
    eq2->trigger = "test pattern";
    eq2->contents = "test contents";
    eq2->iSequence = 50;
    eq2->bEnabled = true;

    EXPECT_TRUE(*eq1 == *eq2) << "Identical triggers should be equal";

    eq2->iSequence = 100; // Change one field
    EXPECT_FALSE(*eq1 == *eq2) << "Different triggers should not be equal";

    delete eq1;
    delete eq2;
}

// Test 12: Alias equality operator
TEST_F(TriggersAliasesTest, AliasEquality)
{
    auto aeq1 = std::make_unique<Alias>();
    aeq1->name = "test";
    aeq1->contents = "test command";
    aeq1->iSequence = 50;

    auto aeq2 = std::make_unique<Alias>();
    aeq2->name = "test";
    aeq2->contents = "test command";
    aeq2->iSequence = 50;

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
    south->iSequence = 200;

    auto west = std::make_unique<Alias>();
    west->name = "w";
    west->contents = "west";
    west->iSequence = 50;

    auto east = std::make_unique<Alias>();
    east->name = "e";
    east->contents = "east";
    east->iSequence = 100;

    Alias* southPtr = south.get();
    Alias* westPtr = west.get();
    Alias* eastPtr = east.get();

    doc->addAlias("south_alias", std::move(south));
    doc->addAlias("west_alias", std::move(west));
    doc->addAlias("east_alias", std::move(east));

    // Force rebuild of alias array (lazy sorting)
    doc->rebuildAliasArray();

    ASSERT_EQ(doc->m_AliasArray.size(), 3) << "All 3 aliases should be in array";

    // Should be sorted: west(50), east(100), south(200)
    EXPECT_EQ(doc->m_AliasArray[0], westPtr) << "First alias should have lowest sequence";
    EXPECT_EQ(doc->m_AliasArray[1], eastPtr) << "Second alias should have middle sequence";
    EXPECT_EQ(doc->m_AliasArray[2], southPtr) << "Third alias should have highest sequence";
}

// Test 14: Field value preservation
TEST_F(TriggersAliasesTest, TriggerFieldPreservation)
{
    Trigger* detailed = new Trigger();
    detailed->trigger = "You have (*) gold";
    detailed->contents = "say I have %1 gold!";
    detailed->strLabel = "gold_trigger";
    detailed->strProcedure = "on_gold_change";
    detailed->iSequence = 75;
    detailed->bEnabled = false;
    detailed->bRegexp = true;
    detailed->bKeepEvaluating = true;
    detailed->bOmitFromOutput = true;
    detailed->colour = 5;
    detailed->iSendTo = 12; // eSendToScript
    detailed->strGroup = "Currency";
    detailed->iUserOption = 42;

    doc->addTrigger("gold_trigger", std::unique_ptr<Trigger>(detailed));

    Trigger* verified = doc->getTrigger("gold_trigger");

    EXPECT_EQ(verified->trigger, "You have (*) gold") << "trigger field should be preserved";
    EXPECT_EQ(verified->contents, "say I have %1 gold!") << "contents field should be preserved";
    EXPECT_EQ(verified->strLabel, "gold_trigger") << "strLabel field should be preserved";
    EXPECT_EQ(verified->strProcedure, "on_gold_change") << "strProcedure field should be preserved";
    EXPECT_EQ(verified->iSequence, 75) << "iSequence field should be preserved";
    EXPECT_FALSE(verified->bEnabled) << "bEnabled field should be preserved";
    EXPECT_TRUE(verified->bRegexp) << "bRegexp field should be preserved";
    EXPECT_TRUE(verified->bKeepEvaluating) << "bKeepEvaluating field should be preserved";
    EXPECT_TRUE(verified->bOmitFromOutput) << "bOmitFromOutput field should be preserved";
    EXPECT_EQ(verified->colour, 5) << "colour field should be preserved";
    EXPECT_EQ(verified->iSendTo, 12) << "iSendTo field should be preserved";
    EXPECT_EQ(verified->strGroup, "Currency") << "strGroup field should be preserved";
    EXPECT_EQ(verified->iUserOption, 42) << "iUserOption field should be preserved";
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
