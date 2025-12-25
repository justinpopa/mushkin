#include "debug_lua_dialog.h"
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

DebugLuaDialog::DebugLuaDialog(QWidget* parent)
    : QDialog(parent), m_aborted(false), m_currentLineEdit(nullptr), m_functionNameEdit(nullptr),
      m_sourceEdit(nullptr), m_whatEdit(nullptr), m_linesEdit(nullptr), m_nupsEdit(nullptr),
      m_commandEdit(nullptr), m_executeButton(nullptr), m_showLocalsButton(nullptr),
      m_showUpvaluesButton(nullptr), m_tracebackButton(nullptr), m_abortButton(nullptr),
      m_continueButton(nullptr)
{
    setWindowTitle("Lua Debugger");
    setModal(true);
    setMinimumSize(450, 350);
    resize(550, 400);
    setupUi();
}

void DebugLuaDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Use monospace font for debug info display
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::TypeWriter);
    monoFont.setPointSize(9);

    // Debug information group
    QGroupBox* infoGroup = new QGroupBox("Debug Information", this);
    QFormLayout* formLayout = new QFormLayout(infoGroup);

    // Current line
    m_currentLineEdit = new QLineEdit(this);
    m_currentLineEdit->setReadOnly(true);
    m_currentLineEdit->setFont(monoFont);
    formLayout->addRow("Current &Line:", m_currentLineEdit);

    // Function name
    m_functionNameEdit = new QLineEdit(this);
    m_functionNameEdit->setReadOnly(true);
    m_functionNameEdit->setFont(monoFont);
    formLayout->addRow("&Function Name:", m_functionNameEdit);

    // Source file
    m_sourceEdit = new QLineEdit(this);
    m_sourceEdit->setReadOnly(true);
    m_sourceEdit->setFont(monoFont);
    formLayout->addRow("&Source:", m_sourceEdit);

    // What type
    m_whatEdit = new QLineEdit(this);
    m_whatEdit->setReadOnly(true);
    m_whatEdit->setFont(monoFont);
    formLayout->addRow("&What:", m_whatEdit);

    // Line range
    m_linesEdit = new QLineEdit(this);
    m_linesEdit->setReadOnly(true);
    m_linesEdit->setFont(monoFont);
    formLayout->addRow("Li&nes:", m_linesEdit);

    // Number of upvalues
    m_nupsEdit = new QLineEdit(this);
    m_nupsEdit->setReadOnly(true);
    m_nupsEdit->setFont(monoFont);
    formLayout->addRow("&Upvalues:", m_nupsEdit);

    mainLayout->addWidget(infoGroup);

    // Command input section
    QGroupBox* commandGroup = new QGroupBox("Debug Command", this);
    QVBoxLayout* commandLayout = new QVBoxLayout(commandGroup);

    QHBoxLayout* commandInputLayout = new QHBoxLayout();
    QLabel* commandLabel = new QLabel("&Command:", this);
    m_commandEdit = new QLineEdit(this);
    m_commandEdit->setFont(monoFont);
    m_commandEdit->setPlaceholderText("Enter debug command...");
    commandLabel->setBuddy(m_commandEdit);

    m_executeButton = new QPushButton("&Execute", this);
    m_executeButton->setToolTip("Execute the debug command");

    commandInputLayout->addWidget(commandLabel);
    commandInputLayout->addWidget(m_commandEdit, 1);
    commandInputLayout->addWidget(m_executeButton);

    commandLayout->addLayout(commandInputLayout);
    mainLayout->addWidget(commandGroup);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_showLocalsButton = new QPushButton("Show &Locals", this);
    m_showLocalsButton->setToolTip("Display local variables");
    buttonLayout->addWidget(m_showLocalsButton);

    m_showUpvaluesButton = new QPushButton("Show &Upvalues", this);
    m_showUpvaluesButton->setToolTip("Display upvalues");
    buttonLayout->addWidget(m_showUpvaluesButton);

    m_tracebackButton = new QPushButton("&Traceback", this);
    m_tracebackButton->setToolTip("Show stack traceback");
    buttonLayout->addWidget(m_tracebackButton);

    buttonLayout->addStretch();

    m_abortButton = new QPushButton("&Abort", this);
    m_abortButton->setToolTip("Abort script execution");
    buttonLayout->addWidget(m_abortButton);

    m_continueButton = new QPushButton("&Continue", this);
    m_continueButton->setToolTip("Continue execution");
    m_continueButton->setDefault(true);
    buttonLayout->addWidget(m_continueButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_executeButton, &QPushButton::clicked, this, &DebugLuaDialog::onExecute);
    connect(m_showLocalsButton, &QPushButton::clicked, this, &DebugLuaDialog::onShowLocals);
    connect(m_showUpvaluesButton, &QPushButton::clicked, this, &DebugLuaDialog::onShowUpvalues);
    connect(m_tracebackButton, &QPushButton::clicked, this, &DebugLuaDialog::onTraceback);
    connect(m_abortButton, &QPushButton::clicked, this, &DebugLuaDialog::onAbort);
    connect(m_continueButton, &QPushButton::clicked, this, &DebugLuaDialog::onContinue);

    // Enter key in command edit executes the command
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &DebugLuaDialog::onExecute);

    // Focus the command input
    m_commandEdit->setFocus();
}

// Setters
void DebugLuaDialog::setCurrentLine(const QString& line)
{
    m_currentLineEdit->setText(line);
}

void DebugLuaDialog::setFunctionName(const QString& name)
{
    m_functionNameEdit->setText(name);
}

void DebugLuaDialog::setFunctionDetails(const QString& details)
{
    // Function details are not displayed separately in this UI
    // They can be shown in the function name field or command output
}

void DebugLuaDialog::setSource(const QString& source)
{
    m_sourceEdit->setText(source);
}

void DebugLuaDialog::setWhat(const QString& what)
{
    m_whatEdit->setText(what);
}

void DebugLuaDialog::setLines(const QString& lines)
{
    m_linesEdit->setText(lines);
}

void DebugLuaDialog::setNups(const QString& nups)
{
    m_nupsEdit->setText(nups);
}

// Getters
QString DebugLuaDialog::command() const
{
    return m_command;
}

bool DebugLuaDialog::wasAborted() const
{
    return m_aborted;
}

// Slots
void DebugLuaDialog::onExecute()
{
    m_command = m_commandEdit->text();
    if (!m_command.isEmpty()) {
        emit executeCommand(m_command);
        m_commandEdit->clear();
    }
}

void DebugLuaDialog::onShowLocals()
{
    emit showLocals();
}

void DebugLuaDialog::onShowUpvalues()
{
    emit showUpvalues();
}

void DebugLuaDialog::onTraceback()
{
    emit showTraceback();
}

void DebugLuaDialog::onAbort()
{
    m_aborted = true;
    emit abortExecution();
    reject();
}

void DebugLuaDialog::onContinue()
{
    m_aborted = false;
    emit continueExecution();
    accept();
}
