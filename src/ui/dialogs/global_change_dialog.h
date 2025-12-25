#ifndef GLOBAL_CHANGE_DIALOG_H
#define GLOBAL_CHANGE_DIALOG_H

#include <QDialog>

// Forward declarations
class QLineEdit;
class QCheckBox;

/**
 * GlobalChangeDialog - Find and replace text globally
 *
 * Simple dialog for performing find/replace operations.
 * Prompts user for find text, replacement text, and search options.
 */
class GlobalChangeDialog : public QDialog {
    Q_OBJECT

  public:
    explicit GlobalChangeDialog(QWidget* parent = nullptr);
    ~GlobalChangeDialog() override = default;

    // Accessor methods
    QString findText() const;
    QString replaceText() const;
    bool matchCase() const;
    bool matchWholeWord() const;

  private:
    void setupUi();

    // UI Components
    QLineEdit* m_findEdit;
    QLineEdit* m_replaceEdit;
    QCheckBox* m_matchCaseCheckBox;
    QCheckBox* m_matchWholeWordCheckBox;
};

#endif // GLOBAL_CHANGE_DIALOG_H
