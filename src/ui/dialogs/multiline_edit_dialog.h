#ifndef MULTILINE_EDIT_DIALOG_H
#define MULTILINE_EDIT_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QPlainTextEdit;
class QPushButton;

/**
 * MultilineEditDialog - Reusable multi-line text editor dialog
 *
 * A general-purpose dialog for editing multi-line text with basic
 * text editing features like go to line, word completion, and function list.
 *
 * Features:
 * - Large plain text editing area with monospace font
 * - Go to line functionality
 * - Word completion (placeholder)
 * - Function list (placeholder)
 * - Can be made read-only
 *
 * Based on original MUSHclient's CEditMultiLine
 */
class MultilineEditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Construct a multiline edit dialog
     * @param title Dialog window title
     * @param initialText Initial text content
     * @param parent Parent widget
     */
    explicit MultilineEditDialog(const QString& title, const QString& initialText = QString(),
                                 QWidget* parent = nullptr);

    // Get/set text
    QString text() const;
    void setText(const QString& text);

    // Make editor read-only
    void setReadOnly(bool readOnly);

  private slots:
    void onGoToLine();
    void onCompleteWord();
    void onFunctionList();

  private:
    void setupUi();

    // UI Components
    QPlainTextEdit* m_textEdit;
    QPushButton* m_goToLineButton;
    QPushButton* m_completeWordButton;
    QPushButton* m_functionListButton;
};

#endif // MULTILINE_EDIT_DIALOG_H
