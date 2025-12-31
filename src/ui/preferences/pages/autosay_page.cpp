#include "autosay_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

AutoSayPage::AutoSayPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void AutoSayPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Enable checkbox
    m_enableCheck = new QCheckBox(tr("Enable auto-say mode"), this);
    connect(m_enableCheck, &QCheckBox::toggled, this, &AutoSayPage::markChanged);
    mainLayout->addWidget(m_enableCheck);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("When enabled, commands that don't start with the override prefix "
           "will automatically have the say string prepended."),
        this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(helpLabel);

    // Settings group
    QGroupBox* settingsGroup = new QGroupBox(tr("Auto-Say Settings"), this);
    QFormLayout* formLayout = new QFormLayout(settingsGroup);

    m_sayStringEdit = new QLineEdit(settingsGroup);
    m_sayStringEdit->setPlaceholderText(tr("say "));
    connect(m_sayStringEdit, &QLineEdit::textChanged, this, &AutoSayPage::markChanged);
    formLayout->addRow(tr("Say string:"), m_sayStringEdit);

    m_overridePrefixEdit = new QLineEdit(settingsGroup);
    m_overridePrefixEdit->setPlaceholderText(tr("/"));
    m_overridePrefixEdit->setMaxLength(10);
    connect(m_overridePrefixEdit, &QLineEdit::textChanged, this, &AutoSayPage::markChanged);
    formLayout->addRow(tr("Override prefix:"), m_overridePrefixEdit);

    QLabel* prefixHelp = new QLabel(
        tr("Commands starting with this prefix bypass auto-say (e.g., /north sends \"north\")"),
        settingsGroup);
    prefixHelp->setWordWrap(true);
    prefixHelp->setStyleSheet("color: gray; font-size: 11px;");
    formLayout->addRow("", prefixHelp);

    mainLayout->addWidget(settingsGroup);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_excludeMacrosCheck = new QCheckBox(tr("Exclude macro/accelerator keys from auto-say"), optionsGroup);
    connect(m_excludeMacrosCheck, &QCheckBox::toggled, this, &AutoSayPage::markChanged);
    optionsLayout->addWidget(m_excludeMacrosCheck);

    m_excludeNonAlphaCheck = new QCheckBox(tr("Exclude commands not starting with a letter"), optionsGroup);
    connect(m_excludeNonAlphaCheck, &QCheckBox::toggled, this, &AutoSayPage::markChanged);
    optionsLayout->addWidget(m_excludeNonAlphaCheck);

    m_reEvaluateCheck = new QCheckBox(tr("Re-evaluate auto-say after alias expansion"), optionsGroup);
    connect(m_reEvaluateCheck, &QCheckBox::toggled, this, &AutoSayPage::markChanged);
    optionsLayout->addWidget(m_reEvaluateCheck);

    mainLayout->addWidget(optionsGroup);

    mainLayout->addStretch();
}

void AutoSayPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_enableCheck->blockSignals(true);
    m_sayStringEdit->blockSignals(true);
    m_overridePrefixEdit->blockSignals(true);
    m_excludeMacrosCheck->blockSignals(true);
    m_excludeNonAlphaCheck->blockSignals(true);
    m_reEvaluateCheck->blockSignals(true);

    m_enableCheck->setChecked(m_doc->m_bEnableAutoSay != 0);
    m_sayStringEdit->setText(m_doc->m_strAutoSayString);
    m_overridePrefixEdit->setText(m_doc->m_strOverridePrefix);
    m_excludeMacrosCheck->setChecked(m_doc->m_bExcludeMacros != 0);
    m_excludeNonAlphaCheck->setChecked(m_doc->m_bExcludeNonAlpha != 0);
    m_reEvaluateCheck->setChecked(m_doc->m_bReEvaluateAutoSay != 0);

    // Unblock signals
    m_enableCheck->blockSignals(false);
    m_sayStringEdit->blockSignals(false);
    m_overridePrefixEdit->blockSignals(false);
    m_excludeMacrosCheck->blockSignals(false);
    m_excludeNonAlphaCheck->blockSignals(false);
    m_reEvaluateCheck->blockSignals(false);

    m_hasChanges = false;
}

void AutoSayPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->m_bEnableAutoSay = m_enableCheck->isChecked() ? 1 : 0;
    m_doc->m_strAutoSayString = m_sayStringEdit->text();
    m_doc->m_strOverridePrefix = m_overridePrefixEdit->text();
    m_doc->m_bExcludeMacros = m_excludeMacrosCheck->isChecked() ? 1 : 0;
    m_doc->m_bExcludeNonAlpha = m_excludeNonAlphaCheck->isChecked() ? 1 : 0;
    m_doc->m_bReEvaluateAutoSay = m_reEvaluateCheck->isChecked() ? 1 : 0;

    m_doc->setModified(true);
    m_hasChanges = false;
}

bool AutoSayPage::hasChanges() const
{
    return m_hasChanges;
}

void AutoSayPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
