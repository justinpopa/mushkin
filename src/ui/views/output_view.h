#ifndef OUTPUT_VIEW_H
#define OUTPUT_VIEW_H

#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QPixmap>
#include <QPoint>
#include <QWidget>
#include <memory>

// Forward declarations
class WorldDocument;
class Line;
class MiniWindow;
class QPainter;
class QPaintEvent;
class QWheelEvent;
class QResizeEvent;
class Action;

/**
 * OutputView - Custom widget to display MUD text with colors and styles
 *
 * Basic Text Display Widget
 *
 * This widget uses custom painting (QPainter) to render the text buffer from
 * WorldDocument. It supports:
 * - ANSI color display (with lookup tables)
 * - Text styles (bold, underline, italic, strikeout)
 * - Scrolling with mouse wheel
 * - Auto-scroll to bottom when new lines arrive
 * - Efficient rendering of large buffers (5000+ lines)
 *
 * Based on original MUSHclient's CSendView (sendvw.cpp) which used
 * custom CDC/GDI drawing.
 */
class OutputView : public QWidget {
    Q_OBJECT

  public:
    explicit OutputView(WorldDocument* doc, QWidget* parent = nullptr);
    ~OutputView() override;

  public slots:
    /**
     * setOutputFont - Update the display font
     *
     * Updates the font used for rendering text and recalculates metrics.
     * Call this when the user changes the font in World Properties.
     *
     * @param font New font to use
     */
    void setOutputFont(const QFont& font);

    /**
     * copyToClipboard - Copy selected text to clipboard
     *
     * Text Selection
     */
    void copyToClipboard();

    /**
     * copyAsHtml - Copy selected text as HTML to clipboard
     *
     * Preserves colors, bold, underline, italic, and other formatting.
     */
    void copyAsHtml();

    /**
     * selectAll - Select all text in buffer
     *
     * Text Selection
     */
    void selectAll();

    /**
     * clearSelection - Clear current selection
     *
     * Text Selection
     */
    void clearSelection();

    /**
     * Direct scroll methods
     */
    void scrollToTop();
    void scrollToBottom();
    void scrollPageUp();
    void scrollPageDown();
    void scrollLineUp();
    void scrollLineDown();
    void scrollToLine(int lineIndex);

    /**
     * getSelectedText - Extract plain text from selection
     *
     * Text Selection
     *
     * @return Selected text with newlines preserved
     */
    QString getSelectedText() const;

    /**
     * hasSelection - Check if selection exists
     *
     * Text Selection
     *
     * @return true if selection active
     */
    bool hasSelection() const;

    /**
     * getSelectionStartLine - Get the starting line of current selection
     *
     * @return Line index of selection start, or -1 if no selection
     */
    int getSelectionStartLine() const
    {
        if (!m_selectionActive)
            return -1;
        int startLine, startChar, endLine, endChar;
        normalizeSelection(startLine, startChar, endLine, endChar);
        return startLine;
    }

    /**
     * normalizeSelection - Ensure start <= end
     *
     * Text Selection
     * Handles backward selections (drag upward/leftward)
     *
     * @param startLine Output: normalized start line
     * @param startChar Output: normalized start char
     * @param endLine Output: normalized end line
     * @param endChar Output: normalized end char
     */
    void normalizeSelection(int& startLine, int& startChar, int& endLine, int& endChar) const;

    /**
     * selectTextAt - Select text at a specific position
     *
     * Used by Find dialog to highlight search results.
     *
     * @param lineIndex Line index in buffer
     * @param charOffset Character offset within line
     * @param length Length of text to select
     */
    void selectTextAt(int lineIndex, int charOffset, int length);

    /**
     * getTextRectangle - Get normalized text rectangle
     *
     * Text Rectangle Architecture
     *
     * Handles negative right/bottom values (offsets from edges).
     * If no text rectangle is set (0,0,0,0), returns full client rect.
     *
     * @param includeBorder If true, expand by border offset
     * @return Normalized text rectangle
     */
    QRect getTextRectangle(bool includeBorder = false) const;

    /**
     * getScrollPositionPixels - Get current scroll position in pixels
     *
     * Converts line-based scroll position to pixel position for GetInfo(296).
     *
     * @return Scroll position in pixels (Y coordinate)
     */
    int getScrollPositionPixels() const
    {
        return m_scrollPos * m_lineHeight;
    }

    /**
     * recalculateMetrics - Public method to trigger metrics recalculation
     *
     * Text Rectangle Architecture
     *
     * Called when text rectangle changes (via TextRectangle Lua function) to
     * update m_visibleLines based on the new text rectangle dimensions.
     */
    void recalculateMetrics()
    {
        calculateMetrics();
    }

    /**
     * reloadBackgroundImage - Reload background image from WorldDocument path
     *
     * Called when SetBackgroundImage Lua function changes the image.
     * Loads the image from m_strBackgroundImageName and triggers repaint.
     */
    void reloadBackgroundImage();

    /**
     * reloadForegroundImage - Reload foreground image from WorldDocument path
     *
     * Called when SetForegroundImage Lua function changes the image.
     * Loads the image from m_strForegroundImageName and triggers repaint.
     */
    void reloadForegroundImage();

