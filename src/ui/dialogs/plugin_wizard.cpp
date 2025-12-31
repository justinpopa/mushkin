#include "plugin_wizard.h"
#include "automation/alias.h"
#include "automation/timer.h"
#include "automation/trigger.h"
#include "automation/variable.h"
#include "storage/global_options.h"
#include "world/world_document.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QUuid>
#include <QVBoxLayout>

// ============================================================================
// Page 1: Plugin Metadata
// ============================================================================

PluginWizardPage1::PluginWizardPage1(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc)
{
    setTitle(tr("Plugin Metadata"));
    setSubTitle(tr("Enter basic information about your plugin"));

    QFormLayout* layout = new QFormLayout(this);

    // Plugin Name
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("e.g., HealthBar"));
    layout->addRow(tr("&Name:"), m_nameEdit);
    registerField("name*", m_nameEdit);

    // Plugin ID (GUID)
    QHBoxLayout* idLayout = new QHBoxLayout();
    m_idEdit = new QLineEdit(this);
    m_idEdit->setPlaceholderText(tr("{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"));
    m_generateIdButton = new QPushButton(tr("Generate"), this);
    idLayout->addWidget(m_idEdit);
    idLayout->addWidget(m_generateIdButton);
    layout->addRow(tr("&ID:"), idLayout);
    registerField("id*", m_idEdit);

    connect(m_generateIdButton, &QPushButton::clicked, [this]() {
        QString guid = QUuid::createUuid().toString();
        m_idEdit->setText(guid);
    });

    // Author
    m_authorEdit = new QLineEdit(this);
    m_authorEdit->setPlaceholderText(tr("Your name"));
    layout->addRow(tr("&Author:"), m_authorEdit);
    registerField("author", m_authorEdit);

    // Purpose
    m_purposeEdit = new QLineEdit(this);
    m_purposeEdit->setPlaceholderText(tr("Brief description (max 100 chars)"));
    m_purposeEdit->setMaxLength(100);
    layout->addRow(tr("&Purpose:"), m_purposeEdit);
    registerField("purpose", m_purposeEdit);

    // Version
    m_versionEdit = new QLineEdit(this);
    m_versionEdit->setText("1.0");
    layout->addRow(tr("&Version:"), m_versionEdit);
    registerField("version", m_versionEdit);

    // Date Written
    m_dateWrittenEdit = new QLineEdit(this);
    m_dateWrittenEdit->setReadOnly(true);
    layout->addRow(tr("Date &Written:"), m_dateWrittenEdit);
    registerField("dateWritten", m_dateWrittenEdit);

    // Requires MUSHclient Version
    m_requiresSpin = new QDoubleSpinBox(this);
    m_requiresSpin->setRange(0.0, 100.0);
    m_requiresSpin->setValue(5.0);
    m_requiresSpin->setDecimals(2);
    m_requiresSpin->setSingleStep(0.01);
    layout->addRow(tr("&Requires Version:"), m_requiresSpin);
    registerField("requires", m_requiresSpin, "value");

    // Remove Items checkbox
    m_removeItemsCheck =
        new QCheckBox(tr("Remove selected items from world after creating plugin"), this);
    m_removeItemsCheck->setChecked(true);
    layout->addRow("", m_removeItemsCheck);
    registerField("removeItems", m_removeItemsCheck);
}

void PluginWizardPage1::initializePage()
{
    // Set current date/time
    m_dateWrittenEdit->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    // Generate initial ID if empty
    if (m_idEdit->text().isEmpty()) {
        m_idEdit->setText(QUuid::createUuid().toString());
    }
}

bool PluginWizardPage1::validatePage()
{
    // Validate plugin name (must start with letter, then alphanumeric or underscore)
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("Plugin name cannot be empty."));
        return false;
    }

    QRegularExpression nameRegex("^[a-zA-Z][a-zA-Z0-9_]*$");
    if (!nameRegex.match(name).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Name"),
                             tr("The plugin name must start with a letter and consist of letters, "
                                "numbers or the underscore character."));
        return false;
    }

    // Validate plugin ID (GUID format)
    QString id = m_idEdit->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid ID"), tr("Plugin ID cannot be empty."));
        return false;
    }

    // GUID should be 38 chars with braces: {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    if (id.length() != 38 || !id.startsWith('{') || !id.endsWith('}')) {
        QMessageBox::warning(this, tr("Invalid ID"),
                             tr("Plugin ID must be a GUID in format:\n"
                                "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"));
        return false;
    }

    // Auto-generate help alias based on plugin name (for Page 2)
    if (wizard()->field("helpAlias").toString().isEmpty()) {
        wizard()->setField("helpAlias", name + ":help");
    }

    return true;
}

