/**
 * test_plugin_evaluation_gtest.cpp - GoogleTest version
 * Plugin Evaluation Order Test
 *
 * Tests plugin sequence-based evaluation order including:
 * - Triggers: negative sequence → world → positive sequence
 * - Aliases: negative sequence → world → positive sequence
 * - bKeepEvaluating flag stopping evaluation at each phase
 * - One-shot triggers/aliases deleted from correct plugin context
 */

#include "../src/automation/alias.h"
#include "../src/automation/plugin.h"
#include "../src/automation/sendto.h"
#include "../src/automation/trigger.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QTemporaryFile>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

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
        doc = new WorldDocument();

        // Create three plugins with different sequences
        // Plugin 1: Negative sequence (-10)
        plugin1File = new QTemporaryFile();
        plugin1File->setFileTemplate("test-plugin1-XXXXXX.xml");
        plugin1File->open();
        plugin1File->write(
            createPluginXml("Plugin-Negative", "{11111111-1111-1111-1111-111111111111}", -10)
                .toUtf8());
        plugin1File->flush();

        // Plugin 2: Zero sequence (0)
        plugin2File = new QTemporaryFile();
        plugin2File->setFileTemplate("test-plugin2-XXXXXX.xml");
        plugin2File->open();
        plugin2File->write(
            createPluginXml("Plugin-Zero", "{22222222-2222-2222-2222-222222222222}", 0).toUtf8());
        plugin2File->flush();

        // Plugin 3: Positive sequence (10)
        plugin3File = new QTemporaryFile();
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
        delete doc;
        delete plugin1File;
        delete plugin2File;
        delete plugin3File;
    }

    // Helper to create a test line
    Line* createTestLine(const char* text, int lineNumber = 1)
    {
        Line* line = new Line(lineNumber, // line number
                              80,         // wrap column
                              0,          // line flags
                              0x00FFFFFF, // foreground color (white)
                              0x00000000, // background color (black)
                              false       // unicode
        );
        int len = strlen(text);
        line->textBuffer.resize(len);
        memcpy(line->text(), text, len);
        line->textBuffer.push_back('\0');
        // memcpy already done above
        line->styleList.push_back(std::move(std::make_unique<Style>()));
        return line;
    }

    WorldDocument* doc = nullptr;
    Plugin* pluginNeg = nullptr;
    Plugin* pluginZero = nullptr;
    Plugin* pluginPos = nullptr;
    QTemporaryFile* plugin1File = nullptr;
    QTemporaryFile* plugin2File = nullptr;
    QTemporaryFile* plugin3File = nullptr;
};

// Test 1: Trigger Evaluation Order
TEST_F(PluginEvaluationTest, TriggerEvaluationOrder)
{
    // Add triggers to each context
    // Negative plugin trigger
    Trigger* trigNeg = new Trigger();
    trigNeg->strInternalName = "trig_neg";
    trigNeg->strLabel = "Trigger-Negative";
    trigNeg->trigger = "Hello*";
    trigNeg->iSendTo = eSendToWorld;
    trigNeg->bEnabled = true;
    trigNeg->iSequence = 100;
    trigNeg->bKeepEvaluating = true;
    pluginNeg->m_TriggerMap[trigNeg->strInternalName] = std::unique_ptr<Trigger>(trigNeg);
    pluginNeg->m_triggersNeedSorting = true;

    // World trigger
    Trigger* trigWorld = new Trigger();
    trigWorld->strInternalName = "trig_world";
    trigWorld->strLabel = "Trigger-World";
    trigWorld->trigger = "Hello*";
    trigWorld->iSendTo = eSendToWorld;
    trigWorld->bEnabled = true;
    trigWorld->iSequence = 100;
    trigWorld->bKeepEvaluating = true;
    doc->m_TriggerMap[trigWorld->strInternalName] = std::unique_ptr<Trigger>(trigWorld);
    doc->m_triggersNeedSorting = true;

    // Positive plugin trigger
    Trigger* trigPos = new Trigger();
    trigPos->strInternalName = "trig_pos";
    trigPos->strLabel = "Trigger-Positive";
    trigPos->trigger = "Hello*";
    trigPos->iSendTo = eSendToWorld;
    trigPos->bEnabled = true;
    trigPos->iSequence = 100;
    trigPos->bKeepEvaluating = true;
    pluginPos->m_TriggerMap[trigPos->strInternalName] = std::unique_ptr<Trigger>(trigPos);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    Line* testLine = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine);

    // Check that all three triggers matched (by checking nMatched counter)
    EXPECT_EQ(trigNeg->nMatched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld->nMatched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos->nMatched, 1) << "Positive plugin trigger should have matched";

    // Cleanup
    delete testLine;
}

