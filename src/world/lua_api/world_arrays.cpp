/**
 * world_arrays.cpp - Array Lua API Functions
 *
 * Implements the MUSHclient Array API for key-value storage:
 * - ArrayCreate, ArrayDelete, ArrayClear
 * - ArraySet, ArrayGet, ArrayDeleteKey
 * - ArrayExists, ArrayKeyExists
 * - ArrayCount, ArraySize
 * - ArrayGetFirstKey, ArrayGetLastKey
 * - ArrayListAll, ArrayListKeys, ArrayListValues
 * - ArrayExport, ArrayExportKeys, ArrayImport
 *
 * Arrays are named collections of key-value pairs (both strings).
 * Plugin-aware: each plugin has its own isolated set of arrays.
 *
 * Based on methods_arrays.cpp
 */

#include "lua_common.h"

// ========== Array Functions ==========

/**
 * ArrayCreate - Create a new named array
 *
 * Lua: result = ArrayCreate(name)
 *
 * @param name Array name
 * @return eOK (0) if created, eArrayAlreadyExists if array exists
 */
static int L_ArrayCreate(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    ArraysMap& arrays = pDoc->getArrayMap();

    // Check if array already exists
    if (arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayAlreadyExists);
        return 1;
    }

    // Create empty array
    arrays[arrayName] = QMap<QString, QString>();

    lua_pushnumber(L, eOK);
    return 1;
}

/**
 * ArrayDelete - Delete a named array
 *
 * Lua: result = ArrayDelete(name)
 *
 * @param name Array name
 * @return eOK if deleted, eArrayDoesNotExist if not found
 */
static int L_ArrayDelete(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    arrays.remove(arrayName);

    lua_pushnumber(L, eOK);
    return 1;
}

/**
 * ArrayClear - Clear all entries from an array
 *
 * Lua: result = ArrayClear(name)
 *
 * @param name Array name
 * @return eOK if cleared, eArrayDoesNotExist if not found
 */
static int L_ArrayClear(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    arrays[arrayName].clear();

    lua_pushnumber(L, eOK);
    return 1;
}

/**
 * ArraySet - Set a key-value pair in an array
 *
 * Lua: result = ArraySet(name, key, value)
 *
 * @param name Array name
 * @param key Key to set
 * @param value Value to set
 * @return eOK if set (new key), eSetReplacingExistingValue if replaced,
 *         eArrayDoesNotExist if array not found
 */
static int L_ArraySet(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString keyStr = QString::fromUtf8(key);
    QString valueStr = QString::fromUtf8(value);
    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    // Check if key already exists
    bool existed = arrays[arrayName].contains(keyStr);

    arrays[arrayName][keyStr] = valueStr;

    lua_pushnumber(L, existed ? eSetReplacingExistingValue : eOK);
    return 1;
}

/**
 * ArrayGet - Get a value from an array by key
 *
 * Lua: value = ArrayGet(name, key)
 *
 * @param name Array name
 * @param key Key to get
 * @return Value if found, nil if array or key not found
 */
static int L_ArrayGet(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString keyStr = QString::fromUtf8(key);
    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnil(L);
        return 1;
    }

    if (!arrays[arrayName].contains(keyStr)) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray valueBytes = arrays[arrayName][keyStr].toUtf8();
    lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
    return 1;
}

/**
 * ArrayDeleteKey - Delete a key from an array
 *
 * Lua: result = ArrayDeleteKey(name, key)
 *
 * @param name Array name
 * @param key Key to delete
 * @return eOK if deleted, eArrayDoesNotExist if array not found,
 *         eKeyDoesNotExist if key not found
 */
static int L_ArrayDeleteKey(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString keyStr = QString::fromUtf8(key);
    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    if (!arrays[arrayName].contains(keyStr)) {
        lua_pushnumber(L, eKeyDoesNotExist);
        return 1;
    }

    arrays[arrayName].remove(keyStr);

    lua_pushnumber(L, eOK);
    return 1;
}

/**
 * ArrayExists - Check if an array exists
 *
 * Lua: exists = ArrayExists(name)
 *
 * @param name Array name
 * @return true if array exists, false otherwise
 */
static int L_ArrayExists(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    lua_pushboolean(L, arrays.contains(arrayName));
    return 1;
}

