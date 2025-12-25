/**
 * world_recall.cpp - WorldDocument recall/buffer search methods
 *
 * Implements the Recall feature which searches through the output buffer
 * and returns matching lines for display in a notepad window.
 */

#include "../text/line.h"
#include "world_document.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QString>

/**
 * RecallText - Search buffer and return matching lines
 *
 * Based on: doc.cpp:RecallText() in original MUSHclient
 *
 * Searches through the output buffer for lines matching the search text.
 * Can filter by line type (output/commands/notes) and use regex matching.
 *
 * @param searchText Text or regex pattern to search for
 * @param matchCase If true, search is case-sensitive
 * @param useRegex If true, treat searchText as regex pattern
 * @param includeOutput If true, include normal MUD output lines
 * @param includeCommands If true, include user command lines
 * @param includeNotes If true, include script note/comment lines
 * @param lineCount Number of lines to search (0 = all lines)
 * @param linePreamble Optional timestamp format to prepend to each line
 * @return QString containing all matching lines (with preambles if specified)
 */
QString WorldDocument::RecallText(const QString& searchText, bool matchCase, bool useRegex,
                                  bool includeOutput, bool includeCommands, bool includeNotes,
                                  int lineCount, const QString& linePreamble)
{
    QString result;

    if (m_lineList.isEmpty()) {
        return result;
    }

    // Determine starting position
    int startIndex = 0;
    if (lineCount > 0 && lineCount < m_lineList.count()) {
        // Start from lineCount lines back
        startIndex = m_lineList.count() - lineCount;
    }

    // Setup case sensitivity
    Qt::CaseSensitivity cs = matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegularExpression::PatternOptions regexOptions =
        matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;

    // Compile regex if needed
    QRegularExpression regex;
    if (useRegex) {
        regex.setPattern(searchText);
        regex.setPatternOptions(regexOptions);
        if (!regex.isValid()) {
            // Invalid regex - return empty
            return result;
        }
    }

    // Prepare search string for non-regex mode
    QString searchTextLower = matchCase ? searchText : searchText.toLower();

    // Iterate through lines
    for (int i = startIndex; i < m_lineList.count(); i++) {
        Line* pLine = m_lineList[i];
        if (!pLine) {
            continue;
        }

        // Build complete line text (may span multiple Line objects if wrapped)
        QString lineText;
        QDateTime lineTime = pLine->m_theTime;
        int flags = pLine->flags;

        // Collect all text for this logical line (until hard_return)
        int j = i;
        while (j < m_lineList.count()) {
            Line* pCurrentLine = m_lineList[j];
            if (!pCurrentLine) {
                break;
            }

            lineText += QString::fromUtf8(pCurrentLine->text(), pCurrentLine->len());

            if (pCurrentLine->hard_return) {
                i = j; // Advance outer loop past this logical line
                break;
            }
            j++;
        }

        // Check if this line type should be included
        bool includeThisLine = false;
        if ((flags & USER_INPUT) && includeCommands) {
            includeThisLine = true;
        } else if ((flags & COMMENT) && includeNotes) {
            includeThisLine = true;
        } else if ((flags & NOTE_OR_COMMAND) == 0 && includeOutput) {
            // Not a command or note, so it's normal output
            includeThisLine = true;
        }

        if (!includeThisLine) {
            continue;
        }

        // Check if text matches
        bool matches = false;
        if (useRegex) {
            QRegularExpressionMatch match = regex.match(lineText);
            matches = match.hasMatch();
        } else {
            QString searchLine = matchCase ? lineText : lineText.toLower();
            matches = searchLine.contains(searchTextLower);
        }

        if (matches) {
            // Add preamble if specified
            if (!linePreamble.isEmpty()) {
                QString preamble = lineTime.toString(linePreamble);
                result += preamble;
                result += " ";
            }

            // Add the line
            result += lineText;
            result += "\n";
        }
    }

    return result;
}
