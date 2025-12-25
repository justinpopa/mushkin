#include "edit_dialog.h"
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

EditDialog::EditDialog(const QString& title, QWidget* parent)
    : QDialog(parent), m_textEdit(nullptr), m_regexButton(nullptr)
{
    setWindowTitle(title);
    setModal(true);
    setMinimumSize(400, 300);

    setupUi();
}

void EditDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Main text edit area with monospace font
    m_textEdit = new QPlainTextEdit(this);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);
    m_textEdit->setTabStopDistance(m_textEdit->fontMetrics().horizontalAdvance(' ') * 4);
    mainLayout->addWidget(m_textEdit);

    // Bottom layout: Regex button + dialog buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    // Regex helper button (initially hidden)
    m_regexButton = new QPushButton("Regex...", this);
    m_regexButton->setToolTip("Insert regular expression special characters");
    m_regexButton->setVisible(false);
    connect(m_regexButton, &QPushButton::clicked, this, &EditDialog::onRegexButtonClicked);
    bottomLayout->addWidget(m_regexButton);

    bottomLayout->addStretch();

    // Dialog buttons (OK/Cancel)
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    bottomLayout->addWidget(buttonBox);

    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
}

QString EditDialog::text() const
{
    return m_textEdit->toPlainText();
}

void EditDialog::setText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

void EditDialog::setRegexMode(bool enabled)
{
    m_regexButton->setVisible(enabled);
}

void EditDialog::onRegexButtonClicked()
{
    // Create popup menu with regex helpers
    QMenu menu(this);

    // Common patterns section
    QAction* anyChar = menu.addAction(". (any character)");
    anyChar->setData(".");

    QAction* zeroOrMore = menu.addAction("* (zero or more)");
    zeroOrMore->setData("*");

    QAction* oneOrMore = menu.addAction("+ (one or more)");
    oneOrMore->setData("+");

    QAction* optional = menu.addAction("? (optional)");
    optional->setData("?");

    menu.addSeparator();

    // Character classes section
    QAction* digit = menu.addAction("\\d (digit)");
    digit->setData("\\d");

    QAction* wordChar = menu.addAction("\\w (word character)");
    wordChar->setData("\\w");

    QAction* whitespace = menu.addAction("\\s (whitespace)");
    whitespace->setData("\\s");

    QAction* notDigit = menu.addAction("\\D (non-digit)");
    notDigit->setData("\\D");

    QAction* notWordChar = menu.addAction("\\W (non-word character)");
    notWordChar->setData("\\W");

    QAction* notWhitespace = menu.addAction("\\S (non-whitespace)");
    notWhitespace->setData("\\S");

    menu.addSeparator();

    // Anchors section
    QAction* startLine = menu.addAction("^ (start of line)");
    startLine->setData("^");

    QAction* endLine = menu.addAction("$ (end of line)");
    endLine->setData("$");

    QAction* wordBoundary = menu.addAction("\\b (word boundary)");
    wordBoundary->setData("\\b");

    menu.addSeparator();

    // Groups section
    QAction* captureGroup = menu.addAction("(...) (capture group)");
    captureGroup->setData("()");

    QAction* nonCaptureGroup = menu.addAction("(?:...) (non-capture group)");
    nonCaptureGroup->setData("(?:)");

    menu.addSeparator();

    // Quantifiers section
    QAction* exactN = menu.addAction("{n} (exactly n times)");
    exactN->setData("{}");

    QAction* atLeastN = menu.addAction("{n,} (n or more times)");
    atLeastN->setData("{,}");

    QAction* rangeNM = menu.addAction("{n,m} (between n and m times)");
    rangeNM->setData("{,}");

    menu.addSeparator();

    // Character sets section
    QAction* charSet = menu.addAction("[...] (character set)");
    charSet->setData("[]");

    QAction* negCharSet = menu.addAction("[^...] (negated character set)");
    negCharSet->setData("[^]");

    menu.addSeparator();

    // Alternation section
    QAction* alternation = menu.addAction("| (alternation/or)");
    alternation->setData("|");

    // Connect all actions to the same slot
    connect(&menu, &QMenu::triggered, this, &EditDialog::onRegexMenuAction);

    // Show menu at button position
    menu.exec(m_regexButton->mapToGlobal(m_regexButton->rect().bottomLeft()));
}

void EditDialog::onRegexMenuAction()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }

    QString pattern = action->data().toString();
    if (pattern.isEmpty()) {
        return;
    }

    // Insert pattern at cursor position
    QTextCursor cursor = m_textEdit->textCursor();

    // For patterns with placeholder positions (like (), [], {}, etc.)
    // position cursor between the delimiters
    int cursorOffset = 0;
    if (pattern == "()") {
        cursorOffset = -1;
    } else if (pattern == "(?:)") {
        cursorOffset = -1;
    } else if (pattern == "[]") {
        cursorOffset = -1;
    } else if (pattern == "[^]") {
        cursorOffset = -1;
    } else if (pattern == "{}") {
        cursorOffset = -1;
    } else if (pattern == "{,}") {
        cursorOffset = -2;
    }

    cursor.insertText(pattern);

    // Position cursor for easy editing
    if (cursorOffset != 0) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, -cursorOffset);
        m_textEdit->setTextCursor(cursor);
    }

    // Return focus to text edit
    m_textEdit->setFocus();
}
