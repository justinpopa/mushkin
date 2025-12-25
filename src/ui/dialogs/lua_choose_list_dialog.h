#ifndef LUA_CHOOSE_LIST_DIALOG_H
#define LUA_CHOOSE_LIST_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QListWidget;
class QLabel;

/**
 * LuaChooseListDialog - Let users select from a list
 *
 * A simple dialog for Lua scripts to present a list of choices
 * to the user via a list widget.
 *
 * Features:
 * - Displays a message/prompt
 * - Presents choices in a list widget
 * - Supports default selection
 * - Returns selected index and text
 * - Double-click accepts selection
 *
 * Used by Lua API for interactive choice selection.
 */
class LuaChooseListDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaChooseListDialog(const QString& title, const QString& message,
                                 const QStringList& items, int defaultIndex = 0,
                                 QWidget* parent = nullptr);
    ~LuaChooseListDialog() override = default;

    // Accessors
    int selectedIndex() const;
    QString selectedText() const;

  private slots:
    void onItemDoubleClicked();

  private:
    void setupUi();

    // UI Components
    QLabel* m_messageLabel;
    QListWidget* m_listWidget;

    // Data
    QString m_title;
    QString m_message;
    QStringList m_items;
    int m_defaultIndex;
};

#endif // LUA_CHOOSE_LIST_DIALOG_H
