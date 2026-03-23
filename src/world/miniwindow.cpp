#include "miniwindow.h"
#include "../utils/font_utils.h"
#include "../world/world_document.h"
#include "blend_modes.h"
#include "color_utils.h"
#include <QColor>
#include <QDebug>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QTransform>

#include "../utils/error_codes.h"

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

    // No change? Early return (original: miniwindow.cpp:4125-4126)
    if (newWidth == width && newHeight == height)
        return eOK;

    // Save old image for content preservation (original: miniwindow.cpp:4140-4157)
    auto oldImage = std::move(image);
    qint32 oldWidth = width;
    qint32 oldHeight = height;

    // Update dimensions
    width = newWidth;
    height = newHeight;
    backgroundColor = bgColor;

    // Create new image
    image = std::make_unique<QImage>(width, height, QImage::Format_ARGB32);

    // Fill with background color
    image->fill(bgrToColor(bgColor));

    // Copy old content back (original: miniwindow.cpp:4157 BitBlt)
    if (oldImage) {
        QPainter painter(image.get());
        painter.drawImage(0, 0, *oldImage, 0, 0, oldWidth, oldHeight);
    }

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

        case 5: { // DrawEdge (original: miniwindow.cpp:301-321)
            // penColor = nEdge (Win32 EDGE_* constant): must be 5, 6, 9, or 10
            // brushColor = ugrfFlags (Win32 BF_* flags): lower byte <= 0x1F
            constexpr int EDGE_RAISED = 5;  // BDR_RAISEDOUTER(1) | BDR_RAISEDINNER(4)
            constexpr int EDGE_ETCHED = 6;  // BDR_SUNKENOUTER(2) | BDR_RAISEDINNER(4)
            constexpr int EDGE_BUMP = 9;    // BDR_RAISEDOUTER(1) | BDR_SUNKENINNER(8)
            constexpr int EDGE_SUNKEN = 10; // BDR_SUNKENOUTER(2) | BDR_SUNKENINNER(8)

            if (penColor != EDGE_RAISED && penColor != EDGE_ETCHED && penColor != EDGE_BUMP &&
                penColor != EDGE_SUNKEN) {
                return eBadParameter;
            }
            if ((brushColor & 0xFF) > 0x1F) {
                return eBadParameter;
            }

            // BF_* border flags
            constexpr int BF_LEFT = 0x0001;
            constexpr int BF_TOP = 0x0002;
            constexpr int BF_RIGHT = 0x0004;
            constexpr int BF_BOTTOM = 0x0008;
            constexpr int BF_MIDDLE = 0x0800;
            int flags = static_cast<int>(brushColor);

            // Decompose nEdge into outer/inner edge type
            // BDR_RAISEDOUTER=1, BDR_SUNKENOUTER=2, BDR_RAISEDINNER=4, BDR_SUNKENINNER=8
            bool outerRaised = (penColor & 0x01) != 0;
            bool innerRaised = (penColor & 0x04) != 0;

            // Windows 3D colors
            QColor highlight(255, 255, 255);
            QColor lightShadow(192, 192, 192);
            QColor darkShadow(64, 64, 64);
            QColor shadow(128, 128, 128);

            // Determine outer edge colors (top-left / bottom-right)
            QColor outerTL = outerRaised ? highlight : darkShadow;
            QColor outerBR = outerRaised ? darkShadow : highlight;
            // Determine inner edge colors
            QColor innerTL = innerRaised ? lightShadow : shadow;
            QColor innerBR = innerRaised ? shadow : lightShadow;

            // Fill middle if BF_MIDDLE set
            if (flags & BF_MIDDLE) {
                painter.setPen(Qt::NoPen);
                painter.fillRect(rect, lightShadow);
            }

            // Draw outer edge (respecting BF_* side flags)
            painter.setPen(outerTL);
            if (flags & BF_TOP)
                painter.drawLine(rect.left(), rect.top(), rect.right(), rect.top());
            if (flags & BF_LEFT)
                painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
            painter.setPen(outerBR);
            if (flags & BF_BOTTOM)
                painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
            if (flags & BF_RIGHT)
                painter.drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());

            // Draw inner edge (1 pixel inset)
            QRect inner = rect.adjusted(1, 1, -1, -1);
            painter.setPen(innerTL);
            if (flags & BF_TOP)
                painter.drawLine(inner.left(), inner.top(), inner.right(), inner.top());
            if (flags & BF_LEFT)
                painter.drawLine(inner.left(), inner.top(), inner.left(), inner.bottom());
            painter.setPen(innerBR);
            if (flags & BF_BOTTOM)
                painter.drawLine(inner.left(), inner.bottom(), inner.right(), inner.bottom());
            if (flags & BF_RIGHT)
                painter.drawLine(inner.right(), inner.top(), inner.right(), inner.bottom());
            break;
        }

        case 4: { // Draw3dRect (original: miniwindow.cpp:295-299)
            // penColor = highlight (top/left), brushColor = shadow (bottom/right)
            QColor hilite = bgrToColor(penColor);
            QColor shad = bgrToColor(brushColor);
            painter.setPen(hilite);
            painter.drawLine(left, top, fixedRight - 1, top);
            painter.drawLine(left, top, left, fixedBottom - 1);
            painter.setPen(shad);
            painter.drawLine(left, fixedBottom - 1, fixedRight - 1, fixedBottom - 1);
            painter.drawLine(fixedRight - 1, top, fixedRight - 1, fixedBottom - 1);
            break;
        }

        case 6:   // Flood fill border (original: miniwindow.cpp:323-331)
        case 7: { // Flood fill surface (original: miniwindow.cpp:334-342)
            // penColor = border/surface color, brushColor = fill color
            painter.end(); // Release painter to access pixels directly

            QRgb fillRgb = bgrToQRgb(brushColor);

            if (left < 0 || left >= image->width() || top < 0 || top >= image->height()) {
                dirty = true;
                emit needsRedraw();
                return eOK;
            }

            QRgb seedRgb = image->pixel(left, top);

            // Don't fill if seed is already fill color, or seed is border (action 6)
            if (seedRgb == fillRgb)
                break;
            if (action == 6) {
                QRgb borderRgb = bgrToQRgb(penColor);
                if (seedRgb == borderRgb)
                    break;
            }

            // Determine target: action 7 fills matching seedRgb, action 6 fills until border
            QRgb borderRgb = bgrToQRgb(penColor);
            QRgb targetRgb = (action == 7) ? seedRgb : 0; // action 6 doesn't use target

            // Scanline flood fill
            std::vector<QPoint> stk;
            stk.push_back(QPoint(left, top));

            while (!stk.empty()) {
                QPoint pt = stk.back();
                stk.pop_back();
                int px = pt.x(), py = pt.y();
                if (px < 0 || px >= image->width() || py < 0 || py >= image->height())
                    continue;
                QRgb cur = image->pixel(px, py);
                if (cur == fillRgb)
                    continue;
                if (action == 7 && cur != targetRgb)
                    continue;
                if (action == 6 && cur == borderRgb)
                    continue;
                image->setPixel(px, py, fillRgb);
                stk.push_back(QPoint(px + 1, py));
                stk.push_back(QPoint(px - 1, py));
                stk.push_back(QPoint(px, py + 1));
                stk.push_back(QPoint(px, py - 1));
            }

            dirty = true;
            emit needsRedraw();
            return eOK;
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
            case 4: // Chord (point-based: extra1,2=start; extra3,4=end)
            case 5: // Pie
            {
                qreal pcx = rect.center().x();
                qreal pcy = rect.center().y();
                qreal psa = qAtan2(-(extra2 - pcy), extra1 - pcx) * 180.0 / M_PI;
                qreal pea = qAtan2(-(extra4 - pcy), extra3 - pcx) * 180.0 / M_PI;
                qreal pspan = pea - psa;
                if (pspan <= 0)
                    pspan += 360.0;
                if (action == 4)
                    painter.drawChord(rect, qRound(psa * 16), qRound(pspan * 16));
                else
                    painter.drawPie(rect, qRound(psa * 16), qRound(pspan * 16));
                break;
            }
                // Note: action 6 (arc) is NOT valid for CircleOp in original — only 1-5
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

        case 4: // Chord — extra1,extra2=start point; extra3,extra4=end point (GDI convention)
        case 5: // Pie   — same point-based parameters
        {
            // Convert GDI point-based coords to Qt angle-based.
            // GDI Chord/Pie draw counter-clockwise from startPoint to endPoint.
            // (original: miniwindow.cpp:592-608)
            qreal cx = rect.center().x();
            qreal cy = rect.center().y();
            qreal startAngle = qAtan2(-(extra2 - cy), extra1 - cx) * 180.0 / M_PI;
            qreal endAngle = qAtan2(-(extra4 - cy), extra3 - cx) * 180.0 / M_PI;
            qreal spanAngle = endAngle - startAngle;
            if (spanAngle <= 0)
                spanAngle += 360.0;
            int qtStart = qRound(startAngle * 16);
            int qtSpan = qRound(spanAngle * 16);
            if (action == 4)
                painter.drawChord(rect, qtStart, qtSpan);
            else
                painter.drawPie(rect, qtStart, qtSpan);
            break;
        }

        // Note: action 6 (arc) is NOT valid for CircleOp — use WindowArc instead
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
    if (mode == 3) {
        // Texture fill (original: MakeTexture in miniwindow.cpp:2555-2585)
        // Generates XOR pattern: pixel = (col ^ row) * color_component
        painter.end(); // Access image pixels directly

        int r = (color1 >> 0) & 0xFF; // BGR format: blue in low byte
        int g = (color1 >> 8) & 0xFF;
        int b = (color1 >> 16) & 0xFF;

        for (int col = left; col < fixedRight; col++) {
            for (int row = top; row < fixedBottom; row++) {
                int c = (col - left) ^ (row - top);
                int pr = (c * b) & 0xFF; // BGR→RGB: original's rval is from GetRValue
                int pg = (c * g) & 0xFF;
                int pb = (c * r) & 0xFF;
                image->setPixel(col, row, qRgb(pr, pg, pb));
            }
        }

        dirty = true;
        emit needsRedraw();
        return eOK;
    }

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
    // Delete font if name is empty and size is 0 (original: miniwindow.cpp:637-639)
    if (fontName.isEmpty() && size == 0.0) {
        fonts.remove(fontId);
        return eOK;
    }

    // Use createScaledFontF for cross-platform DPI-consistent sizing
    QFont font = createScaledFontF(fontName, size);

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

    // Return the lesser of text width and clip rect width
    // (original: miniwindow.cpp:882 — min(textsize.cx, FixRight(Right) - Left))
    qint32 textWidth = fm.horizontalAdvance(displayText);
    qint32 rectWidth = fixedRight - left;
    return qMin(textWidth, rectWidth);
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
        case 10:
            return 0; // tmDigitizedAspectX (not available in Qt)
        case 11:
            return 0; // tmDigitizedAspectY (not available in Qt)
        case 12:
            return 0; // tmFirstChar (not available in Qt)
        case 13:
            return 65535; // tmLastChar (Unicode range, approximate)
        case 14:
            return 0; // tmDefaultChar (not available in Qt)
        case 15:
            return 0; // tmBreakChar (not available in Qt)
        case 16:
            return font.italic() ? 1 : 0; // tmItalic
        case 17:
            return font.underline() ? 1 : 0; // tmUnderlined
        case 18:
            return font.strikeOut() ? 1 : 0; // tmStruckOut
        case 19:
            return 0; // tmPitchAndFamily (not available in Qt)
        case 20:
            return 0; // tmCharSet (not available in Qt, DEFAULT_CHARSET = 1)
        case 21:
            return font.family(); // Font face name (string)
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
        return eFileNotFound;
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
        return eImageNotInstalled;
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

    // Draw modes (original miniwindow.cpp:1403-1448):
    //   1 = BitBlt SRCCOPY (straight copy, no stretch — uses source dimensions)
    //   2 = StretchBlt (stretch source to dest rect)
    //   3 = Transparency (top-left pixel is color key)
    switch (mode) {
        case 1: { // Straight copy — use source dimensions, not dest rect
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            QRect copyDest(left, top, srcRect.width(), srcRect.height());
            painter.drawImage(copyDest, *img, srcRect);
            break;
        }
        case 2: // Stretch — scale source to dest rect
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.drawImage(destRect, *img, srcRect);
            break;
        case 3: { // Transparency — top-left pixel (0,0) of source is the transparent color
            QImage copy = img->copy(srcRect);
            QRgb transparentColor = img->pixel(0, 0); // Always from full image (0,0)
            QImage mask = copy.createMaskFromColor(transparentColor, Qt::MaskOutColor);
            copy.setAlphaChannel(mask);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            QRect copyDest(left, top, srcRect.width(), srcRect.height());
            painter.drawImage(copyDest, copy);
            break;
        }
        default:
            return eBadParameter;
    }

    dirty = true;
    emit needsRedraw();
    return eOK;
}

