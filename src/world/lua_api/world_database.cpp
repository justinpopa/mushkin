// world_database.cpp - Lua database API functions
// Implements SQLite database access for plugins to store persistent data
// Based on methods_database.cpp

#include "lua_common.h"
#include <sqlite3.h>

/**
 * world.DatabaseOpen(Name, Filename, Flags)
 *
 * Opens or creates a SQLite database for use by Lua scripts.
 * Each database is identified by a logical name used in subsequent calls.
 *
 * @param Name (string) Logical name for this database connection
 * @param Filename (string) Path to database file, or ":memory:" for in-memory database
 * @param Flags (number) SQLite open flags (optional, default: SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_DATABASE_ALREADY_EXISTS (-6): Name already used for different file
 *   - Other SQLite error codes on failure
 *
 * @example
 * -- Open a database file
 * local rc = DatabaseOpen("mydb", GetPluginInfo(GetPluginID(), 20) .. "data.db")
 * if rc ~= 0 then
 *     Note("Failed to open database: " .. DatabaseError("mydb"))
 * end
 *
 * @example
 * -- Create in-memory database
 * DatabaseOpen("temp", ":memory:")
 *
 * @see DatabaseClose, DatabaseExec, DatabaseError
 */
int L_DatabaseOpen(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* filename = luaL_checkstring(L, 2);
    lua_Integer flags = luaL_optinteger(L, 3, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    QString qName = QString::fromUtf8(name);
    QString qFilename = QString::fromUtf8(filename);

    // Check if database name already exists
    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it != pDoc->m_DatabaseMap.end()) {
        // Database name already in use - check if it's the same disk file
        if (it->second->db_name == qFilename) {
            // Same file, just return OK
            lua_pushnumber(L, SQLITE_OK);
            return 1;
        } else {
            // Different file, error
            lua_pushnumber(L, DATABASE_ERROR_DATABASE_ALREADY_EXISTS);
            return 1;
        }
    }

    // Create new database entry
    auto pDatabase = std::make_unique<LuaDatabase>();
    pDatabase->db_name = qFilename;

    // Open the database
    int rc = sqlite3_open_v2(filename, &pDatabase->db, flags, nullptr);

    if (rc == SQLITE_OK) {
        // Success - add to map
        pDoc->m_DatabaseMap[qName] = std::move(pDatabase);
    }
    // Note: if open failed, pDatabase is automatically deleted via unique_ptr

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseClose(Name)
 *
 * Closes a database connection and releases all resources.
 * Automatically finalizes any outstanding prepared statement.
 *
 * @param Name (string) Logical database name from DatabaseOpen
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_ID_NOT_FOUND (-1): Database name not found
 *   - DATABASE_ERROR_NOT_OPEN (-2): Database not open
 *
 * @example
 * DatabaseClose("mydb")
 *
 * @see DatabaseOpen, DatabaseList
 */
int L_DatabaseClose(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    // Finalize any outstanding statement
    if (it->second->pStmt) {
        sqlite3_finalize(it->second->pStmt);
    }

    // Close the database
    int rc = sqlite3_close(it->second->db);

    // Remove from map (this deletes the LuaDatabase via unique_ptr)
    pDoc->m_DatabaseMap.erase(it);

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabasePrepare(Name, Sql)
 *
 * Prepares (compiles) an SQL statement for execution with parameters.
 * Only one prepared statement is allowed per database at a time.
 * Must be finalized with DatabaseFinalize before preparing another.
 *
 * @param Name (string) Logical database name
 * @param Sql (string) SQL statement to prepare (can include ? placeholders)
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_ID_NOT_FOUND (-1): Database name not found
 *   - DATABASE_ERROR_NOT_OPEN (-2): Database not open
 *   - DATABASE_ERROR_HAVE_PREPARED_STATEMENT (-3): Statement already prepared
 *
 * @example
 * DatabasePrepare("mydb", "SELECT * FROM players WHERE name = ?")
 * -- Bind parameters and execute with DatabaseStep
 *
 * @see DatabaseStep, DatabaseFinalize, DatabaseExec
 */
int L_DatabasePrepare(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* sql = luaL_checkstring(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt != nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_HAVE_PREPARED_STATEMENT);
        return 1;
    }

    // Reset state
    it->second->bValidRow = false;
    it->second->iColumns = 0;

    // Prepare the statement
    const char* pzTail;
    int rc = sqlite3_prepare_v2(it->second->db, sql, -1, &it->second->pStmt, &pzTail);

    // For future validation that columns are in range
    if (rc == SQLITE_OK) {
        it->second->iColumns = sqlite3_column_count(it->second->pStmt);
    }

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseStep(Name)
 *
 * Executes the next step of a prepared statement. For SELECT queries,
 * call repeatedly until SQLITE_DONE to iterate through all rows.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Result code:
 *   - SQLITE_ROW (100): Row is available, use DatabaseColumnValue to read it
 *   - SQLITE_DONE (101): No more rows / statement complete
 *   - DATABASE_ERROR_ID_NOT_FOUND (-1): Database not found
 *   - DATABASE_ERROR_NO_PREPARED_STATEMENT (-4): No statement prepared
 *
 * @example
 * DatabasePrepare("mydb", "SELECT name, level FROM players")
 * while DatabaseStep("mydb") == 100 do  -- SQLITE_ROW
 *     local name = DatabaseColumnValue("mydb", 1)
 *     local level = DatabaseColumnValue("mydb", 2)
 *     Note(name .. " is level " .. level)
 * end
 * DatabaseFinalize("mydb")
 *
 * @see DatabasePrepare, DatabaseColumnValue, DatabaseFinalize
 */
int L_DatabaseStep(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NO_PREPARED_STATEMENT);
        return 1;
    }

    // Execute the next step
    int rc = sqlite3_step(it->second->pStmt);

    // Track whether we have a valid row for column access
    it->second->bValidRow = (rc == SQLITE_ROW);

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseFinalize(Name)
 *
 * Finalizes (discards) a prepared statement and frees its resources.
 * Must be called after using DatabasePrepare before preparing another statement.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_ID_NOT_FOUND (-1): Database not found
 *   - DATABASE_ERROR_NO_PREPARED_STATEMENT (-4): No statement to finalize
 *
 * @example
 * DatabasePrepare("mydb", "SELECT * FROM items")
 * -- ... process rows ...
 * DatabaseFinalize("mydb")  -- Free resources
 *
 * @see DatabasePrepare, DatabaseStep, DatabaseReset
 */
int L_DatabaseFinalize(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NO_PREPARED_STATEMENT);
        return 1;
    }

    // Finalize the statement
    int rc = sqlite3_finalize(it->second->pStmt);

    // Clear statement state
    it->second->pStmt = nullptr;
    it->second->bValidRow = false;
    it->second->iColumns = 0;

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseExec(Name, Sql)
 *
 * Executes an SQL statement directly without preparing it.
 * Ideal for statements that don't return rows (CREATE, INSERT, UPDATE, DELETE).
 * Cannot be used while a prepared statement is active.
 *
 * @param Name (string) Logical database name
 * @param Sql (string) SQL statement to execute
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_ID_NOT_FOUND (-1): Database not found
 *   - DATABASE_ERROR_HAVE_PREPARED_STATEMENT (-3): Must finalize first
 *   - Other SQLite error codes on SQL failure
 *
 * @example
 * -- Create a table
 * DatabaseExec("mydb", [[
 *     CREATE TABLE IF NOT EXISTS players (
 *         id INTEGER PRIMARY KEY,
 *         name TEXT,
 *         level INTEGER
 *     )
 * ]])
 *
 * @example
 * -- Insert a record
 * DatabaseExec("mydb", "INSERT INTO players (name, level) VALUES ('Hero', 10)")
 *
 * @see DatabaseOpen, DatabasePrepare, DatabaseChanges
 */
