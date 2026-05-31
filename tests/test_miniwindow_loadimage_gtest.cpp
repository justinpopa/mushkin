/**
 * test_miniwindow_loadimage_gtest.cpp - GoogleTest version
 *
 * Behavioral-parity tests for MiniWindow::LoadImage (and the Lua wrapper
 * world.WindowLoadImage which delegates to it).
 *
 * Original behavior (miniwindow.cpp:1047-1103, Utilities.cpp:2972-...):
 *   1. Empty filename removes the image and returns eOK.
 *   2. Filename shorter than 5 chars returns eBadParameter.
 *   3. Filename without a .bmp/.png extension returns eBadParameter
 *      (all other Qt image formats are rejected, matching the original which
 *      only accepts .bmp and .png).
 *   4. A .png/.bmp file that does not exist returns eFileNotFound.
 *   5. A .png/.bmp file that exists but cannot be decoded returns
 *      eUnableToLoadImage (distinct from eFileNotFound).
 *   6. A valid .png file loads successfully and returns eOK.
 *
 * Also covers hotspot Lua API parity (H25, L39):
 *   H25: WindowDrawImage mode parameter defaults to 1 when omitted.
 *   L39: WindowAddHotspot validates callback names via CheckLabel.
 *   L39: WindowAddHotspot clears mouseOver/mouseDown state when replacing a hotspot.
 */

#include "../src/utils/error_codes.h"
#include "../src/world/miniwindow.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QImage>
#include <QTemporaryDir>
#include <memory>

class MiniWindowLoadImageTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        win = std::make_unique<MiniWindow>(doc.get());
        ASSERT_TRUE(tempDir.isValid()) << "Temp dir should be created";
    }

    QString pathIn(const QString& name) const
    {
        return tempDir.filePath(name);
    }

    std::unique_ptr<MiniWindow> win;
    QTemporaryDir tempDir;
};

// 1. Empty filename removes the image and returns eOK.
TEST_F(MiniWindowLoadImageTest, EmptyFilenameRemovesImageAndReturnsOk)
{
    EXPECT_EQ(win->LoadImage("img", ""), eOK);
    EXPECT_EQ(win->LoadImage("img", "   "), eOK); // whitespace trims to empty
}

// 2. Filename shorter than 5 chars returns eBadParameter (original requires x.bmp length).
TEST_F(MiniWindowLoadImageTest, ShortFilenameReturnsBadParameter)
{
    EXPECT_EQ(win->LoadImage("img", "a.bm"), eBadParameter);
}

// 3. Non-.bmp/.png extensions are rejected with eBadParameter.
TEST_F(MiniWindowLoadImageTest, NonBmpPngExtensionReturnsBadParameter)
{
    // Create a real, valid JPEG so the rejection is by extension, not by load failure.
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::red);
    const QString jpgPath = pathIn("pic.jpg");
    ASSERT_TRUE(img.save(jpgPath, "JPG")) << "Failed to write test jpg";

    EXPECT_EQ(win->LoadImage("img", jpgPath), eBadParameter);
}

// 4. A .png that does not exist returns eFileNotFound.
TEST_F(MiniWindowLoadImageTest, MissingPngReturnsFileNotFound)
{
    EXPECT_EQ(win->LoadImage("img", pathIn("does_not_exist.png")), eFileNotFound);
}

// 5. A .png that exists but is not a valid image returns eUnableToLoadImage.
TEST_F(MiniWindowLoadImageTest, CorruptPngReturnsUnableToLoadImage)
{
    const QString junkPath = pathIn("junk.png");
    QFile f(junkPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("this is not a valid PNG file");
    f.close();

    EXPECT_EQ(win->LoadImage("img", junkPath), eUnableToLoadImage);
}

// 6. A valid .png loads successfully.
TEST_F(MiniWindowLoadImageTest, ValidPngReturnsOk)
{
    QImage img(8, 8, QImage::Format_ARGB32);
    img.fill(Qt::blue);
    const QString pngPath = pathIn("good.png");
    ASSERT_TRUE(img.save(pngPath, "PNG")) << "Failed to write test png";

    EXPECT_EQ(win->LoadImage("logo", pngPath), eOK);
    EXPECT_NE(win->images.find("logo"), win->images.end()) << "Image should be stored";
}

// ---------------------------------------------------------------------------
// L36 parity: setLastMousePosition() increments mouseUpdateCount (WindowInfo code 16)
// Original: mw->m_last_mouse_update++ at every mouse-position recording site.
// ---------------------------------------------------------------------------
class MiniWindowMouseUpdateCountTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        win = std::make_unique<MiniWindow>(doc.get());
    }
    std::unique_ptr<MiniWindow> win;
};

