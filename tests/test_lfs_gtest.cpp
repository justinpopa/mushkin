/**
 * test_lfs_gtest.cpp - GoogleTest version
 * LuaFileSystem (lfs) Integration Test
 *
 * Tests that the lfs library is correctly bundled and registered:
 * - lfs global table exists
 * - Core functions are available
 * - Basic operations work (currentdir, attributes, mkdir/rmdir)
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for lfs library tests
class LfsLibraryTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
    }

    // Helper to check if a function exists in a global table
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

// Test that lfs global is a table
TEST_F(LfsLibraryTest, LfsLibraryExists)
{
    lua_getglobal(L, "lfs");
    EXPECT_TRUE(lua_istable(L, -1)) << "lfs global should be a table";
    lua_pop(L, 1);
}

// Test that require("lfs") returns a table (tests package.loaded registration)
TEST_F(LfsLibraryTest, RequireLfsWorks)
{
    const char* code = "return require('lfs')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_istable(L, -1)) << "require('lfs') should return a table";
    lua_pop(L, 1);
}

// Test that core lfs functions are present
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

// Test that lfs.currentdir() returns a non-empty string
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

// Test that lfs.attributes(".") returns a table with a "mode" field equal to "directory"
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

// Test that lfs.mkdir and lfs.rmdir work as a round-trip
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

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
