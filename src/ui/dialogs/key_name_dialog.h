#ifndef KEY_NAME_DIALOG_H
#define KEY_NAME_DIALOG_H

#include <QDialog>
#include <Qt>

// Forward declarations
class QLabel;
class QLineEdit;

/**
 * KeyNameDialog - Capture and display key press information
 *
 * Dialog that captures a key press and displays its name in human-readable format.
 * Used for setting up keyboard shortcuts and macros.
 */
class KeyNameDialog : public QDialog {
    Q_OBJECT

  public:
    explicit KeyNameDialog(QWidget* parent = nullptr);
    ~KeyNameDialog() override = default;

    // Get the captured key name (e.g., "Ctrl+Shift+A", "F5")
    QString keyName() const;

    // Get the Qt key code
    int keyCode() const;

    // Get the keyboard modifiers
    Qt::KeyboardModifiers modifiers() const;

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    void setupUi();

    // UI Components
    QLabel* m_instructionLabel;
    QLineEdit* m_keyNameEdit;

    // Captured key information
    QString m_keyName;
    int m_keyCode;
    Qt::KeyboardModifiers m_modifiers;
};

#endif // KEY_NAME_DIALOG_H
