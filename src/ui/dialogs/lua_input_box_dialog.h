#ifndef LUA_INPUT_BOX_DIALOG_H
#define LUA_INPUT_BOX_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QLineEdit;

/**
 * LuaInputBoxDialog - Prompt for user text input from Lua scripts
 *
 * A simple dialog for collecting text input from the user, typically
 * called from Lua scripts via the utils.inputbox() API.
 *
 * Features:
 * - Customizable title and message
 * - Single-line text input field
 * - Pre-fill with default value if provided
 * - Focus on input field on open
 *
 * Based on original MUSHclient's utils.inputbox functionality
 */
class LuaInputBoxDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaInputBoxDialog(const QString& title, const QString& message,
                               const QString& defaultValue = QString(), QWidget* parent = nullptr);
    ~LuaInputBoxDialog() override = default;

    // Accessors
    QString inputText() const;
    void setInputText(const QString& text);

  private:
    void setupUi(const QString& message);

    // UI Components
    QLineEdit* m_inputEdit;
};

#endif // LUA_INPUT_BOX_DIALOG_H
