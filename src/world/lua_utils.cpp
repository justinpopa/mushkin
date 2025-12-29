/**
 * lua_utils.cpp - Lua utils module for Mushkin
 *
 * Qt-based implementation of essential utils.* functions
 *
 * Based on lua_utils.cpp from original MUSHclient, ported to Qt/cross-platform
 */

#include "lua_api/lua_common.h"
#include "color_utils.h"
#include "lua_dialog_callbacks.h"
#include "script_engine.h"
#include "world_document.h"
#include <QApplication>
#include <QColorDialog>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QSysInfo>
#include <QUrl>
#include <QVBoxLayout>
#include <QXmlStreamReader>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// External utility functions from world_utilities.cpp
extern int L_Utils_MD5(lua_State* L);
extern int L_Utils_SHA256(lua_State* L);
extern int L_Utils_Base64Encode(lua_State* L);
extern int L_Utils_Base64Decode(lua_State* L);

/**
 * utils.getfontfamilies()
 *
 * Returns a table of available font families (font names).
 * Keys are font names, values are true.
 *
 * Example: fonts = utils.getfontfamilies()
 *          if fonts["Arial"] then ... end
 *
 * Based on lua_utils.cpp
 */
static int L_utils_getfontfamilies(lua_State* L)
{
    lua_newtable(L); // table of font families

    // Get all font families from Qt's font database
    QStringList families = QFontDatabase::families();

    // Populate table with font_name = true entries
    for (const QString& family : families) {
        lua_pushstring(L, family.toUtf8().constData());
        lua_pushboolean(L, true);
        lua_rawset(L, -3);
    }

    return 1; // one table
}

/**
 * utils.fontpicker(font_name, font_size, font_color)
 *
 * Opens a font selection dialog.
 *
 * @param font_name (optional) - initial font name
 * @param font_size (optional) - initial font size in points
 * @param font_color (optional) - initial color (RGB)
 *
 * @returns table with: name, size, bold, italic, underline, strikeout
 *          or nil if cancelled
 *
 * Based on lua_utils.cpp
 */
static int L_utils_fontpicker(lua_State* L)
{
    const char* fontname = luaL_optstring(L, 1, "");
    int fontsize = luaL_optnumber(L, 2, 10);
    lua_Integer fontcolor = luaL_optinteger(L, 3, 0); // Get color before any stack changes

    // Create initial font
    QFont initialFont;
    if (fontname && fontname[0]) {
        initialFont.setFamily(QString::fromUtf8(fontname));
    }
    initialFont.setPointSize(fontsize);

    // Show font dialog
    bool ok;
    QFont selectedFont = QFontDialog::getFont(&ok, initialFont, nullptr, "Select Font");

    if (!ok) {
        lua_pushnil(L);
        return 1;
    }

    // Build style string (e.g., "Regular", "Bold", "Bold Italic")
    QStringList styleList;
    if (selectedFont.bold())
        styleList << "Bold";
    if (selectedFont.italic())
        styleList << "Italic";
    QString style = styleList.isEmpty() ? "Regular" : styleList.join(" ");

    // Return table with font properties
    // Use integers for boolean fields (0/1) to match MUSHclient behavior
    lua_newtable(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, selectedFont.family().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "size");
    lua_pushnumber(L, selectedFont.pointSize());
    lua_rawset(L, -3);

    // Boolean fields as integers (0 or 1) for compatibility with "field > 0" checks
    lua_pushstring(L, "bold");
    lua_pushnumber(L, selectedFont.bold() ? 1 : 0);
    lua_rawset(L, -3);

    lua_pushstring(L, "italic");
    lua_pushnumber(L, selectedFont.italic() ? 1 : 0);
    lua_rawset(L, -3);

    lua_pushstring(L, "underline");
    lua_pushnumber(L, selectedFont.underline() ? 1 : 0);
    lua_rawset(L, -3);

    lua_pushstring(L, "strikeout");
    lua_pushnumber(L, selectedFont.strikeOut() ? 1 : 0);
    lua_rawset(L, -3);

    // Style string (e.g., "Regular", "Bold", "Bold Italic")
    lua_pushstring(L, "style");
    lua_pushstring(L, style.toUtf8().constData());
    lua_rawset(L, -3);

    // Pass through colour from third argument (if provided)
    lua_pushstring(L, "colour");
    lua_pushnumber(L, fontcolor);
    lua_rawset(L, -3);

    return 1; // one table
}

/**
 * utils.msgbox(message, title, type, icon, default)
 *
 * Displays a message box dialog and returns the button clicked.
 *
 * @param message - Message text to display
 * @param title - Window title
 * @param type - Button combination:
 *               "ok", "abortretryignore", "okcancel", "retrycancel", "yesno", "yesnocancel"
 * @param icon - Icon to display:
 *               "!" = warning, "?" = question, "i" = information, "." = no icon
 * @param default - Default button (1-based index, optional)
 *
 * @returns String indicating button clicked:
 *          "ok", "yes", "no", "cancel", "abort", "retry", "ignore", "other"
 *
 * Based on lua_utils.cpp
 */
static int L_utils_msgbox(lua_State* L)
{
    const char* message = luaL_optstring(L, 1, "");
    const char* title = luaL_optstring(L, 2, "Mushkin");
    const char* type = luaL_optstring(L, 3, "ok");
    const char* iconStr = luaL_optstring(L, 4, "i");
    // default button parameter is optional (1-based index)

    // Determine button combination
    QMessageBox::StandardButtons buttons;
    QString typeStr = QString::fromUtf8(type);
    if (typeStr.compare("abortretryignore", Qt::CaseInsensitive) == 0) {
        buttons = QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore;
    } else if (typeStr.compare("okcancel", Qt::CaseInsensitive) == 0) {
        buttons = QMessageBox::Ok | QMessageBox::Cancel;
    } else if (typeStr.compare("retrycancel", Qt::CaseInsensitive) == 0) {
        buttons = QMessageBox::Retry | QMessageBox::Cancel;
    } else if (typeStr.compare("yesno", Qt::CaseInsensitive) == 0) {
        buttons = QMessageBox::Yes | QMessageBox::No;
    } else if (typeStr.compare("yesnocancel", Qt::CaseInsensitive) == 0) {
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
    } else {
        // Default: "ok"
        buttons = QMessageBox::Ok;
    }

    // Determine icon
    QMessageBox::Icon icon;
    if (iconStr[0] == '!') {
        icon = QMessageBox::Warning;
    } else if (iconStr[0] == '?') {
        icon = QMessageBox::Question;
    } else if (iconStr[0] == 'i') {
        icon = QMessageBox::Information;
    } else {
        icon = QMessageBox::NoIcon;
    }

    // Create and show message box
    QMessageBox msgBox;
    msgBox.setWindowTitle(QString::fromUtf8(title));
    msgBox.setText(QString::fromUtf8(message));
    msgBox.setIcon(icon);
    msgBox.setStandardButtons(buttons);

    // Show the dialog
    int result = msgBox.exec();

    // Map result to return string
    const char* resultStr = "other";
    switch (result) {
        case QMessageBox::Ok:
            resultStr = "ok";
            break;
        case QMessageBox::Yes:
            resultStr = "yes";
            break;
        case QMessageBox::No:
            resultStr = "no";
            break;
        case QMessageBox::Cancel:
            resultStr = "cancel";
            break;
        case QMessageBox::Abort:
            resultStr = "abort";
            break;
        case QMessageBox::Retry:
            resultStr = "retry";
            break;
        case QMessageBox::Ignore:
            resultStr = "ignore";
            break;
        default:
            resultStr = "other";
            break;
    }

    lua_pushstring(L, resultStr);
    return 1; // one string
}

/**
 * utils.readdir(pattern)
 *
 * Returns a table of files matching the wildcard pattern.
 *
 * @param pattern - Wildcard pattern (e.g., "/path/to/files_*.txt")
 *                  Can use * and ? wildcards
 *
 * @returns Table with filenames as keys (full paths), true as values
 *          Returns nil if no matches found
 *
 * Example: t = utils.readdir("/backup/db_Automatic*")
 *          for filename, _ in pairs(t) do
 *              print(filename)
 *          end
 */
