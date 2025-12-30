// world_logging.cpp
// Log File Open/Close
//
// Implements session logging functionality for Mushkin
// Port of: methods_logging.cpp from original MUSHclient
//
// Provides:
// - OpenLog(filename, append) - Open log file with auto-generated names
// - CloseLog() - Close log file and write postamble
// - WriteToLog(text) - Internal write function
// - WriteLog(message) - API write function (adds newline)
// - FlushLog() - Flush log to disk
// - IsLogOpen() - Check if log is open
// - FormatTime(dt, pattern, forHTML) - Expand time codes in strings

#include "../text/line.h"  // For Line and LOG_LINE flag
#include "../text/style.h" // For Style
#include "color_utils.h"
#include "logging.h"
#include "world_document.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>

// Error codes (defined in lua_methods.cpp)
enum {
    eOK = 0,
    eLogFileAlreadyOpen = 30001,
    eCouldNotOpenFile = 30002,
    eLogFileNotOpen = 30003,
    eLogFileBadWrite = 30004,
};

/**
 * FormatTime - Expand time format codes in a string
 *
 * Port of: methods_dates.cpp + doc.cpp (FormatTime from original MUSHclient)
 *
 * Replaces time formatting codes with actual values from the datetime:
 *
 * Standard time codes:
 * - %Y → 4-digit year (2025)
 * - %y → 2-digit year (25)
 * - %m → 2-digit month (01-12)
 * - %d → 2-digit day (01-31)
 * - %H → 2-digit hour 24h (00-23)
 * - %I → 2-digit hour 12h (01-12)
 * - %M → 2-digit minute (00-59)
 * - %S → 2-digit second (00-59)
 * - %p → AM/PM
 * - %a → Abbreviated weekday (Mon, Tue, etc.)
 * - %A → Full weekday (Monday, Tuesday, etc.)
 * - %b → Abbreviated month (Jan, Feb, etc.)
 * - %B → Full month (January, February, etc.)
 * - %% → Literal % character
 *
 * Custom MUSHclient codes:
 * - %E → Startup directory (working_dir)
 * - %N → World name (m_mush_name)
 * - %P → Player name (m_name)
 * - %F → Default world files directory
 * - %L → Default log files directory
 *
 * @param dt The datetime to format
 * @param pattern The pattern string with % codes
 * @param forHTML If true, escape HTML special characters in custom replacements
 * @return The formatted string
 */
QString WorldDocument::FormatTime(const QDateTime& dt, const QString& pattern, bool forHTML)
{
    if (!dt.isValid()) {
        return pattern; // Return unchanged if datetime invalid
    }

    QString result = pattern;

    // Custom MUSHclient codes (must be done before time codes to avoid double-replacement)
    // Use a temporary replacement to avoid % conflicts
    QString escapedPercent = "\x01PERCENT\x01";

    // %E becomes startup directory
    QString workingDir = QDir::currentPath();
    if (forHTML) {
        workingDir = FixHTMLString(workingDir);
    }
    result.replace("%E", workingDir.replace("%", escapedPercent));

    // %N becomes world name
    QString worldName = m_mush_name;
    if (forHTML) {
        worldName = FixHTMLString(worldName);
    }
    result.replace("%N", worldName.replace("%", escapedPercent));

    // %P becomes player name
    QString playerName = m_name;
    if (forHTML) {
        playerName = FixHTMLString(playerName);
    }
    result.replace("%P", playerName.replace("%", escapedPercent));

    // %F becomes default world files directory
    // TODO: Get from application settings when implemented
    QString worldFilesDir = QCoreApplication::applicationDirPath() + "/worlds";
    if (forHTML) {
        worldFilesDir = FixHTMLString(worldFilesDir);
    }
    result.replace("%F", worldFilesDir.replace("%", escapedPercent));

    // %L becomes default log files directory
    // TODO: Get from application settings when implemented
    QString logFilesDir = QCoreApplication::applicationDirPath() + "/logs";
    if (forHTML) {
        logFilesDir = FixHTMLString(logFilesDir);
    }
    result.replace("%L", logFilesDir.replace("%", escapedPercent));

    // Qt's toString() format codes differ from strftime, so we map them:
    result.replace("%Y", dt.toString("yyyy")); // 4-digit year
    result.replace("%y", dt.toString("yy"));   // 2-digit year
    result.replace("%m", dt.toString("MM"));   // 2-digit month
    result.replace("%d", dt.toString("dd"));   // 2-digit day
    result.replace("%H", dt.toString("HH"));   // 2-digit hour (24h)
    result.replace("%I", dt.toString("hh"));   // 2-digit hour (12h)
    result.replace("%M", dt.toString("mm"));   // 2-digit minute
    result.replace("%S", dt.toString("ss"));   // 2-digit second
    result.replace("%p", dt.toString("AP"));   // AM/PM
    result.replace("%a", dt.toString("ddd"));  // Abbreviated weekday
    result.replace("%A", dt.toString("dddd")); // Full weekday
    result.replace("%b", dt.toString("MMM"));  // Abbreviated month
    result.replace("%B", dt.toString("MMMM")); // Full month
    result.replace("%%", "%");                 // Literal %

    // Restore escaped percents
    result.replace(escapedPercent, "%%");

    return result;
}

