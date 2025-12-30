/**
 * lua_dialog_callbacks.cpp - Callback implementations for Lua dialog functions
 *
 * Provides storage for dialog callback function pointers and default
 * implementations using Qt's built-in dialogs.
 */

#include "lua_dialog_callbacks.h"
#include <QInputDialog>

namespace LuaDialogCallbacks {

// Static storage for callbacks
static ChooseDialogFunc s_chooseCallback = nullptr;
static ListDialogFunc s_listCallback = nullptr;
static MultiListDialogFunc s_multiListCallback = nullptr;
static InputBoxDialogFunc s_inputBoxCallback = nullptr;

/**
 * Default choose dialog using QInputDialog
 */
static LuaDialogResult defaultChooseDialog(const QString& title, const QString& message,
                                           const QStringList& items, int defaultIndex)
{
    LuaDialogResult result;
    result.accepted = false;
    result.selectedIndex = -1;

    if (items.isEmpty()) {
        return result;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(nullptr, title, message, items,
                                             qBound(0, defaultIndex, items.size() - 1), false, &ok);

    if (ok) {
        result.accepted = true;
        result.selectedIndex = items.indexOf(selected);
    }

    return result;
}

/**
 * Default list dialog using QInputDialog (same as choose for default)
 */
static LuaDialogResult defaultListDialog(const QString& title, const QString& message,
                                         const QStringList& items, int defaultIndex)
{
    // Use same implementation as choose for fallback
    return defaultChooseDialog(title, message, items, defaultIndex);
}

/**
 * Default multi-list dialog - returns empty (not supported without custom dialog)
 */
static LuaDialogResult defaultMultiListDialog(const QString& title, const QString& message,
                                              const QStringList& items,
                                              const QList<int>& defaultIndices)
{
    Q_UNUSED(title);
    Q_UNUSED(message);
    Q_UNUSED(items);
    Q_UNUSED(defaultIndices);

    // Multi-select requires custom dialog - return canceled
    LuaDialogResult result;
    result.accepted = false;
    result.selectedIndex = -1;
    return result;
}

/**
 * Default input box using QInputDialog
 */
static std::pair<bool, QString> defaultInputBoxDialog(const QString& title, const QString& prompt,
                                                      const QString& defaultText)
{
    bool ok = false;
    QString text =
        QInputDialog::getText(nullptr, title, prompt, QLineEdit::Normal, defaultText, &ok);

    return {ok, text};
}

// Setter functions
void setChooseDialogCallback(ChooseDialogFunc callback)
{
    s_chooseCallback = callback;
}

void setListDialogCallback(ListDialogFunc callback)
{
    s_listCallback = callback;
}

void setMultiListDialogCallback(MultiListDialogFunc callback)
{
    s_multiListCallback = callback;
}

void setInputBoxDialogCallback(InputBoxDialogFunc callback)
{
    s_inputBoxCallback = callback;
}

// Getter functions - return callback or default implementation
ChooseDialogFunc getChooseDialogCallback()
{
    return s_chooseCallback ? s_chooseCallback : defaultChooseDialog;
}

ListDialogFunc getListDialogCallback()
{
    return s_listCallback ? s_listCallback : defaultListDialog;
}

MultiListDialogFunc getMultiListDialogCallback()
{
    return s_multiListCallback ? s_multiListCallback : defaultMultiListDialog;
}

InputBoxDialogFunc getInputBoxDialogCallback()
{
    return s_inputBoxCallback ? s_inputBoxCallback : defaultInputBoxDialog;
}

} // namespace LuaDialogCallbacks

namespace ToolbarCallbacks {

// Static storage for callbacks
static SetToolBarPositionFunc s_setToolBarPositionCallback = nullptr;
static GetToolBarInfoFunc s_getToolBarInfoCallback = nullptr;

// Default implementations (return error/invalid when callbacks not registered)
static int defaultSetToolBarPosition(int which, bool floating, int side, int top, int left)
{
    Q_UNUSED(which);
    Q_UNUSED(floating);
    Q_UNUSED(side);
    Q_UNUSED(top);
    Q_UNUSED(left);
    return -1; // Error - callback not registered
}

static int defaultGetToolBarInfo(int which, int infoType)
{
    Q_UNUSED(which);
    Q_UNUSED(infoType);
    return -1; // Invalid - callback not registered
}

// Setter functions
void setSetToolBarPositionCallback(SetToolBarPositionFunc callback)
{
    s_setToolBarPositionCallback = callback;
}

void setGetToolBarInfoCallback(GetToolBarInfoFunc callback)
{
    s_getToolBarInfoCallback = callback;
}

// Getter functions
SetToolBarPositionFunc getSetToolBarPositionCallback()
{
    return s_setToolBarPositionCallback ? s_setToolBarPositionCallback : defaultSetToolBarPosition;
}

GetToolBarInfoFunc getGetToolBarInfoCallback()
{
    return s_getToolBarInfoCallback ? s_getToolBarInfoCallback : defaultGetToolBarInfo;
}

} // namespace ToolbarCallbacks

namespace InfoBarCallbacks {

// Static storage for callbacks
static ShowInfoBarFunc s_showInfoBarCallback = nullptr;
static InfoBarAppendFunc s_infoBarAppendCallback = nullptr;
static InfoBarClearFunc s_infoBarClearCallback = nullptr;
static InfoBarSetColorFunc s_infoBarSetColorCallback = nullptr;
static InfoBarSetFontFunc s_infoBarSetFontCallback = nullptr;
static InfoBarSetBackgroundFunc s_infoBarSetBackgroundCallback = nullptr;

// Setter functions
void setShowInfoBarCallback(ShowInfoBarFunc callback)
{
    s_showInfoBarCallback = callback;
}

void setInfoBarAppendCallback(InfoBarAppendFunc callback)
{
    s_infoBarAppendCallback = callback;
}

void setInfoBarClearCallback(InfoBarClearFunc callback)
{
    s_infoBarClearCallback = callback;
}

void setInfoBarSetColorCallback(InfoBarSetColorFunc callback)
{
    s_infoBarSetColorCallback = callback;
}

void setInfoBarSetFontCallback(InfoBarSetFontFunc callback)
{
    s_infoBarSetFontCallback = callback;
}

void setInfoBarSetBackgroundCallback(InfoBarSetBackgroundFunc callback)
{
    s_infoBarSetBackgroundCallback = callback;
}

// Getter functions
ShowInfoBarFunc getShowInfoBarCallback()
{
    return s_showInfoBarCallback;
}

InfoBarAppendFunc getInfoBarAppendCallback()
{
    return s_infoBarAppendCallback;
}

InfoBarClearFunc getInfoBarClearCallback()
{
    return s_infoBarClearCallback;
}

InfoBarSetColorFunc getInfoBarSetColorCallback()
{
    return s_infoBarSetColorCallback;
}

InfoBarSetFontFunc getInfoBarSetFontCallback()
{
    return s_infoBarSetFontCallback;
}

InfoBarSetBackgroundFunc getInfoBarSetBackgroundCallback()
{
    return s_infoBarSetBackgroundCallback;
}

} // namespace InfoBarCallbacks
