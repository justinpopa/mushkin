#include "scripting_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ScriptingPage::ScriptingPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void ScriptingPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Enable scripting
    m_enableScriptCheck = new QCheckBox(tr("Enable scripting"), this);
    connect(m_enableScriptCheck, &QCheckBox::toggled, this, &ScriptingPage::markChanged);
    mainLayout->addWidget(m_enableScriptCheck);

    // Script file section
    QGroupBox* fileGroup = new QGroupBox(tr("Script File"), this);
    QFormLayout* fileLayout = new QFormLayout(fileGroup);
    fileLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // File path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_scriptFileEdit = new QLineEdit(this);
    m_scriptFileEdit->setPlaceholderText(tr("Path to script file"));
    connect(m_scriptFileEdit, &QLineEdit::textChanged, this, &ScriptingPage::markChanged);
    pathLayout->addWidget(m_scriptFileEdit);

    m_browseButton = new QPushButton(tr("Browse..."), this);
    connect(m_browseButton, &QPushButton::clicked, this, &ScriptingPage::onBrowseClicked);
    pathLayout->addWidget(m_browseButton);

    fileLayout->addRow(tr("Script file:"), pathLayout);

    // Language
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(tr("Lua"), "lua");
    m_languageCombo->addItem(tr("YueScript"), "yue");
    m_languageCombo->addItem(tr("MoonScript"), "moon");
    m_languageCombo->addItem(tr("Teal"), "tl");
    m_languageCombo->addItem(tr("Fennel"), "fnl");
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ScriptingPage::markChanged);
    fileLayout->addRow(tr("Language:"), m_languageCombo);

    mainLayout->addWidget(fileGroup);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_autoReloadCheck = new QCheckBox(tr("Automatically reload script when file changes"), this);
    connect(m_autoReloadCheck, &QCheckBox::toggled, this, &ScriptingPage::markChanged);
    optionsLayout->addWidget(m_autoReloadCheck);

    m_warnIfNoHandlerCheck = new QCheckBox(tr("Warn if script function not found"), this);
    connect(m_warnIfNoHandlerCheck, &QCheckBox::toggled, this, &ScriptingPage::markChanged);
    optionsLayout->addWidget(m_warnIfNoHandlerCheck);

    mainLayout->addWidget(optionsGroup);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("The script file is loaded when the world opens. It can define functions "
           "called by triggers, aliases, and timers."),
        this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();
}

void ScriptingPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_enableScriptCheck->blockSignals(true);
    m_scriptFileEdit->blockSignals(true);
    m_languageCombo->blockSignals(true);
    m_autoReloadCheck->blockSignals(true);
    m_warnIfNoHandlerCheck->blockSignals(true);

    m_enableScriptCheck->setChecked(m_doc->m_bEnableScripts != 0);
    m_scriptFileEdit->setText(m_doc->m_strScriptFilename);

    // Determine language from file extension
    QString ext = QFileInfo(m_doc->m_strScriptFilename).suffix().toLower();
    int langIndex = 0; // Default to Lua
    if (ext == "yue")
        langIndex = m_languageCombo->findData("yue");
    else if (ext == "moon")
        langIndex = m_languageCombo->findData("moon");
    else if (ext == "tl")
        langIndex = m_languageCombo->findData("tl");
    else if (ext == "fnl")
        langIndex = m_languageCombo->findData("fnl");
    if (langIndex >= 0)
        m_languageCombo->setCurrentIndex(langIndex);

    // m_nReloadOption: 0=Never, 1=OnFileChange, 2=OnConnect
    m_autoReloadCheck->setChecked(m_doc->m_nReloadOption != 0);
    m_warnIfNoHandlerCheck->setChecked(m_doc->m_bWarnIfScriptingInactive != 0);

    // Unblock signals
    m_enableScriptCheck->blockSignals(false);
    m_scriptFileEdit->blockSignals(false);
    m_languageCombo->blockSignals(false);
    m_autoReloadCheck->blockSignals(false);
    m_warnIfNoHandlerCheck->blockSignals(false);

    m_hasChanges = false;
}

void ScriptingPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->m_bEnableScripts = m_enableScriptCheck->isChecked() ? 1 : 0;
    m_doc->m_strScriptFilename = m_scriptFileEdit->text();
    // Script language is determined by file extension, not stored separately
    m_doc->m_nReloadOption = m_autoReloadCheck->isChecked() ? 1 : 0; // 1 = OnFileChange
    m_doc->m_bWarnIfScriptingInactive = m_warnIfNoHandlerCheck->isChecked() ? 1 : 0;

    m_doc->setModified(true);

    // Reconfigure script file watcher
    m_doc->setupScriptFileWatcher();

    m_hasChanges = false;
}

bool ScriptingPage::hasChanges() const
{
    return m_hasChanges;
}

void ScriptingPage::onBrowseClicked()
{
    QString filter;
    QString langId = m_languageCombo->currentData().toString();

    if (langId == "lua") {
        filter = tr("Lua Scripts (*.lua);;All Files (*)");
    } else if (langId == "yue") {
        filter = tr("YueScript Files (*.yue);;All Files (*)");
    } else if (langId == "moon") {
        filter = tr("MoonScript Files (*.moon);;All Files (*)");
    } else if (langId == "tl") {
        filter = tr("Teal Files (*.tl);;All Files (*)");
    } else if (langId == "fnl") {
        filter = tr("Fennel Files (*.fnl);;All Files (*)");
    } else {
        filter = tr("Script Files (*.lua *.yue *.moon *.tl *.fnl);;All Files (*)");
    }

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Choose Script File"), m_scriptFileEdit->text(), filter);

    if (!fileName.isEmpty()) {
        m_scriptFileEdit->setText(fileName);
        markChanged();
    }
}

void ScriptingPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
