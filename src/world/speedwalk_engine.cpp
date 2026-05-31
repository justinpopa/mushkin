/**
 * speedwalk_engine.cpp - Speedwalk parsing and manipulation
 *
 * Pure string-transformation functions for MUSHclient speedwalk notation.
 * Extracted from WorldDocument for composability.
 */

#include "speedwalk_engine.h"

#include <QMap>

namespace speedwalk {

/**
 * DoEvaluateSpeedwalk - Parse speedwalk notation and expand to commands
 *
 * Speed Walking
 *
 * Parses speedwalk notation like "3n2w" and expands to individual direction commands.
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * Format:
 * - [count]direction: e.g., "3n" = north;north;north (count 1-99)
 * - Directions: N, S, E, W, U, D (case insensitive)
 * - Actions: O=open, C=close, L=lock, K=unlock (must be followed by direction)
 * - Filler: F = spam prevention filler (m_speedwalk.filler)
 * - Custom: (text/reverse) = custom direction (only "text" is used)
 * - Comments: {comment} = ignored
 *
 * Examples:
 * - "3n2w" → "north\nnorth\nnorth\nwest\nwest\n"
 * - "On3e" → "open north\neast\neast\neast\n"
 * - "2(ne/sw)" → "ne\nne\n"
 *
 * @param speedWalkString - Speedwalk notation string
 * @param filler - Spam prevention filler string (replaces F direction code)
 * @return Expanded commands separated by \n, or error string starting with "*"
 */
QString evaluate(const QString& speedWalkString, const QString& filler)
{
    // Initialize default direction map
    static QMap<QChar, DirectionInfo> directionMap;
    if (directionMap.isEmpty()) {
        directionMap['n'] = DirectionInfo("north", "s");
        directionMap['s'] = DirectionInfo("south", "n");
        directionMap['e'] = DirectionInfo("east", "w");
        directionMap['w'] = DirectionInfo("west", "e");
        directionMap['u'] = DirectionInfo("up", "d");
        directionMap['d'] = DirectionInfo("down", "u");
    }

    QString result;
    QString str;
    int count;
    const QChar* p = speedWalkString.constData();
    const QChar* end = p + speedWalkString.length();

    while (p < end) {
        // Bypass spaces
        while (p < end && p->isSpace())
            p++;

        if (p >= end)
            break;

        // Bypass comments {comment}
        if (*p == '{') {
            while (p < end && *p != '}')
                p++;

            if (p >= end || *p != '}')
                return QString("*Comment code of '{' not terminated by a '}'");
            p++; // skip } symbol
            continue;
        }

        // Get counter, if any
        count = 0;
        while (p < end && p->isDigit()) {
            count = (count * 10) + (p->unicode() - '0');
            p++;
            if (count > 99)
                return QString("*Speed walk counter exceeds 99");
        }

        // No counter, assume do once
        if (count == 0)
            count = 1;

        // Bypass spaces after counter
        while (p < end && p->isSpace())
            p++;

        if (count > 1 && p >= end)
            return QString("*Speed walk counter not followed by an action");

        if (count > 1 && *p == '{')
            return QString("*Speed walk counter may not be followed by a comment");

        // Might have had trailing space
        if (p >= end)
            break;

        // Check for action codes (C, O, L, K)
        if (p->toUpper() == 'C' || p->toUpper() == 'O' || p->toUpper() == 'L' ||
            p->toUpper() == 'K') {
            if (count > 1)
                return QString(
                    "*Action code of C, O, L or K must not follow a speed walk count (1-99)");

            switch (p->toUpper().unicode()) {
                case 'C':
                    result += "close ";
                    break;
                case 'O':
                    result += "open ";
                    break;
                case 'L':
                    result += "lock ";
                    break;
                case 'K':
                    result += "unlock ";
                    break;
            }
            p++;

            // Bypass spaces after open/close/lock/unlock
            while (p < end && p->isSpace())
                p++;

            if (p >= end || p->toUpper() == 'F' || *p == '{')
                return QString("*Action code of C, O, L or K must be followed by a direction");
        }

        // Work out which direction we are going
        if (p >= end)
            break;

        QChar dir = p->toUpper();
        switch (dir.unicode()) {
            case 'N':
            case 'S':
            case 'E':
            case 'W':
            case 'U':
            case 'D':
                // Look up the direction to send
                str = directionMap[p->toLower()].m_sDirectionToSend;
                break;

            case 'F':
                str = filler;
                break;

            case '(': // special string (e.g., (ne/sw))
            {
                str.clear();
                p++; // skip '('
                while (p < end && *p != ')')
                    str += *p++;

                if (p >= end || *p != ')')
                    return QString("*Action code of '(' not terminated by a ')'");

                int iSlash = str.indexOf("/");
                if (iSlash != -1)
                    str = str.left(iSlash); // only use up to the slash
            } break;

            default:
                // Original: MakeSpeedWalkErrorString(TFormat("*Invalid direction..."))
                // MakeSpeedWalkErrorString prepends "*", and the format string itself starts
                // with "*", so the result is "**Invalid direction..."
                return QString("**Invalid direction '%1' in speed walk, must be N, S, E, W, U, D, "
                               "F, or (something)")
                    .arg(dir);
        }

        p++; // bypass whatever that character was (or the trailing bracket)

        // Output required number of times (original uses \r\n — ENDLINE)
        for (int j = 0; j < count; j++)
            result += str + "\r\n";
    }

    return result;
}

/**
 * DoReverseSpeedwalk - Reverse a speedwalk string
 *
 * Takes a speedwalk like "3noe" and reverses it to "cw3s"
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * @param speedWalkString - Speedwalk notation string
 * @return Reversed speedwalk, or error string starting with "*"
 */
QString reverse(const QString& speedWalkString)
{
    // Initialize direction map with reverses
    static QMap<QChar, DirectionInfo> directionMap;
    if (directionMap.isEmpty()) {
        directionMap['n'] = DirectionInfo("north", "s");
        directionMap['s'] = DirectionInfo("south", "n");
        directionMap['e'] = DirectionInfo("east", "w");
        directionMap['w'] = DirectionInfo("west", "e");
        directionMap['u'] = DirectionInfo("up", "d");
        directionMap['d'] = DirectionInfo("down", "u");
        directionMap['f'] = DirectionInfo("f", "f"); // filler stays same
    }

    QString result;
    QString str;
    QString strAction;
    int count;
    const QChar* p = speedWalkString.constData();
    const QChar* end = p + speedWalkString.length();

    while (p < end) {
        // Preserve spaces (prepend to result since we're reversing)
        while (p < end && p->isSpace()) {
            if (*p == '\r') {
                // discard carriage returns
            } else if (*p == '\n') {
                result = "\r\n" + result;
            } else {
                result = *p + result;
            }
            p++;
        }

        if (p >= end)
            break;

        // Preserve comments {comment}
        if (*p == '{') {
            str.clear();
            while (p < end && *p != '}')
                str += *p++;

            if (p >= end || *p != '}')
                return QString("*Comment code of '{' not terminated by a '}'");

            p++; // skip } symbol
            str += "}";
            result = str + result;
            continue;
        }

        // Get counter, if any
        count = 0;
        while (p < end && p->isDigit()) {
            count = (count * 10) + (p->unicode() - '0');
            p++;
            if (count > 99)
                return QString("*Speed walk counter exceeds 99");
        }

        if (count == 0)
            count = 1;

        // Bypass spaces after counter
        while (p < end && p->isSpace())
            p++;

        if (count > 1 && p >= end)
            return QString("*Speed walk counter not followed by an action");

        if (count > 1 && *p == '{')
            return QString("*Speed walk counter may not be followed by a comment");

        if (p >= end)
            break;

        // Check for action codes (C, O, L, K)
        strAction.clear();
        if (p->toUpper() == 'C' || p->toUpper() == 'O' || p->toUpper() == 'L' ||
            p->toUpper() == 'K') {
            if (count > 1)
                return QString(
                    "*Action code of C, O, L or K must not follow a speed walk count (1-99)");

            strAction = *p;
            p++;

            while (p < end && p->isSpace())
                p++;

            if (p >= end || p->toUpper() == 'F' || *p == '{')
                return QString("*Action code of C, O, L or K must be followed by a direction");
        }

        if (p >= end)
            break;

        // Work out which direction and get reverse
        QChar dir = p->toLower();
        switch (p->toUpper().unicode()) {
            case 'N':
            case 'S':
            case 'E':
            case 'W':
            case 'U':
            case 'D':
            case 'F':
                str = directionMap[dir].m_sReverseDirection;
                break;

            case '(': // special string (e.g., (ne/sw))
            {
                // Static map for diagonal direction reverses (ne<->sw, nw<->se)
                // All directions from original MapDirectionsMap
                // (original: methods_speedwalks.cpp:279-283 uses MapDirectionsMap)
                static QMap<QString, QString> directionReverseMap;
                if (directionReverseMap.isEmpty()) {
                    // Mirror original MapDirectionsMap (Mapping.cpp:23-43).
                    // Abbreviations (n/s/e/w/u/d) share the same CMapDirection as their
                    // full-name entries, so m_sReverseDirection is always the abbreviation.
                    // Full names like "north" map to reverse "s" (not "south").
                    // Only the entries present in the original are included here —
                    // "northeast/northwest/southeast/southwest" are absent from
                    // the original MapDirectionsMap and are intentionally omitted.
                    directionReverseMap["n"] = "s";
                    directionReverseMap["s"] = "n";
                    directionReverseMap["e"] = "w";
                    directionReverseMap["w"] = "e";
                    directionReverseMap["u"] = "d";
                    directionReverseMap["d"] = "u";
                    directionReverseMap["ne"] = "sw";
                    directionReverseMap["sw"] = "ne";
                    directionReverseMap["nw"] = "se";
                    directionReverseMap["se"] = "nw";
                    // Full names map to abbreviation reverses (same CMapDirection as abbrev)
                    directionReverseMap["north"] = "s";
                    directionReverseMap["south"] = "n";
                    directionReverseMap["east"] = "w";
                    directionReverseMap["west"] = "e";
                    directionReverseMap["up"] = "d";
                    directionReverseMap["down"] = "u";
                }

                str.clear();
                p++; // skip '('
                while (p < end && *p != ')')
                    str += p->toLower(), p++;

                if (p >= end || *p != ')')
                    return QString("*Action code of '(' not terminated by a ')'");

                int iSlash = str.indexOf("/");
                if (iSlash == -1) {
                    // No slash - try to look up reverse (for simple directions like "ne")
                    if (directionReverseMap.contains(str))
                        str = directionReverseMap[str];
                    // Otherwise keep as-is
                } else {
                    // Swap left and right parts
                    QString left = str.left(iSlash);
                    QString right = str.mid(iSlash + 1);
                    str = right + "/" + left;
                }
                str = "(" + str + ")";
            } break;

            default:
                return QString("*Invalid direction '%1' in speed walk, must be N, S, E, W, U, D, "
                               "F, or (something)")
                    .arg(*p);
        }

        p++; // bypass direction char or closing bracket

        // Output it (prepend since reversing)
        if (count > 1)
            result = QString("%1%2%3").arg(count).arg(strAction).arg(str) + result;
        else
            result = strAction + str + result;
    }

    return result;
}

/**
 * RemoveBacktracks - Remove redundant back-and-forth movements from speedwalk
 *
 * Takes a speedwalk and removes movements that cancel each other out.
 * E.g., "nsen" becomes "2n" (the s-e cancel out... wait no, ns would cancel)
 * Actually: "nsew" - n and s cancel, e and w cancel = empty
 *
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * @param speedWalkString - Speedwalk notation string
 * @param filler - Spam prevention filler string (passed through to evaluate())
 * @return Optimized speedwalk with backtracks removed
 */
QString removeBacktracks(const QString& speedWalkString, const QString& filler)
{
    // Initialize direction map with reverses matching original MapDirectionsMap
    // (Mapping.cpp:23-43). Full names share CMapDirection with abbreviations,
    // so their m_sReverseDirection is an abbreviation (e.g. "north" -> "s").
    // "northeast/northwest/southeast/southwest" are absent from the original
    // and intentionally omitted.
    static QMap<QString, QString> reverseMap;
    if (reverseMap.isEmpty()) {
        reverseMap["n"] = "s";
        reverseMap["s"] = "n";
        reverseMap["e"] = "w";
        reverseMap["w"] = "e";
        reverseMap["u"] = "d";
        reverseMap["d"] = "u";
        reverseMap["ne"] = "sw";
        reverseMap["sw"] = "ne";
        reverseMap["nw"] = "se";
        reverseMap["se"] = "nw";
        reverseMap["north"] = "s";
        reverseMap["south"] = "n";
        reverseMap["east"] = "w";
        reverseMap["west"] = "e";
        reverseMap["up"] = "d";
        reverseMap["down"] = "u";
    }

    // First expand the speedwalk
    QString expanded = evaluate(speedWalkString, filler);

    // If error or empty, return as-is
    if (expanded.isEmpty() || expanded.startsWith('*'))
        return expanded;

    // Split into individual directions (normalize \r\n to \n first)
    expanded.replace("\r\n", "\n");
    QStringList directions = expanded.split('\n', Qt::SkipEmptyParts);

    // Use a stack approach - push directions, pop when reverse found
    QList<QString> stack;

    for (const QString& dir : directions) {
        QString trimmed = dir.trimmed().toLower();
        if (trimmed.isEmpty())
            continue;

        // Convert full direction to single letter for comparison
        // Normalize full direction names to short codes (original: MapDirectionsMap lookup)
        QString normalized = trimmed;
        if (trimmed == "north")
            normalized = "n";
        else if (trimmed == "south")
            normalized = "s";
        else if (trimmed == "east")
            normalized = "e";
        else if (trimmed == "west")
            normalized = "w";
        else if (trimmed == "up")
            normalized = "u";
        else if (trimmed == "down")
            normalized = "d";
        else if (trimmed == "northeast")
            normalized = "ne";
        else if (trimmed == "southwest")
            normalized = "sw";
        else if (trimmed == "northwest")
            normalized = "nw";
        else if (trimmed == "southeast")
            normalized = "se";

        if (stack.isEmpty()) {
            stack.append(normalized);
        } else {
            QString top = stack.last();
            // Check if this direction is the reverse of the top
            if (reverseMap.contains(top) && reverseMap[top] == normalized) {
                stack.removeLast(); // Cancel out
            } else {
                stack.append(normalized);
            }
        }
    }

    // Empty result
    if (stack.isEmpty())
        return QString();

    // Compress consecutive identical directions
    QString result;
    QString prev;
    int count = 0;

    for (const QString& dir : stack) {
        if (dir.isEmpty())
            continue;

        QString formatted = dir;
        // Multi-char directions need brackets
        if (dir.length() > 1)
            formatted = "(" + dir + ")";

        if (formatted == prev && count < 99) {
            count++;
        } else {
            // Output previous
            if (!prev.isEmpty()) {
                if (count > 1)
                    result += QString("%1%2 ").arg(count).arg(prev);
                else
                    result += prev + " ";
            }
            prev = formatted;
            count = 1;
        }
    }

    // Output final
    if (!prev.isEmpty()) {
        if (count > 1)
            result += QString("%1%2 ").arg(count).arg(prev);
        else
            result += prev + " ";
    }

    // Original preserves trailing space — don't trim
    return result;
}

} // namespace speedwalk
