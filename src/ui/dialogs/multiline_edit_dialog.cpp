#include "multiline_edit_dialog.h"
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

MultilineEditDialog::MultilineEditDialog(const QString& title, const QString& initialText,
                                         QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setModal(true);
    setMinimumSize(600, 400);

    setupUi();

    // Set initial text if provided
    if (!initialText.isEmpty()) {
        m_textEdit->setPlainText(initialText);
    }
}

void MultilineEditDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Toolbar with editing buttons
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    m_goToLineButton = new QPushButton("Go to &Line...", this);
    m_goToLineButton->setToolTip("Jump to a specific line number");
    connect(m_goToLineButton, &QPushButton::clicked, this, &MultilineEditDialog::onGoToLine);
    toolbarLayout->addWidget(m_goToLineButton);

    m_completeWordButton = new QPushButton("Complete &Word", this);
    m_completeWordButton->setToolTip("Auto-complete word (not implemented)");
    connect(m_completeWordButton, &QPushButton::clicked, this,
            &MultilineEditDialog::onCompleteWord);
    toolbarLayout->addWidget(m_completeWordButton);

    m_functionListButton = new QPushButton("&Function List", this);
    m_functionListButton->setToolTip("Show list of functions (not implemented)");
    connect(m_functionListButton, &QPushButton::clicked, this,
            &MultilineEditDialog::onFunctionList);
    toolbarLayout->addWidget(m_functionListButton);

    toolbarLayout->addStretch();
    mainLayout->addLayout(toolbarLayout);

    // Main text edit area with monospace font
    m_textEdit = new QPlainTextEdit(this);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);
    m_textEdit->setTabStopDistance(m_textEdit->fontMetrics().horizontalAdvance(' ') * 4);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    mainLayout->addWidget(m_textEdit);

    // Dialog buttons (OK/Cancel)
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString MultilineEditDialog::text() const
{
    return m_textEdit->toPlainText();
}

void MultilineEditDialog::setText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

void MultilineEditDialog::setReadOnly(bool readOnly)
{
    m_textEdit->setReadOnly(readOnly);

    // Disable editing buttons when read-only
    m_completeWordButton->setEnabled(!readOnly);
    m_functionListButton->setEnabled(!readOnly);
}

void MultilineEditDialog::onGoToLine()
{
    // Get total line count
    int totalLines = m_textEdit->document()->blockCount();

    // Get current line number (1-based)
    QTextCursor cursor = m_textEdit->textCursor();
    int currentLine = cursor.blockNumber() + 1;

    // Prompt for line number
    bool ok;
    int lineNumber =
        QInputDialog::getInt(this, "Go to Line", QString("Line number (1-%1):").arg(totalLines),
                             currentLine, // default value
                             1,           // minimum
                             totalLines,  // maximum
                             1,           // step
                             &ok);

    if (ok) {
        // Move cursor to specified line (convert to 0-based)
        QTextCursor newCursor(m_textEdit->document()->findBlockByNumber(lineNumber - 1));
        m_textEdit->setTextCursor(newCursor);
        m_textEdit->setFocus();

        // Ensure the line is visible
        m_textEdit->ensureCursorVisible();
    }
}

void MultilineEditDialog::onCompleteWord()
{
    QMessageBox::information(this, "Complete Word", "Word completion is not yet implemented.");
}

void MultilineEditDialog::onFunctionList()
{
    QMessageBox::information(this, "Function List", "Function list is not yet implemented.");
}
