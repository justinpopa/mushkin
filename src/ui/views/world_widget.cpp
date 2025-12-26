#include "world_widget.h"
#include "../../automation/plugin.h"         // For plugin callback constants
#include "../../world/accelerator_manager.h" // For keyboard shortcuts
#include "../../world/miniwindow.h"          // For miniwindow signal connection
#include "../world/world_document.h"
#include "../world/xml_serialization.h"
#include "input_view.h"
#include "logging.h"
#include "output_view.h"
#include <QApplication>
#include <QColor>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#ifdef Q_OS_MACOS
#include <QHBoxLayout>
#include <QMdiSubWindow>
#include <QToolButton>
#endif

/**
 * WorldWidget Constructor
 *
 * Creates a new world widget with:
 * - Fresh WorldDocument with default settings
 * - Vertical splitter with output (top) and input (bottom)
 * - Placeholder views (will be replaced in )
 */
WorldWidget::WorldWidget(QWidget* parent)
    : QWidget(parent), m_document(nullptr), m_splitter(nullptr), m_outputView(nullptr),
      m_inputView(nullptr), m_infoBar(nullptr),
#ifdef Q_OS_MACOS
      m_titleBar(nullptr), m_titleLabel(nullptr),
#endif
      m_modified(false), m_connected(false)
{
    // Create the world document
    m_document = new WorldDocument(this);

    // Setup the UI
    setupUi();

    // Initial window title
    updateWindowTitle();
}

/**
 * WorldWidget Destructor
 *
 * The document is deleted automatically because it's a child QObject.
 */
WorldWidget::~WorldWidget()
{
    // QObject parent-child relationship handles cleanup
}

/**
 * Setup the user interface
 *
 * Creates vertical splitter with:
 * - Top pane: QTextEdit (placeholder for OutputView)
 * - Bottom pane: QLineEdit (placeholder for InputView)
 *
 * Splitter ratio: 80% output, 20% input (similar to original)
 */
