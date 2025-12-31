#ifndef ALIAS_EDIT_DIALOG_H
#define ALIAS_EDIT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QTabWidget>

// Forward declarations
class WorldDocument;
class Alias;

/**
 * AliasEditDialog - Dialog for adding/editing a single alias
 *
 * Provides a tabbed interface with:
 * - General tab: Label, match pattern, enabled, regexp, sequence, group
 * - Response tab: Send text, send-to destination, script name
 * - Options tab: Echo alias, keep evaluating, etc.
 *
 * Can operate in two modes:
 * - Add mode: Creates a new alias
 * - Edit mode: Modifies an existing alias
 *
 * Based on MUSHclient's alias configuration dialog.
 */
class AliasEditDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor for adding a new alias
     * @param doc WorldDocument to add alias to
     * @param parent Parent widget
     */
    explicit AliasEditDialog(WorldDocument* doc, QWidget* parent = nullptr);

    /**
     * Constructor for editing an existing alias
     * @param doc WorldDocument containing the alias
     * @param aliasName Internal name of alias to edit
     * @param parent Parent widget
     */
    AliasEditDialog(WorldDocument* doc, const QString& aliasName, QWidget* parent = nullptr);

    ~AliasEditDialog() override = default;

private slots:
    /**
     * OK button clicked - validate and save
     */
    void onOk();

    /**
     * Cancel button clicked
     */
    void onCancel();

private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load alias data into form fields
     */
    void loadAliasData();

    /**
     * Validate form data
     * @return true if valid, false if validation fails
     */
    bool validateForm();

    /**
     * Save form data to alias
     * @return true if saved successfully
     */
    bool saveAlias();

    // Member variables
    WorldDocument* m_doc;
    QString m_aliasName;  // Empty for new alias, populated for edit
    bool m_isEditMode;

    // UI Components
    QTabWidget* m_tabWidget;

    // General tab widgets
    QLineEdit* m_labelEdit;
    QLineEdit* m_matchEdit;
    QCheckBox* m_enabledCheck;
    QCheckBox* m_regexpCheck;
    QSpinBox* m_sequenceSpin;
    QLineEdit* m_groupEdit;

    // Response tab widgets
    QTextEdit* m_sendTextEdit;
    QComboBox* m_sendToCombo;
    QLineEdit* m_scriptEdit;
    QComboBox* m_scriptLanguageCombo; // Script language (Lua, YueScript)

    // Options tab widgets
    QCheckBox* m_echoAliasCheck;
    QCheckBox* m_keepEvaluatingCheck;
    QCheckBox* m_expandVariablesCheck;
    QCheckBox* m_omitFromOutputCheck;
    QCheckBox* m_omitFromLogCheck;
    QCheckBox* m_omitFromHistoryCheck;
};

#endif // ALIAS_EDIT_DIALOG_H