/**
 * @brief BlendImage - Draw image with opacity and one of 65 blend modes
 *
 * Image Operations
 * Ported from MUSHclient CMiniWindow::BlendImage.
 * Modes 1-35:  per-channel mathematical blend functions (see blend_modes.h)
 * Modes 36-59: channel-routing / channel-selection operations
 * Modes 60-64: HSL (hue/saturation/luminance) component swaps
 *
 * @param imageId  Image identifier (blend/upper layer)
 * @param left     Destination left
 * @param top      Destination top
 * @param right    Destination right  (<=0 → width+right)
 * @param bottom   Destination bottom (<=0 → height+bottom)
 * @param mode     Blend mode 1–64
 * @param opacity  Opacity [0.0, 1.0]
 * @param srcLeft  Source image left   (all zero → use full image)
 * @param srcTop   Source image top
 * @param srcRight Source image right  (<=0 → srcImg.width()+srcRight)
 * @param srcBottom Source image bottom (<=0 → srcImg.height()+srcBottom)
 * @return eOK on success, error code on failure
 */
qint32 MiniWindow::BlendImage(const QString& imageId, qint32 left, qint32 top, qint32 right,
                              qint32 bottom, qint16 mode, double opacity, qint32 srcLeft,
                              qint32 srcTop, qint32 srcRight, qint32 srcBottom)
{
    if (!image)
        return eBadParameter;

    if (mode < 1 || mode > 64)
        return eUnknownOption;

    auto it = images.find(imageId);
    if (it == images.end() || !it->second)
        return eImageNotInstalled;

    // Clamp opacity
    if (opacity < 0.0)
        opacity = 0.0;
    if (opacity > 1.0)
        opacity = 1.0;

    // ── Resolve destination rect ──────────────────────────────────────────
    qint32 destLeft = left;
    qint32 destTop = top;
    qint32 destRight = (right <= 0) ? width + right : right;
    qint32 destBottom = (bottom <= 0) ? height + bottom : bottom;

    // Clamp to miniwindow bounds
    destLeft = std::max(destLeft, 0);
    destTop = std::max(destTop, 0);
    destRight = std::min(destRight, width);
    destBottom = std::min(destBottom, height);

    // ── Resolve source rect ───────────────────────────────────────────────
    // Work on an ARGB32 copy of the source so we can use QRgb scanlines.
    QImage srcImg = it->second->convertToFormat(QImage::Format_ARGB32);

    // Clamp src bounds to actual image dimensions first
    if (srcLeft < 0)
        srcLeft = 0;
    if (srcTop < 0)
        srcTop = 0;
    if (srcRight > srcImg.width())
        srcRight = srcImg.width();
    if (srcBottom > srcImg.height())
        srcBottom = srcImg.height();

    // Apply FixRight/FixBottom: <=0 means offset from the right/bottom edge
    if (srcRight <= 0)
        srcRight = srcImg.width() + srcRight;
    if (srcBottom <= 0)
        srcBottom = srcImg.height() + srcBottom;

    // If all four source args were zero, use the full source image
    if (srcLeft == 0 && srcTop == 0 && srcRight == 0 && srcBottom == 0) {
        srcRight = srcImg.width();
        srcBottom = srcImg.height();
    }

    // Effective pixel region is the minimum of dest and src sizes
    qint32 blendW = std::min(destRight - destLeft, srcRight - srcLeft);
    qint32 blendH = std::min(destBottom - destTop, srcBottom - srcTop);

    if (blendW <= 0 || blendH <= 0)
        return eOK;

    // Ensure destination is ARGB32 for direct pixel access
    if (image->format() != QImage::Format_ARGB32 &&
        image->format() != QImage::Format_ARGB32_Premultiplied) {
        *image = image->convertToFormat(QImage::Format_ARGB32);
    }

    // ── Precompute cosine table (mode 3 / Interpolate) ────────────────────
    static uint8_t cos_table[256];
    static bool cos_table_ready = false;
    if (!cos_table_ready) {
        constexpr double pi_div255 = 3.1415926535898 / 255.0;
        for (int i = 0; i < 256; ++i) {
            double a = 64.0 - std::cos(static_cast<double>(i) * pi_div255) * 64.0;
            cos_table[i] = static_cast<uint8_t>(a + 0.5);
        }
        cos_table_ready = true;
    }

    // ── Per-pixel blend ───────────────────────────────────────────────────
    for (int row = 0; row < blendH; ++row) {
        const int srcY = srcTop + row;
        const int destY = destTop + row;

        const auto* srcLine = reinterpret_cast<const QRgb*>(srcImg.constScanLine(srcY));
        auto* dstLine = reinterpret_cast<QRgb*>(image->scanLine(destY));

        for (int col = 0; col < blendW; ++col) {
            const int srcX = srcLeft + col;
            const int destX = destLeft + col;

            const QRgb pixA = srcLine[srcX];  // A = blend layer
            const QRgb pixB = dstLine[destX]; // B = base layer

            const auto rA = static_cast<uint8_t>(qRed(pixA));
            const auto gA = static_cast<uint8_t>(qGreen(pixA));
            const auto bA = static_cast<uint8_t>(qBlue(pixA));

            const auto rB = static_cast<uint8_t>(qRed(pixB));
            const auto gB = static_cast<uint8_t>(qGreen(pixB));
            const auto bB = static_cast<uint8_t>(qBlue(pixB));

            uint8_t rOut{}, gOut{}, bOut{};

            // Helper lambdas that apply opacity after blending
            auto ch = [&](uint8_t blended, uint8_t base) -> uint8_t {
                return (opacity >= 1.0) ? blended : Simple_Opacity(base, blended, opacity);
            };
            // Colour_Op equivalent: compute final (fR,fG,fB) then apply opacity
            auto colourOp = [&](uint8_t fR, uint8_t fG, uint8_t fB) {
                rOut = (opacity >= 1.0) ? fR : Simple_Opacity(rB, fR, opacity);
                gOut = (opacity >= 1.0) ? fG : Simple_Opacity(gB, fG, opacity);
                bOut = (opacity >= 1.0) ? fB : Simple_Opacity(bB, fB, opacity);
            };

            switch (mode) {
                // ── Modes 1–35: per-channel mathematical blends ──────────
                case 1:
                    rOut = ch(Blend_Normal(rA, rB), rB);
                    gOut = ch(Blend_Normal(gA, gB), gB);
                    bOut = ch(Blend_Normal(bA, bB), bB);
                    break;
                case 2:
                    rOut = ch(Blend_Average(rA, rB), rB);
                    gOut = ch(Blend_Average(gA, gB), gB);
                    bOut = ch(Blend_Average(bA, bB), bB);
                    break;
                case 3: // Interpolate — cosine table
                    rOut = ch(Blend_Interpolate(cos_table[rA], cos_table[rB]), rB);
                    gOut = ch(Blend_Interpolate(cos_table[gA], cos_table[gB]), gB);
                    bOut = ch(Blend_Interpolate(cos_table[bA], cos_table[bB]), bB);
                    break;
                case 4: { // Dissolve — random per-pixel selection
                    double rnd = QRandomGenerator::global()->generateDouble();
                    bool useA = (rnd < opacity);
                    rOut = useA ? rA : rB;
                    gOut = useA ? gA : gB;
                    bOut = useA ? bA : bB;
                    break;
                }
                case 5:
                    rOut = ch(Blend_Darken(rA, rB), rB);
                    gOut = ch(Blend_Darken(gA, gB), gB);
                    bOut = ch(Blend_Darken(bA, bB), bB);
                    break;
                case 6:
                    rOut = ch(Blend_Multiply(rA, rB), rB);
                    gOut = ch(Blend_Multiply(gA, gB), gB);
                    bOut = ch(Blend_Multiply(bA, bB), bB);
                    break;
                case 7:
                    rOut = ch(Blend_ColorBurn(rA, rB), rB);
                    gOut = ch(Blend_ColorBurn(gA, gB), gB);
                    bOut = ch(Blend_ColorBurn(bA, bB), bB);
                    break;
                case 8:
                    rOut = ch(Blend_LinearBurn(rA, rB), rB);
                    gOut = ch(Blend_LinearBurn(gA, gB), gB);
                    bOut = ch(Blend_LinearBurn(bA, bB), bB);
                    break;
                case 9:
                    rOut = ch(Blend_InverseColorBurn(rA, rB), rB);
                    gOut = ch(Blend_InverseColorBurn(gA, gB), gB);
                    bOut = ch(Blend_InverseColorBurn(bA, bB), bB);
                    break;
                case 10:
                    rOut = ch(Blend_Subtract(rA, rB), rB);
                    gOut = ch(Blend_Subtract(gA, gB), gB);
                    bOut = ch(Blend_Subtract(bA, bB), bB);
                    break;
                case 11:
                    rOut = ch(Blend_Lighten(rA, rB), rB);
                    gOut = ch(Blend_Lighten(gA, gB), gB);
                    bOut = ch(Blend_Lighten(bA, bB), bB);
                    break;
                case 12:
                    rOut = ch(Blend_Screen(rA, rB), rB);
                    gOut = ch(Blend_Screen(gA, gB), gB);
                    bOut = ch(Blend_Screen(bA, bB), bB);
                    break;
                case 13:
                    rOut = ch(Blend_ColorDodge(rA, rB), rB);
                    gOut = ch(Blend_ColorDodge(gA, gB), gB);
                    bOut = ch(Blend_ColorDodge(bA, bB), bB);
                    break;
                case 14:
                    rOut = ch(Blend_LinearDodge(rA, rB), rB);
                    gOut = ch(Blend_LinearDodge(gA, gB), gB);
                    bOut = ch(Blend_LinearDodge(bA, bB), bB);
                    break;
                case 15:
                    rOut = ch(Blend_InverseColorDodge(rA, rB), rB);
                    gOut = ch(Blend_InverseColorDodge(gA, gB), gB);
                    bOut = ch(Blend_InverseColorDodge(bA, bB), bB);
                    break;
                case 16:
                    rOut = ch(Blend_Add(rA, rB), rB);
                    gOut = ch(Blend_Add(gA, gB), gB);
                    bOut = ch(Blend_Add(bA, bB), bB);
                    break;
                case 17:
                    rOut = ch(Blend_Overlay(rA, rB), rB);
                    gOut = ch(Blend_Overlay(gA, gB), gB);
                    bOut = ch(Blend_Overlay(bA, bB), bB);
                    break;
                case 18:
                    rOut = ch(Blend_SoftLight(rA, rB), rB);
                    gOut = ch(Blend_SoftLight(gA, gB), gB);
                    bOut = ch(Blend_SoftLight(bA, bB), bB);
                    break;
                case 19:
                    rOut = ch(Blend_HardLight(rA, rB), rB);
                    gOut = ch(Blend_HardLight(gA, gB), gB);
                    bOut = ch(Blend_HardLight(bA, bB), bB);
                    break;
                case 20:
                    rOut = ch(Blend_VividLight(rA, rB), rB);
                    gOut = ch(Blend_VividLight(gA, gB), gB);
                    bOut = ch(Blend_VividLight(bA, bB), bB);
                    break;
                case 21:
                    rOut = ch(Blend_LinearLight(rA, rB), rB);
                    gOut = ch(Blend_LinearLight(gA, gB), gB);
                    bOut = ch(Blend_LinearLight(bA, bB), bB);
                    break;
                case 22:
                    rOut = ch(Blend_PinLight(rA, rB), rB);
                    gOut = ch(Blend_PinLight(gA, gB), gB);
                    bOut = ch(Blend_PinLight(bA, bB), bB);
                    break;
                case 23:
                    rOut = ch(Blend_HardMix(rA, rB), rB);
                    gOut = ch(Blend_HardMix(gA, gB), gB);
                    bOut = ch(Blend_HardMix(bA, bB), bB);
                    break;
                case 24:
                    rOut = ch(Blend_Difference(rA, rB), rB);
                    gOut = ch(Blend_Difference(gA, gB), gB);
                    bOut = ch(Blend_Difference(bA, bB), bB);
                    break;
                case 25:
                    rOut = ch(Blend_Exclusion(rA, rB), rB);
                    gOut = ch(Blend_Exclusion(gA, gB), gB);
                    bOut = ch(Blend_Exclusion(bA, bB), bB);
                    break;
                case 26:
                    rOut = ch(Blend_Reflect(rA, rB), rB);
                    gOut = ch(Blend_Reflect(gA, gB), gB);
                    bOut = ch(Blend_Reflect(bA, bB), bB);
                    break;
                case 27:
                    rOut = ch(Blend_Glow(rA, rB), rB);
                    gOut = ch(Blend_Glow(gA, gB), gB);
                    bOut = ch(Blend_Glow(bA, bB), bB);
                    break;
                case 28:
                    rOut = ch(Blend_Freeze(rA, rB), rB);
                    gOut = ch(Blend_Freeze(gA, gB), gB);
                    bOut = ch(Blend_Freeze(bA, bB), bB);
                    break;
                case 29:
                    rOut = ch(Blend_Heat(rA, rB), rB);
                    gOut = ch(Blend_Heat(gA, gB), gB);
                    bOut = ch(Blend_Heat(bA, bB), bB);
                    break;
                case 30:
                    rOut = ch(Blend_Negation(rA, rB), rB);
                    gOut = ch(Blend_Negation(gA, gB), gB);
                    bOut = ch(Blend_Negation(bA, bB), bB);
                    break;
                case 31:
                    rOut = ch(Blend_Phoenix(rA, rB), rB);
                    gOut = ch(Blend_Phoenix(gA, gB), gB);
                    bOut = ch(Blend_Phoenix(bA, bB), bB);
                    break;
                case 32:
                    rOut = ch(Blend_Stamp(rA, rB), rB);
                    gOut = ch(Blend_Stamp(gA, gB), gB);
                    bOut = ch(Blend_Stamp(bA, bB), bB);
                    break;
                case 33:
                    rOut = ch(Blend_Xor(rA, rB), rB);
                    gOut = ch(Blend_Xor(gA, gB), gB);
                    bOut = ch(Blend_Xor(bA, bB), bB);
                    break;
                case 34:
                    rOut = ch(Blend_And(rA, rB), rB);
                    gOut = ch(Blend_And(gA, gB), gB);
                    bOut = ch(Blend_And(bA, bB), bB);
                    break;
                case 35:
                    rOut = ch(Blend_Or(rA, rB), rB);
                    gOut = ch(Blend_Or(gA, gB), gB);
                    bOut = ch(Blend_Or(bA, bB), bB);
                    break;

                // ── Modes 36–59: channel-routing operations ──────────────
                // Take one channel from A (blend), retain others from B (base)
                case 36:
                    colourOp(rA, gB, bB);
                    break; // red from A
                case 37:
                    colourOp(rB, gA, bB);
                    break; // green from A
                case 38:
                    colourOp(rB, gB, bA);
                    break; // blue from A
                // Take two channels from A, retain one from B
                case 39:
                    colourOp(rA, gA, bB);
                    break; // yellow (R+G from A)
                case 40:
                    colourOp(rB, gA, bA);
                    break; // cyan   (G+B from A)
                case 41:
                    colourOp(rA, gB, bA);
                    break; // magenta (R+B from A)
                // Limit green
                case 42:
                    colourOp(rA, (gA > rA) ? rA : gA, bA);
                    break; // green limited by red
                case 43:
                    colourOp(rA, (gA > bA) ? bA : gA, bA);
                    break; // green limited by blue
                case 44:
                    colourOp(rA, (gA > ((rA + bA) / 2)) ? ((rA + bA) / 2) : gA, bA);
                    break;
                // Limit blue
                case 45:
                    colourOp(rA, gA, (bA > rA) ? rA : bA);
                    break; // blue limited by red
                case 46:
                    colourOp(rA, gA, (bA > gA) ? gA : bA);
                    break; // blue limited by green
                case 47:
                    colourOp(rA, gA, (bA > ((rA + gA) / 2)) ? ((rA + gA) / 2) : bA);
                    break;
                // Limit red
                case 48:
                    colourOp((rA > gA) ? gA : rA, gA, bA);
                    break; // red limited by green
                case 49:
                    colourOp((rA > bA) ? bA : rA, gA, bA);
                    break; // red limited by blue
                case 50:
                    colourOp((rA > ((gA + bA) / 2)) ? ((gA + bA) / 2) : rA, gA, bA);
                    break;
                // Single channel selection
                case 51:
                    colourOp(rA, 0, 0);
                    break; // red only   → looks red
                case 52:
                    colourOp(0, gA, 0);
                    break; // green only → looks green
                case 53:
                    colourOp(0, 0, bA);
                    break; // blue only  → looks blue
                // Discard one channel
                case 54:
                    colourOp(0, gA, bA);
                    break; // discard red   → looks cyan
                case 55:
                    colourOp(rA, 0, bA);
                    break; // discard green → looks magenta
                case 56:
                    colourOp(rA, gA, 0);
                    break; // discard blue  → looks yellow
                // One channel expanded to all
                case 57:
                    colourOp(rA, rA, rA);
                    break; // all red   → grey
                case 58:
                    colourOp(gA, gA, gA);
                    break; // all green → grey
                case 59:
                    colourOp(bA, bA, bA);
                    break; // all blue  → grey

                // ── Modes 60–64: HSL component swaps ─────────────────────
                case 60: { // Hue from A, saturation+lightness from B
                    QColor cA(rA, gA, bA);
                    QColor cB(rB, gB, bB);
                    cB.setHsl(cA.hslHue() < 0 ? 0 : cA.hslHue(), cB.hslSaturation(),
                              cB.lightness());
                    colourOp(static_cast<uint8_t>(cB.red()), static_cast<uint8_t>(cB.green()),
                             static_cast<uint8_t>(cB.blue()));
                    break;
                }
                case 61: { // Saturation from A, hue+lightness from B
                    QColor cA(rA, gA, bA);
                    QColor cB(rB, gB, bB);
                    cB.setHsl(cB.hslHue() < 0 ? 0 : cB.hslHue(), cA.hslSaturation(),
                              cB.lightness());
                    colourOp(static_cast<uint8_t>(cB.red()), static_cast<uint8_t>(cB.green()),
                             static_cast<uint8_t>(cB.blue()));
                    break;
                }
                case 62: { // Hue+Saturation from A, lightness from B
                    QColor cA(rA, gA, bA);
                    QColor cB(rB, gB, bB);
                    cB.setHsl(cA.hslHue() < 0 ? 0 : cA.hslHue(), cA.hslSaturation(),
                              cB.lightness());
                    colourOp(static_cast<uint8_t>(cB.red()), static_cast<uint8_t>(cB.green()),
                             static_cast<uint8_t>(cB.blue()));
                    break;
                }
                case 63: { // Luminance (lightness) from A, hue+saturation from B
                    QColor cA(rA, gA, bA);
                    QColor cB(rB, gB, bB);
                    cB.setHsl(cB.hslHue() < 0 ? 0 : cB.hslHue(), cB.hslSaturation(),
                              cA.lightness());
                    colourOp(static_cast<uint8_t>(cB.red()), static_cast<uint8_t>(cB.green()),
                             static_cast<uint8_t>(cB.blue()));
                    break;
                }
                case 64: { // Visualise HSL: hue→R, saturation→G, lightness→B
                    QColor cA(rA, gA, bA);
                    // Hue is in [0,360) (or -1 for achromatic); scale to [0,255]
                    int hue = cA.hslHue();
                    auto hueOut = static_cast<uint8_t>((hue < 0 ? 0 : hue) * 255 / 359);
                    auto satOut = static_cast<uint8_t>(cA.hslSaturation()); // already [0,255]
                    auto lightOut = static_cast<uint8_t>(cA.lightness());   // already [0,255]
                    colourOp(hueOut, satOut, lightOut);
                    break;
                }

                default:
                    return eUnknownOption;
            }

            dstLine[destX] = qRgb(rOut, gOut, bOut);
        }
    }

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

    // Original miniwindow.cpp:1500-1508 — Windows BITMAP struct fields
    switch (infoType) {
        case 1:
            return 0; // bmType — always 0 for standard bitmaps
        case 2:
            return img->width(); // bmWidth
        case 3:
            return img->height(); // bmHeight
        case 4:
            return img->bytesPerLine(); // bmWidthBytes
        case 5:
            return 1; // bmPlanes — modern displays use 1 plane
        case 6:
            return img->depth(); // bmBitsPixel
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

    // Validate pen style (original: miniwindow.cpp:353-398, 1909-1910)
    {
        int linePat = penStyle & 0x0F; // PS_STYLE_MASK
        if (linePat >= 1 && linePat <= 4 && penWidth > 1) {
            return ePenStyleNotValid; // dash/dot styles require width <= 1
        }
        if (linePat > 6) {
            return ePenStyleNotValid; // unknown style
        }
        int endCap = (penStyle >> 8) & 0x0F; // PS_ENDCAP_MASK
        if (endCap > 2) {
            return ePenStyleNotValid;
        }
        int join = (penStyle >> 12) & 0x0F; // PS_JOIN_MASK
        if (join > 2) {
            return ePenStyleNotValid;
        }
    }

    // Create painter
    QPainter painter(image.get());
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Setup pen using shared Windows GDI pen converter
    QPen pen = createWindowsPen(bgrToColor(penColor), penWidth, penStyle);
    painter.setPen(pen);

    // Setup brush with image pattern
    QBrush brush(*brushImg);
    brush.setStyle(Qt::TexturePattern);
    painter.setBrush(brush);

    // Handle right/bottom <= 0 as offset from window edge (same as RectOp)
    qint32 fixedRight = (right <= 0) ? width + right : right;
    qint32 fixedBottom = (bottom <= 0) ? height + bottom : bottom;

    int w = fixedRight - left;
    int h = fixedBottom - top;
    QRect rect(left, top, w > 0 ? w : 0, h > 0 ? h : 0);

    // Original action codes: 1=ellipse, 2=rectangle, 3=round rect
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
qint32 MiniWindow::LoadImageMemory(const QString& imageId, std::span<const unsigned char> data,
                                   bool hasAlpha)
{
    if (imageId.isEmpty()) {
        return eNoNameSpecified;
    }

    if (data.empty()) {
        return eBadParameter;
    }

    // Load image from memory
    auto img = std::make_unique<QImage>();
    if (!img->loadFromData(data.data(), static_cast<int>(data.size()))) {
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
