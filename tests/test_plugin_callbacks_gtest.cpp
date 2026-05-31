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

#include "../src/automation/plugin.h"
#include "fixtures/world_fixtures.h"
#include <QFile>
#include <QTemporaryFile>

// Test fixture for plugin callback tests
class PluginCallbacksTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document
        doc = std::make_unique<WorldDocument>();

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
  Note("note from OnPluginInstall")
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
  Note("note from OnPluginSend")
  return false  -- Stop propagation
end

-- Callback with int + string parameters
function OnPluginTelnetOption(option, text)
  callback_count = callback_count + 1
  callback_args.telnet_option = option
  callback_args.telnet_text = text
  Note("note from OnPluginTelnetOption")
  return true
end

-- Callback with int + int + string parameters
function OnPluginTelnetSubnegotiation(option, suboption, data)
  callback_count = callback_count + 1
  callback_args.telnet_subneg_option = option
  callback_args.telnet_subneg_suboption = suboption
  callback_args.telnet_subneg_data = data
  Note("note from OnPluginTelnetSubnegotiation")
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
    }

    std::unique_ptr<WorldDocument> doc;
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

// ===========================================================================
// Deferred-note batching (m_bNotesNotWantedNow) parity tests.
//
// Original plugins.cpp sets m_bNotesNotWantedNow=true before the plugin loop
// and false after, in ALL six SendToAllPluginCallbacks overloads. While the
// flag is set, Note()/Tell() issued from a callback are queued in
// m_OutstandingLines instead of being written straight to the output window
// (output_formatter.cpp:48,99). These overloads do NOT drain the queue
// themselves (the caller does via OutputOutstandingLines()), so a note issued
// from a callback must still be present in m_OutstandingLines after the call
// returns. Before the fix, four overloads never set the flag, so the note was
// emitted immediately and the queue stayed empty.
// ===========================================================================

// Test 11: no-args overload batches notes issued from the callback
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_NoArgs_BatchesNotes)
{
    doc->m_OutstandingLines.clear();

    doc->SendToAllPluginCallbacks(ON_PLUGIN_INSTALL);

    EXPECT_FALSE(doc->m_OutstandingLines.empty())
        << "Note from no-args callback should be deferred to the outstanding queue";
    EXPECT_FALSE(doc->m_bNotesNotWantedNow) << "flag must be reset to false after the call";
}

// Test 12: string+bool overload batches notes issued from the callback
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_StringBool_BatchesNotes)
{
    doc->m_OutstandingLines.clear();

    doc->SendToAllPluginCallbacks(ON_PLUGIN_SEND, "test command", true);

    EXPECT_FALSE(doc->m_OutstandingLines.empty())
        << "Note from string+bool callback should be deferred to the outstanding queue";
    EXPECT_FALSE(doc->m_bNotesNotWantedNow) << "flag must be reset to false after the call";
}

// Test 13: int+string+bool overload batches notes issued from the callback
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_IntStringBool_BatchesNotes)
{
    doc->m_OutstandingLines.clear();

    doc->SendToAllPluginCallbacks(ON_PLUGIN_TELNET_OPTION, 24, "terminal-type", false);

    EXPECT_FALSE(doc->m_OutstandingLines.empty())
        << "Note from int+string callback should be deferred to the outstanding queue";
    EXPECT_FALSE(doc->m_bNotesNotWantedNow) << "flag must be reset to false after the call";
}

// Test 14: int+int+string overload batches notes issued from the callback
TEST_F(PluginCallbacksTest, SendToAllPluginCallbacks_IntIntString_BatchesNotes)
{
    doc->m_OutstandingLines.clear();

    doc->SendToAllPluginCallbacks(ON_PLUGIN_TELNET_SUBNEGOTIATION, 86, 1, "compress-data");

    EXPECT_FALSE(doc->m_OutstandingLines.empty())
        << "Note from int+int+string callback should be deferred to the outstanding queue";
    EXPECT_FALSE(doc->m_bNotesNotWantedNow) << "flag must be reset to false after the call";
}

