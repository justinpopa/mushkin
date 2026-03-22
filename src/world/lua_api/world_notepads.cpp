/**
 * world_notepads.cpp - Notepad API Functions
 *
 * Lua API for creating and managing notepad windows
 */

#include "../notepad_widget.h"
#include "../world_document.h"
#include "lua_common.h"
#include <QMdiSubWindow>

// Concatenate all Lua string args from position `start` onward (original: concatArgs)
static QString concatLuaArgs(lua_State* L, int start)
{
    QString result;
    int top = lua_gettop(L);
    for (int i = start; i <= top; i++) {
        if (lua_isstring(L, i)) {
            result += QString::fromUtf8(lua_tostring(L, i));
        }
    }
    return result;
}

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

/**
 * world.SendToNotepad(title, contents) -> boolean
 *
 * Create or replace notepad contents. If notepad exists, replaces all text.
 * If not, creates new notepad with the text.
 *
 * @param title Notepad window title
 * @param contents Text content
 * @return true on success
 */
int L_SendToNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString title = luaCheckQString(L, 1);
    QString contents = concatLuaArgs(L, 2); // original: concatArgs(L, "", 2)
    return luaReturn(L, pDoc->SendToNotepad(title, contents).has_value());
}

/**
 * world.AppendToNotepad(title, contents) -> boolean
 *
 * Append text to notepad. If notepad doesn't exist, creates it.
 *
 * @param title Notepad window title
 * @param contents Text to append
 * @return true on success
 */
int L_AppendToNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString title = luaCheckQString(L, 1);
    QString contents = concatLuaArgs(L, 2); // original: concatArgs(L, "", 2)
    return luaReturn(L, pDoc->AppendToNotepad(title, contents).has_value());
}

/**
 * world.ReplaceNotepad(title, contents) -> boolean
 *
 * Replace notepad contents. Only works if notepad already exists.
 *
 * @param title Notepad window title
 * @param contents New text content
 * @return true if notepad found and replaced
 */
int L_ReplaceNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    QString title = luaCheckQString(L, 1);
    QString contents = concatLuaArgs(L, 2); // original: concatArgs(L, "", 2)
    return luaReturn(L, pDoc->ReplaceNotepad(title, contents).has_value());
}

/**
 * world.ActivateNotepad(title) -> boolean
 *
 * Bring notepad window to front and give it focus.
 *
 * @param title Notepad window title
 * @return true if notepad found
 */
int L_ActivateNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    auto [title] = luaArgs<QString>(L);
    return luaReturn(L, pDoc->ActivateNotepad(title).has_value());
}

/**
 * world.CloseNotepad(title, querySave) -> error_code
 *
 * Close notepad window.
 *
 * @param title Notepad window title
 * @param querySave If true, prompt user to save changes (optional, defaults to false)
 * @return eOK (0) on success, eNoSuchNotepad (30075) if not found
 */
int L_CloseNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool querySave = lua_toboolean(L, 2);

    qint32 result = pDoc->CloseNotepad(luaCheckQString(L, 1), querySave);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.GetNotepadText(title) -> string or nil
 *
 * Get all text from notepad.
 *
 * @param title Notepad window title
 * @return Text content, or nil if notepad not found
 */
int L_GetNotepadText(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    auto [title] = luaArgs<QString>(L);
    NotepadWidget* notepad = pDoc->FindNotepad(title);

    if (!notepad) {
        // Original returns empty string, not nil (CString initialized empty)
        return luaReturn(L, QString());
    }

    // Original uses CEdit::GetWindowText which returns \r\n line endings
    QString text = notepad->GetText();
    text.replace('\n', QStringLiteral("\r\n"));
    return luaReturn(L, text);
}

/**
 * world.GetNotepadLength(title) -> number
 *
 * Get text length in notepad.
 *
 * @param title Notepad window title
 * @return Number of characters, or 0 if notepad not found
 */
int L_GetNotepadLength(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    NotepadWidget* notepad = pDoc->FindNotepad(luaCheckQString(L, 1));

    // Original CEdit::GetWindowTextLength counts \r\n as 2 chars per newline.
    // Qt toPlainText counts \n as 1. Add newline count to match.
    qint32 length = 0;
    if (notepad) {
        QString text = notepad->GetText();
        length = text.length() + text.count('\n'); // +1 per newline for \r
    }
    lua_pushnumber(L, length);

    return 1;
}

/**
 * world.GetNotepadList(includeAll) -> table
 *
 * Get list of notepad titles.
 *
 * @param includeAll If true, include all worlds' notepads (optional, defaults to false)
 * @return Table array of notepad titles (1-indexed)
 */
int L_GetNotepadList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    bool includeAll = lua_toboolean(L, 1);

    QStringList titles = pDoc->GetNotepadList(includeAll);

    luaPushQStringList(L, titles);

    return 1;
}

