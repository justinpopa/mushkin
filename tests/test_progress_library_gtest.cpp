/**
 * test_progress_library_gtest.cpp - GoogleTest version
 * Progress Library Compatibility Test
 *
 * Tests that the progress library is available and compatible with original MUSHclient:
 * - progress.new(title) creates a progress dialog
 * - All methods work correctly (status, range, position, setstep, step, checkcancel, close)
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for progress library tests
class ProgressLibraryTest : public ::testing::Test {
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

// Test that progress library exists
TEST_F(ProgressLibraryTest, ProgressLibraryExists)
{
    lua_getglobal(L, "progress");
    EXPECT_TRUE(lua_istable(L, -1)) << "progress library should be a table";
    lua_pop(L, 1);
}

// Test that progress.new function exists
TEST_F(ProgressLibraryTest, NewFunctionExists)
{
    EXPECT_TRUE(functionExists("progress", "new")) << "progress.new should exist";
}

// Test creating a progress dialog with default title
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

// Test creating a progress dialog with custom title
TEST_F(ProgressLibraryTest, CreateDialogCustomTitle)
{
    const char* code = "return progress.new('Loading Data...')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_isuserdata(L, -1)) << "progress.new('title') should return userdata";
    lua_pop(L, 1);
}

// Test setting status text
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

// Test setting range
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

// Test setting position
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

// Test setstep and step functions
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

// Test default step value (should be 1)
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

// Test checkcancel function
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

// Test close function
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

// Test typical usage pattern
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

// Test with step increment
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

// Test garbage collection
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

// Test error handling - invalid range
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

// Test error handling - invalid position
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

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