// ============================================================================
// Page 2: Description and Help Alias
// ============================================================================

PluginWizardPage2::PluginWizardPage2(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc)
{
    setTitle(tr("Description"));
    setSubTitle(tr("Provide a detailed description of your plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Description text edit
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText(
        tr("Enter a detailed description of what this plugin does..."));
    m_descriptionEdit->setMinimumHeight(150);
    layout->addWidget(m_descriptionEdit);
    registerField("description", m_descriptionEdit, "plainText");

    // Edit button for larger editor
    m_editButton = new QPushButton(tr("&Edit in Larger Window..."), this);
    layout->addWidget(m_editButton);

    connect(m_editButton, &QPushButton::clicked, this, &PluginWizardPage2::onEditDescription);

    // Generate Help section
    QGroupBox* helpGroup = new QGroupBox(tr("Help Alias"), this);
    QFormLayout* helpLayout = new QFormLayout(helpGroup);

    m_generateHelpCheck = new QCheckBox(tr("Generate help alias"), this);
    m_generateHelpCheck->setChecked(true);
    helpLayout->addRow(m_generateHelpCheck);
    registerField("generateHelp", m_generateHelpCheck);

    m_helpAliasLabel = new QLabel(tr("Help Alias &Name:"), this);
    m_helpAliasEdit = new QLineEdit(this);
    m_helpAliasEdit->setPlaceholderText(tr("e.g., pluginname:help"));
    m_helpAliasLabel->setBuddy(m_helpAliasEdit);
    helpLayout->addRow(m_helpAliasLabel, m_helpAliasEdit);
    registerField("helpAlias", m_helpAliasEdit);

    layout->addWidget(helpGroup);

    connect(m_generateHelpCheck, &QCheckBox::toggled, this,
            &PluginWizardPage2::onGenerateHelpToggled);

    // Set initial state
    onGenerateHelpToggled(m_generateHelpCheck->isChecked());
}

void PluginWizardPage2::onEditDescription()
{
    // Create a dialog with a larger text edit
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Plugin Description"));
    dialog.resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTextEdit* textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(m_descriptionEdit->toPlainText());
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_descriptionEdit->setPlainText(textEdit->toPlainText());
    }
}

void PluginWizardPage2::onGenerateHelpToggled(bool checked)
{
    m_helpAliasLabel->setEnabled(checked);
    m_helpAliasEdit->setEnabled(checked);
}

bool PluginWizardPage2::validatePage()
{
    QString description = m_descriptionEdit->toPlainText();

    // Check for invalid XML CDATA sequence
    if (description.contains("]]>")) {
        QMessageBox::warning(this, tr("Invalid Description"),
                             tr("Description may not contain the sequence \"]]>\" as this "
                                "terminates XML CDATA sections."));
        return false;
    }

    return true;
}

// ============================================================================
// Page 3: Triggers Selection
// ============================================================================

PluginWizardPage3::PluginWizardPage3(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc), m_lastColumn(0), m_reverseSort(false)
{
    setTitle(tr("Select Triggers"));
    setSubTitle(tr("Choose which triggers to include in the plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Triggers table
    m_triggerTable = new QTableWidget(this);
    m_triggerTable->setColumnCount(COL_COUNT);
    m_triggerTable->setHorizontalHeaderLabels({tr("Name"), tr("Match"), tr("Send"), tr("Group")});
    m_triggerTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_triggerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_triggerTable->verticalHeader()->hide();
    m_triggerTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_triggerTable);

    connect(m_triggerTable->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &PluginWizardPage3::onHeaderClicked);

    // Select All/None buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("Select &All"), this);
    m_selectNoneButton = new QPushButton(tr("Select &None"), this);
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_selectNoneButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(m_selectAllButton, &QPushButton::clicked, this, &PluginWizardPage3::onSelectAll);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &PluginWizardPage3::onSelectNone);
}

void PluginWizardPage3::initializePage()
{
    // Populate triggers from world (skip temporary ones)
    m_triggerTable->setRowCount(0);

    for (Trigger* trigger : m_doc->m_TriggerArray) {
        if (trigger->bTemporary)
            continue;

        int row = m_triggerTable->rowCount();
        m_triggerTable->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(trigger->strLabel);
        nameItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        nameItem->setCheckState(Qt::Checked); // Select all by default
        nameItem->setData(Qt::UserRole, QVariant::fromValue<void*>(trigger));
        m_triggerTable->setItem(row, COL_NAME, nameItem);

        m_triggerTable->setItem(row, COL_MATCH, new QTableWidgetItem(trigger->trigger));
        m_triggerTable->setItem(row, COL_SEND, new QTableWidgetItem(trigger->contents));
        m_triggerTable->setItem(row, COL_GROUP, new QTableWidgetItem(trigger->strGroup));
    }

    // Set column widths
    m_triggerTable->setColumnWidth(COL_NAME, 100);
    m_triggerTable->setColumnWidth(COL_MATCH, 120);
    m_triggerTable->setColumnWidth(COL_SEND, 120);
}

bool PluginWizardPage3::validatePage()
{
    // Mark selected triggers in world document
    for (Trigger* trigger : m_doc->m_TriggerArray) {
        trigger->bSelected = false;
    }

    for (int row = 0; row < m_triggerTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_triggerTable->item(row, COL_NAME);
        if (item && item->checkState() == Qt::Checked) {
            Trigger* trigger = static_cast<Trigger*>(item->data(Qt::UserRole).value<void*>());
            if (trigger) {
                trigger->bSelected = true;
            }
        }
    }

    return true;
}

void PluginWizardPage3::onSelectAll()
{
    for (int row = 0; row < m_triggerTable->rowCount(); ++row) {
        m_triggerTable->item(row, COL_NAME)->setCheckState(Qt::Checked);
    }
}

void PluginWizardPage3::onSelectNone()
{
    for (int row = 0; row < m_triggerTable->rowCount(); ++row) {
        m_triggerTable->item(row, COL_NAME)->setCheckState(Qt::Unchecked);
    }
}

void PluginWizardPage3::onHeaderClicked(int column)
{
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
    }
    m_lastColumn = column;

    m_triggerTable->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}

