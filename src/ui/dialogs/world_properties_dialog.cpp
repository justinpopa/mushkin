#include "world_properties_dialog.h"
#include "world/world_document.h"

#include "logging.h"
#include "mxp_script_routines_dialog.h"
#include "network/ssh_host_key_manager.h"
#include "world/view_interfaces.h"
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

// For totalPhysicalMemoryBytes() — platform RAM query (M78 memory warning).
#if defined(Q_OS_MACOS) || defined(Q_OS_BSD4)
#    include <sys/sysctl.h>
#    include <sys/types.h>
#elif defined(Q_OS_UNIX)
#    include <unistd.h>
#endif

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

    // Auto-connect method (original: configuration.cpp:702-710)
    m_connectMethodCombo = new QComboBox();
    m_connectMethodCombo->addItem("No auto-connect", eNoAutoConnect);
    m_connectMethodCombo->addItem("MUSH (send: connect name password)", eConnectMUSH);
    m_connectMethodCombo->addItem("Diku (send: name then password)", eConnectDiku);
    m_connectMethodCombo->addItem("MXP", eConnectMXP);
    layout->addRow("Connect method:", m_connectMethodCombo);

    // Connect text (multi-line text sent on connection)
    m_connectTextEdit = new QTextEdit();
    m_connectTextEdit->setMaximumHeight(80);
    m_connectTextEdit->setPlaceholderText("Text to send after connecting (optional)");

    // Dynamic "(N lines)" indicator mirroring the original CPrefsP21 IDC_LINE_COUNT
    // control updated by OnUpdateLineCount (prefspropertypages.cpp:8129-8152).
    QHBoxLayout* connectTextLayout = new QHBoxLayout();
    connectTextLayout->addWidget(m_connectTextEdit);
    m_connectLineCountLabel = new QLabel("(0 lines)");
    m_connectLineCountLabel->setStyleSheet("color: gray;");
    m_connectLineCountLabel->setAlignment(Qt::AlignTop);
    connectTextLayout->addWidget(m_connectLineCountLabel);
    layout->addRow("Connect text:", connectTextLayout);
    connect(m_connectTextEdit, &QTextEdit::textChanged, this,
            &WorldPropertiesDialog::updateConnectLineCount);

    // Proxy settings
    QGroupBox* proxyGroup = new QGroupBox("Proxy");
    QFormLayout* proxyLayout = new QFormLayout(proxyGroup);

    m_proxyTypeCombo = new QComboBox();
    m_proxyTypeCombo->addItems({"None", "SOCKS5", "HTTP CONNECT"});
    proxyLayout->addRow("Type:", m_proxyTypeCombo);

    m_proxyServerEdit = new QLineEdit();
    m_proxyServerEdit->setPlaceholderText("proxy.example.com");
    proxyLayout->addRow("Server:", m_proxyServerEdit);

    m_proxyPortSpin = new QSpinBox();
    m_proxyPortSpin->setRange(0, 65535);
    m_proxyPortSpin->setSpecialValueText("Disabled");
    proxyLayout->addRow("Port:", m_proxyPortSpin);

    m_proxyUsernameEdit = new QLineEdit();
    m_proxyUsernameEdit->setPlaceholderText("Optional");
    proxyLayout->addRow("Username:", m_proxyUsernameEdit);

    m_proxyPasswordEdit = new QLineEdit();
    m_proxyPasswordEdit->setEchoMode(QLineEdit::Password);
    m_proxyPasswordEdit->setPlaceholderText("Optional");
    proxyLayout->addRow("Password:", m_proxyPasswordEdit);

    layout->addRow(proxyGroup);

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

    static constexpr std::array<const char*, 16> colorNames = {
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

    // Display options section
    layout->addSpacing(20);
    layout->addWidget(new QLabel("Display Options:"));

    QFormLayout* displayForm = new QFormLayout();

    m_wrapCheck = new QCheckBox("Enable word wrap");
    displayForm->addRow(m_wrapCheck);

    m_wrapColumnSpin = new QSpinBox();
    m_wrapColumnSpin->setRange(20, 500);
    m_wrapColumnSpin->setSuffix(" chars");

    // "Adjust Width" / "Adjust to Width" buttons carried over from the original
    // CPrefsP14 (IDC_ADJUST_WIDTH / IDC_ADJUST_TO_WIDTH). The first sets the wrap
    // column from the output window's current pixel width; the second resizes the
    // output window to fit the chosen wrap column.
    QHBoxLayout* wrapColumnLayout = new QHBoxLayout();
    wrapColumnLayout->addWidget(m_wrapColumnSpin);
    m_adjustWidthButton = new QPushButton("Adjust Width");
    connect(m_adjustWidthButton, &QPushButton::clicked, this,
            &WorldPropertiesDialog::onAdjustWidthClicked);
    wrapColumnLayout->addWidget(m_adjustWidthButton);
    m_adjustToWidthButton = new QPushButton("Adjust to Width");
    connect(m_adjustToWidthButton, &QPushButton::clicked, this,
            &WorldPropertiesDialog::onAdjustToWidthClicked);
    wrapColumnLayout->addWidget(m_adjustToWidthButton);
    wrapColumnLayout->addStretch();
    displayForm->addRow("Wrap column:", wrapColumnLayout);

    m_maxLinesSpin = new QSpinBox();
    m_maxLinesSpin->setRange(200, 500000);
    displayForm->addRow("Max output lines:", m_maxLinesSpin);

    m_utf8Check = new QCheckBox("UTF-8 (Unicode) output");
    displayForm->addRow(m_utf8Check);

    m_nawsCheck = new QCheckBox("NAWS (send window size to server)");
    displayForm->addRow(m_nawsCheck);

    m_terminalTypeEdit = new QLineEdit();
    // Original enforces DDV_MaxChars(20) on IDC_TERMINAL_TYPE
    // (prefspropertypages.cpp:5774).
    m_terminalTypeEdit->setMaxLength(20);
    displayForm->addRow("Terminal type:", m_terminalTypeEdit);

    m_indentParasCheck = new QCheckBox("Indent paragraphs on word-wrap");
    displayForm->addRow(m_indentParasCheck);

    m_showBoldCheck = new QCheckBox("Show bold");
    displayForm->addRow(m_showBoldCheck);

    m_showItalicCheck = new QCheckBox("Show italic");
    displayForm->addRow(m_showItalicCheck);

    m_showUnderlineCheck = new QCheckBox("Show underline");
    displayForm->addRow(m_showUnderlineCheck);

    m_lineSpacingSpin = new QSpinBox();
    m_lineSpacingSpin->setRange(0, 100);
    displayForm->addRow("Line spacing:", m_lineSpacingSpin);

    // Output-page options carried over from the original CPrefsP14 dialog
    // (prefspropertypages.cpp:5748-5783). All are backed by WorldDocument but
    // were previously unreachable from the UI.
    m_enableBeepsCheck = new QCheckBox("Enable beeps (^G)");
    displayForm->addRow(m_enableBeepsCheck);

    m_lineInformationCheck = new QCheckBox("Show line information");
    displayForm->addRow(m_lineInformationCheck);

    m_startPausedCheck = new QCheckBox("Start paused (more prompt)");
    displayForm->addRow(m_startPausedCheck);

    m_unpauseOnSendCheck = new QCheckBox("Unpause on send");
    displayForm->addRow(m_unpauseOnSendCheck);

    m_autoFreezeCheck = new QCheckBox("Auto-freeze when scrolled back");
    displayForm->addRow(m_autoFreezeCheck);

    m_disableCompressionCheck = new QCheckBox("Disable MCCP compression");
    displayForm->addRow(m_disableCompressionCheck);

    m_useDefaultOutputFontCheck = new QCheckBox("Use default output font");
    displayForm->addRow(m_useDefaultOutputFontCheck);

    m_alternativeInverseCheck = new QCheckBox("Alternative inverse handling");
    displayForm->addRow(m_alternativeInverseCheck);

    m_pixelOffsetSpin = new QSpinBox();
    m_pixelOffsetSpin->setRange(0, 20); // Original: DDV_MinMaxInt 0..20
    m_pixelOffsetSpin->setSuffix(" px");
    displayForm->addRow("Pixel offset:", m_pixelOffsetSpin);

    layout->addLayout(displayForm);

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

    // Echo color — index 0 = "Same as output" (SAMECOLOUR=65535), 1-16 = custom color 0-15
    m_echoColorCombo = new QComboBox();
    m_echoColorCombo->addItem("Same as output");
    for (int i = 0; i < 16; i++) {
        m_echoColorCombo->addItem(QString("Custom color %1").arg(i + 1));
    }
    layout->addRow("Echo color:", m_echoColorCombo);

    // Command history size
    m_historySizeSpin = new QSpinBox();
    m_historySizeSpin->setRange(20,
                                5000); // Original: 20-5000 (prefspropertypages.cpp:DDV_MinMaxInt)
    m_historySizeSpin->setValue(20);
    m_historySizeSpin->setSuffix(" commands");
    layout->addRow("Command history size:", m_historySizeSpin);

    // Command stacking
    layout->addRow("", new QLabel("Command Stacking:"));
    m_enableCommandStackCheck = new QCheckBox("Enable command stacking");
    layout->addRow("", m_enableCommandStackCheck);
    m_commandStackCharEdit = new QLineEdit();
    m_commandStackCharEdit->setMaxLength(1);
    m_commandStackCharEdit->setFixedWidth(40);
    layout->addRow("Stack character:", m_commandStackCharEdit);

    // Speed walk
    layout->addRow("", new QLabel("Speed Walking:"));
    m_enableSpeedwalkCheck = new QCheckBox("Enable speed walking");
    layout->addRow("", m_enableSpeedwalkCheck);
    m_speedwalkPrefixEdit = new QLineEdit();
    m_speedwalkPrefixEdit->setFixedWidth(40);
    layout->addRow("Prefix:", m_speedwalkPrefixEdit);
    // Speed-walk filler: the 'F' token expansion (original IDC_SPEED_WALK_FILLER,
    // prefspropertypages.cpp:4675). Backed by m_speedwalk.filler.
    m_speedwalkFillerEdit = new QLineEdit();
    m_speedwalkFillerEdit->setPlaceholderText("Command substituted for the 'F' token");
    layout->addRow("Filler:", m_speedwalkFillerEdit);
    m_speedwalkDelaySpin = new QSpinBox();
    m_speedwalkDelaySpin->setRange(0, 30000);
    m_speedwalkDelaySpin->setSuffix(" ms");
    layout->addRow("Delay:", m_speedwalkDelaySpin);

    // Spam prevention
    layout->addRow("", new QLabel("Options:"));
    m_escapeDeletesInputCheck = new QCheckBox("Escape key clears input");
    layout->addRow("", m_escapeDeletesInputCheck);
    m_noEchoOffCheck = new QCheckBox("Ignore server echo-off (show passwords)");
    layout->addRow("", m_noEchoOffCheck);

    // Spam prevention (original CPrefsP4: IDC_ENABLE_SPAM_PREVENTION / IDC_SPAM_LINE_COUNT /
    // IDC_SPAM_FILLER, prefspropertypages.cpp:4685-4688). Backed by m_doc->m_spam.* which
    // DoSendMsg already consults; the UI was previously absent (M77).
    layout->addRow("", new QLabel("Spam Prevention:"));
    m_enableSpamPreventionCheck = new QCheckBox("Enable spam prevention");
    layout->addRow("", m_enableSpamPreventionCheck);
    m_spamLineCountSpin = new QSpinBox();
    m_spamLineCountSpin->setRange(5, 500); // Original: DDV_MinMaxInt 5..500
    layout->addRow("Repeats before filler:", m_spamLineCountSpin);
    m_spamMessageEdit = new QLineEdit();
    m_spamMessageEdit->setPlaceholderText("Filler message sent to break spam (e.g. look)");
    layout->addRow("Spam filler message:", m_spamMessageEdit);

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
    // TODO(ui): Connect Browse button to QFileDialog for file selection.
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
    fileLayout->addWidget(m_scriptFileBrowse);
    // Mirror the original CPrefsP17::OnBrowse / ScriptBrowser (prefspropertypages.cpp:7090-7129),
    // which opens a file dialog and writes the chosen path into the script filename field.
    connect(m_scriptFileBrowse, &QPushButton::clicked, this, [this]() {
        QString path =
            QFileDialog::getOpenFileName(this, "Script source file", m_scriptFileEdit->text(),
                                         "Lua scripts (*.lua);;All files (*.*)");
        if (!path.isEmpty()) {
            m_scriptFileEdit->setText(path);
        }
    });

    layout->addRow("Script file:", fileLayout);

    // Language — only Lua is supported. The original hides the language selector
    // entirely (CPrefsP17::OnInitDialog hides m_ctlLanguage since WSH was removed,
    // prefspropertypages.cpp:7178); we keep a disabled single-entry combo so the
    // dialog cannot offer non-functional languages that would be ignored on save.
    m_scriptLanguageCombo = new QComboBox();
    m_scriptLanguageCombo->addItem("Lua");
    m_scriptLanguageCombo->setEnabled(false);
    layout->addRow("Language:", m_scriptLanguageCombo);

    // Script prefix (original IDC_SCRIPT_PREFIX, prefspropertypages.cpp:7007). The
    // prefix that flags a command line as a script expression. Backed by
    // m_scripting.prefix.
    m_scriptPrefixEdit = new QLineEdit();
    m_scriptPrefixEdit->setPlaceholderText("Prefix marking a line as a script expression");
    layout->addRow("Script prefix:", m_scriptPrefixEdit);

    // Script editor (original IDC_SCRIPT_EDITOR + OnChooseEditor,
    // prefspropertypages.cpp:7009, 7247-7267). External editor used to edit the
    // script file; Browse mirrors the original CFileDialog. Backed by
    // m_scripting.editor.
    QHBoxLayout* editorLayout = new QHBoxLayout();
    m_scriptEditorEdit = new QLineEdit();
    m_scriptEditorEdit->setPlaceholderText("Path to external script editor");
    editorLayout->addWidget(m_scriptEditorEdit);
    m_scriptEditorBrowse = new QPushButton("Browse...");
    editorLayout->addWidget(m_scriptEditorBrowse);
    connect(m_scriptEditorBrowse, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Script editor", m_scriptEditorEdit->text(), "Applications (*);;All files (*.*)");
        if (!path.isEmpty()) {
            m_scriptEditorEdit->setText(path);
        }
    });
    layout->addRow("Script editor:", editorLayout);

    // MXP Scripts button (original IDC_MXP_SCRIPTS / CPrefsP17::OnMxpScripts,
    // prefspropertypages.cpp:7069, 7300-7321). Opens the sub-dialog that edits the
    // six MXP callback routine names, which it reads/writes directly on the document.
    m_mxpScriptsButton = new QPushButton("MXP Scripts...");
    connect(m_mxpScriptsButton, &QPushButton::clicked, this, [this]() {
        MxpScriptRoutinesDialog dlg(m_doc, this);
        dlg.exec();
    });
    layout->addRow("", m_mxpScriptsButton);

    // Event handler entry points
    layout->addRow("", new QLabel("Event Handlers:"));
    m_onWorldOpenEdit = new QLineEdit();
    m_onWorldOpenEdit->setPlaceholderText("e.g., OnWorldOpen");
    layout->addRow("On world open:", m_onWorldOpenEdit);

    m_onWorldCloseEdit = new QLineEdit();
    m_onWorldCloseEdit->setPlaceholderText("e.g., OnWorldClose");
    layout->addRow("On world close:", m_onWorldCloseEdit);

    m_onWorldConnectEdit = new QLineEdit();
    m_onWorldConnectEdit->setPlaceholderText("e.g., OnWorldConnect");
    layout->addRow("On connect:", m_onWorldConnectEdit);

    m_onWorldDisconnectEdit = new QLineEdit();
    m_onWorldDisconnectEdit->setPlaceholderText("e.g., OnWorldDisconnect");
    layout->addRow("On disconnect:", m_onWorldDisconnectEdit);

    m_onWorldGetFocusEdit = new QLineEdit();
    m_onWorldGetFocusEdit->setPlaceholderText("e.g., OnWorldGetFocus");
    layout->addRow("On get focus:", m_onWorldGetFocusEdit);

    m_onWorldLoseFocusEdit = new QLineEdit();
    m_onWorldLoseFocusEdit->setPlaceholderText("e.g., OnWorldLoseFocus");
    layout->addRow("On lose focus:", m_onWorldLoseFocusEdit);

    m_onWorldSaveEdit = new QLineEdit();
    m_onWorldSaveEdit->setPlaceholderText("e.g., OnWorldSave");
    layout->addRow("On save:", m_onWorldSaveEdit);

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
                   "(phone, tablet) via SSH while away from your computer.");
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);

    // Enable remote access
    m_enableRemoteAccessCheck = new QCheckBox("Enable SSH remote access server");
    layout->addRow("", m_enableRemoteAccessCheck);

    // Port
    m_remotePortSpin = new QSpinBox();
    m_remotePortSpin->setRange(1, 65535);
    m_remotePortSpin->setValue(4001);
    m_remotePortSpin->setToolTip("Port to listen on for SSH connections");
    layout->addRow("Port:", m_remotePortSpin);

    // Password
    m_remotePasswordEdit = new QLineEdit();
    m_remotePasswordEdit->setEchoMode(QLineEdit::Password);
    m_remotePasswordEdit->setPlaceholderText("For password authentication");
    m_remotePasswordEdit->setToolTip(
        "Password for SSH password authentication (optional if using public keys)");
    layout->addRow("Password:", m_remotePasswordEdit);

    // Authorized keys file
    m_remoteAuthorizedKeysEdit = new QLineEdit();
    m_remoteAuthorizedKeysEdit->setPlaceholderText("Path to authorized_keys file (optional)");
    m_remoteAuthorizedKeysEdit->setToolTip(
        "Path to an authorized_keys file for public key authentication");
    auto* browseBtn = new QPushButton("Browse...");
    auto* keysLayout = new QHBoxLayout();
    keysLayout->addWidget(m_remoteAuthorizedKeysEdit);
    keysLayout->addWidget(browseBtn);
    layout->addRow("Authorized keys:", keysLayout);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select authorized_keys file");
        if (!path.isEmpty()) {
            m_remoteAuthorizedKeysEdit->setText(path);
        }
    });

    // Host key fingerprint (read-only)
    m_hostKeyFingerprintLabel = new QLabel();
    m_hostKeyFingerprintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_hostKeyFingerprintLabel->setStyleSheet("font-family: monospace; color: gray;");
    auto fp = SshHostKeyManager::fingerprint();
    m_hostKeyFingerprintLabel->setText(fp ? *fp : "No host key generated yet");
    layout->addRow("Host key:", m_hostKeyFingerprintLabel);

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
    m_remoteMaxClientsSpin->setToolTip("Maximum simultaneous SSH client connections");
    layout->addRow("Max clients:", m_remoteMaxClientsSpin);

    // Usage hint
    QLabel* usageLabel =
        new QLabel("To connect: ssh -p <port> user@yourhost\n"
                   "The server starts when you connect to a MUD and stops when you disconnect.");
    usageLabel->setWordWrap(true);
    usageLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addRow("", usageLabel);

    m_tabWidget->addTab(tab, "Remote Access");
}

void WorldPropertiesDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Connection tab
    m_serverEdit->setText(m_doc->m_server);
    m_portSpin->setValue(m_doc->m_port);
    m_nameEdit->setText(m_doc->m_name);
    m_passwordEdit->setText(m_doc->m_password);
    int connectIdx = m_connectMethodCombo->findData(m_doc->m_connect_now);
    if (connectIdx >= 0)
        m_connectMethodCombo->setCurrentIndex(connectIdx);
    m_connectTextEdit->setPlainText(m_doc->m_connect_text);

    // Output tab
    // Reconstruct QFont from WorldDocument font properties
    m_outputFont.setFamily(m_doc->m_output.font_name);
    m_outputFont.setPointSize(
        qAbs(m_doc->m_output.font_height)); // Convert pixels to points approximation
    m_outputFont.setWeight(m_doc->m_output.font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_outputFontLabel->setText(
        QString("%1, %2pt").arg(m_outputFont.family()).arg(m_outputFont.pointSize()));

    // Load ANSI colors from WorldDocument (BGR format → QRgb for display)
    // normal_colour[0..7] = colors 0-7, bold_colour[0..7] = colors 8-15
    for (int i = 0; i < 8; i++) {
        QRgb bgr = m_doc->m_colors.normal_colour[i];
        m_ansiColors[i] = qRgb(bgr & 0xFF, (bgr >> 8) & 0xFF, (bgr >> 16) & 0xFF);
    }
    for (int i = 0; i < 8; i++) {
        QRgb bgr = m_doc->m_colors.bold_colour[i];
        m_ansiColors[i + 8] = qRgb(bgr & 0xFF, (bgr >> 8) & 0xFF, (bgr >> 16) & 0xFF);
    }
    for (int i = 0; i < 16; i++) {
        updateColorButton(i);
    }

    // Display options
    m_wrapCheck->setChecked(m_doc->m_display.wrap);
    m_wrapColumnSpin->setValue(m_doc->m_display.wrap_column);
    m_maxLinesSpin->setValue(m_doc->m_display.max_lines);
    m_utf8Check->setChecked(m_doc->m_display.utf8);
    m_nawsCheck->setChecked(m_doc->m_bNAWS);
    m_terminalTypeEdit->setText(m_doc->m_strTerminalIdentification);
    m_indentParasCheck->setChecked(m_doc->m_display.indent_paras);
    m_showBoldCheck->setChecked(m_doc->m_display.show_bold);
    m_showItalicCheck->setChecked(m_doc->m_display.show_italic);
    m_showUnderlineCheck->setChecked(m_doc->m_display.show_underline);
    m_lineSpacingSpin->setValue(m_doc->m_display.line_spacing);

    // Output-page options carried over from original CPrefsP14
    m_enableBeepsCheck->setChecked(m_doc->m_sound.enable_beeps);
    m_lineInformationCheck->setChecked(m_doc->m_display.line_information);
    m_startPausedCheck->setChecked(m_doc->m_bStartPaused);
    m_unpauseOnSendCheck->setChecked(m_doc->m_bUnpauseOnSend);
    m_autoFreezeCheck->setChecked(m_doc->m_display.auto_freeze);
    m_disableCompressionCheck->setChecked(m_doc->m_bDisableCompression);
    m_useDefaultOutputFontCheck->setChecked(m_doc->m_bUseDefaultOutputFont != 0);
    m_alternativeInverseCheck->setChecked(m_doc->m_bAlternativeInverse);
    m_pixelOffsetSpin->setValue(m_doc->m_display.pixel_offset);

    // Activity settings
    m_flashIconCheck->setChecked(m_doc->m_display.flash_icon);

    // Input tab
    m_inputFont.setFamily(m_doc->m_input.font_name);
    m_inputFont.setPointSize(qAbs(m_doc->m_input.font_height));
    m_inputFont.setWeight(m_doc->m_input.font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_inputFont.setItalic(m_doc->m_input.font_italic != 0);
    m_inputFontLabel->setText(
        QString("%1, %2pt").arg(m_inputFont.family()).arg(m_inputFont.pointSize()));

    m_echoInputCheck->setChecked(m_doc->m_display_my_input);
    // echo_colour: 65535 = SAMECOLOUR (combo index 0), 0-15 = custom color (combo index 1-16)
    if (m_doc->m_output.echo_colour == 65535) {
        m_echoColorCombo->setCurrentIndex(0);
    } else {
        m_echoColorCombo->setCurrentIndex(
            qBound(1, static_cast<int>(m_doc->m_output.echo_colour) + 1, 16));
    }

    // Command history size
    m_historySizeSpin->setValue(m_doc->m_maxCommandHistory);

    // Command stacking
    m_enableCommandStackCheck->setChecked(m_doc->m_input.enable_command_stack);
    m_commandStackCharEdit->setText(m_doc->m_input.command_stack_character);

    // Speed walk
    m_enableSpeedwalkCheck->setChecked(m_doc->m_speedwalk.enabled);
    m_speedwalkPrefixEdit->setText(m_doc->m_speedwalk.prefix);
    m_speedwalkFillerEdit->setText(m_doc->m_speedwalk.filler);
    m_speedwalkDelaySpin->setValue(m_doc->m_speedwalk.delay);

    // Options
    m_escapeDeletesInputCheck->setChecked(m_doc->m_input.escape_deletes_input);
    m_noEchoOffCheck->setChecked(m_doc->m_input.no_echo_off);

    // Spam prevention
    m_enableSpamPreventionCheck->setChecked(m_doc->m_spam.enabled);
    m_spamLineCountSpin->setValue(m_doc->m_spam.line_count);
    m_spamMessageEdit->setText(m_doc->m_spam.message);

    // Logging tab
    m_enableLogCheck->setChecked(m_doc->m_logging.log_output);
    m_logFileEdit->setText(m_doc->m_logging.auto_log_file_name);
    // Log format: Text=0, HTML=1, Raw=2
    if (m_doc->m_logging.log_html)
        m_logFormatCombo->setCurrentIndex(1);
    else if (m_doc->m_logging.log_raw)
        m_logFormatCombo->setCurrentIndex(2);
    else
        m_logFormatCombo->setCurrentIndex(0);

    // Scripting tab
    m_enableScriptCheck->setChecked(m_doc->m_scripting.enabled);
    m_scriptFileEdit->setText(m_doc->m_scripting.filename);
    m_scriptPrefixEdit->setText(m_doc->m_scripting.prefix);
    m_scriptEditorEdit->setText(m_doc->m_scripting.editor);
    m_onWorldOpenEdit->setText(m_doc->m_scripting.on_world_open);
    m_onWorldCloseEdit->setText(m_doc->m_scripting.on_world_close);
    m_onWorldConnectEdit->setText(m_doc->m_scripting.on_world_connect);
    m_onWorldDisconnectEdit->setText(m_doc->m_scripting.on_world_disconnect);
    m_onWorldGetFocusEdit->setText(m_doc->m_scripting.on_world_get_focus);
    m_onWorldLoseFocusEdit->setText(m_doc->m_scripting.on_world_lose_focus);
    m_onWorldSaveEdit->setText(m_doc->m_scripting.on_world_save);

    // Paste to World tab
    m_pastePreambleEdit->setText(m_doc->m_paste.paste_preamble);
    m_pastePostambleEdit->setText(m_doc->m_paste.paste_postamble);
    m_pasteLinePreambleEdit->setText(m_doc->m_paste.pasteline_preamble);
    m_pasteLinePostambleEdit->setText(m_doc->m_paste.pasteline_postamble);
    m_pasteDelaySpin->setValue(m_doc->m_paste.paste_delay);
    m_pasteDelayPerLinesSpin->setValue(m_doc->m_paste.paste_delay_per_lines);
    m_pasteCommentedSoftcodeCheck->setChecked(m_doc->m_paste.paste_commented_softcode);
    m_pasteEchoCheck->setChecked(m_doc->m_paste.paste_echo);
    m_pasteConfirmCheck->setChecked(m_doc->m_paste.confirm_on_paste);

    // Send File tab
    m_filePreambleEdit->setText(m_doc->m_paste.file_preamble);
    m_filePostambleEdit->setText(m_doc->m_paste.file_postamble);
    m_fileLinePreambleEdit->setText(m_doc->m_paste.line_preamble);
    m_fileLinePostambleEdit->setText(m_doc->m_paste.line_postamble);
    m_fileDelaySpin->setValue(m_doc->m_paste.file_delay);
    m_fileDelayPerLinesSpin->setValue(m_doc->m_paste.file_delay_per_lines);
    m_fileCommentedSoftcodeCheck->setChecked(m_doc->m_paste.file_commented_softcode);
    m_fileEchoCheck->setChecked(m_doc->m_input.send_echo);
    m_fileConfirmCheck->setChecked(m_doc->m_bConfirmOnSend);

    // Remote Access tab
    m_enableRemoteAccessCheck->setChecked(m_doc->m_remote.enabled);
    m_remotePortSpin->setValue(m_doc->m_remote.port > 0 ? m_doc->m_remote.port : 4001);
    m_remotePasswordEdit->setText(m_doc->m_remote.password);
    m_remoteAuthorizedKeysEdit->setText(m_doc->m_remote.authorized_keys_file);
    m_remoteScrollbackSpin->setValue(m_doc->m_remote.scrollback_lines);
    m_remoteMaxClientsSpin->setValue(m_doc->m_remote.max_clients);

    // Proxy settings
    m_proxyTypeCombo->setCurrentIndex(m_doc->m_proxy.type);
    m_proxyServerEdit->setText(m_doc->m_proxy.server);
    m_proxyPortSpin->setValue(m_doc->m_proxy.port);
    m_proxyUsernameEdit->setText(m_doc->m_proxy.username);
    m_proxyPasswordEdit->setText(m_doc->m_proxy.password);

    qCDebug(lcDialog) << "WorldPropertiesDialog::loadSettings() - loaded from WorldDocument";
}

namespace {
// Assigns src into dst only if they differ, recording that a change occurred.
// Used by saveSettings to reproduce the original's per-page ChangedPrefsPN check
// (configuration.cpp:1973-2016): the document is marked modified iff at least one
// committed value actually differs from its prior state.
template <typename T, typename U> void assignChanged(T& dst, U&& src, bool& changed)
{
    if (dst != static_cast<T>(src)) {
        dst = static_cast<T>(std::forward<U>(src));
        changed = true;
    }
}
} // namespace

void WorldPropertiesDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Tracks whether any committed value differs from the document's prior state.
    // When true at the end, the document is marked modified — mirroring the original
    // CMUSHclientDoc::OnApply, which calls SetModifiedFlag(TRUE) when any preference
    // page changed (configuration.cpp:1973-2016). Clicking OK without editing
    // anything must NOT mark the document modified, so unconditional marking is wrong.
    bool changed = false;

    // Connection tab
    assignChanged(m_doc->m_server, m_serverEdit->text(), changed);
    assignChanged(m_doc->m_port, m_portSpin->value(), changed);
    assignChanged(m_doc->m_name, m_nameEdit->text(), changed);
    assignChanged(m_doc->m_password, m_passwordEdit->text(), changed);
    assignChanged(m_doc->m_connect_now, m_connectMethodCombo->currentData().toInt(), changed);
    assignChanged(m_doc->m_connect_text, m_connectTextEdit->toPlainText(), changed);

    // Output tab
    assignChanged(m_doc->m_output.font_name, m_outputFont.family(), changed);
    assignChanged(m_doc->m_output.font_height, m_outputFont.pointSize(), changed); // points
    assignChanged(m_doc->m_output.font_weight, m_outputFont.weight(), changed);
    // Save ANSI colors back to WorldDocument (QRgb → BGR format)
    for (int i = 0; i < 8; i++) {
        QRgb rgb = m_ansiColors[i];
        assignChanged(m_doc->m_colors.normal_colour[i],
                      static_cast<QRgb>((qRed(rgb)) | (qGreen(rgb) << 8) | (qBlue(rgb) << 16)),
                      changed);
    }
    for (int i = 0; i < 8; i++) {
        QRgb rgb = m_ansiColors[i + 8];
        assignChanged(m_doc->m_colors.bold_colour[i],
                      static_cast<QRgb>((qRed(rgb)) | (qGreen(rgb) << 8) | (qBlue(rgb) << 16)),
                      changed);
    }

    // Display options
    assignChanged(m_doc->m_display.wrap, m_wrapCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.wrap_column, static_cast<quint16>(m_wrapColumnSpin->value()),
                  changed);
    assignChanged(m_doc->m_display.max_lines, m_maxLinesSpin->value(), changed);
    assignChanged(m_doc->m_display.utf8, m_utf8Check->isChecked(), changed);
    assignChanged(m_doc->m_bNAWS, m_nawsCheck->isChecked(), changed);
    assignChanged(m_doc->m_strTerminalIdentification, m_terminalTypeEdit->text(), changed);
    assignChanged(m_doc->m_display.indent_paras, m_indentParasCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.show_bold, m_showBoldCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.show_italic, m_showItalicCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.show_underline, m_showUnderlineCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.line_spacing, static_cast<quint16>(m_lineSpacingSpin->value()),
                  changed);

    // Output-page options carried over from original CPrefsP14
    assignChanged(m_doc->m_sound.enable_beeps, m_enableBeepsCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.line_information, m_lineInformationCheck->isChecked(), changed);
    assignChanged(m_doc->m_bStartPaused, m_startPausedCheck->isChecked(), changed);
    assignChanged(m_doc->m_bUnpauseOnSend, m_unpauseOnSendCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.auto_freeze, m_autoFreezeCheck->isChecked(), changed);
    assignChanged(m_doc->m_bDisableCompression, m_disableCompressionCheck->isChecked(), changed);
    assignChanged(m_doc->m_bUseDefaultOutputFont, m_useDefaultOutputFontCheck->isChecked() ? 1 : 0,
                  changed);
    assignChanged(m_doc->m_bAlternativeInverse, m_alternativeInverseCheck->isChecked(), changed);
    assignChanged(m_doc->m_display.pixel_offset, static_cast<quint16>(m_pixelOffsetSpin->value()),
                  changed);

    // Activity settings
    assignChanged(m_doc->m_display.flash_icon, m_flashIconCheck->isChecked(), changed);

    // Input tab
    assignChanged(m_doc->m_input.font_name, m_inputFont.family(), changed);
    assignChanged(m_doc->m_input.font_height, m_inputFont.pointSize(), changed);
    assignChanged(m_doc->m_input.font_weight, m_inputFont.weight(), changed);
    assignChanged(m_doc->m_input.font_italic, m_inputFont.italic() ? 1 : 0, changed);
    assignChanged(m_doc->m_display_my_input, m_echoInputCheck->isChecked(), changed);
    // echo_colour: combo index 0 = SAMECOLOUR (65535), 1-16 = custom color 0-15
    int echoIdx = m_echoColorCombo->currentIndex();
    assignChanged(m_doc->m_output.echo_colour,
                  (echoIdx == 0) ? quint16(65535) : static_cast<quint16>(echoIdx - 1), changed);

    // Command history size
    assignChanged(m_doc->m_maxCommandHistory, m_historySizeSpin->value(), changed);

    // Command stacking
    assignChanged(m_doc->m_input.enable_command_stack, m_enableCommandStackCheck->isChecked(),
                  changed);
    assignChanged(m_doc->m_input.command_stack_character, m_commandStackCharEdit->text(), changed);
    // Original CPrefsP9::DoDataExchange (prefspropertypages.cpp:4720-4721): when
    // stacking is disabled and the character is blank, default it to a space.
    if (!m_doc->m_input.enable_command_stack && m_doc->m_input.command_stack_character.isEmpty())
        m_doc->m_input.command_stack_character = " ";

    // Speed walk
    assignChanged(m_doc->m_speedwalk.enabled, m_enableSpeedwalkCheck->isChecked(), changed);
    assignChanged(m_doc->m_speedwalk.prefix, m_speedwalkPrefixEdit->text(), changed);
    assignChanged(m_doc->m_speedwalk.filler, m_speedwalkFillerEdit->text(), changed);
    assignChanged(m_doc->m_speedwalk.delay, m_speedwalkDelaySpin->value(), changed);

    // Options
    assignChanged(m_doc->m_input.escape_deletes_input, m_escapeDeletesInputCheck->isChecked(),
                  changed);
    assignChanged(m_doc->m_input.no_echo_off, m_noEchoOffCheck->isChecked(), changed);

    // Spam prevention
    assignChanged(m_doc->m_spam.enabled, m_enableSpamPreventionCheck->isChecked(), changed);
    assignChanged(m_doc->m_spam.line_count, static_cast<quint16>(m_spamLineCountSpin->value()),
                  changed);
    assignChanged(m_doc->m_spam.message, m_spamMessageEdit->text(), changed);

    // Logging tab
    assignChanged(m_doc->m_logging.log_output, m_enableLogCheck->isChecked(), changed);
    assignChanged(m_doc->m_logging.auto_log_file_name, m_logFileEdit->text(), changed);
    // Log format: Text=0, HTML=1, Raw=2
    assignChanged(m_doc->m_logging.log_html, m_logFormatCombo->currentIndex() == 1, changed);
    assignChanged(m_doc->m_logging.log_raw, m_logFormatCombo->currentIndex() == 2, changed);

    // Scripting tab — capture the previous engine-relevant state so applySettings
    // can reconcile the running script engine (original: SavePrefsP17 disables/recreates
    // the engine when the filename or enabled flag changes).
    m_prevScriptFilename = m_doc->m_scripting.filename;
    m_prevScriptEnabled = m_doc->m_scripting.enabled;
    assignChanged(m_doc->m_scripting.enabled, m_enableScriptCheck->isChecked(), changed);
    assignChanged(m_doc->m_scripting.filename, m_scriptFileEdit->text(), changed);
    assignChanged(m_doc->m_scripting.prefix, m_scriptPrefixEdit->text(), changed);
    assignChanged(m_doc->m_scripting.editor, m_scriptEditorEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_open, m_onWorldOpenEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_close, m_onWorldCloseEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_connect, m_onWorldConnectEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_disconnect, m_onWorldDisconnectEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_get_focus, m_onWorldGetFocusEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_lose_focus, m_onWorldLoseFocusEdit->text(), changed);
    assignChanged(m_doc->m_scripting.on_world_save, m_onWorldSaveEdit->text(), changed);
    // Not applicable: Script language is always Lua — no multi-language support needed.

    // Paste to World tab
    assignChanged(m_doc->m_paste.paste_preamble, m_pastePreambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.paste_postamble, m_pastePostambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.pasteline_preamble, m_pasteLinePreambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.pasteline_postamble, m_pasteLinePostambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.paste_delay, m_pasteDelaySpin->value(), changed);
    assignChanged(m_doc->m_paste.paste_delay_per_lines, m_pasteDelayPerLinesSpin->value(), changed);
    assignChanged(m_doc->m_paste.paste_commented_softcode,
                  m_pasteCommentedSoftcodeCheck->isChecked(), changed);
    assignChanged(m_doc->m_paste.paste_echo, m_pasteEchoCheck->isChecked(), changed);
    assignChanged(m_doc->m_paste.confirm_on_paste, m_pasteConfirmCheck->isChecked(), changed);

    // Send File tab
    assignChanged(m_doc->m_paste.file_preamble, m_filePreambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.file_postamble, m_filePostambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.line_preamble, m_fileLinePreambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.line_postamble, m_fileLinePostambleEdit->text(), changed);
    assignChanged(m_doc->m_paste.file_delay, m_fileDelaySpin->value(), changed);
    assignChanged(m_doc->m_paste.file_delay_per_lines, m_fileDelayPerLinesSpin->value(), changed);
    assignChanged(m_doc->m_paste.file_commented_softcode, m_fileCommentedSoftcodeCheck->isChecked(),
                  changed);
    assignChanged(m_doc->m_input.send_echo, m_fileEchoCheck->isChecked(), changed);
    assignChanged(m_doc->m_bConfirmOnSend, m_fileConfirmCheck->isChecked(), changed);

    // Remote Access tab
    assignChanged(m_doc->m_remote.enabled, m_enableRemoteAccessCheck->isChecked(), changed);
    assignChanged(m_doc->m_remote.port, m_remotePortSpin->value(), changed);
    assignChanged(m_doc->m_remote.password, m_remotePasswordEdit->text(), changed);
    assignChanged(m_doc->m_remote.authorized_keys_file, m_remoteAuthorizedKeysEdit->text(),
                  changed);
    assignChanged(m_doc->m_remote.scrollback_lines, m_remoteScrollbackSpin->value(), changed);
    assignChanged(m_doc->m_remote.max_clients, m_remoteMaxClientsSpin->value(), changed);

    // Proxy settings
    assignChanged(m_doc->m_proxy.type, static_cast<quint16>(m_proxyTypeCombo->currentIndex()),
                  changed);
    assignChanged(m_doc->m_proxy.server, m_proxyServerEdit->text(), changed);
    assignChanged(m_doc->m_proxy.port, static_cast<quint16>(m_proxyPortSpin->value()), changed);
    assignChanged(m_doc->m_proxy.username, m_proxyUsernameEdit->text(), changed);
    assignChanged(m_doc->m_proxy.password, m_proxyPasswordEdit->text(), changed);

    // Original CMUSHclientDoc::OnApply marks the document modified when any
    // preference page changed (configuration.cpp:1973-2016). Reproduce that here:
    // set the flag (never clear it) when at least one committed value differed.
    if (changed)
        m_doc->setModified(true);

    qCDebug(lcDialog) << "WorldPropertiesDialog::saveSettings() - saved to WorldDocument";
}

