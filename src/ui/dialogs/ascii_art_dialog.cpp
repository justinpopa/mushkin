#include "ascii_art_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

AsciiArtDialog::AsciiArtDialog(QWidget* parent)
    : QDialog(parent), m_textEdit(nullptr), m_fontCombo(nullptr), m_previewEdit(nullptr)
{
    setupUi();
}

AsciiArtDialog::~AsciiArtDialog()
{
    // Qt handles cleanup of child widgets
}

void AsciiArtDialog::setupUi()
{
    setWindowTitle("Generate ASCII Art");
    setMinimumSize(500, 400);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for input controls
    QFormLayout* formLayout = new QFormLayout();

    // Text input
    m_textEdit = new QLineEdit();
    m_textEdit->setMaxLength(60);
    m_textEdit->setPlaceholderText("Enter text to convert (max 60 chars)");
    connect(m_textEdit, &QLineEdit::textChanged, this, &AsciiArtDialog::updatePreview);
    formLayout->addRow("Text:", m_textEdit);

    // Font selector
    m_fontCombo = new QComboBox();
    m_fontCombo->addItems({"Standard", "Banner", "Big", "Block", "Bubble", "Digital", "Lean",
                           "Mini", "Script", "Shadow", "Slant", "Small", "Smslant"});
    connect(m_fontCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &AsciiArtDialog::updatePreview);
    formLayout->addRow("Font:", m_fontCombo);

    mainLayout->addLayout(formLayout);

    // Preview section
    QLabel* previewLabel = new QLabel("Preview:");
    mainLayout->addWidget(previewLabel);

    m_previewEdit = new QPlainTextEdit();
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setPlainText("ASCII art preview will appear here");

    // Set monospace font for preview
    QFont monoFont("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(10);
    m_previewEdit->setFont(monoFont);

    mainLayout->addWidget(m_previewEdit, 1); // Stretch factor 1

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &QDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString AsciiArtDialog::text() const
{
    return m_textEdit ? m_textEdit->text() : QString();
}

QString AsciiArtDialog::fontName() const
{
    return m_fontCombo ? m_fontCombo->currentText() : QString();
}

QString AsciiArtDialog::generatedArt() const
{
    // For now, return the preview text
    // In the future, this will contain actual ASCII art generation
    return m_previewEdit ? m_previewEdit->toPlainText() : QString();
}

void AsciiArtDialog::updatePreview()
{
    // Placeholder preview update
    // In the future, this will call actual ASCII art generation logic
    QString inputText = text();
    QString font = fontName();

    if (inputText.isEmpty()) {
        m_previewEdit->setPlainText("ASCII art preview will appear here");
    } else {
        QString preview =
            QString("ASCII art for \"%1\" using %2 font\n\n").arg(inputText).arg(font);
        preview += "[Actual ASCII art generation will be implemented later]";
        m_previewEdit->setPlainText(preview);
    }
}
