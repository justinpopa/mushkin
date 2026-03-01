/**
 * speedwalk_engine.h - Speedwalk parsing and manipulation
 *
 * Pure string-transformation functions for MUSHclient speedwalk notation.
 * Extracted from WorldDocument for composability.
 */

#ifndef SPEEDWALK_ENGINE_H
#define SPEEDWALK_ENGINE_H

#include <QString>

/**
 * DirectionInfo - Information about a speedwalk direction
 *
 * Based on original MUSHclient MapDirectionsMap in mapping.cpp
 */
struct DirectionInfo {
    QString m_sDirectionToSend;  // Command to send (e.g., "north")
    QString m_sReverseDirection; // Reverse direction (e.g., for "n" it's "s")

    DirectionInfo(const QString& toSend = "", const QString& reverse = "")
        : m_sDirectionToSend(toSend), m_sReverseDirection(reverse)
    {
    }
};

namespace speedwalk {

/**
 * Parse speedwalk notation and expand to commands.
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
 * - Filler: F = spam prevention filler (passed as parameter)
 * - Custom: (text/reverse) = custom direction (only "text" is used)
 * - Comments: {comment} = ignored
 *
 * Examples:
 * - "3n2w" -> "north\nnorth\nnorth\nwest\nwest\n"
 * - "On3e" -> "open north\neast\neast\neast\n"
 * - "2(ne/sw)" -> "ne\nne\n"
 *
 * @param speedWalkString - Speedwalk notation string
 * @param filler - Spam prevention filler string (replaces F direction code)
 * @return Expanded commands separated by \n, or error string starting with "*"
 */
QString evaluate(const QString& speedWalkString, const QString& filler);

/**
 * Reverse a speedwalk string.
 *
 * Takes a speedwalk like "3noe" and reverses it to "cw3s"
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * @param speedWalkString - Speedwalk notation string
 * @return Reversed speedwalk, or error string starting with "*"
 */
QString reverse(const QString& speedWalkString);

/**
 * Remove redundant back-and-forth movements from speedwalk.
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
QString removeBacktracks(const QString& speedWalkString, const QString& filler);

} // namespace speedwalk

#endif // SPEEDWALK_ENGINE_H