void WorldWidget::setupUi()
{
    // Create main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#ifdef Q_OS_MACOS
    // Create custom title bar for macOS (replaces QMdiSubWindow's native title bar)
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(22);
    m_titleBar->setStyleSheet("background-color: #383838; border-bottom: 1px solid #555;");

    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(4);

    // Title label (will be updated in updateWindowTitle)
    m_titleLabel = new QLabel("New World", m_titleBar);
    m_titleLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    // Minimize button
    QToolButton* minBtn = new QToolButton(m_titleBar);
    minBtn->setFixedSize(16, 16);
    minBtn->setText("−");
    minBtn->setStyleSheet("QToolButton { border: 1px solid #555; border-radius: 2px; background: transparent; color: #888; font-size: 14px; } QToolButton:hover { background: #444; }");
    connect(minBtn, &QToolButton::clicked, this, [this]() {
        if (QMdiSubWindow* mdi = qobject_cast<QMdiSubWindow*>(parentWidget()))
            mdi->showMinimized();
    });
    titleLayout->addWidget(minBtn);

    // Maximize button
    QToolButton* maxBtn = new QToolButton(m_titleBar);
    maxBtn->setFixedSize(16, 16);
    maxBtn->setText("□");
    maxBtn->setStyleSheet("QToolButton { border: 1px solid #555; border-radius: 2px; background: transparent; color: #888; font-size: 11px; } QToolButton:hover { background: #444; }");
    connect(maxBtn, &QToolButton::clicked, this, [this]() {
        if (QMdiSubWindow* mdi = qobject_cast<QMdiSubWindow*>(parentWidget())) {
            if (mdi->isMaximized())
                mdi->showNormal();
            else
                mdi->showMaximized();
        }
    });
    titleLayout->addWidget(maxBtn);

    // Close button
    QToolButton* closeBtn = new QToolButton(m_titleBar);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setText("×");
    closeBtn->setStyleSheet("QToolButton { border: 1px solid #555; border-radius: 2px; background: transparent; color: #888; font-size: 14px; } QToolButton:hover { background: #633; color: #faa; }");
    connect(closeBtn, &QToolButton::clicked, this, [this]() {
        if (QMdiSubWindow* mdi = qobject_cast<QMdiSubWindow*>(parentWidget()))
            mdi->close();
    });
    titleLayout->addWidget(closeBtn);

    layout->addWidget(m_titleBar);
#endif

    // Create info bar (script-controllable status display)
    m_infoBar = new QLabel(this);
    m_infoBar->setWordWrap(true);
    m_infoBar->setTextFormat(Qt::PlainText);
    m_infoBar->setMinimumHeight(20);
    m_infoBar->setContentsMargins(4, 2, 4, 2);
    m_infoBar->hide(); // Hidden by default until ShowInfoBar(true)
    layout->addWidget(m_infoBar);

    // Create vertical splitter
    m_splitter = new QSplitter(Qt::Vertical, this);

    // Create output view (custom text display widget)
    m_outputView = new OutputView(m_document, m_splitter);

    // Set OutputView pointer for miniwindow signal connection
    m_document->m_pActiveOutputView = m_outputView;
    qCDebug(lcUI) << "WorldWidget::setupUi: Set m_pActiveOutputView =" << (void*)m_outputView
                  << "in WorldDocument" << (void*)m_document;

    // Create input view (custom input widget with history)
    m_inputView = new InputView(m_document, m_splitter);

    // Add to splitter
    m_splitter->addWidget(m_outputView);
    m_splitter->addWidget(m_inputView);

    // Set splitter proportions (80% output, 20% input)
    m_splitter->setStretchFactor(0, 4); // Output gets 4 parts
    m_splitter->setStretchFactor(1, 1); // Input gets 1 part

    // Add splitter to layout
    layout->addWidget(m_splitter);

    // Connect input signal (commandEntered emitted when Enter pressed without Shift)
    connect(m_inputView, &InputView::commandEntered, this, &WorldWidget::sendCommand);

    // Connect command text changed signal for plugin notification
    // Use a static flag to prevent recursion (matches original sendvw.cpp behavior)
    connect(m_inputView, &InputView::commandTextChanged, this, [this](const QString& text) {
        static bool doing_change = false;
        if (!doing_change && m_document) {
            doing_change = true;
            m_document->SendToAllPluginCallbacks(ON_PLUGIN_COMMAND_CHANGED);
            doing_change = false;
        }
    });

    // Connect document signals
    connect(m_document, &WorldDocument::connectionStateChanged, this,
            [this](bool connected) { setConnected(connected); });

    // Connect output settings signal
    connect(m_document, &WorldDocument::outputSettingsChanged, this, [this]() {
        QFont font(m_document->m_font_name, m_document->m_font_height);
        m_outputView->setOutputFont(font);
    });

    // Connect input settings signal
    connect(m_document, &WorldDocument::inputSettingsChanged, m_inputView,
            &InputView::applyInputSettings);

    // Connect pasteToCommand signal to insert text into input field
    connect(m_document, &WorldDocument::pasteToCommand, m_inputView,
            &QPlainTextEdit::insertPlainText);

    // Connect info bar signal
    connect(m_document, &WorldDocument::infoBarChanged, this, &WorldWidget::updateInfoBar);

    // Connect miniwindow needsRedraw signals to OutputView update
    connect(m_document, &WorldDocument::miniwindowCreated, this, [this](MiniWindow* iwin) {
        // Cast to concrete MiniWindow to access Qt signals
        MiniWindow* win = iwin;
        if (win && m_outputView) {
            connect(win, &MiniWindow::needsRedraw, m_outputView, QOverload<>::of(&QWidget::update));
        }
    });

    // Forward notepad creation signal to parent (for MDI integration)
    connect(m_document, &WorldDocument::notepadCreated, this, &WorldWidget::notepadRequested);

    // Setup accelerator manager - set parent widget and connect signal
    m_document->m_acceleratorManager->setParentWidget(this);
    connect(m_document->m_acceleratorManager, &AcceleratorManager::acceleratorTriggered, this,
            [this](const QString& action, int sendTo) {
                // Handle accelerator execution based on sendTo type
                // eSendToExecute = 12 (normal command execution)
                if (sendTo == 12) {
                    m_document->Execute(action);
                } else {
                    // For other sendTo types, use sendTo() method
                    QString output;
                    m_document->sendTo(sendTo, action, false, false, QString(), QString(), output);
                }
            });

    // Focus on input
    m_inputView->setFocus();
}

/**
 * Get the world name from document
 */
QString WorldWidget::worldName() const
{
    if (m_document && !m_document->m_mush_name.isEmpty()) {
        return m_document->m_mush_name;
    }
    return "New World";
}

/**
 * Get the server address from document
 */