// ============================================================================
// Page 4: Aliases Selection
// ============================================================================

PluginWizardPage4::PluginWizardPage4(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc), m_lastColumn(0), m_reverseSort(false)
{
    setTitle(tr("Select Aliases"));
    setSubTitle(tr("Choose which aliases to include in the plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Aliases table
    m_aliasTable = new QTableWidget(this);
    m_aliasTable->setColumnCount(COL_COUNT);
    m_aliasTable->setHorizontalHeaderLabels({tr("Name"), tr("Match"), tr("Send"), tr("Group")});
    m_aliasTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_aliasTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_aliasTable->verticalHeader()->hide();
    m_aliasTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_aliasTable);

    connect(m_aliasTable->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &PluginWizardPage4::onHeaderClicked);

    // Select All/None buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("Select &All"), this);
    m_selectNoneButton = new QPushButton(tr("Select &None"), this);
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_selectNoneButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(m_selectAllButton, &QPushButton::clicked, this, &PluginWizardPage4::onSelectAll);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &PluginWizardPage4::onSelectNone);
}

void PluginWizardPage4::initializePage()
{
    // Populate aliases from world (skip temporary ones)
    m_aliasTable->setRowCount(0);

    for (Alias* alias : m_doc->m_AliasArray) {
        if (alias->bTemporary)
            continue;

        int row = m_aliasTable->rowCount();
        m_aliasTable->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(alias->strLabel);
        nameItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        nameItem->setCheckState(Qt::Checked);
        nameItem->setData(Qt::UserRole, QVariant::fromValue<void*>(alias));
        m_aliasTable->setItem(row, COL_NAME, nameItem);

        m_aliasTable->setItem(row, COL_MATCH, new QTableWidgetItem(alias->name));
        m_aliasTable->setItem(row, COL_SEND, new QTableWidgetItem(alias->contents));
        m_aliasTable->setItem(row, COL_GROUP, new QTableWidgetItem(alias->strGroup));
    }

    // Set column widths
    m_aliasTable->setColumnWidth(COL_NAME, 100);
    m_aliasTable->setColumnWidth(COL_MATCH, 120);
    m_aliasTable->setColumnWidth(COL_SEND, 120);
}

