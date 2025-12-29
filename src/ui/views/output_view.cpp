#include "output_view.h"
#include "../../text/action.h"
#include "../../world/color_utils.h"
#include "../../world/miniwindow.h"
#include "../automation/plugin.h"
#include "../text/line.h"
#include "../text/style.h"
#include "../world/script_engine.h"
#include "../world/world_document.h"
#include "hotspot.h"
#include "logging.h"
#include <QApplication>
#include <QBitmap>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QUrl>
#include <QWheelEvent>
#include <algorithm>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

/**
 * OutputView Constructor
 *
 * Basic Text Display Widget
 *
 * Initializes the widget with a fixed-width font (Courier) and connects
 * to the WorldDocument's linesAdded signal for auto-update.
 */
OutputView::OutputView(WorldDocument* doc, QWidget* parent)
    : QWidget(parent), m_doc(doc), m_lineHeight(0), m_charWidth(0), m_scrollPos(0),
      m_visibleLines(0), m_selectionActive(false), m_selectionStartLine(-1),
      m_selectionStartChar(-1), m_selectionEndLine(-1), m_selectionEndChar(-1),
      m_mouseDownButton(Qt::NoButton), m_freeze(false), m_frozenLineCount(0)
{
    // Set up fixed-width font for MUD display
    // Read font from WorldDocument if available, otherwise use default
    if (m_doc && !m_doc->m_font_name.isEmpty()) {
        m_font = QFont(m_doc->m_font_name, m_doc->m_font_height);
        qCDebug(lcUI) << "OutputView: Using font from WorldDocument:" << m_doc->m_font_name
                      << m_doc->m_font_height;
    } else {
        m_font = QFont("Courier New", 10);
        qCDebug(lcUI) << "OutputView: Using default font (Courier New, 10)";
    }
    m_font.setFixedPitch(true);
    m_font.setStyleHint(QFont::TypeWriter);

    // Calculate metrics
    calculateMetrics();

    // Set widget properties
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    // Set minimum size
    setMinimumSize(400, 300);

    // Enable mouse tracking and set cursor for text selection
    setMouseTracking(true);
    setCursor(Qt::IBeamCursor);

    // Connect to document signals
    if (m_doc) {
        connect(m_doc, &WorldDocument::linesAdded, this, &OutputView::onNewLinesAdded);
        connect(m_doc, &WorldDocument::incompleteLine, this, &OutputView::onIncompleteLine);
        connect(m_doc, &WorldDocument::textRectangleConfigChanged, this,
                &OutputView::calculateMetrics);
    }

    qCDebug(lcUI) << "OutputView created - lineHeight:" << m_lineHeight
                  << "charWidth:" << m_charWidth << "visibleLines:" << m_visibleLines;
}

/**
 * OutputView Destructor
 */
OutputView::~OutputView()
{
    // Nothing to clean up (don't own m_doc)
}

/**
 * setOutputFont - Update the display font
 *
 * Called when user changes font in World Properties dialog.
 * Recalculates metrics and triggers repaint.
 */
void OutputView::setOutputFont(const QFont& font)
{
    m_font = font;
    m_font.setFixedPitch(true);
    m_font.setStyleHint(QFont::TypeWriter);

    qCDebug(lcUI) << "OutputView::setOutputFont() - Font changed to:" << m_font.family()
                  << m_font.pointSize();

    // Recalculate metrics with new font
    calculateMetrics();

    // Trigger repaint with new font
    update();
}

/**
 * calculateMetrics - Calculate line height and visible lines
 *
 * Uses QFontMetrics to measure the current font and determine how many
 * lines can fit in the widget's height (or text rectangle height if configured).
 */
void OutputView::calculateMetrics()
{
    QFontMetrics fm(m_font);
    m_lineHeight = fm.height();
    m_charWidth = fm.horizontalAdvance('M'); // Width of average character

    // Calculate how many lines fit in current height
    // Use text rectangle height if one is explicitly configured, otherwise use widget height
    if (m_lineHeight > 0) {
        // Only use text rectangle if it's explicitly configured AND doc is valid
        if (m_doc && haveTextRectangle()) {
            QRect textRect = getTextRectangle();
            m_visibleLines = textRect.height() / m_lineHeight;
            // Push computed rectangle to WorldDocument (for Lua API access)
            m_doc->m_computedTextRectangle = textRect;
        } else {
            // No text rectangle configured - use full widget height
            m_visibleLines = height() / m_lineHeight;
            // Store widget rect as computed rectangle
            if (m_doc) {
                m_doc->m_computedTextRectangle = rect();
            }
        }
    } else {
        m_visibleLines = 0;
        if (m_doc) {
            m_doc->m_computedTextRectangle = QRect();
        }
    }

    qCDebug(lcUI) << "Metrics calculated - height:" << height()
                  << "haveTextRect:" << (m_doc && haveTextRectangle())
                  << "lineHeight:" << m_lineHeight << "visibleLines:" << m_visibleLines;
}

/**
 * resizeEvent - Handle widget resize
 *
 * Recalculates metrics when widget is resized (e.g., window maximized).
 */
void OutputView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    calculateMetrics();
    update(); // Trigger repaint

    // Notify plugins of resize
    if (m_doc) {
        m_doc->SendToAllPluginCallbacks(ON_PLUGIN_WORLD_OUTPUT_RESIZED);
    }
}

/**
 * isAtBottom - Check if scrolled to bottom
 *
 * Returns true if the current scroll position is at or near the bottom
 * of the buffer. Used to determine auto-scroll behavior.
 */
bool OutputView::isAtBottom() const
{
    if (!m_doc)
        return true;

    int totalLines = m_doc->m_lineList.count();
    if (totalLines <= m_visibleLines)
        return true; // All lines visible, always "at bottom"

    // At bottom if scrollPos is within 1 line of the maximum scroll position
    int maxScroll = totalLines - m_visibleLines;
    return (m_scrollPos >= maxScroll - 1);
}

/**
 * onNewLinesAdded - Handle new lines added to document
 *
 * If currently scrolled to bottom, auto-scrolls to show new lines.
 * If scrolled up (user reading history), doesn't auto-scroll.
 * If frozen, counts new lines but doesn't auto-scroll.
 */
void OutputView::onNewLinesAdded()
{
    if (!m_doc)
        return;

    // Flash taskbar icon if enabled and window is not active
    if (m_doc->m_bFlashIcon) {
        QWidget* mainWindow = window();
        if (mainWindow && !mainWindow->isActiveWindow()) {
            QApplication::alert(mainWindow);
        }
    }

    // If frozen, count lines but don't auto-scroll
    if (m_freeze) {
        // Count the new line (we get called once per line)
        m_frozenLineCount++;
        emit freezeStateChanged(true, m_frozenLineCount);
        // Don't scroll, but still repaint
        update();
        return;
    }

    // If at bottom, stay at bottom (auto-scroll)
    if (isAtBottom()) {
        int totalLines = m_doc->m_lineList.count();
        m_scrollPos = qMax(0, totalLines - m_visibleLines);
    }

    // Trigger repaint to show new lines
    update();
}

/**
 * onIncompleteLine - Handle incomplete line (prompt) from document
 *
 * Prompts and other incomplete lines (without \n) need to be displayed
 * even though they haven't been added to the line buffer yet.
 * This just triggers a repaint so paintEvent can draw the current incomplete line.
 */
void OutputView::onIncompleteLine()
{
    // Trigger repaint to show the incomplete line
    update();
}

/**
 * wheelEvent - Handle mouse wheel scrolling
 *
 * Miniwindow Scroll Events
 * If mouse is over a miniwindow with scroll wheel handler, invoke it.
 * Otherwise, scroll the text output by 3 lines per wheel step.
 *
 * Auto-Freeze Feature: If m_bAutoFreeze is enabled and the user scrolls up,
 * automatically freeze the output to prevent auto-scrolling.
 */
