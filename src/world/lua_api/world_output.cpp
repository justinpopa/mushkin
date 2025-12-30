/**
 * world_output.cpp - Output Functions
 *
 * Functions for displaying text in the MUD output window. This includes
 * colored text, hyperlinks, ANSI processing, and info bar management.
 */

#include "lua_common.h"

/**
 * world.Note(text, ...)
 *
 * Displays text in the output window followed by a newline. Multiple arguments
 * are concatenated together. Uses the current note colors (set via NoteColour*
 * functions) or defaults to white on black.
 *
 * @param text (string) Text to display (multiple args are concatenated)
 *
 * @return Nothing
 *
 * @example
 * Note("Hello, world!")
 * Note("HP: ", hp, " / ", max_hp)  -- concatenates all arguments
 *
 * @see Tell, ColourNote, NoteColourRGB
 */
int L_Note(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString text = concatArgs(L);
    pDoc->note(text);
    return 0;
}

/**
 * world.ColourNote(fore, back, text, ...)
 *
 * Displays colored text followed by a newline. Arguments are processed in groups
 * of three: (foreground, background, text). This allows multiple color segments
 * on one line.
 *
 * Colors can be specified as:
 * - Color name: "red", "blue", "yellow", etc.
 * - RGB string: "#FF0000" or "FF0000"
 * - RGB number: 0xFF0000 or 16711680
 *
 * @param fore (string|number) Foreground color for first segment
 * @param back (string|number) Background color for first segment
 * @param text (string) Text for first segment
 *
 * @return Nothing
 *
 * @example
 * ColourNote("red", "black", "Error: ", "white", "black", "File not found")
 *
 * @example
 * -- Single colored message
 * ColourNote("lime", "", "Success!")
 *
 * @see ColourTell, Note, ColourNameToRGB
 */
int L_ColourNote(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int n = lua_gettop(L);

    // Process arguments in groups of 3: (foreColor, backColor, text)
    for (int i = 1; i <= n; i += 3) {
        if (i + 2 > n) {
            return luaL_error(
                L, "ColourNote requires arguments in groups of 3 (foreColor, backColor, text)");
        }

        QRgb foreColor = getColor(L, i, qRgb(255, 255, 255));
        QRgb backColor = getColor(L, i + 1, qRgb(0, 0, 0));
        const char* text = luaL_checkstring(L, i + 2);
        QString qtext = QString::fromUtf8(text);

        // Use ColourTell for all but the last block
        if (i + 3 <= n) {
            pDoc->colourTell(foreColor, backColor, qtext);
        } else {
            // Last block - use ColourNote to complete the line
            pDoc->colourNote(foreColor, backColor, qtext);
        }
    }

    return 0;
}

/**
 * world.Tell(text, ...)
 *
 * Displays text in the output window WITHOUT a trailing newline. Use this to
 * build up a line piece by piece, then finish with Note() or ColourNote().
 * Multiple arguments are concatenated together.
 *
 * @param text (string) Text to display (multiple args are concatenated)
 *
 * @return Nothing
 *
 * @example
 * Tell("Loading")
 * Tell(".")
 * Tell(".")
 * Note("done!")  -- outputs: "Loading...done!" with newline
 *
 * @see Note, ColourTell
 */
int L_Tell(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString text = concatArgs(L);

    // Use default note colors
    QRgb foreColor = pDoc->m_bNotesInRGB ? pDoc->m_iNoteColourFore : qRgb(255, 255, 255);
    QRgb backColor = pDoc->m_bNotesInRGB ? pDoc->m_iNoteColourBack : qRgb(0, 0, 0);

    pDoc->colourTell(foreColor, backColor, text);
    return 0;
}

/**
 * world.ANSI(code, ...)
 *
 * Generates an ANSI escape sequence string for the given code(s). Use this to
 * create colored strings that can be processed by AnsiNote or sent to the MUD.
 *
 * Common ANSI codes:
 * - 0: Reset all attributes
 * - 1: Bold/bright
 * - 30-37: Foreground colors (black, red, green, yellow, blue, magenta, cyan, white)
 * - 40-47: Background colors
 * - 90-97: Bright foreground colors
 *
 * @param code (number) First ANSI code
 *
 * @return (string) ANSI escape sequence (e.g., "\027[1;37m")
 *
 * @example
 * local bold_white = ANSI(1, 37)
 * local reset = ANSI(0)
 * AnsiNote(bold_white .. "Important!" .. reset .. " Normal text")
 *
 * @see AnsiNote
 */
int L_ANSI(lua_State* L)
{
    int n = lua_gettop(L);
    if (n == 0) {
        lua_pushstring(L, "\033[m");
        return 1;
    }

    // Build ANSI escape sequence: ESC [ code1;code2;...;codeN m
    QStringList codes;
    for (int i = 1; i <= n; i++) {
        codes.append(QString::number(luaL_checkinteger(L, i)));
    }

    QString ansi = QString("\033[%1m").arg(codes.join(";"));
    lua_pushstring(L, ansi.toUtf8().constData());
    return 1;
}

/**
 * world.AnsiNote(text)
 *
 * Displays text containing ANSI escape codes in the output window, interpreting
 * the codes to produce colored output. Supports standard ANSI SGR (Select Graphic
 * Rendition) codes for colors and text styles.
 *
 * @param text (string) Text with embedded ANSI escape sequences
 *
 * @return Nothing
 *
 * @example
 * AnsiNote("\027[1;31mRed bold text\027[0m normal text")
 *
 * @example
 * -- Using ANSI() helper
 * AnsiNote(ANSI(1,32) .. "Green!" .. ANSI(0))
 *
 * @see ANSI, Note, ColourNote
 */
