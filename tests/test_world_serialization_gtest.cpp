// test_world_serialization_gtest.cpp - GoogleTest version
// World Serialization Tests
//
// Verifies timer at-time round-trips, variable handling, ExportXML/ImportXML,
// accelerator/macro/keypad persistence, temporary item filtering, CDATA preservation,
// default-value omission, and loaded-value snapshots.

#include "../src/automation/alias.h"
#include "../src/automation/timer.h"
#include "../src/automation/trigger.h"
#include "../src/automation/variable.h"
#include "../src/world/accelerator_manager.h"
#include "../src/world/macro_keypad_compat.h"
#include "../src/world/world_document.h"
#include "../src/world/xml_serialization.h"
#include <QDir>
#include <QFile>
#include <QUuid>
#include <gtest/gtest.h>

static void cleanupSaveFiles(const QString& filename)
{
    QFile::remove(filename);
    QFile::remove(filename + ".tmp");
    QFile::remove(filename + ".bak");
}

static QString generateTempFilename(const QString& prefix = "ws")
{
    QString tempDir = QDir::tempPath();
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    return tempDir + "/" + prefix + "_" + uuid + ".mcl";
}

class WorldSerializationTest : public ::testing::Test {
  protected:
    void TearDown() override
    {
        if (!tmpFilename.isEmpty()) {
            cleanupSaveFiles(tmpFilename);
            tmpFilename.clear();
        }
    }

    QString saveAndReadXml(WorldDocument* doc, const QString& prefix = "ws")
    {
        tmpFilename = generateTempFilename(prefix);
        EXPECT_TRUE(XmlSerialization::SaveWorldXML(doc, tmpFilename)) << "Failed to save XML";
        QFile file(tmpFilename);
        EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open saved file";
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        return content;
    }

    std::unique_ptr<WorldDocument> doc1;
    std::unique_ptr<WorldDocument> doc2;
    QString tmpFilename;
};

// =============================================================================
// Category 1: Timer AtTime Round-trip
// =============================================================================

// Test 1: AtTime timer saves/loads with correct at_time="y" and at_hour/minute/second
TEST_F(WorldSerializationTest, TimerAtTimeRoundtrip)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "AtTime Timer Test";

    auto timerOwned = std::make_unique<Timer>();
    timerOwned->label = "at_timer";
    timerOwned->enabled = true;
    timerOwned->type = Timer::TimerType::AtTime;
    timerOwned->at_hour = 14;
    timerOwned->at_minute = 30;
    timerOwned->at_second = 15.5;
    timerOwned->contents = "say Good afternoon!";
    timerOwned->send_to = eSendToWorld;

    EXPECT_TRUE(doc1->addTimer("at_timer", std::move(timerOwned)).has_value());

    QString content = saveAndReadXml(doc1.get());

    EXPECT_TRUE(content.contains("at_time=\"y\"")) << "Missing at_time=\"y\" for AtTime timer";
    EXPECT_TRUE(content.contains("hour=\"14\"")) << "Missing hour=\"14\"";
    EXPECT_TRUE(content.contains("minute=\"30\"")) << "Missing minute=\"30\"";
    EXPECT_TRUE(content.contains("second=\"15.5")) << "Missing second=\"15.5\"";

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename)) << "Failed to load XML";

    Timer* loaded = doc2->getTimer("at_timer");
    ASSERT_NE(loaded, nullptr) << "AtTime timer not found after load";
    EXPECT_EQ(loaded->type, Timer::TimerType::AtTime);
    EXPECT_EQ(loaded->at_hour, 14);
    EXPECT_EQ(loaded->at_minute, 30);
    EXPECT_DOUBLE_EQ(loaded->at_second, 15.5);
}

// Test 2: AtTime timer round-trip does not pollute interval fields
TEST_F(WorldSerializationTest, TimerAtTimeDoesNotPolluteIntervalFields)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "AtTime Isolation Test";

    auto timerOwned = std::make_unique<Timer>();
    timerOwned->label = "at_timer2";
    timerOwned->enabled = true;
    timerOwned->type = Timer::TimerType::AtTime;
    timerOwned->at_hour = 8;
    timerOwned->at_minute = 0;
    timerOwned->at_second = 0.0;
    timerOwned->every_hour = 0;
    timerOwned->every_minute = 0;
    timerOwned->every_second = 0.0;

    EXPECT_TRUE(doc1->addTimer("at_timer2", std::move(timerOwned)).has_value());

    saveAndReadXml(doc1.get());

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    Timer* loaded = doc2->getTimer("at_timer2");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->every_hour, 0) << "every_hour should be 0 after AtTime load";
    EXPECT_EQ(loaded->every_minute, 0) << "every_minute should be 0 after AtTime load";
    EXPECT_DOUBLE_EQ(loaded->every_second, 0.0) << "every_second should be 0.0 after AtTime load";
}

