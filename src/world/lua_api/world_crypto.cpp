/**
 * world_crypto.cpp - Cryptographic and Encoding Functions
 */

#include "../../utils/name_generator.h"
#include "lua_common.h"
#include <QCryptographicHash>

/**
 * world.Hash(text)
 *
 * Computes SHA-256 hash of text and returns it as a hex string.
 *
 * @param text (string) String to hash (binary safe)
 *
 * @return (string) 64-character hex SHA-256 hash (lowercase)
 *
 * @example
 * local hash = Hash("password123")
 * Note("SHA-256: " .. hash)
 *
 * @see utils.md5, utils.sha256
 */
int L_Hash(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray data(text, len);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString hexHash = hash.toHex();

    luaPushQString(L, hexHash);
    return 1;
}

/**
 * utils.md5(text)
 *
 * Computes MD5 hash of text and returns it as a hex string.
 * Note: MD5 is not cryptographically secure; use SHA-256 for security.
 *
 * @param text (string) String to hash (binary safe)
 *
 * @return (string) 32-character hex MD5 hash (lowercase)
 *
 * @example
 * local hash = utils.md5("hello world")
 * Note("MD5: " .. hash)  -- "5eb63bbbe01eeed093cb22bb8f5acdc3"
 *
 * @see utils.sha256, Hash
 */
int L_Utils_MD5(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray data(text, len);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    QString hexHash = hash.toHex();

    luaPushQString(L, hexHash);
    return 1;
}

/**
 * utils.sha256(text)
 *
 * Computes SHA-256 hash of text and returns it as a hex string.
 * SHA-256 is cryptographically secure and suitable for password hashing.
 *
 * @param text (string) String to hash (binary safe)
 *
 * @return (string) 64-character hex SHA-256 hash (lowercase)
 *
 * @example
 * local hash = utils.sha256("secret data")
 * Note("SHA-256: " .. hash)
 *
 * @see utils.md5, Hash
 */
int L_Utils_SHA256(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray data(text, len);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString hexHash = hash.toHex();

    luaPushQString(L, hexHash);
    return 1;
}

/**
 * utils.base64encode(text)
 *
 * Encodes text to Base64 format. Useful for transmitting binary
 * data as text or for basic obfuscation.
 *
 * @param text (string) String to encode (binary safe)
 *
 * @return (string) Base64-encoded string
 *
 * @example
 * local encoded = utils.base64encode("Hello, World!")
 * Note(encoded)  -- "SGVsbG8sIFdvcmxkIQ=="
 *
 * @see utils.base64decode, Base64Encode
 */
int L_Utils_Base64Encode(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray data(text, len);
    QByteArray encoded = data.toBase64();

    lua_pushlstring(L, encoded.constData(), encoded.length());
    return 1;
}

/**
 * utils.base64decode(text)
 *
 * Decodes a Base64-encoded string back to its original form.
 *
 * @param text (string) Base64-encoded string
 *
 * @return (string) Decoded string (binary safe)
 *
 * @example
 * local decoded = utils.base64decode("SGVsbG8sIFdvcmxkIQ==")
 * Note(decoded)  -- "Hello, World!"
 *
 * @see utils.base64encode, Base64Decode
 */
int L_Utils_Base64Decode(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray encoded(text, len);
    QByteArray decoded = QByteArray::fromBase64(encoded);

    lua_pushlstring(L, decoded.constData(), decoded.length());
    return 1;
}

/**
 * world.Base64Encode(text)
 *
 * Encodes text to Base64 format.
 *
 * @param text (string) String to encode (binary safe)
 *
 * @return (string) Base64-encoded string
 *
 * @example
 * local encoded = Base64Encode("binary\x00data")
 *
 * @see Base64Decode, utils.base64encode
 */
int L_Base64Encode(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray data(text, len);
    QByteArray encoded = data.toBase64();

    lua_pushlstring(L, encoded.constData(), encoded.length());
    return 1;
}

/**
 * world.Base64Decode(text)
 *
 * Decodes a Base64-encoded string.
 *
 * @param text (string) Base64-encoded string
 *
 * @return (string) Decoded string (binary safe)
 *
 * @example
 * local original = Base64Decode(encoded)
 *
 * @see Base64Encode, utils.base64decode
 */