void OutputView::wheelEvent(QWheelEvent* event)
{
    if (!m_doc || m_lineHeight <= 0)
        return;

    // Check if a miniwindow wants to handle the scroll wheel
    if (handleMiniWindowScrollWheel(event->position().toPoint(), event->angleDelta(),
                                    event->modifiers())) {
        event->accept();
        return; // Miniwindow handled the scroll
    }

    // No miniwindow handled it - scroll the text output
    int delta = event->angleDelta().y() / 120; // 120 units = 1 "step"

    // Auto-freeze: If scrolling up and auto-freeze is enabled, freeze output
    if (delta > 0 && m_doc->m_bAutoFreeze && !m_freeze) {
        // Scrolling up - freeze output so we don't auto-scroll
        setFrozen(true);
    }

    // Scroll by 3 lines per step
    m_scrollPos -= delta * 3;

    // Clamp to valid range
    m_scrollPos = qMax(0, m_scrollPos);

    int totalLines = m_doc->m_lineList.count();
    int maxScroll = qMax(0, totalLines - m_visibleLines);
    m_scrollPos = qMin(maxScroll, m_scrollPos);

    // Auto-unfreeze: If we've scrolled to the bottom, unfreeze
    if (m_freeze && m_scrollPos >= maxScroll) {
        setFrozen(false);
    }

    // Trigger repaint
    update();

    event->accept();
}

/**
 * ansiToRgb - Convert ANSI index or BGR value to QColor
 *
 * Color Conversion
 *
 * Handles three color modes based on COLOURTYPE bits in flags:
 * - COLOUR_ANSI: color is index 0-7 for standard, 8-255 for extended
 * - COLOUR_CUSTOM: color is index into custom palette
 * - COLOUR_RGB: color is actual BGR value (Windows COLORREF format)
 *
 * All color values use BGR format (0x00BBGGRR) for MUSHclient compatibility.
 *
 * @param color Color value (index or BGR)
 * @param flags Style flags containing COLOURTYPE bits
 * @param bold If true and ANSI mode, use bold/bright color table
 * @return QColor for rendering
 */
QColor OutputView::ansiToRgb(quint32 color, quint16 flags, bool bold) const
{
    if (!m_doc)
        return Qt::white;

    quint16 colorType = flags & COLOURTYPE;

    // COLOUR_RGB: value is BGR format (Windows COLORREF)
    if (colorType == COLOUR_RGB) {
        return bgrToQColor(color);
    }

    // COLOUR_CUSTOM: look up in custom palette (stored as BGR)
    if (colorType == COLOUR_CUSTOM) {
        int index = color & 0xFF;
        if (index < MAX_CUSTOM) {
            return bgrToQColor(m_doc->m_customtext[index]);
        }
        return Qt::white;
    }

    // COLOUR_ANSI (default): look up in color tables (stored as BGR)
    int index = color & 0xFF;
    if (index < 8) {
        // Standard ANSI color - use normal or bold color table
        QRgb bgr = bold ? m_doc->m_boldcolour[index] : m_doc->m_normalcolour[index];
        return bgrToQColor(bgr);
    } else {
        // Extended 256-color - use xterm palette (stored as BGR)
        return bgrToQColor(xterm_256_colours[index]);
    }
}

/**
 * paintEvent - Render the visible portion of the text buffer
 *
 * Main Rendering Loop
 *
 * Draws only the visible lines (based on scroll position) to avoid
 * rendering thousands of offscreen lines.
 */
void OutputView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setFont(m_font);

    if (!m_doc) {
        // No document - just show black background
        painter.fillRect(rect(), Qt::black);
        return;
    }

    // Safety check: ensure we're initialized
    if (m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::paintEvent - m_lineHeight is" << m_lineHeight
                      << ", skipping render";
        return;
    }

    // Draw background image if set (behind everything)
    if (!m_backgroundImage.isNull()) {
        drawImage(painter, m_backgroundImage, m_doc->m_iBackgroundMode);
    }

    // Text rectangle rendering
    if (haveTextRectangle()) {
        // Fill entire widget with outside fill color/pattern
        QColor outsideFillColor = QColor::fromRgba(m_doc->m_TextRectangleOutsideFillColour);

        // Handle fill style (0=none, 1=solid, 2-14=patterns)
        if (m_doc->m_TextRectangleOutsideFillStyle == 0) {
            // No fill - leave transparent
        } else if (m_doc->m_TextRectangleOutsideFillStyle == 1) {
            // Solid fill
            painter.fillRect(rect(), outsideFillColor);
        } else {
            // Pattern fill (TODO: implement brush patterns)
            painter.fillRect(rect(), outsideFillColor);
        }

        // Get normalized text rectangle
        QRect textRect = getTextRectangle(false); // Without border offset

        // Draw DRAW_UNDERNEATH miniwindows first (in margins, behind text)
        drawMiniWindows(painter, true);

        // Save painter state before clipping
        painter.save();

        // Translate coordinate system to text rectangle origin
        painter.translate(textRect.topLeft());

        // Set clip region to text rectangle for text rendering (in translated coordinates)
        QRect localTextRect(0, 0, textRect.width(), textRect.height());
        painter.setClipRect(localTextRect);

        // Fill text rectangle with black background
        painter.fillRect(localTextRect, Qt::black);

        // Draw text lines (clipped to text rectangle)
        int y = 0;
        if (!m_doc->m_lineList.isEmpty()) {
            int totalLines = m_doc->m_lineList.count();
            m_scrollPos = qBound(0, m_scrollPos, qMax(0, totalLines - 1));

            int firstLine = m_scrollPos;
            int lastLine = qMin(m_scrollPos + m_visibleLines, totalLines);

            // Give plugins a chance to update miniwindows to match text in the output window
            // Parameters: (startline 1-based, pixel offset, "")
            m_doc->SendToAllPluginCallbacks(ON_PLUGIN_DRAW_OUTPUT_WINDOW, firstLine + 1, y,
                                            QString());

            for (int i = firstLine; i < lastLine; i++) {
                if (i < 0 || i >= m_doc->m_lineList.count()) {
                    qCDebug(lcUI) << "OutputView::paintEvent - index" << i
                                  << "out of bounds, lineList size:" << m_doc->m_lineList.count();
                    break;
                }
                Line* line = m_doc->m_lineList[i];
                drawLine(painter, y, line, i);
                y += m_lineHeight;
            }
        }

        // Draw incomplete line (prompt)
        if (isAtBottom() && m_doc->m_currentLine && m_doc->m_currentLine->len() > 0) {
            drawLine(painter, y, m_doc->m_currentLine, m_doc->m_lineList.count());
        }

        // Restore painter state (remove clipping)
        painter.restore();

        // Draw border around text rectangle (if configured)
        if (m_doc->m_TextRectangleBorderWidth > 0) {
            QRect borderRect = getTextRectangle(true); // With border offset
            QColor borderColor = QColor::fromRgba(m_doc->m_TextRectangleBorderColour);

            // Make border color much lighter and more visible
            // Use a bright, saturated version instead of the potentially dark original
            borderColor = borderColor.lighter(250); // Much lighter
            if (borderColor.value() < 128) {
                // If still too dark, force it to be bright
                borderColor = QColor(180, 180, 255); // Light blue-gray
            }

            QPen borderPen(borderColor, m_doc->m_TextRectangleBorderWidth);
            painter.setPen(borderPen);
            painter.setBrush(Qt::NoBrush);

            // Qt's drawRect() draws OUTSIDE the rectangle (right edge at right()+1, bottom at
            // bottom()+1) Adjust by -1 to draw INSIDE the rectangle boundaries
            painter.drawRect(borderRect.adjusted(0, 0, -1, -1));

            // Note: Resize grip visual is provided by plugins (scrollbar with grip indicators)
            // We only provide the resize functionality via mouse event handlers
        }

        // Draw normal miniwindows last (in margins, on top of text)
        drawMiniWindows(painter, false);
    } else {
        // No text rectangle - use full window rendering
        painter.fillRect(rect(), Qt::black);

        // Draw DRAW_UNDERNEATH miniwindows first (behind text)
        drawMiniWindows(painter, true);

        int y = 0;

        // Draw complete lines from buffer (if any)
        if (!m_doc->m_lineList.isEmpty()) {
            // Calculate which lines to draw
            int totalLines = m_doc->m_lineList.count();

            // Clamp scroll position to valid range
            m_scrollPos = qBound(0, m_scrollPos, qMax(0, totalLines - 1));

            int firstLine = m_scrollPos;
            int lastLine = qMin(m_scrollPos + m_visibleLines, totalLines);

            // Give plugins a chance to update miniwindows to match text in the output window
            // Parameters: (startline 1-based, pixel offset, "")
            m_doc->SendToAllPluginCallbacks(ON_PLUGIN_DRAW_OUTPUT_WINDOW, firstLine + 1, y,
                                            QString());

            // Draw each visible line (selection handled within drawLine)
            for (int i = firstLine; i < lastLine; i++) {
                // Bounds check before accessing list
                if (i < 0 || i >= m_doc->m_lineList.count()) {
                    qCDebug(lcUI) << "OutputView::paintEvent - index" << i
                                  << "out of bounds, lineList size:" << m_doc->m_lineList.count();
                    break;
                }
                Line* line = m_doc->m_lineList[i];
                drawLine(painter, y, line, i);
                y += m_lineHeight;
            }
        }

        // Draw incomplete line (prompt) if we're at bottom and have one
        // Incomplete lines don't have newlines, so they stay in m_currentLine
        // until more data arrives or the line completes
        if (isAtBottom() && m_doc->m_currentLine && m_doc->m_currentLine->len() > 0) {
            // Draw the incomplete line at the current y position (after all complete lines)
            // Use the line count as the "line index" for this pseudo-line
            drawLine(painter, y, m_doc->m_currentLine, m_doc->m_lineList.count());
        }

        // Draw normal miniwindows last (on top of text)
        drawMiniWindows(painter, false);
    }

    // Draw foreground image if set (on top of everything)
    if (!m_foregroundImage.isNull()) {
        drawImage(painter, m_foregroundImage, m_doc->m_iForegroundMode);
    }

    // Draw freeze indicator if frozen
    if (m_freeze) {
        // Draw a subtle indicator in the top-right corner
        QString freezeText = QString("PAUSED (%1)").arg(m_frozenLineCount);
        QFont indicatorFont = m_font;
        indicatorFont.setBold(true);
        painter.setFont(indicatorFont);
        QFontMetrics fm(indicatorFont);

        int textWidth = fm.horizontalAdvance(freezeText);
        int textHeight = fm.height();
        int padding = 4;
        int margin = 8;

        QRect indicatorRect(width() - textWidth - padding * 2 - margin, margin,
                            textWidth + padding * 2, textHeight + padding * 2);

        // Semi-transparent background
        painter.fillRect(indicatorRect, QColor(255, 100, 100, 200));

        // White text
        painter.setPen(Qt::white);
        painter.drawText(indicatorRect, Qt::AlignCenter, freezeText);
    }
}

