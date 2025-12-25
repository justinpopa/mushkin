#ifndef MINIWINDOW_H
#define MINIWINDOW_H

#include "hotspot.h"
#include <QFont>
#include <QImage>
#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVariant>
#include <map>
#include <memory>

// Forward declarations
class WorldDocument;

// Miniwindow flags (bitwise OR)
const qint32 MINIWINDOW_DRAW_UNDERNEATH = 1;   // Draw behind text output
const qint32 MINIWINDOW_ABSOLUTE_LOCATION = 2; // Use absolute coordinates
const qint32 MINIWINDOW_TRANSPARENT = 4;       // Use background color as transparency key
const qint32 MINIWINDOW_IGNORE_MOUSE = 8;      // Ignore all mouse events
const qint32 MINIWINDOW_KEEP_HOTSPOTS = 16;    // Keep hotspots when deleting window

// Position modes (0-13)
// miniwin_place_mode enum
enum class MiniWindowPosition : qint16 {
    StretchToOutputWindow = 0,    // Stretch to fill output window
    StretchWithAspectRatio = 1,   // Stretch to output window preserving aspect ratio
    StretchToFrame = 2,           // Stretch to fill frame (entire client area)
    StretchToFrameWithAspect = 3, // Stretch to frame preserving aspect ratio
    TopLeft = 4,                  // Corner: top-left
    TopCenter = 5,                // Centered: along top edge
    TopRight = 6,                 // Corner: top-right
    RightCenter = 7,              // Centered: along right edge
    BottomRight = 8,              // Corner: bottom-right
    BottomCenter = 9,             // Centered: along bottom edge
    BottomLeft = 10,              // Corner: bottom-left
    LeftCenter = 11,              // Centered: along left edge
    CenterAll = 12,               // Centered: middle of window
    Tile = 13                     // Tile mode (not yet implemented)
};

/**
 * @brief The MiniWindow class represents an offscreen drawing surface for custom UI elements.
 *
 * Miniwindows are MUSHclient's custom UI toolkit. Plugins use miniwindows to create:
 * - Health/mana bars and gauges
 * - Clickable navigation maps
 * - Spell timers and buff indicators
 * - Custom buttons and controls
 * - Info panels and HUDs
 *
 * Design Pattern:
 * - Offscreen rendering: Draw to QImage, convert to QPixmap for display (flicker-free)
 * - String-based IDs: Fonts/images/hotspots referenced by string keys
 * - Z-ordering: Layer multiple miniwindows (backgrounds under foregrounds)
 * - Positioning: Anchor to screen regions or absolute coordinates
 * - Hotspots: Clickable regions with Lua callbacks
 *
 * Implements MiniWindow interface to allow world layer to interact with miniwindows
 * without depending on the concrete UI implementation.
 *
 * Reference: miniwindow.h, miniwindow.cpp
 */
class MiniWindow : public QObject {
    Q_OBJECT

  public:
    explicit MiniWindow(WorldDocument* doc, QObject* parent = nullptr);
    ~MiniWindow();

    // ========== Basic Properties ==========

    QString name;          // Unique identifier
    WorldDocument* m_pDoc; // Parent world document

    // ========== Dimensions ==========

    qint32 width;  // Miniwindow width in pixels
    qint32 height; // Miniwindow height in pixels

    // ========== Positioning ==========

    QPoint location; // Absolute position (if ABSOLUTE_LOCATION flag set)
    qint16 position; // Relative position (0-8: center, corners, edges)
    QRect rect;      // Actual screen position after layout calculation

    // ========== Appearance ==========

    qint32 flags;         // DRAW_UNDERNEATH, TRANSPARENT, etc.
    QRgb backgroundColor; // Background color (also used for transparency key)
    bool show;            // Visible?
    bool temporarilyHide; // Hidden due to insufficient space
    bool dirty;           // Needs redraw?

    // ========== Offscreen Drawing Surface ==========

