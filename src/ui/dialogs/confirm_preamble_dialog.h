#ifndef CONFIRM_PREAMBLE_DIALOG_H
#define CONFIRM_PREAMBLE_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

/**
 * ConfirmPreambleDialog - Dialog for confirming and editing paste options
 *
 * Provides controls for:
 * - Paste message (read-only information)
 * - Preamble text (prepend before all pasted text)
 * - Postamble text (append after all pasted text)
 * - Line preamble (prepend before each line)
 * - Line postamble (append after each line)
 * - Commented softcode option
 * - Line delay (milliseconds between lines, 0-10000)
 * - Line delay per lines (apply delay every N lines, 1-1000)
 * - Echo pasted text option
 *
 * Based on MUSHclient's paste confirmation dialog.
 */
class ConfirmPreambleDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit ConfirmPreambleDialog(QWidget* parent = nullptr);

    ~ConfirmPreambleDialog() override = default;

    // Getters
    QString pasteMessage() const
    {
        return m_pasteMessage->toPlainText();
    }
    QString preamble() const
    {
        return m_preamble->text();
    }
    QString postamble() const
    {
        return m_postamble->text();
    }
    QString linePreamble() const
    {
        return m_linePreamble->text();
    }
    QString linePostamble() const
    {
        return m_linePostamble->text();
    }
    bool commentedSoftcode() const
    {
        return m_commentedSoftcode->isChecked();
    }
    int lineDelay() const
    {
        return m_lineDelay->value();
    }
    int lineDelayPerLines() const
    {
        return m_lineDelayPerLines->value();
    }
    bool echo() const
    {
        return m_echo->isChecked();
    }

    // Setters
    void setPasteMessage(const QString& message)
    {
        m_pasteMessage->setPlainText(message);
    }
    void setPreamble(const QString& text)
    {
        m_preamble->setText(text);
    }
    void setPostamble(const QString& text)
    {
        m_postamble->setText(text);
    }
    void setLinePreamble(const QString& text)
    {
        m_linePreamble->setText(text);
    }
    void setLinePostamble(const QString& text)
    {
        m_linePostamble->setText(text);
    }
    void setCommentedSoftcode(bool enabled)
    {
        m_commentedSoftcode->setChecked(enabled);
    }
    void setLineDelay(int ms)
    {
        m_lineDelay->setValue(ms);
    }
    void setLineDelayPerLines(int lines)
    {
        m_lineDelayPerLines->setValue(lines);
    }
    void setEcho(bool enabled)
    {
        m_echo->setChecked(enabled);
    }

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    // UI Components
    QTextEdit* m_pasteMessage;
    QLineEdit* m_preamble;
    QLineEdit* m_postamble;
    QLineEdit* m_linePreamble;
    QLineEdit* m_linePostamble;
    QCheckBox* m_commentedSoftcode;
    QSpinBox* m_lineDelay;
    QSpinBox* m_lineDelayPerLines;
    QCheckBox* m_echo;
};

#endif // CONFIRM_PREAMBLE_DIALOG_H
