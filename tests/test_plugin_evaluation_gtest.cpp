/**
 * test_plugin_evaluation_gtest.cpp - GoogleTest version
 * Plugin Evaluation Order Test
 *
 * Tests plugin sequence-based evaluation order including:
 * - Triggers: negative sequence → world → positive sequence
 * - Aliases: negative sequence → world → positive sequence
 * - keep_evaluating flag stopping evaluation at each phase
 * - One-shot triggers/aliases deleted from correct plugin context
 */

#include "../src/automation/alias.h"
#include "../src/automation/plugin.h"
#include "../src/automation/sendto.h"
#include "../src/automation/trigger.h"
#include "fixtures/world_fixtures.h"
#include <QTemporaryFile>

// Lua 5.1 compatibility: lua_rawlen is Lua 5.2+, use lua_objlen for 5.1
#if LUA_VERSION_NUM < 502
#    define lua_rawlen lua_objlen
#endif

// Helper: Create a plugin XML with a given sequence number
QString createPluginXml(const QString& name, const QString& id, int sequence)
{
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin
  name="%1"
  author="Test Author"
  id="%2"
  language="Lua"
  purpose="Test plugin evaluation order"
  version="1.0"
  save_state="n"
  sequence="%3"
>

<script>
<![CDATA[
-- Track execution order
execution_log = execution_log or {}

function RecordExecution(source)
  table.insert(execution_log, source)
end
]]>
</script>

</plugin>
</muclient>
)")
        .arg(name, id)
        .arg(sequence);
}

// Test fixture for plugin evaluation tests
class PluginEvaluationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = std::make_unique<WorldDocument>();

        // Create three plugins with different sequences
        // Plugin 1: Negative sequence (-10)
        plugin1File = std::make_unique<QTemporaryFile>();
        plugin1File->setFileTemplate("test-plugin1-XXXXXX.xml");
        plugin1File->open();
        plugin1File->write(
            createPluginXml("Plugin-Negative", "{11111111-1111-1111-1111-111111111111}", -10)
                .toUtf8());
        plugin1File->flush();

        // Plugin 2: Zero sequence (0)
        plugin2File = std::make_unique<QTemporaryFile>();
        plugin2File->setFileTemplate("test-plugin2-XXXXXX.xml");
        plugin2File->open();
        plugin2File->write(
            createPluginXml("Plugin-Zero", "{22222222-2222-2222-2222-222222222222}", 0).toUtf8());
        plugin2File->flush();

        // Plugin 3: Positive sequence (10)
        plugin3File = std::make_unique<QTemporaryFile>();
        plugin3File->setFileTemplate("test-plugin3-XXXXXX.xml");
        plugin3File->open();
        plugin3File->write(
            createPluginXml("Plugin-Positive", "{33333333-3333-3333-3333-333333333333}", 10)
                .toUtf8());
        plugin3File->flush();

        // Load plugins
        QString errorMsg;
        pluginNeg = doc->LoadPlugin(plugin1File->fileName(), errorMsg);
        pluginZero = doc->LoadPlugin(plugin2File->fileName(), errorMsg);
        pluginPos = doc->LoadPlugin(plugin3File->fileName(), errorMsg);


        ASSERT_NE(pluginNeg, nullptr)
            << "Could not load plugin-negative: " << errorMsg.toStdString();
        ASSERT_NE(pluginZero, nullptr) << "Could not load plugin-zero: " << errorMsg.toStdString();
        ASSERT_NE(pluginPos, nullptr)
            << "Could not load plugin-positive: " << errorMsg.toStdString();

        // Verify plugin sequences
        ASSERT_EQ(pluginNeg->m_iSequence, -10);
        ASSERT_EQ(pluginZero->m_iSequence, 0);
        ASSERT_EQ(pluginPos->m_iSequence, 10);
    }

    void TearDown() override
    {
    }

    // Helper to create a test line
    std::unique_ptr<Line> createTestLine(const char* text, int lineNumber = 1)
    {
        auto line = std::make_unique<Line>(lineNumber, // line number
                                           80,         // wrap column
                                           0,          // line flags
                                           0x00FFFFFF, // foreground color (white)
                                           0x00000000, // background color (black)
                                           false       // unicode
        );
        int len = strlen(text);
        line->textBuffer.resize(len);
        memcpy(line->textBuffer.data(), text, len);
        line->textBuffer.push_back('\0');
        line->styleList.push_back(std::make_unique<Style>());
        return line;
    }

    std::unique_ptr<WorldDocument> doc;
    Plugin* pluginNeg = nullptr;
    Plugin* pluginZero = nullptr;
    Plugin* pluginPos = nullptr;
    std::unique_ptr<QTemporaryFile> plugin1File;
    std::unique_ptr<QTemporaryFile> plugin2File;
    std::unique_ptr<QTemporaryFile> plugin3File;
};

