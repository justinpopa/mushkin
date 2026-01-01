/**
 * test_plugin_callbacks_gtest.cpp - GoogleTest version
 * Plugin Callback System Test
 *
 * Tests plugin callback execution including:
 * - GetPluginDispid() checking if callbacks exist
 * - ExecutePluginScript() calling Lua functions with various parameters
 * - SendToAllPluginCallbacks() iterating through plugins
 * - Return value propagation (true/false from callbacks)
 */

#include "test_qt_static.h"
#include "../src/automation/plugin.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryFile>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for plugin callback tests
class PluginCallbacksTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = new WorldDocument();

        // Create test plugin XML file
        pluginFile = new QTemporaryFile();
        pluginFile->setFileTemplate("test-plugin-XXXXXX.xml");
        ASSERT_TRUE(pluginFile->open()) << "Could not create temp file";

        QString pluginXml = R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin
  name="Test Plugin"
  author="Test Author"
  id="{12345678-1234-1234-1234-123456789012}"
  language="Lua"
  purpose="Test plugin callbacks"
  version="1.0"
  save_state="n"
>

<script>
<![CDATA[
-- Global counter to track callback invocations
callback_count = 0
callback_args = {}

-- Callback with no parameters
function OnPluginInstall()
  callback_count = callback_count + 1
  callback_args.install = "called"
  return true
end

-- Callback with string parameter
function OnPluginLineReceived(line)
  callback_count = callback_count + 1
  callback_args.line = line
  return true
end

-- Callback that returns false (stops propagation)
function OnPluginSend(text)
  callback_count = callback_count + 1
  callback_args.send = text
  return false  -- Stop propagation
end

-- Callback with int + string parameters
function OnPluginTelnetOption(option, text)
  callback_count = callback_count + 1
  callback_args.telnet_option = option
  callback_args.telnet_text = text
  return true
end

-- Callback with int + int + string parameters
function OnPluginTelnetSubnegotiation(option, suboption, data)
  callback_count = callback_count + 1
  callback_args.telnet_subneg_option = option
  callback_args.telnet_subneg_suboption = suboption
  callback_args.telnet_subneg_data = data
  return true
end
]]>
</script>

</plugin>
</muclient>
)";

        pluginFile->write(pluginXml.toUtf8());
        pluginFile->flush();

        // Load plugin
        QString errorMsg;
        plugin = doc->LoadPlugin(pluginFile->fileName(), errorMsg);
        ASSERT_NE(plugin, nullptr) << "Could not load plugin: " << errorMsg.toStdString();
        ASSERT_NE(plugin->m_ScriptEngine, nullptr) << "Plugin has no script engine";

        L = plugin->m_ScriptEngine->L;
    }

    void TearDown() override
    {
        delete pluginFile;
        delete doc;
    }

    WorldDocument* doc = nullptr;
    Plugin* plugin = nullptr;
    QTemporaryFile* pluginFile = nullptr;
    lua_State* L = nullptr;
};

// Test 1: GetPluginDispid - Check which callbacks exist
TEST_F(PluginCallbacksTest, GetPluginDispid_ExistingCallbacks)
{
    qint32 dispid_install = plugin->GetPluginDispid(ON_PLUGIN_INSTALL);
    qint32 dispid_line = plugin->GetPluginDispid(ON_PLUGIN_LINE_RECEIVED);

    EXPECT_EQ(dispid_install, 1) << "OnPluginInstall should exist (dispid = 1)";
    EXPECT_EQ(dispid_line, 1) << "OnPluginLineReceived should exist (dispid = 1)";
}

// Test 2: GetPluginDispid - Non-existent callback
TEST_F(PluginCallbacksTest, GetPluginDispid_NonExistentCallback)
{
    qint32 dispid_nonexistent = plugin->GetPluginDispid("OnPluginNonExistent");
    EXPECT_EQ(dispid_nonexistent, -1) << "Non-existent callback should return -1";
}

// Test 3: ExecutePluginScript - No parameters
TEST_F(PluginCallbacksTest, ExecutePluginScript_NoParameters)
{
    // Get count before
    lua_getglobal(L, "callback_count");
    int countBefore = lua_tointeger(L, -1);
    lua_pop(L, 1);

    // Save current plugin context
    Plugin* savedPlugin = doc->m_CurrentPlugin;
    doc->m_CurrentPlugin = plugin;

    // Execute callback
    bool result = plugin->ExecutePluginScript(ON_PLUGIN_INSTALL);

    doc->m_CurrentPlugin = savedPlugin;

    EXPECT_TRUE(result) << "Callback should return true";

    // Check count after
    lua_getglobal(L, "callback_count");
    int countAfter = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(countAfter, countBefore + 1) << "Callback count should increment by 1";

    // Verify callback was called
    lua_getglobal(L, "callback_args");
    lua_getfield(L, -1, "install");
    const char* installArg = lua_tostring(L, -1);
    lua_pop(L, 2);

    EXPECT_STREQ(installArg, "called") << "Install callback should have been called";
}