/**
 * OpenLog - Open a log file for writing
 *
 * Port of: methods_logging.cpp (OpenLog from original MUSHclient)
 *
 * Opens a log file for session logging. If no filename is provided, uses
 * m_strAutoLogFileName with time substitution to generate a unique name.
 *
 * Acceptance criteria:
 * - Return eLogFileAlreadyOpen if already open
 * - If filename empty, use m_strAutoLogFileName with time substitution
 * - Support time formatting: %Y, %m, %d, %H, %M, %S
 * - Open in append mode if append=true, overwrite if false
 * - Write file preamble if not raw mode
 * - Expand time codes and %n (newline) in preamble
 * - Return eCouldNotOpenFile if open fails
 * - Return eOK on success
 *
 * @param filename Log file path (empty = use auto-generated name)
 * @param append True to append to existing file, false to overwrite
 * @return Error code (eOK, eLogFileAlreadyOpen, eCouldNotOpenFile)
 */
qint32 WorldDocument::OpenLog(const QString& filename, bool append)
{
    // Check if already open
    if (m_logfile) {
        return eLogFileAlreadyOpen;
    }

    // Determine filename
    QString logName = filename;
    if (logName.isEmpty()) {
        // Use auto-generated filename with time substitution
        QDateTime now = QDateTime::currentDateTime();
        logName = FormatTime(now, m_strAutoLogFileName, false);
    }

    // Filename still empty? Error
    if (logName.isEmpty()) {
        return eCouldNotOpenFile;
    }

    // Store resolved filename
    m_logfile_name = logName;

    // Create QFile
    m_logfile = new QFile(logName);

    // Set open mode
    QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Text;
    if (append) {
        mode |= QIODevice::Append;
    } else {
        mode |= QIODevice::Truncate;
    }

    // Try to open
    if (!m_logfile->open(mode)) {
        qCDebug(lcLogging) << "OpenLog: Failed to open" << logName << ":"
                           << m_logfile->errorString();
        delete m_logfile;
        m_logfile = nullptr;
        return eCouldNotOpenFile;
    }

    qCDebug(lcLogging) << "OpenLog: Successfully opened" << logName << "(append=" << append << ")";

    // Write file preamble if not in raw mode
    if (!m_strLogFilePreamble.isEmpty() && !m_bLogRaw) {
        QDateTime now = QDateTime::currentDateTime();
        QString preamble = m_strLogFilePreamble;

        // Expand %n to newline
        preamble.replace("%n", "\n");

        // Expand time codes
        preamble = FormatTime(now, preamble, m_bLogHTML);

        // Write preamble
        WriteToLog(preamble);
        WriteToLog("\n");
    }

    // Initialize flush time
    m_LastFlushTime = QDateTime::currentDateTime();

    // Retrospective Logging
    // Write all existing lines with LOG_LINE flag to the log
    writeRetrospectiveLog();

    return eOK;
}

