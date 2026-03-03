/**
 * test_lua_libraries_gtest.cpp - Consolidated Lua library tests
 *
 * Tests bit, lfs, and progress libraries in a single executable.
 * Consolidated from test_bit_library_gtest.cpp, test_lfs_gtest.cpp,
 * and test_progress_library_gtest.cpp to reduce test infrastructure duplication.
 */

#include "fixtures/world_fixtures.h"
#include <QTemporaryDir>

// ========== Bit Library Tests ==========

class BitLibraryTest : public LuaWorldTest {};

TEST_F(BitLibraryTest, BitLibraryExists)
{
    lua_getglobal(L, "bit");
    EXPECT_TRUE(lua_istable(L, -1)) << "bit library should be a table";
    lua_pop(L, 1);
}

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

TEST_F(BitLibraryTest, CompatibilityNamesExist)
{
    EXPECT_TRUE(functionExists("bit", "ashr")) << "bit.ashr should exist (alias for arshift)";
    EXPECT_TRUE(functionExists("bit", "neg")) << "bit.neg should exist (alias for bnot)";
    EXPECT_TRUE(functionExists("bit", "shl")) << "bit.shl should exist (alias for lshift)";
    EXPECT_TRUE(functionExists("bit", "shr")) << "bit.shr should exist (alias for rshift)";
    EXPECT_TRUE(functionExists("bit", "xor")) << "bit.xor should exist (alias for bxor)";
}

TEST_F(BitLibraryTest, AdditionalFunctionsExist)
{
    EXPECT_TRUE(functionExists("bit", "test")) << "bit.test should exist";
    EXPECT_TRUE(functionExists("bit", "clear")) << "bit.clear should exist";
    EXPECT_TRUE(functionExists("bit", "mod")) << "bit.mod should exist";
    EXPECT_TRUE(functionExists("bit", "tonumber")) << "bit.tonumber should exist";
    EXPECT_TRUE(functionExists("bit", "tostring")) << "bit.tostring should exist";
}

TEST_F(BitLibraryTest, BitwiseAND)
{
    const char* code = "return bit.band(0x12, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x10);
    lua_pop(L, 1);
}

TEST_F(BitLibraryTest, BitwiseOR)
{
    const char* code = "return bit.bor(0x12, 0x10)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 0x12);
    lua_pop(L, 1);
}

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

TEST_F(BitLibraryTest, ModFunction)
{
    const char* code = "return bit.mod(17, 5)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 2);
    lua_pop(L, 1);
}

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

// ========== LFS (LuaFileSystem) Tests ==========

class LfsLibraryTest : public LuaWorldTest {};

TEST_F(LfsLibraryTest, LfsLibraryExists)
{
    lua_getglobal(L, "lfs");
    EXPECT_TRUE(lua_istable(L, -1)) << "lfs global should be a table";
    lua_pop(L, 1);
}

TEST_F(LfsLibraryTest, RequireLfsWorks)
{
    const char* code = "return require('lfs')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_istable(L, -1)) << "require('lfs') should return a table";
    lua_pop(L, 1);
}

TEST_F(LfsLibraryTest, CoreFunctionsExist)
{
    EXPECT_TRUE(functionExists("lfs", "currentdir")) << "lfs.currentdir should exist";
    EXPECT_TRUE(functionExists("lfs", "dir")) << "lfs.dir should exist";
    EXPECT_TRUE(functionExists("lfs", "mkdir")) << "lfs.mkdir should exist";
    EXPECT_TRUE(functionExists("lfs", "rmdir")) << "lfs.rmdir should exist";
    EXPECT_TRUE(functionExists("lfs", "attributes")) << "lfs.attributes should exist";
    EXPECT_TRUE(functionExists("lfs", "chdir")) << "lfs.chdir should exist";
    EXPECT_TRUE(functionExists("lfs", "lock")) << "lfs.lock should exist";
    EXPECT_TRUE(functionExists("lfs", "touch")) << "lfs.touch should exist";
    EXPECT_TRUE(functionExists("lfs", "symlinkattributes")) << "lfs.symlinkattributes should exist";
}

TEST_F(LfsLibraryTest, CurrentdirReturnsString)
{
    const char* code = "return lfs.currentdir()";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    ASSERT_TRUE(lua_isstring(L, -1)) << "lfs.currentdir() should return a string";
    const char* result = lua_tostring(L, -1);
    EXPECT_NE(result, nullptr);
    EXPECT_GT(strlen(result), 0u) << "lfs.currentdir() should return a non-empty string";
    lua_pop(L, 1);
}

TEST_F(LfsLibraryTest, AttributesReturnsTable)
{
    const char* code = "return lfs.attributes('.')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    ASSERT_TRUE(lua_istable(L, -1)) << "lfs.attributes('.') should return a table";
    lua_getfield(L, -1, "mode");
    ASSERT_TRUE(lua_isstring(L, -1)) << "attributes table should have a 'mode' string field";
    EXPECT_STREQ(lua_tostring(L, -1), "directory") << "mode of '.' should be 'directory'";
    lua_pop(L, 2); // pop "mode" string and the attributes table
}

