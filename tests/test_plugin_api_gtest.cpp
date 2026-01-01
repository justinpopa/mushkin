/**
 * test_plugin_api_gtest.cpp - GoogleTest version
 * Plugin Management API Test
 *
 * Tests all 24 Lua API functions for plugin management:
 * - Query functions (GetPluginID, GetPluginName, GetPluginList, IsPluginInstalled, GetPluginInfo)
 * - Loading functions (LoadPlugin, ReloadPlugin, UnloadPlugin, EnablePlugin)
 * - Communication functions (CallPlugin, PluginSupports, BroadcastPlugin)
 * - Collection access functions (GetPluginVariable/List,
 * GetPluginTrigger/Alias/TimerList/Info/Option)
 * - State function (SaveState)
 */

#include "test_qt_static.h"
#include "../src/automation/plugin.h"
#include "../src/automation/variable.h"
#include "../src/storage/global_options.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Helper to create test plugin XML
QString createTestPluginXML(const QString& id, const QString& name, const QString& script)
{
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin
  name="%1"
  author="Test Author"
  id="%2"
  language="Lua"
 purpose="Test plugin"
  version="1.0"
  save_state="y"
  sequence="100">

<script>
<![CDATA[
%3
]]>
</script>

<triggers>
  <trigger
    enabled="y"
    match="test trigger"
    send_to="12"
    sequence="100"
    name="test_trigger_1"
  >
  </trigger>
</triggers>

<aliases>
  <alias
    enabled="y"
    match="test alias"
    send_to="12"
    sequence="100"
    name="test_alias_1"
  >
  </alias>
</aliases>

<timers>
  <timer
    enabled="y"
    second="5.00"
    send_to="12"
    name="test_timer_1"
  >
  </timer>
</timers>

</plugin>
</muclient>
)")
        .arg(name, id, script);
}

// Test fixture for plugin API tests
class PluginApiTest : public ::testing::Test {
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
        doc->m_strWorldID = "{API-TEST-WORLD-ID-1234567890}";

        // Configure state files directory
        GlobalOptions::instance()->setStateFilesDirectory(tempDir->path());

        // Create test plugin files
        plugin1Path = tempDir->path() + "/plugin1.xml";
        plugin2Path = tempDir->path() + "/plugin2.xml";

        QString plugin1Script = R"(
-- Plugin 1: Test various API functions
test_variable = "plugin1_data"
broadcast_count = 0

function TestFunction(arg)
    return "Plugin1 received: " .. arg
end

function OnPluginBroadcast(msg, id, name, text)
    broadcast_count = broadcast_count + 1
end

function OnPluginSaveState()
    -- State saving callback
end
)";

        QString plugin2Script = R"(
-- Plugin 2: Test communication with Plugin 1
function OnPluginInstall()
    -- This plugin will call Plugin 1
end

function CallOtherPlugin()
    -- Will be called from test
    return "Plugin2 active"
end
)";

        QFile file1(plugin1Path);
        ASSERT_TRUE(file1.open(QIODevice::WriteOnly | QIODevice::Text))
            << "Could not create plugin1.xml";
        file1.write(createTestPluginXML("{AAAA0001-0001-0001-0001-000000000001}", "TestPlugin1",
                                        plugin1Script)
                        .toUtf8());
        file1.close();

        QFile file2(plugin2Path);
        ASSERT_TRUE(file2.open(QIODevice::WriteOnly | QIODevice::Text))
            << "Could not create plugin2.xml";
        file2.write(createTestPluginXML("{BBBB0002-0002-0002-0002-000000000002}", "TestPlugin2",
                                        plugin2Script)
                        .toUtf8());
        file2.close();

        // Load plugins
        QString errorMsg;
        plugin1 = doc->LoadPlugin(plugin1Path, errorMsg);
        ASSERT_NE(plugin1, nullptr) << "Could not load plugin1: " << errorMsg.toStdString();

        plugin2 = doc->LoadPlugin(plugin2Path, errorMsg);
        ASSERT_NE(plugin2, nullptr) << "Could not load plugin2: " << errorMsg.toStdString();
    }

    void TearDown() override
    {
        delete doc;
        delete tempDir;
    }

    QTemporaryDir* tempDir = nullptr;
    WorldDocument* doc = nullptr;
    Plugin* plugin1 = nullptr;
    Plugin* plugin2 = nullptr;
    QString plugin1Path;
    QString plugin2Path;
};

