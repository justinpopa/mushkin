#ifndef MULTILINE_TRIGGER_DIALOG_H
#define MULTILINE_TRIGGER_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QPlainTextEdit;
class QCheckBox;

/**
 * MultilineTriggerDialog - Dialog for editing multi-line trigger patterns
 *
 * This dialog allows users to define triggers that match across multiple lines
 * of output from the MUD server. Unlike single-line triggers, multi-line triggers
 * can detect patterns that span several consecutive lines.
 *
 * Features:
 * - Multi-line pattern text editor with monospace font
 * - Case-sensitive matching option
 *
 * Based on original MUSHclient's multi-line trigger dialog
 */
class MultilineTriggerDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Construct a multiline trigger dialog
     * @param parent Parent widget
     */
    explicit MultilineTriggerDialog(QWidget* parent = nullptr);

    // Get/set trigger pattern text
    QString triggerText() const;
    void setTriggerText(const QString& text);

    // Get/set case matching option
    bool matchCase() const;
    void setMatchCase(bool match);

  private:
    void setupUi();

    // UI Components
    QPlainTextEdit* m_textEdit;
    QCheckBox* m_matchCaseCheck;
};

#endif // MULTILINE_TRIGGER_DIALOG_H
