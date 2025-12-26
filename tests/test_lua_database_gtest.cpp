/**
 * test_lua_database_gtest.cpp - Tests for Lua Database API
 *
 * Tests the 11 database functions exposed to Lua:
 * - DatabaseOpen, DatabaseClose, DatabasePrepare, DatabaseStep, DatabaseFinalize
 * - DatabaseExec, DatabaseColumns, DatabaseColumnType, DatabaseReset
 * - DatabaseChanges, DatabaseTotalChanges
 *
 * These functions provide SQLite database access for plugins to store persistent data.
 */

#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <sqlite3.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

// Test fixture for Lua database API tests
class LuaDatabaseTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create world document (it automatically creates ScriptEngine)
        world = new WorldDocument();

        // Get Lua state from world's ScriptEngine
        L = world->m_ScriptEngine->L;
        ASSERT_NE(L, nullptr) << "Lua state should not be null";
    }

    void TearDown() override
    {
        // Close any open databases
        // The databases will be automatically closed when world is deleted
        // since they're stored in m_DatabaseMap with unique_ptr

        delete world;

        // Clean up test database files
        if (tempDir.isValid()) {
            // Files will be automatically deleted with QTemporaryDir
        }
    }

    // Helper to call a world.DatabaseXxx function
    // Pushes the function onto the stack, ready for arguments
    void pushWorldFunction(const char* funcName)
    {
        lua_getglobal(L, "world");
        lua_getfield(L, -1, funcName);
        lua_remove(L, -2); // Remove world table, leave function
    }

    // Helper to get integer result from Lua stack
    int getIntResult()
    {
        int result = lua_tointeger(L, -1);
        lua_pop(L, 1);
        return result;
    }

    // Helper to check if result is OK (SQLITE_OK = 0)
    void expectOK()
    {
        int result = getIntResult();
        EXPECT_EQ(result, SQLITE_OK) << "Expected SQLITE_OK (0), got " << result;
    }

    // Helper to check if result is a specific error code
    void expectError(int expected)
    {
        int result = getIntResult();
        EXPECT_EQ(result, expected) << "Expected error code " << expected << ", got " << result;
    }

    WorldDocument* world = nullptr;
    lua_State* L = nullptr;
    QTemporaryDir tempDir;
};

// Test 1: DatabaseOpen opens an in-memory database
TEST_F(LuaDatabaseTest, DatabaseOpenInMemory)
{
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "test_db");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);

    expectOK();

    // Verify database was added to the map
    EXPECT_EQ(world->m_DatabaseMap.size(), 1);
    EXPECT_TRUE(world->m_DatabaseMap.find("test_db") != world->m_DatabaseMap.end());
}

// Test 2: DatabaseOpen opens a file database
TEST_F(LuaDatabaseTest, DatabaseOpenFile)
{
    ASSERT_TRUE(tempDir.isValid());
    QString dbPath = tempDir.path() + "/test.db";

    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "file_db");
    lua_pushstring(L, dbPath.toUtf8().constData());
    lua_pcall(L, 2, 1, 0);

    expectOK();

    // Verify database file was created
    EXPECT_TRUE(QFile::exists(dbPath));
}

// Test 3: DatabaseOpen with same name and file returns OK
TEST_F(LuaDatabaseTest, DatabaseOpenDuplicateSameFileReturnsOK)
{
    // Open first time
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "dup_db");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Open again with same name and file
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "dup_db");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();
}

// Test 4: DatabaseOpen with same name but different file returns error
TEST_F(LuaDatabaseTest, DatabaseOpenDuplicateDifferentFileReturnsError)
{
    // Open first time
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "dup_db");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Try to open with same name but different file
    ASSERT_TRUE(tempDir.isValid());
    QString dbPath = tempDir.path() + "/other.db";

    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "dup_db");
    lua_pushstring(L, dbPath.toUtf8().constData());
    lua_pcall(L, 2, 1, 0);

    expectError(DATABASE_ERROR_DATABASE_ALREADY_EXISTS);
}

// Test 5: DatabaseClose closes a database
TEST_F(LuaDatabaseTest, DatabaseCloseClosesDatabase)
{
    // Open database
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "close_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    EXPECT_EQ(world->m_DatabaseMap.size(), 1);

    // Close database
    pushWorldFunction("DatabaseClose");
    lua_pushstring(L, "close_test");
    lua_pcall(L, 1, 1, 0);
    expectOK();

    // Verify database was removed from map
    EXPECT_EQ(world->m_DatabaseMap.size(), 0);
}

