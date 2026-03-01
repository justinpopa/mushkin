// test_ansi_formatter_gtest.cpp
// Unit tests for AnsiFormatter — ANSI escape sequence generation from Line/Style objects.
//
// Coverage: null/empty lines, plain text, text attributes (bold/underline/italic/inverse/
// strikeout), ANSI color indices (standard/bright/256-color), RGB truecolor, custom
// palette colors, multi-style lines, state caching, and reset behavior.

#include "../src/network/ansi_formatter.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QByteArray>
#include <QRgb>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class AnsiFormatterTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        formatter = std::make_unique<AnsiFormatter>(doc.get());
    }

    // Build a Line whose textBuffer contains `text` (plus null terminator) and
    // whose styleList is replaced by the provided styles.
    static std::unique_ptr<Line> makeLine(const std::string& text,
                                          std::vector<std::unique_ptr<Style>> styles)
    {
        auto line = std::make_unique<Line>(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);
        line->textBuffer.assign(text.begin(), text.end());
        line->textBuffer.push_back('\0');
        for (auto& s : styles) {
            line->styleList.push_back(std::move(s));
        }
        return line;
    }

    // Build a Style with common attributes. `fore` and `back` are ANSI color indices
    // (0-255) unless COLOUR_RGB or COLOUR_CUSTOM is encoded into `flags`.
    static std::unique_ptr<Style> makeStyle(quint16 length, quint16 flags = 0, QRgb fore = WHITE,
                                            QRgb back = BLACK)
    {
        auto style = std::make_unique<Style>();
        style->iLength = length;
        style->iFlags = flags;
        style->iForeColour = fore;
        style->iBackColour = back;
        style->pAction = nullptr;
        return style;
    }

    std::unique_ptr<AnsiFormatter> formatter;
};

// ---------------------------------------------------------------------------
// 1. Null / empty handling
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_NullLine_ReturnsNewline)
{
    EXPECT_EQ(formatter->formatLine(nullptr), QByteArray("\r\n"));
}

TEST_F(AnsiFormatterTest, FormatLine_NullLineNoNewline)
{
    EXPECT_EQ(formatter->formatLine(nullptr, false), QByteArray());
}

TEST_F(AnsiFormatterTest, FormatIncompleteLine_NullLine)
{
    EXPECT_EQ(formatter->formatIncompleteLine(nullptr), QByteArray());
}

TEST_F(AnsiFormatterTest, FormatLine_EmptyLine_NoStyles)
{
    auto line = makeLine("", {});
    QByteArray result = formatter->formatLine(line.get());
    // No styles → no ANSI codes, no state update, just the newline
    EXPECT_EQ(result, QByteArray("\r\n"));
}

// ---------------------------------------------------------------------------
// 2. Plain text / formatRaw
// ---------------------------------------------------------------------------

