#include "tab_defaults_dialog.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

TabDefaultsDialog::TabDefaultsDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Tab Completion Defaults");
    setMinimumSize(400, 300);
    setupUi();
    loadSettings();
}

void TabDefaultsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for main controls
    QFormLayout* formLayout = new QFormLayout();

    // Default words
    m_defaultWords = new QPlainTextEdit(this);
    m_defaultWords->setPlaceholderText("Enter default words for tab completion (one per line)");
    m_defaultWords->setToolTip("Default words to use for tab completion, one word per line");
    formLayout->addRow("&Default words:", m_defaultWords);

    // Lines to search
    m_linesToSearch = new QSpinBox(this);
    m_linesToSearch->setRange(1, 500000);
    m_linesToSearch->setToolTip("Number of lines to search for tab completion words");
    formLayout->addRow("&Lines to search:", m_linesToSearch);

    // Add space after completion
    m_addSpace = new QCheckBox("Add &space after completion", this);
    m_addSpace->setToolTip("Automatically add a space after tab-completed words");
    formLayout->addRow("", m_addSpace);

    mainLayout->addLayout(formLayout);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &TabDefaultsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TabDefaultsDialog::onRejected);
}

void TabDefaultsDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Load default words
    m_defaultWords->setPlainText(m_doc->m_strTabCompletionDefaults);

    // Load lines to search
    m_linesToSearch->setValue(m_doc->m_iTabCompletionLines);

    // Load add space option
    m_addSpace->setChecked(m_doc->m_bTabCompletionSpace);
}

void TabDefaultsDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Save default words
    m_doc->m_strTabCompletionDefaults = m_defaultWords->toPlainText();

    // Save lines to search
    m_doc->m_iTabCompletionLines = m_linesToSearch->value();

    // Save add space option
    m_doc->m_bTabCompletionSpace = m_addSpace->isChecked();

    // Mark document as modified
    m_doc->setModified(true);
}

void TabDefaultsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void TabDefaultsDialog::onRejected()
{
    reject();
}