int L_AnsiNote(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);
    QString qtext = QString::fromUtf8(text);

    // Standard ANSI color palette (first 8 colors)
    static const QRgb ansiColors[8] = {
        qRgb(0, 0, 0),      // Black
        qRgb(128, 0, 0),    // Red
        qRgb(0, 128, 0),    // Green
        qRgb(128, 128, 0),  // Yellow
        qRgb(0, 0, 128),    // Blue
        qRgb(128, 0, 128),  // Magenta
        qRgb(0, 128, 128),  // Cyan
        qRgb(192, 192, 192) // White
    };

    // Bright ANSI colors (high intensity)
    static const QRgb ansiBrightColors[8] = {
        qRgb(128, 128, 128), // Bright Black (Gray)
        qRgb(255, 0, 0),     // Bright Red
        qRgb(0, 255, 0),     // Bright Green
        qRgb(255, 255, 0),   // Bright Yellow
        qRgb(0, 0, 255),     // Bright Blue
        qRgb(255, 0, 255),   // Bright Magenta
        qRgb(0, 255, 255),   // Bright Cyan
        qRgb(255, 255, 255)  // Bright White
    };

    // Default colors
    QRgb foreColor = qRgb(192, 192, 192); // Light gray
    QRgb backColor = qRgb(0, 0, 0);       // Black
    bool bold = false;

    int pos = 0;
    int len = qtext.length();

    while (pos < len) {
        // Look for ESC character
        int escPos = qtext.indexOf('\033', pos);

        if (escPos == -1) {
            // No more escape sequences - output rest of text
            QString segment = qtext.mid(pos);
            if (!segment.isEmpty()) {
                pDoc->colourTell(foreColor, backColor, segment);
            }
            break;
        }

        // Output text before the escape sequence
        if (escPos > pos) {
            QString segment = qtext.mid(pos, escPos - pos);
            pDoc->colourTell(foreColor, backColor, segment);
        }

        // Parse escape sequence: ESC [ params m
        pos = escPos + 1;
        if (pos >= len || qtext[pos] != '[') {
            continue; // Not a CSI sequence
        }
        pos++; // Skip '['

        // Collect parameters
        QString params;
        while (pos < len && (qtext[pos].isDigit() || qtext[pos] == ';')) {
            params += qtext[pos++];
        }

        // Check for 'm' (SGR - Select Graphic Rendition)
        if (pos >= len || qtext[pos] != 'm') {
            continue;
        }
        pos++; // Skip 'm'

        // Parse SGR parameters
        QStringList codes = params.split(';', Qt::SkipEmptyParts);
        if (codes.isEmpty()) {
            codes << "0"; // Empty means reset
        }

        for (const QString& codeStr : codes) {
            int code = codeStr.toInt();

            if (code == 0) {
                // Reset all
                foreColor = qRgb(192, 192, 192);
                backColor = qRgb(0, 0, 0);
                bold = false;
            } else if (code == 1) {
                bold = true;
            } else if (code == 22) {
                bold = false;
            } else if (code >= 30 && code <= 37) {
                // Standard foreground colors
                int idx = code - 30;
                foreColor = bold ? ansiBrightColors[idx] : ansiColors[idx];
            } else if (code >= 40 && code <= 47) {
                // Standard background colors
                backColor = ansiColors[code - 40];
            } else if (code >= 90 && code <= 97) {
                // Bright foreground colors
                foreColor = ansiBrightColors[code - 90];
            } else if (code >= 100 && code <= 107) {
                // Bright background colors
                backColor = ansiBrightColors[code - 100];
            }
            // TODO: 256-color and RGB modes (38;5;N and 38;2;R;G;B)
        }
    }

    // Finish with a newline (this is AnsiNote, not AnsiTell)
    pDoc->colourNote(foreColor, backColor, "");
    return 0;
}

/**
 * world.Hyperlink(action, text, hint, forecolour, backcolour, url)
 *
 * Displays a clickable hyperlink in the output window. When clicked, the action
 * is either sent to the MUD as a command or opened in the system browser.
 *
 * @param action (string) Command to send when clicked, or URL if url=true (required)
 * @param text (string) Display text shown to user (optional, defaults to action)
 * @param hint (string) Tooltip text on hover (optional, defaults to action)
 * @param forecolour (string|number) Text color (optional, defaults to hyperlink color)
 * @param backcolour (string|number) Background color (optional, defaults to note background)
 * @param url (boolean) If true, opens in browser instead of sending to MUD (optional, defaults to false)
 *
 * @return Nothing
 *
 * @example
 * -- Send command to MUD when clicked
 * Hyperlink("look north", "[North]", "Look to the north")
 *
 * @example
 * -- Open URL in browser
 * Hyperlink("https://example.com", "Visit Site", "Open in browser", "", "", true)
 *
 * @see ColourNote, Tell
 */
int L_Hyperlink(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // action is required
    const char* action = luaL_checkstring(L, 1);
    if (strlen(action) == 0) {
        return 0; // Empty action - do nothing
    }

    // text is optional, defaults to action
    const char* text = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";

    // hint is optional, defaults to action
    const char* hint = lua_isstring(L, 3) ? lua_tostring(L, 3) : "";

    // forecolour is optional, defaults to hyperlink color
    QRgb foreColor = pDoc->m_iHyperlinkColour;
    if (!lua_isnoneornil(L, 4)) {
        foreColor = getColor(L, 4, foreColor);
    }

    // backcolour is optional, defaults to note background
    QRgb backColor = pDoc->m_bNotesInRGB ? pDoc->m_iNoteColourBack : qRgb(0, 0, 0);
    if (!lua_isnoneornil(L, 5)) {
        backColor = getColor(L, 5, backColor);
    }

    // url is optional, defaults to false
    bool isUrl = lua_toboolean(L, 6);

    pDoc->hyperlink(QString::fromUtf8(action), QString::fromUtf8(text), QString::fromUtf8(hint),
                    foreColor, backColor, isUrl);
    return 0;
}

