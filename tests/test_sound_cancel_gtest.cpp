/**
 * test_sound_cancel_gtest.cpp - StopSound vs CancelSound parity tests
 *
 * Behavioral-parity (group 4, target H8) coverage for the split between
 * StopSound and CancelSound, mirroring original MUSHclient:
 *
 *   - CMUSHclientDoc::StopSound (methods_sounds.cpp:361-402) is a pure
 *     DirectSound buffer stop and fires NO OnPluginPlaySound callback.
 *   - CMUSHclientDoc::CancelSound (doc.cpp:7135-7158) fires OnPluginPlaySound
 *     with the empty string; if a plugin handles it, the cancel is skipped.
 *
 * The pre-fix Mushkin merged the callback into StopSound(0), so Lua calls to
 * world.StopSound(0) wrongly fired the plugin callback and always stopped.
 */

#include "../src/automation/plugin.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QTemporaryFile>

class SoundCancelTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();

        pluginFile = new QTemporaryFile();
        pluginFile->setFileTemplate("test-sound-plugin-XXXXXX.xml");
        ASSERT_TRUE(pluginFile->open()) << "Could not create temp file";

        const QString pluginXml = R"(<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<plugin
  name="Sound Test Plugin"
  author="Test Author"
  id="{abcdef12-1234-1234-1234-123456789abc}"
  language="Lua"
  purpose="Test sound callbacks"
  version="1.0"
  save_state="n"
>
<script>
<![CDATA[
playsound_count = 0
playsound_arg = nil

function OnPluginPlaySound(filename)
  playsound_count = playsound_count + 1
  playsound_arg = filename
end
]]>
</script>
</plugin>
</muclient>
)";
        pluginFile->write(pluginXml.toUtf8());
        pluginFile->flush();

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

    int playSoundCount()
    {
        lua_getglobal(L, "playsound_count");
        const int n = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return n;
    }

    std::unique_ptr<WorldDocument> doc;
    Plugin* plugin = nullptr;
    QTemporaryFile* pluginFile = nullptr;
    lua_State* L = nullptr;
};

// StopSound(0) must NOT fire OnPluginPlaySound (deviation H8 #1).
TEST_F(SoundCancelTest, StopSoundDoesNotFireCallback)
{
    EXPECT_EQ(playSoundCount(), 0);

    const qint32 result = doc->StopSound(0);

    EXPECT_EQ(result, eOK) << "StopSound(0) should return eOK";
    EXPECT_EQ(playSoundCount(), 0)
        << "StopSound(0) must not fire OnPluginPlaySound (that is CancelSound's job)";
}

// StopSound on a specific buffer must likewise not fire the callback.
TEST_F(SoundCancelTest, StopSoundBufferDoesNotFireCallback)
{
    doc->StopSound(1);
    EXPECT_EQ(playSoundCount(), 0) << "StopSound(buffer) must not fire OnPluginPlaySound";
}

// CancelSound() fires OnPluginPlaySound with the empty string (deviation H8 #2).
TEST_F(SoundCancelTest, CancelSoundFiresCallback)
{
    EXPECT_EQ(playSoundCount(), 0);

    doc->CancelSound();

    EXPECT_EQ(playSoundCount(), 1) << "CancelSound() should fire OnPluginPlaySound once";

    // The argument is the empty string (a plugin-cancel signal, not a file).
    lua_getglobal(L, "playsound_arg");
    ASSERT_TRUE(lua_isstring(L, -1)) << "OnPluginPlaySound arg should be a string";
    EXPECT_STREQ(lua_tostring(L, -1), "") << "CancelSound should pass the empty string";
    lua_pop(L, 1);
}

// When a plugin defines OnPluginPlaySound, CancelSound treats it as handled and
// skips its own cancel — but StopSound stays callback-free regardless.
TEST_F(SoundCancelTest, CancelSoundSuppressedButStopSoundUnaffected)
{
    doc->CancelSound();
    EXPECT_EQ(playSoundCount(), 1) << "CancelSound fires the callback once";

    // StopSound never consults the callback, so the count is unchanged.
    EXPECT_EQ(doc->StopSound(0), eOK);
    EXPECT_EQ(playSoundCount(), 1) << "StopSound(0) must leave the callback count untouched";
}
