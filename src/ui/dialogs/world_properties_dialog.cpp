#include "world_properties_dialog.h"
#include "world/world_document.h"

#include "logging.h"
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

WorldPropertiesDialog::WorldPropertiesDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_tabWidget(nullptr), m_buttonBox(nullptr)
{
    setupUi();
    loadSettings();
}

WorldPropertiesDialog::~WorldPropertiesDialog()
{
    // Qt handles cleanup of child widgets
}

void WorldPropertiesDialog::setupUi()
{
    setWindowTitle("World Properties");
    setMinimumSize(600, 500);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Setup individual tabs (stubs for now)
    setupConnectionTab();
    setupOutputTab();
    setupInputTab();
    setupLoggingTab();
    setupScriptingTab();
    setupPasteToWorldTab();
    setupSendFileTab();
    setupRemoteAccessTab();

    // Create button box with OK, Cancel, Apply
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                           QDialogButtonBox::Apply,
                                       Qt::Horizontal, this);

    // Connect button signals
    connect(m_buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &WorldPropertiesDialog::onOkClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &WorldPropertiesDialog::onCancelClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &WorldPropertiesDialog::onApplyClicked);

    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);
}

void WorldPropertiesDialog::setupConnectionTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Server
    m_serverEdit = new QLineEdit();
    m_serverEdit->setPlaceholderText("e.g., aardmud.org");
    layout->addRow("Server:", m_serverEdit);

    // Port
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4000);
    layout->addRow("Port:", m_portSpin);

    // Character name
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Your character name");
    layout->addRow("Character name:", m_nameEdit);

    // Password
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Optional");
    layout->addRow("Password:", m_passwordEdit);

    // Auto-connect (note: backend support may need to be added)
    m_autoConnectCheck = new QCheckBox("Connect automatically on startup");
    layout->addRow("", m_autoConnectCheck);

    // Add some spacing
    layout->addRow("", new QWidget()); // Spacer

    m_tabWidget->addTab(tab, "Connection");
}

void WorldPropertiesDialog::setupOutputTab()
{
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Font selection section
    QHBoxLayout* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel("Output font:"));

    m_outputFontLabel = new QLabel("Courier New, 10pt");
    fontLayout->addWidget(m_outputFontLabel);

    m_outputFontButton = new QPushButton("Choose Font...");
    connect(m_outputFontButton, &QPushButton::clicked, this,
            &WorldPropertiesDialog::onOutputFontButtonClicked);
    fontLayout->addWidget(m_outputFontButton);
    fontLayout->addStretch();

    layout->addLayout(fontLayout);

    // ANSI color palette section
    layout->addSpacing(20);
    layout->addWidget(new QLabel("ANSI Color Palette:"));

    QGridLayout* colorGrid = new QGridLayout();

    const char* colorNames[16] = {
        "Black",        "Red",           "Green",       "Yellow",         "Blue",
        "Magenta",      "Cyan",          "White",       "Bright Black",   "Bright Red",
        "Bright Green", "Bright Yellow", "Bright Blue", "Bright Magenta", "Bright Cyan",
        "Bright White"};

    // Create 16 color buttons in 2 columns
    for (int i = 0; i < 16; i++) {
        QHBoxLayout* colorRow = new QHBoxLayout();

        QLabel* label = new QLabel(colorNames[i]);
        label->setMinimumWidth(120);
        colorRow->addWidget(label);

        m_colorButtons[i] = new QPushButton();
        m_colorButtons[i]->setFixedSize(80, 30);
        m_colorButtons[i]->setProperty("colorIndex", i);
        connect(m_colorButtons[i], &QPushButton::clicked, this,
                &WorldPropertiesDialog::onColorButtonClicked);
        colorRow->addWidget(m_colorButtons[i]);
        colorRow->addStretch();

        // Place in grid: 2 columns, 8 rows
        int row = i % 8;
        int col = i / 8;
        colorGrid->addLayout(colorRow, row, col);
    }

    layout->addLayout(colorGrid);

    // Activity notification section
    layout->addSpacing(20);
    layout->addWidget(new QLabel("Activity:"));
    m_flashIconCheck = new QCheckBox("Flash taskbar icon when new output arrives");
    layout->addWidget(m_flashIconCheck);

    layout->addStretch();

    m_tabWidget->addTab(tab, "Output");
}

