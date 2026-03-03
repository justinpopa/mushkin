/**
 * test_world_protocol_gtest.cpp - GoogleTest tests for world protocol functions
 *
 * Tests ProcessIncomingByte(), InterpretANSIcode(), Interpret256ANSIcode(),
 * GetStyleRGB(), RememberStyle(), SendPacket(), and the xterm 256-color palette
 * defined in src/world/world_protocol.cpp.
 */

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/color_utils.h"
#include "../src/world/telnet_parser.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <gtest/gtest.h>
#include <span>

// Palette defined in world_protocol.cpp
extern QRgb xterm_256_colours[256];

class WorldProtocolTest : public ConnectedWorldTest {
  protected:
    void feed(std::initializer_list<unsigned char> bytes)
    {
        for (unsigned char b : bytes)
            doc->ProcessIncomingByte(b);
    }

    Phase currentPhase()
    {
        return doc->m_telnetParser->m_phase;
    }

    QString lastLineText()
    {
        if (doc->m_lineList.empty())
            return {};
        Line* line = doc->m_lineList.back().get();
        return QString::fromUtf8(line->text().data(), line->text().size());
    }

    int lineCount()
    {
        return static_cast<int>(doc->m_lineList.size());
    }
};

// ========== Category 1: ProcessIncomingByte - Normal text routing ==========

TEST_F(WorldProtocolTest, PlainAscii_AppearsOnCurrentLine)
{
    feed({'H', 'e', 'l', 'l', 'o'});
    feed({'\n'});
    EXPECT_TRUE(lastLineText().contains("Hello"));
}

TEST_F(WorldProtocolTest, Newline_CreatesNewLineInLineList)
{
    int before = lineCount();
    feed({'A', '\n'});
    EXPECT_EQ(lineCount(), before + 1);
}

TEST_F(WorldProtocolTest, CarriageReturn_IsIgnored)
{
    int before = lineCount();
    feed({'\r'});
    EXPECT_EQ(lineCount(), before);
    EXPECT_EQ(currentPhase(), Phase::NONE);
}

TEST_F(WorldProtocolTest, Tab_IsAccepted)
{
    feed({'\t', '\n'});
    EXPECT_GE(lineCount(), 1);
}

// ========== Category 2: ProcessIncomingByte - Phase transitions ==========

TEST_F(WorldProtocolTest, ESC_SetsPhaseHaveEsc)
{
    feed({0x1B});
    EXPECT_EQ(currentPhase(), Phase::HAVE_ESC);
}

TEST_F(WorldProtocolTest, IAC_SetsPhaseHaveIac)
{
    feed({0xFF});
    EXPECT_EQ(currentPhase(), Phase::HAVE_IAC);
}

TEST_F(WorldProtocolTest, MXPOpen_WhenMXPEnabled_SetsPhaseHaveMxpElement)
{
    doc->MXP_On();
    feed({'<'});
    EXPECT_EQ(currentPhase(), Phase::HAVE_MXP_ELEMENT);
}

TEST_F(WorldProtocolTest, MXPEntity_WhenMXPEnabled_SetsPhaseHaveMxpEntity)
{
    doc->MXP_On();
    feed({'&'});
    EXPECT_EQ(currentPhase(), Phase::HAVE_MXP_ENTITY);
}

TEST_F(WorldProtocolTest, MXPOpen_WhenMXPDisabled_DoesNotChangePhase)
{
    // MXP is off by default — '<' is treated as normal text
    EXPECT_EQ(currentPhase(), Phase::NONE);
    feed({'<'});
    EXPECT_EQ(currentPhase(), Phase::NONE);
}

// ========== Category 3: ProcessIncomingByte - UTF-8 handling ==========

TEST_F(WorldProtocolTest, UTF8TwoByte_SetsPhaseAndBytesLeft1)
{
    // 0xC2 is a valid 2-byte UTF-8 lead byte (0xC0-0xDF)
    feed({0xC2});
    EXPECT_EQ(currentPhase(), Phase::HAVE_UTF8_CHARACTER);
    EXPECT_EQ(doc->m_iUTF8BytesLeft, 1);
}