// Test 6: DatabaseClose on non-existent database returns error
TEST_F(LuaDatabaseTest, DatabaseCloseNonExistentReturnsError)
{
    pushWorldFunction("DatabaseClose");
    lua_pushstring(L, "nonexistent");
    lua_pcall(L, 1, 1, 0);

    expectError(DATABASE_ERROR_ID_NOT_FOUND);
}

// Test 7: DatabaseExec executes SQL statement
TEST_F(LuaDatabaseTest, DatabaseExecExecutesSQL)
{
    // Open database
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "exec_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Execute CREATE TABLE
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "exec_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Execute INSERT
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "exec_test");
    lua_pushstring(L, "INSERT INTO test (name) VALUES ('Alice')");
    lua_pcall(L, 2, 1, 0);
    expectOK();
}

// Test 8: DatabaseExec on non-existent database returns error
TEST_F(LuaDatabaseTest, DatabaseExecNonExistentReturnsError)
{
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "nonexistent");
    lua_pushstring(L, "SELECT 1");
    lua_pcall(L, 2, 1, 0);

    expectError(DATABASE_ERROR_ID_NOT_FOUND);
}

// Test 9: DatabasePrepare prepares a statement
TEST_F(LuaDatabaseTest, DatabasePreparePreparesStatement)
{
    // Open and create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER, name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare SELECT statement
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, "SELECT * FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Verify statement exists
    auto it = world->m_DatabaseMap.find("prep_test");
    ASSERT_TRUE(it != world->m_DatabaseMap.end());
    EXPECT_NE(it->second->pStmt, nullptr);
}

// Test 10: DatabasePrepare with existing statement returns error
TEST_F(LuaDatabaseTest, DatabasePrepareWithExistingStatementReturnsError)
{
    // Open database and prepare first statement
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, "SELECT 1");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Try to prepare another statement (should fail)
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "prep_test");
    lua_pushstring(L, "SELECT 2");
    lua_pcall(L, 2, 1, 0);

    expectError(DATABASE_ERROR_HAVE_PREPARED_STATEMENT);
}

// Test 11: DatabaseColumns returns column count
TEST_F(LuaDatabaseTest, DatabaseColumnsReturnsColumnCount)
{
    // Open and create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "col_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "col_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER, name TEXT, age INTEGER)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare SELECT statement
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "col_test");
    lua_pushstring(L, "SELECT id, name, age FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get column count
    pushWorldFunction("DatabaseColumns");
    lua_pushstring(L, "col_test");
    lua_pcall(L, 1, 1, 0);

    int cols = getIntResult();
    EXPECT_EQ(cols, 3);
}

// Test 12: DatabaseStep steps through results
TEST_F(LuaDatabaseTest, DatabaseStepStepsThroughResults)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "step_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "step_test");
    lua_pushstring(L, "CREATE TABLE test (name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "step_test");
    lua_pushstring(L, "INSERT INTO test VALUES ('Alice')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare and step
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "step_test");
    lua_pushstring(L, "SELECT * FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "step_test");
    lua_pcall(L, 1, 1, 0);

    int result = getIntResult();
    EXPECT_EQ(result, SQLITE_ROW); // Should have a row

    // Step again (should be DONE)
    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "step_test");
    lua_pcall(L, 1, 1, 0);

    result = getIntResult();
    EXPECT_EQ(result, SQLITE_DONE); // No more rows
}

// Test 13: DatabaseFinalize finalizes statement
TEST_F(LuaDatabaseTest, DatabaseFinalizeFinalizesStatement)
{
    // Open and prepare
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "fin_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "fin_test");
    lua_pushstring(L, "SELECT 1");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Verify statement exists
    auto it = world->m_DatabaseMap.find("fin_test");
    ASSERT_TRUE(it != world->m_DatabaseMap.end());
    EXPECT_NE(it->second->pStmt, nullptr);

    // Finalize
    pushWorldFunction("DatabaseFinalize");
    lua_pushstring(L, "fin_test");
    lua_pcall(L, 1, 1, 0);
    expectOK();

    // Verify statement was cleared
    EXPECT_EQ(it->second->pStmt, nullptr);
}

