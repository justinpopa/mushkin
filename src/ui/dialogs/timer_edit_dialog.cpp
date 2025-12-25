#include "timer_edit_dialog.h"
#include "../../automation/sendto.h"
#include "../../automation/timer.h"
#include "../../world/world_document.h"
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// Constructor for adding new timer
TimerEditDialog::TimerEditDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_timerName(), m_isEditMode(false)
{
    setWindowTitle("Add Timer - " + doc->m_mush_name);
    resize(600, 600);
    setupUi();
}

// Constructor for editing existing timer
TimerEditDialog::TimerEditDialog(WorldDocument* doc, const QString& timerName, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_timerName(timerName), m_isEditMode(true)
{
    setWindowTitle("Edit Timer - " + doc->m_mush_name);
    resize(600, 600);
    setupUi();
    loadTimerData();
}

void TimerEditDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);

    // ========================================================================
    // GENERAL TAB
    // ========================================================================
    QWidget* generalTab = new QWidget();
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);

    QFormLayout* generalForm = new QFormLayout();

    // Label
    m_labelEdit = new QLineEdit(generalTab);
    m_labelEdit->setPlaceholderText("Timer name/label (required)");
    generalForm->addRow("&Label:", m_labelEdit);

    // Enabled checkbox
    m_enabledCheck = new QCheckBox("Timer is &enabled", generalTab);
    m_enabledCheck->setChecked(true);
    generalForm->addRow("", m_enabledCheck);

    // Group
    m_groupEdit = new QLineEdit(generalTab);
    m_groupEdit->setPlaceholderText("Optional: Group name");
    generalForm->addRow("&Group:", m_groupEdit);

    generalLayout->addLayout(generalForm);

    // Timer type selection
    QGroupBox* typeGroup = new QGroupBox("Timer Type", generalTab);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGroup);

    m_intervalRadio =
        new QRadioButton("&Interval timer (fires every N hours/minutes/seconds)", typeGroup);
    m_intervalRadio->setChecked(true);
    typeLayout->addWidget(m_intervalRadio);

    m_atTimeRadio = new QRadioButton("&At-time timer (fires at specific time each day)", typeGroup);
    typeLayout->addWidget(m_atTimeRadio);

    connect(m_intervalRadio, &QRadioButton::toggled, this, &TimerEditDialog::onTimerTypeChanged);

    generalLayout->addWidget(typeGroup);

    // Interval timing group
    m_intervalGroup = new QGroupBox("Interval Timing", generalTab);
    QFormLayout* intervalForm = new QFormLayout(m_intervalGroup);

    m_everyHourSpin = new QSpinBox(m_intervalGroup);
    m_everyHourSpin->setRange(0, 23);
    m_everyHourSpin->setSuffix(" hours");
    intervalForm->addRow("Every &hours:", m_everyHourSpin);

    m_everyMinuteSpin = new QSpinBox(m_intervalGroup);
    m_everyMinuteSpin->setRange(0, 59);
    m_everyMinuteSpin->setSuffix(" minutes");
    intervalForm->addRow("Every &minutes:", m_everyMinuteSpin);

    m_everySecondSpin = new QDoubleSpinBox(m_intervalGroup);
    m_everySecondSpin->setRange(0.0, 59.9);
    m_everySecondSpin->setDecimals(1);
    m_everySecondSpin->setSuffix(" seconds");
    m_everySecondSpin->setValue(60.0);
    intervalForm->addRow("Every &seconds:", m_everySecondSpin);

    generalLayout->addWidget(m_intervalGroup);

    // Offset group (for intervals)
    m_offsetGroup = new QGroupBox("Offset (optional - shifts timing boundaries)", generalTab);
    QFormLayout* offsetForm = new QFormLayout(m_offsetGroup);

    m_offsetHourSpin = new QSpinBox(m_offsetGroup);
    m_offsetHourSpin->setRange(0, 23);
    m_offsetHourSpin->setSuffix(" hours");
    offsetForm->addRow("Offset hours:", m_offsetHourSpin);

    m_offsetMinuteSpin = new QSpinBox(m_offsetGroup);
    m_offsetMinuteSpin->setRange(0, 59);
    m_offsetMinuteSpin->setSuffix(" minutes");
    offsetForm->addRow("Offset minutes:", m_offsetMinuteSpin);

    m_offsetSecondSpin = new QDoubleSpinBox(m_offsetGroup);
    m_offsetSecondSpin->setRange(0.0, 59.9);
    m_offsetSecondSpin->setDecimals(1);
    m_offsetSecondSpin->setSuffix(" seconds");
    offsetForm->addRow("Offset seconds:", m_offsetSecondSpin);

    generalLayout->addWidget(m_offsetGroup);

    // At-time timing group
    m_atTimeGroup = new QGroupBox("At-Time Timing", generalTab);
    QFormLayout* atTimeForm = new QFormLayout(m_atTimeGroup);

    m_atHourSpin = new QSpinBox(m_atTimeGroup);
    m_atHourSpin->setRange(0, 23);
    atTimeForm->addRow("At &hour:", m_atHourSpin);

    m_atMinuteSpin = new QSpinBox(m_atTimeGroup);
    m_atMinuteSpin->setRange(0, 59);
    atTimeForm->addRow("At m&inute:", m_atMinuteSpin);

    m_atSecondSpin = new QDoubleSpinBox(m_atTimeGroup);
    m_atSecondSpin->setRange(0.0, 59.9);
    m_atSecondSpin->setDecimals(1);
    atTimeForm->addRow("At &second:", m_atSecondSpin);

    generalLayout->addWidget(m_atTimeGroup);

    generalLayout->addStretch();

    m_tabWidget->addTab(generalTab, "&General");

    // Initial state
    onTimerTypeChanged();

    // ========================================================================
    // RESPONSE TAB
    // ========================================================================
    QWidget* responseTab = new QWidget();
    QVBoxLayout* responseLayout = new QVBoxLayout(responseTab);

    QFormLayout* responseForm = new QFormLayout();

    // Send To dropdown
    m_sendToCombo = new QComboBox(responseTab);
    m_sendToCombo->addItem("World", eSendToWorld);
    m_sendToCombo->addItem("Command", eSendToCommand);
    m_sendToCombo->addItem("Output", eSendToOutput);
    m_sendToCombo->addItem("Status", eSendToStatus);
    m_sendToCombo->addItem("Notepad (new)", eSendToNotepad);
    m_sendToCombo->addItem("Notepad (append)", eAppendToNotepad);
    m_sendToCombo->addItem("Log file", eSendToLogFile);
    m_sendToCombo->addItem("Notepad (replace)", eReplaceNotepad);
    m_sendToCombo->addItem("Command queue", eSendToCommandQueue);
    m_sendToCombo->addItem("Variable", eSendToVariable);
    m_sendToCombo->addItem("Execute", eSendToExecute);
    m_sendToCombo->addItem("Speedwalk", eSendToSpeedwalk);
    m_sendToCombo->addItem("Script", eSendToScript);
    m_sendToCombo->addItem("Immediate", eSendImmediate);
    m_sendToCombo->addItem("Script (after omit)", eSendToScriptAfterOmit);
    responseForm->addRow("Send &To:", m_sendToCombo);

    // Script name
    m_scriptEdit = new QLineEdit(responseTab);
    m_scriptEdit->setPlaceholderText("Function name to call");
    responseForm->addRow("Script &function:", m_scriptEdit);

    responseLayout->addLayout(responseForm);

    // Send text
    QLabel* sendLabel = new QLabel("Send &text:", responseTab);
    responseLayout->addWidget(sendLabel);

    m_sendTextEdit = new QTextEdit(responseTab);
    m_sendTextEdit->setPlaceholderText("Text to send when timer fires");
    m_sendTextEdit->setAcceptRichText(false);
    sendLabel->setBuddy(m_sendTextEdit);
    responseLayout->addWidget(m_sendTextEdit);

    m_tabWidget->addTab(responseTab, "&Response");

    // ========================================================================
    // OPTIONS TAB
    // ========================================================================
    QWidget* optionsTab = new QWidget();
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsTab);

    m_oneShotCheck = new QCheckBox("&One shot (delete after first fire)", optionsTab);
    m_oneShotCheck->setToolTip("Timer will be deleted after firing once");
    optionsLayout->addWidget(m_oneShotCheck);

    m_activeWhenClosedCheck = new QCheckBox("&Active when world is closed", optionsTab);
    m_activeWhenClosedCheck->setToolTip("Fire even when disconnected from MUD");
    optionsLayout->addWidget(m_activeWhenClosedCheck);

    m_omitFromOutputCheck = new QCheckBox("&Omit from output", optionsTab);
    m_omitFromOutputCheck->setToolTip("Don't show the timer action in output");
    optionsLayout->addWidget(m_omitFromOutputCheck);

    m_omitFromLogCheck = new QCheckBox("Omit from &log file", optionsTab);
    m_omitFromLogCheck->setToolTip("Don't write the timer action to log");
    optionsLayout->addWidget(m_omitFromLogCheck);

    optionsLayout->addStretch();

    m_tabWidget->addTab(optionsTab, "&Options");

    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);

    // ========================================================================
    // BUTTON BOX
    // ========================================================================
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TimerEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TimerEditDialog::onCancel);
    mainLayout->addWidget(buttonBox);

    // Set focus to label field
    m_labelEdit->setFocus();
}