QString WorldWidget::serverAddress() const
{
    if (m_document && !m_document->m_server.isEmpty()) {
        return QString("%1:%2").arg(m_document->m_server).arg(m_document->m_port);
    }
    return "";
}

/**
 * Set modified state
 */
void WorldWidget::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
        updateWindowTitle();
    }
}

/**
 * Set connection state
 */
void WorldWidget::setConnected(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit connectedChanged(connected);
        updateWindowTitle();
    }
}

/**
 * Update window title
 *
 * Format: "World Name [*] - Status"
 * - [*] appears if modified
 * - Status is "Connected" or "Disconnected"
 */
void WorldWidget::updateWindowTitle()
{
    QString title = worldName();

    // Add modified indicator
    if (m_modified) {
        title += " *";
    }

    // Add connection status
    if (m_connected) {
        title += " - Connected";
    } else {
        title += " - Disconnected";
    }

    setWindowTitle(title);
    emit windowTitleChanged(title);

#ifdef Q_OS_MACOS
    // Update custom title bar label
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
#endif
}

/**
 * Update the info bar appearance from document state
 *
 * Called when document's infoBarChanged() signal is emitted.
 * Updates text, colors, font, and visibility.
 */
void WorldWidget::updateInfoBar()
{
    if (!m_infoBar || !m_document) {
        return;
    }

    // Update text
    m_infoBar->setText(m_document->m_infoBarText);

    // Build stylesheet for colors and font
    QString styleSheet;

    // Colors
    QColor textColor(m_document->m_infoBarTextColor);
    QColor backColor(m_document->m_infoBarBackColor);
    styleSheet +=
        QString("color: %1; background-color: %2;").arg(textColor.name()).arg(backColor.name());

    // Font
    QString fontStyle;
    if (m_document->m_infoBarFontStyle & 1) { // Bold
        fontStyle += "font-weight: bold;";
    }
    if (m_document->m_infoBarFontStyle & 2) { // Italic
        fontStyle += "font-style: italic;";
    }
    if (m_document->m_infoBarFontStyle & 4) { // Underline
        fontStyle += "text-decoration: underline;";
    }
    if (m_document->m_infoBarFontStyle & 8) { // Strikeout
        fontStyle += "text-decoration: line-through;";
    }

    styleSheet += QString("font-family: '%1'; font-size: %2pt;")
                      .arg(m_document->m_infoBarFontName)
                      .arg(m_document->m_infoBarFontSize);
    styleSheet += fontStyle;

    m_infoBar->setStyleSheet(styleSheet);

    // Update visibility
    m_infoBar->setVisible(m_document->m_infoBarVisible);
}

/**
 * Load world from .mcl file
 *
 * Uses XML serialization from
 */
bool WorldWidget::loadFromFile(const QString& filename)
{
    if (!m_document) {
        return false;
    }

    // Load XML
    if (!XmlSerialization::LoadWorldXML(m_document, filename)) {
        return false;
    }

    // Store filename (both in widget and document for Lua API access)
    m_filename = filename;
    m_document->m_strWorldFilePath = filename;
    m_modified = false;

    // Update UI
    updateWindowTitle();

    // Apply loaded settings to views (fonts, colors, etc.)
    // This triggers the output and input views to refresh with the new settings from the XML
    if (m_outputView && m_document) {
        QFont font(m_document->m_font_name, m_document->m_font_height);
        m_outputView->setOutputFont(font);
    }
    if (m_inputView && m_document) {
        m_inputView->applyInputSettings();
    }

    // Load external script file if configured
    // This executes the script file specified in the world properties
    m_document->loadScriptFile();

    // Set up file watcher for script file changes
    // This will monitor the script file and reload it based on m_nReloadOption
    m_document->setupScriptFileWatcher();

    // Note: Welcome messages will be shown when OutputView is connected to document
    // Text will appear when StartNewLine() is called from network data

    return true;
}

/**
 * Save world to .mcl file
 *
 * Uses XML serialization from
 */
bool WorldWidget::saveToFile(const QString& filename)
{
    if (!m_document) {
        return false;
    }

    // Save XML
    if (!XmlSerialization::SaveWorldXML(m_document, filename)) {
        return false;
    }

    // Store filename (both in widget and document for Lua API access)
    m_filename = filename;
    m_document->m_strWorldFilePath = filename;
    m_modified = false;

    // Update UI
    updateWindowTitle();

    return true;
}

