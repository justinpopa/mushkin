#include "alias_edit_dialog.h"
#include "../../automation/alias.h"
#include "../../automation/sendto.h"
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

// Constructor for adding new alias
AliasEditDialog::AliasEditDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_aliasName(), m_isEditMode(false)
{
    setWindowTitle("Add Alias - " + doc->m_mush_name);
    resize(600, 550);
    setupUi();
}

// Constructor for editing existing alias
AliasEditDialog::AliasEditDialog(WorldDocument* doc, const QString& aliasName, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_aliasName(aliasName), m_isEditMode(true)
{
    setWindowTitle("Edit Alias - " + doc->m_mush_name);
    resize(600, 550);
    setupUi();
    loadAliasData();
}

void AliasEditDialog::setupUi()
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

    // Match pattern
    m_matchEdit = new QLineEdit(generalTab);
    m_matchEdit->setPlaceholderText("Pattern to match (required)");
    generalForm->addRow("&Match:", m_matchEdit);

    // Enabled checkbox
    m_enabledCheck = new QCheckBox("Alias is &enabled", generalTab);
    m_enabledCheck->setChecked(true);
    generalForm->addRow("", m_enabledCheck);

    // Regexp checkbox
    m_regexpCheck = new QCheckBox("Treat match as &regular expression", generalTab);
    generalForm->addRow("", m_regexpCheck);

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
    m_sendTextEdit->setPlaceholderText("Text to send when alias matches\nUse %0-%99 for wildcards");
    m_sendTextEdit->setAcceptRichText(false);
    sendLabel->setBuddy(m_sendTextEdit);
    responseLayout->addWidget(m_sendTextEdit);

    m_tabWidget->addTab(responseTab, "&Response");

    // ========================================================================
    // OPTIONS TAB
    // ========================================================================
    QWidget* optionsTab = new QWidget();
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsTab);

    m_echoAliasCheck = new QCheckBox("&Echo alias to output window", optionsTab);
    m_echoAliasCheck->setToolTip("Show the alias match in the output");
    optionsLayout->addWidget(m_echoAliasCheck);

    m_keepEvaluatingCheck = new QCheckBox("&Keep evaluating (process other aliases)", optionsTab);
    m_keepEvaluatingCheck->setToolTip("Allow other aliases to also match");
    optionsLayout->addWidget(m_keepEvaluatingCheck);

    m_expandVariablesCheck = new QCheckBox("E&xpand variables in send text", optionsTab);
    m_expandVariablesCheck->setChecked(true);
    m_expandVariablesCheck->setToolTip("Replace @variable@ with variable value");
    optionsLayout->addWidget(m_expandVariablesCheck);

    m_omitFromOutputCheck = new QCheckBox("&Omit from output", optionsTab);
    m_omitFromOutputCheck->setToolTip("Don't show the alias match in output");
    optionsLayout->addWidget(m_omitFromOutputCheck);

    m_omitFromLogCheck = new QCheckBox("Omit from &log file", optionsTab);
    m_omitFromLogCheck->setToolTip("Don't write the alias match to log");
    optionsLayout->addWidget(m_omitFromLogCheck);

    m_omitFromHistoryCheck = new QCheckBox("Omit from command &history", optionsTab);
    m_omitFromHistoryCheck->setToolTip("Don't add to command recall history");
    optionsLayout->addWidget(m_omitFromHistoryCheck);

    optionsLayout->addStretch();

    m_tabWidget->addTab(optionsTab, "&Options");

    // Add tab widget to main layout
    mainLayout->addWidget(m_tabWidget);

    // ========================================================================
    // BUTTON BOX
    // ========================================================================
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AliasEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AliasEditDialog::onCancel);
    mainLayout->addWidget(buttonBox);

    // Set focus to match field
    m_matchEdit->setFocus();
}

void AliasEditDialog::loadAliasData()
{
    if (!m_isEditMode || m_aliasName.isEmpty()) {
        return;
    }

    Alias* alias = m_doc->getAlias(m_aliasName);
    if (!alias) {
        QMessageBox::warning(this, "Error", "Alias not found: " + m_aliasName);
        reject();
        return;
    }

    // Load data into form
    m_labelEdit->setText(alias->strLabel);
    m_matchEdit->setText(alias->name);
    m_enabledCheck->setChecked(alias->bEnabled);
    m_regexpCheck->setChecked(alias->bRegexp);
    m_sequenceSpin->setValue(alias->iSequence);
    m_groupEdit->setText(alias->strGroup);
    m_sendTextEdit->setPlainText(alias->contents);
    m_scriptEdit->setText(alias->strProcedure);

    // Set send-to combo
    int index = m_sendToCombo->findData(alias->iSendTo);
    if (index >= 0) {
        m_sendToCombo->setCurrentIndex(index);
    }

    // Options
    m_echoAliasCheck->setChecked(alias->bEchoAlias);
    m_keepEvaluatingCheck->setChecked(alias->bKeepEvaluating);
    m_expandVariablesCheck->setChecked(alias->bExpandVariables);
    m_omitFromOutputCheck->setChecked(alias->bOmitFromOutput);
    m_omitFromLogCheck->setChecked(alias->bOmitFromLog);
    m_omitFromHistoryCheck->setChecked(alias->bOmitFromCommandHistory);
}