/**
 * CloseLog - Close the log file
 *
 * Port of: methods_logging.cpp (CloseLog from original MUSHclient)
 *
 * Closes the currently open log file, writing the postamble first.
 *
 * Acceptance criteria:
 * - Return eLogFileNotOpen if not open
 * - Write file postamble if not raw mode
 * - Expand time codes and %n in postamble
 * - Close file handle and set to null
 * - Return eOK on success
 *
 * @return Error code (eOK, eLogFileNotOpen)
 */
qint32 WorldDocument::CloseLog()
{
    // Check if open
    if (!m_logfile) {
        return eLogFileNotOpen;
    }

    qCDebug(lcLogging) << "CloseLog: Closing log file" << m_logfile_name;

    // Write file postamble if not in raw mode
    if (!m_strLogFilePostamble.isEmpty() && !m_bLogRaw) {
        QDateTime now = QDateTime::currentDateTime();
        QString postamble = m_strLogFilePostamble;

        // Expand %n to newline
        postamble.replace("%n", "\n");

        // Expand time codes
        postamble = FormatTime(now, postamble, m_bLogHTML);

        // Write postamble
        WriteToLog(postamble);
        WriteToLog("\n");
    }

    // Close and delete
    m_logfile->close();
    delete m_logfile;
    m_logfile = nullptr;

    qCDebug(lcLogging) << "CloseLog: Log file closed";

    return eOK;
}

/**
 * WriteToLog - Internal function to write text to log
 *
 * Port of: methods_logging.cpp (WriteToLog from original MUSHclient)
 *
 * Writes raw text to the log file. This is an internal function used by
 * other logging functions. Does not add newlines automatically.
 *
 * Acceptance criteria:
 * - Return early if m_logfile is null
 * - Write text using QTextStream with UTF-8 encoding
 * - No automatic newline
 *
 * @param text The text to write
 */
void WorldDocument::WriteToLog(const QString& text)
{
    // Early return if not logging
    if (!m_logfile || !m_logfile->isOpen()) {
        return;
    }

    // Write using QTextStream for proper encoding
    QTextStream stream(m_logfile);
    stream.setEncoding(QStringConverter::Utf8);
    stream << text;
}

/**
 * WriteLog - API function to write a message to the log
 *
 * Port of: methods_logging.cpp (WriteLog from original MUSHclient)
 *
 * Writes a message to the log file, ensuring it ends with a newline.
 * This is the public API function exposed to Lua scripts.
 *
 * Acceptance criteria:
 * - Return eLogFileNotOpen if not open
 * - Check if message ends with newline, append \n if not
 * - Return eLogFileBadWrite if write size != expected size
 * - Return eOK on success
 *
 * @param message The message to write
 * @return Error code (eOK, eLogFileNotOpen, eLogFileBadWrite)
 */
qint32 WorldDocument::WriteLog(const QString& message)
{
    // Check if open
    if (!m_logfile) {
        return eLogFileNotOpen;
    }

    // Ensure message ends with newline
    QString msg = message;
    if (!msg.endsWith("\n")) {
        msg += "\n";
    }

    // Convert to UTF-8 for writing
    QByteArray utf8 = msg.toUtf8();
    qint64 bytesToWrite = utf8.size();
    qint64 bytesWritten = m_logfile->write(utf8);

    // Check if write was successful
    if (bytesWritten != bytesToWrite) {
        qCDebug(lcLogging) << "WriteLog: Write failed - expected" << bytesToWrite << "bytes, wrote"
                           << bytesWritten;
        return eLogFileBadWrite;
    }

    return eOK;
}

/**
 * FlushLog - Flush log file to disk
 *
 * Port of: methods_logging.cpp (FlushLog from original MUSHclient)
 *
 * Forces the log file to be written to disk. Useful to prevent data loss
 * in case of crashes.
 *
 * Acceptance criteria:
 * - Return eLogFileNotOpen if not open
 * - Call QFile::flush() to sync to disk
 * - Return eOK on success
 *
 * @return Error code (eOK, eLogFileNotOpen)
 */