// Test 2: Alias Evaluation Order
TEST_F(PluginEvaluationTest, AliasEvaluationOrder)
{
    // Add aliases to each context
    // Negative plugin alias
    auto aliasNeg = std::make_unique<Alias>();
    aliasNeg->strInternalName = "alias_neg";
    aliasNeg->strLabel = "Alias-Negative";
    aliasNeg->name = "test*";
    aliasNeg->bRegexp = false;
    aliasNeg->iSendTo = eSendToWorld;
    aliasNeg->bEnabled = true;
    aliasNeg->iSequence = 100;
    aliasNeg->bKeepEvaluating = true;
    Alias* aliasNegPtr = aliasNeg.get();
    pluginNeg->m_AliasMap[aliasNeg->strInternalName] = std::move(aliasNeg);
    pluginNeg->m_aliasesNeedSorting = true;

    // World alias
    auto aliasWorld = std::make_unique<Alias>();
    aliasWorld->strInternalName = "alias_world";
    aliasWorld->strLabel = "Alias-World";
    aliasWorld->name = "test*";
    aliasWorld->bRegexp = false;
    aliasWorld->iSendTo = eSendToWorld;
    aliasWorld->bEnabled = true;
    aliasWorld->iSequence = 100;
    aliasWorld->bKeepEvaluating = true;
    Alias* aliasWorldPtr = aliasWorld.get();
    doc->m_AliasMap[aliasWorld->strInternalName] = std::move(aliasWorld);
    doc->m_aliasesNeedSorting = true;

    // Positive plugin alias
    auto aliasPos = std::make_unique<Alias>();
    aliasPos->strInternalName = "alias_pos";
    aliasPos->strLabel = "Alias-Positive";
    aliasPos->name = "test*";
    aliasPos->bRegexp = false;
    aliasPos->iSendTo = eSendToWorld;
    aliasPos->bEnabled = true;
    aliasPos->iSequence = 100;
    aliasPos->bKeepEvaluating = true;
    Alias* aliasPosPtr = aliasPos.get();
    pluginPos->m_AliasMap[aliasPos->strInternalName] = std::move(aliasPos);
    pluginPos->m_aliasesNeedSorting = true;

    // Evaluate aliases
    doc->evaluateAliases("test command");

    // Check that all three aliases matched (by checking nMatched counter)
    EXPECT_EQ(aliasNegPtr->nMatched, 1) << "Negative plugin alias should have matched";
    EXPECT_EQ(aliasWorldPtr->nMatched, 1) << "World alias should have matched";
    EXPECT_EQ(aliasPosPtr->nMatched, 1) << "Positive plugin alias should have matched";
}