/**
 * world.Simulate(text)
 *
 * Processes text as if it were received from the MUD. The text goes through
 * the full input pipeline: ANSI codes are interpreted, triggers are matched
 * and fired, and output appears in the main window.
 *
 * Useful for testing triggers without connecting to a MUD, or for injecting
 * synthetic MUD output from scripts.
 *
 * @param text (string) Text to simulate (may include ANSI escape sequences and newlines)
 *
 * @return Nothing
 *
 * @example
 * -- Test a trigger
 * Simulate("You have gained 100 experience points.\n")
 *
 * @example
 * -- Simulate colored output
 * Simulate("\027[1;31mCRITICAL HIT!\027[0m\n")
 *
 * @see AnsiNote, Note
 */
int L_Simulate(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_checkstring(L, 1);
    pDoc->simulate(QString::fromUtf8(text));
    return 0;
}

/**
 * world.ColourTell(fore, back, text, ...)
 *
 * Displays colored text WITHOUT a trailing newline. Like ColourNote, arguments
 * are processed in groups of three (foreground, background, text). Use this to
 * build up a colorful line piece by piece.
 *
 * @param fore (string|number) Foreground color for first segment
 * @param back (string|number) Background color for first segment
 * @param text (string) Text for first segment
 *
 * @return Nothing
 *
 * @example
 * ColourTell("yellow", "black", "[WARNING] ")
 * ColourNote("white", "black", "Low health!")
 *
 * @see ColourNote, Tell
 */
int L_ColourTell(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int n = lua_gettop(L);

    // Process arguments in groups of 3: (foreColor, backColor, text)
    for (int i = 1; i <= n; i += 3) {
        if (i + 2 > n) {
            return luaL_error(
                L, "ColourTell requires arguments in groups of 3 (foreColor, backColor, text)");
        }

        QRgb foreColor = getColor(L, i, qRgb(255, 255, 255));
        QRgb backColor = getColor(L, i + 1, qRgb(0, 0, 0));
        const char* text = luaL_checkstring(L, i + 2);
        QString qtext = QString::fromUtf8(text);

        pDoc->colourTell(foreColor, backColor, qtext);
    }

    return 0;
}

// Include Line and Style for GetLineInfo/GetStyleInfo
#include "../../text/action.h"
#include "../../text/line.h"
#include "../../text/style.h"

/**
 * world.GetLineInfo(line_number, info_type)
 *
 * Returns information about a specific line in the output buffer. Use this
 * to inspect line content, metadata, and styling for triggers, logging, or
 * display purposes.
 *
 * Info types:
 * - 1: Text content of the line (string)
 * - 2: Length of text in characters (number)
 * - 3: True if line ends with newline (hard return), false if wrapped (boolean)
 * - 4: True if line was from world.Note() or script output (boolean)
 * - 5: True if line was player input (boolean)
 * - 6: True if line is marked for logging (boolean)
 * - 7: True if line is bookmarked (boolean)
 * - 8: True if line is a horizontal rule (boolean)
 * - 9: Date/time line arrived as Unix timestamp (number - seconds since epoch)
 * - 10: Actual line number in session (number)
 * - 11: Count of style runs on the line (number)
 *
 * @param line_number (number) Line number in output buffer (1-based)
 * @param info_type (number) Type of information to retrieve (1-11)
 *
 * @return Requested information (type varies by info_type)
 * @return (nil) If line number is out of range or info_type is invalid
 *
 * @example
 * -- Get the text of the last line
 * local lineCount = GetInfo(212)  -- total lines
 * local text = GetLineInfo(lineCount, 1)
 * print("Last line: " .. text)
 *
 * @example
 * -- Check if a line was player input
 * if GetLineInfo(lineNum, 5) then
 *     print("This was something you typed")
 * end
 *
 * @see GetStyleInfo, GetRecentLines
 */