static int L_utils_readdir(lua_State* L)
{
    const char* pattern = luaL_checkstring(L, 1);
    QString patternStr = QString::fromUtf8(pattern);

    // Split pattern into directory and filename pattern
    // e.g., "/path/to/files_*.txt" -> dir="/path/to", nameFilter="files_*.txt"
    QFileInfo fileInfo(patternStr);
    QString dirPath = fileInfo.absolutePath();
    QString nameFilter = fileInfo.fileName();

    // If no wildcard, treat as exact match
    if (!nameFilter.contains('*') && !nameFilter.contains('?')) {
        nameFilter += "*"; // Make it match anything starting with the name
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        lua_pushnil(L);
        return 1;
    }

    // Get matching files
    QStringList filters;
    filters << nameFilter;
    QStringList files = dir.entryList(filters, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    if (files.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // Create table with filenames as keys
    lua_newtable(L);
    for (const QString& filename : files) {
        // Use full path as key (as the plugin code expects)
        QString fullPath = dir.absoluteFilePath(filename);
        lua_pushstring(L, fullPath.toUtf8().constData());
        lua_pushboolean(L, true);
        lua_rawset(L, -3);
    }

    return 1; // one table
}

/**
 * utils.split(str, delimiter)
 *
 * Splits a string by a delimiter pattern.
 *
 * @param str - String to split
 * @param delimiter - Delimiter string (literal, not pattern)
 *
 * @returns Table (array) of split parts
 *
 * Example: t = utils.split("a,b,c", ",")
 *          -- t = {"a", "b", "c"}
 *
 * Based on string_split.lua behavior for utils.split
 */
static int L_utils_split(lua_State* L)
{
    const char* str = luaL_checkstring(L, 1);
    const char* delimiter = luaL_checkstring(L, 2);

    QString qStr = QString::fromUtf8(str);
    QString qDelim = QString::fromUtf8(delimiter);

    // Split the string
    QStringList parts = qStr.split(qDelim, Qt::KeepEmptyParts);

    // Create result table (array)
    lua_newtable(L);
    int index = 1;
    for (const QString& part : parts) {
        lua_pushnumber(L, index);
        lua_pushstring(L, part.toUtf8().constData());
        lua_rawset(L, -3);
        index++;
    }

    return 1; // one table
}

/**
 * utils.timer()
 *
 * Returns a high-resolution timestamp in seconds.
 * Useful for performance measurement.
 *
 * Based on lua_utils.cpp
 */
static int L_utils_timer(lua_State* L)
{
    // Use QElapsedTimer for high-resolution timing
    static QElapsedTimer globalTimer;
    static bool initialized = false;

    if (!initialized) {
        globalTimer.start();
        initialized = true;
    }

    // Return elapsed time in seconds (as double)
    double elapsed = globalTimer.elapsed() / 1000.0;
    lua_pushnumber(L, elapsed);

    return 1; // one number
}

/**
 * utils.trim(str)
 *
 * Removes leading and trailing whitespace from a string.
 *
 * @param str - String to trim
 * @returns Trimmed string
 *
 * Example: utils.trim("  hello  ") returns "hello"
 */
static int L_utils_trim(lua_State* L)
{
    const char* str = luaL_checkstring(L, 1);
    QString qStr = QString::fromUtf8(str);
    QString trimmed = qStr.trimmed();
    QByteArray ba = trimmed.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.compress(data)
 *
 * Compresses data using zlib compression.
 *
 * @param data - Binary data to compress
 * @returns Compressed data (binary safe)
 */
static int L_utils_compress(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    QByteArray input(data, len);
    QByteArray compressed = qCompress(input);
    lua_pushlstring(L, compressed.constData(), compressed.length());
    return 1;
}

/**
 * utils.decompress(data)
 *
 * Decompresses zlib-compressed data.
 *
 * @param data - Compressed binary data
 * @returns Decompressed data (binary safe)
 */
static int L_utils_decompress(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    QByteArray input(data, len);
    QByteArray decompressed = qUncompress(input);
    if (decompressed.isEmpty()) {
        lua_pushnil(L);
        lua_pushstring(L, "Decompression failed");
        return 2;
    }
    lua_pushlstring(L, decompressed.constData(), decompressed.length());
    return 1;
}

/**
 * utils.editbox(prompt, title, default)
 *
 * Shows a text input dialog.
 *
 * @param prompt - Prompt text
 * @param title - Dialog title
 * @param default - Default text (optional)
 * @returns Text entered by user, or nil if cancelled
 */
static int L_utils_editbox(lua_State* L)
{
    const char* prompt = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Input");
    const char* defaultText = luaL_optstring(L, 3, "");

    bool ok;
    QString text =
        QInputDialog::getText(nullptr, QString::fromUtf8(title), QString::fromUtf8(prompt),
                              QLineEdit::Normal, QString::fromUtf8(defaultText), &ok);

    if (!ok) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray ba = text.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.directorypicker(title, start_dir)
 *
 * Shows a directory picker dialog.
 *
 * @param title - Dialog title (optional)
 * @param start_dir - Starting directory (optional)
 * @returns Selected directory path, or nil if cancelled
 */
static int L_utils_directorypicker(lua_State* L)
{
    const char* title = luaL_optstring(L, 1, "Select Directory");
    const char* startDir = luaL_optstring(L, 2, "");

    QString dir = QFileDialog::getExistingDirectory(nullptr, QString::fromUtf8(title),
                                                    QString::fromUtf8(startDir));

    if (dir.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray ba = dir.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.filepicker(title, filter, start_dir)
 *
 * Shows a file picker dialog.
 *
 * @param title - Dialog title (optional)
 * @param filter - File filter (e.g., "Text files (*.txt);;All files (*)") (optional)
 * @param start_dir - Starting directory (optional)
 * @returns Selected file path, or nil if cancelled
 */
static int L_utils_filepicker(lua_State* L)
{
    const char* title = luaL_optstring(L, 1, "Select File");
    const char* filter = luaL_optstring(L, 2, "All files (*)");
    const char* startDir = luaL_optstring(L, 3, "");

    QString file = QFileDialog::getOpenFileName(
        nullptr, QString::fromUtf8(title), QString::fromUtf8(startDir), QString::fromUtf8(filter));

    if (file.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray ba = file.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.colourpicker(initial_color, title)
 *
 * Shows a color picker dialog.
 *
 * @param initial_color - Initial RGB color value (optional)
 * @param title - Dialog title (optional)
 * @returns RGB color value, or nil if cancelled
 */
static int L_utils_colourpicker(lua_State* L)
{
    // Lua passes colors in BGR format (MUSHclient COLORREF)
    int initialColor = luaL_optinteger(L, 1, 0xFFFFFF);
    const char* title = luaL_optstring(L, 2, "Select Color");

    // Convert BGR to QColor for the dialog
    QColor initial = bgrToQColor(initialColor);
    QColor color = QColorDialog::getColor(initial, nullptr, QString::fromUtf8(title));

    if (!color.isValid()) {
        lua_pushnil(L);
        return 1;
    }

    // Return color in BGR format for Lua
    lua_pushinteger(L, BGR(color.red(), color.green(), color.blue()));
    return 1;
}

/**
 * Helper structure for key-value pairs in choose/listbox functions
 * Matches the original MUSHclient's CKeyValuePair
 */
struct KeyValuePair {
    QString value;     // Display value
    QString stringKey; // String key (if not numeric)
    lua_Number numKey; // Numeric key (if numeric)
    bool isNumeric;    // true if key is a number

    KeyValuePair() : numKey(0), isNumeric(false)
    {
    }
};

/**
 * utils.choose(message, title, choices_table, default_key)
 *
 * Shows a combo box (dropdown) selection dialog.
 *
 * @param message - Message to display above the combo box
 * @param title - Dialog title (optional, defaults to "MUSHclient")
 * @param choices_table - Table of key/value pairs (value is displayed)
 * @param default_key - Default key to select (optional)
 * @returns Selected key (string or number), or nil if cancelled
 *
 * Example:
 *   key = utils.choose("Pick a color:", "Colors",
 *                      {red="Red Color", blue="Blue Color", green="Green Color"},
 *                      "blue")
 */
static int L_utils_choose(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Mushkin");

    // Check for table at position 3
    if (!lua_istable(L, 3)) {
        return luaL_error(L, "must have table of choices as 3rd argument");
    }

    // Check if default is provided
    bool hasDefault = lua_gettop(L) >= 4 && !lua_isnil(L, 4);
    bool defaultIsNumber = hasDefault && lua_type(L, 4) == LUA_TNUMBER;
    QString defaultString;
    lua_Number defaultNumber = 0;

    if (hasDefault) {
        if (!lua_isstring(L, 4) && !lua_isnumber(L, 4)) {
            return luaL_error(L, "default key must be string or number");
        }
        if (defaultIsNumber) {
            defaultNumber = lua_tonumber(L, 4);
        } else {
            defaultString = QString::fromUtf8(lua_tostring(L, 4));
        }
    }

    // Parse the choices table
    QVector<KeyValuePair> pairs;
    QStringList displayItems;
    int defaultIndex = -1;

    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        if (!lua_isstring(L, -2) && !lua_isnumber(L, -2)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string or number keys");
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string values");
        }

        KeyValuePair kv;
        kv.value = QString::fromUtf8(lua_tostring(L, -1));

        if (lua_type(L, -2) == LUA_TSTRING) {
            kv.stringKey = QString::fromUtf8(lua_tostring(L, -2));
            kv.isNumeric = false;
            // Check for default
            if (hasDefault && !defaultIsNumber && kv.stringKey == defaultString) {
                defaultIndex = pairs.size();
            }
        } else {
            kv.numKey = lua_tonumber(L, -2);
            kv.isNumeric = true;
            // Check for default
            if (hasDefault && defaultIsNumber && kv.numKey == defaultNumber) {
                defaultIndex = pairs.size();
            }
        }

        pairs.append(kv);
        displayItems.append(kv.value);
        lua_pop(L, 1);
    }

    if (pairs.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // Show the choose dialog using callback
    auto callback = LuaDialogCallbacks::getChooseDialogCallback();
    LuaDialogResult result = callback(QString::fromUtf8(title), QString::fromUtf8(message),
                                      displayItems, defaultIndex >= 0 ? defaultIndex : 0);

    if (!result.accepted) {
        lua_pushnil(L);
        return 1;
    }

    int selected = result.selectedIndex;
    if (selected < 0 || selected >= pairs.size()) {
        lua_pushnil(L);
        return 1;
    }

    // Return the key (not the value)
    const KeyValuePair& kv = pairs[selected];
    if (kv.isNumeric) {
        lua_pushnumber(L, kv.numKey);
    } else {
        lua_pushstring(L, kv.stringKey.toUtf8().constData());
    }
    return 1;
}

/**
 * utils.listbox(message, title, choices_table, default_key)
 *
 * Shows a list selection dialog.
 *
 * @param message - Message to display above the list
 * @param title - Dialog title (optional, defaults to "MUSHclient")
 * @param choices_table - Table of key/value pairs (value is displayed)
 * @param default_key - Default key to select (optional)
 * @returns Selected key (string or number), or nil if cancelled
 *
 * Example:
 *   key = utils.listbox("Pick a direction:", "Directions",
 *                       {n="North", s="South", e="East", w="West"},
 *                       "n")
 */
static int L_utils_listbox(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Mushkin");

    // Check for table at position 3
    if (!lua_istable(L, 3)) {
        return luaL_error(L, "must have table of choices as 3rd argument");
    }

    // Check if default is provided
    bool hasDefault = lua_gettop(L) >= 4 && !lua_isnil(L, 4);
    bool defaultIsNumber = hasDefault && lua_type(L, 4) == LUA_TNUMBER;
    QString defaultString;
    lua_Number defaultNumber = 0;

    if (hasDefault) {
        if (!lua_isstring(L, 4) && !lua_isnumber(L, 4)) {
            return luaL_error(L, "default key must be string or number");
        }
        if (defaultIsNumber) {
            defaultNumber = lua_tonumber(L, 4);
        } else {
            defaultString = QString::fromUtf8(lua_tostring(L, 4));
        }
    }

    // Parse the choices table
    QVector<KeyValuePair> pairs;
    QStringList displayItems;
    int defaultIndex = -1;

    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        if (!lua_isstring(L, -2) && !lua_isnumber(L, -2)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string or number keys");
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string values");
        }

        KeyValuePair kv;
        kv.value = QString::fromUtf8(lua_tostring(L, -1));

        if (lua_type(L, -2) == LUA_TSTRING) {
            kv.stringKey = QString::fromUtf8(lua_tostring(L, -2));
            kv.isNumeric = false;
            if (hasDefault && !defaultIsNumber && kv.stringKey == defaultString) {
                defaultIndex = pairs.size();
            }
        } else {
            kv.numKey = lua_tonumber(L, -2);
            kv.isNumeric = true;
            if (hasDefault && defaultIsNumber && kv.numKey == defaultNumber) {
                defaultIndex = pairs.size();
            }
        }

        pairs.append(kv);
        displayItems.append(kv.value);
        lua_pop(L, 1);
    }

    if (pairs.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // Show the list dialog using callback
    auto callback = LuaDialogCallbacks::getListDialogCallback();
    LuaDialogResult result = callback(QString::fromUtf8(title), QString::fromUtf8(message),
                                      displayItems, defaultIndex >= 0 ? defaultIndex : 0);

    if (!result.accepted) {
        lua_pushnil(L);
        return 1;
    }

    int selected = result.selectedIndex;
    if (selected < 0 || selected >= pairs.size()) {
        lua_pushnil(L);
        return 1;
    }

    // Return the key (not the value)
    const KeyValuePair& kv = pairs[selected];
    if (kv.isNumeric) {
        lua_pushnumber(L, kv.numKey);
    } else {
        lua_pushstring(L, kv.stringKey.toUtf8().constData());
    }
    return 1;
}

/**
 * utils.utf8len(str)
 *
 * Returns the number of UTF-8 characters in a string.
 *
 * @param str - UTF-8 string
 * @returns Number of characters (not bytes)
 */
static int L_utils_utf8len(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    QString qStr = QString::fromUtf8(str, len);
    lua_pushinteger(L, qStr.length());
    return 1;
}

/**
 * utils.utf8valid(str)
 *
 * Checks if a string is valid UTF-8.
 *
 * @param str - String to validate
 * @returns true if valid UTF-8, false otherwise
 */
static int L_utils_utf8valid(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);

    // Try to convert to QString and check for replacement characters
    QString qStr = QString::fromUtf8(str, len);
    QByteArray backToUtf8 = qStr.toUtf8();

    // If the round-trip produces the same result, it's valid UTF-8
    // Also check that no replacement characters were added
    bool valid = (backToUtf8.length() == static_cast<int>(len)) &&
                 (memcmp(backToUtf8.constData(), str, len) == 0) &&
                 !qStr.contains(QChar::ReplacementCharacter);

    lua_pushboolean(L, valid);
    return 1;
}

/**
 * utils.utf8sub(str, start, end)
 *
 * Extracts a substring using UTF-8 character positions (not byte positions).
 *
 * @param str - UTF-8 string
 * @param start - Starting character position (1-based, inclusive)
 * @param end - Ending character position (1-based, inclusive, optional)
 * @returns Substring
 */
static int L_utils_utf8sub(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    int start = luaL_checkinteger(L, 2);
    int end = luaL_optinteger(L, 3, -1);

    QString qStr = QString::fromUtf8(str, len);
    int strLen = qStr.length();

    // Convert Lua 1-based indexing to 0-based
    start--;

    // Handle negative indices (from end)
    if (start < 0)
        start += strLen + 1;
    if (end < 0)
        end += strLen + 1;
    else
        end--; // Convert to 0-based

    // Clamp to valid range
    if (start < 0)
        start = 0;
    if (end >= strLen)
        end = strLen - 1;
    if (start > end) {
        lua_pushlstring(L, "", 0);
        return 1;
    }

    QString substr = qStr.mid(start, end - start + 1);
    QByteArray ba = substr.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.utf8upper(str)
 *
 * Converts a UTF-8 string to uppercase.
 *
 * @param str - UTF-8 string
 * @returns Uppercase string
 */
static int L_utils_utf8upper(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    QString qStr = QString::fromUtf8(str, len);
    QString upper = qStr.toUpper();
    QByteArray ba = upper.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.utf8lower(str)
 *
 * Converts a UTF-8 string to lowercase.
 *
 * @param str - UTF-8 string
 * @returns Lowercase string
 */
static int L_utils_utf8lower(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    QString qStr = QString::fromUtf8(str, len);
    QString lower = qStr.toLower();
    QByteArray ba = lower.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.utf8encode(...)
 *
 * Encodes a sequence of Unicode codepoints to a UTF-8 string.
 *
 * @param ... - Variable number of codepoint integers
 * @returns UTF-8 encoded string
 */
static int L_utils_utf8encode(lua_State* L)
{
    int n = lua_gettop(L);
    QString result;

    for (int i = 1; i <= n; i++) {
        int codepoint = luaL_checkinteger(L, i);
        if (codepoint < 0 || codepoint > 0x10FFFF) {
            return luaL_error(L, "Invalid codepoint: %d", codepoint);
        }
        result.append(QChar(codepoint));
    }

    QByteArray ba = result.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
    return 1;
}

/**
 * utils.utf8decode(str)
 *
 * Decodes a UTF-8 string to a table of Unicode codepoints.
 *
 * @param str - UTF-8 string
 * @returns Table (array) of codepoint integers
 */
static int L_utils_utf8decode(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    QString qStr = QString::fromUtf8(str, len);

    lua_newtable(L);
    int index = 1;

    for (int i = 0; i < qStr.length(); i++) {
        QChar ch = qStr.at(i);
        lua_pushinteger(L, ch.unicode());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/**
 * utils.multilistbox(message, title, choices_table, [defaults_table])
 *
 * Shows a multi-selection list dialog.
 *
 * @param message - Message to display above the list
 * @param title - Dialog title (optional, defaults to "MUSHclient")
 * @param choices_table - Table of key/value pairs (value is displayed)
 * @param defaults_table - Optional table with pre-selected keys set to true
 * @returns Table with selected keys set to true, or nil if cancelled
 *
 * Example:
 *   selected = utils.multilistbox("Select colors:", "Colors",
 *                                 {red="Red Color", blue="Blue Color", green="Green Color"},
 *                                 {red=true, blue=true})
 *   if selected.red then ... end
 */
static int L_utils_multilistbox(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    const char* title = luaL_optstring(L, 2, "Mushkin");

    // Check for table at position 3
    if (!lua_istable(L, 3)) {
        return luaL_error(L, "must have table of choices as 3rd argument");
    }

    // Check if defaults table is provided
    bool hasDefaults = lua_gettop(L) >= 4 && lua_istable(L, 4);

    // Parse the choices table
    QVector<KeyValuePair> pairs;
    QStringList displayItems;
    QList<int> defaultIndices;

    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        if (!lua_isstring(L, -2) && !lua_isnumber(L, -2)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string or number keys");
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 2);
            return luaL_error(L, "table must have string values");
        }

        KeyValuePair kv;
        kv.value = QString::fromUtf8(lua_tostring(L, -1));

        if (lua_type(L, -2) == LUA_TSTRING) {
            kv.stringKey = QString::fromUtf8(lua_tostring(L, -2));
            kv.isNumeric = false;
        } else {
            kv.numKey = lua_tonumber(L, -2);
            kv.isNumeric = true;
        }

        // Check if this item should be pre-selected
        if (hasDefaults) {
            lua_pushvalue(L, -2); // Copy key to top of stack
            lua_gettable(L, 4);   // Look up key in defaults table
            if (!lua_isnil(L, -1) && !(lua_isboolean(L, -1) && !lua_toboolean(L, -1))) {
                defaultIndices.append(pairs.size());
            }
            lua_pop(L, 1); // Remove lookup result
        }

        pairs.append(kv);
        displayItems.append(kv.value);
        lua_pop(L, 1);
    }

    if (pairs.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    // Show the multi-select dialog using callback
    auto callback = LuaDialogCallbacks::getMultiListDialogCallback();
    LuaDialogResult result = callback(QString::fromUtf8(title), QString::fromUtf8(message),
                                      displayItems, defaultIndices);

    if (!result.accepted) {
        lua_pushnil(L);
        return 1;
    }

    // Build result table with selected keys set to true
    lua_newtable(L);
    for (int idx : result.selectedIndices) {
        if (idx >= 0 && idx < pairs.size()) {
            const KeyValuePair& kv = pairs[idx];
            if (kv.isNumeric) {
                lua_pushnumber(L, kv.numKey);
            } else {
                lua_pushstring(L, kv.stringKey.toUtf8().constData());
            }
            lua_pushboolean(L, 1);
            lua_settable(L, -3);
        }
    }

    return 1;
}

/**
 * utils.hash(str)
 *
 * Computes SHA-1 hash of a string and returns it as a 40-character hex string.
 *
 * @param str - String to hash
 * @returns 40-character hexadecimal hash string
 */
static int L_utils_hash(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);

    QByteArray data(str, len);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);

    // Convert to hex string (40 characters for SHA-1)
    QString hexHash = hash.toHex();

    lua_pushstring(L, hexHash.toUtf8().constData());
    return 1;
}

/**
 * utils.tohex(str)
 *
 * Converts binary data to hexadecimal string.
 *
 * @param str - Binary data to convert
 * @returns Uppercase hexadecimal string
 */
static int L_utils_tohex(lua_State* L)
{
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);

    QByteArray data(str, len);
    QString hexString = data.toHex().toUpper();

    lua_pushstring(L, hexString.toUtf8().constData());
    return 1;
}

/**
 * utils.fromhex(str)
 *
 * Converts hexadecimal string to binary data.
 * Spaces are ignored. Non-hex characters cause an error.
 *
 * @param str - Hexadecimal string to convert
 * @returns Binary data
 */
static int L_utils_fromhex(lua_State* L)
{
    const char* hexStr = luaL_checkstring(L, 1);
    QString hexString = QString::fromUtf8(hexStr);

    // Remove spaces
    hexString.remove(' ');

    // Validate hex characters
    for (int i = 0; i < hexString.length(); i++) {
        QChar ch = hexString[i];
        if (!ch.isDigit() && (ch.toLower() < 'a' || ch.toLower() > 'f')) {
            return luaL_error(L, "Not a hex digit ('%c') at position %d", ch.toLatin1(), i + 1);
        }
    }

    // Convert from hex
    QByteArray binary = QByteArray::fromHex(hexString.toUtf8());
    lua_pushlstring(L, binary.constData(), binary.length());
    return 1;
}

/**
 * utils.info()
 *
 * Returns a table with system and application information.
 *
 * @returns Table with system/app info
 */
static int L_utils_info(lua_State* L)
{
    lua_newtable(L);

    // Current directory
    QString currentDir = QDir::currentPath();
    lua_pushstring(L, "current_directory");
    lua_pushstring(L, currentDir.toUtf8().constData());
    lua_rawset(L, -3);

    // Application directory
    QString appDir = QCoreApplication::applicationDirPath();
    lua_pushstring(L, "app_directory");
    lua_pushstring(L, appDir.toUtf8().constData());
    lua_rawset(L, -3);

    // OS information
    lua_pushstring(L, "os_name");
    lua_pushstring(L, QSysInfo::productType().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "os_version");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "os_pretty_name");
    lua_pushstring(L, QSysInfo::prettyProductName().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "kernel_type");
    lua_pushstring(L, QSysInfo::kernelType().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "kernel_version");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "cpu_architecture");
    lua_pushstring(L, QSysInfo::currentCpuArchitecture().toUtf8().constData());
    lua_rawset(L, -3);

    // Qt version
    lua_pushstring(L, "qt_version");
    lua_pushstring(L, qVersion());
    lua_rawset(L, -3);

    // Application name and version
    lua_pushstring(L, "app_name");
    lua_pushstring(L, QCoreApplication::applicationName().toUtf8().constData());
    lua_rawset(L, -3);

    lua_pushstring(L, "app_version");
    lua_pushstring(L, QCoreApplication::applicationVersion().toUtf8().constData());
    lua_rawset(L, -3);

    return 1;
}

/**
 * utils.shellexecute(filename, [params], [defdir], [operation])
 *
 * Opens a file, URL, or executes a command using the system's default application.
 * Cross-platform implementation using Qt.
 *
 * @param filename - File, URL, or command to execute
 * @param params - Optional parameters (for QProcess only)
 * @param defdir - Optional working directory (for QProcess only)
 * @param operation - Optional operation: "open" (default), "explore", "print"
 * @returns true on success, nil + error message on failure
 *
 * Examples:
 *   utils.shellexecute("http://www.gammon.com.au/")  -- Open URL
 *   utils.shellexecute("file.txt")                   -- Open with default editor
 *   utils.shellexecute("mailto:someone@example.com") -- Open mail client
 */
static int L_utils_shellexecute(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    const char* params = luaL_optstring(L, 2, "");
    const char* defdir = luaL_optstring(L, 3, "");
    const char* operation = luaL_optstring(L, 4, "open");

    QString qFilename = QString::fromUtf8(filename);
    QString qOperation = QString::fromUtf8(operation);

    // For "open" operation with URLs or files, use QDesktopServices
    if (qOperation == "open") {
        QUrl url;

        // Check if it's a URL (has a scheme)
        if (qFilename.contains("://") || qFilename.startsWith("mailto:")) {
            url = QUrl(qFilename);
        } else {
            // It's a file path
            QFileInfo fileInfo(qFilename);
            if (fileInfo.isAbsolute()) {
                url = QUrl::fromLocalFile(qFilename);
            } else {
                // Relative path - resolve it
                QString absPath = QDir::current().absoluteFilePath(qFilename);
                url = QUrl::fromLocalFile(absPath);
            }
        }

        if (QDesktopServices::openUrl(url)) {
            lua_pushboolean(L, 1);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "Failed to open URL or file");
            return 2;
        }
    }
    // For "explore" operation, open the file manager at the location
    else if (qOperation == "explore") {
        QFileInfo fileInfo(qFilename);
        QString dirPath = fileInfo.isDir() ? qFilename : fileInfo.absolutePath();

        if (QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath))) {
            lua_pushboolean(L, 1);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "Failed to explore directory");
            return 2;
        }
    }
    // Other operations not directly supported by QDesktopServices
    else {
        lua_pushnil(L);
        lua_pushfstring(L, "Operation '%s' not supported on this platform", operation);
        return 2;
    }
}

/**
 * Helper function to recursively parse XML and build Lua table
 */
static void xmlParseNode(lua_State* L, QXmlStreamReader& xml)
{
    lua_newtable(L); // Create table for this node

    // Add element name
    lua_pushstring(L, "name");
    lua_pushstring(L, xml.name().toString().toUtf8().constData());
    lua_rawset(L, -3);

    // Add attributes if present
    QXmlStreamAttributes attributes = xml.attributes();
    if (!attributes.isEmpty()) {
        lua_pushstring(L, "attributes");
        lua_newtable(L);

        for (const QXmlStreamAttribute& attr : attributes) {
            lua_pushstring(L, attr.name().toString().toUtf8().constData());
            lua_pushstring(L, attr.value().toString().toUtf8().constData());
            lua_rawset(L, -3);
        }

        lua_rawset(L, -3); // Set attributes table
    }

    // Parse child nodes
    QString content;
    QVector<int> childIndices; // Track which stack positions have child nodes
    int childCount = 0;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement()) {
            break; // End of this element
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            content += xml.text().toString();
        } else if (xml.isStartElement()) {
            // Lazy-create nodes table on first child
            if (childCount == 0) {
                lua_pushstring(L, "nodes");
                lua_newtable(L);
            }

            // Recursively parse child node
            xmlParseNode(L, xml);
            lua_rawseti(L, -2, ++childCount);
        }
    }

    // If we created a nodes table, set it
    if (childCount > 0) {
        lua_rawset(L, -3); // Set nodes table
    }

    // Add content if present
    if (!content.isEmpty()) {
        lua_pushstring(L, "content");
        lua_pushstring(L, content.toUtf8().constData());
        lua_rawset(L, -3);
    }

    // Mark as empty element if it was self-closing
    if (xml.tokenType() == QXmlStreamReader::EndElement && content.isEmpty() && childCount == 0) {
        lua_pushstring(L, "empty");
        lua_pushboolean(L, 1);
        lua_rawset(L, -3);
    }
}