    std::unique_ptr<QImage>
        image; // Offscreen drawing surface (QImage for platform-independent rendering)

    // ========== Collections ==========

    QMap<QString, QFont> fonts;                           // Font cache (fontId -> QFont)
    std::map<QString, std::unique_ptr<QImage>> images;    // Image cache (imageId -> QImage)
    std::map<QString, std::unique_ptr<Hotspot>> hotspots; // Hotspot map (hotspotId -> Hotspot)

    // ========== Tracking ==========

    qint32 zOrder;          // Draw order (lower = earlier, 0 = alphabetical)
    QString creatingPlugin; // Plugin ID that created this window
    QString callbackPlugin; // Plugin ID for hotspot callbacks (defaults to creatingPlugin)
    bool executingScript;   // Currently executing Lua callback? (prevents deletion)

    // ========== Mouse State ==========

    QPoint lastMousePosition; // Last mouse position in miniwindow coordinates (WindowInfo 14/15)
    QPoint
        clientMousePosition;  // Last mouse position in output window coordinates (WindowInfo 17/18)
    QString mouseOverHotspot; // Hotspot ID currently under mouse (empty if none)
    QString mouseDownHotspot; // Hotspot ID where mouse was pressed (empty if none)

    // ========== Methods ==========

    /**
     * @brief Recreate the offscreen image with new dimensions
     * @param newWidth New width in pixels
     * @param newHeight New height in pixels
     * @param bgColor New background color
     * @return Error code (eOK on success)
     */
    qint32 Resize(qint32 newWidth, qint32 newHeight, QRgb bgColor);

    /**
     * @brief Clear the entire miniwindow with background color
     */
    void Clear();

    /**
     * @brief Delete all fonts, images, and hotspots
     */
    void DeleteAllResources();

    // ========== Drawing Primitives ==========