/**
 * drawLine - Render one line with all its styles
 *
 * Style-based Text Rendering
 * Selection-aware rendering
 *
 * Iterates through each style run in the line and renders the corresponding
 * text with the correct colors, font attributes, and effects.
 * Selected text is rendered with highlighted colors.
 *
 * @param painter QPainter to draw with
 * @param y Y coordinate (top of line)
 * @param line Line to draw (can be nullptr)
 * @param lineIndex Line index in buffer (for selection checking)
 */
void OutputView::drawLine(QPainter& painter, int y, Line* line, int lineIndex)
{
    if (!line || !line->text() || line->len() == 0) {
        // Empty line - just leave blank
        return;
    }

    int x = 0;       // Current X position
    int textPos = 0; // Current position in line->text()

    // Timestamp preamble rendering (version 4.62 feature from original MUSHclient)
    // Get appropriate preamble text and colors based on line type
    QString strPreamble;
    QRgb cPreambleText;
    QRgb cPreambleBack;

    if (line->flags & COMMENT) {
        strPreamble = m_doc->m_strOutputLinePreambleNotes;
        cPreambleText = m_doc->m_OutputLinePreambleNotesTextColour;
        cPreambleBack = m_doc->m_OutputLinePreambleNotesBackColour;
    } else if (line->flags & USER_INPUT) {
        strPreamble = m_doc->m_strOutputLinePreambleInput;
        cPreambleText = m_doc->m_OutputLinePreambleInputTextColour;
        cPreambleBack = m_doc->m_OutputLinePreambleInputBackColour;
    } else {
        strPreamble = m_doc->m_strOutputLinePreambleOutput;
        cPreambleText = m_doc->m_OutputLinePreambleOutputTextColour;
        cPreambleBack = m_doc->m_OutputLinePreambleOutputBackColour;
    }

    // Expand time codes if preamble is not empty
    if (!strPreamble.isEmpty()) {
        // Calculate elapsed time from world start (%e)
        double fElapsedTime = 0.0;
        if (m_doc->m_whenWorldStartedHighPrecision != 0 && line->m_lineHighPerformanceTime != 0) {
            // High precision elapsed time in seconds
            // Note: QElapsedTimer uses nanoseconds on some platforms, milliseconds on others
            // For now, use QDateTime which is sufficient for timestamp display
            fElapsedTime = m_doc->m_whenWorldStarted.msecsTo(line->m_theTime) / 1000.0;
        }
        QString strElapsedTime = QString::asprintf("%.6f", fElapsedTime);
        strPreamble.replace("%e", strElapsedTime);

        // Calculate delta time from previous line (%D)
        // TODO: Track previous line time for %D support
        strPreamble.replace("%D", "0.000000");

        // Expand remaining time codes
        strPreamble = m_doc->FormatTime(line->m_theTime, strPreamble, false);

        // Draw preamble with appropriate colors
        QFont font = m_font;
        painter.setFont(font);
        QFontMetrics fm(font);

        QColor preambleFore = bgrToQColor(cPreambleText);
        QColor preambleBack = bgrToQColor(cPreambleBack);

        int preambleWidth = fm.horizontalAdvance(strPreamble);

        // Draw background for preamble
        painter.fillRect(x, y, preambleWidth, m_lineHeight, preambleBack);

        // Draw preamble text
        painter.setPen(preambleFore);
        painter.drawText(x, y + fm.ascent(), strPreamble);

        // Advance x position
        x += preambleWidth;
    }

    // Draw each style run
    for (const auto& style : line->styleList) {
        if (!style || style->iLength == 0)
            continue; // Skip null or empty styles

        // Ensure we don't read past end of text
        if (textPos >= line->len())
            break;

        int styleLength = qMin((int)style->iLength, line->len() - textPos);
        int styleEnd = textPos + styleLength;

        // Apply font attributes once for this style
        QFont font = m_font;
        font.setBold(style->iFlags & HILITE);
        font.setUnderline(style->iFlags & UNDERLINE);
        font.setItalic(style->iFlags & BLINK); // MUSHclient uses BLINK for italic
        font.setStrikeOut(style->iFlags & STRIKEOUT);
        painter.setFont(font);
        QFontMetrics fm(font);

        // Get normal (unselected) colors from style
        QColor normalForeColor =
            ansiToRgb(style->iForeColour, style->iFlags, style->iFlags & HILITE);
        QColor normalBackColor = ansiToRgb(style->iBackColour, style->iFlags, false);

        // Handle INVERSE flag (swap foreground/background)
        if (style->iFlags & INVERSE) {
            qSwap(normalForeColor, normalBackColor);
        }

        // Convert style run bytes to QString using UTF-8
        // This properly handles multi-byte UTF-8 sequences (e.g., box-drawing characters)
        QString styleText = QString::fromUtf8(line->text() + textPos, styleLength);

        // Draw the style run character by character, switching colors for selected portions
        // We need to track byte positions for selection, which is byte-based
        int bytePos = textPos;
        for (int charIdx = 0; charIdx < styleText.length(); charIdx++) {
            // Calculate byte length of this character (1-4 bytes for UTF-8)
            // QChar uses UTF-16 internally, so we need to check for surrogate pairs
            int charByteLen;
            if (styleText[charIdx].isHighSurrogate() && charIdx + 1 < styleText.length() &&
                styleText[charIdx + 1].isLowSurrogate()) {
                // Surrogate pair (4-byte UTF-8 character like emoji)
                charByteLen = 4;
            } else {
                // Regular character - calculate UTF-8 byte length
                QChar ch = styleText[charIdx];
                if (ch.unicode() < 0x80) {
                    charByteLen = 1; // ASCII
                } else if (ch.unicode() < 0x800) {
                    charByteLen = 2; // 2-byte UTF-8
                } else {
                    charByteLen = 3; // 3-byte UTF-8 (most common for non-ASCII)
                }
            }

            // Check if ANY byte of this character is selected
            bool charIsSelected = false;
            if (hasSelection()) {
                for (int b = 0; b < charByteLen && !charIsSelected; b++) {
                    if (isCharSelected(lineIndex, bytePos + b)) {
                        charIsSelected = true;
                    }
                }
            }

            // Choose colors based on selection
            QColor foreColor, backColor;
            if (charIsSelected) {
                foreColor = palette().color(QPalette::HighlightedText);
                backColor = palette().color(QPalette::Highlight);
            } else {
                foreColor = normalForeColor;
                backColor = normalBackColor;
            }

            // Get the character (handle surrogate pairs)
            QString charText;
            if (styleText[charIdx].isHighSurrogate() && charIdx + 1 < styleText.length()) {
                charText = styleText.mid(charIdx, 2);
                charIdx++; // Skip the low surrogate
            } else {
                charText = styleText[charIdx];
            }

            // Measure character width
            int charWidth = fm.horizontalAdvance(charText);

            // Draw background rectangle for this character
            painter.fillRect(x, y, charWidth, m_lineHeight, backColor);

            // Draw character
            painter.setPen(foreColor);
            painter.drawText(x, y + fm.ascent(), charText);

            // Move to next position
            x += charWidth;
            bytePos += charByteLen;
        }

        textPos = styleEnd;
    }
}

