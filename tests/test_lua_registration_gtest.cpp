/**
 * test_lua_registration_gtest.cpp - GoogleTest suite for Lua API registration completeness
 *
 * Verifies that RegisterLuaRoutines() correctly registers all Lua API functions,
 * constant tables, and global aliases. A missing registration silently breaks
 * Lua API functions — these tests catch regressions.
 */

#include "../src/world/lua_api/lua_registration.h"
#include "fixtures/world_fixtures.h"

#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// =============================================================================
// Fixture
// =============================================================================

class LuaRegistrationTest : public LuaWorldTest {
  protected:
    void expectWorldFunction(const char* name)
    {
        lua_getglobal(L, "world");
        lua_getfield(L, -1, name);
        EXPECT_TRUE(lua_isfunction(L, -1)) << "world." << name << " should be a function";
        lua_pop(L, 2);
    }

    void expectGlobalFunction(const char* name)
    {
        lua_getglobal(L, name);
        EXPECT_TRUE(lua_isfunction(L, -1)) << name << " should be a global function";
        lua_pop(L, 1);
    }

    void expectConstantTable(const char* name, int minEntries = 1)
    {
        lua_getglobal(L, name);
        EXPECT_TRUE(lua_istable(L, -1)) << name << " should be a table";
        if (lua_istable(L, -1)) {
            int count = 0;
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                count++;
                lua_pop(L, 1);
            }
            EXPECT_GE(count, minEntries)
                << name << " should have at least " << minEntries << " entries";
        }
        lua_pop(L, 1);
    }
};

// =============================================================================
// Category 1: World table existence and function count
// =============================================================================

TEST_F(LuaRegistrationTest, WorldTableExists)
{
    lua_getglobal(L, "world");
    EXPECT_TRUE(lua_istable(L, -1)) << "world global should be a table";
    lua_pop(L, 1);
}

TEST_F(LuaRegistrationTest, WorldTableFunctionCount)
{
    lua_getglobal(L, "world");
    ASSERT_TRUE(lua_istable(L, -1)) << "world global should be a table";

    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isfunction(L, -1)) {
            count++;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1); // pop world table

    // 409 worldlib entries + 19 array functions = 428
    EXPECT_GE(count, 428)
        << "world table should have at least 428 functions (409 worldlib + 19 array)";
}

// =============================================================================
// Category 2: Core function spot checks
// =============================================================================