/**
 * utils.xmlread(xml_string)
 *
 * Parses XML string into a Lua table structure.
 *
 * @param xml_string - XML text to parse
 * @returns root_table, document_name on success; nil, error_message, line_number on error
 *
 * Table structure:
 * {
 *   name = "element_name",
 *   attributes = { attr1 = "value1", ... },
 *   nodes = { child_table1, child_table2, ... },
 *   content = "text content",
 *   empty = true  -- if self-closing tag
 * }
 */
static int L_utils_xmlread(lua_State* L)
{
    size_t xmlLength;
    const char* xmlStr = luaL_checklstring(L, 1, &xmlLength);

    QString xmlString = QString::fromUtf8(xmlStr, xmlLength);
    QXmlStreamReader xml(xmlString);

    QString documentName;

    // Skip to first element
    while (!xml.atEnd() && !xml.isStartElement()) {
        xml.readNext();
        if (xml.isStartDocument()) {
            // Could extract document info here if needed
        }
    }

    if (xml.hasError()) {
        lua_pushnil(L);
        lua_pushstring(L, xml.errorString().toUtf8().constData());
        lua_pushinteger(L, xml.lineNumber());
        return 3;
    }

    if (!xml.isStartElement()) {
        lua_pushnil(L);
        lua_pushstring(L, "No root element found in XML");
        lua_pushinteger(L, 0);
        return 3;
    }

    // Save document name (root element name)
    documentName = xml.name().toString();

    // Parse the root element
    xmlParseNode(L, xml);

    // Check for errors after parsing
    if (xml.hasError()) {
        lua_pop(L, 1); // Remove the table
        lua_pushnil(L);
        lua_pushstring(L, xml.errorString().toUtf8().constData());
        lua_pushinteger(L, xml.lineNumber());
        return 3;
    }

    // Return root table and document name
    lua_pushstring(L, documentName.toUtf8().constData());
    return 2;
}

