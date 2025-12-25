#include "script_error_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

ScriptErrorDialog::ScriptErrorDialog(const QString& description, const QString& errorNum,
                                     const QString& event, const QString& calledBy,
                                     const QString& raisedBy, QWidget* parent)
    : QDialog(parent), m_description(description), m_errorNum(errorNum), m_event(event),
      m_calledBy(calledBy), m_raisedBy(raisedBy)
{
    setWindowTitle("Script Error");
    setModal(true);
    resize(500, 300);
    setupUi();
}

void ScriptErrorDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create form layout for error details
    QFormLayout* formLayout = new QFormLayout();

    // Error description
    m_descriptionEdit = new QLineEdit(this);
    m_descriptionEdit->setText(m_description);
    m_descriptionEdit->setReadOnly(true);
    formLayout->addRow("&Description:", m_descriptionEdit);

    // Error number
    m_errorNumEdit = new QLineEdit(this);
    m_errorNumEdit->setText(m_errorNum);
    m_errorNumEdit->setReadOnly(true);
    formLayout->addRow("Error &Number:", m_errorNumEdit);

    // Event name
    m_eventEdit = new QLineEdit(this);
    m_eventEdit->setText(m_event);
    m_eventEdit->setReadOnly(true);
    formLayout->addRow("&Event:", m_eventEdit);

    // Called by (what invoked the script)
    m_calledByEdit = new QLineEdit(this);
    m_calledByEdit->setText(m_calledBy);
    m_calledByEdit->setReadOnly(true);
    formLayout->addRow("&Called By:", m_calledByEdit);

    // Raised by (source)
    m_raisedByEdit = new QLineEdit(this);
    m_raisedByEdit->setText(m_raisedBy);
    m_raisedByEdit->setReadOnly(true);
    formLayout->addRow("&Raised By:", m_raisedByEdit);

    mainLayout->addLayout(formLayout);

    // Add spacing
    mainLayout->addSpacing(10);

    // Copy to output button
    m_copyButton = new QPushButton("Copy to &Output Window", this);
    m_copyButton->setToolTip("Copy error details to the output window");
    connect(m_copyButton, &QPushButton::clicked, this, &ScriptErrorDialog::onCopyToOutput);
    mainLayout->addWidget(m_copyButton);

    // Add spacing
    mainLayout->addSpacing(10);

    // Dialog button (OK only)
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}

void ScriptErrorDialog::onCopyToOutput()
{
    // TODO: Implement copying error details to output window
    // This will need to interact with the main window's output system
    // For now, this is a placeholder that can be implemented when
    // the output window system is available

    // Expected behavior:
    // 1. Format error details as a readable message
    // 2. Send to the output window
    // 3. Optionally provide user feedback that copy was successful
}