/**
 * Text Selection Implementation
 */

void OutputView::mousePressEvent(QMouseEvent* event)
{
    // Check miniwindows FIRST before text selection
    if (mouseDownMiniWindow(event->pos(), event->button())) {
        return; // Miniwindow handled the event
    }

    if (event->button() == Qt::LeftButton) {
        // Check for triple-click (third click after doubleClick)
        bool isTripleClick = false;
        if (m_lastClickTimer.isValid() &&
            m_lastClickTimer.elapsed() < QApplication::doubleClickInterval()) {
            // Check if click is in roughly the same location (within 5 pixels)
            QPoint delta = event->pos() - m_lastClickPos;
            if (qAbs(delta.x()) < 5 && qAbs(delta.y()) < 5) {
                isTripleClick = true;
            }
        }

        if (isTripleClick && m_doc) {
            // Triple-click: Select entire line
            int clickLine, clickChar;
            positionToLineChar(event->pos(), clickLine, clickChar);

            if (clickLine >= 0 && clickLine < m_doc->m_lineList.count()) {
                Line* pLine = m_doc->m_lineList[clickLine];
                m_selectionStartLine = clickLine;
                m_selectionStartChar = 0;
                m_selectionEndLine = clickLine;
                m_selectionEndChar = pLine ? pLine->len() : 0;

                // Update selection state in document
                m_doc->setSelection(m_selectionStartLine, m_selectionStartChar, m_selectionEndLine,
                                    m_selectionEndChar);

                m_selectionActive = false;     // Not actively dragging
                m_lastClickTimer.invalidate(); // Reset triple-click detection
                update();
            }
        } else {
            // Regular click: Start new selection
            m_selectionActive = true;
            positionToLineChar(event->pos(), m_selectionStartLine, m_selectionStartChar);
            m_selectionEndLine = m_selectionStartLine;
            m_selectionEndChar = m_selectionStartChar;

            // Update selection state in document
            if (m_doc) {
                m_doc->setSelection(m_selectionStartLine, m_selectionStartChar, m_selectionEndLine,
                                    m_selectionEndChar);
            }

            update();
        }
    }
}

void OutputView::mouseMoveEvent(QMouseEvent* event)
{
    // Check miniwindows FIRST
    if (mouseMoveMiniWindow(event->pos())) {
        // Notify plugins of mouse move (miniwindow handled it, use that ID)
        if (m_doc) {
            m_doc->SendToAllPluginCallbacks(ON_PLUGIN_MOUSE_MOVED, event->pos().x(),
                                            event->pos().y(), m_previousMiniwindow);
        }
        return; // Miniwindow handled the event (or we're in drag mode)
    }

    // Normal text selection handling
    if (m_selectionActive) {
        // Update selection end point
        positionToLineChar(event->pos(), m_selectionEndLine, m_selectionEndChar);

        // Update selection state in document
        if (m_doc) {
            m_doc->setSelection(m_selectionStartLine, m_selectionStartChar, m_selectionEndLine,
                                m_selectionEndChar);
        }

        update();
        setCursor(Qt::IBeamCursor);
    } else {
        // Check if hovering over a hyperlink
        std::shared_ptr<Action> action = getActionAtPosition(event->pos());
        if (action) {
            // Show pointing hand cursor for hyperlinks
            setCursor(Qt::PointingHandCursor);
        } else {
            // Restore default cursor when not over borders or miniwindows
            setCursor(Qt::IBeamCursor);
        }
    }

    // Notify plugins of mouse move (not over miniwindow)
    if (m_doc) {
        m_doc->SendToAllPluginCallbacks(ON_PLUGIN_MOUSE_MOVED, event->pos().x(), event->pos().y(),
                                        QString());
    }
}

void OutputView::mouseReleaseEvent(QMouseEvent* event)
{
    // Check miniwindows FIRST
    if (mouseUpMiniWindow(event->pos(), event->button())) {
        return; // Miniwindow handled the event
    }

    if (event->button() == Qt::LeftButton) {
        // Check if this was a click (not a drag) on a hyperlink
        // A click is when start and end positions are the same (or very close)
        if (m_selectionStartLine == m_selectionEndLine &&
            m_selectionStartChar == m_selectionEndChar) {
            // Check if there's a hyperlink at this position
            std::shared_ptr<Action> action = getActionAtPosition(event->pos());
            if (action) {
                // Open the URL
                QUrl url(action->m_strAction);
                if (url.isValid()) {
                    qDebug() << "Opening hyperlink:" << url.toString();
                    QDesktopServices::openUrl(url);
                }
                // Clear selection and return (don't allow selection on hyperlink clicks)
                m_selectionActive = false;
                update();
                return;
            }
        }

        // Normal selection handling
        if (m_selectionActive) {
            // Finalize selection (but keep it highlighted)
            m_selectionActive = false;
            update();

            // Notify plugins that selection has changed
            if (m_doc) {
                m_doc->SendToAllPluginCallbacks(ON_PLUGIN_SELECTION_CHANGED);
            }
        }
    }
}

