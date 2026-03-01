/**
 * world_miniwindow_images.cpp - Miniwindow Image and Hotspot Lua API Functions
 *
 * Image loading, drawing, blending, alpha operations, transformations,
 * and interactive hotspot management (add, delete, move, drag, scroll,
 * menus, and info queries).
 *
 * Extracted from world_miniwindows.cpp for better code organization.
 */

#include "../../world/miniwindow.h"
#include "../../world/view_interfaces.h"
#include "../../world/world_document.h"
#include "logging.h"
#include "lua_common.h"
#include <QMenu>
#include <QStack>
#include <span>

// Helper to get MiniWindow from map
static inline MiniWindow* getMiniWindow(WorldDocument* doc, const QString& name)
{
    auto it = doc->m_MiniWindowMap.find(name);
    return (it != doc->m_MiniWindowMap.end() && it->second) ? it->second.get() : nullptr;
}

/**
 * world.WindowLoadImage(name, imageId, filename)
 *
 * Loads an image file into a miniwindow for later drawing.
 * Supports PNG, BMP, JPG, GIF, and other common formats.
 * Pass an empty filename to remove a previously loaded image.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Unique ID to reference this image
 * @param filename (string) Path to image file, or "" to remove
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eFileNotFound (30051): Image file not found or unreadable
 *
 * @example
 * -- Load an image
 * WindowLoadImage("mywin", "logo", "C:/Images/logo.png")
 * WindowDrawImage("mywin", "logo", 10, 10, 0, 0, 1)
 *
 * -- Remove the image to free memory
 * WindowLoadImage("mywin", "logo", "")
 *
 * @see WindowDrawImage, WindowImageInfo, WindowImageList, WindowLoadImageMemory
 */
int L_WindowLoadImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* filename = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    QString qImageId = QString::fromUtf8(imageId);
    QString qFilename = QString::fromUtf8(filename);

    // Get miniwindow
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Load the image file using Qt
    qFilename = qFilename.trimmed();

    // Empty filename means remove the image
    if (qFilename.isEmpty()) {
        // Erase from map (unique_ptr automatically deletes)
        win->images.erase(qImageId);
        return luaReturnOK(L);
    }

    // Load the image as a QImage
    auto image = std::make_unique<QImage>(qFilename);
    if (image->isNull()) {
        lua_pushnumber(L, 30051); // eFileNotFound
        return 1;
    }

    // Store in miniwindow's image map (replaces old one if it exists; old unique_ptr automatically
    // deletes)
    win->images[qImageId] = std::move(image);

    return luaReturnOK(L);
}

/**
 * world.WindowDrawImage(name, imageId, left, top, right, bottom, mode, srcLeft, srcTop, srcRight,
 *                       srcBottom)
 *
 * Draws a loaded image to the miniwindow with optional scaling and
 * sprite sheet support. Source coordinates allow drawing a portion.
 *
 * Draw modes:
 *   1 = Copy (opaque)
 *   2 = Transparent copy (top-left pixel color is transparent)
 *   3 = Stretch to fit destination rectangle
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (from WindowLoadImage)
 * @param left (number) Destination left
 * @param top (number) Destination top
 * @param right (number) Destination right (0 = use image width)
 * @param bottom (number) Destination bottom (0 = use image height)
 * @param mode (number) Draw mode (1-3)
 * @param srcLeft (number) Source left (default 0)
 * @param srcTop (number) Source top (default 0)
 * @param srcRight (number) Source right (default 0 = full width)
 * @param srcBottom (number) Source bottom (default 0 = full height)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eImageNotInstalled: Image not found
 *
 * @example
 * -- Draw image at position
 * WindowLoadImage("mywin", "bg", "background.png")
 * WindowDrawImage("mywin", "bg", 0, 0, 0, 0, 1)
 *
 * -- Draw a sprite from a sheet (extract 32x32 tile at row 2, col 3)
 * WindowDrawImage("mywin", "sheet", 10, 10, 42, 42, 1, 64, 32, 96, 64)
 *
 * @see WindowLoadImage, WindowBlendImage, WindowDrawImageAlpha
 */