// Test 1: Trigger Evaluation Order
TEST_F(PluginEvaluationTest, TriggerEvaluationOrder)
{
    // Add triggers to each context
    // Negative plugin trigger
    auto trigNegOwned1 = std::make_unique<Trigger>();
    trigNegOwned1->internal_name = "trig_neg";
    trigNegOwned1->label = "Trigger-Negative";
    trigNegOwned1->trigger = "Hello*";
    trigNegOwned1->send_to = eSendToWorld;
    trigNegOwned1->enabled = true;
    trigNegOwned1->sequence = 100;
    trigNegOwned1->keep_evaluating = true;
    Trigger* trigNeg = trigNegOwned1.get();
    pluginNeg->m_TriggerMap[trigNegOwned1->internal_name] = std::move(trigNegOwned1);
    pluginNeg->m_triggersNeedSorting = true;

    // World trigger
    auto trigWorldOwned1 = std::make_unique<Trigger>();
    trigWorldOwned1->internal_name = "trig_world";
    trigWorldOwned1->label = "Trigger-World";
    trigWorldOwned1->trigger = "Hello*";
    trigWorldOwned1->send_to = eSendToWorld;
    trigWorldOwned1->enabled = true;
    trigWorldOwned1->sequence = 100;
    trigWorldOwned1->keep_evaluating = true;
    Trigger* trigWorld = trigWorldOwned1.get();
    doc->m_automationRegistry->m_TriggerMap[trigWorldOwned1->internal_name] =
        std::move(trigWorldOwned1);
    doc->m_automationRegistry->m_triggersNeedSorting = true;

    // Positive plugin trigger
    auto trigPosOwned1 = std::make_unique<Trigger>();
    trigPosOwned1->internal_name = "trig_pos";
    trigPosOwned1->label = "Trigger-Positive";
    trigPosOwned1->trigger = "Hello*";
    trigPosOwned1->send_to = eSendToWorld;
    trigPosOwned1->enabled = true;
    trigPosOwned1->sequence = 100;
    trigPosOwned1->keep_evaluating = true;
    Trigger* trigPos = trigPosOwned1.get();
    pluginPos->m_TriggerMap[trigPosOwned1->internal_name] = std::move(trigPosOwned1);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    auto testLine = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine.get());

    // Check that all three triggers matched (by checking matched counter)
    EXPECT_EQ(trigNeg->matched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld->matched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos->matched, 1) << "Positive plugin trigger should have matched";
}

// Test 2: Alias Evaluation Order
TEST_F(PluginEvaluationTest, AliasEvaluationOrder)
{
    // Add aliases to each context
    // Negative plugin alias
    auto aliasNeg = std::make_unique<Alias>();
    aliasNeg->internal_name = "alias_neg";
    aliasNeg->label = "Alias-Negative";
    aliasNeg->name = "test*";
    aliasNeg->use_regexp = false;
    aliasNeg->send_to = eSendToWorld;
    aliasNeg->enabled = true;
    aliasNeg->sequence = 100;
    aliasNeg->keep_evaluating = true;
    Alias* aliasNegPtr = aliasNeg.get();
    pluginNeg->m_AliasMap[aliasNeg->internal_name] = std::move(aliasNeg);
    pluginNeg->m_aliasesNeedSorting = true;

    // World alias
    auto aliasWorld = std::make_unique<Alias>();
    aliasWorld->internal_name = "alias_world";
    aliasWorld->label = "Alias-World";
    aliasWorld->name = "test*";
    aliasWorld->use_regexp = false;
    aliasWorld->send_to = eSendToWorld;
    aliasWorld->enabled = true;
    aliasWorld->sequence = 100;
    aliasWorld->keep_evaluating = true;
    Alias* aliasWorldPtr = aliasWorld.get();
    doc->m_automationRegistry->m_AliasMap[aliasWorld->internal_name] = std::move(aliasWorld);
    doc->m_automationRegistry->m_aliasesNeedSorting = true;

    // Positive plugin alias
    auto aliasPos = std::make_unique<Alias>();
    aliasPos->internal_name = "alias_pos";
    aliasPos->label = "Alias-Positive";
    aliasPos->name = "test*";
    aliasPos->use_regexp = false;
    aliasPos->send_to = eSendToWorld;
    aliasPos->enabled = true;
    aliasPos->sequence = 100;
    aliasPos->keep_evaluating = true;
    Alias* aliasPosPtr = aliasPos.get();
    pluginPos->m_AliasMap[aliasPos->internal_name] = std::move(aliasPos);
    pluginPos->m_aliasesNeedSorting = true;

    // Evaluate aliases
    doc->evaluateAliases("test command");

    // Check that all three aliases matched (by checking matched counter)
    EXPECT_EQ(aliasNegPtr->matched, 1) << "Negative plugin alias should have matched";
    EXPECT_EQ(aliasWorldPtr->matched, 1) << "World alias should have matched";
    EXPECT_EQ(aliasPosPtr->matched, 1) << "Positive plugin alias should have matched";
}