void TimerEditDialog::onTimerTypeChanged()
{
    bool isInterval = m_intervalRadio->isChecked();
    m_intervalGroup->setVisible(isInterval);
    m_offsetGroup->setVisible(isInterval);
    m_atTimeGroup->setVisible(!isInterval);
}

void TimerEditDialog::loadTimerData()
{
    if (!m_isEditMode || m_timerName.isEmpty()) {
        return;
    }

    Timer* timer = m_doc->getTimer(m_timerName);
    if (!timer) {
        QMessageBox::warning(this, "Error", "Timer not found: " + m_timerName);
        reject();
        return;
    }

    // Load data into form
    m_labelEdit->setText(timer->strLabel);
    m_enabledCheck->setChecked(timer->bEnabled);
    m_groupEdit->setText(timer->strGroup);

    // Timer type
    if (timer->iType == Timer::eAtTime) {
        m_atTimeRadio->setChecked(true);
        m_atHourSpin->setValue(timer->iAtHour);
        m_atMinuteSpin->setValue(timer->iAtMinute);
        m_atSecondSpin->setValue(timer->fAtSecond);
    } else {
        m_intervalRadio->setChecked(true);
        m_everyHourSpin->setValue(timer->iEveryHour);
        m_everyMinuteSpin->setValue(timer->iEveryMinute);
        m_everySecondSpin->setValue(timer->fEverySecond);
        m_offsetHourSpin->setValue(timer->iOffsetHour);
        m_offsetMinuteSpin->setValue(timer->iOffsetMinute);
        m_offsetSecondSpin->setValue(timer->fOffsetSecond);
    }

    // Response
    m_sendTextEdit->setPlainText(timer->strContents);
    m_scriptEdit->setText(timer->strProcedure);

    // Set send-to combo
    int index = m_sendToCombo->findData(timer->iSendTo);
    if (index >= 0) {
        m_sendToCombo->setCurrentIndex(index);
    }

    // Options
    m_oneShotCheck->setChecked(timer->bOneShot);
    m_activeWhenClosedCheck->setChecked(timer->bActiveWhenClosed);
    m_omitFromOutputCheck->setChecked(timer->bOmitFromOutput);
    m_omitFromLogCheck->setChecked(timer->bOmitFromLog);
}

