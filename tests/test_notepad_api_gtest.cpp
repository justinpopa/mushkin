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

#include "../src/world/notepad_widget.h"
#include "fixtures/world_fixtures.h"

#include "../src/utils/error_codes.h"
#include <QDir>
#include <QFile>
#include <QTextStream>

// Test fixture for Notepad API tests
class NotepadApiTest : public ConnectedWorldTest {};

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
TEST_F(NotepadApiTest, SendToNotepadAlwaysCreatesNew)
{
    // Original always creates a new notepad — does NOT replace existing.
    // Two notepads with the same title can coexist.
    executeLua("world.SendToNotepad('Test', 'Original')");
    executeLua("result = world.SendToNotepad('Test', 'Second')");
    EXPECT_TRUE(getGlobalBool("result"));

    // FindNotepad returns the first match — "Original" should still be there
    NotepadWidget* notepad = doc->FindNotepad("Test");
    ASSERT_NE(notepad, nullptr);
    // The first notepad keeps "Original"; the second has "Second"
    // FindNotepad returns the first one found
    EXPECT_EQ(notepad->GetText(), "Original");
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

// Test 5: ReplaceNotepad creates new notepad if not found (matches original)
// Original returns nil (pushes boolean but return 0 — Lua sees nil)
TEST_F(NotepadApiTest, ReplaceNotepadCreatesIfNotExists)
{
    executeLua("result = world.ReplaceNotepad('NonExistent', 'text')");
    // Original returns nil, not a boolean
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

    NotepadWidget* notepad = doc->FindNotepad("NonExistent");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "text");
}

// Test 6: ReplaceNotepad replaces existing notepad
// Original returns nil (pushes boolean but return 0 — Lua sees nil)
TEST_F(NotepadApiTest, ReplaceNotepadReplacesExisting)
{
    executeLua("world.SendToNotepad('Replace', 'Original')");
    executeLua("result = world.ReplaceNotepad('Replace', 'New Content')");
    // Original returns nil, not a boolean
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

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
TEST_F(NotepadApiTest, GetNotepadTextReturnsEmptyIfNotExists)
{
    // Original returns empty string "" (CString initialized empty), not nil
    executeLua("text = world.GetNotepadText('DoesNotExist')");
    lua_getglobal(L, "text");
    EXPECT_TRUE(lua_isstring(L, -1));
    EXPECT_STREQ(lua_tostring(L, -1), "");
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
    // Original returns numeric error code (eNoSuchNotepad = 30075)
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 13: NotepadFont sets font successfully
TEST_F(NotepadApiTest, NotepadFontSetsFont)
{
    executeLua("world.SendToNotepad('FontTest', 'content')");
    executeLua("result = world.NotepadFont('FontTest', 'Courier New', 14, 1, 0)");
    // Original returns eOK (0) on success
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);

    NotepadWidget* notepad = doc->FindNotepad("FontTest");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_strFontName, "Courier New");
    EXPECT_EQ(notepad->m_iFontSize, 14);
}

// Test 14: NotepadColour returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadColourFailsIfNotExists)
{
    executeLua("result = world.NotepadColour('NoSuchNotepad', 'white', 'black')");
    // Original returns numeric error code (eNoSuchNotepad = 30075)
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 15: NotepadColour sets colors successfully
TEST_F(NotepadApiTest, NotepadColourSetsColors)
{
    executeLua("world.SendToNotepad('ColorTest', 'content')");
    executeLua("result = world.NotepadColour('ColorTest', '#FFFFFF', '#000000')");
    // Original returns eOK (0) on success
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);

    NotepadWidget* notepad = doc->FindNotepad("ColorTest");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_textColour, qRgb(255, 255, 255));
    EXPECT_EQ(notepad->m_backColour, qRgb(0, 0, 0));
}

// Test 16: NotepadColour returns error for invalid color
TEST_F(NotepadApiTest, NotepadColourFailsForInvalidColor)
{
    // Original returns numeric eInvalidColourName (30077) for bad color name
    executeLua("world.SendToNotepad('InvalidColor', 'content')");
    executeLua("result = world.NotepadColour('InvalidColor', 'notacolor', 'black')");
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eInvalidColourName);
}