// =============================================================================
// Category 2: Variable Round-trip
// =============================================================================

// Test 3: Variable saves and loads correctly via file
TEST_F(WorldSerializationTest, VariableRoundtripViaFile)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Variable Roundtrip Test";

    auto var = std::make_unique<Variable>();
    var->label = "myvar";
    var->contents = "hello world";
    doc1->m_VariableMap["myvar"] = std::move(var);

    saveAndReadXml(doc1.get());

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    auto it = doc2->m_VariableMap.find("myvar");
    ASSERT_NE(it, doc2->m_VariableMap.end()) << "Variable 'myvar' not found after load";
    EXPECT_EQ(it->second->contents, "hello world");
}

// Test 4: Variable name is lowercased on load
TEST_F(WorldSerializationTest, VariableNameLowercasedOnLoad)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Variable Case Test";

    auto var = std::make_unique<Variable>();
    var->label = "myvar";
    var->contents = "case test value";
    doc1->m_VariableMap["myvar"] = std::move(var);

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_VARIABLES);
    ASSERT_FALSE(xmlStr.isEmpty()) << "ExportXML returned empty string";

    // Manually inject mixed-case variable name into the exported XML
    QString mixedCaseXml = xmlStr;
    mixedCaseXml.replace("name=\"myvar\"", "name=\"MyVar\"");

    doc2 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc2.get(), mixedCaseXml, XML_VARIABLES);
    EXPECT_GE(imported, 0) << "ImportXML failed on mixed-case variable";

    auto it = doc2->m_VariableMap.find("myvar");
    EXPECT_NE(it, doc2->m_VariableMap.end())
        << "Variable key should be lowercased to 'myvar' after import";
}

// Test 5: Duplicate variable names on import keep only the first
TEST_F(WorldSerializationTest, VariableDuplicateSkippedOnImport)
{
    // Build XML manually with two variables sharing the same (lowercased) name
    // but different contents. The second should be skipped.
    QString xmlStr = R"(<?xml version="1.0" encoding="UTF-8"?>
<muclient>
<variables>
<variable name="dupvar"><![CDATA[first_value]]></variable>
<variable name="dupvar"><![CDATA[second_value]]></variable>
</variables>
</muclient>)";

    doc1 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc1.get(), xmlStr, XML_VARIABLES);
    EXPECT_GE(imported, 0) << "ImportXML failed";

    auto it = doc1->m_VariableMap.find("dupvar");
    ASSERT_NE(it, doc1->m_VariableMap.end()) << "Variable 'dupvar' not found";
    EXPECT_EQ(it->second->contents, "first_value")
        << "Duplicate variable should keep first value, not second";
}

// =============================================================================
// Category 3: ExportXML / ImportXML
// =============================================================================

// Test 6: Export triggers to XML string, import into fresh doc
TEST_F(WorldSerializationTest, ExportImportTriggersViaString)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Export Trigger Test";

    auto trig = std::make_unique<Trigger>();
    trig->label = "export_trigger";
    trig->internal_name = "export_trigger";
    trig->trigger = "You feel hungry";
    trig->contents = "eat bread";
    trig->enabled = true;
    trig->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTrigger("export_trigger", std::move(trig)).has_value());

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_TRIGGERS);
    ASSERT_FALSE(xmlStr.isEmpty()) << "ExportXML returned empty string";

    doc2 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_TRIGGERS);
    EXPECT_GE(imported, 1) << "ImportXML should have imported at least 1 trigger";

    Trigger* loaded = doc2->getTrigger("export_trigger");
    ASSERT_NE(loaded, nullptr) << "Trigger not found in doc2 after import";
    EXPECT_EQ(loaded->trigger, "You feel hungry");
    EXPECT_EQ(loaded->contents, "eat bread");
}