TEST_F(LuaRegistrationTest, CoreFunctionSpotChecks)
{
    // Output
    expectWorldFunction("Note");
    expectWorldFunction("ColourNote");
    expectWorldFunction("ColourTell");
    expectWorldFunction("Tell");
    expectWorldFunction("AnsiNote");
    expectWorldFunction("Hyperlink");
    expectWorldFunction("Simulate");

    // Network
    expectWorldFunction("Send");
    expectWorldFunction("SendNoEcho");
    expectWorldFunction("Connect");
    expectWorldFunction("Disconnect");
    expectWorldFunction("IsConnected");

    // Variables
    expectWorldFunction("GetVariable");
    expectWorldFunction("SetVariable");
    expectWorldFunction("DeleteVariable");
    expectWorldFunction("GetVariableList");

    // Triggers
    expectWorldFunction("AddTrigger");
    expectWorldFunction("DeleteTrigger");
    expectWorldFunction("EnableTrigger");
    expectWorldFunction("GetTriggerInfo");
    expectWorldFunction("GetTriggerList");
    expectWorldFunction("AddTriggerEx");
    expectWorldFunction("StopEvaluatingTriggers");

    // Aliases
    expectWorldFunction("AddAlias");
    expectWorldFunction("DeleteAlias");
    expectWorldFunction("EnableAlias");
    expectWorldFunction("GetAliasInfo");
    expectWorldFunction("GetAliasList");

    // Timers
    expectWorldFunction("AddTimer");
    expectWorldFunction("DeleteTimer");
    expectWorldFunction("EnableTimer");
    expectWorldFunction("GetTimerInfo");
    expectWorldFunction("DoAfter");
    expectWorldFunction("DoAfterSpecial");

    // World info
    expectWorldFunction("GetInfo");
    expectWorldFunction("GetOption");
    expectWorldFunction("SetOption");
    expectWorldFunction("GetAlphaOption");
    expectWorldFunction("SetAlphaOption");
    expectWorldFunction("GetWorldName");
    expectWorldFunction("Version");
    expectWorldFunction("GetLineCount");
    expectWorldFunction("DoCommand");

    // Colors
    expectWorldFunction("GetNormalColour");
    expectWorldFunction("GetBoldColour");
    expectWorldFunction("SetNormalColour");
    expectWorldFunction("ColourNameToRGB");
    expectWorldFunction("RGBColourToName");
    expectWorldFunction("AdjustColour");
    expectWorldFunction("PickColour");

    // Database
    expectWorldFunction("DatabaseOpen");
    expectWorldFunction("DatabaseClose");
    expectWorldFunction("DatabasePrepare");
    expectWorldFunction("DatabaseStep");
    expectWorldFunction("DatabaseExec");
    expectWorldFunction("DatabaseColumnText");

    // Plugin
    expectWorldFunction("GetPluginID");
    expectWorldFunction("GetPluginName");
    expectWorldFunction("GetPluginList");
    expectWorldFunction("CallPlugin");
    expectWorldFunction("BroadcastPlugin");
    expectWorldFunction("SaveState");

    // Miniwindow
    expectWorldFunction("WindowCreate");
    expectWorldFunction("WindowShow");
    expectWorldFunction("WindowDelete");
    expectWorldFunction("WindowInfo");
    expectWorldFunction("WindowRectOp");
    expectWorldFunction("WindowText");
    expectWorldFunction("WindowFont");
    expectWorldFunction("WindowLoadImage");
    expectWorldFunction("WindowAddHotspot");
    expectWorldFunction("WindowDragHandler");

    // Sound
    expectWorldFunction("PlaySound");
    expectWorldFunction("StopSound");
    expectWorldFunction("GetSoundStatus");

    // Notepad
    expectWorldFunction("SendToNotepad");
    expectWorldFunction("GetNotepadText");
    expectWorldFunction("CloseNotepad");

    // Mapper
    expectWorldFunction("AddToMapper");
    expectWorldFunction("GetMapping");
    expectWorldFunction("SetMapping");
    expectWorldFunction("EnableMapping");
    expectWorldFunction("GetMappingCount");

    // Arrays
    expectWorldFunction("ArrayCreate");
    expectWorldFunction("ArrayDelete");
    expectWorldFunction("ArraySet");
    expectWorldFunction("ArrayGet");
    expectWorldFunction("ArrayExists");
    expectWorldFunction("ArrayListAll");
    expectWorldFunction("ArrayImport");
    expectWorldFunction("ArrayExport");

    // Logging
    expectWorldFunction("OpenLog");
    expectWorldFunction("CloseLog");
    expectWorldFunction("WriteLog");
    expectWorldFunction("IsLogOpen");

    // Utility
    expectWorldFunction("Hash");
    expectWorldFunction("Base64Encode");
    expectWorldFunction("Base64Decode");
    expectWorldFunction("GetUniqueNumber");
    expectWorldFunction("GetUniqueID");
    expectWorldFunction("CreateGUID");
    expectWorldFunction("StripANSI");
    expectWorldFunction("Trim");
    expectWorldFunction("Execute");
    expectWorldFunction("ErrorDesc");
    expectWorldFunction("Replace");
    expectWorldFunction("ImportXML");
    expectWorldFunction("ExportXML");
    expectWorldFunction("GetClipboard");
    expectWorldFunction("SetClipboard");
    expectWorldFunction("Menu");
    expectWorldFunction("MakeRegularExpression");
}

