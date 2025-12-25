#ifndef DEBUG_WORLD_INPUT_DIALOG_H
#define DEBUG_WORLD_INPUT_DIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QPlainTextEdit;
class QComboBox;
class QPushButton;

/**
 * DebugWorldInputDialog - Send debug/test input to world
 *
 * A dialog for sending test input to the world, with support for
 * inserting special characters like ANSI escape codes, MXP sequences,
 * and other control characters.
 *
 * Features:
 * - Multi-line text input area with monospace font
 * - Combo box with special character options
 * - Insert button to insert selected special at cursor position
 * - Insert Unicode button (placeholder for future functionality)
 * - Standard OK/Cancel buttons
 *
 * Based on original MUSHclient's debug world input dialog
 */
class DebugWorldInputDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Construct a debug world input dialog
     * @param parent Parent widget
     */
    explicit DebugWorldInputDialog(QWidget* parent = nullptr);
    ~DebugWorldInputDialog() override = default;

    // Get/set text
    QString text() const;
    void setText(const QString& text);

  private slots:
    void onInsertSpecialClicked();
    void onInsertUnicodeClicked();

  private:
    void setupUi();
    void populateSpecialCharacters();

    // UI Components
    QPlainTextEdit* m_textEdit;
    QComboBox* m_specialsCombo;
    QPushButton* m_insertButton;
    QPushButton* m_insertUnicodeButton;

    // Special character data structure
    struct SpecialChar {
        QString name;
        QString sequence;
    };
};

#endif // DEBUG_WORLD_INPUT_DIALOG_H
