/**
 * world_notepads.cpp - Notepad API Functions
 *
 * Lua API for creating and managing notepad windows
 */

#include "../notepad_widget.h"
#include "../world_document.h"
#include "lua_common.h"

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
    const char* title = luaL_checkstring(L, 1);
    const char* contents = luaL_checkstring(L, 2);

    bool result = pDoc->SendToNotepad(QString::fromUtf8(title), QString::fromUtf8(contents));

    lua_pushboolean(L, result);
    return 1;
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
    const char* title = luaL_checkstring(L, 1);
    const char* contents = luaL_checkstring(L, 2);

    bool result = pDoc->AppendToNotepad(QString::fromUtf8(title), QString::fromUtf8(contents));

    lua_pushboolean(L, result);
    return 1;
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
    const char* title = luaL_checkstring(L, 1);
    const char* contents = luaL_checkstring(L, 2);

    bool result = pDoc->ReplaceNotepad(QString::fromUtf8(title), QString::fromUtf8(contents));

    lua_pushboolean(L, result);
    return 1;
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
    const char* title = luaL_checkstring(L, 1);

    bool result = pDoc->ActivateNotepad(QString::fromUtf8(title));

    lua_pushboolean(L, result);
    return 1;
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
    const char* title = luaL_checkstring(L, 1);
    bool querySave = lua_toboolean(L, 2);

    qint32 result = pDoc->CloseNotepad(QString::fromUtf8(title), querySave);

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
    const char* title = luaL_checkstring(L, 1);

    NotepadWidget* notepad = pDoc->FindNotepad(QString::fromUtf8(title));

    if (!notepad) {
        lua_pushnil(L);
        return 1;
    }

    QString text = notepad->GetText();
    QByteArray ba = text.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());

    return 1;
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
    const char* title = luaL_checkstring(L, 1);

    NotepadWidget* notepad = pDoc->FindNotepad(QString::fromUtf8(title));

    qint32 length = notepad ? notepad->GetLength() : 0;
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

    lua_newtable(L);
    for (int i = 0; i < titles.size(); i++) {
        QByteArray ba = titles[i].toUtf8();
        lua_pushlstring(L, ba.constData(), ba.length());
        lua_rawseti(L, -2, i + 1);
    }

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
    const char* title = luaL_checkstring(L, 1);
    const char* filename = luaL_checkstring(L, 2);
    bool replaceExisting = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : true;

    qint32 result =
        pDoc->SaveNotepad(QString::fromUtf8(title), QString::fromUtf8(filename), replaceExisting);

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
    const char* title = luaL_checkstring(L, 1);
    const char* fontName = luaL_checkstring(L, 2);
    qint32 size = luaL_checknumber(L, 3);
    qint32 style = luaL_checknumber(L, 4);
    qint32 charset = luaL_checknumber(L, 5);

    qint32 result = pDoc->NotepadFont(QString::fromUtf8(title), QString::fromUtf8(fontName), size,
                                      style, charset);

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
    const char* title = luaL_checkstring(L, 1);
    const char* textColour = luaL_checkstring(L, 2);
    const char* backColour = luaL_checkstring(L, 3);

    qint32 result = pDoc->NotepadColour(QString::fromUtf8(title), QString::fromUtf8(textColour),
                                        QString::fromUtf8(backColour));

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
    const char* title = luaL_checkstring(L, 1);
    bool readOnly = lua_toboolean(L, 2);

    qint32 result = pDoc->NotepadReadOnly(QString::fromUtf8(title), readOnly);

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
    const char* title = luaL_checkstring(L, 1);
    qint32 method = luaL_checknumber(L, 2);

    qint32 result = pDoc->NotepadSaveMethod(QString::fromUtf8(title), method);

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
    const char* title = luaL_checkstring(L, 1);
    qint32 left = luaL_checknumber(L, 2);
    qint32 top = luaL_checknumber(L, 3);
    qint32 width = luaL_checknumber(L, 4);
    qint32 height = luaL_checknumber(L, 5);

    bool result = pDoc->MoveNotepadWindow(QString::fromUtf8(title), left, top, width, height);

    lua_pushboolean(L, result);
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
    const char* title = luaL_checkstring(L, 1);

    QString position = pDoc->GetNotepadWindowPosition(QString::fromUtf8(title));

    if (position.isEmpty()) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray ba = position.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());

    return 1;
}