void OutputView::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Check miniwindows FIRST
    if (mouseDownMiniWindow(event->pos(), event->button())) {
        return; // Miniwindow handled the event
    }

    if (event->button() == Qt::LeftButton) {
        // Track double-click for triple-click detection
        // Qt sends: press → release → doubleClick → release → press (for triple-click)
        // So we track the doubleClick and check for the next press within interval
        m_lastClickTimer.restart();
        m_lastClickPos = event->pos();

        // For now, we don't select anything on double-click
        // (Could add word selection here as an enhancement)
        event->accept();
    }
}

void OutputView::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy)) {
        // Ctrl+C
        copyToClipboard();
        event->accept();
        return;
    }

    // Page Up/Down scrolling
    if (event->key() == Qt::Key_PageUp) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Scroll up by one page (keep 2 lines overlap for context)
        int scrollAmount = qMax(1, m_visibleLines - 2);
        m_scrollPos -= scrollAmount;
        m_scrollPos = qMax(0, m_scrollPos);
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageDown) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Scroll down by one page (keep 2 lines overlap for context)
        int scrollAmount = qMax(1, m_visibleLines - 2);
        m_scrollPos += scrollAmount;

        int totalLines = m_doc->m_lineList.count();
        int maxScroll = qMax(0, totalLines - m_visibleLines);
        m_scrollPos = qMin(maxScroll, m_scrollPos);

        // Auto-unfreeze if we've reached the bottom
        if (m_freeze && m_scrollPos >= maxScroll) {
            setFrozen(false);
        }

        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Home && (event->modifiers() & Qt::ControlModifier)) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Ctrl+Home: Scroll to top
        m_scrollPos = 0;
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_End && (event->modifiers() & Qt::ControlModifier)) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Ctrl+End: Scroll to bottom and unfreeze
        int totalLines = m_doc->m_lineList.count();
        m_scrollPos = qMax(0, totalLines - m_visibleLines);
        if (m_freeze) {
            setFrozen(false);
        }
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Up && (event->modifiers() & Qt::ControlModifier)) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Ctrl+Up: Scroll up one line
        m_scrollPos = qMax(0, m_scrollPos - 1);
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Down && (event->modifiers() & Qt::ControlModifier)) {
        if (!m_doc || m_lineHeight <= 0) {
            event->accept();
            return;
        }
        // Ctrl+Down: Scroll down one line
        int totalLines = m_doc->m_lineList.count();
        int maxScroll = qMax(0, totalLines - m_visibleLines);
        m_scrollPos = qMin(maxScroll, m_scrollPos + 1);
        // Auto-unfreeze if we've reached the bottom
        if (m_freeze && m_scrollPos >= maxScroll) {
            setFrozen(false);
        }
        update();
        event->accept();
        return;
    }

    // Freeze/Pause shortcuts
    if (event->key() == Qt::Key_Pause || event->key() == Qt::Key_ScrollLock) {
        // Pause or Scroll Lock: Toggle freeze
        toggleFreeze();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F &&
        (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) ==
            (Qt::ControlModifier | Qt::ShiftModifier)) {
        // Ctrl+Shift+F: Toggle freeze
        toggleFreeze();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void OutputView::contextMenuEvent(QContextMenuEvent* event)
{
    // Check if we're over a miniwindow - if so, don't show default context menu
    // (miniwindows handle their own right-click menus via hotspots)
    QPoint pos = event->pos();
    MiniWindow* mw = mouseOverMiniwindow(pos);
    if (mw) {
        event->accept(); // Consume the event
        return;
    }

    // Show default context menu for text area
    QMenu menu(this);

    QAction* copyAction = menu.addAction("Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(hasSelection());
    connect(copyAction, &QAction::triggered, this, &OutputView::copyToClipboard);

    QAction* copyHtmlAction = menu.addAction("Copy as HTML");
    copyHtmlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    copyHtmlAction->setEnabled(hasSelection());
    connect(copyHtmlAction, &QAction::triggered, this, &OutputView::copyAsHtml);

    QAction* selectAllAction = menu.addAction("Select All");
    connect(selectAllAction, &QAction::triggered, this, &OutputView::selectAll);

    menu.exec(event->globalPos());
}

void OutputView::positionToLineChar(const QPoint& pos, int& line, int& charOffset)
{
    if (!m_doc || m_lineHeight <= 0) {
        line = 0;
        charOffset = 0;
        return;
    }

    // Get text rectangle to account for miniwindow offsets
    QRect textRect = getTextRectangle();

    // Convert pixel Y to line number (accounting for scroll position AND text rectangle offset)
    int lineIndex = ((pos.y() - textRect.top()) / m_lineHeight) + m_scrollPos;

    // Clamp to buffer bounds
    if (lineIndex < 0)
        lineIndex = 0;
    if (lineIndex >= m_doc->m_lineList.count())
        lineIndex = m_doc->m_lineList.count() - 1;

    line = lineIndex;

    // Convert pixel X to character offset
    if (lineIndex < 0 || lineIndex >= m_doc->m_lineList.count()) {
        charOffset = 0;
        return;
    }

    Line* pLine = m_doc->m_lineList[lineIndex];
    if (!pLine || pLine->len() == 0) {
        charOffset = 0;
        return;
    }

    // Adjust X for text rectangle offset
    int adjustedX = pos.x() - textRect.left();
    int x = 0;

    // Account for preamble width (must match drawLine logic)
    QString strPreamble;
    if (pLine->flags & COMMENT) {
        strPreamble = m_doc->m_strOutputLinePreambleNotes;
    } else if (pLine->flags & USER_INPUT) {
        strPreamble = m_doc->m_strOutputLinePreambleInput;
    } else {
        strPreamble = m_doc->m_strOutputLinePreambleOutput;
    }

    if (!strPreamble.isEmpty()) {
        // Expand time codes (simplified - matches drawLine)
        strPreamble.replace("%e", "0.000000");
        strPreamble.replace("%D", "0.000000");
        strPreamble = m_doc->FormatTime(pLine->m_theTime, strPreamble, false);

        // Use widget as paint device for accurate DPI-aware metrics
        QFontMetrics fm(m_font, const_cast<OutputView*>(this));
        x += fm.horizontalAdvance(strPreamble);
    }

    // If click is in preamble area, return position 0
    if (adjustedX < x) {
        charOffset = 0;
        return;
    }

    // Iterate through styles (must match drawLine rendering)
    int textPos = 0; // Byte position in line text
    charOffset = 0;

    for (const auto& style : pLine->styleList) {
        if (!style || style->iLength == 0)
            continue;

        if (textPos >= pLine->len())
            break;

        int styleLength = qMin((int)style->iLength, pLine->len() - textPos);

        // Apply same font attributes as drawLine
        QFont font = m_font;
        font.setBold(style->iFlags & HILITE);
        font.setUnderline(style->iFlags & UNDERLINE);
        font.setItalic(style->iFlags & BLINK);
        font.setStrikeOut(style->iFlags & STRIKEOUT);
        // Use widget as paint device for accurate DPI-aware metrics
        QFontMetrics fm(font, const_cast<OutputView*>(this));

        // Convert style text to QString using UTF-8 (matches drawLine)
        QString styleText = QString::fromUtf8(pLine->text() + textPos, styleLength);

        // Calculate byte length for each character (for proper byte offset tracking)
        int bytePos = textPos;

        for (int charIdx = 0; charIdx < styleText.length(); charIdx++) {
            // Handle surrogate pairs the same way as drawLine
            QString charText;
            int charByteLen;

            if (styleText[charIdx].isHighSurrogate() && charIdx + 1 < styleText.length() &&
                styleText[charIdx + 1].isLowSurrogate()) {
                // Surrogate pair (4-byte UTF-8 character like emoji)
                charText = styleText.mid(charIdx, 2);
                charByteLen = 4;
                charIdx++; // Skip the low surrogate (same as drawLine)
            } else {
                charText = styleText[charIdx];
                QChar ch = styleText[charIdx];
                if (ch.unicode() < 0x80) {
                    charByteLen = 1;
                } else if (ch.unicode() < 0x800) {
                    charByteLen = 2;
                } else {
                    charByteLen = 3;
                }
            }

            // Measure character width (use QString, same as drawLine)
            int charWidth = fm.horizontalAdvance(charText);

            // Check if click is on this character
            if (x + charWidth / 2 > adjustedX) {
                charOffset = bytePos;
                return;
            }

            x += charWidth;
            bytePos += charByteLen;
            charOffset = bytePos;
        }

        textPos += styleLength;
    }

    // Clicked beyond end of line
    charOffset = pLine->len();
}

/**
 * getActionAtPosition - Get the Action object at a specific pixel position
 *
 * Returns the Action object (hyperlink/command) at the given position, if any.
 * This is used for click handling and hover cursor changes.
 *
 * @param pos The pixel position to check
 * @return Shared pointer to Action if found, nullptr otherwise
 */
std::shared_ptr<Action> OutputView::getActionAtPosition(const QPoint& pos)
{
    if (!m_doc) {
        return nullptr;
    }

    // Convert position to line/char
    int lineIndex, charOffset;
    positionToLineChar(pos, lineIndex, charOffset);

    // Validate line index
    if (lineIndex < 0 || lineIndex >= m_doc->m_lineList.count()) {
        return nullptr;
    }

    Line* pLine = m_doc->m_lineList[lineIndex];
    if (!pLine || charOffset < 0 || charOffset >= pLine->len()) {
        return nullptr;
    }

    // Find the style that contains this character
    quint16 currentPos = 0;
    for (const auto& style : pLine->styleList) {
        quint16 styleEnd = currentPos + style->iLength;
        if (charOffset >= currentPos && charOffset < styleEnd) {
            // Found the style - check if it has a hyperlink action
            if (style->pAction && (style->iFlags & ACTION_HYPERLINK)) {
                return style->pAction;
            }
            break;
        }
        currentPos = styleEnd;
    }

    return nullptr;
}

void OutputView::normalizeSelection(int& startLine, int& startChar, int& endLine,
                                    int& endChar) const
{
    startLine = m_selectionStartLine;
    startChar = m_selectionStartChar;
    endLine = m_selectionEndLine;
    endChar = m_selectionEndChar;

    // Ensure start < end (handle backward selection)
    if (startLine > endLine || (startLine == endLine && startChar > endChar)) {
        qSwap(startLine, endLine);
        qSwap(startChar, endChar);
    }
}

bool OutputView::hasSelection() const
{
    return (m_selectionStartLine >= 0 && m_selectionEndLine >= 0 &&
            !(m_selectionStartLine == m_selectionEndLine &&
              m_selectionStartChar == m_selectionEndChar));
}

QString OutputView::getSelectedText() const
{
    if (!hasSelection() || !m_doc)
        return QString();

    int startLine, startChar, endLine, endChar;
    normalizeSelection(startLine, startChar, endLine, endChar);

    QString result;

    for (int lineIdx = startLine; lineIdx <= endLine; lineIdx++) {
        if (lineIdx < 0 || lineIdx >= m_doc->m_lineList.count())
            continue;

        Line* pLine = m_doc->m_lineList[lineIdx];
        if (!pLine || pLine->len() == 0)
            continue;

        int rangeStart = (lineIdx == startLine) ? startChar : 0;
        int rangeEnd = (lineIdx == endLine) ? endChar : pLine->len();

        // Clamp to line bounds
        rangeStart = qBound(0, rangeStart, pLine->len());
        rangeEnd = qBound(0, rangeEnd, pLine->len());

        // Extract text
        QString lineText = QString::fromUtf8(pLine->text() + rangeStart, rangeEnd - rangeStart);
        result += lineText;

        // Add newline between lines (except at end)
        if (lineIdx < endLine && pLine->hard_return) {
            result += "\n";
        }
    }

    return result;
}

void OutputView::copyToClipboard()
{
    QString selectedText = getSelectedText();
    if (!selectedText.isEmpty()) {
        QGuiApplication::clipboard()->setText(selectedText);
    }
}

QString OutputView::getSelectedTextAsHtml() const
{
    if (!hasSelection() || !m_doc)
        return QString();

    int startLine, startChar, endLine, endChar;
    normalizeSelection(startLine, startChar, endLine, endChar);

    // Get default background color from first style
    QColor defaultBackColor = Qt::black;
    if (startLine >= 0 && startLine < m_doc->m_lineList.count()) {
        Line* pLine = m_doc->m_lineList[startLine];
        if (pLine && !pLine->styleList.empty()) {
            Style* firstStyle = pLine->styleList.front().get();
            if (firstStyle) {
                defaultBackColor = ansiToRgb(firstStyle->iBackColour, firstStyle->iFlags, false);
            }
        }
    }

    // Build HTML with full document structure for better clipboard compatibility
    // (especially on macOS where apps expect complete HTML documents)
    QString html;
    html += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n";
    html += "<html><head>\n";
    html += "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n";
    html += "</head><body>\n";

    // Use table-based HTML like original MUSHclient (better compatibility)
    html += QString("<table border=0 cellpadding=5 bgcolor=\"%1\">").arg(defaultBackColor.name());
    html += "<tr><td>";
    html += "<pre><code>";
    html += QString("<font size=2 face=\"Courier New, Courier, monospace\">");
    html += QString("<font color=\"%1\">").arg(QColor(Qt::white).name());

    QColor lastForeColor = Qt::white;
    QColor lastBackColor = defaultBackColor;
    bool inSpan = false;

    for (int lineIdx = startLine; lineIdx <= endLine; lineIdx++) {
        if (lineIdx < 0 || lineIdx >= m_doc->m_lineList.count())
            continue;

        Line* pLine = m_doc->m_lineList[lineIdx];
        if (!pLine || pLine->len() == 0)
            continue;

        int rangeStart = (lineIdx == startLine) ? startChar : 0;
        int rangeEnd = (lineIdx == endLine) ? endChar : pLine->len();

        // Clamp to line bounds
        rangeStart = qBound(0, rangeStart, pLine->len());
        rangeEnd = qBound(0, rangeEnd, pLine->len());

        // Process each style in this line's range
        int textPos = 0;
        for (const auto& style : pLine->styleList) {
            if (!style || style->iLength == 0)
                continue;

            int styleStart = textPos;
            int styleEnd = textPos + style->iLength;
            textPos = styleEnd;

            // Skip styles before selection range
            if (styleEnd <= rangeStart)
                continue;

            // Stop if style starts after selection range
            if (styleStart >= rangeEnd)
                break;

            // Calculate intersection with selection
            int start = qMax(styleStart, rangeStart);
            int end = qMin(styleEnd, rangeEnd);
            int length = end - start;

            if (length <= 0)
                continue;

            // Get colors
            QColor foreColor = ansiToRgb(style->iForeColour, style->iFlags, style->iFlags & HILITE);
            QColor backColor = ansiToRgb(style->iBackColour, style->iFlags, false);

            // Handle INVERSE flag
            if (style->iFlags & INVERSE) {
                qSwap(foreColor, backColor);
            }

            // Change colors if needed (like original MUSHclient)
            if (foreColor != lastForeColor || backColor != lastBackColor) {
                // Close previous span if open
                if (inSpan) {
                    html += "</span>";
                    inSpan = false;
                }

                // Change font color
                html += QString("</font><font color=\"%1\">").arg(foreColor.name());

                // Add span for background color if different from default
                if (backColor != defaultBackColor) {
                    html += QString("<span style=\"background: %1\">").arg(backColor.name());
                    inSpan = true;
                }

                lastForeColor = foreColor;
                lastBackColor = backColor;
            }

            // Apply text decorations
            if (style->iFlags & UNDERLINE)
                html += "<u>";

            // Extract text for this style segment
            QString text = QString::fromUtf8(pLine->text() + start, length);

            // HTML-escape the text
            text = text.toHtmlEscaped();

            html += text;

            if (style->iFlags & UNDERLINE)
                html += "</u>";
        }

        // Add newline between lines (except at end)
        if (lineIdx < endLine && pLine->hard_return) {
            html += "\n";
        }
    }

    // Close any open span
    if (inSpan)
        html += "</span>";

    html += "</font></font></code></pre>";
    html += "</td></tr></table>";
    html += "\n</body></html>";

    return html;
}

void OutputView::copyAsHtml()
{
    QString htmlText = getSelectedTextAsHtml();
    if (!htmlText.isEmpty()) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setHtml(htmlText);
        // Also set plain text as fallback for apps that don't support HTML
        mimeData->setText(getSelectedText());
        QGuiApplication::clipboard()->setMimeData(mimeData);
    }
}

void OutputView::selectAll()
{
    if (!m_doc || m_doc->m_lineList.isEmpty())
        return;

    m_selectionStartLine = 0;
    m_selectionStartChar = 0;
    m_selectionEndLine = m_doc->m_lineList.count() - 1;

    Line* lastLine = m_doc->m_lineList.last();
    m_selectionEndChar = lastLine ? lastLine->len() : 0;

    // Update selection state in document
    m_doc->setSelection(m_selectionStartLine, m_selectionStartChar, m_selectionEndLine,
                        m_selectionEndChar);

    update();
}

void OutputView::clearSelection()
{
    m_selectionStartLine = -1;
    m_selectionStartChar = -1;
    m_selectionEndLine = -1;
    m_selectionEndChar = -1;
    m_selectionActive = false;

    // Update selection state in document
    if (m_doc) {
        m_doc->clearSelection();
    }

    update();
}

bool OutputView::isCharSelected(int lineIdx, int charOffset) const
{
    if (!hasSelection())
        return false;

    int startLine, startChar, endLine, endChar;
    normalizeSelection(startLine, startChar, endLine, endChar);

    // Check if character is within selection range
    if (lineIdx < startLine || lineIdx > endLine)
        return false;

    if (lineIdx == startLine && lineIdx == endLine) {
        // Selection on single line
        return (charOffset >= startChar && charOffset < endChar);
    } else if (lineIdx == startLine) {
        // First line of multi-line selection
        return (charOffset >= startChar);
    } else if (lineIdx == endLine) {
        // Last line of multi-line selection
        return (charOffset < endChar);
    } else {
        // Middle line of multi-line selection
        return true;
    }
}

/**
 * Direct scroll methods - called from Display menu
 */

void OutputView::scrollToTop()
{
    qCDebug(lcUI) << "OutputView::scrollToTop() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollToTop - not initialized";
        return;
    }
    m_scrollPos = 0;
    update();
}