int L_DatabaseExec(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* sql = luaL_checkstring(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt != nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_HAVE_PREPARED_STATEMENT);
        return 1;
    }

    // Reset state
    it->second->bValidRow = false;
    it->second->iColumns = 0;

    // Execute the SQL
    int rc = sqlite3_exec(it->second->db, sql, nullptr, nullptr, nullptr);

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseColumns(Name)
 *
 * Returns the number of columns in the result set of a prepared statement.
 * Can be called immediately after DatabasePrepare.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Column count, or negative error code
 *
 * @example
 * DatabasePrepare("mydb", "SELECT name, level, gold FROM players")
 * local cols = DatabaseColumns("mydb")  -- Returns 3
 *
 * @see DatabaseColumnName, DatabaseColumnType, DatabaseColumnValue
 */
int L_DatabaseColumns(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NO_PREPARED_STATEMENT);
        return 1;
    }

    // Return column count
    lua_pushnumber(L, sqlite3_column_count(it->second->pStmt));
    return 1;
}

/**
 * world.DatabaseColumnType(Name, Column)
 *
 * Returns the SQLite data type of a column value from the current row.
 * Column numbers are 1-based.
 *
 * Type values:
 * - 1: SQLITE_INTEGER
 * - 2: SQLITE_FLOAT
 * - 3: SQLITE_TEXT
 * - 4: SQLITE_BLOB
 * - 5: SQLITE_NULL
 *
 * @param Name (string) Logical database name
 * @param Column (number) Column number (1-based)
 *
 * @return (number) Column type (1-5), or negative error code
 *
 * @example
 * local colType = DatabaseColumnType("mydb", 1)
 * if colType == 3 then  -- SQLITE_TEXT
 *     Note("Column 1 is text")
 * end
 *
 * @see DatabaseColumnValue, DatabaseColumnName, DatabaseColumns
 */