// ===========================================================================
// PluginListChanged batching parity (M190).
//
// The Plugin Management dialog's Add/Remove/Reinstall handlers load/unload a
// batch of plugins and must fire OnPluginListChanged exactly ONCE after the
// whole batch, matching original PluginsDlg.cpp OnAddPlugin:382,
// OnDeletePlugin:434, OnReload:543. The WorldDocument primitives support this
// via the suppressListChanged flag on LoadPlugin/UnloadPlugin plus an explicit
// PluginListChanged() call. The tests below pin that contract: a batch of
// suppressed loads followed by one PluginListChanged() notifies an observer
// plugin once, whereas an unsuppressed load notifies per-plugin.
// ===========================================================================

// Helper: build a plugin XML that counts OnPluginListChanged invocations into a
// uniquely-named Lua global so the observer survives the batch operations.
static QString makeObserverPluginXml(const QString& id, const QString& counterGlobal)
{
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin name="Observer %1" author="Test" id="%2" language="Lua"
        purpose="count list-changed" version="1.0" save_state="n">
<script>
<![CDATA[
%3 = 0
function OnPluginListChanged()
  %3 = %3 + 1
end
]]>
</script>
</plugin>
</muclient>
)")
        .arg(id, id, counterGlobal);
}

static Plugin* loadPluginFromString(WorldDocument* doc, const QString& xml, bool suppress)
{
    QTemporaryFile f;
    f.setFileTemplate("observer-plugin-XXXXXX.xml");
    if (!f.open()) {
        return nullptr;
    }
    f.write(xml.toUtf8());
    f.flush();
    QString err;
    return doc->LoadPlugin(f.fileName(), err, suppress);
}

// Test 15: a suppressed batch + one explicit PluginListChanged() notifies once.
TEST_F(PluginCallbacksTest, PluginListChanged_BatchedLoad_NotifiesOnce)
{
    // Install an observer plugin (not suppressed — it isn't part of the batch).
    Plugin* observer = loadPluginFromString(
        doc.get(),
        makeObserverPluginXml("{aaaaaaaa-0000-0000-0000-000000000001}", "list_changed_count"),
        /*suppress=*/false);
    ASSERT_NE(observer, nullptr);
    lua_State* obsL = observer->m_ScriptEngine->L;

    // Reset the counter after install-time notifications settle.
    lua_pushinteger(obsL, 0);
    lua_setglobal(obsL, "list_changed_count");

    // Simulate the dialog's Add handler: load a batch with suppression...
    for (int i = 2; i <= 4; ++i) {
        Plugin* p = loadPluginFromString(
            doc.get(),
            makeObserverPluginXml(QString("{bbbbbbbb-0000-0000-0000-00000000000%1}").arg(i),
                                  QString("ignored_%1").arg(i)),
            /*suppress=*/true);
        ASSERT_NE(p, nullptr) << "batch plugin " << i << " failed to load";
    }

    // ...then fire the single batched notification.
    doc->PluginListChanged();

    lua_getglobal(obsL, "list_changed_count");
    int count = lua_tointeger(obsL, -1);
    lua_pop(obsL, 1);

    EXPECT_EQ(count, 1) << "Batched load must notify OnPluginListChanged exactly once, not "
                           "once per plugin";
}

// ===========================================================================
// SendToFirstPluginCallbacks fall-through on error (H10).
//
// Original plugins.cpp:1380: after executing the callback, the loop checks
// callinfo._dispid_info.isvalid(). If the callback errored, the dispid is
// set to DISPID_UNKNOWN and isvalid() returns false — the loop falls through
// to the next plugin instead of stopping. Mushkin mirrors this by re-checking
// hasCallback() after execution: if the callback cleared its own dispid entry
// (error path in plugin.cpp:346), iteration continues to the next plugin.
// ===========================================================================