void OutputView::scrollToBottom()
{
    qCDebug(lcUI) << "OutputView::scrollToBottom() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollToBottom - not initialized";
        return;
    }
    int totalLines = m_doc->m_lineList.count();
    m_scrollPos = qMax(0, totalLines - m_visibleLines);
    // Unfreeze when scrolling to bottom
    if (m_freeze) {
        setFrozen(false);
    }
    update();
}

void OutputView::scrollPageUp()
{
    qCDebug(lcUI) << "OutputView::scrollPageUp() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollPageUp - not initialized";
        return;
    }
    int scrollAmount = qMax(1, m_visibleLines - 2);
    m_scrollPos -= scrollAmount;
    m_scrollPos = qMax(0, m_scrollPos);
    update();
}

void OutputView::scrollPageDown()
{
    qCDebug(lcUI) << "OutputView::scrollPageDown() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollPageDown - not initialized";
        return;
    }
    int scrollAmount = qMax(1, m_visibleLines - 2);
    m_scrollPos += scrollAmount;
    int totalLines = m_doc->m_lineList.count();
    int maxScroll = qMax(0, totalLines - m_visibleLines);
    m_scrollPos = qMin(maxScroll, m_scrollPos);
    // Auto-unfreeze if we've reached the bottom
    if (m_freeze && m_scrollPos >= maxScroll) {
        setFrozen(false);
    }
    update();
}

