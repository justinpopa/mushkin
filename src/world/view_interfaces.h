/**
 * view_interfaces.h - Abstract interfaces for view components
 *
 * These interfaces allow the world module to interact with UI views
 * without directly depending on the ui module, breaking circular dependencies.
 *
 * The ui module implements these interfaces in OutputView and InputView.
 */

#ifndef VIEW_INTERFACES_H
#define VIEW_INTERFACES_H

#include <QCursor>
#include <QPoint>
#include <QString>

class QWidget;

/**
 * IOutputView - Interface for output view operations
 *
 * Provides methods that the world module needs to interact with
 * the output display without knowing about the concrete OutputView class.
 */
class IOutputView
{
public:
    virtual ~IOutputView() = default;

    // ========== Dimensions ==========
    // Used by GetInfo(281), GetInfo(288) - output window dimensions
    virtual int viewHeight() const = 0;
    virtual int viewWidth() const = 0;

    // ========== Scroll Position ==========
    // Used by GetInfo(296) - scroll bar position
    virtual int getScrollPositionPixels() const = 0;

    // ========== Coordinate Conversion ==========
    // Used by miniwindow popup menu positioning
    virtual QPoint mapToGlobal(const QPoint& pos) const = 0;

    // ========== Cursor Control ==========
    // Used by SetCursor() Lua function
    virtual void setViewCursor(const QCursor& cursor) = 0;

    // ========== Repaint ==========
    // Request view update after miniwindow changes
    virtual void requestUpdate() = 0;

    // ========== Parent Window ==========
    // Used by sound positioning (spatial audio)
    virtual QWidget* parentWindow() const = 0;

    // ========== Background/Foreground Images ==========
    // Used by SetBackgroundImage(), SetForegroundImage()
    virtual void reloadBackgroundImage() = 0;
    virtual void reloadForegroundImage() = 0;

    // ========== Freeze/Pause State ==========
    // Used by Pause() Lua function
    virtual bool isFrozen() const = 0;
    virtual void setFrozen(bool frozen) = 0;
};

/**
 * IInputView - Interface for input view operations
 *
 * Provides methods that the world module needs to interact with
 * the command input without knowing about the concrete InputView class.
 */
class IInputView
{
public:
    virtual ~IInputView() = default;

    // ========== Text Access ==========
    // Used by GetCommand(), SetCommand()
    virtual QString inputText() const = 0;
    virtual void setInputText(const QString& text) = 0;

    // ========== Cursor Position ==========
    // Used by SetCommandSelection()
    virtual int cursorPosition() const = 0;
    virtual void setCursorPosition(int pos) = 0;

    // ========== Selection ==========
    // Used by SetCommandSelection() with length
    virtual void setSelection(int start, int length) = 0;

    // Used by SelectCommand()
    virtual void selectAll() = 0;

    // ========== Clear ==========
    // Used by Execute() when clearing input after send
    virtual void clearInput() = 0;
};

#endif // VIEW_INTERFACES_H
