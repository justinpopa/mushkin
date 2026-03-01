/**
 * world_misc.cpp - Miscellaneous Utility Functions
 */

#include "../../automation/alias.h"
#include "../../automation/trigger.h"
#include "../../storage/database.h"
#include "../../utils/app_paths.h"
#include "../xml_serialization.h"
#include "lua_common.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <algorithm>
#include <vector>

/**
 * world.Trim(text)
 *
 * Removes leading and trailing whitespace from text.
 *
 * @param text (string) String to trim
 *
 * @return (string) Trimmed string
 *
 * @example
 * local clean = Trim("  hello world  ")
 * Note("[" .. clean .. "]")  -- "[hello world]"
 */
int L_Trim(lua_State* L)
{
    luaPushQString(L, luaCheckQString(L, 1).trimmed());
    return 1;
}

/**
 * world.GetGlobalOption(name)
 *
 * Gets a global application option value from the preferences database.
 * These are application-wide settings, not per-world options.
 *
 * @param name (string) Option name (case-insensitive)
 *
 * @return (number|string|nil) Option value or nil if not found
 *
 * Common numeric options:
 *   - "AutoConnectWorlds": Auto-connect worlds on startup (0/1)
 *   - "ConfirmBeforeClosingWorld": Confirm before closing (0/1)
 *   - "AlwaysOnTop": Keep window on top (0/1)
 *   - "TrayIcon": Show system tray icon (0/1)
 *   - "SmoothScrolling": Enable smooth scrolling (0/1)
 *
 * Common string options:
 *   - "DefaultLogFileDirectory": Default log file directory
 *   - "DefaultWorldFileDirectory": Default world file directory
 *   - "PluginsDirectory": Plugins directory path
 *   - "DefaultInputFont": Input font name
 *   - "DefaultOutputFont": Output font name
 *
 * @example
 * local logDir = GetGlobalOption("DefaultLogFileDirectory")
 * Note("Logs saved to: " .. (logDir or "not set"))
 *
 * if GetGlobalOption("AutoConnectWorlds") == 1 then
 *     Note("Auto-connect is enabled")
 * end
 *
 * @see GetGlobalOptionList, GetOption, SetOption
 */
int L_GetGlobalOption(lua_State* L)
{
    QString qName = luaCheckQString(L, 1);

    auto& db = Database::instance();
    if (!db.isOpen()) {
        lua_pushnil(L);
        return 1;
    }

    // Numeric options (from GlobalOptionsTable in original)
    static const QSet<QString> numericOptions = {"AllTypingToCommandWindow",
                                                 "AlwaysOnTop",
                                                 "AppendToLogFiles",
                                                 "AutoConnectWorlds",
                                                 "AutoExpandConfig",
                                                 "FlatToolbars",
                                                 "AutoLogWorld",
                                                 "BleedBackground",
                                                 "ColourGradientConfig",
                                                 "ConfirmBeforeClosingMXPdebug",
                                                 "ConfirmBeforeClosingMushclient",
                                                 "ConfirmBeforeClosingWorld",
                                                 "ConfirmBeforeSavingVariables",
                                                 "ConfirmLogFileClose",
                                                 "AllowLoadingDlls",
                                                 "F1macro",
                                                 "FixedFontForEditing",
                                                 "NotepadWordWrap",
                                                 "NotifyIfCannotConnect",
                                                 "ErrorNotificationToOutputWindow",
                                                 "NotifyOnDisconnect",
                                                 "OpenActivityWindow",
                                                 "OpenWorldsMaximised",
                                                 "WindowTabsStyle",
                                                 "ReconnectOnLinkFailure",
                                                 "RegexpMatchEmpty",
                                                 "ShowGridLinesInListViews",
                                                 "SmoothScrolling",
                                                 "SmootherScrolling",
                                                 "DisableKeyboardMenuActivation",
                                                 "TriggerRemoveCheck",
                                                 "NotepadBackColour",
                                                 "NotepadTextColour",
                                                 "ActivityButtonBarStyle",
                                                 "AsciiArtLayout",
                                                 "DefaultInputFontHeight",
                                                 "DefaultInputFontItalic",
                                                 "DefaultInputFontWeight",
                                                 "DefaultOutputFontHeight",
                                                 "IconPlacement",
                                                 "TrayIcon",
                                                 "ActivityWindowRefreshInterval",
                                                 "ActivityWindowRefreshType",
                                                 "ParenMatchFlags",
                                                 "PrinterFontSize",
                                                 "PrinterLeftMargin",
                                                 "PrinterLinesPerPage",
                                                 "PrinterTopMargin",
                                                 "TimerInterval",
                                                 "FixedPitchFontSize",
                                                 "TabInsertsTabInMultiLineDialogs"};

    // Check if it's a numeric option
    if (numericOptions.contains(qName)) {
        int value = db.getPreferenceInt(qName, 0);
        lua_pushnumber(L, value);
        return 1;
    }

    // String options (from AlphaGlobalOptionsTable in original)
    QString value = db.getPreference(qName, QString());
    if (!value.isNull()) {
        luaPushQString(L, value);
        return 1;
    }

    // Not found
    lua_pushnil(L);
    return 1;
}

/**
 * world.GetGlobalOptionList()
 *
 * Returns a list of all available global option names.
 * Use GetGlobalOption() to retrieve values for these options.
 *
 * @return (table) Array of option name strings (1-indexed)
 *
 * @example
 * -- List all available global options
 * local options = GetGlobalOptionList()
 * for i, name in ipairs(options) do
 *     local value = GetGlobalOption(name)
 *     Note(name .. " = " .. tostring(value))
 * end
 *
 * @see GetGlobalOption
 */
