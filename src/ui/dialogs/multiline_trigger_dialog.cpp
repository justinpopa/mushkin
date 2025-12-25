#include "multiline_trigger_dialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

MultilineTriggerDialog::MultilineTriggerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Multi-line Trigger");
    setModal(true);
    setMinimumSize(450, 300);

    setupUi();
}

void MultilineTriggerDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Instructions label
    QLabel* instructionLabel =
        new QLabel("Enter the multi-line trigger pattern below.\n"
                   "Each line will be matched against consecutive lines from the server.",
                   this);
    instructionLabel->setWordWrap(true);
    mainLayout->addWidget(instructionLabel);

    // Main text edit area with monospace font
    m_textEdit = new QPlainTextEdit(this);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);
    m_textEdit->setTabStopDistance(m_textEdit->fontMetrics().horizontalAdvance(' ') * 4);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    mainLayout->addWidget(m_textEdit);

    // Match case checkbox
    m_matchCaseCheck = new QCheckBox("Match case", this);
    mainLayout->addWidget(m_matchCaseCheck);

    // Dialog buttons (OK/Cancel)
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString MultilineTriggerDialog::triggerText() const
{
    return m_textEdit->toPlainText();
}

void MultilineTriggerDialog::setTriggerText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

bool MultilineTriggerDialog::matchCase() const
{
    return m_matchCaseCheck->isChecked();
}

void MultilineTriggerDialog::setMatchCase(bool match)
{
    m_matchCaseCheck->setChecked(match);
}