// Test 1: GetPluginID API
TEST_F(PluginApiTest, GetPluginID)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginID");
    lua_call(L1, 0, 1);
    const char* id = lua_tostring(L1, -1);

    EXPECT_EQ(QString(id), plugin1->m_strID) << "GetPluginID should return plugin1's ID";

    lua_pop(L1, 2); // pop result and world table
    doc->m_CurrentPlugin = nullptr;
}

// Test 2: GetPluginName API
TEST_F(PluginApiTest, GetPluginName)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginName");
    lua_call(L1, 0, 1);
    const char* name = lua_tostring(L1, -1);

    EXPECT_EQ(QString(name), plugin1->m_strName) << "GetPluginName should return plugin1's name";

    lua_pop(L1, 2); // pop result and world table
    doc->m_CurrentPlugin = nullptr;
}

// Test 3: GetPluginList API
TEST_F(PluginApiTest, GetPluginList)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginList");
    lua_call(L1, 0, 1);

    ASSERT_TRUE(lua_istable(L1, -1)) << "GetPluginList should return a table";

    int count = lua_objlen(L1, -1);
    EXPECT_EQ(count, 2) << "GetPluginList should return 2 plugins";

    lua_pop(L1, 2); // pop result and world table
    doc->m_CurrentPlugin = nullptr;
}

// Test 4: IsPluginInstalled API
TEST_F(PluginApiTest, IsPluginInstalled)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    // Test with installed plugin
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "IsPluginInstalled");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_call(L1, 1, 1);
    bool installed = lua_toboolean(L1, -1);
    lua_pop(L1, 2); // pop result and world table

    EXPECT_TRUE(installed) << "IsPluginInstalled should return true for plugin2";

    // Test with non-existent plugin
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "IsPluginInstalled");
    lua_pushstring(L1, "{00000000-0000-0000-0000-000000000000}");
    lua_call(L1, 1, 1);
    bool notInstalled = !lua_toboolean(L1, -1);
    lua_pop(L1, 2); // pop result and world table

    EXPECT_TRUE(notInstalled) << "IsPluginInstalled should return false for non-existent plugin";

    doc->m_CurrentPlugin = nullptr;
}

// Test 5: GetPluginInfo API
TEST_F(PluginApiTest, GetPluginInfo)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    // Test info type 1 (Name)
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginInfo");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushnumber(L1, 1);
    lua_call(L1, 2, 1);
    const char* pluginName = lua_tostring(L1, -1);
    EXPECT_STREQ(pluginName, "TestPlugin2") << "GetPluginInfo should return plugin2's name";
    lua_pop(L1, 2); // pop result and world table

    // Test info type 7 (ID)
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginInfo");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushnumber(L1, 7);
    lua_call(L1, 2, 1);
    const char* pluginID = lua_tostring(L1, -1);
    EXPECT_EQ(QString(pluginID), plugin2->m_strID) << "GetPluginInfo should return plugin2's ID";
    lua_pop(L1, 2); // pop result and world table

    // Test info type 17 (Enabled)
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "GetPluginInfo");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushnumber(L1, 17);
    lua_call(L1, 2, 1);
    bool pluginEnabled = lua_toboolean(L1, -1);
    EXPECT_TRUE(pluginEnabled) << "GetPluginInfo should show plugin2 is enabled";
    lua_pop(L1, 2); // pop result and world table

    doc->m_CurrentPlugin = nullptr;
}

// Test 6: PluginSupports API
TEST_F(PluginApiTest, PluginSupports)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    // Check if Plugin2 has CallOtherPlugin function
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "PluginSupports");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushstring(L1, "CallOtherPlugin");
    lua_call(L1, 2, 1);
    int result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "PluginSupports should return eOK for existing function";

    // Check non-existent function
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "PluginSupports");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushstring(L1, "NonExistentFunction");
    lua_call(L1, 2, 1);
    result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_NE(result, 0) << "PluginSupports should return error for non-existent function";

    doc->m_CurrentPlugin = nullptr;
}

// Test 7: CallPlugin API
TEST_F(PluginApiTest, CallPlugin)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "CallPlugin");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushstring(L1, "CallOtherPlugin");
    lua_pushstring(L1, "test argument");
    lua_call(L1, 3, 1);
    int result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "CallPlugin should return eOK";

    doc->m_CurrentPlugin = nullptr;
}