/**
 * utils.functionlist()
 *
 * Returns a table of world API function names.
 *
 * @returns Table (array) of function names
 *
 * Based on lua_utils.cpp:functionlist (original MUSHclient)
 */
static int L_utils_functionlist(lua_State* L)
{
    // List of world API functions (from lua_registration.cpp)
    static const char* functions[] = {"Note",
                                      "ColourNote",
                                      "ColourTell",
                                      "Send",
                                      "Connect",
                                      "Disconnect",
                                      "IsConnected",
                                      "GetVariable",
                                      "SetVariable",
                                      "DeleteVariable",
                                      "GetVariableList",
                                      "GetInfo",
                                      "GetWorldName",
                                      "GetOption",
                                      "SetOption",
                                      "SetStatus",
                                      "Repaint",
                                      "TextRectangle",
                                      "SetBackgroundImage",
                                      "GetCommand",
                                      "SetCommand",
                                      "SetCommandSelection",
                                      "SetCommandWindowHeight",
                                      "SetScroll",
                                      "GetLineCount",
                                      "GetSentBytes",
                                      "GetReceivedBytes",
                                      "GetConnectDuration",
                                      "WorldAddress",
                                      "WorldPort",
                                      "WorldName",
                                      "Version",
                                      "GetLinesInBufferCount",
                                      "Queue",
                                      "DiscardQueue",
                                      "GetNormalColour",
                                      "GetBoldColour",
                                      "GetCustomColourText",
                                      "GetCustomColourBackground",
                                      "SetCustomColourName",
                                      "PickColour",
                                      "AdjustColour",
                                      "ColourNameToRGB",
                                      "RGBColourToName",
                                      "AddTrigger",
                                      "DeleteTrigger",
                                      "EnableTrigger",
                                      "GetTriggerInfo",
                                      "GetTriggerList",
                                      "EnableTriggerGroup",
                                      "DeleteTriggerGroup",
                                      "DeleteTemporaryTriggers",
                                      "GetTriggerOption",
                                      "SetTriggerOption",
                                      "AddTriggerEx",
                                      "StopEvaluatingTriggers",
                                      "AddAlias",
                                      "DeleteAlias",
                                      "EnableAlias",
                                      "GetAliasInfo",
                                      "GetAliasList",
                                      "EnableAliasGroup",
                                      "DeleteAliasGroup",
                                      "DeleteTemporaryAliases",
                                      "GetAliasOption",
                                      "SetAliasOption",
                                      "AddTimer",
                                      "DeleteTimer",
                                      "EnableTimer",
                                      "IsTimer",
                                      "GetTimerInfo",
                                      "GetTimerList",
                                      "ResetTimer",
                                      "ResetTimers",
                                      "DoAfter",
                                      "DoAfterNote",
                                      "DoAfterSpeedWalk",
                                      "DoAfterSpecial",
                                      "EnableTimerGroup",
                                      "DeleteTimerGroup",
                                      "DeleteTemporaryTimers",
                                      "GetTimerOption",
                                      "SetTimerOption",
                                      "Hash",
                                      "Base64Encode",
                                      "Base64Decode",
                                      "Trim",
                                      "GetUniqueNumber",
                                      "GetUniqueID",
                                      "CreateGUID",
                                      "StripANSI",
                                      "FixupEscapeSequences",
                                      "FixupHTML",
                                      "MakeRegularExpression",
                                      "Execute",
                                      "GetGlobalOption",
                                      "SetCursor",
                                      "Accelerator",
                                      "AcceleratorTo",
                                      "GetUdpPort",
                                      "OpenLog",
                                      "CloseLog",
                                      "WriteLog",
                                      "FlushLog",
                                      "IsLogOpen",
                                      "DatabaseOpen",
                                      "DatabaseClose",
                                      "DatabasePrepare",
                                      "DatabaseStep",
                                      "DatabaseFinalize",
                                      "DatabaseExec",
                                      "DatabaseColumns",
                                      "DatabaseColumnType",
                                      "DatabaseReset",
                                      "DatabaseChanges",
                                      "DatabaseTotalChanges",
                                      "GetPluginID",
                                      "GetPluginName",
                                      "GetPluginList",
                                      "IsPluginInstalled",
                                      "GetPluginInfo",
                                      "LoadPlugin",
                                      "ReloadPlugin",
                                      "UnloadPlugin",
                                      "EnablePlugin",
                                      "CallPlugin",
                                      "PluginSupports",
                                      "BroadcastPlugin",
                                      "SendPkt",
                                      "SaveState",
                                      "GetPluginVariable",
                                      "GetPluginVariableList",
                                      "GetPluginTriggerList",
                                      "GetPluginAliasList",
                                      "GetPluginTimerList",
                                      "GetPluginTriggerInfo",
                                      "GetPluginAliasInfo",
                                      "GetPluginTimerInfo",
                                      "GetPluginTriggerOption",
                                      "GetPluginAliasOption",
                                      "GetPluginTimerOption",
                                      "AddFont",
                                      "WindowCreate",
                                      "WindowShow",
                                      "WindowPosition",
                                      "WindowSetZOrder",
                                      "WindowDelete",
                                      "WindowInfo",
                                      "WindowResize",
                                      "WindowRectOp",
                                      "WindowCircleOp",
                                      "WindowLine",
                                      "WindowPolygon",
                                      "WindowGradient",
                                      "WindowSetPixel",
                                      "WindowGetPixel",
                                      "WindowArc",
                                      "WindowBezier",
                                      "WindowFont",
                                      "WindowText",
                                      "WindowTextWidth",
                                      "WindowFontInfo",
                                      "WindowFontList",
                                      "WindowLoadImage",
                                      "WindowDrawImage",
                                      "WindowBlendImage",
                                      "WindowImageFromWindow",
                                      "WindowImageInfo",
                                      "WindowImageList",
                                      "WindowWrite",
                                      "WindowGetImageAlpha",
                                      "WindowDrawImageAlpha",
                                      "WindowMergeImageAlpha",
                                      "WindowTransformImage",
                                      "WindowFilter",
                                      "WindowAddHotspot",
                                      "WindowDeleteHotspot",
                                      "WindowDeleteAllHotspots",
                                      "WindowHotspotTooltip",
                                      "WindowDragHandler",
                                      "WindowMenu",
                                      "WindowHotspotInfo",
                                      "WindowMoveHotspot",
                                      "WindowScrollwheelHandler",
                                      "PlaySound",
                                      "StopSound",
                                      "Sound",
                                      "GetSoundStatus",
                                      "SendToNotepad",
                                      "AppendToNotepad",
                                      "ReplaceNotepad",
                                      "ActivateNotepad",
                                      "CloseNotepad",
                                      "GetNotepadText",
                                      "GetNotepadLength",
                                      "GetNotepadList",
                                      "SaveNotepad",
                                      "NotepadFont",
                                      "NotepadColour",
                                      "NotepadReadOnly",
                                      "NotepadSaveMethod",
                                      "MoveNotepadWindow",
                                      "GetNotepadWindowPosition",
                                      nullptr};

    lua_newtable(L);
    int i = 1;
    for (const char** p = functions; *p != nullptr; p++, i++) {
        lua_pushstring(L, *p);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/**
 * utils.callbackslist()
 *
 * Returns a table of plugin callback names.
 *
 * @returns Table (array) of callback names
 *
 * Based on lua_utils.cpp:callbackslist (original MUSHclient)
 */
static int L_utils_callbackslist(lua_State* L)
{
    // Plugin callback names (from plugin.cpp)
    static const char* callbacks[] = {"OnPluginInstall",        "OnPluginClose",
                                      "OnPluginEnable",         "OnPluginDisable",
                                      "OnPluginConnect",        "OnPluginDisconnect",
                                      "OnPluginLineReceived",   "OnPluginPartialLine",
                                      "OnPluginPacketReceived", "OnPluginSend",
                                      "OnPluginSent",           "OnPluginCommand",
                                      "OnPluginCommandEntered", "OnPluginCommandChanged",
                                      "OnPluginTelnetOption",   "OnPluginTelnetSubnegotiation",
                                      "OnPlugin_IAC_GA",        "OnPluginMXPstart",
                                      "OnPluginMXPstop",        "OnPluginMXPopenTag",
                                      "OnPluginMXPcloseTag",    "OnPluginGetFocus",
                                      "OnPluginLoseFocus",      "OnPluginTick",
                                      "OnPluginSaveState",      "OnPluginWorldSave",
                                      "OnPluginBroadcast",      "OnPluginListChanged",
                                      "OnPluginPlaySound",      nullptr};

    lua_newtable(L);
    int i = 1;
    for (const char** p = callbacks; *p != nullptr; p++, i++) {
        lua_pushstring(L, *p);
        lua_rawseti(L, -2, i);
    }

    return 1;
}

/**
 * utils.edit_distance(str1, str2)
 *
 * Computes the Levenshtein edit distance between two strings.
 *
 * @param str1 - First string
 * @param str2 - Second string
 * @returns Number of edits (insertions, deletions, substitutions) needed
 *
 * Based on lua_utils.cpp:edit_distance (original MUSHclient)
 */
static int L_utils_edit_distance(lua_State* L)
{
    size_t len1, len2;
    const char* s1 = luaL_checklstring(L, 1, &len1);
    const char* s2 = luaL_checklstring(L, 2, &len2);

    // Use Wagner-Fischer algorithm
    QVector<QVector<int>> d(len1 + 1, QVector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; i++)
        d[i][0] = i;
    for (size_t j = 0; j <= len2; j++)
        d[0][j] = j;

    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = qMin(qMin(d[i - 1][j] + 1,    // deletion
                                d[i][j - 1] + 1),   // insertion
                           d[i - 1][j - 1] + cost); // substitution
        }
    }

    lua_pushinteger(L, d[len1][len2]);
    return 1;
}