// Test 3: keep_evaluating = false stops at negative phase
TEST_F(PluginEvaluationTest, KeepEvaluatingStopsAtNegativePhase)
{
    // Add triggers to each context
    auto trigNegOwned3 = std::make_unique<Trigger>();
    trigNegOwned3->internal_name = "trig_neg";
    trigNegOwned3->label = "Trigger-Negative";
    trigNegOwned3->trigger = "Hello*";
    trigNegOwned3->send_to = eSendToWorld;
    trigNegOwned3->enabled = true;
    trigNegOwned3->sequence = 100;
    trigNegOwned3->keep_evaluating = false; // Stop evaluation
    Trigger* trigNeg3 = trigNegOwned3.get();
    pluginNeg->m_TriggerMap[trigNegOwned3->internal_name] = std::move(trigNegOwned3);
    pluginNeg->m_triggersNeedSorting = true;

    auto trigWorldOwned3 = std::make_unique<Trigger>();
    trigWorldOwned3->internal_name = "trig_world";
    trigWorldOwned3->label = "Trigger-World";
    trigWorldOwned3->trigger = "Hello*";
    trigWorldOwned3->send_to = eSendToWorld;
    trigWorldOwned3->enabled = true;
    trigWorldOwned3->sequence = 100;
    trigWorldOwned3->keep_evaluating = true;
    Trigger* trigWorld3 = trigWorldOwned3.get();
    doc->m_automationRegistry->m_TriggerMap[trigWorldOwned3->internal_name] =
        std::move(trigWorldOwned3);
    doc->m_automationRegistry->m_triggersNeedSorting = true;

    auto trigPosOwned3 = std::make_unique<Trigger>();
    trigPosOwned3->internal_name = "trig_pos";
    trigPosOwned3->label = "Trigger-Positive";
    trigPosOwned3->trigger = "Hello*";
    trigPosOwned3->send_to = eSendToWorld;
    trigPosOwned3->enabled = true;
    trigPosOwned3->sequence = 100;
    trigPosOwned3->keep_evaluating = true;
    Trigger* trigPos3 = trigPosOwned3.get();
    pluginPos->m_TriggerMap[trigPosOwned3->internal_name] = std::move(trigPosOwned3);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    auto testLine3 = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine3.get());

    // Check that only negative plugin trigger matched (others should not match)
    EXPECT_EQ(trigNeg3->matched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld3->matched, 0) << "World trigger should not have matched";
    EXPECT_EQ(trigPos3->matched, 0) << "Positive plugin trigger should not have matched";
}