TEST_F(WorldProtocolTest, UTF8ThreeByte_SetsPhaseAndBytesLeft2)
{
    // 0xE2 is a valid 3-byte UTF-8 lead byte (0xE0-0xEF)
    feed({0xE2});
    EXPECT_EQ(currentPhase(), Phase::HAVE_UTF8_CHARACTER);
    EXPECT_EQ(doc->m_iUTF8BytesLeft, 2);
}

TEST_F(WorldProtocolTest, UTF8FourByte_SetsPhaseAndBytesLeft3)
{
    // 0xF0 is a valid 4-byte UTF-8 lead byte (0xF0-0xF7)
    feed({0xF0});
    EXPECT_EQ(currentPhase(), Phase::HAVE_UTF8_CHARACTER);
    EXPECT_EQ(doc->m_iUTF8BytesLeft, 3);
}

TEST_F(WorldProtocolTest, UTF8InvalidHighByte_FallsThroughToNormal)
{
    // 0x80 is a continuation byte with no preceding lead — falls through, phase stays NONE
    feed({0x80});
    EXPECT_EQ(currentPhase(), Phase::NONE);
}

// ========== Category 4: InterpretANSIcode - Reset and style flags ==========

TEST_F(WorldProtocolTest, ANSI_Bold_SetsBoldFlag)
{
    feed({0x1B, '[', '1', 'm'});
    EXPECT_NE(doc->m_iFlags & HILITE, 0);
}

TEST_F(WorldProtocolTest, ANSI_Underline_SetsUnderlineFlag)
{
    feed({0x1B, '[', '4', 'm'});
    EXPECT_NE(doc->m_iFlags & UNDERLINE, 0);
}

TEST_F(WorldProtocolTest, ANSI_Blink_SetsBlinkFlag)
{
    // Code 3 (ANSI_BLINK — also used for italic)
    feed({0x1B, '[', '3', 'm'});
    EXPECT_NE(doc->m_iFlags & BLINK, 0);
}

TEST_F(WorldProtocolTest, ANSI_Inverse_SetsInverseFlag)
{
    feed({0x1B, '[', '7', 'm'});
    EXPECT_NE(doc->m_iFlags & INVERSE, 0);
}

TEST_F(WorldProtocolTest, ANSI_Strikeout_SetsStrikeoutFlag)
{
    feed({0x1B, '[', '9', 'm'});
    EXPECT_NE(doc->m_iFlags & STRIKEOUT, 0);
}

TEST_F(WorldProtocolTest, ANSI_Reset_ClearsAllStyleFlags)
{
    // Set some flags first
    feed({0x1B, '[', '1', 'm'}); // bold
    feed({0x1B, '[', '4', 'm'}); // underline
    EXPECT_NE(doc->m_iFlags & HILITE, 0);
    EXPECT_NE(doc->m_iFlags & UNDERLINE, 0);

    // Reset
    feed({0x1B, '[', '0', 'm'});
    EXPECT_EQ(doc->m_iFlags & HILITE, 0);
    EXPECT_EQ(doc->m_iFlags & UNDERLINE, 0);
    EXPECT_EQ(doc->m_iFlags & BLINK, 0);
    EXPECT_EQ(doc->m_iFlags & INVERSE, 0);
    EXPECT_EQ(doc->m_iFlags & STRIKEOUT, 0);
}

// ========== Category 5: InterpretANSIcode - Cancel codes ==========

TEST_F(WorldProtocolTest, ANSI_CancelBold_ClearsHilite)
{
    feed({0x1B, '[', '1', 'm'}); // set bold
    EXPECT_NE(doc->m_iFlags & HILITE, 0);
    feed({0x1B, '[', '2', '2', 'm'}); // cancel bold
    EXPECT_EQ(doc->m_iFlags & HILITE, 0);
}

TEST_F(WorldProtocolTest, ANSI_CancelUnderline_ClearsUnderline)
{
    feed({0x1B, '[', '4', 'm'}); // set underline
    EXPECT_NE(doc->m_iFlags & UNDERLINE, 0);
    feed({0x1B, '[', '2', '4', 'm'}); // cancel underline
    EXPECT_EQ(doc->m_iFlags & UNDERLINE, 0);
}

TEST_F(WorldProtocolTest, ANSI_CancelInverse_ClearsInverse)
{
    feed({0x1B, '[', '7', 'm'}); // set inverse
    EXPECT_NE(doc->m_iFlags & INVERSE, 0);
    feed({0x1B, '[', '2', '7', 'm'}); // cancel inverse
    EXPECT_EQ(doc->m_iFlags & INVERSE, 0);
}

