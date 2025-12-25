#include "lua_input_edit_dialog.h"
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

LuaInputEditDialog::LuaInputEditDialog(const QString& title, const QString& message,
                                       QWidget* parent)
    : QDialog(parent), m_messageLabel(nullptr), m_replyEdit(nullptr), m_okButton(nullptr),
      m_cancelButton(nullptr), m_maxLength(0), m_noDefault(false)
{
    setWindowTitle(title);
    setupUi(message);

    // Set minimum width and make dialog resizable
    setMinimumWidth(400);
    setMinimumHeight(200);

    // Resize to fit content
    adjustSize();
}

void LuaInputEditDialog::setupUi(const QString& message)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label (supports multi-line text)
    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setWordWrap(true);
    mainLayout->addWidget(m_messageLabel);

    // Multi-line text edit for reply
    m_replyEdit = new QTextEdit(this);
    m_replyEdit->setAcceptRichText(false); // Plain text only
    mainLayout->addWidget(m_replyEdit);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on text edit
    m_replyEdit->setFocus();
}

QString LuaInputEditDialog::replyText() const
{
    return m_replyEdit->toPlainText();
}

void LuaInputEditDialog::setFont(const QString& name, int size)
{
    if (!name.isEmpty() && size > 0) {
        QFont font(name, size);
        m_replyEdit->setFont(font);
    }
}

void LuaInputEditDialog::setDialogSize(int width, int height)
{
    if (width > 0 && height > 0) {
        resize(width, height);
    }
}

void LuaInputEditDialog::setMaxLength(int max)
{
    m_maxLength = max;

    if (max > 0) {
        // Connect to textChanged signal to enforce max length
        connect(m_replyEdit, &QTextEdit::textChanged, this, [this]() {
            if (m_maxLength > 0) {
                QString text = m_replyEdit->toPlainText();
                if (text.length() > m_maxLength) {
                    // Save cursor position
                    int position = m_replyEdit->textCursor().position();

                    // Truncate text
                    text = text.left(m_maxLength);
                    m_replyEdit->setPlainText(text);

                    // Restore cursor position (adjusted if needed)
                    // Need fresh cursor after setPlainText creates new document
                    position = qMin(position, text.length());
                    QTextCursor newCursor = m_replyEdit->textCursor();
                    newCursor.setPosition(position);
                    m_replyEdit->setTextCursor(newCursor);
                }
            }
        });
    }
}

void LuaInputEditDialog::setReadOnly(bool readOnly)
{
    m_replyEdit->setReadOnly(readOnly);
}

void LuaInputEditDialog::setButtonLabels(const QString& ok, const QString& cancel)
{
    if (!ok.isEmpty()) {
        m_okButton->setText(ok);
    }
    if (!cancel.isEmpty()) {
        m_cancelButton->setText(cancel);
    }
}

void LuaInputEditDialog::setDefaultText(const QString& text)
{
    m_replyEdit->setPlainText(text);
    // Keep focus on text edit after setting text
    m_replyEdit->setFocus();
}

void LuaInputEditDialog::setNoDefault(bool noDefault)
{
    m_noDefault = noDefault;

    if (noDefault) {
        // Remove default button behavior
        m_okButton->setDefault(false);
        m_okButton->setAutoDefault(false);
        m_cancelButton->setDefault(false);
        m_cancelButton->setAutoDefault(false);
    } else {
        // Set OK as default button
        m_okButton->setDefault(true);
        m_okButton->setAutoDefault(true);
    }
}