/**
 * ArrayKeyExists - Check if a key exists in an array
 *
 * Lua: exists = ArrayKeyExists(name, key)
 *
 * @param name Array name
 * @param key Key to check
 * @return true if key exists, false if array or key not found
 */
static int L_ArrayKeyExists(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString keyStr = QString::fromUtf8(key);
    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, arrays[arrayName].contains(keyStr));
    return 1;
}

/**
 * ArrayCount - Get the number of arrays
 *
 * Lua: count = ArrayCount()
 *
 * @return Number of arrays
 */
static int L_ArrayCount(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const ArraysMap& arrays = pDoc->getArrayMap();

    lua_pushnumber(L, static_cast<lua_Number>(arrays.size()));
    return 1;
}

/**
 * ArraySize - Get the number of elements in an array
 *
 * Lua: size = ArraySize(name)
 *
 * @param name Array name
 * @return Number of elements, 0 if array not found
 */
static int L_ArraySize(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, 0);
        return 1;
    }

    lua_pushnumber(L, static_cast<lua_Number>(arrays[arrayName].size()));
    return 1;
}

/**
 * ArrayGetFirstKey - Get the first key in an array (alphabetically)
 *
 * Lua: key = ArrayGetFirstKey(name)
 *
 * @param name Array name
 * @return First key (alphabetically), nil if array empty or not found
 */
static int L_ArrayGetFirstKey(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName) || arrays[arrayName].isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // QMap is sorted by key, so constBegin() gives the first key
    QByteArray keyBytes = arrays[arrayName].constBegin().key().toUtf8();
    lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
    return 1;
}

/**
 * ArrayGetLastKey - Get the last key in an array (alphabetically)
 *
 * Lua: key = ArrayGetLastKey(name)
 *
 * @param name Array name
 * @return Last key (alphabetically), nil if array empty or not found
 */
static int L_ArrayGetLastKey(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName) || arrays[arrayName].isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // QMap is sorted by key, so lastKey() gives the last key
    QByteArray keyBytes = arrays[arrayName].lastKey().toUtf8();
    lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
    return 1;
}

/**
 * ArrayListAll - List all array names
 *
 * Lua: names = ArrayListAll()
 *
 * @return Table of array names (1-indexed)
 */