void WorldPropertiesDialog::applySettings()
{
    saveSettings();

    // Emit signal to notify OutputView and other UI components of changes
    if (m_doc) {
        emit m_doc->outputSettingsChanged();

        // Refresh input-view word-wrap. The original committed the new wrap column and
        // then called FixInputWrap() unconditionally (configuration.cpp:1373), which
        // re-runs CSendView::UpdateWrap — the input edit's wrap margin is derived from
        // m_nWrapColumn (sendvw.cpp:3029). InputView::applyInputSettings(), driven by
        // inputSettingsChanged, re-applies the input wrap mode here.
        emit m_doc->inputSettingsChanged();

        // Reconcile the script engine if the script file or enabled flag changed
        // (original CMUSHclientDoc::SavePrefsP17: disable old engine, recreate if
        // enabled, reload script, rediscover entry points).
        m_doc->reinitializeScripting(m_prevScriptFilename, m_prevScriptEnabled);

        // Reconfigure script file watcher in case filename or reload option changed
        m_doc->setupScriptFileWatcher();
    }

    qCDebug(lcDialog)
        << "WorldPropertiesDialog::applySettings() - settings saved and signal emitted";
}

bool WorldPropertiesDialog::isCharacterNameValid() const
{
    // Original CPrefsP21::DoDataExchange (prefspropertypages.cpp:8100-8105): a blank
    // character name is rejected when an auto-connect method is selected, because
    // auto-login would send "connect <blank> <password>". Index 0 = "No auto-connect".
    const bool autoConnect = m_connectMethodCombo->currentData().toInt() != eNoAutoConnect;
    return !(autoConnect && m_nameEdit->text().trimmed().isEmpty());
}