    /**
     * @brief Draw rectangle with various operations
     * @param action Operation: 1=frame, 2=fill, 3=invert, 5=3D rect
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param penColor Pen color (ARGB)
     * @param brushColor Brush color (ARGB)
     * @return Error code
     */
    qint32 RectOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                  QRgb penColor, QRgb brushColor);

    /**
     * @brief Draw circle/ellipse with various operations
     * @param action Operation: 1=ellipse, 2=rectangle, 3=round_rect, 4=chord, 5=pie, 6=arc
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param penColor Pen color (ARGB)
     * @param penStyle Pen style (0=solid, 1=dash, 2=dot, etc.)
     * @param penWidth Pen width in pixels
     * @param brushColor Brush color (ARGB)
     * @param brushStyle Brush style (0=none, 1=solid, etc.)
     * @param extra1 Extra parameter 1 (arc start angle or corner radius)
     * @param extra2 Extra parameter 2 (arc span angle or corner radius)
     * @param extra3 Extra parameter 3 (reserved)
     * @param extra4 Extra parameter 4 (reserved)
     * @return Error code
     */
    qint32 CircleOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                    QRgb penColor, qint32 penStyle, qint32 penWidth, QRgb brushColor,
                    qint32 brushStyle, qint32 extra1, qint32 extra2, qint32 extra3, qint32 extra4);

    /**
     * @brief Draw line
     * @param x1 Start X coordinate
     * @param y1 Start Y coordinate
     * @param x2 End X coordinate
     * @param y2 End Y coordinate
     * @param penColor Pen color (ARGB)
     * @param penStyle Pen style (0=solid, 1=dash, etc.)
     * @param penWidth Pen width in pixels
     * @return Error code
     */
    qint32 Line(qint32 x1, qint32 y1, qint32 x2, qint32 y2, QRgb penColor, qint32 penStyle,
                qint32 penWidth);

    /**
     * @brief Set pixel color
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Color (ARGB)
     * @return Error code
     */
    qint32 SetPixel(qint32 x, qint32 y, QRgb color);

    /**
     * @brief Get pixel color
     * @param x X coordinate
     * @param y Y coordinate
     * @return Color (ARGB), or 0 if out of bounds
     */
    QRgb GetPixel(qint32 x, qint32 y);

    /**
     * @brief Draw polygon
     * @param points Comma-separated coordinate pairs (x1,y1,x2,y2,...)
     * @param penColor Pen color (ARGB)
     * @param penStyle Pen style (0=solid, 1=dash, etc.)
     * @param penWidth Pen width in pixels
     * @param brushColor Brush color (ARGB)
     * @param brushStyle Brush style (0=none, 1=solid, etc.)
     * @param close Close the polygon?
     * @param winding Use winding fill rule? (vs odd-even)
     * @return Error code
     */
    qint32 Polygon(const QString& points, QRgb penColor, qint32 penStyle, qint32 penWidth,
                   QRgb brushColor, qint32 brushStyle, bool close, bool winding);

    /**
     * @brief Draw gradient fill
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param color1 Start color (ARGB)
     * @param color2 End color (ARGB)
     * @param mode Gradient mode (1=horizontal, 2=vertical)
     * @return Error code
     */
    qint32 Gradient(qint32 left, qint32 top, qint32 right, qint32 bottom, QRgb color1, QRgb color2,
                    qint32 mode);

    /**
     * @brief Draw arc (portion of ellipse)
     * @param left Left coordinate of bounding rectangle
     * @param top Top coordinate of bounding rectangle
     * @param right Right coordinate of bounding rectangle
     * @param bottom Bottom coordinate of bounding rectangle
     * @param x1 X coordinate of arc start point
     * @param y1 Y coordinate of arc start point
     * @param x2 X coordinate of arc end point
     * @param y2 Y coordinate of arc end point
     * @param penColor Pen color (ARGB)
     * @param penStyle Pen style (0=solid, 1=dash, etc.)
     * @param penWidth Pen width in pixels
     * @return Error code
     */
    qint32 Arc(qint32 left, qint32 top, qint32 right, qint32 bottom, qint32 x1, qint32 y1,
               qint32 x2, qint32 y2, QRgb penColor, qint32 penStyle, qint32 penWidth);

    /**
     * @brief Draw Bezier curve
     * @param pointsStr Comma-separated coordinate pairs (x1,y1,x2,y2,...) - must be (3n+1) points
     * @param penColor Pen color (ARGB)
     * @param penStyle Pen style (0=solid, 1=dash, etc.)
     * @param penWidth Pen width in pixels
     * @return Error code
     */
    qint32 Bezier(const QString& pointsStr, QRgb penColor, qint32 penStyle, qint32 penWidth);

    // ========== Text and Fonts ==========

    /**
     * @brief Create or update font
     * @param fontId Font identifier
     * @param fontName Font family name
     * @param size Font size in points
     * @param bold Bold?
     * @param italic Italic?
     * @param underline Underline?
     * @param strikeout Strikeout?
     * @return Error code
     */
    qint32 Font(const QString& fontId, const QString& fontName, double size, bool bold, bool italic,
                bool underline, bool strikeout);

    /**
     * @brief Draw text
     * @param fontId Font identifier
     * @param text Text to draw
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param color Text color (ARGB)
     * @param unicode UTF-8 encoding?
     * @return Error code
     */
    qint32 Text(const QString& fontId, const QString& text, qint32 left, qint32 top, qint32 right,
                qint32 bottom, QRgb color, bool unicode);

    /**
     * @brief Measure text width
     * @param fontId Font identifier
     * @param text Text to measure
     * @param unicode UTF-8 encoding?
     * @return Text width in pixels (0 if font not found)
     */
    qint32 TextWidth(const QString& fontId, const QString& text, bool unicode);

    /**
     * @brief Get font information
     * @param fontId Font identifier
     * @param infoType Information type (1-9)
     * @return QVariant with requested info (or empty if not found)
     */
    QVariant FontInfo(const QString& fontId, qint32 infoType);

    /**
     * @brief Get list of all font IDs registered in this miniwindow
     * @return QStringList of font IDs
     */
    QStringList FontList();

    // ========== Image Operations ==========

    /**
     * @brief Load image from file into image cache
     * @param imageId Image identifier
     * @param filepath Path to image file (PNG, BMP, JPG)
     * @return Error code (eOK on success)
     */
    qint32 LoadImage(const QString& imageId, const QString& filepath);

    /**
     * @brief Draw image with scaling and sprite sheet support
     * @param imageId Image identifier
     * @param left Destination left coordinate
     * @param top Destination top coordinate
     * @param right Destination right coordinate
     * @param bottom Destination bottom coordinate
     * @param mode Draw mode (1=copy, 2=transparent_copy)
     * @param srcLeft Source left coordinate (0 for full image)
     * @param srcTop Source top coordinate
     * @param srcRight Source right coordinate
     * @param srcBottom Source bottom coordinate
     * @return Error code
     */
    qint32 DrawImage(const QString& imageId, qint32 left, qint32 top, qint32 right, qint32 bottom,
                     qint16 mode, qint32 srcLeft, qint32 srcTop, qint32 srcRight, qint32 srcBottom);

    /**
     * @brief Draw image with opacity and blend modes
     * @param imageId Image identifier
     * @param left Destination left coordinate
     * @param top Destination top coordinate
     * @param right Destination right coordinate
     * @param bottom Destination bottom coordinate
     * @param mode Blend mode (1=normal, 2=multiply, 3=screen, 4=overlay)
     * @param opacity Opacity (0.0 = transparent, 1.0 = opaque)
     * @param srcLeft Source left coordinate (0 for full image)
     * @param srcTop Source top coordinate
     * @param srcRight Source right coordinate
     * @param srcBottom Source bottom coordinate
     * @return Error code
     */
    qint32 BlendImage(const QString& imageId, qint32 left, qint32 top, qint32 right, qint32 bottom,
                      qint16 mode, double opacity, qint32 srcLeft, qint32 srcTop, qint32 srcRight,
                      qint32 srcBottom);

    /**
     * @brief Copy another miniwindow's image as an image (implements MiniWindow)
     * @param imageId Image identifier to store under
     * @param srcWindow Source miniwindow
     * @return Error code
     */
    qint32 ImageFromWindow(const QString& imageId, MiniWindow* srcWindow);

    /**
     * @brief Copy another miniwindow's image as an image (legacy overload)
     * @param imageId Image identifier to store under
     * @param srcDoc Parent world document
     * @param srcWindowName Source miniwindow name
     * @return Error code
     */
    qint32 ImageFromWindow(const QString& imageId, WorldDocument* srcDoc,
                           const QString& srcWindowName);

    /**
     * @brief Get image information
     * @param imageId Image identifier
     * @param infoType Information type (1=width, 2=height)
     * @return QVariant with requested info (or empty if not found)
     */
    QVariant ImageInfo(const QString& imageId, qint32 infoType);

    /**
     * @brief Get list of all image IDs loaded in this miniwindow
     * @return QStringList of image IDs
     */
    QStringList ImageList();

    /**
     * @brief Get list of all hotspot IDs in this miniwindow
     * @return QStringList of hotspot IDs
     */
    QStringList HotspotList();

    /**
     * @brief Create a small 8x8 monochrome image from row data
     * @param imageId Image identifier
     * @param row1-row8 Each row is 8 bits representing pixels (bit 7 = leftmost)
     * @return Error code (eOK on success)
     */
    qint32 CreateImage(const QString& imageId, qint32 row1, qint32 row2, qint32 row3, qint32 row4,
                       qint32 row5, qint32 row6, qint32 row7, qint32 row8);

    /**
     * @brief Perform rectangle/ellipse operations using an image as brush
     * @param action Operation type (same as RectOp/CircleOp actions)
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param penColor Pen color
     * @param penStyle Pen style
     * @param penWidth Pen width
     * @param brushColor Brush color (for 0-bits in image)
     * @param imageId Image to use as brush pattern
     * @param ellipseWidth Corner width for rounded rect
     * @param ellipseHeight Corner height for rounded rect
     * @return Error code
     */
    qint32 ImageOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                   QRgb penColor, qint32 penStyle, qint32 penWidth, QRgb brushColor,
                   const QString& imageId, qint32 ellipseWidth, qint32 ellipseHeight);

    /**
     * @brief Load image from memory buffer
     * @param imageId Image identifier
     * @param data Raw image data (PNG, BMP, JPG, etc.)
     * @param length Length of data
     * @param hasAlpha Whether to preserve alpha channel
     * @return Error code (eOK on success)
     */
    qint32 LoadImageMemory(const QString& imageId, const unsigned char* data, size_t length,
                           bool hasAlpha);

    /**
     * @brief Save miniwindow to BMP or PNG file
     * @param filename Output file path (must end in .bmp or .png)
     * @return Error code (eOK on success)
     */
    qint32 Write(const QString& filename);

    /**
     * @brief Extract alpha channel from image as grayscale
     * @param imageId Image identifier (must be 32-bit with alpha)
     * @param left Destination left coordinate
     * @param top Destination top coordinate
     * @param right Destination right coordinate
     * @param bottom Destination bottom coordinate
     * @param srcLeft Source left coordinate
     * @param srcTop Source top coordinate
     * @return Error code (eOK on success)
     */
    qint32 GetImageAlpha(const QString& imageId, qint32 left, qint32 top, qint32 right,
                         qint32 bottom, qint32 srcLeft, qint32 srcTop);

    /**
     * @brief Draw image with alpha channel blending
     * @param imageId Image identifier (must be 32-bit with alpha)
     * @param left Destination left coordinate
     * @param top Destination top coordinate
     * @param right Destination right coordinate
     * @param bottom Destination bottom coordinate
     * @param opacity Additional opacity multiplier (0.0=transparent, 1.0=opaque)
     * @param srcLeft Source left coordinate
     * @param srcTop Source top coordinate
     * @return Error code (eOK on success)
     */
    qint32 DrawImageAlpha(const QString& imageId, qint32 left, qint32 top, qint32 right,
                          qint32 bottom, double opacity, qint32 srcLeft, qint32 srcTop);

    /**
     * @brief Merge image with separate alpha mask
     * @param imageId Main image identifier
     * @param maskId Mask image identifier
     * @param left Destination left coordinate
     * @param top Destination top coordinate
     * @param right Destination right coordinate
     * @param bottom Destination bottom coordinate
     * @param mode Blend mode (0=normal, 1=color key transparency)
     * @param opacity Additional opacity multiplier (0.0-1.0)
     * @param srcLeft Source left coordinate
     * @param srcTop Source top coordinate
     * @param srcRight Source right coordinate
     * @param srcBottom Source bottom coordinate
     * @return Error code (eOK on success)
     */
    qint32 MergeImageAlpha(const QString& imageId, const QString& maskId, qint32 left, qint32 top,
                           qint32 right, qint32 bottom, qint16 mode, double opacity, qint32 srcLeft,
                           qint32 srcTop, qint32 srcRight, qint32 srcBottom);

    /**
     * @brief Apply affine transformation to image (rotate, scale, skew)
     * @param imageId Image identifier
     * @param left X translation offset
     * @param top Y translation offset
     * @param mode Draw mode (1=opaque, 3=transparent with color key)
     * @param mxx Transformation matrix [0][0] (cos θ for rotation)
     * @param mxy Transformation matrix [1][0] (sin θ for rotation)
     * @param myx Transformation matrix [0][1] (-sin θ for rotation)
     * @param myy Transformation matrix [1][1] (cos θ for rotation)
     * @return Error code (eOK on success)
     */
    qint32 TransformImage(const QString& imageId, float left, float top, qint16 mode, float mxx,
                          float mxy, float myx, float myy);

    /**
     * @brief Apply pixel filter to rectangular region
     * @param left Left coordinate
     * @param top Top coordinate
     * @param right Right coordinate
     * @param bottom Bottom coordinate
     * @param operation Filter type (1-27)
     * @param options Filter-specific parameter
     * @return Error code (eOK on success)
     */
    qint32 Filter(qint32 left, qint32 top, qint32 right, qint32 bottom, qint16 operation,
                  double options);

    // ========== MiniWindow Interface Implementation ==========
    // Getters and setters for public properties

    QString getName() const
    {
        return name;
    }
    void setName(const QString& n)
    {
        name = n;
    }

    qint32 getWidth() const
    {
        return width;
    }
    qint32 getHeight() const
    {
        return height;
    }

    QPoint getLocation() const
    {
        return location;
    }
    void setLocation(const QPoint& loc)
    {
        location = loc;
    }

    qint16 getPosition() const
    {
        return position;
    }
    void setPosition(qint16 pos)
    {
        position = pos;
    }

    QRect getRect() const
    {
        return rect;
    }
    void setRect(const QRect& r)
    {
        rect = r;
    }

    qint32 getFlags() const
    {
        return flags;
    }
    void setFlags(qint32 f)
    {
        flags = f;
    }

    QRgb getBackgroundColor() const
    {
        return backgroundColor;
    }
    void setBackgroundColor(QRgb color)
    {
        backgroundColor = color;
    }

    bool getShow() const
    {
        return show;
    }
    void setShow(bool visible)
    {
        show = visible;
    }

    bool getTemporarilyHide() const
    {
        return temporarilyHide;
    }
    void setTemporarilyHide(bool hide)
    {
        temporarilyHide = hide;
    }

    bool getDirty() const
    {
        return dirty;
    }
    void setDirty(bool d)
    {
        dirty = d;
    }

    qint32 getZOrder() const
    {
        return zOrder;
    }
    void setZOrder(qint32 z)
    {
        zOrder = z;
    }

    QString getCreatingPlugin() const
    {
        return creatingPlugin;
    }
    void setCreatingPlugin(const QString& plugin)
    {
        creatingPlugin = plugin;
    }

    QString getCallbackPlugin() const
    {
        return callbackPlugin;
    }
    void setCallbackPlugin(const QString& plugin)
    {
        callbackPlugin = plugin;
    }

    bool getExecutingScript() const
    {
        return executingScript;
    }
    void setExecutingScript(bool executing)
    {
        executingScript = executing;
    }

    QPoint getLastMousePosition() const
    {
        return lastMousePosition;
    }
    void setLastMousePosition(const QPoint& pos)
    {
        lastMousePosition = pos;
    }

    QPoint getClientMousePosition() const
    {
        return clientMousePosition;
    }
    void setClientMousePosition(const QPoint& pos)
    {
        clientMousePosition = pos;
    }

    QString getMouseOverHotspot() const
    {
        return mouseOverHotspot;
    }
    void setMouseOverHotspot(const QString& hotspot)
    {
        mouseOverHotspot = hotspot;
    }

    QString getMouseDownHotspot() const
    {
        return mouseDownHotspot;
    }
    void setMouseDownHotspot(const QString& hotspot)
    {
        mouseDownHotspot = hotspot;
    }

    QImage* getImage()
    {
        return image.get();
    }
    const QImage* getImage() const
    {
        return image.get();
    }

    // Convert QImage to QPixmap for display in UI
    QPixmap toPixmap() const
    {
        if (!image)
            return QPixmap();
        return QPixmap::fromImage(*image);
    }

  signals:
    void needsRedraw(); // Emitted when miniwindow changes and needs repaint

  private:
    // Prevent copying
    MiniWindow(const MiniWindow&) = delete;
    MiniWindow& operator=(const MiniWindow&) = delete;
};

#endif // MINIWINDOW_H