void WorldPropertiesDialog::setupInputTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Input font
    QHBoxLayout* fontLayout = new QHBoxLayout();
    m_inputFontLabel = new QLabel("Courier New, 10pt");
    fontLayout->addWidget(m_inputFontLabel);

    m_inputFontButton = new QPushButton("Choose Font...");
    connect(m_inputFontButton, &QPushButton::clicked, this,
            &WorldPropertiesDialog::onInputFontButtonClicked);
    fontLayout->addWidget(m_inputFontButton);
    fontLayout->addStretch();

    layout->addRow("Input font:", fontLayout);

    // Echo input
    m_echoInputCheck = new QCheckBox("Echo my input in output window");
    layout->addRow("", m_echoInputCheck);

    // Echo color
    m_echoColorCombo = new QComboBox();
    m_echoColorCombo->addItems({"Same as output", "Custom color"});
    layout->addRow("Echo color:", m_echoColorCombo);

    // Command history size
    m_historySizeSpin = new QSpinBox();
    m_historySizeSpin->setRange(20,
                                5000); // Original: 20-5000 (prefspropertypages.cpp:DDV_MinMaxInt)
    m_historySizeSpin->setValue(20);
    m_historySizeSpin->setSuffix(" commands");
    layout->addRow("Command history size:", m_historySizeSpin);

    // Add some spacing
    layout->addRow("", new QWidget()); // Spacer

    m_tabWidget->addTab(tab, "Input");
}

void WorldPropertiesDialog::setupLoggingTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Enable logging
    m_enableLogCheck = new QCheckBox("Enable logging");
    layout->addRow("", m_enableLogCheck);

    // Log file
    QHBoxLayout* fileLayout = new QHBoxLayout();
    m_logFileEdit = new QLineEdit();
    m_logFileEdit->setPlaceholderText("Path to log file");
    fileLayout->addWidget(m_logFileEdit);

    m_logFileBrowse = new QPushButton("Browse...");
    // TODO: Connect to file browser when implemented
    fileLayout->addWidget(m_logFileBrowse);

    layout->addRow("Log file:", fileLayout);

    // Format
    m_logFormatCombo = new QComboBox();
    m_logFormatCombo->addItems({"Text", "HTML", "Raw"});
    layout->addRow("Format:", m_logFormatCombo);

    // Add some spacing
    layout->addRow("", new QWidget()); // Spacer

    m_tabWidget->addTab(tab, "Logging");
}

void WorldPropertiesDialog::setupScriptingTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Enable scripting
    m_enableScriptCheck = new QCheckBox("Enable scripting");
    layout->addRow("", m_enableScriptCheck);

    // Script file
    QHBoxLayout* fileLayout = new QHBoxLayout();
    m_scriptFileEdit = new QLineEdit();
    m_scriptFileEdit->setPlaceholderText("Path to script file");
    fileLayout->addWidget(m_scriptFileEdit);

    m_scriptFileBrowse = new QPushButton("Browse...");
    // TODO: Connect to file browser when implemented
    fileLayout->addWidget(m_scriptFileBrowse);

    layout->addRow("Script file:", fileLayout);

    // Language
    m_scriptLanguageCombo = new QComboBox();
    m_scriptLanguageCombo->addItems({"Lua", "YueScript", "MoonScript", "Teal", "Fennel"});
    layout->addRow("Language:", m_scriptLanguageCombo);

    // Add some spacing
    layout->addRow("", new QWidget()); // Spacer

    m_tabWidget->addTab(tab, "Scripting");
}