// Test 14: DatabaseReset resets statement
TEST_F(LuaDatabaseTest, DatabaseResetResetsStatement)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "reset_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "reset_test");
    lua_pushstring(L, "CREATE TABLE test (n INTEGER)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "reset_test");
    lua_pushstring(L, "INSERT INTO test VALUES (1)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare and step to end
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "reset_test");
    lua_pushstring(L, "SELECT * FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "reset_test");
    lua_pcall(L, 1, 1, 0);
    getIntResult(); // SQLITE_ROW

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "reset_test");
    lua_pcall(L, 1, 1, 0);
    getIntResult(); // SQLITE_DONE

    // Reset
    pushWorldFunction("DatabaseReset");
    lua_pushstring(L, "reset_test");
    lua_pcall(L, 1, 1, 0);
    expectOK();

    // Step again - should get a row again
    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "reset_test");
    lua_pcall(L, 1, 1, 0);

    int result = getIntResult();
    EXPECT_EQ(result, SQLITE_ROW); // Should have a row again
}

// Test 15: DatabaseChanges returns row count
TEST_F(LuaDatabaseTest, DatabaseChangesReturnsRowCount)
{
    // Open and create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "changes_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "changes_test");
    lua_pushstring(L, "CREATE TABLE test (n INTEGER)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Insert 3 rows
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "changes_test");
    lua_pushstring(L, "INSERT INTO test VALUES (1), (2), (3)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Check changes
    pushWorldFunction("DatabaseChanges");
    lua_pushstring(L, "changes_test");
    lua_pcall(L, 1, 1, 0);

    int changes = getIntResult();
    EXPECT_EQ(changes, 3);
}

// Test 16: DatabaseTotalChanges returns total row count
TEST_F(LuaDatabaseTest, DatabaseTotalChangesReturnsTotalRowCount)
{
    // Open and create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "total_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "total_test");
    lua_pushstring(L, "CREATE TABLE test (n INTEGER)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Insert multiple times
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "total_test");
    lua_pushstring(L, "INSERT INTO test VALUES (1)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "total_test");
    lua_pushstring(L, "INSERT INTO test VALUES (2)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Check total changes (should be at least 2)
    pushWorldFunction("DatabaseTotalChanges");
    lua_pushstring(L, "total_test");
    lua_pcall(L, 1, 1, 0);

    int total = getIntResult();
    EXPECT_GE(total, 2);
}

// Test 17: DatabaseColumnType returns correct type
TEST_F(LuaDatabaseTest, DatabaseColumnTypeReturnsCorrectType)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "type_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "type_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER, name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "type_test");
    lua_pushstring(L, "INSERT INTO test VALUES (42, 'Alice')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare and step
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "type_test");
    lua_pushstring(L, "SELECT id, name FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "type_test");
    lua_pcall(L, 1, 1, 0);
    expectError(SQLITE_ROW);

    // Get column 1 type (id - INTEGER)
    pushWorldFunction("DatabaseColumnType");
    lua_pushstring(L, "type_test");
    lua_pushinteger(L, 1); // 1-based column index
    lua_pcall(L, 2, 1, 0);

    int type1 = getIntResult();
    EXPECT_EQ(type1, SQLITE_INTEGER);

    // Get column 2 type (name - TEXT)
    pushWorldFunction("DatabaseColumnType");
    lua_pushstring(L, "type_test");
    lua_pushinteger(L, 2); // 1-based column index
    lua_pcall(L, 2, 1, 0);

    int type2 = getIntResult();
    EXPECT_EQ(type2, SQLITE_TEXT);
}

// Test 18: DatabaseColumnName returns column names
TEST_F(LuaDatabaseTest, DatabaseColumnNameReturnsColumnName)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "colname_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colname_test");
    lua_pushstring(L, "CREATE TABLE users (user_id INTEGER, user_name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare query
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "colname_test");
    lua_pushstring(L, "SELECT user_id, user_name FROM users");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get column 1 name
    pushWorldFunction("DatabaseColumnName");
    lua_pushstring(L, "colname_test");
    lua_pushinteger(L, 1);
    lua_pcall(L, 2, 1, 0);

    const char* name1 = lua_tostring(L, -1);
    EXPECT_STREQ(name1, "user_id");
    lua_pop(L, 1);

    // Get column 2 name
    pushWorldFunction("DatabaseColumnName");
    lua_pushstring(L, "colname_test");
    lua_pushinteger(L, 2);
    lua_pcall(L, 2, 1, 0);

    const char* name2 = lua_tostring(L, -1);
    EXPECT_STREQ(name2, "user_name");
    lua_pop(L, 1);
}

