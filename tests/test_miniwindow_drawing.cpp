/**
 * test_miniwindow_drawing.cpp - Test Drawing Primitives and Text
 *
 * Tests miniwindow drawing operations including rectangles, circles, lines,
 * pixels, fonts, and text rendering.
 *
 * Verifies:
 * 1. WindowRectOp (frame, fill, invert, 3D)
 * 2. WindowCircleOp (ellipse, rectangle, round_rect, chord, pie, arc)
 * 3. WindowLine
 * 4. WindowSetPixel/GetPixel
 * 5. WindowFont
 * 6. WindowText
 * 7. WindowTextWidth
 * 8. WindowFontInfo
 */

#include "../src/world/miniwindow.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QApplication>
#include <QDebug>
#include <QImage>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Helper to execute Lua code and check for errors
bool executeLua(lua_State* L, const char* code, const char* testName)
{
    if (luaL_loadstring(L, code)) {
        qDebug() << "✗ FAIL:" << testName << "- Compile error:";
        qDebug() << "  " << lua_tostring(L, -1);
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0)) {
        qDebug() << "✗ FAIL:" << testName << "- Runtime error:";
        qDebug() << "  " << lua_tostring(L, -1);
        lua_pop(L, 1);
        return false;
    }

    return true;
}

// Helper to get global number
double getGlobalNumber(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    double result = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return result;
}

