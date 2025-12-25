#include "trigger_edit_dialog.h"
#include "../../automation/sendto.h"
#include "../../automation/trigger.h"
#include "../../world/world_document.h"
#include <QColorDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// Constructor for adding new trigger
TriggerEditDialog::TriggerEditDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_triggerName(), m_isEditMode(false)
{
    setWindowTitle("Add Trigger - " + doc->m_mush_name);
    resize(600, 500);
    setupUi();
}

// Constructor for editing existing trigger
TriggerEditDialog::TriggerEditDialog(WorldDocument* doc, const QString& triggerName,
                                     QWidget* parent)
    : QDialog(parent), m_doc(doc), m_triggerName(triggerName), m_isEditMode(true)
{
    setWindowTitle("Edit Trigger - " + doc->m_mush_name);
    resize(600, 500);
    setupUi();
    loadTriggerData();
}

void TriggerEditDialog::setupUi()
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
    m_labelEdit->setPlaceholderText("Optional: Name for scripting access");
    generalForm->addRow("&Label:", m_labelEdit);

    // Pattern
    m_patternEdit = new QLineEdit(generalTab);
    m_patternEdit->setPlaceholderText("Text to match (required)");
    generalForm->addRow("&Pattern:", m_patternEdit);

    // Enabled checkbox
    m_enabledCheck = new QCheckBox("Trigger is &enabled", generalTab);
    m_enabledCheck->setChecked(true);
    generalForm->addRow("", m_enabledCheck);

    // Regexp checkbox
    m_regexpCheck = new QCheckBox("Treat pattern as &regular expression", generalTab);
    generalForm->addRow("", m_regexpCheck);

    // Multi-line matching
    m_multiLineCheck = new QCheckBox("&Multi-line trigger", generalTab);
    m_multiLineCheck->setToolTip("Match pattern across multiple recent lines");
    generalForm->addRow("", m_multiLineCheck);

    // Lines to match
    m_linesToMatchSpin = new QSpinBox(generalTab);
    m_linesToMatchSpin->setRange(2, 200);
    m_linesToMatchSpin->setValue(2);
    m_linesToMatchSpin->setEnabled(false);
    m_linesToMatchSpin->setToolTip("Number of recent lines to match against (2-200)");
    generalForm->addRow("Lines to &match:", m_linesToMatchSpin);

    // Connect multi-line checkbox to enable/disable lines spinbox
    connect(m_multiLineCheck, &QCheckBox::toggled, m_linesToMatchSpin, &QSpinBox::setEnabled);

    // Sequence
    m_sequenceSpin = new QSpinBox(generalTab);
    m_sequenceSpin->setRange(1, 10000);
    m_sequenceSpin->setValue(100);
    m_sequenceSpin->setToolTip("Lower sequence executes first");
    generalForm->addRow("&Sequence:", m_sequenceSpin);

    // Group
    m_groupEdit = new QLineEdit(generalTab);
    m_groupEdit->setPlaceholderText("Optional: Group name");
    generalForm->addRow("&Group:", m_groupEdit);

    generalLayout->addLayout(generalForm);
    generalLayout->addStretch();

    m_tabWidget->addTab(generalTab, "&General");

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
    m_sendTextEdit->setPlaceholderText("Text to send when triggered\nUse %0-%99 for wildcards");
    m_sendTextEdit->setAcceptRichText(false);
    sendLabel->setBuddy(m_sendTextEdit);
    responseLayout->addWidget(m_sendTextEdit);

    m_tabWidget->addTab(responseTab, "&Response");

    // ========================================================================
    // OPTIONS TAB
    // ========================================================================
    QWidget* optionsTab = new QWidget();
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsTab);

    m_keepEvaluatingCheck = new QCheckBox("&Keep evaluating (process other triggers)", optionsTab);
    m_keepEvaluatingCheck->setToolTip("Allow other triggers to also match this line");
    optionsLayout->addWidget(m_keepEvaluatingCheck);

    m_expandVariablesCheck = new QCheckBox("E&xpand variables in send text", optionsTab);
    m_expandVariablesCheck->setToolTip("Replace @variable@ with variable value");
    optionsLayout->addWidget(m_expandVariablesCheck);

    m_omitFromOutputCheck = new QCheckBox("&Omit from output", optionsTab);
    m_omitFromOutputCheck->setToolTip("Don't show the matched line in output window");
    optionsLayout->addWidget(m_omitFromOutputCheck);

    m_omitFromLogCheck = new QCheckBox("Omit from &log file", optionsTab);
    m_omitFromLogCheck->setToolTip("Don't write the matched line to log file");
    optionsLayout->addWidget(m_omitFromLogCheck);

    m_oneShotCheck = new QCheckBox("O&ne-shot (disable after firing once)", optionsTab);
    m_oneShotCheck->setToolTip("Trigger will be disabled after it fires once");
    optionsLayout->addWidget(m_oneShotCheck);

    m_repeatCheck = new QCheckBox("&Repeat on same line", optionsTab);
    m_repeatCheck->setToolTip("Keep matching pattern on same line until no more matches");
    optionsLayout->addWidget(m_repeatCheck);

    m_soundIfInactiveCheck = new QCheckBox("&Sound only if window inactive", optionsTab);
    m_soundIfInactiveCheck->setToolTip("Only play trigger sound when window is not focused");
    optionsLayout->addWidget(m_soundIfInactiveCheck);

    m_lowercaseWildcardCheck = new QCheckBox("Con&vert wildcards to lowercase", optionsTab);
    m_lowercaseWildcardCheck->setToolTip("Convert captured wildcards to lowercase");
    optionsLayout->addWidget(m_lowercaseWildcardCheck);

    // Clipboard argument
    QHBoxLayout* clipboardLayout = new QHBoxLayout();
    QLabel* clipboardLabel = new QLabel("Copy wildcard to clipboard:", optionsTab);
    m_clipboardArgSpin = new QSpinBox(optionsTab);
    m_clipboardArgSpin->setRange(0, 99);
    m_clipboardArgSpin->setSpecialValueText("None");
    m_clipboardArgSpin->setToolTip("Copy this wildcard number to clipboard (0 = none)");
    clipboardLayout->addWidget(clipboardLabel);
    clipboardLayout->addWidget(m_clipboardArgSpin);
    clipboardLayout->addStretch();
    optionsLayout->addLayout(clipboardLayout);

    optionsLayout->addStretch();

    m_tabWidget->addTab(optionsTab, "&Options");

    // ========================================================================
    // APPEARANCE TAB
    // ========================================================================
    QWidget* appearanceTab = new QWidget();
    QVBoxLayout* appearanceLayout = new QVBoxLayout(appearanceTab);

    m_changeColorsCheck = new QCheckBox("&Change colors when triggered", appearanceTab);
    m_changeColorsCheck->setToolTip("Change the text/background color of matched line");
    appearanceLayout->addWidget(m_changeColorsCheck);

    // Color change type
    QFormLayout* colorForm = new QFormLayout();
    m_colorChangeTypeCombo = new QComboBox(appearanceTab);
    m_colorChangeTypeCombo->addItem("Both foreground and background", 0);
    m_colorChangeTypeCombo->addItem("Foreground only", 1);
    m_colorChangeTypeCombo->addItem("Background only", 2);
    m_colorChangeTypeCombo->setEnabled(false);
    colorForm->addRow("Color change &type:", m_colorChangeTypeCombo);

    // Color buttons
    QHBoxLayout* colorButtonLayout = new QHBoxLayout();

    m_foregroundColor = qRgb(255, 255, 255);
    m_foregroundColorButton = new QPushButton("Foreground", appearanceTab);
    m_foregroundColorButton->setEnabled(false);
    updateColorButton(m_foregroundColorButton, m_foregroundColor);
    connect(m_foregroundColorButton, &QPushButton::clicked, this,
            &TriggerEditDialog::onForegroundColorClicked);
    colorButtonLayout->addWidget(m_foregroundColorButton);

    m_backgroundColor = qRgb(0, 0, 0);
    m_backgroundColorButton = new QPushButton("Background", appearanceTab);
    m_backgroundColorButton->setEnabled(false);
    updateColorButton(m_backgroundColorButton, m_backgroundColor);
    connect(m_backgroundColorButton, &QPushButton::clicked, this,
            &TriggerEditDialog::onBackgroundColorClicked);
    colorButtonLayout->addWidget(m_backgroundColorButton);

    colorButtonLayout->addStretch();
    colorForm->addRow("Colors:", colorButtonLayout);

    appearanceLayout->addLayout(colorForm);

    // Connect change colors checkbox to enable/disable color controls
    connect(m_changeColorsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_colorChangeTypeCombo->setEnabled(checked);
        m_foregroundColorButton->setEnabled(checked);
        m_backgroundColorButton->setEnabled(checked);
    });

    appearanceLayout->addStretch();

    m_tabWidget->addTab(appearanceTab, "&Appearance");

    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);

    // ========================================================================
    // BUTTON BOX
    // ========================================================================
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TriggerEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TriggerEditDialog::onCancel);
    mainLayout->addWidget(buttonBox);

    // Set focus to pattern field
    m_patternEdit->setFocus();
}