qint32 WorldDocument::FlushLog()
{
    // Check if open
    if (!m_logfile) {
        return eLogFileNotOpen;
    }

    // Flush to disk
    m_logfile->flush();

    return eOK;
}

/**
 * IsLogOpen - Check if log file is open
 *
 * Port of: methods_logging.cpp (IsLogOpen from original MUSHclient)
 *
 * Returns true if a log file is currently open.
 *
 * Acceptance criteria:
 * - Return true if m_logfile != null and is open
 * - Return false otherwise
 *
 * @return True if log is open
 */
bool WorldDocument::IsLogOpen() const
{
    return m_logfile != nullptr && m_logfile->isOpen();
}

/**
 * FixHTMLString - Escape HTML special characters
 *
 * Port of: Utilities.cpp (FixHTMLString from original MUSHclient)
 *
 * Replaces HTML special characters with their entity equivalents:
 * - & → &amp; (MUST be first to avoid double-escaping)
 * - < → &lt;
 * - > → &gt;
 * - " → &quot;
 *
 * Used when logging in HTML mode to prevent MUD text from breaking HTML formatting.
 *
 * @param text The text to escape
 * @return Text with HTML entities replaced
 */
QString WorldDocument::FixHTMLString(const QString& text)
{
    QString result = text;

    // Replace & first to avoid double-escaping
    // Example: if we did < first, "a<b" → "a&lt;b" → "a&amp;lt;b" (wrong!)
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");

    return result;
}

/**
 * LogLineInHTMLcolour - Write styled line as HTML with color codes
 *
 * Port of: ProcessPreviousLine.cpp (LogLineInHTMLcolour from original MUSHclient)
 *
 * Converts a line's style runs into HTML with <font> tags for foreground color
 * and <span> tags for background color (only when non-black). Tracks color
 * changes to minimize tag overhead.
 *
 * HTML Structure:
 * - <font color="#RRGGBB"> for foreground
 * - <span style="color:#RRGGBB;background:#RRGGBB"> for background (only if not black)
 * - <u>...</u> for underline
 * - Newline at end of line
 *
 * @param line The line to write as HTML
 */
void WorldDocument::LogLineInHTMLcolour(Line* line)
{
    if (!line || !m_logfile) {
        return;
    }

    QRgb prevcolour = 0xFFFFFFFF; // NO_COLOUR sentinel
    bool bInSpan = false;
    QRgb lastforecolour = 0;
    QRgb lastbackcolour = 0;

    // If line has no styles, write as plain text
    if (line->styleList.empty()) {
        QString lineText = QString::fromUtf8(line->text(), line->len());
        WriteToLog(FixHTMLString(lineText));
        WriteToLog("\n");
        return;
    }

    // Process all style runs in the line
    qint32 iCol = 0; // Column position in line
    QString lineText = QString::fromUtf8(line->text(), line->len());

    for (const auto& pStyle : line->styleList) {
        // Skip zero-length styles
        if (pStyle->iLength <= 0) {
            continue;
        }

        // Get RGB colors for this style
        QRgb colour1, colour2;
        GetStyleRGB(pStyle.get(), colour1, colour2);

        // Color change detected - close old tags and open new ones
        if (colour1 != lastforecolour || colour2 != lastbackcolour) {
            // Close previous span if open
            if (bInSpan) {
                WriteToLog("</span>");
                bInSpan = false;
            }

            // Close previous font color if set
            if (prevcolour != 0xFFFFFFFF) {
                WriteToLog("</font>");
            }

            // Open new font color - convert from BGR to RGB for HTML
            QColor fore = bgrToQColor(colour1);
            WriteToLog(QString("<font color=\"#%1%2%3\">")
                           .arg(fore.red(), 2, 16, QChar('0'))
                           .arg(fore.green(), 2, 16, QChar('0'))
                           .arg(fore.blue(), 2, 16, QChar('0')));
            prevcolour = colour1;

            // Open span for background color (only if not black)
            if (colour2 != 0) {
                QColor back = bgrToQColor(colour2);
                WriteToLog(QString("<span style=\"color:#%1%2%3;background:#%4%5%6\">")
                               .arg(fore.red(), 2, 16, QChar('0'))
                               .arg(fore.green(), 2, 16, QChar('0'))
                               .arg(fore.blue(), 2, 16, QChar('0'))
                               .arg(back.red(), 2, 16, QChar('0'))
                               .arg(back.green(), 2, 16, QChar('0'))
                               .arg(back.blue(), 2, 16, QChar('0')));
                bInSpan = true;
            }

            lastforecolour = colour1;
            lastbackcolour = colour2;
        }

        // Open underline tag if needed
        if (pStyle->iFlags & UNDERLINE) {
            WriteToLog("<u>");
        }

        // Write the text (with HTML escaping)
        QString segment = lineText.mid(iCol, pStyle->iLength);
        WriteToLog(FixHTMLString(segment));

        // Close underline tag if needed
        if (pStyle->iFlags & UNDERLINE) {
            WriteToLog("</u>");
        }

        iCol += pStyle->iLength;
    }

    // Write newline at end of line
    WriteToLog("\n");

    // Close open tags at end of line
    if (bInSpan) {
        WriteToLog("</span>");
    }
    if (prevcolour != 0xFFFFFFFF) {
        WriteToLog("</font>");
    }
}