bool TimerEditDialog::validateForm()
{
    // Label is required
    if (m_labelEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
                             "Label is required.\n\nPlease enter a name for this timer.");
        m_labelEdit->setFocus();
        return false;
    }

    // Validate interval timing
    if (m_intervalRadio->isChecked()) {
        if (m_everyHourSpin->value() == 0 && m_everyMinuteSpin->value() == 0 &&
            m_everySecondSpin->value() == 0.0) {
            QMessageBox::warning(this, "Validation Error",
                                 "Interval timer requires at least one non-zero time value.\n\n"
                                 "Please set hours, minutes, or seconds.");
            m_everyHourSpin->setFocus();
            return false;
        }
    }

    return true;
}

bool TimerEditDialog::saveTimer()
{
    Timer* timer = nullptr;

    if (m_isEditMode) {
        // Edit existing timer
        timer = m_doc->getTimer(m_timerName);
        if (!timer) {
            QMessageBox::warning(this, "Error", "Timer not found: " + m_timerName);
            return false;
        }
    } else {
        // Create new timer
        timer = new Timer();
    }

    // Save form data to timer
    timer->strLabel = m_labelEdit->text().trimmed();
    timer->bEnabled = m_enabledCheck->isChecked();
    timer->strGroup = m_groupEdit->text().trimmed();

    // Timer type and timing
    if (m_atTimeRadio->isChecked()) {
        timer->iType = Timer::eAtTime;
        timer->iAtHour = m_atHourSpin->value();
        timer->iAtMinute = m_atMinuteSpin->value();
        timer->fAtSecond = m_atSecondSpin->value();
    } else {
        timer->iType = Timer::eInterval;
        timer->iEveryHour = m_everyHourSpin->value();
        timer->iEveryMinute = m_everyMinuteSpin->value();
        timer->fEverySecond = m_everySecondSpin->value();
        timer->iOffsetHour = m_offsetHourSpin->value();
        timer->iOffsetMinute = m_offsetMinuteSpin->value();
        timer->fOffsetSecond = m_offsetSecondSpin->value();
    }

    // Response
    timer->strContents = m_sendTextEdit->toPlainText();
    timer->strProcedure = m_scriptEdit->text().trimmed();
    timer->iSendTo = m_sendToCombo->currentData().toInt();

    // Options
    timer->bOneShot = m_oneShotCheck->isChecked();
    timer->bActiveWhenClosed = m_activeWhenClosedCheck->isChecked();
    timer->bOmitFromOutput = m_omitFromOutputCheck->isChecked();
    timer->bOmitFromLog = m_omitFromLogCheck->isChecked();

    // Add to document if new
    if (!m_isEditMode) {
        // Transfer ownership to unique_ptr
        auto timerPtr = std::unique_ptr<Timer>(timer);
        Timer* rawTimer = timerPtr.get(); // Keep raw pointer for later use
        if (!m_doc->addTimer(timerPtr->strLabel, std::move(timerPtr))) {
            QMessageBox::warning(this, "Error",
                                 "Failed to add timer. A timer with this name may already exist.");
            // timerPtr will automatically delete on scope exit
            return false;
        }
        // Reset the timer to calculate its fire time
        m_doc->resetOneTimer(rawTimer);
    } else {
        // For edited timer, recalculate fire time
        m_doc->resetOneTimer(timer);
    }

    return true;
}

void TimerEditDialog::onOk()
{
    if (!validateForm()) {
        return;
    }

    if (saveTimer()) {
        accept();
    }
}

void TimerEditDialog::onCancel()
{
    reject();
}
