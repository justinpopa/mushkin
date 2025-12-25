#include "immediate_dialog.h"
#include "../../world/script_engine.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QTextCursor>
#include <QVBoxLayout>

// Static member to preserve expression between invocations
QString ImmediateDialog::s_lastExpression;

ImmediateDialog::ImmediateDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_expressionEdit(nullptr), m_executeButton(nullptr),
      m_buttonBox(nullptr)
{
    setupUi();
    setupConnections();

    // Restore the last expression
    if (!s_lastExpression.isEmpty()) {
        m_expressionEdit->setPlainText(s_lastExpression);
        // Move cursor to end (don't select all)
        QTextCursor cursor = m_expressionEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_expressionEdit->setTextCursor(cursor);
    }
}

ImmediateDialog::~ImmediateDialog()
{
}

void ImmediateDialog::setupUi()
{
    setWindowTitle("Immediate");
    resize(600, 400);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Label
    QLabel* label = new QLabel("Enter Lua code to execute immediately:", this);
    mainLayout->addWidget(label);

    // Expression text edit (multiline)
    m_expressionEdit = new QPlainTextEdit(this);
    m_expressionEdit->setPlaceholderText(
        "Enter Lua code here...\n\nExample:\nprint(\"Hello, World!\")\nNote(\"Test message\")");

    // Use a monospace font for code editing
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(10);
    m_expressionEdit->setFont(font);

    // Enable line wrapping
    m_expressionEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    mainLayout->addWidget(m_expressionEdit);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_executeButton = new QPushButton("&Execute", this);
    m_executeButton->setToolTip("Execute the Lua code (Ctrl+Enter)");
    m_executeButton->setDefault(true);
    buttonLayout->addWidget(m_executeButton);

    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // Dialog buttons (Close only)
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mainLayout->addWidget(m_buttonBox);

    // Focus the text edit
    m_expressionEdit->setFocus();
}

void ImmediateDialog::setupConnections()
{
    // Button connections
    connect(m_executeButton, &QPushButton::clicked, this, &ImmediateDialog::executeCode);

    // Dialog buttons
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ImmediateDialog::closeDialog);

    // Ctrl+Enter to execute
    QShortcut* executeShortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(executeShortcut, &QShortcut::activated, this, &ImmediateDialog::executeCode);
}

void ImmediateDialog::executeCode()
{
    if (!m_doc) {
        QMessageBox::warning(this, "Error", "No world document available");
        return;
    }

    if (!m_doc->m_ScriptEngine) {
        QMessageBox::warning(
            this, "Error",
            "Scripting is not enabled for this world.\n\n"
            "To enable scripting, go to World Properties and set the script language to Lua.");
        return;
    }

    QString code = m_expressionEdit->toPlainText();

    if (code.trimmed().isEmpty()) {
        QMessageBox::information(this, "Immediate", "Please enter some Lua code to execute");
        return;
    }

    // Execute the Lua code
    // Based on CImmediateDlg::OnRun() from ImmediateDlg.cpp

    // Parse and execute the code
    // parseLua returns true on error, false on success
    bool error = m_doc->m_ScriptEngine->parseLua(code, "Immediate");

    if (error) {
        // Error message already shown by parseLua
        // The error has been displayed to the user via the script engine
    } else {
        // Success - could show a brief status message if desired
        // Original MFC version updates status bar but doesn't show a dialog
    }
}

void ImmediateDialog::closeDialog()
{
    // Save the expression text for next time
    s_lastExpression = m_expressionEdit->toPlainText();

    // Close the dialog
    reject();
}