int L_GetGlobalOptionList(lua_State* L)
{
    lua_newtable(L);
    int index = 1;

    // Numeric options (same list as in L_GetGlobalOption)
    static const char* numericOptions[] = {"AllTypingToCommandWindow",
                                           "AlwaysOnTop",
                                           "AppendToLogFiles",
                                           "AutoConnectWorlds",
                                           "AutoExpandConfig",
                                           "FlatToolbars",
                                           "AutoLogWorld",
                                           "BleedBackground",
                                           "ColourGradientConfig",
                                           "ConfirmBeforeClosingMXPdebug",
                                           "ConfirmBeforeClosingMushclient",
                                           "ConfirmBeforeClosingWorld",
                                           "ConfirmBeforeSavingVariables",
                                           "ConfirmLogFileClose",
                                           "AllowLoadingDlls",
                                           "F1macro",
                                           "FixedFontForEditing",
                                           "NotepadWordWrap",
                                           "NotifyIfCannotConnect",
                                           "ErrorNotificationToOutputWindow",
                                           "NotifyOnDisconnect",
                                           "OpenActivityWindow",
                                           "OpenWorldsMaximised",
                                           "WindowTabsStyle",
                                           "ReconnectOnLinkFailure",
                                           "RegexpMatchEmpty",
                                           "ShowGridLinesInListViews",
                                           "SmoothScrolling",
                                           "SmootherScrolling",
                                           "DisableKeyboardMenuActivation",
                                           "TriggerRemoveCheck",
                                           "NotepadBackColour",
                                           "NotepadTextColour",
                                           "ActivityButtonBarStyle",
                                           "AsciiArtLayout",
                                           "DefaultInputFontHeight",
                                           "DefaultInputFontItalic",
                                           "DefaultInputFontWeight",
                                           "DefaultOutputFontHeight",
                                           "IconPlacement",
                                           "TrayIcon",
                                           "ActivityWindowRefreshInterval",
                                           "ActivityWindowRefreshType",
                                           "ParenMatchFlags",
                                           "PrinterFontSize",
                                           "PrinterLeftMargin",
                                           "PrinterLinesPerPage",
                                           "PrinterTopMargin",
                                           "TimerInterval",
                                           "FixedPitchFontSize",
                                           "TabInsertsTabInMultiLineDialogs",
                                           nullptr};

    // String options (from AlphaGlobalOptionsTable)
    static const char* stringOptions[] = {"AsciiArtFont",
                                          "DefaultAliasesFile",
                                          "DefaultColoursFile",
                                          "DefaultInputFont",
                                          "DefaultLogFileDirectory",
                                          "DefaultMacrosFile",
                                          "DefaultNameGenerationFile",
                                          "DefaultOutputFont",
                                          "DefaultTimersFile",
                                          "DefaultTriggersFile",
                                          "DefaultWorldFileDirectory",
                                          "NotepadQuoteString",
                                          "PluginList",
                                          "PluginsDirectory",
                                          "StateFilesDirectory",
                                          "PrinterFont",
                                          "TrayIconFileName",
                                          "WordDelimiters",
                                          "WordDelimitersDblClick",
                                          "WorldList",
                                          "LuaScript",
                                          "Locale",
                                          "FixedPitchFont",
                                          nullptr};

    // Add numeric options
    for (int i = 0; numericOptions[i] != nullptr; i++) {
        lua_pushstring(L, numericOptions[i]);
        lua_rawseti(L, -2, index++);
    }

    // Add string options
    for (int i = 0; stringOptions[i] != nullptr; i++) {
        lua_pushstring(L, stringOptions[i]);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * world.EditDistance(source, target)
 *
 * Calculates the Damerau-Levenshtein edit distance between two strings.
 * This is the minimum number of single-character edits (insertions,
 * deletions, substitutions, or transpositions) needed to transform
 * one string into another.
 *
 * Useful for fuzzy string matching, spell checking, and finding
 * similar commands or names.
 *
 * @param source (string) First string to compare
 * @param target (string) Second string to compare
 *
 * @return (number) Number of edits needed (0 if identical)
 *   Note: Only first 20 characters are compared for performance.
 *
 * @example
 * -- Check similarity
 * local dist = EditDistance("hello", "hallo")
 * Note("Distance: " .. dist)  -- Output: 1
 *
 * -- Find closest match to user input
 * local commands = {"attack", "cast", "look", "inventory"}
 * local input = "atack"  -- typo
 * local closest, minDist = nil, 999
 * for _, cmd in ipairs(commands) do
 *     local d = EditDistance(input, cmd)
 *     if d < minDist then
 *         closest, minDist = cmd, d
 *     end
 * end
 * Note("Did you mean: " .. closest)  -- Output: attack
 *
 * @see utils.md5, utils.sha256
 */
int L_EditDistance(lua_State* L)
{
    const char* sourceStr = luaL_checkstring(L, 1);
    const char* targetStr = luaL_checkstring(L, 2);

    std::string source(sourceStr);
    std::string target(targetStr);

    // Keep maximum down to avoid performance issues
    const int MAX_LENGTH = 20;

    const int n = std::min(static_cast<int>(source.length()), MAX_LENGTH);
    const int m = std::min(static_cast<int>(target.length()), MAX_LENGTH);

    if (n == 0) {
        lua_pushinteger(L, m);
        return 1;
    }

    if (m == 0) {
        lua_pushinteger(L, n);
        return 1;
    }

    // Create matrix for dynamic programming
    std::vector<std::vector<int>> matrix(n + 1, std::vector<int>(m + 1));

    // Initialize first column and row
    for (int i = 0; i <= n; i++) {
        matrix[i][0] = i;
    }

    for (int j = 0; j <= m; j++) {
        matrix[0][j] = j;
    }

    // Fill in the matrix
    for (int i = 1; i <= n; i++) {
        const char s_i = source[i - 1];

        for (int j = 1; j <= m; j++) {
            const char t_j = target[j - 1];

            // Cost is 0 if characters match, 1 otherwise
            int cost = (s_i == t_j) ? 0 : 1;

            // Calculate minimum of deletion, insertion, substitution
            const int above = matrix[i - 1][j];
            const int left = matrix[i][j - 1];
            const int diag = matrix[i - 1][j - 1];
            int cell = std::min({above + 1, left + 1, diag + cost});

            // Also check transposition (Damerau-Levenshtein)
            if (i > 2 && j > 2) {
                int trans = matrix[i - 2][j - 2] + 1;
                if (source[i - 2] != t_j)
                    trans++;
                if (s_i != target[j - 2])
                    trans++;
                if (cell > trans)
                    cell = trans;
            }

            matrix[i][j] = cell;
        }
    }

    lua_pushinteger(L, matrix[n][m]);
    return 1;
}

/**
 * world.OpenBrowser(url)
 *
 * Opens a URL in the system's default web browser.
 * For security, only http://, https://, and mailto: URLs are allowed.
 *
 * @param url (string) URL to open
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eBadParameter (30): Invalid, empty, or disallowed URL scheme
 *   - eCouldNotOpenFile (30009): System failed to open URL
 *
 * @example
 * -- Open a website
 * OpenBrowser("https://www.mudconnect.com")
 *
 * -- Open email client
 * OpenBrowser("mailto:support@example.com")
 *
 * -- MXP-style link handler
 * function OnMXPLink(url)
 *     if url:match("^https?://") then
 *         OpenBrowser(url)
 *     end
 * end
 *
 * @see Execute
 */
int L_OpenBrowser(lua_State* L)
{
    QString url = luaCheckQString(L, 1);

    if (url.isEmpty()) {
        return luaReturnError(L, eBadParameter);
    }

    // Security: only allow http://, https://, and mailto: URLs
    if (!url.startsWith("http://", Qt::CaseInsensitive) &&
        !url.startsWith("https://", Qt::CaseInsensitive) &&
        !url.startsWith("mailto:", Qt::CaseInsensitive)) {
        return luaReturnError(L, eBadParameter);
    }

    // Open URL in default browser
    if (!QDesktopServices::openUrl(QUrl(url))) {
        return luaReturnError(L, eCouldNotOpenFile);
    }

    return luaReturnOK(L);
}

/**
 * world.ChangeDir(path)
 *
 * Changes the current working directory for the application.
 * Affects relative paths in subsequent file operations.
 *
 * @param path (string) Directory path to change to (absolute or relative)
 *
 * @return (boolean) true if successful, false if directory doesn't exist
 *
 * @example
 * -- Change to plugin directory
 * local success = ChangeDir(GetPluginInfo(GetPluginID(), 20))
 * if success then
 *     Note("Changed to plugin directory")
 * end
 *
 * -- Use absolute path
 * ChangeDir("/Users/player/muds/data")
 *
 * @see GetInfo
 */
int L_ChangeDir(lua_State* L)
{
    QString path = luaCheckQString(L, 1);

    bool success = QDir::setCurrent(path);
    lua_pushboolean(L, success);
    return 1;
}

/**
 * world.TranslateDebug(message)
 *
 * Calls a Debug function in a translator Lua script (if loaded).
 * Note: This feature is not implemented in the Qt version.
 *
 * @param message (string) Debug message to pass to translator
 *
 * @return (number) Status code:
 *   - 1: No translator script loaded (always returns this in Qt version)
 *
 * @see Trace, TraceOut
 */
int L_TranslateDebug(lua_State* L)
{
    // Translator feature not implemented in Qt version
    // Return 1 to indicate "no script"
    lua_pushinteger(L, 1);
    return 1;
}

/**
 * world.ImportXML(xml_string)
 *
 * Imports triggers, aliases, timers, and variables from an XML string.
 * Useful for migrating automation from existing MUSHclient files or
 * sharing configurations between worlds.
 *
 * Note: Does NOT import world configuration options (name, server, port, etc.)
 *
 * @param xml_string (string) XML content in MUSHclient format
 *
 * @return (number) Number of items imported (triggers + aliases + timers + variables)
 *   Returns -1 if XML is invalid or parsing fails.
 *
 * @example
 * -- Import from file
 * local file = io.open("my_triggers.xml", "r")
 * if file then
 *     local xml = file:read("*all")
 *     file:close()
 *     local count = ImportXML(xml)
 *     Note("Imported " .. count .. " items")
 * end
 *
 * -- Import inline XML
 * local xml = [[
 * <triggers>
 *   <trigger name="hp_warning" match="HP: (\d+)" ...>
 *   </trigger>
 * </triggers>
 * ]]
 * ImportXML(xml)
 *
 * @see ExportXML
 */
int L_ImportXML(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    size_t len;
    const char* xmlStr = luaL_checklstring(L, 1, &len);
    QString xmlString = QString::fromUtf8(xmlStr, len);

    int count = XmlSerialization::ImportXML(pDoc, xmlString);

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.ExportXML(flags, comment)
 *
 * Exports triggers, aliases, timers, and variables to an XML string.
 * Useful for sharing configurations or backing up automation items.
 *
 * @param flags (number, optional) Bitmask of item types to export:
 *   - 1: triggers
 *   - 2: aliases
 *   - 4: timers
 *   - 8: macros
 *   - 16: variables
 *   - 32: colours
 *   - 64: keypad
 *   - 128: printing
 *   Default: all (255)
 * @param comment (string, optional) Comment to include in XML header
 *
 * @return (string) XML string containing the exported items
 *
 * @example
 * -- Export all automation items
 * local xml = ExportXML()
 * local file = io.open("backup.xml", "w")
 * file:write(xml)
 * file:close()
 *
 * -- Export only triggers and aliases
 * local xml = ExportXML(1 + 2, "Combat automation v1.0")
 *
 * -- Export triggers for sharing
 * local triggers_only = ExportXML(1, "Trigger pack by MyName")
 *
 * @see ImportXML
 */
int L_ExportXML(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Get flags (default to all)
    int flags = luaL_optinteger(L, 1, XML_ALL);

    // Get optional comment
    QString qComment = luaOptQString(L, 2);

    QString xmlOutput = XmlSerialization::ExportXML(pDoc, flags, qComment);

    luaPushQString(L, xmlOutput);
    return 1;
}

/**
 * world.SpellCheck(Text)
 *
 * Spell checks text (deprecated - spell check removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param Text Text to spell check
 * @return Always returns empty table (spell check removed)
 */
int L_SpellCheck(lua_State* L)
{
    Q_UNUSED(L);
    lua_newtable(L);
    return 1;
}

/**
 * world.SpellCheckCommand(StartCol, EndCol)
 *
 * Spell checks command input (deprecated - spell check removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param StartCol Start column
 * @param EndCol End column
 * @return Always returns 0 (spell check removed)
 */
int L_SpellCheckCommand(lua_State* L)
{
    Q_UNUSED(L);
    lua_pushnumber(L, 0);
    return 1;
}

/**
 * world.AddSpellCheckWord(OriginalWord, ActionCode, ReplacementWord)
 *
 * Adds word to spell check dictionary (deprecated - spell check removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param OriginalWord Word to add
 * @param ActionCode Action code
 * @param ReplacementWord Replacement word
 * @return Always returns 0 (spell check removed)
 */
int L_AddSpellCheckWord(lua_State* L)
{
    Q_UNUSED(L);
    lua_pushnumber(L, 0);
    return 1;
}

/**
 * world.StripANSI(message)
 *
 * Strips ANSI escape sequences from text. Removes ESC[...m sequences
 * commonly used for colors and formatting in MUD output.
 *
 * Useful for logging, searching, or processing MUD text without formatting.
 *
 * @param message (string) Text containing ANSI escape codes
 *
 * @return (string) Text with all ANSI codes removed
 *
 * @example
 * -- Strip colors from MUD output for logging
 * local raw_text = "\27[31mYou are hurt!\27[0m"
 * local clean = StripANSI(raw_text)
 * Note(clean)  -- Output: "You are hurt!"
 *
 * -- Process text for pattern matching
 * function OnTriggerMatch(name, line, wildcards)
 *     local clean_line = StripANSI(line)
 *     -- Now pattern match without color codes
 * end
 *
 * @see FixupEscapeSequences, FixupHTML
 */
int L_StripANSI(lua_State* L)
{
    size_t len;
    const char* message = luaL_checklstring(L, 1, &len);
    QString result;

    const char* p = message;
    const char* start = p;
    const char ESC = '\x1B';

    while (*p) {
        if (*p == ESC) {
            // Append text before ESC
            if (p > start) {
                result += QString::fromUtf8(start, p - start);
            }

            p++; // Skip ESC

            // Handle ESC[...m sequences (ANSI color codes)
            if (*p == '[') {
                p++; // Skip [

                // Skip until we find 'm' or end of string
                while (*p && *p != 'm') {
                    p++;
                }

                if (*p) {
                    p++; // Skip 'm'
                }
            }

            start = p;
        } else {
            p++;
        }
    }

    // Append remaining text
    if (p > start) {
        result += QString::fromUtf8(start, p - start);
    }

    luaPushQString(L, result);
    return 1;
}

/**
 * world.FixupEscapeSequences(source)
 *
 * Converts C-style escape sequences to actual characters.
 * Useful when reading configuration files or processing user input
 * that may contain literal escape notation.
 *
 * Supported escape sequences:
 *   - \n: newline
 *   - \r: carriage return
 *   - \t: tab
 *   - \a: alert (bell)
 *   - \b: backspace
 *   - \f: form feed
 *   - \v: vertical tab
 *   - \\: backslash
 *   - \': single quote
 *   - \": double quote
 *   - \?: question mark
 *   - \xhh: hex character (e.g., \x1B for ESC)
 *
 * @param source (string) Text containing escape sequences
 *
 * @return (string) Text with escape sequences converted to actual characters
 *
 * @example
 * local text = FixupEscapeSequences("Hello\\nWorld")
 * Note(text)  -- Output: Hello
 *             --         World
 *
 * -- Insert special characters
 * local bell = FixupEscapeSequences("\\a")  -- Bell character
 * local esc = FixupEscapeSequences("\\x1B[31m")  -- ANSI red
 *
 * @see StripANSI, FixupHTML
 */
int L_FixupEscapeSequences(lua_State* L)
{
    size_t len;
    const char* source = luaL_checklstring(L, 1, &len);
    QString result;

    for (size_t i = 0; i < len; i++) {
        char c = source[i];

        if (c == '\\' && i + 1 < len) {
            i++; // Skip backslash
            c = source[i];

            switch (c) {
                case 'a':
                    result += '\a';
                    break; // Alert (bell)
                case 'b':
                    result += '\b';
                    break; // Backspace
                case 'f':
                    result += '\f';
                    break; // Formfeed
                case 'n':
                    result += '\n';
                    break; // Newline
                case 'r':
                    result += '\r';
                    break; // Carriage return
                case 't':
                    result += '\t';
                    break; // Tab
                case 'v':
                    result += '\v';
                    break; // Vertical tab
                case '\'':
                    result += '\'';
                    break; // Single quote
                case '\"':
                    result += '\"';
                    break; // Double quote
                case '\\':
                    result += '\\';
                    break; // Backslash
                case '?':
                    result += '\?';
                    break;  // Question mark
                case 'x': { // Hex escape \xhh
                    int value = 0;
                    i++; // Move past 'x'

                    // Read up to 2 hex digits
                    for (int j = 0; j < 2 && i < len; j++) {
                        char digit = source[i];
                        if (digit >= '0' && digit <= '9') {
                            value = (value << 4) + (digit - '0');
                            i++;
                        } else if (digit >= 'A' && digit <= 'F') {
                            value = (value << 4) + (digit - 'A' + 10);
                            i++;
                        } else if (digit >= 'a' && digit <= 'f') {
                            value = (value << 4) + (digit - 'a' + 10);
                            i++;
                        } else {
                            break; // Not a hex digit
                        }
                    }
                    i--; // Back up one since loop will increment

                    result += QChar(value);
                    break;
                }
                default:
                    // Unknown escape, keep the backslash
                    result += '\\';
                    result += c;
                    break;
            }
        } else {
            result += c;
        }
    }

    luaPushQString(L, result);
    return 1;
}

/**
 * world.FixupHTML(string_to_convert)
 *
 * HTML entity encoding - converts special characters to HTML entities
 * to prevent XSS vulnerabilities and ensure proper display.
 *
 * Conversions:
 *   - < → &lt;
 *   - > → &gt;
 *   - & → &amp;
 *   - " → &quot;
 *
 * @param string_to_convert (string) Text to encode
 *
 * @return (string) HTML-safe encoded text
 *
 * @example
 * -- Escape user input for HTML display
 * local userInput = "<script>alert('xss')</script>"
 * local safe = FixupHTML(userInput)
 * Note(safe)  -- Output: &lt;script&gt;alert('xss')&lt;/script&gt;
 *
 * -- Use in MXP/HTML output
 * local name = "Player<1>"
 * Tell(FixupHTML(name) .. " says hello")
 *
 * @see StripANSI, FixupEscapeSequences
 */
int L_FixupHTML(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);
    QString source = QString::fromUtf8(text, len);
    QString result;

    for (const QChar& ch : source) {
        switch (ch.unicode()) {
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            case '\"':
                result += "&quot;";
                break;
            default:
                result += ch;
                break;
        }
    }

    luaPushQString(L, result);
    return 1;
}

/**
 * world.MakeRegularExpression(text)
 *
 * Converts literal text to a regular expression by escaping special
 * regex metacharacters. The result anchors the pattern to match the
 * entire line (adds ^ at start and $ at end).
 *
 * Escapes: . * + ? ^ $ { } [ ] ( ) | \
 *
 * @param text (string) Literal text to convert
 *
 * @return (string) Regular expression pattern with escaped metacharacters
 *
 * @example
 * -- User wants to match literal "[HP: 100]"
 * local literal = "[HP: 100]"
 * local pattern = MakeRegularExpression(literal)
 * Note(pattern)  -- Output: ^\[HP: 100\]$
 *
 * -- Use escaped pattern in trigger
 * local userText = GetVariable("match_text")
 * AddTrigger("user_match", MakeRegularExpression(userText), ...)
 *
 * @see AddTrigger, AddAlias
 */
int L_MakeRegularExpression(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);
    QString source = QString::fromUtf8(text, len);
    QString result;

    // Add ^ at start for whole-line matching
    result += '^';

    for (const QChar& ch : source) {
        unsigned char c = ch.unicode() & 0xFF;

        if (c == '\n') {
            // Newline becomes \n
            result += "\\n";
        } else if (c < ' ') {
            // Non-printable characters become \xhh
            result += QString("\\x%1").arg(c, 2, 16, QChar('0'));
        } else if (ch.isLetterOrNumber() || ch == ' ' || c >= 0x80) {
            // Alphanumeric, spaces, high-bit characters pass through
            result += ch;
        } else {
            // Escape special regex characters
            result += '\\';
            result += ch;
        }
    }

    // Add $ at end for whole-line matching
    result += '$';

    luaPushQString(L, result);
    return 1;
}

// Include headers for group operations
#include "../../automation/alias.h"
#include "../../automation/timer.h"
#include "../../automation/trigger.h"

/**
 * world.EnableGroup(group_name, enabled)
 *
 * Enables or disables all triggers, aliases, and timers in a named group.
 * Groups provide a convenient way to organize related automation items.
 *
 * @param group_name (string) Name of the group (case-sensitive)
 * @param enabled (boolean, optional) true to enable, false to disable. Default: true
 *
 * @return (number) Total count of items affected (triggers + aliases + timers)
 *
 * @example
 * -- Disable combat automation when out of combat
 * function OnCombatEnd()
 *     local count = EnableGroup("combat", false)
 *     Note("Disabled " .. count .. " combat items")
 * end
 *
 * -- Re-enable when combat starts
 * function OnCombatStart()
 *     EnableGroup("combat", true)
 * end
 *
 * -- Toggle a group
 * local enabled = GetVariable("social_enabled") == "1"
 * EnableGroup("social_triggers", not enabled)
 * SetVariable("social_enabled", enabled and "0" or "1")
 *
 * @see DeleteGroup, EnableTriggerGroup, EnableAliasGroup, EnableTimerGroup
 */
int L_EnableGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool enabled = lua_isnone(L, 2) ? true : lua_toboolean(L, 2);

    QString qGroupName = luaCheckQString(L, 1);

    if (qGroupName.isEmpty()) {
        lua_pushnumber(L, 0);
        return 1;
    }

    long count = 0;

    // Enable/disable triggers in the group
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();
        if (trigger && trigger->group == qGroupName) {
            trigger->enabled = enabled;
            count++;
        }
    }

    // Enable/disable aliases in the group
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        Alias* alias = aliasPtr.get();
        if (alias && alias->group == qGroupName) {
            alias->enabled = enabled;
            count++;
        }
    }

    // Enable/disable timers in the group
    for (const auto& [name, timerPtr] : pDoc->m_automationRegistry->m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (timer && timer->group == qGroupName) {
            timer->enabled = enabled;
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.DeleteGroup(group_name)
 *
 * Deletes all triggers, aliases, and timers in a named group.
 * This permanently removes all items in the group - use with caution.
 *
 * @param group_name (string) Name of the group to delete (case-sensitive)
 *
 * @return (number) Total count of items deleted (triggers + aliases + timers)
 *
 * @example
 * -- Clean up temporary automation
 * local count = DeleteGroup("temp_quest")
 * Note("Cleaned up " .. count .. " quest items")
 *
 * -- Uninstall a feature
 * function UninstallCombatModule()
 *     DeleteGroup("combat_triggers")
 *     DeleteGroup("combat_aliases")
 *     DeleteGroup("combat_timers")
 *     Note("Combat module removed")
 * end
 *
 * @see EnableGroup, DeleteTrigger, DeleteAlias, DeleteTimer
 */
int L_DeleteGroup(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    QString qGroupName = luaCheckQString(L, 1);

    if (qGroupName.isEmpty()) {
        lua_pushnumber(L, 0);
        return 1;
    }

    long count = 0;

    // Collect triggers to delete (can't modify map while iterating)
    QStringList triggersToDelete;
    for (const auto& [name, triggerPtr] : pDoc->m_automationRegistry->m_TriggerMap) {
        Trigger* trigger = triggerPtr.get();
        if (trigger && trigger->group == qGroupName) {
            triggersToDelete.append(name);
        }
    }
    for (const QString& name : triggersToDelete) {
        (void)pDoc->deleteTrigger(name); // intentional: bulk group delete
        count++;
    }

    // Collect aliases to delete
    QStringList aliasesToDelete;
    for (const auto& [name, aliasPtr] : pDoc->m_automationRegistry->m_AliasMap) {
        Alias* alias = aliasPtr.get();
        if (alias && alias->group == qGroupName) {
            aliasesToDelete.append(name);
        }
    }
    for (const QString& name : aliasesToDelete) {
        (void)pDoc->deleteAlias(name); // intentional: bulk group delete
        count++;
    }

    // Collect timers to delete
    QStringList timersToDelete;
    for (const auto& [name, timerPtr] : pDoc->m_automationRegistry->m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (timer && timer->group == qGroupName) {
            timersToDelete.append(name);
        }
    }
    for (const QString& name : timersToDelete) {
        (void)pDoc->deleteTimer(name); // intentional: bulk group delete
        count++;
    }

    lua_pushnumber(L, count);
    return 1;
}

/**
 * world.ErrorDesc(code)
 *
 * Returns a human-readable description for an error code.
 * Useful for debugging and displaying meaningful error messages.
 *
 * @param code (number) Error code returned by API functions
 *
 * @return (string) Description of the error, or "Unknown error code: N"
 *
 * Common error codes:
 *   - 0 (eOK): No error
 *   - 9 (eTriggerNotFound): Trigger not found
 *   - 14 (eAliasNotFound): Alias not found
 *   - 25 (eTimerNotFound): Timer not found
 *   - 27 (eVariableNotFound): Variable not found
 *   - 30 (eBadParameter): Bad parameter
 *   - 32 (ePluginNotInstalled): Plugin not found
 *   - 34 (eMiniWindowNotFound): Miniwindow not found
 *
 * @example
 * local result = DeleteTrigger("nonexistent")
 * if result ~= error_code.eOK then
 *     Note("Error: " .. ErrorDesc(result))
 *     -- Output: "Error: Trigger not found"
 * end
 *
 * @see error_code
 */
int L_ErrorDesc(lua_State* L)
{
    int code = luaL_checkinteger(L, 1);

    static const char* errorDescriptions[] = {
        "No error",                          // 0 eOK
        "The world is currently closed",     // 1 eWorldClosed
        "The world is not open for reading", // 2 eWorldOpen
        "No such file exists",               // 3 eNoNameSpecified
        "Cannot open that file",             // 4 eCannotOpenFile
        "Log file not open",                 // 5 eLogFileNotOpen
        "Log file already open",             // 6 eLogFileAlreadyOpen
        "Log file not specified",            // 7 eLogFileBadWrite
        "Bad regular expression",            // 8 eBadRegularExpression
        "Trigger not found",                 // 9 eTriggerNotFound
        "Trigger already exists",            // 10 eTriggerAlreadyExists
        "Trigger cannot be empty",           // 11 eTriggerCannotBeEmpty
        "Invalid object label",              // 12 eInvalidObjectLabel
        "Script function not found",         // 13 eScriptNameNotLocated
        "Alias not found",                   // 14 eAliasNotFound
        "Alias already exists",              // 15 eAliasAlreadyExists
        "Alias cannot be empty",             // 16 eAliasCannotBeEmpty
        "Cannot write to file",              // 17 eCouldNotSaveWorld
        "Plugin not installed",              // 18 ePluginFileNotFound
        "World is closed",                   // 19 eWorldClosed2
        "Invalid command",                   // 20 eInvalidCommand
        "Unexpected command",                // 21 eUnexpectedCommand
        "Array is not defined",              // 22 eNoArraySpace
        "Bad parameter",                     // 23 eBadMapItem
        "Foreground is same as background",  // 24 eNoMapItems
        "Timer not found",                   // 25 eTimerNotFound
        "Timer already exists",              // 26 eTimerAlreadyExists
        "Variable not found",                // 27 eVariableNotFound
        "Command not empty",                 // 28 eCommandNotEmpty
        "Bad syntax for regular expression", // 29 eBadRegularExpressionSyntax
        "Timer cannot fire on zero time",    // 30 eTimeInvalid
        "Bad parameter",                     // 31 eBadParameter
        "Plugin not found",                  // 32 ePluginNotInstalled
        "Plugin is disabled",                // 33 ePluginDisabled
        "Miniwindow not found",              // 34 eMiniWindowNotFound
        "Bad key",                           // 35 eBadKey
        "Hotspot already exists",            // 36 eHotspotPluginChanged
        "Hotspot not found",                 // 37 eHotspotNotInstalled
        "Plugin ID required",                // 38 eNoSuchPlugin
        "Miniwindow hotspot not found",      // 39 eHotspotNotFound
        "No such window",                    // 40 eNoSuchWindow
        "Broadcast method not supported",    // 41 eBroadcastsDisabled
        "Plugin cannot set option",          // 42 ePluginCannotSetOption
        "Plugin is not active",              // 43 ePluginInactive
        "No such font",                      // 44 eNoSuchFont
        "No such image",                     // 45 eImageNotFound
    };

    int numDescriptions = sizeof(errorDescriptions) / sizeof(errorDescriptions[0]);

    if (code >= 0 && code < numDescriptions) {
        lua_pushstring(L, errorDescriptions[code]);
    } else {
        lua_pushfstring(L, "Unknown error code: %d", code);
    }
    return 1;
}

/**
 * world.Replace(source, search_for, replace_with, multiple)
 *
 * Replaces occurrences of a substring with another string.
 * Case-sensitive matching.
 *
 * @param source (string) The source string to search
 * @param search_for (string) The string to search for
 * @param replace_with (string) The replacement string
 * @param multiple (boolean, optional) true to replace all occurrences,
 *   false to replace only the first. Default: false
 *
 * @return (string) The modified string
 *
 * @example
 * -- Replace first occurrence
 * local text = Replace("hello hello", "hello", "hi")
 * Note(text)  -- Output: "hi hello"
 *
 * -- Replace all occurrences
 * local text = Replace("hello hello", "hello", "hi", true)
 * Note(text)  -- Output: "hi hi"
 *
 * -- Clean up MUD output
 * local clean = Replace(line, "  ", " ", true)  -- Remove double spaces
 *
 * @see string.gsub
 */
int L_Replace(lua_State* L)
{
    QString qSource = luaCheckQString(L, 1);
    QString qSearchFor = luaCheckQString(L, 2);
    QString qReplaceWith = luaCheckQString(L, 3);
    bool multiple = lua_toboolean(L, 4);

    QString result;
    if (multiple) {
        result = qSource.replace(qSearchFor, qReplaceWith);
    } else {
        // Replace only first occurrence
        int pos = qSource.indexOf(qSearchFor);
        if (pos >= 0) {
            result = qSource.left(pos) + qReplaceWith + qSource.mid(pos + qSearchFor.length());
        } else {
            result = qSource;
        }
    }

    luaPushQString(L, result);
    return 1;
}

/**
 * isValidCompletionName - Check if string is valid for tab completion
 *
 * Based on IsValidName() from mxp/mxputils.cpp
 *
 * Rules:
 * - Must start with a letter
 * - Can only contain alphanumeric, underscore, hyphen, or period
 */
static bool isValidCompletionName(const char* str)
{
    if (!str || !*str) {
        return false;
    }

    // Must start with a letter
    if (!isalpha(static_cast<unsigned char>(*str))) {
        return false;
    }

    for (const char* p = str; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            continue;
        }
        return false;
    }

    return true;
}

/**
 * world.ShiftTabCompleteItem(item)
 *
 * Adds or manages items for the Shift+Tab completion menu.
 * This allows plugins to add custom completions for user convenience.
 *
 * @param item (string) Item to add or special command:
 *   - "<clear>": Clear all extra completion items
 *   - "<functions>": Enable showing Lua functions in menu
 *   - "<nofunctions>": Disable showing Lua functions in menu
 *   - Other: Add this as a completion item (must be valid name: starts with
 *     letter, contains only alphanumeric, underscore, hyphen, or period)
 *
 * @return (number) Error code:
 *   - eOK (0): Success
 *   - eBadParameter (30): Invalid item (empty, too long, or invalid characters)
 *
 * @example
 * -- Add custom completions for mob names
 * ShiftTabCompleteItem("goblin.warrior")
 * ShiftTabCompleteItem("goblin.shaman")
 * ShiftTabCompleteItem("orc-chief")
 *
 * -- Clear and rebuild the list
 * ShiftTabCompleteItem("<clear>")
 * for _, name in ipairs(mob_names) do
 *     ShiftTabCompleteItem(name)
 * end
 *
 * -- Disable function suggestions for simpler menu
 * ShiftTabCompleteItem("<nofunctions>")
 *
 * @see GetCommand
 */
int L_ShiftTabCompleteItem(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* item = luaL_checkstring(L, 1);
    size_t len = strlen(item);

    // Validate length (1-30 characters)
    if (len == 0 || len > 30) {
        return luaReturnError(L, eBadParameter);
    }

    if (strcmp(item, "<clear>") == 0) {
        // Clear the extra items list
        pDoc->m_ExtraShiftTabCompleteItems.clear();
    } else if (strcmp(item, "<functions>") == 0) {
        // Enable function display
        pDoc->m_scripting.tab_complete_functions = true;
    } else if (strcmp(item, "<nofunctions>") == 0) {
        // Disable function display
        pDoc->m_scripting.tab_complete_functions = false;
    } else {
        // Validate name (alphanumeric, dot, hyphen, underscore, starts with letter)
        if (!isValidCompletionName(item)) {
            return luaReturnError(L, eBadParameter);
        }
        // Add item to the completion list
        pDoc->m_ExtraShiftTabCompleteItems.insert(QString::fromUtf8(item));
    }

    return luaReturnOK(L);
}

/**
 * world.GetTriggerWildcard(name, wildcard)
 *
 * Returns the value of a wildcard from the last trigger match.
 * Wildcards are captured groups from the trigger's regex pattern.
 *
 * @param name (string) Trigger name (case-insensitive)
 * @param wildcard (string) Wildcard identifier:
 *   - Numeric: "0" (full match), "1", "2", ... (capture groups)
 *   - Named: Named capture group name
 *
 * @return (string|nil) Wildcard value, or nil if trigger/wildcard not found
 *
 * @example
 * -- Trigger pattern: "HP: (\d+)/(\d+)"
 * -- Get captured values later
 * local current = GetTriggerWildcard("hp_trigger", "1")
 * local max = GetTriggerWildcard("hp_trigger", "2")
 * Note("HP: " .. current .. "/" .. max)
 *
 * -- Get full matched text
 * local full = GetTriggerWildcard("hp_trigger", "0")
 *
 * @see GetAliasWildcard, AddTrigger
 */
int L_GetTriggerWildcard(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString name = luaCheckQString(L, 1).trimmed().toLower();
    QString wcName = luaCheckQString(L, 2);

    // Find the trigger
    auto it = pDoc->m_automationRegistry->m_TriggerMap.find(name);
    if (it == pDoc->m_automationRegistry->m_TriggerMap.end()) {
        lua_pushnil(L);
        return 1;
    }

    Trigger* trigger = it->second.get();

    // Try to get wildcard by name or number
    // Try as number first
    bool ok;
    int wcNum = wcName.toInt(&ok);
    if (ok && wcNum >= 0 && wcNum < trigger->wildcards.size()) {
        luaPushQString(L, trigger->wildcards[wcNum]);
        return 1;
    }

    // Named captures would require the regex to have stored them - for now return nil
    lua_pushnil(L);
    return 1;
}

/**
 * world.GetAliasWildcard(name, wildcard)
 *
 * Returns the value of a wildcard from the last alias match.
 * Wildcards are captured groups from the alias's regex pattern.
 *
 * @param name (string) Alias name (case-insensitive)
 * @param wildcard (string) Wildcard identifier:
 *   - Numeric: "0" (full match), "1", "2", ... (capture groups)
 *   - Named: Named capture group name
 *
 * @return (string|nil) Wildcard value, or nil if alias/wildcard not found
 *
 * @example
 * -- Alias pattern: "^cast (\w+) (?:at |on )?(.*)$"
 * -- Later retrieve values
 * local spell = GetAliasWildcard("cast_alias", "1")
 * local target = GetAliasWildcard("cast_alias", "2")
 * Note("Casting " .. spell .. " at " .. target)
 *
 * @see GetTriggerWildcard, AddAlias
 */
int L_GetAliasWildcard(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString name = luaCheckQString(L, 1).trimmed().toLower();
    QString wcName = luaCheckQString(L, 2);

    // Find the alias
    auto it = pDoc->m_automationRegistry->m_AliasMap.find(name);
    if (it == pDoc->m_automationRegistry->m_AliasMap.end()) {
        lua_pushnil(L);
        return 1;
    }

    Alias* alias = it->second.get();

    // Try to get wildcard by name or number
    // Try as number first
    bool ok;
    int wcNum = wcName.toInt(&ok);
    if (ok && wcNum >= 0 && wcNum < alias->wildcards.size()) {
        luaPushQString(L, alias->wildcards[wcNum]);
        return 1;
    }

    // Named captures would require the regex to have stored them - for now return nil
    lua_pushnil(L);
    return 1;
}

/**
 * world.GetNotes()
 *
 * Returns the world's notes/comments text.
 * These are the free-form notes stored with the world file.
 *
 * @return (string) World notes text
 *
 * @example
 * local notes = GetNotes()
 * if notes ~= "" then
 *     Note("World notes:\n" .. notes)
 * end
 *
 * @see SetNotes
 */
int L_GetNotes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    luaPushQString(L, pDoc->m_notes);
    return 1;
}

