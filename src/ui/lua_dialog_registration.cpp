/**
 * lua_dialog_registration.cpp - Register UI dialogs with Lua callback system
 *
 * This module connects the ui module's dialog implementations to the world
 * module's Lua callback system, enabling lua_utils.cpp to show custom dialogs.
 */

#include "lua_dialog_registration.h"
#include "dialogs/lua_choose_box_dialog.h"
#include "dialogs/lua_choose_list_dialog.h"
#include "dialogs/lua_choose_list_multi_dialog.h"
#include "dialogs/lua_input_box_dialog.h"
#include "lua_dialog_callbacks.h"
#include "main_window.h"
#include "views/output_view.h"
#include "world_document.h"
#include <QApplication>
#include <QDialog>

namespace LuaDialogRegistration {

/**
 * Choose dialog callback implementation
 */
static LuaDialogResult chooseDialogImpl(const QString& title, const QString& message,
                                        const QStringList& items, int defaultIndex)
{
    LuaDialogResult result;
    result.accepted = false;
    result.selectedIndex = -1;

    if (items.isEmpty()) {
        return result;
    }

    LuaChooseBoxDialog dialog(title, message, items, defaultIndex);
    if (dialog.exec() == QDialog::Accepted) {
        result.accepted = true;
        result.selectedIndex = dialog.selectedIndex();
    }

    return result;
}

/**
 * List dialog callback implementation
 */
static LuaDialogResult listDialogImpl(const QString& title, const QString& message,
                                      const QStringList& items, int defaultIndex)
{
    LuaDialogResult result;
    result.accepted = false;
    result.selectedIndex = -1;

    if (items.isEmpty()) {
        return result;
    }

    LuaChooseListDialog dialog(title, message, items, defaultIndex);
    if (dialog.exec() == QDialog::Accepted) {
        result.accepted = true;
        result.selectedIndex = dialog.selectedIndex();
    }

    return result;
}

/**
 * Multi-select list dialog callback implementation
 */
static LuaDialogResult multiListDialogImpl(const QString& title, const QString& message,
                                           const QStringList& items,
                                           const QList<int>& defaultIndices)
{
    LuaDialogResult result;
    result.accepted = false;
    result.selectedIndex = -1;

    if (items.isEmpty()) {
        return result;
    }

    LuaChooseListMultiDialog dialog(title, message, items, defaultIndices);
    if (dialog.exec() == QDialog::Accepted) {
        result.accepted = true;
        result.selectedIndices = dialog.selectedIndices();
        if (!result.selectedIndices.isEmpty()) {
            result.selectedIndex = result.selectedIndices.first();
        }
    }

    return result;
}

/**
 * Input box dialog callback implementation
 */
static std::pair<bool, QString> inputBoxDialogImpl(const QString& title, const QString& prompt,
                                                   const QString& defaultText)
{
    LuaInputBoxDialog dialog(title, prompt, defaultText);
    if (dialog.exec() == QDialog::Accepted) {
        return {true, dialog.inputText()};
    }
    return {false, QString()};
}

/**
 * Get MainWindow instance
 */
static MainWindow* getMainWindow()
{
    // Try to find MainWindow in top-level widgets
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (MainWindow* mainWindow = qobject_cast<MainWindow*>(widget)) {
            return mainWindow;
        }
    }
    return nullptr;
}

/**
 * Set toolbar position callback implementation
 */
static int setToolBarPositionImpl(int which, bool floating, int side, int top, int left)
{
    MainWindow* mainWindow = getMainWindow();
    if (!mainWindow) {
        return -1; // Error - no main window
    }
    return mainWindow->setToolBarPosition(which, floating, side, top, left);
}

/**
 * Get toolbar info callback implementation
 */
static int getToolBarInfoImpl(int which, int infoType)
{
    MainWindow* mainWindow = getMainWindow();
    if (!mainWindow) {
        return 0; // No main window
    }
    return mainWindow->getToolBarInfo(which, infoType);
}

/**
 * InfoBar callback implementations
 */
static void showInfoBarImpl(bool visible)
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->showInfoBar(visible);
    }
}

static void infoBarAppendImpl(const QString& text)
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->infoBarAppend(text);
    }
}

static void infoBarClearImpl()
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->infoBarClear();
    }
}

static void infoBarSetColorImpl(int r, int g, int b)
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->infoBarSetColor(QColor(r, g, b));
    }
}

static void infoBarSetFontImpl(const QString& fontName, int size, int style)
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->infoBarSetFont(fontName, size, style);
    }
}

static void infoBarSetBackgroundImpl(int r, int g, int b)
{
    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->infoBarSetBackground(QColor(r, g, b));
    }
}

void registerDialogCallbacks()
{
    LuaDialogCallbacks::setChooseDialogCallback(chooseDialogImpl);
    LuaDialogCallbacks::setListDialogCallback(listDialogImpl);
    LuaDialogCallbacks::setMultiListDialogCallback(multiListDialogImpl);
    LuaDialogCallbacks::setInputBoxDialogCallback(inputBoxDialogImpl);

    // Register toolbar callbacks
    ToolbarCallbacks::setSetToolBarPositionCallback(setToolBarPositionImpl);
    ToolbarCallbacks::setGetToolBarInfoCallback(getToolBarInfoImpl);

    // Register InfoBar callbacks
    InfoBarCallbacks::setShowInfoBarCallback(showInfoBarImpl);
    InfoBarCallbacks::setInfoBarAppendCallback(infoBarAppendImpl);
    InfoBarCallbacks::setInfoBarClearCallback(infoBarClearImpl);
    InfoBarCallbacks::setInfoBarSetColorCallback(infoBarSetColorImpl);
    InfoBarCallbacks::setInfoBarSetFontCallback(infoBarSetFontImpl);
    InfoBarCallbacks::setInfoBarSetBackgroundCallback(infoBarSetBackgroundImpl);
}

} // namespace LuaDialogRegistration
