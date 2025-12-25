#include "recall_dialog.h"
#include <QDialogButtonBox>
#include <QFont>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

RecallDialog::RecallDialog(const QString& title, const QColor& backgroundColor, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setMinimumSize(400, 300);

    setupUi();

    // Set initial background color
    setColors(Qt::black, backgroundColor);
}

void RecallDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Main text edit area
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    mainLayout->addWidget(m_textEdit);

    // Close button only (no OK/Cancel)
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString RecallDialog::text() const
{
    return m_textEdit->toPlainText();
}

void RecallDialog::setText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

void RecallDialog::setReadOnly(bool readOnly)
{
    m_textEdit->setReadOnly(readOnly);
}

void RecallDialog::setFont(const QString& fontName, int size, int weight)
{
    QFont font(fontName, size);
    font.setWeight(static_cast<QFont::Weight>(weight));
    m_textEdit->setFont(font);
}

void RecallDialog::setColors(const QColor& textColor, const QColor& backgroundColor)
{
    QPalette palette = m_textEdit->palette();
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Base, backgroundColor);
    m_textEdit->setPalette(palette);
}

void RecallDialog::setFilename(const QString& filename)
{
    m_filename = filename;
}