static int L_ArrayListAll(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const ArraysMap& arrays = pDoc->getArrayMap();

    lua_newtable(L);

    int index = 1;
    for (auto it = arrays.constBegin(); it != arrays.constEnd(); ++it) {
        QByteArray nameBytes = it.key().toUtf8();
        lua_pushlstring(L, nameBytes.constData(), nameBytes.length());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * ArrayListKeys - List all keys in an array
 *
 * Lua: keys = ArrayListKeys(name)
 *
 * @param name Array name
 * @return Table of keys (1-indexed), empty table if array not found
 */
static int L_ArrayListKeys(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    lua_newtable(L);

    if (!arrays.contains(arrayName)) {
        return 1; // Return empty table
    }

    const QMap<QString, QString>& arr = arrays[arrayName];
    int index = 1;
    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
        QByteArray keyBytes = it.key().toUtf8();
        lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * ArrayListValues - List all values in an array
 *
 * Lua: values = ArrayListValues(name)
 *
 * @param name Array name
 * @return Table of values (1-indexed), empty table if array not found
 */
static int L_ArrayListValues(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    lua_newtable(L);

    if (!arrays.contains(arrayName)) {
        return 1; // Return empty table
    }

    const QMap<QString, QString>& arr = arrays[arrayName];
    int index = 1;
    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
        QByteArray valueBytes = it.value().toUtf8();
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * Helper: Escape delimiter and backslash for export
 */
static QString escapeForExport(const QString& str, const QString& delimiter)
{
    QString result = str;
    // Replace backslash with double backslash
    result.replace("\\", "\\\\");
    // Replace delimiter with escaped delimiter
    result.replace(delimiter, "\\" + delimiter);
    return result;
}

/**
 * ArrayExport - Export array as delimited string
 *
 * Lua: result = ArrayExport(name, delimiter)
 *
 * Format: key1<delim>value1<delim>key2<delim>value2...
 * Backslashes and delimiters in keys/values are escaped with backslash.
 *
 * @param name Array name
 * @param delimiter Single character delimiter (not backslash)
 * @return Delimited string if success, error code number if error
 */
static int L_ArrayExport(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* delim = luaL_checkstring(L, 2);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString delimiter = QString::fromUtf8(delim);

    // Validate delimiter
    if (delimiter.length() != 1 || delimiter == "\\") {
        lua_pushnumber(L, eBadDelimiter);
        return 1;
    }

    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    const QMap<QString, QString>& arr = arrays[arrayName];

    // Empty array returns empty string
    if (arr.isEmpty()) {
        lua_pushstring(L, "");
        return 1;
    }

    // Build result string
    QString result;
    bool first = true;

    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
        if (!first) {
            result += delimiter;
        }
        first = false;

        QString escapedKey = escapeForExport(it.key(), delimiter);
        QString escapedValue = escapeForExport(it.value(), delimiter);

        result += escapedKey + delimiter + escapedValue;
    }

    QByteArray resultBytes = result.toUtf8();
    lua_pushlstring(L, resultBytes.constData(), resultBytes.length());
    return 1;
}

/**
 * ArrayExportKeys - Export array keys as delimited string
 *
 * Lua: result = ArrayExportKeys(name, delimiter)
 *
 * @param name Array name
 * @param delimiter Single character delimiter (not backslash)
 * @return Delimited string if success, error code number if error
 */
static int L_ArrayExportKeys(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* delim = luaL_checkstring(L, 2);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString delimiter = QString::fromUtf8(delim);

    // Validate delimiter
    if (delimiter.length() != 1 || delimiter == "\\") {
        lua_pushnumber(L, eBadDelimiter);
        return 1;
    }

    const ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    const QMap<QString, QString>& arr = arrays[arrayName];

    // Empty array returns empty string
    if (arr.isEmpty()) {
        lua_pushstring(L, "");
        return 1;
    }

    // Build result string
    QString result;
    bool first = true;

    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
        if (!first) {
            result += delimiter;
        }
        first = false;

        result += escapeForExport(it.key(), delimiter);
    }

    QByteArray resultBytes = result.toUtf8();
    lua_pushlstring(L, resultBytes.constData(), resultBytes.length());
    return 1;
}

/**
 * ArrayImport - Import key-value pairs from delimited string
 *
 * Lua: result = ArrayImport(name, values, delimiter)
 *
 * Format: key1<delim>value1<delim>key2<delim>value2...
 * Must have even number of values (key-value pairs).
 * Escaped delimiters (\<delim>) and double backslashes (\\) are unescaped.
 *
 * @param name Array name (must already exist)
 * @param values Delimited string of key-value pairs
 * @param delimiter Single character delimiter (not backslash)
 * @return eOK if success, eImportedWithDuplicates if some keys replaced,
 *         eArrayDoesNotExist, eBadDelimiter, eArrayNotEvenNumberOfValues,
 *         eCannotImport if appropriate errors
 */
static int L_ArrayImport(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* values = luaL_checkstring(L, 2);
    const char* delim = luaL_checkstring(L, 3);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    QString valuesStr = QString::fromUtf8(values);
    QString delimiter = QString::fromUtf8(delim);

    // Validate delimiter
    if (delimiter.length() != 1 || delimiter == "\\") {
        lua_pushnumber(L, eBadDelimiter);
        return 1;
    }

    ArraysMap& arrays = pDoc->getArrayMap();

    if (!arrays.contains(arrayName)) {
        lua_pushnumber(L, eArrayDoesNotExist);
        return 1;
    }

    // Empty values string is OK, just do nothing
    if (valuesStr.isEmpty()) {
        lua_pushnumber(L, eOK);
        return 1;
    }

    // Handle escaped delimiters by temporarily replacing them
    // Find a character not in the string to use as placeholder
    QString escapedDelimiter = "\\" + delimiter;
    QString placeholder;

    if (valuesStr.contains(escapedDelimiter)) {
        // Find an unused character
        bool found = false;
        for (int ch = 1; ch <= 255; ch++) {
            QString test = QString(QChar(ch));
            if (!valuesStr.contains(test)) {
                placeholder = test;
                found = true;
                break;
            }
        }

        if (!found) {
            lua_pushnumber(L, eCannotImport);
            return 1;
        }

        // Replace escaped delimiter with placeholder
        valuesStr.replace(escapedDelimiter, placeholder);
    }

    // Split by delimiter
    QStringList parts = valuesStr.split(delimiter);

    // Must have even number of parts (key-value pairs)
    if (parts.size() % 2 != 0) {
        lua_pushnumber(L, eArrayNotEvenNumberOfValues);
        return 1;
    }

    // Import pairs
    QMap<QString, QString>& arr = arrays[arrayName];
    int duplicates = 0;

    for (int i = 0; i < parts.size(); i += 2) {
        QString key = parts[i];
        QString value = parts[i + 1];

        // Unescape
        key.replace("\\\\", "\\");
        if (!placeholder.isEmpty()) {
            key.replace(placeholder, delimiter);
        }
        value.replace("\\\\", "\\");
        if (!placeholder.isEmpty()) {
            value.replace(placeholder, delimiter);
        }

        // Check if key already exists
        if (arr.contains(key)) {
            duplicates++;
        }

        arr[key] = value;
    }

    lua_pushnumber(L, duplicates > 0 ? eImportedWithDuplicates : eOK);
    return 1;
}

/**
 * ArrayList - Get array as a Lua table (key-value pairs)
 *
 * Lua: table = ArrayList(name)
 *
 * Returns the array as a Lua table where keys map to values.
 * This is different from ArrayListKeys/ArrayListValues which return indexed tables.
 *
 * @param name Array name
 * @return Table of key-value pairs, or nothing if array not found
 */
static int L_ArrayList(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    WorldDocument* pDoc = doc(L);

    QString arrayName = QString::fromUtf8(name);
    const ArraysMap& arrays = pDoc->getArrayMap();

    auto it = arrays.find(arrayName);
    if (it == arrays.end()) {
        return 0; // Array not found, return nothing (nil)
    }

    lua_newtable(L);

    const QMap<QString, QString>& arr = it.value();
    for (auto i = arr.constBegin(); i != arr.constEnd(); ++i) {
        QByteArray keyBytes = i.key().toUtf8();
        QByteArray valueBytes = i.value().toUtf8();
        lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
        lua_pushlstring(L, valueBytes.constData(), valueBytes.length());
        lua_rawset(L, -3);
    }

    return 1;
}

// ========== Registration ==========

// Array function table
static const luaL_Reg arrayFuncs[] = {{"ArrayCreate", L_ArrayCreate},
                                      {"ArrayDelete", L_ArrayDelete},
                                      {"ArrayClear", L_ArrayClear},
                                      {"ArraySet", L_ArraySet},
                                      {"ArrayGet", L_ArrayGet},
                                      {"ArrayDeleteKey", L_ArrayDeleteKey},
                                      {"ArrayExists", L_ArrayExists},
                                      {"ArrayKeyExists", L_ArrayKeyExists},
                                      {"ArrayCount", L_ArrayCount},
                                      {"ArraySize", L_ArraySize},
                                      {"ArrayGetFirstKey", L_ArrayGetFirstKey},
                                      {"ArrayGetLastKey", L_ArrayGetLastKey},
                                      {"ArrayList", L_ArrayList},
                                      {"ArrayListAll", L_ArrayListAll},
                                      {"ArrayListKeys", L_ArrayListKeys},
                                      {"ArrayListValues", L_ArrayListValues},
                                      {"ArrayExport", L_ArrayExport},
                                      {"ArrayExportKeys", L_ArrayExportKeys},
                                      {"ArrayImport", L_ArrayImport},
                                      {nullptr, nullptr}};

/**
 * register_array_functions - Register array functions with Lua
 *
 * Called by register_lua_functions() during scripting engine initialization.
 * Registers functions in both the world table and as globals for compatibility.
 */
void register_array_functions(lua_State* L, int worldTable)
{
    for (const luaL_Reg* func = arrayFuncs; func->name != nullptr; func++) {
        // Register in world table
        lua_pushcfunction(L, func->func);
        lua_setfield(L, worldTable, func->name);

        // Also register as global for backward compatibility
        lua_pushcfunction(L, func->func);
        lua_setglobal(L, func->name);
    }
}