void WorldPropertiesDialog::setupPasteToWorldTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Info label
    QLabel* infoLabel =
        new QLabel("Configure how text is sent when pasting from the clipboard to the MUD.");
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);

    // Preamble - sent before all pasted content
    m_pastePreambleEdit = new QLineEdit();
    m_pastePreambleEdit->setPlaceholderText("Text sent before pasted content");
    layout->addRow("Preamble:", m_pastePreambleEdit);

    // Postamble - sent after all pasted content
    m_pastePostambleEdit = new QLineEdit();
    m_pastePostambleEdit->setPlaceholderText("Text sent after pasted content");
    layout->addRow("Postamble:", m_pastePostambleEdit);

    // Line preamble - sent before each line
    m_pasteLinePreambleEdit = new QLineEdit();
    m_pasteLinePreambleEdit->setPlaceholderText("Text prepended to each line");
    layout->addRow("Line preamble:", m_pasteLinePreambleEdit);

    // Line postamble - sent after each line
    m_pasteLinePostambleEdit = new QLineEdit();
    m_pasteLinePostambleEdit->setPlaceholderText("Text appended to each line");
    layout->addRow("Line postamble:", m_pasteLinePostambleEdit);

    // Line delay
    m_pasteDelaySpin = new QSpinBox();
    m_pasteDelaySpin->setRange(0, 10000);
    m_pasteDelaySpin->setSuffix(" ms");
    m_pasteDelaySpin->setToolTip("Delay between sending lines (0-10000 ms)");
    layout->addRow("Line delay:", m_pasteDelaySpin);

    // Delay per lines
    m_pasteDelayPerLinesSpin = new QSpinBox();
    m_pasteDelayPerLinesSpin->setRange(1, 100000);
    m_pasteDelayPerLinesSpin->setValue(1);
    m_pasteDelayPerLinesSpin->setToolTip("Apply delay every N lines");
    layout->addRow("Delay every N lines:", m_pasteDelayPerLinesSpin);

    // Commented softcode
    m_pasteCommentedSoftcodeCheck = new QCheckBox("Commented softcode (strip leading #)");
    m_pasteCommentedSoftcodeCheck->setToolTip(
        "Remove leading # from lines for MUD softcode compatibility");
    layout->addRow("", m_pasteCommentedSoftcodeCheck);

    // Echo
    m_pasteEchoCheck = new QCheckBox("Echo pasted lines to output");
    layout->addRow("", m_pasteEchoCheck);

    // Confirm before paste
    m_pasteConfirmCheck = new QCheckBox("Confirm before pasting");
    m_pasteConfirmCheck->setToolTip("Show confirmation dialog before sending pasted text");
    layout->addRow("", m_pasteConfirmCheck);

    m_tabWidget->addTab(tab, "Paste to World");
}

void WorldPropertiesDialog::setupSendFileTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Info label
    QLabel* infoLabel = new QLabel("Configure how text is sent when sending a file to the MUD.");
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);

    // Preamble - sent before all file content
    m_filePreambleEdit = new QLineEdit();
    m_filePreambleEdit->setPlaceholderText("Text sent before file content");
    layout->addRow("Preamble:", m_filePreambleEdit);

    // Postamble - sent after all file content
    m_filePostambleEdit = new QLineEdit();
    m_filePostambleEdit->setPlaceholderText("Text sent after file content");
    layout->addRow("Postamble:", m_filePostambleEdit);

    // Line preamble - sent before each line
    m_fileLinePreambleEdit = new QLineEdit();
    m_fileLinePreambleEdit->setPlaceholderText("Text prepended to each line");
    layout->addRow("Line preamble:", m_fileLinePreambleEdit);

    // Line postamble - sent after each line
    m_fileLinePostambleEdit = new QLineEdit();
    m_fileLinePostambleEdit->setPlaceholderText("Text appended to each line");
    layout->addRow("Line postamble:", m_fileLinePostambleEdit);

    // Line delay
    m_fileDelaySpin = new QSpinBox();
    m_fileDelaySpin->setRange(0, 10000);
    m_fileDelaySpin->setSuffix(" ms");
    m_fileDelaySpin->setToolTip("Delay between sending lines (0-10000 ms)");
    layout->addRow("Line delay:", m_fileDelaySpin);

    // Delay per lines
    m_fileDelayPerLinesSpin = new QSpinBox();
    m_fileDelayPerLinesSpin->setRange(1, 100000);
    m_fileDelayPerLinesSpin->setValue(1);
    m_fileDelayPerLinesSpin->setToolTip("Apply delay every N lines");
    layout->addRow("Delay every N lines:", m_fileDelayPerLinesSpin);

    // Commented softcode
    m_fileCommentedSoftcodeCheck = new QCheckBox("Commented softcode (strip leading #)");
    m_fileCommentedSoftcodeCheck->setToolTip(
        "Remove leading # from lines for MUD softcode compatibility");
    layout->addRow("", m_fileCommentedSoftcodeCheck);

    // Echo
    m_fileEchoCheck = new QCheckBox("Echo sent lines to output");
    layout->addRow("", m_fileEchoCheck);

    // Confirm before send
    m_fileConfirmCheck = new QCheckBox("Confirm before sending");
    m_fileConfirmCheck->setToolTip("Show confirmation dialog before sending file");
    layout->addRow("", m_fileConfirmCheck);

    m_tabWidget->addTab(tab, "Send File");
}

