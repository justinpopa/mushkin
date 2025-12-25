#include "generate_name_dialog.h"
#include "../../utils/name_generator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QTimer>

GenerateNameDialog::GenerateNameDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Generate Character Name");
    setModal(true);

    // Create layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Add label
    QLabel *label = new QLabel("Generated Character Name:");
    mainLayout->addWidget(label);

    // Add line edit to show the name (read-only, selectable)
    m_nameEdit = new QLineEdit();
    m_nameEdit->setReadOnly(true);
    m_nameEdit->setMinimumWidth(300);
    mainLayout->addWidget(m_nameEdit);

    // Generate initial name
    generateAndDisplay();

    // Add buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_regenerateButton = new QPushButton("&Regenerate");
    m_regenerateButton->setToolTip("Generate a new random name");
    connect(m_regenerateButton, &QPushButton::clicked, this, &GenerateNameDialog::onRegenerate);
    buttonLayout->addWidget(m_regenerateButton);

    m_copyButton = new QPushButton("&Copy to Clipboard");
    m_copyButton->setToolTip("Copy the name to clipboard");
    connect(m_copyButton, &QPushButton::clicked, this, &GenerateNameDialog::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    m_closeButton = new QPushButton("&Close");
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    adjustSize();
}

void GenerateNameDialog::generateAndDisplay()
{
    QString name = generateCharacterName();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Error",
            "Failed to generate name. Names resource file may be missing or corrupted.");
        m_nameEdit->setText("(error)");
    } else {
        m_nameEdit->setText(name);
        // Select the text so user can easily copy it
        m_nameEdit->selectAll();
    }
}

void GenerateNameDialog::onRegenerate()
{
    generateAndDisplay();
}

void GenerateNameDialog::onCopyToClipboard()
{
    QString name = m_nameEdit->text();
    if (!name.isEmpty() && name != "(error)") {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(name);

        // Give user feedback
        m_copyButton->setText("Copied!");
        QTimer::singleShot(1000, this, [this]() {
            m_copyButton->setText("&Copy to Clipboard");
        });
    }
}
