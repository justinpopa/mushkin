/**
 * test_unique_id_api_gtest.cpp - GoogleTest version
 * GetUniqueID and CreateGUID Lua API Test
 *
 * Tests unique ID generation functions:
 * - world.GetUniqueID() - Returns 24-char hex ID
 * - world.CreateGUID() - Returns 36-char UUID with dashes
 * - world.GetUniqueNumber() - Returns sequential number (existing)
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <QRegularExpression>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for unique ID API tests
class UniqueIDAPITest : public ::testing::Test {
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

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test GetUniqueID returns 24-character hex string
TEST_F(UniqueIDAPITest, GetUniqueIDReturns24CharHex)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueID");
    int result = lua_pcall(L, 0, 1, 0);
    if (result != 0) {
        const char* error = lua_tostring(L, -1);
        FAIL() << "GetUniqueID error: " << (error ? error : "unknown");
    }
    ASSERT_EQ(result, 0) << "GetUniqueID should not error";

    const char* id = lua_tostring(L, -1);
    ASSERT_NE(id, nullptr) << "GetUniqueID should return a string";

    QString idStr(id);
    EXPECT_EQ(idStr.length(), 24) << "GetUniqueID should return 24 characters";

    // Verify all characters are hexadecimal
    QRegularExpression hexPattern("^[0-9a-fA-F]{24}$");
    EXPECT_TRUE(hexPattern.match(idStr).hasMatch())
        << "GetUniqueID should return only hex characters";

    lua_pop(L, 2); // pop result and world table
}

// Test GetUniqueID returns different values on each call
TEST_F(UniqueIDAPITest, GetUniqueIDReturnsUnique)
{
    QString id1, id2, id3;

    // Get first ID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueID");
    lua_pcall(L, 0, 1, 0);
    id1 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    // Get second ID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueID");
    lua_pcall(L, 0, 1, 0);
    id2 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    // Get third ID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueID");
    lua_pcall(L, 0, 1, 0);
    id3 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    EXPECT_NE(id1, id2) << "GetUniqueID should return different values";
    EXPECT_NE(id2, id3) << "GetUniqueID should return different values";
    EXPECT_NE(id1, id3) << "GetUniqueID should return different values";
}

// Test CreateGUID returns 36-character UUID with dashes
TEST_F(UniqueIDAPITest, CreateGUIDReturnsStandardFormat)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "CreateGUID");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0) << "CreateGUID should not error";

    const char* guid = lua_tostring(L, -1);
    ASSERT_NE(guid, nullptr) << "CreateGUID should return a string";

    QString guidStr(guid);
    EXPECT_EQ(guidStr.length(), 36) << "CreateGUID should return 36 characters";

    // Verify format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    QRegularExpression guidPattern(
        "^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}$");
    EXPECT_TRUE(guidPattern.match(guidStr).hasMatch())
        << "CreateGUID should match format XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX";

    // Verify dashes are in correct positions
    EXPECT_EQ(guidStr[8], '-') << "Dash should be at position 8";
    EXPECT_EQ(guidStr[13], '-') << "Dash should be at position 13";
    EXPECT_EQ(guidStr[18], '-') << "Dash should be at position 18";
    EXPECT_EQ(guidStr[23], '-') << "Dash should be at position 23";

    lua_pop(L, 2); // pop result and world table
}

// Test CreateGUID returns different values on each call
TEST_F(UniqueIDAPITest, CreateGUIDReturnsUnique)
{
    QString guid1, guid2, guid3;

    // Get first GUID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "CreateGUID");
    lua_pcall(L, 0, 1, 0);
    guid1 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    // Get second GUID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "CreateGUID");
    lua_pcall(L, 0, 1, 0);
    guid2 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    // Get third GUID
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "CreateGUID");
    lua_pcall(L, 0, 1, 0);
    guid3 = QString(lua_tostring(L, -1));
    lua_pop(L, 2);

    EXPECT_NE(guid1, guid2) << "CreateGUID should return different values";
    EXPECT_NE(guid2, guid3) << "CreateGUID should return different values";
    EXPECT_NE(guid1, guid3) << "CreateGUID should return different values";
}

// Test GetUniqueNumber still works (existing function)
TEST_F(UniqueIDAPITest, GetUniqueNumberWorks)
{
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueNumber");
    ASSERT_EQ(lua_pcall(L, 0, 1, 0), 0) << "GetUniqueNumber should not error";

    ASSERT_TRUE(lua_isnumber(L, -1)) << "GetUniqueNumber should return a number";

    double num = lua_tonumber(L, -1);
    EXPECT_GT(num, 0) << "GetUniqueNumber should return positive number";

    lua_pop(L, 2); // pop result and world table
}

// Test GetUniqueNumber returns incrementing values
TEST_F(UniqueIDAPITest, GetUniqueNumberIncrements)
{
    double num1, num2, num3;

    // Get first number
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueNumber");
    lua_pcall(L, 0, 1, 0);
    num1 = lua_tonumber(L, -1);
    lua_pop(L, 2);

    // Get second number
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueNumber");
    lua_pcall(L, 0, 1, 0);
    num2 = lua_tonumber(L, -1);
    lua_pop(L, 2);

    // Get third number
    lua_getglobal(L, "world");
    lua_getfield(L, -1, "GetUniqueNumber");
    lua_pcall(L, 0, 1, 0);
    num3 = lua_tonumber(L, -1);
    lua_pop(L, 2);

    EXPECT_GT(num2, num1) << "GetUniqueNumber should increment";
    EXPECT_GT(num3, num2) << "GetUniqueNumber should increment";
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    // Qt application needed for QUuid and WorldDocument
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
