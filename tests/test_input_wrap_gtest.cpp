/**
 * test_input_wrap_gtest.cpp
 *
 * M149: Input wrapping uses wrap_column, not widget-width.
 *
 * Verifies that InputView::applyInputSettings() honours m_display.wrap_column
 * when auto_wrap is enabled, mirroring CSendView::UpdateWrap() from
 * sendvw.cpp:3015-3062 in the original MUSHclient.
 *
 * The original sets a pixel-based right margin:
 *   iWidth  = clamp(wrap_column + 1, 20, MAX_LINE_WIDTH)
 *   rMargin = widgetWidth - iWidth * charWidth - pixelOffset
 * so that text wraps at the configured column count rather than at the full
 * widget boundary.
 *
 * In Mushkin this is implemented via a right viewport margin in WidgetWidth
 * mode (see updateWrapMargin() in input_view.cpp).
 */

#include "../src/ui/views/input_view.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QFontMetrics>

class InputWrapTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        input = new InputView(doc.get());
        // Give the widget a concrete size so viewport margin calculations
        // have a non-zero width to work with.
        input->resize(800, 50);
    }

    void TearDown() override
    {
        delete input;
    }

    InputView* input = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: NoWrap when auto_wrap is disabled
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, NoWrapModeWhenAutoWrapDisabled)
{
    doc->m_input.auto_wrap = false;
    input->applyInputSettings();

    EXPECT_EQ(input->lineWrapMode(), QPlainTextEdit::NoWrap)
        << "auto_wrap=false should set NoWrap line-wrap mode";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: WidgetWidth mode is used when auto_wrap is enabled
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, WidgetWidthModeWhenAutoWrapEnabled)
{
    doc->m_input.auto_wrap = true;
    doc->m_display.wrap_column = 80;
    input->applyInputSettings();

    // WidgetWidth is used; the column constraint is enforced via viewport margin
    EXPECT_EQ(input->lineWrapMode(), QPlainTextEdit::WidgetWidth)
        << "auto_wrap=true should set WidgetWidth line-wrap mode";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Right viewport margin is positive when widget is wide enough
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, RightViewportMarginSetForColumnWrap)
{
    doc->m_input.auto_wrap = true;
    doc->m_display.wrap_column = 80;
    input->applyInputSettings();

    // The right margin must be > 0 when the widget (800px) is wider than the
    // column text area — which it always should be for a 80-column font.
    QMargins margins = input->inputViewportMargins();
    EXPECT_GT(margins.right(), 0)
        << "Right viewport margin should be positive to enforce the column limit";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: No viewport margin when auto_wrap is disabled
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, NoViewportMarginWhenAutoWrapDisabled)
{
    doc->m_input.auto_wrap = false;
    input->applyInputSettings();

    QMargins margins = input->inputViewportMargins();
    EXPECT_EQ(margins.right(), 0)
        << "No right viewport margin should be set when auto_wrap is disabled";
    EXPECT_EQ(margins.left(), 0)
        << "No left viewport margin should be set when auto_wrap is disabled";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: Narrower column produces a larger right margin
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, NarrowerColumnProducesLargerRightMargin)
{
    doc->m_input.auto_wrap = true;

    doc->m_display.wrap_column = 80;
    input->applyInputSettings();
    int margin80 = input->inputViewportMargins().right();

    doc->m_display.wrap_column = 40;
    input->applyInputSettings();
    int margin40 = input->inputViewportMargins().right();

    EXPECT_GT(margin40, margin80)
        << "Wrapping at 40 columns should leave a larger right margin than 80 columns";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: wrap_column=0 is clamped to minimum 20 columns (original: iWidth min 20)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(InputWrapTest, WrapColumnClampedToMinimum20)
{
    doc->m_input.auto_wrap = true;

    // Column 0 should behave identically to column 19 (both clamped to 20)
    doc->m_display.wrap_column = 0;
    input->applyInputSettings();
    int marginAt0 = input->inputViewportMargins().right();

    doc->m_display.wrap_column = 19;
    input->applyInputSettings();
    int marginAt19 = input->inputViewportMargins().right();

    EXPECT_EQ(marginAt0, marginAt19)
        << "wrap_column below 20 should be clamped to 20 (original: iWidth min 20)";
}