int L_DatabaseColumnType(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    lua_Integer column = luaL_checkinteger(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NO_PREPARED_STATEMENT);
        return 1;
    }

    if (!it->second->bValidRow) {
        lua_pushnumber(L, DATABASE_ERROR_NO_VALID_ROW);
        return 1;
    }

    if (column < 1 || column > it->second->iColumns) {
        lua_pushnumber(L, DATABASE_ERROR_COLUMN_OUT_OF_RANGE);
        return 1;
    }

    // Return column type (convert 1-based to 0-based index)
    int type = sqlite3_column_type(it->second->pStmt, column - 1);
    lua_pushnumber(L, type);
    return 1;
}

/**
 * world.DatabaseReset(Name)
 *
 * Resets a prepared statement to the beginning, allowing re-execution.
 * Useful for re-executing the same query with different bound parameters.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Error code:
 *   - SQLITE_OK (0): Success
 *   - DATABASE_ERROR_NO_PREPARED_STATEMENT (-4): No statement to reset
 *
 * @example
 * -- Re-run a query after modifying parameters
 * DatabaseReset("mydb")
 * while DatabaseStep("mydb") == 100 do
 *     -- Process rows again
 * end
 *
 * @see DatabasePrepare, DatabaseStep, DatabaseFinalize
 */
int L_DatabaseReset(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    if (it->second->pStmt == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NO_PREPARED_STATEMENT);
        return 1;
    }

    // Reset the statement
    int rc = sqlite3_reset(it->second->pStmt);

    lua_pushnumber(L, rc);
    return 1;
}

/**
 * world.DatabaseChanges(Name)
 *
 * Returns the number of rows modified by the most recent INSERT, UPDATE, or DELETE.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Rows changed, or negative error code
 *
 * @example
 * DatabaseExec("mydb", "UPDATE players SET level = level + 1 WHERE active = 1")
 * local changed = DatabaseChanges("mydb")
 * Note("Updated " .. changed .. " players")
 *
 * @see DatabaseTotalChanges, DatabaseExec, DatabaseLastInsertRowid
 */
int L_DatabaseChanges(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    // Return number of changes
    lua_pushnumber(L, sqlite3_changes(it->second->db));
    return 1;
}

/**
 * world.DatabaseTotalChanges(Name)
 *
 * Returns the total number of rows modified since the database connection was opened.
 *
 * @param Name (string) Logical database name
 *
 * @return (number) Total rows changed, or negative error code
 *
 * @example
 * local total = DatabaseTotalChanges("mydb")
 * Note("Total changes this session: " .. total)
 *
 * @see DatabaseChanges, DatabaseOpen
 */