bool WorldPropertiesDialog::validateSettings()
{
    if (!isCharacterNameValid()) {
        QMessageBox::warning(this, "World Properties",
                             "Your character name cannot be blank for auto-connect.");
        m_tabWidget->setCurrentIndex(0); // Connection tab
        m_nameEdit->setFocus();
        return false;
    }

    // Original CPrefsP9::DoDataExchange (prefspropertypages.cpp:4695-4700) rejects
    // speed walking enabled with an empty prefix.
    if (!isSpeedwalkPrefixValid()) {
        QMessageBox::warning(this, "World Properties", "You must supply a speed-walk prefix.");
        m_tabWidget->setCurrentIndex(2); // Input tab
        m_speedwalkPrefixEdit->setFocus();
        return false;
    }

    // Original (prefspropertypages.cpp:4703-4715) rejects command stacking enabled
    // with an empty or non-printable stack character.
    if (!isCommandStackCharacterValid()) {
        const QString stackChar = m_commandStackCharEdit->text();
        QMessageBox::warning(this, "World Properties",
                             stackChar.isEmpty() ? "You must supply a command stack character."
                                                 : "The command stack character is invalid.");
        m_tabWidget->setCurrentIndex(2); // Input tab
        m_commandStackCharEdit->setFocus();
        return false;
    }

    // Original CPrefsP14::DoDataExchange (prefspropertypages.cpp:5789-5813): warn if
    // the output-buffer line count changed, exceeds 1000, and the estimated allocation
    // would exceed physical RAM. The user may continue (Yes) or abort the save (No).
    // saveSettings has not run yet, so m_doc still holds the previous line count.
    if (isMaxLinesMemoryWarningNeeded(m_doc->m_display.max_lines, m_maxLinesSpin->value(),
                                      totalPhysicalMemoryBytes())) {
        const QString msg =
            QStringLiteral(
                "You are allocating %1 lines for your output buffer, but have only %2 Mb of "
                "physical RAM. This is not recommended. Do you wish to continue anyway?")
                .arg(m_maxLinesSpin->value())
                .arg(totalPhysicalMemoryBytes() / 1024 / 1024);
        if (QMessageBox::question(this, "World Properties", msg,
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            m_tabWidget->setCurrentIndex(1); // Output tab
            m_maxLinesSpin->setFocus();
            return false;
        }
    }

    return true;
}

bool WorldPropertiesDialog::isMaxLinesMemoryWarningNeeded(int oldLines, int newLines,
                                                          quint64 totalPhysicalBytes)
{
    // Mirrors prefspropertypages.cpp:5789-5811 exactly: only when the count changed
    // and exceeds 1000, estimate 16 MB for the OS plus 60 bytes per line and warn if
    // that exceeds physical RAM. A zero RAM reading (query failed) suppresses the warning.
    if (oldLines == newLines || newLines <= 1000 || totalPhysicalBytes == 0)
        return false;
    const quint64 bytesNeeded = 16ULL * 1024ULL * 1024ULL + static_cast<quint64>(newLines) * 60ULL;
    return bytesNeeded > totalPhysicalBytes;
}

quint64 WorldPropertiesDialog::totalPhysicalMemoryBytes()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_BSD4)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize = 0;
    size_t len = sizeof(memsize);
    if (sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0)
        return static_cast<quint64>(memsize);
    return 0;