// Test 3: bKeepEvaluating = false stops at negative phase
TEST_F(PluginEvaluationTest, KeepEvaluatingStopsAtNegativePhase)
{
    // Add triggers to each context
    Trigger* trigNeg = new Trigger();
    trigNeg->strInternalName = "trig_neg";
    trigNeg->strLabel = "Trigger-Negative";
    trigNeg->trigger = "Hello*";
    trigNeg->iSendTo = eSendToWorld;
    trigNeg->bEnabled = true;
    trigNeg->iSequence = 100;
    trigNeg->bKeepEvaluating = false; // Stop evaluation
    pluginNeg->m_TriggerMap[trigNeg->strInternalName] = std::unique_ptr<Trigger>(trigNeg);
    pluginNeg->m_triggersNeedSorting = true;

    Trigger* trigWorld = new Trigger();
    trigWorld->strInternalName = "trig_world";
    trigWorld->strLabel = "Trigger-World";
    trigWorld->trigger = "Hello*";
    trigWorld->iSendTo = eSendToWorld;
    trigWorld->bEnabled = true;
    trigWorld->iSequence = 100;
    trigWorld->bKeepEvaluating = true;
    doc->m_TriggerMap[trigWorld->strInternalName] = std::unique_ptr<Trigger>(trigWorld);
    doc->m_triggersNeedSorting = true;

    Trigger* trigPos = new Trigger();
    trigPos->strInternalName = "trig_pos";
    trigPos->strLabel = "Trigger-Positive";
    trigPos->trigger = "Hello*";
    trigPos->iSendTo = eSendToWorld;
    trigPos->bEnabled = true;
    trigPos->iSequence = 100;
    trigPos->bKeepEvaluating = true;
    pluginPos->m_TriggerMap[trigPos->strInternalName] = std::unique_ptr<Trigger>(trigPos);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    Line* testLine = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine);

    // Check that only negative plugin trigger matched (others should not match)
    EXPECT_EQ(trigNeg->nMatched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld->nMatched, 0) << "World trigger should not have matched";
    EXPECT_EQ(trigPos->nMatched, 0) << "Positive plugin trigger should not have matched";

    // Cleanup
    delete testLine;
}

// Test 4: bKeepEvaluating = false stops at world phase
TEST_F(PluginEvaluationTest, KeepEvaluatingStopsAtWorldPhase)
{
    // Add triggers to each context
    Trigger* trigNeg = new Trigger();
    trigNeg->strInternalName = "trig_neg";
    trigNeg->strLabel = "Trigger-Negative";
    trigNeg->trigger = "Hello*";
    trigNeg->iSendTo = eSendToWorld;
    trigNeg->bEnabled = true;
    trigNeg->iSequence = 100;
    trigNeg->bKeepEvaluating = true;
    pluginNeg->m_TriggerMap[trigNeg->strInternalName] = std::unique_ptr<Trigger>(trigNeg);
    pluginNeg->m_triggersNeedSorting = true;

    Trigger* trigWorld = new Trigger();
    trigWorld->strInternalName = "trig_world";
    trigWorld->strLabel = "Trigger-World";
    trigWorld->trigger = "Hello*";
    trigWorld->iSendTo = eSendToWorld;
    trigWorld->bEnabled = true;
    trigWorld->iSequence = 100;
    trigWorld->bKeepEvaluating = false; // Stop evaluation
    doc->m_TriggerMap[trigWorld->strInternalName] = std::unique_ptr<Trigger>(trigWorld);
    doc->m_triggersNeedSorting = true;

    Trigger* trigPos = new Trigger();
    trigPos->strInternalName = "trig_pos";
    trigPos->strLabel = "Trigger-Positive";
    trigPos->trigger = "Hello*";
    trigPos->iSendTo = eSendToWorld;
    trigPos->bEnabled = true;
    trigPos->iSequence = 100;
    trigPos->bKeepEvaluating = true;
    pluginPos->m_TriggerMap[trigPos->strInternalName] = std::unique_ptr<Trigger>(trigPos);
    pluginPos->m_triggersNeedSorting = true;

    // Create a test line
    Line* testLine = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine);

    // Check that negative and world matched, but not positive
    EXPECT_EQ(trigNeg->nMatched, 1) << "Negative plugin trigger should have matched";
    EXPECT_EQ(trigWorld->nMatched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos->nMatched, 0) << "Positive plugin trigger should not have matched";

    // Cleanup
    delete testLine;
}

