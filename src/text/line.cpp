#include "line.h"
#include "style.h"

/**
 * Constructor
 *
 * Source: OtherTypes.h (original MUSHclient) and method implementation in methods.cpp
 *
 * Creates a new Line with an initial text buffer and default styling.
 * The text buffer is allocated to accommodate at least wrapColumn characters.
 *
 * @param lineNumber Sequential line number for tracking
 * @param wrapColumn Wrap column width (used to size initial buffer)
 * @param lineFlags Line flags (COMMENT, USER_INPUT, etc.)
 * @param foreColour Default foreground color
 * @param backColour Default background color
 * @param isUnicode Whether text is Unicode (affects buffer sizing)
 */
Line::Line(qint32 lineNumber, quint32 wrapColumn, quint16 lineFlags, QRgb foreColour,
           QRgb backColour, bool isUnicode)
    : hard_return(false), flags(static_cast<unsigned char>(lineFlags)),
      m_theTime(QDateTime::currentDateTime()), m_lineHighPerformanceTime(0),
      m_nLineNumber(lineNumber), m_iPreambleOffset(0)
{
    // Calculate initial buffer size
    // Use at least INITIAL_BUFFER_SIZE (256), or wrapColumn + some margin
    qint32 bufferSize = INITIAL_BUFFER_SIZE;
    if (wrapColumn > 0) {
        // Add 50% margin for overflow
        bufferSize = static_cast<qint32>(wrapColumn * 1.5);
        if (bufferSize < INITIAL_BUFFER_SIZE) {
            bufferSize = INITIAL_BUFFER_SIZE;
        }
    }

    // Reserve space in text buffer
    textBuffer.reserve(bufferSize);

    // Initialize text buffer to empty (with null terminator for C-string compatibility)
    textBuffer.push_back('\0');

    // Create initial Style with provided colors
    // The Style will be added to styleList when text is added
    // For now, just leave styleList empty - caller will add styles as needed
}

/**
 * Destructor
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * Automatic cleanup via RAII:
 * - std::vector<char> automatically cleans up text buffer
 * - std::unique_ptr automatically deletes Style objects
 * - std::shared_ptr in Style automatically manages Action lifetime
 */
Line::~Line()
{
    // Everything is automatically cleaned up by destructors
}
