#ifndef IMMEDIATE_DIALOG_H
#define IMMEDIATE_DIALOG_H

#include <QDialog>

// Forward declarations
class WorldDocument;
class QPlainTextEdit;
class QPushButton;
class QDialogButtonBox;
class QComboBox;

/**
 * ImmediateDialog - Execute Lua code immediately
 *
 * Provides a quick way to execute Lua expressions and commands without
 * having to create a script file or plugin. Useful for:
 * - Testing Lua expressions
 * - Quick commands and calculations
 * - Debugging script code
 * - Interactive Lua REPL-style usage
 *
 * The expression text is preserved between invocations for easy iteration.
 *
 * Based on: ImmediateDlg.h/cpp in original MUSHclient
 * Original dialog: IDD_IMMEDIATE
 */
class ImmediateDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc World document containing script engine
     * @param parent Parent widget
     */
    explicit ImmediateDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~ImmediateDialog() override;

  private slots:
    /**
     * Execute button clicked - run the Lua code
     */
    void executeCode();

    /**
     * Close button clicked - save expression and close
     */
    void closeDialog();

  private:
    // Setup methods
    void setupUi();
    void setupConnections();

    // Member variables
    WorldDocument* m_doc;

    QPlainTextEdit* m_expressionEdit;
    QComboBox* m_languageCombo;

    QPushButton* m_executeButton;
    QDialogButtonBox* m_buttonBox;

    // Stored expression text (preserved between invocations)
    static QString s_lastExpression;
};

#endif // IMMEDIATE_DIALOG_H
