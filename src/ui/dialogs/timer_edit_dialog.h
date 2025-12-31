#ifndef TIMER_EDIT_DIALOG_H
#define TIMER_EDIT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTabWidget>
#include <QRadioButton>
#include <QGroupBox>

// Forward declarations
class WorldDocument;
class Timer;

/**
 * TimerEditDialog - Dialog for adding/editing a single timer
 *
 * Provides a tabbed interface with:
 * - General tab: Label, type (interval/at-time), timing fields, group
 * - Response tab: Send text, send-to destination, script name
 * - Options tab: One shot, active when closed, omit flags
 *
 * Can operate in two modes:
 * - Add mode: Creates a new timer
 * - Edit mode: Modifies an existing timer
 *
 * Based on MUSHclient's timer configuration dialog.
 */
class TimerEditDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor for adding a new timer
     * @param doc WorldDocument to add timer to
     * @param parent Parent widget
     */
    explicit TimerEditDialog(WorldDocument* doc, QWidget* parent = nullptr);

    /**
     * Constructor for editing an existing timer
     * @param doc WorldDocument containing the timer
     * @param timerName Label of timer to edit
     * @param parent Parent widget
     */
    TimerEditDialog(WorldDocument* doc, const QString& timerName, QWidget* parent = nullptr);

    ~TimerEditDialog() override = default;

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
     * Timer type changed - update UI
     */
    void onTimerTypeChanged();

private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load timer data into form fields
     */
    void loadTimerData();

    /**
     * Validate form data
     * @return true if valid, false if validation fails
     */
    bool validateForm();

    /**
     * Save form data to timer
     * @return true if saved successfully
     */
    bool saveTimer();

    // Member variables
    WorldDocument* m_doc;
    QString m_timerName;  // Empty for new timer, populated for edit
    bool m_isEditMode;

    // UI Components
    QTabWidget* m_tabWidget;

    // General tab widgets
    QLineEdit* m_labelEdit;
    QCheckBox* m_enabledCheck;
    QRadioButton* m_intervalRadio;
    QRadioButton* m_atTimeRadio;
    QLineEdit* m_groupEdit;

    // Interval timing widgets
    QGroupBox* m_intervalGroup;
    QSpinBox* m_everyHourSpin;
    QSpinBox* m_everyMinuteSpin;
    QDoubleSpinBox* m_everySecondSpin;

    // At-time timing widgets
    QGroupBox* m_atTimeGroup;
    QSpinBox* m_atHourSpin;
    QSpinBox* m_atMinuteSpin;
    QDoubleSpinBox* m_atSecondSpin;

    // Offset timing widgets (for intervals)
    QGroupBox* m_offsetGroup;
    QSpinBox* m_offsetHourSpin;
    QSpinBox* m_offsetMinuteSpin;
    QDoubleSpinBox* m_offsetSecondSpin;

    // Response tab widgets
    QTextEdit* m_sendTextEdit;
    QComboBox* m_sendToCombo;
    QLineEdit* m_scriptEdit;
    QComboBox* m_scriptLanguageCombo; // Script language (Lua, YueScript)

    // Options tab widgets
    QCheckBox* m_oneShotCheck;
    QCheckBox* m_activeWhenClosedCheck;
    QCheckBox* m_omitFromOutputCheck;
    QCheckBox* m_omitFromLogCheck;
};

#endif // TIMER_EDIT_DIALOG_H
