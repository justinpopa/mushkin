#ifndef VARIABLE_EDIT_DIALOG_H
#define VARIABLE_EDIT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

// Forward declarations
class WorldDocument;
class Variable;

/**
 * VariableEditDialog - Dialog for adding/editing a single variable
 *
 * Provides interface for:
 * - Variable name (with validation)
 * - Variable value (multi-line text)
 *
 * Can operate in two modes:
 * - Add mode: Creates a new variable
 * - Edit mode: Modifies an existing variable
 *
 * Based on MUSHclient's EditVariable dialog.
 */
class VariableEditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for adding a new variable
     * @param doc WorldDocument to add variable to
     * @param parent Parent widget
     */
    explicit VariableEditDialog(WorldDocument* doc, QWidget* parent = nullptr);

    /**
     * Constructor for editing an existing variable
     * @param doc WorldDocument containing the variable
     * @param variableName Name of variable to edit
     * @param parent Parent widget
     */
    VariableEditDialog(WorldDocument* doc, const QString& variableName, QWidget* parent = nullptr);

    ~VariableEditDialog() override = default;

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
     * Edit Contents button clicked - open multi-line editor
     */
    void onEditContents();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load variable data into form fields
     */
    void loadVariableData();

    /**
     * Validate form data
     * @return true if valid, false if validation fails
     */
    bool validateForm();

    /**
     * Save form data to variable
     * @return true if saved successfully
     */
    bool saveVariable();

    /**
     * Validate variable name format
     * @param name Variable name to validate
     * @return true if name is invalid, false if valid (matches original CheckLabel semantics)
     */
    bool checkLabel(const QString& name);

    // Member variables
    WorldDocument* m_doc;
    QString m_variableName; // Empty for new variable, populated for edit
    bool m_isEditMode;

    // UI Components
    QLineEdit* m_nameEdit;
    QLineEdit* m_contentsEdit;
    QPushButton* m_editContentsButton;
};

#endif // VARIABLE_EDIT_DIALOG_H
