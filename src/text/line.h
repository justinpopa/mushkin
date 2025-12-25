#ifndef LINE_H
#define LINE_H

#include <QColor>
#include <QDateTime>
#include <QtGlobal>
#include <memory>
#include <vector>

// Forward declaration
class Style;

// ========== LINE FLAGS (OtherTypes.h) ==========
// Bit flags for the 'flags' member of Line

const int COMMENT = 0x01;    // This is a comment from a script
const int USER_INPUT = 0x02; // This is echoed user input
const int LOG_LINE = 0x04;   // This line should be logged to file
const int BOOKMARK = 0x08;   // Line is bookmarked by user
const int HORIZ_RULE = 0x10; // Line is a horizontal rule

const int NOTE_OR_COMMAND = 0x03; // Helper: test if line is comment or input (not output)

/**
 * Line - One line of text with associated styles
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * WHY THIS EXISTS:
 * Represents one complete line of text received from the MUD, along with
 * all the styling information (colors, bold, hyperlinks, etc.) needed to
 * display it.
 *
 * THE DATA FLOW:
 * 1. Text arrives from MUD as stream of bytes
 * 2. Telnet protocol processor strips IAC codes
 * 3. ANSI parser processes escape sequences (colors, styles)
 * 4. Text and style info accumulate in "current line"
 * 5. When newline arrives, create a Line object
 * 6. Line is added to document's line buffer (m_LineList)
 * 7. OutputView renders Line objects to screen
 *
 * STRUCTURE:
 * - text: raw text bytes in a dynamically-allocated buffer
 * - styleList: list of Style* objects describing how to render the text
 * - Each Style covers a contiguous run of same-styled characters
 * - Example: "Hello world" with "Hello" red, "world" blue = 2 Styles
 *
 * MEMORY MANAGEMENT:
 * - text buffer is managed by std::vector<char> - automatic growth and cleanup
 * - styleList uses std::unique_ptr for automatic Style ownership
 * - No manual memory management required
 */
class Line {
  public:
    /**
     * Constructor
     *
     * Source: OtherTypes.h (original MUSHclient)
     *
     * @param lineNumber Sequential line number (for tracking)
     * @param wrapColumn Wrap column width (for initial buffer allocation)
     * @param lineFlags Line flags (COMMENT, USER_INPUT, LOG_LINE, etc.)
     * @param foreColour Default foreground color for this line
     * @param backColour Default background color for this line
     * @param isUnicode Whether text is Unicode (affects buffer allocation)
     */
    Line(qint32 lineNumber, quint32 wrapColumn, quint16 lineFlags, QRgb foreColour, QRgb backColour,
         bool isUnicode);

    /**
     * Destructor
     *
     * Source: OtherTypes.h (original MUSHclient)
     *
     * Automatic cleanup via RAII - no manual management needed.
     */
    ~Line();

    // Public members

    bool hard_return;             // true if line ended with CR/LF, false if wrapped
    unsigned char flags;          // Line flags (see defines above)
    std::vector<char> textBuffer; // Text buffer (automatically managed)
    std::vector<std::unique_ptr<Style>> styleList; // List of Style objects (automatically managed)
    QDateTime m_theTime;                           // When this line arrived
    qint64 m_lineHighPerformanceTime;              // High-resolution timestamp
    qint32 m_nLineNumber;                          // Sequential line number
    qint16 m_iPreambleOffset;                      // How far the preamble text extends

    // Compatibility accessors for code that expects raw pointer
    char* text()
    {
        return textBuffer.data();
    }
    const char* text() const
    {
        return textBuffer.data();
    }
    qint32 len() const
    {
        // Return text length excluding null terminator
        // textBuffer should always end with \0, so size > 0
        return textBuffer.empty() ? 0 : static_cast<qint32>(textBuffer.size() - 1);
    }
    qint32 iMemoryAllocated() const
    {
        return static_cast<qint32>(textBuffer.capacity());
    }

  private:
    // Helper: initial buffer size for text allocation
    static const qint32 INITIAL_BUFFER_SIZE = 256;
};

#endif // LINE_H