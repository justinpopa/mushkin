/**
 * test_bit_library_gtest.cpp - GoogleTest version
 * Bit Library Compatibility Test
 *
 * Tests that the bit library is available and compatible with original MUSHclient:
 * - All original function names (ashr, neg, shl, shr, xor, etc.)
 * - Bitwise operations (band, bor, bxor, bnot)
 * - Shift operations (lshift, rshift, arshift)
 * - Helper functions (test, clear, tonumber, tostring, mod)
 */

#include "test_qt_static.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for bit library tests
class BitLibraryTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to check if a function exists
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

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test that bit library exists
TEST_F(BitLibraryTest, BitLibraryExists)
{
    lua_getglobal(L, "bit");
    EXPECT_TRUE(lua_istable(L, -1)) << "bit library should be a table";
    lua_pop(L, 1);
}

// Test core LuaJIT bit functions
TEST_F(BitLibraryTest, CoreFunctionsExist)
{
    EXPECT_TRUE(functionExists("bit", "band")) << "bit.band should exist";
    EXPECT_TRUE(functionExists("bit", "bor")) << "bit.bor should exist";
    EXPECT_TRUE(functionExists("bit", "bxor")) << "bit.bxor should exist";
    EXPECT_TRUE(functionExists("bit", "bnot")) << "bit.bnot should exist";
    EXPECT_TRUE(functionExists("bit", "lshift")) << "bit.lshift should exist";
    EXPECT_TRUE(functionExists("bit", "rshift")) << "bit.rshift should exist";
    EXPECT_TRUE(functionExists("bit", "arshift")) << "bit.arshift should exist";
}

// Test original MUSHclient compatibility names
TEST_F(BitLibraryTest, CompatibilityNamesExist)
{
    EXPECT_TRUE(functionExists("bit", "ashr")) << "bit.ashr should exist (alias for arshift)";
    EXPECT_TRUE(functionExists("bit", "neg")) << "bit.neg should exist (alias for bnot)";
    EXPECT_TRUE(functionExists("bit", "shl")) << "bit.shl should exist (alias for lshift)";
    EXPECT_TRUE(functionExists("bit", "shr")) << "bit.shr should exist (alias for rshift)";
    EXPECT_TRUE(functionExists("bit", "xor")) << "bit.xor should exist (alias for bxor)";
}

// Test additional original MUSHclient functions
TEST_F(BitLibraryTest, AdditionalFunctionsExist)
{
    EXPECT_TRUE(functionExists("bit", "test")) << "bit.test should exist";
    EXPECT_TRUE(functionExists("bit", "clear")) << "bit.clear should exist";
    EXPECT_TRUE(functionExists("bit", "mod")) << "bit.mod should exist";
    EXPECT_TRUE(functionExists("bit", "tonumber")) << "bit.tonumber should exist";
    EXPECT_TRUE(functionExists("bit", "tostring")) << "bit.tostring should exist";
}

// Test bitwise AND
TEST_F(BitLibraryTest, BitwiseAND)
{
    const char* code = "return bit.band(0x12, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x10);
    lua_pop(L, 1);
}

// Test bitwise OR
TEST_F(BitLibraryTest, BitwiseOR)
{
    const char* code = "return bit.bor(0x12, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x12);
    lua_pop(L, 1);
}

// Test bitwise XOR (both names)
TEST_F(BitLibraryTest, BitwiseXOR)
{
    const char* code1 = "return bit.bxor(0x12, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x02);
    lua_pop(L, 1);

    const char* code2 = "return bit.xor(0x12, 0x10)"; // compatibility name
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x02);
    lua_pop(L, 1);
}

// Test bitwise NOT (both names)
TEST_F(BitLibraryTest, BitwiseNOT)
{
    const char* code1 = "return bit.bnot(0)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), -1);
    lua_pop(L, 1);

    const char* code2 = "return bit.neg(0)"; // compatibility name
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), -1);
    lua_pop(L, 1);
}

// Test left shift (both names)
TEST_F(BitLibraryTest, LeftShift)
{
    const char* code1 = "return bit.lshift(1, 4)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 16);
    lua_pop(L, 1);

    const char* code2 = "return bit.shl(1, 4)"; // compatibility name
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 16);
    lua_pop(L, 1);
}

// Test right shift (both names)
TEST_F(BitLibraryTest, RightShift)
{
    const char* code1 = "return bit.rshift(16, 4)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 1);
    lua_pop(L, 1);

    const char* code2 = "return bit.shr(16, 4)"; // compatibility name
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 1);
    lua_pop(L, 1);
}

// Test arithmetic right shift (both names)
TEST_F(BitLibraryTest, ArithmeticRightShift)
{
    const char* code1 = "return bit.arshift(-16, 2)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), -4);
    lua_pop(L, 1);

    const char* code2 = "return bit.ashr(-16, 2)"; // compatibility name
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), -4);
    lua_pop(L, 1);
}

// Test bit.test function
TEST_F(BitLibraryTest, TestFunction)
{
    const char* code1 = "return bit.test(0x42, 0x02)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    const char* code2 = "return bit.test(0x42, 0x40, 0x02)";
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    const char* code3 = "return bit.test(0x02, 0x03)";
    ASSERT_EQ(luaL_dostring(L, code3), 0) << lua_tostring(L, -1);
    EXPECT_FALSE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// Test bit.clear function
TEST_F(BitLibraryTest, ClearFunction)
{
    const char* code1 = "return bit.clear(0x111, 0x01)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x110);
    lua_pop(L, 1);

    const char* code2 = "return bit.clear(0x111, 0x01, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x100);
    lua_pop(L, 1);
}

// Test bit.mod function
TEST_F(BitLibraryTest, ModFunction)
{
    const char* code = "return bit.mod(17, 5)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 2);
    lua_pop(L, 1);
}

// Test bit.tonumber function
TEST_F(BitLibraryTest, ToNumberFunction)
{
    const char* code1 = "return bit.tonumber('ABCDEF', 16)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0xABCDEF);
    lua_pop(L, 1);

    const char* code2 = "return bit.tonumber('1010', 2)";
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 10);
    lua_pop(L, 1);
}

// Test bit.tostring function
TEST_F(BitLibraryTest, ToStringFunction)
{
    const char* code1 = "return bit.tostring(255, 16)";
    ASSERT_EQ(luaL_dostring(L, code1), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "FF");
    lua_pop(L, 1);

    const char* code2 = "return bit.tostring(10, 2)";
    ASSERT_EQ(luaL_dostring(L, code2), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "1010");
    lua_pop(L, 1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
