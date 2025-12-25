#ifndef MXP_SCRIPT_ROUTINES_DIALOG_H
#define MXP_SCRIPT_ROUTINES_DIALOG_H

#include <QDialog>
#include <QLineEdit>

// Forward declarations
class WorldDocument;

/**
 * MxpScriptRoutinesDialog - Dialog for configuring MXP callback script routine names
 *
 * Provides controls for:
 * - Script to call when MXP starts
 * - Script to call when MXP stops
 * - Script to call on open tag
 * - Script to call on close tag
 * - Script to call when variable is set
 * - Script to call on MXP error
 *
 * Based on MUSHclient's MXP script routines configuration dialog.
 */
class MxpScriptRoutinesDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure MXP script routines for
     * @param parent Parent widget
     */
    explicit MxpScriptRoutinesDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~MxpScriptRoutinesDialog() override = default;

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
    QLineEdit* m_onMxpStart;
    QLineEdit* m_onMxpStop;
    QLineEdit* m_onMxpOpenTag;
    QLineEdit* m_onMxpCloseTag;
    QLineEdit* m_onMxpSetVariable;
    QLineEdit* m_onMxpError;
};

#endif // MXP_SCRIPT_ROUTINES_DIALOG_H