bool PluginWizardPage4::validatePage()
{
    // Mark selected aliases in world document
    for (Alias* alias : m_doc->m_AliasArray) {
        alias->bSelected = false;
    }

    for (int row = 0; row < m_aliasTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_aliasTable->item(row, COL_NAME);
        if (item && item->checkState() == Qt::Checked) {
            Alias* alias = static_cast<Alias*>(item->data(Qt::UserRole).value<void*>());
            if (alias) {
                alias->bSelected = true;
            }
        }
    }

    return true;
}

void PluginWizardPage4::onSelectAll()
{
    for (int row = 0; row < m_aliasTable->rowCount(); ++row) {
        m_aliasTable->item(row, COL_NAME)->setCheckState(Qt::Checked);
    }
}

void PluginWizardPage4::onSelectNone()
{
    for (int row = 0; row < m_aliasTable->rowCount(); ++row) {
        m_aliasTable->item(row, COL_NAME)->setCheckState(Qt::Unchecked);
    }
}

void PluginWizardPage4::onHeaderClicked(int column)
{
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
    }
    m_lastColumn = column;

    m_aliasTable->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}

// ============================================================================
// Page 5: Timers Selection
// ============================================================================

PluginWizardPage5::PluginWizardPage5(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc), m_lastColumn(0), m_reverseSort(false)
{
    setTitle(tr("Select Timers"));
    setSubTitle(tr("Choose which timers to include in the plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Timers table
    m_timerTable = new QTableWidget(this);
    m_timerTable->setColumnCount(COL_COUNT);
    m_timerTable->setHorizontalHeaderLabels({tr("Name"), tr("Time"), tr("Send"), tr("Group")});
    m_timerTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_timerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_timerTable->verticalHeader()->hide();
    m_timerTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_timerTable);

    connect(m_timerTable->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &PluginWizardPage5::onHeaderClicked);

    // Select All/None buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("Select &All"), this);
    m_selectNoneButton = new QPushButton(tr("Select &None"), this);
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_selectNoneButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(m_selectAllButton, &QPushButton::clicked, this, &PluginWizardPage5::onSelectAll);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &PluginWizardPage5::onSelectNone);
}

void PluginWizardPage5::initializePage()
{
    // Populate timers from world (skip temporary ones)
    m_timerTable->setRowCount(0);

    for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
        Timer* timer = timerPtr.get();
        if (timer->bTemporary)
            continue;

        int row = m_timerTable->rowCount();
        m_timerTable->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(timer->strLabel);
        nameItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        nameItem->setCheckState(Qt::Checked);
        nameItem->setData(Qt::UserRole, QVariant::fromValue<void*>(timer));
        m_timerTable->setItem(row, COL_NAME, nameItem);

        // Format time string
        QString timeStr;
        if (timer->iType == 0) { // Interval timer
            timeStr = QString("Every %1:%2:%3")
                          .arg(timer->iEveryHour, 2, 10, QChar('0'))
                          .arg(timer->iEveryMinute, 2, 10, QChar('0'))
                          .arg(timer->fEverySecond, 4, 'f', 2);
        } else { // At time timer
            timeStr = QString("At %1:%2:%3")
                          .arg(timer->iAtHour, 2, 10, QChar('0'))
                          .arg(timer->iAtMinute, 2, 10, QChar('0'))
                          .arg(timer->fAtSecond, 4, 'f', 2);
        }

        m_timerTable->setItem(row, COL_TIME, new QTableWidgetItem(timeStr));
        m_timerTable->setItem(row, COL_SEND, new QTableWidgetItem(timer->strContents));
        m_timerTable->setItem(row, COL_GROUP, new QTableWidgetItem(timer->strGroup));
    }

    // Set column widths
    m_timerTable->setColumnWidth(COL_NAME, 100);
    m_timerTable->setColumnWidth(COL_TIME, 120);
    m_timerTable->setColumnWidth(COL_SEND, 120);
}

bool PluginWizardPage5::validatePage()
{
    // Mark selected timers in world document
    for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
        timerPtr->bSelected = false;
    }

    for (int row = 0; row < m_timerTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_timerTable->item(row, COL_NAME);
        if (item && item->checkState() == Qt::Checked) {
            Timer* timer = static_cast<Timer*>(item->data(Qt::UserRole).value<void*>());
            if (timer) {
                timer->bSelected = true;
            }
        }
    }

    return true;
}

void PluginWizardPage5::onSelectAll()
{
    for (int row = 0; row < m_timerTable->rowCount(); ++row) {
        m_timerTable->item(row, COL_NAME)->setCheckState(Qt::Checked);
    }
}