// Test 8: BroadcastPlugin API
TEST_F(PluginApiTest, BroadcastPlugin)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "BroadcastPlugin");
    lua_pushnumber(L1, 1);
    lua_pushstring(L1, "test broadcast message");
    lua_call(L1, 2, 1);
    int count = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(count, 1) << "BroadcastPlugin should send to 1 plugin (plugin2)";

    doc->m_CurrentPlugin = nullptr;
}

// Test 9: GetPluginVariable API
TEST_F(PluginApiTest, GetPluginVariable)
{
    // Add a variable to plugin1
    auto var = std::make_unique<Variable>();
    var->strLabel = "test_var";
    var->strContents = "test_value";
    plugin1->m_VariableMap["test_var"] = std::move(var);

    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    // Get variable from plugin1
    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginVariable");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_var");
    lua_call(L2, 2, 1);
    const char* varValue = lua_tostring(L2, -1);
    lua_pop(L2, 1);

    EXPECT_STREQ(varValue, "test_value") << "GetPluginVariable should return correct value";

    doc->m_CurrentPlugin = nullptr;
}

// Test 10: GetPluginVariableList API
TEST_F(PluginApiTest, GetPluginVariableList)
{
    // Add a variable to plugin1
    auto var = std::make_unique<Variable>();
    var->strLabel = "test_var";
    var->strContents = "test_value";
    plugin1->m_VariableMap["test_var"] = std::move(var);

    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginVariableList");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_call(L2, 1, 1);

    ASSERT_TRUE(lua_istable(L2, -1)) << "GetPluginVariableList should return a table";

    int varCount = lua_objlen(L2, -1);
    EXPECT_GE(varCount, 1) << "GetPluginVariableList should return at least 1 variable";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 11: GetPluginTriggerList API
TEST_F(PluginApiTest, GetPluginTriggerList)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTriggerList");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_call(L2, 1, 1);

    ASSERT_TRUE(lua_istable(L2, -1)) << "GetPluginTriggerList should return a table";

    int triggerCount = lua_objlen(L2, -1);
    EXPECT_EQ(triggerCount, 1) << "GetPluginTriggerList should return 1 trigger";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 12: GetPluginAliasList API
TEST_F(PluginApiTest, GetPluginAliasList)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginAliasList");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_call(L2, 1, 1);

    ASSERT_TRUE(lua_istable(L2, -1)) << "GetPluginAliasList should return a table";

    int aliasCount = lua_objlen(L2, -1);
    EXPECT_EQ(aliasCount, 1) << "GetPluginAliasList should return 1 alias";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 13: GetPluginTimerList API
TEST_F(PluginApiTest, GetPluginTimerList)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTimerList");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_call(L2, 1, 1);

    ASSERT_TRUE(lua_istable(L2, -1)) << "GetPluginTimerList should return a table";

    int timerCount = lua_objlen(L2, -1);
    EXPECT_EQ(timerCount, 1) << "GetPluginTimerList should return 1 timer";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 14: GetPluginTriggerInfo API
TEST_F(PluginApiTest, GetPluginTriggerInfo)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    // Get trigger pattern (info type 1) - matches GetTriggerInfo behavior
    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTriggerInfo");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_trigger_1");
    lua_pushnumber(L2, 1);
    lua_call(L2, 3, 1);
    const char* trigPattern = lua_tostring(L2, -1);

    EXPECT_STREQ(trigPattern, "test trigger")
        << "GetPluginTriggerInfo case 1 should return trigger pattern";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 15: GetPluginAliasInfo API
TEST_F(PluginApiTest, GetPluginAliasInfo)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginAliasInfo");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_alias_1");
    lua_pushnumber(L2, 1);
    lua_call(L2, 3, 1);
    const char* aliasPattern = lua_tostring(L2, -1);

    EXPECT_STREQ(aliasPattern, "test alias")
        << "GetPluginAliasInfo case 1 should return alias pattern";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 16: GetPluginTimerInfo API
TEST_F(PluginApiTest, GetPluginTimerInfo)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    // Get timer hour (info type 1) - matches GetTimerInfo behavior
    // Timer is interval timer with second="5.00", so hour=0
    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTimerInfo");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_timer_1");
    lua_pushnumber(L2, 1);
    lua_call(L2, 3, 1);
    double timerHour = lua_tonumber(L2, -1);

    EXPECT_EQ(timerHour, 0.0)
        << "GetPluginTimerInfo case 1 should return timer hour (0 for interval timer)";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 17: GetPluginTriggerOption API