/**
 * utils.sendtofront()
 *
 * Brings the main application window to the front.
 *
 * @returns nothing
 *
 * Based on lua_utils.cpp:send_to_front (original MUSHclient)
 */
static int L_utils_sendtofront(lua_State* L)
{
    Q_UNUSED(L);
    // Find the main window and bring it to front
    QWidgetList topLevel = QApplication::topLevelWidgets();
    for (QWidget* w : topLevel) {
        if (w->isWindow() && !w->windowTitle().isEmpty()) {
            w->raise();
            w->activateWindow();
            break;
        }
    }
    return 0;
}

/**
 * utils.infotypes()
 *
 * Returns a table mapping GetInfo type numbers to their descriptions.
 *
 * @returns Table where key is info type number, value is description
 *
 * Based on lua_utils.cpp:infotypes (original MUSHclient)
 */
static int L_utils_infotypes(lua_State* L)
{
    // GetInfo type mappings (from world_info.cpp)
    static const struct {
        int type;
        const char* desc;
    } infoTypes[] = {{1, "World name"},
                     {2, "World IP address"},
                     {3, "World TCP/IP port"},
                     {4, "Current directory"},
                     {5, "Application directory"},
                     {6, "World files directory"},
                     {7, "Plugin directory"},
                     {8, "MUSHclient version"},
                     {9, "World file location"},
                     {10, "World identifier (unique ID)"},
                     {11, "Server protocol (if known)"},
                     {12, "Time plugin loaded"},
                     {13, "Current line"},
                     {14, "Current line number"},
                     {15, "Number of lines received"},
                     {16, "Total bytes received"},
                     {17, "Total bytes sent"},
                     {18, "Connect duration (seconds)"},
                     {19, "Connected flag"},
                     {20, "Triggers enabled"},
                     {21, "Aliases enabled"},
                     {22, "Timers enabled"},
                     {23, "Unique session number"},
                     {24, "Foreground colour"},
                     {25, "Background colour"},
                     {26, "MCCP mode"},
                     {27, "Plugin ID"},
                     {28, "Plugin name"},
                     {29, "Plugin author"},
                     {30, "Plugin description"},
                     {31, "Plugin script language"},
                     {32, "Plugin script filename"},
                     {33, "Plugin date installed"},
                     {34, "Plugin date written"},
                     {35, "Plugin version"},
                     {36, "Plugin enabled"},
                     {37, "Plugin sequence number"},
                     {38, "MXP active"},
                     {39, "Pueblo active"},
                     {40, "Number of miniwindows"},
                     {41, "Log file name"},
                     {42, "Log mode"},
                     {43, "Number of worlds open"},
                     {44, "Number of active plugins"},
                     {45, "Script prefix"},
                     {46, "Last error message"},
                     {47, "Received out-of-band data"},
                     {48, "Total packets received"},
                     {49, "NAWS negotiated"},
                     {50, "MXP is in secure mode"},
                     {51, "MXP tag matching type"},
                     {52, "Compile date"},
                     {53, "Note color"},
                     {54, "Echo color"},
                     {55, "Input font name"},
                     {56, "Input font size"},
                     {57, "Output font name"},
                     {58, "Output font size"},
                     {59, "Script time format"},
                     {60, "Scripting enabled"},
                     {61, "Trace output enabled"},
                     {62, "Auto-wrap column"},
                     {63, "Output window height"},
                     {64, "Output window width"},
                     {65, "World window position (left)"},
                     {66, "World window position (top)"},
                     {67, "World window width"},
                     {68, "World window height"},
                     {69, "Command window height"},
                     {70, "Number of notepad windows"},
                     {71, "State files directory"},
                     {72, "Log files directory"},
                     {0, nullptr}};

    lua_newtable(L);
    for (int i = 0; infoTypes[i].desc != nullptr; i++) {
        lua_pushstring(L, infoTypes[i].desc);
        lua_rawseti(L, -2, infoTypes[i].type);
    }

    return 1;
}