int L_GetLineInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int lineNumber = luaL_checkinteger(L, 1);
    int infoType = luaL_checkinteger(L, 2);

    // Check line exists (1-based index)
    if (lineNumber <= 0 || lineNumber > pDoc->m_lineList.size()) {
        lua_pushnil(L);
        return 1;
    }

    // Get the line (convert to 0-based index)
    Line* line = pDoc->m_lineList.at(lineNumber - 1);
    if (!line) {
        lua_pushnil(L);
        return 1;
    }

    switch (infoType) {
        case 1: // text of line
            lua_pushlstring(L, line->text(), line->len());
            break;
        case 2: // length of text
            lua_pushinteger(L, line->len());
            break;
        case 3: // true if newline (hard return)
            lua_pushboolean(L, line->hard_return);
            break;
        case 4: // true if world.note (comment)
            lua_pushboolean(L, (line->flags & COMMENT) != 0);
            break;
        case 5: // true if player input
            lua_pushboolean(L, (line->flags & USER_INPUT) != 0);
            break;
        case 6: // true if line logged
            lua_pushboolean(L, (line->flags & LOG_LINE) != 0);
            break;
        case 7: // true if bookmarked
            lua_pushboolean(L, (line->flags & BOOKMARK) != 0);
            break;
        case 8: // true if horizontal rule
            lua_pushboolean(L, (line->flags & HORIZ_RULE) != 0);
            break;
        case 9: // date/time line arrived (seconds since epoch)
            lua_pushnumber(L, line->m_theTime.toSecsSinceEpoch());
            break;
        case 10: // actual line number
            lua_pushinteger(L, line->m_nLineNumber);
            break;
        case 11: // count of style runs
            lua_pushinteger(L, line->styleList.size());
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.GetStyleInfo(line_number, style_number, info_type)
 *
 * Returns information about a specific style run within a line. A "style run"
 * is a contiguous segment of text with the same formatting (color, bold, etc.).
 * Use GetLineInfo(line, 11) to get the count of style runs on a line.
 *
 * Info types:
 * - 1: Text content of this style run (string)
 * - 2: Length of style run in characters (number)
 * - 3: Starting column of style, 1-based (number)
 * - 4: Action type: 0=none, 1=send to MUD, 2=hyperlink, 3=prompt (number)
 * - 5: Action string (what to send/open when clicked) (string)
 * - 6: Hint/tooltip text (string)
 * - 7: MXP variable name to set (string)
 * - 8: True if bold (boolean)
 * - 9: True if underlined (boolean)
 * - 10: True if blinking/italic (boolean)
 * - 11: True if inverse video (boolean)
 * - 12: True if modified by a trigger (boolean)
 * - 13: True if start of an MXP tag (boolean)
 * - 14: Foreground (text) color as RGB integer (number)
 * - 15: Background color as RGB integer (number)
 *
 * @param line_number (number) Line number in output buffer (1-based)
 * @param style_number (number) Style run index within the line (1-based)
 * @param info_type (number) Type of information to retrieve (1-15)
 *
 * @return Requested information (type varies by info_type)
 * @return (nil) If line/style number is out of range or info_type is invalid
 *
 * @example
 * -- Iterate through all style runs on a line
 * local styleCount = GetLineInfo(lineNum, 11)
 * for i = 1, styleCount do
 *     local text = GetStyleInfo(lineNum, i, 1)
 *     local fore = GetStyleInfo(lineNum, i, 14)
 *     print(string.format("Style %d: '%s' color=#%06X", i, text, fore))
 * end
 *
 * @see GetLineInfo, GetRecentLines
 */
int L_GetStyleInfo(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int lineNumber = luaL_checkinteger(L, 1);
    int styleNumber = luaL_checkinteger(L, 2);
    int infoType = luaL_checkinteger(L, 3);

    // Check line exists (1-based index)
    if (lineNumber <= 0 || lineNumber > pDoc->m_lineList.size()) {
        lua_pushnil(L);
        return 1;
    }

    Line* line = pDoc->m_lineList.at(lineNumber - 1);
    if (!line) {
        lua_pushnil(L);
        return 1;
    }

    // Check style exists (1-based index)
    if (styleNumber <= 0 || styleNumber > (int)line->styleList.size()) {
        lua_pushnil(L);
        return 1;
    }

    Style* style = line->styleList[styleNumber - 1].get();
    if (!style) {
        lua_pushnil(L);
        return 1;
    }

    // Calculate starting column for this style
    int startCol = 1;
    for (int i = 0; i < styleNumber - 1; i++) {
        startCol += line->styleList[i]->iLength;
    }

    switch (infoType) {
        case 1: { // text of style
            // Extract text covered by this style
            int offset = startCol - 1;
            int length = style->iLength;
            if (offset >= 0 && offset + length <= line->len()) {
                lua_pushlstring(L, line->text() + offset, length);
            } else {
                lua_pushstring(L, "");
            }
            break;
        }
        case 2: // length of style run
            lua_pushinteger(L, style->iLength);
            break;
        case 3: // starting column (1-based)
            lua_pushinteger(L, startCol);
            break;
        case 4: { // action type
            int actionType = (style->iFlags & ACTIONTYPE);
            int result = 0;
            if (actionType == ACTION_SEND)
                result = 1;
            else if (actionType == ACTION_HYPERLINK)
                result = 2;
            else if (actionType == ACTION_PROMPT)
                result = 3;
            lua_pushinteger(L, result);
            break;
        }
        case 5: // action (what to send)
            if (style->pAction) {
                lua_pushstring(L, style->pAction->m_strAction.toUtf8().constData());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 6: // hint (tooltip)
            if (style->pAction) {
                lua_pushstring(L, style->pAction->m_strHint.toUtf8().constData());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 7: // variable (MXP)
            if (style->pAction) {
                lua_pushstring(L, style->pAction->m_strVariable.toUtf8().constData());
            } else {
                lua_pushstring(L, "");
            }
            break;
        case 8: // bold
            lua_pushboolean(L, (style->iFlags & HILITE) != 0);
            break;
        case 9: // underlined
            lua_pushboolean(L, (style->iFlags & UNDERLINE) != 0);
            break;
        case 10: // blinking/italic
            lua_pushboolean(L, (style->iFlags & BLINK) != 0);
            break;
        case 11: // inverse
            lua_pushboolean(L, (style->iFlags & INVERSE) != 0);
            break;
        case 12: // changed by trigger
            lua_pushboolean(L, (style->iFlags & CHANGED) != 0);
            break;
        case 13: // start of tag
            lua_pushboolean(L, (style->iFlags & START_TAG) != 0);
            break;
        case 14: // foreground colour (RGB)
            lua_pushinteger(L, style->iForeColour);
            break;
        case 15: // background colour (RGB)
            lua_pushinteger(L, style->iBackColour);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

/**
 * world.GetRecentLines(count)
 *
 * Returns the most recent lines from the multiline trigger buffer as a single
 * string with lines joined by newlines. This is useful for multiline trigger
 * matching or analyzing recent MUD output.
 *
 * The multiline buffer contains only lines received from the MUD (not notes
 * or player input). Buffer size is configurable in world settings.
 *
 * @param count (number) Number of recent lines to retrieve
 *
 * @return (string) Lines joined by newlines, or empty string if buffer is empty
 *
 * @example
 * -- Get last 5 lines for pattern matching
 * local recent = GetRecentLines(5)
 * if string.find(recent, "combat ended") then
 *     print("Combat is over!")
 * end
 *
 * @see GetLineInfo, GetStyleInfo
 */
int L_GetRecentLines(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int count = luaL_checkinteger(L, 1);

    if (count <= 0 || pDoc->m_recentLines.empty()) {
        lua_pushstring(L, "");
        return 1;
    }

    // Assemble multi-line text from recent lines buffer
    // Based on methods_output.cpp
    int bufferSize = static_cast<int>(pDoc->m_recentLines.size());
    int startPos = bufferSize - count;
    if (startPos < 0) {
        startPos = 0;
    }

    QString result;
    for (int i = startPos; i < bufferSize; i++) {
        if (!result.isEmpty()) {
            result += '\n';
        }
        result += pDoc->m_recentLines[static_cast<size_t>(i)];
    }

    QByteArray utf8 = result.toUtf8();
    lua_pushlstring(L, utf8.constData(), utf8.size());
    return 1;
}

/**
 * world.NoteColour()
 *
 * Returns the current note color mode as an index. Note colors are used by
 * Note() and Tell() functions when no explicit color is specified.
 *
 * @return (number) Color index:
 *   - -1: RGB mode (use NoteColourFore/NoteColourBack for actual values)
 *   - 0: Same color mode (use default text color)
 *   - 1-16: Custom color index
 *
 * @example
 * local colorMode = NoteColour()
 * if colorMode == -1 then
 *     print("Using RGB colors: " .. NoteColourFore())
 * elseif colorMode == 0 then
 *     print("Using default colors")
 * else
 *     print("Using custom color " .. colorMode)
 * end
 *
 * @see NoteColourFore, NoteColourBack, NoteColourRGB, SetNoteColour
 */
int L_NoteColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_bNotesInRGB) {
        lua_pushinteger(L, -1);
    } else {
        // SAMECOLOUR is typically -1 or 65535
        lua_pushinteger(L, pDoc->m_iNoteTextColour == 65535 ? 0 : pDoc->m_iNoteTextColour + 1);
    }
    return 1;
}

/**
 * world.NoteColourFore()
 *
 * Returns the raw note foreground color as an RGB integer. This is the color
 * stored in settings, not necessarily the effective color (see GetNoteColourFore
 * for the effective color).
 *
 * @return (number) RGB color value (0x000000 to 0xFFFFFF)
 *
 * @example
 * local fore = NoteColourFore()
 * print(string.format("Foreground: #%06X", fore))
 *
 * @see NoteColourBack, GetNoteColourFore, NoteColourRGB
 */
int L_NoteColourFore(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushinteger(L, pDoc->m_iNoteColourFore & 0x00FFFFFF);
    return 1;
}

/**
 * world.NoteColourBack()
 *
 * Returns the raw note background color as an RGB integer. This is the color
 * stored in settings, not necessarily the effective color (see GetNoteColourBack
 * for the effective color).
 *
 * @return (number) RGB color value (0x000000 to 0xFFFFFF)
 *
 * @example
 * local back = NoteColourBack()
 * print(string.format("Background: #%06X", back))
 *
 * @see NoteColourFore, GetNoteColourBack, NoteColourRGB
 */
int L_NoteColourBack(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushinteger(L, pDoc->m_iNoteColourBack & 0x00FFFFFF);
    return 1;
}

/**
 * world.NoteColourRGB(foreground, background)
 *
 * Sets the note colors to specific RGB values. This switches to RGB mode
 * (NoteColour() will return -1). Subsequent Note() and Tell() calls will
 * use these colors.
 *
 * @param foreground (number) Foreground color as RGB integer (0x000000 to 0xFFFFFF)
 * @param background (number) Background color as RGB integer (0x000000 to 0xFFFFFF)
 *
 * @return Nothing
 *
 * @example
 * -- Set cyan text on dark blue background
 * NoteColourRGB(0x00FFFF, 0x000080)
 * Note("This is cyan on dark blue")
 *
 * @see NoteColourName, NoteColourFore, NoteColourBack, SetNoteColour
 */
int L_NoteColourRGB(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_Integer fore = luaL_checkinteger(L, 1);
    lua_Integer back = luaL_checkinteger(L, 2);

    pDoc->m_bNotesInRGB = true;
    pDoc->m_iNoteColourFore = fore & 0x00FFFFFF;
    pDoc->m_iNoteColourBack = back & 0x00FFFFFF;
    return 0;
}

/**
 * world.NoteColourName(foreground, background)
 *
 * Sets the note colors by color name. This switches to RGB mode and converts
 * the named colors to their RGB equivalents. Supports standard color names
 * like "red", "blue", "yellow", etc.
 *
 * @param foreground (string) Foreground color name (e.g., "red", "cyan", "white")
 * @param background (string) Background color name (e.g., "black", "blue")
 *
 * @return Nothing
 *
 * @example
 * NoteColourName("yellow", "black")
 * Note("Warning message in yellow")
 *
 * @see NoteColourRGB, ColourNameToRGB, SetNoteColour
 */
int L_NoteColourName(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* foreName = luaL_checkstring(L, 1);
    const char* backName = luaL_checkstring(L, 2);

    pDoc->m_bNotesInRGB = true;
    pDoc->m_iNoteColourFore = getColor(L, 1, pDoc->m_iNoteColourFore);
    pDoc->m_iNoteColourBack = getColor(L, 2, pDoc->m_iNoteColourBack);
    return 0;
}

// SAMECOLOUR constant - special value meaning "use default color"
constexpr quint16 SAMECOLOUR = 65535;

/**
 * world.GetNoteColourFore()
 *
 * Returns the effective note foreground color as RGB. Unlike NoteColourFore()
 * which returns the raw stored value, this function resolves the actual color
 * that would be used based on the current color mode.
 *
 * Resolution rules:
 * - RGB mode: Returns the stored RGB foreground value
 * - Same color mode: Returns custom color 16 (if enabled) or default white
 * - Index mode: Returns the custom text color for that index
 *
 * @return (number) Effective foreground color as RGB integer
 *
 * @example
 * -- Get the actual color that Note() would use
 * local actualColor = GetNoteColourFore()
 * ColourNote(RGBColourToName(actualColor), "black", "Same color as Note()")
 *
 * @see GetNoteColourBack, NoteColourFore, NoteColour
 */
int L_GetNoteColourFore(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_bNotesInRGB) {
        lua_pushinteger(L, pDoc->m_iNoteColourFore);
    } else if (pDoc->m_iNoteTextColour == SAMECOLOUR) {
        if (pDoc->m_bCustom16isDefaultColour)
            lua_pushinteger(L, pDoc->m_customtext[15]);
        else
            lua_pushinteger(L, pDoc->m_normalcolour[ANSI_WHITE]);
    } else {
        // Ensure valid index
        int index = pDoc->m_iNoteTextColour;
        if (index >= 0 && index < MAX_CUSTOM)
            lua_pushinteger(L, pDoc->m_customtext[index]);
        else
            lua_pushinteger(L, pDoc->m_normalcolour[ANSI_WHITE]); // fallback
    }
    return 1;
}

/**
 * world.GetNoteColourBack()
 *
 * Returns the effective note background color as RGB. Unlike NoteColourBack()
 * which returns the raw stored value, this function resolves the actual color
 * that would be used based on the current color mode.
 *
 * Resolution rules:
 * - RGB mode: Returns the stored RGB background value
 * - Same color mode: Returns custom color 16 background (if enabled) or default black
 * - Index mode: Returns the custom background color for that index
 *
 * @return (number) Effective background color as RGB integer
 *
 * @example
 * local backColor = GetNoteColourBack()
 * print(string.format("Note background: #%06X", backColor))
 *
 * @see GetNoteColourFore, NoteColourBack, NoteColour
 */
int L_GetNoteColourBack(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_bNotesInRGB) {
        lua_pushinteger(L, pDoc->m_iNoteColourBack);
    } else if (pDoc->m_iNoteTextColour == SAMECOLOUR) {
        if (pDoc->m_bCustom16isDefaultColour)
            lua_pushinteger(L, pDoc->m_customback[15]);
        else
            lua_pushinteger(L, pDoc->m_normalcolour[ANSI_BLACK]);
    } else {
        // Ensure valid index
        int index = pDoc->m_iNoteTextColour;
        if (index >= 0 && index < MAX_CUSTOM)
            lua_pushinteger(L, pDoc->m_customback[index]);
        else
            lua_pushinteger(L, pDoc->m_normalcolour[ANSI_BLACK]); // fallback
    }
    return 1;
}

/**
 * world.SetNoteColour(colour)
 *
 * Sets the note color by index, switching out of RGB mode. Use this to select
 * from the 16 custom colors defined in world settings, or to use the default
 * "same color" mode.
 *
 * @param colour (number) Color index:
 *   - 0: Same color mode (use default text color)
 *   - 1-16: Custom color index from world settings
 *
 * @return Nothing
 *
 * @example
 * SetNoteColour(1)  -- Use custom color 1
 * Note("This uses custom color 1")
 * SetNoteColour(0)  -- Reset to default
 *
 * @see NoteColour, NoteColourRGB, NoteColourName
 */
int L_SetNoteColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_Integer colour = luaL_checkinteger(L, 1);

    if (colour >= 0 && colour <= MAX_CUSTOM) {
        pDoc->m_iNoteTextColour =
            colour - 1; // Convert 1-based to 0-based (0 becomes -1/SAMECOLOUR)
        pDoc->m_bNotesInRGB = false;
    }
    return 0;
}