/**
 * world.SaveNotepad(title, filename, replaceExisting) -> error_code
 *
 * Save notepad contents to file.
 *
 * @param title Notepad window title
 * @param filename File path to save to
 * @param replaceExisting If true, overwrite existing file (optional, defaults to true)
 * @return eOK (0) on success, eNoSuchNotepad (30075) if notepad not found,
 *         eFileNotOpened (30076) if file cannot be opened
 */
int L_SaveNotepad(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Original: optboolean(L, 3, 0) — default false (do not overwrite)
    bool replaceExisting = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;

    qint32 result =
        pDoc->SaveNotepad(luaCheckQString(L, 1), luaCheckQString(L, 2), replaceExisting);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.NotepadFont(title, fontName, size, style, charset) -> error_code
 *
 * Set notepad font.
 *
 * @param title Notepad window title
 * @param fontName Font family name (e.g., "Courier New")
 * @param size Font size in points
 * @param style Style flags: 1=bold, 2=italic, 4=underline, 8=strikeout (can be combined)
 * @param charset Character set (usually 0 for default)
 * @return eOK (0) on success, eNoSuchNotepad (30075) if not found
 */
int L_NotepadFont(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 size = luaL_checknumber(L, 3);
    qint32 style = luaL_checknumber(L, 4);
    qint32 charset = static_cast<qint32>(luaL_optnumber(L, 5, 0)); // optional, default 0

    qint32 result =
        pDoc->NotepadFont(luaCheckQString(L, 1), luaCheckQString(L, 2), size, style, charset);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.NotepadColour(title, textColour, backColour) -> error_code
 *
 * Set notepad text and background colors.
 *
 * @param title Notepad window title
 * @param textColour Text color (color name like "red" or hex "#RRGGBB")
 * @param backColour Background color (color name or hex)
 * @return eOK (0) on success, eNoSuchNotepad (30075) if notepad not found,
 *         eInvalidColourName (30077) if color invalid
 */
int L_NotepadColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    qint32 result =
        pDoc->NotepadColour(luaCheckQString(L, 1), luaCheckQString(L, 2), luaCheckQString(L, 3));

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.NotepadReadOnly(title, readOnly) -> error_code
 *
 * Set notepad read-only mode.
 *
 * @param title Notepad window title
 * @param readOnly true to make read-only, false to make editable
 * @return eOK (0) on success, eNoSuchNotepad (30075) if not found
 */
int L_NotepadReadOnly(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Original: optboolean(L, 2, 1) — default true (make read-only if arg missing)
    bool readOnly = (lua_gettop(L) >= 2) ? lua_toboolean(L, 2) : true;

    qint32 result = pDoc->NotepadReadOnly(luaCheckQString(L, 1), readOnly);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.NotepadSaveMethod(title, method) -> error_code
 *
 * Set notepad auto-save method.
 *
 * @param title Notepad window title
 * @param method Save method: 0=ask on close, 1=always save, 2=never save
 * @return eOK (0) on success, eNoSuchNotepad (30075) if not found
 */
int L_NotepadSaveMethod(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 method = luaL_checknumber(L, 2);

    qint32 result = pDoc->NotepadSaveMethod(luaCheckQString(L, 1), method);

    lua_pushnumber(L, result);
    return 1;
}

/**
 * world.MoveNotepadWindow(title, left, top, width, height) -> boolean
 *
 * Move and resize notepad window.
 *
 * @param title Notepad window title
 * @param left Left position in pixels
 * @param top Top position in pixels
 * @param width Width in pixels
 * @param height Height in pixels
 * @return true if notepad found and moved
 */
int L_MoveNotepadWindow(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    qint32 left = luaL_checknumber(L, 2);
    qint32 top = luaL_checknumber(L, 3);
    qint32 width = luaL_checknumber(L, 4);
    qint32 height = luaL_checknumber(L, 5);

    auto result = pDoc->MoveNotepadWindow(luaCheckQString(L, 1), left, top, width, height);

    lua_pushboolean(L, result.has_value() ? 1 : 0);
    return 1;
}

/**
 * world.GetNotepadWindowPosition(title) -> string or nil
 *
 * Get notepad window position and size.
 *
 * @param title Notepad window title
 * @return Position string "left,top,width,height", or nil if not found
 */
int L_GetNotepadWindowPosition(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    NotepadWidget* notepad = pDoc->FindNotepad(luaCheckQString(L, 1));

    if (!notepad || !notepad->m_pMdiSubWindow) {
        // Original returns nil (no push, return 0) when notepad not found
        return 0;
    }

    // Original Lua API returns a table with named keys (not a comma string)
    // Reference: lua_methods.cpp L_GetNotepadWindowPosition uses lua_newtable + MakeTableItem
    QRect geometry = notepad->m_pMdiSubWindow->geometry();
    lua_newtable(L);
    lua_pushnumber(L, geometry.left());
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, geometry.top());
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, geometry.width());
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, geometry.height());
    lua_setfield(L, -2, "height");
    return 1;
}
