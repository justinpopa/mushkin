#ifndef SCRIPT_ERROR_DIALOG_H
#define SCRIPT_ERROR_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

/**
 * ScriptErrorDialog - Displays Lua script errors with context information
 *
 * Shows error details including:
 * - Error description
 * - Error number
 * - Event that triggered the error
 * - What called the script (caller context)
 * - Where the error was raised (source)
 *
 * This is a display-only dialog with the option to copy error details
 * to the output window for logging/debugging purposes.
 */
class ScriptErrorDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for displaying a script error
     * @param description Error description/message
     * @param errorNum Error number or code
     * @param event Event name that triggered the script
     * @param calledBy What invoked the script (context)
     * @param raisedBy Source that raised the error
     * @param parent Parent widget
     */
    explicit ScriptErrorDialog(const QString& description, const QString& errorNum,
                               const QString& event, const QString& calledBy,
                               const QString& raisedBy, QWidget* parent = nullptr);

    ~ScriptErrorDialog() override = default;

  private slots:
    /**
     * Copy error details to output window
     */
    void onCopyToOutput();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    // Error information
    QString m_description;
    QString m_errorNum;
    QString m_event;
    QString m_calledBy;
    QString m_raisedBy;

    // UI Components
    QLineEdit* m_descriptionEdit;
    QLineEdit* m_errorNumEdit;
    QLineEdit* m_eventEdit;
    QLineEdit* m_calledByEdit;
    QLineEdit* m_raisedByEdit;
    QPushButton* m_copyButton;
};

#endif // SCRIPT_ERROR_DIALOG_H