/**
 * world.SetNotes(notes)
 *
 * Sets the world's notes/comments text.
 * Marks the document as modified (will prompt to save on exit).
 *
 * @param notes (string) New notes text
 *
 * @example
 * SetNotes("This world is for testing combat scripts")
 *
 * -- Append to existing notes
 * local current = GetNotes()
 * SetNotes(current .. "\n" .. os.date() .. ": Session started")
 *
 * @see GetNotes
 */
int L_SetNotes(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_notes = luaCheckQString(L, 1);
    pDoc->setModified(true);
    return 0;
}

/**
 * world.DeleteCommandHistory()
 *
 * Clears all command history.
 * The up/down arrow recall will be empty after this.
 *
 * @example
 * -- Clear history for privacy
 * DeleteCommandHistory()
 * Note("Command history cleared")
 *
 * @see GetCommandList
 */
int L_DeleteCommandHistory(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->clearCommandHistory();
    return 0;
}

/**
 * world.PushCommand()
 *
 * Gets the current command from the input field, adds it to history,
 * clears the input field, and returns the command text.
 * Useful for capturing user input before processing it.
 *
 * @return (string) The command text that was in the input field
 *
 * @example
 * -- Capture and process command
 * local cmd = PushCommand()
 * if cmd:sub(1,1) == "/" then
 *     -- Process as local command
 *     processLocalCommand(cmd:sub(2))
 * else
 *     Send(cmd)
 * end
 *
 * @see GetCommand, SetCommand, GetCommandList
 */