/**
 * utils.functionargs()
 *
 * Returns a table of world API functions keyed by name, value is argument list.
 *
 * @returns Table where key is function name, value is argument description
 *
 * Based on lua_utils.cpp:functionargs (original MUSHclient)
 */
static int L_utils_functionargs(lua_State* L)
{
    // Function arguments (simplified - showing key functions)
    static const struct {
        const char* name;
        const char* args;
    } funcArgs[] = {
        {"Note", "text"},
        {"ColourNote", "foreground, background, text, ..."},
        {"ColourTell", "foreground, background, text, ..."},
        {"Send", "text"},
        {"Execute", "command"},
        {"Connect", ""},
        {"Disconnect", ""},
        {"IsConnected", ""},
        {"GetVariable", "name"},
        {"SetVariable", "name, value"},
        {"DeleteVariable", "name"},
        {"GetVariableList", ""},
        {"GetInfo", "type"},
        {"GetOption", "name"},
        {"SetOption", "name, value"},
        {"AddTrigger", "name, match, response, flags, colour, wildcard, sound, script"},
        {"DeleteTrigger", "name"},
        {"EnableTrigger", "name, enabled"},
        {"GetTriggerInfo", "name, type"},
        {"GetTriggerList", ""},
        {"AddAlias", "name, match, response, flags, script"},
        {"DeleteAlias", "name"},
        {"EnableAlias", "name, enabled"},
        {"GetAliasInfo", "name, type"},
        {"GetAliasList", ""},
        {"AddTimer", "name, hour, minute, second, response, flags, script"},
        {"DeleteTimer", "name"},
        {"EnableTimer", "name, enabled"},
        {"GetTimerInfo", "name, type"},
        {"GetTimerList", ""},
        {"DoAfter", "seconds, response"},
        {"DoAfterNote", "seconds, response"},
        {"DoAfterSpecial", "seconds, response, sendto"},
        {"OpenLog", "filename, append"},
        {"CloseLog", ""},
        {"WriteLog", "text"},
        {"FlushLog", ""},
        {"IsLogOpen", ""},
        {"WindowCreate", "name, left, top, width, height, position, flags, background"},
        {"WindowShow", "name, show"},
        {"WindowDelete", "name"},
        {"WindowRectOp", "name, action, left, top, right, bottom, colour1, colour2"},
        {"WindowText", "name, fontid, text, left, top, right, bottom, colour, unicode"},
        {"WindowFont",
         "name, fontid, fontname, size, bold, italic, underline, strikeout, charset, family"},
        {"WindowLine", "name, x1, y1, x2, y2, colour, style, width"},
        {"WindowDrawImage",
         "name, imageid, left, top, right, bottom, mode, srcleft, srctop, srcright, srcbottom"},
        {"WindowLoadImage", "name, imageid, filename"},
        {"PlaySound", "filename, loop, volume, pan"},
        {"StopSound", "channel"},
        {nullptr, nullptr}};

    lua_newtable(L);
    for (int i = 0; funcArgs[i].name != nullptr; i++) {
        lua_pushstring(L, funcArgs[i].name);
        lua_pushstring(L, funcArgs[i].args);
        lua_rawset(L, -3);
    }

    return 1;
}