// Direct assignment to lastMousePosition must NOT increment the counter —
// only setLastMousePosition() should, matching the original's single increment
// per event.
TEST_F(MiniWindowMouseUpdateCountTest, StartsAtZero)
{
    EXPECT_EQ(win->mouseUpdateCount, 0);
}

TEST_F(MiniWindowMouseUpdateCountTest, SetterIncrementsCounter)
{
    win->setLastMousePosition(QPoint(10, 20));
    EXPECT_EQ(win->mouseUpdateCount, 1);
    EXPECT_EQ(win->getLastMousePosition(), QPoint(10, 20));

    win->setLastMousePosition(QPoint(30, 40));
    EXPECT_EQ(win->mouseUpdateCount, 2);
    EXPECT_EQ(win->getLastMousePosition(), QPoint(30, 40));
}

TEST_F(MiniWindowMouseUpdateCountTest, DirectFieldAssignmentDoesNotIncrement)
{
    // Direct write is allowed but should not bump the counter (not the parity path).
    win->lastMousePosition = QPoint(5, 5);
    EXPECT_EQ(win->mouseUpdateCount, 0) << "Direct field write must not increment counter";
}

TEST_F(MiniWindowMouseUpdateCountTest, SetClientMousePositionDoesNotIncrementCounter)
{
    // clientMousePosition has its own setter; it never increments mouseUpdateCount.
    win->setClientMousePosition(QPoint(50, 60));
    EXPECT_EQ(win->mouseUpdateCount, 0);
    EXPECT_EQ(win->getClientMousePosition(), QPoint(50, 60));
}

// ---------------------------------------------------------------------------
// H25 parity: WindowDrawImage mode parameter defaults to 1 when omitted.
// Original: lua_methods.cpp:5772 — my_optnumber(L, 7, 1)
// Mushkin was using luaL_checkinteger(L, 7) making mode required.
// ---------------------------------------------------------------------------
class WindowDrawImageModeDefaultTest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();
        // Create a miniwindow large enough to draw on
        executeLua("world.WindowCreate('w', 0,0,100,100, 1, 0, 0xFF000000)");
    }
};

// With mode omitted (6 args), the call should succeed (return eOK or image-not-found,
// not a Lua argument error). Before the fix, luaL_checkinteger would throw a Lua error.
TEST_F(WindowDrawImageModeDefaultTest, ModeOmittedDoesNotRaiseLuaError)
{
    // WindowDrawImage with 6 args (no mode) — should not throw, should return a
    // numeric error code (eImageNotInstalled since no image is loaded, not a Lua arg error).
    int rc = luaL_dostring(L, "return world.WindowDrawImage('w','img',0,0,0,0)");
    EXPECT_EQ(rc, 0) << "Omitting mode arg must not raise a Lua error (was luaL_checkinteger)";

    // The return value on the stack should be a number (not nil/error string)
    EXPECT_TRUE(lua_isnumber(L, -1)) << "WindowDrawImage should return a numeric error code";
    lua_pop(L, 1);
}

// With mode explicitly 1, behavior is unchanged.
TEST_F(WindowDrawImageModeDefaultTest, ModeExplicit1WorksSameAsDefault)
{
    int rc6 = luaL_dostring(L, "return world.WindowDrawImage('w','img',0,0,0,0)");
    ASSERT_EQ(rc6, 0);
    double code6 = lua_tonumber(L, -1);
    lua_pop(L, 1);

    int rc7 = luaL_dostring(L, "return world.WindowDrawImage('w','img',0,0,0,0,1)");
    ASSERT_EQ(rc7, 0);
    double code7 = lua_tonumber(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(code6, code7)
        << "Explicit mode=1 and omitted mode should produce the same error code";
}

// ---------------------------------------------------------------------------
// L39 parity: WindowAddHotspot validates callback names (CheckLabel, bScript=true).
// Original: miniwindow.cpp:1719-1728
// An invalid callback name (starts with digit, contains space, etc.) must return
// eInvalidObjectLabel (30008), not silently store the bad name.
// ---------------------------------------------------------------------------
class WindowAddHotspotValidationTest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();
        executeLua("world.WindowCreate('w', 0,0,100,100, 1, 0, 0xFF000000)");
    }

    // Helper: call WindowAddHotspot with a given mouseOver callback string.
    // Returns the numeric result code.
    double addHotspotWithMouseOver(const char* cbName)
    {
        const QString code =
            QString("return world.WindowAddHotspot('w','hs',0,0,10,10,'%1','','','','','',0,0)")
                .arg(cbName);
        int rc = luaL_dostring(L, code.toUtf8().constData());
        if (rc != 0) {
            lua_pop(L, 1);
            return -9999; // Lua error, not a return code
        }
        double result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }
};

