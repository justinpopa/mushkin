#ifndef INSERT_UNICODE_DIALOG_H
#define INSERT_UNICODE_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QLineEdit;
class QCheckBox;
class QLabel;

/**
 * InsertUnicodeDialog - Insert Unicode characters by code point
 *
 * A dialog for inserting Unicode characters by entering their code point
 * in either hexadecimal (default) or decimal format.
 *
 * Features:
 * - Code point input field (e.g., "263A" or "9786")
 * - Hexadecimal/Decimal toggle
 * - Real-time preview of the Unicode character
 * - Input validation
 *
 * Usage:
 *   InsertUnicodeDialog dialog(this);
 *   if (dialog.exec() == QDialog::Accepted) {
 *       QString ch = dialog.character();
 *       int cp = dialog.codePoint();
 *       // Insert character into text
 *   }
 */
class InsertUnicodeDialog : public QDialog {
    Q_OBJECT

  public:
    explicit InsertUnicodeDialog(QWidget* parent = nullptr);
    ~InsertUnicodeDialog() override = default;

    // Accessors
    QString character() const;
    int codePoint() const;

  private slots:
    void updatePreview();

  private:
    void setupUi();

    // UI Components
    QLineEdit* m_codePointEdit;
    QCheckBox* m_hexCheckBox;
    QLabel* m_previewLabel;

    // Current state
    int m_currentCodePoint;
};

#endif // INSERT_UNICODE_DIALOG_H