int L_PushCommand(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    luaPushQString(L, pDoc->PushCommand());
    return 1;
}

/**
 * world.SetChanged(changed)
 *
 * Sets the document's modified flag.
 * When true, will prompt to save on exit.
 *
 * @param changed (boolean, optional) true to mark as modified, false to mark as saved.
 *   Default: true
 *
 * @example
 * -- Mark world as needing save
 * SetChanged(true)
 *
 * -- Mark world as saved (suppress save prompt)
 * SetChanged(false)
 *
 * @see Save
 */
int L_SetChanged(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true if no argument provided (matches original optboolean behavior)
    bool changed = lua_isnone(L, 1) ? true : lua_toboolean(L, 1);
    pDoc->setModified(changed);
    return 0;
}

// ========== Random Number Functions ==========

// Global Mersenne Twister RNG instance
static QRandomGenerator s_mtRng;

/**
 * world.MtSrand(seed)
 *
 * Seeds the Mersenne Twister random number generator.
 * Useful for reproducible random sequences in testing or procedural content.
 *
 * @param seed (number|table, optional) Seed value or table of seed values.
 *   If table, values are XOR'd together. Default: current time in milliseconds.
 *
 * @example
 * -- Seed with specific value for reproducible results
 * MtSrand(12345)
 * local r1 = MtRand()  -- Always same value for seed 12345
 *
 * -- Seed with current time (default behavior)
 * MtSrand()
 *
 * -- Seed with multiple values
 * MtSrand({os.time(), GetUniqueNumber(), 42})
 *
 * @see MtRand
 */