// Test 17: NotepadReadOnly returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadReadOnlyFailsIfNotExists)
{
    executeLua("result = world.NotepadReadOnly('NoSuchNotepad', true)");
    // Original returns numeric error code (eNoSuchNotepad = 30075)
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 18: NotepadReadOnly sets read-only mode
TEST_F(NotepadApiTest, NotepadReadOnlySetsMode)
{
    executeLua("world.SendToNotepad('ReadOnly', 'content')");
    executeLua("result = world.NotepadReadOnly('ReadOnly', true)");
    // Original returns eOK (0) on success
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);

    NotepadWidget* notepad = doc->FindNotepad("ReadOnly");
    ASSERT_NE(notepad, nullptr);
    EXPECT_TRUE(notepad->m_bReadOnly);
}

// Test 17b: NotepadReadOnly — nil arg defaults to true (make read-only)
TEST_F(NotepadApiTest, NotepadReadOnlyNilDefaultsToTrue)
{
    executeLua("world.SendToNotepad('RONil', 'content')");
    // Omitting the second arg: original optboolean(L,2,1) defaults to true
    executeLua("result = world.NotepadReadOnly('RONil')");
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);
    NotepadWidget* notepad = doc->FindNotepad("RONil");
    ASSERT_NE(notepad, nullptr);
    EXPECT_TRUE(notepad->m_bReadOnly);
}

// Test 17c: NotepadReadOnly — numeric arg coerced to boolean
TEST_F(NotepadApiTest, NotepadReadOnlyNumericArgCoerced)
{
    executeLua("world.SendToNotepad('RONum', 'content')");
    // Passing 0 (numeric) should coerce to false
    executeLua("result = world.NotepadReadOnly('RONum', 0)");
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);
    NotepadWidget* notepad = doc->FindNotepad("RONum");
    ASSERT_NE(notepad, nullptr);
    EXPECT_FALSE(notepad->m_bReadOnly);
}

// Test 19: NotepadSaveMethod returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, NotepadSaveMethodFailsIfNotExists)
{
    executeLua("result = world.NotepadSaveMethod('NoSuchNotepad', 1)");
    // Original returns numeric error code (eNoSuchNotepad = 30075)
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 20: NotepadSaveMethod sets save method
TEST_F(NotepadApiTest, NotepadSaveMethodSetsSaveMethod)
{
    executeLua("world.SendToNotepad('SaveMethod', 'content')");
    executeLua("result = world.NotepadSaveMethod('SaveMethod', 2)");
    // Original returns eOK (0) on success
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);

    NotepadWidget* notepad = doc->FindNotepad("SaveMethod");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->m_iSaveOnChange, 2);
}

// Test 21: CloseNotepad returns eNoSuchNotepad if not found
TEST_F(NotepadApiTest, CloseNotepadFailsIfNotExists)
{
    executeLua("result = world.CloseNotepad('NoSuchNotepad', false)");
    // Original returns numeric error code (eNoSuchNotepad = 30075)
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 22: CloseNotepad returns eOK for existing notepad
TEST_F(NotepadApiTest, CloseNotepadSucceedsForExisting)
{
    executeLua("world.SendToNotepad('ToClose', 'content')");
    ASSERT_NE(doc->FindNotepad("ToClose"), nullptr);

    executeLua("result = world.CloseNotepad('ToClose', false)");
    // Original returns eOK (0) on success
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eOK);
}

// Test 22b: SaveNotepad numeric replaceExisting arg coerced to bool
TEST_F(NotepadApiTest, SaveNotepadNumericReplaceArgCoerced)
{
    // Passing 1 (numeric) for replaceExisting should coerce to true (original optboolean behavior)
    // We just verify the call doesn't error — actual file I/O not tested here
    executeLua("result = world.SaveNotepad('DoesNotExist', '/tmp/test_notepad.txt', 1)");
    // Should return a numeric error code (eNoSuchNotepad = 30075), not crash
    EXPECT_EQ(static_cast<int>(getGlobalNumber("result")), eNoSuchNotepad);
}

// Test 22c: SendToNotepad with boolean arg — concatLuaArgs uses tostring
TEST_F(NotepadApiTest, SendToNotepadToStringCoercesBoolean)
{
    // Original concatArgs calls Lua's tostring, so boolean arg becomes "true"/"false"
    executeLua("world.SendToNotepad('BoolArg', true)");
    NotepadWidget* notepad = doc->FindNotepad("BoolArg");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "true");
}

