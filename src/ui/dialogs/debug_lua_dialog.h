#ifndef DEBUG_LUA_DIALOG_H
#define DEBUG_LUA_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

/**
 * DebugLuaDialog - Interactive Lua debugger dialog
 *
 * Shows current execution state and allows debug commands.
 * Displays:
 * - Current line number
 * - Function name and details
 * - Source file name
 * - What type (Lua, C, main, etc.)
 * - Line range info
 * - Number of upvalues
 *
 * Provides debug commands:
 * - Execute custom debug command
 * - Show local variables
 * - Show upvalues
 * - Show stack traceback
 * - Abort execution
 * - Continue execution
 *
 * Based on: Original MFC debug dialog
 */
class DebugLuaDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit DebugLuaDialog(QWidget* parent = nullptr);
    ~DebugLuaDialog() override = default;

    // Setters for debug information
    void setCurrentLine(const QString& line);
    void setFunctionName(const QString& name);
    void setFunctionDetails(const QString& details);
    void setSource(const QString& source);
    void setWhat(const QString& what);
    void setLines(const QString& lines);
    void setNups(const QString& nups);

    // Getters
    QString command() const;
    bool wasAborted() const;

  signals:
    /**
     * Emitted when Execute button is clicked
     */
    void executeCommand(const QString& command);

    /**
     * Emitted when Show Locals button is clicked
     */
    void showLocals();

    /**
     * Emitted when Show Upvalues button is clicked
     */
    void showUpvalues();

    /**
     * Emitted when Traceback button is clicked
     */
    void showTraceback();

    /**
     * Emitted when Abort button is clicked
     */
    void abortExecution();

    /**
     * Emitted when Continue button is clicked
     */
    void continueExecution();

  private slots:
    void onExecute();
    void onShowLocals();
    void onShowUpvalues();
    void onTraceback();
    void onAbort();
    void onContinue();

  private:
    void setupUi();

    // State
    bool m_aborted;
    QString m_command;

    // UI Components - Debug info display (read-only)
    QLineEdit* m_currentLineEdit;
    QLineEdit* m_functionNameEdit;
    QLineEdit* m_sourceEdit;
    QLineEdit* m_whatEdit;
    QLineEdit* m_linesEdit;
    QLineEdit* m_nupsEdit;

    // Command input
    QLineEdit* m_commandEdit;

    // Buttons
    QPushButton* m_executeButton;
    QPushButton* m_showLocalsButton;
    QPushButton* m_showUpvaluesButton;
    QPushButton* m_tracebackButton;
    QPushButton* m_abortButton;
    QPushButton* m_continueButton;
};

#endif // DEBUG_LUA_DIALOG_H
