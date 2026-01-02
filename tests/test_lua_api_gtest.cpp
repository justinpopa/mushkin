/**
 * test_lua_api_gtest.cpp - GoogleTest version
 * Test Lua API function registration and functionality
 *
 * Verifies:
 * 1. world table exists
 * 2. world.Note() works
 * 3. world.ColourNote() works
 * 4. world.Send() works
 * 5. world.GetInfo() works
 * 6. Utility functions work (Hash, Base64, etc.)
 * 7. error_code table exists
 */

#include "test_qt_static.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for Lua API tests
class LuaApiTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();

        // Initialize basic state
        doc->m_mush_name = "Test World";
        doc->m_server = "test.mud.com";
        doc->m_port = 4000;
        doc->m_iConnectPhase = eConnectConnectedToMud;
        doc->m_bUTF_8 = true;

        // Create initial line (needed for note() to work)
        doc->m_currentLine = new Line(1, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_RGB;
        initialStyle->iForeColour = qRgb(192, 192, 192);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));

        // Set current style
        doc->m_iFlags = COLOUR_RGB;
        doc->m_iForeColour = qRgb(192, 192, 192);
        doc->m_iBackColour = qRgb(0, 0, 0);

        // Get Lua state
        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should exist";
        ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should exist";
        L = doc->m_ScriptEngine->L;
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to execute Lua code
    void executeLua(const char* code)
    {
        ASSERT_EQ(luaL_loadstring(L, code), 0) << "Lua code should compile: " << code;
        ASSERT_EQ(lua_pcall(L, 0, 0, 0), 0) << "Lua code should execute: " << code;
    }

    // Helper to get global string value
    QString getGlobalString(const char* name)
    {
        lua_getglobal(L, name);
        const char* str = lua_tostring(L, -1);
        QString result = str ? QString::fromUtf8(str) : QString();
        lua_pop(L, 1);
        return result;
    }

    // Helper to get global number value
    double getGlobalNumber(const char* name)
    {
        lua_getglobal(L, name);
        double result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test 1: world table exists
TEST_F(LuaApiTest, WorldTableExists)
{
    lua_getglobal(L, "world");
    EXPECT_TRUE(lua_istable(L, -1)) << "'world' should be a table";
    lua_pop(L, 1);
}

// Test 2: world.Note() works
TEST_F(LuaApiTest, WorldNote)
{
    executeLua("world.Note('Test note from Lua')");

    // Verify note was added to buffer
    ASSERT_GE(doc->m_lineList.count(), 1) << "Note should be added to buffer";

    Line* lastLine = doc->m_lineList.last();
    QString noteText = QString::fromUtf8(lastLine->text(), lastLine->len());
    EXPECT_TRUE(noteText.contains("Test note from Lua")) << "Note text should be correct";
}

// Test 3: world.ColourNote() with RGB integers
TEST_F(LuaApiTest, ColourNoteRGB)
{
    executeLua("world.ColourNote(0xFF0000, 0x000000, 'Red text')");

    // Verify colored note was added
    ASSERT_GE(doc->m_lineList.count(), 1) << "Colored note should be added";

    Line* coloredLine = doc->m_lineList.last();
    QString coloredText = QString::fromUtf8(coloredLine->text(), coloredLine->len());
    EXPECT_TRUE(coloredText.contains("Red text")) << "Colored note text should be correct";
}

// Test 3b: world.ColourNote() with color names
TEST_F(LuaApiTest, ColourNoteNames)
{
    executeLua("world.ColourNote('red', 'black', 'Red by name')");

    Line* namedColorLine = doc->m_lineList.last();
    QString namedColorText = QString::fromUtf8(namedColorLine->text(), namedColorLine->len());
    EXPECT_TRUE(namedColorText.contains("Red by name")) << "Color name note should work";
}

// Test 3c: Multi-block ColourNote
TEST_F(LuaApiTest, ColourNoteMultiBlock)
{
    executeLua("world.ColourNote('red', 'black', 'Error: ', 'yellow', 'black', 'Warning!')");

    Line* multiBlockLine = doc->m_lineList.last();
    QString multiBlockText = QString::fromUtf8(multiBlockLine->text(), multiBlockLine->len());
    EXPECT_TRUE(multiBlockText.contains("Error:")) << "Multi-block should contain first part";
    EXPECT_TRUE(multiBlockText.contains("Warning!")) << "Multi-block should contain second part";
}

// Test 4: world.GetInfo() returns world information
TEST_F(LuaApiTest, WorldGetInfo)
{
    // GetInfo(1) = server address, GetInfo(2) = world name (matches original)
    executeLua("test_server = world.GetInfo(1)");
    executeLua("test_world_name = world.GetInfo(2)");

    QString server = getGlobalString("test_server");
    QString worldName = getGlobalString("test_world_name");

    EXPECT_EQ(server, "test.mud.com") << "GetInfo(1) should return server address";
    EXPECT_EQ(worldName, "Test World") << "GetInfo(2) should return world name";
}

// Test 5: world.IsConnected() returns connection status
TEST_F(LuaApiTest, WorldIsConnected)
{
    executeLua("test_connected = world.IsConnected()");

    lua_getglobal(L, "test_connected");
    bool connected = lua_toboolean(L, -1);
    lua_pop(L, 1);

    EXPECT_TRUE(connected) << "IsConnected() should return true when connected";
}

// Test 6: world.Hash() computes SHA-256 hashes
TEST_F(LuaApiTest, WorldHash)
{
    executeLua("test_hash = world.Hash('test')");

    QString hash = getGlobalString("test_hash");
    EXPECT_FALSE(hash.isEmpty()) << "Hash() should return non-empty string";
    EXPECT_EQ(hash.length(), 64) << "SHA-256 hash should be 64 hex characters";
}

// Test 7: world.Base64Encode/Decode
TEST_F(LuaApiTest, Base64EncodeDecodeRoundTrip)
{
    executeLua(R"(
        local text = "Hello World"
        local encoded = world.Base64Encode(text)
        local decoded = world.Base64Decode(encoded)
        test_base64_ok = (decoded == text)
        test_base64_encoded = encoded
    )");

    lua_getglobal(L, "test_base64_ok");
    bool base64Ok = lua_toboolean(L, -1);
    lua_pop(L, 1);

    EXPECT_TRUE(base64Ok) << "Base64 encode/decode round-trip should succeed";

    QString encoded = getGlobalString("test_base64_encoded");
    EXPECT_FALSE(encoded.isEmpty()) << "Encoded string should not be empty";
}

// Test 8: world.Trim() trims whitespace
TEST_F(LuaApiTest, WorldTrim)
{
    executeLua("test_trimmed = world.Trim('  hello  ')");

    QString trimmed = getGlobalString("test_trimmed");
    EXPECT_EQ(trimmed, "hello") << "Trim() should remove leading/trailing whitespace";
}

// Test 9: world.GetUniqueNumber() generates unique numbers
TEST_F(LuaApiTest, WorldGetUniqueNumber)
{
    executeLua("test_unique = world.GetUniqueNumber()");

    double unique = getGlobalNumber("test_unique");
    EXPECT_NE(unique, 0.0) << "GetUniqueNumber() should return non-zero value";
}

// Test 10: error_code table exists
TEST_F(LuaApiTest, ErrorCodeTable)
{
    lua_getglobal(L, "error_code");
    ASSERT_TRUE(lua_istable(L, -1)) << "'error_code' should be a table";

    lua_getfield(L, -1, "eOK");
    double eOK = lua_tonumber(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(eOK, 0.0) << "error_code.eOK should be 0";

    lua_pop(L, 1); // Pop error_code table
}

// Test 11: Multiple parameters to Note()
TEST_F(LuaApiTest, NoteMultipleParameters)
{
    executeLua("world.Note('HP: ', 100, '/', 150)");

    Line* multiLine = doc->m_lineList.last();
    QString multiText = QString::fromUtf8(multiLine->text(), multiLine->len());
    EXPECT_TRUE(multiText.contains("HP: 100/150")) << "Multiple parameters should be concatenated";
}

// Test 12: world.ColourTell() builds multi-colored lines
TEST_F(LuaApiTest, ColourTellMultiColor)
{
    executeLua(R"(
        world.ColourTell(0xFFFF00, 0x000000, 'Yellow ')
        world.ColourTell(0xFF00FF, 0x000000, 'Magenta ')
        world.ColourTell(0x00FFFF, 0x000000, 'Cyan')
        world.Note('')  -- Complete the line
    )");

    Line* multiColorLine = doc->m_lineList.last();
    QString multiColorText = QString::fromUtf8(multiColorLine->text(), multiColorLine->len());
    EXPECT_TRUE(multiColorText.contains("Yellow")) << "Multi-color line should contain 'Yellow'";
    EXPECT_TRUE(multiColorText.contains("Magenta")) << "Multi-color line should contain 'Magenta'";
    EXPECT_TRUE(multiColorText.contains("Cyan")) << "Multi-color line should contain 'Cyan'";
}

// Test 13: world.Send() returns error codes correctly
TEST_F(LuaApiTest, SendErrorCodes)
{
    // Test Send() when connected - should return eOK
    doc->m_iConnectPhase = eConnectConnectedToMud;
    doc->m_bPluginProcessingSent = false;
    executeLua("result_ok = world.Send('test command')");
    double resultOk = getGlobalNumber("result_ok");
    EXPECT_EQ(resultOk, 0.0) << "Send() should return eOK (0) when connected";

    // Test Send() when not connected - should return eWorldClosed
    doc->m_iConnectPhase = eConnectNotConnected;
    executeLua("result_closed = world.Send('test command')");
    double resultClosed = getGlobalNumber("result_closed");
    EXPECT_EQ(resultClosed, 30002.0)
        << "Send() should return eWorldClosed (30002) when not connected";

    // Test Send() when plugin is processing - should return eItemInUse
    doc->m_iConnectPhase = eConnectConnectedToMud;
    doc->m_bPluginProcessingSent = true;
    executeLua("result_in_use = world.Send('test command')");
    double resultInUse = getGlobalNumber("result_in_use");
    EXPECT_EQ(resultInUse, 30063.0)
        << "Send() should return eItemInUse (30063) when plugin is processing";
}

// Test 14: world.Connect() returns error codes correctly
TEST_F(LuaApiTest, ConnectErrorCodes)
{
    // Test Connect() when not connected - should return eOK
    doc->m_iConnectPhase = eConnectNotConnected;
    executeLua("result_ok = world.Connect()");
    double resultOk = getGlobalNumber("result_ok");
    EXPECT_EQ(resultOk, 0.0) << "Connect() should return eOK (0) when not connected";

    // Test Connect() when already connected - should return eWorldOpen
    doc->m_iConnectPhase = eConnectConnectedToMud;
    executeLua("result_open = world.Connect()");
    double resultOpen = getGlobalNumber("result_open");
    EXPECT_EQ(resultOpen, 30001.0)
        << "Connect() should return eWorldOpen (30001) when already connected";
}

// Test 15: world.Disconnect() returns error codes correctly
TEST_F(LuaApiTest, DisconnectErrorCodes)
{
    // Test Disconnect() when connected - should return eOK
    doc->m_iConnectPhase = eConnectConnectedToMud;
    executeLua("result_ok = world.Disconnect()");
    double resultOk = getGlobalNumber("result_ok");
    EXPECT_EQ(resultOk, 0.0) << "Disconnect() should return eOK (0) when connected";

    // Test Disconnect() when not connected - should return eWorldClosed
    doc->m_iConnectPhase = eConnectNotConnected;
    executeLua("result_closed = world.Disconnect()");
    double resultClosed = getGlobalNumber("result_closed");
    EXPECT_EQ(resultClosed, 30002.0)
        << "Disconnect() should return eWorldClosed (30002) when not connected";

    // Test Disconnect() when disconnecting - should return eWorldClosed
    doc->m_iConnectPhase = eConnectDisconnecting;
    executeLua("result_disconnecting = world.Disconnect()");
    double resultDisconnecting = getGlobalNumber("result_disconnecting");
    EXPECT_EQ(resultDisconnecting, 30002.0)
        << "Disconnect() should return eWorldClosed (30002) when already disconnecting";
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
