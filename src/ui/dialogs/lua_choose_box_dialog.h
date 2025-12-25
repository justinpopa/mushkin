#ifndef LUA_CHOOSE_BOX_DIALOG_H
#define LUA_CHOOSE_BOX_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QComboBox;
class QLabel;

/**
 * LuaChooseBoxDialog - Let users select from a dropdown
 *
 * A simple dialog for Lua scripts to present a list of choices
 * to the user via a dropdown/combobox.
 *
 * Features:
 * - Displays a message/prompt
 * - Presents choices in a combo box
 * - Supports default selection
 * - Returns selected index and text
 *
 * Used by Lua API for interactive choice selection.
 */
class LuaChooseBoxDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaChooseBoxDialog(const QString& title, const QString& message,
                                const QStringList& choices, int defaultIndex = 0,
                                QWidget* parent = nullptr);
    ~LuaChooseBoxDialog() override = default;

    // Accessors
    int selectedIndex() const;
    QString selectedText() const;

  private:
    void setupUi();

    // UI Components
    QLabel* m_messageLabel;
    QComboBox* m_choiceCombo;

    // Data
    QString m_title;
    QString m_message;
    QStringList m_choices;
    int m_defaultIndex;
};

#endif // LUA_CHOOSE_BOX_DIALOG_H