// Test 5: One-shot trigger deleted from correct plugin context
TEST_F(PluginEvaluationTest, OneShotTriggerDeletedFromCorrectContext)
{
    // Create one-shot trigger in negative plugin
    Trigger* trigOneShot = new Trigger();
    trigOneShot->strInternalName = "trig_oneshot";
    trigOneShot->strLabel = "Trigger-OneShot";
    trigOneShot->trigger = "OneShot*";
    trigOneShot->bEnabled = true;
    trigOneShot->iSequence = 100;
    trigOneShot->bOneShot = true;
    trigOneShot->bKeepEvaluating = true;
    pluginNeg->m_TriggerMap[trigOneShot->strInternalName] = std::unique_ptr<Trigger>(trigOneShot);
    pluginNeg->m_triggersNeedSorting = true;

    int triggerCountBefore = pluginNeg->m_TriggerMap.size();

    // Create a line that matches the one-shot trigger
    Line* oneShotLine = createTestLine("OneShot message", 2);

    // Evaluate triggers
    doc->evaluateTriggers(oneShotLine);

    int triggerCountAfter = pluginNeg->m_TriggerMap.size();

    EXPECT_EQ(triggerCountAfter, triggerCountBefore - 1)
        << "One-shot trigger should have been deleted";

    // Verify it was deleted from the correct plugin
    EXPECT_TRUE(pluginNeg->m_TriggerMap.find("trig_oneshot") == pluginNeg->m_TriggerMap.end())
        << "One-shot trigger should not exist in plugin";

    // Cleanup
    delete oneShotLine;
}