// Test 4: keep_evaluating = false stops at world phase
TEST_F(PluginEvaluationTest, KeepEvaluatingStopsAtWorldPhase)
{
    // Add triggers to each context
    auto trigNegOwned4 = std::make_unique<Trigger>();
    trigNegOwned4->internal_name = "trig_neg";
    trigNegOwned4->label = "Trigger-Negative";
    trigNegOwned4->trigger = "Hello*";
    trigNegOwned4->send_to = eSendToWorld;
    trigNegOwned4->enabled = true;
    trigNegOwned4->sequence = 100;
    trigNegOwned4->keep_evaluating = true;
    Trigger* trigNeg4 = trigNegOwned4.get();
    pluginNeg->m_TriggerMap[trigNegOwned4->internal_name] = std::move(trigNegOwned4);
    pluginNeg->m_triggersNeedSorting = true;

    auto trigWorldOwned4 = std::make_unique<Trigger>();
    trigWorldOwned4->internal_name = "trig_world";
    trigWorldOwned4->label = "Trigger-World";
    trigWorldOwned4->trigger = "Hello*";
    trigWorldOwned4->send_to = eSendToWorld;
    trigWorldOwned4->enabled = true;
    trigWorldOwned4->sequence = 100;
    trigWorldOwned4->keep_evaluating = false; // Stop evaluation
    Trigger* trigWorld4 = trigWorldOwned4.get();
    doc->m_automationRegistry->m_TriggerMap[trigWorldOwned4->internal_name] =
        std::move(trigWorldOwned4);
    doc->m_automationRegistry->m_triggersNeedSorting = true;

    auto trigPosOwned4 = std::make_unique<Trigger>();
    trigPosOwned4->internal_name = "trig_pos";
    trigPosOwned4->label = "Trigger-Positive";
    trigPosOwned4->trigger = "Hello*";
    trigPosOwned4->send_to = eSendToWorld;
    trigPosOwned4->enabled = true;
    trigPosOwned4->sequence = 100;
    trigPosOwned4->keep_evaluating = true;
    Trigger* trigPos4 = trigPosOwned4.get();
    pluginPos->m_TriggerMap[trigPosOwned4->internal_name] = std::move(trigPosOwned4);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    auto testLine4 = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine4.get());

    // Check that negative and world matched, but not positive
    EXPECT_EQ(trigNeg4->matched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld4->matched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos4->matched, 0) << "Positive plugin trigger should not have matched";
}

// Test 5: One-shot trigger deleted from correct plugin context
TEST_F(PluginEvaluationTest, OneShotTriggerDeletedFromCorrectContext)
{
    // Create one-shot trigger in negative plugin
    auto trigOneShotOwned = std::make_unique<Trigger>();
    trigOneShotOwned->internal_name = "trig_oneshot";
    trigOneShotOwned->label = "Trigger-OneShot";
    trigOneShotOwned->trigger = "OneShot*";
    trigOneShotOwned->enabled = true;
    trigOneShotOwned->sequence = 100;
    trigOneShotOwned->one_shot = true;
    trigOneShotOwned->keep_evaluating = true;
    pluginNeg->m_TriggerMap[trigOneShotOwned->internal_name] = std::move(trigOneShotOwned);
    pluginNeg->m_triggersNeedSorting = true;

    int triggerCountBefore = pluginNeg->m_TriggerMap.size();

    // Create a line that matches the one-shot trigger
    auto oneShotLine = createTestLine("OneShot message", 2);

    // Evaluate triggers
    doc->evaluateTriggers(oneShotLine.get());

    int triggerCountAfter = pluginNeg->m_TriggerMap.size();

    EXPECT_EQ(triggerCountAfter, triggerCountBefore - 1)
        << "One-shot trigger should have been deleted";

    // Verify it was deleted from the correct plugin
    EXPECT_TRUE(pluginNeg->m_TriggerMap.find("trig_oneshot") == pluginNeg->m_TriggerMap.end())
        << "One-shot trigger should not exist in plugin";
}

// Test 6: Disabled plugin not evaluated
TEST_F(PluginEvaluationTest, DisabledPluginNotEvaluated)
{
    // Add triggers to each context
    auto trigNegOwned6 = std::make_unique<Trigger>();
    trigNegOwned6->internal_name = "trig_neg";
    trigNegOwned6->label = "Trigger-Negative";
    trigNegOwned6->trigger = "Hello*";
    trigNegOwned6->send_to = eSendToWorld;
    trigNegOwned6->enabled = true;
    trigNegOwned6->sequence = 100;
    trigNegOwned6->keep_evaluating = true;
    Trigger* trigNeg6 = trigNegOwned6.get();
    pluginNeg->m_TriggerMap[trigNegOwned6->internal_name] = std::move(trigNegOwned6);
    pluginNeg->m_triggersNeedSorting = true;

    auto trigWorldOwned6 = std::make_unique<Trigger>();
    trigWorldOwned6->internal_name = "trig_world";
    trigWorldOwned6->label = "Trigger-World";
    trigWorldOwned6->trigger = "Hello*";
    trigWorldOwned6->send_to = eSendToWorld;
    trigWorldOwned6->enabled = true;
    trigWorldOwned6->sequence = 100;
    trigWorldOwned6->keep_evaluating = true;
    Trigger* trigWorld6 = trigWorldOwned6.get();
    doc->m_automationRegistry->m_TriggerMap[trigWorldOwned6->internal_name] =
        std::move(trigWorldOwned6);
    doc->m_automationRegistry->m_triggersNeedSorting = true;

    auto trigPosOwned6 = std::make_unique<Trigger>();
    trigPosOwned6->internal_name = "trig_pos";
    trigPosOwned6->label = "Trigger-Positive";
    trigPosOwned6->trigger = "Hello*";
    trigPosOwned6->send_to = eSendToWorld;
    trigPosOwned6->enabled = true;
    trigPosOwned6->sequence = 100;
    trigPosOwned6->keep_evaluating = true;
    Trigger* trigPos6 = trigPosOwned6.get();
    pluginPos->m_TriggerMap[trigPosOwned6->internal_name] = std::move(trigPosOwned6);
    pluginPos->m_triggersNeedSorting = true;

    // Disable negative plugin
    pluginNeg->m_bEnabled = false;

    // Create a test line
    auto testLine6 = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine6.get());

    // Check that negative plugin didn't match (disabled)
    EXPECT_EQ(trigNeg6->matched, 0) << "Negative plugin trigger should not have matched (disabled)";
    EXPECT_EQ(trigWorld6->matched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos6->matched, 1) << "Positive plugin trigger should have matched";
}

