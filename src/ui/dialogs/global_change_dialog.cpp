#include "global_change_dialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

GlobalChangeDialog::GlobalChangeDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Global Find and Replace");
    setMinimumWidth(400);
    setupUi();
}

void GlobalChangeDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for find/replace fields
    QFormLayout* formLayout = new QFormLayout();

    m_findEdit = new QLineEdit(this);
    formLayout->addRow("Find:", m_findEdit);

    m_replaceEdit = new QLineEdit(this);
    formLayout->addRow("Replace with:", m_replaceEdit);

    mainLayout->addLayout(formLayout);

    // Options checkboxes
    m_matchCaseCheckBox = new QCheckBox("Match case", this);
    mainLayout->addWidget(m_matchCaseCheckBox);

    m_matchWholeWordCheckBox = new QCheckBox("Match whole word", this);
    mainLayout->addWidget(m_matchWholeWordCheckBox);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus to the find field
    m_findEdit->setFocus();
}

QString GlobalChangeDialog::findText() const
{
    return m_findEdit->text();
}

QString GlobalChangeDialog::replaceText() const
{
    return m_replaceEdit->text();
}

bool GlobalChangeDialog::matchCase() const
{
    return m_matchCaseCheckBox->isChecked();
}

bool GlobalChangeDialog::matchWholeWord() const
{
    return m_matchWholeWordCheckBox->isChecked();
}
