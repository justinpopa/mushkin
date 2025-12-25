/**
 * test_plugin_state_gtest.cpp - GoogleTest version
 * Plugin State Saving Test
 *
 * Tests plugin state persistence including:
 * - Variable saving/loading (strLabel, strContents)
 * - Array saving/loading (nested map structure)
 * - OnPluginSaveState callback execution
 * - m_bSaveState flag behavior
 * - File format verification
 * - Multiple save/load cycles
 */

#include "../src/automation/plugin.h"
#include "../src/automation/variable.h"
#include "../src/storage/global_options.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Helper to create test plugin XML
QString createPluginXML(const QString& id, bool saveState)
{
    QString saveStateValue = saveState ? "y" : "n";
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin
  name="Test State Plugin"
  author="Test Author"
  id="%1"
  language="Lua"
  purpose="Test plugin state saving"
  version="1.0"
  save_state="%2"
>

<script>
<![CDATA[
-- Track whether OnPluginSaveState was called
save_state_called = false

function OnPluginSaveState()
  save_state_called = true
end

-- Function to create test variables
function CreateTestData()
  -- This will be exposed via Lua API in
  -- For now, we'll create data directly in C++
end
]]>
</script>

</plugin>
</muclient>
)")
        .arg(id, saveStateValue);
}

// Test fixture for plugin state tests
class PluginStateTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create temp directory
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid()) << "Could not create temp directory";

        // Create world document
        doc = new WorldDocument();
        doc->m_mush_name = "Test World";
        doc->m_server = "localhost";
        doc->m_port = 4000;
        worldID = "{WORLD-1234-5678-9ABC-DEF0TESTWORLD}";
        doc->m_strWorldID = worldID;

        // Configure state files directory to use temp directory
        GlobalOptions::instance()->setStateFilesDirectory(tempDir->path());

        // Create test plugin file
        pluginPath = tempDir->path() + "/test-plugin.xml";
        pluginID = "{ABCD1234-5678-9ABC-DEF0-123456789ABC}";
        // State file path uses new format: {stateDir}/{worldID}-{pluginID}-state.xml
        stateFilePath = tempDir->path() + "/" + worldID + "-" + pluginID + "-state.xml";

        QFile pluginFile(pluginPath);
        ASSERT_TRUE(pluginFile.open(QIODevice::WriteOnly | QIODevice::Text))
            << "Could not create plugin file";
        pluginFile.write(createPluginXML(pluginID, true).toUtf8());
        pluginFile.close();

        // Load plugin
        QString errorMsg;
        plugin = doc->LoadPlugin(pluginPath, errorMsg);
        ASSERT_NE(plugin, nullptr) << "Could not load plugin: " << errorMsg.toStdString();
    }

    void TearDown() override
    {
        delete doc;
        delete tempDir;
    }

    QTemporaryDir* tempDir = nullptr;
    WorldDocument* doc = nullptr;
    Plugin* plugin = nullptr;
    QString pluginPath;
    QString pluginID;
    QString worldID;
    QString stateFilePath;
};

// Test 1: Verify save_state flag is loaded correctly
TEST_F(PluginStateTest, SaveStateFlagTrue)
{
    EXPECT_TRUE(plugin->m_bSaveState) << "save_state flag should be true when set to 'y' in XML";
}

// Test 2: Save state with variables
TEST_F(PluginStateTest, SaveStateWithVariables)
{
    // Create variables
    auto var1 = std::make_unique<Variable>();
    var1->strLabel = "player_name";
    var1->strContents = "Gandalf";
    plugin->m_VariableMap["player_name"] = std::move(var1);

    auto var2 = std::make_unique<Variable>();
    var2->strLabel = "player_hp";
    var2->strContents = "250";
    plugin->m_VariableMap["player_hp"] = std::move(var2);

    auto var3 = std::make_unique<Variable>();
    var3->strLabel = "guild";
    var3->strContents = "Wizards";
    plugin->m_VariableMap["guild"] = std::move(var3);

    EXPECT_EQ(plugin->m_VariableMap.size(), 3) << "Should have 3 variables";

    // Save state
    bool saveResult = plugin->SaveState();
    EXPECT_TRUE(saveResult) << "SaveState() should return true";

    // Verify state file was created
    EXPECT_TRUE(QFile::exists(stateFilePath)) << "State file should be created";
}

