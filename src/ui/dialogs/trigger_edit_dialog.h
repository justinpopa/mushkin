#ifndef TRIGGER_EDIT_DIALOG_H
#define TRIGGER_EDIT_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRgb>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>

// Forward declarations
class WorldDocument;
class Trigger;

/**
 * TriggerEditDialog - Dialog for adding/editing a single trigger
 *
 * Provides a tabbed interface with:
 * - General tab: Label, pattern, enabled, regexp, sequence, group
 * - Response tab: Send text, send-to destination, script name
 * - Options tab: Keep evaluating, expand variables, omit options, one-shot
 * - Appearance tab: Color change options
 *
 * Can operate in two modes:
 * - Add mode: Creates a new trigger
 * - Edit mode: Modifies an existing trigger
 *
 * Based on MUSHclient's trigger configuration dialog.
 */
class TriggerEditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for adding a new trigger
     * @param doc WorldDocument to add trigger to
     * @param parent Parent widget
     */
    explicit TriggerEditDialog(WorldDocument* doc, QWidget* parent = nullptr);

    /**
     * Constructor for editing an existing trigger
     * @param doc WorldDocument containing the trigger
     * @param triggerName Internal name of trigger to edit
     * @param parent Parent widget
     */
    TriggerEditDialog(WorldDocument* doc, const QString& triggerName, QWidget* parent = nullptr);

    ~TriggerEditDialog() override = default;

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
     * Foreground color button clicked
     */
    void onForegroundColorClicked();

    /**
     * Background color button clicked
     */
    void onBackgroundColorClicked();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load trigger data into form fields
     */
    void loadTriggerData();

    /**
     * Validate form data
     * @return true if valid, false if validation fails
     */
    bool validateForm();

    /**
     * Save form data to trigger
     * @return true if saved successfully
     */
    bool saveTrigger();

    /**
     * Update color button appearance
     */
    void updateColorButton(QPushButton* button, QRgb color);

    // Member variables
    WorldDocument* m_doc;
    QString m_triggerName; // Empty for new trigger, populated for edit
    bool m_isEditMode;

    // UI Components
    QTabWidget* m_tabWidget;

    // General tab widgets
    QLineEdit* m_labelEdit;
    QLineEdit* m_patternEdit;
    QCheckBox* m_enabledCheck;
    QCheckBox* m_regexpCheck;
    QCheckBox* m_multiLineCheck;
    QSpinBox* m_linesToMatchSpin;
    QSpinBox* m_sequenceSpin;
    QLineEdit* m_groupEdit;

    // Response tab widgets
    QTextEdit* m_sendTextEdit;
    QComboBox* m_sendToCombo;
    QLineEdit* m_scriptEdit;
    QComboBox* m_scriptLanguageCombo; // Script language (Lua, YueScript)
    QLineEdit* m_variableEdit;        // For "send to variable"

    // Options tab widgets
    QCheckBox* m_keepEvaluatingCheck;
    QCheckBox* m_expandVariablesCheck;
    QCheckBox* m_omitFromOutputCheck;
    QCheckBox* m_omitFromLogCheck;
    QCheckBox* m_oneShotCheck;
    QCheckBox* m_repeatCheck;
    QCheckBox* m_soundIfInactiveCheck;
    QCheckBox* m_lowercaseWildcardCheck;
    QSpinBox* m_clipboardArgSpin;

    // Appearance tab widgets
    QCheckBox* m_changeColorsCheck;
    QComboBox* m_colorChangeTypeCombo;
    QPushButton* m_foregroundColorButton;
    QPushButton* m_backgroundColorButton;
    QRgb m_foregroundColor;
    QRgb m_backgroundColor;
};

#endif // TRIGGER_EDIT_DIALOG_H