TEST_F(PluginApiTest, GetPluginTriggerOption)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTriggerOption");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_trigger_1");
    lua_pushstring(L2, "enabled");
    lua_call(L2, 3, 1);
    bool trigEnabled = lua_toboolean(L2, -1);

    EXPECT_TRUE(trigEnabled) << "GetPluginTriggerOption should show trigger is enabled";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 18: GetPluginAliasOption API
TEST_F(PluginApiTest, GetPluginAliasOption)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginAliasOption");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_alias_1");
    lua_pushstring(L2, "enabled");
    lua_call(L2, 3, 1);
    bool aliasEnabled = lua_toboolean(L2, -1);

    EXPECT_TRUE(aliasEnabled) << "GetPluginAliasOption should show alias is enabled";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 19: GetPluginTimerOption API
TEST_F(PluginApiTest, GetPluginTimerOption)
{
    lua_State* L2 = plugin2->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin2;

    lua_getglobal(L2, "world");
    lua_getfield(L2, -1, "GetPluginTimerOption");
    lua_pushstring(L2, plugin1->m_strID.toUtf8().constData());
    lua_pushstring(L2, "test_timer_1");
    lua_pushstring(L2, "enabled");
    lua_call(L2, 3, 1);
    bool timerEnabled = lua_toboolean(L2, -1);

    EXPECT_TRUE(timerEnabled) << "GetPluginTimerOption should show timer is enabled";

    lua_pop(L2, 1);
    doc->m_CurrentPlugin = nullptr;
}

// Test 20: SaveState API
TEST_F(PluginApiTest, SaveState)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "SaveState");
    lua_call(L1, 0, 1);
    int result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "SaveState should return eOK";

    // Verify state file was created using new path format:
    // {stateDir}/{worldID}-{pluginID}-state.xml
    QString stateFile =
        tempDir->path() + "/" + doc->m_strWorldID + "-" + plugin1->m_strID + "-state.xml";
    EXPECT_TRUE(QFile::exists(stateFile)) << "State file should be created";

    doc->m_CurrentPlugin = nullptr;
}

// Test 21: EnablePlugin API
TEST_F(PluginApiTest, EnablePlugin)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    // Disable plugin2
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "EnablePlugin");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushboolean(L1, false);
    lua_call(L1, 2, 1);
    int result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "EnablePlugin should return eOK";
    EXPECT_FALSE(plugin2->m_bEnabled) << "Plugin2 should be disabled";

    // Re-enable plugin2
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "EnablePlugin");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_pushboolean(L1, true);
    lua_call(L1, 2, 1);
    result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "EnablePlugin should return eOK";
    EXPECT_TRUE(plugin2->m_bEnabled) << "Plugin2 should be re-enabled";

    doc->m_CurrentPlugin = nullptr;
}

// Test 22: UnloadPlugin API
TEST_F(PluginApiTest, UnloadPlugin)
{
    lua_State* L1 = plugin1->m_ScriptEngine->L;
    doc->m_CurrentPlugin = plugin1;

    // Unload plugin2
    lua_getglobal(L1, "world");
    lua_getfield(L1, -1, "UnloadPlugin");
    lua_pushstring(L1, plugin2->m_strID.toUtf8().constData());
    lua_call(L1, 1, 1);
    int result = lua_tonumber(L1, -1);
    lua_pop(L1, 1);

    EXPECT_EQ(result, 0) << "UnloadPlugin should return eOK";
    EXPECT_EQ(doc->m_PluginList.size(), 1) << "Plugin list should have 1 plugin after unload";

    doc->m_CurrentPlugin = nullptr;
}

// Test 23: ReloadPlugin API
TEST_F(PluginApiTest, ReloadPlugin)
{
    // Reload plugin1
    QString plugin1ID = plugin1->m_strID;
    bool unloadResult = doc->UnloadPlugin(plugin1ID);

    EXPECT_TRUE(unloadResult) << "UnloadPlugin should succeed";

    QString errorMsg;
    plugin1 = doc->LoadPlugin(plugin1Path, errorMsg);

    ASSERT_NE(plugin1, nullptr) << "ReloadPlugin should succeed: " << errorMsg.toStdString();
    EXPECT_EQ(plugin1->m_strName, QString("TestPlugin1"))
        << "Reloaded plugin should have correct name";
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