TEST_F(WorldProtocolTest, ANSI_CancelStrikeout_ClearsStrikeout)
{
    feed({0x1B, '[', '9', 'm'}); // set strikeout
    EXPECT_NE(doc->m_iFlags & STRIKEOUT, 0);
    feed({0x1B, '[', '2', '9', 'm'}); // cancel strikeout
    EXPECT_EQ(doc->m_iFlags & STRIKEOUT, 0);
}

// ========== Category 6: InterpretANSIcode - Foreground colors ANSI mode ==========
// The fixture starts in COLOUR_RGB mode. We need to reset to ANSI mode first
// by starting from a fresh state. The ConnectedWorldTest fixture sets COLOUR_RGB.
// However, InterpretANSIcode operates on the current color mode. In ANSI/default mode
// (COLOUR_ANSI = 0x0000), color codes store ANSI indices.
// We test starting from the fixture's COLOUR_RGB state — in that mode the code
// performs RGB lookup via the color tables, which is the live path the app uses.
// The ANSI index storage path (COLOUR_ANSI) is covered by resetting flags to 0.

TEST_F(WorldProtocolTest, ANSI_ForeRed_SetsANSIRedIndex)
{
    // Put doc into COLOUR_ANSI mode so color indices are stored
    doc->m_iFlags = COLOUR_ANSI;
    doc->m_iForeColour = ANSI_WHITE;
    doc->m_iBackColour = ANSI_BLACK;

    feed({0x1B, '[', '3', '1', 'm'}); // foreground red
    EXPECT_EQ(doc->m_iForeColour, static_cast<QRgb>(ANSI_RED));
}

TEST_F(WorldProtocolTest, ANSI_ForeWhite_SetsANSIWhiteIndex)
{
    doc->m_iFlags = COLOUR_ANSI;
    doc->m_iForeColour = ANSI_BLACK;
    doc->m_iBackColour = ANSI_BLACK;

    feed({0x1B, '[', '3', '7', 'm'}); // foreground white
    EXPECT_EQ(doc->m_iForeColour, static_cast<QRgb>(ANSI_WHITE));
}

TEST_F(WorldProtocolTest, ANSI_DefaultForeground_SetsANSIWhiteIndex)
{
    doc->m_iFlags = COLOUR_ANSI;
    doc->m_iForeColour = ANSI_RED;
    doc->m_iBackColour = ANSI_BLACK;

    feed({0x1B, '[', '3', '9', 'm'}); // default foreground
    EXPECT_EQ(doc->m_iForeColour, static_cast<QRgb>(ANSI_WHITE));
}

// ========== Category 7: InterpretANSIcode - Background colors ANSI mode ==========

TEST_F(WorldProtocolTest, ANSI_BackRed_SetsANSIRedIndex)
{
    doc->m_iFlags = COLOUR_ANSI;
    doc->m_iForeColour = ANSI_WHITE;
    doc->m_iBackColour = ANSI_BLACK;

    feed({0x1B, '[', '4', '1', 'm'}); // background red
    EXPECT_EQ(doc->m_iBackColour, static_cast<QRgb>(ANSI_RED));
}

TEST_F(WorldProtocolTest, ANSI_DefaultBackground_SetsANSIBlackIndex)
{
    doc->m_iFlags = COLOUR_ANSI;
    doc->m_iForeColour = ANSI_WHITE;
    doc->m_iBackColour = ANSI_RED;

    feed({0x1B, '[', '4', '9', 'm'}); // default background
    EXPECT_EQ(doc->m_iBackColour, static_cast<QRgb>(ANSI_BLACK));
}

// ========== Category 8: GetStyleRGB ==========

TEST_F(WorldProtocolTest, GetStyleRGB_ANSIMode_ConvertsIndexToRGB)
{
    Style s;
    s.iFlags = COLOUR_ANSI;
    s.iForeColour = ANSI_WHITE; // index 7
    s.iBackColour = ANSI_BLACK; // index 0

    QRgb fore{}, back{};
    doc->GetStyleRGB(&s, fore, back);

    EXPECT_EQ(fore, doc->m_colors.normal_colour[ANSI_WHITE]);
    EXPECT_EQ(back, doc->m_colors.normal_colour[ANSI_BLACK]);
}

