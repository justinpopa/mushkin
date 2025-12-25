#include "recall_search_dialog.h"
#include "../../storage/database.h"
#include "../../world/world_document.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

RecallSearchDialog::RecallSearchDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_matchCase(false), m_useRegex(false), m_includeOutput(true),
      m_includeCommands(false), m_includeNotes(false), m_lineCount(0)
{
    setWindowTitle("Recall");
    setupUi();
    loadSettings();

    // Set line count to total lines in buffer
    if (m_doc) {
        m_lineCountSpin->setValue(m_doc->m_lineList.count());
    }
}

RecallSearchDialog::~RecallSearchDialog()
{
    saveSettings();
}

void RecallSearchDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search text
    QFormLayout* searchLayout = new QFormLayout();
    m_searchTextCombo = new QComboBox(this);
    m_searchTextCombo->setEditable(true);
    m_searchTextCombo->setMinimumWidth(300);
    searchLayout->addRow("Find what:", m_searchTextCombo);
    mainLayout->addLayout(searchLayout);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_matchCaseCheck = new QCheckBox("&Match case", this);
    optionsLayout->addWidget(m_matchCaseCheck);

    m_useRegexCheck = new QCheckBox("&Regular expression", this);
    optionsLayout->addWidget(m_useRegexCheck);

    mainLayout->addWidget(optionsGroup);

    // Line types group
    QGroupBox* lineTypesGroup = new QGroupBox("Line Types", this);
    QVBoxLayout* lineTypesLayout = new QVBoxLayout(lineTypesGroup);

    m_includeOutputCheck = new QCheckBox("&Output", this);
    m_includeOutputCheck->setChecked(true);
    m_includeOutputCheck->setToolTip("Include normal MUD output lines");
    lineTypesLayout->addWidget(m_includeOutputCheck);

    m_includeCommandsCheck = new QCheckBox("&Commands", this);
    m_includeCommandsCheck->setToolTip("Include echoed user commands");
    lineTypesLayout->addWidget(m_includeCommandsCheck);

    m_includeNotesCheck = new QCheckBox("&Notes", this);
    m_includeNotesCheck->setToolTip("Include script notes/comments");
    lineTypesLayout->addWidget(m_includeNotesCheck);

    mainLayout->addWidget(lineTypesGroup);

    // Line count
    QFormLayout* lineCountLayout = new QFormLayout();
    m_lineCountSpin = new QSpinBox(this);
    m_lineCountSpin->setMinimum(0);
    m_lineCountSpin->setMaximum(1000000);
    m_lineCountSpin->setToolTip("Number of lines to search (0 = all lines)");
    lineCountLayout->addRow("Lines to search:", m_lineCountSpin);
    mainLayout->addLayout(lineCountLayout);

    // Line preamble
    QFormLayout* preambleLayout = new QFormLayout();
    m_linePreambleEdit = new QLineEdit(this);
    m_linePreambleEdit->setToolTip(
        "Optional timestamp format to prepend to each line (e.g., %Y-%m-%d %H:%M:%S)");
    preambleLayout->addRow("Line preamble:", m_linePreambleEdit);
    mainLayout->addLayout(preambleLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton("&OK", this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &RecallSearchDialog::onOkClicked);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &RecallSearchDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    QPushButton* regexpHelpButton = new QPushButton("Regexp Help", this);
    connect(regexpHelpButton, &QPushButton::clicked, this,
            &RecallSearchDialog::onRegexpHelpClicked);
    buttonLayout->addWidget(regexpHelpButton);

    mainLayout->addLayout(buttonLayout);
}

