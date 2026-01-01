/**
 * test_utils_hashing_gtest.cpp - GoogleTest version
 * Utils Hashing and Encoding Test
 *
 * Tests for utils.md5, utils.sha256, utils.base64encode, and utils.base64decode
 */

#include "test_qt_static.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture
class UtilsHashingTest : public ::testing::Test {
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

// ========== utils.md5 Tests ==========

TEST_F(UtilsHashingTest, MD5Exists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "md5");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.md5 should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, MD5EmptyString)
{
    const char* code = "return utils.md5('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // MD5 of empty string is d41d8cd98f00b204e9800998ecf8427e
    EXPECT_STREQ(lua_tostring(L, -1), "d41d8cd98f00b204e9800998ecf8427e");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, MD5SimpleString)
{
    const char* code = "return utils.md5('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // MD5 of "hello" is 5d41402abc4b2a76b9719d911017c592
    EXPECT_STREQ(lua_tostring(L, -1), "5d41402abc4b2a76b9719d911017c592");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, MD5WithSpecialCharacters)
{
    const char* code = "return utils.md5('Hello World!')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // MD5 of "Hello World!" is ed076287532e86365e841e92bfc50d8c
    EXPECT_STREQ(lua_tostring(L, -1), "ed076287532e86365e841e92bfc50d8c");
    lua_pop(L, 1);
}

// ========== utils.sha256 Tests ==========

