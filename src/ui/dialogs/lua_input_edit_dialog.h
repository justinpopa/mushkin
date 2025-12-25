#ifndef LUA_INPUT_EDIT_DIALOG_H
#define LUA_INPUT_EDIT_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QTextEdit;
class QLabel;
class QPushButton;

/**
 * LuaInputEditDialog - Advanced multi-line input dialog for Lua scripts
 *
 * A customizable dialog for collecting multi-line text input from the user,
 * typically called from Lua scripts. This is more advanced than the simple
 * LuaInputBoxDialog and supports extensive customization.
 *
 * Features:
 * - Multi-line text input using QTextEdit
 * - Customizable dialog and component sizes
 * - Customizable font settings
 * - Maximum input length restrictions
 * - Read-only mode support
 * - Customizable button labels
 * - Resizable dialog
 * - Optional default button behavior
 *
 * Based on original MUSHclient's more advanced input functionality
 */
class LuaInputEditDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaInputEditDialog(const QString& title, const QString& message,
                                QWidget* parent = nullptr);
    ~LuaInputEditDialog() override = default;

    // Accessors
    QString replyText() const;

    // Customization setters
    void setFont(const QString& name, int size);
    void setDialogSize(int width, int height);
    void setMaxLength(int max);
    void setReadOnly(bool readOnly);
    void setButtonLabels(const QString& ok, const QString& cancel);
    void setDefaultText(const QString& text);
    void setNoDefault(bool noDefault);

  private:
    void setupUi(const QString& message);

    // UI Components
    QLabel* m_messageLabel;
    QTextEdit* m_replyEdit;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Configuration
    int m_maxLength;
    bool m_noDefault;
};

#endif // LUA_INPUT_EDIT_DIALOG_H