void WorldPropertiesDialog::setupRemoteAccessTab()
{
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    // Info label
    QLabel* infoLabel =
        new QLabel("Remote Access allows you to connect to this world from another device "
                   "(phone, tablet, SSH session) via telnet while away from your computer.");
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);

    // Enable remote access
    m_enableRemoteAccessCheck = new QCheckBox("Enable remote access server");
    layout->addRow("", m_enableRemoteAccessCheck);

    // Port
    m_remotePortSpin = new QSpinBox();
    m_remotePortSpin->setRange(1, 65535);
    m_remotePortSpin->setValue(4001);
    m_remotePortSpin->setToolTip("Port to listen on for remote connections");
    layout->addRow("Port:", m_remotePortSpin);

    // Password
    m_remotePasswordEdit = new QLineEdit();
    m_remotePasswordEdit->setEchoMode(QLineEdit::Password);
    m_remotePasswordEdit->setPlaceholderText("Required for security");
    m_remotePasswordEdit->setToolTip("Password required to authenticate remote clients");
    layout->addRow("Password:", m_remotePasswordEdit);

    // Scrollback lines
    m_remoteScrollbackSpin = new QSpinBox();
    m_remoteScrollbackSpin->setRange(0, 10000);
    m_remoteScrollbackSpin->setValue(100);
    m_remoteScrollbackSpin->setSuffix(" lines");
    m_remoteScrollbackSpin->setToolTip("Number of recent lines sent when a client connects");
    layout->addRow("Scrollback on connect:", m_remoteScrollbackSpin);

    // Max clients
    m_remoteMaxClientsSpin = new QSpinBox();
    m_remoteMaxClientsSpin->setRange(1, 100);
    m_remoteMaxClientsSpin->setValue(5);
    m_remoteMaxClientsSpin->setToolTip("Maximum simultaneous remote client connections");
    layout->addRow("Max clients:", m_remoteMaxClientsSpin);

    // Usage hint
    QLabel* usageLabel =
        new QLabel("To connect: telnet yourhost <port>\n"
                   "The server starts when you connect to a MUD and stops when you disconnect.");
    usageLabel->setWordWrap(true);
    usageLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addRow("", usageLabel);

    // Add some spacing
    layout->addRow("", new QWidget()); // Spacer

    m_tabWidget->addTab(tab, "Remote Access");
}

void WorldPropertiesDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Connection tab
    m_serverEdit->setText(m_doc->m_server);
    m_portSpin->setValue(m_doc->m_port);
    m_nameEdit->setText(m_doc->m_mush_name);
    m_passwordEdit->setText(m_doc->m_password);
    m_autoConnectCheck->setChecked(m_doc->m_connect_now != 0);

    // Output tab
    // Reconstruct QFont from WorldDocument font properties
    m_outputFont.setFamily(m_doc->m_font_name);
    m_outputFont.setPointSize(qAbs(m_doc->m_font_height)); // Convert pixels to points approximation
    m_outputFont.setWeight(m_doc->m_font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_outputFontLabel->setText(
        QString("%1, %2pt").arg(m_outputFont.family()).arg(m_outputFont.pointSize()));

    // TODO: ANSI colors - WorldDocument needs m_normalColour[8] and m_boldColour[8] arrays
    // For now, initialize with default colors
    QRgb defaultColors[16] = {
        qRgb(0, 0, 0),       // Black
        qRgb(128, 0, 0),     // Red
        qRgb(0, 128, 0),     // Green
        qRgb(128, 128, 0),   // Yellow
        qRgb(0, 0, 128),     // Blue
        qRgb(128, 0, 128),   // Magenta
        qRgb(0, 128, 128),   // Cyan
        qRgb(192, 192, 192), // White
        qRgb(128, 128, 128), // Bright Black (Gray)
        qRgb(255, 0, 0),     // Bright Red
        qRgb(0, 255, 0),     // Bright Green
        qRgb(255, 255, 0),   // Bright Yellow
        qRgb(0, 0, 255),     // Bright Blue
        qRgb(255, 0, 255),   // Bright Magenta
        qRgb(0, 255, 255),   // Bright Cyan
        qRgb(255, 255, 255)  // Bright White
    };
    for (int i = 0; i < 16; i++) {
        m_ansiColors[i] = defaultColors[i];
        updateColorButton(i);
    }

    // Activity settings
    m_flashIconCheck->setChecked(m_doc->m_bFlashIcon != 0);

    // Input tab
    m_inputFont.setFamily(m_doc->m_input_font_name);
    m_inputFont.setPointSize(qAbs(m_doc->m_input_font_height));
    m_inputFont.setWeight(m_doc->m_input_font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_inputFont.setItalic(m_doc->m_input_font_italic != 0);
    m_inputFontLabel->setText(
        QString("%1, %2pt").arg(m_inputFont.family()).arg(m_inputFont.pointSize()));

    m_echoInputCheck->setChecked(m_doc->m_display_my_input != 0);
    // TODO: m_echoColorCombo - needs echo color setting in WorldDocument

    // Command history size
    m_historySizeSpin->setValue(m_doc->m_maxCommandHistory);

    // Logging tab
    m_enableLogCheck->setChecked(m_doc->m_bLogOutput != 0);
    m_logFileEdit->setText(m_doc->m_strAutoLogFileName);
    // TODO: m_logFormatCombo - WorldDocument needs log format enum

    // Scripting tab
    m_enableScriptCheck->setChecked(m_doc->m_bEnableScripts != 0);
    m_scriptFileEdit->setText(m_doc->m_strScriptFilename);
    // TODO: m_scriptLanguageCombo - WorldDocument needs script language setting

    // Paste to World tab
    m_pastePreambleEdit->setText(m_doc->m_paste_preamble);
    m_pastePostambleEdit->setText(m_doc->m_paste_postamble);
    m_pasteLinePreambleEdit->setText(m_doc->m_pasteline_preamble);
    m_pasteLinePostambleEdit->setText(m_doc->m_pasteline_postamble);
    m_pasteDelaySpin->setValue(m_doc->m_nPasteDelay);
    m_pasteDelayPerLinesSpin->setValue(m_doc->m_nPasteDelayPerLines);
    m_pasteCommentedSoftcodeCheck->setChecked(m_doc->m_bPasteCommentedSoftcode != 0);
    m_pasteEchoCheck->setChecked(m_doc->m_bPasteEcho != 0);
    m_pasteConfirmCheck->setChecked(m_doc->m_bConfirmOnPaste != 0);

    // Send File tab
    m_filePreambleEdit->setText(m_doc->m_file_preamble);
    m_filePostambleEdit->setText(m_doc->m_file_postamble);
    m_fileLinePreambleEdit->setText(m_doc->m_line_preamble);
    m_fileLinePostambleEdit->setText(m_doc->m_line_postamble);
    m_fileDelaySpin->setValue(m_doc->m_nFileDelay);
    m_fileDelayPerLinesSpin->setValue(m_doc->m_nFileDelayPerLines);
    m_fileCommentedSoftcodeCheck->setChecked(m_doc->m_bFileCommentedSoftcode != 0);
    m_fileEchoCheck->setChecked(m_doc->m_bSendEcho != 0);
    m_fileConfirmCheck->setChecked(m_doc->m_bConfirmOnSend != 0);

    // Remote Access tab
    m_enableRemoteAccessCheck->setChecked(m_doc->m_bEnableRemoteAccess != 0);
    m_remotePortSpin->setValue(m_doc->m_iRemotePort > 0 ? m_doc->m_iRemotePort : 4001);
    m_remotePasswordEdit->setText(m_doc->m_strRemotePassword);
    m_remoteScrollbackSpin->setValue(m_doc->m_iRemoteScrollbackLines);
    m_remoteMaxClientsSpin->setValue(m_doc->m_iRemoteMaxClients);

    qCDebug(lcDialog) << "WorldPropertiesDialog::loadSettings() - loaded from WorldDocument";
}

void WorldPropertiesDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Connection tab
    m_doc->m_server = m_serverEdit->text();
    m_doc->m_port = m_portSpin->value();
    m_doc->m_mush_name = m_nameEdit->text();
    m_doc->m_password = m_passwordEdit->text();
    m_doc->m_connect_now = m_autoConnectCheck->isChecked() ? 1 : 0;

    // Output tab
    m_doc->m_font_name = m_outputFont.family();
    m_doc->m_font_height = m_outputFont.pointSize(); // Store as points
    m_doc->m_font_weight = m_outputFont.weight();
    // TODO: ANSI colors - save to m_normalColour[] and m_boldColour[] when they exist

    // Activity settings
    m_doc->m_bFlashIcon = m_flashIconCheck->isChecked() ? 1 : 0;

    // Input tab
    m_doc->m_input_font_name = m_inputFont.family();
    m_doc->m_input_font_height = m_inputFont.pointSize();
    m_doc->m_input_font_weight = m_inputFont.weight();
    m_doc->m_input_font_italic = m_inputFont.italic() ? 1 : 0;
    m_doc->m_display_my_input = m_echoInputCheck->isChecked() ? 1 : 0;
    // TODO: m_echoColorCombo - save when WorldDocument has echo color setting

    // Command history size
    m_doc->m_maxCommandHistory = m_historySizeSpin->value();

    // Logging tab
    m_doc->m_bLogOutput = m_enableLogCheck->isChecked() ? 1 : 0;
    m_doc->m_strAutoLogFileName = m_logFileEdit->text();

    // Scripting tab
    m_doc->m_bEnableScripts = m_enableScriptCheck->isChecked() ? 1 : 0;
    m_doc->m_strScriptFilename = m_scriptFileEdit->text();
    // TODO: m_scriptLanguageCombo - save when WorldDocument has script language

    // Paste to World tab
    m_doc->m_paste_preamble = m_pastePreambleEdit->text();
    m_doc->m_paste_postamble = m_pastePostambleEdit->text();
    m_doc->m_pasteline_preamble = m_pasteLinePreambleEdit->text();
    m_doc->m_pasteline_postamble = m_pasteLinePostambleEdit->text();
    m_doc->m_nPasteDelay = m_pasteDelaySpin->value();
    m_doc->m_nPasteDelayPerLines = m_pasteDelayPerLinesSpin->value();
    m_doc->m_bPasteCommentedSoftcode = m_pasteCommentedSoftcodeCheck->isChecked() ? 1 : 0;
    m_doc->m_bPasteEcho = m_pasteEchoCheck->isChecked() ? 1 : 0;
    m_doc->m_bConfirmOnPaste = m_pasteConfirmCheck->isChecked() ? 1 : 0;

    // Send File tab
    m_doc->m_file_preamble = m_filePreambleEdit->text();
    m_doc->m_file_postamble = m_filePostambleEdit->text();
    m_doc->m_line_preamble = m_fileLinePreambleEdit->text();
    m_doc->m_line_postamble = m_fileLinePostambleEdit->text();
    m_doc->m_nFileDelay = m_fileDelaySpin->value();
    m_doc->m_nFileDelayPerLines = m_fileDelayPerLinesSpin->value();
    m_doc->m_bFileCommentedSoftcode = m_fileCommentedSoftcodeCheck->isChecked() ? 1 : 0;
    m_doc->m_bSendEcho = m_fileEchoCheck->isChecked() ? 1 : 0;
    m_doc->m_bConfirmOnSend = m_fileConfirmCheck->isChecked() ? 1 : 0;

    // Remote Access tab
    m_doc->m_bEnableRemoteAccess = m_enableRemoteAccessCheck->isChecked() ? 1 : 0;
    m_doc->m_iRemotePort = m_remotePortSpin->value();
    m_doc->m_strRemotePassword = m_remotePasswordEdit->text();
    m_doc->m_iRemoteScrollbackLines = m_remoteScrollbackSpin->value();
    m_doc->m_iRemoteMaxClients = m_remoteMaxClientsSpin->value();

    m_doc->setModified(true);

    qCDebug(lcDialog) << "WorldPropertiesDialog::saveSettings() - saved to WorldDocument";
}

