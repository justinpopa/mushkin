#ifndef GO_TO_LINE_DIALOG_H
#define GO_TO_LINE_DIALOG_H

#include <QDialog>

// Forward declarations
class QSpinBox;

/**
 * GoToLineDialog - Navigate to a specific line number
 *
 * Simple dialog that prompts the user for a line number to navigate to.
 * Used for quick navigation in the output buffer.
 */
class GoToLineDialog : public QDialog {
    Q_OBJECT

  public:
    explicit GoToLineDialog(int maxLine = 1, int currentLine = 1, QWidget* parent = nullptr);
    ~GoToLineDialog() override = default;

    // Get the selected line number
    int lineNumber() const;

  private:
    void setupUi();

    // UI Components
    QSpinBox* m_lineNumberSpinBox;
    int m_maxLine;
    int m_currentLine;
};

#endif // GO_TO_LINE_DIALOG_H
