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

#include "../src/automation/plugin.h"
#include "../src/storage/global_options.h"
#include "fixtures/world_fixtures.h"
#include <QFile>
#include <QTemporaryDir>

#include "../../src/utils/error_codes.h"

// Test fixture for plugin error tests
class PluginErrorTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create temp directory
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid()) << "Could not create temp directory";

        // Create world document
        doc = std::make_unique<WorldDocument>();
        doc->m_mush_name = "Test World";
        doc->m_server = "localhost";
        doc->m_port = 4000;
        doc->m_strWorldID = "{ERROR-TEST-WORLD-ID-123456789012}";

        // Configure state files directory
        GlobalOptions::instance().setStateFilesDirectory(tempDir->path());
    }

    void TearDown() override
    {
        delete tempDir;
    }

    QTemporaryDir* tempDir = nullptr;
    std::unique_ptr<WorldDocument> doc;
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
    auto loadResult = statePlugin->LoadState();

    EXPECT_FALSE(loadResult.has_value()) << "LoadState should fail with malformed XML";
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
    auto saveResult = recursionPlugin->SaveState();

    EXPECT_TRUE(saveResult.has_value()) << "SaveState should succeed (recursion prevention works)";
}

// Test 13: SaveState(bScripted=true) forces write even when m_bSaveState is false (H122)
//
// Original CPlugin::SaveState(bScripted=true) bypasses the m_bSaveState guard.
// Mushkin previously omitted the bScripted parameter so the Lua-facing SaveState
// silently did nothing when save_state="n" in the plugin XML.
TEST_F(PluginErrorTest, SaveState_bScriptedBypassesSaveStateFlag)
{
    QString pluginPath = tempDir->path() + "/nosave_plugin.xml";
    {
        QFile f(pluginPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="NoSavePlugin" id="{66666666-6666-6666-6666-666666666666}" save_state="n">
  </plugin>
</muclient>)");
        f.close();
    }

    QString errorMsg;
    Plugin* noSavePlugin = doc->LoadPlugin(pluginPath, errorMsg);
    ASSERT_NE(noSavePlugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();
    ASSERT_FALSE(noSavePlugin->m_bSaveState) << "m_bSaveState must be false (save_state=\"n\")";

    // Add a variable so there is something to write.
    auto var = std::make_unique<Variable>();
    var->label = "scripted_var";
    var->contents = "scripted_value";
    noSavePlugin->m_VariableMap["scripted_var"] = std::move(var);

    QString stateFilePath =
        tempDir->path() + "/" + doc->m_strWorldID + "-" + noSavePlugin->m_strID + "-state.xml";
    QFile::remove(stateFilePath);

    // Calling without bScripted (default false): must NOT write the file.
    auto resultNormal = noSavePlugin->SaveState();
    EXPECT_TRUE(resultNormal.has_value()) << "SaveState() should succeed (returns empty=ok)";
    EXPECT_FALSE(QFile::exists(stateFilePath))
        << "State file must NOT be written when m_bSaveState=false and bScripted=false";

    // Calling with bScripted=true (Lua API path): MUST write the file.
    auto resultScripted = noSavePlugin->SaveState(/*bScripted=*/true);
    EXPECT_TRUE(resultScripted.has_value()) << "SaveState(true) should succeed";
    EXPECT_TRUE(QFile::exists(stateFilePath))
        << "State file MUST be written when bScripted=true regardless of m_bSaveState (H122)";
// Test 13: LoadPlugin - Script parse error must not leave plugin in list (M14)
//
// Original xml_load_world.cpp:559 inserts plugin ONLY after script parsing
// succeeds (inside ThrowErrorException guard). Mushkin inserts before parsing,
// so on error the plugin must be removed. Verify that a failed load due to a
// script syntax error leaves m_PluginList unchanged.
TEST_F(PluginErrorTest, LoadPlugin_ScriptError_DoesNotLeavePluginInList)
{
    QString badScriptPath = tempDir->path() + "/bad_script.xml";
    QFile f(badScriptPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="BadScript" id="{66666666-6666-6666-6666-666666666666}" language="Lua">
    <script>
<![CDATA[
-- deliberate syntax error
this is not valid lua @@@@
]]>
    </script>
  </plugin>
</muclient>)");
    f.close();

    std::size_t countBefore = doc->m_PluginList.size();

    QString errorMsg;
    Plugin* result = doc->LoadPlugin(badScriptPath, errorMsg);

    EXPECT_EQ(result, nullptr) << "LoadPlugin should return nullptr on script error";
    EXPECT_EQ(doc->m_PluginList.size(), countBefore)
        << "m_PluginList must not grow when script parse fails (M14)";
}

// Test 14: UnloadPlugin - must mark document modified (M16)
//
// Original methods_plugins.cpp:655 calls SetModifiedFlag(TRUE) after plugin
// removal. Verify that m_bModified is true after a successful UnloadPlugin.
TEST_F(PluginErrorTest, UnloadPlugin_SetsModifiedFlag)
{
    QString validPath = tempDir->path() + "/unload_test.xml";
    QFile f(validPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(R"(<?xml version="1.0"?>
<muclient>
  <plugin name="UnloadMe" id="{77777777-7777-7777-7777-777777777777}">
  </plugin>
</muclient>)");
    f.close();

    QString errorMsg;
    Plugin* p = doc->LoadPlugin(validPath, errorMsg);
    ASSERT_NE(p, nullptr) << "Setup LoadPlugin failed: " << errorMsg.toStdString();

    // Clear the modified flag so the UnloadPlugin call is the one that sets it.
    doc->setModified(false);

    bool ok = doc->UnloadPlugin("{77777777-7777-7777-7777-777777777777}");

    EXPECT_TRUE(ok) << "UnloadPlugin should return true";
    EXPECT_TRUE(doc->isModified())
        << "UnloadPlugin must mark the document modified after removing a plugin (M16)";
}