void PluginWizardPage5::onSelectNone()
{
    for (int row = 0; row < m_timerTable->rowCount(); ++row) {
        m_timerTable->item(row, COL_NAME)->setCheckState(Qt::Unchecked);
    }
}

void PluginWizardPage5::onHeaderClicked(int column)
{
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
    }
    m_lastColumn = column;

    m_timerTable->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}

// ============================================================================
// Page 6: Variables Selection
// ============================================================================

PluginWizardPage6::PluginWizardPage6(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc), m_lastColumn(0), m_reverseSort(false)
{
    setTitle(tr("Select Variables"));
    setSubTitle(tr("Choose which variables to include in the plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Variables table
    m_variableTable = new QTableWidget(this);
    m_variableTable->setColumnCount(COL_COUNT);
    m_variableTable->setHorizontalHeaderLabels({tr("Name"), tr("Contents")});
    m_variableTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_variableTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_variableTable->verticalHeader()->hide();
    m_variableTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_variableTable);

    connect(m_variableTable->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &PluginWizardPage6::onHeaderClicked);

    // Select All/None buttons and Save State checkbox
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("Select &All"), this);
    m_selectNoneButton = new QPushButton(tr("Select &None"), this);
    m_saveStateCheck = new QCheckBox(tr("&Save state (retain variables across restarts)"), this);
    buttonLayout->addWidget(m_selectAllButton);
    buttonLayout->addWidget(m_selectNoneButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveStateCheck);
    layout->addLayout(buttonLayout);

    registerField("saveState", m_saveStateCheck);

    connect(m_selectAllButton, &QPushButton::clicked, this, &PluginWizardPage6::onSelectAll);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &PluginWizardPage6::onSelectNone);
}

void PluginWizardPage6::initializePage()
{
    // Populate variables from world
    m_variableTable->setRowCount(0);

    int varCount = 0;
    // Variable system integration
    for (const auto& [name, varPtr] : m_doc->m_VariableMap) {
        Variable* variable = varPtr.get();

        int row = m_variableTable->rowCount();
        m_variableTable->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(variable->strLabel);
        nameItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        nameItem->setCheckState(Qt::Checked);
        nameItem->setData(Qt::UserRole, QVariant::fromValue<void*>(variable));
        m_variableTable->setItem(row, COL_NAME, nameItem);

        m_variableTable->setItem(row, COL_CONTENTS, new QTableWidgetItem(variable->strContents));

        varCount++;
    }

    // Set column widths
    m_variableTable->setColumnWidth(COL_NAME, 150);

    // Auto-check save state if there are variables
    if (varCount > 0) {
        m_saveStateCheck->setChecked(true);
    }
}

bool PluginWizardPage6::validatePage()
{
    // Mark selected variables in world document
    // Variable system integration
    for (auto& [name, varPtr] : m_doc->m_VariableMap) {
        varPtr->bSelected = false;
    }

    for (int row = 0; row < m_variableTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_variableTable->item(row, COL_NAME);
        if (item && item->checkState() == Qt::Checked) {
            Variable* variable = static_cast<Variable*>(item->data(Qt::UserRole).value<void*>());
            if (variable) {
                variable->bSelected = true;
            }
        }
    }

    return true;
}

void PluginWizardPage6::onSelectAll()
{
    for (int row = 0; row < m_variableTable->rowCount(); ++row) {
        m_variableTable->item(row, COL_NAME)->setCheckState(Qt::Checked);
    }
}

void PluginWizardPage6::onSelectNone()
{
    for (int row = 0; row < m_variableTable->rowCount(); ++row) {
        m_variableTable->item(row, COL_NAME)->setCheckState(Qt::Unchecked);
    }
}

void PluginWizardPage6::onHeaderClicked(int column)
{
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
    }
    m_lastColumn = column;

    m_variableTable->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}

// ============================================================================
// Page 7: Script Editor
// ============================================================================

