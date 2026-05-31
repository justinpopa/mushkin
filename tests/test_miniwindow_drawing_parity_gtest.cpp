/**
 * test_miniwindow_drawing_parity_gtest.cpp
 *
 * Behavioral-parity tests for miniwindow drawing Lua API return values.
 * Verifies that error sentinel values match the original MUSHclient source.
 *
 * References:
 *   - methods_miniwindows.cpp:234-240 (WindowText returns -1 on not-found)
 *   - methods_miniwindows.cpp:248-251 (WindowTextWidth returns -1 on not-found)
 *   - methods_miniwindows.cpp:791-796 (WindowGetPixel returns -2 on not-found)
 */

#include "../src/world/miniwindow.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"

class MiniWindowDrawingParityTest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();
        // Create a window so we can also test valid cases
        executeLua(R"(
            world.WindowCreate("ptest", 0, 0, 100, 100, miniwin.pos_center_all, 0, 0x000000)
            world.WindowShow("ptest", true)
            world.WindowFont("ptest", "f", "Arial", 12, false, false, false, false)
        )");
    }
};

// H76: WindowText returns -1 when window not found (not eNoSuchWindow=30073)
TEST_F(MiniWindowDrawingParityTest, WindowTextReturnsNegOneOnWindowNotFound)
{
    executeLua(
        R"(result = world.WindowText("nonexistent", "f", "hi", 0, 0, 0, 0, 0xFFFFFF, false))");
    EXPECT_EQ(getGlobalNumber("result"), -1.0);
}

// H76: WindowText returns -1 for empty window name
TEST_F(MiniWindowDrawingParityTest, WindowTextReturnsNegOneOnEmptyName)
{
    executeLua(R"(result = world.WindowText("", "f", "hi", 0, 0, 0, 0, 0xFFFFFF, false))");
    EXPECT_EQ(getGlobalNumber("result"), -1.0);
}

// H76: WindowText returns positive width on success (verify fix didn't break success path)
TEST_F(MiniWindowDrawingParityTest, WindowTextReturnsPositiveWidthOnSuccess)
{
    executeLua(R"(result = world.WindowText("ptest", "f", "Hello", 5, 5, 0, 0, 0xFFFFFF, false))");
    EXPECT_GT(getGlobalNumber("result"), 0.0);
}

// H76: WindowTextWidth returns -1 when window not found (not 0)
TEST_F(MiniWindowDrawingParityTest, WindowTextWidthReturnsNegOneOnWindowNotFound)
{
    executeLua(R"(result = world.WindowTextWidth("nonexistent", "f", "hi", false))");
    EXPECT_EQ(getGlobalNumber("result"), -1.0);
}

// H76: WindowTextWidth returns positive width on success
TEST_F(MiniWindowDrawingParityTest, WindowTextWidthReturnsPositiveWidthOnSuccess)
{
    executeLua(R"(result = world.WindowTextWidth("ptest", "f", "Hello", false))");
    EXPECT_GT(getGlobalNumber("result"), 0.0);
}

// L85: WindowGetPixel returns -2 when window not found (not 0)
TEST_F(MiniWindowDrawingParityTest, WindowGetPixelReturnsNegTwoOnWindowNotFound)
{
    executeLua(R"(result = world.WindowGetPixel("nonexistent", 10, 10))");
    EXPECT_EQ(getGlobalNumber("result"), -2.0);
}

// L85: WindowGetPixel returns a valid color when the window exists
TEST_F(MiniWindowDrawingParityTest, WindowGetPixelReturnsColorOnSuccess)
{
    // Set a known pixel then read it back
    executeLua(R"(
        world.WindowSetPixel("ptest", 10, 10, 0x0000FF)
        result = world.WindowGetPixel("ptest", 10, 10)
    )");
    // 0x0000FF in BGR = red channel=0xFF, green=0x00, blue=0x00
    double val = getGlobalNumber("result");
    EXPECT_GE(val, 0.0);
    EXPECT_NE(val, -2.0);
}
