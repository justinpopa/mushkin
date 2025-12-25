#include "generate_id_dialog.h"
#include "../../utils/name_generator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QClipboard>
#include <QGuiApplication>
#include <QTimer>

GenerateIDDialog::GenerateIDDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Generate Unique ID");
    setModal(true);

    // Create layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Add description label
    QLabel *descLabel = new QLabel(
        "Generated 40-character unique identifier.\n"
        "Suitable for use as plugin IDs or other unique identifiers.");
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Add line edit to show the ID (read-only, selectable)
    m_idEdit = new QLineEdit();
    m_idEdit->setReadOnly(true);
    m_idEdit->setMinimumWidth(400);
    // Use monospace font for better readability of hex string
    QFont monoFont("Courier");
    monoFont.setPointSize(10);
    m_idEdit->setFont(monoFont);
    mainLayout->addWidget(m_idEdit);

    // Generate initial ID
    generateAndDisplay();

    // Add buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_regenerateButton = new QPushButton("&Regenerate");
    m_regenerateButton->setToolTip("Generate a new unique ID");
    connect(m_regenerateButton, &QPushButton::clicked, this, &GenerateIDDialog::onRegenerate);
    buttonLayout->addWidget(m_regenerateButton);

    m_copyButton = new QPushButton("&Copy to Clipboard");
    m_copyButton->setToolTip("Copy the ID to clipboard");
    connect(m_copyButton, &QPushButton::clicked, this, &GenerateIDDialog::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    m_closeButton = new QPushButton("&Close");
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    adjustSize();
}

void GenerateIDDialog::generateAndDisplay()
{
    QString id = generateUniqueID();
    m_idEdit->setText(id);
    // Select the text so user can easily copy it
    m_idEdit->selectAll();
}

void GenerateIDDialog::onRegenerate()
{
    generateAndDisplay();
}

void GenerateIDDialog::onCopyToClipboard()
{
    QString id = m_idEdit->text();
    if (!id.isEmpty()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(id);

        // Give user feedback
        m_copyButton->setText("Copied!");
        QTimer::singleShot(1000, this, [this]() {
            m_copyButton->setText("&Copy to Clipboard");
        });
    }
}