// Test 7: Export variables to XML string, import into fresh doc
TEST_F(WorldSerializationTest, ExportImportVariablesViaString)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Export Variable Test";

    auto var = std::make_unique<Variable>();
    var->label = "export_var";
    var->contents = "exported_value";
    doc1->m_VariableMap["export_var"] = std::move(var);

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_VARIABLES);
    ASSERT_FALSE(xmlStr.isEmpty()) << "ExportXML returned empty string";

    doc2 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_VARIABLES);
    EXPECT_GE(imported, 1) << "ImportXML should have imported at least 1 variable";

    auto it = doc2->m_VariableMap.find("export_var");
    ASSERT_NE(it, doc2->m_VariableMap.end()) << "Variable 'export_var' not found in doc2";
    EXPECT_EQ(it->second->contents, "exported_value");
}

// Test 8: Selective import with XML_TRIGGERS flag only imports triggers, not aliases
TEST_F(WorldSerializationTest, ImportSelectiveFlagsOnlyImportsTriggers)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Selective Import Test";

    auto trig = std::make_unique<Trigger>();
    trig->label = "selective_trigger";
    trig->internal_name = "selective_trigger";
    trig->trigger = "test trigger pattern";
    trig->enabled = true;
    trig->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTrigger("selective_trigger", std::move(trig)).has_value());

    auto alias = std::make_unique<Alias>();
    alias->label = "selective_alias";
    alias->internal_name = "selective_alias";
    alias->name = "sa";
    alias->contents = "south";
    alias->enabled = true;
    alias->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addAlias("selective_alias", std::move(alias)).has_value());

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_ALL);
    ASSERT_FALSE(xmlStr.isEmpty());

    doc2 = std::make_unique<WorldDocument>();
    XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_TRIGGERS);

    EXPECT_NE(doc2->getTrigger("selective_trigger"), nullptr)
        << "Trigger should be present after XML_TRIGGERS import";
    EXPECT_EQ(doc2->getAlias("selective_alias"), nullptr)
        << "Alias should be absent when only XML_TRIGGERS flag used";
}

// Test 9: Selective import with XML_ALIASES flag only imports aliases, not triggers
TEST_F(WorldSerializationTest, ImportSelectiveFlagsOnlyImportsAliases)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Selective Alias Import Test";

    auto trig = std::make_unique<Trigger>();
    trig->label = "alias_test_trigger";
    trig->internal_name = "alias_test_trigger";
    trig->trigger = "another pattern";
    trig->enabled = true;
    trig->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTrigger("alias_test_trigger", std::move(trig)).has_value());

    auto alias = std::make_unique<Alias>();
    alias->label = "alias_test_alias";
    alias->internal_name = "alias_test_alias";
    alias->name = "n";
    alias->contents = "north";
    alias->enabled = true;
    alias->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addAlias("alias_test_alias", std::move(alias)).has_value());

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_ALL);
    ASSERT_FALSE(xmlStr.isEmpty());

    doc2 = std::make_unique<WorldDocument>();
    XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_ALIASES);

    EXPECT_NE(doc2->getAlias("alias_test_alias"), nullptr)
        << "Alias should be present after XML_ALIASES import";
    EXPECT_EQ(doc2->getTrigger("alias_test_trigger"), nullptr)
        << "Trigger should be absent when only XML_ALIASES flag used";
}

// Test 10: ImportXML returns -1 on invalid/empty input
TEST_F(WorldSerializationTest, ImportXMLReturnsMinus1OnInvalidInput)
{
    doc1 = std::make_unique<WorldDocument>();

    int result1 = XmlSerialization::ImportXML(doc1.get(), "not xml at all");
    EXPECT_EQ(result1, -1) << "ImportXML should return -1 for non-XML string";

    int result2 = XmlSerialization::ImportXML(doc1.get(), "");
    EXPECT_EQ(result2, -1) << "ImportXML should return -1 for empty string";
}

// =============================================================================
// Category 4: Accelerator / Macro / Keypad Round-trip
// =============================================================================