void TriggerEditDialog::loadTriggerData()
{
    if (!m_isEditMode || m_triggerName.isEmpty()) {
        return;
    }

    Trigger* trigger = m_doc->getTrigger(m_triggerName);
    if (!trigger) {
        QMessageBox::warning(this, "Error", "Trigger not found: " + m_triggerName);
        reject();
        return;
    }

    // General tab
    m_labelEdit->setText(trigger->strLabel);
    m_patternEdit->setText(trigger->trigger);
    m_enabledCheck->setChecked(trigger->bEnabled);
    m_regexpCheck->setChecked(trigger->bRegexp);
    m_multiLineCheck->setChecked(trigger->bMultiLine);
    m_linesToMatchSpin->setValue(trigger->iLinesToMatch > 0 ? trigger->iLinesToMatch : 2);
    m_linesToMatchSpin->setEnabled(trigger->bMultiLine);
    m_sequenceSpin->setValue(trigger->iSequence);
    m_groupEdit->setText(trigger->strGroup);

    // Response tab
    m_sendTextEdit->setPlainText(trigger->contents);
    m_scriptEdit->setText(trigger->strProcedure);
    int index = m_sendToCombo->findData(trigger->iSendTo);
    if (index >= 0) {
        m_sendToCombo->setCurrentIndex(index);
    }

    // Options tab
    m_keepEvaluatingCheck->setChecked(trigger->bKeepEvaluating);
    m_expandVariablesCheck->setChecked(trigger->bExpandVariables);
    m_omitFromOutputCheck->setChecked(trigger->bOmitFromOutput);
    m_omitFromLogCheck->setChecked(trigger->omit_from_log);
    m_oneShotCheck->setChecked(trigger->bOneShot);
    m_repeatCheck->setChecked(trigger->bRepeat);
    m_soundIfInactiveCheck->setChecked(trigger->bSoundIfInactive);
    m_lowercaseWildcardCheck->setChecked(trigger->bLowercaseWildcard);
    m_clipboardArgSpin->setValue(trigger->iClipboardArg);

    // Appearance tab
    bool hasColorChange = (trigger->iOtherForeground != 0 || trigger->iOtherBackground != 0);
    m_changeColorsCheck->setChecked(hasColorChange);
    m_colorChangeTypeCombo->setEnabled(hasColorChange);
    m_foregroundColorButton->setEnabled(hasColorChange);
    m_backgroundColorButton->setEnabled(hasColorChange);

    int colorIndex = m_colorChangeTypeCombo->findData(trigger->iColourChangeType);
    if (colorIndex >= 0) {
        m_colorChangeTypeCombo->setCurrentIndex(colorIndex);
    }

    m_foregroundColor = trigger->iOtherForeground;
    m_backgroundColor = trigger->iOtherBackground;
    updateColorButton(m_foregroundColorButton, m_foregroundColor);
    updateColorButton(m_backgroundColorButton, m_backgroundColor);
}

