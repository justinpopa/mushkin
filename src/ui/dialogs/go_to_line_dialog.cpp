#include "go_to_line_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

GoToLineDialog::GoToLineDialog(int maxLine, int currentLine, QWidget* parent)
    : QDialog(parent), m_maxLine(maxLine), m_currentLine(currentLine)
{
    setWindowTitle("Go To Line");
    setupUi();
}

void GoToLineDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Line number input
    QFormLayout* formLayout = new QFormLayout();
    m_lineNumberSpinBox = new QSpinBox(this);
    m_lineNumberSpinBox->setRange(1, m_maxLine);
    m_lineNumberSpinBox->setValue(m_currentLine);
    m_lineNumberSpinBox->setMinimumWidth(150);
    formLayout->addRow("Line number:", m_lineNumberSpinBox);
    mainLayout->addLayout(formLayout);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus to the spinbox
    m_lineNumberSpinBox->setFocus();
    m_lineNumberSpinBox->selectAll();
}

int GoToLineDialog::lineNumber() const
{
    return m_lineNumberSpinBox->value();
}
