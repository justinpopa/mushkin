/**
 * test_config_introspection_gtest.cpp - GoogleTest version
 * Test config introspection Lua API: GetDefaultValue, GetCurrentValue, GetLoadedValue
 *
 * Verifies:
 * 1. GetDefaultValue returns the static default from OptionsTable / AlphaOptionsTable
 * 2. GetCurrentValue mirrors world.GetOption for numeric options
 * 3. GetCurrentValue returns the live string field for alpha options
 * 4. GetCurrentValue reflects direct C++ mutations
 * 5. GetLoadedValue returns nil when no XML snapshot exists
 * 6. GetLoadedValue returns the snapshot value after manual population
 * 7. GetLoadedValue and GetCurrentValue can differ when the field is mutated post-load
 * 8. Unknown option names return nil
 * 9. Option name matching is case-insensitive
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/config_options.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

class ConfigIntrospectionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();

        doc->m_mush_name = "Test World";
        doc->m_server = "test.mud.com";
        doc->m_port = 4000;
        doc->m_connectionManager->m_iConnectPhase = eConnectConnectedToMud;
        doc->m_bUTF_8 = true;

        doc->m_currentLine =
            std::make_unique<Line>(1, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_RGB;
        initialStyle->iForeColour = qRgb(192, 192, 192);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));

        doc->m_iFlags = COLOUR_RGB;
        doc->m_iForeColour = qRgb(192, 192, 192);
        doc->m_iBackColour = qRgb(0, 0, 0);

        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should exist";
        ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should exist";
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
    }

    void executeLua(const char* code)
    {
        ASSERT_EQ(luaL_loadstring(L, code), 0) << "Lua compile: " << code;
        ASSERT_EQ(lua_pcall(L, 0, 0, 0), 0) << "Lua exec: " << code;
    }

    double getGlobalNumber(const char* name)
    {
        lua_getglobal(L, name);
        double result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    QString getGlobalString(const char* name)
    {
        lua_getglobal(L, name);
        const char* str = lua_tostring(L, -1);
        QString result = str ? QString::fromUtf8(str) : QString();
        lua_pop(L, 1);
        return result;
    }

    bool isGlobalNil(const char* name)
    {
        lua_getglobal(L, name);
        bool result = lua_isnil(L, -1);
        lua_pop(L, 1);
        return result;
    }

    std::unique_ptr<WorldDocument> doc;
    lua_State* L = nullptr;
};

// Test 1: GetDefaultValue returns the static default for a numeric option
TEST_F(ConfigIntrospectionTest, GetDefaultValueNumeric)
{
    executeLua("default_port = world.GetDefaultValue('port')");
    double defaultPort = getGlobalNumber("default_port");
    EXPECT_EQ(defaultPort, 4000.0) << "GetDefaultValue('port') should return 4000";
}

// Test 2: GetDefaultValue returns the static default for an alpha option
TEST_F(ConfigIntrospectionTest, GetDefaultValueAlpha)
{
    executeLua("default_name = world.GetDefaultValue('name')");
    QString defaultName = getGlobalString("default_name");
    EXPECT_EQ(defaultName, "") << "GetDefaultValue('name') should return empty string";
}

// Test 3: GetCurrentValue matches world.GetOption for numeric options
TEST_F(ConfigIntrospectionTest, GetCurrentValueMatchesGetOption)
{
    executeLua("opt_port = world.GetOption('port')");
    executeLua("cur_port = world.GetCurrentValue('port')");
    double optPort = getGlobalNumber("opt_port");
    double curPort = getGlobalNumber("cur_port");
    EXPECT_EQ(curPort, optPort) << "GetCurrentValue('port') should match GetOption('port')";
}

// Test 4: GetCurrentValue returns the live string field for alpha options
TEST_F(ConfigIntrospectionTest, GetCurrentValueAlpha)
{
    executeLua("cur_name = world.GetCurrentValue('name')");
    QString curName = getGlobalString("cur_name");
    EXPECT_EQ(curName, doc->m_mush_name)
        << "GetCurrentValue('name') should return the world name set in SetUp";
}

// Test 5: GetCurrentValue reflects direct C++ mutations to the field
TEST_F(ConfigIntrospectionTest, GetCurrentValueReflectsSetOption)
{
    doc->m_port = 9999;
    executeLua("cur_port = world.GetCurrentValue('port')");
    double curPort = getGlobalNumber("cur_port");
    EXPECT_EQ(curPort, 9999.0)
        << "GetCurrentValue('port') should return 9999 after directly setting m_port";
}

// Test 6: GetLoadedValue returns nil when no XML snapshot has been taken
TEST_F(ConfigIntrospectionTest, GetLoadedValueNilWithoutLoad)
{
    executeLua("loaded_port = world.GetLoadedValue('port')");
    EXPECT_TRUE(isGlobalNil("loaded_port"))
        << "GetLoadedValue('port') should be nil when no snapshot exists";
}

// Test 7: GetLoadedValue returns the snapshot value after manual population
TEST_F(ConfigIntrospectionTest, GetLoadedValueAfterSnapshot)
{
    doc->m_loadedNumericOptions["port"] = 4000.0;
    executeLua("loaded_port = world.GetLoadedValue('port')");
    double loadedPort = getGlobalNumber("loaded_port");
    EXPECT_EQ(loadedPort, 4000.0) << "GetLoadedValue('port') should return 4000 from the snapshot";
}

// Test 8: GetLoadedValue and GetCurrentValue can diverge after mutation
TEST_F(ConfigIntrospectionTest, GetLoadedValueDiffersFromCurrent)
{
    doc->m_loadedNumericOptions["port"] = 4000.0;
    doc->m_port = 9999;

    executeLua("cur_port = world.GetCurrentValue('port')");
    executeLua("loaded_port = world.GetLoadedValue('port')");

    double curPort = getGlobalNumber("cur_port");
    double loadedPort = getGlobalNumber("loaded_port");

    EXPECT_EQ(curPort, 9999.0) << "GetCurrentValue('port') should reflect the mutated value 9999";
    EXPECT_EQ(loadedPort, 4000.0)
        << "GetLoadedValue('port') should still return the snapshot value 4000";
}

// Test 9: Unknown option names return nil
TEST_F(ConfigIntrospectionTest, UnknownOptionReturnsNil)
{
    executeLua("unknown_val = world.GetCurrentValue('nonexistent_option_xyz')");
    EXPECT_TRUE(isGlobalNil("unknown_val"))
        << "GetCurrentValue with an unknown option name should return nil";
}

// Test 10: Option name matching is case-insensitive
TEST_F(ConfigIntrospectionTest, CaseInsensitiveMatching)
{
    executeLua("lower_port = world.GetCurrentValue('port')");
    executeLua("upper_port = world.GetCurrentValue('Port')");
    double lowerPort = getGlobalNumber("lower_port");
    double upperPort = getGlobalNumber("upper_port");
    EXPECT_EQ(upperPort, lowerPort)
        << "GetCurrentValue should match option names case-insensitively";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
