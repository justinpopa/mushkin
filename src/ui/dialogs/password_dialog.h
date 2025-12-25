#ifndef PASSWORD_DIALOG_H
#define PASSWORD_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QLineEdit;

/**
 * PasswordDialog - Prompt for character name and password
 *
 * A simple dialog for collecting character credentials.
 *
 * Features:
 * - Character name field
 * - Password field (masked input)
 * - Pre-fill character name if known
 * - Focus on password field on open
 *
 * Based on original MUSHclient's password prompt functionality
 */
class PasswordDialog : public QDialog {
    Q_OBJECT

  public:
    explicit PasswordDialog(const QString& title = "Enter Password", QWidget* parent = nullptr);
    ~PasswordDialog() override = default;

    // Accessors
    QString character() const;
    QString password() const;
    void setCharacter(const QString& name);

  private:
    void setupUi();

    // UI Components
    QLineEdit* m_characterEdit;
    QLineEdit* m_passwordEdit;
};

#endif // PASSWORD_DIALOG_H
