// world_database.cpp - Lua database API functions
// Implements SQLite database access for plugins to store persistent data
// Based on methods_database.cpp

#include "lua_common.h"
#include <sqlite3.h>

/**
 * world.DatabaseOpen(Name, Filename, Flags)
 *
 * Opens or creates a SQLite database for use by Lua scripts.
 *
 * @param Name - Logical name for the database (used in all other Database* functions)
 * @param Filename - Path to database file, or ":memory:" for in-memory database
 * @param Flags - SQLite open flags (default: SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
 * @return SQLite error code (SQLITE_OK = 0 on success), or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Closes a database and removes it from the database map.
 * Automatically finalizes any outstanding prepared statement.
 *
 * @param Name - Database name
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Prepares (compiles) an SQL statement for execution.
 * Only one prepared statement is allowed per database at a time.
 *
 * @param Name - Database name
 * @param Sql - SQL statement to prepare
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Executes the next step of a prepared statement.
 * Returns SQLITE_ROW (100) if a row is ready, SQLITE_DONE (101) if finished.
 *
 * @param Name - Database name
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Finalizes (discards) a prepared statement.
 * Must be called to free resources after using DatabasePrepare.
 *
 * @param Name - Database name
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Useful for simple statements that don't return rows (CREATE, INSERT, UPDATE, DELETE).
 * Cannot be used if a prepared statement is already active.
 *
 * @param Name - Database name
 * @param Sql - SQL statement to execute
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Returns the number of columns in the prepared statement.
 * Can be called immediately after DatabasePrepare, doesn't require DatabaseStep.
 *
 * @param Name - Database name
 * @return Column count (positive), or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Returns the type of a column from the last DatabaseStep() result.
 * Column numbers are 1-based (first column is 1).
 *
 * @param Name - Database name
 * @param Column - Column number (1-based)
 * @return Column type (SQLITE_INTEGER=1, SQLITE_FLOAT=2, SQLITE_TEXT=3, SQLITE_BLOB=4,
 * SQLITE_NULL=5), or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Resets a prepared statement to the beginning, allowing it to be executed again.
 * Useful for re-executing the same query with different parameters.
 *
 * @param Name - Database name
 * @return SQLite error code, or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Returns the number of rows modified by the most recent INSERT, UPDATE, or DELETE statement.
 *
 * @param Name - Database name
 * @return Number of rows changed (positive), or negative custom error code
 *
 * Based on methods_database.cpp
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
 * Returns the total number of rows modified since the database was opened.
 *
 * @param Name - Database name
 * @return Total number of rows changed (positive), or negative custom error code
 *
 * Based on methods_database.cpp
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
 *
 * @param Name - Database name
 * @return Error message string, or empty string if no error/database not found
 *
 * Based on methods_database.cpp
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
 * @param Name - Database name
 * @param Column - Column number (1-based)
 * @return Column name string, or empty string on error
 *
 * Based on methods_database.cpp
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
 * Returns the value of a column as text from the current row.
 * Column numbers are 1-based.
 *
 * @param Name - Database name
 * @param Column - Column number (1-based)
 * @return Column value as string, or nil on error
 *
 * Based on methods_database.cpp
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
 * Integer/float columns return numbers, text/blob return strings, null returns nil.
 * Column numbers are 1-based.
 *
 * @param Name - Database name
 * @param Column - Column number (1-based)
 * @return Column value with appropriate type, or nil on error
 *
 * Based on methods_database.cpp
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
 * Returns a table of all column names from the prepared statement.
 *
 * @param Name - Database name
 * @return Table of column names (1-indexed), or empty table on error
 *
 * Based on methods_database.cpp
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
 * Returns a table of all column values from the current row.
 * Values are returned with appropriate Lua types.
 *
 * @param Name - Database name
 * @return Table of column values (1-indexed), or empty table on error
 *
 * Based on methods_database.cpp
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
 * Convenience function: executes SQL, steps once, returns first column value,
 * then finalizes the statement. Useful for simple queries like "SELECT count(*) FROM...".
 *
 * @param Name - Database name
 * @param Sql - SQL query to execute
 * @return First column value from first row, or nil if no results/error
 *
 * Based on methods_database.cpp
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
 * Returns information about a database.
 *
 * @param Name - Database name
 * @param InfoType - Type of info to return:
 *                   1 = disk filename
 *                   2 = has prepared statement (boolean)
 *                   3 = has valid row (boolean)
 *                   4 = number of columns
 * @return Requested info, or nil if database not found or invalid type
 *
 * Based on methods_database.cpp
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
 * Returns as a string to handle large rowids that might exceed Lua number precision.
 *
 * @param Name - Database name
 * @return Rowid as string, or empty string on error
 *
 * Based on methods_database.cpp
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
 * Returns a table of all open database names.
 *
 * @return Table of database names (1-indexed)
 *
 * Based on methods_database.cpp
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