int L_MtSrand(lua_State* L)
{
    if (lua_istable(L, 1)) {
        // Table of seeds - combine them
        quint32 combinedSeed = 0;
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            if (lua_isnumber(L, -1)) {
                combinedSeed ^= static_cast<quint32>(lua_tonumber(L, -1));
            }
            lua_pop(L, 1);
        }
        s_mtRng.seed(combinedSeed);
    } else {
        // Single seed value
        quint32 seed =
            static_cast<quint32>(luaL_optinteger(L, 1, QDateTime::currentMSecsSinceEpoch()));
        s_mtRng.seed(seed);
    }
    return 0;
}

/**
 * world.MtRand()
 *
 * Returns a random number from the Mersenne Twister RNG.
 * The Mersenne Twister provides high-quality random numbers with a very
 * long period (2^19937 - 1).
 *
 * @return (number) Random double in range [0, 1)
 *
 * @example
 * -- Random float
 * local r = MtRand()  -- 0.0 to 0.999...
 *
 * -- Random integer 1-100
 * local d100 = math.floor(MtRand() * 100) + 1
 *
 * -- Random percentage check
 * if MtRand() < 0.25 then
 *     Note("25% chance occurred!")
 * end
 *
 * @see MtSrand
 */
int L_MtRand(lua_State* L)
{
    Q_UNUSED(L);
    // Generate random double in [0, 1)
    double value = s_mtRng.generateDouble();
    lua_pushnumber(L, value);
    return 1;
}

/**
 * world.GetScriptTime()
 *
 * Returns the total time spent executing scripts in seconds.
 * Useful for performance profiling and identifying slow scripts.
 *
 * @return (number) Total script execution time in seconds (double precision)
 *
 * @example
 * local before = GetScriptTime()
 * -- Run expensive operation
 * expensiveFunction()
 * local after = GetScriptTime()
 * Note(string.format("Operation took %.3f seconds", after - before))
 *
 * -- Monitor total script time
 * Note("Total script time: " .. GetScriptTime() .. " seconds")
 *
 * @see GetInfo
 */
