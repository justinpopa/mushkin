#include "quick_connect_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

QuickConnectDialog::QuickConnectDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Quick Connect");
    setModal(true);
    setupUi();
}

void QuickConnectDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create form layout for input fields
    QFormLayout* formLayout = new QFormLayout();

    // World name input
    m_worldNameEdit = new QLineEdit(this);
    m_worldNameEdit->setMaxLength(255);
    m_worldNameEdit->setPlaceholderText("e.g., My Favorite MUD");
    formLayout->addRow("World name:", m_worldNameEdit);

    // Server address input
    m_serverAddressEdit = new QLineEdit(this);
    m_serverAddressEdit->setMaxLength(255);
    m_serverAddressEdit->setPlaceholderText("e.g., mud.example.com");
    formLayout->addRow("Server address:", m_serverAddressEdit);

    // Port input
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(23);
    formLayout->addRow("Port:", m_portSpinBox);

    mainLayout->addLayout(formLayout);

    // Add standard OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QuickConnectDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus to world name field
    m_worldNameEdit->setFocus();

    // Adjust dialog size
    adjustSize();
    setMinimumWidth(400);
}

void QuickConnectDialog::onAccept()
{
    if (validateInput()) {
        accept();
    }
}

bool QuickConnectDialog::validateInput()
{
    // Validate world name
    QString worldName = m_worldNameEdit->text().trimmed();
    if (worldName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a world name.");
        m_worldNameEdit->setFocus();
        return false;
    }

    if (worldName.length() > 255) {
        QMessageBox::warning(this, "Validation Error",
                             "World name must be 255 characters or less.");
        m_worldNameEdit->setFocus();
        return false;
    }

    // Validate server address
    QString serverAddress = m_serverAddressEdit->text().trimmed();
    if (serverAddress.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a server address.");
        m_serverAddressEdit->setFocus();
        return false;
    }

    if (serverAddress.length() > 255) {
        QMessageBox::warning(this, "Validation Error",
                             "Server address must be 255 characters or less.");
        m_serverAddressEdit->setFocus();
        return false;
    }

    // Port validation is handled by QSpinBox range (1-65535)
    return true;
}

QString QuickConnectDialog::worldName() const
{
    return m_worldNameEdit->text().trimmed();
}

QString QuickConnectDialog::serverAddress() const
{
    return m_serverAddressEdit->text().trimmed();
}

int QuickConnectDialog::port() const
{
    return m_portSpinBox->value();
}