/**
 * utils.metaphone(word, [max_length])
 *
 * Returns the Double Metaphone phonetic encoding of a word.
 *
 * @param word - Word to encode
 * @param max_length - Maximum length of output (default 4)
 * @returns primary, secondary (or nil if no secondary)
 *
 * Based on lua_utils.cpp:metaphone (original MUSHclient)
 */
static int L_utils_metaphone(lua_State* L)
{
    const char* word = luaL_checkstring(L, 1);
    int maxLen = luaL_optinteger(L, 2, 4);

    // Simplified metaphone - just return uppercase consonants
    // A proper Double Metaphone would need a full implementation
    QString input = QString::fromUtf8(word).toUpper();
    QString primary, secondary;

    // Simple phonetic encoding (simplified version)
    for (int i = 0; i < input.length() && primary.length() < maxLen; i++) {
        QChar c = input[i];
        if (c.isLetter() && QString("AEIOU").indexOf(c) < 0) {
            // Map some common letter groups
            if (c == 'C' && i + 1 < input.length()) {
                QChar next = input[i + 1];
                if (next == 'H') {
                    primary += 'X';
                    i++;
                    continue;
                }
                if (next == 'K') {
                    i++;
                } // skip C in CK
            }
            if (c == 'G' && i + 1 < input.length() && input[i + 1] == 'H') {
                primary += 'F';
                i++;
                continue;
            }
            if (c == 'P' && i + 1 < input.length() && input[i + 1] == 'H') {
                primary += 'F';
                i++;
                continue;
            }
            if (c == 'Q')
                c = 'K';
            if (c == 'X') {
                primary += "KS";
                continue;
            }
            if (c == 'Z')
                c = 'S';

            // Avoid duplicates
            if (primary.isEmpty() || primary[primary.length() - 1] != c) {
                primary += c;
            }
        }
    }

    lua_pushstring(L, primary.toUtf8().constData());
    lua_pushnil(L); // No secondary in simplified version
    return 2;
}

/**
 * utils.glyph_available(fontname, codepoint)
 *
 * Checks if a font has a glyph for the given Unicode code point.
 *
 * @param fontname - Name of the font to check
 * @param codepoint - Unicode code point to check
 * @returns Glyph index if available, 0 if not
 *
 * Based on lua_utils.cpp:glyph_available (original MUSHclient)
 */
static int L_utils_glyph_available(lua_State* L)
{
    const char* fontName = luaL_checkstring(L, 1);
    int codepoint = luaL_checkinteger(L, 2);

    QFont font(QString::fromUtf8(fontName));
    QFontMetrics metrics(font);

    // Check if the font has the glyph
    QChar ch = QChar(codepoint);
    if (metrics.inFont(ch)) {
        lua_pushinteger(L, codepoint); // Return codepoint as glyph index
    } else {
        lua_pushinteger(L, 0); // Not available
    }

    return 1;
}

/**
 * utils.colourcube(which)
 *
 * Sets the xterm 256-color cube mode.
 *
 * @param which - 1 for xterm (default), 2 for Netscape
 * @returns nothing
 *
 * Based on lua_utils.cpp:colourcube (original MUSHclient)
 * Note: In Qt port, this is informational only as color handling is different.
 */
static int L_utils_colourcube(lua_State* L)
{
    int which = luaL_checkinteger(L, 1);

    if (which != 1 && which != 2) {
        return luaL_error(L, "Unknown option (use 1 for xterm, 2 for Netscape)");
    }

    // In Qt port, color cube is handled internally
    // This function exists for compatibility
    return 0;
}