void OutputView::scrollLineUp()
{
    qCDebug(lcUI) << "OutputView::scrollLineUp() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollLineUp - not initialized";
        return;
    }
    m_scrollPos = qMax(0, m_scrollPos - 1);
    update();
}

void OutputView::scrollLineDown()
{
    qCDebug(lcUI) << "OutputView::scrollLineDown() called";
    if (!m_doc || m_lineHeight <= 0) {
        qCDebug(lcUI) << "OutputView::scrollLineDown - not initialized";
        return;
    }
    int totalLines = m_doc->m_lineList.count();
    int maxScroll = qMax(0, totalLines - m_visibleLines);
    m_scrollPos = qMin(maxScroll, m_scrollPos + 1);
    // Auto-unfreeze if we've reached the bottom
    if (m_freeze && m_scrollPos >= maxScroll) {
        setFrozen(false);
    }
    update();
}

void OutputView::scrollToLine(int lineIndex)
{
    qCDebug(lcUI) << "OutputView::scrollToLine() called, line:" << lineIndex;
    if (!m_doc || m_lineHeight <= 0) {
        return;
    }
    int totalLines = m_doc->m_lineList.count();
    int maxScroll = qMax(0, totalLines - m_visibleLines);
    m_scrollPos = qBound(0, lineIndex, maxScroll);
    update();
}

/**
 * selectTextAt - Select text at a specific position
 *
 * Used by Find dialog to highlight search results.
 */
void OutputView::selectTextAt(int lineIndex, int charOffset, int length)
{
    if (!m_doc || lineIndex < 0 || lineIndex >= m_doc->m_lineList.count())
        return;

    // Set selection range
    m_selectionStartLine = lineIndex;
    m_selectionStartChar = charOffset;
    m_selectionEndLine = lineIndex; // Single-line selection for now
    m_selectionEndChar = charOffset + length;

    // Update selection state in document
    m_doc->setSelection(m_selectionStartLine, m_selectionStartChar, m_selectionEndLine,
                        m_selectionEndChar);

    // Scroll to show the selected line if it's not visible
    if (lineIndex < m_scrollPos) {
        // Selection is above visible area - scroll up
        m_scrollPos = lineIndex;
    } else if (lineIndex >= m_scrollPos + m_visibleLines) {
        // Selection is below visible area - scroll down
        m_scrollPos = lineIndex - m_visibleLines + 1;
        m_scrollPos = qMax(0, m_scrollPos);
    }

    // Trigger repaint to show selection
    update();
}

/**
 * drawMiniWindows - Draw miniwindows layer
 *
 * Miniwindow Rendering Integration
 *
 * Draws all visible miniwindows in a specific layer (underneath or on top of text).
 * Miniwindows are sorted by z-order before drawing (lower values draw first).
 * Within the same z-order, miniwindows are drawn alphabetically by name.
 *
 * @param painter QPainter to draw with
 * @param underneath true to draw DRAW_UNDERNEATH layer, false for normal layer
 */
QRect OutputView::getTextRectangle(bool includeBorder) const
{
    if (!m_doc)
        return rect();

    QRect textRect = m_doc->m_TextRectangle;
    QRect clientRect = rect();

    // Check if text rectangle is set (not all zeros)
    if (textRect == QRect(0, 0, 0, 0)) {
        return clientRect; // No text rectangle - use full window
    }

    // Handle negative right/bottom values (offsets from edges)
    if (textRect.right() <= 0) {
        textRect.setRight(clientRect.right() + textRect.right());
        // Ensure not negative
        if (textRect.right() < textRect.left() + 20)
            textRect.setRight(textRect.left() + 20);
    }

    if (textRect.bottom() <= 0) {
        textRect.setBottom(clientRect.bottom() + textRect.bottom());
        // Ensure not negative
        if (textRect.bottom() < textRect.top() + 20)
            textRect.setBottom(textRect.top() + 20);
    }

    // Include border offset if requested
    if (includeBorder) {
        int offset = m_doc->m_TextRectangleBorderOffset;
        textRect.adjust(-offset, -offset, offset, offset);
    }

    return textRect;
}