/**
 * Send command
 *
 * Command History Integration
 * Command Execution Pipeline
 * Command Stacking
 *
 * Called when user presses Enter in the input field.
 * Processes command through Execute() which handles:
 * - Command stacking (e.g., "north;south;east")
 * - Alias evaluation
 * - Sending to MUD via SendMsg()
 *
 * Based on CSendView::SendCommand() from sendvw.cpp and
 * CMUSHclientDoc::Execute() from methods_commands.cpp
 */
void WorldWidget::sendCommand()
{
    QString command = m_inputView->text();

    if (!m_document) {
        return;
    }

    // ========== Plugin Command Entered Callback ==========
    // Notify plugins that a command is about to be entered
    // Note: Original uses SendToAllPluginCallbacksRtn which can modify command
    // For now, we just notify without modification
    m_document->SendToAllPluginCallbacks(ON_PLUGIN_COMMAND_ENTERED, command, false);

    // ========== Unpause on Send ==========
    // Based on mushview.cpp and sendvw.cpp
    if (m_document->m_bUnpauseOnSend && m_outputView && m_outputView->isFrozen()) {
        m_outputView->setFrozen(false);
    }

    // ========== Auto-Say Processing ==========
    // Complete Auto-Say implementation matching CSendView::SendCommand() from sendvw.cpp
    // This handles all edge cases: override prefix, exclude non-alpha, exclude macros,
    // self-exclusion, multi-line processing, and re-evaluation

    bool bAutoSay = m_document->m_bEnableAutoSay;

    // 1. Check for override prefix - if present, disable auto-say and remove prefix
    if (bAutoSay && !m_document->m_strOverridePrefix.isEmpty()) {
        if (command.startsWith(m_document->m_strOverridePrefix)) {
            bAutoSay = false;
            command = command.mid(m_document->m_strOverridePrefix.length());
        }
    }

    // 2. Exclude non-alphanumeric commands (e.g., "#", ".", etc.) if configured
    if (!command.isEmpty() && bAutoSay && m_document->m_bExcludeNonAlpha) {
        QChar firstChar = command[0];
        if (!firstChar.isLetterOrNumber()) {
            bAutoSay = false;
        }
    }

    // 3. Exclude macro commands if configured
    if (bAutoSay && m_document->m_bExcludeMacros) {
        // Check if command starts with any configured macro
        // Note: Macros are stored in m_macros array, checked in original sendvw.cpp
        // For now, we don't have macro support implemented, so this is a placeholder
        // TODO: Implement macro exclusion when macro system is added
    }

    // 4. Exclude if command already starts with auto-say string (prevent "say say Hello!")
    if (bAutoSay && command.startsWith(m_document->m_strAutoSayString)) {
        bAutoSay = false;
    }

    // 5. If auto-say still enabled, process with special handling
    if (bAutoSay) {
        // Check connection if not re-evaluating (original sendvw.cpp)
        if (!m_document->m_bReEvaluateAutoSay &&
            m_document->m_iConnectPhase != eConnectConnectedToMud) {
            return; // Don't send if not connected
        }

        // Split command on newlines for multi-line processing (original sendvw.cpp)
        QStringList lines = command.split('\n', Qt::KeepEmptyParts);

        // Temporarily disable auto-say and command stacking to prevent loops
        // (original sendvw.cpp)
        bool savedAutoSay = m_document->m_bEnableAutoSay;
        bool savedCommandStack = m_document->m_enable_command_stack;
        m_document->m_bEnableAutoSay = false;
        m_document->m_enable_command_stack = false;

        // Process each line (original sendvw.cpp)
        for (const QString& line : lines) {
            if (m_document->m_bReEvaluateAutoSay) {
                // Re-evaluate mode: prepend auto-say string and call Execute()
                // This allows aliases/triggers to process the "say" command
                // (original sendvw.cpp)
                m_document->m_iExecutionDepth = 0; // Hand-typed command, depth zero
                QString sayCommand = m_document->m_strAutoSayString + line;
                m_document->Execute(sayCommand);
            } else {
                // Direct send mode: prepend auto-say string and call SendMsg()
                // This sends directly without alias/trigger processing
                // (original sendvw.cpp)
                m_document->SendMsg(m_document->m_strAutoSayString + line,
                                    m_document->m_display_my_input, false, m_document->m_log_input);
            }
        }

        // Re-enable auto-say and command stacking (original sendvw.cpp)
        m_document->m_bEnableAutoSay = savedAutoSay;
        m_document->m_enable_command_stack = savedCommandStack;

        // Early return - don't execute the normal command path
        // Clear input and return
        if (m_document->m_bAutoRepeat && !m_document->m_bNoEcho) {
            m_inputView->selectAll();
        } else {
            m_inputView->clear();
        }
        setModified(true);
        return;
    }

    // ========== Normal Command Execution ==========
    // If auto-say is disabled or was disabled by exclusion rules, execute normally
    // Execute() handles command stacking, alias evaluation, and sending
    // Note: Empty commands are allowed - sends blank line to MUD (matches original MUSHclient)
    m_document->Execute(command);

    // ========== Clear Input ==========
    // Clear input after sending (configurable)
    // m_bAutoRepeat allows command to stay in field for easy repetition
    // Based on CSendView::SendCommand() from sendvw.cpp
    if (m_document->m_bAutoRepeat && !m_document->m_bNoEcho) {
        // Keep command and select all (so typing replaces it)
        m_inputView->selectAll();
    } else {
        // Clear the input field
        m_inputView->clear();
    }

    // Mark as modified (command or alias changed state)
    setModified(true);
}