    /**
     * Freeze/Pause Control
     *
     * When frozen, auto-scrolling is disabled and new lines are counted
     * but the view doesn't scroll to show them.
     */

    /**
     * isFrozen - Check if output is frozen
     * @return true if output is frozen
     */
    bool isFrozen() const
    {
        return m_freeze;
    }

    /**
     * setFrozen - Set freeze state
     * @param frozen true to freeze, false to unfreeze
     */
    void setFrozen(bool frozen);

    /**
     * toggleFreeze - Toggle freeze state
     */
    void toggleFreeze();

    /**
     * frozenLineCount - Get count of lines received while frozen
     * @return Number of lines received since freeze started
     */
    int frozenLineCount() const
    {
        return m_frozenLineCount;
    }

  signals:
    /**
     * freezeStateChanged - Emitted when freeze state changes
     * @param frozen New freeze state
     * @param lineCount Number of lines buffered while frozen
     */
    void freezeStateChanged(bool frozen, int lineCount);

  protected:
    // Qt event handlers
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

  private slots:
    /**
     * onNewLinesAdded - Called when WorldDocument adds new lines
     *
     * If scrolled to bottom, auto-scrolls to show new lines.
     * If scrolled up (user is reading history), doesn't auto-scroll.
     */
    void onNewLinesAdded();

    /**
     * onIncompleteLine - Called when WorldDocument has incomplete line data (prompts)
     *
     * Incomplete lines are lines without newlines (like MUD prompts).
     * They need to be displayed but aren't added to the line buffer yet.
     */
    void onIncompleteLine();

  private:
    /**
     * calculateMetrics - Calculate line height and visible lines from font
     *
     * Called in constructor and whenever font or window size changes.
     */
    void calculateMetrics();

    /**
     * drawLine - Render one line with all its styles
     *
     * @param painter QPainter to draw with
     * @param y Y coordinate (top of line)
     * @param line Line to draw
     * @param lineIndex Line index in buffer (for selection checking)
     */
    void drawLine(QPainter& painter, int y, Line* line, int lineIndex);

    /**
     * ansiToRgb - Convert ANSI color index or RGB value to QColor
     *
     * Uses COLOURTYPE bits from flags to determine interpretation:
     * - COLOUR_ANSI: index 0-7 for standard, 8-255 for extended
     * - COLOUR_CUSTOM: index into custom palette
     * - COLOUR_RGB: direct RGB value
     *
     * @param color ANSI index or RGB value
     * @param flags Style flags containing COLOURTYPE bits
     * @param bold If true and color is ANSI, use bold/bright color table
     * @return QColor for rendering
     */
    QColor ansiToRgb(quint32 color, quint16 flags, bool bold) const;

    /**
     * isAtBottom - Check if scrolled to bottom of buffer
     *
     * Used to determine whether to auto-scroll when new lines arrive.
     *
     * @return true if at bottom, false if scrolled up
     */
    bool isAtBottom() const;

    /**
     * positionToLineChar - Convert pixel position to line/char coordinates
     *
     * Text Selection
     *
     * @param pos Mouse position in widget coordinates
     * @param line Output: line index in buffer
     * @param charOffset Output: character offset in line
     */
    void positionToLineChar(const QPoint& pos, int& line, int& charOffset);

    /**
     * getActionAtPosition - Get the Action object at a specific pixel position
     *
     * Returns the Action object (hyperlink/command) at the given position, if any.
     * Used for click handling and hover cursor changes.
     *
     * @param pos The pixel position to check
     * @return Shared pointer to Action if found, nullptr otherwise
     */
    std::shared_ptr<Action> getActionAtPosition(const QPoint& pos);

    /**
     * getSelectedTextAsHtml - Extract HTML-formatted text from selection
     *
     * Converts selected text with styles to HTML with inline CSS.
     * Preserves colors, bold, underline, italic, strikeout, etc.
     *
     * @return Selected text as HTML
     */
    QString getSelectedTextAsHtml() const;

    /**
     * isCharSelected - Check if a character position is within selection
     *
     * Text Selection
     *
     * @param lineIdx Line index
     * @param charOffset Character offset in line
     * @return true if character is selected
     */
    bool isCharSelected(int lineIdx, int charOffset) const;

    /**
     * haveTextRectangle - Check if text rectangle is configured
     *
     * Text Rectangle Architecture
     *
     * @return true if text rectangle is set, false if using full window
     */
    bool haveTextRectangle() const;

    /**
     * calculateMiniWindowRectangles - Position miniwindows based on text rectangle
     *
     * Text Rectangle Architecture
     *
     * Implements the full positioning algorithm:
     * 1. Position corners and absolute miniwindows
     * 2. Calculate available space after corners placed
     * 3. Drop miniwindows that don't fit (set temporarilyHide = true)
     * 4. Position centered miniwindows with even gaps
     *
     * @param underneath true for DRAW_UNDERNEATH layer, false for on-top layer
     */
    void calculateMiniWindowRectangles(bool underneath);

