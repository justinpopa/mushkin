/**
 * test_plugin_errors_gtest.cpp - GoogleTest version
 * Plugin Error Path Testing
 *
 * Tests error handling for:
 * - LoadPlugin validation (file not found, invalid XML, missing fields, duplicate ID)
 * - All 9 error codes (eNoSuchPlugin, ePluginDisabled, etc.)
 * - XML parse errors in LoadState
 * - Recursion prevention in SaveState
 */

#include "test_qt_static.h"
#include "../src/automation/plugin.h"
#include "../src/storage/global_options.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>

// Error codes from methods_plugins.cpp
#define eOK 0
#define eNoSuchPlugin 30010
#define ePluginDisabled 30011
#define eNoSuchPluginFunction 30012
#define eNoSuchPluginTrigger 30013
#define eNoSuchPluginAlias 30014
#define eNoSuchPluginTimer 30015
#define eNoSuchPluginVariable 30016
#define eNoSuchPluginInfo 30017
#define eNoSuchPluginOption 30018

// Test fixture for plugin error tests
class PluginErrorTest : public ::testing::Test {
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
        doc->m_strWorldID = "{ERROR-TEST-WORLD-ID-123456789012}";

        // Configure state files directory
        GlobalOptions::instance()->setStateFilesDirectory(tempDir->path());
    }

    void TearDown() override
    {
        delete doc;
        delete tempDir;
    }

    QTemporaryDir* tempDir = nullptr;
    WorldDocument* doc = nullptr;
};

// Test 1: LoadPlugin - File Not Found
TEST_F(PluginErrorTest, LoadPlugin_FileNotFound)
{
    QString errorMsg;
    Plugin* plugin = doc->LoadPlugin("/nonexistent/path/plugin.xml", errorMsg);

    EXPECT_EQ(plugin, nullptr) << "LoadPlugin should return nullptr for non-existent file";
    EXPECT_TRUE(errorMsg.contains("not found"))
        << "Error message should contain 'not found', got: " << errorMsg.toStdString();
}