int L_GetScriptTime(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // m_iScriptTimeTaken is in nanoseconds, convert to seconds
    double seconds = static_cast<double>(pDoc->m_iScriptTimeTaken) / 1000000000.0;
    lua_pushnumber(L, seconds);
    return 1;
}

// ========== Pixel Manipulation Functions ==========

// Helper to clamp a value to 0-255 range
static inline int clamp255(int v)
{
    return qBound(0, v, 255);
}

// Helper to clamp a double to 0-255 range
static inline int clamp255d(double v)
{
    return qBound(0, static_cast<int>(v), 255);
}

// Helper to convert RGB to HSL
static void rgbToHsl(int r, int g, int b, double& h, double& s, double& l)
{
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;

    double maxC = qMax(rd, qMax(gd, bd));
    double minC = qMin(rd, qMin(gd, bd));
    double delta = maxC - minC;

    l = (maxC + minC) / 2.0;

    if (delta < 0.00001) {
        s = 0;
        h = 0;
        return;
    }

    s = l < 0.5 ? delta / (maxC + minC) : delta / (2.0 - maxC - minC);

    if (rd >= maxC)
        h = (gd - bd) / delta;
    else if (gd >= maxC)
        h = 2.0 + (bd - rd) / delta;
    else
        h = 4.0 + (rd - gd) / delta;

    h *= 60.0;
    if (h < 0)
        h += 360.0;
}

// Helper to convert HSL to RGB
static void hslToRgb(double h, double s, double l, int& r, int& g, int& b)
{
    if (s < 0.00001) {
        r = g = b = static_cast<int>(l * 255);
        return;
    }

    auto hueToRgb = [](double p, double q, double t) -> double {
        if (t < 0)
            t += 1;
        if (t > 1)
            t -= 1;
        if (t < 1.0 / 6.0)
            return p + (q - p) * 6.0 * t;
        if (t < 0.5)
            return q;
        if (t < 2.0 / 3.0)
            return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
        return p;
    };

    double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    double p = 2 * l - q;
    double hk = h / 360.0;

    r = static_cast<int>(hueToRgb(p, q, hk + 1.0 / 3.0) * 255);
    g = static_cast<int>(hueToRgb(p, q, hk) * 255);
    b = static_cast<int>(hueToRgb(p, q, hk - 1.0 / 3.0) * 255);
}

/**
 * world.BlendPixel(blend, base, mode, opacity)
 *
 * Blends two pixels using one of 64 blend modes.
 *
 * Based on methods_miniwindows.cpp BlendPixel function.
 * Reference: https://www.gammon.com.au/scripts/doc.php?function=BlendPixel
 *
 * @param blend RGB color code of the pixel to blend
 * @param base RGB color code of the base pixel
 * @param mode Blend mode (1-64)
 * @param opacity Blend opacity (0.0-1.0)
 * @return Blended RGB color, or -1 for invalid mode, -2 for invalid opacity
 */
