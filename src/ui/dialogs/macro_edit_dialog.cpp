#include "macro_edit_dialog.h"
#include "../../world/world_document.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// Macro type constants (match original MUSHclient)
enum MacroType { REPLACE_COMMAND = 0, SEND_NOW = 1, ADD_TO_COMMAND = 2 };

// Macro key descriptions (must match original order)
static const char* MACRO_DESCRIPTIONS[] = {
    "up",        "down",     "north",    "south",    "east",     "west",      "examine",
    "look",      "page",     "say",      "whisper",  "doing",    "who",       "drop",
    "take",      "F2",       "F3",       "F4",       "F5",       "F7",        "F8",
    "F9",        "F10",      "F11",      "F12",      "F2+Shift", "F3+Shift",  "F4+Shift",
    "F5+Shift",  "F6+Shift", "F7+Shift", "F8+Shift", "F9+Shift", "F10+Shift", "F11+Shift",
    "F12+Shift", "F2+Ctrl",  "F3+Ctrl",  "F5+Ctrl",  "F7+Ctrl",  "F8+Ctrl",   "F9+Ctrl",
    "F10+Ctrl",  "F11+Ctrl", "F12+Ctrl", "logout",   "quit",     "Alt+A",     "Alt+B",
    "Alt+J",     "Alt+K",    "Alt+L",    "Alt+M",    "Alt+N",    "Alt+O",     "Alt+P",
    "Alt+Q",     "Alt+R",    "Alt+S",    "Alt+T",    "Alt+U",    "Alt+X",     "Alt+Y",
    "Alt+Z",     "F1",       "F1+Ctrl",  "F1+Shift", "F6",       "F6+Ctrl"};

constexpr int TOTAL_MACRO_KEYS = sizeof(MACRO_DESCRIPTIONS) / sizeof(MACRO_DESCRIPTIONS[0]);

MacroEditDialog::MacroEditDialog(WorldDocument* doc, int macroIndex, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_macroIndex(macroIndex)
{
    // Validate macro index
    if (macroIndex < 0 || macroIndex >= TOTAL_MACRO_KEYS) {
        m_macroDescription = "Unknown";
    } else {
        m_macroDescription = QString::fromUtf8(MACRO_DESCRIPTIONS[macroIndex]);
    }

    setWindowTitle("Edit macro " + m_macroDescription + " - " + doc->m_mush_name);
    resize(500, 350);
    setupUi();
    loadMacroData();
}

void MacroEditDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ========================================================================
    // MACRO INFO SECTION
    // ========================================================================
    QFormLayout* infoForm = new QFormLayout();

    // Macro description (read-only)
    m_descriptionEdit = new QLineEdit(this);
    m_descriptionEdit->setText(m_macroDescription);
    m_descriptionEdit->setReadOnly(true);
    m_descriptionEdit->setEnabled(false);
    infoForm->addRow("&Key:", m_descriptionEdit);

    mainLayout->addLayout(infoForm);

    // ========================================================================
    // SEND TYPE GROUP BOX
    // ========================================================================
    QGroupBox* sendTypeBox = new QGroupBox("Send &Type", this);
    QVBoxLayout* sendTypeLayout = new QVBoxLayout(sendTypeBox);

    m_sendTypeGroup = new QButtonGroup(this);

    m_replaceRadio = new QRadioButton("&Replace current command with macro text", sendTypeBox);
    m_replaceRadio->setToolTip("Replace the entire command line with the macro text");
    m_sendTypeGroup->addButton(m_replaceRadio, REPLACE_COMMAND);
    sendTypeLayout->addWidget(m_replaceRadio);

    m_sendNowRadio = new QRadioButton("Send macro text &now", sendTypeBox);
    m_sendNowRadio->setToolTip("Send the macro text immediately to the MUD");
    m_sendTypeGroup->addButton(m_sendNowRadio, SEND_NOW);
    sendTypeLayout->addWidget(m_sendNowRadio);

    m_insertRadio = new QRadioButton("&Insert macro text into current command", sendTypeBox);
    m_insertRadio->setToolTip("Insert the macro text at the cursor position");
    m_sendTypeGroup->addButton(m_insertRadio, ADD_TO_COMMAND);
    sendTypeLayout->addWidget(m_insertRadio);

    mainLayout->addWidget(sendTypeBox);

    // ========================================================================
    // SEND TEXT SECTION
    // ========================================================================
    QHBoxLayout* sendTextLabelLayout = new QHBoxLayout();
    QLabel* sendLabel = new QLabel("Send &text:", this);
    sendTextLabelLayout->addWidget(sendLabel);
    sendTextLabelLayout->addStretch();

    QPushButton* editButton = new QPushButton("&Edit...", this);
    editButton->setToolTip("Open multi-line editor");
    connect(editButton, &QPushButton::clicked, this, &MacroEditDialog::onEdit);
    sendTextLabelLayout->addWidget(editButton);

    mainLayout->addLayout(sendTextLabelLayout);

    m_sendTextEdit = new QTextEdit(this);
    m_sendTextEdit->setPlaceholderText("Text to send when macro key is pressed");
    m_sendTextEdit->setAcceptRichText(false);
    sendLabel->setBuddy(m_sendTextEdit);
    mainLayout->addWidget(m_sendTextEdit);

    // ========================================================================
    // BUTTON BOX
    // ========================================================================
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MacroEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MacroEditDialog::onCancel);
    mainLayout->addWidget(buttonBox);

    // Set focus to send text field
    m_sendTextEdit->setFocus();
}

void MacroEditDialog::loadMacroData()
{
    if (!m_doc || m_macroIndex < 0 || m_macroIndex >= TOTAL_MACRO_KEYS) {
        return;
    }

    // TODO: Load from WorldDocument when macro support is added
    // For now, we'll set up stubs
    // m_sendTextEdit->setPlainText(m_doc->m_macros[m_macroIndex]);
    // int macroType = m_doc->m_macro_type[m_macroIndex];

    // Stub: Default to SEND_NOW
    int macroType = SEND_NOW;
    QString macroText = "";

    // Set the radio button based on macro type
    QAbstractButton* button = m_sendTypeGroup->button(macroType);
    if (button) {
        button->setChecked(true);
    } else {
        // Default to SEND_NOW if no valid type
        m_sendNowRadio->setChecked(true);
    }

    m_sendTextEdit->setPlainText(macroText);
}

bool MacroEditDialog::validateForm()
{
    // No validation required - empty macros are allowed
    return true;
}

bool MacroEditDialog::saveMacro()
{
    if (!m_doc || m_macroIndex < 0 || m_macroIndex >= TOTAL_MACRO_KEYS) {
        QMessageBox::warning(this, "Error", "Invalid macro index.");
        return false;
    }

    // Get selected macro type
    int macroType = m_sendTypeGroup->checkedId();
    if (macroType < 0) {
        macroType = SEND_NOW; // Default
    }

    QString macroText = m_sendTextEdit->toPlainText();

    // TODO: Save to WorldDocument when macro support is added
    // m_doc->m_macros[m_macroIndex] = macroText;
    // m_doc->m_macro_type[m_macroIndex] = macroType;

    return true;
}

void MacroEditDialog::onOk()
{
    if (!validateForm()) {
        return;
    }

    if (saveMacro()) {
        accept();
    }
}

void MacroEditDialog::onCancel()
{
    reject();
}

void MacroEditDialog::onEdit()
{
    // TODO: Implement multi-line editor dialog similar to original
    // For now, just show a message
    QMessageBox::information(this, "Multi-line Editor",
                             "Multi-line editor not yet implemented.\n\n"
                             "You can edit the text directly in the main text area.");
}