/**
 * haveTextRectangle - Check if text rectangle is configured
 *
 * Based on CMUSHView::HaveTextRectangle()
 */
bool OutputView::haveTextRectangle() const
{
    if (!m_doc)
        return false;

    return m_doc->m_TextRectangle != QRect(0, 0, 0, 0);
}

/**
 * reloadBackgroundImage - Load background image from WorldDocument path
 *
 * Background/Foreground Image Support
 *
 * Called when SetBackgroundImage Lua function changes the image path.
 * Loads the image from m_strBackgroundImageName and triggers repaint.
 */
void OutputView::reloadBackgroundImage()
{
    if (!m_doc)
        return;

    if (m_doc->m_strBackgroundImageName.isEmpty()) {
        m_backgroundImage = QPixmap(); // Clear image
    } else {
        // Try to load the image
        if (!m_backgroundImage.load(m_doc->m_strBackgroundImageName)) {
            qCWarning(lcUI) << "Failed to load background image:"
                            << m_doc->m_strBackgroundImageName;
            m_backgroundImage = QPixmap();
        }
    }

    update(); // Trigger repaint
}

/**
 * reloadForegroundImage - Load foreground image from WorldDocument path
 *
 * Background/Foreground Image Support
 *
 * Called when SetForegroundImage Lua function changes the image path.
 * Loads the image from m_strForegroundImageName and triggers repaint.
 */
void OutputView::reloadForegroundImage()
{
    if (!m_doc)
        return;

    if (m_doc->m_strForegroundImageName.isEmpty()) {
        m_foregroundImage = QPixmap(); // Clear image
    } else {
        // Try to load the image
        if (!m_foregroundImage.load(m_doc->m_strForegroundImageName)) {
            qCWarning(lcUI) << "Failed to load foreground image:"
                            << m_doc->m_strForegroundImageName;
            m_foregroundImage = QPixmap();
        }
    }

    update(); // Trigger repaint
}

/**
 * drawImage - Draw an image with the specified mode
 *
 * Background/Foreground Image Support
 *
 * Based on CMUSHView::Blit_Bitmap()
 *
 * Modes:
 *   0 = Stretch to fit (ignore aspect ratio)
 *   1 = Stretch with aspect ratio (center, fill one dimension)
 *   4 = Top-left corner
 *   5 = Top-center
 *   6 = Top-right corner
 *   7 = Right-center
 *   8 = Bottom-right corner
 *   9 = Bottom-center
 *  10 = Bottom-left corner
 *  11 = Left-center
 *  12 = Center
 *  13 = Tile
 *
 * @param painter QPainter to draw with
 * @param pixmap Image to draw
 * @param mode Drawing mode (0-13)
 */
void OutputView::drawImage(QPainter& painter, const QPixmap& pixmap, int mode)
{
    if (pixmap.isNull())
        return;

    // Validate mode (matches original: Mode < 0 || Mode > 13 returns eBadParameter)
    if (mode < 0 || mode > 13)
        return;

    int imgWidth = pixmap.width();
    int imgHeight = pixmap.height();

    if (imgWidth <= 0 || imgHeight <= 0)
        return;

    // Get appropriate rect based on mode
    // Modes 2, 3 use parent window rect in original; modes 0, 1, 4-13 use view client rect
    QRect targetRect;
    if (mode == 2 || mode == 3) {
        // Use parent window's rect (like original's GetFrame()->GetClientRect)
        QWidget* parentWindow = window();
        if (parentWindow) {
            targetRect = parentWindow->rect();
        } else {
            targetRect = rect();
        }
    } else {
        targetRect = rect();
    }

    int clientWidth = targetRect.width();
    int clientHeight = targetRect.height();

    switch (mode) {
        case 0: // Stretch to fit view (ignore aspect ratio)
        case 2: // Stretch to fit frame (ignore aspect ratio)
            painter.drawPixmap(targetRect, pixmap);
            break;

        case 1: // Stretch with aspect ratio (height-based, view)
        case 3: // Stretch with aspect ratio (height-based, frame)
        {
            // Original: width = height * ratio, height = clientHeight
            // This fills by height and may leave empty space on sides
            double ratio = (double)imgWidth / (double)imgHeight;
            int scaledWidth = (int)(clientHeight * ratio);
            int scaledHeight = clientHeight;
            painter.drawPixmap(QRect(0, 0, scaledWidth, scaledHeight), pixmap);
            break;
        }

        case 4: // Top-left corner
            painter.drawPixmap(0, 0, pixmap);
            break;

        case 5: // Top-center
            painter.drawPixmap((clientWidth - imgWidth) / 2, 0, pixmap);
            break;

        case 6: // Top-right corner
            painter.drawPixmap(clientWidth - imgWidth, 0, pixmap);
            break;

        case 7: // Right-center
            painter.drawPixmap(clientWidth - imgWidth, (clientHeight - imgHeight) / 2, pixmap);
            break;

        case 8: // Bottom-right corner
            painter.drawPixmap(clientWidth - imgWidth, clientHeight - imgHeight, pixmap);
            break;

        case 9: // Bottom-center
            painter.drawPixmap((clientWidth - imgWidth) / 2, clientHeight - imgHeight, pixmap);
            break;

        case 10: // Bottom-left corner
            painter.drawPixmap(0, clientHeight - imgHeight, pixmap);
            break;

        case 11: // Left-center
            painter.drawPixmap(0, (clientHeight - imgHeight) / 2, pixmap);
            break;

        case 12: // Center
            painter.drawPixmap((clientWidth - imgWidth) / 2, (clientHeight - imgHeight) / 2,
                               pixmap);
            break;

        case 13: { // Tile
            int iAcross = (clientWidth / imgWidth) + 1;
            int iDown = (clientHeight / imgHeight) + 1;
            for (int x = 0; x < iAcross; x++) {
                for (int y = 0; y < iDown; y++) {
                    painter.drawPixmap(x * imgWidth, y * imgHeight, pixmap);
                }
            }
            break;
        }
    }
}

/**
 * calculateMiniWindowRectangles - Position miniwindows based on text rectangle
 *
 * Text Rectangle Architecture
 *
 * Based on CMUSHView::Calculate_MiniWindow_Rectangles()
 *
 * This is the full positioning algorithm that positions miniwindows relative to
 * the text rectangle edges. It handles:
 * - Corner positioning (positions 4, 6, 8, 10)
 * - Centered positioning (positions 5, 7, 9, 11) with even spacing
 * - Absolute positioning (MINIWINDOW_ABSOLUTE_LOCATION flag)
 * - Stretch modes (positions 0-3, 12)
 * - Overflow handling (temporarilyHide flag)
 */

/**
 * setFrozen - Set freeze state
 *
 * When frozen, auto-scrolling is disabled and new lines are counted
 * but the view doesn't scroll to show them.
 *
 * @param frozen true to freeze, false to unfreeze
 */
void OutputView::setFrozen(bool frozen)
{
    if (m_freeze == frozen)
        return; // No change

    m_freeze = frozen;

    if (frozen) {
        // Just started freezing - reset line count
        m_frozenLineCount = 0;
    } else {
        // Unfreezing - scroll to bottom if we have new lines
        if (m_frozenLineCount > 0 && m_doc) {
            int totalLines = m_doc->m_lineList.count();
            m_scrollPos = qMax(0, totalLines - m_visibleLines);
        }
        m_frozenLineCount = 0;
    }

    emit freezeStateChanged(m_freeze, m_frozenLineCount);
    update();
}

/**
 * toggleFreeze - Toggle freeze state
 */
void OutputView::toggleFreeze()
{
    setFrozen(!m_freeze);
}
