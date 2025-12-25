#include "find_dialog.h"
#include "../../storage/database.h"
#include "../../text/line.h"
#include "../../world/world_document.h"
#include "../views/output_view.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QVBoxLayout>

FindDialog::FindDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_lastMatchCase(false), m_lastUseRegex(false),
      m_lastSearchForward(true), m_lastFoundLine(-1), m_lastFoundChar(-1),
      m_totalMatches(0), m_currentMatchIndex(0)
{
    setWindowTitle("Find");
    setupUi();
    loadSettings();
}

FindDialog::~FindDialog()
{
    saveSettings();
}

void FindDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search text combo box
    QFormLayout* searchLayout = new QFormLayout();
    m_searchText = new QComboBox(this);
    m_searchText->setEditable(true);
    m_searchText->setMinimumWidth(300);
    searchLayout->addRow("Find what:", m_searchText);
    mainLayout->addLayout(searchLayout);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_matchCase = new QCheckBox("Match &case", this);
    optionsLayout->addWidget(m_matchCase);

    m_useRegex = new QCheckBox("Use regular &expressions", this);
    optionsLayout->addWidget(m_useRegex);

    mainLayout->addWidget(optionsGroup);

    // Direction group
    QGroupBox* directionGroup = new QGroupBox("Direction", this);
    QVBoxLayout* directionLayout = new QVBoxLayout(directionGroup);

    m_searchForward = new QRadioButton("&Forward", this);
    m_searchForward->setChecked(true);
    directionLayout->addWidget(m_searchForward);

    m_searchBackward = new QRadioButton("&Backward", this);
    directionLayout->addWidget(m_searchBackward);

    mainLayout->addWidget(directionGroup);

    // Match counter label
    m_matchCounterLabel = new QLabel("", this);
    m_matchCounterLabel->setMinimumHeight(20);
    mainLayout->addWidget(m_matchCounterLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_findPrevButton = new QPushButton("Find &Previous", this);
    connect(m_findPrevButton, &QPushButton::clicked, this, &FindDialog::findPrevious);
    buttonLayout->addWidget(m_findPrevButton);

    m_findButton = new QPushButton("&Find Next", this);
    m_findButton->setDefault(true);
    connect(m_findButton, &QPushButton::clicked, this, &FindDialog::findNext);
    buttonLayout->addWidget(m_findButton);

    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &FindDialog::closeDialog);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void FindDialog::loadSettings()
{
    Database* db = Database::instance();

    // Load search history
    QString historyStr = db->getPreference("FindHistory", "");
    if (!historyStr.isEmpty()) {
        m_searchHistory = historyStr.split('\n', Qt::SkipEmptyParts);
        for (const QString& text : m_searchHistory) {
            m_searchText->addItem(text);
        }
    }

    // Load last search options
    m_matchCase->setChecked(db->getPreferenceInt("FindMatchCase", 0) != 0);
    m_useRegex->setChecked(db->getPreferenceInt("FindUseRegex", 0) != 0);
    m_searchForward->setChecked(db->getPreferenceInt("FindForward", 1) != 0);
    m_searchBackward->setChecked(!m_searchForward->isChecked());
}

void FindDialog::saveSettings()
{
    Database* db = Database::instance();

    // Save search history (limit to 20)
    while (m_searchHistory.size() > 20) {
        m_searchHistory.removeLast();
    }
    QString historyStr = m_searchHistory.join('\n');
    db->setPreference("FindHistory", historyStr);

    // Save search options
    db->setPreferenceInt("FindMatchCase", m_matchCase->isChecked() ? 1 : 0);
    db->setPreferenceInt("FindUseRegex", m_useRegex->isChecked() ? 1 : 0);
    db->setPreferenceInt("FindForward", m_searchForward->isChecked() ? 1 : 0);
}

void FindDialog::findNext()
{
    bool forward = m_searchForward->isChecked();
    performSearch(forward);
    updateMatchCounter();
}

void FindDialog::findPrevious()
{
    performSearch(false); // Search backward
    updateMatchCounter();
}

void FindDialog::closeDialog()
{
    accept();
}