int L_DatabaseTotalChanges(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnumber(L, DATABASE_ERROR_ID_NOT_FOUND);
        return 1;
    }

    if (it->second->db == nullptr) {
        lua_pushnumber(L, DATABASE_ERROR_NOT_OPEN);
        return 1;
    }

    // Return total number of changes
    lua_pushnumber(L, sqlite3_total_changes(it->second->db));
    return 1;
}

/**
 * world.DatabaseError(Name)
 *
 * Returns the error message from the most recent SQLite operation.
 * Useful for diagnosing failed database operations.
 *
 * @param Name (string) Logical database name
 *
 * @return (string) Error message, or empty string if no error
 *
 * @example
 * local rc = DatabaseExec("mydb", "INVALID SQL")
 * if rc ~= 0 then
 *     Note("SQL Error: " .. DatabaseError("mydb"))
 * end
 *
 * @see DatabaseOpen, DatabaseExec, DatabasePrepare
 */
int L_DatabaseError(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr) {
        lua_pushstring(L, "");
        return 1;
    }

    const char* errMsg = sqlite3_errmsg(it->second->db);
    lua_pushstring(L, errMsg ? errMsg : "");
    return 1;
}

/**
 * world.DatabaseColumnName(Name, Column)
 *
 * Returns the name of a column from the prepared statement.
 * Column numbers are 1-based.
 *
 * @param Name (string) Logical database name
 * @param Column (number) Column number (1-based)
 *
 * @return (string) Column name, or empty string on error
 *
 * @example
 * DatabasePrepare("mydb", "SELECT name, level FROM players")
 * for i = 1, DatabaseColumns("mydb") do
 *     Note("Column " .. i .. ": " .. DatabaseColumnName("mydb", i))
 * end
 *
 * @see DatabaseColumnNames, DatabaseColumns, DatabaseColumnType
 */
int L_DatabaseColumnName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    lua_Integer column = luaL_checkinteger(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr ||
        it->second->pStmt == nullptr || column < 1 || column > it->second->iColumns) {
        lua_pushstring(L, "");
        return 1;
    }

    const char* colName = sqlite3_column_name(it->second->pStmt, column - 1);
    lua_pushstring(L, colName ? colName : "");
    return 1;
}

/**
 * world.DatabaseColumnText(Name, Column)
 *
 * Returns the value of a column as a string from the current row.
 * All value types are coerced to text representation.
 * Column numbers are 1-based.
 *
 * @param Name (string) Logical database name
 * @param Column (number) Column number (1-based)
 *
 * @return (string) Column value as text, or nil on error
 *
 * @example
 * local name = DatabaseColumnText("mydb", 1)
 *
 * @see DatabaseColumnValue, DatabaseColumnName, DatabaseStep
 */