#elif defined(Q_OS_UNIX)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0)
        return static_cast<quint64>(pages) * static_cast<quint64>(pageSize);
    return 0;
#else
    return 0;
#endif
}

bool WorldPropertiesDialog::isSpeedwalkPrefixValid() const
{
    // Mirrors original: only enforced when speed walking is enabled.
    return !(m_enableSpeedwalkCheck->isChecked() && m_speedwalkPrefixEdit->text().isEmpty());
}

bool WorldPropertiesDialog::isCommandStackCharacterValid() const
{
    // Only enforced when command stacking is enabled.
    if (!m_enableCommandStackCheck->isChecked())
        return true;
    const QString stackChar = m_commandStackCharEdit->text();
    if (stackChar.isEmpty())
        return false;
    // ::isprint over the first character: printable ASCII including space (0x20-0x7E).
    const ushort code = stackChar.at(0).unicode();
    return code >= 0x20 && code <= 0x7E;
}

void WorldPropertiesDialog::onOkClicked()
{
    qCDebug(lcDialog) << "WorldPropertiesDialog: OK clicked";
    if (!validateSettings())
        return;
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
    if (!validateSettings())
        return;
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

int WorldPropertiesDialog::computeAdjustedWrapColumn(int viewWidth, int pixelOffset,
                                                     int avgCharWidth)
{
    // Original CPrefsP14::OnAdjustWidth (prefspropertypages.cpp:5937-5943):
    //   iWidth = (outputWindowWidth - pixelOffset) / averageCharWidth, clamped [20, MAX].
    // MAX_LINE_WIDTH lives in config_options.cpp (not the header); inline the value.
    constexpr int kMaxLineWidth = 32000;
    if (avgCharWidth <= 0)
        return 20;
    int column = (viewWidth - pixelOffset) / avgCharWidth;
    if (column < 20)
        column = 20;
    if (column > kMaxLineWidth)
        column = kMaxLineWidth;
    return column;
}

void WorldPropertiesDialog::onAdjustWidthClicked()
{
    // Mirror CPrefsP14::OnAdjustWidth: size the wrap column to the first/active output
    // window using the chosen output font's average character width.
    if (!m_doc)
        return;
    IOutputView* view = m_doc->activeOutputView();
    if (!view)
        return;

    const int avgCharWidth = QFontMetrics(m_outputFont).averageCharWidth();
    const int column =
        computeAdjustedWrapColumn(view->viewWidth(), m_doc->m_display.pixel_offset, avgCharWidth);
    m_wrapColumnSpin->setValue(column);
}

void WorldPropertiesDialog::onAdjustToWidthClicked()
{
    // Mirror CPrefsP14::OnAdjustToWidth: resize the output window so it is exactly wide
    // enough for the current wrap column. The original resizes the MDI child frame; here
    // we resize the output view's top-level window, adjusting only its width.
    if (!m_doc)
        return;
    IOutputView* view = m_doc->activeOutputView();
    if (!view)
        return;

    int column = m_wrapColumnSpin->value();
    if (column < 20)
        column = 20;

    const int avgCharWidth = QFontMetrics(m_outputFont).averageCharWidth();
    QWidget* parent = view->parentWindow();
    QWidget* window = parent ? parent->window() : nullptr;
    if (!window || avgCharWidth <= 0)
        return;

    // Extra chrome (frame/scrollbar) is the difference between the window's current width
    // and the output view's drawable width — analogous to the original's system-metric sum.
    const int chrome = window->width() - view->viewWidth();
    const int newWidth = (avgCharWidth * column) + m_doc->m_display.pixel_offset + chrome;
    window->resize(newWidth, window->height());
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

void WorldPropertiesDialog::updateConnectLineCount()
{
    // Mirror original CPrefsP21::OnUpdateLineCount (prefspropertypages.cpp:8129-8152):
    // count newlines, plus one more for a final line that does not end in a newline.
    const QString text = m_connectTextEdit->toPlainText();
    int count = 0;
    if (!text.isEmpty()) {
        count = static_cast<int>(text.count(QLatin1Char('\n')));
        if (!text.endsWith(QLatin1Char('\n')))
            count++;
    }
    m_connectLineCountLabel->setText(QString("(%1 line%2)").arg(count).arg(count == 1 ? "" : "s"));
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
