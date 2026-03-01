#include "world_document.h"
#include "../automation/alias.h"
#include "../automation/plugin.h"
#include "../automation/timer.h"
#include "../automation/trigger.h"
#include "../automation/variable.h"
#include "../network/remote_access_server.h"
#include "../text/action.h"
#include "../text/line.h"
#include "../text/style.h"
#include "accelerator_manager.h"
#include "logging.h"
#include "miniwindow.h" // MiniWindow (complete type for unique_ptr destructor)
#include "script_engine.h"
#include "sound_manager.h"    // SoundManager (complete type for make_unique + destructor)
#include "speedwalk_engine.h" // speedwalk::evaluate/reverse/removeBacktracks
#include "view_interfaces.h"
#include "world_socket.h"
#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QThread>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <array>   // for std::array
#include <cstring> // for memcpy, strlen

#include "color_utils.h"

// Default ANSI colors in BGR format (Windows COLORREF)
// These match the original MUSHclient values shown in aardwolf_colors.lua
static const QRgb DEFAULT_NORMAL_COLORS[8] = {
    BGR(0, 0, 0),      // BLACK      - 0x000000
    BGR(128, 0, 0),    // RED        - 0x000080
    BGR(0, 128, 0),    // GREEN      - 0x008000
    BGR(128, 128, 0),  // YELLOW     - 0x008080
    BGR(0, 0, 128),    // BLUE       - 0x800000
    BGR(128, 0, 128),  // MAGENTA    - 0x800080
    BGR(0, 128, 128),  // CYAN       - 0x808000
    BGR(192, 192, 192) // WHITE      - 0xC0C0C0
};

static const QRgb DEFAULT_BOLD_COLORS[8] = {
    BGR(128, 128, 128), // BLACK (bright)  - 0x808080
    BGR(255, 0, 0),     // RED (bright)    - 0x0000FF
    BGR(0, 255, 0),     // GREEN (bright)  - 0x00FF00
    BGR(255, 255, 0),   // YELLOW (bright) - 0x00FFFF
    BGR(0, 0, 255),     // BLUE (bright)   - 0xFF0000
    BGR(255, 0, 255),   // MAGENTA (bright)- 0xFF00FF
    BGR(0, 255, 255),   // CYAN (bright)   - 0xFFFF00
    BGR(255, 255, 255)  // WHITE (bright)  - 0xFFFFFF
};


WorldDocument::WorldDocument(QObject* parent) : QObject(parent)
{
    // Create AutomationRegistry companion FIRST — other companions may call into it.
    m_automationRegistry = std::make_unique<AutomationRegistry>(*this);

    // Create TelnetParser companion FIRST — many other init sites below delegate to it.
    m_telnetParser = std::make_unique<TelnetParser>(*this);

    // Create MXPEngine companion — depends on TelnetParser existing (accesses
    // m_telnetParser->m_phase).
    m_mxpEngine = std::make_unique<MXPEngine>(*this);

    // Create SoundManager companion — audio engine is lazy-loaded on first playSound() call.
    m_soundManager = std::make_unique<SoundManager>(*this);

    // Create ConnectionManager companion — owns connection state, statistics, timing, queue.
    m_connectionManager = std::make_unique<ConnectionManager>(*this);

    // Create OutputFormatter companion — owns note/colour/hyperlink output formatting.
    m_outputFormatter = std::make_unique<OutputFormatter>(*this);

    // Create the network socket.
    // WorldSocket calls our methods directly via m_pDoc pointer:
    // - onReadyRead()  → ReceiveMsg()
    // - onConnected()  → OnConnect(0) → m_connectionManager->onConnect(0)
    // - onError()      → OnConnect(errorCode) → m_connectionManager->onConnect(errorCode)
    // - onDisconnected() → OnConnectionDisconnect() → m_connectionManager->onConnectionDisconnect()
    // The socket is stored in m_connectionManager->m_pSocket (non-owning; Qt parent-child).
    m_connectionManager->m_pSocket = new WorldSocket(this, this);

    // ========== Initialize connection settings ==========
    m_server = QString();
    m_mush_name = QString();
    m_name = QString();
    m_password = QString();
    m_port = 4000; // Default MUD port
    m_connect_now = false;

    // ========== Initialize display settings ==========
    // m_display uses default member initializers (DisplayConfig)
    // m_output uses default member initializers (OutputConfig)

    // ========== Initialize colors ==========
    initializeColors();

    // ========== Initialize input configuration ==========
    // (InputConfig m_input uses default member initializers)

    // ========== Initialize triggers, aliases, timers - Enable Flags ==========
    m_enable_aliases = true;
    m_enable_triggers = true;
    m_bEnableTimers = true;

    // ========== Initialize Trigger/Alias/Timer Collections ==========
    // All collections are owned by m_automationRegistry (auto-initialized by its constructor).
    // m_triggersNeedSorting and m_aliasesNeedSorting default to false in AutomationRegistry.

    // ========== Initialize input handling ==========
    m_display_my_input = true;
    // m_output.echo_colour uses default member initializer (65535 = SAMECOLOUR)

    // ========== Initialize command history ==========
    m_commandHistory.clear();
    m_maxCommandHistory = 20; // Default: 20 commands (matches original)
    m_historyPosition = 0;
    m_bFilterDuplicates = false;                 // Don't filter duplicates by default
    m_last_command = QString();                  // No previous command yet
    m_iHistoryStatus = HistoryStatus::eAtBottom; // Start at bottom (ready for new commands)

    // (SoundConfig m_sound uses default member initializers)

    // ========== Initialize macros ==========
    m_macros.fill(QString());
    m_macro_type.fill(0); // SEND_NOW
    m_macro_name.fill(QString());

    // ========== Initialize keypad ==========
    m_keypad.fill(QString());
    m_keypad_enable = false;

    // ========== Initialize connection text ==========
    m_connect_text = QString();

    // ========== Initialize notes ==========
    m_notes = QString();

    // ========== Initialize scripting ==========
    // (ScriptConfig m_scripting uses default member initializers)

    // ========== Initialize MXP ==========
    m_iUseMXP = MXPMode::eMXP_On; // eOnCommandMXP - Default: on command (like original MUSHclient)
    m_iMXPdebugLevel = 0;
    m_strOnMXP_Start = QString();
    m_strOnMXP_Stop = QString();
    m_strOnMXP_Error = QString();
    m_strOnMXP_OpenTag = QString();
    m_strOnMXP_CloseTag = QString();
    m_strOnMXP_SetVariable = QString();

    // ========== Initialize misc flags ==========
    m_bSaveWorldAutomatically = false;
    m_bStartPaused = false;
    // m_colors uses default member initializers (ColorConfig)
    // Array members (normal_colour, bold_colour, custom_text, custom_back, custom_colour_name)
    // are initialized by initializeColors() called below.

    // ========== Initialize auto-say ==========
    // (defaults provided by AutoSayConfig member initializers)

    // ========== Initialize Script Variables Collection ==========
    // m_VariableMap is automatically initialized as empty QMap

    // ========== Initialize print styles ==========
    m_nNormalPrintStyle.fill(0);
    m_nBoldPrintStyle.fill(0);

    // ========== Initialize display options (version 9+) ==========
    // m_display uses default member initializers (DisplayConfig)
    m_bAutoRepeat = false;
    m_bDisableCompression = false;
    m_bConfirmOnSend = true; // Default: true (like original)
    m_bTranslateGerman = false;

    // m_command_window uses default member initializers (CommandWindowConfig)
    // m_scripting.tab_complete_functions = true (default member initializer)
    // m_ExtraShiftTabCompleteItems initialized empty by default

    // ========== Initialize auto logging ==========
    // (LoggingConfig m_logging uses default member initializers)

    // ========== Initialize output line preambles and recall ==========
    // m_output uses default member initializers (OutputConfig)

    // ========== Initialize miscellaneous sending flags ==========
    // (PasteSendConfig m_paste uses default member initializers)

    // ========== Initialize miscellaneous options ==========
    // m_scripting.reload_option, edit_with_notepad, warn_if_inactive: default member initializers
    m_bUseDefaultOutputFont = 0;
    m_bTranslateBackslashSequences = 0;

    // ========== Initialize default options ==========
    m_bUseDefaultTriggers = false;
    m_bUseDefaultAliases = false;
    m_bUseDefaultMacros = false;
    m_bUseDefaultTimers = false;
    m_bUseDefaultInputFont = false;

    // ========== Initialize terminal settings ==========
    m_strTerminalIdentification = "mushkin"; // Identify as Mushkin

    // ========== Initialize flag containers ==========
    m_iFlags1 = 0;
    m_iFlags2 = 0;

    // ========== Initialize world ID ==========
    m_strWorldID = QString(); // Will be generated on save

    // ========== Initialize more options (version 15+) ==========
    m_bAlwaysRecordCommandHistory = false;
    m_bSendMXP_AFK_Response = true;
    m_bMudCanChangeOptions = true;
    // (m_spam uses default member initializers)

    // ========== Initialize clipboard and display ==========
    m_iCurrentActionSource = ActionSource::eUnknownActionSource;

    // ========== Initialize filters ==========
    // (m_scripting.*_filter use default member initializers)

    // ========== Initialize script errors ==========
    // (m_scripting.errors_to_output_window uses default member initializer)

    // ========== Initialize editor window ==========
    m_strEditorWindowName = QString();
    m_bSendKeepAlives = false;
    // (CommandWindowConfig m_command_window uses default member initializers)

    // (AutomationDefaultsConfig m_automation_defaults uses default member initializers)

    // ========== Initialize HTML logging ==========
    m_bUnpauseOnSend = false;
    // (LoggingConfig m_logging bool flags use default member initializers)

    // ========== Initialize tree views ==========
    m_bTreeviewTriggers = true;
    m_bTreeviewAliases = true;
    m_bTreeviewTimers = true;

    // ========== Initialize tooltips ==========
    m_iToolTipVisibleTime = 30000; // 30 seconds
    m_iToolTipStartTime = 500;     // 0.5 seconds

    // ========== Initialize save file options ==========
    m_bOmitSavedDateFromSaveFiles = false;

    // m_output fading fields use default member initializers (OutputConfig)
    m_bCtrlBackspaceDeletesLastWord = false;

    // m_remote fields use default member initializers (RemoteAccessConfig struct)
    // m_pRemoteServer is automatically nullptr via unique_ptr

    // ========== RUNTIME STATE VARIABLES (not saved to disk) ==========

    // ========== Deprecated/Legacy (pre-version 11) ==========
    m_page_colour = 0;
    m_whisper_colour = 0;
    m_mail_colour = 0;
    m_game_colour = 0;
    m_remove_channels1 = false;
    m_remove_channels2 = false;
    m_remove_pages = false;
    m_remove_whispers = false;
    m_remove_set = false;
    m_remove_mail = false;
    m_remove_game = false;

    // ========== Runtime State Flags ==========
    // m_bNAWS_wanted, m_bCHARSET_wanted, m_bNoEcho moved to TelnetParser.
    m_bLoaded = false;
    m_bSelected = false;
    m_bVariablesChanged = false;
    m_bModified = false;
    m_bDebugIncomingPackets = false;

    // ========== Statistics Counters ==========
    // m_iInputPacketCount, m_iOutputPacketCount: zero-initialized by ConnectionManager ctor.
    m_iUTF8ErrorCount = 0;
    m_iOutputWindowRedrawCount = 0;

    // ========== UTF-8 Processing State ==========
    m_UTF8Sequence.fill(0);
    m_iUTF8BytesLeft = 0;

    // Trigger/Alias/Timer statistics are zero-initialized by AutomationRegistry member
    // initializers.

    // ========== UI State ==========
    m_last_prefs_page = 0;
    m_bConfigEnableTimers = false;
    m_strLastSelectedTrigger = QString();
    m_strLastSelectedAlias = QString();
    m_strLastSelectedTimer = QString();
    m_strLastSelectedVariable = QString();

    // ========== View Pointers ==========
    m_pActiveInputView = nullptr;

    m_pActiveOutputView = nullptr;

    // ========== Text Selection State ==========
    m_selectionStartLine = -1;
    m_selectionStartChar = -1;
    m_selectionEndLine = -1;
    m_selectionEndChar = -1;

    // ========== Line Buffer ==========
    // m_lineList initializes to empty std::vector automatically
    // m_currentLine is a std::unique_ptr, initializes to nullptr automatically
    m_strCurrentLine = QString();

    // ========== Line Position Tracking ==========
    m_total_lines = 0;
    m_new_lines = 0;
    m_newlines_received = 0;
    // m_nTotalLinesSent, m_nTotalLinesReceived: zero-initialized by ConnectionManager ctor.
    m_last_line_with_IAC_GA = 0;

    // ========== Timing Variables ==========
    // m_tConnectTime, m_tsConnectDuration, m_whenWorldStarted,
    // m_whenWorldStartedHighPrecision: zero-initialized by ConnectionManager ctor.
    m_tLastPlayerInput = QDateTime();
    m_tStatusTime = QDateTime();
    m_lastMousePosition = QPoint(0, 0);
    m_view_number = 0;

    // ========== Telnet Negotiation Phase ==========
    // Fields moved to TelnetParser; TelnetParser constructor zero-initializes them.
    // m_telnetParser already created above.

    // ========== MSP / ZMP / ATCP ==========
    // m_bMSP, m_bZMP, m_bATCP moved to TelnetParser.
    m_strZMPpackage.clear();

    // ========== GMCP (Generic Mud Communication Protocol) ==========

    // ========== MXP/Pueblo Runtime State ==========
    // All MXP runtime fields moved to MXPEngine (m_mxpEngine).
    // MXPEngine constructor zero-initializes them.
    m_cLastChar = 0;
    m_lastSpace = -1; // No space position yet
    m_iLastOutstandingTagCount = 0;

    // ========== ANSI State ==========
    m_code = 0;
    m_lastGoTo = 0;
    m_bWorldClosing = false;
    m_iFlags = 0;          // COLOUR_ANSI mode
    m_iForeColour = WHITE; // ANSI index 7 (silver/grey)
    m_iBackColour = BLACK; // ANSI index 0 (black)
    m_currentAction = nullptr;
    m_bNotesInRGB = false;
    m_iNoteColourFore = qRgb(0, 0, 0);
    m_iNoteColourBack = qRgb(255, 255, 255);
    m_iNoteStyle = 0;

    // ========== Logging ==========
    m_logfile.reset(); // ensure closed/null (default-constructed is already null)
    m_logfile_name = QString();
    m_LastFlushTime = QDateTime();

    // ========== Fonts ==========
    m_FontHeight = 0;
    m_FontWidth = 0;
    m_InputFontHeight = 0;
    m_InputFontWidth = 0;

    // ========== Byte Counters ==========
    // m_nBytesIn, m_nBytesOut: zero-initialized by ConnectionManager ctor.

    // m_iConnectPhase: zero-initialized by ConnectionManager ctor (= CONNECT_NOT_CONNECTED).

    // ========== Scripting Engine ==========
    // Create script engine and initialize Lua
    m_ScriptEngine = std::make_unique<ScriptEngine>(this, m_scripting.language, this);
    if (m_scripting.enabled) {
        m_ScriptEngine->createScriptEngine();
        qCDebug(lcWorld) << "Lua scripting initialized for world:" << worldName();
    }

    m_bSyntaxErrorOnly = false;
    m_bDisconnectOK = false;
    m_bTrace = false;
    m_bInSendToScript = false;
    m_iScriptTimeTaken = 0;
    // m_scripting.tab_complete_functions and m_ExtraShiftTabCompleteItems initialized in Tab
    // Completion section
    m_strLastImmediateExpression = QString();
    m_bInScriptFileChanged = false;
    m_timeScriptFileMod = QDateTime();
    m_scriptFileWatcher = nullptr; // Created when script file is set
    m_strStatusMessage = QString();
    m_tStatusDisplayed = QDateTime();
    m_strScript = QString();

    // ========== Script Handler DISPIDs ==========
    m_dispidWorldOpen = 0;
    m_dispidWorldClose = 0;
    m_dispidWorldSave = 0;
    m_dispidWorldConnect = 0;
    m_dispidWorldDisconnect = 0;
    m_dispidWorldGetFocus = 0;
    m_dispidWorldLoseFocus = 0;
    // MXP DISPID fields moved to MXPEngine (m_mxpEngine).

    // ========== Plugin State ==========
    m_bPluginProcessesOpenTag = false;
    m_bPluginProcessesCloseTag = false;
    m_bPluginProcessesSetVariable = false;
    m_bPluginProcessesSetEntity = false;
    m_bPluginProcessesError = false;

    m_bRecallCommands = false;
    m_bRecallOutput = false;
    m_bRecallNotes = false;

    // ========== Document ID ==========
    m_iUniqueDocumentNumber = 0;

    // ========== Mapping ==========
    // m_CommandQueue moved to ConnectionManager; initialized empty by ConnectionManager ctor.
    m_bShowingMapperStatus = false;

    // ========== Plugins ==========
    m_PluginList.clear(); // QList, not pointer
    m_CurrentPlugin = nullptr;
    m_bPluginProcessingCommand = false;
    m_bPluginProcessingSend = false;
    m_bPluginProcessingSent = false;
    m_strLastCommandSent = QString();
    m_iLastCommandCount = 0;
    m_iExecutionDepth = 0;
    m_bOmitFromCommandHistory = false;

    // ========== Arrays for scripting ==========
    // m_Arrays is a std::map and is default-initialized to empty

    // ========== Background Image ==========
    m_strBackgroundImageName = QString();
    m_iBackgroundMode = 0;
    m_iBackgroundColour = qRgb(0, 0, 0);

    // ========== Foreground Image ==========
    m_strForegroundImageName = QString();
    m_iForegroundMode = 0;

    // ========== MiniWindows ==========
    // m_MiniWindowMap and m_MiniWindowsOrder are initialized by their default constructors

    // ========== Text Rectangle ==========
    m_TextRectangle = QRect(0, 0, 0, 0);
    m_TextRectangleBorderOffset = 0;
    m_TextRectangleBorderColour = 0;
    m_TextRectangleBorderWidth = 0;
    m_TextRectangleOutsideFillColour = 0;
    m_TextRectangleOutsideFillStyle = 0;

    // ========== Sound System ==========
    // m_soundManager created above; audio engine lazy-loaded on first playSound() call.

    // ========== Info Bar ==========
    m_infoBarText = QString();
    m_infoBarVisible = false;
    m_infoBarTextColor = qRgb(0, 0, 0);       // Black text
    m_infoBarBackColor = qRgb(255, 255, 255); // White background
    m_infoBarFontName = "Courier New";
    m_infoBarFontSize = 10;
    m_infoBarFontStyle = 0; // Normal

    // ========== Accelerators ==========
    m_acceleratorManager = new AcceleratorManager(this, this);

    m_bNotesNotWantedNow = false;
    m_bDoingSimulate = false;
    m_bLineOmittedFromOutput = false;
    m_bOmitCurrentLineFromLog = false; //
    m_bScrollBarWanted = false;

    // ========== IAC Counters ==========
    // Moved to TelnetParser; initialized to 0 by TelnetParser constructor.

    // ========== UI State Strings ==========
    m_strWordUnderMenu = QString();
    m_strWindowTitle = QString();
    m_strMainWindowTitle = QString();

    // ========== Timing for fading ==========
    m_timeFadeCancelled = QDateTime();
    m_timeLastWindowDraw = QDateTime();

    // ========== Trigger Evaluation Control ==========
    m_iStopTriggerEvaluation = 0;

    // ========== Initialize unpacked flags (these will be set from m_iFlags1/2) ==========
    m_bCtrlZGoesToEndOfBuffer = false;
    m_bCtrlPGoesToPreviousCommand = false;
    m_bCtrlNGoesToNextCommand = false;
    m_bHyperlinkAddsToCommandHistory = false;
    // m_output.echo_hyperlink_in_output_window uses default member initializer (false)
    m_bAutoWrapWindowWidth = false;
    m_bNAWS = false;
    m_bUseZMP = false;
    m_bUseATCP = false;
    m_bPueblo = false;
    m_bUseCustomLinkColour = false;
    m_bMudCanChangeLinkColour = false;
    m_bUnderlineHyperlinks = false;
    m_bMudCanRemoveUnderline = false;

    m_bAlternativeInverse = false;
    m_bShowConnectDisconnect = false;
    m_bIgnoreMXPcolourChanges = false;
    m_bCustom16isDefaultColour = false;

    // ========== Timer Evaluation Loop ==========
    // Create and start the 1-second timer that calls checkTimers()
    // This is the main timer loop that checks all timers every second
    m_timerCheckTimer = new QTimer(this);
    connect(m_timerCheckTimer, &QTimer::timeout, this, &WorldDocument::checkTimers);
    m_timerCheckTimer->start(1000); // Fire every 1000ms (1 second)

    // ========== Sound System ==========
    // Audio system is lazy-loaded on first use (in m_soundManager->playSound())
    // This avoids creating audio objects in tests and headless environments.
    // m_soundManager itself is already created above.
}