// Test 22d: AppendToNotepad with nil arg — tostring gives "nil"
TEST_F(NotepadApiTest, AppendToNotepadToStringCoercesNil)
{
    executeLua("world.SendToNotepad('NilArg', nil)");
    NotepadWidget* notepad = doc->FindNotepad("NilArg");
    ASSERT_NE(notepad, nullptr);
    EXPECT_EQ(notepad->GetText(), "nil");
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

// Test 31: SaveToFile writes content to disk (M79 — notepad has save-to-file capability)
TEST_F(NotepadApiTest, SaveToFileWritesContent)
{
    executeLua(R"(world.SendToNotepad('SaveTest', "line one\nline two\n"))");
    NotepadWidget* notepad = doc->FindNotepad("SaveTest");
    ASSERT_NE(notepad, nullptr);

    // Write to a temp file
    const QString tmpPath = QDir::temp().filePath("mushkin_notepad_save_test.txt");
    EXPECT_TRUE(notepad->SaveToFile(tmpPath, /*replaceExisting=*/true));

    // Verify the file was written with correct content
    QFile file(tmpPath);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString written = QTextStream(&file).readAll();
    file.close();
    EXPECT_EQ(written, "line one\nline two\n");

    // After save the QTextDocument should report not-modified
    EXPECT_FALSE(notepad->m_pTextEdit->document()->isModified());

    // Filename is persisted on the widget
    EXPECT_EQ(notepad->m_strFilename, tmpPath);

    QFile::remove(tmpPath);
}

// Test 32: SaveToFile respects replaceExisting=false
TEST_F(NotepadApiTest, SaveToFileRespectsNoReplace)
{
    executeLua("world.SendToNotepad('SaveGuard', 'content')");
    NotepadWidget* notepad = doc->FindNotepad("SaveGuard");
    ASSERT_NE(notepad, nullptr);

    const QString tmpPath = QDir::temp().filePath("mushkin_notepad_guard_test.txt");

    // First write creates the file
    ASSERT_TRUE(notepad->SaveToFile(tmpPath, /*replaceExisting=*/true));

    // Second write with replaceExisting=false should refuse
    EXPECT_FALSE(notepad->SaveToFile(tmpPath, /*replaceExisting=*/false));

    QFile::remove(tmpPath);
// Test 31: CloseNotepad with querySave=false ignores m_iSaveOnChange and closes immediately
// (no prompt regardless of save method)
TEST_F(NotepadApiTest, CloseNotepadNoQuerySaveIgnoresSaveMethod)
{
    executeLua("world.SendToNotepad('SaveMethodNever', 'content')");
    NotepadWidget* notepad = doc->FindNotepad("SaveMethodNever");
    ASSERT_NE(notepad, nullptr);
    notepad->m_iSaveOnChange = eNotepadSaveNever;

    // querySave=false — should close without prompt regardless of save method
    executeLua("result = world.CloseNotepad('SaveMethodNever', false)");
    EXPECT_TRUE(getGlobalBool("result"));
    EXPECT_EQ(doc->FindNotepad("SaveMethodNever"), nullptr);
}

// Test 32: CloseNotepad with eNotepadSaveNever skips the save prompt even when
// querySave=true and the document is modified (original: TextDocument.cpp:476).
// In test environment there's no QMessageBox so we verify by direct doc access.
TEST_F(NotepadApiTest, CloseNotepadSaveMethodNeverSkipsPrompt)
{
    executeLua("world.SendToNotepad('NeverSave', 'content')");
    NotepadWidget* notepad = doc->FindNotepad("NeverSave");
    ASSERT_NE(notepad, nullptr);
    // Mark as modified so the default path would normally prompt
    if (notepad->m_pTextEdit) {
        notepad->m_pTextEdit->document()->setModified(true);
    }
    notepad->m_iSaveOnChange = eNotepadSaveNever;

    // querySave=true but eNotepadSaveNever — must close without prompt (returns true)
    qint32 result = doc->CloseNotepad("NeverSave", true);
    EXPECT_EQ(result, eOK);
    EXPECT_EQ(doc->FindNotepad("NeverSave"), nullptr);
}

// Test 33: CloseNotepad with eNotepadSaveAlways also skips the interactive prompt
// (original: TextDocument.cpp:475 — "fall through to save if changed", but in
// Mushkin there is no file-backed save so it just closes without prompting).
TEST_F(NotepadApiTest, CloseNotepadSaveMethodAlwaysSkipsPrompt)
{
    executeLua("world.SendToNotepad('AlwaysSave', 'content')");
    NotepadWidget* notepad = doc->FindNotepad("AlwaysSave");
    ASSERT_NE(notepad, nullptr);
    if (notepad->m_pTextEdit) {
        notepad->m_pTextEdit->document()->setModified(true);
    }
    notepad->m_iSaveOnChange = eNotepadSaveAlways;

    qint32 result = doc->CloseNotepad("AlwaysSave", true);
    EXPECT_EQ(result, eOK);
    EXPECT_EQ(doc->FindNotepad("AlwaysSave"), nullptr);
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