int L_Base64Decode(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QByteArray encoded(text, len);
    QByteArray decoded = QByteArray::fromBase64(encoded);

    lua_pushlstring(L, decoded.constData(), decoded.length());
    return 1;
}

/**
 * world.GetUniqueNumber()
 *
 * Returns a unique number based on timestamp and counter.
 * Useful for generating unique identifiers.
 *
 * @return (number) Unique number that increases with each call
 *
 * @example
 * local id = GetUniqueNumber()
 * local timerName = "timer_" .. id
 *
 * @see GetUniqueID, CreateGUID
 */
int L_GetUniqueNumber(lua_State* L)
{
    static quint64 counter = 0;
    quint64 unique = QDateTime::currentMSecsSinceEpoch() * 1000 + (++counter % 1000);

    lua_pushnumber(L, unique);
    return 1;
}

/**
 * world.GetUniqueID()
 *
 * Returns a 24-character unique hex ID suitable for plugin IDs.
 * Based on UUID hashed with SHA-1, matching original MUSHclient.
 *
 * @return (string) 24-character lowercase hex ID
 *
 * @example
 * local pluginId = GetUniqueID()
 * Note("Plugin ID: " .. pluginId)  -- e.g., "3e7dedcf168620e8f3e7d3a6"
 *
 * @see CreateGUID, GetUniqueNumber
 */
int L_GetUniqueID(lua_State* L)
{
    return luaReturn(L, generateUniqueID());
}

/**
 * world.CreateGUID()
 *
 * Creates a UUID/GUID in standard RFC 4122 format with dashes.
 * Suitable for unique identifiers, database keys, or tracking IDs.
 *
 * @return (string) 36-character GUID: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
 *
 * @example
 * local guid = CreateGUID()
 * Note("New GUID: " .. guid)  -- e.g., "550E8400-E29B-41D4-A716-446655440000"
 *
 * -- Use as unique database key
 * DatabaseExec(db, "INSERT INTO sessions (id) VALUES ('" .. guid .. "')")
 *
 * @see GetUniqueID, GetUniqueNumber
 */
int L_CreateGUID(lua_State* L)
{
    return luaReturn(L, createGUID());
}

/**
 * world.GenerateName()
 *
 * Generates a random fantasy character name using the built-in syllable
 * generator. The generator combines consonant-vowel syllable blocks to
 * produce pronounceable fantasy names (e.g., "Drakmorwyn", "Zarethion").
 *
 * In the original MUSHclient this required calling ReadNamesFile() first
 * to load an external syllable list. Mushkin uses an embedded generator
 * that works without any external file.
 *
 * @return (string) Generated character name (2-3 syllables, 5-12 chars)
 *
 * @example
 * local name = GenerateName()
 * Note("Your character name: " .. name)
 *
 * @see ReadNamesFile
 */
int L_GenerateName(lua_State* L)
{
    QString name = generateCharacterName();
    if (name.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }
    luaPushQString(L, name);
    return 1;
}

/**
 * world.ReadNamesFile(filename)
 *
 * Load a custom syllable file for GenerateName(). In the original
 * MUSHclient this replaced the built-in generator's syllable lists.
 * Mushkin uses an embedded generator that does not require an external
 * file, so this function is a stub that accepts the argument and returns
 * eOK for plugin compatibility.
 *
 * @param filename (string) Path to the names file (accepted but ignored)
 *
 * @return (number) Error code:
 *   - eOK (0): Always succeeds (stub)
 *   - eNoNameSpecified (30003): filename was empty
 *
 * @example
 * local result = ReadNamesFile("custom_names.txt")
 * if result == 0 then
 *     local name = GenerateName()
 * end
 *
 * @see GenerateName
 */
int L_ReadNamesFile(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    if (filename == nullptr || filename[0] == '\0') {
        return luaReturnError(L, eNoNameSpecified);
    }
    // Stub: the built-in generator needs no external file.
    return luaReturnOK(L);
}

/**
 * world.Metaphone(Word, Length)
 *
 * Computes metaphone phonetic encoding (deprecated - spell check removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param Word Word to encode
 * @param Length Maximum length of result
 * @return Always returns empty string (spell check removed)
 */
int L_Metaphone(lua_State* L)
{
    Q_UNUSED(L);
    lua_pushstring(L, "");
    return 1;
}
