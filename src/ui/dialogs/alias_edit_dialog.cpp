#include "alias_edit_dialog.h"
#include "../../automation/alias.h"
#include "../../automation/script_language.h"
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

    // Script language dropdown
    m_scriptLanguageCombo = new QComboBox(responseTab);
    m_scriptLanguageCombo->addItem("Lua", static_cast<int>(ScriptLanguage::Lua));
    m_scriptLanguageCombo->addItem("YueScript", static_cast<int>(ScriptLanguage::YueScript));
    m_scriptLanguageCombo->addItem("MoonScript", static_cast<int>(ScriptLanguage::MoonScript));
    m_scriptLanguageCombo->addItem("Teal", static_cast<int>(ScriptLanguage::Teal));
    m_scriptLanguageCombo->addItem("Fennel", static_cast<int>(ScriptLanguage::Fennel));
    m_scriptLanguageCombo->setToolTip("Script language for the send text when Send To is Script");
    responseForm->addRow("Script &language:", m_scriptLanguageCombo);

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
    m_labelEdit->setText(alias->label);
    m_matchEdit->setText(alias->name);
    m_enabledCheck->setChecked(alias->enabled);
    m_regexpCheck->setChecked(alias->use_regexp);
    m_sequenceSpin->setValue(alias->sequence);
    m_groupEdit->setText(alias->group);
    m_sendTextEdit->setPlainText(alias->contents);
    m_scriptEdit->setText(alias->procedure);

    // Set send-to combo
    int index = m_sendToCombo->findData(static_cast<int>(alias->send_to));
    if (index >= 0) {
        m_sendToCombo->setCurrentIndex(index);
    }

    // Set script language combo
    int langIndex = m_scriptLanguageCombo->findData(static_cast<int>(alias->scriptLanguage));
    if (langIndex >= 0) {
        m_scriptLanguageCombo->setCurrentIndex(langIndex);
    }

    // Options
    m_echoAliasCheck->setChecked(alias->echo_alias);
    m_keepEvaluatingCheck->setChecked(alias->keep_evaluating);
    m_expandVariablesCheck->setChecked(alias->expand_variables);
    m_omitFromOutputCheck->setChecked(alias->omit_from_output);
    m_omitFromLogCheck->setChecked(alias->omit_from_log);
    m_omitFromHistoryCheck->setChecked(alias->omit_from_command_history);
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
        alias->label = m_labelEdit->text().trimmed();
        alias->name = m_matchEdit->text();
        alias->enabled = m_enabledCheck->isChecked();
        alias->use_regexp = m_regexpCheck->isChecked();
        alias->sequence = m_sequenceSpin->value();
        alias->group = m_groupEdit->text().trimmed();
        alias->contents = m_sendTextEdit->toPlainText();
        alias->procedure = m_scriptEdit->text().trimmed();
        alias->send_to = static_cast<SendTo>(m_sendToCombo->currentData().toInt());
        alias->scriptLanguage =
            static_cast<ScriptLanguage>(m_scriptLanguageCombo->currentData().toInt());

        // Options
        alias->echo_alias = m_echoAliasCheck->isChecked();
        alias->keep_evaluating = m_keepEvaluatingCheck->isChecked();
        alias->expand_variables = m_expandVariablesCheck->isChecked();
        alias->omit_from_output = m_omitFromOutputCheck->isChecked();
        alias->omit_from_log = m_omitFromLogCheck->isChecked();
        alias->omit_from_command_history = m_omitFromHistoryCheck->isChecked();

        // Compile regexp if needed
        if (alias->use_regexp) {
            (void)alias->compileRegexp();
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
            newAlias->internal_name =
                QString("alias_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qHash(match));
        } else {
            newAlias->internal_name = label;
        }

        // Save form data to alias
        newAlias->label = m_labelEdit->text().trimmed();
        newAlias->name = m_matchEdit->text();
        newAlias->enabled = m_enabledCheck->isChecked();
        newAlias->use_regexp = m_regexpCheck->isChecked();
        newAlias->sequence = m_sequenceSpin->value();
        newAlias->group = m_groupEdit->text().trimmed();
        newAlias->contents = m_sendTextEdit->toPlainText();
        newAlias->procedure = m_scriptEdit->text().trimmed();
        newAlias->send_to = static_cast<SendTo>(m_sendToCombo->currentData().toInt());
        newAlias->scriptLanguage =
            static_cast<ScriptLanguage>(m_scriptLanguageCombo->currentData().toInt());

        // Options
        newAlias->echo_alias = m_echoAliasCheck->isChecked();
        newAlias->keep_evaluating = m_keepEvaluatingCheck->isChecked();
        newAlias->expand_variables = m_expandVariablesCheck->isChecked();
        newAlias->omit_from_output = m_omitFromOutputCheck->isChecked();
        newAlias->omit_from_log = m_omitFromLogCheck->isChecked();
        newAlias->omit_from_command_history = m_omitFromHistoryCheck->isChecked();

        // Compile regexp if needed
        if (newAlias->use_regexp) {
            (void)newAlias->compileRegexp();
        }

        // Add to document
        QString internalName = newAlias->internal_name;
        if (auto res = m_doc->addAlias(internalName, std::move(newAlias)); !res.has_value()) {
            QMessageBox::warning(this, "Error",
                                 QString("Failed to add alias: %1").arg(res.error().message));
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