TEST_F(WorldProtocolTest, GetStyleRGB_ANSIModeWithHilite_UsesBoldTable)
{
    Style s;
    s.iFlags = COLOUR_ANSI | HILITE;
    s.iForeColour = ANSI_RED; // index 1
    s.iBackColour = ANSI_BLACK;

    QRgb fore{}, back{};
    doc->GetStyleRGB(&s, fore, back);

    EXPECT_EQ(fore, doc->m_colors.bold_colour[ANSI_RED]);
    EXPECT_EQ(back, doc->m_colors.normal_colour[ANSI_BLACK]);
}

TEST_F(WorldProtocolTest, GetStyleRGB_RGBMode_PassesThroughUnchanged)
{
    Style s;
    s.iFlags = COLOUR_RGB;
    s.iForeColour = qRgb(100, 150, 200);
    s.iBackColour = qRgb(10, 20, 30);

    QRgb fore{}, back{};
    doc->GetStyleRGB(&s, fore, back);

    EXPECT_EQ(fore, qRgb(100, 150, 200));
    EXPECT_EQ(back, qRgb(10, 20, 30));
}

TEST_F(WorldProtocolTest, GetStyleRGB_CustomMode_UsesCustomTables)
{
    // Set a known value in the custom table
    doc->m_colors.custom_text[2] = qRgb(11, 22, 33);
    doc->m_colors.custom_back[3] = qRgb(44, 55, 66);

    Style s;
    s.iFlags = COLOUR_CUSTOM;
    s.iForeColour = 2; // index into custom_text
    s.iBackColour = 3; // index into custom_back

    QRgb fore{}, back{};
    doc->GetStyleRGB(&s, fore, back);

    EXPECT_EQ(fore, qRgb(11, 22, 33));
    EXPECT_EQ(back, qRgb(44, 55, 66));
}

TEST_F(WorldProtocolTest, GetStyleRGB_NullStyle_UsesCurrentDocStyle)
{
    doc->m_iFlags = COLOUR_RGB;
    doc->m_iForeColour = qRgb(1, 2, 3);
    doc->m_iBackColour = qRgb(4, 5, 6);

    QRgb fore{}, back{};
    doc->GetStyleRGB(nullptr, fore, back);

    EXPECT_EQ(fore, qRgb(1, 2, 3));
    EXPECT_EQ(back, qRgb(4, 5, 6));
}

// ========== Category 9: RememberStyle ==========

TEST_F(WorldProtocolTest, RememberStyle_CopiesStyleFields)
{
    Style s;
    s.iFlags = COLOUR_RGB | HILITE | UNDERLINE;
    s.iForeColour = qRgb(10, 20, 30);
    s.iBackColour = qRgb(40, 50, 60);
    s.pAction = nullptr;

    doc->RememberStyle(&s);

    EXPECT_EQ(doc->m_iFlags, static_cast<quint16>(s.iFlags & STYLE_BITS));
    EXPECT_EQ(doc->m_iForeColour, qRgb(10, 20, 30));
    EXPECT_EQ(doc->m_iBackColour, qRgb(40, 50, 60));
}

TEST_F(WorldProtocolTest, RememberStyle_Nullptr_IsNoOp)
{
    doc->m_iFlags = COLOUR_RGB | HILITE;
    doc->m_iForeColour = qRgb(1, 2, 3);
    doc->m_iBackColour = qRgb(4, 5, 6);

    doc->RememberStyle(nullptr);

    EXPECT_EQ(doc->m_iFlags, static_cast<quint16>(COLOUR_RGB | HILITE));
    EXPECT_EQ(doc->m_iForeColour, qRgb(1, 2, 3));
    EXPECT_EQ(doc->m_iBackColour, qRgb(4, 5, 6));
}

// ========== Category 10: xterm 256-color palette ==========

TEST_F(WorldProtocolTest, Xterm256_Index0_IsBlack)
{
    EXPECT_EQ(xterm_256_colours[0], BGR(0, 0, 0));
}

TEST_F(WorldProtocolTest, Xterm256_Index9_IsBrightRed)
{
    EXPECT_EQ(xterm_256_colours[9], BGR(255, 0, 0));
}

TEST_F(WorldProtocolTest, Xterm256_Index15_IsWhite)
{
    EXPECT_EQ(xterm_256_colours[15], BGR(255, 255, 255));
}