int L_DatabaseColumnText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    lua_Integer column = luaL_checkinteger(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr ||
        it->second->pStmt == nullptr || !it->second->bValidRow || column < 1 ||
        column > it->second->iColumns) {
        lua_pushnil(L);
        return 1;
    }

    const unsigned char* text = sqlite3_column_text(it->second->pStmt, column - 1);
    if (text) {
        lua_pushstring(L, reinterpret_cast<const char*>(text));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

/**
 * Helper: Push a database column value with appropriate Lua type
 */
static void pushDatabaseColumnValue(lua_State* L, sqlite3_stmt* pStmt, int column)
{
    int type = sqlite3_column_type(pStmt, column);
    switch (type) {
        case SQLITE_INTEGER:
            lua_pushnumber(L, static_cast<lua_Number>(sqlite3_column_int64(pStmt, column)));
            break;
        case SQLITE_FLOAT:
            lua_pushnumber(L, sqlite3_column_double(pStmt, column));
            break;
        case SQLITE_TEXT: {
            const unsigned char* text = sqlite3_column_text(pStmt, column);
            if (text) {
                lua_pushstring(L, reinterpret_cast<const char*>(text));
            } else {
                lua_pushnil(L);
            }
            break;
        }
        case SQLITE_BLOB: {
            const void* blob = sqlite3_column_blob(pStmt, column);
            int bytes = sqlite3_column_bytes(pStmt, column);
            if (blob && bytes > 0) {
                lua_pushlstring(L, static_cast<const char*>(blob), bytes);
            } else {
                lua_pushnil(L);
            }
            break;
        }
        case SQLITE_NULL:
        default:
            lua_pushnil(L);
            break;
    }
}

/**
 * world.DatabaseColumnValue(Name, Column)
 *
 * Returns the value of a column with appropriate Lua type from the current row.
 * Integer/float columns return numbers, text/blob return strings, NULL returns nil.
 * Column numbers are 1-based.
 *
 * @param Name (string) Logical database name
 * @param Column (number) Column number (1-based)
 *
 * @return (varies) Column value with native Lua type, or nil on error/NULL
 *
 * @example
 * local name = DatabaseColumnValue("mydb", 1)   -- string
 * local level = DatabaseColumnValue("mydb", 2)  -- number
 *
 * @see DatabaseColumnText, DatabaseColumnValues, DatabaseStep
 */
int L_DatabaseColumnValue(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    lua_Integer column = luaL_checkinteger(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr ||
        it->second->pStmt == nullptr || !it->second->bValidRow || column < 1 ||
        column > it->second->iColumns) {
        lua_pushnil(L);
        return 1;
    }

    pushDatabaseColumnValue(L, it->second->pStmt, column - 1);
    return 1;
}

/**
 * world.DatabaseColumnNames(Name)
 *
 * Returns a table containing all column names from the prepared statement.
 *
 * @param Name (string) Logical database name
 *
 * @return (table) Array of column name strings (1-indexed), or empty table on error
 *
 * @example
 * local names = DatabaseColumnNames("mydb")
 * for i, name in ipairs(names) do
 *     Note("Column " .. i .. ": " .. name)
 * end
 *
 * @see DatabaseColumnName, DatabaseColumns, DatabaseColumnValues
 */
int L_DatabaseColumnNames(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    lua_newtable(L);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr ||
        it->second->pStmt == nullptr) {
        return 1; // Return empty table
    }

    for (int i = 0; i < it->second->iColumns; i++) {
        const char* colName = sqlite3_column_name(it->second->pStmt, i);
        lua_pushstring(L, colName ? colName : "");
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/**
 * world.DatabaseColumnValues(Name)
 *
 * Returns a table containing all column values from the current row.
 * Values are returned with appropriate Lua types.
 *
 * @param Name (string) Logical database name
 *
 * @return (table) Array of column values (1-indexed), or empty table on error
 *
 * @example
 * while DatabaseStep("mydb") == 100 do
 *     local row = DatabaseColumnValues("mydb")
 *     Note("Row: " .. table.concat(row, ", "))
 * end
 *
 * @see DatabaseColumnValue, DatabaseColumnNames, DatabaseStep
 */
int L_DatabaseColumnValues(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    lua_newtable(L);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr ||
        it->second->pStmt == nullptr || !it->second->bValidRow) {
        return 1; // Return empty table
    }

    for (int i = 0; i < it->second->iColumns; i++) {
        pushDatabaseColumnValue(L, it->second->pStmt, i);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/**
 * world.DatabaseGetField(Name, Sql)
 *
 * Convenience function that executes SQL, returns the first column of the first row.
 * Automatically prepares, steps once, gets the value, and finalizes.
 * Ideal for simple queries like "SELECT count(*) FROM..." or "SELECT MAX(id) FROM...".
 *
 * @param Name (string) Logical database name
 * @param Sql (string) SQL query to execute
 *
 * @return (varies) First column value from first row, or nil if no results/error
 *
 * @example
 * local count = DatabaseGetField("mydb", "SELECT COUNT(*) FROM players")
 * Note("Total players: " .. (count or 0))
 *
 * @example
 * local name = DatabaseGetField("mydb", "SELECT name FROM players WHERE id = 1")
 *
 * @see DatabaseExec, DatabasePrepare, DatabaseStep
 */
int L_DatabaseGetField(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* sql = luaL_checkstring(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    // Can't use if there's already a prepared statement
    if (it->second->pStmt != nullptr) {
        lua_pushnil(L);
        return 1;
    }

    // Prepare the statement
    sqlite3_stmt* pStmt = nullptr;
    const char* pzTail;
    int rc = sqlite3_prepare_v2(it->second->db, sql, -1, &pStmt, &pzTail);

    if (rc != SQLITE_OK || pStmt == nullptr) {
        if (pStmt) {
            sqlite3_finalize(pStmt);
        }
        lua_pushnil(L);
        return 1;
    }

    // Step to get first row
    rc = sqlite3_step(pStmt);

    if (rc == SQLITE_ROW && sqlite3_column_count(pStmt) > 0) {
        pushDatabaseColumnValue(L, pStmt, 0);
    } else {
        lua_pushnil(L);
    }

    // Clean up
    sqlite3_finalize(pStmt);

    return 1;
}

/**
 * world.DatabaseInfo(Name, InfoType)
 *
 * Returns information about a database connection.
 *
 * Info types:
 * - 1: Disk filename (string)
 * - 2: Has prepared statement (boolean)
 * - 3: Has valid row after DatabaseStep (boolean)
 * - 4: Number of columns in prepared statement (number)
 *
 * @param Name (string) Logical database name
 * @param InfoType (number) Type of information (1-4)
 *
 * @return (varies) Requested info, or nil if database not found
 *
 * @example
 * local filename = DatabaseInfo("mydb", 1)
 * Note("Database file: " .. filename)
 *
 * @see DatabaseOpen, DatabaseList, DatabaseColumns
 */
int L_DatabaseInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    lua_Integer infoType = luaL_checkinteger(L, 2);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end()) {
        lua_pushnil(L);
        return 1;
    }

    LuaDatabase* pDatabase = it->second.get();

    switch (infoType) {
        case 1: // disk filename
            lua_pushstring(L, pDatabase->db_name.toUtf8().constData());
            break;
        case 2: // has prepared statement
            lua_pushboolean(L, pDatabase->pStmt != nullptr);
            break;
        case 3: // has valid row
            lua_pushboolean(L, pDatabase->bValidRow);
            break;
        case 4: // number of columns
            lua_pushnumber(L, pDatabase->iColumns);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.DatabaseLastInsertRowid(Name)
 *
 * Returns the rowid of the last successful INSERT operation.
 * Returned as a string to preserve precision for large rowids.
 *
 * @param Name (string) Logical database name
 *
 * @return (string) Rowid as string, or empty string on error
 *
 * @example
 * DatabaseExec("mydb", "INSERT INTO players (name) VALUES ('NewPlayer')")
 * local id = DatabaseLastInsertRowid("mydb")
 * Note("Inserted player with ID: " .. id)
 *
 * @see DatabaseChanges, DatabaseExec
 */
int L_DatabaseLastInsertRowid(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QString qName = QString::fromUtf8(name);

    auto it = pDoc->m_DatabaseMap.find(qName);
    if (it == pDoc->m_DatabaseMap.end() || it->second->db == nullptr) {
        lua_pushstring(L, "");
        return 1;
    }

    sqlite3_int64 rowid = sqlite3_last_insert_rowid(it->second->db);
    QString rowidStr = QString::number(rowid);
    lua_pushstring(L, rowidStr.toUtf8().constData());
    return 1;
}

/**
 * world.DatabaseList()
 *
 * Returns a table of all currently open database connection names.
 *
 * @return (table) Array of database names (1-indexed)
 *
 * @example
 * local dbs = DatabaseList()
 * Note("Open databases: " .. #dbs)
 * for i, name in ipairs(dbs) do
 *     Note("  " .. name)
 * end
 *
 * @see DatabaseOpen, DatabaseClose, DatabaseInfo
 */
int L_DatabaseList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    int index = 1;
    for (const auto& pair : pDoc->m_DatabaseMap) {
        lua_pushstring(L, pair.first.toUtf8().constData());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}