// Test 3: Save state with arrays
TEST_F(PluginStateTest, SaveStateWithArrays)
{
    // Create arrays
    QMap<QString, QString> inventory;
    inventory["sword"] = "Steel Longsword";
    inventory["shield"] = "Oak Shield";
    inventory["potion"] = "Healing Potion";
    plugin->m_Arrays["inventory"] = inventory;

    QMap<QString, QString> stats;
    stats["strength"] = "18";
    stats["wisdom"] = "20";
    stats["dexterity"] = "14";
    plugin->m_Arrays["stats"] = stats;

    EXPECT_EQ(plugin->m_Arrays.size(), 2) << "Should have 2 arrays";
    EXPECT_EQ(plugin->m_Arrays["inventory"].size(), 3) << "inventory should have 3 items";
    EXPECT_EQ(plugin->m_Arrays["stats"].size(), 3) << "stats should have 3 items";

    // Save state
    bool saveResult = plugin->SaveState();
    EXPECT_TRUE(saveResult) << "SaveState() should return true";

    // Verify state file was created
    EXPECT_TRUE(QFile::exists(stateFilePath)) << "State file should be created";
}

// Test 4: OnPluginSaveState callback is called
TEST_F(PluginStateTest, OnPluginSaveStateCallback)
{
    // Save state
    plugin->SaveState();

    // Verify OnPluginSaveState callback was called
    lua_State* L = plugin->m_ScriptEngine->L;
    lua_getglobal(L, "save_state_called");
    bool callbackCalled = lua_toboolean(L, -1);
    lua_pop(L, 1);

    EXPECT_TRUE(callbackCalled) << "OnPluginSaveState callback should be called";
}

// Test 5: Verify XML structure
TEST_F(PluginStateTest, VerifyXMLStructure)
{
    // Create variables
    auto var1 = std::make_unique<Variable>();
    var1->strLabel = "test_var1";
    var1->strContents = "value1";
    plugin->m_VariableMap["test_var1"] = std::move(var1);

    auto var2 = std::make_unique<Variable>();
    var2->strLabel = "test_var2";
    var2->strContents = "value2";
    plugin->m_VariableMap["test_var2"] = std::move(var2);

    auto var3 = std::make_unique<Variable>();
    var3->strLabel = "test_var3";
    var3->strContents = "value3";
    plugin->m_VariableMap["test_var3"] = std::move(var3);

    // Create arrays
    QMap<QString, QString> array1;
    array1["key1"] = "val1";
    plugin->m_Arrays["array1"] = array1;

    QMap<QString, QString> array2;
    array2["key2"] = "val2";
    plugin->m_Arrays["array2"] = array2;

    // Save state
    plugin->SaveState();

    // Parse and verify XML
    QFile xmlFile(stateFilePath);
    ASSERT_TRUE(xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
        << "Could not open state file for reading";

    QXmlStreamReader xml(&xmlFile);

    int variableCount = 0;
    int arrayCount = 0;
    bool foundMuclient = false;
    bool foundVariables = false;
    bool foundArrays = false;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            QString name = xml.name().toString();

            if (name == "muclient") {
                foundMuclient = true;
            } else if (name == "variables") {
                foundVariables = true;
            } else if (name == "variable") {
                variableCount++;
            } else if (name == "arrays") {
                foundArrays = true;
            } else if (name == "array") {
                arrayCount++;
            }
        }
    }

    xmlFile.close();

    EXPECT_TRUE(foundMuclient) << "XML should have <muclient> element";
    EXPECT_TRUE(foundVariables) << "XML should have <variables> element";
    EXPECT_TRUE(foundArrays) << "XML should have <arrays> element";
    EXPECT_EQ(variableCount, 3) << "Expected 3 variables in XML";
    EXPECT_EQ(arrayCount, 2) << "Expected 2 arrays in XML";
}