// Test 11: User accelerator (non-macro, non-keypad) saves as <accelerators> and loads back
TEST_F(WorldSerializationTest, AcceleratorRoundtripViaFile)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Accelerator Test";

    // "Ctrl+Shift+G" is not a macro key and not a keypad key
    ASSERT_FALSE(MacroKeypadCompat::isMacroKey("Ctrl+Shift+G"));
    ASSERT_FALSE(MacroKeypadCompat::isKeypadKey("Ctrl+Shift+G"));

    doc1->m_acceleratorManager->addKeyBinding("Ctrl+Shift+G", "say hello", 0);

    QString content = saveAndReadXml(doc1.get());

    EXPECT_TRUE(content.contains("<accelerators")) << "XML should contain <accelerators> element";

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    QVector<AcceleratorEntry> bindings = doc2->m_acceleratorManager->keyBindingList();
    bool found = false;
    for (const AcceleratorEntry& entry : bindings) {
        if (entry.keyString == "Ctrl+Shift+G") {
            found = true;
            EXPECT_EQ(entry.action, "say hello");
            break;
        }
    }
    EXPECT_TRUE(found) << "Ctrl+Shift+G accelerator not found in doc2 after load";
}

// Test 12: Macro key (F2) exports as <macros> via ExportXML and imports back
TEST_F(WorldSerializationTest, MacroRoundtripViaExportImport)
{
    doc1 = std::make_unique<WorldDocument>();

    // "F2" is a macro key
    ASSERT_TRUE(MacroKeypadCompat::isMacroKey("F2"));

    doc1->m_acceleratorManager->addKeyBinding("F2", "look", 0);

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_MACROS);
    ASSERT_FALSE(xmlStr.isEmpty());

    EXPECT_TRUE(xmlStr.contains("<macros")) << "XML should contain <macros> element";
    EXPECT_TRUE(xmlStr.contains("<macro")) << "XML should contain <macro> element";

    doc2 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_MACROS);
    EXPECT_GE(imported, 0) << "ImportXML should not fail";

    QVector<AcceleratorEntry> bindings = doc2->m_acceleratorManager->keyBindingList();
    bool found = false;
    for (const AcceleratorEntry& entry : bindings) {
        if (entry.keyString == "F2") {
            found = true;
            EXPECT_EQ(entry.action, "look");
            break;
        }
    }
    EXPECT_TRUE(found) << "F2 macro not found in doc2 after import";
}

// Test 13: Keypad key (numpad 8) exports as <keypad> via ExportXML and imports back
TEST_F(WorldSerializationTest, KeypadRoundtripViaExportImport)
{
    doc1 = std::make_unique<WorldDocument>();

    QString keyString = MacroKeypadCompat::keypadNameToKeyString("8");
    ASSERT_FALSE(keyString.isEmpty()) << "keypadNameToKeyString('8') should not return empty";
    ASSERT_TRUE(MacroKeypadCompat::isKeypadKey(keyString));

    doc1->m_acceleratorManager->addKeyBinding(keyString, "north", 0);

    QString xmlStr = XmlSerialization::ExportXML(doc1.get(), XML_MACROS);
    ASSERT_FALSE(xmlStr.isEmpty());

    EXPECT_TRUE(xmlStr.contains("<keypad")) << "XML should contain <keypad> element";
    EXPECT_TRUE(xmlStr.contains("<key")) << "XML should contain <key> element";

    doc2 = std::make_unique<WorldDocument>();
    int imported = XmlSerialization::ImportXML(doc2.get(), xmlStr, XML_MACROS);
    EXPECT_GE(imported, 0) << "ImportXML should not fail";

    QVector<AcceleratorEntry> bindings = doc2->m_acceleratorManager->keyBindingList();
    bool found = false;
    for (const AcceleratorEntry& entry : bindings) {
        if (entry.keyString == keyString) {
            found = true;
            EXPECT_EQ(entry.action, "north");
            break;
        }
    }
    EXPECT_TRUE(found) << "Keypad '8' binding not found in doc2 after import";
}

// =============================================================================
// Category 5: Temporary / Defaults / CDATA
// =============================================================================

// Test 14: Temporary trigger is skipped on save
TEST_F(WorldSerializationTest, TemporaryTriggerSkippedOnSave)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Temp Trigger Test";

    auto trig = std::make_unique<Trigger>();
    trig->label = "temp_trigger";
    trig->internal_name = "temp_trigger";
    trig->trigger = "transient pattern";
    trig->enabled = true;
    trig->temporary = true;
    trig->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTrigger("temp_trigger", std::move(trig)).has_value());

    QString content = saveAndReadXml(doc1.get());

    EXPECT_FALSE(content.contains("temp_trigger"))
        << "Temporary trigger name should not appear in saved XML";

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));
    EXPECT_EQ(doc2->getTrigger("temp_trigger"), nullptr)
        << "Temporary trigger should not exist in loaded doc";
}

