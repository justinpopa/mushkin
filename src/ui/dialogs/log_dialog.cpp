#include "log_dialog.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

LogDialog::LogDialog(WorldDocument* doc, QWidget* parent) : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Logging - " + doc->m_mush_name);
    setupUi();
    loadSettings();
}

void LogDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for main controls
    QFormLayout* formLayout = new QFormLayout();

    // Lines to log
    m_lines = new QSpinBox(this);
    m_lines->setRange(0, 500000);
    m_lines->setToolTip("Number of lines to log (0 = unlimited)");
    formLayout->addRow("&Lines to log:", m_lines);

    // Log file preamble
    m_preamble = new QLineEdit(this);
    m_preamble->setPlaceholderText("Optional text to write at start of log file");
    m_preamble->setToolTip("Text to write at the beginning of the log file");
    formLayout->addRow("Log file &preamble:", m_preamble);

    mainLayout->addLayout(formLayout);

    // Checkbox group
    QGroupBox* optionsGroup = new QGroupBox("Logging Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_appendToLogFile = new QCheckBox("&Append to log file", this);
    m_appendToLogFile->setToolTip("Append to existing log file instead of overwriting");
    optionsLayout->addWidget(m_appendToLogFile);

    m_writeWorldName = new QCheckBox("&Write world name", this);
    m_writeWorldName->setToolTip("Write world name to log file");
    optionsLayout->addWidget(m_writeWorldName);

    m_logNotes = new QCheckBox("Log &notes", this);
    m_logNotes->setToolTip("Log notes/script output to log file");
    optionsLayout->addWidget(m_logNotes);

    m_logInput = new QCheckBox("Log &input", this);
    m_logInput->setToolTip("Log player input/commands to log file");
    optionsLayout->addWidget(m_logInput);

    m_logOutput = new QCheckBox("Log &output", this);
    m_logOutput->setToolTip("Log MUD output to log file");
    optionsLayout->addWidget(m_logOutput);

    mainLayout->addWidget(optionsGroup);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &LogDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &LogDialog::onRejected);
}

void LogDialog::loadSettings()
{
    if (!m_doc)
        return;

    // TODO: Load lines to log setting when field is added to WorldDocument
    // m_lines->setValue(m_doc->???);

    // Load log file preamble
    m_preamble->setText(m_doc->m_strLogFilePreamble);

    // TODO: Load append to log file setting when field is added to WorldDocument
    // m_appendToLogFile->setChecked(m_doc->???);

    // Load write world name
    m_writeWorldName->setChecked(m_doc->m_bWriteWorldNameToLog);

    // Load logging flags
    m_logNotes->setChecked(m_doc->m_bLogNotes);
    m_logInput->setChecked(m_doc->m_log_input);
    m_logOutput->setChecked(m_doc->m_bLogOutput);
}

void LogDialog::saveSettings()
{
    if (!m_doc)
        return;

    // TODO: Save lines to log setting when field is added to WorldDocument
    // m_doc->??? = m_lines->value();

    // Save log file preamble
    m_doc->m_strLogFilePreamble = m_preamble->text();

    // TODO: Save append to log file setting when field is added to WorldDocument
    // m_doc->??? = m_appendToLogFile->isChecked();

    // Save write world name
    m_doc->m_bWriteWorldNameToLog = m_writeWorldName->isChecked();

    // Save logging flags
    m_doc->m_bLogNotes = m_logNotes->isChecked();
    m_doc->m_log_input = m_logInput->isChecked();
    m_doc->m_bLogOutput = m_logOutput->isChecked();

    // Mark document as modified
    m_doc->setModified(true);
}

void LogDialog::onAccepted()
{
    saveSettings();
    accept();
}

void LogDialog::onRejected()
{
    reject();
}