/**
 * logCompletedLine - Log a completed line to the log file
 *
 * Port of: ProcessPreviousLine.cpp (line logging logic)
 *
 * Implements selective logging based on line type:
 * - COMMENT flag + m_bLogNotes → log notes
 * - USER_INPUT flag + m_log_input → log user input
 * - Otherwise + m_bLogOutput → log MUD output
 *
 * Sets LOG_LINE flag for retrospective logging.
 * Writes line with appropriate preamble/postamble if log file is open.
 *
 * @param line The completed line to log
 */
void WorldDocument::logCompletedLine(Line* line)
{
    if (!line) {
        return;
    }

    unsigned char flags = line->flags;
    bool bShouldLog = false;
    QString preamble, postamble;

    // Determine if this line should be logged based on type
    if ((flags & COMMENT) && m_bLogNotes) {
        bShouldLog = true;
        preamble = m_strLogLinePreambleNotes;
        postamble = m_strLogLinePostambleNotes;
    } else if ((flags & USER_INPUT) && m_log_input) {
        bShouldLog = true;
        preamble = m_strLogLinePreambleInput;
        postamble = m_strLogLinePostambleInput;
    } else if (!(flags & NOTE_OR_COMMAND) && m_bLogOutput) {
        // Regular MUD output (not notes or user input)
        bShouldLog = true;
        preamble = m_strLogLinePreambleOutput;
        postamble = m_strLogLinePostambleOutput;
    }

    // Check if a trigger wants to omit this line from log
    if (m_bOmitCurrentLineFromLog) {
        bShouldLog = false;
    }

    // Set LOG_LINE flag for retrospective logging
    if (bShouldLog) {
        line->flags |= LOG_LINE;
    }

    // Write to log file if open and not raw mode
    if (bShouldLog && m_logfile && !m_bLogRaw) {
        // Expand %n to newline in preamble
        preamble.replace("%n", "\n");

        // Expand time codes if preamble contains %
        if (preamble.contains('%')) {
            preamble = FormatTime(line->m_theTime, preamble, m_bLogHTML);
        }

        // Expand %n to newline in postamble
        postamble.replace("%n", "\n");

        // Expand time codes if postamble contains %
        if (postamble.contains('%')) {
            postamble = FormatTime(line->m_theTime, postamble, m_bLogHTML);
        }

        // Write preamble
        WriteToLog(preamble);

        // Write line content
        QString lineText = QString::fromUtf8(line->text(), line->len());

        if (m_bLogHTML && m_bLogInColour) {
            // HTML logging with colors
            LogLineInHTMLcolour(line);
        } else if (m_bLogHTML) {
            // HTML logging without colors
            WriteToLog(FixHTMLString(lineText));
        } else {
            // Plain text logging
            WriteToLog(lineText);
        }

        // Write postamble
        WriteToLog(postamble);

        // Write newline (unless HTML color mode already added one)
        if (!(m_bLogHTML && m_bLogInColour)) {
            WriteToLog("\n");
        }
    } else if (bShouldLog && m_logfile && m_bLogRaw) {
        // Raw mode: just write the line as-is
        QString lineText = QString::fromUtf8(line->text(), line->len());
        WriteToLog(lineText);
        WriteToLog("\n");
    }
}

