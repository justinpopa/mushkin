/**
 * lua_dialog_callbacks.h - Callback interface for Lua dialog functions
 *
 * This header provides a callback interface that allows the world module
 * to show dialogs without directly linking against the ui module.
 *
 * The ui module registers its implementations at startup, and lua_utils.cpp
 * calls through these function pointers.
 */

#ifndef LUA_DIALOG_CALLBACKS_H
#define LUA_DIALOG_CALLBACKS_H

#include <QString>
#include <QStringList>
#include <functional>

/**
 * Result from choose/listbox dialogs
 */
struct LuaDialogResult {
    bool accepted;              // True if user clicked OK
    int selectedIndex;          // Selected index (for single-select)
    QList<int> selectedIndices; // Selected indices (for multi-select)
};

/**
 * Callback types for dialog functions
 */
namespace LuaDialogCallbacks {

// Choose dialog (dropdown combo box)
using ChooseDialogFunc = std::function<LuaDialogResult(const QString& title, const QString& message,
                                                       const QStringList& items, int defaultIndex)>;

// List dialog (single-select list)
using ListDialogFunc = std::function<LuaDialogResult(const QString& title, const QString& message,
                                                     const QStringList& items, int defaultIndex)>;

// Multi-select list dialog
using MultiListDialogFunc =
    std::function<LuaDialogResult(const QString& title, const QString& message,
                                  const QStringList& items, const QList<int>& defaultIndices)>;

// Input box dialog
using InputBoxDialogFunc = std::function<std::pair<bool, QString>(
    const QString& title, const QString& prompt, const QString& defaultText)>;

/**
 * Register callbacks - called by ui module at startup
 */
void setChooseDialogCallback(ChooseDialogFunc callback);
void setListDialogCallback(ListDialogFunc callback);
void setMultiListDialogCallback(MultiListDialogFunc callback);
void setInputBoxDialogCallback(InputBoxDialogFunc callback);

/**
 * Get callbacks - called by lua_utils.cpp
 */
ChooseDialogFunc getChooseDialogCallback();
ListDialogFunc getListDialogCallback();
MultiListDialogFunc getMultiListDialogCallback();
InputBoxDialogFunc getInputBoxDialogCallback();

} // namespace LuaDialogCallbacks

// Forward declaration for WorldDocument (used by view callbacks)
class WorldDocument;

/**
 * Callback types for view updates
 * These allow world module to trigger UI updates without linking against ui module
 */
namespace ViewUpdateCallbacks {

// Reload background/foreground image in output view
using ReloadBackgroundImageFunc = std::function<void(WorldDocument*)>;
using ReloadForegroundImageFunc = std::function<void(WorldDocument*)>;

// Set freeze/pause state on output view
using SetFreezeFunc = std::function<void(WorldDocument*, bool)>;

// Get freeze state from output view
using GetFreezeFunc = std::function<bool(WorldDocument*)>;

/**
 * Register callbacks - called by ui module at startup
 */
void setReloadBackgroundImageCallback(ReloadBackgroundImageFunc callback);
void setReloadForegroundImageCallback(ReloadForegroundImageFunc callback);
void setSetFreezeCallback(SetFreezeFunc callback);
void setGetFreezeCallback(GetFreezeFunc callback);

/**
 * Get callbacks - called by world module
 */
ReloadBackgroundImageFunc getReloadBackgroundImageCallback();
ReloadForegroundImageFunc getReloadForegroundImageCallback();
SetFreezeFunc getSetFreezeCallback();
GetFreezeFunc getGetFreezeCallback();

} // namespace ViewUpdateCallbacks

/**
 * Callback types for toolbar control
 * These allow world module to control toolbars without linking against ui module
 */
namespace ToolbarCallbacks {

// Set toolbar position
// which: 1=main, 2=game, 3=activity
// floating: true to float, false to dock
// side: 1=top, 2=bottom, 3=left, 4=right (for docking)
// top, left: position (for floating)
// Returns: 0 on success, error code on failure
using SetToolBarPositionFunc =
    std::function<int(int which, bool floating, int side, int top, int left)>;

// Get toolbar info (dimensions)
// which: 1=main, 2=game, 3=activity
// infoType: 0=height, 1=width
// Returns: dimension in pixels, or -1 if invalid
using GetToolBarInfoFunc = std::function<int(int which, int infoType)>;

/**
 * Register callbacks - called by ui module at startup
 */
void setSetToolBarPositionCallback(SetToolBarPositionFunc callback);
void setGetToolBarInfoCallback(GetToolBarInfoFunc callback);

/**
 * Get callbacks - called by world module
 */
SetToolBarPositionFunc getSetToolBarPositionCallback();
GetToolBarInfoFunc getGetToolBarInfoCallback();

} // namespace ToolbarCallbacks

/**
 * Callback types for Info Bar control
 * These allow world module to control the Info Bar without linking against ui module
 */
namespace InfoBarCallbacks {

// Show/hide the info bar
using ShowInfoBarFunc = std::function<void(bool visible)>;

// Append text to the info bar
using InfoBarAppendFunc = std::function<void(const QString& text)>;

// Clear the info bar
using InfoBarClearFunc = std::function<void()>;

// Set text color (takes RGB values)
using InfoBarSetColorFunc = std::function<void(int r, int g, int b)>;

// Set font (fontName, size, style bits: 1=bold, 2=italic, 4=underline, 8=strikeout)
using InfoBarSetFontFunc = std::function<void(const QString& fontName, int size, int style)>;

// Set background color (takes RGB values)
using InfoBarSetBackgroundFunc = std::function<void(int r, int g, int b)>;

/**
 * Register callbacks - called by ui module at startup
 */
void setShowInfoBarCallback(ShowInfoBarFunc callback);
void setInfoBarAppendCallback(InfoBarAppendFunc callback);
void setInfoBarClearCallback(InfoBarClearFunc callback);
void setInfoBarSetColorCallback(InfoBarSetColorFunc callback);
void setInfoBarSetFontCallback(InfoBarSetFontFunc callback);
void setInfoBarSetBackgroundCallback(InfoBarSetBackgroundFunc callback);

/**
 * Get callbacks - called by world module
 */
ShowInfoBarFunc getShowInfoBarCallback();
InfoBarAppendFunc getInfoBarAppendCallback();
InfoBarClearFunc getInfoBarClearCallback();
InfoBarSetColorFunc getInfoBarSetColorCallback();
InfoBarSetFontFunc getInfoBarSetFontCallback();
InfoBarSetBackgroundFunc getInfoBarSetBackgroundCallback();

} // namespace InfoBarCallbacks

#endif // LUA_DIALOG_CALLBACKS_H