// Test 6: Load state restores variables
TEST_F(PluginStateTest, LoadStateRestoresVariables)
{
    // Create and save variables
    auto var1 = std::make_unique<Variable>();
    var1->strLabel = "player_name";
    var1->strContents = "Gandalf";
    plugin->m_VariableMap["player_name"] = std::move(var1);

    auto var2 = std::make_unique<Variable>();
    var2->strLabel = "player_hp";
    var2->strContents = "250";
    plugin->m_VariableMap["player_hp"] = std::move(var2);

    auto var3 = std::make_unique<Variable>();
    var3->strLabel = "guild";
    var3->strContents = "Wizards";
    plugin->m_VariableMap["guild"] = std::move(var3);

    plugin->SaveState();

    // Clear variables (unique_ptr handles deletion automatically)
    plugin->m_VariableMap.clear();
    EXPECT_EQ(plugin->m_VariableMap.size(), 0) << "Variables should be cleared";

    // Load state
    bool loadResult = plugin->LoadState();
    EXPECT_TRUE(loadResult) << "LoadState() should return true";

    // Verify variables were restored
    EXPECT_EQ(plugin->m_VariableMap.size(), 3) << "Should have 3 variables after loading";
    EXPECT_TRUE(plugin->m_VariableMap.find("player_name") != plugin->m_VariableMap.end())
        << "Should have player_name";
    EXPECT_TRUE(plugin->m_VariableMap.find("player_hp") != plugin->m_VariableMap.end())
        << "Should have player_hp";
    EXPECT_TRUE(plugin->m_VariableMap.find("guild") != plugin->m_VariableMap.end())
        << "Should have guild";

    EXPECT_EQ(plugin->m_VariableMap["player_name"]->strContents, QString("Gandalf"))
        << "player_name value should be correct";
    EXPECT_EQ(plugin->m_VariableMap["player_hp"]->strContents, QString("250"))
        << "player_hp value should be correct";
    EXPECT_EQ(plugin->m_VariableMap["guild"]->strContents, QString("Wizards"))
        << "guild value should be correct";
}

// Test 7: Load state restores arrays
TEST_F(PluginStateTest, LoadStateRestoresArrays)
{
    // Create and save arrays
    QMap<QString, QString> inventory;
    inventory["sword"] = "Steel Longsword";
    inventory["shield"] = "Oak Shield";
    inventory["potion"] = "Healing Potion";
    plugin->m_Arrays["inventory"] = inventory;

    QMap<QString, QString> stats;
    stats["strength"] = "18";
    stats["wisdom"] = "20";
    stats["dexterity"] = "14";
    plugin->m_Arrays["stats"] = stats;

    plugin->SaveState();

    // Clear arrays
    plugin->m_Arrays.clear();
    EXPECT_EQ(plugin->m_Arrays.size(), 0) << "Arrays should be cleared";

    // Load state
    bool loadResult = plugin->LoadState();
    EXPECT_TRUE(loadResult) << "LoadState() should return true";

    // Verify arrays were restored
    EXPECT_EQ(plugin->m_Arrays.size(), 2) << "Should have 2 arrays after loading";
    EXPECT_TRUE(plugin->m_Arrays.contains("inventory")) << "Should have inventory array";
    EXPECT_TRUE(plugin->m_Arrays.contains("stats")) << "Should have stats array";

    QMap<QString, QString> loadedInventory = plugin->m_Arrays["inventory"];
    EXPECT_EQ(loadedInventory["sword"], QString("Steel Longsword"))
        << "inventory.sword should be correct";
    EXPECT_EQ(loadedInventory["shield"], QString("Oak Shield"))
        << "inventory.shield should be correct";
    EXPECT_EQ(loadedInventory["potion"], QString("Healing Potion"))
        << "inventory.potion should be correct";

    QMap<QString, QString> loadedStats = plugin->m_Arrays["stats"];
    EXPECT_EQ(loadedStats["strength"], QString("18")) << "stats.strength should be correct";
    EXPECT_EQ(loadedStats["wisdom"], QString("20")) << "stats.wisdom should be correct";
    EXPECT_EQ(loadedStats["dexterity"], QString("14")) << "stats.dexterity should be correct";
}

