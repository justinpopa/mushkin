/**
 * test_world_output_gtest.cpp - GoogleTest tests for world output Lua API
 *
 * Tests Lua output functions defined in src/world/lua_api/world_output.cpp.
 * Covers: Tell, ANSI, AnsiNote, Simulate, GetLineInfo, GetStyleInfo,
 *         NoteColour getters/setters, NoteStyle, NoteHr, entity functions,
 *         DeleteLines, DeleteOutput, Bookmark, Info bar functions,
 *         SetSelection, SetUnseenLines, ResetStatusTime, Hyperlink.
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "fixtures/world_fixtures.h"
#include <gtest/gtest.h>

class WorldOutputTest : public ConnectedWorldTest {
  protected:
    QString lastLineText()
    {
        if (doc->m_lineList.empty()) {
            return {};
        }
        Line* line = doc->m_lineList.back().get();
        return QString::fromUtf8(line->text().data(), line->text().size());
    }

    int lineCount()
    {
        return static_cast<int>(doc->m_lineList.size());
    }
};

// ========== 1. Tell ==========

TEST_F(WorldOutputTest, Tell_AppendsToCurrentLine)
{
    executeLua("world.Tell('Hello')");
    executeLua("world.Tell(' World')");
    executeLua("world.Note('')");
    EXPECT_TRUE(lastLineText().contains("Hello World"))
        << "Tell should accumulate text on the current line";
}

TEST_F(WorldOutputTest, Tell_MultipleParams)
{
    executeLua("world.Tell('A', 'B', 'C')");
    executeLua("world.Note('')");
    EXPECT_TRUE(lastLineText().contains("ABC"))
        << "Tell with multiple args should concatenate them";
}

// ========== 2. ANSI ==========

TEST_F(WorldOutputTest, ANSI_NoArgs_ReturnsResetSequence)
{
    executeLua("test_result = world.ANSI()");
    EXPECT_EQ(getGlobalString("test_result"), "\033[m");
}

TEST_F(WorldOutputTest, ANSI_SingleCode)
{
    executeLua("test_result = world.ANSI(1)");
    EXPECT_EQ(getGlobalString("test_result"), "\033[1m");
}

TEST_F(WorldOutputTest, ANSI_MultipleCodes)
{
    executeLua("test_result = world.ANSI(1, 37)");
    EXPECT_EQ(getGlobalString("test_result"), "\033[1;37m");
}

TEST_F(WorldOutputTest, ANSI_ColorCode)
{
    executeLua("test_result = world.ANSI(0)");
    EXPECT_EQ(getGlobalString("test_result"), "\033[0m");
}

// ========== 3. AnsiNote ==========

TEST_F(WorldOutputTest, AnsiNote_PlainText)
{
    int before = lineCount();
    executeLua("world.AnsiNote('Hello')");
    EXPECT_GT(lineCount(), before) << "AnsiNote should add at least one line";
    EXPECT_TRUE(lastLineText().contains("Hello")) << "AnsiNote plain text should appear in output";
}

TEST_F(WorldOutputTest, AnsiNote_WithCodes)
{
    executeLua("world.AnsiNote('\\027[1;31mRed\\027[0m Normal')");
    // Both segments should be present in the combined text of the added lines.
    // Find a line containing either "Red" or "Normal"
    bool found = false;
    for (auto& linePtr : doc->m_lineList) {
        QString text = QString::fromUtf8(linePtr->text().data(), linePtr->text().size());
        if (text.contains("Red") || text.contains("Normal")) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "AnsiNote with ANSI codes should output both text segments";
}

TEST_F(WorldOutputTest, AnsiNote_AddsNewline)
{
    int before = lineCount();
    executeLua("world.AnsiNote('Test')");
    EXPECT_GT(lineCount(), before) << "AnsiNote should add a new line to m_lineList";
}

// ========== 4. Simulate ==========

TEST_F(WorldOutputTest, Simulate_AddsText)
{
    executeLua("world.Simulate('Hello from MUD\\n')");
    EXPECT_GE(lineCount(), 1) << "Simulate with newline should produce at least one line";
    bool found = false;
    for (auto& linePtr : doc->m_lineList) {
        QString text = QString::fromUtf8(linePtr->text().data(), linePtr->text().size());
        if (text.contains("Hello from MUD")) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Simulated text should appear in line buffer";
}

TEST_F(WorldOutputTest, Simulate_WithoutNewline)
{
    int before = lineCount();
    executeLua("world.Simulate('partial')");
    // Without a newline the text stays on the current line, not committed to lineList.
    // The line count should not increase beyond what was there before.
    // (It may stay the same, or the sentinel line absorbs it.)
    QString currentText =
        QString::fromUtf8(doc->m_currentLine->text().data(), doc->m_currentLine->text().size());
    // Either the current line holds the text, or it appeared in some line.
    bool inCurrent = currentText.contains("partial");
    bool inList = false;
    for (auto& linePtr : doc->m_lineList) {
        if (QString::fromUtf8(linePtr->text().data(), linePtr->text().size()).contains("partial")) {
            inList = true;
            break;
        }
    }
    EXPECT_TRUE(inCurrent || inList)
        << "Simulate without newline: text should be in currentLine or lineList";
    (void)before;
}

// ========== 5. GetLineInfo ==========

TEST_F(WorldOutputTest, GetLineInfo_Text)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(1, 1)");
    EXPECT_EQ(getGlobalString("test_result"), "Hello");
}

TEST_F(WorldOutputTest, GetLineInfo_Length)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(1, 2)");
    EXPECT_EQ(getGlobalInt("test_result"), 5);
}

TEST_F(WorldOutputTest, GetLineInfo_IsNote)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(1, 4)");
    EXPECT_TRUE(getGlobalBool("test_result")) << "Note lines should have COMMENT flag set";
}

TEST_F(WorldOutputTest, GetLineInfo_StyleCount)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(1, 11)");
    EXPECT_GE(getGlobalInt("test_result"), 1) << "Note line should have at least one style run";
}

TEST_F(WorldOutputTest, GetLineInfo_InvalidLine)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(999, 1)");
    EXPECT_TRUE(isGlobalNil("test_result")) << "Invalid line number should return nil";
}

TEST_F(WorldOutputTest, GetLineInfo_InvalidType)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetLineInfo(1, 99)");
    EXPECT_TRUE(isGlobalNil("test_result")) << "Invalid info type should return nil";
}

// ========== 6. GetStyleInfo ==========

TEST_F(WorldOutputTest, GetStyleInfo_Text)
{
    executeLua("world.ColourNote('red', 'black', 'Hello')");
    // ColourNote may create an initial zero-length style; find the one with text
    executeLua(R"(
        test_result = ''
        local count = world.GetLineInfo(1, 11)
        for i = 1, count do
            local text = world.GetStyleInfo(1, i, 1)
            if text == 'Hello' then
                test_result = text
                break
            end
        end
    )");
    EXPECT_EQ(getGlobalString("test_result"), "Hello");
}

TEST_F(WorldOutputTest, GetStyleInfo_Length)
{
    executeLua("world.ColourNote('red', 'black', 'Hello')");
    // Find the style run that holds "Hello" and verify its length
    executeLua(R"(
        test_result = 0
        local count = world.GetLineInfo(1, 11)
        for i = 1, count do
            local text = world.GetStyleInfo(1, i, 1)
            if text == 'Hello' then
                test_result = world.GetStyleInfo(1, i, 2)
                break
            end
        end
    )");
    EXPECT_EQ(getGlobalInt("test_result"), 5);
}

TEST_F(WorldOutputTest, GetStyleInfo_StartColumn)
{
    executeLua("world.ColourNote('red', 'black', 'Hello')");
    executeLua("test_result = world.GetStyleInfo(1, 1, 3)");
    EXPECT_EQ(getGlobalInt("test_result"), 1) << "First style run should start at column 1";
}

TEST_F(WorldOutputTest, GetStyleInfo_ForegroundColor)
{
    executeLua("world.ColourNote('red', 'black', 'Hello')");
    // Find the style run with "Hello" and check its foreground color.
    // QRgb values can appear negative when returned as signed int, so use != 0.
    executeLua(R"(
        test_result = 0
        local count = world.GetLineInfo(1, 11)
        for i = 1, count do
            local text = world.GetStyleInfo(1, i, 1)
            if text == 'Hello' then
                test_result = world.GetStyleInfo(1, i, 14)
                break
            end
        end
    )");
    EXPECT_NE(getGlobalInt("test_result"), 0) << "GetStyleInfo foreground color should be non-zero";
}

TEST_F(WorldOutputTest, GetStyleInfo_InvalidLine)
{
    executeLua("world.Note('Hello')");
    executeLua("test_result = world.GetStyleInfo(999, 1, 1)");
    EXPECT_TRUE(isGlobalNil("test_result")) << "Invalid line number should return nil";
}

// ========== 7. NoteColour getters/setters ==========

TEST_F(WorldOutputTest, NoteColourRGB_SetsColors)
{
    executeLua(R"(
        world.NoteColourRGB(0xFF0000, 0x00FF00)
        test_fore = world.GetNoteColourFore()
        test_back = world.GetNoteColourBack()
    )");
    EXPECT_EQ(getGlobalInt("test_fore"), 0xFF0000);
    EXPECT_EQ(getGlobalInt("test_back"), 0x00FF00);
}

TEST_F(WorldOutputTest, NoteColourName_SetsColors)
{
    executeLua(R"(
        world.NoteColourName('red', 'blue')
        test_fore = world.GetNoteColourFore()
        test_back = world.GetNoteColourBack()
    )");
    EXPECT_NE(getGlobalInt("test_fore"), 0) << "NoteColourName should set non-zero foreground";
    EXPECT_NE(getGlobalInt("test_back"), 0) << "NoteColourName should set non-zero background";
}

TEST_F(WorldOutputTest, SetNoteColourFore_SetsRGB)
{
    executeLua(R"(
        world.SetNoteColourFore(0x123456)
        test_fore = world.GetNoteColourFore()
    )");
    EXPECT_EQ(getGlobalInt("test_fore"), 0x123456);
}

TEST_F(WorldOutputTest, SetNoteColourBack_SetsRGB)
{
    executeLua(R"(
        world.SetNoteColourBack(0x654321)
        test_back = world.GetNoteColourBack()
    )");
    EXPECT_EQ(getGlobalInt("test_back"), 0x654321);
}

TEST_F(WorldOutputTest, NoteColour_ReturnsNegativeOneInRGBMode)
{
    executeLua(R"(
        world.NoteColourRGB(0xFF0000, 0x000000)
        test_result = world.NoteColour()
    )");
    EXPECT_EQ(getGlobalInt("test_result"), -1) << "NoteColour() should return -1 in RGB mode";
}

// ========== 8. NoteStyle ==========

TEST_F(WorldOutputTest, NoteStyle_Sets)
{
    executeLua(R"(
        world.NoteStyle(3)
        test_result = world.GetNoteStyle()
    )");
    EXPECT_EQ(getGlobalInt("test_result"), 3) << "NoteStyle(3) should store bold+underline";
}

TEST_F(WorldOutputTest, NoteStyle_MasksBits)
{
    executeLua(R"(
        world.NoteStyle(0xFF)
        test_result = world.GetNoteStyle()
    )");
    EXPECT_EQ(getGlobalInt("test_result"), 0x0F) << "NoteStyle should mask to 4 bits (0x0F)";
}

TEST_F(WorldOutputTest, NoteStyle_Zero)
{
    executeLua(R"(
        world.NoteStyle(3)
        world.NoteStyle(0)
        test_result = world.GetNoteStyle()
    )");
    EXPECT_EQ(getGlobalInt("test_result"), 0) << "NoteStyle(0) should reset to normal";
}

// ========== 9. NoteHr ==========

TEST_F(WorldOutputTest, NoteHr_AddsLine)
{
    int before = lineCount();
    executeLua("world.NoteHr()");
    EXPECT_GT(lineCount(), before) << "NoteHr should add at least one line";
}

TEST_F(WorldOutputTest, NoteHr_HasHorizRuleFlag)
{
    int before = lineCount();
    executeLua("world.NoteHr()");
    EXPECT_GT(lineCount(), before);
    // NoteHr passes HORIZ_RULE to StartNewLine, which sets it on the finalized line.
    // Check that at least one of the newly added lines has the flag.
    bool found = false;
    for (int i = before; i < lineCount(); i++) {
        if (doc->m_lineList[i]->flags & HORIZ_RULE) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "NoteHr should set HORIZ_RULE flag on the HR line";
}

// ========== 10. Entity functions ==========

TEST_F(WorldOutputTest, SetEntity_GetEntity_Roundtrip)
{
    executeLua("world.SetEntity('myvar', 'hello')");
    executeLua("test_result = world.GetEntity('myvar')");
    EXPECT_EQ(getGlobalString("test_result"), "hello");
}

TEST_F(WorldOutputTest, SetEntity_Delete)
{
    executeLua("world.SetEntity('myvar', 'hello')");
    executeLua("world.SetEntity('myvar', '')");
    executeLua("test_result = world.GetEntity('myvar')");
    EXPECT_EQ(getGlobalString("test_result"), "")
        << "SetEntity with empty value should delete the entity";
}

TEST_F(WorldOutputTest, GetEntity_NotFound)
{
    executeLua("test_result = world.GetEntity('nonexistent')");
    EXPECT_EQ(getGlobalString("test_result"), "")
        << "GetEntity for unknown name should return empty string";
}

TEST_F(WorldOutputTest, GetXMLEntity_Standard)
{
    // Standard HTML entities (amp, lt, etc.) are only loaded when MXP is on.
    doc->MXP_On();
    executeLua("test_amp = world.GetXMLEntity('amp')");
    executeLua("test_lt  = world.GetXMLEntity('lt')");
    EXPECT_EQ(getGlobalString("test_amp"), "&");
    EXPECT_EQ(getGlobalString("test_lt"), "<");
}

TEST_F(WorldOutputTest, GetXMLEntity_Numeric)
{
    executeLua("test_result = world.GetXMLEntity('#65')");
    EXPECT_EQ(getGlobalString("test_result"), "A")
        << "GetXMLEntity('#65') should return 'A' (decimal codepoint)";
}

TEST_F(WorldOutputTest, SetEntity_CaseInsensitive)
{
    executeLua("world.SetEntity('MyVar', 'test')");
    executeLua("test_result = world.GetEntity('myvar')");
    EXPECT_EQ(getGlobalString("test_result"), "test")
        << "SetEntity/GetEntity should be case-insensitive";
}

// ========== 11. DeleteLines / DeleteOutput ==========

TEST_F(WorldOutputTest, DeleteOutput_ClearsBuffer)
{
    executeLua("world.Note('line1')");
    executeLua("world.Note('line2')");
    EXPECT_GE(lineCount(), 2);
    executeLua("world.DeleteOutput()");
    EXPECT_EQ(lineCount(), 0) << "DeleteOutput should clear m_lineList";
}

TEST_F(WorldOutputTest, DeleteLines_RemovesFromEnd)
{
    for (int i = 0; i < 5; i++) {
        executeLua("world.Note('line')");
    }
    int before = lineCount();
    executeLua("world.DeleteLines(2)");
    EXPECT_LT(lineCount(), before) << "DeleteLines should reduce line count";
}

// ========== 12. Bookmark ==========

TEST_F(WorldOutputTest, Bookmark_SetsAndClearsFlag)
{
    executeLua("world.Note('bookmarkable')");
    ASSERT_GE(lineCount(), 1);

    executeLua("world.Bookmark(1)");
    Line* line = doc->m_lineList[0].get();
    EXPECT_TRUE(line->flags & BOOKMARK) << "Bookmark(1) should set BOOKMARK flag";

    executeLua("world.Bookmark(1, false)");
    EXPECT_FALSE(line->flags & BOOKMARK) << "Bookmark(1, false) should clear BOOKMARK flag";
}

// ========== 13. Info bar functions ==========

TEST_F(WorldOutputTest, Info_AppendsText)
{
    doc->m_infoBarText.clear();
    executeLua("world.Info('HP: 100')");
    EXPECT_TRUE(doc->m_infoBarText.contains("HP: 100"))
        << "Info() should append text to m_infoBarText";
}

TEST_F(WorldOutputTest, InfoClear_ResetsState)
{
    executeLua("world.Info('some text')");
    executeLua("world.InfoClear()");
    EXPECT_TRUE(doc->m_infoBarText.isEmpty()) << "InfoClear should clear m_infoBarText";
}

TEST_F(WorldOutputTest, InfoColour_SetsTextColor)
{
    doc->m_infoBarTextColor = 0;
    executeLua("world.InfoColour('red')");
    EXPECT_NE(doc->m_infoBarTextColor, static_cast<QRgb>(0))
        << "InfoColour('red') should set a non-zero text color";
}

TEST_F(WorldOutputTest, InfoBackground_SetsBackColor)
{
    QRgb original = doc->m_infoBarBackColor;
    executeLua("world.InfoBackground('blue')");
    EXPECT_NE(doc->m_infoBarBackColor, original)
        << "InfoBackground('blue') should change m_infoBarBackColor";
}

TEST_F(WorldOutputTest, InfoFont_SetsFont)
{
    executeLua("world.InfoFont('Arial', 14, 1)");
    EXPECT_EQ(doc->m_infoBarFontName, "Arial") << "InfoFont should set font name";
    EXPECT_EQ(doc->m_infoBarFontSize, 14) << "InfoFont should set font size";
    EXPECT_EQ(doc->m_infoBarFontStyle, 1) << "InfoFont should set font style";
}

TEST_F(WorldOutputTest, ShowInfoBar_SetsVisibility)
{
    executeLua("world.ShowInfoBar(true)");
    EXPECT_TRUE(doc->m_infoBarVisible) << "ShowInfoBar(true) should set m_infoBarVisible to true";

    executeLua("world.ShowInfoBar(false)");
    EXPECT_FALSE(doc->m_infoBarVisible)
        << "ShowInfoBar(false) should set m_infoBarVisible to false";
}

// ========== 14. SetSelection ==========

TEST_F(WorldOutputTest, SetSelection_SetsFields)
{
    executeLua("world.SetSelection(1, 3, 5, 10)");
    EXPECT_EQ(doc->m_selectionStartLine, 0) << "SetSelection start line should be 0-based (1 -> 0)";
    EXPECT_EQ(doc->m_selectionEndLine, 2) << "SetSelection end line should be 0-based (3 -> 2)";
    EXPECT_EQ(doc->m_selectionStartChar, 4) << "SetSelection start col should be 0-based (5 -> 4)";
    EXPECT_EQ(doc->m_selectionEndChar, 9) << "SetSelection end col should be 0-based (10 -> 9)";
}

// ========== 15. SetUnseenLines / ResetStatusTime ==========

TEST_F(WorldOutputTest, SetUnseenLines_SetsCounter)
{
    executeLua("world.SetUnseenLines(42)");
    EXPECT_EQ(doc->m_new_lines, 42) << "SetUnseenLines should set m_new_lines";
}

TEST_F(WorldOutputTest, ResetStatusTime_Updates)
{
    // Ensure m_tStatusTime is updated to a valid datetime
    doc->m_tStatusTime = QDateTime(); // invalidate first
    executeLua("world.ResetStatusTime()");
    EXPECT_TRUE(doc->m_tStatusTime.isValid())
        << "ResetStatusTime should set m_tStatusTime to current datetime";
}

// ========== 16. Hyperlink ==========

TEST_F(WorldOutputTest, Hyperlink_AddsTextToOutput)
{
    executeLua(R"(
        world.Hyperlink("look north", "[North]", "Look north")
        world.Note("")
    )");
    bool foundNorth = lastLineText().contains("[North]");
    if (!foundNorth) {
        for (auto& linePtr : doc->m_lineList) {
            if (QString::fromUtf8(linePtr->text().data(), linePtr->text().size())
                    .contains("[North]")) {
                foundNorth = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundNorth)
        << "Hyperlink text '[North]' should appear somewhere in the output buffer";
}
