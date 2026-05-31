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
        m_lineCountSpin->setValue(static_cast<int>(m_doc->m_lineList.size()));
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
    auto& db = Database::instance();

    // Load search history
    QString historyStr = db.getPreference("RecallHistory", "");
    if (!historyStr.isEmpty()) {
        m_searchHistory = historyStr.split('\n', Qt::SkipEmptyParts);
        for (const QString& text : m_searchHistory) {
            m_searchTextCombo->addItem(text);
        }
    }

    // Match-case and regex are not document-backed in the original (they live in the
    // transient find info); persist them globally for convenience.
    m_matchCaseCheck->setChecked(db.getPreferenceInt("RecallMatchCase", 0) != 0);
    m_useRegexCheck->setChecked(db.getPreferenceInt("RecallUseRegex", 0) != 0);

    // Line-type flags and the recall preamble are per-world document state in the
    // original (CMUSHclientDoc::m_bRecallCommands/Output/Notes, m_strRecallLinePreamble;
    // see doc.cpp:4930-4933). Seed the dialog from the document so each world keeps
    // its own settings; fall back to global preferences only when no document is set.
    if (m_doc) {
        m_includeOutputCheck->setChecked(m_doc->m_bRecallOutput);
        m_includeCommandsCheck->setChecked(m_doc->m_bRecallCommands);
        m_includeNotesCheck->setChecked(m_doc->m_bRecallNotes);
        m_linePreambleEdit->setText(m_doc->m_output.recall_line_preamble);
    } else {
        m_includeOutputCheck->setChecked(db.getPreferenceInt("RecallIncludeOutput", 1) != 0);
        m_includeCommandsCheck->setChecked(db.getPreferenceInt("RecallIncludeCommands", 0) != 0);
        m_includeNotesCheck->setChecked(db.getPreferenceInt("RecallIncludeNotes", 0) != 0);
        m_linePreambleEdit->setText(db.getPreference("RecallLinePreamble", ""));
    }
}

void RecallSearchDialog::saveSettings()
{
    auto& db = Database::instance();

    // Save search history (limit to 20)
    while (m_searchHistory.size() > 20) {
        m_searchHistory.removeLast();
    }
    QString historyStr = m_searchHistory.join('\n');
    db.setPreference("RecallHistory", historyStr);

    // Match-case and regex are global conveniences (not document-backed).
    db.setPreferenceInt("RecallMatchCase", m_matchCaseCheck->isChecked() ? 1 : 0);
    db.setPreferenceInt("RecallUseRegex", m_useRegexCheck->isChecked() ? 1 : 0);

    // The line-type flags and recall preamble belong to the world document and are
    // written back there in onOkClicked() (matching the original, which only commits
    // them on IDOK). When there is no document, keep persisting the globals so the
    // documentless path retains its previous behavior.
    if (!m_doc) {
        db.setPreferenceInt("RecallIncludeOutput", m_includeOutputCheck->isChecked() ? 1 : 0);
        db.setPreferenceInt("RecallIncludeCommands", m_includeCommandsCheck->isChecked() ? 1 : 0);
        db.setPreferenceInt("RecallIncludeNotes", m_includeNotesCheck->isChecked() ? 1 : 0);
        db.setPreference("RecallLinePreamble", m_linePreambleEdit->text());
    }
}

void RecallSearchDialog::onOkClicked()
{
    QString searchText = m_searchTextCombo->currentText();
    if (searchText.isEmpty()) {
        QMessageBox::information(this, "Recall", "Please enter text to search for.");
        m_searchTextCombo->setFocus();
        return;
    }

    // At least one line type must be selected
    if (!m_includeOutputCheck->isChecked() && !m_includeCommandsCheck->isChecked() &&
        !m_includeNotesCheck->isChecked()) {
        QMessageBox::information(this, "Recall", "Please select at least one line type to search.");
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

    // Commit per-world recall state back to the document, matching the original
    // (doc.cpp:4941-4949): line-type flags are stored on the document, and changing
    // the recall preamble marks the document modified.
    if (m_doc) {
        m_doc->m_bRecallCommands = m_includeCommands;
        m_doc->m_bRecallOutput = m_includeOutput;
        m_doc->m_bRecallNotes = m_includeNotes;

        if (m_doc->m_output.recall_line_preamble != m_linePreamble) {
            m_doc->m_output.recall_line_preamble = m_linePreamble;
            m_doc->setModified(true);
        }
    }

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