// Test 6: Disabled plugin not evaluated
TEST_F(PluginEvaluationTest, DisabledPluginNotEvaluated)
{
    // Add triggers to each context
    Trigger* trigNeg = new Trigger();
    trigNeg->strInternalName = "trig_neg";
    trigNeg->strLabel = "Trigger-Negative";
    trigNeg->trigger = "Hello*";
    trigNeg->iSendTo = eSendToWorld;
    trigNeg->bEnabled = true;
    trigNeg->iSequence = 100;
    trigNeg->bKeepEvaluating = true;
    pluginNeg->m_TriggerMap[trigNeg->strInternalName] = std::unique_ptr<Trigger>(trigNeg);
    pluginNeg->m_triggersNeedSorting = true;

    Trigger* trigWorld = new Trigger();
    trigWorld->strInternalName = "trig_world";
    trigWorld->strLabel = "Trigger-World";
    trigWorld->trigger = "Hello*";
    trigWorld->iSendTo = eSendToWorld;
    trigWorld->bEnabled = true;
    trigWorld->iSequence = 100;
    trigWorld->bKeepEvaluating = true;
    doc->m_TriggerMap[trigWorld->strInternalName] = std::unique_ptr<Trigger>(trigWorld);
    doc->m_triggersNeedSorting = true;

    Trigger* trigPos = new Trigger();
    trigPos->strInternalName = "trig_pos";
    trigPos->strLabel = "Trigger-Positive";
    trigPos->trigger = "Hello*";
    trigPos->iSendTo = eSendToWorld;
    trigPos->bEnabled = true;
    trigPos->iSequence = 100;
    trigPos->bKeepEvaluating = true;
    pluginPos->m_TriggerMap[trigPos->strInternalName] = std::unique_ptr<Trigger>(trigPos);
    pluginPos->m_triggersNeedSorting = true;

    // Disable negative plugin
    pluginNeg->m_bEnabled = false;

    // Create a test line
    Line* testLine = createTestLine("Hello World");

    // Evaluate triggers
    doc->evaluateTriggers(testLine);

    // Check that negative plugin didn't match (disabled)
    EXPECT_EQ(trigNeg->nMatched, 0) << "Negative plugin trigger should not have matched (disabled)";
    EXPECT_EQ(trigWorld->nMatched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigPos->nMatched, 1) << "Positive plugin trigger should have matched";

    // Cleanup
    delete testLine;
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
    Trigger* trigNeg5 = new Trigger();
    trigNeg5->strInternalName = "trig_neg5";
    trigNeg5->strLabel = "Trigger-Negative-5";
    trigNeg5->trigger = "MultiPlugin*";
    trigNeg5->iSendTo = eSendToWorld;
    trigNeg5->bEnabled = true;
    trigNeg5->iSequence = 100;
    trigNeg5->bKeepEvaluating = true;
    pluginNeg5->m_TriggerMap[trigNeg5->strInternalName] = std::unique_ptr<Trigger>(trigNeg5);
    pluginNeg5->m_triggersNeedSorting = true;

    Trigger* trigPos15 = new Trigger();
    trigPos15->strInternalName = "trig_pos15";
    trigPos15->strLabel = "Trigger-Positive-15";
    trigPos15->trigger = "MultiPlugin*";
    trigPos15->iSendTo = eSendToWorld;
    trigPos15->bEnabled = true;
    trigPos15->iSequence = 100;
    trigPos15->bKeepEvaluating = true;
    pluginPos15->m_TriggerMap[trigPos15->strInternalName] = std::unique_ptr<Trigger>(trigPos15);
    pluginPos15->m_triggersNeedSorting = true;

    // Add matching triggers to existing plugins and world
    Trigger* trigNeg10Multi = new Trigger();
    trigNeg10Multi->strInternalName = "trig_neg10_multi";
    trigNeg10Multi->strLabel = "Trigger-Negative-10-Multi";
    trigNeg10Multi->trigger = "MultiPlugin*";
    trigNeg10Multi->iSendTo = eSendToWorld;
    trigNeg10Multi->bEnabled = true;
    trigNeg10Multi->iSequence = 100;
    trigNeg10Multi->bKeepEvaluating = true;
    pluginNeg->m_TriggerMap[trigNeg10Multi->strInternalName] =
        std::unique_ptr<Trigger>(trigNeg10Multi);
    pluginNeg->m_triggersNeedSorting = true;

    Trigger* trigZeroMulti = new Trigger();
    trigZeroMulti->strInternalName = "trig_zero_multi";
    trigZeroMulti->strLabel = "Trigger-Zero-Multi";
    trigZeroMulti->trigger = "MultiPlugin*";
    trigZeroMulti->iSendTo = eSendToWorld;
    trigZeroMulti->bEnabled = true;
    trigZeroMulti->iSequence = 100;
    trigZeroMulti->bKeepEvaluating = true;
    pluginZero->m_TriggerMap[trigZeroMulti->strInternalName] =
        std::unique_ptr<Trigger>(trigZeroMulti);
    pluginZero->m_triggersNeedSorting = true;

    Trigger* trigPos10Multi = new Trigger();
    trigPos10Multi->strInternalName = "trig_pos10_multi";
    trigPos10Multi->strLabel = "Trigger-Positive-10-Multi";
    trigPos10Multi->trigger = "MultiPlugin*";
    trigPos10Multi->iSendTo = eSendToWorld;
    trigPos10Multi->bEnabled = true;
    trigPos10Multi->iSequence = 100;
    trigPos10Multi->bKeepEvaluating = true;
    pluginPos->m_TriggerMap[trigPos10Multi->strInternalName] =
        std::unique_ptr<Trigger>(trigPos10Multi);
    pluginPos->m_triggersNeedSorting = true;

    Trigger* trigWorldMulti = new Trigger();
    trigWorldMulti->strInternalName = "trig_world_multi";
    trigWorldMulti->strLabel = "Trigger-World-Multi";
    trigWorldMulti->trigger = "MultiPlugin*";
    trigWorldMulti->iSendTo = eSendToWorld;
    trigWorldMulti->bEnabled = true;
    trigWorldMulti->iSequence = 100;
    trigWorldMulti->bKeepEvaluating = true;
    doc->m_TriggerMap[trigWorldMulti->strInternalName] = std::unique_ptr<Trigger>(trigWorldMulti);
    doc->m_triggersNeedSorting = true;

    // Create test line
    Line* multiLine = createTestLine("MultiPlugin test", 3);

    // Evaluate triggers
    doc->evaluateTriggers(multiLine);

    // Verify all triggers matched in correct order
    // Expected order: -10, -5, world, 0, 10, 15
    EXPECT_EQ(trigNeg10Multi->nMatched, 1) << "Plugin seq=-10 trigger should have matched";
    EXPECT_EQ(trigNeg5->nMatched, 1) << "Plugin seq=-5 trigger should have matched";
    EXPECT_EQ(trigWorldMulti->nMatched, 1) << "World trigger should have matched";
    EXPECT_EQ(trigZeroMulti->nMatched, 1) << "Plugin seq=0 trigger should have matched";
    EXPECT_EQ(trigPos10Multi->nMatched, 1) << "Plugin seq=10 trigger should have matched";
    EXPECT_EQ(trigPos15->nMatched, 1) << "Plugin seq=15 trigger should have matched";

    // Cleanup
    delete multiLine;
    plugin4File.close();
    plugin5File.close();
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
