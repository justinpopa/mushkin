#include "password_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

PasswordDialog::PasswordDialog(const QString& title, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(title);
    setupUi();
}

void PasswordDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for character and password fields
    QFormLayout* formLayout = new QFormLayout();

    m_characterEdit = new QLineEdit(this);
    formLayout->addRow("Character:", m_characterEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Password:", m_passwordEdit);

    mainLayout->addLayout(formLayout);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on password field
    m_passwordEdit->setFocus();
}

QString PasswordDialog::character() const
{
    return m_characterEdit->text();
}

QString PasswordDialog::password() const
{
    return m_passwordEdit->text();
}

void PasswordDialog::setCharacter(const QString& name)
{
    m_characterEdit->setText(name);
    // Keep focus on password field after setting character name
    m_passwordEdit->setFocus();
}
