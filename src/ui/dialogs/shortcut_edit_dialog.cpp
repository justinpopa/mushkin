/**
 * shortcut_edit_dialog.cpp - Dialog for adding/editing keyboard shortcuts
 *
 * Provides a modern interface for configuring keyboard shortcuts with
 * record mode (press keys to capture) and conflict detection.
 */

#include "shortcut_edit_dialog.h"
#include "../../world/accelerator_manager.h"
#include "../../world/world_document.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// SendTo constants (from world_document.h)
enum eSendTo {
    eSendToWorld = 0,
    eSendToCommand = 1,
    eSendToOutput = 2,
    eSendToStatus = 3,
    eSendToNotepad = 4,
    eSendToNotepadAppend = 5,
    eSendToLogFile = 6,
    eSendToNotepadReplace = 7,
    eSendToCommandQueue = 8,
    eSendToVariable = 9,
    eSendToExecute = 12,
    eSendToSpeedwalk = 13,
    eSendToScript = 14,
    eSendToImmediate = 15,
    eSendToScriptAfterOmit = 16
};

ShortcutEditDialog::ShortcutEditDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_isEditMode(false)
{
    setupUi();
    setWindowTitle("Add Shortcut");
}

ShortcutEditDialog::ShortcutEditDialog(WorldDocument* doc, const QString& keyString,
                                       QWidget* parent)
    : QDialog(parent), m_doc(doc), m_originalKeyString(keyString), m_isEditMode(true)
{
    setupUi();
    setWindowTitle("Edit Shortcut");
    loadShortcutData();
}

void ShortcutEditDialog::setupUi()
{
    setMinimumWidth(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for fields
    QFormLayout* formLayout = new QFormLayout();

    // Key sequence edit with clear button
    QHBoxLayout* keyLayout = new QHBoxLayout();
    m_keySequenceEdit = new QKeySequenceEdit(this);
    m_keySequenceEdit->setToolTip("Click and press a key combination to record it");
    QPushButton* clearButton = new QPushButton("Clear", this);
    clearButton->setToolTip("Clear the key sequence");
    keyLayout->addWidget(m_keySequenceEdit, 1);
    keyLayout->addWidget(clearButton);
    formLayout->addRow("Shortcut:", keyLayout);

    // Action text edit
    m_actionEdit = new QLineEdit(this);
    m_actionEdit->setPlaceholderText("Command or script to execute");
    formLayout->addRow("Action:", m_actionEdit);

    // Send-to combo
    m_sendToCombo = new QComboBox(this);
    m_sendToCombo->addItem("Execute (run as command)", eSendToExecute);
    m_sendToCombo->addItem("World (send to MUD)", eSendToWorld);
    m_sendToCombo->addItem("Script (execute Lua)", eSendToScript);
    m_sendToCombo->addItem("Speedwalk", eSendToSpeedwalk);
    m_sendToCombo->addItem("Command window", eSendToCommand);
    m_sendToCombo->addItem("Output window", eSendToOutput);
    m_sendToCombo->addItem("Variable", eSendToVariable);
    m_sendToCombo->addItem("Notepad (new)", eSendToNotepad);
    m_sendToCombo->addItem("Notepad (append)", eSendToNotepadAppend);
    m_sendToCombo->addItem("Notepad (replace)", eSendToNotepadReplace);
    m_sendToCombo->addItem("Log file", eSendToLogFile);
    formLayout->addRow("Send to:", m_sendToCombo);

    mainLayout->addLayout(formLayout);

    // Conflict warning label
    m_conflictLabel = new QLabel(this);
    m_conflictLabel->setStyleSheet("QLabel { color: #c0392b; font-weight: bold; }");
    m_conflictLabel->setWordWrap(true);
    m_conflictLabel->hide();
    mainLayout->addWidget(m_conflictLabel);

    // Add some spacing
    mainLayout->addSpacing(10);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(m_keySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this,
            &ShortcutEditDialog::onKeySequenceChanged);
    connect(clearButton, &QPushButton::clicked, this, &ShortcutEditDialog::onClearKey);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ShortcutEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ShortcutEditDialog::loadShortcutData()
{
    if (!m_doc || !m_doc->m_acceleratorManager || m_originalKeyString.isEmpty()) {
        return;
    }

    const AcceleratorEntry* entry =
        m_doc->m_acceleratorManager->getAccelerator(m_originalKeyString);
    if (!entry) {
        return;
    }

    // Load key sequence
    m_keySequenceEdit->setKeySequence(entry->keySeq);

    // Load action
    m_actionEdit->setText(entry->action);

    // Load send-to
    int sendToIndex = m_sendToCombo->findData(entry->sendTo);
    if (sendToIndex >= 0) {
        m_sendToCombo->setCurrentIndex(sendToIndex);
    }
}

void ShortcutEditDialog::onKeySequenceChanged(const QKeySequence& keySequence)
{
    Q_UNUSED(keySequence);
    checkConflict();
}

void ShortcutEditDialog::onClearKey()
{
    m_keySequenceEdit->clear();
    m_conflictLabel->hide();
}

void ShortcutEditDialog::checkConflict()
{
    if (!m_doc || !m_doc->m_acceleratorManager) {
        m_conflictLabel->hide();
        return;
    }

    QKeySequence keySeq = m_keySequenceEdit->keySequence();
    if (keySeq.isEmpty()) {
        m_conflictLabel->hide();
        return;
    }

    // Get portable key string for lookup
    QString keyString = keySeq.toString(QKeySequence::PortableText);

    // Check if this key is already assigned
    const AcceleratorEntry* existing = m_doc->m_acceleratorManager->getAccelerator(keyString);

    if (existing) {
        // In edit mode, don't warn about the original key
        if (m_isEditMode) {
            QString originalNormalized = m_originalKeyString.toUpper().simplified();
            QString currentNormalized = keyString.toUpper();
            if (originalNormalized == currentNormalized) {
                m_conflictLabel->hide();
                return;
            }
        }

        // Show conflict warning
        QString sourceText;
        switch (existing->source) {
            case AcceleratorSource::User:
                sourceText = "another user shortcut";
                break;
            case AcceleratorSource::Script:
                sourceText = "a world script";
                break;
            case AcceleratorSource::Plugin:
                sourceText = QString("plugin '%1'").arg(existing->pluginId);
                break;
        }

        m_conflictLabel->setText(
            QString("Warning: %1 is already assigned to %2").arg(keyString).arg(sourceText));
        m_conflictLabel->show();
    } else {
        m_conflictLabel->hide();
    }
}

bool ShortcutEditDialog::validateForm()
{
    // Check for key sequence
    if (m_keySequenceEdit->keySequence().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a key combination.");
        m_keySequenceEdit->setFocus();
        return false;
    }

    // Check for action
    if (m_actionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter an action to execute.");
        m_actionEdit->setFocus();
        return false;
    }

    return true;
}

void ShortcutEditDialog::onOk()
{
    if (validateForm()) {
        accept();
    }
}

QString ShortcutEditDialog::keyString() const
{
    return m_keySequenceEdit->keySequence().toString(QKeySequence::PortableText);
}

QString ShortcutEditDialog::action() const
{
    return m_actionEdit->text().trimmed();
}

int ShortcutEditDialog::sendTo() const
{
    return m_sendToCombo->currentData().toInt();
}
