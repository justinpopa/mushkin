/**
 * test_notepad_api_gtest.cpp - Notepad API tests
 * Test Lua notepad window functions
 *
 * Verifies:
 * 1. SendToNotepad creates new notepads and replaces content
 * 2. AppendToNotepad appends to existing notepads
 * 3. ReplaceNotepad only replaces existing notepads
 * 4. GetNotepadText retrieves text content
 * 5. GetNotepadLength returns correct length
 * 6. GetNotepadList returns notepad titles
 * 7. NotepadFont sets font properties
 * 8. NotepadColour sets text/background colors
 * 9. NotepadReadOnly sets read-only mode
 * 10. NotepadSaveMethod sets save behavior
 * 11. CloseNotepad closes notepads
 * 12. Error codes for non-existent notepads
 */

#include "test_qt_static.h"
#include "../src/world/notepad_widget.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Error codes from lua_common.h
const int eOK = 0;
const int eNoSuchNotepad = 30075;
const int eInvalidColourName = 30077;

// Test fixture for Notepad API tests
class NotepadApiTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();

        // Initialize basic state
        doc->m_mush_name = "Test World";
        doc->m_server = "test.mud.com";
        doc->m_port = 4000;
        doc->m_bUTF_8 = true;

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

    // Helper to get global boolean value
    bool getGlobalBool(const char* name)
    {
        lua_getglobal(L, name);
        bool result = lua_toboolean(L, -1);
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

    // Helper to get global string value
    QString getGlobalString(const char* name)
    {
        lua_getglobal(L, name);
        const char* str = lua_tostring(L, -1);
        QString result = str ? QString::fromUtf8(str) : QString();
        lua_pop(L, 1);
        return result;
    }

    WorldDocument* doc = nullptr;
    lua_State* L = nullptr;
};

// Test 1: SendToNotepad creates new notepad
TEST_F(NotepadApiTest, SendToNotepadCreatesNew)
{
    executeLua("result = world.SendToNotepad('Test Notepad', 'Hello, World!')");
    EXPECT_TRUE(getGlobalBool("result"));

    // Verify notepad was created
    NotepadWidget* notepad = doc->FindNotepad("Test Notepad");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "Hello, World!");
}

// Test 2: SendToNotepad replaces existing notepad
TEST_F(NotepadApiTest, SendToNotepadReplacesExisting)
{
    executeLua("world.SendToNotepad('Test', 'Original')");
    executeLua("result = world.SendToNotepad('Test', 'Replaced')");
    EXPECT_TRUE(getGlobalBool("result"));

    NotepadWidget* notepad = doc->FindNotepad("Test");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "Replaced");
}

// Test 3: AppendToNotepad creates new notepad if needed
TEST_F(NotepadApiTest, AppendToNotepadCreatesNew)
{
    executeLua("result = world.AppendToNotepad('Append Test', 'First line')");
    EXPECT_TRUE(getGlobalBool("result"));

    NotepadWidget* notepad = doc->FindNotepad("Append Test");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "First line");
}

// Test 4: AppendToNotepad appends to existing notepad
TEST_F(NotepadApiTest, AppendToNotepadAppendsExisting)
{
    executeLua("world.SendToNotepad('Append', 'Line 1\\n')");
    executeLua("result = world.AppendToNotepad('Append', 'Line 2\\n')");
    EXPECT_TRUE(getGlobalBool("result"));

    NotepadWidget* notepad = doc->FindNotepad("Append");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "Line 1\nLine 2\n");
}

// Test 5: ReplaceNotepad fails if notepad doesn't exist
TEST_F(NotepadApiTest, ReplaceNotepadFailsIfNotExists)
{
    executeLua("result = world.ReplaceNotepad('NonExistent', 'text')");
    EXPECT_FALSE(getGlobalBool("result"));
}

// Test 6: ReplaceNotepad replaces existing notepad
TEST_F(NotepadApiTest, ReplaceNotepadReplacesExisting)
{
    executeLua("world.SendToNotepad('Replace', 'Original')");
    executeLua("result = world.ReplaceNotepad('Replace', 'New Content')");
    EXPECT_TRUE(getGlobalBool("result"));

    NotepadWidget* notepad = doc->FindNotepad("Replace");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "New Content");
}

// Test 7: GetNotepadText retrieves content
TEST_F(NotepadApiTest, GetNotepadTextRetrievesContent)
{
    executeLua("world.SendToNotepad('Content', 'Test Content 123')");
    executeLua("text = world.GetNotepadText('Content')");
    EXPECT_EQ(getGlobalString("text"), "Test Content 123");
}

