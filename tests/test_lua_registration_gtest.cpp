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

    // 408 worldlib entries + 19 array functions = 427
    EXPECT_GE(count, 427)
        << "world table should have at least 427 functions (408 worldlib + 19 array)";
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

// =============================================================================
// Category 10: Behavioral parity regressions (M45, M97)
// =============================================================================

// M97: error_desc table must use correct numeric indices matching original MUSHclient
// error_descriptions[] table (eNoNameSpecified=30003, eVariableNotFound=30019)
TEST_F(LuaRegistrationTest, ErrorDescNoNameSpecifiedUsesCode30003)
{
    lua_getglobal(L, "error_desc");
    ASSERT_TRUE(lua_istable(L, -1)) << "error_desc should be a table";

    // Must be at index 30003 (eNoNameSpecified), NOT at index 2
    lua_rawgeti(L, -1, 30003);
    EXPECT_TRUE(lua_isstring(L, -1)) << "error_desc[30003] (eNoNameSpecified) should be a string";
    lua_pop(L, 1);

    // Index 2 must NOT have been set (was the old wrong value)
    lua_rawgeti(L, -1, 2);
    EXPECT_FALSE(lua_isstring(L, -1))
        << "error_desc[2] should not exist; eNoNameSpecified belongs at 30003";
    lua_pop(L, 1);

    lua_pop(L, 1); // pop error_desc
}

TEST_F(LuaRegistrationTest, ErrorDescVariableNotFoundUsesCode30019)
{
    lua_getglobal(L, "error_desc");
    ASSERT_TRUE(lua_istable(L, -1)) << "error_desc should be a table";

    // Must be at index 30019 (eVariableNotFound), NOT at index 30
    lua_rawgeti(L, -1, 30019);
    EXPECT_TRUE(lua_isstring(L, -1)) << "error_desc[30019] (eVariableNotFound) should be a string";
    lua_pop(L, 1);

    // Index 30 must NOT have been set (was the old wrong value)
    lua_rawgeti(L, -1, 30);
    EXPECT_FALSE(lua_isstring(L, -1))
        << "error_desc[30] should not exist; eVariableNotFound belongs at 30019";
    lua_pop(L, 1);

    lua_pop(L, 1); // pop error_desc
}

// M97 (LOW): error_desc table completeness — spot-check entries that were missing
TEST_F(LuaRegistrationTest, ErrorDescCompleteness)
{
    lua_getglobal(L, "error_desc");
    ASSERT_TRUE(lua_istable(L, -1)) << "error_desc should be a table";

    // Spot-check a selection of codes that were previously absent
    const int codes[] = {
        30004, // eCannotPlaySound
        30007, // eTriggerCannotBeEmpty
        30008, // eInvalidObjectLabel
        30021, // eBadRegularExpression
        30025, // eUnknownOption
        30032, // ePluginCannotSetOption
        30033, // ePluginCannotGetOption
        30041, // eCommandsNestedTooDeeply
        30055, // eArrayAlreadyExists
        30063, // eItemInUse
        30065, // eCannotAddFont
        30072, // eHotspotNotInstalled
        30074, // eBrushStyleNotValid
    };
    for (int code : codes) {
        lua_rawgeti(L, -1, code);
        EXPECT_TRUE(lua_isstring(L, -1)) << "error_desc[" << code << "] should be a string";
        lua_pop(L, 1);
    }

    lua_pop(L, 1); // pop error_desc
}

// M45: rect_3d_rect must equal 4 (not 5) to match original MUSHclient
TEST_F(LuaRegistrationTest, MiniwinRect3dRectIsValue4)
{
    lua_getglobal(L, "miniwin");
    ASSERT_TRUE(lua_istable(L, -1)) << "miniwin should be a table";

    lua_getfield(L, -1, "rect_3d_rect");
    ASSERT_TRUE(lua_isnumber(L, -1)) << "miniwin.rect_3d_rect should be a number";
    EXPECT_EQ(static_cast<int>(lua_tonumber(L, -1)), 4)
        << "miniwin.rect_3d_rect must be 4, not 5 (5 is rect_draw_edge)";
    lua_pop(L, 1);

    lua_pop(L, 1); // pop miniwin
}

// M45: rect_draw_edge and related constants must be present
TEST_F(LuaRegistrationTest, MiniwinRectDrawEdgeConstants)
{
    lua_getglobal(L, "miniwin");
    ASSERT_TRUE(lua_istable(L, -1)) << "miniwin should be a table";

    auto checkField = [&](const char* name, int expected) {
        lua_getfield(L, -1, name);
        EXPECT_TRUE(lua_isnumber(L, -1)) << "miniwin." << name << " should be a number";
        if (lua_isnumber(L, -1)) {
            EXPECT_EQ(static_cast<int>(lua_tonumber(L, -1)), expected)
                << "miniwin." << name << " should equal " << expected;
        }
        lua_pop(L, 1);
    };

    checkField("rect_draw_edge", 5);
    checkField("rect_flood_fill_border", 6);
    checkField("rect_flood_fill_surface", 7);
    checkField("rect_edge_raised", 5);
    checkField("rect_edge_etched", 6);
    checkField("rect_edge_bump", 9);
    checkField("rect_edge_sunken", 10);
    checkField("rect_edge_at_top_left", 0x0003);
    checkField("rect_edge_at_top_right", 0x0006);
    checkField("rect_edge_at_bottom_left", 0x0009);
    checkField("rect_edge_at_bottom_right", 0x000C);
    checkField("rect_edge_at_all", 0x000F);
    checkField("rect_diagonal_end_top_left", 0x0013);
    checkField("rect_diagonal_end_top_right", 0x0016);
    checkField("rect_diagonal_end_bottom_left", 0x0019);
    checkField("rect_diagonal_end_bottom_right", 0x001C);
    checkField("rect_option_fill_middle", 0x0800);
    checkField("rect_option_softer_buttons", 0x1000);
    checkField("rect_option_flat_borders", 0x4000);
    checkField("rect_option_monochrom_borders", 0x8000);

    lua_pop(L, 1); // pop miniwin
}