/**
 * world.SetNoteColourFore(rgb)
 *
 * Sets the note foreground color to a specific RGB value. This automatically
 * enables RGB mode. If not already in RGB mode, the current background color
 * is preserved by converting it from the index-based color.
 *
 * @param rgb (number) Foreground color as RGB integer (0x000000 to 0xFFFFFF)
 *
 * @return Nothing
 *
 * @example
 * -- Set just the foreground, keep existing background
 * SetNoteColourFore(0xFF0000)  -- Red foreground
 * Note("Red text on current background")
 *
 * @see SetNoteColourBack, NoteColourRGB, GetNoteColourFore
 */
int L_SetNoteColourFore(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_Integer rgb = luaL_checkinteger(L, 1);

    // Convert background to RGB if necessary
    if (!pDoc->m_bNotesInRGB) {
        if (pDoc->m_iNoteTextColour == SAMECOLOUR) {
            if (pDoc->m_bCustom16isDefaultColour)
                pDoc->m_iNoteColourBack = pDoc->m_customback[15];
            else
                pDoc->m_iNoteColourBack = pDoc->m_normalcolour[ANSI_BLACK];
        } else {
            int index = pDoc->m_iNoteTextColour;
            if (index >= 0 && index < MAX_CUSTOM)
                pDoc->m_iNoteColourBack = pDoc->m_customback[index];
            else
                pDoc->m_iNoteColourBack = pDoc->m_normalcolour[ANSI_BLACK];
        }
    }

    pDoc->m_bNotesInRGB = true;
    pDoc->m_iNoteColourFore = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.SetNoteColourBack(rgb)
 *
 * Sets the note background color to a specific RGB value. This automatically
 * enables RGB mode. If not already in RGB mode, the current foreground color
 * is preserved by converting it from the index-based color.
 *
 * @param rgb (number) Background color as RGB integer (0x000000 to 0xFFFFFF)
 *
 * @return Nothing
 *
 * @example
 * -- Set just the background, keep existing foreground
 * SetNoteColourBack(0x000080)  -- Dark blue background
 * Note("Current text color on dark blue")
 *
 * @see SetNoteColourFore, NoteColourRGB, GetNoteColourBack
 */
int L_SetNoteColourBack(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_Integer rgb = luaL_checkinteger(L, 1);

    // Convert foreground to RGB if necessary
    if (!pDoc->m_bNotesInRGB) {
        if (pDoc->m_iNoteTextColour == SAMECOLOUR) {
            if (pDoc->m_bCustom16isDefaultColour)
                pDoc->m_iNoteColourFore = pDoc->m_customtext[15];
            else
                pDoc->m_iNoteColourFore = pDoc->m_normalcolour[ANSI_WHITE];
        } else {
            int index = pDoc->m_iNoteTextColour;
            if (index >= 0 && index < MAX_CUSTOM)
                pDoc->m_iNoteColourFore = pDoc->m_customtext[index];
            else
                pDoc->m_iNoteColourFore = pDoc->m_normalcolour[ANSI_WHITE];
        }
    }

    pDoc->m_bNotesInRGB = true;
    pDoc->m_iNoteColourBack = rgb & 0x00FFFFFF;
    return 0;
}

/**
 * world.NoteStyle(style)
 *
 * Sets the text style for subsequent Note() and Tell() output. Styles can be
 * combined by adding the values together.
 *
 * Style values (can be combined):
 * - 0: Normal (no special styling)
 * - 1: Bold/highlight
 * - 2: Underline
 * - 4: Blink/italic
 * - 8: Inverse (swap foreground/background)
 *
 * @param style (number) Style flags (0-15, bitwise OR of style values)
 *
 * @return Nothing
 *
 * @example
 * NoteStyle(1)  -- Bold
 * Note("Bold text")
 * NoteStyle(3)  -- Bold + Underline (1 + 2)
 * Note("Bold and underlined")
 * NoteStyle(0)  -- Reset to normal
 *
 * @see GetNoteStyle, ColourNote
 */
int L_NoteStyle(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int style = luaL_checkinteger(L, 1);

    // TEXT_STYLE mask is 0x0F (HILITE|UNDERLINE|BLINK|INVERSE)
    pDoc->m_iNoteStyle = style & 0x0F;
    return 0;
}

/**
 * world.GetNoteStyle()
 *
 * Returns the current note text style as a bitfield. See NoteStyle() for
 * the meaning of each bit.
 *
 * @return (number) Current style flags (0-15)
 *
 * @example
 * local style = GetNoteStyle()
 * if (style % 2) == 1 then  -- Check bit 0
 *     print("Bold is enabled")
 * end
 *
 * @see NoteStyle
 */
int L_GetNoteStyle(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushinteger(L, pDoc->m_iNoteStyle & 0x0F);
    return 1;
}

/**
 * world.NoteHr()
 *
 * Outputs a horizontal rule (divider line) in the output window. The rule
 * spans the full width of the output area and is rendered using the current
 * note colors.
 *
 * @return Nothing
 *
 * @example
 * Note("Section 1 content")
 * NoteHr()
 * Note("Section 2 content")
 *
 * @see Note, ColourNote
 */
int L_NoteHr(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->noteHr();
    return 0;
}

// ========== Info Bar Functions ==========

/**
 * world.Info(text)
 *
 * Appends text to the info bar. The info bar is a separate display area
 * (typically at the top or bottom of the world window) for showing status
 * information, gauges, or other persistent data.
 *
 * Text accumulates until cleared with InfoClear(). Use ShowInfoBar(true)
 * to make the info bar visible.
 *
 * @param text (string) Text to append to the info bar (optional, empty string if omitted)
 *
 * @return Nothing
 *
 * @example
 * InfoClear()
 * Info("HP: 100/100  MP: 50/50")
 * ShowInfoBar(true)
 *
 * @see InfoClear, InfoColour, ShowInfoBar
 */
int L_Info(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* text = luaL_optstring(L, 1, "");
    pDoc->m_infoBarText.append(QString::fromUtf8(text));
    emit pDoc->infoBarChanged();
    return 0;
}

/**
 * world.InfoClear()
 *
 * Clears all info bar content and resets formatting to defaults:
 * - Text color: Black
 * - Background color: White
 * - Font: Courier New, 10pt, normal style
 *
 * Call this before building new info bar content to start fresh.
 *
 * @return Nothing
 *
 * @example
 * -- Update info bar with new stats
 * InfoClear()
 * InfoColour("darkgreen")
 * Info("HP: " .. hp .. "/" .. maxHp)
 *
 * @see Info, InfoColour, InfoBackground
 */
int L_InfoClear(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_infoBarText.clear();
    // Reset to defaults
    pDoc->m_infoBarTextColor = qRgb(0, 0, 0);       // Black
    pDoc->m_infoBarBackColor = qRgb(255, 255, 255); // White
    pDoc->m_infoBarFontName = "Courier New";
    pDoc->m_infoBarFontSize = 10;
    pDoc->m_infoBarFontStyle = 0;
    emit pDoc->infoBarChanged();
    return 0;
}

/**
 * world.InfoColour(name)
 *
 * Sets the info bar text color by name. Supports standard color names
 * like "red", "blue", "darkgreen", etc.
 *
 * @param name (string) Color name (e.g., "red", "blue", "yellow")
 *
 * @return Nothing
 *
 * @example
 * InfoClear()
 * InfoColour("red")
 * Info("WARNING: Low health!")
 *
 * @see InfoBackground, InfoClear, ColourNameToRGB
 */
int L_InfoColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QRgb color = ColourNameToRGB(QString::fromUtf8(name));
    pDoc->m_infoBarTextColor = color;
    emit pDoc->infoBarChanged();
    return 0;
}

