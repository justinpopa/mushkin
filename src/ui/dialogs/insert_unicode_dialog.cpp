#include "insert_unicode_dialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

InsertUnicodeDialog::InsertUnicodeDialog(QWidget* parent) : QDialog(parent), m_currentCodePoint(0)
{
    setWindowTitle("Insert Unicode Character");
    setupUi();
}

void InsertUnicodeDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for code point and checkbox
    QFormLayout* formLayout = new QFormLayout();

    // Code point input
    m_codePointEdit = new QLineEdit(this);
    m_codePointEdit->setPlaceholderText("e.g., 263A");
    m_codePointEdit->setMinimumWidth(200);
    connect(m_codePointEdit, &QLineEdit::textChanged, this, &InsertUnicodeDialog::updatePreview);
    formLayout->addRow("Code point:", m_codePointEdit);

    // Hexadecimal checkbox
    m_hexCheckBox = new QCheckBox("Hexadecimal", this);
    m_hexCheckBox->setChecked(true); // Default to hexadecimal
    connect(m_hexCheckBox, &QCheckBox::toggled, this, &InsertUnicodeDialog::updatePreview);
    formLayout->addRow("", m_hexCheckBox);

    mainLayout->addLayout(formLayout);

    // Preview label
    QFormLayout* previewLayout = new QFormLayout();
    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumHeight(60);
    m_previewLabel->setAlignment(Qt::AlignCenter);

    // Set large font for preview (24pt)
    QFont previewFont = m_previewLabel->font();
    previewFont.setPointSize(24);
    m_previewLabel->setFont(previewFont);

    // Add border to make preview area more visible
    m_previewLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_previewLabel->setLineWidth(2);

    previewLayout->addRow("Preview:", m_previewLabel);
    mainLayout->addLayout(previewLayout);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on code point field
    m_codePointEdit->setFocus();

    // Initial preview update
    updatePreview();
}

void InsertUnicodeDialog::updatePreview()
{
    QString text = m_codePointEdit->text().trimmed();

    if (text.isEmpty()) {
        m_previewLabel->clear();
        m_currentCodePoint = 0;
        return;
    }

    bool ok = false;
    int codePoint = 0;

    if (m_hexCheckBox->isChecked()) {
        // Hexadecimal input
        codePoint = text.toInt(&ok, 16);
    } else {
        // Decimal input
        codePoint = text.toInt(&ok, 10);
    }

    if (ok && codePoint >= 0 && codePoint <= 0x10FFFF) {
        // Valid Unicode code point
        m_currentCodePoint = codePoint;
        char32_t cp = static_cast<char32_t>(codePoint);
        QString unicodeChar = QString::fromUcs4(&cp, 1);
        m_previewLabel->setText(unicodeChar);

        // Clear any error styling
        m_codePointEdit->setStyleSheet("");
    } else {
        // Invalid code point
        m_currentCodePoint = 0;
        m_previewLabel->setText("Invalid");

        // Apply error styling
        m_codePointEdit->setStyleSheet("QLineEdit { background-color: #ffe0e0; }");
    }
}

QString InsertUnicodeDialog::character() const
{
    if (m_currentCodePoint > 0) {
        char32_t cp = static_cast<char32_t>(m_currentCodePoint);
        return QString::fromUcs4(&cp, 1);
    }
    return QString();
}

int InsertUnicodeDialog::codePoint() const
{
    return m_currentCodePoint;
}
