#include "miniwindow.h"
#include "../world/world_document.h"
#include <QColor>
#include <QDebug>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QTransform>

// Error codes (from lua_methods.cpp / lua_common.h)
enum {
    eOK = 0,
    eNoNameSpecified = 30003,
    eCouldNotOpenFile = 30013,
    eLogFileBadWrite = 30016,
    eUnknownOption = 30025,
    eBadParameter = 30046,
    eUnableToLoadImage = 30067,
    eImageNotInstalled = 30068,
    eInvalidNumberOfPoints = 30069,
    eInvalidPoint = 30070,
    eHotspotNotInstalled = 30072,
    eNoSuchWindow = 30073,
};

// Helper to convert BGR color value to QColor
// Mushkin Lua API uses BGR format (Windows COLORREF): 0x00BBGGRR
// This matches original MUSHclient for compatibility with existing plugins
// The high byte (alpha) is ignored for drawing - we always use full opacity
static inline QColor bgrToColor(quint32 bgr)
{
    return QColor(bgr & 0xFF, (bgr >> 8) & 0xFF, (bgr >> 16) & 0xFF);
}

// Helper to convert BGR to QRgb (for QImage::setPixel)
// Input: 0x00BBGGRR (BGR), Output: 0xFFRRGGBB (ARGB with full alpha)
static inline QRgb bgrToQRgb(quint32 bgr)
{
    int r = bgr & 0xFF;
    int g = (bgr >> 8) & 0xFF;
    int b = (bgr >> 16) & 0xFF;
    return qRgba(r, g, b, 255);
}

// Helper to convert QRgb (ARGB) back to BGR for return to Lua
// Input: 0xAARRGGBB, Output: 0x00BBGGRR
static inline quint32 qRgbToBgr(QRgb argb)
{
    return qRed(argb) | (qGreen(argb) << 8) | (qBlue(argb) << 16);
}

// Helper to create a pen with Windows GDI-compatible dash patterns
// Windows GDI cosmetic pens (width 0 or 1) render dash patterns as nearly solid
// because the dash pattern is device-dependent and effectively invisible at 1 pixel.
// Only geometric pens (ExtCreatePen with PS_GEOMETRIC) show visible dashes.
// Original MUSHclient uses CreatePen() for simple pens, so 1-px dashes appear solid.
static QPen createWindowsPen(const QColor& color, int width, int penStyle)
{
    int linePattern = penStyle & 0x0F;
    QPen pen(color, width);

    // Windows GDI cosmetic pens (width <= 1) render dash styles as solid
    // This matches the original MUSHclient behavior where 1-pixel dashed lines appear solid
    bool isCosmetic = (width <= 1);

    switch (linePattern) {
        case 0: // PS_SOLID
            pen.setStyle(Qt::SolidLine);
            break;
        case 1: // PS_DASH
            if (isCosmetic) {
                // Cosmetic pens render dashes as solid in Windows GDI
                pen.setStyle(Qt::SolidLine);
            } else {
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern({6.0, 2.0}); // 6 pixel dash, 2 pixel gap
            }
            break;
        case 2: // PS_DOT
            if (isCosmetic) {
                pen.setStyle(Qt::SolidLine);
            } else {
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern({1.0, 2.0}); // 1 pixel dot, 2 pixel gap
            }
            break;
        case 3: // PS_DASHDOT
            if (isCosmetic) {
                pen.setStyle(Qt::SolidLine);
            } else {
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern({6.0, 2.0, 1.0, 2.0}); // dash, gap, dot, gap
            }
            break;
        case 4: // PS_DASHDOTDOT
            if (isCosmetic) {
                pen.setStyle(Qt::SolidLine);
            } else {
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern({6.0, 2.0, 1.0, 2.0, 1.0, 2.0}); // dash, gap, dot, gap, dot, gap
            }
            break;
        case 5: // PS_NULL
            pen.setStyle(Qt::NoPen);
            break;
        default:
            pen.setStyle(Qt::SolidLine);
            break;
    }

    // Handle end cap style (bits 8-11)
    int endCapStyle = (penStyle >> 8) & 0x0F;
    switch (endCapStyle) {
        case 0: // PS_ENDCAP_ROUND (default)
            pen.setCapStyle(Qt::RoundCap);
            break;
        case 1: // PS_ENDCAP_SQUARE
            pen.setCapStyle(Qt::SquareCap);
            break;
        case 2: // PS_ENDCAP_FLAT
            pen.setCapStyle(Qt::FlatCap);
            break;
        default:
            pen.setCapStyle(Qt::RoundCap);
            break;
    }

    // Handle join style (bits 12-15)
    int joinStyle = (penStyle >> 12) & 0x0F;
    switch (joinStyle) {
        case 0: // PS_JOIN_ROUND (default)
            pen.setJoinStyle(Qt::RoundJoin);
            break;
        case 1: // PS_JOIN_BEVEL
            pen.setJoinStyle(Qt::BevelJoin);
            break;
        case 2: // PS_JOIN_MITER
            pen.setJoinStyle(Qt::MiterJoin);
            break;
        default:
            pen.setJoinStyle(Qt::RoundJoin);
            break;
    }

    return pen;
}

/**
 * @brief Constructor
 * @param doc Parent WorldDocument
 * @param parent QObject parent
 *
 * Creates a miniwindow with default values. The offscreen image is created
 * when width/height are set via Resize().
 *
 * Reference: miniwindow.cpp - CMiniWindow::CMiniWindow
 */
MiniWindow::MiniWindow(WorldDocument* doc, QObject* parent)
    : QObject(parent), m_pDoc(doc), width(0), height(0), location(0, 0),
      position(0) // Default: center
      ,
      rect(), flags(0), backgroundColor(0xFF000000) // Black
      ,
      show(false) // Hidden by default (must call WindowShow)
      ,
      temporarilyHide(false), dirty(true), zOrder(0), executingScript(false)
{
    // Collections are empty by default (unique_ptr members automatically initialize to nullptr)
}

/**
 * @brief Destructor
 *
 * Resources (fonts, images, hotspots) and the offscreen image are automatically
 * cleaned up by unique_ptr and QMap destructors.
 * Note: If executingScript is true, we still delete (but WindowDelete() will
 * prevent deletion during script execution).
 *
 * Reference: miniwindow.cpp - CMiniWindow::~CMiniWindow
 */
MiniWindow::~MiniWindow()
{
    // unique_ptr members (image, images, hotspots) automatically clean up
    // QMap destructor will clean up all unique_ptr entries
}

/**
 * @brief Resize the miniwindow and recreate the offscreen image
 * @param newWidth New width in pixels
 * @param newHeight New height in pixels
 * @param bgColor New background color
 * @return eOK on success, eBadParameter if dimensions invalid
 *
 * This recreates the offscreen QImage with the new dimensions and fills it
 * with the background color. All previous drawing is lost.
 *
 * Called by:
 * - WindowCreate() to create initial image
 * - WindowResize() to change dimensions
 *
 * Reference: miniwindow.cpp - CMiniWindow::Resize
 */
qint32 MiniWindow::Resize(qint32 newWidth, qint32 newHeight, QRgb bgColor)
{
    // Validate dimensions
    if (newWidth <= 0 || newHeight <= 0)
        return eBadParameter;

    // Update dimensions
    width = newWidth;
    height = newHeight;
    backgroundColor = bgColor;

    // Create new image with ARGB32 format (platform-independent, supports alpha)
    // Old image is automatically deleted when unique_ptr is reassigned
    image = std::make_unique<QImage>(width, height, QImage::Format_ARGB32);

    // Fill with background color - start fresh (no content preservation)
    // This matches original MUSHclient behavior where WindowCreate/Resize clears the window
    image->fill(bgrToColor(bgColor));

    // Mark for redraw
    dirty = true;
    emit needsRedraw();

    return eOK;
}

/**
 * @brief Clear the entire miniwindow with background color
 *
 * Fills the offscreen image with the current background color, erasing all
 * previous drawing.
 *
 * This is faster than manually drawing a filled rectangle covering the entire
 * miniwindow.
 *
 * Reference: miniwindow.cpp - CMiniWindow::Clear
 */
void MiniWindow::Clear()
{
    if (!image)
        return;

    image->fill(bgrToColor(backgroundColor));

    dirty = true;
    emit needsRedraw();
}

/**
 * @brief Delete all fonts, images, and hotspots
 *
 * Called by:
 * - Destructor (automatic cleanup)
 * - WindowDelete() (explicit cleanup)
 *
 * Note: We don't delete the pixmap here (that's done in destructor or Resize).
 * unique_ptr automatically handles deletion when cleared from the maps.
 *
 * Reference: miniwindow.cpp - CMiniWindow::DeleteContents
 */
void MiniWindow::DeleteAllResources()
{
    // Clear all fonts (no deletion needed - value types)
    fonts.clear();

    // Clear all images (unique_ptr handles deletion automatically)
    images.clear();

    // Clear all hotspots (unique_ptr handles deletion automatically)
    hotspots.clear();
}

// ========== Drawing Primitives ==========

/**
 * @brief RectOp - Draw rectangle with various operations
 *
 * Drawing Primitives
 * Actions: 1=frame (outline), 2=fill (solid), 3=invert (XOR), 5=3D rect
 *
 * Reference: miniwindow.cpp - CMiniWindow::RectOp
 */