/**
 * Connect to MUD
 *
 * Initiates connection to the MUD server configured in the document.
 */
void WorldWidget::connectToMud()
{
    if (!m_document) {
        return;
    }

    m_document->connectToMud();
}

/**
 * Disconnect from MUD
 *
 * Closes the connection to the MUD server.
 */
void WorldWidget::disconnectFromMud()
{
    if (!m_document) {
        return;
    }

    m_document->disconnectFromMud();
}

/**
 * Activate Input Area (Focus the input field)
 *
 * Based on: CMUSHView::OnKeysActivatecommandview() from mushview.cpp
 *
 * Sets keyboard focus to the input field, allowing the user to
 * immediately start typing commands without clicking.
 *
 * Triggered by:
 * - Input → Activate Input Area menu item
 * - Tab key
 * - Escape key
 */
void WorldWidget::activateInputArea()
{
    if (m_inputView) {
        m_inputView->setFocus();
    }
}

/**
 * Previous Command - Navigate backward in command history
 *
 * Input menu
 * Wrapper for InputView::recallPreviousCommand()
 */
void WorldWidget::previousCommand()
{
    if (m_inputView) {
        m_inputView->previousCommand();
    }
}

/**
 * Next Command - Navigate forward in command history
 *
 * Input menu
 * Wrapper for InputView::recallNextCommand()
 */
void WorldWidget::nextCommand()
{
    if (m_inputView) {
        m_inputView->nextCommand();
    }
}

/**
 * Repeat Last Command - Re-send the most recent command
 *
 * Input menu
 *
 * Retrieves the last command from history and executes it.
 * Useful for quickly repeating actions without typing.
 */
void WorldWidget::repeatLastCommand()
{
    if (!m_document) {
        return;
    }

    // Get last command from history
    if (m_document->m_commandHistory.isEmpty()) {
        return;
    }

    QString lastCommand = m_document->m_commandHistory.last();

    // Execute it
    m_document->Execute(lastCommand);
}

/**
 * Clear Command History - Empty the command history
 *
 * Input menu
 * Wrapper for WorldDocument::clearCommandHistory()
 */
void WorldWidget::clearCommandHistory()
{
    if (m_document) {
        m_document->clearCommandHistory();
    }
}

/**
 * Handle keyboard shortcuts
 *
 * Based on: mushview.cpp handling Tab and Escape keys
 *
 * Tab and Escape keys activate the input area (set focus to input field).
 * This matches original MUSHclient behavior where these keys always
 * return focus to the command line.
 */
void WorldWidget::keyPressEvent(QKeyEvent* event)
{
    // Tab and Escape activate input area
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Escape) {
        activateInputArea();
        event->accept();
        return;
    }

    // Page Up/Down scroll the output view
    if (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown ||
        ((event->key() == Qt::Key_Home || event->key() == Qt::Key_End) &&
         (event->modifiers() & Qt::ControlModifier)) ||
        ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) &&
         (event->modifiers() & Qt::ControlModifier))) {
        // Forward to output view for scrolling
        if (m_outputView) {
            QApplication::sendEvent(m_outputView, event);
            event->accept();
            return;
        }
    }

    // Pass other keys to base class
    QWidget::keyPressEvent(event);
}
