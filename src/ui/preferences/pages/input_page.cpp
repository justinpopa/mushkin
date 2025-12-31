#include "input_page.h"
#include "../../dialogs/command_options_dialog.h"
#include "../../dialogs/tab_defaults_dialog.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

InputPage::InputPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void InputPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Font section
    QGroupBox* fontGroup = new QGroupBox(tr("Font"), this);
    QHBoxLayout* fontLayout = new QHBoxLayout(fontGroup);

    m_inputFontLabel = new QLabel(tr("Courier New, 10pt"), this);
    fontLayout->addWidget(m_inputFontLabel);

    m_inputFontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_inputFontButton, &QPushButton::clicked, this,
            &InputPage::onInputFontButtonClicked);
    fontLayout->addWidget(m_inputFontButton);
    fontLayout->addStretch();

    mainLayout->addWidget(fontGroup);

    // Echo section
    QGroupBox* echoGroup = new QGroupBox(tr("Echo"), this);
    QFormLayout* echoLayout = new QFormLayout(echoGroup);

    m_echoInputCheck = new QCheckBox(tr("Echo my input in output window"), this);
    connect(m_echoInputCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    echoLayout->addRow("", m_echoInputCheck);

    m_echoColorCombo = new QComboBox(this);
    m_echoColorCombo->addItem(tr("Same as output"), 0);
    m_echoColorCombo->addItem(tr("Custom color"), 1);
    connect(m_echoColorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &InputPage::markChanged);
    echoLayout->addRow(tr("Echo color:"), m_echoColorCombo);

    mainLayout->addWidget(echoGroup);

    // History section
    QGroupBox* historyGroup = new QGroupBox(tr("Command History"), this);
    QFormLayout* historyLayout = new QFormLayout(historyGroup);

    m_historySizeSpin = new QSpinBox(this);
    m_historySizeSpin->setRange(20, 5000);
    m_historySizeSpin->setValue(1000);
    m_historySizeSpin->setSuffix(tr(" commands"));
    connect(m_historySizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &InputPage::markChanged);
    historyLayout->addRow(tr("History size:"), m_historySizeSpin);

    m_duplicateHistoryCheck = new QCheckBox(tr("Don't add duplicate commands to history"), this);
    connect(m_duplicateHistoryCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    historyLayout->addRow("", m_duplicateHistoryCheck);

    m_arrowHistoryCheck = new QCheckBox(tr("Use arrow keys to recall history"), this);
    connect(m_arrowHistoryCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    historyLayout->addRow("", m_arrowHistoryCheck);

    mainLayout->addWidget(historyGroup);

    // Behavior section
    QGroupBox* behaviorGroup = new QGroupBox(tr("Input Behavior"), this);
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);

    m_autoRepeatCheck = new QCheckBox(tr("Auto-repeat last command on empty input"), this);
    connect(m_autoRepeatCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    behaviorLayout->addWidget(m_autoRepeatCheck);

    m_escClearCheck = new QCheckBox(tr("Escape key clears input"), this);
    connect(m_escClearCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    behaviorLayout->addWidget(m_escClearCheck);

    m_doubleClickSelectCheck = new QCheckBox(tr("Double-click selects word"), this);
    connect(m_doubleClickSelectCheck, &QCheckBox::toggled, this, &InputPage::markChanged);
    behaviorLayout->addWidget(m_doubleClickSelectCheck);

    mainLayout->addWidget(behaviorGroup);

    // Advanced options section (matches original MUSHclient Page 9)
    QGroupBox* advancedGroup = new QGroupBox(tr("Advanced Options"), this);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);

    QPushButton* commandOptionsButton = new QPushButton(tr("Command Options..."), this);
    commandOptionsButton->setToolTip(
        tr("Configure double-click behavior, arrow keys, and keyboard shortcuts"));
    connect(commandOptionsButton, &QPushButton::clicked, this, &InputPage::onCommandOptionsClicked);
    advancedLayout->addWidget(commandOptionsButton);

    QPushButton* tabDefaultsButton = new QPushButton(tr("Tab Completion Defaults..."), this);
    tabDefaultsButton->setToolTip(tr("Configure default words and settings for tab completion"));
    connect(tabDefaultsButton, &QPushButton::clicked, this, &InputPage::onTabDefaultsClicked);
    advancedLayout->addWidget(tabDefaultsButton);

    mainLayout->addWidget(advancedGroup);

    mainLayout->addStretch();
}

void InputPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_echoInputCheck->blockSignals(true);
    m_echoColorCombo->blockSignals(true);
    m_historySizeSpin->blockSignals(true);
    m_duplicateHistoryCheck->blockSignals(true);
    m_arrowHistoryCheck->blockSignals(true);
    m_autoRepeatCheck->blockSignals(true);
    m_escClearCheck->blockSignals(true);
    m_doubleClickSelectCheck->blockSignals(true);

    // Load font
    m_inputFont.setFamily(m_doc->m_input_font_name);
    m_inputFont.setPointSize(qAbs(m_doc->m_input_font_height));
    m_inputFont.setWeight(m_doc->m_input_font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_inputFont.setItalic(m_doc->m_input_font_italic != 0);
    m_inputFontLabel->setText(
        QString("%1, %2pt").arg(m_inputFont.family()).arg(m_inputFont.pointSize()));

    // Load echo settings
    m_echoInputCheck->setChecked(m_doc->m_display_my_input != 0);
    m_echoColorCombo->setCurrentIndex(0); // Default to same as output

    // Load history settings
    m_historySizeSpin->setValue(m_doc->m_maxCommandHistory);
    m_duplicateHistoryCheck->setChecked(false); // Not stored in WorldDocument
    m_arrowHistoryCheck->setChecked(m_doc->m_bArrowKeysWrap != 0);

    // Load behavior settings
    m_autoRepeatCheck->setChecked(m_doc->m_bAutoRepeat != 0);
    m_escClearCheck->setChecked(m_doc->m_bEscapeDeletesInput != 0);
    m_doubleClickSelectCheck->setChecked(m_doc->m_bDoubleClickInserts != 0);

    // Unblock signals
    m_echoInputCheck->blockSignals(false);
    m_echoColorCombo->blockSignals(false);
    m_historySizeSpin->blockSignals(false);
    m_duplicateHistoryCheck->blockSignals(false);
    m_arrowHistoryCheck->blockSignals(false);
    m_autoRepeatCheck->blockSignals(false);
    m_escClearCheck->blockSignals(false);
    m_doubleClickSelectCheck->blockSignals(false);

    m_hasChanges = false;
}

void InputPage::saveSettings()
{
    if (!m_doc)
        return;

    // Save font settings
    m_doc->m_input_font_name = m_inputFont.family();
    m_doc->m_input_font_height = m_inputFont.pointSize();
    m_doc->m_input_font_weight = m_inputFont.weight();
    m_doc->m_input_font_italic = m_inputFont.italic() ? 1 : 0;

    // Save echo settings
    m_doc->m_display_my_input = m_echoInputCheck->isChecked() ? 1 : 0;
    // m_echoColorCombo and m_duplicateHistoryCheck not stored in WorldDocument

    // Save history settings
    m_doc->m_maxCommandHistory = m_historySizeSpin->value();
    m_doc->m_bArrowKeysWrap = m_arrowHistoryCheck->isChecked() ? 1 : 0;

    // Save behavior settings
    m_doc->m_bAutoRepeat = m_autoRepeatCheck->isChecked() ? 1 : 0;
    m_doc->m_bEscapeDeletesInput = m_escClearCheck->isChecked() ? 1 : 0;
    m_doc->m_bDoubleClickInserts = m_doubleClickSelectCheck->isChecked() ? 1 : 0;

    m_doc->setModified(true);
    emit m_doc->outputSettingsChanged();

    m_hasChanges = false;
}

bool InputPage::hasChanges() const
{
    return m_hasChanges;
}

void InputPage::onInputFontButtonClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_inputFont, this, tr("Choose Input Font"));

    if (ok) {
        m_inputFont = font;
        m_inputFontLabel->setText(
            QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        markChanged();
    }
}

void InputPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}

void InputPage::onCommandOptionsClicked()
{
    if (!m_doc)
        return;

    CommandOptionsDialog dialog(m_doc, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Settings are saved by the dialog
        markChanged();
    }
}

void InputPage::onTabDefaultsClicked()
{
    if (!m_doc)
        return;

    TabDefaultsDialog dialog(m_doc, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Settings are saved by the dialog
        markChanged();
    }
}
