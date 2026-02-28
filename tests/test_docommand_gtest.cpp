/**
 * test_docommand_gtest.cpp - GoogleTest version
 * DoCommand and GetInternalCommandsList Lua API Test
 *
 * Verifies:
 * 1. world.DoCommand is registered as a function in the world table
 * 2. world.GetInternalCommandsList is registered as a function in the world table
 * 3. GetInternalCommandsList() returns a table
 * 4. GetInternalCommandsList() contains all expected command names
 * 5. GetInternalCommandsList() returns exactly 9 entries
 * 6. DoCommand() returns eNoSuchCommand (30054) for an unknown command name
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for DoCommand / GetInternalCommandsList tests
class DoCommandTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should be initialised";
        ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should be initialised";
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
        // WorldDocument owns the Lua state — nothing extra to clean up here.
    }

    // Helper: check that tableName.funcName is a function
    bool functionExists(const char* tableName, const char* funcName)
    {
        lua_getglobal(L, tableName);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_getfield(L, -1, funcName);
        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 2);
        return exists;
    }

    std::unique_ptr<WorldDocument> doc;
    lua_State* L = nullptr;
};

// ========== Registration tests ==========

TEST_F(DoCommandTest, DoCommandExistsInWorldTable)
{
    EXPECT_TRUE(functionExists("world", "DoCommand"))
        << "world.DoCommand should be registered as a function";
}

TEST_F(DoCommandTest, GetInternalCommandsListExistsInWorldTable)
{
    EXPECT_TRUE(functionExists("world", "GetInternalCommandsList"))
        << "world.GetInternalCommandsList should be registered as a function";
}

// ========== GetInternalCommandsList tests ==========

TEST_F(DoCommandTest, GetInternalCommandsListReturnsTable)
{
    int rc = luaL_dostring(L, "return world.GetInternalCommandsList()");
    ASSERT_EQ(rc, 0) << "GetInternalCommandsList() should not error: " << lua_tostring(L, -1);
    EXPECT_TRUE(lua_istable(L, -1)) << "GetInternalCommandsList() should return a table";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListCorrectCount)
{
    int rc = luaL_dostring(L, "return #world.GetInternalCommandsList()");
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    ASSERT_TRUE(lua_isnumber(L, -1)) << "# operator should return a number";
    int count = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(count, 9) << "GetInternalCommandsList() should contain exactly 9 entries";
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsConnect)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Connect' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Connect' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsDisconnect)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Disconnect' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Disconnect' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsSave)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Save' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Save' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsReloadScriptFile)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'ReloadScriptFile' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1))
        << "'ReloadScriptFile' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsResetTimers)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'ResetTimers' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'ResetTimers' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsPause)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Pause' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Pause' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsUnpause)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Unpause' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Unpause' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsFreezeOutput)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'FreezeOutput' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'FreezeOutput' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsUnfreezeOutput)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'UnfreezeOutput' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'UnfreezeOutput' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

// ========== DoCommand unknown-command path ==========

// doc(L) reads DOCUMENT_STATE from the registry and returns the pointer
// without dereferencing it. The pointer is only dereferenced when a matching
// command IS found. For unknown commands the loop exhausts without a match and
// luaReturnError(L, eNoSuchCommand) is returned — the doc pointer is never
// used beyond the initial read. WorldDocument constructs its Lua state with
// the real doc pointer already in the registry, so this test exercises the
// real code path without any stub.
TEST_F(DoCommandTest, DoCommandRejectsUnknownCommand)
{
    // eNoSuchCommand == 30054
    const char* code = "return world.DoCommand('NonexistentCommand123')";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << "DoCommand() call itself should not error: " << lua_tostring(L, -1);
    ASSERT_TRUE(lua_isnumber(L, -1)) << "DoCommand() should return a number";
    int error_code = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    EXPECT_EQ(error_code, 30054) << "Unknown command should return eNoSuchCommand (30054)";
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
