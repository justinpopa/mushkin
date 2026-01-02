/**
 * test_array_api_gtest.cpp - Array API Tests
 *
 * Tests the MUSHclient Array API functions:
 * - ArrayCreate, ArrayDelete, ArrayClear
 * - ArraySet, ArrayGet, ArrayDeleteKey
 * - ArrayExists, ArrayKeyExists
 * - ArrayCount, ArraySize
 * - ArrayGetFirstKey, ArrayGetLastKey
 * - ArrayListAll, ArrayListKeys, ArrayListValues
 * - ArrayExport, ArrayExportKeys, ArrayImport
 */

#include "test_qt_static.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Error code constants (from lua_common.h)
#define eOK 0
#define eArrayAlreadyExists 30055
#define eArrayDoesNotExist 30056
#define eArrayNotEvenNumberOfValues 30057
#define eImportedWithDuplicates 30058
#define eBadDelimiter 30059
#define eSetReplacingExistingValue 30060
#define eKeyDoesNotExist 30061

// Test fixture for Array API tests
class ArrayAPITest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        L = doc->m_ScriptEngine->L;
        ASSERT_NE(L, nullptr) << "Lua state should be available";
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to run Lua code and check for success
    void runLua(const char* code, const char* msg = nullptr)
    {
        int result = luaL_dostring(L, code);
        if (result != 0) {
            const char* err = lua_tostring(L, -1);
            lua_pop(L, 1);
            FAIL() << (msg ? msg : "Lua error") << ": " << (err ? err : "unknown error");
        }
    }

    // Helper to get integer result from Lua global
    int getInt(const char* name)
    {
        lua_getglobal(L, name);
        int val = lua_tointeger(L, -1);
        lua_pop(L, 1);
        return val;
    }

    // Helper to get boolean result from Lua global
    bool getBool(const char* name)
    {
        lua_getglobal(L, name);
        bool val = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return val;
    }

    // Helper to get string result from Lua global
    std::string getString(const char* name)
    {
        lua_getglobal(L, name);
        const char* s = lua_tostring(L, -1);
        std::string val = s ? s : "";
        lua_pop(L, 1);
        return val;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test ArrayCreate
TEST_F(ArrayAPITest, ArrayCreate)
{
    runLua("result = ArrayCreate('test1')");
    EXPECT_EQ(getInt("result"), eOK) << "ArrayCreate should return eOK";

    runLua("result = ArrayCreate('test1')");
    EXPECT_EQ(getInt("result"), eArrayAlreadyExists)
        << "ArrayCreate should return eArrayAlreadyExists for duplicate";
}

// Test ArrayExists
TEST_F(ArrayAPITest, ArrayExists)
{
    runLua("ArrayCreate('myarray')");

    runLua("exists = ArrayExists('myarray')");
    EXPECT_TRUE(getBool("exists")) << "ArrayExists should return true for existing array";

    runLua("exists = ArrayExists('nonexistent')");
    EXPECT_FALSE(getBool("exists")) << "ArrayExists should return false for nonexistent array";
}

// Test ArraySet and ArrayGet
TEST_F(ArrayAPITest, ArraySetAndGet)
{
    runLua("ArrayCreate('data')");

    runLua("result = ArraySet('data', 'key1', 'value1')");
    EXPECT_EQ(getInt("result"), eOK) << "ArraySet should return eOK for new key";

    runLua("result = ArraySet('data', 'key1', 'newvalue')");
    EXPECT_EQ(getInt("result"), eSetReplacingExistingValue)
        << "ArraySet should return eSetReplacingExistingValue for existing key";

    runLua("val = ArrayGet('data', 'key1')");
    EXPECT_EQ(getString("val"), "newvalue") << "ArrayGet should return updated value";

    runLua("val = ArrayGet('data', 'nokey')");
    runLua("isnil = (val == nil)");
    EXPECT_TRUE(getBool("isnil")) << "ArrayGet should return nil for nonexistent key";

    runLua("result = ArraySet('noarray', 'key', 'val')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArraySet should return eArrayDoesNotExist for nonexistent array";
}

// Test ArraySize
TEST_F(ArrayAPITest, ArraySize)
{
    runLua("ArrayCreate('sized')");

    runLua("sz = ArraySize('sized')");
    EXPECT_EQ(getInt("sz"), 0) << "Empty array should have size 0";

    runLua("ArraySet('sized', 'a', '1')");
    runLua("ArraySet('sized', 'b', '2')");
    runLua("ArraySet('sized', 'c', '3')");
    runLua("sz = ArraySize('sized')");
    EXPECT_EQ(getInt("sz"), 3) << "Array should have size 3";

    runLua("sz = ArraySize('nonexistent')");
    EXPECT_EQ(getInt("sz"), 0) << "Nonexistent array should have size 0";
}

// Test ArrayCount
TEST_F(ArrayAPITest, ArrayCount)
{
    runLua("cnt = ArrayCount()");
    EXPECT_EQ(getInt("cnt"), 0) << "Initial array count should be 0";

    runLua("ArrayCreate('arr1')");
    runLua("ArrayCreate('arr2')");
    runLua("ArrayCreate('arr3')");
    runLua("cnt = ArrayCount()");
    EXPECT_EQ(getInt("cnt"), 3) << "Array count should be 3";
}

// Test ArrayKeyExists
TEST_F(ArrayAPITest, ArrayKeyExists)
{
    runLua("ArrayCreate('keys')");
    runLua("ArraySet('keys', 'present', 'yes')");

    runLua("exists = ArrayKeyExists('keys', 'present')");
    EXPECT_TRUE(getBool("exists")) << "ArrayKeyExists should return true for existing key";

    runLua("exists = ArrayKeyExists('keys', 'absent')");
    EXPECT_FALSE(getBool("exists")) << "ArrayKeyExists should return false for missing key";

    runLua("exists = ArrayKeyExists('noarray', 'key')");
    EXPECT_FALSE(getBool("exists")) << "ArrayKeyExists should return false for missing array";
}

// Test ArrayDeleteKey
TEST_F(ArrayAPITest, ArrayDeleteKey)
{
    runLua("ArrayCreate('delkey')");
    runLua("ArraySet('delkey', 'k1', 'v1')");
    runLua("ArraySet('delkey', 'k2', 'v2')");

    runLua("result = ArrayDeleteKey('delkey', 'k1')");
    EXPECT_EQ(getInt("result"), eOK) << "ArrayDeleteKey should return eOK";

    runLua("exists = ArrayKeyExists('delkey', 'k1')");
    EXPECT_FALSE(getBool("exists")) << "Key should be deleted";

    runLua("sz = ArraySize('delkey')");
    EXPECT_EQ(getInt("sz"), 1) << "Size should be 1 after delete";

    runLua("result = ArrayDeleteKey('delkey', 'nokey')");
    EXPECT_EQ(getInt("result"), eKeyDoesNotExist)
        << "ArrayDeleteKey should return eKeyDoesNotExist";

    runLua("result = ArrayDeleteKey('noarray', 'key')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArrayDeleteKey should return eArrayDoesNotExist";
}

// Test ArrayClear
TEST_F(ArrayAPITest, ArrayClear)
{
    runLua("ArrayCreate('clearme')");
    runLua("ArraySet('clearme', 'a', '1')");
    runLua("ArraySet('clearme', 'b', '2')");

    runLua("result = ArrayClear('clearme')");
    EXPECT_EQ(getInt("result"), eOK) << "ArrayClear should return eOK";

    runLua("sz = ArraySize('clearme')");
    EXPECT_EQ(getInt("sz"), 0) << "Array should be empty after clear";

    runLua("exists = ArrayExists('clearme')");
    EXPECT_TRUE(getBool("exists")) << "Array should still exist after clear";

    runLua("result = ArrayClear('noarray')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArrayClear should return eArrayDoesNotExist";
}

// Test ArrayDelete
TEST_F(ArrayAPITest, ArrayDelete)
{
    runLua("ArrayCreate('deleteme')");

    runLua("result = ArrayDelete('deleteme')");
    EXPECT_EQ(getInt("result"), eOK) << "ArrayDelete should return eOK";

    runLua("exists = ArrayExists('deleteme')");
    EXPECT_FALSE(getBool("exists")) << "Array should not exist after delete";

    runLua("result = ArrayDelete('deleteme')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArrayDelete should return eArrayDoesNotExist";
}

// Test ArrayGetFirstKey and ArrayGetLastKey
TEST_F(ArrayAPITest, ArrayGetFirstLastKey)
{
    runLua("ArrayCreate('ordered')");
    runLua("ArraySet('ordered', 'banana', '1')");
    runLua("ArraySet('ordered', 'apple', '2')");
    runLua("ArraySet('ordered', 'cherry', '3')");

    runLua("first = ArrayGetFirstKey('ordered')");
    EXPECT_EQ(getString("first"), "apple") << "First key should be 'apple' (alphabetically)";

    runLua("last = ArrayGetLastKey('ordered')");
    EXPECT_EQ(getString("last"), "cherry") << "Last key should be 'cherry' (alphabetically)";

    // Test empty array
    runLua("ArrayCreate('empty')");
    runLua("first = ArrayGetFirstKey('empty')");
    runLua("isnil = (first == nil)");
    EXPECT_TRUE(getBool("isnil")) << "First key of empty array should be nil";
}

// Test ArrayListAll
TEST_F(ArrayAPITest, ArrayListAll)
{
    // Clear any existing arrays by creating fresh doc
    runLua("ArrayCreate('list1')");
    runLua("ArrayCreate('list2')");
    runLua("ArrayCreate('list3')");

    runLua(R"(
        arrays = ArrayListAll()
        count = #arrays
    )");
    EXPECT_EQ(getInt("count"), 3) << "ArrayListAll should return 3 arrays";
}

// Test ArrayListKeys
TEST_F(ArrayAPITest, ArrayListKeys)
{
    runLua("ArrayCreate('keylist')");
    runLua("ArraySet('keylist', 'x', '1')");
    runLua("ArraySet('keylist', 'y', '2')");
    runLua("ArraySet('keylist', 'z', '3')");

    runLua(R"(
        keys = ArrayListKeys('keylist')
        keycount = #keys
    )");
    EXPECT_EQ(getInt("keycount"), 3) << "ArrayListKeys should return 3 keys";

    // Empty result for nonexistent array
    runLua("keys = ArrayListKeys('noarray')");
    runLua("keycount = #keys");
    EXPECT_EQ(getInt("keycount"), 0) << "ArrayListKeys should return empty for nonexistent array";
}

// Test ArrayListValues
TEST_F(ArrayAPITest, ArrayListValues)
{
    runLua("ArrayCreate('vallist')");
    runLua("ArraySet('vallist', 'a', 'one')");
    runLua("ArraySet('vallist', 'b', 'two')");

    runLua(R"(
        vals = ArrayListValues('vallist')
        valcount = #vals
    )");
    EXPECT_EQ(getInt("valcount"), 2) << "ArrayListValues should return 2 values";
}

// Test ArrayExport
TEST_F(ArrayAPITest, ArrayExport)
{
    runLua("ArrayCreate('export')");
    runLua("ArraySet('export', 'hello', 'world')");
    runLua("ArraySet('export', 'foo', 'bar')");

    runLua("exported = ArrayExport('export', ',')");
    runLua("isstring = (type(exported) == 'string')");
    EXPECT_TRUE(getBool("isstring")) << "ArrayExport should return a string";

    // Test error conditions
    runLua("result = ArrayExport('noarray', ',')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArrayExport should return eArrayDoesNotExist";

    runLua("result = ArrayExport('export', '\\\\')"); // Backslash
    EXPECT_EQ(getInt("result"), eBadDelimiter)
        << "ArrayExport should return eBadDelimiter for backslash";

    runLua("result = ArrayExport('export', 'ab')"); // Multi-char
    EXPECT_EQ(getInt("result"), eBadDelimiter)
        << "ArrayExport should return eBadDelimiter for multi-char";
}

// Test ArrayExportKeys
TEST_F(ArrayAPITest, ArrayExportKeys)
{
    runLua("ArrayCreate('expkeys')");
    runLua("ArraySet('expkeys', 'a', '1')");
    runLua("ArraySet('expkeys', 'b', '2')");

    runLua("keys = ArrayExportKeys('expkeys', '|')");
    runLua("isstring = (type(keys) == 'string')");
    EXPECT_TRUE(getBool("isstring")) << "ArrayExportKeys should return a string";

    // Should contain both keys
    runLua("has_a = string.find(keys, 'a') ~= nil");
    runLua("has_b = string.find(keys, 'b') ~= nil");
    EXPECT_TRUE(getBool("has_a")) << "Exported keys should contain 'a'";
    EXPECT_TRUE(getBool("has_b")) << "Exported keys should contain 'b'";
}

// Test ArrayImport
TEST_F(ArrayAPITest, ArrayImport)
{
    runLua("ArrayCreate('import')");

    // Import key-value pairs
    runLua("result = ArrayImport('import', 'k1,v1,k2,v2', ',')");
    EXPECT_EQ(getInt("result"), eOK) << "ArrayImport should return eOK";

    runLua("val = ArrayGet('import', 'k1')");
    EXPECT_EQ(getString("val"), "v1") << "Imported value should match";

    runLua("val = ArrayGet('import', 'k2')");
    EXPECT_EQ(getString("val"), "v2") << "Imported value should match";

    // Test odd number of values
    runLua("result = ArrayImport('import', 'a,b,c', ',')");
    EXPECT_EQ(getInt("result"), eArrayNotEvenNumberOfValues)
        << "ArrayImport should return eArrayNotEvenNumberOfValues";

    // Test nonexistent array
    runLua("result = ArrayImport('noarray', 'a,b', ',')");
    EXPECT_EQ(getInt("result"), eArrayDoesNotExist)
        << "ArrayImport should return eArrayDoesNotExist";

    // Test duplicate detection
    runLua("result = ArrayImport('import', 'k1,newval', ',')");
    EXPECT_EQ(getInt("result"), eImportedWithDuplicates)
        << "ArrayImport should return eImportedWithDuplicates when overwriting";
}

// Test export/import roundtrip
TEST_F(ArrayAPITest, ExportImportRoundtrip)
{
    runLua("ArrayCreate('source')");
    runLua("ArraySet('source', 'name', 'John Doe')");
    runLua("ArraySet('source', 'city', 'New York')");
    runLua("ArraySet('source', 'count', '42')");

    runLua("exported = ArrayExport('source', '|')");
    runLua("ArrayCreate('dest')");
    runLua("ArrayImport('dest', exported, '|')");

    runLua("v1 = ArrayGet('dest', 'name')");
    runLua("v2 = ArrayGet('dest', 'city')");
    runLua("v3 = ArrayGet('dest', 'count')");

    EXPECT_EQ(getString("v1"), "John Doe") << "Roundtrip should preserve name";
    EXPECT_EQ(getString("v2"), "New York") << "Roundtrip should preserve city";
    EXPECT_EQ(getString("v3"), "42") << "Roundtrip should preserve count";
}

// Main entry point
int main(int argc, char** argv)
{
    // Qt requires QCoreApplication even for non-GUI tests
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
