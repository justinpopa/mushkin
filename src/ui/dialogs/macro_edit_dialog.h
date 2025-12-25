#ifndef MACRO_EDIT_DIALOG_H
#define MACRO_EDIT_DIALOG_H

#include <QButtonGroup>
#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QTextEdit>

// Forward declarations
class WorldDocument;

/**
 * MacroEditDialog - Dialog for editing a single keyboard macro
 *
 * Allows editing of one of the 64 predefined keyboard macros (F1-F12,
 * Shift+F1-F12, Ctrl+F1-F12, Alt+A-Z, etc.).
 *
 * Each macro has:
 * - A fixed key description (e.g., "F1", "Alt+A") - displayed but not editable
 * - Send text: The text to send when the key is pressed
 * - Send type: Replace, Send now, or Insert
 *
 * Based on MUSHclient's CEditMacro dialog.
 */
class MacroEditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for editing a macro
     * @param doc WorldDocument containing the macro
     * @param macroIndex Index of the macro to edit (0-63)
     * @param parent Parent widget
     */
    MacroEditDialog(WorldDocument* doc, int macroIndex, QWidget* parent = nullptr);

    ~MacroEditDialog() override = default;

  private slots:
    /**
     * OK button clicked - validate and save
     */
    void onOk();

    /**
     * Cancel button clicked
     */
    void onCancel();

    /**
     * Edit button clicked - opens multi-line editor
     */
    void onEdit();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load macro data into form fields
     */
    void loadMacroData();

    /**
     * Validate form data
     * @return true if valid, false if validation fails
     */
    bool validateForm();

    /**
     * Save form data to macro
     * @return true if saved successfully
     */
    bool saveMacro();

    // Member variables
    WorldDocument* m_doc;
    int m_macroIndex;           // Index into macro array (0-63)
    QString m_macroDescription; // e.g., "F1", "Alt+A", "north"

    // UI Components
    QLineEdit* m_descriptionEdit;  // Read-only display of key name
    QTextEdit* m_sendTextEdit;     // Text to send
    QButtonGroup* m_sendTypeGroup; // Button group for radio buttons
    QRadioButton* m_replaceRadio;  // Replace command line
    QRadioButton* m_sendNowRadio;  // Send immediately
    QRadioButton* m_insertRadio;   // Insert into command line
};

#endif // MACRO_EDIT_DIALOG_H
