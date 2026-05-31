#include "recall_dialog.h"
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QMessageBox>
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

    // Save, Save As, and Close buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton(tr("Save"), this);
    QPushButton* saveAsBtn = new QPushButton(tr("Save As..."), this);
    QPushButton* closeBtn = new QPushButton(tr("Close"), this);
    connect(saveBtn, &QPushButton::clicked, this, &RecallDialog::onSave);
    connect(saveAsBtn, &QPushButton::clicked, this, &RecallDialog::onSaveAs);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(saveAsBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

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

void RecallDialog::onSave()
{
    if (m_filename.isEmpty()) {
        onSaveAs();
        return;
    }
    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"),
                             tr("Could not open file for writing:\n%1").arg(m_filename));
        return;
    }
    file.write(m_textEdit->toPlainText().toUtf8());
}

void RecallDialog::onSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Recalled Text"), m_filename,
                                                tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    m_filename = path;
    onSave();
}