TEST_F(LfsLibraryTest, MkdirRmdirRoundTrip)
{
    QTemporaryDir tempBase;
    ASSERT_TRUE(tempBase.isValid()) << "Failed to create temporary base directory";

    // Build a path for a subdirectory that does not yet exist
    QString subPath = tempBase.path() + "/lfs_test_subdir";
    QByteArray subPathBytes = subPath.toUtf8();

    // Push the path into Lua and call lfs.mkdir
    lua_getglobal(L, "lfs");
    lua_getfield(L, -1, "mkdir");
    lua_pushstring(L, subPathBytes.constData());
    int mkdirRet = lua_pcall(L, 1, 2, 0);
    ASSERT_EQ(mkdirRet, 0) << "lfs.mkdir pcall failed: " << lua_tostring(L, -1);

    // lfs.mkdir returns true on success, or nil + error message on failure
    bool mkdirOk = lua_toboolean(L, -2);
    if (!mkdirOk) {
        const char* err = lua_isstring(L, -1) ? lua_tostring(L, -1) : "(no error message)";
        FAIL() << "lfs.mkdir failed: " << err;
    }
    lua_pop(L, 2); // pop return values
    lua_pop(L, 1); // pop lfs table

    // Call lfs.rmdir on the newly created directory
    lua_getglobal(L, "lfs");
    lua_getfield(L, -1, "rmdir");
    lua_pushstring(L, subPathBytes.constData());
    int rmdirRet = lua_pcall(L, 1, 2, 0);
    ASSERT_EQ(rmdirRet, 0) << "lfs.rmdir pcall failed: " << lua_tostring(L, -1);

    bool rmdirOk = lua_toboolean(L, -2);
    if (!rmdirOk) {
        const char* err = lua_isstring(L, -1) ? lua_tostring(L, -1) : "(no error message)";
        FAIL() << "lfs.rmdir failed: " << err;
    }
    lua_pop(L, 2); // pop return values
    lua_pop(L, 1); // pop lfs table
}

// ========== Progress Library Tests ==========

class ProgressLibraryTest : public LuaWorldTest {};

TEST_F(ProgressLibraryTest, ProgressLibraryExists)
{
    lua_getglobal(L, "progress");
    EXPECT_TRUE(lua_istable(L, -1)) << "progress library should be a table";
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, NewFunctionExists)
{
    EXPECT_TRUE(functionExists("progress", "new")) << "progress.new should exist";
}

TEST_F(ProgressLibraryTest, CreateDialogDefaultTitle)
{
    const char* code = "return progress.new()";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_isuserdata(L, -1)) << "progress.new() should return userdata";

    // Verify it's the correct userdata type
    const char* checkCode = "local dlg = progress.new(); return tostring(dlg)";
    ASSERT_EQ(luaL_dostring(L, checkCode), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "progress_dialog");
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, CreateDialogCustomTitle)
{
    const char* code = "return progress.new('Loading Data...')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_isuserdata(L, -1)) << "progress.new('title') should return userdata";
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, SetStatus)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:status('Processing item 1')
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, SetRange)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:range(0, 200)
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, SetPosition)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:range(0, 100)
        dlg:position(50)
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, SetStepAndStep)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:range(0, 100)
        dlg:position(0)
        dlg:setstep(10)
        dlg:step()  -- Should advance by 10
        dlg:step()  -- Should advance by another 10
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, DefaultStepValue)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:range(0, 100)
        dlg:position(0)
        dlg:step()  -- Should advance by 1 (default)
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, CheckCancel)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        local cancelled = dlg:checkcancel()
        dlg:close()
        return type(cancelled) == 'boolean'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "checkcancel() should return a boolean";
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, CloseDialog)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, TypicalUsagePattern)
{
    const char* code = R"(
        local dlg = progress.new('Loading...')
        dlg:range(0, 100)

        for i = 1, 100 do
            dlg:position(i)
            dlg:status('Processing item ' .. i)

            -- Simulate some work (without actual delay)

            -- Check for cancellation
            if dlg:checkcancel() then
                break
            end
        end

        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, StepIncrementPattern)
{
    const char* code = R"(
        local dlg = progress.new('Processing...')
        dlg:range(0, 100)
        dlg:position(0)
        dlg:setstep(5)

        for i = 1, 20 do
            dlg:step()
            dlg:status('Step ' .. i)
        end

        dlg:close()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, GarbageCollection)
{
    const char* code = R"(
        do
            local dlg = progress.new('Test')
            dlg:range(0, 100)
            dlg:position(50)
            -- Dialog should be garbage collected when going out of scope
        end
        collectgarbage()
        return true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, ErrorHandlingInvalidRange)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        local ok, err = pcall(function()
            dlg:range('invalid', 100)
        end)
        dlg:close()
        return not ok  -- Should fail
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "range() should error with invalid parameters";
    lua_pop(L, 1);
}

TEST_F(ProgressLibraryTest, ErrorHandlingInvalidPosition)
{
    const char* code = R"(
        local dlg = progress.new('Test')
        local ok, err = pcall(function()
            dlg:position('invalid')
        end)
        dlg:close()
        return not ok  -- Should fail
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "position() should error with invalid parameters";
    lua_pop(L, 1);
}
