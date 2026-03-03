/**
 * output_formatter.cpp - Output display formatting companion
 *
 * Implements note/color/hyperlink output to the line buffer.
 * Extracted from WorldDocument for composability.
 */

#include "output_formatter.h"
#include "../text/action.h"
#include "../text/line.h"
#include "color_utils.h"
#include "logging.h"
#include "world_document.h"

OutputFormatter::OutputFormatter(WorldDocument& doc) : m_doc(doc)
{
}

void OutputFormatter::note(const QString& text)
{
    // Use default note colors (or white on black if not set) - BGR format
    QRgb foreColor = m_doc.m_bNotesInRGB ? m_doc.m_iNoteColourFore : BGR(255, 255, 255);
    QRgb backColor = m_doc.m_bNotesInRGB ? m_doc.m_iNoteColourBack : BGR(0, 0, 0);

    colourNote(foreColor, backColor, text);
}

/**
 * colourNote - Display a colored note in the output window
 *
 * Prep: Required for world.ColourNote Lua API
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::ColourNote)
 *
 * Displays a note with specified foreground and background colors.
 * The text is added to the output buffer with the COMMENT flag,
 * then a newline is started to complete the note line.
 *
 * This method temporarily changes the current style to display the note,
 * then restores the previous style so MUD output continues normally.
 *
 * @param foreColor RGB foreground color (QRgb format)
 * @param backColor RGB background color (QRgb format)
 * @param text The text to display
 */
void OutputFormatter::colourNote(QRgb foreColor, QRgb backColor, const QString& text)
{
    // Don't output notes if disabled
    if (m_doc.m_bNotesNotWantedNow) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_doc.m_iFlags;
    QRgb savedFore = m_doc.m_iForeColour;
    QRgb savedBack = m_doc.m_iBackColour;

    // Set note style: RGB colors with note style flags
    m_doc.m_iFlags =
        COLOUR_RGB | m_doc.m_iNoteStyle; // Use configured note style (bold, underline, etc.)
    m_doc.m_iForeColour = foreColor;
    m_doc.m_iBackColour = backColor;

    // Split text on newlines and output each line separately
    // This matches original MUSHclient behavior for multi-line notes
    QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray utf8 = lines[i].toUtf8();
        m_doc.AddToLine(utf8.constData(), utf8.length());

        // Complete each line (hard break with COMMENT flag)
        m_doc.StartNewLine(true, COMMENT);
    }

    // Restore previous style for MUD output
    m_doc.m_iFlags = savedFlags;
    m_doc.m_iForeColour = savedFore;
    m_doc.m_iBackColour = savedBack;

    qCDebug(lcWorld) << "note:" << text;
}

/**
 * colourTell - Display colored text without newline
 *
 * Prep: Optional for world.ColourTell Lua API
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::ColourTell)
 *
 * Similar to colourNote() but does NOT add a newline after the text.
 * This allows building up a line with multiple colored segments.
 *
 * @param foreColor RGB foreground color (QRgb format)
 * @param backColor RGB background color (QRgb format)
 * @param text The text to display
 */
void OutputFormatter::colourTell(QRgb foreColor, QRgb backColor, const QString& text)
{
    // Don't output notes if disabled
    if (m_doc.m_bNotesNotWantedNow) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_doc.m_iFlags;
    QRgb savedFore = m_doc.m_iForeColour;
    QRgb savedBack = m_doc.m_iBackColour;

    // Set note style: RGB colors with note style flags
    m_doc.m_iFlags = COLOUR_RGB | m_doc.m_iNoteStyle;
    m_doc.m_iForeColour = foreColor;
    m_doc.m_iBackColour = backColor;

    // Handle embedded newlines but don't add final newline
    // This matches original MUSHclient behavior
    QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray utf8 = lines[i].toUtf8();
        m_doc.AddToLine(utf8.constData(), utf8.length());

        // Add newline for all but the last segment
        if (i < lines.size() - 1) {
            m_doc.StartNewLine(true, COMMENT);
        }
    }

    // Restore previous style (no final newline - caller must call note("") or StartNewLine)
    m_doc.m_iFlags = savedFlags;
    m_doc.m_iForeColour = savedFore;
    m_doc.m_iBackColour = savedBack;
}

/**
 * hyperlink - Display a clickable hyperlink in the output window
 *
 * Creates a clickable link that executes an action when clicked.
 *
 * @param action What to send/execute when clicked
 * @param text Display text (if empty, uses action)
 * @param hint Tooltip text
 * @param foreColor Foreground color
 * @param backColor Background color
 * @param isUrl True for URL hyperlinks (opens browser), false for send actions
 */
void OutputFormatter::hyperlink(const QString& action, const QString& text, const QString& hint,
                                QRgb foreColor, QRgb backColor, bool isUrl)
{
    // Don't output if disabled
    if (m_doc.m_bNotesNotWantedNow) {
        return;
    }

    // Don't do anything for empty action
    if (action.isEmpty()) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_doc.m_iFlags;
    QRgb savedFore = m_doc.m_iForeColour;
    QRgb savedBack = m_doc.m_iBackColour;
    auto savedAction = m_doc.m_currentAction;

    // Create Action for this hyperlink
    QString hintText = hint.isEmpty() ? action : hint;
    m_doc.m_currentAction = std::make_shared<Action>(action, hintText, QString(), &m_doc);

    // Set hyperlink style: RGB colors, underline, and action flag
    quint16 actionFlag = isUrl ? ACTION_HYPERLINK : ACTION_SEND;
    m_doc.m_iFlags = COLOUR_RGB | actionFlag;
    if (m_doc.m_bUnderlineHyperlinks) {
        m_doc.m_iFlags |= UNDERLINE;
    }
    m_doc.m_iForeColour = foreColor;
    m_doc.m_iBackColour = backColor;

    // Output the link text (use action if text is empty)
    QString displayText = text.isEmpty() ? action : text;
    QByteArray utf8 = displayText.toUtf8();
    m_doc.AddToLine(utf8.constData(), utf8.length());

    // Restore previous style and action
    m_doc.m_iFlags = savedFlags;
    m_doc.m_iForeColour = savedFore;
    m_doc.m_iBackColour = savedBack;
    m_doc.m_currentAction = savedAction;
}

/**
 * simulate - Process text as if received from MUD
 *
 * Processes text through the telnet state machine and trigger system
 * as if it came from the MUD connection. Useful for testing scripts.
 *
 * @param text Text to simulate (may contain ANSI codes)
 */
void OutputFormatter::simulate(const QString& text)
{
    // Set flag so triggers know we're simulating
    m_doc.m_bDoingSimulate = true;

    // Process each byte through the telnet state machine
    QByteArray utf8 = text.toUtf8();
    for (int i = 0; i < utf8.length(); i++) {
        m_doc.ProcessIncomingByte((unsigned char)utf8[i]);
    }

    m_doc.m_bDoingSimulate = false;

    // Check if we have an incomplete line (prompt)
    if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0) {
        emit m_doc.incompleteLine();
    }
}

/**
 * noteHr - Display horizontal rule
 *
 * Outputs a horizontal rule line in the output window.
 */
void OutputFormatter::noteHr()
{
    // Wrap up previous line if necessary
    if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0) {
        m_doc.StartNewLine(true, 0);
    }

    // Finish this line as a horizontal rule.
    // Pass HORIZ_RULE as iFlags — StartNewLine sets m_currentLine->flags = iFlags
    // before pushing the line, so setting flags beforehand would be overwritten.
    m_doc.StartNewLine(true, HORIZ_RULE);
}
