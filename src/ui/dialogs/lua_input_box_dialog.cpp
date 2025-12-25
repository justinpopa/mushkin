#include "lua_input_box_dialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

LuaInputBoxDialog::LuaInputBoxDialog(const QString& title, const QString& message,
                                     const QString& defaultValue, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setupUi(message);

    // Set default value if provided
    if (!defaultValue.isEmpty()) {
        setInputText(defaultValue);
    }

    // Set minimum width
    setMinimumWidth(300);

    // Resize to fit content
    adjustSize();
}

void LuaInputBoxDialog::setupUi(const QString& message)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label (supports multi-line text)
    QLabel* messageLabel = new QLabel(message, this);
    messageLabel->setWordWrap(true);
    mainLayout->addWidget(messageLabel);

    // Input field
    m_inputEdit = new QLineEdit(this);
    mainLayout->addWidget(m_inputEdit);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on input field
    m_inputEdit->setFocus();
}

QString LuaInputBoxDialog::inputText() const
{
    return m_inputEdit->text();
}

void LuaInputBoxDialog::setInputText(const QString& text)
{
    m_inputEdit->setText(text);
    // Keep focus on input field after setting text
    m_inputEdit->setFocus();
}
