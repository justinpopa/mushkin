/**
 * lua_progress.cpp - Progress Dialog Lua Library
 *
 * Implements the progress.* library for showing progress dialogs during long-running operations.
 *
 * Original MUSHclient implementation: scripting/lua_progressdlg.cpp
 *
 * Usage:
 *   local dlg = progress.new("Loading...")
 *   dlg:range(0, 100)
 *   for i = 1, 100 do
 *     dlg:position(i)
 *     dlg:status("Processing " .. i)
 *     -- do work
 *     if dlg:checkcancel() then
 *       break
 *     end
 *   end
 *   dlg:close()
 */

#include <QApplication>
#include <QProgressDialog>
#include <QVariant>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

static const char PROGRESS_DLG_HANDLE[] = "mushclient.progress_dialog_handle";

// Get the QProgressDialog from Lua userdata
static QProgressDialog* getProgressDialog(lua_State* L)
{
    QProgressDialog** ud = (QProgressDialog**)luaL_checkudata(L, 1, PROGRESS_DLG_HANDLE);
    luaL_argcheck(L, *ud != nullptr, 1, "progress dialog userdata expected");
    return *ud;
}

// progress_dlg:status(text) - Set the label text
static int Lprogress_status(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);
    const char* text = luaL_checkstring(L, 2);
    dlg->setLabelText(QString::fromUtf8(text));

    // Process events to keep UI responsive
    QApplication::processEvents();

    return 0;
}

// progress_dlg:range(min, max) - Set progress bar range
static int Lprogress_range(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);
    int min = luaL_checkinteger(L, 2);
    int max = luaL_checkinteger(L, 3);
    dlg->setRange(min, max);
    return 0;
}

// progress_dlg:position(value) - Set current progress position
static int Lprogress_position(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);
    int pos = luaL_checkinteger(L, 2);
    dlg->setValue(pos);

    // Process events to keep UI responsive
    QApplication::processEvents();

    return 0;
}

// progress_dlg:setstep(increment) - Set step increment
static int Lprogress_setstep(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);
    int step = luaL_checkinteger(L, 2);

    // Store step value using Qt's property system
    dlg->setProperty("stepIncrement", step);

    return 0;
}

// progress_dlg:step() - Advance progress by step amount
static int Lprogress_step(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);

    // Get step from Qt property
    QVariant stepVar = dlg->property("stepIncrement");
    int step = stepVar.isValid() ? stepVar.toInt() : 1;

    dlg->setValue(dlg->value() + step);

    // Process events to keep UI responsive
    QApplication::processEvents();

    return 0;
}

// progress_dlg:checkcancel() - Check if user clicked cancel button
static int Lprogress_checkcancel(lua_State* L)
{
    QProgressDialog* dlg = getProgressDialog(L);
    lua_pushboolean(L, dlg->wasCanceled());
    return 1;
}

// progress_dlg:close() or __gc - Close/destroy the dialog
static int Lprogress_gc(lua_State* L)
{
    QProgressDialog** ud = (QProgressDialog**)luaL_checkudata(L, 1, PROGRESS_DLG_HANDLE);
    QProgressDialog* dlg = *ud;

    if (dlg) {
        dlg->close();
        delete dlg;
        *ud = nullptr;
    }

    return 0;
}

// __tostring metamethod
static int Lprogress_tostring(lua_State* L)
{
    lua_pushstring(L, "progress_dialog");
    return 1;
}

// progress.new(title) - Create a new progress dialog
static int Lprogress_new(lua_State* L)
{
    const char* title = luaL_optstring(L, 1, "Progress...");

    // Create the progress dialog
    QProgressDialog* dlg = new QProgressDialog();
    dlg->setWindowTitle(QString::fromUtf8(title));
    dlg->setLabelText(QString::fromUtf8(title));
    dlg->setRange(0, 100);
    dlg->setValue(0);
    dlg->setWindowModality(Qt::ApplicationModal);
    dlg->setAutoClose(false);
    dlg->setAutoReset(false);
    dlg->show();

    // Initialize default step value
    dlg->setProperty("stepIncrement", 1);

    // Create userdata
    QProgressDialog** ud = (QProgressDialog**)lua_newuserdata(L, sizeof(QProgressDialog*));
    *ud = dlg;

    // Set metatable
    luaL_getmetatable(L, PROGRESS_DLG_HANDLE);
    lua_setmetatable(L, -2);

    return 1;
}

// Metatable for progress dialog objects
static const luaL_Reg progress_dialog_meta[] = {{"__gc", Lprogress_gc},
                                                {"__tostring", Lprogress_tostring},
                                                {"checkcancel", Lprogress_checkcancel},
                                                {"close", Lprogress_gc},
                                                {"position", Lprogress_position},
                                                {"range", Lprogress_range},
                                                {"setstep", Lprogress_setstep},
                                                {"status", Lprogress_status},
                                                {"step", Lprogress_step},
                                                {nullptr, nullptr}};

// Library functions
static const luaL_Reg progress_dialog_lib[] = {{"new", Lprogress_new}, {nullptr, nullptr}};

// Create metatable
static void createMeta(lua_State* L, const char* name)
{
    luaL_newmetatable(L, name); // create new metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2); // push metatable
    lua_rawset(L, -3);    // metatable.__index = metatable
}

// Open the progress library
extern "C" int luaopen_progress(lua_State* L)
{
    // Create metatable
    createMeta(L, PROGRESS_DLG_HANDLE);

    // Register metamethods
    for (int i = 0; progress_dialog_meta[i].name != nullptr; i++) {
        lua_pushstring(L, progress_dialog_meta[i].name);
        lua_pushcfunction(L, progress_dialog_meta[i].func);
        lua_rawset(L, -3);
    }

    lua_pop(L, 1); // pop metatable

    // Register library functions
    lua_newtable(L);
    luaL_setfuncs(L, progress_dialog_lib, 0);

    return 1;
}