int L_WindowDrawImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint16 mode = luaL_checkinteger(L, 7);
    qint32 srcLeft = luaL_optinteger(L, 8, 0);
    qint32 srcTop = luaL_optinteger(L, 9, 0);
    qint32 srcRight = luaL_optinteger(L, 10, 0);
    qint32 srcBottom = luaL_optinteger(L, 11, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->DrawImage(QString::fromUtf8(imageId), left, top, right, bottom, mode,
                                   srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowBlendImage(name, imageId, left, top, right, bottom, mode, opacity, srcLeft, srcTop,
 *                        srcRight, srcBottom)
 *
 * Draws an image with opacity and blend modes for visual effects.
 *
 * Blend modes:
 *   1 = Normal (alpha blend)
 *   2 = Multiply (darkens)
 *   3 = Screen (lightens)
 *   4 = Overlay (contrast)
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param left (number) Destination left
 * @param top (number) Destination top
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param mode (number) Blend mode (1-4)
 * @param opacity (number) Opacity from 0.0 (transparent) to 1.0 (opaque)
 * @param srcLeft (number) Source left (default 0)
 * @param srcTop (number) Source top (default 0)
 * @param srcRight (number) Source right (default 0)
 * @param srcBottom (number) Source bottom (default 0)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw semi-transparent image
 * WindowBlendImage("mywin", "overlay", 0, 0, 0, 0, 1, 0.5, 0, 0, 0, 0)
 *
 * -- Apply darkening effect with multiply blend
 * WindowBlendImage("mywin", "shadow", 0, 0, 0, 0, 2, 0.7, 0, 0, 0, 0)
 *
 * @see WindowDrawImage, WindowDrawImageAlpha, WindowMergeImageAlpha
 */
int L_WindowBlendImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint16 mode = luaL_checkinteger(L, 7);
    double opacity = luaL_checknumber(L, 8);
    qint32 srcLeft = luaL_optinteger(L, 9, 0);
    qint32 srcTop = luaL_optinteger(L, 10, 0);
    qint32 srcRight = luaL_optinteger(L, 11, 0);
    qint32 srcBottom = luaL_optinteger(L, 12, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->BlendImage(QString::fromUtf8(imageId), left, top, right, bottom, mode,
                                    opacity, srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageFromWindow(name, imageId, srcWindowName)
 *
 * Copies the contents of another miniwindow as an image.
 * Useful for double-buffering or creating window snapshots.
 *
 * @param name (string) Destination miniwindow name
 * @param imageId (string) Image identifier to store under
 * @param srcWindowName (string) Source miniwindow name to copy from
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Destination window doesn't exist
 *
 * @example
 * -- Create a snapshot of another window
 * WindowImageFromWindow("mywin", "snapshot", "otherwin")
 * WindowDrawImage("mywin", "snapshot", 0, 0, 0, 0, 1)
 *
 * @see WindowLoadImage, WindowDrawImage
 */
int L_WindowImageFromWindow(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* srcWindowName = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->ImageFromWindow(QString::fromUtf8(imageId), pDoc, QString::fromUtf8(srcWindowName));
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageInfo(name, imageId, infoType)
 *
 * Returns information about a loaded image.
 *
 * Info types:
 *   1 = Image width in pixels
 *   2 = Image height in pixels
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param infoType (number) Type of information (1 or 2)
 *
 * @return (number) Requested info, or nil if image/window not found
 *
 * @example
 * -- Get image dimensions
 * local width = WindowImageInfo("mywin", "logo", 1)
 * local height = WindowImageInfo("mywin", "logo", 2)
 * Note("Image size: " .. width .. "x" .. height)
 *
 * @see WindowLoadImage, WindowImageList
 */
int L_WindowImageInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 infoType = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    QVariant result = win->ImageInfo(QString::fromUtf8(imageId), infoType);
    if (!result.isValid()) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, result.toInt());
    }
    return 1;
}

/**
 * world.WindowImageList(name)
 *
 * Returns a table of all loaded image IDs in a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (table) Array of image ID strings (1-indexed), or empty table
 *
 * @example
 * -- List all images in the window
 * local images = WindowImageList("mywin")
 * for i, id in ipairs(images) do
 *     local w = WindowImageInfo("mywin", id, 1)
 *     local h = WindowImageInfo("mywin", id, 2)
 *     Note("Image: " .. id .. " (" .. w .. "x" .. h .. ")")
 * end
 *
 * @see WindowLoadImage, WindowImageInfo
 */
int L_WindowImageList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_newtable(L); // Return empty table
        return 1;
    }

    QStringList imageIds = win->ImageList();

    // Create Lua table
    lua_newtable(L);
    for (int i = 0; i < imageIds.size(); ++i) {
        lua_pushstring(L, imageIds[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1); // Lua tables are 1-indexed
    }

    return 1;
}

/**
 * world.WindowGetImageAlpha(name, imageId, left, top, right, bottom, srcLeft, srcTop)
 *
 * Extracts the alpha channel from a 32-bit image and draws it
 * as a grayscale image (white = opaque, black = transparent).
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (must have alpha channel)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Visualize the alpha channel of an image
 * WindowGetImageAlpha("mywin", "sprite", 0, 0, 0, 0, 0, 0)
 *
 * @see WindowDrawImageAlpha, WindowMergeImageAlpha
 */
int L_WindowGetImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint32 srcLeft = luaL_checkinteger(L, 7);
    qint32 srcTop = luaL_checkinteger(L, 8);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->GetImageAlpha(QString::fromUtf8(imageId), left, top, right, bottom, srcLeft, srcTop);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowDrawImageAlpha(name, imageId, left, top, right, bottom, opacity, srcLeft, srcTop)
 *
 * Draws a 32-bit image using its embedded alpha channel for transparency.
 * An additional opacity multiplier can be applied.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier (must have alpha channel)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param opacity (number) Additional opacity (0.0 = transparent, 1.0 = use image alpha)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Draw PNG with transparency
 * WindowLoadImage("mywin", "icon", "icon.png")
 * WindowDrawImageAlpha("mywin", "icon", 10, 10, 0, 0, 1.0, 0, 0)
 *
 * -- Draw at 50% opacity
 * WindowDrawImageAlpha("mywin", "icon", 10, 10, 0, 0, 0.5, 0, 0)
 *
 * @see WindowGetImageAlpha, WindowMergeImageAlpha, WindowBlendImage
 */
int L_WindowDrawImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    double opacity = luaL_checknumber(L, 7);
    qint32 srcLeft = luaL_checkinteger(L, 8);
    qint32 srcTop = luaL_checkinteger(L, 9);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->DrawImageAlpha(QString::fromUtf8(imageId), left, top, right, bottom,
                                        opacity, srcLeft, srcTop);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowMergeImageAlpha(name, imageId, maskId, left, top, right, bottom, mode, opacity,
 *                             srcLeft, srcTop, srcRight, srcBottom)
 *
 * Draws an image using a separate grayscale image as an alpha mask.
 * White areas in the mask are opaque, black areas are transparent.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Main image identifier
 * @param maskId (string) Mask image identifier (grayscale)
 * @param left (number) Destination left coordinate
 * @param top (number) Destination top coordinate
 * @param right (number) Destination right (0 = window width)
 * @param bottom (number) Destination bottom (0 = window height)
 * @param mode (number) Blend mode (0 = normal, 1 = color key)
 * @param opacity (number) Additional opacity (0.0-1.0)
 * @param srcLeft (number) Source left coordinate
 * @param srcTop (number) Source top coordinate
 * @param srcRight (number) Source right coordinate
 * @param srcBottom (number) Source bottom coordinate
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Apply a mask to an image
 * WindowLoadImage("mywin", "photo", "photo.jpg")
 * WindowLoadImage("mywin", "mask", "mask.png")
 * WindowMergeImageAlpha("mywin", "photo", "mask", 0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0)
 *
 * @see WindowDrawImageAlpha, WindowGetImageAlpha
 */
int L_WindowMergeImageAlpha(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    const char* maskId = luaL_checkstring(L, 3);
    qint32 left = luaL_checkinteger(L, 4);
    qint32 top = luaL_checkinteger(L, 5);
    qint32 right = luaL_checkinteger(L, 6);
    qint32 bottom = luaL_checkinteger(L, 7);
    qint16 mode = luaL_checkinteger(L, 8);
    double opacity = luaL_checknumber(L, 9);
    qint32 srcLeft = luaL_checkinteger(L, 10);
    qint32 srcTop = luaL_checkinteger(L, 11);
    qint32 srcRight = luaL_checkinteger(L, 12);
    qint32 srcBottom = luaL_checkinteger(L, 13);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->MergeImageAlpha(QString::fromUtf8(imageId), QString::fromUtf8(maskId), left, top,
                             right, bottom, mode, opacity, srcLeft, srcTop, srcRight, srcBottom);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowTransformImage(name, imageId, left, top, mode, mxx, mxy, myx, myy)
 *
 * Applies an affine transformation to an image (rotate, scale, skew).
 * Uses a 2x2 transformation matrix.
 *
 * Common transformations:
 *   Rotation by θ: mxx=cos(θ), mxy=sin(θ), myx=-sin(θ), myy=cos(θ)
 *   Scale by s: mxx=s, mxy=0, myx=0, myy=s
 *   Horizontal flip: mxx=-1, mxy=0, myx=0, myy=1
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param left (number) X translation offset (destination position)
 * @param top (number) Y translation offset (destination position)
 * @param mode (number) Draw mode (1 = opaque, 3 = transparent)
 * @param mxx (number) Matrix element [0][0]
 * @param mxy (number) Matrix element [0][1]
 * @param myx (number) Matrix element [1][0]
 * @param myy (number) Matrix element [1][1]
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Rotate image 45 degrees
 * local angle = math.rad(45)
 * local cos, sin = math.cos(angle), math.sin(angle)
 * WindowTransformImage("mywin", "img", 100, 100, 1, cos, sin, -sin, cos)
 *
 * -- Scale image to 50%
 * WindowTransformImage("mywin", "img", 0, 0, 1, 0.5, 0, 0, 0.5)
 *
 * @see WindowDrawImage, WindowBlendImage
 */
int L_WindowTransformImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    float left = luaL_checknumber(L, 3);
    float top = luaL_checknumber(L, 4);
    qint16 mode = luaL_checkinteger(L, 5);
    float mxx = luaL_checknumber(L, 6);
    float mxy = luaL_checknumber(L, 7);
    float myx = luaL_checknumber(L, 8);
    float myy = luaL_checknumber(L, 9);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result =
        win->TransformImage(QString::fromUtf8(imageId), left, top, mode, mxx, mxy, myx, myy);
    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowCreateImage(name, imageId, row1, row2, row3, row4, row5, row6, row7, row8)
 *
 * Creates an 8x8 monochrome image from row bit patterns.
 * Each row is 8 bits where bit 7 is the leftmost pixel.
 * Useful for creating small icons or patterns programmatically.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param row1 (number) Bit pattern for row 1 (0-255)
 * @param row2 (number) Bit pattern for row 2
 * @param row3 (number) Bit pattern for row 3
 * @param row4 (number) Bit pattern for row 4
 * @param row5 (number) Bit pattern for row 5
 * @param row6 (number) Bit pattern for row 6
 * @param row7 (number) Bit pattern for row 7
 * @param row8 (number) Bit pattern for row 8
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a simple arrow pattern
 * WindowCreateImage("mywin", "arrow",
 *     0x18,  -- 00011000
 *     0x3C,  -- 00111100
 *     0x7E,  -- 01111110
 *     0xFF,  -- 11111111
 *     0x18,  -- 00011000
 *     0x18,  -- 00011000
 *     0x18,  -- 00011000
 *     0x18)  -- 00011000
 *
 * @see WindowLoadImage, WindowImageOp
 */
int L_WindowCreateImage(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);
    qint32 row1 = luaL_checkinteger(L, 3);
    qint32 row2 = luaL_checkinteger(L, 4);
    qint32 row3 = luaL_checkinteger(L, 5);
    qint32 row4 = luaL_checkinteger(L, 6);
    qint32 row5 = luaL_checkinteger(L, 7);
    qint32 row6 = luaL_checkinteger(L, 8);
    qint32 row7 = luaL_checkinteger(L, 9);
    qint32 row8 = luaL_checkinteger(L, 10);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->CreateImage(QString::fromUtf8(imageId), row1, row2, row3, row4, row5, row6,
                                     row7, row8);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowImageOp(name, action, left, top, right, bottom, penColor, penStyle, penWidth,
 *                     brushColor, imageId, ellipseWidth, ellipseHeight)
 *
 * Draws shapes using an image as a brush pattern for fills.
 *
 * Action codes:
 *   1 = Frame rectangle
 *   2 = Fill rectangle with image pattern
 *   3 = Rounded rectangle
 *   4 = Ellipse frame
 *   5 = Filled ellipse with image pattern
 *
 * @param name (string) Miniwindow name
 * @param action (number) Drawing operation (1-5)
 * @param left (number) Left coordinate
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate
 * @param bottom (number) Bottom coordinate
 * @param penColor (number) Outline color (BGR)
 * @param penStyle (number) Pen style (0=solid, etc.)
 * @param penWidth (number) Pen width in pixels
 * @param brushColor (number) Background color (BGR)
 * @param imageId (string) Image ID for brush pattern
 * @param ellipseWidth (number) Corner width for rounded rect (optional)
 * @param ellipseHeight (number) Corner height for rounded rect (optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a pattern image then use it as brush
 * WindowCreateImage("mywin", "dots", 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55)
 * WindowImageOp("mywin", 2, 10, 10, 100, 100, 0, 0, 0, 0xFFFFFF, "dots")
 *
 * @see WindowCreateImage, WindowRectOp, WindowCircleOp
 */
int L_WindowImageOp(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint16 action = luaL_checkinteger(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    qint32 penColor = luaL_checkinteger(L, 7);
    qint32 penStyle = luaL_checkinteger(L, 8);
    qint32 penWidth = luaL_checkinteger(L, 9);
    qint32 brushColor = luaL_checkinteger(L, 10);
    const char* imageId = luaL_checkstring(L, 11);
    qint32 ellipseWidth = luaL_optinteger(L, 12, 0);
    qint32 ellipseHeight = luaL_optinteger(L, 13, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->ImageOp(action, left, top, right, bottom, static_cast<QRgb>(penColor),
                                 penStyle, penWidth, static_cast<QRgb>(brushColor),
                                 QString::fromUtf8(imageId), ellipseWidth, ellipseHeight);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowLoadImageMemory(name, imageId, data, alpha)
 *
 * Loads an image from a string containing raw image data.
 * Supports PNG, BMP, JPG, GIF and other common formats.
 * Useful for loading embedded images or images from network.
 *
 * @param name (string) Miniwindow name
 * @param imageId (string) Image identifier
 * @param data (string) Binary string containing image data
 * @param alpha (boolean) true to preserve alpha channel (optional)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Load image from base64-encoded data
 * local imgData = base64.decode(encodedPng)
 * WindowLoadImageMemory("mywin", "icon", imgData, true)
 *
 * -- Load image from HTTP response
 * -- (assumes you have image data in a string)
 * WindowLoadImageMemory("mywin", "avatar", httpResponseBody, true)
 *
 * @see WindowLoadImage, WindowDrawImage
 */
int L_WindowLoadImageMemory(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* imageId = luaL_checkstring(L, 2);

    // Get data as string (can contain binary data including nulls)
    size_t len;
    const char* data = luaL_checklstring(L, 3, &len);
    bool alpha = lua_toboolean(L, 4);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    qint32 result = win->LoadImageMemory(
        QString::fromUtf8(imageId),
        std::span<const unsigned char>(reinterpret_cast<const unsigned char*>(data), len), alpha);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.WindowAddHotspot(name, hotspotId, left, top, right, bottom,
 *                        mouseOver, cancelMouseOver, mouseDown, cancelMouseDown,
 *                        mouseUp, tooltipText, cursor, flags)
 *
 * Creates an interactive hotspot (clickable area) in a miniwindow.
 * Hotspots respond to mouse events with Lua callback functions.
 *
 * Cursor types:
 *   0 = Arrow (default)
 *   1 = Hand/pointer
 *   6 = I-beam (text)
 *   11 = Cross
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Unique identifier for this hotspot
 * @param left (number) Left coordinate (0 or negative = relative to right edge)
 * @param top (number) Top coordinate
 * @param right (number) Right coordinate (0 or negative = window edge)
 * @param bottom (number) Bottom coordinate (0 or negative = window edge)
 * @param mouseOver (string) Function called when mouse enters
 * @param cancelMouseOver (string) Function called when mouse leaves
 * @param mouseDown (string) Function called on mouse button press
 * @param cancelMouseDown (string) Function called if released outside
 * @param mouseUp (string) Function called on mouse button release inside
 * @param tooltipText (string) Tooltip to show on hover
 * @param cursor (number) Cursor type to display
 * @param flags (number) Hotspot behavior flags
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Create a clickable button
 * WindowAddHotspot("mywin", "btn1", 10, 10, 100, 40,
 *     "", "",           -- mouse over/cancel
 *     "", "",           -- mouse down/cancel
 *     "OnButtonClick",  -- mouse up
 *     "Click me!",      -- tooltip
 *     1, 0)             -- hand cursor, no flags
 *
 * function OnButtonClick(flags, hotspotId)
 *     Note("Button clicked!")
 * end
 *
 * @see WindowDeleteHotspot, WindowMoveHotspot, WindowHotspotInfo, WindowDragHandler
 */
int L_WindowAddHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Extract all parameters (14 total)
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);
    const char* mouseOver = luaL_optstring(L, 7, "");
    const char* cancelMouseOver = luaL_optstring(L, 8, "");
    const char* mouseDown = luaL_optstring(L, 9, "");
    const char* cancelMouseDown = luaL_optstring(L, 10, "");
    const char* mouseUp = luaL_optstring(L, 11, "");
    const char* tooltipText = luaL_optstring(L, 12, "");
    qint32 cursor = luaL_optinteger(L, 13, 0);
    qint32 flags = luaL_optinteger(L, 14, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Create or replace hotspot
    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto hotspot = std::make_unique<Hotspot>();

    // Handle special case: right=0 or bottom=0 means "use window edge"
    // CMiniWindow::AddHotSpot
    if (right <= 0) {
        right = win->width + right; // 0 becomes width, -1 becomes width-1, etc.
    }
    if (bottom <= 0) {
        bottom = win->height + bottom; // 0 becomes height, -1 becomes height-1, etc.
    }

    // Set rectangle (miniwindow-relative coordinates)
    hotspot->m_rect = QRect(left, top, right - left, bottom - top);

    // Set mouse event callbacks
    hotspot->m_sMouseOver = QString::fromUtf8(mouseOver);
    hotspot->m_sCancelMouseOver = QString::fromUtf8(cancelMouseOver);
    hotspot->m_sMouseDown = QString::fromUtf8(mouseDown);
    hotspot->m_sCancelMouseDown = QString::fromUtf8(cancelMouseDown);
    hotspot->m_sMouseUp = QString::fromUtf8(mouseUp);

    // Set tooltip
    hotspot->m_sTooltipText = QString::fromUtf8(tooltipText);

    // Set cursor and flags
    hotspot->m_Cursor = cursor;
    hotspot->m_Flags = flags;

    // Store in miniwindow's hotspot map (replaces old one if it exists; old unique_ptr
    // automatically deletes)
    win->hotspots[hotspotIdStr] = std::move(hotspot);

    return luaReturnOK(L);
}

/**
 * world.WindowDeleteHotspot(name, hotspotId)
 *
 * Deletes a specific hotspot from a miniwindow.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to delete
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Remove a button
 * WindowDeleteHotspot("mywin", "btn1")
 *
 * @see WindowAddHotspot, WindowDeleteAllHotspots, WindowHotspotList
 */
int L_WindowDeleteHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end()) {
        return luaReturnError(L, eHotspotNotInstalled);
    }

    // Delete the hotspot
    win->hotspots.erase(it);

    return luaReturnOK(L);
}

/**
 * world.WindowDeleteAllHotspots(name)
 *
 * Deletes all hotspots from a miniwindow.
 *
 * @param name (string) Miniwindow name
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *
 * @example
 * -- Clear all hotspots before rebuilding UI
 * WindowDeleteAllHotspots("mywin")
 *
 * @see WindowAddHotspot, WindowDeleteHotspot, WindowHotspotList
 */
int L_WindowDeleteAllHotspots(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    // Clear all hotspots
    win->hotspots.clear();

    return luaReturnOK(L);
}

/**
 * world.WindowHotspotTooltip(name, hotspotId, tooltipText)
 *
 * Sets or updates the tooltip text for a hotspot.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID
 * @param tooltipText (string) New tooltip text
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Update tooltip dynamically
 * WindowHotspotTooltip("mywin", "hp_bar", "HP: 50/100")
 *
 * @see WindowAddHotspot, WindowHotspotInfo
 */
int L_WindowHotspotTooltip(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* tooltipText = luaL_checkstring(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end()) {
        return luaReturnError(L, eHotspotNotInstalled);
    }

    // Update tooltip
    it->second->m_sTooltipText = QString::fromUtf8(tooltipText);

    return luaReturnOK(L);
}

/**
 * world.WindowDragHandler(name, hotspotId, moveCallback, releaseCallback, flags)
 *
 * Sets up drag-and-drop handling for a miniwindow hotspot.
 * The hotspot must already exist (created with WindowAddHotspot).
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to configure
 * @param moveCallback (string) Function called during drag (on mouse move)
 * @param releaseCallback (string) Function called when drag ends
 * @param flags (number) Drag-and-drop flags
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Make a window draggable
 * WindowAddHotspot("mywin", "drag", 0, 0, 0, 0, "", "", "", "", "", "", 0, 0)
 * WindowDragHandler("mywin", "drag", "OnDrag", "OnDragEnd", 0)
 *
 * function OnDrag(flags, hotspotId, x, y)
 *     local newX = WindowInfo("mywin", 10) + x
 *     local newY = WindowInfo("mywin", 11) + y
 *     WindowPosition("mywin", newX, newY, 0, 2)
 * end
 *
 * @see WindowAddHotspot, WindowMoveHotspot, WindowScrollwheelHandler
 */
int L_WindowDragHandler(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* moveCallback = luaL_optstring(L, 3, "");
    const char* releaseCallback = luaL_optstring(L, 4, "");
    qint32 flags = luaL_optinteger(L, 5, 0);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Set drag-and-drop callbacks and flags
    hotspot->m_sMoveCallback = QString::fromUtf8(moveCallback);
    hotspot->m_sReleaseCallback = QString::fromUtf8(releaseCallback);
    hotspot->m_DragFlags = flags;

    return luaReturnOK(L);
}

/**
 * world.WindowMenu(name, x, y, menuString)
 *
 * Shows a popup context menu and returns the selected item number.
 * Blocks until user selects an item or cancels.
 *
 * Menu prefix flags:
 *   ">" - Start submenu (text is submenu title)
 *   "<" - End submenu, return to parent
 *   "+" - Checked item (checkmark shown)
 *   "!" - Default/bold item
 *   "^" - Disabled/grayed item
 *   "-" - Separator line
 *   "~" - Column break (not implemented)
 *
 * @param name (string) Miniwindow name
 * @param x (number) X coordinate (miniwindow-relative)
 * @param y (number) Y coordinate (miniwindow-relative)
 * @param menuString (string) Pipe-separated menu items with prefix flags
 *
 * @return (string) Selected item's 1-based position number, or "" if canceled
 *
 * @example
 * -- Simple menu
 * local result = WindowMenu("mywin", 10, 10, "Attack|Defend|Run")
 * if result == "1" then Note("Attack!") end
 *
 * -- Menu with separator and disabled item
 * local result = WindowMenu("mywin", 10, 10, "New|Open|-|^Save|Save As")
 *
 * -- Menu with submenu
 * local result = WindowMenu("mywin", 10, 10, "File|>Edit|Cut|Copy|Paste|<|Help")
 *
 * @see WindowAddHotspot
 */
int L_WindowMenu(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    qint32 x = luaL_checkinteger(L, 2);
    qint32 y = luaL_checkinteger(L, 3);
    const char* menuString = luaL_checkstring(L, 4);

    QString windowName = QString::fromUtf8(name);
    QString menuStr = QString::fromUtf8(menuString);

    // Get miniwindow to convert coordinates
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushstring(L, "");
        return 1;
    }

    // Parse menu string (pipe-separated items)
    QStringList items = menuStr.split('|');

    // Create popup menu with submenu stack
    QMenu rootMenu;
    QStack<QMenu*> menuStack;
    menuStack.push(&rootMenu);

    QMap<QAction*, int> actionIndexMap; // Map actions to 1-based position index
    int itemIndex = 0; // Counter for item positions (1-based, only counting selectable items)

    for (const QString& item : items) {
        QString text = item.trimmed();

        if (text.isEmpty()) {
            continue;
        }

        // Parse prefix flags
        bool isSubmenu = false;
        bool isSubmenuEnd = false;
        bool isChecked = false;
        bool isDefault = false;
        bool isDisabled = false;
        bool isSeparator = false;

        // Process all prefix characters
        // MUSHclient menu prefixes:
        // + = checked item
        // ! = default item (bold) - NOT checked
        // ^ = disabled/grayed
        // - = separator
        // > = submenu start
        // < = submenu end
        // ~ = column break
        while (!text.isEmpty()) {
            QChar prefix = text.at(0);

            if (prefix == '>') {
                isSubmenu = true;
                text = text.mid(1);
            } else if (prefix == '<') {
                isSubmenuEnd = true;
                text = text.mid(1);
            } else if (prefix == '+') {
                isChecked = true;
                text = text.mid(1);
            } else if (prefix == '!') {
                // Default item (shown bold) - NOT a checkmark
                isDefault = true;
                text = text.mid(1);
            } else if (prefix == '^') {
                isDisabled = true;
                text = text.mid(1);
            } else if (prefix == '-') {
                isSeparator = true;
                text = text.mid(1);
                break; // Separator consumes the rest
            } else if (prefix == '~') {
                // Column break - skip (not easily supported in Qt)
                text = text.mid(1);
            } else {
                break; // No more prefix characters
            }
        }

        QMenu* currentMenu = menuStack.top();

        // Handle submenu end - go back up one level
        if (isSubmenuEnd) {
            if (menuStack.size() > 1) {
                menuStack.pop();
            }
            // If there's remaining text after '<', process it as a menu item
            if (text.isEmpty()) {
                continue;
            }
            currentMenu = menuStack.top();
        }

        // Handle separator
        if (isSeparator) {
            currentMenu->addSeparator();
            continue;
        }

        // Handle submenu start
        if (isSubmenu && !text.isEmpty()) {
            QMenu* submenu = currentMenu->addMenu(text);
            menuStack.push(submenu);
            continue;
        }

        // Handle regular menu item
        if (!text.isEmpty()) {
            itemIndex++; // Increment for each selectable item (1-based)
            QAction* action = currentMenu->addAction(text);

            if (isChecked) {
                action->setCheckable(true);
                action->setChecked(true);
            }

            if (isDefault) {
                // '!' prefix - just ignore, doesn't add checkmark
                // (Some docs say it means "default" but in practice it's often just ignored)
            }

            if (isDisabled) {
                action->setEnabled(false);
            }

            // Store the 1-based index for return value
            actionIndexMap[action] = itemIndex;
        }
    }

    // Convert miniwindow-relative coordinates to output-window coordinates
    QPoint outputPos = win->rect.topLeft() + QPoint(x, y);

    // Convert to global screen coordinates
    QPoint globalPos = outputPos;
    if (pDoc->m_pActiveOutputView) {
        globalPos = pDoc->m_pActiveOutputView->mapToGlobal(outputPos);
    }

    // Show menu at specified position
    QAction* selectedAction = rootMenu.exec(globalPos);

    // Return selected item's 1-based position number as string (or empty string if canceled)
    // MUSHclient returns the position number, not the text
    if (selectedAction && actionIndexMap.contains(selectedAction)) {
        int index = actionIndexMap[selectedAction];
        lua_pushstring(L, QString::number(index).toUtf8().constData());
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

/**
 * world.WindowMoveHotspot(name, hotspotId, left, top, right, bottom)
 *
 * Moves or resizes an existing hotspot to a new location.
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to move
 * @param left (number) New left coordinate (0 or negative = relative to edge)
 * @param top (number) New top coordinate
 * @param right (number) New right coordinate (0 or negative = window edge)
 * @param bottom (number) New bottom coordinate (0 or negative = window edge)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Move button to new position
 * WindowMoveHotspot("mywin", "btn1", 50, 50, 150, 80)
 *
 * -- Resize to cover entire window
 * WindowMoveHotspot("mywin", "fullscreen", 0, 0, 0, 0)
 *
 * @see WindowAddHotspot, WindowHotspotInfo, WindowDragHandler
 */
int L_WindowMoveHotspot(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 left = luaL_checkinteger(L, 3);
    qint32 top = luaL_checkinteger(L, 4);
    qint32 right = luaL_checkinteger(L, 5);
    qint32 bottom = luaL_checkinteger(L, 6);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Handle special case: right=0 or bottom=0 means "use window edge"
    if (right <= 0) {
        right = win->width + right;
    }
    if (bottom <= 0) {
        bottom = win->height + bottom;
    }

    // Update hotspot rectangle
    hotspot->m_rect = QRect(left, top, right - left, bottom - top);

    return luaReturnOK(L);
}

/**
 * world.WindowHotspotInfo(name, hotspotId, infoType)
 *
 * Gets information about a miniwindow hotspot.
 *
 * Info types:
 *   1 = left coordinate
 *   2 = top coordinate
 *   3 = right coordinate
 *   4 = bottom coordinate
 *   5 = MouseOver callback name
 *   6 = CancelMouseOver callback name
 *   7 = MouseDown callback name
 *   8 = CancelMouseDown callback name
 *   9 = MouseUp callback name
 *   10 = Tooltip text
 *   11 = Cursor type
 *   12 = Flags
 *   13 = Drag move callback name
 *   14 = Drag release callback name
 *   15 = Drag flags
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID
 * @param infoType (number) Type of information (1-15)
 *
 * @return (varies) Requested information, or nil if hotspot doesn't exist
 *
 * @example
 * -- Get hotspot boundaries
 * local left = WindowHotspotInfo("mywin", "btn1", 1)
 * local top = WindowHotspotInfo("mywin", "btn1", 2)
 * local right = WindowHotspotInfo("mywin", "btn1", 3)
 * local bottom = WindowHotspotInfo("mywin", "btn1", 4)
 *
 * @see WindowAddHotspot, WindowMoveHotspot, WindowHotspotList
 */
int L_WindowHotspotInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    qint32 infoType = luaL_checkinteger(L, 3);

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        lua_pushnil(L); // No such window
        return 1;
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        lua_pushnil(L); // No such hotspot
        return 1;
    }
    Hotspot* hotspot = it->second.get();

    // Return actual hotspot data
    switch (infoType) {
        case 1: // left
            lua_pushinteger(L, hotspot->m_rect.left());
            break;
        case 2: // top
            lua_pushinteger(L, hotspot->m_rect.top());
            break;
        case 3: // right
            lua_pushinteger(L, hotspot->m_rect.right());
            break;
        case 4: // bottom
            lua_pushinteger(L, hotspot->m_rect.bottom());
            break;
        case 5: // MouseOver callback
            lua_pushstring(L, hotspot->m_sMouseOver.toUtf8().constData());
            break;
        case 6: // CancelMouseOver callback
            lua_pushstring(L, hotspot->m_sCancelMouseOver.toUtf8().constData());
            break;
        case 7: // MouseDown callback
            lua_pushstring(L, hotspot->m_sMouseDown.toUtf8().constData());
            break;
        case 8: // CancelMouseDown callback
            lua_pushstring(L, hotspot->m_sCancelMouseDown.toUtf8().constData());
            break;
        case 9: // MouseUp callback
            lua_pushstring(L, hotspot->m_sMouseUp.toUtf8().constData());
            break;
        case 10: // TooltipText
            lua_pushstring(L, hotspot->m_sTooltipText.toUtf8().constData());
            break;
        case 11: // cursor code
            lua_pushinteger(L, hotspot->m_Cursor);
            break;
        case 12: // flags
            lua_pushinteger(L, hotspot->m_Flags);
            break;
        case 13: // MoveCallback (drag-and-drop)
            lua_pushstring(L, hotspot->m_sMoveCallback.toUtf8().constData());
            break;
        case 14: // ReleaseCallback (drag-and-drop)
            lua_pushstring(L, hotspot->m_sReleaseCallback.toUtf8().constData());
            break;
        case 15: // drag flags
            lua_pushinteger(L, hotspot->m_DragFlags);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.WindowScrollwheelHandler(name, hotspotId, scrollCallback)
 *
 * Sets up mouse scroll wheel handling for a hotspot.
 * The hotspot must already exist (created with WindowAddHotspot).
 *
 * @param name (string) Miniwindow name
 * @param hotspotId (string) Hotspot ID to configure
 * @param scrollCallback (string) Function called on scroll wheel event
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eNoSuchWindow: Window doesn't exist
 *   - eHotspotNotInstalled: Hotspot doesn't exist
 *
 * @example
 * -- Handle scroll events for zooming
 * WindowAddHotspot("mywin", "scroll_area", 0, 0, 0, 0, "", "", "", "", "", "", 0, 0)
 * WindowScrollwheelHandler("mywin", "scroll_area", "OnScroll")
 *
 * function OnScroll(flags, hotspotId, delta)
 *     if delta > 0 then
 *         Note("Scroll up")
 *     else
 *         Note("Scroll down")
 *     end
 * end
 *
 * @see WindowAddHotspot, WindowDragHandler
 */
int L_WindowScrollwheelHandler(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* name = luaL_checkstring(L, 1);
    const char* hotspotId = luaL_checkstring(L, 2);
    const char* scrollCallback = luaL_optstring(L, 3, "");

    QString windowName = QString::fromUtf8(name);
    MiniWindow* win = getMiniWindow(pDoc, windowName);
    if (!win) {
        return luaReturnError(L, eNoSuchWindow);
    }

    QString hotspotIdStr = QString::fromUtf8(hotspotId);

    // Get raw pointer from unique_ptr in map
    auto it = win->hotspots.find(hotspotIdStr);
    if (it == win->hotspots.end() || !it->second) {
        return luaReturnError(L, eHotspotNotInstalled);
    }
    Hotspot* hotspot = it->second.get();

    // Set scroll wheel callback
    hotspot->m_sScrollwheelCallback = QString::fromUtf8(scrollCallback);

    return luaReturnOK(L);
}