// Helper: build a plugin XML where OnPluginTrace errors at runtime.
static QString makeErroringTracePlugin(const QString& id)
{
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin name="ErrorTrace %1" author="Test" id="%2" language="Lua"
        purpose="erroring trace" version="1.0" save_state="n">
<script>
<![CDATA[
function OnPluginTrace(msg)
  -- deliberate runtime error: call an undefined global
  return undefined_function_that_does_not_exist()
end
]]>
</script>
</plugin>
</muclient>
)")
        .arg(id, id);
}

// Helper: build a plugin XML where OnPluginTrace succeeds and records the call.
static QString makeWorkingTracePlugin(const QString& id, const QString& flagGlobal)
{
    return QString(R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin name="WorkTrace %1" author="Test" id="%2" language="Lua"
        purpose="working trace" version="1.0" save_state="n">
<script>
<![CDATA[
%3 = false
function OnPluginTrace(msg)
  %3 = true
  return true
end
]]>
</script>
</plugin>
</muclient>
)")
        .arg(id, id, flagGlobal);
}

// Test 17: SendToFirstPluginCallbacks falls through to the next plugin when
// the first plugin's callback errors at runtime (H10).
TEST_F(PluginCallbacksTest, SendToFirstPluginCallbacks_FallsThroughOnCallbackError)
{
    // Load a plugin whose OnPluginTrace will error at runtime.
    Plugin* errorPlugin = loadPluginFromString(
        doc.get(), makeErroringTracePlugin("{eeeeeeee-0000-0000-0000-000000000001}"),
        /*suppress=*/true);
    ASSERT_NE(errorPlugin, nullptr) << "erroring trace plugin failed to load";

    // Load a second plugin whose OnPluginTrace works correctly.
    Plugin* workPlugin = loadPluginFromString(
        doc.get(),
        makeWorkingTracePlugin("{ffffffff-0000-0000-0000-000000000001}", "trace_handled"),
        /*suppress=*/true);
    ASSERT_NE(workPlugin, nullptr) << "working trace plugin failed to load";
    lua_State* workL = workPlugin->m_ScriptEngine->L;

    // Confirm the working plugin's flag starts false.
    lua_getglobal(workL, "trace_handled");
    bool handledBefore = lua_toboolean(workL, -1);
    lua_pop(workL, 1);
    EXPECT_FALSE(handledBefore);

    // Fire the callback; the first plugin will error, the second should handle it.
    bool result = doc->SendToFirstPluginCallbacks(ON_PLUGIN_TRACE, "test trace message");

    EXPECT_TRUE(result) << "SendToFirstPluginCallbacks should return true (second plugin handled)";

    lua_getglobal(workL, "trace_handled");
    bool handledAfter = lua_toboolean(workL, -1);
    lua_pop(workL, 1);
    EXPECT_TRUE(handledAfter)
        << "Second plugin's OnPluginTrace must run when first plugin's callback errors (H10)";
}

// Test 16: without suppression each load notifies the observer (the deviating
// behavior). Confirms the suppress flag is what makes batching possible.
TEST_F(PluginCallbacksTest, PluginListChanged_UnsuppressedLoad_NotifiesPerPlugin)
{
    Plugin* observer = loadPluginFromString(
        doc.get(),
        makeObserverPluginXml("{cccccccc-0000-0000-0000-000000000001}", "per_plugin_count"),
        /*suppress=*/false);
    ASSERT_NE(observer, nullptr);
    lua_State* obsL = observer->m_ScriptEngine->L;

    lua_pushinteger(obsL, 0);
    lua_setglobal(obsL, "per_plugin_count");

    for (int i = 2; i <= 4; ++i) {
        Plugin* p = loadPluginFromString(
            doc.get(),
            makeObserverPluginXml(QString("{dddddddd-0000-0000-0000-00000000000%1}").arg(i),
                                  QString("ignored2_%1").arg(i)),
            /*suppress=*/false);
        ASSERT_NE(p, nullptr) << "plugin " << i << " failed to load";
    }

    lua_getglobal(obsL, "per_plugin_count");
    int count = lua_tointeger(obsL, -1);
    lua_pop(obsL, 1);

    EXPECT_EQ(count, 3) << "Each unsuppressed load should notify the observer once";
}
