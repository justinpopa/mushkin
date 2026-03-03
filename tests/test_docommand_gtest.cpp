/**
 * test_docommand_gtest.cpp - GoogleTest version
 * DoCommand and GetInternalCommandsList Lua API Test
 *
 * Verifies:
 * 1. world.DoCommand is registered as a function in the world table
 * 2. world.GetInternalCommandsList is registered as a function in the world table
 * 3. GetInternalCommandsList() returns a table
 * 4. GetInternalCommandsList() contains all expected command names
 * 5. GetInternalCommandsList() returns 252 entries (9 original + 17 direction/action +
 *    36 macro + 30 keypad + 17 alt + ~143 UI commands)
 * 6. DoCommand() returns eNoSuchCommand (30054) for an unknown command name
 * 7. Spot-checks: North, CascadeWindows, Find, MacroF1, ConfigureAliases, AltA
 */

#include "fixtures/world_fixtures.h"

// Test fixture for DoCommand / GetInternalCommandsList tests
class DoCommandTest : public LuaWorldTest {};

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
    EXPECT_EQ(count, 252) << "GetInternalCommandsList() should contain exactly 252 entries";
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

// ========== Spot-checks for newly added commands ==========

TEST_F(DoCommandTest, GetInternalCommandsListContainsNorth)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'North' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'North' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsCascadeWindows)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'CascadeWindows' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'CascadeWindows' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsFind)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'Find' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'Find' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsMacroF1)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'MacroF1' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'MacroF1' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsConfigureAliases)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'ConfigureAliases' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1))
        << "'ConfigureAliases' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}

TEST_F(DoCommandTest, GetInternalCommandsListContainsAltA)
{
    const char* code = "local t = world.GetInternalCommandsList()\n"
                       "for _, v in ipairs(t) do\n"
                       "    if v == 'AltA' then return true end\n"
                       "end\n"
                       "return false";
    int rc = luaL_dostring(L, code);
    ASSERT_EQ(rc, 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "'AltA' should be in GetInternalCommandsList()";
    lua_pop(L, 1);
}