/**
 * world.InfoBackground(name)
 *
 * Sets the info bar background color by name. Supports standard color names
 * like "black", "white", "darkblue", etc.
 *
 * @param name (string) Color name (e.g., "black", "white", "navy")
 *
 * @return Nothing
 *
 * @example
 * InfoClear()
 * InfoBackground("darkblue")
 * InfoColour("white")
 * Info("Status Bar")
 *
 * @see InfoColour, InfoClear, ColourNameToRGB
 */
int L_InfoBackground(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* name = luaL_checkstring(L, 1);
    QRgb color = ColourNameToRGB(QString::fromUtf8(name));
    pDoc->m_infoBarBackColor = color;
    emit pDoc->infoBarChanged();
    return 0;
}

/**
 * world.InfoFont(fontName, size, style)
 *
 * Sets the info bar font family, size, and style. Any parameter can be
 * omitted or set to a default value to keep the current setting.
 *
 * Style values (can be combined):
 * - 0: Normal
 * - 1: Bold
 * - 2: Italic
 * - 4: Underline
 * - 8: Strikeout
 *
 * @param fontName (string) Font family name (optional, empty string keeps current)
 * @param size (number) Font size in points (optional, 0 or negative keeps current)
 * @param style (number) Style flags (optional, 0-15, bitwise OR of style values)
 *
 * @return Nothing
 *
 * @example
 * -- Large bold monospace font
 * InfoFont("Consolas", 14, 1)
 * Info("IMPORTANT STATUS")
 *
 * @example
 * -- Just change the size, keep font and style
 * InfoFont("", 12, 0)
 *
 * @see Info, InfoClear, InfoColour
 */