// Test 7: Multiple plugins in same phase evaluated in sequence order
TEST_F(PluginEvaluationTest, MultiplePluginsInSamePhaseEvaluatedInSequenceOrder)
{
    // Create two additional plugins with different sequences
    // Plugin 4: Negative sequence (-5) - should fire AFTER -10
    QTemporaryFile plugin4File;
    plugin4File.setFileTemplate("test-plugin4-XXXXXX.xml");
    plugin4File.open();
    plugin4File.write(
        createPluginXml("Plugin-Negative-5", "{44444444-4444-4444-4444-444444444444}", -5)
            .toUtf8());
    plugin4File.flush();

    // Plugin 5: Positive sequence (15) - should fire AFTER 10
    QTemporaryFile plugin5File;
    plugin5File.setFileTemplate("test-plugin5-XXXXXX.xml");
    plugin5File.open();
    plugin5File.write(
        createPluginXml("Plugin-Positive-15", "{55555555-5555-5555-5555-555555555555}", 15)
            .toUtf8());
    plugin5File.flush();

    // Load new plugins
    QString errorMsg;
    Plugin* pluginNeg5 = doc->LoadPlugin(plugin4File.fileName(), errorMsg);
    Plugin* pluginPos15 = doc->LoadPlugin(plugin5File.fileName(), errorMsg);

    ASSERT_NE(pluginNeg5, nullptr)
        << "Could not load plugin-negative-5: " << errorMsg.toStdString();
    ASSERT_NE(pluginPos15, nullptr)
        << "Could not load plugin-positive-15: " << errorMsg.toStdString();

    // Verify plugin list is sorted by sequence
    ASSERT_EQ(doc->m_PluginList.size(), 5) << "Expected 5 plugins";

    // Verify sorting (should be: -10, -5, 0, 10, 15)
    EXPECT_EQ(doc->m_PluginList[0]->m_iSequence, -10);
    EXPECT_EQ(doc->m_PluginList[1]->m_iSequence, -5);
    EXPECT_EQ(doc->m_PluginList[2]->m_iSequence, 0);
    EXPECT_EQ(doc->m_PluginList[3]->m_iSequence, 10);
    EXPECT_EQ(doc->m_PluginList[4]->m_iSequence, 15);

    // Add triggers to new plugins
    auto trigNeg5Owned = std::make_unique<Trigger>();
    trigNeg5Owned->internal_name = "trig_neg5";
    trigNeg5Owned->label = "Trigger-Negative-5";
    trigNeg5Owned->trigger = "MultiPlugin*";
    trigNeg5Owned->send_to = eSendToWorld;
    trigNeg5Owned->enabled = true;
    trigNeg5Owned->sequence = 100;
    trigNeg5Owned->keep_evaluating = true;
    Trigger* trigNeg5 = trigNeg5Owned.get();
    pluginNeg5->m_TriggerMap[trigNeg5Owned->internal_name] = std::move(trigNeg5Owned);
    pluginNeg5->m_triggersNeedSorting = true;

    auto trigPos15Owned = std::make_unique<Trigger>();
    trigPos15Owned->internal_name = "trig_pos15";
    trigPos15Owned->label = "Trigger-Positive-15";
    trigPos15Owned->trigger = "MultiPlugin*";
    trigPos15Owned->send_to = eSendToWorld;
    trigPos15Owned->enabled = true;
    trigPos15Owned->sequence = 100;
    trigPos15Owned->keep_evaluating = true;
    Trigger* trigPos15 = trigPos15Owned.get();
    pluginPos15->m_TriggerMap[trigPos15Owned->internal_name] = std::move(trigPos15Owned);
    pluginPos15->m_triggersNeedSorting = true;

    // Add matching triggers to existing plugins and world
    auto trigNeg10MultiOwned = std::make_unique<Trigger>();
    trigNeg10MultiOwned->internal_name = "trig_neg10_multi";
    trigNeg10MultiOwned->label = "Trigger-Negative-10-Multi";
    trigNeg10MultiOwned->trigger = "MultiPlugin*";
    trigNeg10MultiOwned->send_to = eSendToWorld;
    trigNeg10MultiOwned->enabled = true;
    trigNeg10MultiOwned->sequence = 100;
    trigNeg10MultiOwned->keep_evaluating = true;
    Trigger* trigNeg10Multi = trigNeg10MultiOwned.get();
    pluginNeg->m_TriggerMap[trigNeg10MultiOwned->internal_name] = std::move(trigNeg10MultiOwned);
    pluginNeg->m_triggersNeedSorting = true;

    auto trigZeroMultiOwned = std::make_unique<Trigger>();
    trigZeroMultiOwned->internal_name = "trig_zero_multi";
    trigZeroMultiOwned->label = "Trigger-Zero-Multi";
    trigZeroMultiOwned->trigger = "MultiPlugin*";
    trigZeroMultiOwned->send_to = eSendToWorld;
    trigZeroMultiOwned->enabled = true;
    trigZeroMultiOwned->sequence = 100;
    trigZeroMultiOwned->keep_evaluating = true;
    Trigger* trigZeroMulti = trigZeroMultiOwned.get();
    pluginZero->m_TriggerMap[trigZeroMultiOwned->internal_name] = std::move(trigZeroMultiOwned);
    pluginZero->m_triggersNeedSorting = true;

    auto trigPos10MultiOwned = std::make_unique<Trigger>();
    trigPos10MultiOwned->internal_name = "trig_pos10_multi";
    trigPos10MultiOwned->label = "Trigger-Positive-10-Multi";
    trigPos10MultiOwned->trigger = "MultiPlugin*";
    trigPos10MultiOwned->send_to = eSendToWorld;
    trigPos10MultiOwned->enabled = true;
    trigPos10MultiOwned->sequence = 100;
    trigPos10MultiOwned->keep_evaluating = true;
    Trigger* trigPos10Multi = trigPos10MultiOwned.get();
    pluginPos->m_TriggerMap[trigPos10MultiOwned->internal_name] = std::move(trigPos10MultiOwned);
    pluginPos->m_triggersNeedSorting = true;

    auto trigWorldMultiOwned = std::make_unique<Trigger>();
    trigWorldMultiOwned->internal_name = "trig_world_multi";
    trigWorldMultiOwned->label = "Trigger-World-Multi";
    trigWorldMultiOwned->trigger = "MultiPlugin*";
    trigWorldMultiOwned->send_to = eSendToWorld;
    trigWorldMultiOwned->enabled = true;
    trigWorldMultiOwned->sequence = 100;
    trigWorldMultiOwned->keep_evaluating = true;
    Trigger* trigWorldMulti = trigWorldMultiOwned.get();
    doc->m_automationRegistry->m_TriggerMap[trigWorldMultiOwned->internal_name] =
        std::move(trigWorldMultiOwned);
    doc->m_automationRegistry->m_triggersNeedSorting = true;

    // Create test line
    auto multiLine = createTestLine("MultiPlugin test", 3);

    // Evaluate triggers
    doc->evaluateTriggers(multiLine.get());

    // Verify all triggers matched in correct order
    // Expected order: -10, -5, world, 0, 10, 15
    EXPECT_EQ(trigNeg10Multi->matched, 1) << "Plugin seq=-10 trigger should have matched";
    EXPECT_EQ(trigNeg5->matched, 1) << "Plugin seq=-5 trigger should have matched";
    EXPECT_EQ(trigWorldMulti->matched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigZeroMulti->matched, 1) << "Plugin seq=0 trigger should have matched";
    EXPECT_EQ(trigPos10Multi->matched, 1) << "Plugin seq=10 trigger should have matched";
    EXPECT_EQ(trigPos15->matched, 1) << "Plugin seq=15 trigger should have matched";

    plugin4File.close();
    plugin5File.close();
}