TEST_F(WorldProtocolTest, Xterm256_ColorCube_Index16)
{
    // Index 16 = r=0,g=0,b=0 in the 6x6x6 cube: values[0]=0
    EXPECT_EQ(xterm_256_colours[16], BGR(0, 0, 0));
}

TEST_F(WorldProtocolTest, Xterm256_ColorCube_MidIndex)
{
    // Index 16 + 1*36 + 2*6 + 3 = 16 + 36 + 12 + 3 = 67
    // r=values[1]=95, g=values[2]=135, b=values[3]=175
    EXPECT_EQ(xterm_256_colours[67], BGR(95, 135, 175));
}

TEST_F(WorldProtocolTest, Xterm256_Grayscale_Index232)
{
    // grey=0: value = 8 + 0*10 = 8
    EXPECT_EQ(xterm_256_colours[232], BGR(8, 8, 8));
}

TEST_F(WorldProtocolTest, Xterm256_Grayscale_Index255)
{
    // grey=23: value = 8 + 23*10 = 238
    EXPECT_EQ(xterm_256_colours[255], BGR(238, 238, 238));
}

// ========== Category 11: Interpret256ANSIcode - 256-color ==========

TEST_F(WorldProtocolTest, Xterm256_Foreground_BrightRed)
{
    // ESC[38;5;9m — 256-color foreground index 9 (bright red)
    feed({0x1B, '[', '3', '8', ';', '5', ';', '9', 'm'});
    EXPECT_EQ(doc->m_iForeColour, xterm_256_colours[9]);
    EXPECT_NE(doc->m_iFlags & COLOUR_RGB, 0);
}

TEST_F(WorldProtocolTest, Xterm256_Background_Blue)
{
    // ESC[48;5;12m — 256-color background index 12 (blue)
    feed({0x1B, '[', '4', '8', ';', '5', ';', '1', '2', 'm'});
    EXPECT_EQ(doc->m_iBackColour, xterm_256_colours[12]);
    EXPECT_NE(doc->m_iFlags & COLOUR_RGB, 0);
}

TEST_F(WorldProtocolTest, Xterm256_Foreground_PaletteIndex0)
{
    // ESC[38;5;0m — 256-color foreground index 0 (black)
    feed({0x1B, '[', '3', '8', ';', '5', ';', '0', 'm'});
    EXPECT_EQ(doc->m_iForeColour, xterm_256_colours[0]);
    EXPECT_NE(doc->m_iFlags & COLOUR_RGB, 0);
}

// ========== Category 12: Interpret256ANSIcode - 24-bit truecolor ==========

TEST_F(WorldProtocolTest, Truecolor_Foreground_RGB)
{
    // ESC[38;2;100;150;200m — 24-bit foreground (R=100, G=150, B=200)
    feed({0x1B, '[', '3', '8', ';', '2', ';', '1', '0', '0', ';', '1', '5', '0', ';', '2', '0', '0',
          'm'});
    EXPECT_EQ(doc->m_iForeColour, qRgb(100, 150, 200));
    EXPECT_NE(doc->m_iFlags & COLOUR_RGB, 0);
}

TEST_F(WorldProtocolTest, Truecolor_Background_RGB)
{
    // ESC[48;2;50;100;150m — 24-bit background (R=50, G=100, B=150)
    feed({0x1B, '[', '4', '8', ';', '2', ';', '5', '0', ';', '1', '0', '0', ';', '1', '5', '0',
          'm'});
    EXPECT_EQ(doc->m_iBackColour, qRgb(50, 100, 150));
    EXPECT_NE(doc->m_iFlags & COLOUR_RGB, 0);
}

// ========== Category 13: SendPacket null guard ==========

TEST_F(WorldProtocolTest, SendPacket_EmptySpan_DoesNotCrash)
{
    // SendPacket with an empty span should not crash regardless of socket state.
    EXPECT_NO_FATAL_FAILURE(doc->SendPacket(std::span<const unsigned char>{}));
}

TEST_F(WorldProtocolTest, SendPacket_WithData_DoesNotCrash)
{
    // SendPacket with data to a non-connected socket should not crash.
    const unsigned char data[] = {0xFF, 0xFB, 0x01};
    EXPECT_NO_FATAL_FAILURE(doc->SendPacket(std::span<const unsigned char>{data, 3}));
}