qint32 MiniWindow::RectOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                          QRgb penColor, QRgb brushColor)
{
    if (!image)
        return eBadParameter;

    // Handle special meaning for right/bottom (from miniwindow.cpp)
    // - right <= 0 means offset from right edge (0 = right edge, -1 = 1px from right)
    // - bottom <= 0 means offset from bottom edge (0 = bottom edge, -1 = 1px from bottom)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    QPainter painter(image.get());
    // Don't use antialiasing for RectOp - need sharp pixel-perfect lines

    // Convert MFC CRect(left, top, right, bottom) to Qt QRect(x, y, width, height)
    // Windows GDI right/bottom are exclusive; Qt QRect right()=x+w-1, bottom()=y+h-1
    // For WindowRectOp, MUSHclient passes coordinates where right/bottom are exclusive
    // So we need to subtract 1 from width/height to get the correct Qt rect
    int w = fixedRight - left;
    int h = fixedBottom - top;
    QRect rect(left, top, w > 0 ? w : 0, h > 0 ? h : 0);

    switch (action) {
        case 1: { // Frame (outline only) - draw 4 lines manually for pixel-perfect match
            QColor color = bgrToColor(penColor);
            // Windows FrameRect draws inside the rect, excluding right/bottom edge pixels
            // Draw top line: left to right-1
            painter.setPen(color);
            painter.drawLine(left, top, fixedRight - 1, top);
            // Draw bottom line: left to right-1, at bottom-1
            painter.drawLine(left, fixedBottom - 1, fixedRight - 1, fixedBottom - 1);
            // Draw left line: top to bottom-1
            painter.drawLine(left, top, left, fixedBottom - 1);
            // Draw right line: top to bottom-1, at right-1
            painter.drawLine(fixedRight - 1, top, fixedRight - 1, fixedBottom - 1);
            break;
        }

        case 2: { // Fill (solid)
            // Uses brushColor for fill, but if brushColor is 0 (default), use penColor
            // This matches MUSHclient behavior where scripts often pass only one color
            QRgb fillColor = (brushColor != 0) ? brushColor : penColor;
            painter.setPen(Qt::NoPen);
            painter.setBrush(bgrToColor(fillColor));
            painter.fillRect(rect, QBrush(bgrToColor(fillColor)));
            break;
        }

        case 3: // Invert (XOR)
            painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
            painter.fillRect(rect, bgrToColor(penColor));
            break;

        case 5: { // 3D rectangle (DrawEdge style)
            // penColor is actually the edge type flags, not a color
            // Lower byte = inner edge: 0=none, 1=raised, 2=sunken
            // Upper byte = outer edge: 0=none, 1=raised, 2=sunken
            int innerEdge = penColor & 0xFF;
            int outerEdge = (penColor >> 8) & 0xFF;

            // Fill the rectangle first with brushColor
            painter.setPen(Qt::NoPen);
            painter.fillRect(rect, bgrToColor(brushColor));

            // Define colors for 3D effect (Windows standard 3D colors)
            QColor highlight(255, 255, 255);   // White - top/left outer highlight
            QColor lightShadow(192, 192, 192); // Light gray - top/left inner
            QColor darkShadow(64, 64, 64);     // Dark gray - bottom/right outer shadow
            QColor shadow(128, 128, 128);      // Medium gray - bottom/right inner

            // Draw outer edge
            if (outerEdge == 1) { // Raised outer
                // Top and left = highlight
                painter.setPen(highlight);
                painter.drawLine(rect.left(), rect.top(), rect.right(), rect.top());
                painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
                // Bottom and right = dark shadow
                painter.setPen(darkShadow);
                painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
                painter.drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());
            } else if (outerEdge == 2) { // Sunken outer
                // Top and left = dark shadow
                painter.setPen(darkShadow);
                painter.drawLine(rect.left(), rect.top(), rect.right(), rect.top());
                painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
                // Bottom and right = highlight
                painter.setPen(highlight);
                painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
                painter.drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());
            }

            // Draw inner edge (1 pixel inset)
            QRect innerRect = rect.adjusted(1, 1, -1, -1);
            if (innerEdge == 1) { // Raised inner
                painter.setPen(lightShadow);
                painter.drawLine(innerRect.left(), innerRect.top(), innerRect.right(),
                                 innerRect.top());
                painter.drawLine(innerRect.left(), innerRect.top(), innerRect.left(),
                                 innerRect.bottom());
                painter.setPen(shadow);
                painter.drawLine(innerRect.left(), innerRect.bottom(), innerRect.right(),
                                 innerRect.bottom());
                painter.drawLine(innerRect.right(), innerRect.top(), innerRect.right(),
                                 innerRect.bottom());
            } else if (innerEdge == 2) { // Sunken inner
                painter.setPen(shadow);
                painter.drawLine(innerRect.left(), innerRect.top(), innerRect.right(),
                                 innerRect.top());
                painter.drawLine(innerRect.left(), innerRect.top(), innerRect.left(),
                                 innerRect.bottom());
                painter.setPen(lightShadow);
                painter.drawLine(innerRect.left(), innerRect.bottom(), innerRect.right(),
                                 innerRect.bottom());
                painter.drawLine(innerRect.right(), innerRect.top(), innerRect.right(),
                                 innerRect.bottom());
            }
            break;
        }

        default:
            return eUnknownOption;
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief CircleOp - Draw circle/ellipse with various operations
 *
 * Drawing Primitives
 * Actions: 1=ellipse, 2=rectangle, 3=round_rect, 4=chord, 5=pie, 6=arc
 *
 * Reference: miniwindow.cpp - CMiniWindow::CircleOp
 */