PluginWizardPage7::PluginWizardPage7(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc)
{
    setTitle(tr("Script"));
    setSubTitle(tr("Add script code for your plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Language selector
    QHBoxLayout* langLayout = new QHBoxLayout();
    langLayout->addWidget(new QLabel(tr("Script Language:"), this));
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItems({"Lua", "YueScript", "MoonScript", "Teal", "Fennel"});
    langLayout->addWidget(m_languageCombo);
    langLayout->addStretch();
    layout->addLayout(langLayout);
    registerField("scriptLanguage", m_languageCombo, "currentText");

    // Script text edit
    m_scriptEdit = new QTextEdit(this);
    m_scriptEdit->setPlaceholderText(tr("Enter script code here..."));
    m_scriptEdit->setMinimumHeight(200);
    m_scriptEdit->setFontFamily("Courier New");
    layout->addWidget(m_scriptEdit);
    registerField("script", m_scriptEdit, "plainText");

    // Edit button and constants checkbox
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_editButton = new QPushButton(tr("&Edit in Larger Window..."), this);
    m_includeConstantsCheck = new QCheckBox(tr("&Include standard constants"), this);
    bottomLayout->addWidget(m_editButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_includeConstantsCheck);
    layout->addLayout(bottomLayout);

    registerField("includeConstants", m_includeConstantsCheck);

    connect(m_editButton, &QPushButton::clicked, this, &PluginWizardPage7::onEditScript);
}

void PluginWizardPage7::initializePage()
{
    // Load world's script file if it exists and script is empty
    if (m_scriptEdit->toPlainText().isEmpty() && !m_doc->m_strScriptFilename.isEmpty()) {
        QFile file(m_doc->m_strScriptFilename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            m_scriptEdit->setPlainText(in.readAll());
        }
    }
}

void PluginWizardPage7::onEditScript()
{
    // Create a dialog with a larger text edit
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Plugin Script"));
    dialog.resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTextEdit* textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(m_scriptEdit->toPlainText());
    textEdit->setFontFamily("Courier New");
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_scriptEdit->setPlainText(textEdit->toPlainText());
    }
}

bool PluginWizardPage7::validatePage()
{
    QString script = m_scriptEdit->toPlainText();

    // Check for invalid XML CDATA sequence
    if (script.contains("]]>")) {
        QMessageBox::warning(this, tr("Invalid Script"),
                             tr("Script may not contain the sequence \"]]>\" as this "
                                "terminates XML CDATA sections."));
        return false;
    }

    return true;
}

// ============================================================================
// Page 8: Comments
// ============================================================================

PluginWizardPage8::PluginWizardPage8(WorldDocument* doc, QWidget* parent)
    : QWizardPage(parent), m_doc(doc)
{
    setTitle(tr("Comments"));
    setSubTitle(tr("Add any additional comments about your plugin"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Comments text edit
    m_commentsEdit = new QTextEdit(this);
    m_commentsEdit->setPlaceholderText(
        tr("Enter comments here (e.g., version history, credits, notes)..."));
    m_commentsEdit->setMinimumHeight(200);
    layout->addWidget(m_commentsEdit);
    registerField("comments", m_commentsEdit, "plainText");

    // Edit button
    m_editButton = new QPushButton(tr("&Edit in Larger Window..."), this);
    layout->addWidget(m_editButton);

    connect(m_editButton, &QPushButton::clicked, this, &PluginWizardPage8::onEditComments);
}

void PluginWizardPage8::onEditComments()
{
    // Create a dialog with a larger text edit
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Plugin Comments"));
    dialog.resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTextEdit* textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(m_commentsEdit->toPlainText());
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_commentsEdit->setPlainText(textEdit->toPlainText());
    }
}

bool PluginWizardPage8::validatePage()
{
    QString comments = m_commentsEdit->toPlainText();

    // Check for invalid XML comment sequence
    if (comments.contains("--")) {
        QMessageBox::warning(this, tr("Invalid Comments"),
                             tr("Comments may not contain the sequence \"--\" as this is "
                                "the XML comment terminator."));
        return false;
    }

    return true;
}

// ============================================================================
// Main Wizard Class
// ============================================================================

PluginWizard::PluginWizard(WorldDocument* doc, QWidget* parent) : QWizard(parent), m_doc(doc)
{
    setWindowTitle(tr("Plugin Wizard"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::HaveHelpButton, false);

    // Change OK button to "Create..."
    setButtonText(QWizard::FinishButton, tr("&Create..."));

    // Add all pages
    setPage(PAGE_METADATA, new PluginWizardPage1(doc, this));
    setPage(PAGE_DESCRIPTION, new PluginWizardPage2(doc, this));
    setPage(PAGE_TRIGGERS, new PluginWizardPage3(doc, this));
    setPage(PAGE_ALIASES, new PluginWizardPage4(doc, this));
    setPage(PAGE_TIMERS, new PluginWizardPage5(doc, this));
    setPage(PAGE_VARIABLES, new PluginWizardPage6(doc, this));
    setPage(PAGE_SCRIPT, new PluginWizardPage7(doc, this));
    setPage(PAGE_COMMENTS, new PluginWizardPage8(doc, this));

    resize(800, 600);
}

void PluginWizard::accept()
{
    // Generate plugin XML
    QString xml = generatePluginXml();

    // Save to file
    if (savePluginXml(xml)) {
        // Remove items from world if requested
        if (field("removeItems").toBool()) {
            removeItemsFromWorld();
        }

        QWizard::accept();
    }
}

QString PluginWizard::generatePluginXml()
{
    QString xml;
    QTextStream out(&xml);

    QString name = field("name").toString();
    QString id = field("id").toString();
    QString author = field("author").toString();
    QString purpose = field("purpose").toString();
    QString version = field("version").toString();
    QString dateWritten = field("dateWritten").toString();
    double
        requires
    = field("requires").toDouble();
    QString description = field("description").toString();
    bool generateHelp = field("generateHelp").toBool();
    QString helpAlias = field("helpAlias").toString();
    bool saveState = field("saveState").toBool();
    QString script = field("script").toString();
    bool includeConstants = field("includeConstants").toBool();
    QString comments = field("comments").toString();

    // XML prolog
    out << "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
    out << "<!DOCTYPE muclient>\n\n";

    // Timestamp
    out << QString("<!-- Saved on %1 -->\n")
               .arg(QDateTime::currentDateTime().toString("dddd, MMMM d, yyyy, h:mm AP"));
    out << "<!-- MUSHclient version 5.06 -->\n\n";

    out << QString("<!-- Plugin \"%1\" generated by Plugin Wizard -->\n\n").arg(name);

    // Comments
    if (!comments.isEmpty()) {
        out << "<!--\n";
        out << comments << "\n";
        out << "-->\n\n";
    }

    // Start muclient and plugin
    out << "<muclient>\n";
    out << "<plugin\n";
    out << QString("   name=\"%1\"\n").arg(name);
    out << QString("   author=\"%1\"\n").arg(author);
    out << QString("   id=\"%1\"\n").arg(id);
    out << QString("   language=\"%1\"\n").arg(field("scriptLanguage").toString());
    out << QString("   purpose=\"%1\"\n").arg(purpose);
    out << QString("   save_state=\"%1\"\n").arg(saveState ? "y" : "n");
    out << QString("   date_written=\"%1\"\n").arg(dateWritten);
    out << QString("   requires=\"%1\"\n").arg(requires, 0, 'f', 2);
    out << QString("   version=\"%1\"\n").arg(version);
    out << "   >\n";

    // Description
    if (!description.isEmpty()) {
        out << "<description trim=\"y\">\n";
        out << "<![CDATA[\n";
        out << description << "\n";
        out << "]]>\n";
        out << "</description>\n\n";
    }

    out << "</plugin>\n\n";

    // Include constants
    if (includeConstants) {
        out << "<!--  Get our standard constants -->\n\n";
        out << "<include name=\"constants.lua\"/>\n\n";
    }

    // Triggers
    int triggerCount = 0;
    for (Trigger* t : m_doc->m_TriggerArray) {
        if (t->bSelected)
            triggerCount++;
    }
    if (triggerCount > 0) {
        out << "<!--  Triggers  -->\n\n";
        out << "<triggers>\n";
        for (Trigger* t : m_doc->m_TriggerArray) {
            if (t->bSelected) {
                // Call world document's trigger serialization
                m_doc->Save_One_Trigger_XML(out, t);
            }
        }
        out << "</triggers>\n\n";
    }

    // Aliases
    int aliasCount = 0;
    for (Alias* a : m_doc->m_AliasArray) {
        if (a->bSelected)
            aliasCount++;
    }
    if (aliasCount > 0) {
        out << "<!--  Aliases  -->\n\n";
        out << "<aliases>\n";
        for (Alias* a : m_doc->m_AliasArray) {
            if (a->bSelected) {
                m_doc->Save_One_Alias_XML(out, a);
            }
        }
        out << "</aliases>\n\n";
    }

    // Timers
    int timerCount = 0;
    for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
        if (timerPtr->bSelected)
            timerCount++;
    }
    if (timerCount > 0) {
        out << "<!--  Timers  -->\n\n";
        out << "<timers>\n";
        for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
            if (timerPtr->bSelected) {
                m_doc->Save_One_Timer_XML(out, timerPtr.get());
            }
        }
        out << "</timers>\n\n";
    }

    // Variables
    // Variable system integration
    int variableCount = 0;
    for (const auto& [name, varPtr] : m_doc->m_VariableMap) {
        if (varPtr->bSelected)
            variableCount++;
    }
    if (variableCount > 0) {
        out << "<!--  Variables  -->\n\n";
        out << "<variables>\n";
        for (const auto& [name, varPtr] : m_doc->m_VariableMap) {
            if (varPtr->bSelected) {
                m_doc->Save_One_Variable_XML(out, varPtr.get());
            }
        }
        out << "</variables>\n\n";
    }

    // Script
    if (!script.isEmpty()) {
        out << "<!--  Script  -->\n\n";
        out << "<script>\n";
        out << "<![CDATA[\n";
        out << script << "\n";
        out << "]]>\n";
        out << "</script>\n\n";
    }

    // Help alias
    if (generateHelp && !helpAlias.isEmpty() && !description.isEmpty()) {
        out << "<!--  Plugin help  -->\n\n";
        out << "<aliases>\n";
        out << "  <alias\n";
        out << "   script=\"OnHelp\"\n";
        out << QString("   match=\"%1\"\n").arg(helpAlias);
        out << "   enabled=\"y\"\n";
        out << "  >\n";
        out << "  </alias>\n";
        out << "</aliases>\n\n";
        out << "<script>\n";
        out << "<![CDATA[\n";
        out << "function OnHelp ()\n";
        out << "  world.Note (world.GetPluginInfo (world.GetPluginID (), 3))\n";
        out << "end\n";
        out << "]]>\n";
        out << "</script>\n\n";
    }

    // Close muclient
    out << "</muclient>\n";

    return xml;
}

bool PluginWizard::savePluginXml(const QString& xml)
{
    QString pluginName = field("name").toString();
    QString suggestedFilename = pluginName + ".xml";

    // Use configured plugins directory (matches original MUSHclient behavior)
    QString pluginDir = GlobalOptions::instance()->pluginsDirectory();

    // File save dialog
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Plugin As"),
                                                    QDir(pluginDir).filePath(suggestedFilename),
                                                    tr("Plugin files (*.xml);;All files (*)"));

    if (filename.isEmpty()) {
        return false; // User cancelled
    }

    // Save file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Save Error"),
                              tr("Could not save plugin file:\n%1").arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << xml;
    file.close();

    m_outputFilename = filename;

    QMessageBox::information(this, tr("Plugin Created"),
                             tr("Plugin created successfully:\n%1").arg(filename));

    return true;
}