int L_InfoFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* fontName = luaL_optstring(L, 1, "");
    int size = luaL_optinteger(L, 2, 0);
    int style = luaL_optinteger(L, 3, 0);

    // Font name (if provided and not empty)
    if (fontName[0] != '\0') {
        pDoc->m_infoBarFontName = QString::fromUtf8(fontName);
    }

    // Size (if positive)
    if (size > 0) {
        pDoc->m_infoBarFontSize = size;
    }

    // Style bits: 1=bold, 2=italic, 4=underline, 8=strikeout
    pDoc->m_infoBarFontStyle = style & 0x0F;

    emit pDoc->infoBarChanged();
    return 0;
}

/**
 * world.ShowInfoBar(visible)
 *
 * Shows or hides the info bar. The info bar retains its content when hidden,
 * so you can hide it temporarily and show it again with the same content.
 *
 * @param visible (boolean) True to show the info bar, false to hide it
 *
 * @return Nothing
 *
 * @example
 * -- Toggle info bar visibility
 * local isVisible = GetInfo(280)  -- hypothetical info type for visibility
 * ShowInfoBar(not isVisible)
 *
 * @example
 * -- Hide info bar during combat, show after
 * ShowInfoBar(false)
 * -- ... combat happens ...
 * ShowInfoBar(true)
 *
 * @see Info, InfoClear
 */