// Test 19: DatabaseColumnNames returns all column names
TEST_F(LuaDatabaseTest, DatabaseColumnNamesReturnsAllNames)
{
    // Open, create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "colnames_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colnames_test");
    lua_pushstring(L, "CREATE TABLE items (id INTEGER, name TEXT, price REAL)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare query
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "colnames_test");
    lua_pushstring(L, "SELECT id, name, price FROM items");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get all column names
    pushWorldFunction("DatabaseColumnNames");
    lua_pushstring(L, "colnames_test");
    lua_pcall(L, 1, 1, 0);

    ASSERT_TRUE(lua_istable(L, -1)) << "DatabaseColumnNames should return a table";

    // Check table length (lua_objlen for LuaJIT/Lua 5.1)
    int len = static_cast<int>(lua_objlen(L, -1));
    EXPECT_EQ(len, 3);

    // Check first column name
    lua_rawgeti(L, -1, 1);
    EXPECT_STREQ(lua_tostring(L, -1), "id");
    lua_pop(L, 1);

    // Check second column name
    lua_rawgeti(L, -1, 2);
    EXPECT_STREQ(lua_tostring(L, -1), "name");
    lua_pop(L, 1);

    // Check third column name
    lua_rawgeti(L, -1, 3);
    EXPECT_STREQ(lua_tostring(L, -1), "price");
    lua_pop(L, 2); // Pop name and table
}

// Test 20: DatabaseColumnText returns text value
TEST_F(LuaDatabaseTest, DatabaseColumnTextReturnsText)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "coltext_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "coltext_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER, msg TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "coltext_test");
    lua_pushstring(L, "INSERT INTO test VALUES (1, 'Hello World')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare, step
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "coltext_test");
    lua_pushstring(L, "SELECT id, msg FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "coltext_test");
    lua_pcall(L, 1, 1, 0);
    expectError(SQLITE_ROW);

    // Get text value of column 2
    pushWorldFunction("DatabaseColumnText");
    lua_pushstring(L, "coltext_test");
    lua_pushinteger(L, 2);
    lua_pcall(L, 2, 1, 0);

    const char* text = lua_tostring(L, -1);
    EXPECT_STREQ(text, "Hello World");
    lua_pop(L, 1);
}

// Test 21: DatabaseColumnValue returns typed values
TEST_F(LuaDatabaseTest, DatabaseColumnValueReturnsTypedValue)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "colval_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colval_test");
    lua_pushstring(L, "CREATE TABLE test (i INTEGER, f REAL, t TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colval_test");
    lua_pushstring(L, "INSERT INTO test VALUES (42, 3.14, 'text')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare, step
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "colval_test");
    lua_pushstring(L, "SELECT i, f, t FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "colval_test");
    lua_pcall(L, 1, 1, 0);
    expectError(SQLITE_ROW);

    // Get integer column
    pushWorldFunction("DatabaseColumnValue");
    lua_pushstring(L, "colval_test");
    lua_pushinteger(L, 1);
    lua_pcall(L, 2, 1, 0);

    EXPECT_TRUE(lua_isnumber(L, -1));
    EXPECT_EQ(lua_tointeger(L, -1), 42);
    lua_pop(L, 1);

    // Get float column
    pushWorldFunction("DatabaseColumnValue");
    lua_pushstring(L, "colval_test");
    lua_pushinteger(L, 2);
    lua_pcall(L, 2, 1, 0);

    EXPECT_TRUE(lua_isnumber(L, -1));
    EXPECT_DOUBLE_EQ(lua_tonumber(L, -1), 3.14);
    lua_pop(L, 1);

    // Get text column
    pushWorldFunction("DatabaseColumnValue");
    lua_pushstring(L, "colval_test");
    lua_pushinteger(L, 3);
    lua_pcall(L, 2, 1, 0);

    EXPECT_TRUE(lua_isstring(L, -1));
    EXPECT_STREQ(lua_tostring(L, -1), "text");
    lua_pop(L, 1);
}