// Test 4: ExecutePluginScript - String parameter
TEST_F(PluginCallbacksTest, ExecutePluginScript_StringParameter)
{
    doc->m_CurrentPlugin = plugin;
    bool result = plugin->ExecutePluginScript(ON_PLUGIN_LINE_RECEIVED, "Test line from MUD");
    doc->m_CurrentPlugin = nullptr;

    EXPECT_TRUE(result) << "Callback should return true";

    // Check that parameter was passed
    lua_getglobal(L, "callback_args");
    lua_getfield(L, -1, "line");
    const char* lineArg = lua_tostring(L, -1);
    lua_pop(L, 2);

    EXPECT_STREQ(lineArg, "Test line from MUD") << "String parameter should be passed correctly";
}

// Test 5: ExecutePluginScript - Return false (stop propagation)
TEST_F(PluginCallbacksTest, ExecutePluginScript_ReturnFalse)
{
    doc->m_CurrentPlugin = plugin;
    bool result = plugin->ExecutePluginScript(ON_PLUGIN_SEND, "look north");
    doc->m_CurrentPlugin = nullptr;

    EXPECT_FALSE(result) << "Callback should return false to stop propagation";
}

// Test 6: ExecutePluginScript - Int + String parameters
TEST_F(PluginCallbacksTest, ExecutePluginScript_IntStringParameters)
{
    doc->m_CurrentPlugin = plugin;
    bool result = plugin->ExecutePluginScript(ON_PLUGIN_TELNET_OPTION, 24, "terminal-type");
    doc->m_CurrentPlugin = nullptr;

    EXPECT_TRUE(result) << "Callback should return true";

    // Check parameters
    lua_getglobal(L, "callback_args");
    lua_getfield(L, -1, "telnet_option");
    int optionArg = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "telnet_text");
    const char* textArg = lua_tostring(L, -1);
    lua_pop(L, 2);

    EXPECT_EQ(optionArg, 24) << "Int parameter should be passed correctly";
    EXPECT_STREQ(textArg, "terminal-type") << "String parameter should be passed correctly";
}

// Test 7: ExecutePluginScript - Int + Int + String parameters
TEST_F(PluginCallbacksTest, ExecutePluginScript_IntIntStringParameters)
{
    doc->m_CurrentPlugin = plugin;
    bool result =
        plugin->ExecutePluginScript(ON_PLUGIN_TELNET_SUBNEGOTIATION, 86, 1, "compress-data");
    doc->m_CurrentPlugin = nullptr;

    EXPECT_TRUE(result) << "Callback should return true";

    // Check parameters
    lua_getglobal(L, "callback_args");
    lua_getfield(L, -1, "telnet_subneg_option");
    int subnegOption = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "telnet_subneg_suboption");
    int subnegSuboption = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "telnet_subneg_data");
    const char* subnegData = lua_tostring(L, -1);
    lua_pop(L, 2);

    EXPECT_EQ(subnegOption, 86) << "First int parameter should be passed correctly";
    EXPECT_EQ(subnegSuboption, 1) << "Second int parameter should be passed correctly";
    EXPECT_STREQ(subnegData, "compress-data") << "String parameter should be passed correctly";
}

// Test 8: SendToAllPluginCallbacks - No args
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_NoArgs)
{
    // Get count before
    lua_getglobal(L, "callback_count");
    int countBefore = lua_tointeger(L, -1);
    lua_pop(L, 1);

    doc->SendToAllPluginCallbacks(ON_PLUGIN_INSTALL);

    // Get count after
    lua_getglobal(L, "callback_count");
    int countAfter = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_GT(countAfter, countBefore) << "SendToAllPluginCallbacks should call plugin callback";
}

// Test 9: SendToAllPluginCallbacks - With args, stop on false
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_StopOnFalse)
{
    bool result = doc->SendToAllPluginCallbacks(ON_PLUGIN_SEND, "test command", true);

    EXPECT_FALSE(result) << "SendToAllPluginCallbacks should stop on false return";
}

// Test 10: ExecutePluginScript - Non-existent callback
TEST_F(PluginCallbacksTest, ExecutePluginScript_NonExistentCallback)
{
    doc->m_CurrentPlugin = plugin;
    bool result = plugin->ExecutePluginScript("OnPluginNonExistent");
    doc->m_CurrentPlugin = nullptr;

    EXPECT_TRUE(result) << "Non-existent callback should return true (default = continue)";
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