int L_ShowInfoBar(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool visible = lua_toboolean(L, 1);
    pDoc->m_infoBarVisible = visible;
    emit pDoc->infoBarChanged();
    return 0;
}

// ========== Registration ==========

void register_world_output_functions(luaL_Reg*& ptr)
{
    *ptr++ = {"Note", L_Note};
    *ptr++ = {"ColourNote", L_ColourNote};
    *ptr++ = {"ColourTell", L_ColourTell};
    *ptr++ = {"Tell", L_Tell};
    *ptr++ = {"ANSI", L_ANSI};
    *ptr++ = {"AnsiNote", L_AnsiNote};
    *ptr++ = {"Hyperlink", L_Hyperlink};
    *ptr++ = {"Simulate", L_Simulate};
    *ptr++ = {"GetLineInfo", L_GetLineInfo};
    *ptr++ = {"GetStyleInfo", L_GetStyleInfo};
    *ptr++ = {"GetRecentLines", L_GetRecentLines};
    // Info Bar functions
    *ptr++ = {"Info", L_Info};
    *ptr++ = {"InfoClear", L_InfoClear};
    *ptr++ = {"InfoColour", L_InfoColour};
    *ptr++ = {"InfoBackground", L_InfoBackground};
    *ptr++ = {"InfoFont", L_InfoFont};
    *ptr++ = {"ShowInfoBar", L_ShowInfoBar};
}
