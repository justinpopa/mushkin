#ifndef REGEXP_PROBLEM_DIALOG_H
#define REGEXP_PROBLEM_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

/**
 * RegexpProblemDialog - Displays regular expression syntax errors
 *
 * Shows error details including:
 * - The regex pattern that failed
 * - Error message describing what went wrong
 * - Error position in the pattern (if available)
 * - Visual highlighting of the error position
 *
 * This is a display-only dialog to help users fix regex patterns.
 */
class RegexpProblemDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for displaying a regex error
     * @param pattern The regular expression pattern that failed
     * @param errorMessage Error description/message
     * @param errorPosition Position in pattern where error occurred (-1 if unknown)
     * @param parent Parent widget
     */
    explicit RegexpProblemDialog(const QString& pattern, const QString& errorMessage,
                                 int errorPosition = -1, QWidget* parent = nullptr);

    ~RegexpProblemDialog() override = default;

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    // Error information
    QString m_pattern;
    QString m_errorMessage;
    int m_errorPosition;

    // UI Components
    QLineEdit* m_patternEdit;
    QLabel* m_errorLabel;
    QLabel* m_positionLabel;
};

#endif // REGEXP_PROBLEM_DIALOG_H
