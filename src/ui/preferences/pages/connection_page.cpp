#include "connection_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

ConnectionPage::ConnectionPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void ConnectionPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // Server
    m_serverEdit = new QLineEdit(this);
    m_serverEdit->setPlaceholderText(tr("e.g., aardmud.org"));
    formLayout->addRow(tr("Server:"), m_serverEdit);
    connect(m_serverEdit, &QLineEdit::textChanged, this, [this]() {
        m_hasChanges = true;
        emit settingsChanged();
    });

    // Port
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4000);
    formLayout->addRow(tr("Port:"), m_portSpin);
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        m_hasChanges = true;
        emit settingsChanged();
    });

    // Character name
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Your character name"));
    formLayout->addRow(tr("Character name:"), m_nameEdit);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() {
        m_hasChanges = true;
        emit settingsChanged();
    });

    // Password
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Optional"));
    formLayout->addRow(tr("Password:"), m_passwordEdit);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, [this]() {
        m_hasChanges = true;
        emit settingsChanged();
    });

    // Auto-connect
    m_autoConnectCheck = new QCheckBox(tr("Connect automatically on startup"), this);
    formLayout->addRow("", m_autoConnectCheck);
    connect(m_autoConnectCheck, &QCheckBox::toggled, this, [this]() {
        m_hasChanges = true;
        emit settingsChanged();
    });

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
}

void ConnectionPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading to avoid triggering hasChanges
    m_serverEdit->blockSignals(true);
    m_portSpin->blockSignals(true);
    m_nameEdit->blockSignals(true);
    m_passwordEdit->blockSignals(true);
    m_autoConnectCheck->blockSignals(true);

    m_serverEdit->setText(m_doc->m_server);
    m_portSpin->setValue(m_doc->m_port);
    m_nameEdit->setText(m_doc->m_mush_name);
    m_passwordEdit->setText(m_doc->m_password);
    m_autoConnectCheck->setChecked(m_doc->m_connect_now != 0);

    m_serverEdit->blockSignals(false);
    m_portSpin->blockSignals(false);
    m_nameEdit->blockSignals(false);
    m_passwordEdit->blockSignals(false);
    m_autoConnectCheck->blockSignals(false);

    m_hasChanges = false;
}

void ConnectionPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->m_server = m_serverEdit->text();
    m_doc->m_port = m_portSpin->value();
    m_doc->m_mush_name = m_nameEdit->text();
    m_doc->m_password = m_passwordEdit->text();
    m_doc->m_connect_now = m_autoConnectCheck->isChecked() ? 1 : 0;

    m_hasChanges = false;
}

bool ConnectionPage::hasChanges() const
{
    return m_hasChanges;
}
