#include "logging_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoggingPage::LoggingPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void LoggingPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Enable logging
    m_enableLogCheck = new QCheckBox(tr("Enable automatic logging"), this);
    connect(m_enableLogCheck, &QCheckBox::toggled, this, &LoggingPage::markChanged);
    mainLayout->addWidget(m_enableLogCheck);

    // Log file section
    QGroupBox* fileGroup = new QGroupBox(tr("Log File"), this);
    QFormLayout* fileLayout = new QFormLayout(fileGroup);
    fileLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // File path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_logFileEdit = new QLineEdit(this);
    m_logFileEdit->setPlaceholderText(tr("Path to log file"));
    connect(m_logFileEdit, &QLineEdit::textChanged, this, &LoggingPage::markChanged);
    pathLayout->addWidget(m_logFileEdit);

    m_browseButton = new QPushButton(tr("Browse..."), this);
    connect(m_browseButton, &QPushButton::clicked, this, &LoggingPage::onBrowseClicked);
    pathLayout->addWidget(m_browseButton);

    fileLayout->addRow(tr("Log file:"), pathLayout);

    // Format
    m_logFormatCombo = new QComboBox(this);
    m_logFormatCombo->addItem(tr("Plain Text"), 0);
    m_logFormatCombo->addItem(tr("HTML"), 1);
    m_logFormatCombo->addItem(tr("Raw (with codes)"), 2);
    connect(m_logFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LoggingPage::markChanged);
    fileLayout->addRow(tr("Format:"), m_logFormatCombo);

    mainLayout->addWidget(fileGroup);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_appendLogCheck = new QCheckBox(tr("Append to existing log file"), this);
    connect(m_appendLogCheck, &QCheckBox::toggled, this, &LoggingPage::markChanged);
    optionsLayout->addWidget(m_appendLogCheck);

    m_logInputCheck = new QCheckBox(tr("Log my input"), this);
    connect(m_logInputCheck, &QCheckBox::toggled, this, &LoggingPage::markChanged);
    optionsLayout->addWidget(m_logInputCheck);

    m_logNotesCheck = new QCheckBox(tr("Log notes and system messages"), this);
    connect(m_logNotesCheck, &QCheckBox::toggled, this, &LoggingPage::markChanged);
    optionsLayout->addWidget(m_logNotesCheck);

    mainLayout->addWidget(optionsGroup);

    mainLayout->addStretch();
}

void LoggingPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_enableLogCheck->blockSignals(true);
    m_logFileEdit->blockSignals(true);
    m_logFormatCombo->blockSignals(true);
    m_appendLogCheck->blockSignals(true);
    m_logInputCheck->blockSignals(true);
    m_logNotesCheck->blockSignals(true);

    m_enableLogCheck->setChecked(m_doc->m_bLogOutput != 0);
    m_logFileEdit->setText(m_doc->m_strAutoLogFileName);
    // Determine format from flags: HTML if m_bLogHTML, Raw if m_bLogRaw, else Text
    if (m_doc->m_bLogHTML)
        m_logFormatCombo->setCurrentIndex(1);
    else if (m_doc->m_bLogRaw)
        m_logFormatCombo->setCurrentIndex(2);
    else
        m_logFormatCombo->setCurrentIndex(0);
    m_appendLogCheck->setChecked(false); // Not stored in WorldDocument
    m_logInputCheck->setChecked(false);  // Not stored in WorldDocument
    m_logNotesCheck->setChecked(m_doc->m_bLogNotes != 0);

    // Unblock signals
    m_enableLogCheck->blockSignals(false);
    m_logFileEdit->blockSignals(false);
    m_logFormatCombo->blockSignals(false);
    m_appendLogCheck->blockSignals(false);
    m_logInputCheck->blockSignals(false);
    m_logNotesCheck->blockSignals(false);

    m_hasChanges = false;
}

void LoggingPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->m_bLogOutput = m_enableLogCheck->isChecked() ? 1 : 0;
    m_doc->m_strAutoLogFileName = m_logFileEdit->text();
    // Save format as flags
    m_doc->m_bLogHTML = (m_logFormatCombo->currentIndex() == 1) ? 1 : 0;
    m_doc->m_bLogRaw = (m_logFormatCombo->currentIndex() == 2) ? 1 : 0;
    // m_appendLogCheck and m_logInputCheck not stored in WorldDocument
    m_doc->m_bLogNotes = m_logNotesCheck->isChecked() ? 1 : 0;

    m_doc->setModified(true);

    m_hasChanges = false;
}

bool LoggingPage::hasChanges() const
{
    return m_hasChanges;
}

void LoggingPage::onBrowseClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Choose Log File"), m_logFileEdit->text(),
        tr("Log Files (*.log *.txt);;HTML Files (*.html *.htm);;All Files (*)"));

    if (!fileName.isEmpty()) {
        m_logFileEdit->setText(fileName);
        markChanged();
    }
}

void LoggingPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