// Test 8: save_state=false prevents file creation
TEST_F(PluginStateTest, SaveStateFalsePreventsFileCreation)
{
    // Set save_state to false
    plugin->m_bSaveState = false;

    // Create a variable
    auto var = std::make_unique<Variable>();
    var->strLabel = "test_var";
    var->strContents = "test_value";
    plugin->m_VariableMap["test_var"] = std::move(var);

    // Remove state file if it exists
    QFile::remove(stateFilePath);

    // Save state
    bool saveResult = plugin->SaveState();
    EXPECT_TRUE(saveResult) << "SaveState() should return true even when save_state=false";

    // Verify state file was NOT created
    EXPECT_FALSE(QFile::exists(stateFilePath))
        << "State file should not be created when save_state=false";
}

// Test 9: Empty state is saved correctly
TEST_F(PluginStateTest, EmptyStateSaved)
{
    // Ensure no variables or arrays
    plugin->m_VariableMap.clear();
    plugin->m_Arrays.clear();

    // Save state
    bool saveResult = plugin->SaveState();
    EXPECT_TRUE(saveResult) << "SaveState() should succeed for empty state";

    // Verify file was created
    EXPECT_TRUE(QFile::exists(stateFilePath))
        << "State file should be created even for empty state";
}

// Test 10: LoadState on non-existent file succeeds gracefully
TEST_F(PluginStateTest, LoadStateNonExistentFile)
{
    // Remove state file if it exists
    QFile::remove(stateFilePath);

    // Load state
    bool loadResult = plugin->LoadState();
    EXPECT_TRUE(loadResult) << "LoadState() should return true when file doesn't exist";
}

// Test 11: Multiple save/load cycles preserve latest data
TEST_F(PluginStateTest, MultipleSaveLoadCycles)
{
    // Create initial data
    auto var = std::make_unique<Variable>();
    var->strLabel = "cycle_test";
    var->strContents = "iteration 1";
    plugin->m_VariableMap["cycle_test"] = std::move(var);

    // First save
    plugin->SaveState();

    // Modify
    plugin->m_VariableMap["cycle_test"]->strContents = "iteration 2";

    // Second save
    plugin->SaveState();

    // Clear and reload
    plugin->m_VariableMap.clear();
    plugin->LoadState();

    // Verify latest value
    ASSERT_TRUE(plugin->m_VariableMap.find("cycle_test") != plugin->m_VariableMap.end())
        << "Variable should exist after reload";
    EXPECT_EQ(plugin->m_VariableMap["cycle_test"]->strContents, QString("iteration 2"))
        << "Multiple saves should overwrite, keeping latest value";
}

// Test 12: Complex state with variables and arrays
TEST_F(PluginStateTest, ComplexStateSaveLoad)
{
    // Create complex state
    auto var1 = std::make_unique<Variable>();
    var1->strLabel = "var1";
    var1->strContents = "value1";
    plugin->m_VariableMap["var1"] = std::move(var1);

    auto var2 = std::make_unique<Variable>();
    var2->strLabel = "var2";
    var2->strContents = "value2";
    plugin->m_VariableMap["var2"] = std::move(var2);

    QMap<QString, QString> array1;
    array1["a"] = "1";
    array1["b"] = "2";
    plugin->m_Arrays["array1"] = array1;

    QMap<QString, QString> array2;
    array2["x"] = "10";
    array2["y"] = "20";
    plugin->m_Arrays["array2"] = array2;

    // Save
    plugin->SaveState();

    // Clear
    plugin->m_VariableMap.clear();
    plugin->m_Arrays.clear();

    // Load
    plugin->LoadState();

    // Verify everything restored
    EXPECT_EQ(plugin->m_VariableMap.size(), 2) << "Should have 2 variables";
    EXPECT_EQ(plugin->m_Arrays.size(), 2) << "Should have 2 arrays";

    EXPECT_EQ(plugin->m_VariableMap["var1"]->strContents, QString("value1"))
        << "var1 should be correct";
    EXPECT_EQ(plugin->m_VariableMap["var2"]->strContents, QString("value2"))
        << "var2 should be correct";

    EXPECT_EQ(plugin->m_Arrays["array1"]["a"], QString("1")) << "array1.a should be correct";
    EXPECT_EQ(plugin->m_Arrays["array1"]["b"], QString("2")) << "array1.b should be correct";
    EXPECT_EQ(plugin->m_Arrays["array2"]["x"], QString("10")) << "array2.x should be correct";
    EXPECT_EQ(plugin->m_Arrays["array2"]["y"], QString("20")) << "array2.y should be correct";
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