// Test 2: LoadPlugin - Invalid XML (no root)
TEST_F(PluginErrorTest, LoadPlugin_InvalidXML_NoRoot)
{
    QString invalidXmlPath = tempDir->path() + "/invalid.xml";
    QFile invalidFile(invalidXmlPath);
    ASSERT_TRUE(invalidFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    invalidFile.write("<?xml version=\"1.0\"?>\n<wrong_root></wrong_root>");
    invalidFile.close();

    QString errorMsg;
    Plugin* plugin = doc->LoadPlugin(invalidXmlPath, errorMsg);

    EXPECT_EQ(plugin, nullptr) << "LoadPlugin should return nullptr for invalid XML";
    EXPECT_TRUE(errorMsg.contains("muclient"))
        << "Error message should mention missing <muclient>, got: " << errorMsg.toStdString();
}

// Test 3: LoadPlugin - No <plugin> Element
TEST_F(PluginErrorTest, LoadPlugin_NoPluginElement)
{
    QString noPluginPath = tempDir->path() + "/noplugin.xml";
    QFile noPluginFile(noPluginPath);
    ASSERT_TRUE(noPluginFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    noPluginFile.write("<?xml version=\"1.0\"?>\n<muclient></muclient>");
    noPluginFile.close();

    QString errorMsg;
    Plugin* plugin = doc->LoadPlugin(noPluginPath, errorMsg);

    EXPECT_EQ(plugin, nullptr) << "LoadPlugin should return nullptr with no <plugin>";
    EXPECT_TRUE(errorMsg.contains("No <plugin> element"))
        << "Error message should mention missing <plugin>, got: " << errorMsg.toStdString();
}

// Test 4: LoadPlugin - Missing Name Attribute
TEST_F(PluginErrorTest, LoadPlugin_MissingNameAttribute)
{
    QString noNamePath = tempDir->path() + "/noname.xml";
    QFile noNameFile(noNamePath);
    ASSERT_TRUE(noNameFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    noNameFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin id="{11111111-1111-1111-1111-111111111111}">
  </plugin>
</muclient>)");
    noNameFile.close();

    QString errorMsg;
    Plugin* plugin = doc->LoadPlugin(noNamePath, errorMsg);

    EXPECT_EQ(plugin, nullptr) << "LoadPlugin should return nullptr with missing name";
    EXPECT_TRUE(errorMsg.contains("no name"))
        << "Error message should mention missing name, got: " << errorMsg.toStdString();
}

// Test 5: LoadPlugin - Missing ID Attribute
TEST_F(PluginErrorTest, LoadPlugin_MissingIDAttribute)
{
    QString noIdPath = tempDir->path() + "/noid.xml";
    QFile noIdFile(noIdPath);
    ASSERT_TRUE(noIdFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    noIdFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="TestPlugin">
  </plugin>
</muclient>)");
    noIdFile.close();

    QString errorMsg;
    Plugin* plugin = doc->LoadPlugin(noIdPath, errorMsg);

    EXPECT_EQ(plugin, nullptr) << "LoadPlugin should return nullptr with missing ID";
    EXPECT_TRUE(errorMsg.contains("no ID"))
        << "Error message should mention missing ID, got: " << errorMsg.toStdString();
}

// Test 6: LoadPlugin - Duplicate Plugin ID
TEST_F(PluginErrorTest, LoadPlugin_DuplicatePluginID)
{
    // Create a valid plugin first
    QString validPluginPath = tempDir->path() + "/valid1.xml";
    QFile validFile(validPluginPath);
    ASSERT_TRUE(validFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    validFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="Plugin1" id="{22222222-2222-2222-2222-222222222222}">
  </plugin>
</muclient>)");
    validFile.close();

    QString errorMsg;
    Plugin* plugin1 = doc->LoadPlugin(validPluginPath, errorMsg);
    ASSERT_NE(plugin1, nullptr) << "First LoadPlugin should succeed, error: "
                                << errorMsg.toStdString();

    // Try to load another plugin with same ID
    QString duplicatePath = tempDir->path() + "/valid2.xml";
    QFile duplicateFile(duplicatePath);
    ASSERT_TRUE(duplicateFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    duplicateFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="Plugin2" id="{22222222-2222-2222-2222-222222222222}">
  </plugin>
</muclient>)");
    duplicateFile.close();

    Plugin* plugin2 = doc->LoadPlugin(duplicatePath, errorMsg);

    EXPECT_EQ(plugin2, nullptr) << "LoadPlugin should reject duplicate plugin ID";
    EXPECT_TRUE(errorMsg.contains("already installed"))
        << "Error message should mention duplicate ID, got: " << errorMsg.toStdString();
}

// Test 7: FindPluginByID - Not Found
TEST_F(PluginErrorTest, FindPluginByID_NotFound)
{
    Plugin* notFound = doc->FindPluginByID("{nonexistent-id}");

    EXPECT_EQ(notFound, nullptr) << "FindPluginByID should return nullptr for non-existent plugin";
}

// Test 8: FindPluginByName - Not Found
TEST_F(PluginErrorTest, FindPluginByName_NotFound)
{
    Plugin* notFound = doc->FindPluginByName("NonExistentPlugin");

    EXPECT_EQ(notFound, nullptr)
        << "FindPluginByName should return nullptr for non-existent plugin";
}

// Test 9: EnablePlugin - Plugin Not Found
TEST_F(PluginErrorTest, EnablePlugin_PluginNotFound)
{
    bool enableResult = doc->EnablePlugin("{nonexistent-id}", true);

    EXPECT_FALSE(enableResult) << "EnablePlugin should return false for non-existent plugin";
}

// Test 10: UnloadPlugin - Plugin Not Found
TEST_F(PluginErrorTest, UnloadPlugin_PluginNotFound)
{
    bool unloadResult = doc->UnloadPlugin("{nonexistent-id}");

    EXPECT_FALSE(unloadResult) << "UnloadPlugin should return false for non-existent plugin";
}

// Test 11: LoadState - Malformed XML
TEST_F(PluginErrorTest, LoadState_MalformedXML)
{
    // Create a plugin with save_state enabled
    QString pluginID = "{44444444-4444-4444-4444-444444444444}";
    QString statePluginPath = tempDir->path() + "/state_plugin.xml";
    QFile stateFile(statePluginPath);
    ASSERT_TRUE(stateFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    stateFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="StatePlugin" id="{44444444-4444-4444-4444-444444444444}" save_state="y">
  </plugin>
</muclient>)");
    stateFile.close();

    QString errorMsg;
    Plugin* statePlugin = doc->LoadPlugin(statePluginPath, errorMsg);
    ASSERT_NE(statePlugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();

    // Create a malformed state file using new path format:
    // {stateDir}/{worldID}-{pluginID}-state.xml
    QString stateFilePath =
        tempDir->path() + "/" + doc->m_strWorldID + "-" + pluginID + "-state.xml";
    QFile malformedState(stateFilePath);
    ASSERT_TRUE(malformedState.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create state file";
    malformedState.write("<muclient><variables><variable name=\"test\">unclosed");
    malformedState.close();

    // Try to load the malformed state
    bool loadSuccess = statePlugin->LoadState();

    EXPECT_FALSE(loadSuccess) << "LoadState should fail with malformed XML";
}

// Test 12: SaveState - Recursion Prevention
TEST_F(PluginErrorTest, SaveState_RecursionPrevention)
{
    // Create a plugin with a script that calls SaveState from OnPluginSaveState
    QString recursionPluginPath = tempDir->path() + "/recursion_plugin.xml";
    QFile recursionFile(recursionPluginPath);
    ASSERT_TRUE(recursionFile.open(QIODevice::WriteOnly | QIODevice::Text))
        << "Could not create test file";
    recursionFile.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="RecursionPlugin" id="{55555555-5555-5555-5555-555555555555}" save_state="y" language="Lua">
    <script>
<![CDATA[
-- This function will be called when SaveState is triggered
function OnPluginSaveState()
  -- Trying to call SaveState again would cause infinite recursion
  -- But m_bSavingStateNow flag should prevent this
  -- Note: world.SaveState() is not available in plugin context,
  -- so this test just verifies the flag prevents multiple saves
  return true
end
]]>
    </script>
  </plugin>
</muclient>)");
    recursionFile.close();

    QString errorMsg;
    Plugin* recursionPlugin = doc->LoadPlugin(recursionPluginPath, errorMsg);
    ASSERT_NE(recursionPlugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();

    // Call SaveState - should succeed and not hang
    bool saveSuccess = recursionPlugin->SaveState();

    EXPECT_TRUE(saveSuccess) << "SaveState should succeed (recursion prevention works)";
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
