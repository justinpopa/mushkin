#include "debug_world_input_dialog.h"
#include "insert_unicode_dialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

DebugWorldInputDialog::DebugWorldInputDialog(QWidget* parent)
    : QDialog(parent), m_textEdit(nullptr), m_specialsCombo(nullptr), m_insertButton(nullptr),
      m_insertUnicodeButton(nullptr)
{
    setWindowTitle("Debug World Input");
    setModal(true);
    setMinimumSize(450, 300);

    setupUi();
    populateSpecialCharacters();
}

void DebugWorldInputDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Text input area with monospace font
    QLabel* textLabel = new QLabel("Text to send:", this);
    mainLayout->addWidget(textLabel);

    m_textEdit = new QPlainTextEdit(this);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);
    m_textEdit->setTabStopDistance(m_textEdit->fontMetrics().horizontalAdvance(' ') * 4);
    m_textEdit->setPlaceholderText("Enter text to send to the world...");
    mainLayout->addWidget(m_textEdit);

    // Special characters section
    QGridLayout* specialsLayout = new QGridLayout();

    QLabel* specialsLabel = new QLabel("Special characters:", this);
    specialsLayout->addWidget(specialsLabel, 0, 0);

    m_specialsCombo = new QComboBox(this);
    m_specialsCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    specialsLayout->addWidget(m_specialsCombo, 0, 1);

    m_insertButton = new QPushButton("Insert", this);
    m_insertButton->setToolTip("Insert selected special character at cursor position");
    connect(m_insertButton, &QPushButton::clicked, this,
            &DebugWorldInputDialog::onInsertSpecialClicked);
    specialsLayout->addWidget(m_insertButton, 0, 2);

    m_insertUnicodeButton = new QPushButton("Insert Unicode...", this);
    m_insertUnicodeButton->setToolTip("Insert a Unicode character by code point");
    connect(m_insertUnicodeButton, &QPushButton::clicked, this,
            &DebugWorldInputDialog::onInsertUnicodeClicked);
    specialsLayout->addWidget(m_insertUnicodeButton, 1, 1, 1, 2);

    mainLayout->addLayout(specialsLayout);

    // Dialog buttons (OK/Cancel)
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void DebugWorldInputDialog::populateSpecialCharacters()
{
    // ANSI escape codes
    m_specialsCombo->addItem("ESC (0x1B) - Escape character", QString(QChar(0x1B)));
    m_specialsCombo->addItem("CSI (ESC[) - Control Sequence Introducer",
                             QString(QChar(0x1B)) + "[");
    m_specialsCombo->addItem("OSC (ESC]) - Operating System Command", QString(QChar(0x1B)) + "]");

    // MXP codes
    m_specialsCombo->addItem("MXP Start (ESC[1z)", QString(QChar(0x1B)) + "[1z");
    m_specialsCombo->addItem("MXP Stop (ESC[2z)", QString(QChar(0x1B)) + "[2z");
    m_specialsCombo->addItem("MXP Secure (ESC[3z)", QString(QChar(0x1B)) + "[3z");
    m_specialsCombo->addItem("MXP Locked (ESC[4z)", QString(QChar(0x1B)) + "[4z");
    m_specialsCombo->addItem("MXP Reset (ESC[5z)", QString(QChar(0x1B)) + "[5z");
    m_specialsCombo->addItem("MXP Temp Secure (ESC[6z)", QString(QChar(0x1B)) + "[6z");
    m_specialsCombo->addItem("MXP Line Open (ESC[0z)", QString(QChar(0x1B)) + "[0z");

    // Common control characters
    m_specialsCombo->addItem("NUL (0x00) - Null", QString(QChar(0x00)));
    m_specialsCombo->addItem("BEL (0x07) - Bell", QString(QChar(0x07)));
    m_specialsCombo->addItem("BS (0x08) - Backspace", QString(QChar(0x08)));
    m_specialsCombo->addItem("TAB (0x09) - Horizontal Tab", QString(QChar(0x09)));
    m_specialsCombo->addItem("LF (0x0A) - Line Feed", QString(QChar(0x0A)));
    m_specialsCombo->addItem("VT (0x0B) - Vertical Tab", QString(QChar(0x0B)));
    m_specialsCombo->addItem("FF (0x0C) - Form Feed", QString(QChar(0x0C)));
    m_specialsCombo->addItem("CR (0x0D) - Carriage Return", QString(QChar(0x0D)));

    // Telnet IAC and related
    m_specialsCombo->addItem("IAC (0xFF) - Telnet Interpret As Command", QString(QChar(0xFF)));
    m_specialsCombo->addItem("WILL (0xFB) - Telnet WILL", QString(QChar(0xFB)));
    m_specialsCombo->addItem("WONT (0xFC) - Telnet WONT", QString(QChar(0xFC)));
    m_specialsCombo->addItem("DO (0xFD) - Telnet DO", QString(QChar(0xFD)));
    m_specialsCombo->addItem("DONT (0xFE) - Telnet DONT", QString(QChar(0xFE)));
    m_specialsCombo->addItem("SB (0xFA) - Telnet Subnegotiation Begin", QString(QChar(0xFA)));
    m_specialsCombo->addItem("SE (0xF0) - Telnet Subnegotiation End", QString(QChar(0xF0)));

    // Additional useful codes
    m_specialsCombo->addItem("SO (0x0E) - Shift Out", QString(QChar(0x0E)));
    m_specialsCombo->addItem("SI (0x0F) - Shift In", QString(QChar(0x0F)));
    m_specialsCombo->addItem("CAN (0x18) - Cancel", QString(QChar(0x18)));
    m_specialsCombo->addItem("SUB (0x1A) - Substitute", QString(QChar(0x1A)));
    m_specialsCombo->addItem("DEL (0x7F) - Delete", QString(QChar(0x7F)));
}

QString DebugWorldInputDialog::text() const
{
    return m_textEdit->toPlainText();
}

void DebugWorldInputDialog::setText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

void DebugWorldInputDialog::onInsertSpecialClicked()
{
    // Get selected special character sequence
    QString sequence = m_specialsCombo->currentData().toString();

    if (sequence.isEmpty()) {
        return;
    }

    // Insert at cursor position
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.insertText(sequence);

    // Return focus to text edit
    m_textEdit->setFocus();
}

void DebugWorldInputDialog::onInsertUnicodeClicked()
{
    InsertUnicodeDialog unicodeDialog(this);

    if (unicodeDialog.exec() == QDialog::Accepted) {
        QString character = unicodeDialog.character();

        if (!character.isEmpty()) {
            // Insert at cursor position
            QTextCursor cursor = m_textEdit->textCursor();
            cursor.insertText(character);

            // Return focus to text edit
            m_textEdit->setFocus();
        }
    }
}
