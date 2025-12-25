#include "confirm_preamble_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

ConfirmPreambleDialog::ConfirmPreambleDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Confirm Paste");
    setupUi();
}

void ConfirmPreambleDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Paste message (read-only information box)
    QLabel* messageLabel = new QLabel("&Paste Message:", this);
    m_pasteMessage = new QTextEdit(this);
    m_pasteMessage->setReadOnly(true);
    m_pasteMessage->setMaximumHeight(60);
    m_pasteMessage->setToolTip("Information about the paste operation");
    messageLabel->setBuddy(m_pasteMessage);
    mainLayout->addWidget(messageLabel);
    mainLayout->addWidget(m_pasteMessage);

    // Form layout for paste controls
    QFormLayout* formLayout = new QFormLayout();

    // Preamble
    m_preamble = new QLineEdit(this);
    m_preamble->setPlaceholderText("Text to prepend before all pasted text");
    m_preamble->setToolTip("Text to prepend before all pasted text");
    formLayout->addRow("&Preamble:", m_preamble);

    // Postamble
    m_postamble = new QLineEdit(this);
    m_postamble->setPlaceholderText("Text to append after all pasted text");
    m_postamble->setToolTip("Text to append after all pasted text");
    formLayout->addRow("P&ostamble:", m_postamble);

    // Line preamble
    m_linePreamble = new QLineEdit(this);
    m_linePreamble->setPlaceholderText("Text to prepend before each line");
    m_linePreamble->setToolTip("Text to prepend before each line");
    formLayout->addRow("&Line preamble:", m_linePreamble);

    // Line postamble
    m_linePostamble = new QLineEdit(this);
    m_linePostamble->setPlaceholderText("Text to append after each line");
    m_linePostamble->setToolTip("Text to append after each line");
    formLayout->addRow("Line p&ostamble:", m_linePostamble);

    // Line delay
    m_lineDelay = new QSpinBox(this);
    m_lineDelay->setRange(0, 10000);
    m_lineDelay->setSuffix(" ms");
    m_lineDelay->setToolTip("Delay between lines in milliseconds (0 = no delay)");
    formLayout->addRow("Line &delay:", m_lineDelay);

    // Line delay per lines
    m_lineDelayPerLines = new QSpinBox(this);
    m_lineDelayPerLines->setRange(1, 1000);
    m_lineDelayPerLines->setSuffix(" lines");
    m_lineDelayPerLines->setToolTip("Apply delay every N lines");
    formLayout->addRow("Delay per &lines:", m_lineDelayPerLines);

    mainLayout->addLayout(formLayout);

    // Checkbox group
    QGroupBox* optionsGroup = new QGroupBox("Paste Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_commentedSoftcode = new QCheckBox("&Commented softcode", this);
    m_commentedSoftcode->setToolTip("Use commented softcode format for pasted text");
    optionsLayout->addWidget(m_commentedSoftcode);

    m_echo = new QCheckBox("&Echo pasted text", this);
    m_echo->setToolTip("Echo pasted text to the output window");
    optionsLayout->addWidget(m_echo);

    mainLayout->addWidget(optionsGroup);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