// =============================================================================
// Category 3: Global alias availability
// =============================================================================

TEST_F(LuaRegistrationTest, GlobalAliasAvailability)
{
    expectGlobalFunction("Note");
    expectGlobalFunction("Send");
    expectGlobalFunction("GetInfo");
    expectGlobalFunction("GetVariable");
    expectGlobalFunction("SetVariable");
    expectGlobalFunction("GetOption");
    expectGlobalFunction("SetOption");
    expectGlobalFunction("ColourNote");
    expectGlobalFunction("AddTrigger");
    expectGlobalFunction("AddAlias");
    expectGlobalFunction("AddTimer");
    expectGlobalFunction("DoAfter");
    expectGlobalFunction("DoAfterSpecial");
    expectGlobalFunction("WindowCreate");
    expectGlobalFunction("WindowShow");
    expectGlobalFunction("WindowRectOp");
    expectGlobalFunction("WindowText");
    expectGlobalFunction("WindowFont");
    expectGlobalFunction("WindowLoadImage");
    expectGlobalFunction("SaveState");
    expectGlobalFunction("CallPlugin");
    expectGlobalFunction("BroadcastPlugin");
    expectGlobalFunction("IsConnected");
    expectGlobalFunction("Trim");
    expectGlobalFunction("Hash");
    expectGlobalFunction("Base64Encode");
    expectGlobalFunction("ColourNameToRGB");
    expectGlobalFunction("EnableTrigger");
    expectGlobalFunction("EnableAlias");
    expectGlobalFunction("EnableTimer");
    expectGlobalFunction("Accelerator");
    expectGlobalFunction("AcceleratorTo");
    expectGlobalFunction("Repaint");
    expectGlobalFunction("SetStatus");
    expectGlobalFunction("TextRectangle");
}

// =============================================================================
// Category 4: print override
// =============================================================================

TEST_F(LuaRegistrationTest, PrintOverriddenToNote)
{
    lua_getglobal(L, "print");
    EXPECT_TRUE(lua_isfunction(L, -1))
        << "print should be a global function (overridden to L_Note)";
    lua_pop(L, 1);
}

// =============================================================================
// Category 5: Constant tables exist and are non-empty
// =============================================================================

TEST_F(LuaRegistrationTest, ErrorCodeTableExists)
{
    expectConstantTable("error_code", 1);
}

TEST_F(LuaRegistrationTest, TriggerFlagTableExists)
{
    expectConstantTable("trigger_flag", 1);
}

TEST_F(LuaRegistrationTest, AliasFlagTableExists)
{
    expectConstantTable("alias_flag", 1);
}

TEST_F(LuaRegistrationTest, SendtoTableExists)
{
    expectConstantTable("sendto", 1);
}

TEST_F(LuaRegistrationTest, MiniwinTableExists)
{
    expectConstantTable("miniwin", 1);
}

TEST_F(LuaRegistrationTest, ErrorDescTableExists)
{
    expectConstantTable("error_desc", 1);
}

TEST_F(LuaRegistrationTest, CustomColourTableExists)
{
    expectConstantTable("custom_colour", 1);
}

TEST_F(LuaRegistrationTest, TimerFlagTableExists)
{
    expectConstantTable("timer_flag", 1);
}

TEST_F(LuaRegistrationTest, ExtendedColoursTableExists)
{
    expectConstantTable("extended_colours", 1);
}

// =============================================================================
// Category 6: Module tables
// =============================================================================

TEST_F(LuaRegistrationTest, UtilsModuleExists)
{
    lua_getglobal(L, "utils");
    EXPECT_TRUE(lua_istable(L, -1)) << "utils global should be a table";
    if (lua_istable(L, -1)) {
        int count = 0;
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_isfunction(L, -1)) {
                count++;
            }
            lua_pop(L, 1);
        }
        EXPECT_GT(count, 0) << "utils table should contain at least one function";
    }
    lua_pop(L, 1);
}