WorldDocument::~WorldDocument()
{
    // Destructor - cleanup will go here in future stories
    // For now, just basic cleanup

    // ========== FIRST: Stop all timers to prevent callbacks during cleanup ==========
    // This must happen before any other cleanup to avoid race conditions.
    // m_timerCheckTimer is Qt parent-child owned (parent = this); Qt deletes it automatically.
    // We only disconnect and stop it here to prevent stray callbacks during teardown.
    if (m_timerCheckTimer) {
        m_timerCheckTimer->disconnect();
        m_timerCheckTimer->stop();
        // Do NOT delete: Qt parent-child ownership handles deletion.
    }

    // Process pending events multiple times to ensure all queued callbacks are drained
    // This is especially important on Windows where timer events may be queued
    for (int i = 0; i < 3; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    // ========== Sound System Cleanup ==========
    if (m_soundManager)
        m_soundManager->cleanup();

    // Socket is Qt parent-child owned (parent = this); Qt deletes it when WorldDocument dies.
    // m_connectionManager->m_pSocket is non-owning — just null it before ConnectionManager dtor.
    if (m_connectionManager) {
        m_connectionManager->m_pSocket = nullptr;
    }

    // ========== Save and Clean Up Plugins ==========
    // Save plugin state for all plugins before cleanup
    for (const auto& plugin : m_PluginList) {
        if (plugin) {
            (void)plugin->SaveState();
        }
    }

    // Clean up all plugins (unique_ptr handles deletion automatically)
    m_PluginList.clear();

    // ========== Variable Cleanup ==========
    // Delete all variables (unique_ptr handles deletion automatically)
    m_VariableMap.clear();

    // ========== Script Engine Cleanup ==========
    // Delete script engine (this will call closeLua() automatically)
    // unique_ptr handles deletion
    if (m_ScriptEngine) {
        m_ScriptEngine.reset();
        qCDebug(lcWorld) << "Script engine destroyed";
    }

    // ========== Script File Watcher Cleanup ==========
    // m_scriptFileWatcher is NOT Qt parent-child owned (created without parent in
    // setupScriptFileWatcher). Manual delete is the sole owner.
    delete m_scriptFileWatcher;
    m_scriptFileWatcher = nullptr;

    // ========== Log File Cleanup ==========
    // Close log file if open
    if (m_logfile) {
        CloseLog();
    }

    // ========== MCCP Compression Cleanup ==========
    // Handled by ZlibStream destructor inside TelnetParser (calls inflateEnd automatically).
    // m_telnetParser (unique_ptr) is destroyed automatically after this destructor body.

    // ========== Line Buffer Cleanup ==========
    // m_lineList (std::vector<std::unique_ptr<Line>>) cleans up automatically
    // m_currentLine (std::unique_ptr<Line>) cleans up automatically
    m_lineList.clear();
    m_currentLine.reset();

    // ========== MXP Cleanup ==========
    // MXPEngine destructor calls CleanupMXP() automatically.
    // m_mxpEngine (unique_ptr) is destroyed automatically after this destructor body.

    // ========== MiniWindow Cleanup ==========
    // Clear all miniwindows (unique_ptr handles deletion automatically)
    m_MiniWindowMap.clear();
    m_MiniWindowsOrder.clear();
}

void WorldDocument::initializeColors()
{
    // Initialize ANSI normal colors
    for (int i = 0; i < 8; i++) {
        m_colors.normal_colour[i] = DEFAULT_NORMAL_COLORS[i];
        m_colors.bold_colour[i] = DEFAULT_BOLD_COLORS[i];
    }

    // Initialize custom colors
    for (int i = 0; i < MAX_CUSTOM; i++) {
        m_colors.custom_text[i] = qRgb(255, 255, 255); // White
        m_colors.custom_back[i] = qRgb(0, 0, 0);       // Black
    }

    // Initialize custom color names
    for (int i = 0; i < 255; i++) {
        m_colors.custom_colour_name[i] = QString("Custom%1").arg(i + 1);
    }
}

void WorldDocument::unpackFlags()
{
    // Unpack m_iFlags1 into individual bool members
    m_input.arrow_recalls_partial = (m_iFlags1 & FLAGS1_ArrowRecallsPartial) != 0;
    m_bCtrlZGoesToEndOfBuffer = (m_iFlags1 & FLAGS1_CtrlZGoesToEndOfBuffer) != 0;
    m_bCtrlPGoesToPreviousCommand = (m_iFlags1 & FLAGS1_CtrlPGoesToPreviousCommand) != 0;
    m_bCtrlNGoesToNextCommand = (m_iFlags1 & FLAGS1_CtrlNGoesToNextCommand) != 0;
    m_bHyperlinkAddsToCommandHistory = (m_iFlags1 & FLAGS1_HyperlinkAddsToCommandHistory) != 0;
    m_output.echo_hyperlink_in_output_window =
        (m_iFlags1 & FLAGS1_EchoHyperlinkInOutputWindow) != 0;
    m_bAutoWrapWindowWidth = (m_iFlags1 & FLAGS1_AutoWrapWindowWidth) != 0;
    m_bNAWS = (m_iFlags1 & FLAGS1_NAWS) != 0;
    m_bPueblo = (m_iFlags1 & FLAGS1_Pueblo) != 0;
    m_input.no_echo_off = (m_iFlags1 & FLAGS1_NoEchoOff) != 0;
    m_bUseCustomLinkColour = (m_iFlags1 & FLAGS1_UseCustomLinkColour) != 0;
    m_bMudCanChangeLinkColour = (m_iFlags1 & FLAGS1_MudCanChangeLinkColour) != 0;
    m_bUnderlineHyperlinks = (m_iFlags1 & FLAGS1_UnderlineHyperlinks) != 0;
    m_bMudCanRemoveUnderline = (m_iFlags1 & FLAGS1_MudCanRemoveUnderline) != 0;

    // Unpack m_iFlags2 into individual bool members
    m_bAlternativeInverse = (m_iFlags2 & FLAGS2_AlternativeInverse) != 0;
    m_bShowConnectDisconnect = (m_iFlags2 & FLAGS2_ShowConnectDisconnect) != 0;
    m_bIgnoreMXPcolourChanges = (m_iFlags2 & FLAGS2_IgnoreMXPcolourChanges) != 0;
    m_bCustom16isDefaultColour = (m_iFlags2 & FLAGS2_Custom16isDefaultColour) != 0;
}

void WorldDocument::packFlags()
{
    // Pack individual bool members back into m_iFlags1
    m_iFlags1 = 0;
    if (m_input.arrow_recalls_partial)
        m_iFlags1 |= FLAGS1_ArrowRecallsPartial;
    if (m_bCtrlZGoesToEndOfBuffer)
        m_iFlags1 |= FLAGS1_CtrlZGoesToEndOfBuffer;
    if (m_bCtrlPGoesToPreviousCommand)
        m_iFlags1 |= FLAGS1_CtrlPGoesToPreviousCommand;
    if (m_bCtrlNGoesToNextCommand)
        m_iFlags1 |= FLAGS1_CtrlNGoesToNextCommand;
    if (m_bHyperlinkAddsToCommandHistory)
        m_iFlags1 |= FLAGS1_HyperlinkAddsToCommandHistory;
    if (m_output.echo_hyperlink_in_output_window)
        m_iFlags1 |= FLAGS1_EchoHyperlinkInOutputWindow;
    if (m_bAutoWrapWindowWidth)
        m_iFlags1 |= FLAGS1_AutoWrapWindowWidth;
    if (m_bNAWS)
        m_iFlags1 |= FLAGS1_NAWS;
    if (m_bPueblo)
        m_iFlags1 |= FLAGS1_Pueblo;
    if (m_input.no_echo_off)
        m_iFlags1 |= FLAGS1_NoEchoOff;
    if (m_bUseCustomLinkColour)
        m_iFlags1 |= FLAGS1_UseCustomLinkColour;
    if (m_bMudCanChangeLinkColour)
        m_iFlags1 |= FLAGS1_MudCanChangeLinkColour;
    if (m_bUnderlineHyperlinks)
        m_iFlags1 |= FLAGS1_UnderlineHyperlinks;
    if (m_bMudCanRemoveUnderline)
        m_iFlags1 |= FLAGS1_MudCanRemoveUnderline;

    // Pack individual bool members back into m_iFlags2
    m_iFlags2 = 0;
    if (m_bAlternativeInverse)
        m_iFlags2 |= FLAGS2_AlternativeInverse;
    if (m_bShowConnectDisconnect)
        m_iFlags2 |= FLAGS2_ShowConnectDisconnect;
    if (m_bIgnoreMXPcolourChanges)
        m_iFlags2 |= FLAGS2_IgnoreMXPcolourChanges;
    if (m_bCustom16isDefaultColour)
        m_iFlags2 |= FLAGS2_Custom16isDefaultColour;
}

// ========== Network Event Handlers ==========

/**
 * ReceiveMsg - Called when socket has data to read
 *
 * This is called from WorldSocket::onReadyRead() when data arrives.
 * Reads all available data and processes each byte through the telnet state machine.
 *
 * Port from: doc.cpp (multiple locations, search for "ReceiveMsg")
 */
void WorldDocument::ReceiveMsg()
{
    if (!m_connectionManager->m_pSocket) {
        qCDebug(lcWorld) << "ReceiveMsg: No socket";
        return;
    }

    // Read all available data
    std::array<char, 8192> buffer{};
    auto result = m_connectionManager->m_pSocket->receive(buffer);
    if (!result) {
        qCDebug(lcWorld) << "ReceiveMsg: Socket read error:" << result.error();
        return;
    }
    qint64 nRead = *result;
    if (nRead == 0) {
        return;
    }

    // Update statistics
    m_connectionManager->m_nBytesIn += nRead;
    m_connectionManager->m_iInputPacketCount++;

    // Notify plugins of raw packet received (for protocol debugging)
    // Note: Original MUSHclient uses SendToAllPluginCallbacksRtn which can modify the data
    // For now, we just notify without modification capability
    QString packetData = QString::fromLatin1(buffer.data(), nRead);
    SendToAllPluginCallbacks(ON_PLUGIN_PACKET_RECEIVED, packetData, false);

    // Check if we need to decompress the data
    auto& tp = *m_telnetParser;
    if (tp.m_bCompress) {
        // MCCP decompression with staging buffer
        // Check if we have space in the input buffer for the new data
        z_stream& zs = tp.m_zlibStream.stream;
        if ((TELNET_COMPRESS_BUFFER_LENGTH - zs.avail_in) < static_cast<uInt>(nRead)) {
            qCDebug(lcWorld) << "Insufficient space in compression input buffer";
            OnConnectionDisconnect();
            return;
        }

        // Shuffle any remaining unconsumed data to the start of the vector
        if (zs.avail_in > 0) {
            memmove(tp.m_CompressInput.data(), zs.next_in, zs.avail_in);
        }
        zs.next_in = tp.m_CompressInput.data();

        // Append new compressed data to the staging buffer
        memcpy(tp.m_CompressInput.data() + zs.avail_in, buffer.data(), nRead);
        zs.avail_in += static_cast<uInt>(nRead);
        tp.m_nTotalCompressed += nRead;

        // Keep decompressing until all input is consumed
        do {
            // Set up output buffer — z_stream.next_out points into the vector's storage
            zs.next_out = tp.m_CompressOutput.data();
            zs.avail_out = static_cast<uInt>(tp.m_CompressOutput.size());

            // Decompress
            QElapsedTimer timer;
            timer.start();
            int izError = inflate(&zs, Z_SYNC_FLUSH);
            tp.m_iCompressionTimeTaken += timer.elapsed();

            // Handle Z_BUF_ERROR by growing the output buffer
            if (izError == Z_BUF_ERROR) {
                const std::size_t newSize = tp.m_CompressOutput.size() * 2;
                tp.m_CompressOutput.resize(newSize);
                qCDebug(lcWorld) << "Grew compression output buffer to" << newSize << "bytes";

                zs.next_out = tp.m_CompressOutput.data();
                zs.avail_out = static_cast<uInt>(newSize);

                timer.start();
                izError = inflate(&zs, Z_SYNC_FLUSH);
                tp.m_iCompressionTimeTaken += timer.elapsed();
            }

            if (izError != Z_OK && izError != Z_STREAM_END) {
                qCDebug(lcWorld) << "MCCP decompression error:" << izError;
                if (zs.msg) {
                    qCDebug(lcWorld) << "  zlib message:" << zs.msg;
                }
                tp.m_bCompress = false;
                OnConnectionDisconnect();
                return;
            }

            // Calculate how many bytes were decompressed
            const qint64 nDecompressed =
                static_cast<qint64>(tp.m_CompressOutput.size()) - zs.avail_out;
            tp.m_nTotalUncompressed += nDecompressed;

            // Process each decompressed byte through the telnet state machine
            for (qint64 i = 0; i < nDecompressed; i++) {
                ProcessIncomingByte(tp.m_CompressOutput[static_cast<std::size_t>(i)]);
            }

            // If we hit stream end, compression is done
            if (izError == Z_STREAM_END) {
                qCDebug(lcWorld) << "MCCP stream ended";
                tp.m_bCompress = false;
                break;
            }

        } while (zs.avail_in > 0);
    } else {
        // No compression - process each byte directly through the telnet state machine
        for (qint64 i = 0; i < nRead; i++) {
            ProcessIncomingByte((unsigned char)buffer[i]);
        }
    }

    // After processing all data, check if we have an incomplete line (prompt)
    // Prompts don't have newlines, so they stay in m_currentLine
    if (m_currentLine && m_currentLine->len() > 0) {
        emit incompleteLine();
    }
}

// NOTE: OnConnect(), connectToMud(), disconnectFromMud(), connectedTime(),
//       resetConnectedTime() implementation moved to ConnectionManager.
//       WorldDocument::OnConnect() / connectToMud() / disconnectFromMud() /
//       connectedTime() / resetConnectedTime() are now inline forwarding wrappers
//       in world_document.h that delegate to m_connectionManager.

/**
 * sendToMud - Send text to the MUD
 *
 * Called when user presses Enter in the input field.
 * Sends the text followed by a newline (CRLF or LF based on MUD preference).
 *
 * TODO Process aliases and scripting before sending
 * TODO Handle speed walking, command stacking, etc.
 */
void WorldDocument::sendToMud(const QString& text)
{
    auto& cm = *m_connectionManager;
    if (!cm.m_pSocket || !cm.m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "sendToMud: Not connected";
        return;
    }

    // Convert to UTF-8 (or Latin-1 if not UTF-8 mode)
    QByteArray data = m_display.utf8 ? text.toUtf8() : text.toLatin1();

    // Add newline - most MUDs expect \n, some want \r\n
    // For now, send just \n (Unix style)
    data.append('\n');

    // Send through socket
    SendPacket({reinterpret_cast<const unsigned char*>(data.constData()),
                static_cast<std::size_t>(data.length())});

    // Increment statistics
    cm.m_nTotalLinesSent++;

    qCDebug(lcWorld) << "sendToMud:" << text;
}

// ========== Command Execution Pipeline ==========

/**
 * SendMsg - High-level command sending with echo/queue/log control
 *
 * This is the main entry point for sending commands through the execution pipeline.
 * Based on doc.cpp in original MUSHclient.
 *
 * Features:
 * - Breaks multi-line text into individual lines
 * - Queues commands if speedwalk delay active
 * - Encodes echo/log flags into queue prefix
 * - Routes to DoSendMsg() for immediate sending
 *
 * @param text - Text to send (may contain multiple lines separated by \r\n)
 * @param bEcho - Should command be echoed to output window?
 * @param bQueue - Should command be queued (for speedwalking)?
 * @param bLog - Should command be logged to log file?
 */
void WorldDocument::SendMsg(const QString& text, bool bEcho, bool bQueue, bool bLog)
{
    QString strText = text;

    // Strip trailing \r\n (ENDLINE in original)
    if (strText.endsWith("\r\n")) {
        strText.chop(2);
    } else if (strText.endsWith("\n")) {
        strText.chop(1);
    }

    // Break into individual lines (split on \r\n or \n)
    // First replace \r\n with \n, then split on \n
    QString normalized = strText;
    normalized.replace("\r\n", "\n");
    QStringList lines = normalized.split("\n", Qt::SkipEmptyParts);

    // If no lines after splitting, treat as single empty line
    if (lines.isEmpty()) {
        lines.append(strText);
    }

    auto& commandQueue = m_connectionManager->m_CommandQueue;

    for (const QString& line : lines) {
        // Queue if speedwalk delay active OR already items in queue
        if (m_speedwalk.delay > 0 && (bQueue || !commandQueue.isEmpty())) {
            // Encode echo/log flags in prefix character
            // Original uses: Q = queue+echo, q = queue+no-echo,
            //                I = immediate+echo, i = immediate+no-echo
            // Lowercase prefix = no logging
            QString prefix;

            if (bQueue) {
                prefix = bEcho ? "Q" : "q"; // QUEUE_WITH_ECHO / QUEUE_WITHOUT_ECHO
            } else {
                prefix = bEcho ? "I" : "i"; // IMMEDIATE_WITH_ECHO / IMMEDIATE_WITHOUT_ECHO
            }

            // Lowercase = no logging
            if (!bLog) {
                prefix = prefix.toLower();
            }

            // Queue it
            commandQueue.append(prefix + line);
        } else {
            // Send immediately
            DoSendMsg(line, bEcho, bLog);
        }
    }
}

/**
 * sendTextToMud - Send multi-line text to MUD with preamble/postamble support
 *
 * Used by "Paste to MUD" and "Send File" features.
 * Based on SendToMushHelper in original MUSHclient (doc.cpp).
 *
 * When commentedSoftcode is false: sends every line as-is with preamble/postamble.
 * When commentedSoftcode is true: processes MUD softcode format:
 *   - Hash mode (first non-blank starts with #): skip # comment lines, accumulate others
 *   - @@ mode: strip @@ and everything after as end-of-line comments
 *   - Lines are accumulated until "-" delimiter, then sent as single line
 *
 * @return true if send completed, false if cancelled
 */
bool WorldDocument::sendTextToMud(const QString& text, const QString& preamble,
                                  const QString& linePreamble, const QString& linePostamble,
                                  const QString& postamble, bool commentedSoftcode, int lineDelay,
                                  int delayPerLines, bool echo, ProgressCallback progressCallback)
{
    // Split text into lines
    QStringList lines = text.split(QRegularExpression("\\r?\\n"));
    int totalLines = lines.size();

    // Use actual logging setting
    bool bLog = m_logging.log_input;

    // Track state for commented softcode mode
    bool hashCommenting = false;
    bool firstNonBlank = true;
    int delayLineCount = 0;
    int processedLineCount = 0;
    QString softcodeAccum; // Accumulated softcode text
    bool cancelled = false;

    // Send preamble
    if (!preamble.isEmpty()) {
        SendMsg(preamble, echo, false, bLog);
    }

    // Helper lambda to send a line with delay handling
    auto sendLineWithDelay = [&](const QString& lineText) -> bool {
        QString fullLine = linePreamble + lineText + linePostamble;
        SendMsg(fullLine, echo, false, bLog);

        delayLineCount++;
        if (lineDelay > 0 && delayPerLines > 0) {
            if (delayLineCount >= delayPerLines) {
                QCoreApplication::processEvents();
                QThread::msleep(static_cast<unsigned long>(lineDelay));
                delayLineCount = 0;
            }
        }
        return true;
    };

    // Process each line
    for (int i = 0; i < totalLines; ++i) {
        const QString& rawLine = lines[i];
        QString line = rawLine;

        // Update progress and check for cancellation
        if (progressCallback) {
            QCoreApplication::processEvents(); // Allow UI to update
            if (!progressCallback(processedLineCount, totalLines)) {
                cancelled = true;
                break;
            }
        }

        processedLineCount++;

        if (commentedSoftcode) {
            // Commented softcode mode - MUD-specific processing
            line = line.trimmed();

            // Skip blank lines in softcode mode
            if (line.isEmpty()) {
                continue;
            }

            // Determine comment mode from first non-blank line
            if (firstNonBlank) {
                firstNonBlank = false;
                hashCommenting = line.startsWith('#');
            }

            // A line containing just "-" ends the accumulated softcode
            if (line == "-") {
                sendLineWithDelay(softcodeAccum);
                softcodeAccum.clear();
                continue;
            }

            if (hashCommenting) {
                // Hash mode: skip lines starting with #
                if (line.startsWith('#')) {
                    continue;
                }
            } else {
                // @@ mode: strip @@ and everything after (end-of-line comment)
                int atPos = line.indexOf("@@");
                if (atPos != -1) {
                    line = line.left(atPos);
                }
            }

            // Accumulate this line (trimmed of leading spaces)
            line = line.trimmed();
            softcodeAccum += line;

        } else {
            // Normal mode - send every line as-is
            sendLineWithDelay(line);
        }
    }

    // If cancelled, don't send remaining content
    if (cancelled) {
        return false;
    }

    // If in softcode mode, send any remaining accumulated text
    if (commentedSoftcode && !softcodeAccum.isEmpty()) {
        sendLineWithDelay(softcodeAccum);
    }

    // Send postamble
    if (!postamble.isEmpty()) {
        SendMsg(postamble, echo, false, bLog);
    }

    // Final progress update
    if (progressCallback) {
        progressCallback(totalLines, totalLines);
    }

    return true;
}

/**
 * DoSendMsg - Low-level command sending with spam prevention
 *
 * Actually sends the command to the MUD with echo/log support.
 * Based on doc.cpp in original MUSHclient.
 *
 * Features:
 * - Spam prevention (inserts spam message if too many repeats)
 * - Input echoing to output window (if bEcho=true and m_display_my_input)
 * - Input logging to log file (if bLog=true and m_logging.log_input)
 * - IAC doubling (telnet protocol)
 *
 * @param text - Single line of text to send
 * @param bEcho - Should command be echoed to output window?
 * @param bLog - Should command be logged to log file?
 */
void WorldDocument::DoSendMsg(const QString& text, bool bEcho, bool bLog)
{
    QString str = text;

    // ========== Plugin Send Callback ==========
    // Call ON_PLUGIN_SEND - plugins can return false to cancel the send
    // Use recursion guard to prevent infinite loops if plugin calls Send()
    if (!m_bPluginProcessingSend) {
        m_bPluginProcessingSend = true;
        bool shouldSend = SendToAllPluginCallbacks(ON_PLUGIN_SEND, str, true);
        m_bPluginProcessingSend = false;
        if (!shouldSend) {
            return; // Plugin cancelled the send
        }
    }

    // ========== Spam Prevention ==========
    // Track repeated commands and insert spam filler message if needed
    if (str == m_strLastCommandSent) {
        m_iLastCommandCount++;
    } else {
        m_strLastCommandSent = str;
        m_iLastCommandCount = 1;
    }

    // If spam prevention enabled and we've exceeded threshold, send spam message
    if (m_spam.enabled && m_spam.line_count > 2 && !m_spam.message.isEmpty() &&
        m_iLastCommandCount > m_spam.line_count) {
        // Reset counter and send spam message
        m_iLastCommandCount = 0;
        DoSendMsg(m_spam.message, m_display_my_input, m_logging.log_input);

        // Reset tracking for current command
        m_strLastCommandSent = str;
        m_iLastCommandCount = 1;
    }

    // ========== Input Echoing ==========
    // Echo sent text to output window if requested
    // Don't echo if m_bNoEcho is set (MUD requested echo suppression for password entry)
    if (bEcho && m_display_my_input && !m_telnetParser->m_bNoEcho) {
        // Display as colored note with input colors
        QRgb inputFore = m_colors.normal_colour[m_output.echo_colour % 8]; // Ensure valid index 0-7
        QRgb inputBack = m_colors.normal_colour[0]; // Black background (index 0)

        // Use colourNote to display in output window
        colourNote(inputFore, inputBack, str);
    }

    // ========== Input Logging ==========
    // Log sent text to log file if requested
    if (bLog && m_logging.log_input && IsLogOpen()) {
        logCommand(str);
    }

    // ========== IAC Doubling ==========
    // Double IAC (0xFF) to IAC IAC for telnet protocol (unless disabled)
    if (!m_spam.do_not_translate_iac) {
        // Replace all 0xFF with 0xFF 0xFF
        QByteArray data = m_display.utf8 ? str.toUtf8() : str.toLatin1();
        QByteArray doubled;
        for (unsigned char c : data) {
            doubled.append(c);
            if (c == IAC) {
                doubled.append(static_cast<char>(IAC)); // Double it
            }
        }

        // Send the doubled data + newline
        doubled.append('\n');
        SendPacket({reinterpret_cast<const unsigned char*>(doubled.constData()),
                    static_cast<std::size_t>(doubled.length())});
    } else {
        // Just send normally (use existing sendToMud)
        sendToMud(str);
    }

    // Update statistics
    m_connectionManager->m_nTotalLinesSent++;
    m_tLastPlayerInput = QDateTime::currentDateTime();

    // Notify plugins that send completed
    // Use recursion guard to prevent infinite loops if plugin calls Send()
    if (!m_bPluginProcessingSent) {
        m_bPluginProcessingSent = true;
        SendToAllPluginCallbacks(ON_PLUGIN_SENT, str, false);
        m_bPluginProcessingSent = false;
    }

    qCDebug(lcWorld) << "DoSendMsg:" << str << "(echo=" << bEcho << ", log=" << bLog << ")";
}

/**
 * logCommand - Log command to input log with preamble/postamble
 *
 * Writes player input to the log file with appropriate formatting.
 * Based on original MUSHclient LogCommand() implementation.
 *
 * @param text - Command text to log
 */
void WorldDocument::logCommand(const QString& text)
{
    if (!IsLogOpen()) {
        return;
    }

    // Write preamble (if any)
    if (!m_logging.line_preamble_input.isEmpty()) {
        QString preamble = FormatTime(QDateTime::currentDateTime(), m_logging.line_preamble_input,
                                      m_logging.log_html);
        WriteToLog(preamble);
    }

    // Write the command text
    if (m_logging.log_html) {
        WriteToLog(FixHTMLString(text));
    } else {
        WriteToLog(text);
    }

    // Write postamble (if any)
    if (!m_logging.line_postamble_input.isEmpty()) {
        QString postamble = FormatTime(QDateTime::currentDateTime(), m_logging.line_postamble_input,
                                       m_logging.log_html);
        WriteToLog(postamble);
    }

    // Always add newline after command
    WriteToLog("\n");
}

// ========== Command Stacking ==========

/**
 * Execute - Process command with command stacking support
 *
 * Command Stacking
 *
 * Handles command stacking (multiple commands separated by delimiter).
 * Based on methods_commands.cpp in original MUSHclient.
 *
 * Features:
 * - Leading delimiter disables stacking for that line
 * - Escape sequence: ;; → literal ;
 * - Splits commands on delimiter
 * - Each command processed through alias evaluation + SendMsg()
 * - Whitespace preserved (NOT trimmed - matches original)
 * - Empty commands ARE sent (matches original)
 *
 * @param command - User-entered command (may contain multiple commands)
 */
void WorldDocument::Execute(const QString& command)
{
    QString strFixedCommand = command;

    // ========== Auto-Say Mode ==========
    // Handle auto-say mode and override prefix
    // Based on sendvw.cpp
    // When auto-say is enabled, commands are prepended with "say " unless overridden

    bool bAutoSay = m_auto_say.enabled;

    // Check for override prefix to disable auto-say for this command
    if (bAutoSay && !m_auto_say.override_prefix.isEmpty() &&
        strFixedCommand.startsWith(m_auto_say.override_prefix)) {
        bAutoSay = false;
        strFixedCommand = strFixedCommand.mid(m_auto_say.override_prefix.length()); // Strip prefix
    }

    // Exclude auto-say string itself (prevent "say say hello")
    if (bAutoSay && !m_auto_say.say_string.isEmpty() &&
        strFixedCommand.startsWith(m_auto_say.say_string)) {
        bAutoSay = false;
    }

    // If auto-say is still enabled, prepend the auto-say string
    if (bAutoSay && !m_auto_say.say_string.isEmpty()) {
        strFixedCommand = m_auto_say.say_string + strFixedCommand;
    }

    // ========== Script Prefix ==========
    // Check for script prefix to execute Lua code directly
    // Based on methods_commands.cpp

    if (m_scripting.enabled && !m_scripting.prefix.isEmpty() &&
        strFixedCommand.startsWith(m_scripting.prefix)) {
        // Remove prefix and get script command
        QString strCommand = strFixedCommand.mid(m_scripting.prefix.length());

        // Execute the script if scripting engine is available
        if (m_ScriptEngine) {
            m_ScriptEngine->parseLua(strCommand, "Command line");
        } else {
            // Warn that scripting is not active
            note("Scripting is not active yet, or script file had a parse error.");
        }
        return; // Don't process further (script handled)
    }

    // ========== Speed Walking ==========
    // Check for speedwalk prefix BEFORE command stacking
    // Based on evaluate.cpp

    if (m_speedwalk.enabled && !m_speedwalk.prefix.isEmpty() &&
        strFixedCommand.startsWith(m_speedwalk.prefix)) {
        // Remove prefix and evaluate speedwalk
        QString speedwalkInput = strFixedCommand.mid(m_speedwalk.prefix.length());
        QString expandedSpeedwalk = speedwalk::evaluate(speedwalkInput, m_speedwalk.filler);

        if (!expandedSpeedwalk.isEmpty()) {
            // Check for error (starts with '*')
            if (expandedSpeedwalk[0] == '*') {
                // Show error message to user (skip the '*')
                qCDebug(lcWorld) << "Speedwalk error:" << expandedSpeedwalk.mid(1);
                note(expandedSpeedwalk.mid(1)); // Display error in output window
                return;
            }

            // Send expanded speedwalk commands via SendMsg with queue=true
            SendMsg(expandedSpeedwalk, m_display_my_input, true, m_logging.log_input);
        }
        return; // Don't process further (speedwalk handled)
    }

    // ========== Command Stacking ==========
    // Based on methods_commands.cpp

    if (m_input.enable_command_stack && !m_input.command_stack_character.isEmpty()) {
        // NEW in version 3.74 - command stack character at start of line disables command stacking
        if (!strFixedCommand.isEmpty() &&
            strFixedCommand[0] == m_input.command_stack_character[0]) {
            // Remove leading delimiter - this line won't be stacked
            strFixedCommand = strFixedCommand.mid(1);
        } else {
            // Still want command stacking
            QString strTwoStacks =
                QString(m_input.command_stack_character[0]) + m_input.command_stack_character[0];

            // Convert two command stacks in a row to 0x01 (e.g., ;; → \x01)
            strFixedCommand.replace(strTwoStacks, "\x01");

            // Convert any remaining command stacks to \r\n
            strFixedCommand.replace(m_input.command_stack_character[0], '\n');

            // Replace any 0x01 with one command stack character (restore escaped delimiters)
            strFixedCommand.replace('\x01', m_input.command_stack_character[0]);
        }
    }

    // Change \r\n to \n (normalize line endings)
    strFixedCommand.replace("\r\n", "\n");

    // Break up command into a list, terminated by newlines
    QStringList strList = strFixedCommand.split('\n');

    // If list is empty, make sure we send at least one empty line
    // (matches original MUSHclient behavior)
    if (strList.isEmpty()) {
        strList.append("");
    }

    // Process each command individually
    for (const QString& str : strList) {
        QString processedCommand = str;
        bool bypassAliases = false;

        // ========== Immediate Prefix ==========
        // Check for immediate prefix to bypass alias system
        // Default prefix is "/" - sends command directly to MUD
        // This allows users to bypass troublesome aliases temporarily

        QString immediatePrefix = "/"; // Default immediate prefix
        if (!immediatePrefix.isEmpty() && processedCommand.startsWith(immediatePrefix)) {
            // Remove prefix and set flag to bypass aliases
            processedCommand = processedCommand.mid(immediatePrefix.length());
            bypassAliases = true;
        }

        // ========== Alias Evaluation ==========
        bool aliasHandled = false;

        if (m_enable_aliases && !bypassAliases) {
            aliasHandled = evaluateAliases(processedCommand);
        }

        // ========== Send to MUD ==========
        if (!aliasHandled) {
            // No alias matched - send to MUD
            bool bEcho = m_display_my_input;
            bool bQueue = false;
            bool bLog = m_logging.log_input;

            SendMsg(processedCommand, bEcho, bQueue, bLog);

            // Add to command history
            addToCommandHistory(processedCommand);
        }
        // Note: If alias handled, the alias execution already added to history (if
        // !omit_from_command_history)
    }
}

// NOTE: GetCommandQueue(), Queue(), DiscardQueue() implementation moved to ConnectionManager.
// WorldDocument::GetCommandQueue() / Queue() / DiscardQueue() are now inline forwarding wrappers
// in world_document.h that delegate to m_connectionManager.

// ========== Command Input Window ==========

/**
 * setActiveInputView - Set the active input view
 *
 * Sets the active input view for this document. This is typically called
 * when the UI creates the input widget.
 *
 * @param inputView The input view to set as active
 */
void WorldDocument::setActiveInputView(IInputView* inputView)
{
    m_pActiveInputView = inputView;
}

void WorldDocument::setActiveOutputView(IOutputView* outputView)
{
    m_pActiveOutputView = outputView;
}

/**
 * GetCommand - Get current text in command input window
 *
 * Returns the text currently in the command input field.
 *
 * Based on methods_commands.cpp
 *
 * @return Current command text, or empty string if no input view
 */
QString WorldDocument::GetCommand() const
{
    if (!m_pActiveInputView) {
        return QString();
    }

    return m_pActiveInputView->inputText();
}

/**
 * SetCommand - Set command input window text
 *
 * Sets the text in the command input field. Returns eCommandNotEmpty
 * if the input field is not empty (refuses to overwrite existing text).
 *
 * Based on methods_commands.cpp
 *
 * @param text Text to set in command input
 * @return 0 (eOK) on success, 30011 (eCommandNotEmpty) if input not empty
 */
qint32 WorldDocument::SetCommand(const QString& text)
{
    if (!m_pActiveInputView) {
        return 0; // eOK - no input view to set
    }

    QString current = m_pActiveInputView->inputText();
    if (!current.isEmpty()) {
        return 30011; // eCommandNotEmpty
    }

    m_pActiveInputView->setInputText(text);
    SendToAllPluginCallbacks(ON_PLUGIN_COMMAND_CHANGED);
    return 0; // eOK
}

/**
 * SetCommandSelection - Set selection in command input window
 *
 * Sets the text selection in the command input field.
 * Parameters are 1-based (converted to 0-based for Qt).
 *
 * Based on methods_commands.cpp
 *
 * @param first Start position (1-based)
 * @param last End position (1-based, -1 for end of text)
 * @return 0 (eOK)
 */
qint32 WorldDocument::SetCommandSelection(qint32 first, qint32 last)
{
    if (!m_pActiveInputView) {
        return 0; // eOK - no input view to modify
    }

    int textLen = m_pActiveInputView->inputText().length();

    // Convert from 1-based to 0-based
    int start = first - 1;
    int end = last;

    // Handle special case: -1 means end of text
    if (last == -1) {
        end = textLen;
    }

    // Clamp to valid range
    start = qBound(0, start, textLen);
    end = qBound(0, end, textLen);

    // Use interface method for selection
    m_pActiveInputView->setSelection(start, end - start);

    return 0; // eOK
}

/**
 * SelectCommand - Select all text in command input window
 *
 * Selects all text in the command input field.
 *
 * Based on methods_commands.cpp (SelectCommand)
 */
void WorldDocument::SelectCommand()
{
    if (m_pActiveInputView) {
        m_pActiveInputView->selectAll();
    }
}

/**
 * PushCommand - Get and clear command input
 *
 * Returns the current command text and clears the input field.
 * Useful for command stacking.
 *
 * Based on methods_commands.cpp
 *
 * @return Current command text before clearing
 */
QString WorldDocument::PushCommand()
{
    if (!m_pActiveInputView) {
        return QString();
    }

    QString command = m_pActiveInputView->inputText();
    m_pActiveInputView->clearInput();
    SendToAllPluginCallbacks(ON_PLUGIN_COMMAND_CHANGED);
    return command;
}

// ========== Custom Colours ==========

/**
 * SetCustomColourName - Set the name for a custom colour
 *
 * Sets the display name for a custom color slot. The name is used in the UI
 * and can be up to 30 characters long. Note: The array holds 255 names but
 * currently only 1-16 are supported (MAX_CUSTOM).
 *
 * Based on methods_colours.cpp
 *
 * @param whichColour Index of custom color (1-16)
 * @param name Name for the color (1-30 characters)
 * @return eOK (0) on success,
 *         eOptionOutOfRange (30009) if whichColour out of range,
 *         eNoNameSpecified (30003) if name is empty,
 *         eInvalidObjectLabel (30008) if name too long
 */
qint32 WorldDocument::SetCustomColourName(qint16 whichColour, const QString& name)
{
    // Validate color index (1-based, matches original which checks against MAX_CUSTOM)
    if (whichColour < 1 || whichColour > MAX_CUSTOM) {
        return 30009; // eOptionOutOfRange
    }

    // Validate name not empty (original uses strlen > 0)
    if (name.length() <= 0) {
        return 30003; // eNoNameSpecified
    }

    // Validate name length (max 30 characters)
    if (name.length() > 30) {
        return 30008; // eInvalidObjectLabel
    }

    // Check if changed and mark document modified (matches original ordering)
    if (m_colors.custom_colour_name[whichColour - 1] != name) {
        m_bModified = true; // Mark document as modified
    }

    // Set the name (convert to 0-based index)
    m_colors.custom_colour_name[whichColour - 1] = name;

    return 0; // eOK
}


// ========== Style Management ==========

/**
 * AddStyle - Create and add a new style to the current line
 *
 * Style Tracking
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::AddStyle)
 *
 * Creates a new Style object with specified attributes and adds it to the current
 * line's style list. If the last style has zero length (and isn't a START_TAG),
 * it will be removed first to avoid accumulating empty styles.
 *
 * @param iFlags Style flags (HILITE, UNDERLINE, BLINK, color mode, etc.)
 * @param iForeColour Foreground color (ANSI index or RGB value depending on flags)
 * @param iBackColour Background color (ANSI index or RGB value depending on flags)
 * @param iLength Length of text with this style (can be 0 for initial style)
 * @param pAction Hyperlink action (nullptr if no action)
 * @return Pointer to the newly created Style, or nullptr if no current line
 */
Style* WorldDocument::AddStyle(quint16 iFlags, QRgb iForeColour, QRgb iBackColour, quint16 iLength,
                               const std::shared_ptr<Action>& pAction)
{
    Line* pLine = m_currentLine.get();

    // Can't add style without a line
    if (!pLine)
        return nullptr;

    // Remove last style if it has zero length (unless it's a START_TAG)
    if (!pLine->styleList.empty()) {
        Style* pOldStyle = pLine->styleList.back().get();
        if (pOldStyle->iLength == 0 && (pOldStyle->iFlags & START_TAG) == 0) {
            pLine->styleList.pop_back();
        }
    }

    // Create new style
    auto pNewStyle = std::make_unique<Style>();
    pNewStyle->iFlags = iFlags;
    pNewStyle->iForeColour = iForeColour;
    pNewStyle->iBackColour = iBackColour;
    pNewStyle->iLength = iLength;
    pNewStyle->pAction = pAction; // shared_ptr assignment handles reference counting

    // Get raw pointer before moving
    Style* rawPtr = pNewStyle.get();

    // Add to line's style list
    pLine->styleList.push_back(std::move(pNewStyle));

    return rawPtr;
}

/**
 * RememberStyle - Save a style's attributes as the current style
 *
 * Style Tracking
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::RememberStyle)
 *
 * This is called whenever the style changes (via ANSI codes, MXP, etc.)
 * to update the document's current style state. Subsequent AddToLine()
 * calls will use these style attributes.
 *
 * @param pStyle Style object whose attributes to remember (can be nullptr)
 */

// ========== Line Buffer Management ==========

/**
 * AddLineToBuffer - Add a completed line to the buffer with size limiting
 *
 * Source: doc.cpp (various locations where lines are added)
 * Line Buffer and CLine/CStyle Management
 *
 * When a line is completed (via StartNewLine), this method adds it to the
 * line buffer and ensures the buffer doesn't exceed m_display.max_lines by removing
 * the oldest lines.
 *
 * @param line The completed Line object to add (ownership transferred to buffer)
 */
void WorldDocument::AddLineToBuffer(std::unique_ptr<Line> line)
{
    if (!line) {
        return; // Null check
    }

    // Add line to buffer (ownership transferred)
    m_lineList.push_back(std::move(line));

    // Trim buffer if too large
    while (static_cast<qint32>(m_lineList.size()) > m_display.max_lines) {
        m_lineList.erase(m_lineList.begin()); // oldest line is deleted automatically
    }
}

// ========== AddToLine() - Character Accumulation ==========

/**
 * AddToLine - Add text to the current line being built
 *
 * Source: doc.cpp (AddToLine implementation)
 * Implement AddToLine - Character Accumulation
 *
 * As bytes arrive from the network and pass through telnet/ANSI parser,
 * they are added to the current line using this method. This function:
 * 1. Extends the last style if it matches current style (coalescing)
 * 2. Creates new style run if style has changed
 * 3. Grows text buffer as needed using realloc()
 * 4. Copies text bytes to buffer
 * 5. Tracks last character for protocol parsing
 *
 * @param sText Pointer to text to add (might be single char or UTF-8 sequence)
 * @param iLength Length of text (0 means use strlen, -1 same as 0)
 */
void WorldDocument::AddToLine(const char* sText, int iLength)
{
    // Sanity checks
    if (!sText || !m_currentLine) {
        return;
    }

    // Get text length if not specified
    if (iLength <= 0) {
        iLength = static_cast<int>(strlen(sText));
    }

    // Nothing to add?
    if (iLength == 0) {
        return;
    }

    // Security: force a line break if the line exceeds 1MB to prevent
    // OOM from servers sending continuous data without newlines.
    constexpr qint32 kMaxLineLength = 1024 * 1024;
    if (m_currentLine->len() + iLength > kMaxLineLength) {
        StartNewLine(false, 0); // soft break (not a real MUD newline)
    }

    // Style coalescing: Check if we can extend the last style instead of creating new one
    Style* lastStyle = nullptr;
    if (!m_currentLine->styleList.empty()) {
        lastStyle = m_currentLine->styleList.back().get();
    }

    bool canExtendLastStyle = false;
    if (lastStyle && lastStyle->iFlags == m_iFlags && lastStyle->iForeColour == m_iForeColour &&
        lastStyle->iBackColour == m_iBackColour && lastStyle->pAction == m_currentAction) {
        // Same style - can extend it
        canExtendLastStyle = true;
    }

    if (canExtendLastStyle) {
        // Extend existing style
        lastStyle->iLength += static_cast<quint16>(iLength);
    } else {
        // Different style - create new style run
        auto newStyle = std::make_unique<Style>();
        newStyle->iLength = static_cast<quint16>(iLength);
        newStyle->iFlags = m_iFlags;
        newStyle->iForeColour = m_iForeColour;
        newStyle->iBackColour = m_iBackColour;
        newStyle->pAction = m_currentAction; // shared_ptr assignment handles reference counting

        m_currentLine->styleList.push_back(std::move(newStyle));
    }

    // Resize text buffer if needed (using doubling strategy)
    qint32 currentLen = m_currentLine->len();
    qint32 currentCapacity = m_currentLine->iMemoryAllocated();

    if (currentLen + iLength >= currentCapacity) {
        // Need more space - double the buffer size
        qint32 newSize = currentCapacity * 2;

        // Keep doubling until we have enough space (plus null terminator)
        while (newSize < currentLen + iLength + 1) {
            newSize *= 2;
        }

        // Reserve new capacity
        m_currentLine->textBuffer.reserve(newSize);
    }

    // Resize to accommodate new text (excluding null terminator for now)
    m_currentLine->textBuffer.resize(currentLen + iLength);

    // Copy text to buffer
    memcpy(m_currentLine->textBuffer.data() + currentLen, sText, iLength);

    // Add null terminator for C-string compatibility
    m_currentLine->textBuffer.push_back('\0');

    // Update last character tracking (for protocol parsing)
    if (iLength > 0) {
        m_cLastChar = sText[iLength - 1];
    }

    // Track space positions for word-wrap
    // Scan added text for spaces and update m_lastSpace
    // Only track spaces that are within the wrap column
    for (int i = 0; i < iLength; i++) {
        if (sText[i] == ' ') {
            int spacePos = currentLen + i;
            // Only remember this space if it's before the wrap column
            // (or if no wrap column is set)
            if (m_display.wrap_column == 0 || spacePos < static_cast<int>(m_display.wrap_column)) {
                m_lastSpace = spacePos;
            }
        }
    }

    // Check if line needs to wrap (exceeds wrap column)
    // Only wrap if m_display.wrap_column > 0 (0 means no wrapping)
    if (m_display.wrap_column > 0 &&
        m_currentLine->len() >= static_cast<qint32>(m_display.wrap_column)) {
        handleLineWrap();
    }
}

/**
 * AddToLine - Add single character to current line (convenience overload)
 *
 * @param c Character to add
 */
void WorldDocument::AddToLine(unsigned char c)
{
    char ch = static_cast<char>(c);
    AddToLine(&ch, 1);
}

/**
 * handleLineWrap - Handle line wrapping when wrap column is exceeded
 *
 * Source: doc.cpp:1512-1600 (original MUSHclient word-wrap logic)
 *
 * This is called when the current line length exceeds m_display.wrap_column.
 * Behavior depends on m_display.wrap setting:
 * - m_display.wrap = false: Hard break at column boundary
 * - m_display.wrap = true: Word-wrap at last space (if found within reasonable distance)
 *
 * Word-wrap logic:
 * 1. If m_display.wrap is enabled and we have a valid last space position
 * 2. And the text after the space isn't too long (< m_display.wrap_column)
 * 3. Break at the space, carry text after space to new line
 * 4. Otherwise, hard break at the column boundary
 */
void WorldDocument::handleLineWrap()
{
    if (!m_currentLine) {
        return;
    }

    qint32 lineLen = m_currentLine->len();

    // Determine break point
    int breakPoint = -1;

    // If word-wrap is enabled and we have a valid space position
    if (m_display.wrap && m_lastSpace >= 0) {
        // Check that the text after the space isn't too long
        // (if remaining text is >= wrap column, we'd have to break again immediately)
        qint32 remainingLen = lineLen - m_lastSpace;
        if (remainingLen < static_cast<qint32>(m_display.wrap_column)) {
            breakPoint = m_lastSpace;
        }
    }

    if (breakPoint >= 0) {
        // Word-wrap: break at the space
        // Original MUSHclient behavior (doc.cpp:1557-1562):
        // When m_display.indent_paras is false (normal case), KEEP the space at end of this line
        // This ensures soft-wrapped lines display correctly when joined

        // Save the text AFTER the space to carry over to the next line
        int carryOverStart = breakPoint + 1; // Text starts after the space
        int carryOverLen = lineLen - carryOverStart;

        QByteArray carryOver;
        if (carryOverLen > 0) {
            carryOver = QByteArray(m_currentLine->text().data() + carryOverStart, carryOverLen);
        }

        // Truncate current line AFTER the space (include the space in this line)
        // This matches original MUSHclient when m_display.indent_paras is false
        int truncateAt = breakPoint + 1; // Keep up to and including the space
        m_currentLine->textBuffer.resize(truncateAt);
        m_currentLine->textBuffer.push_back('\0');

        // Adjust styles: need to truncate styles to match new line length
        adjustStylesForTruncation(truncateAt);

        // Start new line (soft wrap - bNewLine=false)
        StartNewLine(false, 0);

        // Reset last space tracking for new line
        m_lastSpace = -1;

        // Add carried-over text to new line
        if (carryOverLen > 0) {
            // Temporarily disable wrap checking to avoid recursion
            quint16 savedWrapColumn = m_display.wrap_column;
            m_display.wrap_column = 0;

            AddToLine(carryOver.constData(), carryOverLen);

            m_display.wrap_column = savedWrapColumn;

            // Re-check for wrap (in case carried text is still too long)
            if (m_display.wrap_column > 0 &&
                m_currentLine->len() >= static_cast<qint32>(m_display.wrap_column)) {
                handleLineWrap();
            }
        }
    } else {
        // Hard break: split at the wrap column
        // Save text beyond the wrap column
        int splitPoint = static_cast<int>(m_display.wrap_column);
        int carryOverLen = lineLen - splitPoint;

        QByteArray carryOver;
        if (carryOverLen > 0) {
            carryOver = QByteArray(m_currentLine->text().data() + splitPoint, carryOverLen);
        }

        // Truncate current line at the split point
        m_currentLine->textBuffer.resize(splitPoint);
        m_currentLine->textBuffer.push_back('\0');

        // Adjust styles
        adjustStylesForTruncation(splitPoint);

        // Start new line (soft wrap - bNewLine=false)
        StartNewLine(false, 0);

        // Reset last space tracking for new line
        m_lastSpace = -1;

        // Add carried-over text to new line
        if (carryOverLen > 0) {
            // Temporarily disable wrap checking to avoid recursion
            quint16 savedWrapColumn = m_display.wrap_column;
            m_display.wrap_column = 0;

            AddToLine(carryOver.constData(), carryOverLen);

            m_display.wrap_column = savedWrapColumn;

            // Re-check for wrap (in case carried text is still too long)
            if (m_display.wrap_column > 0 &&
                m_currentLine->len() >= static_cast<qint32>(m_display.wrap_column)) {
                handleLineWrap();
            }
        }
    }
}

/**
 * adjustStylesForTruncation - Adjust style runs when line is truncated
 *
 * When we truncate a line for word-wrap, we need to adjust the style runs
 * so they don't reference text beyond the truncation point.
 *
 * @param newLength The new length of the line text
 */
void WorldDocument::adjustStylesForTruncation(qint32 newLength)
{
    if (!m_currentLine || m_currentLine->styleList.empty()) {
        return;
    }

    qint32 pos = 0;
    auto it = m_currentLine->styleList.begin();

    while (it != m_currentLine->styleList.end()) {
        Style* style = it->get();
        qint32 styleEnd = pos + style->iLength;

        if (pos >= newLength) {
            // This style is entirely beyond the truncation point - remove it
            it = m_currentLine->styleList.erase(it);
        } else if (styleEnd > newLength) {
            // This style spans the truncation point - truncate it
            style->iLength = static_cast<quint16>(newLength - pos);
            ++it;
        } else {
            // This style is entirely before the truncation point - keep it
            ++it;
        }
        pos = styleEnd;
    }
}

// ========== Lua Function Callbacks ==========

// NOTE: onWorldConnect(), onWorldDisconnect(), OnConnectionDisconnect() implementation
// moved to ConnectionManager. They are called from ConnectionManager::onConnect() and
// ConnectionManager::onConnectionDisconnect().
// WorldDocument::OnConnectionDisconnect() is now an inline forwarding wrapper in
// world_document.h that calls m_connectionManager->onConnectionDisconnect().

// ========== Stub Methods (Future Implementation) ==========

/**
 * StartNewLine - Complete current line and start a new one
 *
 * Line Completion
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::StartNewLine)
 *
 * This is called when:
 * - A newline character arrives from the MUD (bNewLine=true)
 * - The line must wrap to a new line (bNewLine=false)
 *
 * @param bNewLine True if this is a hard break (MUD sent newline), false for soft wrap
 * @param iFlags Line flags (COMMENT, USER_INPUT, etc.)
 *
 * Process:
 * 1. Finalize current line (set timestamp, run triggers if hard break)
 * 2. Add completed line to buffer
 * 3. Create new line with initial style
 * 4. Update display
 */
void WorldDocument::StartNewLine(bool bNewLine, unsigned char iFlags)
{
    // We may not have a current line (first line of session)
    if (m_currentLine) {
        // Finalize the line with timestamp
        m_currentLine->m_theTime = QDateTime::currentDateTime();

        // Performance timing uses QDateTime::currentMSecsSinceEpoch() — sufficient precision for
        // this use.
        m_currentLine->m_lineHighPerformanceTime = 0;

        // Set hard_return flag (true = MUD newline, false = wrapped by client)
        m_currentLine->hard_return = bNewLine;

        // Set line flags (COMMENT, USER_INPUT, etc.)
        m_currentLine->flags = iFlags;

        // Reset omit-from-log flag before trigger evaluation
        m_bOmitCurrentLineFromLog = false;

        // Notify plugins of partial line (line without newline yet)
        // This allows plugins to process prompts and other unterminated lines
        // Based on original MUSHclient's SendLineToPlugin()
        if (!(iFlags & NOTE_OR_COMMAND)) {
            QString partialLine =
                QString::fromUtf8(m_currentLine->text().data(), m_currentLine->text().size());
            SendToAllPluginCallbacks(ON_PLUGIN_PARTIAL_LINE, partialLine, false);
        }

        // If this is a hard break (newline from MUD), evaluate triggers
        // Call trigger pattern matching engine
        if (bNewLine && !(iFlags & NOTE_OR_COMMAND)) {
            // Add line text to recent lines buffer for multi-line trigger matching
            // Based on ProcessPreviousLine.cpp in original MUSHclient
            QString lineText =
                QString::fromUtf8(m_currentLine->text().data(), m_currentLine->text().size());
            m_recentLines.push_back(lineText);
            m_newlines_received++;

            // Limit buffer size - remove oldest if too many
            if (static_cast<int>(m_recentLines.size()) > MAX_RECENT_LINES) {
                m_recentLines.pop_front();
            }

            evaluateTriggers(m_currentLine.get());
        }

        // Line-level logging integration
        // Log completed lines based on line type and logging flags
        if (bNewLine) {
            logCompletedLine(m_currentLine.get());
        }

        // Detect and linkify URLs in the completed line
        DetectAndLinkifyURLs(m_currentLine.get());

        // Notify plugins about the line being displayed (for accessibility/screen readers)
        // type: 0 = MUD output, 1 = COMMENT (note), 2 = USER_INPUT (command)
        // iLog: whether line should be logged based on type
        if (bNewLine) {
            QString lineText =
                QString::fromUtf8(m_currentLine->text().data(), m_currentLine->text().size());
            unsigned char lineFlags = m_currentLine->flags;
            if (lineFlags & COMMENT) {
                Screendraw(COMMENT, m_logging.log_notes, lineText);
            } else if (lineFlags & USER_INPUT) {
                Screendraw(USER_INPUT, m_logging.log_input, lineText);
            } else {
                // Regular MUD output (type 0)
                // Log flag: m_logging.log_output AND not omitted
                Screendraw(0, m_logging.log_output && !m_bOmitCurrentLineFromLog, lineText);
            }
        }

        // Add completed line to buffer (ownership transferred via move)
        AddLineToBuffer(std::move(m_currentLine));
    }

    // Create new line
    // Increment line counter
    m_total_lines++;

    // Determine initial style colors for new line
    quint16 initialFlags = m_iFlags;
    QRgb initialFore = m_iForeColour;
    QRgb initialBack = m_iBackColour;

    // Special handling for user input and comments (like original)
    if (iFlags & USER_INPUT) {
        // TODO(feature): Apply m_output.echo_colour setting to echoed command text.
    } else if (iFlags & COMMENT) {
        // TODO(feature): Apply m_colors.note_text_colour setting to Note() output text.
    }

    // Create new Line object
    m_currentLine = std::make_unique<Line>(m_total_lines,         // line number
                                           m_display.wrap_column, // wrap column
                                           iFlags,                // line flags
                                           initialFore,           // foreground color
                                           initialBack,           // background color
                                           m_display.utf8);       // UTF-8 mode

    // Reset word-wrap tracking for new line
    m_lastSpace = -1;

    // IMPORTANT: Create initial style for new line
    // The Line constructor does NOT create an initial style
    // We create it here so AddToLine() can extend it
    auto initialStyle = std::make_unique<Style>();
    initialStyle->iLength = 0; // No text yet
    initialStyle->iFlags = initialFlags;
    initialStyle->iForeColour = initialFore;
    initialStyle->iBackColour = initialBack;
    initialStyle->pAction = m_currentAction; // Current hyperlink/action (may be nullptr) -
                                             // shared_ptr handles ref counting

    // Add initial style to new line
    m_currentLine->styleList.push_back(std::move(initialStyle));

    // Emit signal to update display
    emit linesAdded();
}

/**
 * DetectAndLinkifyURLs - Detect URLs in completed line and convert to hyperlinks
 *
 * Scans the text of a completed line for URL patterns (http://, https://, ftp://, mailto:)
 * and creates hyperlink Actions for them. This function:
 * - Uses regex to find all URLs in the line text
 * - Splits existing Style objects at URL boundaries
 * - Creates Action objects with ACTION_HYPERLINK flag for each URL
 *
 * This integrates with the existing MXP hyperlink infrastructure.
 */
void WorldDocument::DetectAndLinkifyURLs(Line* line)
{
    if (!line || line->len() == 0) {
        return;
    }

    // Convert line text to QString for regex matching
    QString lineText = QString::fromUtf8(line->text().data(), line->text().size());

    // URL regex pattern - matches http://, https://, ftp://, mailto:
    // Excludes common punctuation that shouldn't be part of URLs
    static QRegularExpression urlPattern(R"((https?://|ftp://|mailto:)[^\s<>"{}|\\^`\[\]]+)",
                                         QRegularExpression::CaseInsensitiveOption);

    // Find all URL matches
    QRegularExpressionMatchIterator matches = urlPattern.globalMatch(lineText);
    if (!matches.hasNext()) {
        return; // No URLs found
    }

    // Collect all matches first (we'll process them in reverse to avoid offset issues)
    QList<QRegularExpressionMatch> matchList;
    while (matches.hasNext()) {
        matchList.append(matches.next());
    }

    // Process URLs in reverse order (right to left) to maintain correct positions
    // when splitting styles
    for (int i = matchList.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& match = matchList[i];
        int urlStart = match.capturedStart();
        int urlLength = match.capturedLength();
        QString url = match.captured();

        // Find which style(s) this URL spans
        quint16 currentPos = 0;
        for (size_t styleIdx = 0; styleIdx < line->styleList.size(); ++styleIdx) {
            Style* style = line->styleList[styleIdx].get();
            quint16 styleEnd = currentPos + style->iLength;

            // Check if URL overlaps with this style
            if (urlStart < styleEnd && urlStart + urlLength > currentPos) {
                // URL overlaps this style - we need to split it

                // Calculate positions relative to this style
                int relativeStart = urlStart - currentPos;
                int relativeEnd = urlStart + urlLength - currentPos;

                // Clamp to style boundaries
                if (relativeStart < 0)
                    relativeStart = 0;
                if (relativeEnd > style->iLength)
                    relativeEnd = style->iLength;

                // Split style into three parts: before URL, URL, after URL
                SplitStyleForURL(line, styleIdx, relativeStart, relativeEnd - relativeStart, url);

                // After splitting, we've inserted new styles, so we need to adjust our iteration
                // The split function handles updating the style list
                break; // Move to next URL
            }

            currentPos = styleEnd;
        }
    }
}

/**
 * SplitStyleForURL - Split a style to create a hyperlink section
 *
 * Given a style that contains a URL, splits it into up to three parts:
 * 1. Before URL (if any) - original style
 * 2. URL section - original style + ACTION_HYPERLINK + Action object
 * 3. After URL (if any) - original style
 *
 * @param line The line being processed
 * @param styleIdx Index of the style to split
 * @param urlStart Start position of URL within this style
 * @param urlLength Length of URL
 * @param url The URL string
 */
void WorldDocument::SplitStyleForURL(Line* line, size_t styleIdx, int urlStart, int urlLength,
                                     const QString& url)
{
    Style* originalStyle = line->styleList[styleIdx].get();

    // Create Action object for the URL
    auto action = std::make_shared<Action>(url,       // action = URL to open
                                           url,       // hint = show URL in tooltip
                                           QString(), // variable = none for auto-detected URLs
                                           this);     // world document

    // Three cases:
    // 1. URL at start: [URL][rest]
    // 2. URL in middle: [before][URL][after]
    // 3. URL at end: [before][URL]

    std::vector<std::unique_ptr<Style>> newStyles;

    // Part 1: Before URL (if URL doesn't start at position 0)
    if (urlStart > 0) {
        auto beforeStyle = std::make_unique<Style>();
        beforeStyle->iLength = urlStart;
        beforeStyle->iFlags = originalStyle->iFlags;
        beforeStyle->iForeColour = originalStyle->iForeColour;
        beforeStyle->iBackColour = originalStyle->iBackColour;
        beforeStyle->pAction = originalStyle->pAction; // Keep original action if any
        newStyles.push_back(std::move(beforeStyle));
    }

    // Part 2: URL section with hyperlink
    auto urlStyle = std::make_unique<Style>();
    urlStyle->iLength = urlLength;
    urlStyle->iFlags = originalStyle->iFlags | ACTION_HYPERLINK | UNDERLINE;
    urlStyle->iForeColour = BGR(0, 0, 255); // Blue for URLs
    urlStyle->iBackColour = originalStyle->iBackColour;
    urlStyle->pAction = action;
    newStyles.push_back(std::move(urlStyle));

    // Part 3: After URL (if there's text after the URL)
    int afterLength = originalStyle->iLength - (urlStart + urlLength);
    if (afterLength > 0) {
        auto afterStyle = std::make_unique<Style>();
        afterStyle->iLength = afterLength;
        afterStyle->iFlags = originalStyle->iFlags;
        afterStyle->iForeColour = originalStyle->iForeColour;
        afterStyle->iBackColour = originalStyle->iBackColour;
        afterStyle->pAction = originalStyle->pAction; // Keep original action if any
        newStyles.push_back(std::move(afterStyle));
    }

    // Replace the original style with the split styles
    // Erase the original
    line->styleList.erase(line->styleList.begin() + styleIdx);

    // Insert the new styles at the same position
    line->styleList.insert(line->styleList.begin() + styleIdx,
                           std::make_move_iterator(newStyles.begin()),
                           std::make_move_iterator(newStyles.end()));
}

/**
 * addToCommandHistory - Add command to history list
 *
 * Command History
 * Consecutive duplicate filtering using m_last_command
 *
 * Based on CSendView::AddToCommandHistory() from sendvw.cpp
 *
 * Adds a command to the history list, enforcing:
 * - No empty commands
 * - Skip if echo suppressed (m_bNoEcho) unless m_bAlwaysRecordCommandHistory
 * - Skip consecutive duplicates (command == m_last_command)
 * - Maximum size limit (m_nHistoryLines)
 *
 * After adding, resets history position to end (ready for browsing).
 */
void WorldDocument::addToCommandHistory(const QString& command)
{
    // Don't add empty commands
    if (command.trimmed().isEmpty()) {
        return;
    }

    // Don't record if server requested no echo (password entry) unless overridden
    // Based on sendvw.cpp: !(pDoc->m_bNoEcho && !pDoc->m_bAlwaysRecordCommandHistory)
    if (m_telnetParser->m_bNoEcho && !m_bAlwaysRecordCommandHistory) {
        qCDebug(lcWorld) << "Command history: skipping due to echo suppression";
        return;
    }

    // Don't record commands identical to the previous one (consecutive duplicates only)
    // Based on sendvw.cpp: strCommand != m_last_command
    if (command == m_last_command) {
        qCDebug(lcWorld) << "Command history: skipping consecutive duplicate:" << command;
        return;
    }

    // Add to end of history
    m_commandHistory.append(command);

    // Trim to max size (remove oldest if over limit)
    // Use m_input.history_lines instead of m_maxCommandHistory (sendvw.cpp)
    while (m_commandHistory.count() > m_input.history_lines) {
        m_commandHistory.removeFirst();
    }

    // Save as last command for consecutive duplicate check
    m_last_command = command;

    // Reset position to end (ready for Up arrow to recall)
    m_historyPosition = m_commandHistory.count();

    // Set history status to bottom (sendvw.cpp)
    m_iHistoryStatus = HistoryStatus::eAtBottom;

    qCDebug(lcWorld) << "Command history: added" << command
                     << "- history size:" << m_commandHistory.count()
                     << "/ max:" << m_input.history_lines;
}

/**
 * clearCommandHistory - Clear all command history
 *
 * Command History Navigation
 *
 * Based on CSendView::OnDisplayClearCommandHistory() from sendvw.cpp
 *
 * Clears all stored command history and resets navigation state.
 */
void WorldDocument::clearCommandHistory()
{
    // Clear the history list
    m_commandHistory.clear();

    // Reset position to end (ready for new commands)
    m_historyPosition = 0;

    // Reset history status
    m_iHistoryStatus = HistoryStatus::eAtBottom;

    // Clear last command (for consecutive duplicate check)
    m_last_command.clear();

    qCDebug(lcWorld) << "Command history cleared";
}

// ========== Script Output Methods ==========
// Implementations moved to OutputFormatter (output_formatter.cpp).
// WorldDocument provides inline forwarding wrappers in world_document.h.

// ========== Script Loading and Parsing ==========

/**
 * loadScriptFile - Load and execute script file
 *
 * Script Loading and Parsing
 * Based on CMUSHclientDoc::LoadScriptFile() and Execute_Script_File()
 *
 * Reads the script file from disk and executes it using parseLua().
 * The script file path is stored in m_scripting.filename.
 *
 * This is typically called:
 * - When world is opened
 * - When user manually reloads script (Ctrl+R)
 * - When script file is modified (if file watching enabled)
 */
void WorldDocument::loadScriptFile()
{
    // Check if script filename is set
    if (m_scripting.filename.isEmpty()) {
        qCDebug(lcWorld) << "loadScriptFile: No script filename set";
        return;
    }

    // Check if scripting engine exists
    if (!m_ScriptEngine || !m_ScriptEngine->L) {
        qWarning() << "loadScriptFile: No scripting engine";
        note("Cannot load script file: Scripting engine not initialized");
        return;
    }

    qCDebug(lcWorld) << "loadScriptFile: Loading" << m_scripting.filename;

    // Open script file
    QFile file(m_scripting.filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("Cannot open script file: %1").arg(m_scripting.filename);
        qWarning() << error;
        colourNote(BGR(255, 0, 0), BGR(0, 0, 0), error);
        return;
    }

    // Read entire file
    QByteArray scriptData = file.readAll();
    QString scriptCode = QString::fromUtf8(scriptData);
    file.close();

    qCDebug(lcWorld) << "loadScriptFile: Read" << scriptData.size() << "bytes";

    // Parse and execute the script
    bool error = m_ScriptEngine->parseLua(scriptCode, "Script file");

    if (error) {
        // Orange error text on black background
        colourNote(BGR(255, 140, 0), BGR(0, 0, 0),
                   QString("Script file contains errors: %1").arg(m_scripting.filename));
    } else {
        qCDebug(lcWorld) << "loadScriptFile: Script executed successfully";
        // Optional: notify user
        // note(QString("Script file loaded: %1").arg(m_scripting.filename));
    }

    // Update file modification time for change detection
    QFileInfo fileInfo(m_scripting.filename);
    if (fileInfo.exists()) {
        m_timeScriptFileMod = fileInfo.lastModified();
    }
}

/**
 * setupScriptFileWatcher - Set up file watcher for script file changes
 *
 * Creates or updates the QFileSystemWatcher to monitor the script file.
 * Called after setting m_scripting.filename or loading a world.
 *
 * Based on CMUSHclientDoc file watching behavior in scripting.cpp
 */
void WorldDocument::setupScriptFileWatcher()
{
    // Clean up existing watcher
    if (m_scriptFileWatcher) {
        delete m_scriptFileWatcher;
        m_scriptFileWatcher = nullptr;
    }

    // Don't set up watcher if no script file or if reload option is "never"
    if (m_scripting.filename.isEmpty()) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: No script file to watch";
        return;
    }

    if (m_scripting.reload_option == ScriptReloadOption::eReloadNever) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: Reload option is 'never', not watching";
        return;
    }

    // Check if script file exists
    QFileInfo fileInfo(m_scripting.filename);
    if (!fileInfo.exists()) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: Script file does not exist:"
                         << m_scripting.filename;
        return;
    }

    // Create file watcher without Qt parent-child ownership.
    // Manual delete in setupScriptFileWatcher (reset) and ~WorldDocument (final cleanup)
    // are the sole owners. Passing 'this' as parent would cause double-free: Qt would
    // delete the watcher again after ~WorldDocument already did.
    m_scriptFileWatcher = new QFileSystemWatcher();
    m_scriptFileWatcher->addPath(m_scripting.filename);

    // Connect file change signal
    connect(m_scriptFileWatcher, &QFileSystemWatcher::fileChanged, this,
            &WorldDocument::onScriptFileChanged);

    // Store initial modification time
    m_timeScriptFileMod = fileInfo.lastModified();

    qCDebug(lcWorld) << "setupScriptFileWatcher: Watching" << m_scripting.filename;
}

/**
 * onScriptFileChanged - Handle script file change notification
 *
 * Called when QFileSystemWatcher detects the script file has changed.
 * Behavior depends on m_scripting.reload_option:
 * - ScriptReloadOption::eReloadConfirm (0): Show dialog asking user
 * - ScriptReloadOption::eReloadAlways (1): Automatically reload
 * - ScriptReloadOption::eReloadNever (2): Ignore (but shouldn't get here since watcher not set up)
 *
 * Based on CMUSHclientDoc::OnScriptFileChange() in scripting.cpp
 *
 * @param path Path to the file that changed
 */
void WorldDocument::onScriptFileChanged(const QString& path)
{
    Q_UNUSED(path);

    // Prevent re-entrancy
    if (m_bInScriptFileChanged) {
        return;
    }
    m_bInScriptFileChanged = true;

    qCDebug(lcWorld) << "onScriptFileChanged: Script file changed:" << m_scripting.filename;

    // Check if file still exists (may have been deleted)
    QFileInfo fileInfo(m_scripting.filename);
    if (!fileInfo.exists()) {
        qCDebug(lcWorld) << "onScriptFileChanged: Script file no longer exists";
        m_bInScriptFileChanged = false;
        return;
    }

    // Check modification time to avoid spurious notifications
    QDateTime newModTime = fileInfo.lastModified();
    if (newModTime == m_timeScriptFileMod) {
        qCDebug(lcWorld) << "onScriptFileChanged: Modification time unchanged, ignoring";
        m_bInScriptFileChanged = false;
        return;
    }

    // Handle based on reload option
    switch (m_scripting.reload_option) {
        case ScriptReloadOption::eReloadAlways:
            // Automatically reload
            qCDebug(lcWorld) << "onScriptFileChanged: Auto-reloading script";
            note(QString("Script file changed, reloading: %1")
                     .arg(QFileInfo(m_scripting.filename).fileName()));
            loadScriptFile();
            break;

        case ScriptReloadOption::eReloadConfirm: {
            // Show confirmation dialog
            QString filename = QFileInfo(m_scripting.filename).fileName();
            int result = QMessageBox::question(
                nullptr, "Script File Changed",
                QString("The script file '%1' has been modified.\n\nDo you want to reload it?")
                    .arg(filename),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            if (result == QMessageBox::Yes) {
                qCDebug(lcWorld) << "onScriptFileChanged: User chose to reload";
                note(QString("Script file reloaded: %1").arg(filename));
                loadScriptFile();
            } else {
                qCDebug(lcWorld) << "onScriptFileChanged: User declined reload";
                // Update stored mod time so we don't ask again for same change
                m_timeScriptFileMod = newModTime;
            }
            break;
        }

        case ScriptReloadOption::eReloadNever:
        default:
            // Ignore changes
            qCDebug(lcWorld) << "onScriptFileChanged: Ignoring (reload option is 'never')";
            m_timeScriptFileMod = newModTime;
            break;
    }

    // Re-add the file to the watcher (QFileSystemWatcher removes it after notification)
    if (m_scriptFileWatcher && fileInfo.exists()) {
        m_scriptFileWatcher->addPath(m_scripting.filename);
    }

    m_bInScriptFileChanged = false;
}

/**
 * showErrorLines - Display error context around line
 *
 * Script Loading and Parsing
 * Based on CMUSHclientDoc::ShowErrorLines()
 *
 * Displays 3 lines before and after the error line from the script file,
 * with the error line highlighted with ">>> " prefix.
 *
 * @param lineNumber Line number with error (1-based)
 */
void WorldDocument::showErrorLines(int lineNumber)
{
    // Check if script filename is set
    if (m_scripting.filename.isEmpty()) {
        return;
    }

    // Open script file
    QFile file(m_scripting.filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    // Read all lines
    QTextStream in(&file);
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }
    file.close();

    // Calculate range (3 lines before and after)
    int start = qMax(0, lineNumber - 4); // lineNumber is 1-based, so -4 gives us 3 lines before
    int end = qMin(lines.count(), lineNumber + 3);

    // Display context - orange error text on black background
    for (int i = start; i < end; i++) {
        QString prefix = (i + 1 == lineNumber) ? ">>> " : "    ";
        colourNote(BGR(255, 140, 0), BGR(0, 0, 0),
                   QString("%1%2: %3").arg(prefix).arg(i + 1).arg(lines[i]));
    }
}

// ============================================================================
// Trigger/Alias XML Serialization
// ============================================================================

// SAMECOLOUR constant from original MUSHclient
#define SAMECOLOUR 65535

// Style bit masks from original MUSHclient
#define HILITE 0x0001
#define UNDERLINE 0x0002
#define BLINK 0x0004 // italic
#define INVERSE 0x0008


/**
 * Repaint - Trigger a UI repaint
 *
 * Forces an immediate repaint of the active output view.
 * Used by plugins to refresh miniwindows after updates.
 */
void WorldDocument::Repaint()
{
    if (m_pActiveOutputView) {
        m_pActiveOutputView->requestUpdate();
    }
}

// ========== Text Selection ==========

/**
 * setSelection - Update selection state
 *
 * Called by OutputView when selection changes.
 * Coordinates are 0-based internally.
 *
 * @param startLine Selection start line (0-based)
 * @param startChar Selection start character (0-based)
 * @param endLine Selection end line (0-based)
 * @param endChar Selection end character (0-based)
 */
void WorldDocument::setSelection(int startLine, int startChar, int endLine, int endChar)
{
    m_selectionStartLine = startLine;
    m_selectionStartChar = startChar;
    m_selectionEndLine = endLine;
    m_selectionEndChar = endChar;
}

/**
 * clearSelection - Clear selection state
 *
 * Called by OutputView when selection is cleared.
 */
void WorldDocument::clearSelection()
{
    m_selectionStartLine = -1;
    m_selectionStartChar = -1;
    m_selectionEndLine = -1;
    m_selectionEndChar = -1;
}

/**
 * GetSelectionStartLine - Get the line number where selection starts
 *
 * Based on methods_info.cpp
 *
 * @return Line number (1-based) where selection starts, or 0 if no selection
 */
qint32 WorldDocument::GetSelectionStartLine() const
{
    // Check if there's a selection
    if (m_selectionStartLine < 0 || m_selectionEndLine < 0) {
        return 0;
    }

    // Normalize selection (handle backward selection)
    int startLine = m_selectionStartLine;
    int startChar = m_selectionStartChar;
    int endLine = m_selectionEndLine;
    int endChar = m_selectionEndChar;

    if (startLine > endLine || (startLine == endLine && startChar > endChar)) {
        qSwap(startLine, endLine);
        qSwap(startChar, endChar);
    }

    // Check if there's actually a selection (not just a cursor position)
    if (!(endLine > startLine || (endLine == startLine && endChar > startChar))) {
        return 0;
    }

    return startLine + 1; // Convert to 1-based
}

/**
 * GetSelectionEndLine - Get the line number where selection ends
 *
 * Based on methods_info.cpp
 *
 * @return Line number (1-based) where selection ends, or 0 if no selection
 */
qint32 WorldDocument::GetSelectionEndLine() const
{
    // Check if there's a selection
    if (m_selectionStartLine < 0 || m_selectionEndLine < 0) {
        return 0;
    }

    // Normalize selection (handle backward selection)
    int startLine = m_selectionStartLine;
    int startChar = m_selectionStartChar;
    int endLine = m_selectionEndLine;
    int endChar = m_selectionEndChar;

    if (startLine > endLine || (startLine == endLine && startChar > endChar)) {
        qSwap(startLine, endLine);
        qSwap(startChar, endChar);
    }

    // Check if there's actually a selection (not just a cursor position)
    if (!(endLine > startLine || (endLine == startLine && endChar > startChar))) {
        return 0;
    }

    return endLine + 1; // Convert to 1-based
}

/**
 * GetSelectionStartColumn - Get the column where selection starts
 *
 * Based on methods_info.cpp
 *
 * @return Column (1-based) where selection starts, or 0 if no selection
 */
qint32 WorldDocument::GetSelectionStartColumn() const
{
    // Check if there's a selection
    if (m_selectionStartLine < 0 || m_selectionEndLine < 0) {
        return 0;
    }

    // Normalize selection (handle backward selection)
    int startLine = m_selectionStartLine;
    int startChar = m_selectionStartChar;
    int endLine = m_selectionEndLine;
    int endChar = m_selectionEndChar;

    if (startLine > endLine || (startLine == endLine && startChar > endChar)) {
        qSwap(startLine, endLine);
        qSwap(startChar, endChar);
    }

    // Check if there's actually a selection (not just a cursor position)
    if (!(endLine > startLine || (endLine == startLine && endChar > startChar))) {
        return 0;
    }

    return startChar + 1; // Convert to 1-based
}

/**
 * GetSelectionEndColumn - Get the column where selection ends
 *
 * Based on methods_info.cpp
 *
 * @return Column (1-based) where selection ends, or 0 if no selection
 */
qint32 WorldDocument::GetSelectionEndColumn() const
{
    // Check if there's a selection
    if (m_selectionStartLine < 0 || m_selectionEndLine < 0) {
        return 0;
    }

    // Normalize selection (handle backward selection)
    int startLine = m_selectionStartLine;
    int startChar = m_selectionStartChar;
    int endLine = m_selectionEndLine;
    int endChar = m_selectionEndChar;

    if (startLine > endLine || (startLine == endLine && startChar > endChar)) {
        qSwap(startLine, endLine);
        qSwap(startChar, endChar);
    }

    // Check if there's actually a selection (not just a cursor position)
    if (!(endLine > startLine || (endLine == startLine && endChar > startChar))) {
        return 0;
    }

    return endChar + 1; // Convert to 1-based
}

// ============================================================
// IWorldContext Implementation
// ============================================================

bool WorldDocument::isConnectedToMud() const
{
    return m_connectionManager && m_connectionManager->m_iConnectPhase == eConnectConnectedToMud;
}

void WorldDocument::flushLogIfNeeded()
{
    if (m_logfile && m_logfile->isOpen()) {
        QDateTime now = QDateTime::currentDateTime();
        qint64 elapsed = m_LastFlushTime.secsTo(now);
        if (elapsed > 120) {
            m_LastFlushTime = now;
            QString savedFileName = m_logfile_name;
            m_logfile->close();
            QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
            if (!m_logfile->open(mode)) {
                qCDebug(lcLogging)
                    << "flushLogIfNeeded: Failed to reopen log file" << savedFileName;
            }
        }
    }
}
