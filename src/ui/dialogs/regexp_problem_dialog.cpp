#include "regexp_problem_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

RegexpProblemDialog::RegexpProblemDialog(const QString& pattern, const QString& errorMessage,
                                         int errorPosition, QWidget* parent)
    : QDialog(parent), m_pattern(pattern), m_errorMessage(errorMessage),
      m_errorPosition(errorPosition)
{
    setWindowTitle("Regular Expression Error");
    setModal(true);
    setMinimumWidth(450);
    setupUi();
}

void RegexpProblemDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create form layout for error details
    QFormLayout* formLayout = new QFormLayout();

    // Pattern field
    m_patternEdit = new QLineEdit(this);
    m_patternEdit->setText(m_pattern);
    m_patternEdit->setReadOnly(true);

    // Highlight error position if available
    if (m_errorPosition >= 0 && m_errorPosition < m_pattern.length()) {
        m_patternEdit->setSelection(m_errorPosition, 1);
    }

    formLayout->addRow("&Pattern:", m_patternEdit);

    // Error message label
    m_errorLabel = new QLabel(m_errorMessage, this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet("QLabel { color: red; }");
    formLayout->addRow("&Error:", m_errorLabel);

    // Position label (only if position is known)
    if (m_errorPosition >= 0) {
        m_positionLabel = new QLabel(QString::number(m_errorPosition), this);
        formLayout->addRow("P&osition:", m_positionLabel);
    }

    mainLayout->addLayout(formLayout);

    // Add spacing
    mainLayout->addSpacing(10);

    // Dialog button (OK only)
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}