qint32 MiniWindow::CircleOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                            QRgb penColor, qint32 penStyle, qint32 penWidth, QRgb brushColor,
                            qint32 brushStyle, qint32 extra1, qint32 extra2, qint32 extra3,
                            qint32 extra4)
{
    Q_UNUSED(extra3);
    Q_UNUSED(extra4);

    if (!image)
        return eBadParameter;

    // Handle special meaning for right/bottom (from miniwindow.cpp)
    // - right <= 0 means offset from right edge (0 = right edge, -1 = 1px from right)
    // - bottom <= 0 means offset from bottom edge (0 = bottom edge, -1 = 1px from bottom)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::Antialiasing);

    // Convert MFC CRect(left, top, right, bottom) to Qt QRect(x, y, width, height)
    QRect rect(left, top, fixedRight - left, fixedBottom - top);

    // Create pen with Windows GDI-compatible dash patterns
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);

    // Handle brush - map MUSHclient brush styles to Qt
    // MUSHclient hatched brushes have TWO colors:
    // - Pattern lines: penColor
    // - Background: brushColor
    // Qt patterned brushes only have one color with transparent background
    // Solution: Draw background first (solid brushColor), then pattern on top (penColor)

    bool isPatternBrush = (brushStyle >= 2 && brushStyle <= 12);

    if (isPatternBrush) {
        // For hatched patterns: first fill with background color (brushColor)
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgrToColor(brushColor));

        switch (action) {
            case 1: // Ellipse
                painter.drawEllipse(rect);
                break;
            case 2: // Rectangle
                painter.drawRect(rect);
                break;
            case 3: // Rounded rectangle
                painter.drawRoundedRect(rect, extra1, extra2);
                break;
            case 4: // Chord
                painter.drawChord(rect, extra1 * 16, extra2 * 16);
                break;
            case 5: // Pie
                painter.drawPie(rect, extra1 * 16, extra2 * 16);
                break;
            case 6: // Arc
                painter.drawArc(rect, extra1 * 16, extra2 * 16);
                break;
        }

        // Now set up pattern brush with penColor for the pattern lines
        Qt::BrushStyle qtBrushStyle;
        switch (brushStyle) {
            case 2:
                qtBrushStyle = Qt::HorPattern;
                break;
            case 3:
                qtBrushStyle = Qt::VerPattern;
                break;
            case 4:
                qtBrushStyle = Qt::FDiagPattern;
                break;
            case 5:
                qtBrushStyle = Qt::BDiagPattern;
                break;
            case 6:
                qtBrushStyle = Qt::CrossPattern;
                break;
            case 7:
                qtBrushStyle = Qt::DiagCrossPattern;
                break;
            case 8:
                qtBrushStyle = Qt::Dense6Pattern;
                break;
            case 9:
                qtBrushStyle = Qt::Dense4Pattern;
                break;
            case 10:
                qtBrushStyle = Qt::Dense2Pattern;
                break;
            default:
                qtBrushStyle = Qt::SolidPattern;
                break;
        }

        // Restore pen for drawing pattern
        painter.setPen(pen);
        painter.setBrush(QBrush(bgrToColor(penColor), qtBrushStyle));
    } else {
        // Solid or null brush - simple case
        Qt::BrushStyle qtBrushStyle;
        QRgb brushFillColor;

        if (brushStyle == 0) { // Solid
            qtBrushStyle = Qt::SolidPattern;
            brushFillColor = brushColor;
        } else { // Null or other
            qtBrushStyle = Qt::NoBrush;
            brushFillColor = brushColor;
        }

        painter.setBrush(QBrush(bgrToColor(brushFillColor), qtBrushStyle));
    }

    // Draw the shape (with pattern if applicable)
    switch (action) {
        case 1: // Ellipse
            painter.drawEllipse(rect);
            break;

        case 2: // Rectangle
            painter.drawRect(rect);
            break;

        case 3: // Rounded rectangle
            // extra1, extra2 are corner radii (x, y)
            painter.drawRoundedRect(rect, extra1, extra2);
            break;

        case 4: // Chord
            // extra1=start angle (degrees), extra2=span angle (degrees)
            // Qt uses 1/16 degree units
            painter.drawChord(rect, extra1 * 16, extra2 * 16);
            break;

        case 5: // Pie
            // extra1=start angle (degrees), extra2=span angle (degrees)
            painter.drawPie(rect, extra1 * 16, extra2 * 16);
            break;

        case 6: // Arc
            // extra1=start angle (degrees), extra2=span angle (degrees)
            painter.drawArc(rect, extra1 * 16, extra2 * 16);
            break;

        default:
            return eUnknownOption;
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief Line - Draw line
 *
 * Drawing Primitives
 *
 * Reference: miniwindow.cpp - CMiniWindow::Line
 */
qint32 MiniWindow::Line(qint32 x1, qint32 y1, qint32 x2, qint32 y2, QRgb penColor, qint32 penStyle,
                        qint32 penWidth)
{
    if (!image)
        return eBadParameter;

    QPainter painter(image.get());
    // Don't use antialiasing - can cause pixels to extend beyond bounds
    // painter.setRenderHint(QPainter::Antialiasing);

    // Create pen with Windows GDI-compatible dash patterns and styles
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);
    painter.drawLine(x1, y1, x2, y2);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief Arc - Draw arc
 *
 * Drawing Primitives
 * Draws an elliptical arc from point (x1,y1) to point (x2,y2)
 * within the bounding rectangle (left, top, right, bottom).
 *
 * Reference: miniwindow.cpp - CMiniWindow::Arc (line 972)
 *
 * Note: Windows GDI Arc() uses start/end points to define arc endpoints,
 * while Qt drawArc() uses start angle and span. We calculate the angles
 * from the points.
 */
qint32 MiniWindow::Arc(qint32 left, qint32 top, qint32 right, qint32 bottom, qint32 x1, qint32 y1,
                       qint32 x2, qint32 y2, QRgb penColor, qint32 penStyle, qint32 penWidth)
{
    if (!image)
        return eBadParameter;

    QPainter painter(image.get());

    // Create pen with Windows GDI-compatible dash patterns
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Handle special case: right=0 or bottom=0 means "use window edge"
    if (right <= 0)
        right = width + right;
    if (bottom <= 0)
        bottom = height + bottom;

    QRect rect(left, top, right - left, bottom - top);

    // Calculate center of ellipse
    qreal cx = (left + right) / 2.0;
    qreal cy = (top + bottom) / 2.0;

    // Calculate angles from center to start/end points (in degrees)
    qreal angle1 = qAtan2(y1 - cy, x1 - cx) * 180.0 / M_PI;
    qreal angle2 = qAtan2(y2 - cy, x2 - cx) * 180.0 / M_PI;

    // Calculate span angle (going counter-clockwise from angle1 to angle2)
    qreal spanAngle = angle2 - angle1;
    if (spanAngle < 0)
        spanAngle += 360;

    // Qt uses 1/16 degree units and measures angles counter-clockwise from 3 o'clock
    int startAngle16 = qRound(angle1 * 16);
    int spanAngle16 = qRound(spanAngle * 16);

    painter.drawArc(rect, startAngle16, spanAngle16);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief Bezier - Draw Bezier curve
 *
 * Drawing Primitives
 * Draws cubic Bezier curves through a series of control points.
 *
 * Reference: miniwindow.cpp - CMiniWindow::Bezier (line 1520)
 *
 * @param points Comma-separated list of x,y coordinates
 *               Must be (3n+1) points: start + (3 control points per curve)
 *               e.g., "10,20,30,40,50,60,70,80" = 4 points (1 cubic Bezier)
 */
qint32 MiniWindow::Bezier(const QString& pointsStr, QRgb penColor, qint32 penStyle, qint32 penWidth)
{
    if (!image)
        return eBadParameter;

    // Parse comma-separated points
    QStringList parts = pointsStr.split(',');
    int count = parts.size();

    // Must have at least 8 numbers (4 points: start + 3 control)
    if (count < 8)
        return eInvalidNumberOfPoints;

    // Must be even (pairs of x,y)
    if (count % 2 != 0)
        return eInvalidNumberOfPoints;

    // Must be (3n+1) points, so 4, 7, 10, 13... points
    // which is 8, 14, 20, 26... numbers
    // Formula: (count/2 - 1) % 3 == 0
    int numPoints = count / 2;
    if ((numPoints - 1) % 3 != 0)
        return eInvalidNumberOfPoints;

    // Convert to points
    QVector<QPointF> points;
    for (int i = 0; i < count; i += 2) {
        bool okX, okY;
        qreal x = parts[i].toDouble(&okX);
        qreal y = parts[i + 1].toDouble(&okY);
        if (!okX || !okY)
            return eInvalidPoint;
        points.append(QPointF(x, y));
    }

    QPainter painter(image.get());

    // Create pen with Windows GDI-compatible dash patterns
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);

    // Build Bezier path
    QPainterPath path;
    path.moveTo(points[0]);

    // Each cubic Bezier segment uses 3 points
    for (int i = 1; i < points.size(); i += 3) {
        if (i + 2 < points.size()) {
            path.cubicTo(points[i], points[i + 1], points[i + 2]);
        }
    }

    painter.drawPath(path);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief SetPixel - Set pixel color
 *
 * Drawing Primitives
 *
 * Reference: miniwindow.cpp - CMiniWindow::SetPixel
 */
qint32 MiniWindow::SetPixel(qint32 x, qint32 y, QRgb color)
{
    if (!image)
        return eBadParameter;

    if (x < 0 || x >= image->width() || y < 0 || y >= image->height())
        return eBadParameter;

    // Convert BGR (from Lua) to ARGB (for QImage)
    image->setPixel(x, y, bgrToQRgb(color));

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief GetPixel - Get pixel color
 *
 * Drawing Primitives
 *
 * Reference: miniwindow.cpp - CMiniWindow::GetPixel
 */
QRgb MiniWindow::GetPixel(qint32 x, qint32 y)
{
    if (!image)
        return 0;

    if (x < 0 || x >= image->width() || y < 0 || y >= image->height())
        return 0;

    // Convert ARGB (from QImage) to BGR (for Lua)
    return qRgbToBgr(image->pixel(x, y));
}

/**
 * @brief Polygon - Draw polygon
 *
 * Drawing Primitives
 *
 * Reference: miniwindow.cpp - CMiniWindow::Polygon
 */
qint32 MiniWindow::Polygon(const QString& points, QRgb penColor, qint32 penStyle, qint32 penWidth,
                           QRgb brushColor, qint32 brushStyle, bool close, bool winding)
{
    if (!image)
        return eBadParameter;

    // Parse comma-separated coordinate pairs (x1,y1,x2,y2,...)
    QStringList coords = points.split(',', Qt::SkipEmptyParts);
    if (coords.size() < 4 || coords.size() % 2 != 0) // Need at least 2 points (4 coords)
        return eBadParameter;

    // Convert to QPolygon
    QPolygon polygon;
    for (int i = 0; i < coords.size(); i += 2) {
        bool okX, okY;
        int x = coords[i].toInt(&okX);
        int y = coords[i + 1].toInt(&okY);
        if (!okX || !okY)
            return eBadParameter;
        polygon << QPoint(x, y);
    }

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::Antialiasing);

    // Create pen with Windows GDI-compatible dash patterns
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);

    // Handle brush - map MUSHclient brush styles to Qt
    // MUSHclient hatched brushes have TWO colors:
    // - Pattern lines: penColor
    // - Background: brushColor
    // Qt patterned brushes only have one color with transparent background
    // Solution: Draw background first (solid brushColor), then pattern on top (penColor)

    Qt::FillRule fillRule = winding ? Qt::WindingFill : Qt::OddEvenFill;
    bool isPatternBrush = (brushStyle >= 2 && brushStyle <= 12);

    if (isPatternBrush) {
        // For hatched patterns: first fill with background color (brushColor)
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgrToColor(brushColor));

        if (close) {
            painter.drawPolygon(polygon, fillRule);
        } else {
            painter.drawPolyline(polygon);
        }

        // Now set up pattern brush with penColor for the pattern lines
        Qt::BrushStyle qtBrushStyle;
        switch (brushStyle) {
            case 2:
                qtBrushStyle = Qt::HorPattern;
                break;
            case 3:
                qtBrushStyle = Qt::VerPattern;
                break;
            case 4:
                qtBrushStyle = Qt::FDiagPattern;
                break;
            case 5:
                qtBrushStyle = Qt::BDiagPattern;
                break;
            case 6:
                qtBrushStyle = Qt::CrossPattern;
                break;
            case 7:
                qtBrushStyle = Qt::DiagCrossPattern;
                break;
            case 8:
                qtBrushStyle = Qt::Dense6Pattern;
                break;
            case 9:
                qtBrushStyle = Qt::Dense4Pattern;
                break;
            case 10:
                qtBrushStyle = Qt::Dense2Pattern;
                break;
            default:
                qtBrushStyle = Qt::SolidPattern;
                break;
        }

        // Restore pen for drawing pattern
        painter.setPen(pen);
        painter.setBrush(QBrush(bgrToColor(penColor), qtBrushStyle));
    } else {
        // Solid or null brush - simple case
        Qt::BrushStyle qtBrushStyle;
        QRgb brushFillColor;

        if (brushStyle == 0) { // Solid
            qtBrushStyle = Qt::SolidPattern;
            brushFillColor = brushColor;
        } else { // Null or other
            qtBrushStyle = Qt::NoBrush;
            brushFillColor = brushColor;
        }

        painter.setBrush(QBrush(bgrToColor(brushFillColor), qtBrushStyle));
    }

    // Draw polygon (closed or open polyline) with pattern if applicable
    if (close) {
        painter.drawPolygon(polygon, fillRule);
    } else {
        painter.drawPolyline(polygon);
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief Gradient - Draw gradient fill
 *
 * Drawing Primitives
 * Fills a rectangle with a smooth color gradient.
 *
 * @param left Left coordinate
 * @param top Top coordinate
 * @param right Right coordinate
 * @param bottom Bottom coordinate
 * @param color1 Start color (ARGB)
 * @param color2 End color (ARGB)
 * @param mode Gradient mode (1=horizontal left-to-right, 2=vertical top-to-bottom)
 * @return eOK on success, eBadParameter on error
 *
 * Reference: MUSHclient WindowGradient function
 */
qint32 MiniWindow::Gradient(qint32 left, qint32 top, qint32 right, qint32 bottom, QRgb color1,
                            QRgb color2, qint32 mode)
{
    if (!image)
        return eBadParameter;

    // Handle special meaning for right/bottom (from miniwindow.cpp)
    // - right <= 0 means offset from right edge (0 = right edge, -1 = 1px from right)
    // - bottom <= 0 means offset from bottom edge (0 = bottom edge, -1 = 1px from bottom)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    // Bounds check
    if (left < 0 || top < 0 || fixedRight > width || fixedBottom > height)
        return eBadParameter;

    // Convert to QRect
    QRect rect(left, top, fixedRight - left, fixedBottom - top);

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::Antialiasing);

    // Create gradient based on mode
    QLinearGradient gradient;

    switch (mode) {
        case 1: // Horizontal (left to right)
            gradient = QLinearGradient(rect.left(), rect.top(), rect.right(), rect.top());
            break;

        case 2: // Vertical (top to bottom)
            gradient = QLinearGradient(rect.left(), rect.top(), rect.left(), rect.bottom());
            break;

        default:
            return eBadParameter;
    }

    // Set gradient colors
    gradient.setColorAt(0.0, bgrToColor(color1));
    gradient.setColorAt(1.0, bgrToColor(color2));

    // Fill the rectangle with gradient
    painter.fillRect(rect, gradient);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

// ========== Text and Fonts ==========

/**
 * @brief Font - Create or update font
 *
 * Text Drawing and Font Management
 *
 * Reference: miniwindow.cpp - CMiniWindow::Font
 */
qint32 MiniWindow::Font(const QString& fontId, const QString& fontName, double size, bool bold,
                        bool italic, bool underline, bool strikeout)
{
    QFont font(fontName);

#ifdef Q_OS_MACOS
    // macOS uses 72 DPI logical, but MUSHclient plugins expect Windows GDI sizing (96 DPI)
    // Convert point size to pixel size: pointSize * 96 / 72 = pixelSize
    int pixelSize = qRound(size * 96.0 / 72.0);
    font.setPixelSize(pixelSize);
#else
    // Windows/Linux: use point size directly, Qt handles DPI conversion
    // This respects system DPI scaling settings
    font.setPointSizeF(size);
#endif

    font.setBold(bold);
    font.setItalic(italic);
    font.setUnderline(underline);
    font.setStrikeOut(strikeout);
    font.setStyleStrategy(QFont::PreferAntialias);

    // Store font (replaces existing if present)
    fonts[fontId] = font;
    return eOK;
}

/**
 * @brief Text - Draw text
 *
 * Text Drawing and Font Management
 *
 * Reference: miniwindow.cpp - CMiniWindow::Text
 */
qint32 MiniWindow::Text(const QString& fontId, const QString& text, qint32 left, qint32 top,
                        qint32 right, qint32 bottom, QRgb color, bool unicode)
{
    if (!image)
        return eBadParameter;

    if (!fonts.contains(fontId))
        return -2; // Font not found

    QString displayText = unicode ? QString::fromUtf8(text.toUtf8()) : text;

    // Handle special meaning for right/bottom (from miniwindow.cpp)
    // - right <= 0 means offset from right edge (0 = right edge, -1 = 1px from right)
    // - bottom <= 0 means offset from bottom edge (0 = bottom edge, -1 = 1px from bottom)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(fonts[fontId]);
    painter.setPen(bgrToColor(color));

    // Set clipping rectangle (ETO_CLIPPED behavior from ExtTextOutW)
    // The rect is for clipping only - text is positioned at (left, top)
    QRect clipRect(left, top, fixedRight - left, fixedBottom - top);
    painter.setClipRect(clipRect);

    // Draw text at position (not in layout box)
    // Use font metrics to get baseline offset
    QFontMetrics fm(fonts[fontId]);
    painter.drawText(left, top + fm.ascent(), displayText);

    dirty = true;
    emit needsRedraw();

    // Return the width of the text drawn (MUSHclient API returns text length in pixels)
    return fm.horizontalAdvance(displayText);
}

/**
 * @brief TextWidth - Measure text width
 *
 * Text Drawing and Font Management
 *
 * Reference: miniwindow.cpp - CMiniWindow::TextWidth
 */
qint32 MiniWindow::TextWidth(const QString& fontId, const QString& text, bool unicode)
{
    if (!fonts.contains(fontId))
        return 0;

    QString measureText = unicode ? QString::fromUtf8(text.toUtf8()) : text;

    QFontMetrics fm(fonts[fontId]);
    return fm.horizontalAdvance(measureText);
}

/**
 * @brief FontInfo - Get font information
 *
 * Text Drawing and Font Management
 *
 * Reference: miniwindow.cpp - CMiniWindow::FontInfo
 */
QVariant MiniWindow::FontInfo(const QString& fontId, qint32 infoType)
{
    if (!fonts.contains(fontId))
        return QVariant();

    const QFont& font = fonts[fontId];
    QFontMetrics fm(font);

    // Windows TEXTMETRIC compatible font info
    // Reference: miniwindow.cpp CMiniWindow::FontInfo
    switch (infoType) {
        case 1:
            return fm.height(); // tmHeight - total height of font
        case 2:
            return fm.ascent(); // tmAscent - height above baseline
        case 3:
            return fm.descent(); // tmDescent - height below baseline
        case 4:
            return fm.leading(); // tmInternalLeading - space above characters
        case 5:
            return 0; // tmExternalLeading - extra space between lines (Qt doesn't have this)
        case 6:
            return fm.averageCharWidth(); // tmAveCharWidth
        case 7:
            return fm.maxWidth(); // tmMaxCharWidth
        case 8:
            return font.weight(); // tmWeight (QFont::Weight enum, compatible with Windows values)
        case 9:
            return fm.leftBearing('x') + fm.rightBearing('x'); // tmOverhang approximation
        default:
            return QVariant();
    }
}

/**
 * @brief FontList - Get list of all font IDs
 *
 * Text Drawing and Font Management
 *
 * Returns a QStringList of all font IDs registered in this miniwindow.
 * Used for iteration and debugging.
 *
 * Reference: miniwindow.cpp - CMiniWindow::GetFontList
 */
QStringList MiniWindow::FontList()
{
    return fonts.keys();
}

// ========== Image Operations ==========

/**
 * @brief LoadImage - Load image file into cache
 *
 * Image Operations
 * Loads an image file (PNG, BMP, JPG) from disk and stores it with the given ID.
 * Images can be reused multiple times without reloading.
 *
 * @param imageId Identifier for the image
 * @param filepath Path to image file
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::LoadImage(const QString& imageId, const QString& filepath)
{
    // Empty filepath means remove the image
    if (filepath.trimmed().isEmpty()) {
        // Erase from map (unique_ptr automatically deletes)
        images.erase(imageId);
        return eOK;
    }

    // Load image using Qt (QImage is platform-independent)
    auto img = std::make_unique<QImage>(filepath);
    if (img->isNull()) {
        return 30051; // eFileNotFound / eCouldNotOpenFile
    }

    // Insert new image (replaces old one if it exists; old unique_ptr automatically deletes)
    images[imageId] = std::move(img);
    return eOK;
}

/**
 * @brief DrawImage - Draw image with scaling and sprite sheet support
 *
 * Image Operations
 * Draws an image to the miniwindow, optionally scaling to fit the destination
 * rectangle and supporting sprite sheets via source rectangle.
 *
 * @param imageId Image identifier
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate
 * @param bottom Destination bottom coordinate
 * @param mode Draw mode (1=copy/overwrite, 2=transparent_copy)
 * @param srcLeft Source left (0 = use full image)
 * @param srcTop Source top
 * @param srcRight Source right (0 = use full width)
 * @param srcBottom Source bottom (0 = use full height)
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::DrawImage(const QString& imageId, qint32 left, qint32 top, qint32 right,
                             qint32 bottom, qint16 mode, qint32 srcLeft, qint32 srcTop,
                             qint32 srcRight, qint32 srcBottom)
{
    if (!image)
        return eBadParameter;

    // Get raw pointer from unique_ptr in map
    auto it = images.find(imageId);
    if (it == images.end() || !it->second)
        return 30009; // eImageNotFound
    QImage* img = it->second.get();

    // Handle special meaning for right/bottom (from miniwindow.cpp)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QRect destRect(left, top, fixedRight - left, fixedBottom - top);
    QRect srcRect(srcLeft, srcTop, srcRight - srcLeft, srcBottom - srcTop);

    // If source rect is 0,0,0,0 use full image
    if (srcRect.isEmpty() || srcRect.width() == 0 || srcRect.height() == 0) {
        srcRect = QRect(0, 0, img->width(), img->height());
    }

    // Set composition mode based on draw mode
    switch (mode) {
        case 1: // image_copy (overwrite)
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            break;
        case 2: // image_transparent_copy (respect alpha)
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            break;
        default:
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            break;
    }

    painter.drawImage(destRect, *img, srcRect);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief BlendImage - Draw image with opacity and blend modes
 *
 * Image Operations
 * Draws an image with alpha blending and various composition modes
 * for advanced visual effects.
 *
 * @param imageId Image identifier
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate
 * @param bottom Destination bottom coordinate
 * @param mode Blend mode (1=normal, 2=multiply, 3=screen, 4=overlay)
 * @param opacity Opacity (0.0 = transparent, 1.0 = opaque)
 * @param srcLeft Source left (0 = use full image)
 * @param srcTop Source top
 * @param srcRight Source right
 * @param srcBottom Source bottom
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::BlendImage(const QString& imageId, qint32 left, qint32 top, qint32 right,
                              qint32 bottom, qint16 mode, double opacity, qint32 srcLeft,
                              qint32 srcTop, qint32 srcRight, qint32 srcBottom)
{
    if (!image)
        return eBadParameter;

    // Get raw pointer from unique_ptr in map
    auto it = images.find(imageId);
    if (it == images.end() || !it->second)
        return 30009; // eImageNotFound
    QImage* img = it->second.get();

    // Handle special meaning for right/bottom
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setOpacity(opacity);

    // Map blend mode to Qt composition mode
    QPainter::CompositionMode compMode;
    switch (mode) {
        case 1: // Normal (SourceOver)
            compMode = QPainter::CompositionMode_SourceOver;
            break;
        case 2: // Multiply
            compMode = QPainter::CompositionMode_Multiply;
            break;
        case 3: // Screen
            compMode = QPainter::CompositionMode_Screen;
            break;
        case 4: // Overlay
            compMode = QPainter::CompositionMode_Overlay;
            break;
        default:
            compMode = QPainter::CompositionMode_SourceOver;
            break;
    }
    painter.setCompositionMode(compMode);

    QRect destRect(left, top, fixedRight - left, fixedBottom - top);
    QRect srcRect(srcLeft, srcTop, srcRight - srcLeft, srcBottom - srcTop);

    // If source rect is empty, use full image
    if (srcRect.isEmpty() || srcRect.width() == 0 || srcRect.height() == 0) {
        srcRect = QRect(0, 0, img->width(), img->height());
    }

    painter.drawImage(destRect, *img, srcRect);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief ImageFromWindow - Copy another miniwindow's pixmap as an image (interface version)
 *
 * Image Operations
 * Copies the entire contents of another miniwindow into this miniwindow's
 * image cache. Useful for composition and layering effects.
 *
 * This version implements the MiniWindow interface and takes a source window directly.
 *
 * @param imageId Image identifier to store the copy under
 * @param srcWindow Source miniwindow interface pointer
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::ImageFromWindow(const QString& imageId, MiniWindow* srcWindow)
{
    if (!srcWindow)
        return eBadParameter;

    const QImage* srcImage = srcWindow->getImage();
    if (!srcImage)
        return eBadParameter;

    // Create a copy of the source image
    auto imgCopy = std::make_unique<QImage>(*srcImage);

    // Insert new image (replaces old one if it exists; old unique_ptr automatically deletes)
    images[imageId] = std::move(imgCopy);
    return eOK;
}

/**
 * @brief ImageFromWindow - Copy another miniwindow's pixmap as an image (legacy version)
 *
 * Image Operations
 * Copies the entire contents of another miniwindow into this miniwindow's
 * image cache. Useful for composition and layering effects.
 *
 * This is the legacy version that takes WorldDocument + window name.
 * It delegates to the interface version.
 *
 * @param imageId Image identifier to store under
 * @param srcDoc Parent world document
 * @param srcWindowName Source miniwindow name
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::ImageFromWindow(const QString& imageId, WorldDocument* srcDoc,
                                   const QString& srcWindowName)
{
    if (!srcDoc)
        return eBadParameter;

    // Find source miniwindow
    auto it = srcDoc->m_MiniWindowMap.find(srcWindowName);
    if (it == srcDoc->m_MiniWindowMap.end() || !it->second)
        return eBadParameter; // Source window not found
    MiniWindow* srcWin = it->second.get();

    // Delegate to interface version
    return ImageFromWindow(imageId, srcWin);
}

/**
 * @brief ImageInfo - Get image information
 *
 * Image Operations
 * Returns metadata about a loaded image.
 *
 * @param imageId Image identifier
 * @param infoType Info type (1=width, 2=height)
 * @return QVariant with requested info, or empty on error
 */
QVariant MiniWindow::ImageInfo(const QString& imageId, qint32 infoType)
{
    // Get raw pointer from unique_ptr in map
    auto it = images.find(imageId);
    if (it == images.end() || !it->second)
        return QVariant();
    QImage* img = it->second.get();

    switch (infoType) {
        case 1:
            return img->width();
        case 2:
            return img->height();
        default:
            return QVariant();
    }
}

/**
 * @brief ImageList - Get list of all loaded image IDs
 *
 * Image Operations
 * Returns all image identifiers currently loaded in this miniwindow.
 *
 * @return QStringList of image IDs
 */
QStringList MiniWindow::ImageList()
{
    QStringList result;
    for (const auto& pair : images) {
        result.append(pair.first);
    }
    return result;
}

/**
 * @brief HotspotList - Get list of all hotspot IDs in this miniwindow
 *
 * @return QStringList of hotspot IDs
 */
QStringList MiniWindow::HotspotList()
{
    QStringList result;
    for (const auto& pair : hotspots) {
        result.append(pair.first);
    }
    return result;
}

/**
 * @brief CreateImage - Create an 8x8 monochrome bitmap from row values
 *
 * Each row parameter contains 8 bits representing pixels.
 * Bit 7 (0x80) is the leftmost pixel, bit 0 (0x01) is the rightmost.
 * Set bits are drawn in foreground color, clear bits in background color.
 *
 * @param imageId Image identifier to store under
 * @param row1-row8 Bit patterns for each row (8 rows x 8 pixels = 64 pixels)
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CreateImage function
 */
qint32 MiniWindow::CreateImage(const QString& imageId, qint32 row1, qint32 row2, qint32 row3,
                               qint32 row4, qint32 row5, qint32 row6, qint32 row7, qint32 row8)
{
    if (imageId.isEmpty()) {
        return eNoNameSpecified;
    }

    // Create 8x8 monochrome image
    auto img = std::make_unique<QImage>(8, 8, QImage::Format_ARGB32);

    // Row values array - reversed order because bitmap rows are bottom-up
    // Row1 is the TOP row visually, but stored last in bitmap data
    // Match original MUSHclient: Row8 at y=0, Row1 at y=7
    qint32 rows[8] = {row8, row7, row6, row5, row4, row3, row2, row1};

    // Fill pixels - bit 7 is leftmost, bit 0 is rightmost
    for (int y = 0; y < 8; y++) {
        qint32 rowValue = rows[y];
        for (int x = 0; x < 8; x++) {
            // Check bit (7-x) for pixel at position x
            bool set = (rowValue & (0x80 >> x)) != 0;
            // Set pixels: white (0xFFFFFFFF) for set bits, black (0xFF000000) for clear
            img->setPixel(x, y, set ? qRgba(255, 255, 255, 255) : qRgba(0, 0, 0, 255));
        }
    }

    // Store in image cache (replaces existing if present)
    images[imageId] = std::move(img);
    return eOK;
}

/**
 * @brief ImageOp - Draw shapes using an image as brush pattern
 *
 * Similar to RectOp/CircleOp but uses an image for the brush pattern.
 * The image is tiled to fill the shape.
 *
 * @param action Operation type (1=frame rect, 2=fill rect, 3=round rect frame, etc.)
 * @param left Left coordinate
 * @param top Top coordinate
 * @param right Right coordinate
 * @param bottom Bottom coordinate
 * @param penColor Pen color for outline
 * @param penStyle Pen style
 * @param penWidth Pen width
 * @param brushColor Background color (for 0-bits in image pattern)
 * @param imageId Image to use as brush pattern
 * @param ellipseWidth Corner width for rounded rectangles
 * @param ellipseHeight Corner height for rounded rectangles
 * @return Error code
 *
 * Reference: miniwindow.cpp ImageOp function
 */
qint32 MiniWindow::ImageOp(qint16 action, qint32 left, qint32 top, qint32 right, qint32 bottom,
                           QRgb penColor, qint32 penStyle, qint32 penWidth, QRgb brushColor,
                           const QString& imageId, qint32 ellipseWidth, qint32 ellipseHeight)
{
    if (!image) {
        return eNoSuchWindow;
    }

    // Find the brush image
    auto it = images.find(imageId);
    if (it == images.end() || !it->second) {
        return eImageNotInstalled;
    }

    QImage* brushImg = it->second.get();

    // Create painter
    QPainter painter(image.get());
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Setup pen (convert BGR to Qt color)
    QPen pen(bgrToColor(penColor));
    pen.setWidth(penWidth);
    pen.setStyle(static_cast<Qt::PenStyle>(penStyle));
    painter.setPen(pen);

    // Setup brush with image pattern
    QBrush brush(*brushImg);
    brush.setStyle(Qt::TexturePattern);
    painter.setBrush(brush);

    // Normalize coordinates
    if (left > right)
        std::swap(left, right);
    if (top > bottom)
        std::swap(top, bottom);

    QRect rect(left, top, right - left, bottom - top);

    // Perform operation based on action
    // Match original MUSHclient action codes: 1=ellipse, 2=rectangle, 3=round rect
    switch (action) {
        case 1: // Ellipse (filled with pattern brush)
            painter.drawEllipse(rect);
            break;
        case 2: // Rectangle (filled with pattern brush)
            painter.drawRect(rect);
            break;
        case 3: // Rounded rectangle (filled with pattern brush)
            painter.drawRoundedRect(rect, ellipseWidth, ellipseHeight);
            break;
        default:
            return eUnknownOption;
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief LoadImageMemory - Load image from memory buffer
 *
 * Loads an image from raw bytes (PNG, BMP, JPG, GIF, etc.) into the image cache.
 * Qt's QImage::loadFromData handles format detection automatically.
 *
 * @param imageId Image identifier to store under
 * @param data Raw image data
 * @param length Length of data in bytes
 * @param hasAlpha Whether to preserve alpha channel (convert to ARGB32 if true)
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp LoadImageMemory function
 */
qint32 MiniWindow::LoadImageMemory(const QString& imageId, const unsigned char* data, size_t length,
                                   bool hasAlpha)
{
    if (imageId.isEmpty()) {
        return eNoNameSpecified;
    }

    if (!data || length == 0) {
        return eBadParameter;
    }

    // Load image from memory
    auto img = std::make_unique<QImage>();
    if (!img->loadFromData(data, static_cast<int>(length))) {
        return eUnableToLoadImage;
    }

    // Convert to appropriate format
    if (hasAlpha) {
        *img = img->convertToFormat(QImage::Format_ARGB32);
    } else {
        *img = img->convertToFormat(QImage::Format_RGB32);
    }

    // Store in image cache
    images[imageId] = std::move(img);
    return eOK;
}

/**
 * @brief Write - Save miniwindow to BMP or PNG file
 *
 * Image Operations
 * Saves the entire miniwindow drawing surface to a file.
 * Supports BMP (24-bit uncompressed) and PNG (compressed) formats.
 *
 * @param filename Output file path (must end in .bmp or .png)
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowWrite()
 */
qint32 MiniWindow::Write(const QString& filename)
{
    // Validate filename
    if (filename.isEmpty()) {
        return eNoNameSpecified;
    }

    // Filename must be at least 5 characters (.bmp or .png)
    if (filename.length() < 5) {
        return eBadParameter;
    }

    // Check file extension
    QString lower = filename.toLower();
    if (!lower.endsWith(".bmp") && !lower.endsWith(".png")) {
        return eBadParameter;
    }

    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Save the image using Qt's built-in save function
    // Qt automatically handles BMP and PNG formats
    if (!image->save(filename)) {
        // Could not write file
        return eCouldNotOpenFile;
    }

    return eOK;
}

/**
 * @brief GetImageAlpha - Extract alpha channel from image as grayscale
 *
 * Image Operations
 * Extracts the alpha channel from a 32-bit RGBA image and displays it as grayscale
 * in the miniwindow. Each pixel's alpha value (0-255) becomes a grayscale value (R=G=B=alpha).
 *
 * @param imageId Image identifier (must be 32-bit with alpha channel)
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param srcLeft Source left coordinate in image
 * @param srcTop Source top coordinate in image
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowGetImageAlpha()
 */
qint32 MiniWindow::GetImageAlpha(const QString& imageId, qint32 left, qint32 top, qint32 right,
                                 qint32 bottom, qint32 srcLeft, qint32 srcTop)
{
    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Find the source image
    auto it = images.find(imageId);
    if (it == images.end() || !it->second) {
        return eImageNotInstalled;
    }
    QImage* srcImage = it->second.get();

    // Image must be 32-bit ARGB format with alpha channel
    if (srcImage->format() != QImage::Format_ARGB32 &&
        srcImage->format() != QImage::Format_ARGB32_Premultiplied) {
        return eImageNotInstalled;
    }

    // Handle right=0 or bottom=0 (use window dimensions)
    if (right <= 0)
        right = width + right;
    if (bottom <= 0)
        bottom = height + bottom;

    // Constrain to window bounds
    left = qMax(0, qMin(left, width));
    top = qMax(0, qMin(top, height));
    right = qMax(0, qMin(right, width));
    bottom = qMax(0, qMin(bottom, height));

    // Calculate dimensions
    qint32 destWidth = right - left;
    qint32 destHeight = bottom - top;

    if (destWidth <= 0 || destHeight <= 0) {
        return eBadParameter;
    }

    // Check source bounds
    if (srcLeft < 0 || srcTop < 0 || srcLeft >= srcImage->width() || srcTop >= srcImage->height()) {
        return eBadParameter;
    }

    // Calculate actual copy dimensions (limited by source image size)
    qint32 copyWidth = qMin(destWidth, srcImage->width() - srcLeft);
    qint32 copyHeight = qMin(destHeight, srcImage->height() - srcTop);

    // Extract alpha channel and write as grayscale to window
    QPainter painter(image.get());

    for (qint32 y = 0; y < copyHeight; y++) {
        for (qint32 x = 0; x < copyWidth; x++) {
            // Get source pixel
            QRgb srcPixel = srcImage->pixel(srcLeft + x, srcTop + y);

            // Extract alpha channel
            int alpha = qAlpha(srcPixel);

            // Create grayscale color from alpha (R=G=B=alpha)
            QRgb gray = qRgb(alpha, alpha, alpha);

            // Set destination pixel
            image->setPixel(left + x, top + y, gray);
        }
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief DrawImageAlpha - Draw image with alpha channel blending
 *
 * Image Operations
 * Draws a 32-bit RGBA image with alpha channel blending onto the miniwindow.
 * Applies an additional opacity multiplier for fading effects.
 *
 * @param imageId Image identifier (must be 32-bit with alpha channel)
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param opacity Additional opacity multiplier (0.0 = transparent, 1.0 = opaque)
 * @param srcLeft Source left coordinate in image
 * @param srcTop Source top coordinate in image
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowDrawImageAlpha()
 */
qint32 MiniWindow::DrawImageAlpha(const QString& imageId, qint32 left, qint32 top, qint32 right,
                                  qint32 bottom, double opacity, qint32 srcLeft, qint32 srcTop)
{
    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Validate opacity
    if (opacity < 0.0 || opacity > 1.0) {
        return eBadParameter;
    }

    // Find the source image
    auto it = images.find(imageId);
    if (it == images.end() || !it->second) {
        return eImageNotInstalled;
    }
    QImage* srcImage = it->second.get();

    // Image must be 32-bit ARGB format with alpha channel
    if (srcImage->format() != QImage::Format_ARGB32 &&
        srcImage->format() != QImage::Format_ARGB32_Premultiplied) {
        return eImageNotInstalled;
    }

    // Handle right=0 or bottom=0 (use window dimensions)
    if (right <= 0)
        right = width + right;
    if (bottom <= 0)
        bottom = height + bottom;

    // Constrain to window bounds
    left = qMax(0, qMin(left, width));
    top = qMax(0, qMin(top, height));
    right = qMax(0, qMin(right, width));
    bottom = qMax(0, qMin(bottom, height));

    // Calculate dimensions
    qint32 destWidth = right - left;
    qint32 destHeight = bottom - top;

    if (destWidth <= 0 || destHeight <= 0) {
        return eBadParameter;
    }

    // Check source bounds
    if (srcLeft < 0 || srcTop < 0) {
        return eBadParameter;
    }

    // Calculate source rectangle
    qint32 srcWidth = qMin(destWidth, srcImage->width() - srcLeft);
    qint32 srcHeight = qMin(destHeight, srcImage->height() - srcTop);

    if (srcWidth <= 0 || srcHeight <= 0) {
        return eBadParameter;
    }

    // Use QPainter to draw with alpha blending
    QPainter painter(image.get());

    // Set opacity
    painter.setOpacity(opacity);

    // Use source-over composition (alpha blending)
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Draw the source region to destination
    QRect destRect(left, top, destWidth, destHeight);
    QRect srcRect(srcLeft, srcTop, srcWidth, srcHeight);

    painter.drawImage(destRect, *srcImage, srcRect);

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief MergeImageAlpha - Merge image with separate alpha mask
 *
 * Image Operations
 * Merges an image with a separate alpha mask image. The mask controls
 * transparency - brighter mask pixels = more opaque result.
 *
 * @param imageId Main image identifier
 * @param maskId Mask image identifier (grayscale values used as alpha)
 * @param left Destination left coordinate
 * @param top Destination top coordinate
 * @param right Destination right coordinate (0 = window width)
 * @param bottom Destination bottom coordinate (0 = window height)
 * @param mode Blend mode: 0=normal mask, 1=color-key transparency
 * @param opacity Additional opacity multiplier (0.0-1.0)
 * @param srcLeft Source left coordinate
 * @param srcTop Source top coordinate
 * @param srcRight Source right coordinate
 * @param srcBottom Source bottom coordinate
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowMergeImageAlpha()
 */
qint32 MiniWindow::MergeImageAlpha(const QString& imageId, const QString& maskId, qint32 left,
                                   qint32 top, qint32 right, qint32 bottom, qint16 mode,
                                   double opacity, qint32 srcLeft, qint32 srcTop, qint32 srcRight,
                                   qint32 srcBottom)
{
    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Validate opacity
    if (opacity < 0.0 || opacity > 1.0) {
        return eBadParameter;
    }

    // Find the source image
    auto imgIt = images.find(imageId);
    if (imgIt == images.end() || !imgIt->second) {
        return eImageNotInstalled;
    }
    QImage* srcImage = imgIt->second.get();

    // Find the mask image
    auto maskIt = images.find(maskId);
    if (maskIt == images.end() || !maskIt->second) {
        return eImageNotInstalled;
    }
    QImage* maskImage = maskIt->second.get();

    // Handle right=0 or bottom=0 (use window dimensions)
    if (right <= 0)
        right = width + right;
    if (bottom <= 0)
        bottom = height + bottom;

    // Constrain to window bounds
    left = qMax(0, qMin(left, width));
    top = qMax(0, qMin(top, height));
    right = qMax(0, qMin(right, width));
    bottom = qMax(0, qMin(bottom, height));

    // Calculate dimensions
    qint32 destWidth = right - left;
    qint32 destHeight = bottom - top;

    if (destWidth <= 0 || destHeight <= 0) {
        return eBadParameter;
    }

    // Handle source rectangle defaults
    if (srcRight <= 0)
        srcRight = srcImage->width();
    if (srcBottom <= 0)
        srcBottom = srcImage->height();

    qint32 srcWidth = srcRight - srcLeft;
    qint32 srcHeight = srcBottom - srcTop;

    if (srcWidth <= 0 || srcHeight <= 0) {
        return eBadParameter;
    }

    // Validate mask size
    if (maskImage->width() < srcWidth || maskImage->height() < srcHeight) {
        return eBadParameter;
    }

    // Mode 1: Color-key transparency (pixel at 0,0 becomes transparent)
    QRgb transparentColor = 0;
    if (mode == 1) {
        transparentColor = srcImage->pixel(0, 0);
    }

    // Apply mask-based blending
    for (qint32 y = 0; y < qMin(destHeight, srcHeight); y++) {
        for (qint32 x = 0; x < qMin(destWidth, srcWidth); x++) {
            // Get source pixel
            QRgb srcPixel = srcImage->pixel(srcLeft + x, srcTop + y);

            // Mode 1: Skip transparent color
            if (mode == 1 && srcPixel == transparentColor) {
                continue;
            }

            // Get mask pixel (use red channel as mask value)
            QRgb maskPixel = maskImage->pixel(x, y);
            int maskValue = qRed(maskPixel); // 0-255

            // Apply opacity to mask
            if (opacity < 1.0) {
                maskValue = qRound(maskValue * opacity);
            }

            // Get destination pixel
            QRgb destPixel = image->pixel(left + x, top + y);

            // Blend: (src * mask + dest * (255-mask)) / 255
            int r = (qRed(srcPixel) * maskValue + qRed(destPixel) * (255 - maskValue)) / 255;
            int g = (qGreen(srcPixel) * maskValue + qGreen(destPixel) * (255 - maskValue)) / 255;
            int b = (qBlue(srcPixel) * maskValue + qBlue(destPixel) * (255 - maskValue)) / 255;

            // Set blended pixel
            image->setPixel(left + x, top + y, qRgb(r, g, b));
        }
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief TransformImage - Apply affine transformation to image
 *
 * Image Operations
 * Applies affine transformations (rotation, scaling, skewing) to an image
 * using a 2x2 transformation matrix plus translation offset.
 *
 * Transformation formula:
 *   x' = x * Mxx + y * Mxy + Left
 *   y' = x * Myx + y * Myy + Top
 *
 * For rotation by angle θ:
 *   Mxx = cos(θ), Mxy = sin(θ), Myx = -sin(θ), Myy = cos(θ)
 *
 * @param imageId Image identifier
 * @param left X translation offset
 * @param top Y translation offset
 * @param mode Draw mode: 1=opaque, 3=transparent (color key at 0,0)
 * @param mxx Transformation matrix element [0][0]
 * @param mxy Transformation matrix element [1][0]
 * @param myx Transformation matrix element [0][1]
 * @param myy Transformation matrix element [1][1]
 * @return Error code (eOK on success)
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowTransformImage()
 */
qint32 MiniWindow::TransformImage(const QString& imageId, float left, float top, qint16 mode,
                                  float mxx, float mxy, float myx, float myy)
{
    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Validate mode
    if (mode != 1 && mode != 3) {
        return eBadParameter;
    }

    // Find the source image
    auto it = images.find(imageId);
    if (it == images.end() || !it->second) {
        return eImageNotInstalled;
    }
    QImage* srcImage = it->second.get();

    // Set up the affine transformation matrix
    // QTransform uses: m11, m12, m13, m21, m22, m23, m31, m32, m33
    // For 2D affine: [m11 m21 m31]   [Mxx Mxy dx]
    //                [m12 m22 m32] = [Myx Myy dy]
    //                [m13 m23 m33]   [0   0   1 ]
    QTransform transform(mxx, myx, mxy, myy, left, top);

    QPainter painter(image.get());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // Smooth transformation
    painter.setTransform(transform);

    // Mode 3: Transparent (color key at pixel 0,0)
    if (mode == 3 && srcImage->width() > 0 && srcImage->height() > 0) {
        // Get transparent color (top-left pixel)
        QRgb transparentColor = srcImage->pixel(0, 0);

        // Create a copy with transparency
        QImage tempImage = srcImage->copy();
        tempImage = tempImage.convertToFormat(QImage::Format_ARGB32);

        // Make pixels matching transparent color fully transparent
        for (int y = 0; y < tempImage.height(); y++) {
            for (int x = 0; x < tempImage.width(); x++) {
                QRgb pixel = tempImage.pixel(x, y);
                if (pixel == transparentColor) {
                    tempImage.setPixel(x, y, qRgba(0, 0, 0, 0)); // Fully transparent
                }
            }
        }

        // Draw the transparent image with transformation
        painter.drawImage(0, 0, tempImage);
    } else {
        // Mode 1: Opaque - draw directly
        painter.drawImage(0, 0, *srcImage);
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief Filter - Apply pixel filter to rectangular region
 *
 * Image Operations
 * Applies various pixel-based filters to a region of the miniwindow.
 * Supports 27 different filter operations including blur, sharpen, brightness,
 * contrast, gamma, grayscale, noise, and color channel adjustments.
 *
 * @param left Left coordinate
 * @param top Top coordinate
 * @param right Right coordinate (0 = window width)
 * @param bottom Bottom coordinate (0 = window height)
 * @param operation Filter type (1-27)
 * @param options Filter-specific parameter (intensity, threshold, etc.)
 * @return Error code (eOK on success)
 *
 * Filter operations:
 *  1: Noise, 2: MonoNoise, 3: Blur, 4: Sharpen, 5: EdgeDetect, 6: Emboss
 *  7: Brightness, 8: Contrast, 9: Gamma
 * 10-12: Color Brightness/Contrast/Gamma (Red)
 * 13-15: Color Brightness/Contrast/Gamma (Green)
 * 16-18: Color Brightness/Contrast/Gamma (Blue)
 * 19: Grayscale (linear), 20: Grayscale (perceptual)
 * 21: Brightness (multiply), 22-24: Color Brightness multiply (RGB)
 * 25: Lesser blur, 26: Minor blur, 27: Average
 *
 * Reference: miniwindow.cpp CMiniWindow::WindowFilter()
 */
qint32 MiniWindow::Filter(qint32 left, qint32 top, qint32 right, qint32 bottom, qint16 operation,
                          double options)
{
    // Check if window has an image
    if (!image) {
        return eBadParameter;
    }

    // Handle right=0 or bottom=0 (use window dimensions)
    if (right <= 0)
        right = width + right;
    if (bottom <= 0)
        bottom = height + bottom;

    // Constrain to window bounds
    left = qMax(0, qMin(left, width));
    top = qMax(0, qMin(top, height));
    right = qMax(0, qMin(right, width));
    bottom = qMax(0, qMin(bottom, height));

    qint32 w = right - left;
    qint32 h = bottom - top;

    if (w <= 0 || h <= 0) {
        return eBadParameter;
    }

    // Apply filter based on operation
    switch (operation) {
        case 1: { // Noise - add random noise (from original line 2849)
            // Original: c += (128 - genrand() * 256) * threshold where threshold = Options / 100.0
            double threshold = options / 100.0;
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    // genrand() returns 0.0 to 1.0, so (128 - genrand() * 256) gives range -128 to
                    // +128
                    int noise =
                        (int)((128.0 - QRandomGenerator::global()->generateDouble() * 256.0) *
                              threshold);
                    int r = qBound(0, qRed(pixel) + noise, 255);
                    int g = qBound(0, qGreen(pixel) + noise, 255);
                    int b = qBound(0, qBlue(pixel) + noise, 255);
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;
        }

        case 2: { // MonoNoise - monochrome noise (from original line 2865)
            // Original: applies same random noise to all 3 RGB channels
            double threshold = options / 100.0;
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int noise =
                        (int)((128.0 - QRandomGenerator::global()->generateDouble() * 256.0) *
                              threshold);
                    int r = qBound(0, qRed(pixel) + noise, 255);
                    int g = qBound(0, qGreen(pixel) + noise, 255);
                    int b = qBound(0, qBlue(pixel) + noise, 255);
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;
        }

        case 3:    // Blur (5x5 kernel)
        case 25:   // Lesser blur (3x3 kernel)
        case 26: { // Minor blur (3x3 kernel, lighter)
            int kernelSize = (operation == 3) ? 5 : 3;
            QImage temp = image->copy(left, top, w, h);

            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    int r = 0, g = 0, b = 0, count = 0;
                    int halfKernel = kernelSize / 2;

                    // Average surrounding pixels
                    for (int ky = -halfKernel; ky <= halfKernel; ky++) {
                        for (int kx = -halfKernel; kx <= halfKernel; kx++) {
                            int px = x + kx;
                            int py = y + ky;
                            if (px >= left && px < right && py >= top && py < bottom) {
                                QRgb pixel = temp.pixel(px - left, py - top);
                                r += qRed(pixel);
                                g += qGreen(pixel);
                                b += qBlue(pixel);
                                count++;
                            }
                        }
                    }
                    if (count > 0) {
                        image->setPixel(x, y, qRgb(r / count, g / count, b / count));
                    }
                }
            }
            break;
        }

        case 4: { // Sharpen
            QImage temp = image->copy(left, top, w, h);
            // Sharpen kernel: [-1, -1, 7, -1, -1] with divisor 3 (from original line 3224)
            double kernel[5] = {-1, -1, 7, -1, -1};
            double divisor = 3.0;

            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    double r = 0, g = 0, b = 0;

                    // Apply 1D kernel horizontally and vertically
                    for (int i = -2; i <= 2; i++) {
                        int px = qBound(left, x + i, right - 1);
                        int py = qBound(top, y + i, bottom - 1);
                        QRgb hPixel = temp.pixel(px - left, py - top);
                        QRgb vPixel = temp.pixel(x - left, py - top);

                        r += (qRed(hPixel) + qRed(vPixel)) * kernel[i + 2];
                        g += (qGreen(hPixel) + qGreen(vPixel)) * kernel[i + 2];
                        b += (qBlue(hPixel) + qBlue(vPixel)) * kernel[i + 2];
                    }

                    image->setPixel(x, y,
                                    qRgb(qBound(0, (int)(r / divisor), 255),
                                         qBound(0, (int)(g / divisor), 255),
                                         qBound(0, (int)(b / divisor), 255)));
                }
            }
            break;
        }

        case 5: { // Edge detect (from original line 3228)
            QImage temp = image->copy(left, top, w, h);
            // Edge detect kernel: [0, 2.5, -6, 2.5, 0] with divisor 1
            double kernel[5] = {0, 2.5, -6, 2.5, 0};
            double divisor = 1.0;

            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    double r = 0, g = 0, b = 0;

                    // Apply 1D kernel horizontally and vertically
                    for (int i = -2; i <= 2; i++) {
                        int px = qBound(left, x + i, right - 1);
                        int py = qBound(top, y + i, bottom - 1);
                        QRgb hPixel = temp.pixel(px - left, py - top);
                        QRgb vPixel = temp.pixel(x - left, py - top);

                        r += (qRed(hPixel) + qRed(vPixel)) * kernel[i + 2];
                        g += (qGreen(hPixel) + qGreen(vPixel)) * kernel[i + 2];
                        b += (qBlue(hPixel) + qBlue(vPixel)) * kernel[i + 2];
                    }

                    image->setPixel(x, y,
                                    qRgb(qBound(0, (int)(r / divisor), 255),
                                         qBound(0, (int)(g / divisor), 255),
                                         qBound(0, (int)(b / divisor), 255)));
                }
            }
            break;
        }

        case 6: { // Emboss (from original line 3234)
            QImage temp = image->copy(left, top, w, h);
            // Emboss kernel: [1, 2, 1, -1, -2] with divisor 1
            double kernel[5] = {1, 2, 1, -1, -2};
            double divisor = 1.0;

            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    double r = 0, g = 0, b = 0;

                    // Apply 1D kernel horizontally and vertically
                    for (int i = -2; i <= 2; i++) {
                        int px = qBound(left, x + i, right - 1);
                        int py = qBound(top, y + i, bottom - 1);
                        QRgb hPixel = temp.pixel(px - left, py - top);
                        QRgb vPixel = temp.pixel(x - left, py - top);

                        r += (qRed(hPixel) + qRed(vPixel)) * kernel[i + 2];
                        g += (qGreen(hPixel) + qGreen(vPixel)) * kernel[i + 2];
                        b += (qBlue(hPixel) + qBlue(vPixel)) * kernel[i + 2];
                    }

                    image->setPixel(x, y,
                                    qRgb(qBound(0, (int)(r / divisor), 255),
                                         qBound(0, (int)(g / divisor), 255),
                                         qBound(0, (int)(b / divisor), 255)));
                }
            }
            break;
        }

        case 7:  // Brightness (additive)
        case 10: // Red brightness
        case 13: // Green brightness
        case 16: // Blue brightness
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);

                    if (operation == 7) { // All channels
                        r = qBound(0, r + (int)options, 255);
                        g = qBound(0, g + (int)options, 255);
                        b = qBound(0, b + (int)options, 255);
                    } else if (operation == 10) { // Red only
                        r = qBound(0, r + (int)options, 255);
                    } else if (operation == 13) { // Green only
                        g = qBound(0, g + (int)options, 255);
                    } else if (operation == 16) { // Blue only
                        b = qBound(0, b + (int)options, 255);
                    }
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;

        case 8:  // Contrast (from original line 2923)
        case 11: // Red contrast (from original line 3016)
        case 14: // Green contrast
        case 17: // Blue contrast
            // Original: c = (i - 128) * Options + 128
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);

                    if (operation == 8 || operation == 11) {
                        r = qBound(0, (int)((r - 128) * options + 128), 255);
                    }
                    if (operation == 8 || operation == 14) {
                        g = qBound(0, (int)((g - 128) * options + 128), 255);
                    }
                    if (operation == 8 || operation == 17) {
                        b = qBound(0, (int)((b - 128) * options + 128), 255);
                    }
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;

        case 9:  // Gamma (from original line 2947)
        case 12: // Red gamma (from original line 3046)
        case 15: // Green gamma
        case 18: // Blue gamma
            // Original: c = pow(c / 255.0, Options) * 255
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);

                    double gamma = qMax(0.0, options); // clamp to 0 if negative
                    if (operation == 9 || operation == 12) {
                        r = qBound(0, (int)(255.0 * qPow(r / 255.0, gamma)), 255);
                    }
                    if (operation == 9 || operation == 15) {
                        g = qBound(0, (int)(255.0 * qPow(g / 255.0, gamma)), 255);
                    }
                    if (operation == 9 || operation == 18) {
                        b = qBound(0, (int)(255.0 * qPow(b / 255.0, gamma)), 255);
                    }
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;

        case 19: // Grayscale (linear average)
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int gray = (qRed(pixel) + qGreen(pixel) + qBlue(pixel)) / 3;
                    image->setPixel(x, y, qRgb(gray, gray, gray));
                }
            }
            break;

        case 20: // Grayscale (perceptual: from original line 3082)
            // Original uses: blue * 0.11 + green * 0.59 + red * 0.30
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int gray =
                        (int)(qBlue(pixel) * 0.11 + qGreen(pixel) * 0.59 + qRed(pixel) * 0.30);
                    image->setPixel(x, y, qRgb(gray, gray, gray));
                }
            }
            break;

        case 21: // Brightness (multiply)
        case 22: // Red brightness multiply
        case 23: // Green brightness multiply
        case 24: // Blue brightness multiply
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);

                    double factor = options;
                    if (operation == 21 || operation == 22) {
                        r = qBound(0, (int)(r * factor), 255);
                    }
                    if (operation == 21 || operation == 23) {
                        g = qBound(0, (int)(g * factor), 255);
                    }
                    if (operation == 21 || operation == 24) {
                        b = qBound(0, (int)(b * factor), 255);
                    }
                    image->setPixel(x, y, qRgb(r, g, b));
                }
            }
            break;

        case 27: { // Average
            int r = 0, g = 0, b = 0, count = 0;
            for (qint32 y = top; y < bottom; y++) {
                for (qint32 x = left; x < right; x++) {
                    QRgb pixel = image->pixel(x, y);
                    r += qRed(pixel);
                    g += qGreen(pixel);
                    b += qBlue(pixel);
                    count++;
                }
            }
            if (count > 0) {
                QRgb avg = qRgb(r / count, g / count, b / count);
                for (qint32 y = top; y < bottom; y++) {
                    for (qint32 x = left; x < right; x++) {
                        image->setPixel(x, y, avg);
                    }
                }
            }
            break;
        }

        default:
            return eUnknownOption;
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}