// Test 8: GetNotepadText returns nil for non-existent notepad
TEST_F(NotepadApiTest, GetNotepadTextReturnsNilIfNotExists)
{
    executeLua("text = world.GetNotepadText('DoesNotExist')");
    lua_getglobal(L, "text");
    EXPECT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

// Test 9: GetNotepadLength returns correct length
TEST_F(NotepadApiTest, GetNotepadLengthReturnsCorrectLength)
{
    executeLua("world.SendToNotepad('Length', '12345678901234567890')");
    executeLua("len = world.GetNotepadLength('Length')");
    EXPECT_EQ(getGlobalNumber("len"), 20);
}

// Test 10: GetNotepadLength returns 0 for non-existent notepad
TEST_F(NotepadApiTest, GetNotepadLengthReturnsZeroIfNotExists)
{
    executeLua("len = world.GetNotepadLength('DoesNotExist')");
    EXPECT_EQ(getGlobalNumber("len"), 0);
}

// Test 11: GetNotepadList returns all notepad titles
TEST_F(NotepadApiTest, GetNotepadListReturnsAllTitles)
{
    executeLua("world.SendToNotepad('Notepad1', 'content1')");
    executeLua("world.SendToNotepad('Notepad2', 'content2')");
    executeLua("world.SendToNotepad('Notepad3', 'content3')");
    executeLua(R"(
        list = world.GetNotepadList()
        count = #list
    )");
    EXPECT_EQ(getGlobalNumber("count"), 3);
}

// Test 12: NotepadFont returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadFontFailsIfNotExists)
{
    executeLua("result = world.NotepadFont('NoSuchNotepad', 'Courier', 12, 0, 0)");
    EXPECT_EQ(getGlobalNumber("result"), eNoSuchNotepad);
}

// Test 13: NotepadFont sets font successfully
TEST_F(NotepadApiTest, NotepadFontSetsFont)
{
    executeLua("world.SendToNotepad('FontTest', 'content')");
    executeLua("result = world.NotepadFont('FontTest', 'Courier New', 14, 1, 0)");
    EXPECT_EQ(getGlobalNumber("result"), eOK);

    NotepadWidget* notepad = doc->FindNotepad("FontTest");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_strFontName, "Courier New");
    EXPECT_EQ(notepad->m_iFontSize, 14);
}

// Test 14: NotepadColour returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadColourFailsIfNotExists)
{
    executeLua("result = world.NotepadColour('NoSuchNotepad', 'white', 'black')");
    EXPECT_EQ(getGlobalNumber("result"), eNoSuchNotepad);
}

// Test 15: NotepadColour sets colors successfully
TEST_F(NotepadApiTest, NotepadColourSetsColors)
{
    executeLua("world.SendToNotepad('ColorTest', 'content')");
    executeLua("result = world.NotepadColour('ColorTest', '#FFFFFF', '#000000')");
    EXPECT_EQ(getGlobalNumber("result"), eOK);

    NotepadWidget* notepad = doc->FindNotepad("ColorTest");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_textColour, qRgb(255, 255, 255));
    EXPECT_EQ(notepad->m_backColour, qRgb(0, 0, 0));
}

// Test 16: NotepadColour returns error for invalid color
TEST_F(NotepadApiTest, NotepadColourFailsForInvalidColor)
{
    executeLua("world.SendToNotepad('InvalidColor', 'content')");
    executeLua("result = world.NotepadColour('InvalidColor', 'notacolor', 'black')");
    EXPECT_EQ(getGlobalNumber("result"), eInvalidColourName);
}

// Test 17: NotepadReadOnly returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadReadOnlyFailsIfNotExists)
{
    executeLua("result = world.NotepadReadOnly('NoSuchNotepad', true)");
    EXPECT_EQ(getGlobalNumber("result"), eNoSuchNotepad);
}

// Test 18: NotepadReadOnly sets read-only mode
TEST_F(NotepadApiTest, NotepadReadOnlySetsMode)
{
    executeLua("world.SendToNotepad('ReadOnly', 'content')");
    executeLua("result = world.NotepadReadOnly('ReadOnly', true)");
    EXPECT_EQ(getGlobalNumber("result"), eOK);

    NotepadWidget* notepad = doc->FindNotepad("ReadOnly");
    ASSERT_NE(notepad, nullptr);
    EXPECT_TRUE(notepad->m_bReadOnly);
}

// Test 19: NotepadSaveMethod returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadSaveMethodFailsIfNotExists)
{
    executeLua("result = world.NotepadSaveMethod('NoSuchNotepad', 1)");
    EXPECT_EQ(getGlobalNumber("result"), eNoSuchNotepad);
}

// Test 20: NotepadSaveMethod sets save method
TEST_F(NotepadApiTest, NotepadSaveMethodSetsSaveMethod)
{
    executeLua("world.SendToNotepad('SaveMethod', 'content')");
    executeLua("result = world.NotepadSaveMethod('SaveMethod', 2)");
    EXPECT_EQ(getGlobalNumber("result"), eOK);

    NotepadWidget* notepad = doc->FindNotepad("SaveMethod");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_iSaveOnChange, 2);
}