    /**
     * drawMiniWindows - Draw miniwindows layer
     *
     * Miniwindow Rendering
     *
     * Draws all visible miniwindows in a specific layer (underneath or on top).
     * Miniwindows are sorted by z-order before drawing (lower z-order draws first).
     *
     * @param painter QPainter to draw with
     * @param underneath true to draw DRAW_UNDERNEATH layer, false for normal layer
     */
    void drawMiniWindows(QPainter& painter, bool underneath);

    /**
     * mouseOverMiniwindow - Find miniwindow under mouse cursor
     *
     * Miniwindow Mouse Events
     *
     * Searches for the topmost miniwindow at the given position.
     * Iterates in reverse z-order (topmost first) and returns the first
     * visible, non-underneath, non-ignore-mouse miniwindow that contains the point.
     *
     * @param pos Mouse position in widget coordinates
     * @return Pointer to miniwindow under cursor, or nullptr if none
     */
    class MiniWindow* mouseOverMiniwindow(const QPoint& pos);

    /**
     * mouseDownMiniWindow - Handle mouse button press in miniwindow
     *
     * Miniwindow Mouse Events
     *
     * @param pos Mouse position in widget coordinates
     * @param button Qt::MouseButton that was pressed
     * @return true if a miniwindow handled the event, false otherwise
     */
    bool mouseDownMiniWindow(const QPoint& pos, Qt::MouseButton button);

    /**
     * mouseMoveMiniWindow - Handle mouse move over miniwindow
     *
     * Miniwindow Mouse Events
     *
     * @param pos Mouse position in widget coordinates
     * @return true if a miniwindow handled the event, false otherwise
     */
    bool mouseMoveMiniWindow(const QPoint& pos);

    /**
     * mouseUpMiniWindow - Handle mouse button release in miniwindow
     *
     * Miniwindow Mouse Events
     *
     * @param pos Mouse position in widget coordinates
     * @param button Qt::MouseButton that was released
     * @return true if a miniwindow handled the event, false otherwise
     */
    bool mouseUpMiniWindow(const QPoint& pos, Qt::MouseButton button);

    /**
     * handleMiniWindowScrollWheel - Handle scroll wheel over miniwindow
     *
     * Checks if mouse is over a miniwindow with scroll wheel handler.
     * Returns true if handled, false to allow text scrolling.
     *
     * @param pos Mouse position in output view coordinates
     * @param angleDelta Scroll wheel angle delta
     * @param modifiers Keyboard modifiers
     * @return true if miniwindow handled scroll, false otherwise
     */
    bool handleMiniWindowScrollWheel(const QPoint& pos, const QPoint& angleDelta,
                                     Qt::KeyboardModifiers modifiers);

    // Data members
    WorldDocument* m_doc; // Document to display (not owned)
    QFont m_font;         // Display font (fixed-width recommended)
    int m_lineHeight;     // Height of one line in pixels
    int m_charWidth;      // Width of one character (for monospace font)
    int m_scrollPos;      // Current scroll position (line index from top of buffer)
    int m_visibleLines;   // How many lines fit in widget height

    // Text Selection
    bool m_selectionActive;         // Is selection in progress?
    int m_selectionStartLine;       // Line number where selection starts
    int m_selectionStartChar;       // Character offset in start line
    int m_selectionEndLine;         // Line number where selection ends
    int m_selectionEndChar;         // Character offset in end line
    QElapsedTimer m_lastClickTimer; // Timer for triple-click detection
    QPoint m_lastClickPos;          // Position of last click for triple-click detection

    // Miniwindow Mouse Tracking
    QString m_mouseDownMiniwindow; // Name of miniwindow where mouse was pressed (empty if none)
    QString m_mouseOverMiniwindow; // Name of miniwindow mouse is currently over (empty if none)
    QString m_previousMiniwindow;  // Previous miniwindow with mouse capture (for drag operations)
    Qt::MouseButton m_mouseDownButton; // Which button was pressed (for drag operations)

    // Background/Foreground Images
    QPixmap m_backgroundImage; // Cached background image
    QPixmap m_foregroundImage; // Cached foreground image

    // Freeze/Pause State
    bool m_freeze;         // Output is frozen (no auto-scroll)
    int m_frozenLineCount; // Lines received while frozen

    /**
     * drawImage - Draw an image with specified mode
     *
     * Based on CMUSHView::Blit_Bitmap()
     *
     * Modes:
     * 0 = stretch to fit view (ignore aspect ratio)
     * 1 = stretch with aspect ratio (height-based, view rect)
     * 2 = stretch to fit frame (ignore aspect ratio)
     * 3 = stretch with aspect ratio (height-based, frame rect)
     * 4 = top-left corner
     * 5 = top-center
     * 6 = top-right corner
     * 7 = right-center
     * 8 = bottom-right corner
     * 9 = bottom-center
     * 10 = bottom-left corner
     * 11 = left-center
     * 12 = center
     * 13 = tile
     *
     * @param painter QPainter to draw with
     * @param pixmap Image to draw
     * @param mode Drawing mode (0-13)
     */
    void drawImage(QPainter& painter, const QPixmap& pixmap, int mode);
};

#endif // OUTPUT_VIEW_H