/**
 * writeRetrospectiveLog - Write all buffered lines with LOG_LINE flag to log
 *
 * Port of: Implied by ProcessPreviousLine.cpp LOG_LINE flag system
 *
 * Retrospective Logging
 *
 * When opening a log file mid-session, this function writes all lines that
 * have been marked with the LOG_LINE flag. This allows users to capture
 * session history that occurred before the log was opened.
 *
 * Acceptance criteria:
 * - Iterate through m_lineList from head to tail (chronological order)
 * - For each line with LOG_LINE flag set, write to log
 * - Respect line type: use appropriate preamble/postamble (output/input/notes)
 * - Use line's timestamp (m_theTime) for time formatting
 * - Format as HTML if m_bLogHTML, with color if m_bLogInColour
 * - Write in raw mode if m_bLogRaw
 * - Don't write lines without LOG_LINE flag (trigger-omitted lines)
 */
void WorldDocument::writeRetrospectiveLog()
{
    if (!m_logfile || m_lineList.isEmpty()) {
        return;
    }

    qCDebug(lcLogging) << "writeRetrospectiveLog: Writing" << m_lineList.size() << "buffered lines";

    int linesWritten = 0;

    // Iterate through all buffered lines in chronological order
    for (Line* line : m_lineList) {
        if (!line) {
            continue;
        }

        // Only write lines that have been marked for logging
        if (!(line->flags & LOG_LINE)) {
            continue;
        }

        // Determine line type and select appropriate preamble/postamble
        QString preamble, postamble;
        unsigned char flags = line->flags;

        if (flags & COMMENT) {
            preamble = m_strLogLinePreambleNotes;
            postamble = m_strLogLinePostambleNotes;
        } else if (flags & USER_INPUT) {
            preamble = m_strLogLinePreambleInput;
            postamble = m_strLogLinePostambleInput;
        } else {
            // Regular MUD output
            preamble = m_strLogLinePreambleOutput;
            postamble = m_strLogLinePostambleOutput;
        }

        // Write the line (respecting raw/HTML modes)
        if (m_bLogRaw) {
            // Raw mode: just write the line as-is
            QString lineText = QString::fromUtf8(line->text(), line->len());
            WriteToLog(lineText);
            WriteToLog("\n");
        } else {
            // Formatted mode: expand preambles/postambles and handle HTML
            preamble.replace("%n", "\n");
            if (preamble.contains('%')) {
                preamble = FormatTime(line->m_theTime, preamble, m_bLogHTML);
            }

            postamble.replace("%n", "\n");
            if (postamble.contains('%')) {
                postamble = FormatTime(line->m_theTime, postamble, m_bLogHTML);
            }

            WriteToLog(preamble);

            QString lineText = QString::fromUtf8(line->text(), line->len());

            if (m_bLogHTML && m_bLogInColour) {
                // HTML logging with colors
                LogLineInHTMLcolour(line);
            } else if (m_bLogHTML) {
                // HTML logging without colors
                WriteToLog(FixHTMLString(lineText));
            } else {
                // Plain text logging
                WriteToLog(lineText);
            }

            WriteToLog(postamble);

            // Write newline (unless HTML color mode already added one)
            if (!(m_bLogHTML && m_bLogInColour)) {
                WriteToLog("\n");
            }
        }

        linesWritten++;
    }

    qCDebug(lcLogging) << "writeRetrospectiveLog: Wrote" << linesWritten << "lines to log";
}
