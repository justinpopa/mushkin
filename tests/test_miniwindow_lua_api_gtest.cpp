/**
 * test_miniwindow_lua_api_gtest.cpp - Miniwindow Lua API Tests
 *
 * Tests for miniwindow functions via Lua API:
 * - Drawing: WindowRectOp, WindowCircleOp, WindowLine, WindowSetPixel, WindowGetPixel
 * - Shapes: WindowArc, WindowBezier, WindowPolygon, WindowGradient
 * - Hotspots: WindowHotspots, WindowHotspotInfo, WindowMoveHotspot, etc.
 * - Images: WindowLoadImage, WindowDrawImage, WindowImageFromWindow, etc.
 * - Text: WindowFont, WindowText, WindowTextWidth, WindowFontInfo, WindowFontList
 * - Filters: WindowShow, WindowWrite, WindowFilter
 * - Position: WindowPositionResize, WindowZOrder
 */

#include "lua_api_test_fixture.h"

// ========== Miniwindow Shape Functions Tests ==========

// Test 159: WindowArc
TEST_F(LuaApiTest, WindowArc)
{
    lua_getglobal(L, "test_window_arc");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_arc should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_arc should succeed";
}

// Test 160: WindowBezier
TEST_F(LuaApiTest, WindowBezier)
{
    lua_getglobal(L, "test_window_bezier");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_bezier should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_bezier should succeed";
}

// Test 161: WindowPolygon
TEST_F(LuaApiTest, WindowPolygon)
{
    lua_getglobal(L, "test_window_polygon");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_polygon should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_polygon should succeed";
}

// Test 162: WindowGradient
TEST_F(LuaApiTest, WindowGradient)
{
    lua_getglobal(L, "test_window_gradient");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_gradient should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_gradient should succeed";
}

// ========== Miniwindow Hotspot Functions Tests ==========

// Test 163: WindowHotspots
TEST_F(LuaApiTest, WindowHotspots)
{
    lua_getglobal(L, "test_window_hotspots");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_hotspots should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_hotspots should succeed";
}