bool TriggerEditDialog::validateForm()
{
    // Pattern is required
    if (m_patternEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
                             "Pattern is required.\n\nPlease enter the text to match.");
        m_patternEdit->setFocus();
        return false;
    }

    // If regexp is checked, validate the pattern
    if (m_regexpCheck->isChecked()) {
        QRegularExpression regex(m_patternEdit->text());
        if (!regex.isValid()) {
            QMessageBox::warning(
                this, "Validation Error",
                QString("Invalid regular expression:\n\n%1").arg(regex.errorString()));
            m_patternEdit->setFocus();
            return false;
        }
    }

    return true;
}

bool TriggerEditDialog::saveTrigger()
{
    Trigger* trigger = nullptr;

    if (m_isEditMode) {
        // Edit existing trigger
        trigger = m_doc->getTrigger(m_triggerName);
        if (!trigger) {
            QMessageBox::warning(this, "Error", "Trigger not found: " + m_triggerName);
            return false;
        }
    } else {
        // Create new trigger
        trigger = new Trigger();

        // Generate internal name
        QString label = m_labelEdit->text().trimmed();
        if (label.isEmpty()) {
            // Generate unique name based on pattern
            QString pattern = m_patternEdit->text();
            if (pattern.length() > 50) {
                pattern = pattern.left(50) + "...";
            }
            trigger->strInternalName = QString("trigger_%1_%2")
                                           .arg(QDateTime::currentMSecsSinceEpoch())
                                           .arg(qHash(pattern));
        } else {
            trigger->strInternalName = label;
        }
    }

    // General tab
    trigger->strLabel = m_labelEdit->text().trimmed();
    trigger->trigger = m_patternEdit->text();
    trigger->bEnabled = m_enabledCheck->isChecked();
    trigger->bRegexp = m_regexpCheck->isChecked();
    trigger->bMultiLine = m_multiLineCheck->isChecked();
    trigger->iLinesToMatch = m_multiLineCheck->isChecked() ? m_linesToMatchSpin->value() : 0;
    trigger->iSequence = m_sequenceSpin->value();
    trigger->strGroup = m_groupEdit->text().trimmed();

    // Response tab
    trigger->contents = m_sendTextEdit->toPlainText();
    trigger->strProcedure = m_scriptEdit->text().trimmed();
    trigger->iSendTo = m_sendToCombo->currentData().toInt();

    // Options tab
    trigger->bKeepEvaluating = m_keepEvaluatingCheck->isChecked();
    trigger->bExpandVariables = m_expandVariablesCheck->isChecked();
    trigger->bOmitFromOutput = m_omitFromOutputCheck->isChecked();
    trigger->omit_from_log = m_omitFromLogCheck->isChecked();
    trigger->bOneShot = m_oneShotCheck->isChecked();
    trigger->bRepeat = m_repeatCheck->isChecked();
    trigger->bSoundIfInactive = m_soundIfInactiveCheck->isChecked();
    trigger->bLowercaseWildcard = m_lowercaseWildcardCheck->isChecked();
    trigger->iClipboardArg = m_clipboardArgSpin->value();

    // Appearance tab
    if (m_changeColorsCheck->isChecked()) {
        trigger->iColourChangeType = m_colorChangeTypeCombo->currentData().toInt();
        trigger->iOtherForeground = m_foregroundColor;
        trigger->iOtherBackground = m_backgroundColor;
    } else {
        trigger->iColourChangeType = 0;
        trigger->iOtherForeground = 0;
        trigger->iOtherBackground = 0;
    }

    // Compile regexp if needed
    if (trigger->bRegexp) {
        trigger->compileRegexp();
    }

    // Add to document if new
    if (!m_isEditMode) {
        auto triggerPtr = std::unique_ptr<Trigger>(trigger);
        if (!m_doc->addTrigger(triggerPtr->strInternalName, std::move(triggerPtr))) {
            QMessageBox::warning(
                this, "Error",
                "Failed to add trigger. A trigger with this name may already exist.");
            // triggerPtr will automatically delete on scope exit
            return false;
        }
    }

    return true;
}

void TriggerEditDialog::onOk()
{
    if (!validateForm()) {
        return;
    }

    if (saveTrigger()) {
        accept();
    }
}

void TriggerEditDialog::onCancel()
{
    reject();
}

void TriggerEditDialog::updateColorButton(QPushButton* button, QRgb color)
{
    QColor qcolor(color);
    QString style = QString("background-color: %1; color: %2;")
                        .arg(qcolor.name())
                        .arg(qcolor.lightness() > 128 ? "black" : "white");
    button->setStyleSheet(style);
}

void TriggerEditDialog::onForegroundColorClicked()
{
    QColor initial(m_foregroundColor);
    QColor color = QColorDialog::getColor(initial, this, "Select Foreground Color");
    if (color.isValid()) {
        m_foregroundColor = color.rgb();
        updateColorButton(m_foregroundColorButton, m_foregroundColor);
    }
}

void TriggerEditDialog::onBackgroundColorClicked()
{
    QColor initial(m_backgroundColor);
    QColor color = QColorDialog::getColor(initial, this, "Select Background Color");
    if (color.isValid()) {
        m_backgroundColor = color.rgb();
        updateColorButton(m_backgroundColorButton, m_backgroundColor);
    }
}
