#include "paste_send_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

PasteSendPage::PasteSendPage(WorldDocument* doc, QWidget* parent) : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void PasteSendPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Use a tab widget to separate Paste and Send File settings
    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->addTab(createPasteTab(), tr("Paste to World"));
    tabWidget->addTab(createSendFileTab(), tr("Send File"));

    mainLayout->addWidget(tabWidget);
}

QWidget* PasteSendPage::createPasteTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Info label
    QLabel* infoLabel = new QLabel(
        tr("Configure how text is sent when pasting from the clipboard to the MUD."), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // Preamble/Postamble section
    QGroupBox* wrapGroup = new QGroupBox(tr("Wrapper Text"), this);
    QFormLayout* wrapLayout = new QFormLayout(wrapGroup);
    wrapLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_pastePreambleEdit = new QLineEdit(this);
    m_pastePreambleEdit->setPlaceholderText(tr("Text sent before pasted content"));
    connect(m_pastePreambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Preamble:"), m_pastePreambleEdit);

    m_pastePostambleEdit = new QLineEdit(this);
    m_pastePostambleEdit->setPlaceholderText(tr("Text sent after pasted content"));
    connect(m_pastePostambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Postamble:"), m_pastePostambleEdit);

    m_pasteLinePreambleEdit = new QLineEdit(this);
    m_pasteLinePreambleEdit->setPlaceholderText(tr("Text prepended to each line"));
    connect(m_pasteLinePreambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Line preamble:"), m_pasteLinePreambleEdit);

    m_pasteLinePostambleEdit = new QLineEdit(this);
    m_pasteLinePostambleEdit->setPlaceholderText(tr("Text appended to each line"));
    connect(m_pasteLinePostambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Line postamble:"), m_pasteLinePostambleEdit);

    layout->addWidget(wrapGroup);

    // Timing section
    QGroupBox* timingGroup = new QGroupBox(tr("Timing"), this);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    m_pasteDelaySpin = new QSpinBox(this);
    m_pasteDelaySpin->setRange(0, 10000);
    m_pasteDelaySpin->setSuffix(tr(" ms"));
    m_pasteDelaySpin->setToolTip(tr("Delay between sending lines (0-10000 ms)"));
    connect(m_pasteDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PasteSendPage::markChanged);
    timingLayout->addRow(tr("Line delay:"), m_pasteDelaySpin);

    m_pasteDelayPerLinesSpin = new QSpinBox(this);
    m_pasteDelayPerLinesSpin->setRange(1, 100000);
    m_pasteDelayPerLinesSpin->setValue(1);
    m_pasteDelayPerLinesSpin->setToolTip(tr("Apply delay every N lines"));
    connect(m_pasteDelayPerLinesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PasteSendPage::markChanged);
    timingLayout->addRow(tr("Delay every N lines:"), m_pasteDelayPerLinesSpin);

    layout->addWidget(timingGroup);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_pasteCommentedSoftcodeCheck = new QCheckBox(tr("Commented softcode (strip leading #)"), this);
    m_pasteCommentedSoftcodeCheck->setToolTip(
        tr("Remove leading # from lines for MUD softcode compatibility"));
    connect(m_pasteCommentedSoftcodeCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_pasteCommentedSoftcodeCheck);

    m_pasteEchoCheck = new QCheckBox(tr("Echo pasted lines to output"), this);
    connect(m_pasteEchoCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_pasteEchoCheck);

    m_pasteConfirmCheck = new QCheckBox(tr("Confirm before pasting"), this);
    m_pasteConfirmCheck->setToolTip(tr("Show confirmation dialog before sending pasted text"));
    connect(m_pasteConfirmCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_pasteConfirmCheck);

    layout->addWidget(optionsGroup);

    layout->addStretch();
    return tab;
}

QWidget* PasteSendPage::createSendFileTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Info label
    QLabel* infoLabel =
        new QLabel(tr("Configure how text is sent when sending a file to the MUD."), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // Preamble/Postamble section
    QGroupBox* wrapGroup = new QGroupBox(tr("Wrapper Text"), this);
    QFormLayout* wrapLayout = new QFormLayout(wrapGroup);
    wrapLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_filePreambleEdit = new QLineEdit(this);
    m_filePreambleEdit->setPlaceholderText(tr("Text sent before file content"));
    connect(m_filePreambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Preamble:"), m_filePreambleEdit);

    m_filePostambleEdit = new QLineEdit(this);
    m_filePostambleEdit->setPlaceholderText(tr("Text sent after file content"));
    connect(m_filePostambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Postamble:"), m_filePostambleEdit);

    m_fileLinePreambleEdit = new QLineEdit(this);
    m_fileLinePreambleEdit->setPlaceholderText(tr("Text prepended to each line"));
    connect(m_fileLinePreambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Line preamble:"), m_fileLinePreambleEdit);

    m_fileLinePostambleEdit = new QLineEdit(this);
    m_fileLinePostambleEdit->setPlaceholderText(tr("Text appended to each line"));
    connect(m_fileLinePostambleEdit, &QLineEdit::textChanged, this, &PasteSendPage::markChanged);
    wrapLayout->addRow(tr("Line postamble:"), m_fileLinePostambleEdit);

    layout->addWidget(wrapGroup);

    // Timing section
    QGroupBox* timingGroup = new QGroupBox(tr("Timing"), this);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    m_fileDelaySpin = new QSpinBox(this);
    m_fileDelaySpin->setRange(0, 10000);
    m_fileDelaySpin->setSuffix(tr(" ms"));
    m_fileDelaySpin->setToolTip(tr("Delay between sending lines (0-10000 ms)"));
    connect(m_fileDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PasteSendPage::markChanged);
    timingLayout->addRow(tr("Line delay:"), m_fileDelaySpin);

    m_fileDelayPerLinesSpin = new QSpinBox(this);
    m_fileDelayPerLinesSpin->setRange(1, 100000);
    m_fileDelayPerLinesSpin->setValue(1);
    m_fileDelayPerLinesSpin->setToolTip(tr("Apply delay every N lines"));
    connect(m_fileDelayPerLinesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PasteSendPage::markChanged);
    timingLayout->addRow(tr("Delay every N lines:"), m_fileDelayPerLinesSpin);

    layout->addWidget(timingGroup);

    // Options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_fileCommentedSoftcodeCheck = new QCheckBox(tr("Commented softcode (strip leading #)"), this);
    m_fileCommentedSoftcodeCheck->setToolTip(
        tr("Remove leading # from lines for MUD softcode compatibility"));
    connect(m_fileCommentedSoftcodeCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_fileCommentedSoftcodeCheck);

    m_fileEchoCheck = new QCheckBox(tr("Echo sent lines to output"), this);
    connect(m_fileEchoCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_fileEchoCheck);

    m_fileConfirmCheck = new QCheckBox(tr("Confirm before sending"), this);
    m_fileConfirmCheck->setToolTip(tr("Show confirmation dialog before sending file"));
    connect(m_fileConfirmCheck, &QCheckBox::toggled, this, &PasteSendPage::markChanged);
    optionsLayout->addWidget(m_fileConfirmCheck);

    layout->addWidget(optionsGroup);

    layout->addStretch();
    return tab;
}

void PasteSendPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading - Paste settings
    m_pastePreambleEdit->blockSignals(true);
    m_pastePostambleEdit->blockSignals(true);
    m_pasteLinePreambleEdit->blockSignals(true);
    m_pasteLinePostambleEdit->blockSignals(true);
    m_pasteDelaySpin->blockSignals(true);
    m_pasteDelayPerLinesSpin->blockSignals(true);
    m_pasteCommentedSoftcodeCheck->blockSignals(true);
    m_pasteEchoCheck->blockSignals(true);
    m_pasteConfirmCheck->blockSignals(true);

    // Block signals - File settings
    m_filePreambleEdit->blockSignals(true);
    m_filePostambleEdit->blockSignals(true);
    m_fileLinePreambleEdit->blockSignals(true);
    m_fileLinePostambleEdit->blockSignals(true);
    m_fileDelaySpin->blockSignals(true);
    m_fileDelayPerLinesSpin->blockSignals(true);
    m_fileCommentedSoftcodeCheck->blockSignals(true);
    m_fileEchoCheck->blockSignals(true);
    m_fileConfirmCheck->blockSignals(true);

    // Load Paste to World settings
    m_pastePreambleEdit->setText(m_doc->m_paste.paste_preamble);
    m_pastePostambleEdit->setText(m_doc->m_paste.paste_postamble);
    m_pasteLinePreambleEdit->setText(m_doc->m_paste.pasteline_preamble);
    m_pasteLinePostambleEdit->setText(m_doc->m_paste.pasteline_postamble);
    m_pasteDelaySpin->setValue(m_doc->m_paste.paste_delay);
    m_pasteDelayPerLinesSpin->setValue(m_doc->m_paste.paste_delay_per_lines);
    m_pasteCommentedSoftcodeCheck->setChecked(m_doc->m_paste.paste_commented_softcode);
    m_pasteEchoCheck->setChecked(m_doc->m_paste.paste_echo);
    m_pasteConfirmCheck->setChecked(m_doc->m_paste.confirm_on_paste);

    // Load Send File settings
    m_filePreambleEdit->setText(m_doc->m_paste.file_preamble);
    m_filePostambleEdit->setText(m_doc->m_paste.file_postamble);
    m_fileLinePreambleEdit->setText(m_doc->m_paste.line_preamble);
    m_fileLinePostambleEdit->setText(m_doc->m_paste.line_postamble);
    m_fileDelaySpin->setValue(m_doc->m_paste.file_delay);
    m_fileDelayPerLinesSpin->setValue(m_doc->m_paste.file_delay_per_lines);
    m_fileCommentedSoftcodeCheck->setChecked(m_doc->m_paste.file_commented_softcode);
    m_fileEchoCheck->setChecked(m_doc->m_input.send_echo);
    m_fileConfirmCheck->setChecked(m_doc->m_bConfirmOnSend);

    // Unblock signals - Paste settings
    m_pastePreambleEdit->blockSignals(false);
    m_pastePostambleEdit->blockSignals(false);
    m_pasteLinePreambleEdit->blockSignals(false);
    m_pasteLinePostambleEdit->blockSignals(false);
    m_pasteDelaySpin->blockSignals(false);
    m_pasteDelayPerLinesSpin->blockSignals(false);
    m_pasteCommentedSoftcodeCheck->blockSignals(false);
    m_pasteEchoCheck->blockSignals(false);
    m_pasteConfirmCheck->blockSignals(false);

    // Unblock signals - File settings
    m_filePreambleEdit->blockSignals(false);
    m_filePostambleEdit->blockSignals(false);
    m_fileLinePreambleEdit->blockSignals(false);
    m_fileLinePostambleEdit->blockSignals(false);
    m_fileDelaySpin->blockSignals(false);
    m_fileDelayPerLinesSpin->blockSignals(false);
    m_fileCommentedSoftcodeCheck->blockSignals(false);
    m_fileEchoCheck->blockSignals(false);
    m_fileConfirmCheck->blockSignals(false);

    m_hasChanges = false;
}

void PasteSendPage::saveSettings()
{
    if (!m_doc)
        return;

    // Save Paste to World settings
    m_doc->m_paste.paste_preamble = m_pastePreambleEdit->text();
    m_doc->m_paste.paste_postamble = m_pastePostambleEdit->text();
    m_doc->m_paste.pasteline_preamble = m_pasteLinePreambleEdit->text();
    m_doc->m_paste.pasteline_postamble = m_pasteLinePostambleEdit->text();
    m_doc->m_paste.paste_delay = m_pasteDelaySpin->value();
    m_doc->m_paste.paste_delay_per_lines = m_pasteDelayPerLinesSpin->value();
    m_doc->m_paste.paste_commented_softcode = m_pasteCommentedSoftcodeCheck->isChecked();
    m_doc->m_paste.paste_echo = m_pasteEchoCheck->isChecked();
    m_doc->m_paste.confirm_on_paste = m_pasteConfirmCheck->isChecked();

    // Save Send File settings
    m_doc->m_paste.file_preamble = m_filePreambleEdit->text();
    m_doc->m_paste.file_postamble = m_filePostambleEdit->text();
    m_doc->m_paste.line_preamble = m_fileLinePreambleEdit->text();
    m_doc->m_paste.line_postamble = m_fileLinePostambleEdit->text();
    m_doc->m_paste.file_delay = m_fileDelaySpin->value();
    m_doc->m_paste.file_delay_per_lines = m_fileDelayPerLinesSpin->value();
    m_doc->m_paste.file_commented_softcode = m_fileCommentedSoftcodeCheck->isChecked();
    m_doc->m_input.send_echo = m_fileEchoCheck->isChecked();
    m_doc->m_bConfirmOnSend = m_fileConfirmCheck->isChecked();

    m_doc->setModified(true);

    m_hasChanges = false;
}

bool PasteSendPage::hasChanges() const
{
    return m_hasChanges;
}

void PasteSendPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