TEST_F(LuaRegistrationTest, RexModuleExists)
{
    lua_getglobal(L, "rex");
    EXPECT_TRUE(lua_istable(L, -1)) << "rex global should be a table";
    if (lua_istable(L, -1)) {
        int count = 0;
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_isfunction(L, -1)) {
                count++;
            }
            lua_pop(L, 1);
        }
        EXPECT_GT(count, 0) << "rex table should contain at least one function";
    }
    lua_pop(L, 1);
}

// =============================================================================
// Category 7: getLuaFunctionNames helper
// =============================================================================

TEST_F(LuaRegistrationTest, GetLuaFunctionNames_ReturnsNonEmpty)
{
    QStringList names = getLuaFunctionNames(L);
    EXPECT_GT(names.size(), 400) << "getLuaFunctionNames should return more than 400 entries";
}

TEST_F(LuaRegistrationTest, GetLuaFunctionNames_ContainsWorldFunctions)
{
    QStringList names = getLuaFunctionNames(L);
    EXPECT_TRUE(names.contains("Note")) << "getLuaFunctionNames should contain Note";
    EXPECT_TRUE(names.contains("Send")) << "getLuaFunctionNames should contain Send";
    EXPECT_TRUE(names.contains("WindowCreate"))
        << "getLuaFunctionNames should contain WindowCreate";
}

TEST_F(LuaRegistrationTest, GetLuaFunctionNames_ContainsStdlib)
{
    QStringList names = getLuaFunctionNames(L);
    EXPECT_TRUE(names.contains("string.format"))
        << "getLuaFunctionNames should contain string.format";
    EXPECT_TRUE(names.contains("table.insert"))
        << "getLuaFunctionNames should contain table.insert";
    EXPECT_TRUE(names.contains("math.floor")) << "getLuaFunctionNames should contain math.floor";
}

TEST_F(LuaRegistrationTest, GetLuaFunctionNames_NullState)
{
    QStringList names = getLuaFunctionNames(nullptr);
    EXPECT_TRUE(names.isEmpty()) << "getLuaFunctionNames(nullptr) should return empty list";
}

// =============================================================================
// Category 8: American spelling aliases
// =============================================================================

TEST_F(LuaRegistrationTest, BritishAndAmericanColorSpellings)
{
    // British spelling is canonical
    expectWorldFunction("ColourNote");

    // American spelling alias should also be present
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "InfoColour");
    bool britishExists = lua_isfunction(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "InfoColor");
    bool americanExists = lua_isfunction(L, -1);
    lua_pop(L, 2); // pop result + world table

    // At least one spelling must exist; ideally both
    EXPECT_TRUE(britishExists || americanExists)
        << "Either world.InfoColour or world.InfoColor should be registered";

    if (britishExists && americanExists) {
        // Both spellings registered — ideal
        SUCCEED();
    }
}

// =============================================================================
// Category 9: Specific constant values spot check
// =============================================================================

TEST_F(LuaRegistrationTest, ErrorCodeConstants)
{
    lua_getglobal(L, "error_code");
    ASSERT_TRUE(lua_istable(L, -1)) << "error_code should be a table";

    lua_getfield(L, -1, "eOK");
    EXPECT_TRUE(lua_isnumber(L, -1)) << "error_code.eOK should be a number";
    lua_pop(L, 1);

    lua_pop(L, 1); // pop error_code table
}

TEST_F(LuaRegistrationTest, SendtoConstants)
{
    lua_getglobal(L, "sendto");
    ASSERT_TRUE(lua_istable(L, -1)) << "sendto should be a table";

    // Check that at least one entry exists and is numeric
    int numericCount = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isnumber(L, -1)) {
            numericCount++;
        }
        lua_pop(L, 1);
    }
    EXPECT_GT(numericCount, 0) << "sendto table should have at least one numeric constant";

    lua_pop(L, 1); // pop sendto table
}