// Test 164: WindowHotspotInfo
TEST_F(LuaApiTest, WindowHotspotInfo)
{
    lua_getglobal(L, "test_window_hotspot_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_hotspot_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_hotspot_info should succeed";
}

TEST_F(LuaApiTest, DISABLED_WindowMenu)
{
    lua_getglobal(L, "test_window_menu");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_menu should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_menu should succeed";
}

TEST_F(LuaApiTest, WindowMoveHotspot)
{
    lua_getglobal(L, "test_window_move_hotspot");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_move_hotspot should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_move_hotspot should succeed";
}

TEST_F(LuaApiTest, WindowHotspotTooltip)
{
    lua_getglobal(L, "test_window_hotspot_tooltip");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_hotspot_tooltip should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_hotspot_tooltip should succeed";
}

TEST_F(LuaApiTest, WindowDragHandler)
{
    lua_getglobal(L, "test_window_drag_handler");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_drag_handler should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_drag_handler should succeed";
}

TEST_F(LuaApiTest, WindowScrollwheelHandler)
{
    lua_getglobal(L, "test_window_scrollwheel_handler");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_scrollwheel_handler should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_scrollwheel_handler should succeed";
}

// ========== Miniwindow Image Functions Tests ==========

TEST_F(LuaApiTest, DISABLED_WindowLoadImage)
{
    lua_getglobal(L, "test_window_load_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_load_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_load_image should succeed";
}

TEST_F(LuaApiTest, WindowDrawImage)
{
    lua_getglobal(L, "test_window_draw_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_draw_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_draw_image should succeed";
}

TEST_F(LuaApiTest, WindowDrawImageAlpha)
{
    lua_getglobal(L, "test_window_draw_image_alpha");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_draw_image_alpha should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_draw_image_alpha should succeed";
}

TEST_F(LuaApiTest, WindowBlendImage)
{
    lua_getglobal(L, "test_window_blend_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_blend_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_blend_image should succeed";
}

TEST_F(LuaApiTest, WindowImageFromWindow)
{
    lua_getglobal(L, "test_window_image_from_window");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_image_from_window should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_image_from_window should succeed";
}

TEST_F(LuaApiTest, WindowGetImageAlpha)
{
    lua_getglobal(L, "test_window_get_image_alpha");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_get_image_alpha should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_get_image_alpha should succeed";
}

TEST_F(LuaApiTest, WindowMergeImageAlpha)
{
    lua_getglobal(L, "test_window_merge_image_alpha");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_merge_image_alpha should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_merge_image_alpha should succeed";
}

TEST_F(LuaApiTest, WindowTransformImage)
{
    lua_getglobal(L, "test_window_transform_image");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_transform_image should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_transform_image should succeed";
}

// Test 165: WindowImages
TEST_F(LuaApiTest, WindowImages)
{
    lua_getglobal(L, "test_window_images");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_images should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_images should succeed";
}

// ========== Miniwindow Position Functions Tests ==========

// Test 166: WindowPositionResize
TEST_F(LuaApiTest, WindowPositionResize)
{
    lua_getglobal(L, "test_window_position_resize");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_position_resize should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_position_resize should succeed";
}

// Test 167: WindowZOrder
TEST_F(LuaApiTest, WindowZOrder)
{
    lua_getglobal(L, "test_window_zorder");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_zorder should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_zorder should succeed";
}

// ========== Miniwindow Text Functions Tests ==========

// Test WindowFont API
TEST_F(LuaApiTest, WindowFont)
{
    lua_getglobal(L, "test_window_font");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_window_font should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_window_font should succeed";
}

// Test WindowText API
TEST_F(LuaApiTest, WindowText)
{
    lua_getglobal(L, "test_window_text");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_window_text should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_window_text should succeed";
}

// Test WindowTextWidth API
TEST_F(LuaApiTest, WindowTextWidth)
{
    lua_getglobal(L, "test_window_text_width");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_window_text_width should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_window_text_width should succeed";
}

// Test WindowFontInfo API
TEST_F(LuaApiTest, WindowFontInfo)
{
    lua_getglobal(L, "test_window_font_info");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_window_font_info should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_window_font_info should succeed";
}

// Test WindowFontList API
TEST_F(LuaApiTest, WindowFontList)
{
    lua_getglobal(L, "test_window_font_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_window_font_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_window_font_list should succeed";
}

// ========== Miniwindow Drawing Functions Tests ==========

TEST_F(LuaApiTest, WindowRectOp)
{
    lua_getglobal(L, "test_window_rect_op");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_rect_op should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_rect_op should succeed";
}

TEST_F(LuaApiTest, WindowCircleOp)
{
    lua_getglobal(L, "test_window_circle_op");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_circle_op should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_circle_op should succeed";
}

TEST_F(LuaApiTest, WindowLine)
{
    lua_getglobal(L, "test_window_line");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_line should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_line should succeed";
}

TEST_F(LuaApiTest, WindowSetPixel)
{
    lua_getglobal(L, "test_window_set_pixel");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_set_pixel should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_set_pixel should succeed";
}

TEST_F(LuaApiTest, WindowGetPixel)
{
    lua_getglobal(L, "test_window_get_pixel");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_get_pixel should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_get_pixel should succeed";
}

// ========== Miniwindow Filter Functions Tests ==========

TEST_F(LuaApiTest, WindowShow)
{
    lua_getglobal(L, "test_window_show");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_show should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_show should succeed";
}

TEST_F(LuaApiTest, WindowWrite)
{
    lua_getglobal(L, "test_window_write");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_write should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_write should succeed";
}

TEST_F(LuaApiTest, WindowFilter)
{
    lua_getglobal(L, "test_window_filter");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_window_filter should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_window_filter should succeed";
}

int main(int argc, char** argv)
{
    // Initialize Qt GUI application (required for Lua API operations)
    QGuiApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