bool FindDialog::performSearch(bool forward)
{
    QString searchText = m_searchText->currentText();
    if (searchText.isEmpty()) {
        QMessageBox::information(this, "Find", "Please enter text to find.");
        m_searchText->setFocus();
        return false;
    }

    // Add to history
    if (!m_searchHistory.contains(searchText)) {
        m_searchHistory.prepend(searchText);
        m_searchText->insertItem(0, searchText);

        // Limit history
        while (m_searchHistory.size() > 20) {
            m_searchHistory.removeLast();
            m_searchText->removeItem(m_searchText->count() - 1);
        }
    }

    // Store search parameters for Find Next
    m_lastSearchText = searchText;
    m_lastMatchCase = m_matchCase->isChecked();
    m_lastUseRegex = m_useRegex->isChecked();
    m_lastSearchForward = forward;

    // Perform search
    if (!m_doc || m_doc->m_lineList.isEmpty()) {
        QMessageBox::information(this, "Find", "No text to search.");
        return false;
    }

    Qt::CaseSensitivity cs = m_lastMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;

    // Determine start position
    int startLine = (m_lastFoundLine >= 0) ? m_lastFoundLine : 0;
    int startChar = (m_lastFoundChar >= 0) ? m_lastFoundChar + 1 : 0;

    // If searching backward, adjust start position
    if (!forward && m_lastFoundLine >= 0) {
        startChar = qMax(0, m_lastFoundChar - 1);
    }

    // Search forward
    if (forward) {
        for (int i = startLine; i < m_doc->m_lineList.count(); i++) {
            Line* pLine = m_doc->m_lineList[i];
            if (!pLine || pLine->len() == 0)
                continue;

            QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

            int index = -1;
            if (m_lastUseRegex) {
                QRegularExpression re(searchText, m_lastMatchCase
                                                      ? QRegularExpression::NoPatternOption
                                                      : QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = re.match(lineText, startChar);
                if (match.hasMatch()) {
                    index = match.capturedStart();
                }
            } else {
                index = lineText.indexOf(searchText, startChar, cs);
            }

            if (index != -1) {
                // Found!
                m_lastFoundLine = i;
                m_lastFoundChar = index;

                // Highlight the result in OutputView
                // Get the parent widget (WorldWidget) to access OutputView
                QWidget* parentWidget = qobject_cast<QWidget*>(parent());
                if (parentWidget) {
                    OutputView* outputView = parentWidget->findChild<OutputView*>();
                    if (outputView) {
                        // Select the found text
                        outputView->selectTextAt(i, index, searchText.length());
                    }
                }

                return true;
            }

            startChar = 0; // Reset for next lines
        }
    }
    // Search backward
    else {
        for (int i = startLine; i >= 0; i--) {
            Line* pLine = m_doc->m_lineList[i];
            if (!pLine || pLine->len() == 0)
                continue;

            QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

            int index = -1;
            if (m_lastUseRegex) {
                // For backward regex search, find all matches and take the last one
                QRegularExpression re(searchText, m_lastMatchCase
                                                      ? QRegularExpression::NoPatternOption
                                                      : QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = re.globalMatch(lineText);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    if (i == startLine && match.capturedStart() >= startChar)
                        break;
                    index = match.capturedStart();
                }
            } else {
                int searchEnd = (i == startLine) ? startChar : -1;
                index = lineText.lastIndexOf(searchText, searchEnd, cs);
            }

            if (index != -1) {
                // Found!
                m_lastFoundLine = i;
                m_lastFoundChar = index;

                // Highlight the result in OutputView
                QWidget* parentWidget = qobject_cast<QWidget*>(parent());
                if (parentWidget) {
                    OutputView* outputView = parentWidget->findChild<OutputView*>();
                    if (outputView) {
                        // Select the found text
                        outputView->selectTextAt(i, index, searchText.length());
                    }
                }

                return true;
            }

            startChar = -1; // Search entire line for previous lines
        }
    }

    // Not found
    QMessageBox::information(this, "Find", QString("Cannot find \"%1\"").arg(searchText));
    return false;
}

int FindDialog::countAllMatches()
{
    QString searchText = m_searchText->currentText();
    if (searchText.isEmpty() || !m_doc || m_doc->m_lineList.isEmpty()) {
        return 0;
    }

    Qt::CaseSensitivity cs = m_matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    bool useRegex = m_useRegex->isChecked();
    int totalMatches = 0;

    // Count matches in all lines
    for (int i = 0; i < m_doc->m_lineList.count(); i++) {
        Line* pLine = m_doc->m_lineList[i];
        if (!pLine || pLine->len() == 0)
            continue;

        QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

        if (useRegex) {
            QRegularExpression re(searchText, m_matchCase->isChecked()
                                                  ? QRegularExpression::NoPatternOption
                                                  : QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator it = re.globalMatch(lineText);
            while (it.hasNext()) {
                it.next();
                totalMatches++;
            }
        } else {
            int pos = 0;
            while ((pos = lineText.indexOf(searchText, pos, cs)) != -1) {
                totalMatches++;
                pos++;
            }
        }
    }

    return totalMatches;
}

void FindDialog::updateMatchCounter()
{
    // Count total matches if we haven't already or if search parameters changed
    m_totalMatches = countAllMatches();

    if (m_totalMatches == 0) {
        m_matchCounterLabel->setText("");
        m_currentMatchIndex = 0;
        return;
    }

    // Calculate current match index by counting matches up to current position
    QString searchText = m_searchText->currentText();
    Qt::CaseSensitivity cs = m_matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    bool useRegex = m_useRegex->isChecked();
    int matchesSoFar = 0;

    for (int i = 0; i < m_doc->m_lineList.count(); i++) {
        Line* pLine = m_doc->m_lineList[i];
        if (!pLine || pLine->len() == 0)
            continue;

        QString lineText = QString::fromUtf8(pLine->text(), pLine->len());

        if (useRegex) {
            QRegularExpression re(searchText, m_matchCase->isChecked()
                                                  ? QRegularExpression::NoPatternOption
                                                  : QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator it = re.globalMatch(lineText);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                matchesSoFar++;
                // If this is the current match, we're done
                if (i == m_lastFoundLine && match.capturedStart() == m_lastFoundChar) {
                    m_currentMatchIndex = matchesSoFar;
                    m_matchCounterLabel->setText(
                        QString("Match %1 of %2").arg(m_currentMatchIndex).arg(m_totalMatches));
                    return;
                }
            }
        } else {
            int pos = 0;
            while ((pos = lineText.indexOf(searchText, pos, cs)) != -1) {
                matchesSoFar++;
                // If this is the current match, we're done
                if (i == m_lastFoundLine && pos == m_lastFoundChar) {
                    m_currentMatchIndex = matchesSoFar;
                    m_matchCounterLabel->setText(
                        QString("Match %1 of %2").arg(m_currentMatchIndex).arg(m_totalMatches));
                    return;
                }
                pos++;
            }
        }
    }

    // If we get here, show total matches but no current index
    m_matchCounterLabel->setText(QString("%1 matches found").arg(m_totalMatches));
}
