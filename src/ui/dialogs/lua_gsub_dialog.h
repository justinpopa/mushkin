#ifndef LUA_GSUB_DIALOG_H
#define LUA_GSUB_DIALOG_H

#include <QDialog>

// Forward declarations
class QLineEdit;
class QTextEdit;
class QCheckBox;
class QLabel;
class QPushButton;

/**
 * LuaGsubDialog - UI for Lua's string.gsub function
 *
 * Provides a dialog interface for find and replace with regex support,
 * matching Lua's string.gsub functionality. Allows processing text with
 * regular expressions, escape sequences, and optional function-based replacements.
 *
 * Based on original MUSHclient's Lua gsub dialog.
 */
class LuaGsubDialog : public QDialog {
    Q_OBJECT

  public:
    explicit LuaGsubDialog(QWidget* parent = nullptr);
    ~LuaGsubDialog() override = default;

    // Accessor methods
    QString findPattern() const;
    QString replacementText() const;
    bool processEachLine() const;
    bool interpretEscapeSequences() const;
    bool callFunction() const;
    QString functionText() const;

    // Setter methods
    void setFindPattern(const QString& pattern);
    void setReplacementText(const QString& text);
    void setProcessEachLine(bool enabled);
    void setInterpretEscapeSequences(bool enabled);
    void setCallFunction(bool enabled);
    void setFunctionText(const QString& text);
    void setSelectionSizeInfo(const QString& info);

  private slots:
    void onEditFindPattern();
    void onEditReplacement();
    void onCallFunctionToggled(bool checked);

  private:
    void setupUi();
    void updateReplacementFieldState();

    // UI Components
    QLineEdit* m_findPatternEdit;
    QPushButton* m_editFindButton;

    QLineEdit* m_replacementEdit;
    QTextEdit* m_functionTextEdit;
    QPushButton* m_editReplacementButton;

    QCheckBox* m_eachLineCheckBox;
    QCheckBox* m_escapeSequencesCheckBox;
    QCheckBox* m_callFunctionCheckBox;

    QLabel* m_selectionSizeLabel;
    QLabel* m_replacementLabel;
    QLabel* m_functionLabel;
};

#endif // LUA_GSUB_DIALOG_H
