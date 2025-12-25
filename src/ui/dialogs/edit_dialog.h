#ifndef EDIT_DIALOG_H
#define EDIT_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QPlainTextEdit;
class QPushButton;

/**
 * EditDialog - General-purpose text editing dialog with optional regex helper
 *
 * A reusable dialog for editing text with support for regex pattern helpers.
 * Used for editing trigger patterns, alias matches, and other text fields
 * that may contain regular expressions.
 *
 * Features:
 * - Plain text editing area with monospace font
 * - Optional "Regex..." button with popup menu of regex helpers
 * - Resizable dialog
 * - Standard OK/Cancel buttons
 *
 * Based on original MUSHclient's edit dialog
 */
class EditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Construct an edit dialog
     * @param title Dialog window title
     * @param parent Parent widget
     */
    explicit EditDialog(const QString& title, QWidget* parent = nullptr);

    // Get/set text
    QString text() const;
    void setText(const QString& text);

    // Control regex helper button visibility
    void setRegexMode(bool enabled);

  private slots:
    void onRegexButtonClicked();
    void onRegexMenuAction();

  private:
    void setupUi();

    // UI Components
    QPlainTextEdit* m_textEdit;
    QPushButton* m_regexButton;
};

#endif // EDIT_DIALOG_H
