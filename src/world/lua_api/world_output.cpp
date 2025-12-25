/**
 * world_output.cpp - Output Functions
 */

#include "lua_common.h"

/**
 * world.Note(text, ...)
 *
 * Displays a note in the output window using default colors.
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
 * Displays colored notes in the output window.
 * Supports multiple color blocks in one call.
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
 * Displays text in the output window without a trailing newline.
 * Uses default note colors.
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
 * Returns the ANSI escape sequence string for the given code(s).
 * Supports multiple arguments which are joined with semicolons.
 * Example: ANSI(1) returns "\027[1m" (bold)
 * Example: ANSI(1, 37) returns "\027[1;37m" (bold white)
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
 * Displays text containing ANSI escape codes in the output window.
 * The ANSI codes are interpreted and converted to colored output.
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
 * Displays a clickable hyperlink in the output window.
 *
 * @param action The command to send when clicked (required)
 * @param text The display text (optional, defaults to action)
 * @param hint Tooltip text (optional, defaults to action)
 * @param forecolour Foreground color (optional, defaults to hyperlink color)
 * @param backcolour Background color (optional, defaults to note background)
 * @param url If true, opens as URL in browser instead of sending to MUD (optional, defaults to
 * false)
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
 * Processes text as if it were received from the MUD.
 * ANSI codes are interpreted, triggers are fired, etc.
 *
 * @param text Text to simulate (may include ANSI escape sequences)
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
 * Displays colored text without a newline.
 * Supports multiple color blocks in one call.
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
 * Returns information about a specific line in the output buffer.
 *
 * Info types:
 *  1: text of line (string)
 *  2: length of text (number)
 *  3: true if newline (hard return), false if wrapped
 *  4: true if world.note (comment)
 *  5: true if player input
 *  6: true if line logged
 *  7: true if bookmarked
 *  8: true if horizontal rule (<hr>)
 *  9: date/time line arrived (number - seconds since epoch)
 * 10: actual line number (not position in buffer)
 * 11: count of style runs
 *
 * Returns nil if line number out of range or invalid info type.
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
 * Returns information about a specific style run in a line.
 *
 * Info types:
 *  1: text of style (string)
 *  2: length of style run
 *  3: starting column of style (1-based)
 *  4: action type - 0=none, 1=send to mud, 2=hyperlink, 3=prompt
 *  5: action (what to send)
 *  6: hint (tooltip text)
 *  7: variable (MXP variable to set)
 *  8: true if bold
 *  9: true if underlined
 * 10: true if blinking/italic
 * 11: true if inverse
 * 12: true if changed by trigger
 * 13: true if start of a tag
 * 14: foreground (text) colour in RGB
 * 15: background colour in RGB
 *
 * Returns nil if line/style number out of range or invalid info type.
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
 * Returns the most recent lines from the multiline trigger buffer as a string.
 * Lines are joined with newlines, matching original MUSHclient behavior.
 *
 * Based on methods_output.cpp GetRecentLines
 *
 * @param count Number of lines to return
 * @return String with lines joined by newlines, or empty string if no lines
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
 * Returns the current note color index (1-16), or -1 if in RGB mode.
 * Returns 0 if using "same color" mode.
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
 * Returns the current note foreground color as RGB.
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
 * Returns the current note background color as RGB.
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
 * Sets the note colors to specific RGB values.
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
 * Sets the note colors by color name (e.g., "red", "blue").
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
 * Returns the effective note foreground color as RGB.
 * Unlike NoteColourFore which returns the raw RGB value, this function
 * returns the actual color that would be used based on the current mode:
 * - If in RGB mode: returns m_iNoteColourFore
 * - If using SAMECOLOUR: returns custom color 16 (if Custom16isDefaultColour) or white
 * - Otherwise: returns the custom text color for the current index
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
 * Returns the effective note background color as RGB.
 * Unlike NoteColourBack which returns the raw RGB value, this function
 * returns the actual color that would be used based on the current mode:
 * - If in RGB mode: returns m_iNoteColourBack
 * - If using SAMECOLOUR: returns custom back 16 (if Custom16isDefaultColour) or black
 * - Otherwise: returns the custom back color for the current index
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
 * Sets the note color by index (0-16).
 * - 0 = SAMECOLOUR (use default)
 * - 1-16 = custom color index
 * This disables RGB mode.
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
 * Sets the note foreground color to a specific RGB value.
 * This enables RGB mode. If not already in RGB mode, the background
 * color is converted from the current index-based color to preserve it.
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
 * Sets the note background color to a specific RGB value.
 * This enables RGB mode. If not already in RGB mode, the foreground
 * color is converted from the current index-based color to preserve it.
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
 * Sets the note text style (combination of HILITE, UNDERLINE, BLINK, INVERSE).
 * Style values: 0=normal, 1=bold, 2=underline, 4=blink/italic, 8=inverse
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
 * Returns the current note text style.
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
 * Outputs a horizontal rule in the output window.
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
 * Displays text in the info bar.
 * Text is appended to the current content.
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
 * Clears the info bar content and resets to default formatting.
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
 * Sets the info bar text color by color name.
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
 * Sets the info bar background color by color name.
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
 * Sets the info bar font.
 * fontName: Font family name (empty string to keep current)
 * size: Font size in points (0 or negative to keep current)
 * style: Style bits - 1=bold, 2=italic, 4=underline, 8=strikeout
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
 * Shows or hides the info bar.
 * visible: true to show, false to hide
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