int L_BlendPixel(lua_State* L)
{
    qint32 blendColor = luaL_checkinteger(L, 1);
    qint32 baseColor = luaL_checkinteger(L, 2);
    qint16 mode = luaL_checkinteger(L, 3);
    double opacity = luaL_checknumber(L, 4);

    // Validate mode
    if (mode < 1 || mode > 64) {
        lua_pushinteger(L, -1);
        return 1;
    }

    // Validate opacity
    if (opacity < 0.0 || opacity > 1.0) {
        lua_pushinteger(L, -2);
        return 1;
    }

    // Extract RGB components from blend color (BGR format)
    int bB = (blendColor >> 16) & 0xFF;
    int bG = (blendColor >> 8) & 0xFF;
    int bR = blendColor & 0xFF;

    // Extract RGB components from base color (BGR format)
    int aB = (baseColor >> 16) & 0xFF;
    int aG = (baseColor >> 8) & 0xFF;
    int aR = baseColor & 0xFF;

    // Result components
    int rR, rG, rB;

    switch (mode) {
        case 1: // Normal
            rR = bR;
            rG = bG;
            rB = bB;
            break;

        case 2: // Average
            rR = (aR + bR) / 2;
            rG = (aG + bG) / 2;
            rB = (aB + bB) / 2;
            break;

        case 3: // Interpolate (same as average for static blend)
            rR = (aR + bR) / 2;
            rG = (aG + bG) / 2;
            rB = (aB + bB) / 2;
            break;

        case 4: // Dissolve (random per pixel - just use blend for single pixel)
            rR = bR;
            rG = bG;
            rB = bB;
            break;

        case 5: // Darken
            rR = qMin(aR, bR);
            rG = qMin(aG, bG);
            rB = qMin(aB, bB);
            break;

        case 6: // Multiply
            rR = (aR * bR) / 255;
            rG = (aG * bG) / 255;
            rB = (aB * bB) / 255;
            break;

        case 7: // Colour Burn
            rR = bR == 0 ? 0 : clamp255(255 - ((255 - aR) * 255 / bR));
            rG = bG == 0 ? 0 : clamp255(255 - ((255 - aG) * 255 / bG));
            rB = bB == 0 ? 0 : clamp255(255 - ((255 - aB) * 255 / bB));
            break;

        case 8: // Linear Burn
            rR = clamp255(aR + bR - 255);
            rG = clamp255(aG + bG - 255);
            rB = clamp255(aB + bB - 255);
            break;

        case 9: // Inverse Colour Burn
            rR = aR == 0 ? 0 : clamp255(255 - ((255 - bR) * 255 / aR));
            rG = aG == 0 ? 0 : clamp255(255 - ((255 - bG) * 255 / aG));
            rB = aB == 0 ? 0 : clamp255(255 - ((255 - bB) * 255 / aB));
            break;

        case 10: // Subtract
            rR = clamp255(aR - bR);
            rG = clamp255(aG - bG);
            rB = clamp255(aB - bB);
            break;

        case 11: // Lighten
            rR = qMax(aR, bR);
            rG = qMax(aG, bG);
            rB = qMax(aB, bB);
            break;

        case 12: // Screen
            rR = 255 - ((255 - aR) * (255 - bR) / 255);
            rG = 255 - ((255 - aG) * (255 - bG) / 255);
            rB = 255 - ((255 - aB) * (255 - bB) / 255);
            break;

        case 13: // Colour Dodge
            rR = bR == 255 ? 255 : clamp255((aR * 255) / (255 - bR));
            rG = bG == 255 ? 255 : clamp255((aG * 255) / (255 - bG));
            rB = bB == 255 ? 255 : clamp255((aB * 255) / (255 - bB));
            break;

        case 14: // Linear Dodge (Add)
            rR = clamp255(aR + bR);
            rG = clamp255(aG + bG);
            rB = clamp255(aB + bB);
            break;

        case 15: // Inverse Colour Dodge
            rR = aR == 255 ? 255 : clamp255((bR * 255) / (255 - aR));
            rG = aG == 255 ? 255 : clamp255((bG * 255) / (255 - aG));
            rB = aB == 255 ? 255 : clamp255((bB * 255) / (255 - aB));
            break;

        case 16: // Add
            rR = clamp255(aR + bR);
            rG = clamp255(aG + bG);
            rB = clamp255(aB + bB);
            break;

        case 17: // Overlay
            rR = aR < 128 ? (2 * aR * bR / 255) : (255 - 2 * (255 - aR) * (255 - bR) / 255);
            rG = aG < 128 ? (2 * aG * bG / 255) : (255 - 2 * (255 - aG) * (255 - bG) / 255);
            rB = aB < 128 ? (2 * aB * bB / 255) : (255 - 2 * (255 - aB) * (255 - bB) / 255);
            break;

        case 18: { // Soft Light
            auto softLight = [](int a, int b) -> int {
                double fa = a / 255.0;
                double fb = b / 255.0;
                double result;
                if (fb < 0.5)
                    result = fa - (1 - 2 * fb) * fa * (1 - fa);
                else
                    result = fa + (2 * fb - 1) * (sqrt(fa) - fa);
                return clamp255d(result * 255);
            };
            rR = softLight(aR, bR);
            rG = softLight(aG, bG);
            rB = softLight(aB, bB);
            break;
        }

        case 19: // Hard Light
            rR = bR < 128 ? (2 * aR * bR / 255) : (255 - 2 * (255 - aR) * (255 - bR) / 255);
            rG = bG < 128 ? (2 * aG * bG / 255) : (255 - 2 * (255 - aG) * (255 - bG) / 255);
            rB = bB < 128 ? (2 * aB * bB / 255) : (255 - 2 * (255 - aB) * (255 - bB) / 255);
            break;

        case 20: { // Vivid Light
            auto vividLight = [](int a, int b) -> int {
                if (b < 128) {
                    return b == 0 ? 0 : clamp255(255 - ((255 - a) * 255 / (2 * b)));
                } else {
                    int b2 = 2 * (b - 128);
                    return b2 == 255 ? 255 : clamp255(a * 255 / (255 - b2));
                }
            };
            rR = vividLight(aR, bR);
            rG = vividLight(aG, bG);
            rB = vividLight(aB, bB);
            break;
        }

        case 21: // Linear Light
            rR = clamp255(aR + 2 * bR - 255);
            rG = clamp255(aG + 2 * bG - 255);
            rB = clamp255(aB + 2 * bB - 255);
            break;

        case 22: // Pin Light
            rR = bR < 128 ? qMin(aR, 2 * bR) : qMax(aR, 2 * (bR - 128));
            rG = bG < 128 ? qMin(aG, 2 * bG) : qMax(aG, 2 * (bG - 128));
            rB = bB < 128 ? qMin(aB, 2 * bB) : qMax(aB, 2 * (bB - 128));
            break;

        case 23: // Hard Mix
            rR = (aR + bR >= 255) ? 255 : 0;
            rG = (aG + bG >= 255) ? 255 : 0;
            rB = (aB + bB >= 255) ? 255 : 0;
            break;

        case 24: // Difference
            rR = qAbs(aR - bR);
            rG = qAbs(aG - bG);
            rB = qAbs(aB - bB);
            break;

        case 25: // Exclusion
            rR = aR + bR - 2 * aR * bR / 255;
            rG = aG + bG - 2 * aG * bG / 255;
            rB = aB + bB - 2 * aB * bB / 255;
            break;

        case 26: // Reflect
            rR = bR == 255 ? 255 : clamp255((aR * aR) / (255 - bR));
            rG = bG == 255 ? 255 : clamp255((aG * aG) / (255 - bG));
            rB = bB == 255 ? 255 : clamp255((aB * aB) / (255 - bB));
            break;

        case 27: // Glow
            rR = aR == 255 ? 255 : clamp255((bR * bR) / (255 - aR));
            rG = aG == 255 ? 255 : clamp255((bG * bG) / (255 - aG));
            rB = aB == 255 ? 255 : clamp255((bB * bB) / (255 - aB));
            break;

        case 28: // Freeze
            rR = bR == 0 ? 0 : clamp255(255 - ((255 - aR) * (255 - aR)) / bR);
            rG = bG == 0 ? 0 : clamp255(255 - ((255 - aG) * (255 - aG)) / bG);
            rB = bB == 0 ? 0 : clamp255(255 - ((255 - aB) * (255 - aB)) / bB);
            break;

        case 29: // Heat
            rR = aR == 0 ? 0 : clamp255(255 - ((255 - bR) * (255 - bR)) / aR);
            rG = aG == 0 ? 0 : clamp255(255 - ((255 - bG) * (255 - bG)) / aG);
            rB = aB == 0 ? 0 : clamp255(255 - ((255 - bB) * (255 - bB)) / aB);
            break;

        case 30: // Negation
            rR = 255 - qAbs(255 - aR - bR);
            rG = 255 - qAbs(255 - aG - bG);
            rB = 255 - qAbs(255 - aB - bB);
            break;

        case 31: // Phoenix
            rR = qMin(aR, bR) - qMax(aR, bR) + 255;
            rG = qMin(aG, bG) - qMax(aG, bG) + 255;
            rB = qMin(aB, bB) - qMax(aB, bB) + 255;
            break;

        case 32: // Stamp
            rR = clamp255(aR + 2 * bR - 256);
            rG = clamp255(aG + 2 * bG - 256);
            rB = clamp255(aB + 2 * bB - 256);
            break;

        case 33: // Xor
            rR = aR ^ bR;
            rG = aG ^ bG;
            rB = aB ^ bB;
            break;

        case 34: // And
            rR = aR & bR;
            rG = aG & bG;
            rB = aB & bB;
            break;

        case 35: // Or
            rR = aR | bR;
            rG = aG | bG;
            rB = aB | bB;
            break;

        case 36: // Red (use blend's red)
            rR = bR;
            rG = aG;
            rB = aB;
            break;

        case 37: // Green (use blend's green)
            rR = aR;
            rG = bG;
            rB = aB;
            break;

        case 38: // Blue (use blend's blue)
            rR = aR;
            rG = aG;
            rB = bB;
            break;

        case 39: // Yellow (blend's red and green)
            rR = bR;
            rG = bG;
            rB = aB;
            break;

        case 40: // Cyan (blend's green and blue)
            rR = aR;
            rG = bG;
            rB = bB;
            break;

        case 41: // Magenta (blend's red and blue)
            rR = bR;
            rG = aG;
            rB = bB;
            break;

        case 42: // Green limited by red
            rR = aR;
            rG = qMin(bG, aR);
            rB = aB;
            break;

        case 43: // Green limited by blue
            rR = aR;
            rG = qMin(bG, aB);
            rB = aB;
            break;

        case 44: // Green limited by average of red and blue
            rR = aR;
            rG = qMin(bG, (aR + aB) / 2);
            rB = aB;
            break;

        case 45: // Blue limited by red
            rR = aR;
            rG = aG;
            rB = qMin(bB, aR);
            break;

        case 46: // Blue limited by green
            rR = aR;
            rG = aG;
            rB = qMin(bB, aG);
            break;

        case 47: // Blue limited by average of red and green
            rR = aR;
            rG = aG;
            rB = qMin(bB, (aR + aG) / 2);
            break;

        case 48: // Red limited by green
            rR = qMin(bR, aG);
            rG = aG;
            rB = aB;
            break;

        case 49: // Red limited by blue
            rR = qMin(bR, aB);
            rG = aG;
            rB = aB;
            break;

        case 50: // Red limited by average of green and blue
            rR = qMin(bR, (aG + aB) / 2);
            rG = aG;
            rB = aB;
            break;

        case 51: // Red only
            rR = bR;
            rG = 0;
            rB = 0;
            break;

        case 52: // Green only
            rR = 0;
            rG = bG;
            rB = 0;
            break;

        case 53: // Blue only
            rR = 0;
            rG = 0;
            rB = bB;
            break;

        case 54: // Discard red
            rR = 0;
            rG = bG;
            rB = bB;
            break;

        case 55: // Discard green
            rR = bR;
            rG = 0;
            rB = bB;
            break;

        case 56: // Discard blue
            rR = bR;
            rG = bG;
            rB = 0;
            break;

        case 57: // All red
            rR = 255;
            rG = bG;
            rB = bB;
            break;

        case 58: // All green
            rR = bR;
            rG = 255;
            rB = bB;
            break;

        case 59: // All blue
            rR = bR;
            rG = bG;
            rB = 255;
            break;

        case 60: { // Hue mode (use blend's hue with base's saturation and lightness)
            double bH, bS, bL, aH, aS, aL;
            rgbToHsl(bR, bG, bB, bH, bS, bL);
            rgbToHsl(aR, aG, aB, aH, aS, aL);
            hslToRgb(bH, aS, aL, rR, rG, rB);
            break;
        }

        case 61: { // Saturation mode (use blend's saturation with base's hue and lightness)
            double bH, bS, bL, aH, aS, aL;
            rgbToHsl(bR, bG, bB, bH, bS, bL);
            rgbToHsl(aR, aG, aB, aH, aS, aL);
            hslToRgb(aH, bS, aL, rR, rG, rB);
            break;
        }

        case 62: { // Colour mode (use blend's hue and saturation with base's lightness)
            double bH, bS, bL, aH, aS, aL;
            rgbToHsl(bR, bG, bB, bH, bS, bL);
            rgbToHsl(aR, aG, aB, aH, aS, aL);
            hslToRgb(bH, bS, aL, rR, rG, rB);
            break;
        }

        case 63: { // Luminance mode (use blend's lightness with base's hue and saturation)
            double bH, bS, bL, aH, aS, aL;
            rgbToHsl(bR, bG, bB, bH, bS, bL);
            rgbToHsl(aR, aG, aB, aH, aS, aL);
            hslToRgb(aH, aS, bL, rR, rG, rB);
            break;
        }

        case 64: { // HSL (full HSL blend)
            double bH, bS, bL, aH, aS, aL;
            rgbToHsl(bR, bG, bB, bH, bS, bL);
            rgbToHsl(aR, aG, aB, aH, aS, aL);
            hslToRgb(bH, bS, bL, rR, rG, rB);
            break;
        }

        default:
            lua_pushinteger(L, -1);
            return 1;
    }

    // Apply opacity: result = base + (blended - base) * opacity
    rR = aR + static_cast<int>((rR - aR) * opacity);
    rG = aG + static_cast<int>((rG - aG) * opacity);
    rB = aB + static_cast<int>((rB - aB) * opacity);

    // Clamp results
    rR = clamp255(rR);
    rG = clamp255(rG);
    rB = clamp255(rB);

    // Combine back to BGR format
    qint32 result = (rB << 16) | (rG << 8) | rR;
    lua_pushinteger(L, result);
    return 1;
}

/**
 * world.FilterPixel(pixel, operation, options)
 *
 * Applies a filter operation to a single pixel.
 *
 * Based on methods_miniwindows.cpp FilterPixel function.
 * Reference: https://www.gammon.com.au/scripts/doc.php?function=FilterPixel
 *
 * Operations:
 *  1: Noise, 2: MonoNoise (use WindowFilter for these)
 *  7: Brightness (additive), 8: Contrast, 9: Gamma
 * 10-12: Red brightness/contrast/gamma
 * 13-15: Green brightness/contrast/gamma
 * 16-18: Blue brightness/contrast/gamma
 * 19: Grayscale (linear), 20: Grayscale (perceptual)
 * 21-24: Brightness multiply (all/R/G/B)
 * 27: Average (returns unchanged)
 *
 * @param pixel RGB color code to filter
 * @param operation Filter operation (1-27)
 * @param options Operation-specific parameter
 * @return Filtered RGB color, or -1 for invalid operation
 */