// Test 21: CloseNotepad returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, CloseNotepadFailsIfNotExists)
{
    executeLua("result = world.CloseNotepad('NoSuchNotepad', false)");
    EXPECT_EQ(getGlobalNumber("result"), eNoSuchNotepad);
}

// Test 22: CloseNotepad succeeds for existing notepad
TEST_F(NotepadApiTest, CloseNotepadSucceedsForExisting)
{
    executeLua("world.SendToNotepad('ToClose', 'content')");
    ASSERT_NE(doc->FindNotepad("ToClose"), nullptr);

    executeLua("result = world.CloseNotepad('ToClose', false)");
    EXPECT_EQ(getGlobalNumber("result"), eOK);
}

// Test 23: UTF-8 content handling
TEST_F(NotepadApiTest, HandlesUTF8Content)
{
    executeLua(R"(
        world.SendToNotepad('UTF8', 'Hello 世界 🌍')
        text = world.GetNotepadText('UTF8')
    )");
    EXPECT_EQ(getGlobalString("text"), "Hello 世界 🌍");
}

// Test 24: Case-insensitive notepad lookup
TEST_F(NotepadApiTest, CaseInsensitiveLookup)
{
    executeLua("world.SendToNotepad('MyNotepad', 'content')");
    executeLua("text1 = world.GetNotepadText('MyNotepad')");
    executeLua("text2 = world.GetNotepadText('mynotepad')");
    executeLua("text3 = world.GetNotepadText('MYNOTEPAD')");

    EXPECT_EQ(getGlobalString("text1"), "content");
    EXPECT_EQ(getGlobalString("text2"), "content");
    EXPECT_EQ(getGlobalString("text3"), "content");
}

// Test 25: Multiple notepads are independent
TEST_F(NotepadApiTest, MultipleNotepadsAreIndependent)
{
    executeLua("world.SendToNotepad('Notepad1', 'Content1')");
    executeLua("world.SendToNotepad('Notepad2', 'Content2')");
    executeLua("world.AppendToNotepad('Notepad1', ' Appended')");

    executeLua("text1 = world.GetNotepadText('Notepad1')");
    executeLua("text2 = world.GetNotepadText('Notepad2')");

    EXPECT_EQ(getGlobalString("text1"), "Content1 Appended");
    EXPECT_EQ(getGlobalString("text2"), "Content2");
}

// Test 26: MoveNotepadWindow fails if notepad not found
TEST_F(NotepadApiTest, MoveNotepadWindowFailsIfNotExists)
{
    executeLua("result = world.MoveNotepadWindow('NonExistent', 100, 100, 400, 300)");
    EXPECT_FALSE(getGlobalBool("result"));
}

// Test 27: MoveNotepadWindow returns false without MDI window in test environment
TEST_F(NotepadApiTest, MoveNotepadWindowFailsWithoutMDI)
{
    executeLua("world.SendToNotepad('MoveTest', 'content')");

    // Verify notepad was created
    NotepadWidget* notepad = doc->FindNotepad("MoveTest");
    ASSERT_NE(notepad, nullptr);

    // In test environment, there's no MainWindow so m_pMdiSubWindow is NULL
    // MoveNotepadWindow should return false
    executeLua("result = world.MoveNotepadWindow('MoveTest', 100, 150, 500, 400)");
    EXPECT_FALSE(getGlobalBool("result")); // Returns false without MDI window
}

// Test 28: GetNotepadWindowPosition returns nil for non-existent notepad
TEST_F(NotepadApiTest, GetNotepadWindowPositionReturnsNilIfNotExists)
{
    executeLua("pos = world.GetNotepadWindowPosition('NonExistent')");
    lua_getglobal(L, "pos");
    EXPECT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

// Test 29: GetNotepadWindowPosition returns empty without MDI window in test environment
TEST_F(NotepadApiTest, GetNotepadWindowPositionReturnsEmptyWithoutMDI)
{
    executeLua("world.SendToNotepad('PosTest', 'content')");
    executeLua("pos = world.GetNotepadWindowPosition('PosTest')");

    // In test environment, there's no MainWindow so m_pMdiSubWindow is NULL
    // GetNotepadWindowPosition returns empty string (converted to nil)
    lua_getglobal(L, "pos");
    EXPECT_TRUE(lua_isnil(L, -1)); // Empty QString becomes nil in Lua
    lua_pop(L, 1);
}

// Test 30: Functions work correctly when notepad exists
TEST_F(NotepadApiTest, WindowPositionFunctionsExist)
{
    // Verify the functions exist and can be called (even if they fail without MDI)
    executeLua("world.SendToNotepad('Test', 'content')");

    // These should not crash, just return false/nil
    executeLua("move_result = world.MoveNotepadWindow('Test', 100, 100, 400, 300)");
    executeLua("pos_result = world.GetNotepadWindowPosition('Test')");

    // Verify they returned appropriate failure values
    EXPECT_FALSE(getGlobalBool("move_result"));

    lua_getglobal(L, "pos_result");
    EXPECT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

// Main test runner
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