/**
 * utils.filterpicker(title, default_name, extension, filter_table, save, filter_callback)
 *
 * Advanced file picker with filter callback.
 *
 * @param title - Dialog title
 * @param default_name - Default filename
 * @param extension - Default extension
 * @param filter_table - Table of filter name/pattern pairs
 * @param save - true for save dialog, false for open
 * @param filter_callback - Lua function to filter files (optional)
 * @returns Selected filename or nil
 *
 * Based on lua_utils.cpp:filterpicker (original MUSHclient)
 * Note: Filter callback is not supported in Qt version.
 */
static int L_utils_filterpicker(lua_State* L)
{
    const char* title = luaL_optstring(L, 1, "");
    const char* defaultName = luaL_optstring(L, 2, "");
    const char* extension = luaL_optstring(L, 3, "");
    bool save = lua_toboolean(L, 5);

    // Build filter string from table
    QString filter;
    if (lua_istable(L, 4)) {
        lua_pushnil(L);
        while (lua_next(L, 4) != 0) {
            if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
                if (!filter.isEmpty())
                    filter += ";;";
                filter += QString::fromUtf8(lua_tostring(L, -2));
                filter += " (";
                filter += QString::fromUtf8(lua_tostring(L, -1));
                filter += ")";
            }
            lua_pop(L, 1);
        }
    }

    if (filter.isEmpty()) {
        filter = "All Files (*)";
    }

    QString result;
    if (save) {
        result = QFileDialog::getSaveFileName(nullptr, QString::fromUtf8(title),
                                              QString::fromUtf8(defaultName), filter);
    } else {
        result = QFileDialog::getOpenFileName(nullptr, QString::fromUtf8(title),
                                              QString::fromUtf8(defaultName), filter);
    }

    if (result.isEmpty()) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, result.toUtf8().constData());
    }

    return 1;
}

/**
 * utils.showdebugstatus(show)
 *
 * Shows or hides the debug status window.
 *
 * @param show - true to show, false to hide (optional)
 * @returns nothing
 *
 * Based on lua_utils.cpp:showdebugstatus (original MUSHclient)
 * Note: Not implemented in Qt port - debug output goes to Qt debug log.
 */
static int L_utils_showdebugstatus(lua_State* L)
{
    Q_UNUSED(L);
    // In Qt port, debug output uses qDebug() which goes to console/debugger
    // A future implementation could add a debug window
    return 0;
}

/**
 * utils.spellcheckdialog(word)
 *
 * Shows spell check dialog for a word.
 *
 * @param word - Word to check
 * @returns action, replacement (empty strings - spell check not implemented)
 *
 * Based on lua_utils.cpp:spellcheckdialog (original MUSHclient)
 * Note: Spell check was disabled in original too.
 */
static int L_utils_spellcheckdialog(lua_State* L)
{
    const char* word = luaL_checkstring(L, 1);

    // Spell check not implemented - return original word unchanged
    lua_pushstring(L, "");   // Empty action
    lua_pushstring(L, word); // Return original word
    return 2;
}

/**
 * utils.reload_global_prefs()
 *
 * Reloads global preferences from disk.
 *
 * @returns nothing
 *
 * Based on lua_utils.cpp:reload_global_prefs (original MUSHclient)
 */
static int L_utils_reload_global_prefs(lua_State* L)
{
    Q_UNUSED(L);
    // In Qt port, preferences are handled by QSettings
    // Could reload from QSettings here if needed
    return 0;
}

/**
 * utils.activatenotepad(title) -> boolean
 *
 * Lowercase alias for world.ActivateNotepad() for compatibility with original MUSHclient.
 * Brings notepad window to front and gives it focus.
 *
 * @param title Notepad window title
 * @return true if notepad found
 *
 * Based on lua_utils.cpp (original MUSHclient)
 */
static int L_utils_activatenotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* title = luaL_checkstring(L, 1);

    bool result = pDoc->ActivateNotepad(QString::fromUtf8(title));

    lua_pushboolean(L, result);
    return 1;
}

/**
 * utils.appendtonotepad(title, contents) -> boolean
 *
 * Lowercase alias for world.AppendToNotepad() for compatibility with original MUSHclient.
 * Appends text to notepad. If notepad doesn't exist, creates it.
 *
 * @param title Notepad window title
 * @param contents Text to append
 * @return true on success
 *
 * Based on lua_utils.cpp (original MUSHclient)
 */
static int L_utils_appendtonotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    const char* title = luaL_checkstring(L, 1);
    const char* contents = luaL_checkstring(L, 2);

    bool result = pDoc->AppendToNotepad(QString::fromUtf8(title), QString::fromUtf8(contents));

    lua_pushboolean(L, result);
    return 1;
}

/**
 * utils.setbackgroundcolour(colour) -> nil
 *
 * Sets the background color of the main frame/window.
 * This is different from SetBackgroundColour which sets the world window background.
 *
 * @param colour RGB color value (optional, defaults to 0)
 * @return nothing
 *
 * NOTE: Not yet implemented in Qt port. Main frame background color setting
 * is not currently supported. This function exists for compatibility but does nothing.
 *
 * Based on lua_utils.cpp:setbackgroundcolour (original MUSHclient)
 */
static int L_utils_setbackgroundcolour(lua_State* L)
{
    // TODO: Implement main frame background color setting
    // In original: Frame.m_backgroundColour = luaL_optnumber (L, 1, 0);
    //              Frame.InvalidateRect(NULL);

    // For now, this is a no-op for compatibility
    // qDebug() << "setbackgroundcolour: Not yet implemented in Qt port";

    return 0; // return nothing
}

/**
 * Register utils module functions
 *
 * Called from ScriptEngine to register the utils table
 */
static const struct luaL_Reg utilslib[] = {
    {"activatenotepad", L_utils_activatenotepad},
    {"appendtonotepad", L_utils_appendtonotepad},
    {"callbackslist", L_utils_callbackslist},
    {"choose", L_utils_choose},
    {"colourcube", L_utils_colourcube},
    {"colourpicker", L_utils_colourpicker},
    {"compress", L_utils_compress},
    {"decompress", L_utils_decompress},
    {"directorypicker", L_utils_directorypicker},
    {"edit_distance", L_utils_edit_distance},
    {"editbox", L_utils_editbox},
    {"filepicker", L_utils_filepicker},
    {"filterpicker", L_utils_filterpicker},
    {"fontpicker", L_utils_fontpicker},
    {"fromhex", L_utils_fromhex},
    {"functionargs", L_utils_functionargs},
    {"functionlist", L_utils_functionlist},
    {"getfontfamilies", L_utils_getfontfamilies},
    {"glyph_available", L_utils_glyph_available},
    {"info", L_utils_info},
    {"infotypes", L_utils_infotypes},
    {"inputbox", L_utils_editbox}, // Alias for compatibility
    {"listbox", L_utils_listbox},
    {"md5", L_Utils_MD5},
    {"metaphone", L_utils_metaphone},
    {"msgbox", L_utils_msgbox},
    {"multilistbox", L_utils_multilistbox},
    {"readdir", L_utils_readdir},
    {"reload_global_prefs", L_utils_reload_global_prefs},
    {"sendtofront", L_utils_sendtofront},
    {"setbackgroundcolour", L_utils_setbackgroundcolour},
    {"sha256", L_Utils_SHA256},
    {"shellexecute", L_utils_shellexecute},
    {"showdebugstatus", L_utils_showdebugstatus},
    {"spellcheckdialog", L_utils_spellcheckdialog},
    {"split", L_utils_split},
    {"timer", L_utils_timer},
    {"tohex", L_utils_tohex},
    {"umsgbox", L_utils_msgbox}, // Unicode msgbox - same as msgbox in Qt (UTF-8)
    {"utf8decode", L_utils_utf8decode},
    {"utf8encode", L_utils_utf8encode},
    {"utf8len", L_utils_utf8len},
    {"utf8lower", L_utils_utf8lower},
    {"utf8sub", L_utils_utf8sub},
    {"utf8upper", L_utils_utf8upper},
    {"utf8valid", L_utils_utf8valid},
    {"xmlread", L_utils_xmlread},
    {NULL, NULL}};

/**
 * Initialize the utils module in Lua state
 *
 * Creates a global "utils" table and registers functions
 */
void luaopen_utils(lua_State* L)
{
    // Create utils table
    luaL_newlib(L, utilslib);

    // Set it as global "utils"
    lua_setglobal(L, "utils");
}