int L_FilterPixel(lua_State* L)
{
    qint32 pixel = luaL_checkinteger(L, 1);
    qint16 operation = luaL_checkinteger(L, 2);
    double options = luaL_checknumber(L, 3);

    // Extract RGB components (BGR format)
    int b = (pixel >> 16) & 0xFF;
    int g = (pixel >> 8) & 0xFF;
    int r = pixel & 0xFF;

    switch (operation) {
        case 1:   // Noise (requires random context - add random noise)
        case 2: { // MonoNoise
            double threshold = options / 100.0;
            int noise = static_cast<int>(
                (128.0 - QRandomGenerator::global()->generateDouble() * 256.0) * threshold);
            r = clamp255(r + noise);
            g = clamp255(g + noise);
            b = clamp255(b + noise);
            break;
        }

        case 7: // Brightness (additive)
            r = clamp255(r + static_cast<int>(options));
            g = clamp255(g + static_cast<int>(options));
            b = clamp255(b + static_cast<int>(options));
            break;

        case 8: // Contrast: (c - 128) * options + 128
            r = clamp255(static_cast<int>((r - 128) * options + 128));
            g = clamp255(static_cast<int>((g - 128) * options + 128));
            b = clamp255(static_cast<int>((b - 128) * options + 128));
            break;

        case 9: // Gamma: pow(c/255, options) * 255
            r = clamp255d(255.0 * qPow(r / 255.0, options));
            g = clamp255d(255.0 * qPow(g / 255.0, options));
            b = clamp255d(255.0 * qPow(b / 255.0, options));
            break;

        case 10: // Red brightness
            r = clamp255(r + static_cast<int>(options));
            break;

        case 11: // Red contrast
            r = clamp255(static_cast<int>((r - 128) * options + 128));
            break;

        case 12: // Red gamma
            r = clamp255d(255.0 * qPow(r / 255.0, options));
            break;

        case 13: // Green brightness
            g = clamp255(g + static_cast<int>(options));
            break;

        case 14: // Green contrast
            g = clamp255(static_cast<int>((g - 128) * options + 128));
            break;

        case 15: // Green gamma
            g = clamp255d(255.0 * qPow(g / 255.0, options));
            break;

        case 16: // Blue brightness
            b = clamp255(b + static_cast<int>(options));
            break;

        case 17: // Blue contrast
            b = clamp255(static_cast<int>((b - 128) * options + 128));
            break;

        case 18: // Blue gamma
            b = clamp255d(255.0 * qPow(b / 255.0, options));
            break;

        case 19: { // Grayscale (linear average)
            int gray = (r + g + b) / 3;
            r = g = b = gray;
            break;
        }

        case 20: { // Grayscale (perceptual: 0.30*R + 0.59*G + 0.11*B)
            int gray = static_cast<int>(r * 0.30 + g * 0.59 + b * 0.11);
            r = g = b = gray;
            break;
        }

        case 21: // Brightness multiply (all channels)
            r = clamp255d(r * options);
            g = clamp255d(g * options);
            b = clamp255d(b * options);
            break;

        case 22: // Red brightness multiply
            r = clamp255d(r * options);
            break;

        case 23: // Green brightness multiply
            g = clamp255d(g * options);
            break;

        case 24: // Blue brightness multiply
            b = clamp255d(b * options);
            break;

        case 27: // Average (for single pixel, returns unchanged)
            // No change
            break;

        default:
            lua_pushinteger(L, -1);
            return 1;
    }

    // Combine back to BGR format
    qint32 result = (b << 16) | (g << 8) | r;
    lua_pushinteger(L, result);
    return 1;
}

/**
 * world.Save(name)
 *
 * Saves the current world to disk.
 * Triggers ON_PLUGIN_WORLD_SAVE callback for all plugins.
 *
 * @param name (string, optional) File path to save to. If empty or nil, uses
 *   current world path. If world is new/unsaved, shows Save As dialog.
 *
 * @return (boolean) true on success, false on failure or cancel
 *
 * @example
 * -- Save to current file
 * if Save() then
 *     Note("World saved successfully")
 * end
 *
 * -- Save to specific file
 * Save("/path/to/backup.mcl")
 *
 * -- Auto-save on disconnect
 * function OnDisconnect()
 *     Save()
 * end
 *
 * @see SetChanged
 */
int L_Save(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    // Get optional filename parameter
    QString filename;
    if (lua_gettop(L) >= 1 && !lua_isnil(L, 1)) {
        filename = luaCheckQString(L, 1);
    }

    // If empty, use current world path
    if (filename.isEmpty()) {
        filename = pDoc->m_strWorldFilePath;
    }

    // If still empty (new unsaved world), show Save As dialog
    if (filename.isEmpty()) {
        // Get default world directory
        auto& db = Database::instance();
        QString defaultDir = db.getPreference("DefaultWorldFileDirectory", "./worlds/");

        // Resolve relative path against application directory
        if (!QDir::isAbsolutePath(defaultDir)) {
            defaultDir = QDir(AppPaths::getAppDirectory()).absoluteFilePath(defaultDir);
        }

        // Create suggested filename from world name (sanitize invalid characters)
        QString suggestedName = pDoc->m_mush_name;
        suggestedName.remove(QRegularExpression("[<>\"|?:#%;/\\\\]"));
        if (suggestedName.isEmpty()) {
            suggestedName = "world";
        }
        QString suggestedPath = QDir(defaultDir).filePath(suggestedName + ".mcl");

        // Show Save As dialog
        filename = QFileDialog::getSaveFileName(
            nullptr, QObject::tr("Save World As"), suggestedPath,
            QObject::tr("MUSHclient World Files (*.mcl);;All Files (*)"));

        // User cancelled
        if (filename.isEmpty()) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }

    // Execute "save" script handler if configured
    if (!pDoc->m_scripting.on_world_save.isEmpty() && pDoc->m_ScriptEngine) {
        QList<double> nparams;
        QList<QString> sparams;
        qint32 invocation_count = 0;
        pDoc->m_ScriptEngine->executeLua(pDoc->m_dispidWorldSave, pDoc->m_scripting.on_world_save,
                                         ActionSource::eWorldAction, "world", "world save", nparams,
                                         sparams, invocation_count);
    }

    // Notify plugins via ON_PLUGIN_WORLD_SAVE callback
    pDoc->SendToAllPluginCallbacks(ON_PLUGIN_WORLD_SAVE, "");

    // Save to file
    bool success = XmlSerialization::SaveWorldXML(pDoc, filename);

    // Update state on success
    if (success) {
        pDoc->m_strWorldFilePath = filename;
        pDoc->m_bVariablesChanged = false;
    }

    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

/**
 * world.TranslateGerman(text)
 *
 * Translates German ISO 8859-1 umlaut characters to their ASCII
 * equivalents. This is useful when connecting to MUDs that send German
 * text encoded as extended Latin-1 characters. The conversion table is:
 *   ä/Ä → ae/Ae, ö/Ö → oe/Oe, ü/Ü → ue/Ue, ß → ss
 *
 * @param text (string) Text containing German umlauts
 *
 * @return (string) Text with umlauts replaced by ASCII digraphs
 *
 * @example
 * -- "Über" → "Ueber", "Straße" → "Strasse"
 * local ascii = TranslateGerman("Über die Straße")
 * Note(ascii)  -- "Ueber die Strasse"
 *
 * @see FixupEscapeSequences, StripANSI
 */
int L_TranslateGerman(lua_State* L)
{
    size_t len;
    const char* text = luaL_checklstring(L, 1, &len);

    QString input = QString::fromUtf8(text, len);

    // German umlaut substitutions (ISO 8859-1 / Unicode equivalents)
    // Ordered from longest source to shortest to avoid partial replacements.
    input.replace(QChar(0xDF), "ss"); // ß → ss
    input.replace(QChar(0xE4), "ae"); // ä → ae
    input.replace(QChar(0xF6), "oe"); // ö → oe
    input.replace(QChar(0xFC), "ue"); // ü → ue
    input.replace(QChar(0xC4), "Ae"); // Ä → Ae
    input.replace(QChar(0xD6), "Oe"); // Ö → Oe
    input.replace(QChar(0xDC), "Ue"); // Ü → Ue

    luaPushQString(L, input);
    return 1;
}

/**
 * world.LowercaseWildcard(wildcard_number)
 *
 * Converts a specific numbered wildcard from the most recently matched
 * trigger to lowercase. Wildcards are 1-based (wildcard 1 is the first
 * capture group). Wildcard 0 is the full match and is not modified.
 *
 * This is useful when the eLowercaseWildcard trigger flag is not set but
 * you want to normalize one specific wildcard at script run time without
 * affecting the others.
 *
 * The function modifies the wildcard in-place on the executing trigger's
 * wildcard list. If no trigger is executing, or the wildcard number is
 * out of range, the function is a no-op.
 *
 * @param wildcard_number (number) 1-based index of the wildcard to lowercase
 *
 * @return Nothing
 *
 * @example
 * -- Trigger script: normalize the first capture group to lowercase
 * function OnPlayerInput()
 *     LowercaseWildcard(1)
 *     local name = GetTriggerWildcard("player_trigger", "1")
 *     Note("Name (lowercase): " .. name)
 * end
 *
 * @see GetTriggerWildcard, eLowercaseWildcard
 */
int L_LowercaseWildcard(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    int wildcardNumber = luaL_checkinteger(L, 1);

    // Wildcard 0 is the full match — lowercasing it is valid but unusual.
    // We iterate over all known triggers to find which one is currently
    // executing (has been most recently matched with wildcards populated).
    // Because we have no explicit "current trigger" pointer in the Lua
    // context, we lowercase the wildcard on every trigger that has enough
    // captured groups. This matches original MUSHclient behaviour where
    // only the executing trigger is in scope.

    if (wildcardNumber < 0) {
        return 0; // Invalid: negative index
    }

    // Walk the trigger map and update the wildcard on any trigger whose
    // wildcards vector contains the requested index. In practice exactly
    // one trigger will be executing at this point.
    for (auto& [key, trigger] : pDoc->m_automationRegistry->m_TriggerMap) {
        if (wildcardNumber < trigger->wildcards.size()) {
            trigger->wildcards[wildcardNumber] = trigger->wildcards[wildcardNumber].toLower();
        }
    }

    return 0;
}