// Test 22: DatabaseColumnValues returns all values
TEST_F(LuaDatabaseTest, DatabaseColumnValuesReturnsAllValues)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "colvals_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colvals_test");
    lua_pushstring(L, "CREATE TABLE test (a INTEGER, b TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "colvals_test");
    lua_pushstring(L, "INSERT INTO test VALUES (100, 'hundred')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Prepare, step
    pushWorldFunction("DatabasePrepare");
    lua_pushstring(L, "colvals_test");
    lua_pushstring(L, "SELECT a, b FROM test");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseStep");
    lua_pushstring(L, "colvals_test");
    lua_pcall(L, 1, 1, 0);
    expectError(SQLITE_ROW);

    // Get all values
    pushWorldFunction("DatabaseColumnValues");
    lua_pushstring(L, "colvals_test");
    lua_pcall(L, 1, 1, 0);

    ASSERT_TRUE(lua_istable(L, -1)) << "DatabaseColumnValues should return a table";

    // Check table length (lua_objlen for LuaJIT/Lua 5.1)
    int len = static_cast<int>(lua_objlen(L, -1));
    EXPECT_EQ(len, 2);

    // Check first value
    lua_rawgeti(L, -1, 1);
    EXPECT_EQ(lua_tointeger(L, -1), 100);
    lua_pop(L, 1);

    // Check second value
    lua_rawgeti(L, -1, 2);
    EXPECT_STREQ(lua_tostring(L, -1), "hundred");
    lua_pop(L, 2);
}

// Test 23: DatabaseError returns error message
TEST_F(LuaDatabaseTest, DatabaseErrorReturnsMessage)
{
    // Open database
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "error_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Execute invalid SQL to trigger an error
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "error_test");
    lua_pushstring(L, "INVALID SQL SYNTAX HERE");
    lua_pcall(L, 2, 1, 0);
    // Ignore result (will be error code)
    lua_pop(L, 1);

    // Get error message
    pushWorldFunction("DatabaseError");
    lua_pushstring(L, "error_test");
    lua_pcall(L, 1, 1, 0);

    ASSERT_TRUE(lua_isstring(L, -1)) << "DatabaseError should return a string";
    const char* errMsg = lua_tostring(L, -1);
    EXPECT_TRUE(strlen(errMsg) > 0) << "Error message should not be empty";
    lua_pop(L, 1);
}

// Test 24: DatabaseLastInsertRowid returns rowid
TEST_F(LuaDatabaseTest, DatabaseLastInsertRowidReturnsRowid)
{
    // Open, create table
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "rowid_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "rowid_test");
    lua_pushstring(L, "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Insert first row
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "rowid_test");
    lua_pushstring(L, "INSERT INTO test (name) VALUES ('first')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get rowid
    pushWorldFunction("DatabaseLastInsertRowid");
    lua_pushstring(L, "rowid_test");
    lua_pcall(L, 1, 1, 0);

    lua_Integer rowid1 = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(rowid1, 1);

    // Insert second row
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "rowid_test");
    lua_pushstring(L, "INSERT INTO test (name) VALUES ('second')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get rowid again
    pushWorldFunction("DatabaseLastInsertRowid");
    lua_pushstring(L, "rowid_test");
    lua_pcall(L, 1, 1, 0);

    lua_Integer rowid2 = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(rowid2, 2);
}

// Test 25: DatabaseList returns list of databases
TEST_F(LuaDatabaseTest, DatabaseListReturnsDatabaseNames)
{
    // Open multiple databases
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "db_alpha");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "db_beta");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Get database list
    pushWorldFunction("DatabaseList");
    lua_pcall(L, 0, 1, 0);

    ASSERT_TRUE(lua_istable(L, -1)) << "DatabaseList should return a table";

    // Check table length (lua_objlen for LuaJIT/Lua 5.1)
    int len = static_cast<int>(lua_objlen(L, -1));
    EXPECT_EQ(len, 2);

    // Verify names are in table (order may vary)
    bool foundAlpha = false, foundBeta = false;
    for (int i = 1; i <= 2; i++) {
        lua_rawgeti(L, -1, i);
        const char* name = lua_tostring(L, -1);
        if (strcmp(name, "db_alpha") == 0)
            foundAlpha = true;
        if (strcmp(name, "db_beta") == 0)
            foundBeta = true;
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    EXPECT_TRUE(foundAlpha) << "db_alpha should be in list";
    EXPECT_TRUE(foundBeta) << "db_beta should be in list";
}

// Test 26: DatabaseGetField convenience function
TEST_F(LuaDatabaseTest, DatabaseGetFieldReturnsFirstValue)
{
    // Open, create table, insert data
    pushWorldFunction("DatabaseOpen");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, ":memory:");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "CREATE TABLE test (n INTEGER)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "INSERT INTO test VALUES (1), (2), (3)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    // Use GetField to count rows
    pushWorldFunction("DatabaseGetField");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "SELECT count(*) FROM test");
    lua_pcall(L, 2, 1, 0);

    lua_Integer count = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(count, 3);

    // Use GetField to get a text value
    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "CREATE TABLE names (name TEXT)");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseExec");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "INSERT INTO names VALUES ('Alice')");
    lua_pcall(L, 2, 1, 0);
    expectOK();

    pushWorldFunction("DatabaseGetField");
    lua_pushstring(L, "getfield_test");
    lua_pushstring(L, "SELECT name FROM names LIMIT 1");
    lua_pcall(L, 2, 1, 0);

    const char* name = lua_tostring(L, -1);
    EXPECT_STREQ(name, "Alice");
    lua_pop(L, 1);
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