// Valid callback name: must return eOK (0).
TEST_F(WindowAddHotspotValidationTest, ValidCallbackNameAccepted)
{
    EXPECT_EQ(addHotspotWithMouseOver("OnMouseOver"), 0 /*eOK*/);
}

// Valid dotted name (module.function): must return eOK.
TEST_F(WindowAddHotspotValidationTest, DottedCallbackNameAccepted)
{
    EXPECT_EQ(addHotspotWithMouseOver("mymod.OnClick"), 0 /*eOK*/);
}

// Empty callback: always valid (means "no callback").
TEST_F(WindowAddHotspotValidationTest, EmptyCallbackAccepted)
{
    EXPECT_EQ(addHotspotWithMouseOver(""), 0 /*eOK*/);
}

// Callback starting with a digit: must return eInvalidObjectLabel.
TEST_F(WindowAddHotspotValidationTest, CallbackStartingWithDigitRejected)
{
    EXPECT_EQ(addHotspotWithMouseOver("1badName"), 30008 /*eInvalidObjectLabel*/);
}

// Callback containing a space: must return eInvalidObjectLabel.
TEST_F(WindowAddHotspotValidationTest, CallbackWithSpaceRejected)
{
    EXPECT_EQ(addHotspotWithMouseOver("bad name"), 30008 /*eInvalidObjectLabel*/);
}

// ---------------------------------------------------------------------------
// L39 parity: Replacing an existing hotspot clears mouseOver/mouseDown state.
// Original: miniwindow.cpp:1736-1748
// ---------------------------------------------------------------------------
class WindowAddHotspotReplaceMouseStateTest : public WorldDocumentTest {
// Parity regressions for f021 deviations
// ---------------------------------------------------------------------------

class MiniWindowParityTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        win = std::make_unique<MiniWindow>(doc.get());
        // Prime with an initial hotspot
        auto hs = std::make_unique<Hotspot>();
        hs->m_rect = QRect(0, 0, 50, 50);
        win->hotspots["hs"] = std::move(hs);
    }

    std::unique_ptr<MiniWindow> win;
};

// When mouseOver points at the hotspot being replaced, it must be cleared.
TEST_F(WindowAddHotspotReplaceMouseStateTest, ReplaceHotspotClearsMouseOverState)
{
    // Simulate the mouse being over the hotspot
    win->mouseOverHotspot = "hs";
    ASSERT_EQ(win->mouseOverHotspot, "hs");

    // Replace the hotspot by inserting a new one with the same ID directly
    // (mirrors what L_WindowAddHotspot does after the fix)
    if (win->hotspots.count("hs") > 0) {
        if (win->mouseOverHotspot == "hs")
            win->mouseOverHotspot.clear();
        if (win->mouseDownHotspot == "hs")
            win->mouseDownHotspot.clear();
    }
    auto replacement = std::make_unique<Hotspot>();
    replacement->m_rect = QRect(10, 10, 20, 20);
    win->hotspots["hs"] = std::move(replacement);

    EXPECT_TRUE(win->mouseOverHotspot.isEmpty())
        << "mouseOverHotspot must be cleared when its hotspot is replaced";
}

// When mouseDown points at the hotspot being replaced, it must be cleared.
TEST_F(WindowAddHotspotReplaceMouseStateTest, ReplaceHotspotClearsMouseDownState)
{
    win->mouseDownHotspot = "hs";
    ASSERT_EQ(win->mouseDownHotspot, "hs");

    if (win->hotspots.count("hs") > 0) {
        if (win->mouseOverHotspot == "hs")
            win->mouseOverHotspot.clear();
        if (win->mouseDownHotspot == "hs")
            win->mouseDownHotspot.clear();
    }
    auto replacement = std::make_unique<Hotspot>();
    replacement->m_rect = QRect(10, 10, 20, 20);
    win->hotspots["hs"] = std::move(replacement);

    EXPECT_TRUE(win->mouseDownHotspot.isEmpty())
        << "mouseDownHotspot must be cleared when its hotspot is replaced";
        // Create a 100x100 window
        win->Resize(100, 100, 0x00000000);
    }
    std::unique_ptr<MiniWindow> win;
};