// Test 15: Temporary alias is skipped on save
TEST_F(WorldSerializationTest, TemporaryAliasSkippedOnSave)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Temp Alias Test";

    auto alias = std::make_unique<Alias>();
    alias->label = "temp_alias";
    alias->internal_name = "temp_alias";
    alias->name = "tmp";
    alias->contents = "transient";
    alias->enabled = true;
    alias->temporary = true;
    alias->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addAlias("temp_alias", std::move(alias)).has_value());

    QString content = saveAndReadXml(doc1.get());

    EXPECT_FALSE(content.contains("temp_alias"))
        << "Temporary alias name should not appear in saved XML";

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));
    EXPECT_EQ(doc2->getAlias("temp_alias"), nullptr)
        << "Temporary alias should not exist in loaded doc";
}

// Test 16: Temporary timer is skipped on save
TEST_F(WorldSerializationTest, TemporaryTimerSkippedOnSave)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Temp Timer Test";

    auto timer = std::make_unique<Timer>();
    timer->label = "temp_timer";
    timer->enabled = true;
    timer->temporary = true;
    timer->type = Timer::TimerType::Interval;
    timer->every_minute = 1;
    timer->every_second = 0.0;
    timer->contents = "transient action";
    timer->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTimer("temp_timer", std::move(timer)).has_value());

    QString content = saveAndReadXml(doc1.get());

    EXPECT_FALSE(content.contains("temp_timer"))
        << "Temporary timer name should not appear in saved XML";

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));
    EXPECT_EQ(doc2->getTimer("temp_timer"), nullptr)
        << "Temporary timer should not exist in loaded doc";
}

// Test 17: Trigger contents with special XML characters survive round-trip via CDATA
TEST_F(WorldSerializationTest, CdataPreservesSpecialChars)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "CDATA Test";

    QString specialContents = "if x < 5 & y > 3 then \"ok\"";

    auto trig = std::make_unique<Trigger>();
    trig->label = "cdata_trigger";
    trig->internal_name = "cdata_trigger";
    trig->trigger = "cdata pattern";
    trig->contents = specialContents;
    trig->enabled = true;
    trig->send_to = eSendToWorld;
    EXPECT_TRUE(doc1->addTrigger("cdata_trigger", std::move(trig)).has_value());

    saveAndReadXml(doc1.get());

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    Trigger* loaded = doc2->getTrigger("cdata_trigger");
    ASSERT_NE(loaded, nullptr) << "Trigger not found after CDATA round-trip";
    EXPECT_EQ(loaded->contents, specialContents)
        << "Special characters in contents not preserved through CDATA round-trip";
}

// Test 18: Default port value (4000) is not written to XML
TEST_F(WorldSerializationTest, DefaultNumericOptionsOmittedFromXml)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Default Options Test";
    doc1->m_port = 4000; // default value

    QString content = saveAndReadXml(doc1.get());

    EXPECT_FALSE(content.contains("port=\"4000\""))
        << "Default port value 4000 should be omitted from XML";
}

// =============================================================================
// Category 6: Loaded-Value Snapshots
// =============================================================================

// Test 19: Non-default numeric option is captured in m_loadedNumericOptions after load
TEST_F(WorldSerializationTest, LoadedNumericSnapshotPopulated)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "Numeric Snapshot Test";
    doc1->m_port = 5555; // non-default

    saveAndReadXml(doc1.get());

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    auto it = doc2->m_loadedNumericOptions.find("port");
    ASSERT_NE(it, doc2->m_loadedNumericOptions.end())
        << "m_loadedNumericOptions should contain 'port' key after load";
    EXPECT_DOUBLE_EQ(it->second, 5555.0) << "m_loadedNumericOptions['port'] should be 5555.0";
}

// Test 20: Alpha option (world name) is captured in m_loadedAlphaOptions after load
TEST_F(WorldSerializationTest, LoadedAlphaSnapshotPopulated)
{
    doc1 = std::make_unique<WorldDocument>();
    doc1->m_mush_name = "TestWorld";

    saveAndReadXml(doc1.get());

    doc2 = std::make_unique<WorldDocument>();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2.get(), tmpFilename));

    auto it = doc2->m_loadedAlphaOptions.find("name");
    ASSERT_NE(it, doc2->m_loadedAlphaOptions.end())
        << "m_loadedAlphaOptions should contain 'name' key after load";
    EXPECT_EQ(it->second, "TestWorld") << "m_loadedAlphaOptions['name'] should be 'TestWorld'";
}