void RecallSearchDialog::loadSettings()
{
    Database* db = Database::instance();

    // Load search history
    QString historyStr = db->getPreference("RecallHistory", "");
    if (!historyStr.isEmpty()) {
        m_searchHistory = historyStr.split('\n', Qt::SkipEmptyParts);
        for (const QString& text : m_searchHistory) {
            m_searchTextCombo->addItem(text);
        }
    }

    // Load last search options
    m_matchCaseCheck->setChecked(db->getPreferenceInt("RecallMatchCase", 0) != 0);
    m_useRegexCheck->setChecked(db->getPreferenceInt("RecallUseRegex", 0) != 0);
    m_includeOutputCheck->setChecked(db->getPreferenceInt("RecallIncludeOutput", 1) != 0);
    m_includeCommandsCheck->setChecked(db->getPreferenceInt("RecallIncludeCommands", 0) != 0);
    m_includeNotesCheck->setChecked(db->getPreferenceInt("RecallIncludeNotes", 0) != 0);
    m_linePreambleEdit->setText(db->getPreference("RecallLinePreamble", ""));
}

void RecallSearchDialog::saveSettings()
{
    Database* db = Database::instance();

    // Save search history (limit to 20)
    while (m_searchHistory.size() > 20) {
        m_searchHistory.removeLast();
    }
    QString historyStr = m_searchHistory.join('\n');
    db->setPreference("RecallHistory", historyStr);

    // Save search options
    db->setPreferenceInt("RecallMatchCase", m_matchCaseCheck->isChecked() ? 1 : 0);
    db->setPreferenceInt("RecallUseRegex", m_useRegexCheck->isChecked() ? 1 : 0);
    db->setPreferenceInt("RecallIncludeOutput", m_includeOutputCheck->isChecked() ? 1 : 0);
    db->setPreferenceInt("RecallIncludeCommands", m_includeCommandsCheck->isChecked() ? 1 : 0);
    db->setPreferenceInt("RecallIncludeNotes", m_includeNotesCheck->isChecked() ? 1 : 0);
    db->setPreference("RecallLinePreamble", m_linePreambleEdit->text());
}

void RecallSearchDialog::onOkClicked()
{
    QString searchText = m_searchTextCombo->currentText();
    if (searchText.isEmpty()) {
        QMessageBox::information(this, "Recall", "Please enter text to search for.");
        m_searchTextCombo->setFocus();
        return;
    }

    // Add to history
    if (!m_searchHistory.contains(searchText)) {
        m_searchHistory.prepend(searchText);
        m_searchTextCombo->insertItem(0, searchText);

        // Limit history
        while (m_searchHistory.size() > 20) {
            m_searchHistory.removeLast();
            m_searchTextCombo->removeItem(m_searchTextCombo->count() - 1);
        }
    }

    // Store results
    m_searchText = searchText;
    m_matchCase = m_matchCaseCheck->isChecked();
    m_useRegex = m_useRegexCheck->isChecked();
    m_includeOutput = m_includeOutputCheck->isChecked();
    m_includeCommands = m_includeCommandsCheck->isChecked();
    m_includeNotes = m_includeNotesCheck->isChecked();
    m_lineCount = m_lineCountSpin->value();
    m_linePreamble = m_linePreambleEdit->text();

    accept();
}

void RecallSearchDialog::onCancelClicked()
{
    reject();
}

void RecallSearchDialog::onRegexpHelpClicked()
{
    QString helpText = "Regular Expression Help:\n\n"
                       ".       Match any character\n"
                       "^       Match start of line\n"
                       "$       Match end of line\n"
                       "*       Match 0 or more of previous\n"
                       "+       Match 1 or more of previous\n"
                       "?       Match 0 or 1 of previous\n"
                       "[abc]   Match any of a, b, or c\n"
                       "[^abc]  Match anything except a, b, or c\n"
                       "\\d      Match any digit\n"
                       "\\w      Match any word character\n"
                       "\\s      Match any whitespace\n"
                       "(...)   Capture group\n\n"
                       "Example: \"damage.*\\d+\" matches lines like:\n"
                       "  \"You deal 42 damage to the goblin\"\n"
                       "  \"The troll takes 15 damage\"";

    QMessageBox::information(this, "Regular Expression Help", helpText);
}
