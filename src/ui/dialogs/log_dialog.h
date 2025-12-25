#ifndef LOG_DIALOG_H
#define LOG_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

// Forward declarations
class WorldDocument;

/**
 * LogDialog - Dialog for configuring logging options
 *
 * Provides controls for:
 * - Number of lines to log (0-500000)
 * - Log file preamble text
 * - Append to log file option
 * - Write world name to log
 * - Log notes, input, and output options
 *
 * Based on MUSHclient's logging configuration dialog.
 */
class LogDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure logging for
     * @param parent Parent widget
     */
    explicit LogDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~LogDialog() override = default;

  private slots:
    /**
     * OK button clicked - validate and save
     */
    void onAccepted();

    /**
     * Cancel button clicked
     */
    void onRejected();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load settings from WorldDocument
     */
    void loadSettings();

    /**
     * Save settings to WorldDocument
     */
    void saveSettings();

    // Member variables
    WorldDocument* m_doc;

    // UI Components
    QSpinBox* m_lines;
    QLineEdit* m_preamble;
    QCheckBox* m_appendToLogFile;
    QCheckBox* m_writeWorldName;
    QCheckBox* m_logNotes;
    QCheckBox* m_logInput;
    QCheckBox* m_logOutput;
};

#endif // LOG_DIALOG_H
