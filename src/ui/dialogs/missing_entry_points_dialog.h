#ifndef MISSING_ENTRY_POINTS_DIALOG_H
#define MISSING_ENTRY_POINTS_DIALOG_H

#include <QDialog>
#include <QTextEdit>

/**
 * MissingEntryPointsDialog - Displays missing script entry points error
 *
 * Shows a list of expected script functions that were not found.
 * This is a simple error dialog for displaying script loading/validation errors
 * where certain required entry points are missing from the script file.
 *
 * This is a display-only dialog with just an OK button to dismiss.
 */
class MissingEntryPointsDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for displaying missing entry points error
     * @param errorMessage The error message listing missing functions
     * @param parent Parent widget
     */
    explicit MissingEntryPointsDialog(const QString& errorMessage, QWidget* parent = nullptr);

    ~MissingEntryPointsDialog() override = default;

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    // Error information
    QString m_errorMessage;

    // UI Components
    QTextEdit* m_errorTextEdit;
};

#endif // MISSING_ENTRY_POINTS_DIALOG_H