// On the first style after construction m_stateValid == false, so the formatter
// always emits reset + color codes even for "default" colours.
TEST_F(AnsiFormatterTest, FormatLine_PlainText_EmitsResetAndColors)
{
    // WHITE (index 7) FG, BLACK (index 0) BG, COLOUR_ANSI, no attribute flags
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(5, COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("Hello", std::move(styles));

    QByteArray result = formatter->formatLine(line.get());

    // Must contain reset before the first style
    EXPECT_TRUE(result.contains("\x1b[0m")) << result.toStdString();
    // FG white = index 7 → \x1b[37m
    EXPECT_TRUE(result.contains("\x1b[37m")) << result.toStdString();
    // BG black = index 0 → \x1b[40m
    EXPECT_TRUE(result.contains("\x1b[40m")) << result.toStdString();
    // Actual text present
    EXPECT_TRUE(result.contains("Hello")) << result.toStdString();
    // Trailing reset + CRLF
    EXPECT_TRUE(result.endsWith("\x1b[0m\r\n")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatRaw_WithNewline)
{
    EXPECT_EQ(AnsiFormatter::formatRaw("test"), QByteArray("test\r\n"));
}

TEST_F(AnsiFormatterTest, FormatRaw_WithoutNewline)
{
    EXPECT_EQ(AnsiFormatter::formatRaw("test", false), QByteArray("test"));
}

TEST_F(AnsiFormatterTest, FormatRaw_EmptyString)
{
    EXPECT_EQ(AnsiFormatter::formatRaw(""), QByteArray("\r\n"));
    EXPECT_EQ(AnsiFormatter::formatRaw("", false), QByteArray());
}

// ---------------------------------------------------------------------------
// 3. Text attributes
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_Bold)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, HILITE | COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("bold", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[1m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_Underline)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(9, UNDERLINE | COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("underline", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[4m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_Italic)
{
    // BLINK is repurposed as italic
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(6, BLINK | COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("italic", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[3m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_Inverse)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(7, INVERSE | COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("inverse", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[7m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_Strikeout)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(9, STRIKEOUT | COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("strikeout", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[9m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_AllAttributes)
{
    const quint16 allFlags = HILITE | UNDERLINE | BLINK | INVERSE | STRIKEOUT | COLOUR_ANSI;
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(3, allFlags, WHITE, BLACK));
    auto line = makeLine("all", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[1m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[4m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[3m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[7m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[9m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_NoNewline)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, WHITE, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_FALSE(result.endsWith("\r\n")) << result.toStdString();
    EXPECT_TRUE(result.contains("text")) << result.toStdString();
}

// ---------------------------------------------------------------------------
// 4. ANSI color indices (COLOUR_ANSI)
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_StandardForeground)
{
    // index 1 = red → \x1b[31m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, 1, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[31m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_StandardBackground)
{
    // BG index 2 = green → \x1b[42m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, WHITE, 2));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[42m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_BrightForeground)
{
    // index 9 = bright red → \x1b[91m   (90 + (9-8) = 91)
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, 9, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[91m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_BrightBackground)
{
    // BG index 10 = bright green → \x1b[102m   (100 + (10-8) = 102)
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, WHITE, 10));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[102m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_256ColorForeground)
{
    // index 200 → \x1b[38;5;200m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, 200, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[38;5;200m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_256ColorBackground)
{
    // BG index 220 → \x1b[48;5;220m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, WHITE, 220));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[48;5;220m")) << result.toStdString();
}

// ---------------------------------------------------------------------------
// 5. RGB truecolor (COLOUR_RGB)
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_RgbForeground)
{
    // qRgb(255,128,0) → \x1b[38;2;255;128;0m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_RGB, qRgb(255, 128, 0), qRgb(0, 0, 0)));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[38;2;255;128;0m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_RgbBackground)
{
    // BG qRgb(0,64,128) → \x1b[48;2;0;64;128m
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_RGB, qRgb(255, 255, 255), qRgb(0, 64, 128)));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[48;2;0;64;128m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_RgbBothChannels)
{
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_RGB, qRgb(10, 20, 30), qRgb(40, 50, 60)));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    EXPECT_TRUE(result.contains("\x1b[38;2;10;20;30m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[48;2;40;50;60m")) << result.toStdString();
}

// ---------------------------------------------------------------------------
// 6. Custom colors (COLOUR_CUSTOM)
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_CustomForeground)
{
    // Set custom_text[3] = qRgb(100,200,50)
    doc->m_colors.custom_text[3] = qRgb(100, 200, 50);

    // COLOUR_CUSTOM stored in iFlags; fore index 3 stored in low nibble of iForeColour
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_CUSTOM, 3, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    // Lookup maps index 3 → RGB(100,200,50) → \x1b[38;2;100;200;50m
    EXPECT_TRUE(result.contains("\x1b[38;2;100;200;50m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_CustomBackground)
{
    // Set custom_back[7] = qRgb(30,60,90)
    doc->m_colors.custom_back[7] = qRgb(30, 60, 90);

    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_CUSTOM, WHITE, 7));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get(), false);
    // Lookup maps index 7 → RGB(30,60,90) → \x1b[48;2;30;60;90m
    EXPECT_TRUE(result.contains("\x1b[48;2;30;60;90m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_CustomColorNoDoc)
{
    // AnsiFormatter with nullptr doc: COLOUR_CUSTOM → no color code emitted
    AnsiFormatter nullFormatter(nullptr);
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_CUSTOM, 3, 7));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = nullFormatter.formatLine(line.get(), false);
    // No truecolor or 256-color escape for the custom palette lookup
    EXPECT_FALSE(result.contains("\x1b[38;")) << result.toStdString();
    EXPECT_FALSE(result.contains("\x1b[48;")) << result.toStdString();
    // Text is still emitted
    EXPECT_TRUE(result.contains("text")) << result.toStdString();
}

// ---------------------------------------------------------------------------
// 7. Multi-style lines
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_TwoStyles_DifferentColors)
{
    // "HelloWorld": first 5 chars red (index 1), last 5 chars blue (index 4)
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(5, COLOUR_ANSI, 1, BLACK)); // red FG
    styles.push_back(makeStyle(5, COLOUR_ANSI, 4, BLACK)); // blue FG

    auto line = makeLine("HelloWorld", std::move(styles));
    QByteArray result = formatter->formatLine(line.get(), false);

    // Red = \x1b[31m, Blue = \x1b[34m
    EXPECT_TRUE(result.contains("\x1b[31m")) << result.toStdString();
    EXPECT_TRUE(result.contains("\x1b[34m")) << result.toStdString();
    // Both text segments present
    EXPECT_TRUE(result.contains("Hello")) << result.toStdString();
    EXPECT_TRUE(result.contains("World")) << result.toStdString();
    // Red code appears before blue in the byte stream
    EXPECT_LT(result.indexOf("\x1b[31m"), result.indexOf("\x1b[34m")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_StyleShorterThanText)
{
    // Style only covers 3 of 5 characters; remaining 2 are emitted without style change
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(3, COLOUR_ANSI, 1, BLACK));

    auto line = makeLine("Hello", std::move(styles));
    QByteArray result = formatter->formatLine(line.get(), false);

    // All 5 characters should be present
    EXPECT_TRUE(result.contains("Hel")) << result.toStdString();
    // "lo" is appended in the tail path (pos < textSpan.size() after styles exhausted)
    EXPECT_TRUE(result.contains("lo")) << result.toStdString();
}

// ---------------------------------------------------------------------------
// 8. State caching — identical consecutive styles must not re-emit ANSI codes
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, StateCaching_SameStyleWithinLine_NoRedundantCodes)
{
    // State caching works within a single line: two consecutive styles with
    // identical attributes should emit ANSI codes only for the first one.
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(5, COLOUR_ANSI, 2, BLACK)); // green, covers "Hello"
    styles.push_back(makeStyle(5, COLOUR_ANSI, 2, BLACK)); // same green, covers "World"

    auto line = makeLine("HelloWorld", std::move(styles));
    QByteArray result = formatter->formatLine(line.get(), false);

    // Green FG (index 2 → \x1b[32m) should appear exactly ONCE — for the first style.
    // The second style has identical attributes, so styleToAnsi returns empty.
    int firstGreen = result.indexOf("\x1b[32m");
    EXPECT_NE(firstGreen, -1) << "First green code must be present";
    int secondGreen = result.indexOf("\x1b[32m", firstGreen + 1);
    EXPECT_EQ(secondGreen, -1) << "Second green code must NOT be present (cached)";

    // Both text segments present
    EXPECT_TRUE(result.contains("Hello")) << result.toStdString();
    EXPECT_TRUE(result.contains("World")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, StateCaching_DifferentStyleEmitsNewCodes)
{
    // Prime with style A
    {
        std::vector<std::unique_ptr<Style>> styles;
        styles.push_back(makeStyle(4, COLOUR_ANSI, 1, BLACK)); // red
        auto line = makeLine("text", std::move(styles));
        formatter->formatIncompleteLine(line.get());
    }

    // Now switch to style B (different foreground color)
    {
        std::vector<std::unique_ptr<Style>> styles;
        styles.push_back(makeStyle(4, COLOUR_ANSI, 4, BLACK)); // blue
        auto line = makeLine("text", std::move(styles));
        QByteArray result = formatter->formatIncompleteLine(line.get());
        // Must emit new color codes for the changed foreground
        EXPECT_TRUE(result.contains("\x1b[34m")) << result.toStdString();
    }
}

TEST_F(AnsiFormatterTest, Reset_ClearsState)
{
    // Prime with a known style so m_stateValid becomes true
    {
        std::vector<std::unique_ptr<Style>> styles;
        styles.push_back(makeStyle(4, COLOUR_ANSI, 3, BLACK)); // green
        auto line = makeLine("text", std::move(styles));
        formatter->formatIncompleteLine(line.get());
    }

    // After reset(), m_stateValid is false → next style always emits
    formatter->reset();

    // Format a SECOND line with the SAME style; codes must be re-emitted
    {
        std::vector<std::unique_ptr<Style>> styles;
        styles.push_back(makeStyle(4, COLOUR_ANSI, 3, BLACK)); // same green
        auto line = makeLine("text", std::move(styles));
        QByteArray result = formatter->formatIncompleteLine(line.get());
        // \x1b[33m = FG green (30 + 3)
        EXPECT_TRUE(result.contains("\x1b[33m")) << result.toStdString();
    }
}

// ---------------------------------------------------------------------------
// 9. Edge cases
// ---------------------------------------------------------------------------

TEST_F(AnsiFormatterTest, FormatLine_ZeroLengthStyle_Skipped)
{
    // A style with iLength == 0 must be skipped entirely (no ANSI codes, no text)
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(0, COLOUR_ANSI, 1, BLACK));     // zero-length: skipped
    styles.push_back(makeStyle(5, COLOUR_ANSI, WHITE, BLACK)); // covers actual text

    auto line = makeLine("Hello", std::move(styles));
    QByteArray result = formatter->formatLine(line.get(), false);

    // Red color (index 1 → \x1b[31m) must NOT appear because the red style was skipped
    EXPECT_FALSE(result.contains("\x1b[31m")) << result.toStdString();
    EXPECT_TRUE(result.contains("Hello")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_NullStylePointer_Skipped)
{
    // A null style pointer in styleList is also skipped
    auto line = makeLine("Hello", {});
    line->styleList.push_back(nullptr); // explicit null entry

    // Should not crash, should emit just the text (tail path) + reset + CRLF
    QByteArray result;
    EXPECT_NO_THROW(result = formatter->formatLine(line.get()));
    EXPECT_TRUE(result.endsWith("\r\n")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatIncompleteLine_EquivalentToFormatLineNoNewline)
{
    std::vector<std::unique_ptr<Style>> styles1;
    styles1.push_back(makeStyle(5, COLOUR_ANSI, 3, BLACK));
    auto line1 = makeLine("Hello", std::move(styles1));

    AnsiFormatter f1(doc.get());
    AnsiFormatter f2(doc.get());

    std::vector<std::unique_ptr<Style>> styles2;
    styles2.push_back(makeStyle(5, COLOUR_ANSI, 3, BLACK));
    auto line2 = makeLine("Hello", std::move(styles2));

    QByteArray via_incomplete = f1.formatIncompleteLine(line1.get());
    QByteArray via_no_newline = f2.formatLine(line2.get(), false);

    EXPECT_EQ(via_incomplete, via_no_newline);
}

TEST_F(AnsiFormatterTest, FormatLine_TrailingResetPresent_WhenStyleEmitted)
{
    // Any line that causes a style update must end with \x1b[0m before the CRLF
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(4, COLOUR_ANSI, 1, BLACK));
    auto line = makeLine("text", std::move(styles));

    QByteArray result = formatter->formatLine(line.get());
    EXPECT_TRUE(result.endsWith("\x1b[0m\r\n")) << result.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_TrailingResetAbsent_WhenNoStyleEmitted)
{
    // A line with no styles (and therefore no state change) must NOT emit a trailing reset
    auto line = makeLine("", {});

    QByteArray result = formatter->formatLine(line.get());
    // Only \r\n, no reset before it
    EXPECT_EQ(result, QByteArray("\r\n"));
}

TEST_F(AnsiFormatterTest, FormatLine_MultipleLines_TrailingResetResetsState)
{
    // After formatLine() the state is reset (m_stateValid = false), so the next
    // line with the same style must re-emit codes.
    std::vector<std::unique_ptr<Style>> styles1;
    styles1.push_back(makeStyle(4, COLOUR_ANSI, 2, BLACK)); // green
    auto line1 = makeLine("line", std::move(styles1));
    formatter->formatLine(line1.get()); // state goes false after trailing reset

    std::vector<std::unique_ptr<Style>> styles2;
    styles2.push_back(makeStyle(4, COLOUR_ANSI, 2, BLACK)); // same green
    auto line2 = makeLine("line", std::move(styles2));
    QByteArray result2 = formatter->formatLine(line2.get(), false);

    // Must re-emit codes because m_stateValid was cleared by the trailing reset
    EXPECT_TRUE(result2.contains("\x1b[32m")) << result2.toStdString();
}

TEST_F(AnsiFormatterTest, FormatLine_TextOrderPreserved)
{
    // Verify the relative order of text segments and ANSI codes in the output
    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(makeStyle(5, COLOUR_ANSI, 1, BLACK)); // "Hello" in red
    styles.push_back(makeStyle(5, COLOUR_ANSI, 4, BLACK)); // "World" in blue

    auto line = makeLine("HelloWorld", std::move(styles));
    QByteArray result = formatter->formatLine(line.get(), false);

    int posRed = result.indexOf("\x1b[31m");
    int posHello = result.indexOf("Hello");
    int posBlue = result.indexOf("\x1b[34m");
    int posWorld = result.indexOf("World");

    EXPECT_LT(posRed, posHello) << "Red code must precede Hello text";
    EXPECT_LT(posHello, posBlue) << "Hello text must precede blue code";
    EXPECT_LT(posBlue, posWorld) << "Blue code must precede World text";
}