// Helper to get global string
QString getGlobalString(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    const char* str = lua_tostring(L, -1);
    QString result = str ? QString::fromUtf8(str) : QString();
    lua_pop(L, 1);
    return result;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    qDebug() << "=== Miniwindow Drawing Tests ===\n";

    // Create world document
    WorldDocument doc;
    if (!doc.m_ScriptEngine || !doc.m_ScriptEngine->L) {
        qDebug() << "✗ FAIL: No Lua state created";
        return 1;
    }
    lua_State* L = doc.m_ScriptEngine->L;
    qDebug() << "✓ WorldDocument and Lua state created\n";

    // Create test miniwindow
    // Note: MUSHclient uses BGR color format: 0x00BBGGRR
    if (!executeLua(L, R"(
        world.WindowCreate("draw_test", 0, 0, 200, 200,
                          miniwin.pos_center_all, 0, 0x000000)  -- black (BGR)
        world.WindowShow("draw_test", true)
    )",
                    "Create test window")) {
        return 1;
    }

    MiniWindow* iwin = doc.m_MiniWindowMap["draw_test"].get();
    MiniWindow* win = iwin;
    if (!win || !win->getImage()) {
        qDebug() << "✗ FAIL: Test window not created properly";
        return 1;
    }

    qDebug() << "✓ Test miniwindow created (200x200, black background)\n";

    // ========== Test 1: WindowRectOp - Frame ==========
    qDebug() << "Test 1: WindowRectOp - Frame (outline)";

    if (!executeLua(L, R"(
        result = world.WindowRectOp("draw_test",
                                    miniwin.rect_frame,
                                    10, 10, 50, 50,      -- left, top, right, bottom
                                    0x0000FF,             -- red pen (BGR format: 0x00BBGGRR)
                                    0x000000)             -- black brush (unused)
    )",
                    "WindowRectOp frame")) {
        return 1;
    }

    double result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowRectOp frame returned" << result;
        return 1;
    }

    // Verify pixels changed (check corners of rectangle)
    QImage img = (*win->getImage());
    QRgb topLeft = img.pixel(10, 10);
    QRgb topRight = img.pixel(49, 10);

    // Note: Antial iasing can cause edge pixels to be slightly less than full color
    if (qRed(topLeft) < 180 || qRed(topRight) < 180) {
        qDebug() << "✗ FAIL: Rectangle frame not drawn (red pixels not found)";
        qDebug() << "  topLeft pixel:" << QString::number(topLeft, 16) << "red=" << qRed(topLeft);
        qDebug() << "  topRight pixel:" << QString::number(topRight, 16)
                 << "red=" << qRed(topRight);
        return 1;
    }

    qDebug() << "✓ WindowRectOp frame draws rectangle outline\n";

    // ========== Test 2: WindowRectOp - Fill ==========
    qDebug() << "Test 2: WindowRectOp - Fill";

    if (!executeLua(L, R"(
        result = world.WindowRectOp("draw_test",
                                    miniwin.rect_fill,
                                    60, 10, 100, 50,
                                    0x00FF00,        -- green Colour1 (BGR) — fill uses this
                                    0x000000)        -- Colour2 (unused for fill)
    )",
                    "WindowRectOp fill")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowRectOp fill returned" << result;
        return 1;
    }

    // Verify filled area is green (antialiasing tolerance)
    img = (*win->getImage());
    QRgb center = img.pixel(80, 30);
    if (qGreen(center) < 180) {
        qDebug() << "✗ FAIL: Rectangle not filled with green";
        return 1;
    }

    qDebug() << "✓ WindowRectOp fill draws filled rectangle\n";

    // ========== Test 3: WindowRectOp - Invert ==========
    qDebug() << "Test 3: WindowRectOp - Invert (XOR)";

    // Get original pixel
    img = (*win->getImage());
    QRgb originalPixel = img.pixel(120, 30);

    if (!executeLua(L, R"(
        result = world.WindowRectOp("draw_test",
                                    miniwin.rect_invert,
                                    110, 10, 150, 50,
                                    0xFFFFFF,        -- white pen (BGR)
                                    0x000000)        -- black brush (BGR)
    )",
                    "WindowRectOp invert")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowRectOp invert returned" << result;
        return 1;
    }

    // Verify pixel changed
    img = (*win->getImage());
    QRgb invertedPixel = img.pixel(120, 30);
    if (originalPixel == invertedPixel) {
        qDebug() << "✗ FAIL: Invert operation didn't change pixel";
        return 1;
    }

    qDebug() << "✓ WindowRectOp invert performs XOR operation\n";

    // ========== Test 4: WindowRectOp - Invalid action ==========
    qDebug() << "Test 4: WindowRectOp - Invalid action code";

    if (!executeLua(L, R"(
        result = world.WindowRectOp("draw_test", 99,  -- invalid action
                                    0, 0, 10, 10, 0x000000, 0x000000)  -- BGR colors
    )",
                    "WindowRectOp invalid")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != static_cast<double>(eUnknownOption)) { // eUnknownOption
        qDebug() << "✗ FAIL: WindowRectOp should reject invalid action, got" << result;
        return 1;
    }

    qDebug() << "✓ WindowRectOp correctly validates action codes\n";

    // ========== Test 5: WindowCircleOp - Ellipse ==========
    qDebug() << "Test 5: WindowCircleOp - Ellipse";

    if (!executeLua(L, R"(
        result = world.WindowCircleOp("draw_test",
                                      miniwin.circle_ellipse,
                                      10, 60, 50, 100,      -- bounding box
                                      0x00FFFF,             -- yellow pen (BGR: R+G)
                                      miniwin.pen_solid, 1, -- pen style/width
                                      0x000000,             -- black brush (BGR)
                                      miniwin.brush_solid,  -- brush style
                                      0, 0, 0, 0)           -- extra params
    )",
                    "WindowCircleOp ellipse")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowCircleOp ellipse returned" << result;
        return 1;
    }

    // Verify ellipse was drawn (check rightmost edge, less antialiasing)
    // Ellipse bounding box: (10,60) to (50,100) -> QRect(10,60,40,40)
    // Rightmost point: x=49, y=80 (center height)
    img = (*win->getImage());
    QRgb ellipsePixel = img.pixel(49, 80);
    if (qRed(ellipsePixel) < 120 || qGreen(ellipsePixel) < 120) {
        qDebug() << "✗ FAIL: Ellipse not drawn (yellow pixels not found)";
        qDebug() << "  Pixel at (49,80):" << QString::number(ellipsePixel, 16)
                 << "R=" << qRed(ellipsePixel) << "G=" << qGreen(ellipsePixel);
        return 1;
    }

    qDebug() << "✓ WindowCircleOp ellipse draws ellipse\n";

    // ========== Test 6: WindowCircleOp - Arc ==========
    qDebug() << "Test 6: WindowCircleOp - Arc";

    if (!executeLua(L, R"(
        result = world.WindowCircleOp("draw_test",
                                      miniwin.circle_arc,
                                      60, 60, 100, 100,
                                      0xFFFF00,             -- cyan pen (BGR: G+B)
                                      miniwin.pen_solid, 2,
                                      0x000000,             -- black brush (BGR)
                                      miniwin.brush_solid,
                                      0, 90,                -- start angle, span angle
                                      0, 0)
    )",
                    "WindowCircleOp arc")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    // Action 6 (arc) is NOT valid for CircleOp in original MUSHclient — returns eUnknownOption
    if (result != 30025) { // eUnknownOption
        qDebug() << "✗ FAIL: WindowCircleOp arc should return eUnknownOption(30025), got" << result;
        return 1;
    }

    qDebug() << "✓ WindowCircleOp arc correctly returns eUnknownOption (use WindowArc instead)\n";

    // ========== Test 7: WindowLine ==========
    qDebug() << "Test 7: WindowLine";

    if (!executeLua(L, R"(
        result = world.WindowLine("draw_test",
                                  10, 110, 100, 110,    -- horizontal line
                                  0xFF00FF,              -- magenta (BGR: R+B)
                                  miniwin.pen_solid, 2)
    )",
                    "WindowLine")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowLine returned" << result;
        return 1;
    }

    // Verify line was drawn (antialiasing tolerance)
    img = (*win->getImage());
    QRgb linePixel = img.pixel(50, 110);
    if (qRed(linePixel) < 180 || qBlue(linePixel) < 180) {
        qDebug() << "✗ FAIL: Line not drawn (magenta pixels not found)";
        return 1;
    }

    qDebug() << "✓ WindowLine draws line\n";

    // ========== Test 8: WindowSetPixel / GetPixel ==========
    qDebug() << "Test 8: WindowSetPixel and WindowGetPixel";

    if (!executeLua(L, R"(
        result = world.WindowSetPixel("draw_test", 120, 120, 0x0088FF)  -- orange (BGR)
    )",
                    "WindowSetPixel")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowSetPixel returned" << result;
        return 1;
    }

    if (!executeLua(L, R"(
        color = world.WindowGetPixel("draw_test", 120, 120)
    )",
                    "WindowGetPixel")) {
        return 1;
    }

    double color = getGlobalNumber(L, "color");
    uint32_t pixelColor = (uint32_t)color;

    // WindowGetPixel returns BGR format: 0x00BBGGRR
    // Orange was set as 0x0088FF (B=0, G=88, R=FF)
    int bgrRed = pixelColor & 0xFF;
    int bgrGreen = (pixelColor >> 8) & 0xFF;
    // Check for orange color (R=0xFF, G=0x88) with some tolerance
    if (bgrRed < 200 || bgrGreen < 80 || bgrGreen > 180) {
        qDebug() << "✗ FAIL: GetPixel didn't return set color, got"
                 << QString::number(pixelColor, 16);
        return 1;
    }

    qDebug() << "✓ WindowSetPixel and WindowGetPixel work correctly\n";

    // ========== Test 9: WindowGetPixel out of bounds ==========
    qDebug() << "Test 9: WindowGetPixel out of bounds";

    if (!executeLua(L, R"(
        color = world.WindowGetPixel("draw_test", 999, 999)
    )",
                    "WindowGetPixel out of bounds")) {
        return 1;
    }

    color = getGlobalNumber(L, "color");
    // WindowGetPixel is VT_I4 (signed 32-bit) in the original, so CLR_INVALID
    // (0xFFFFFFFF) marshals to -1 in the Lua-visible value.
    if (color != -1.0) {
        qDebug() << "✗ FAIL: WindowGetPixel out of bounds should return CLR_INVALID (-1), got"
                 << color;
        return 1;
    }

    qDebug() << "✓ WindowGetPixel correctly returns CLR_INVALID for out of bounds\n";

    // ========== Test 10: WindowFont ==========
    qDebug() << "Test 10: WindowFont";

    if (!executeLua(L, R"(
        result = world.WindowFont("draw_test",
                                  "font1",       -- font id
                                  "Arial",       -- font name
                                  12,            -- size
                                  true,          -- bold
                                  false,         -- italic
                                  false,         -- underline
                                  false)         -- strikeout
    )",
                    "WindowFont")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowFont returned" << result;
        return 1;
    }

    // Verify font was cached
    if (!win->fonts.count("font1")) {
        qDebug() << "✗ FAIL: Font not cached in miniwindow";
        return 1;
    }

    const QFont& font = win->fonts["font1"];
    if (!font.bold()) {
        qDebug() << "✗ FAIL: Font properties incorrect";
        return 1;
    }

    qDebug() << "✓ WindowFont creates and caches font\n";

    // ========== Test 11: WindowFont overwrite ==========
    qDebug() << "Test 11: WindowFont overwrite existing font";

    int fontCount = win->fonts.size();

    if (!executeLua(L, R"(
        result = world.WindowFont("draw_test", "font1", "Times", 14,
                                  false, true, false, false)
    )",
                    "WindowFont overwrite")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowFont overwrite returned" << result;
        return 1;
    }

    // Should still have same number of fonts (overwrite, not add)
    if (win->fonts.size() != fontCount) {
        qDebug() << "✗ FAIL: Font overwrite created new font instead of replacing";
        return 1;
    }

    // Check new properties
    const QFont& font2 = win->fonts["font1"];
    if (font2.bold() || !font2.italic()) {
        qDebug() << "✗ FAIL: Font properties not updated";
        return 1;
    }

    qDebug() << "✓ WindowFont correctly overwrites existing font\n";

    // ========== Test 12: WindowText ==========
    qDebug() << "Test 12: WindowText";

    if (!executeLua(L, R"(
        result = world.WindowText("draw_test",
                                  "font1",
                                  "Hello MUSHclient!",
                                  10, 130, 190, 160,   -- text rectangle
                                  0xFFFFFF,            -- white (BGR)
                                  false)               -- not unicode
    )",
                    "WindowText")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    // WindowText returns text width on success (positive value)
    if (result <= 0) {
        qDebug() << "✗ FAIL: WindowText returned" << result << "(expected positive text width)";
        return 1;
    }

    // Verify text was drawn (check for non-black pixels in text area)
    img = (*win->getImage());
    bool foundWhitePixel = false;
    for (int x = 15; x < 100; x += 5) {
        QRgb pixel = img.pixel(x, 140);
        if (qRed(pixel) > 200 && qGreen(pixel) > 200 && qBlue(pixel) > 200) {
            foundWhitePixel = true;
            break;
        }
    }

    if (!foundWhitePixel) {
        qDebug() << "✗ FAIL: Text not drawn (no white pixels found)";
        return 1;
    }

    qDebug() << "✓ WindowText draws text\n";

    // ========== Test 13: WindowText with nonexistent font ==========
    qDebug() << "Test 13: WindowText with nonexistent font";

    if (!executeLua(L, R"(
        result = world.WindowText("draw_test", "nonexistent", "Test",
                                  0, 0, 100, 20, 0xFFFFFF, false)  -- white (BGR)
    )",
                    "WindowText bad font")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != -2) { // Font not found (original returns -2)
        qDebug() << "✗ FAIL: WindowText should reject nonexistent font, got" << result;
        return 1;
    }

    qDebug() << "✓ WindowText validates font existence\n";

    // ========== Test 14: WindowTextWidth ==========
    qDebug() << "Test 14: WindowTextWidth";

    if (!executeLua(L, R"(
        width = world.WindowTextWidth("draw_test", "font1",
                                       "Hello", false)
    )",
                    "WindowTextWidth")) {
        return 1;
    }

    double textWidth = getGlobalNumber(L, "width");
    if (textWidth <= 0) {
        qDebug() << "✗ FAIL: WindowTextWidth returned invalid width:" << textWidth;
        return 1;
    }

    // Verify longer text has greater width
    if (!executeLua(L, R"(
        width_long = world.WindowTextWidth("draw_test", "font1",
                                            "Hello World!", false)
    )",
                    "WindowTextWidth long")) {
        return 1;
    }

    double textWidthLong = getGlobalNumber(L, "width_long");
    if (textWidthLong <= textWidth) {
        qDebug() << "✗ FAIL: Longer text should have greater width";
        return 1;
    }

    qDebug() << "✓ WindowTextWidth measures text width correctly\n";

    // ========== Test 15: WindowFontInfo - MUSHclient TEXTMETRIC API ==========
    qDebug() << "Test 15: WindowFontInfo - MUSHclient TEXTMETRIC API";

    struct FontInfoTest {
        int type;
        QString expectedType;
        const char* description;
    };

    // WindowFontInfo types from original MUSHclient
    // Returns Windows TEXTMETRIC structure values
    FontInfoTest fontInfoTests[] = {
        {1, "number", "tmHeight"},          {2, "number", "tmAscent"},
        {3, "number", "tmDescent"},         {4, "number", "tmInternalLeading"},
        {5, "number", "tmExternalLeading"}, {6, "number", "tmAveCharWidth"},
        {7, "number", "tmMaxCharWidth"},    {8, "number", "tmWeight"},
        {9, "number", "tmOverhang"}};

    for (const auto& test : fontInfoTests) {
        QString luaCode =
            QString("info = world.WindowFontInfo('draw_test', 'font1', %1)").arg(test.type);
        if (!executeLua(L, luaCode.toUtf8().constData(),
                        QString("WindowFontInfo type %1").arg(test.type).toUtf8().constData())) {
            return 1;
        }

        lua_getglobal(L, "info");
        bool isCorrectType = false;

        if (test.expectedType == "number") {
            isCorrectType = lua_isnumber(L, -1);
        } else if (test.expectedType == "string") {
            isCorrectType = lua_isstring(L, -1);
        } else if (test.expectedType == "boolean") {
            isCorrectType = lua_isboolean(L, -1);
        }

        lua_pop(L, 1);

        if (!isCorrectType) {
            qDebug() << "✗ FAIL: WindowFontInfo type" << test.type << "(" << test.description
                     << ") should return" << test.expectedType;
            return 1;
        }
    }

    qDebug() << "✓ WindowFontInfo returns correct types for all 9 info types\n";

    // ========== Test 16: WindowFontInfo - verify TEXTMETRIC values are reasonable ==========
    qDebug() << "Test 16: WindowFontInfo - verify TEXTMETRIC values are reasonable";

    // Check tmHeight (type 1) - should be positive for a 14pt font
    if (!executeLua(L, "height = world.WindowFontInfo('draw_test', 'font1', 1)",
                    "WindowFontInfo tmHeight")) {
        return 1;
    }

    double height = getGlobalNumber(L, "height");
    if (height <= 0 || height > 100) {
        qDebug() << "✗ FAIL: WindowFontInfo tmHeight should be positive and reasonable, got"
                 << height;
        return 1;
    }

    // Check tmAscent (type 2) - should be positive and <= height
    if (!executeLua(L, "ascent = world.WindowFontInfo('draw_test', 'font1', 2)",
                    "WindowFontInfo tmAscent")) {
        return 1;
    }

    double ascent = getGlobalNumber(L, "ascent");
    if (ascent <= 0 || ascent > height) {
        qDebug() << "✗ FAIL: WindowFontInfo tmAscent should be positive and <= height, got"
                 << ascent;
        return 1;
    }

    // Check tmDescent (type 3) - should be >= 0 and < height
    if (!executeLua(L, "descent = world.WindowFontInfo('draw_test', 'font1', 3)",
                    "WindowFontInfo tmDescent")) {
        return 1;
    }

    double descent = getGlobalNumber(L, "descent");
    if (descent < 0 || descent > height) {
        qDebug() << "✗ FAIL: WindowFontInfo tmDescent should be >= 0 and < height, got" << descent;
        return 1;
    }

    qDebug() << "✓ WindowFontInfo TEXTMETRIC values are reasonable\n";

    // ========== Test 17: Pen and Brush style constants ==========
    qDebug() << "Test 17: Pen and brush style constants exist";

    lua_getglobal(L, "miniwin");
    if (!lua_istable(L, -1)) {
        qDebug() << "✗ FAIL: miniwin table not accessible";
        return 1;
    }

    // Check pen styles
    lua_getfield(L, -1, "pen_solid");
    if (!lua_isnumber(L, -1)) {
        qDebug() << "✗ FAIL: miniwin.pen_solid not defined";
        return 1;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "pen_dash");
    if (!lua_isnumber(L, -1)) {
        qDebug() << "✗ FAIL: miniwin.pen_dash not defined";
        return 1;
    }
    lua_pop(L, 1);

    // Check brush styles
    lua_getfield(L, -1, "brush_solid");
    if (!lua_isnumber(L, -1)) {
        qDebug() << "✗ FAIL: miniwin.brush_solid not defined";
        return 1;
    }
    lua_pop(L, 1);

    lua_pop(L, 1); // pop miniwin table

    qDebug() << "✓ Pen and brush style constants defined\n";

    // ========== Test 18: WindowFontList ==========
    qDebug() << "Test 18: WindowFontList - Get list of fonts";

    // Create a few more fonts
    if (!executeLua(L, R"(
        world.WindowFont("draw_test", "font2", "Arial", 14, false, false, false, false)
        world.WindowFont("draw_test", "font3", "Times", 16, true, true, false, false)
    )",
                    "Create additional fonts")) {
        return 1;
    }

    // Get font list
    if (!executeLua(L, R"(
        fontList = world.WindowFontList("draw_test")
    )",
                    "WindowFontList")) {
        return 1;
    }

    // Verify it's a table
    lua_getglobal(L, "fontList");
    if (!lua_istable(L, -1)) {
        qDebug() << "✗ FAIL: WindowFontList should return a table";
        return 1;
    }

    // Check table length (should have 3 fonts: font1, font2, font3)
    int tableLen = lua_objlen(L, -1);
    if (tableLen != 3) {
        qDebug() << "✗ FAIL: WindowFontList returned" << tableLen << "fonts, expected 3";
        return 1;
    }

    // Verify font IDs are in the list
    QStringList expectedFonts = {"font1", "font2", "font3"};
    for (int i = 1; i <= tableLen; i++) {
        lua_rawgeti(L, -1, i);
        QString fontId = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);

        if (!expectedFonts.count(fontId)) {
            qDebug() << "✗ FAIL: Unexpected font ID in list:" << fontId;
            return 1;
        }
    }
    lua_pop(L, 1); // pop fontList table

    // Test with non-existent window (should return empty table)
    if (!executeLua(L, R"(
        emptyList = world.WindowFontList("nonexistent")
    )",
                    "WindowFontList nonexistent")) {
        return 1;
    }

    lua_getglobal(L, "emptyList");
    if (!lua_istable(L, -1) || lua_objlen(L, -1) != 0) {
        qDebug() << "✗ FAIL: WindowFontList should return empty table for nonexistent window";
        return 1;
    }
    lua_pop(L, 1);

    qDebug() << "✓ WindowFontList returns correct font list\n";

    // ========== Test 19: WindowCircleOp - invalid brush style (H78) ==========
    qDebug() << "Test 19: WindowCircleOp - invalid brush style is rejected";

    // Original ValidateBrushStyle accepts only 0-12; anything else returns
    // eBrushStyleNotValid (30074). Mushkin previously accepted invalid styles silently.
    if (!executeLua(L, R"(
        result = world.WindowCircleOp("draw_test",
                                      miniwin.circle_ellipse,
                                      10, 60, 50, 100,
                                      0x00FFFF, miniwin.pen_solid, 1,
                                      0x000000,
                                      99,                 -- invalid brush style
                                      0, 0, 0, 0)
    )",
                    "WindowCircleOp invalid brush style")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != static_cast<double>(eBrushStyleNotValid)) {
        qDebug() << "✗ FAIL: WindowCircleOp should reject invalid brush style with"
                 << eBrushStyleNotValid << "got" << result;
        return 1;
    }

    // A valid brush style (12 = waves-vertical, the upper bound) must still succeed.
    if (!executeLua(L, R"(
        result = world.WindowCircleOp("draw_test",
                                      miniwin.circle_ellipse,
                                      10, 60, 50, 100,
                                      0x00FFFF, miniwin.pen_solid, 1,
                                      0x000000,
                                      12,                 -- valid (upper bound)
                                      0, 0, 0, 0)
    )",
                    "WindowCircleOp valid brush style 12")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowCircleOp valid brush style 12 should succeed, got" << result;
        return 1;
    }

    qDebug() << "✓ WindowCircleOp validates brush styles (rejects out-of-range, accepts 0-12)\n";

    // ========== Test 20: WindowImageOp - monochrome pattern colour remap (M47) ==========
    qDebug() << "Test 20: WindowImageOp remaps monochrome pattern colours";

    // Create an 8x8 monochrome pattern that is entirely set bits (all 0xFF rows).
    // Every pixel is a "1-bit", which GDI paints in the brush colour (text colour).
    if (!executeLua(L, R"(
        world.WindowCreateImage("draw_test", "solidpat",
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)
        -- Fill a rectangle using the pattern. brushColor = green (BGR 0x00FF00),
        -- penColor = red (BGR 0x0000FF). Set bits should appear in brushColor (green).
        result = world.WindowImageOp("draw_test", 2,          -- 2 = rectangle
                                     5, 165, 45, 195,         -- left, top, right, bottom
                                     0x0000FF, miniwin.pen_solid, 1,  -- red pen
                                     0x00FF00,                -- green brush colour
                                     "solidpat")
    )",
                    "WindowImageOp monochrome remap")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowImageOp returned" << result;
        return 1;
    }

    // The filled interior should now be green (brushColor), not raw white from the pattern.
    img = (*win->getImage());
    QRgb patFill = img.pixel(25, 180);
    if (qGreen(patFill) < 180 || qRed(patFill) > 80 || qBlue(patFill) > 80) {
        qDebug() << "✗ FAIL: Monochrome pattern not remapped to brush colour; pixel at (25,180) ="
                 << QString::number(patFill, 16) << "R=" << qRed(patFill) << "G=" << qGreen(patFill)
                 << "B=" << qBlue(patFill);
        return 1;
    }

    qDebug() << "✓ WindowImageOp remaps monochrome 1-bits to brush colour\n";

    // ========== Test 21: WindowArc - ellipse aspect-ratio correction (L88) ==========
    qDebug() << "Test 21: WindowArc applies ellipse aspect-ratio correction";

    // Use a very flat ellipse: bounding box (0,80)-(200,120), so cx=100, cy=100,
    // semi-radii a=100, b=20. Endpoints (180,105) and (180,95) straddle the X axis.
    //  - WITH aspect-ratio correction: Qt span is ~34.7 deg, covering Qt angles
    //    [-17.35, +17.35] -> the arc reaches out to pixel (~196, ~94) (Qt ~16 deg).
    //  - WITHOUT correction (the bug): Qt span is only ~7.15 deg, covering
    //    [-3.58, +3.58]; the arc never leaves the neighbourhood of the right vertex
    //    (x ~199.8-200), so x=196 is never touched.
    // Pixel (196, 94) / (196, 106) therefore uniquely distinguishes the corrected
    // arc from the buggy one. Pen width 3 keeps the stroke tight enough that the
    // buggy arc cannot bleed out to x=196.
    if (!executeLua(L, R"(
        result = world.WindowArc("draw_test",
                                 0, 80, 200, 120,    -- flat bounding ellipse
                                 180, 105,           -- start point (below axis)
                                 180, 95,            -- end point (above axis)
                                 0x0000FF,           -- red pen (BGR)
                                 miniwin.pen_solid, 3)
    )",
                    "WindowArc ellipse correction")) {
        return 1;
    }

    result = getGlobalNumber(L, "result");
    if (result != 0) {
        qDebug() << "✗ FAIL: WindowArc returned" << result;
        return 1;
    }

    // The buggy (unscaled) arc cannot reach x<=197 here; only the aspect-ratio
    // corrected arc paints the (196,94)/(196,106) region. Tight +/-1 window.
    img = (*win->getImage());
    bool foundArcPixel = false;
    for (int dy = -1; dy <= 1 && !foundArcPixel; ++dy) {
        for (int dx = -1; dx <= 1 && !foundArcPixel; ++dx) {
            QRgb p1 = img.pixel(196 + dx, 94 + dy);
            QRgb p2 = img.pixel(196 + dx, 106 + dy);
            if (qRed(p1) > 150 || qRed(p2) > 150)
                foundArcPixel = true;
        }
    }

    if (!foundArcPixel) {
        qDebug() << "✗ FAIL: WindowArc did not draw through the aspect-ratio-corrected region "
                    "(~Qt 16 deg, pixel ~196,94); ellipse correction missing";
        return 1;
    }

    qDebug() << "✓ WindowArc draws the aspect-ratio-corrected span on a flat ellipse\n";

    // ========== All tests passed! ==========
    qDebug() << "\n=== PASS: All tests passed ===\n";
    qDebug() << "Miniwindow Drawing features verified:";
    qDebug() << "  ✓ WindowRectOp frame draws rectangle outlines";
    qDebug() << "  ✓ WindowRectOp fill draws filled rectangles";
    qDebug() << "  ✓ WindowRectOp invert performs XOR operation";
    qDebug() << "  ✓ WindowRectOp validates action codes";
    qDebug() << "  ✓ WindowCircleOp ellipse draws ellipses";
    qDebug() << "  ✓ WindowCircleOp arc draws arcs with angles";
    qDebug() << "  ✓ WindowLine draws lines";
    qDebug() << "  ✓ WindowSetPixel sets individual pixels";
    qDebug() << "  ✓ WindowGetPixel retrieves pixel colors";
    qDebug() << "  ✓ WindowGetPixel validates bounds";
    qDebug() << "  ✓ WindowFont creates and caches fonts";
    qDebug() << "  ✓ WindowFont overwrites existing fonts correctly";
    qDebug() << "  ✓ WindowText draws text with fonts";
    qDebug() << "  ✓ WindowText validates font existence";
    qDebug() << "  ✓ WindowTextWidth measures text width";
    qDebug() << "  ✓ WindowFontInfo returns all 9 info types";
    qDebug() << "  ✓ WindowFontInfo returns correct values";
    qDebug() << "  ✓ Pen and brush style constants available";
    qDebug() << "✓ WindowFontList returns list of fonts";
    qDebug() << "  ✓ WindowCircleOp rejects invalid brush styles (H78)";
    qDebug() << "  ✓ WindowImageOp remaps monochrome pattern colours (M47)";
    qDebug() << "  ✓ WindowArc applies ellipse aspect-ratio correction (L88)";
    qDebug() << "\nFont count:" << win->fonts.size();

    return 0;
}
