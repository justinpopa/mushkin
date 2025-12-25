#include "lua_gsub_dialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

LuaGsubDialog::LuaGsubDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Lua gsub - Find and Replace");
    setMinimumWidth(500);
    setMinimumHeight(400);
    setupUi();
}

void LuaGsubDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Selection size info label at top
    m_selectionSizeLabel = new QLabel(this);
    m_selectionSizeLabel->setWordWrap(true);
    m_selectionSizeLabel->setText("No selection information");
    mainLayout->addWidget(m_selectionSizeLabel);

    // Form layout for find/replace fields
    QFormLayout* formLayout = new QFormLayout();

    // Find pattern field with Edit button
    QHBoxLayout* findLayout = new QHBoxLayout();
    m_findPatternEdit = new QLineEdit(this);
    m_findPatternEdit->setPlaceholderText("Pattern to find (regex)");
    findLayout->addWidget(m_findPatternEdit);

    m_editFindButton = new QPushButton("Edit...", this);
    m_editFindButton->setToolTip("Edit pattern in multiline editor (placeholder)");
    connect(m_editFindButton, &QPushButton::clicked, this, &LuaGsubDialog::onEditFindPattern);
    findLayout->addWidget(m_editFindButton);

    formLayout->addRow("&Find pattern:", findLayout);

    // Replacement field with Edit button
    QHBoxLayout* replaceLayout = new QHBoxLayout();
    m_replacementEdit = new QLineEdit(this);
    m_replacementEdit->setPlaceholderText("Replacement text");
    replaceLayout->addWidget(m_replacementEdit);

    m_editReplacementButton = new QPushButton("Edit...", this);
    m_editReplacementButton->setToolTip("Edit replacement in multiline editor (placeholder)");
    connect(m_editReplacementButton, &QPushButton::clicked, this,
            &LuaGsubDialog::onEditReplacement);
    replaceLayout->addWidget(m_editReplacementButton);

    formLayout->addRow("&Replace with:", replaceLayout);

    mainLayout->addLayout(formLayout);

    // Create replacement label (will be toggled based on function checkbox)
    m_replacementLabel = new QLabel("&Replace with:", this);
    m_replacementLabel->setVisible(false); // Hidden initially since it's in the form layout

    // Function text edit (hidden by default)
    m_functionLabel = new QLabel("&Function text:", this);
    m_functionLabel->setVisible(false);
    mainLayout->addWidget(m_functionLabel);

    m_functionTextEdit = new QTextEdit(this);
    m_functionTextEdit->setPlaceholderText(
        "Lua function for replacement (e.g., function(match) return match:upper() end)");
    m_functionTextEdit->setVisible(false);

    // Use monospace font for function text
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_functionTextEdit->setFont(monoFont);
    m_functionTextEdit->setTabStopDistance(
        m_functionTextEdit->fontMetrics().horizontalAdvance(' ') * 4);
    m_functionTextEdit->setMinimumHeight(100);

    mainLayout->addWidget(m_functionTextEdit);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_eachLineCheckBox = new QCheckBox("Process &each line separately", this);
    m_eachLineCheckBox->setToolTip("Apply gsub to each line individually instead of entire text");
    optionsLayout->addWidget(m_eachLineCheckBox);

    m_escapeSequencesCheckBox = new QCheckBox("&Interpret escape sequences in replacement", this);
    m_escapeSequencesCheckBox->setToolTip(
        "Process escape sequences like \\n, \\t in replacement text");
    optionsLayout->addWidget(m_escapeSequencesCheckBox);

    m_callFunctionCheckBox =
        new QCheckBox("&Call function instead of using replacement text", this);
    m_callFunctionCheckBox->setToolTip("Use a Lua function to generate replacement text");
    connect(m_callFunctionCheckBox, &QCheckBox::toggled, this,
            &LuaGsubDialog::onCallFunctionToggled);
    optionsLayout->addWidget(m_callFunctionCheckBox);

    mainLayout->addWidget(optionsGroup);

    // Add stretch to push buttons to bottom
    mainLayout->addStretch();

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus to the find field
    m_findPatternEdit->setFocus();
}

// Accessor methods
QString LuaGsubDialog::findPattern() const
{
    return m_findPatternEdit->text();
}

QString LuaGsubDialog::replacementText() const
{
    if (m_callFunctionCheckBox->isChecked()) {
        return m_functionTextEdit->toPlainText();
    }
    return m_replacementEdit->text();
}

bool LuaGsubDialog::processEachLine() const
{
    return m_eachLineCheckBox->isChecked();
}

bool LuaGsubDialog::interpretEscapeSequences() const
{
    return m_escapeSequencesCheckBox->isChecked();
}

bool LuaGsubDialog::callFunction() const
{
    return m_callFunctionCheckBox->isChecked();
}

QString LuaGsubDialog::functionText() const
{
    return m_functionTextEdit->toPlainText();
}

// Setter methods
void LuaGsubDialog::setFindPattern(const QString& pattern)
{
    m_findPatternEdit->setText(pattern);
}

void LuaGsubDialog::setReplacementText(const QString& text)
{
    m_replacementEdit->setText(text);
}

void LuaGsubDialog::setProcessEachLine(bool enabled)
{
    m_eachLineCheckBox->setChecked(enabled);
}

void LuaGsubDialog::setInterpretEscapeSequences(bool enabled)
{
    m_escapeSequencesCheckBox->setChecked(enabled);
}

void LuaGsubDialog::setCallFunction(bool enabled)
{
    m_callFunctionCheckBox->setChecked(enabled);
    updateReplacementFieldState();
}

void LuaGsubDialog::setFunctionText(const QString& text)
{
    m_functionTextEdit->setPlainText(text);
}

void LuaGsubDialog::setSelectionSizeInfo(const QString& info)
{
    m_selectionSizeLabel->setText(info);
}

// Slots
void LuaGsubDialog::onEditFindPattern()
{
    QMessageBox::information(this, "Edit Find Pattern",
                             "Multiline editor for find pattern is not yet implemented.\n\n"
                             "This would open a larger editor for complex patterns.");
}

void LuaGsubDialog::onEditReplacement()
{
    QMessageBox::information(this, "Edit Replacement",
                             "Multiline editor for replacement text is not yet implemented.\n\n"
                             "This would open a larger editor for complex replacements.");
}

void LuaGsubDialog::onCallFunctionToggled(bool checked)
{
    updateReplacementFieldState();
}

void LuaGsubDialog::updateReplacementFieldState()
{
    bool useFunction = m_callFunctionCheckBox->isChecked();

    // Toggle visibility between replacement text field and function text field
    m_replacementEdit->setVisible(!useFunction);
    m_editReplacementButton->setVisible(!useFunction);

    m_functionLabel->setVisible(useFunction);
    m_functionTextEdit->setVisible(useFunction);
}