bool AliasEditDialog::validateForm()
{
    // Match pattern is required
    if (m_matchEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
                             "Match pattern is required.\n\nPlease enter the text to match.");
        m_matchEdit->setFocus();
        return false;
    }

    // If regexp is checked, validate the pattern
    if (m_regexpCheck->isChecked()) {
        QRegularExpression regex(m_matchEdit->text());
        if (!regex.isValid()) {
            QMessageBox::warning(
                this, "Validation Error",
                QString("Invalid regular expression:\n\n%1").arg(regex.errorString()));
            m_matchEdit->setFocus();
            return false;
        }
    }

    return true;
}

bool AliasEditDialog::saveAlias()
{
    Alias* alias = nullptr;

    if (m_isEditMode) {
        // Edit existing alias
        alias = m_doc->getAlias(m_aliasName);
        if (!alias) {
            QMessageBox::warning(this, "Error", "Alias not found: " + m_aliasName);
            return false;
        }

        // Save form data to alias
        alias->strLabel = m_labelEdit->text().trimmed();
        alias->name = m_matchEdit->text();
        alias->bEnabled = m_enabledCheck->isChecked();
        alias->bRegexp = m_regexpCheck->isChecked();
        alias->iSequence = m_sequenceSpin->value();
        alias->strGroup = m_groupEdit->text().trimmed();
        alias->contents = m_sendTextEdit->toPlainText();
        alias->strProcedure = m_scriptEdit->text().trimmed();
        alias->iSendTo = m_sendToCombo->currentData().toInt();

        // Options
        alias->bEchoAlias = m_echoAliasCheck->isChecked();
        alias->bKeepEvaluating = m_keepEvaluatingCheck->isChecked();
        alias->bExpandVariables = m_expandVariablesCheck->isChecked();
        alias->bOmitFromOutput = m_omitFromOutputCheck->isChecked();
        alias->bOmitFromLog = m_omitFromLogCheck->isChecked();
        alias->bOmitFromCommandHistory = m_omitFromHistoryCheck->isChecked();

        // Compile regexp if needed
        if (alias->bRegexp) {
            alias->compileRegexp();
        }
    } else {
        // Create new alias
        auto newAlias = std::make_unique<Alias>();

        // Generate internal name
        QString label = m_labelEdit->text().trimmed();
        if (label.isEmpty()) {
            // Generate unique name based on match pattern
            QString match = m_matchEdit->text();
            if (match.length() > 50) {
                match = match.left(50) + "...";
            }
            newAlias->strInternalName =
                QString("alias_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qHash(match));
        } else {
            newAlias->strInternalName = label;
        }

        // Save form data to alias
        newAlias->strLabel = m_labelEdit->text().trimmed();
        newAlias->name = m_matchEdit->text();
        newAlias->bEnabled = m_enabledCheck->isChecked();
        newAlias->bRegexp = m_regexpCheck->isChecked();
        newAlias->iSequence = m_sequenceSpin->value();
        newAlias->strGroup = m_groupEdit->text().trimmed();
        newAlias->contents = m_sendTextEdit->toPlainText();
        newAlias->strProcedure = m_scriptEdit->text().trimmed();
        newAlias->iSendTo = m_sendToCombo->currentData().toInt();

        // Options
        newAlias->bEchoAlias = m_echoAliasCheck->isChecked();
        newAlias->bKeepEvaluating = m_keepEvaluatingCheck->isChecked();
        newAlias->bExpandVariables = m_expandVariablesCheck->isChecked();
        newAlias->bOmitFromOutput = m_omitFromOutputCheck->isChecked();
        newAlias->bOmitFromLog = m_omitFromLogCheck->isChecked();
        newAlias->bOmitFromCommandHistory = m_omitFromHistoryCheck->isChecked();

        // Compile regexp if needed
        if (newAlias->bRegexp) {
            newAlias->compileRegexp();
        }

        // Add to document
        QString internalName = newAlias->strInternalName;
        if (!m_doc->addAlias(internalName, std::move(newAlias))) {
            QMessageBox::warning(this, "Error",
                                 "Failed to add alias. An alias with this name may already exist.");
            return false;
        }
    }

    return true;
}

void AliasEditDialog::onOk()
{
    if (!validateForm()) {
        return;
    }

    if (saveAlias()) {
        accept();
    }
}

void AliasEditDialog::onCancel()
{
    reject();
}