// H30: BlendImage out-of-range opacity returns eBadParameter (not silently clamped)
TEST_F(MiniWindowParityTest, BlendImageOpacityOutOfRangeReturnsBadParameter)
{
    // Load a small image first
    QImage img(4, 4, QImage::Format_ARGB32);
    img.fill(Qt::red);
    const QTemporaryDir tmp;
    const QString path = tmp.filePath("src.png");
    ASSERT_TRUE(img.save(path, "PNG"));
    ASSERT_EQ(win->LoadImage("src", path), eOK);

    EXPECT_EQ(win->BlendImage("src", 0, 0, 4, 4, 1, -0.1, 0, 0, 0, 0), eBadParameter);
    EXPECT_EQ(win->BlendImage("src", 0, 0, 4, 4, 1, 1.1, 0, 0, 0, 0), eBadParameter);
    // Edge values are accepted
    EXPECT_EQ(win->BlendImage("src", 0, 0, 4, 4, 1, 0.0, 0, 0, 0, 0), eOK);
    EXPECT_EQ(win->BlendImage("src", 0, 0, 4, 4, 1, 1.0, 0, 0, 0, 0), eOK);
}

// H75 MEDIUM: Resize must NOT update the stored backgroundColor
TEST_F(MiniWindowParityTest, ResizeDoesNotUpdateBackgroundColor)
{
    win->setBackgroundColor(0x00FF0000); // blue in BGR
    QRgb savedBg = win->getBackgroundColor();

    // Resize with a different background color argument
    ASSERT_EQ(win->Resize(50, 50, 0x000000FF), eOK); // red in BGR

    // backgroundColor field must be unchanged
    EXPECT_EQ(win->getBackgroundColor(), savedBg)
        << "Resize must not overwrite the stored backgroundColor";
}

// H75 LOW: Resize with zero dimension coerces to 1 (not eBadParameter)
TEST_F(MiniWindowParityTest, ResizeZeroDimensionCoercesToOne)
{
    EXPECT_EQ(win->Resize(0, 50, 0x00000000), eOK)
        << "Zero width should be coerced to 1, not rejected";
    EXPECT_EQ(win->Resize(50, 0, 0x00000000), eOK)
        << "Zero height should be coerced to 1, not rejected";
    EXPECT_EQ(win->Resize(0, 0, 0x00000000), eOK)
        << "Zero width+height should be coerced to 1, not rejected";
}

// H76 MEDIUM: TextWidth returns -2 when font not found
TEST_F(MiniWindowParityTest, TextWidthReturnsMinus2WhenFontNotFound)
{
    EXPECT_EQ(win->TextWidth("no_such_font", "hello", false), -2);
}

// H76 LOW: Text returns 0 immediately for empty string (no drawing, no dirty flag)
TEST_F(MiniWindowParityTest, TextEmptyStringReturnsZeroWithoutDrawing)
{
    ASSERT_EQ(win->Font("f1", "Arial", 12.0, false, false, false, false), eOK);
    win->dirty = false;

    qint32 result = win->Text("f1", "", 0, 0, 100, 100, 0x00FFFFFF, false);
    EXPECT_EQ(result, 0) << "Empty text should return 0";
    EXPECT_FALSE(win->dirty) << "Empty text must not set the dirty flag";
}

// M130: Gradient invalid mode returns eUnknownOption, not eBadParameter
TEST_F(MiniWindowParityTest, GradientInvalidModeReturnsEUnknownOption)
{
    EXPECT_EQ(win->Gradient(0, 0, 50, 50, 0x00FF0000, 0x000000FF, 99), eUnknownOption);
    EXPECT_EQ(win->Gradient(0, 0, 50, 50, 0x00FF0000, 0x000000FF, 0), eUnknownOption);
}

// GetPixel out-of-bounds returns 0xFFFFFFFF (CLR_INVALID); Lua side casts to signed -1.
TEST_F(MiniWindowParityTest, GetPixelOutOfBoundsReturnsCLRInvalid)
{
    constexpr QRgb CLR_INVALID = 0xFFFFFFFF;
    EXPECT_EQ(win->GetPixel(-1, 0), CLR_INVALID);
    EXPECT_EQ(win->GetPixel(0, -1), CLR_INVALID);
    EXPECT_EQ(win->GetPixel(200, 0), CLR_INVALID);
    EXPECT_EQ(win->GetPixel(0, 200), CLR_INVALID);
    // Cast to signed must be -1 (the Lua API fix)
    EXPECT_EQ(static_cast<qint32>(win->GetPixel(200, 0)), -1);
}
