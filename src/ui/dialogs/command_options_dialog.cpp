#include "command_options_dialog.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

CommandOptionsDialog::CommandOptionsDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Command Options");
    setMinimumSize(450, 400);
    setupUi();
    loadSettings();
}

void CommandOptionsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Double-click behavior group
    QGroupBox* doubleClickGroup = new QGroupBox("Double-click behavior", this);
    QVBoxLayout* doubleClickLayout = new QVBoxLayout(doubleClickGroup);

    m_doubleClickInserts = new QCheckBox("Double-click &inserts word", this);
    m_doubleClickInserts->setToolTip(
        "Double-clicking a word in the output window inserts it into the command line");
    doubleClickLayout->addWidget(m_doubleClickInserts);

    m_doubleClickSends = new QCheckBox("Double-click &sends command", this);
    m_doubleClickSends->setToolTip("Double-clicking a word sends it as a command to the MUD");
    doubleClickLayout->addWidget(m_doubleClickSends);

    mainLayout->addWidget(doubleClickGroup);

    // Arrow key behavior group
    QGroupBox* arrowKeyGroup = new QGroupBox("Arrow key behavior", this);
    QVBoxLayout* arrowKeyLayout = new QVBoxLayout(arrowKeyGroup);

    m_arrowKeysWrap = new QCheckBox("Arrow keys &wrap in history", this);
    m_arrowKeysWrap->setToolTip("Arrow keys wrap from top to bottom of command history");
    arrowKeyLayout->addWidget(m_arrowKeysWrap);

    m_arrowsChangeHistory = new QCheckBox("Up/down changes &history", this);
    m_arrowsChangeHistory->setToolTip("Up and down arrow keys navigate through command history");
    arrowKeyLayout->addWidget(m_arrowsChangeHistory);

    m_arrowRecallsPartial = new QCheckBox("Arrow recalls &partial match", this);
    m_arrowRecallsPartial->setToolTip(
        "Arrow keys recall commands that start with the current input");
    arrowKeyLayout->addWidget(m_arrowRecallsPartial);

    m_altArrowRecallsPartial = new QCheckBox("&Alt+Arrow recalls partial", this);
    m_altArrowRecallsPartial->setToolTip(
        "Alt+Arrow keys recall commands that start with the current input");
    arrowKeyLayout->addWidget(m_altArrowRecallsPartial);

    mainLayout->addWidget(arrowKeyGroup);

    // Input options group
    QGroupBox* inputOptionsGroup = new QGroupBox("Input options", this);
    QVBoxLayout* inputOptionsLayout = new QVBoxLayout(inputOptionsGroup);

    m_escapeDeletesInput = new QCheckBox("&Escape clears input", this);
    m_escapeDeletesInput->setToolTip("Pressing Escape clears the command input line");
    inputOptionsLayout->addWidget(m_escapeDeletesInput);

    m_saveDeletedCommand = new QCheckBox("Save &deleted commands", this);
    m_saveDeletedCommand->setToolTip(
        "Save commands that are deleted with Escape to command history");
    inputOptionsLayout->addWidget(m_saveDeletedCommand);

    m_confirmBeforeReplacingTyping = new QCheckBox("&Confirm before replacing typed text", this);
    m_confirmBeforeReplacingTyping->setToolTip(
        "Ask for confirmation before replacing text you've typed");
    inputOptionsLayout->addWidget(m_confirmBeforeReplacingTyping);

    mainLayout->addWidget(inputOptionsGroup);

    // Keyboard shortcuts group
    QGroupBox* shortcutsGroup = new QGroupBox("Keyboard shortcuts", this);
    QVBoxLayout* shortcutsLayout = new QVBoxLayout(shortcutsGroup);

    m_ctrlZGoesToEndOfBuffer = new QCheckBox("Ctrl+&Z goes to end of buffer", this);
    m_ctrlZGoesToEndOfBuffer->setToolTip("Ctrl+Z scrolls the output window to the bottom");
    shortcutsLayout->addWidget(m_ctrlZGoesToEndOfBuffer);

    m_ctrlPGoesToPreviousCommand = new QCheckBox("Ctrl+&P previous command", this);
    m_ctrlPGoesToPreviousCommand->setToolTip("Ctrl+P recalls the previous command from history");
    shortcutsLayout->addWidget(m_ctrlPGoesToPreviousCommand);

    m_ctrlNGoesToNextCommand = new QCheckBox("Ctrl+&N next command", this);
    m_ctrlNGoesToNextCommand->setToolTip("Ctrl+N recalls the next command from history");
    shortcutsLayout->addWidget(m_ctrlNGoesToNextCommand);

    mainLayout->addWidget(shortcutsGroup);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &CommandOptionsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CommandOptionsDialog::onRejected);
}

void CommandOptionsDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Double-click behavior
    m_doubleClickInserts->setChecked(m_doc->m_display.double_click_inserts);
    m_doubleClickSends->setChecked(m_doc->m_input.double_click_sends);

    // Arrow key behavior
    m_arrowKeysWrap->setChecked(m_doc->m_command_window.arrow_keys_wrap);
    m_arrowsChangeHistory->setChecked(m_doc->m_input.arrows_change_history);
    m_arrowRecallsPartial->setChecked(m_doc->m_input.arrow_recalls_partial);
    m_altArrowRecallsPartial->setChecked(m_doc->m_input.alt_arrow_recalls_partial);

    // Input options
    m_escapeDeletesInput->setChecked(m_doc->m_input.escape_deletes_input);
    m_saveDeletedCommand->setChecked(m_doc->m_input.save_deleted_command);
    m_confirmBeforeReplacingTyping->setChecked(m_doc->m_auto_say.confirm_before_replacing);

    // Keyboard shortcuts
    m_ctrlZGoesToEndOfBuffer->setChecked(m_doc->m_bCtrlZGoesToEndOfBuffer);
    m_ctrlPGoesToPreviousCommand->setChecked(m_doc->m_bCtrlPGoesToPreviousCommand);
    m_ctrlNGoesToNextCommand->setChecked(m_doc->m_bCtrlNGoesToNextCommand);
}

void CommandOptionsDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Double-click behavior
    m_doc->m_display.double_click_inserts = m_doubleClickInserts->isChecked();
    m_doc->m_input.double_click_sends = m_doubleClickSends->isChecked();

    // Arrow key behavior
    m_doc->m_command_window.arrow_keys_wrap = m_arrowKeysWrap->isChecked();
    m_doc->m_input.arrows_change_history = m_arrowsChangeHistory->isChecked();
    m_doc->m_input.arrow_recalls_partial = m_arrowRecallsPartial->isChecked();
    m_doc->m_input.alt_arrow_recalls_partial = m_altArrowRecallsPartial->isChecked();

    // Input options
    m_doc->m_input.escape_deletes_input = m_escapeDeletesInput->isChecked();
    m_doc->m_input.save_deleted_command = m_saveDeletedCommand->isChecked();
    m_doc->m_auto_say.confirm_before_replacing = m_confirmBeforeReplacingTyping->isChecked();

    // Keyboard shortcuts
    m_doc->m_bCtrlZGoesToEndOfBuffer = m_ctrlZGoesToEndOfBuffer->isChecked();
    m_doc->m_bCtrlPGoesToPreviousCommand = m_ctrlPGoesToPreviousCommand->isChecked();
    m_doc->m_bCtrlNGoesToNextCommand = m_ctrlNGoesToNextCommand->isChecked();

    // Pack the flags back into m_iFlags1
    m_doc->packFlags();

    // Mark document as modified
    m_doc->setModified(true);
}

void CommandOptionsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void CommandOptionsDialog::onRejected()
{
    reject();
}
