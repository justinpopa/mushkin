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
 */

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