void PluginWizard::removeItemsFromWorld()
{
    // Remove selected triggers
    QList<QString> triggersToRemove;
    for (Trigger* t : m_doc->m_TriggerArray) {
        if (t->bSelected) {
            triggersToRemove.append(t->strLabel);
        }
    }
    for (const QString& name : triggersToRemove) {
        m_doc->deleteTrigger(name);
    }

    // Remove selected aliases
    QList<QString> aliasesToRemove;
    for (Alias* a : m_doc->m_AliasArray) {
        if (a->bSelected) {
            aliasesToRemove.append(a->strLabel);
        }
    }
    for (const QString& name : aliasesToRemove) {
        m_doc->deleteAlias(name);
    }

    // Remove selected timers
    QList<QString> timersToRemove;
    for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
        if (timerPtr->bSelected) {
            timersToRemove.append(timerPtr->strLabel);
        }
    }
    for (const QString& name : timersToRemove) {
        m_doc->deleteTimer(name);
    }

    // Remove selected variables
    // Variable system integration
    QList<QString> variablesToRemove;
    for (const auto& [name, varPtr] : m_doc->m_VariableMap) {
        if (varPtr->bSelected) {
            variablesToRemove.append(varPtr->strLabel);
        }
    }
    for (const QString& name : variablesToRemove) {
        m_doc->deleteVariable(name);
    }

    // Mark world as modified so user knows to save
    m_doc->setModified(true);

    QMessageBox::information(this, tr("Items Removed"),
                             tr("Selected items have been removed from the world:\n"
                                "%1 trigger(s), %2 alias(es), %3 timer(s), %4 variable(s)")
                                 .arg(triggersToRemove.size())
                                 .arg(aliasesToRemove.size())
                                 .arg(timersToRemove.size())
                                 .arg(variablesToRemove.size()));
}
