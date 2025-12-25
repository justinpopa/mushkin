#ifndef LUA_CHOOSE_LIST_MULTI_DIALOG_H
#define LUA_CHOOSE_LIST_MULTI_DIALOG_H

#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>

// Forward declarations
class QListWidget;
class QLabel;

/**
 * LuaChooseListMultiDialog - Let users select multiple items from a list
 *
 * A dialog for Lua scripts to present a list of choices to the user
 * via a list widget with multi-selection support.
 *
 * Features:
 * - Displays a message/prompt
 * - Presents choices in a list widget with extended selection
 * - Supports default selections (multiple indices)
 * - Returns selected indices and texts
 * - Double-click accepts selection
 *
 * Used by Lua API for interactive multi-choice selection.
 */
class LuaChooseListMultiDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaChooseListMultiDialog(const QString& title, const QString& message,
                                      const QStringList& items,
                                      const QList<int>& defaultIndices = QList<int>(),
                                      QWidget* parent = nullptr);
    ~LuaChooseListMultiDialog() override = default;

    // Accessors
    QList<int> selectedIndices() const;
    QStringList selectedTexts() const;

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
    QList<int> m_defaultIndices;
};

#endif // LUA_CHOOSE_LIST_MULTI_DIALOG_H