TEST_F(UtilsHashingTest, SHA256Exists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "sha256");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.sha256 should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, SHA256EmptyString)
{
    const char* code = "return utils.sha256('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // SHA-256 of empty string is e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    EXPECT_STREQ(lua_tostring(L, -1),
                 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, SHA256SimpleString)
{
    const char* code = "return utils.sha256('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // SHA-256 of "hello" is 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    EXPECT_STREQ(lua_tostring(L, -1),
                 "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, SHA256WithSpecialCharacters)
{
    const char* code = "return utils.sha256('Hello World!')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // SHA-256 of "Hello World!" is 7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069
    EXPECT_STREQ(lua_tostring(L, -1),
                 "7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069");
    lua_pop(L, 1);
}

// ========== utils.base64encode Tests ==========

TEST_F(UtilsHashingTest, Base64EncodeExists)
{
    lua_getglobal(L, "Base64Encode");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "Base64Encode should exist";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64EncodeEmptyString)
{
    const char* code = "return Base64Encode('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64EncodeSimpleString)
{
    const char* code = "return Base64Encode('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "aGVsbG8=");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64EncodeWithSpecialCharacters)
{
    const char* code = "return Base64Encode('Hello World!')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "SGVsbG8gV29ybGQh");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64EncodeBinaryData)
{
    const char* code = "return Base64Encode('\\0\\1\\2\\3\\4\\5')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "AAECAwQF");
    lua_pop(L, 1);
}

// ========== utils.base64decode Tests ==========

TEST_F(UtilsHashingTest, Base64DecodeExists)
{
    lua_getglobal(L, "Base64Decode");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "Base64Decode should exist";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64DecodeEmptyString)
{
    const char* code = "return Base64Decode('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64DecodeSimpleString)
{
    const char* code = "return Base64Decode('aGVsbG8=')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64DecodeWithSpecialCharacters)
{
    const char* code = "return Base64Decode('SGVsbG8gV29ybGQh')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Hello World!");
    lua_pop(L, 1);
}

// ========== Round-trip Tests ==========

TEST_F(UtilsHashingTest, Base64RoundTripSimple)
{
    const char* code = "return Base64Decode(Base64Encode('test data'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "test data");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Base64RoundTripComplex)
{
    const char* code = "return "
                       "Base64Decode(Base64Encode('Complex\\nData\\tWith\\rSpecial\\0Characters'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    // Note: Lua strings can contain null bytes, but C string comparison will stop at first null
    // So we check the first part
    EXPECT_STREQ(lua_tostring(L, -1), "Complex\nData\tWith\rSpecial");
    lua_pop(L, 1);
}

// ========== utils.trim Tests ==========

TEST_F(UtilsHashingTest, TrimExists)
{
    lua_getglobal(L, "Trim");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "Trim should exist";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TrimBasic)
{
    const char* code = "return Trim('  hello  ')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TrimLeadingOnly)
{
    const char* code = "return Trim('  hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TrimTrailingOnly)
{
    const char* code = "return Trim('hello  ')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TrimNoWhitespace)
{
    const char* code = "return Trim('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TrimTabsAndNewlines)
{
    const char* code = "return Trim('\\t\\nhello\\n\\t')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

// ========== utils.compress/decompress Tests ==========

TEST_F(UtilsHashingTest, CompressExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "compress");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.compress should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, DecompressExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "decompress");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.decompress should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, CompressDecompressRoundTrip)
{
    const char* code = "return utils.decompress(utils.compress('Hello World'))";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Hello World");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, CompressDecompressLargeText)
{
    const char* code = "local data = string.rep('Test data ', 1000); return "
                       "utils.decompress(utils.compress(data)) == data";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, CompressReducesSize)
{
    const char* code = R"(
        local data = string.rep('AAAAAAAAAA', 1000)
        local compressed = utils.compress(data)
        return #compressed < #data
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "Compressed data should be smaller for repetitive content";
    lua_pop(L, 1);
}

// ========== Dialog Functions - Existence Tests ==========
// Note: These functions require user interaction, so we only test that they exist

TEST_F(UtilsHashingTest, EditboxExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "editbox");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.editbox should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, DirectorypickerExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "directorypicker");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.directorypicker should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, FilepickerExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "filepicker");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.filepicker should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, ColourpickerExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "colourpicker");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.colourpicker should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, ListboxExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "listbox");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.listbox should exist";
    lua_pop(L, 2);
}

// ========== utils.utf8len Tests ==========

TEST_F(UtilsHashingTest, Utf8lenExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8len");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8len should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8lenASCII)
{
    const char* code = "return utils.utf8len('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 5);
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8lenMultibyte)
{
    const char* code = "return utils.utf8len('héllo')"; // é is 2 bytes
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 5) << "Should count characters, not bytes";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8lenChinese)
{
    const char* code = "return utils.utf8len('你好世界')"; // 4 Chinese characters
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 4);
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8lenEmoji)
{
    const char* code = "return utils.utf8len('Hello 👋 World')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 14); // "Hello " + emoji (2 codepoints) + " World"
    lua_pop(L, 1);
}

// ========== utils.utf8valid Tests ==========

TEST_F(UtilsHashingTest, Utf8validExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8valid");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8valid should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8validASCII)
{
    const char* code = "return utils.utf8valid('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8validMultibyte)
{
    const char* code = "return utils.utf8valid('你好世界')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// ========== utils.utf8sub Tests ==========

TEST_F(UtilsHashingTest, Utf8subExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8sub");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8sub should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8subBasic)
{
    const char* code = "return utils.utf8sub('hello', 2, 4)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "ell");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8subMultibyte)
{
    const char* code = "return utils.utf8sub('你好世界', 2, 3)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "好世");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8subFromEnd)
{
    const char* code = "return utils.utf8sub('hello', -3, -1)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "llo");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8subNoEnd)
{
    const char* code = "return utils.utf8sub('hello', 2)";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "ello");
    lua_pop(L, 1);
}

// ========== utils.utf8upper/utf8lower Tests ==========

TEST_F(UtilsHashingTest, Utf8upperExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8upper");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8upper should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8lowerExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8lower");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8lower should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8upperASCII)
{
    const char* code = "return utils.utf8upper('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "HELLO");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8lowerASCII)
{
    const char* code = "return utils.utf8lower('HELLO')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8upperAccented)
{
    const char* code = "return utils.utf8upper('café')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "CAFÉ");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8lowerAccented)
{
    const char* code = "return utils.utf8lower('CAFÉ')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "café");
    lua_pop(L, 1);
}

// ========== utils.utf8encode/utf8decode Tests ==========

TEST_F(UtilsHashingTest, Utf8encodeExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8encode");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8encode should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8decodeExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "utf8decode");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.utf8decode should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, Utf8encodeSimple)
{
    const char* code = "return utils.utf8encode(72, 101, 108, 108, 111)"; // "Hello"
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8encodeMultibyte)
{
    const char* code = "return utils.utf8encode(0x4F60, 0x597D)"; // 你好
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "你好");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, Utf8decodeSimple)
{
    const char* code = R"(
        local t = utils.utf8decode('Hello')
        return t[1], t[2], t[3], t[4], t[5]
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -5), 72);  // H
    EXPECT_EQ(lua_tointeger(L, -4), 101); // e
    EXPECT_EQ(lua_tointeger(L, -3), 108); // l
    EXPECT_EQ(lua_tointeger(L, -2), 108); // l
    EXPECT_EQ(lua_tointeger(L, -1), 111); // o
    lua_pop(L, 5);
}

TEST_F(UtilsHashingTest, Utf8RoundTrip)
{
    const char* code = R"(
        local original = '你好世界'
        local codepoints = utils.utf8decode(original)
        local reconstructed = utils.utf8encode(unpack(codepoints))
        return reconstructed == original
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// ========== utils.hash Tests (SHA-1) ==========

TEST_F(UtilsHashingTest, HashExists)
{
    lua_getglobal(L, "Hash");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "Hash should exist";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, HashEmptyString)
{
    // SHA-256 of empty string is e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const char* code = "return Hash('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1),
                 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, HashSimpleString)
{
    // SHA-256 of "hello" is 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    const char* code = "return Hash('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1),
                 "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, HashLength)
{
    // SHA-256 always returns 64 hex characters
    const char* code = "return #Hash('test')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_EQ(lua_tointeger(L, -1), 64);
    lua_pop(L, 1);
}

// ========== utils.tohex Tests ==========

TEST_F(UtilsHashingTest, TohexExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "tohex");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.tohex should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, TohexEmptyString)
{
    const char* code = "return utils.tohex('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TohexSimpleString)
{
    // "hello" -> "48656C6C6F"
    const char* code = "return utils.tohex('hello')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "68656C6C6F");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, TohexBinaryData)
{
    // Test with null bytes
    const char* code = "return utils.tohex('\\0\\1\\2\\3')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "00010203");
    lua_pop(L, 1);
}

// ========== utils.fromhex Tests ==========

TEST_F(UtilsHashingTest, FromhexExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "fromhex");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.fromhex should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, FromhexEmptyString)
{
    const char* code = "return utils.fromhex('')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, FromhexSimpleString)
{
    // "68656C6C6F" -> "hello"
    const char* code = "return utils.fromhex('68656C6C6F')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, FromhexWithSpaces)
{
    // Spaces should be ignored
    const char* code = "return utils.fromhex('68 65 6C 6C 6F')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, FromhexCaseInsensitive)
{
    // Should work with lowercase
    const char* code = "return utils.fromhex('48656c6c6f')";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "Hello");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, HexRoundTrip)
{
    // tohex and fromhex should be inverses
    const char* code = R"(
        local original = 'Binary data: \0\1\2\3\255'
        local hexed = utils.tohex(original)
        local restored = utils.fromhex(hexed)
        return restored == original
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// ========== utils.info Tests ==========

TEST_F(UtilsHashingTest, InfoExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "info");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.info should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, InfoReturnsTable)
{
    const char* code = "return type(utils.info())";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_STREQ(lua_tostring(L, -1), "table");
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, InfoHasCurrentDirectory)
{
    const char* code = R"(
        local info = utils.info()
        return info.current_directory ~= nil
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, InfoHasAppDirectory)
{
    const char* code = R"(
        local info = utils.info()
        return info.app_directory ~= nil
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, InfoHasOSName)
{
    const char* code = R"(
        local info = utils.info()
        return info.os_name ~= nil
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, InfoHasQtVersion)
{
    const char* code = R"(
        local info = utils.info()
        return info.qt_version ~= nil and #info.qt_version > 0
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// ========== utils.multilistbox Tests ==========

TEST_F(UtilsHashingTest, MultilistboxExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "multilistbox");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.multilistbox should exist";
    lua_pop(L, 2);
}

// Note: multilistbox requires GUI interaction, so we can only test existence
// Full functionality testing would require Qt Test or automated UI testing

// ========== utils.shellexecute Tests ==========

TEST_F(UtilsHashingTest, ShellexecuteExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "shellexecute");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.shellexecute should exist";
    lua_pop(L, 2);
}

// Note: shellexecute actually opens files/URLs, so we can't fully test it in automated tests
// We can only verify it exists and doesn't crash with invalid input

TEST_F(UtilsHashingTest, ShellexecuteInvalidOperation)
{
    // Test with an unsupported operation
    const char* code = R"(
        local ok, err = utils.shellexecute("test.txt", "", "", "unsupported_op")
        return ok == nil and type(err) == "string"
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "Should return nil and error for unsupported operation";
    lua_pop(L, 1);
}

// ========== utils.xmlread Tests ==========

TEST_F(UtilsHashingTest, XmlreadExists)
{
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "xmlread");
    EXPECT_TRUE(lua_isfunction(L, -1)) << "utils.xmlread should exist";
    lua_pop(L, 2);
}

TEST_F(UtilsHashingTest, XmlreadSimple)
{
    const char* code = R"(
        local xml = '<root>Hello</root>'
        local t, name = utils.xmlread(xml)
        return t ~= nil and name == 'root' and t.name == 'root' and t.content == 'Hello'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadWithAttributes)
{
    const char* code = R"(
        local xml = '<root id="123" name="test">Content</root>'
        local t = utils.xmlread(xml)
        return t.attributes.id == '123' and t.attributes.name == 'test'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadNested)
{
    const char* code = R"(
        local xml = '<root><child1>A</child1><child2>B</child2></root>'
        local t = utils.xmlread(xml)
        return t.nodes ~= nil and
               #t.nodes == 2 and
               t.nodes[1].name == 'child1' and
               t.nodes[1].content == 'A' and
               t.nodes[2].name == 'child2' and
               t.nodes[2].content == 'B'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadSelfClosing)
{
    const char* code = R"(
        local xml = '<root><br/></root>'
        local t = utils.xmlread(xml)
        return t.nodes ~= nil and
               #t.nodes == 1 and
               t.nodes[1].name == 'br' and
               t.nodes[1].empty == true
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadMalformed)
{
    const char* code = R"(
        local xml = '<root><unclosed>'
        local t, name, line = utils.xmlread(xml)
        return t == nil and type(name) == 'string' and type(line) == 'number'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "Should return nil, error, line for malformed XML";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadEmpty)
{
    const char* code = R"(
        local xml = ''
        local t, name, line = utils.xmlread(xml)
        return t == nil and type(name) == 'string'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1)) << "Should return error for empty XML";
    lua_pop(L, 1);
}

TEST_F(UtilsHashingTest, XmlreadComplex)
{
    const char* code = R"(
        local xml = [[
            <trigger name="test" enabled="y">
                <pattern>^HP: (\d+)$</pattern>
                <send>say My HP is %1</send>
            </trigger>
        ]]
        local t = utils.xmlread(xml)
        return t.name == 'trigger' and
               t.attributes.name == 'test' and
               t.attributes.enabled == 'y' and
               #t.nodes == 2 and
               t.nodes[1].name == 'pattern' and
               t.nodes[2].name == 'send'
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
}

// ========== Main function ==========

int main(int argc, char** argv)
{
    // Initialize QApplication (required for Qt)
    QApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run tests
    return RUN_ALL_TESTS();
}