void WorldPropertiesDialog::applySettings()
{
    saveSettings();

    // Emit signal to notify OutputView and other UI components of changes
    if (m_doc) {
        emit m_doc->outputSettingsChanged();

        // Reconfigure script file watcher in case filename or reload option changed
        m_doc->setupScriptFileWatcher();
    }

    qCDebug(lcDialog)
        << "WorldPropertiesDialog::applySettings() - settings saved and signal emitted";
}

void WorldPropertiesDialog::onOkClicked()
{
    qCDebug(lcDialog) << "WorldPropertiesDialog: OK clicked";
    applySettings();
    accept(); // Close dialog with accepted status
}

void WorldPropertiesDialog::onCancelClicked()
{
    qCDebug(lcDialog) << "WorldPropertiesDialog: Cancel clicked";
    reject(); // Close dialog with rejected status
}

void WorldPropertiesDialog::onApplyClicked()
{
    qCDebug(lcDialog) << "WorldPropertiesDialog: Apply clicked";
    applySettings();
    // Don't close the dialog
}

void WorldPropertiesDialog::onOutputFontButtonClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_outputFont, this, "Choose Output Font");

    if (ok) {
        m_outputFont = font;
        m_outputFontLabel->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        qCDebug(lcDialog) << "Font changed to:" << font.family() << font.pointSize();
    }
}

void WorldPropertiesDialog::onColorButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    int index = btn->property("colorIndex").toInt();

    QColor color = QColorDialog::getColor(QColor(m_ansiColors[index]), this,
                                          QString("Choose color for index %1").arg(index));

    if (color.isValid()) {
        m_ansiColors[index] = color.rgb();
        updateColorButton(index);
        qCDebug(lcDialog) << "Color" << index << "changed to:" << color.name();
    }
}

void WorldPropertiesDialog::updateColorButton(int index)
{
    if (index < 0 || index >= 16)
        return;

    QColor color(m_ansiColors[index]);
    QString style = QString("background-color: %1; color: %2;")
                        .arg(color.name())
                        .arg(color.lightness() > 128 ? "black" : "white"); // Contrasting text

    m_colorButtons[index]->setStyleSheet(style);
    m_colorButtons[index]->setText(color.name());
}

void WorldPropertiesDialog::onInputFontButtonClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_inputFont, this, "Choose Input Font");

    if (ok) {
        m_inputFont = font;
        m_inputFontLabel->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        qCDebug(lcDialog) << "Input font changed to:" << font.family() << font.pointSize();
    }
}
