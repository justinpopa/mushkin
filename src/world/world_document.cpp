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
#include "view_interfaces.h"
#include "logging.h"
#include "script_engine.h"
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
    // Create the network socket
    // WorldSocket calls our methods directly via m_pDoc pointer:
    // - onReadyRead() calls ReceiveMsg()
    // - onConnected() calls OnConnect(0)
    // - onError() calls OnConnect(errorCode)
    m_pSocket = new WorldSocket(this, this);

    // ========== Initialize connection settings ==========
    m_server = QString();
    m_mush_name = QString();
    m_name = QString();
    m_password = QString();
    m_port = 4000; // Default MUD port
    m_connect_now = eNoAutoConnect;

    // ========== Initialize display settings ==========
    m_font_name = "Courier New";
    m_font_height = 12; // matches original MUSHclient default
    m_font_weight = 400; // FW_NORMAL
    m_font_charset = 0;  // DEFAULT_CHARSET
    m_wrap = 1; // true = wrap enabled (matches original MUSHclient default)
    m_timestamps = 0;
    m_match_width = 30;

    // ========== Initialize colors ==========
    initializeColors();

    // ========== Initialize input colors and font ==========
    // Colors are stored in BGR format (Windows COLORREF: 0x00BBGGRR)
    m_input_text_colour = BGR(0, 0, 0);              // Black text
    m_input_background_colour = BGR(255, 255, 255);  // White background
    m_input_font_height = 12;
    m_input_font_name = "Courier New";
    m_input_font_italic = 0;
    m_input_font_weight = 400; // FW_NORMAL
    m_input_font_charset = 0;

    // ========== Initialize buffer settings ==========
    m_maxlines = 5000;
    m_nHistoryLines = 1000;
    m_nWrapColumn = 80;

    // ========== Initialize triggers, aliases, timers - Enable Flags ==========
    m_enable_aliases = 1;
    m_enable_triggers = 1;
    m_bEnableTimers = 1;

    // ========== Initialize Trigger/Alias/Timer Collections ==========
    // m_AliasMap and m_TriggerMap are QMap/QVector (auto-initialized)
    // m_AliasArray and m_TriggerArray are QVector (auto-initialized)
    // m_TimerMap and m_TimerRevMap are QMap (auto-initialized)
    m_triggersNeedSorting = false; // No triggers yet, no sort needed
    m_aliasesNeedSorting = false;  // No aliases yet, no sort needed

    // ========== Initialize input handling ==========
    m_display_my_input = 1;
    m_echo_colour = 65535; // SAMECOLOUR
    m_bEscapeDeletesInput = 0;
    m_bArrowsChangeHistory = 1;
    m_bConfirmOnPaste = 1;              // Default: true (like original)

    // ========== Initialize command history ==========
    m_commandHistory.clear();
    m_maxCommandHistory = 20; // Default: 20 commands (matches original)
    m_historyPosition = 0;
    m_bFilterDuplicates = false;  // Don't filter duplicates by default
    m_last_command = QString();   // No previous command yet
    m_iHistoryStatus = eAtBottom; // Start at bottom (ready for new commands)

    // ========== Initialize sound ==========
    m_enable_beeps = 1;
    m_enable_trigger_sounds = 1;
    m_new_activity_sound = QString();
    m_strBeepSound = QString();

    // ========== Initialize macros ==========
    for (int i = 0; i < MACRO_COUNT; i++) {
        m_macros[i] = QString();
        m_macro_type[i] = 0; // SEND_NOW
        m_macro_name[i] = QString();
    }

    // ========== Initialize keypad ==========
    for (int i = 0; i < KEYPAD_MAX_ITEMS; i++) {
        m_keypad[i] = QString();
    }
    m_keypad_enable = 0;

    // ========== Initialize speed walking ==========
    m_enable_speed_walk = 0;
    m_speed_walk_prefix = QString();
    m_strSpeedWalkFiller = QString();
    m_iSpeedWalkDelay = 0;

    // ========== Initialize command stack ==========
    m_enable_command_stack = 0;
    m_strCommandStackCharacter = ";";

    // ========== Initialize connection text ==========
    m_connect_text = QString();

    // ========== Initialize file handling ==========
    m_file_postamble = QString();
    m_file_preamble = QString();
    m_line_postamble = QString();
    m_line_preamble = QString();
    m_strLogFilePreamble = QString();

    // ========== Initialize paste settings ==========
    m_paste_postamble = QString();
    m_paste_preamble = QString();
    m_pasteline_postamble = QString();
    m_pasteline_preamble = QString();

    // ========== Initialize notes ==========
    m_notes = QString();

    // ========== Initialize scripting ==========
    m_strLanguage = "Lua";
    m_bEnableScripts = 1;
    m_strScriptFilename = QString();
    m_strScriptPrefix = "/";
    m_strScriptEditor = QString();
    m_strScriptEditorArgument = QString();

    // ========== Initialize script event handlers ==========
    m_strWorldOpen = QString();
    m_strWorldClose = QString();
    m_strWorldSave = QString();
    m_strWorldConnect = QString();
    m_strWorldDisconnect = QString();
    m_strWorldGetFocus = QString();
    m_strWorldLoseFocus = QString();

    // ========== Initialize MXP ==========
    m_iUseMXP = 2; // eOnCommandMXP - Default: on command (like original MUSHclient)
    m_iMXPdebugLevel = 0;
    m_strOnMXP_Start = QString();
    m_strOnMXP_Stop = QString();
    m_strOnMXP_Error = QString();
    m_strOnMXP_OpenTag = QString();
    m_strOnMXP_CloseTag = QString();
    m_strOnMXP_SetVariable = QString();

    // ========== Initialize hyperlinks ==========
    m_iHyperlinkColour = BGR(255, 128, 0); // Light blue - RGB(0, 128, 255) like original

    // ========== Initialize misc flags ==========
    m_indent_paras = 1;                 // Default: true (like original)
    m_bSaveWorldAutomatically = 0;
    m_bLineInformation = 1;             // Default: true (like original)
    m_bStartPaused = 0;
    m_iNoteTextColour = 4;              // Default: 4 (cyan) like original
    m_bKeepCommandsOnSameLine = 0;

    // ========== Initialize auto-say ==========
    m_strAutoSayString = "say ";        // Default: "say " (like original)
    m_bEnableAutoSay = 0;
    m_bExcludeMacros = 0;
    m_bExcludeNonAlpha = 0;
    m_strOverridePrefix = "-";          // Default: "-" (like original)
    m_bConfirmBeforeReplacingTyping = 1; // Default: true (like original)
    m_bReEvaluateAutoSay = 0;

    // ========== Initialize Script Variables Collection ==========
    // m_VariableMap is automatically initialized as empty QMap

    // ========== Initialize print styles ==========
    for (int i = 0; i < 8; i++) {
        m_nNormalPrintStyle[i] = 0;
        m_nBoldPrintStyle[i] = 0;
    }

    // ========== Initialize display options (version 9+) ==========
    m_bShowBold = 0;                    // Default: false (like original)
    m_bShowItalic = 1;
    m_bShowUnderline = 1;
    m_bAltArrowRecallsPartial = 0;
    m_iPixelOffset = 1;                 // Default: 1 (like original)
    m_bAutoFreeze = 1;                  // Default: true (like original) - auto_pause
    m_bKeepFreezeAtBottom = 0;
    m_bAutoRepeat = 0;
    m_bDisableCompression = 0;
    m_bLowerCaseTabCompletion = 0;
    m_bDoubleClickInserts = 0;
    m_bDoubleClickSends = 0;
    m_bConfirmOnSend = 1;               // Default: true (like original)
    m_bTranslateGerman = 0;

    // ========== Initialize tab completion ==========
    m_strTabCompletionDefaults = QString();
    m_iTabCompletionLines = 200;
    m_bTabCompletionSpace = 0;          // Default: false (like original)
    m_strWordDelimiters = "-._~!@#$%^&*()+=[]{}\\|;:'\",<>?/"; // Word delimiters
    m_bTabCompleteFunctions = true; // Show Lua functions in Shift+Tab menu by default
    // m_ExtraShiftTabCompleteItems initialized empty by default

    // ========== Initialize auto logging ==========
    m_strAutoLogFileName = QString();
    m_strLogLinePreambleOutput = QString();
    m_strLogLinePreambleInput = QString();
    m_strLogLinePreambleNotes = QString();
    m_strLogFilePostamble = QString();
    m_strLogLinePostambleOutput = QString();
    m_strLogLinePostambleInput = QString();
    m_strLogLinePostambleNotes = QString();

    // ========== Initialize output line preambles ==========
    // Colors stored in BGR format (Windows COLORREF)
    m_strOutputLinePreambleOutput = QString();
    m_strOutputLinePreambleInput = QString();
    m_strOutputLinePreambleNotes = QString();
    m_OutputLinePreambleOutputTextColour = BGR(255, 255, 255); // White (like original)
    m_OutputLinePreambleOutputBackColour = BGR(0, 0, 0);       // Black
    m_OutputLinePreambleInputTextColour = BGR(0, 0, 128);      // Dark red RGB(128,0,0) (like original)
    m_OutputLinePreambleInputBackColour = BGR(0, 0, 0);        // Black
    m_OutputLinePreambleNotesTextColour = BGR(255, 0, 0);      // Blue RGB(0,0,255) (like original)
    m_OutputLinePreambleNotesBackColour = BGR(0, 0, 0);        // Black

    // ========== Initialize recall window ==========
    m_strRecallLinePreamble = QString();

    // ========== Initialize paste/file options ==========
    m_bPasteCommentedSoftcode = 0;
    m_bFileCommentedSoftcode = 0;
    m_bFlashIcon = 0; // Off by default (like original MUSHclient)
    m_bArrowKeysWrap = 0;
    m_bSpellCheckOnSend = 0;
    m_nPasteDelay = 0;
    m_nFileDelay = 0;
    m_nPasteDelayPerLines = 1;
    m_nFileDelayPerLines = 1;

    // ========== Initialize miscellaneous options ==========
    m_nReloadOption = 0; // eReloadConfirm
    m_bUseDefaultOutputFont = 0;
    m_bSaveDeletedCommand = 0;
    m_bTranslateBackslashSequences = 0;
    m_bEditScriptWithNotepad = 1;       // Default: true (like original)
    m_bWarnIfScriptingInactive = 1;     // Default: true (like original)

    // ========== Initialize sending options ==========
    m_bWriteWorldNameToLog = 1;         // Default: true (like original)
    m_bSendEcho = 0;
    m_bPasteEcho = 0;

    // ========== Initialize default options ==========
    m_bUseDefaultColours = 0;
    m_bUseDefaultTriggers = 0;
    m_bUseDefaultAliases = 0;
    m_bUseDefaultMacros = 0;
    m_bUseDefaultTimers = 0;
    m_bUseDefaultInputFont = 0;

    // ========== Initialize terminal settings ==========
    m_strTerminalIdentification = "mushkin"; // Identify as Mushkin

    // ========== Initialize mapping ==========
    m_strMappingFailure = "Alas, you cannot go that way."; // Default like original
    m_bMapFailureRegexp = 0;

    // ========== Initialize flag containers ==========
    m_iFlags1 = 0;
    m_iFlags2 = 0;

    // ========== Initialize world ID ==========
    m_strWorldID = QString(); // Will be generated on save

    // ========== Initialize more options (version 15+) ==========
    m_bAlwaysRecordCommandHistory = 0;
    m_bCopySelectionToClipboard = 0;
    m_bCarriageReturnClearsLine = 0;
    m_bSendMXP_AFK_Response = 1;
    m_bMudCanChangeOptions = 1;
    m_bEnableSpamPrevention = 0;
    m_iSpamLineCount = 20;
    m_strSpamMessage = "look";

    m_bDoNotShowOutstandingLines = 0;
    m_bDoNotTranslateIACtoIACIAC = 0;

    // ========== Initialize clipboard and display ==========
    m_bAutoCopyInHTML = 0;
    m_iLineSpacing = 0;
    m_bUTF_8 = 0;
    m_bConvertGAtoNewline = 0;
    m_iCurrentActionSource = 0; // eUnknownActionSource

    // ========== Initialize filters ==========
    m_strTriggersFilter = QString();
    m_strAliasesFilter = QString();
    m_strTimersFilter = QString();
    m_strVariablesFilter = QString();

    // ========== Initialize script errors ==========
    m_bScriptErrorsToOutputWindow = 0;
    m_bLogScriptErrors = 0;

    // ========== Initialize command window auto-resize ==========
    m_bAutoResizeCommandWindow = 0;
    m_strEditorWindowName = QString();
    m_iAutoResizeMinimumLines = 1;
    m_iAutoResizeMaximumLines = 20;
    m_bDoNotAddMacrosToCommandHistory = 0;
    m_bSendKeepAlives = 0;

    // ========== Initialize default trigger settings ==========
    m_iDefaultTriggerSendTo = 0;
    m_iDefaultTriggerSequence = 100;
    m_bDefaultTriggerRegexp = 0;
    m_bDefaultTriggerExpandVariables = 0;
    m_bDefaultTriggerKeepEvaluating = 0;
    m_bDefaultTriggerIgnoreCase = 0;

    // ========== Initialize default alias settings ==========
    m_iDefaultAliasSendTo = 0;
    m_iDefaultAliasSequence = 100;
    m_bDefaultAliasRegexp = 0;
    m_bDefaultAliasExpandVariables = 0;
    m_bDefaultAliasKeepEvaluating = 0;
    m_bDefaultAliasIgnoreCase = 0;

    // ========== Initialize default timer settings ==========
    m_iDefaultTimerSendTo = 0;

    // ========== Initialize sound ==========
    m_bPlaySoundsInBackground = 0;

    // ========== Initialize HTML logging ==========
    m_bLogHTML = 0;
    m_bUnpauseOnSend = 0;

    // ========== Initialize logging options ==========
    m_log_input = 0;
    m_bLogOutput = 1; // Log output by default
    m_bLogNotes = 0;
    m_bLogInColour = 0;
    m_bLogRaw = 0;

    // ========== Initialize tree views ==========
    m_bTreeviewTriggers = 1;
    m_bTreeviewAliases = 1;
    m_bTreeviewTimers = 1;

    // ========== Initialize input wrapping ==========
    m_bAutoWrapInput = 0;

    // ========== Initialize tooltips ==========
    m_iToolTipVisibleTime = 30000; // 30 seconds
    m_iToolTipStartTime = 500;     // 0.5 seconds

    // ========== Initialize save file options ==========
    m_bOmitSavedDateFromSaveFiles = 0;

    // ========== Initialize output buffer fading ==========
    m_iFadeOutputBufferAfterSeconds = 0;
    m_FadeOutputOpacityPercent = 20;
    m_FadeOutputSeconds = 8;
    m_bCtrlBackspaceDeletesLastWord = 0;

    // ========== Initialize remote access server settings ==========
    m_bEnableRemoteAccess = 0;
    m_iRemotePort = 0;
    m_strRemotePassword = QString();
    m_iRemoteScrollbackLines = 100;
    m_iRemoteMaxClients = 5;
    m_iRemoteLockoutAttempts = 3;
    m_iRemoteLockoutSeconds = 300;
    // m_pRemoteServer is automatically nullptr via unique_ptr

    // ========== RUNTIME STATE VARIABLES (not saved to disk) ==========

    // ========== Deprecated/Legacy (pre-version 11) ==========
    m_page_colour = 0;
    m_whisper_colour = 0;
    m_mail_colour = 0;
    m_game_colour = 0;
    m_remove_channels1 = 0;
    m_remove_channels2 = 0;
    m_remove_pages = 0;
    m_remove_whispers = 0;
    m_remove_set = 0;
    m_remove_mail = 0;
    m_remove_game = 0;

    // ========== Runtime State Flags ==========
    m_bNAWS_wanted = false;
    m_bCHARSET_wanted = false;
    m_bLoaded = false;
    m_bSelected = false;
    m_bVariablesChanged = false;
    m_bModified = false;
    m_bNoEcho = false;
    m_bDebugIncomingPackets = false;

    // ========== Statistics Counters ==========
    m_iInputPacketCount = 0;
    m_iOutputPacketCount = 0;
    m_iUTF8ErrorCount = 0;
    m_iOutputWindowRedrawCount = 0;

    // ========== UTF-8 Processing State ==========
    for (int i = 0; i < 8; i++) {
        m_UTF8Sequence[i] = 0;
    }
    m_iUTF8BytesLeft = 0;

    // ========== Trigger/Alias/Timer Statistics ==========
    m_iTriggersEvaluatedCount = 0;
    m_iTriggersMatchedCount = 0;
    m_iAliasesEvaluatedCount = 0;
    m_iAliasesMatchedCount = 0;
    m_iTimersFiredCount = 0;
    m_iTriggersMatchedThisSessionCount = 0;
    m_iAliasesMatchedThisSessionCount = 0;
    m_iTimersFiredThisSessionCount = 0;

    // ========== UI State ==========
    m_last_prefs_page = 0;
    m_bConfigEnableTimers = 0;
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
    // m_lineList initializes to empty QList automatically
    m_currentLine = nullptr;
    m_strCurrentLine = QString();

    // ========== Actions List ==========
    m_ActionList = nullptr;

    // ========== Line Position Tracking ==========
    m_pLinePositions = nullptr;
    m_total_lines = 0;
    m_new_lines = 0;
    m_newlines_received = 0;
    m_nTotalLinesSent = 0;
    m_nTotalLinesReceived = 0;
    m_last_line_with_IAC_GA = 0;

    // ========== Timing Variables ==========
    m_tConnectTime = QDateTime();
    m_tLastPlayerInput = QDateTime();
    m_tsConnectDuration = 0;
    m_whenWorldStarted = QDateTime();
    m_whenWorldStartedHighPrecision = 0;
    m_tStatusTime = QDateTime();
    m_lastMousePosition = QPoint(0, 0);
    m_view_number = 0;

    // ========== Telnet Negotiation Phase ==========
    m_phase = NONE; // Start in normal text mode
    m_ttype_sequence = 0;

    // ========== MCCP Compression ==========
    memset(&m_zCompress, 0, sizeof(m_zCompress)); // Zero-initialize z_stream
    m_bCompress = false;
    m_bCompressInitOK = false;
    m_CompressInput = nullptr;
    m_CompressOutput = nullptr;
    m_nTotalUncompressed = 0;
    m_nTotalCompressed = 0;
    m_iCompressionTimeTaken = 0;
    m_nCompressionOutputBufferSize = COMPRESS_BUFFER_LENGTH; // Default 20000
    m_iMCCP_type = 0;
    m_bSupports_MCCP_2 = false;

    // ========== Telnet Subnegotiation ==========
    m_subnegotiation_type = 0;
    m_IAC_subnegotiation_data = QByteArray();

    // ========== Telnet Negotiation Arrays ==========
    for (int i = 0; i < 256; i++) {
        m_bClient_sent_IAC_DO[i] = false;
        m_bClient_sent_IAC_DONT[i] = false;
        m_bClient_sent_IAC_WILL[i] = false;
        m_bClient_sent_IAC_WONT[i] = false;
        m_bClient_got_IAC_DO[i] = false;
        m_bClient_got_IAC_DONT[i] = false;
        m_bClient_got_IAC_WILL[i] = false;
        m_bClient_got_IAC_WONT[i] = false;
    }

    // ========== MSP (MUD Sound Protocol) ==========
    m_bMSP = false;

    // ========== ZMP (Zenith MUD Protocol) ==========
    m_bZMP = false;
    m_strZMPpackage.clear();

    // ========== ATCP (Achaea Telnet Client Protocol) ==========
    m_bATCP = false;

    // ========== GMCP (Generic Mud Communication Protocol) ==========

    // ========== MXP/Pueblo Runtime State ==========
    m_bMXP = false;
    m_bPuebloActive = false;
    m_iPuebloLevel = QString();
    m_bPreMode = false;
    m_iMXP_mode = 0;
    m_iMXP_defaultMode = 0;
    m_iMXP_previousMode = 0;
    m_bInParagraph = false;
    m_bMXP_script = false;
    m_bSuppressNewline = false;
    m_bMXP_nobr = false;
    m_bMXP_preformatted = false;
    m_bMXP_centered = false;
    m_strMXP_link.clear();
    m_strMXP_hint.clear();
    m_bMXP_link_prompt = false;
    m_iMXP_list_depth = 0;
    m_iMXP_list_counter = 0;
    m_iListMode = 0;
    m_iListCount = 0;
    m_strMXPstring = QString();
    m_strMXPtagcontents = QString();
    m_cMXPquoteTerminator = 0;
    // MXP maps and lists are auto-initialized to empty
    m_cLastChar = 0;
    m_lastSpace = -1; // No space position yet
    m_iLastOutstandingTagCount = 0;
    m_strPuebloMD5 = QString();

    // ========== MXP Statistics ==========
    m_iMXPerrors = 0;
    m_iMXPtags = 0;
    m_iMXPentities = 0;

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
    m_logfile = nullptr;
    m_logfile_name = QString();
    m_LastFlushTime = QDateTime();

    // ========== Fonts ==========
    for (int i = 0; i < 16; i++) {
        m_font[i] = nullptr;
    }
    m_FontHeight = 0;
    m_FontWidth = 0;
    m_input_font = nullptr;
    m_InputFontHeight = 0;
    m_InputFontWidth = 0;

    // ========== Byte Counters ==========
    m_nBytesIn = 0;
    m_nBytesOut = 0;

    // ========== Sockets ==========
    m_sockAddr = nullptr;
    m_hNameLookup = nullptr;
    m_pGetHostStruct = nullptr;
    m_iConnectPhase = 0;

    // ========== Scripting Engine ==========
    // Create script engine and initialize Lua
    m_ScriptEngine = std::make_unique<ScriptEngine>(this, m_strLanguage, this);
    if (m_bEnableScripts) {
        m_ScriptEngine->createScriptEngine();
        qCDebug(lcWorld) << "Lua scripting initialized for world:" << worldName();
    }

    m_bSyntaxErrorOnly = false;
    m_bDisconnectOK = false;
    m_bTrace = false;
    m_bInSendToScript = false;
    m_iScriptTimeTaken = 0;
    // m_bTabCompleteFunctions and m_ExtraShiftTabCompleteItems initialized in Tab Completion
    // section
    m_strLastImmediateExpression = QString();
    m_pThread = nullptr;
    m_eventScriptFileChanged = nullptr;
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
    m_dispidOnMXP_Start = 0;
    m_dispidOnMXP_Stop = 0;
    m_dispidOnMXP_OpenTag = 0;
    m_dispidOnMXP_CloseTag = 0;
    m_dispidOnMXP_SetVariable = 0;
    m_dispidOnMXP_Error = 0;

    // ========== Plugin State ==========
    m_bPluginProcessesOpenTag = false;
    m_bPluginProcessesCloseTag = false;
    m_bPluginProcessesSetVariable = false;
    m_bPluginProcessesSetEntity = false;
    m_bPluginProcessesError = false;

    // ========== Find Info Structures ==========
    m_DisplayFindInfo = nullptr;
    m_RecallFindInfo = nullptr;
    m_TriggersFindInfo = nullptr;
    m_AliasesFindInfo = nullptr;
    m_MacrosFindInfo = nullptr;
    m_TimersFindInfo = nullptr;
    m_VariablesFindInfo = nullptr;
    m_NotesFindInfo = nullptr;
    m_bRecallCommands = false;
    m_bRecallOutput = false;
    m_bRecallNotes = false;

    // ========== Document ID ==========
    m_iUniqueDocumentNumber = 0;

    // ========== Mapping ==========
    m_strMapList = nullptr;
    m_MapFailureRegexp = nullptr;
    m_strSpecialForwards = QString();
    m_strSpecialBackwards = QString();
    m_pTimerWnd = nullptr;
    // m_CommandQueue initializes empty
    m_bShowingMapperStatus = false;
    m_strIncludeFileList = nullptr;
    m_strCurrentIncludeFileList = nullptr;

    // ========== Configuration Arrays ==========
    m_NumericConfiguration = nullptr;
    m_AlphaConfiguration = nullptr;

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

    // ========== Special Fonts ==========
    m_strSpecialFontName = nullptr;

    // ========== Background Image ==========
    m_strBackgroundImageName = QString();
    m_BackgroundBitmap = nullptr;
    m_iBackgroundMode = 0;
    m_iBackgroundColour = qRgb(0, 0, 0);

    // ========== Foreground Image ==========
    m_strForegroundImageName = QString();
    m_ForegroundBitmap = nullptr;
    m_iForegroundMode = 0;

    // ========== MiniWindows ==========
    // m_MiniWindowMap and m_MiniWindowsOrder are initialized by their default constructors

    // ========== Databases ==========
    m_Databases = nullptr;

    // ========== Text Rectangle ==========
    m_TextRectangle = QRect(0, 0, 0, 0);
    m_TextRectangleBorderOffset = 0;
    m_TextRectangleBorderColour = 0;
    m_TextRectangleBorderWidth = 0;
    m_TextRectangleOutsideFillColour = 0;
    m_TextRectangleOutsideFillStyle = 0;

    // ========== Sound System ==========
    // Sound buffers initialized in InitializeSoundSystem()

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

    // ========== Color Translation ==========
    m_ColourTranslationMap = nullptr;

    // ========== Outstanding Lines ==========
    m_OutstandingLines = nullptr;
    m_bNotesNotWantedNow = false;
    m_bDoingSimulate = false;
    m_bLineOmittedFromOutput = false;
    m_bOmitCurrentLineFromLog = false; //
    m_bScrollBarWanted = false;

    // ========== IAC Counters ==========
    m_nCount_IAC_DO = 0;
    m_nCount_IAC_DONT = 0;
    m_nCount_IAC_WILL = 0;
    m_nCount_IAC_WONT = 0;
    m_nCount_IAC_SB = 0;

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
    m_bArrowRecallsPartial = false;
    m_bCtrlZGoesToEndOfBuffer = false;
    m_bCtrlPGoesToPreviousCommand = false;
    m_bCtrlNGoesToNextCommand = false;
    m_bHyperlinkAddsToCommandHistory = false;
    m_bEchoHyperlinkInOutputWindow = false;
    m_bAutoWrapWindowWidth = false;
    m_bNAWS = false;
    m_bUseZMP = false;
    m_bUseATCP = false;
    m_bUseMSP = false;
    m_bPueblo = false;
    m_bNoEchoOff = false;
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
    // Audio system is lazy-loaded on first use (in PlaySound)
    // This avoids creating audio objects in tests and headless environments
}

WorldDocument::~WorldDocument()
{
    // Destructor - cleanup will go here in future stories
    // For now, just basic cleanup

    // ========== FIRST: Stop all timers to prevent callbacks during cleanup ==========
    // This must happen before any other cleanup to avoid race conditions
    if (m_timerCheckTimer) {
        // Disconnect all signals first to prevent any pending callbacks
        m_timerCheckTimer->disconnect();
        m_timerCheckTimer->stop();
        delete m_timerCheckTimer;
        m_timerCheckTimer = nullptr;
    }

    // Process pending events multiple times to ensure all queued callbacks are drained
    // This is especially important on Windows where timer events may be queued
    for (int i = 0; i < 3; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    // ========== Sound System Cleanup ==========
    CleanupSoundSystem();

    if (m_pSocket) {
        // Socket cleanup handled by network layer
        m_pSocket = nullptr;
    }

    // ========== Save and Clean Up Plugins ==========
    // Save plugin state for all plugins before cleanup
    for (const auto& plugin : m_PluginList) {
        if (plugin) {
            plugin->SaveState();
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
    if (m_scriptFileWatcher) {
        delete m_scriptFileWatcher;
        m_scriptFileWatcher = nullptr;
    }

    // ========== Log File Cleanup ==========
    // Close log file if open
    if (m_logfile) {
        CloseLog();
    }

    // ========== MCCP Compression Cleanup ==========
    // Clean up zlib decompression stream
    if (m_bCompressInitOK) {
        inflateEnd(&m_zCompress);
        m_bCompressInitOK = false;
    }

    // Free compression buffers
    if (m_CompressOutput) {
        free(m_CompressOutput);
        m_CompressOutput = nullptr;
    }

    if (m_CompressInput) {
        free(m_CompressInput);
        m_CompressInput = nullptr;
    }

    // ========== Line Buffer Cleanup ==========
    // Delete all lines in buffer
    qDeleteAll(m_lineList);
    m_lineList.clear();

    // Delete current line if it exists
    if (m_currentLine) {
        delete m_currentLine;
        m_currentLine = nullptr;
    }

    // ========== MiniWindow Cleanup ==========
    // Clear all miniwindows (unique_ptr handles deletion automatically)
    m_MiniWindowMap.clear();
    m_MiniWindowsOrder.clear();
}

void WorldDocument::initializeColors()
{
    // Initialize ANSI normal colors
    for (int i = 0; i < 8; i++) {
        m_normalcolour[i] = DEFAULT_NORMAL_COLORS[i];
        m_boldcolour[i] = DEFAULT_BOLD_COLORS[i];
    }

    // Initialize custom colors
    for (int i = 0; i < MAX_CUSTOM; i++) {
        m_customtext[i] = qRgb(255, 255, 255); // White
        m_customback[i] = qRgb(0, 0, 0);       // Black
    }

    // Initialize custom color names
    for (int i = 0; i < 255; i++) {
        m_strCustomColourName[i] = QString("Custom%1").arg(i + 1);
    }
}

void WorldDocument::unpackFlags()
{
    // Unpack m_iFlags1 into individual bool members
    m_bArrowRecallsPartial = (m_iFlags1 & FLAGS1_ArrowRecallsPartial) != 0;
    m_bCtrlZGoesToEndOfBuffer = (m_iFlags1 & FLAGS1_CtrlZGoesToEndOfBuffer) != 0;
    m_bCtrlPGoesToPreviousCommand = (m_iFlags1 & FLAGS1_CtrlPGoesToPreviousCommand) != 0;
    m_bCtrlNGoesToNextCommand = (m_iFlags1 & FLAGS1_CtrlNGoesToNextCommand) != 0;
    m_bHyperlinkAddsToCommandHistory = (m_iFlags1 & FLAGS1_HyperlinkAddsToCommandHistory) != 0;
    m_bEchoHyperlinkInOutputWindow = (m_iFlags1 & FLAGS1_EchoHyperlinkInOutputWindow) != 0;
    m_bAutoWrapWindowWidth = (m_iFlags1 & FLAGS1_AutoWrapWindowWidth) != 0;
    m_bNAWS = (m_iFlags1 & FLAGS1_NAWS) != 0;
    m_bPueblo = (m_iFlags1 & FLAGS1_Pueblo) != 0;
    m_bNoEchoOff = (m_iFlags1 & FLAGS1_NoEchoOff) != 0;
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
    if (m_bArrowRecallsPartial)
        m_iFlags1 |= FLAGS1_ArrowRecallsPartial;
    if (m_bCtrlZGoesToEndOfBuffer)
        m_iFlags1 |= FLAGS1_CtrlZGoesToEndOfBuffer;
    if (m_bCtrlPGoesToPreviousCommand)
        m_iFlags1 |= FLAGS1_CtrlPGoesToPreviousCommand;
    if (m_bCtrlNGoesToNextCommand)
        m_iFlags1 |= FLAGS1_CtrlNGoesToNextCommand;
    if (m_bHyperlinkAddsToCommandHistory)
        m_iFlags1 |= FLAGS1_HyperlinkAddsToCommandHistory;
    if (m_bEchoHyperlinkInOutputWindow)
        m_iFlags1 |= FLAGS1_EchoHyperlinkInOutputWindow;
    if (m_bAutoWrapWindowWidth)
        m_iFlags1 |= FLAGS1_AutoWrapWindowWidth;
    if (m_bNAWS)
        m_iFlags1 |= FLAGS1_NAWS;
    if (m_bPueblo)
        m_iFlags1 |= FLAGS1_Pueblo;
    if (m_bNoEchoOff)
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
    if (!m_pSocket) {
        qCDebug(lcWorld) << "ReceiveMsg: No socket";
        return;
    }

    // Read all available data
    char buffer[8192];
    qint64 nRead = m_pSocket->receive(buffer, sizeof(buffer));

    if (nRead <= 0) {
        if (nRead < 0) {
            qCDebug(lcWorld) << "ReceiveMsg: Socket read error";
        }
        return;
    }

    // Update statistics
    m_nBytesIn += nRead;
    m_iInputPacketCount++;

    // Notify plugins of raw packet received (for protocol debugging)
    // Note: Original MUSHclient uses SendToAllPluginCallbacksRtn which can modify the data
    // For now, we just notify without modification capability
    QString packetData = QString::fromLatin1(buffer, nRead);
    SendToAllPluginCallbacks(ON_PLUGIN_PACKET_RECEIVED, packetData, false);

    // Check if we need to decompress the data
    if (m_bCompress) {
        // MCCP decompression with staging buffer
        // Check if we have space in the input buffer for the new data
        if ((COMPRESS_BUFFER_LENGTH - m_zCompress.avail_in) < (unsigned int)nRead) {
            qCDebug(lcWorld) << "Insufficient space in compression input buffer";
            OnConnectionDisconnect();
            return;
        }

        // Shuffle any remaining unconsumed data to the start of the buffer
        if (m_zCompress.avail_in > 0) {
            memmove(m_CompressInput, m_zCompress.next_in, m_zCompress.avail_in);
        }
        m_zCompress.next_in = m_CompressInput;

        // Append new compressed data to the staging buffer
        memcpy(&m_zCompress.next_in[m_zCompress.avail_in], buffer, nRead);
        m_zCompress.avail_in += nRead;
        m_nTotalCompressed += nRead;

        // Keep decompressing until all input is consumed
        do {
            // Set up output buffer
            m_zCompress.next_out = m_CompressOutput;
            m_zCompress.avail_out = m_nCompressionOutputBufferSize;

            // Decompress
            QElapsedTimer timer;
            timer.start();
            int izError = inflate(&m_zCompress, Z_SYNC_FLUSH);
            m_iCompressionTimeTaken += timer.elapsed();

            // Handle Z_BUF_ERROR by growing the output buffer
            if (izError == Z_BUF_ERROR) {
                // Output buffer is too small - double it and try again
                qint32 newSize = m_nCompressionOutputBufferSize * 2;
                Bytef* newBuffer = (Bytef*)realloc(m_CompressOutput, newSize);
                if (!newBuffer) {
                    qCDebug(lcWorld) << "Failed to grow compression output buffer";
                    m_bCompress = false;
                    OnConnectionDisconnect();
                    return;
                }
                m_CompressOutput = newBuffer;
                m_nCompressionOutputBufferSize = newSize;
                qCDebug(lcWorld) << "Grew compression output buffer to" << newSize << "bytes";

                // Reset output buffer pointers and try again
                m_zCompress.next_out = m_CompressOutput;
                m_zCompress.avail_out = m_nCompressionOutputBufferSize;

                timer.start();
                izError = inflate(&m_zCompress, Z_SYNC_FLUSH);
                m_iCompressionTimeTaken += timer.elapsed();
            }

            if (izError != Z_OK && izError != Z_STREAM_END) {
                qCDebug(lcWorld) << "MCCP decompression error:" << izError;
                if (m_zCompress.msg) {
                    qCDebug(lcWorld) << "  zlib message:" << m_zCompress.msg;
                }
                // Stop compression on error
                m_bCompress = false;
                OnConnectionDisconnect();
                return;
            }

            // Calculate how many bytes were decompressed
            qint64 nDecompressed = m_nCompressionOutputBufferSize - m_zCompress.avail_out;
            m_nTotalUncompressed += nDecompressed;

            // Process each decompressed byte through the telnet state machine
            for (qint64 i = 0; i < nDecompressed; i++) {
                ProcessIncomingByte(m_CompressOutput[i]);
            }

            // If we hit stream end, compression is done
            if (izError == Z_STREAM_END) {
                qCDebug(lcWorld) << "MCCP stream ended";
                m_bCompress = false;
                break;
            }

        } while (m_zCompress.avail_in > 0);
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

/**
 * OnConnect - Called when connection succeeds or fails
 *
 * This is called from WorldSocket::onConnected() (errorCode=0)
 * or WorldSocket::onError() (errorCode=socket error).
 *
 * Will be fully implemented in .
 *
 * Port from: doc.cpp (search for "OnConnect")
 */
void WorldDocument::OnConnect(int errorCode)
{
    if (errorCode == 0) {
        // Connection succeeded!
        qCDebug(lcWorld) << "WorldDocument::OnConnect() - connected successfully to" << m_server
                         << ":" << m_port;
        m_iConnectPhase = eConnectConnectedToMud;
        m_tConnectTime = QDateTime::currentDateTime();

        // Initialize parser state
        m_phase = NONE;
        m_bCompress = false;
        m_code = 0;
        memset(m_UTF8Sequence, 0, sizeof(m_UTF8Sequence));
        m_iUTF8BytesLeft = 0;
        memset(m_bClient_sent_IAC_DO, 0, sizeof(m_bClient_sent_IAC_DO));
        memset(m_bClient_sent_IAC_DONT, 0, sizeof(m_bClient_sent_IAC_DONT));
        memset(m_bClient_sent_IAC_WILL, 0, sizeof(m_bClient_sent_IAC_WILL));
        memset(m_bClient_sent_IAC_WONT, 0, sizeof(m_bClient_sent_IAC_WONT));
        memset(m_bClient_got_IAC_DO, 0, sizeof(m_bClient_got_IAC_DO));
        memset(m_bClient_got_IAC_DONT, 0, sizeof(m_bClient_got_IAC_DONT));
        memset(m_bClient_got_IAC_WILL, 0, sizeof(m_bClient_got_IAC_WILL));
        memset(m_bClient_got_IAC_WONT, 0, sizeof(m_bClient_got_IAC_WONT));

        // Create initial line if needed
        if (!m_currentLine) {
            m_currentLine = new Line(1,             // Line number
                                     m_nWrapColumn, // Wrap column
                                     m_iFlags,      // Style flags
                                     m_iForeColour, // Foreground
                                     m_iBackColour, // Background
                                     m_bUTF_8       // UTF-8 mode
            );

            // Create initial style
            auto initialStyle = std::make_unique<Style>();
            initialStyle->iLength = 0;
            initialStyle->iFlags = m_iFlags;
            initialStyle->iForeColour = m_iForeColour;
            initialStyle->iBackColour = m_iBackColour;
            initialStyle->pAction = nullptr;
            m_currentLine->styleList.push_back(std::move(initialStyle));
        }

        // Call Lua callback for world connect
        onWorldConnect();

        // Notify plugins of connection
        SendToAllPluginCallbacks(ON_PLUGIN_CONNECT);

        // Start remote access server if configured
        qCDebug(lcWorld) << "Remote access check: enabled=" << m_bEnableRemoteAccess
                         << "port=" << m_iRemotePort
                         << "password_set=" << !m_strRemotePassword.isEmpty();
        if (m_bEnableRemoteAccess && m_iRemotePort > 0 && !m_strRemotePassword.isEmpty()) {
            if (!m_pRemoteServer) {
                m_pRemoteServer = std::make_unique<RemoteAccessServer>(this);
            }
            m_pRemoteServer->setPassword(m_strRemotePassword);
            m_pRemoteServer->setScrollbackLines(m_iRemoteScrollbackLines);
            m_pRemoteServer->setMaxClients(m_iRemoteMaxClients);
            if (m_pRemoteServer->start(m_iRemotePort)) {
                qCDebug(lcWorld) << "Remote access server started on port" << m_iRemotePort;
            } else {
                qCWarning(lcWorld)
                    << "Remote access server FAILED to start on port" << m_iRemotePort;
            }
        } else {
            qCDebug(lcWorld) << "Remote access server not starting (conditions not met)";
        }

        // TODO: Implement additional connect handling:
        // - Send connect text (m_connect_text)
        // - Execute connect script (m_strWorldConnect)
        // - Send plugin callbacks
        // - Start timers

        emit connectionStateChanged(true);
    } else {
        // Connection failed
        qCDebug(lcWorld) << "WorldDocument::OnConnect() - connection failed with error:"
                         << errorCode;
        m_iConnectPhase = eConnectNotConnected;

        // TODO: Implement error handling:
        // - Display error message
        // - Execute error script
        // - Attempt reconnect if configured

        emit connectionStateChanged(false);
    }
}

/**
 * connectToMud - Initiate connection to the MUD server
 *
 * Called by user action (Game → Connect) or auto-connect on world open.
 */
void WorldDocument::connectToMud()
{
    if (!m_pSocket) {
        qCDebug(lcWorld) << "connectToMud: No socket available!";
        return;
    }

    if (m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "connectToMud: Already connected";
        return;
    }

    if (m_server.isEmpty()) {
        qCDebug(lcWorld) << "connectToMud: No server specified";
        return;
    }

    qCDebug(lcWorld) << "connectToMud: Connecting to" << m_server << ":" << m_port;
    m_iConnectPhase = eConnectConnectingToMud;
    m_pSocket->connectToHost(m_server, m_port);
}

/**
 * disconnectFromMud - Disconnect from the MUD server
 *
 * Called by user action (Game → Disconnect) or on world close.
 */
void WorldDocument::disconnectFromMud()
{
    if (!m_pSocket) {
        return;
    }

    if (!m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "disconnectFromMud: Not connected";
        return;
    }

    qCDebug(lcWorld) << "disconnectFromMud: Disconnecting from" << m_server;
    m_iConnectPhase = eConnectDisconnecting;
    m_pSocket->disconnectFromHost();
}

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
    if (!m_pSocket || !m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "sendToMud: Not connected";
        return;
    }

    // Convert to UTF-8 (or Latin-1 if not UTF-8 mode)
    QByteArray data = m_bUTF_8 ? text.toUtf8() : text.toLatin1();

    // Add newline - most MUDs expect \n, some want \r\n
    // For now, send just \n (Unix style)
    data.append('\n');

    // Send through socket
    SendPacket((const unsigned char*)data.constData(), data.length());

    // Increment statistics
    m_nTotalLinesSent++;

    qCDebug(lcWorld) << "sendToMud:" << text;
}

/**
 * connectedTime - Get the duration of the current connection
 *
 * @return Seconds connected, or -1 if not connected
 */
qint64 WorldDocument::connectedTime() const
{
    if (!m_tConnectTime.isValid()) {
        return -1;
    }
    return m_tConnectTime.secsTo(QDateTime::currentDateTime());
}

/**
 * resetConnectedTime - Reset the connection timer to now
 *
 * Called when user double-clicks the time indicator in the status bar.
 */
void WorldDocument::resetConnectedTime()
{
    m_tConnectTime = QDateTime::currentDateTime();
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

    for (const QString& line : lines) {
        // Queue if speedwalk delay active OR already items in queue
        if (m_iSpeedWalkDelay > 0 && (bQueue || !m_CommandQueue.isEmpty())) {
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
            m_CommandQueue.append(prefix + line);
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
    bool bLog = (m_log_input != 0);

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
 * - Input logging to log file (if bLog=true and m_log_input)
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
    if (m_bEnableSpamPrevention && m_iSpamLineCount > 2 && !m_strSpamMessage.isEmpty() &&
        m_iLastCommandCount > m_iSpamLineCount) {
        // Reset counter and send spam message
        m_iLastCommandCount = 0;
        DoSendMsg(m_strSpamMessage, m_display_my_input, m_log_input);

        // Reset tracking for current command
        m_strLastCommandSent = str;
        m_iLastCommandCount = 1;
    }

    // ========== Input Echoing ==========
    // Echo sent text to output window if requested
    // Don't echo if m_bNoEcho is set (MUD requested echo suppression for password entry)
    if (bEcho && m_display_my_input && !m_bNoEcho) {
        // Display as colored note with input colors
        QRgb inputFore = m_normalcolour[m_echo_colour % 8]; // Ensure valid index 0-7
        QRgb inputBack = m_normalcolour[0];                 // Black background (index 0)

        // Use colourNote to display in output window
        colourNote(inputFore, inputBack, str);
    }

    // ========== Input Logging ==========
    // Log sent text to log file if requested
    if (bLog && m_log_input && IsLogOpen()) {
        logCommand(str);
    }

    // ========== IAC Doubling ==========
    // Double IAC (0xFF) to IAC IAC for telnet protocol (unless disabled)
    if (!m_bDoNotTranslateIACtoIACIAC) {
        // Replace all 0xFF with 0xFF 0xFF
        QByteArray data = m_bUTF_8 ? str.toUtf8() : str.toLatin1();
        QByteArray doubled;
        for (unsigned char c : data) {
            doubled.append(c);
            if (c == IAC) {
                doubled.append(static_cast<char>(IAC)); // Double it
            }
        }

        // Send the doubled data + newline
        doubled.append('\n');
        SendPacket((const unsigned char*)doubled.constData(), doubled.length());
    } else {
        // Just send normally (use existing sendToMud)
        sendToMud(str);
    }

    // Update statistics
    m_nTotalLinesSent++;
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
    if (!m_strLogLinePreambleInput.isEmpty()) {
        QString preamble =
            FormatTime(QDateTime::currentDateTime(), m_strLogLinePreambleInput, m_bLogHTML);
        WriteToLog(preamble);
    }

    // Write the command text
    if (m_bLogHTML) {
        WriteToLog(FixHTMLString(text));
    } else {
        WriteToLog(text);
    }

    // Write postamble (if any)
    if (!m_strLogLinePostambleInput.isEmpty()) {
        QString postamble =
            FormatTime(QDateTime::currentDateTime(), m_strLogLinePostambleInput, m_bLogHTML);
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

    bool bAutoSay = m_bEnableAutoSay;

    // Check for override prefix to disable auto-say for this command
    if (bAutoSay && !m_strOverridePrefix.isEmpty() &&
        strFixedCommand.startsWith(m_strOverridePrefix)) {
        bAutoSay = false;
        strFixedCommand = strFixedCommand.mid(m_strOverridePrefix.length()); // Strip prefix
    }

    // Exclude auto-say string itself (prevent "say say hello")
    if (bAutoSay && !m_strAutoSayString.isEmpty() &&
        strFixedCommand.startsWith(m_strAutoSayString)) {
        bAutoSay = false;
    }

    // If auto-say is still enabled, prepend the auto-say string
    if (bAutoSay && !m_strAutoSayString.isEmpty()) {
        strFixedCommand = m_strAutoSayString + strFixedCommand;
    }

    // ========== Script Prefix ==========
    // Check for script prefix to execute Lua code directly
    // Based on methods_commands.cpp

    if (m_bEnableScripts && !m_strScriptPrefix.isEmpty() &&
        strFixedCommand.startsWith(m_strScriptPrefix)) {
        // Remove prefix and get script command
        QString strCommand = strFixedCommand.mid(m_strScriptPrefix.length());

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

    if (m_enable_speed_walk && !m_speed_walk_prefix.isEmpty() &&
        strFixedCommand.startsWith(m_speed_walk_prefix)) {
        // Remove prefix and evaluate speedwalk
        QString speedwalkInput = strFixedCommand.mid(m_speed_walk_prefix.length());
        QString expandedSpeedwalk = DoEvaluateSpeedwalk(speedwalkInput);

        if (!expandedSpeedwalk.isEmpty()) {
            // Check for error (starts with '*')
            if (expandedSpeedwalk[0] == '*') {
                // Show error message to user (skip the '*')
                qCDebug(lcWorld) << "Speedwalk error:" << expandedSpeedwalk.mid(1);
                note(expandedSpeedwalk.mid(1)); // Display error in output window
                return;
            }

            // Send expanded speedwalk commands via SendMsg with queue=true
            SendMsg(expandedSpeedwalk, m_display_my_input, true, m_log_input);
        }
        return; // Don't process further (speedwalk handled)
    }

    // ========== Command Stacking ==========
    // Based on methods_commands.cpp

    if (m_enable_command_stack && !m_strCommandStackCharacter.isEmpty()) {
        // NEW in version 3.74 - command stack character at start of line disables command stacking
        if (!strFixedCommand.isEmpty() && strFixedCommand[0] == m_strCommandStackCharacter[0]) {
            // Remove leading delimiter - this line won't be stacked
            strFixedCommand = strFixedCommand.mid(1);
        } else {
            // Still want command stacking
            QString strTwoStacks =
                QString(m_strCommandStackCharacter[0]) + m_strCommandStackCharacter[0];

            // Convert two command stacks in a row to 0x01 (e.g., ;; → \x01)
            strFixedCommand.replace(strTwoStacks, "\x01");

            // Convert any remaining command stacks to \r\n
            strFixedCommand.replace(m_strCommandStackCharacter[0], '\n');

            // Replace any 0x01 with one command stack character (restore escaped delimiters)
            strFixedCommand.replace('\x01', m_strCommandStackCharacter[0]);
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
            bool bLog = m_log_input;

            SendMsg(processedCommand, bEcho, bQueue, bLog);

            // Add to command history
            addToCommandHistory(processedCommand);
        }
        // Note: If alias handled, the alias execution already added to history (if
        // !bOmitFromCommandHistory)
    }
}

// ========== Command Queue ==========

/**
 * GetCommandQueue - Get list of queued commands
 *
 * Returns a copy of the current command queue.
 *
 * @return QStringList of queued commands
 */
QStringList WorldDocument::GetCommandQueue() const
{
    return m_CommandQueue;
}

/**
 * Queue - Add command to command queue
 *
 * Queues a command to be sent to the MUD. The command will be sent
 * according to the speedwalk delay setting.
 *
 * Based on methods_speedwalks.cpp
 *
 * @param message Command to queue
 * @param echo Whether to echo the command when sent
 * @return Error code (0=success, 30002=not connected, 30063=plugin processing)
 */
qint32 WorldDocument::Queue(const QString& message, bool echo)
{
    if (m_iConnectPhase != eConnectConnectedToMud) {
        return 30002; // eWorldClosed
    }

    if (m_bPluginProcessingSent) {
        return 30063; // eItemInUse
    }

    // Add to queue via SendMsg with queue=true
    SendMsg(message, echo, true, false);
    return 0; // eOK
}

/**
 * DiscardQueue - Clear the command queue
 *
 * Clears all queued commands and returns the count of discarded items.
 *
 * Based on methods_speedwalks.cpp
 *
 * @return Number of commands that were discarded
 */
qint32 WorldDocument::DiscardQueue()
{
    qint32 count = m_CommandQueue.size();
    m_CommandQueue.clear();
    // TODO: Update status line to show queue is empty
    return count;
}

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
    // TODO: Notify plugins of command change
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
    // TODO: Notify plugins of command change
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
    if (m_strCustomColourName[whichColour - 1] != name) {
        m_bModified = true; // Mark document as modified
    }

    // Set the name (convert to 0-based index)
    m_strCustomColourName[whichColour - 1] = name;

    return 0; // eOK
}

// ========== Speed Walking ==========

/**
 * DoEvaluateSpeedwalk - Parse speedwalk notation and expand to commands
 *
 * Speed Walking
 *
 * Parses speedwalk notation like "3n2w" and expands to individual direction commands.
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * Format:
 * - [count]direction: e.g., "3n" = north;north;north (count 1-99)
 * - Directions: N, S, E, W, U, D (case insensitive)
 * - Actions: O=open, C=close, L=lock, K=unlock (must be followed by direction)
 * - Filler: F = spam prevention filler (m_strSpeedWalkFiller)
 * - Custom: (text/reverse) = custom direction (only "text" is used)
 * - Comments: {comment} = ignored
 *
 * Examples:
 * - "3n2w" → "north\nnorth\nnorth\nwest\nwest\n"
 * - "On3e" → "open north\neast\neast\neast\n"
 * - "2(ne/sw)" → "ne\nne\n"
 *
 * @param speedWalkString - Speedwalk notation string
 * @return Expanded commands separated by \n, or error string starting with "*"
 */
QString WorldDocument::DoEvaluateSpeedwalk(const QString& speedWalkString)
{
    // Initialize default direction map
    static QMap<QChar, DirectionInfo> directionMap;
    if (directionMap.isEmpty()) {
        directionMap['n'] = DirectionInfo("north", "s");
        directionMap['s'] = DirectionInfo("south", "n");
        directionMap['e'] = DirectionInfo("east", "w");
        directionMap['w'] = DirectionInfo("west", "e");
        directionMap['u'] = DirectionInfo("up", "d");
        directionMap['d'] = DirectionInfo("down", "u");
    }

    QString result;
    QString str;
    int count;
    const QChar* p = speedWalkString.constData();
    const QChar* end = p + speedWalkString.length();

    while (p < end) {
        // Bypass spaces
        while (p < end && p->isSpace())
            p++;

        if (p >= end)
            break;

        // Bypass comments {comment}
        if (*p == '{') {
            while (p < end && *p != '}')
                p++;

            if (p >= end || *p != '}')
                return QString("*Comment code of '{' not terminated by a '}'");
            p++; // skip } symbol
            continue;
        }

        // Get counter, if any
        count = 0;
        while (p < end && p->isDigit()) {
            count = (count * 10) + (p->unicode() - '0');
            p++;
            if (count > 99)
                return QString("*Speed walk counter exceeds 99");
        }

        // No counter, assume do once
        if (count == 0)
            count = 1;

        // Bypass spaces after counter
        while (p < end && p->isSpace())
            p++;

        if (count > 1 && p >= end)
            return QString("*Speed walk counter not followed by an action");

        if (count > 1 && *p == '{')
            return QString("*Speed walk counter may not be followed by a comment");

        // Might have had trailing space
        if (p >= end)
            break;

        // Check for action codes (C, O, L, K)
        if (p->toUpper() == 'C' || p->toUpper() == 'O' || p->toUpper() == 'L' ||
            p->toUpper() == 'K') {
            if (count > 1)
                return QString(
                    "*Action code of C, O, L or K must not follow a speed walk count (1-99)");

            switch (p->toUpper().unicode()) {
                case 'C':
                    result += "close ";
                    break;
                case 'O':
                    result += "open ";
                    break;
                case 'L':
                    result += "lock ";
                    break;
                case 'K':
                    result += "unlock ";
                    break;
            }
            p++;

            // Bypass spaces after open/close/lock/unlock
            while (p < end && p->isSpace())
                p++;

            if (p >= end || p->toUpper() == 'F' || *p == '{')
                return QString("*Action code of C, O, L or K must be followed by a direction");
        }

        // Work out which direction we are going
        if (p >= end)
            break;

        QChar dir = p->toUpper();
        switch (dir.unicode()) {
            case 'N':
            case 'S':
            case 'E':
            case 'W':
            case 'U':
            case 'D':
                // Look up the direction to send
                str = directionMap[p->toLower()].m_sDirectionToSend;
                break;

            case 'F':
                str = m_strSpeedWalkFiller;
                break;

            case '(': // special string (e.g., (ne/sw))
            {
                str.clear();
                p++; // skip '('
                while (p < end && *p != ')')
                    str += *p++;

                if (p >= end || *p != ')')
                    return QString("*Action code of '(' not terminated by a ')'");

                int iSlash = str.indexOf("/");
                if (iSlash != -1)
                    str = str.left(iSlash); // only use up to the slash
            } break;

            default:
                return QString("*Invalid direction '%1' in speed walk, must be N, S, E, W, U, D, "
                               "F, or (something)")
                    .arg(dir);
        }

        p++; // bypass whatever that character was (or the trailing bracket)

        // Output required number of times
        for (int j = 0; j < count; j++)
            result += str + "\n";
    }

    return result;
}

/**
 * DoReverseSpeedwalk - Reverse a speedwalk string
 *
 * Takes a speedwalk like "3noe" and reverses it to "cw3s"
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * @param speedWalkString - Speedwalk notation string
 * @return Reversed speedwalk, or error string starting with "*"
 */
QString WorldDocument::DoReverseSpeedwalk(const QString& speedWalkString)
{
    // Initialize direction map with reverses
    static QMap<QChar, DirectionInfo> directionMap;
    if (directionMap.isEmpty()) {
        directionMap['n'] = DirectionInfo("north", "s");
        directionMap['s'] = DirectionInfo("south", "n");
        directionMap['e'] = DirectionInfo("east", "w");
        directionMap['w'] = DirectionInfo("west", "e");
        directionMap['u'] = DirectionInfo("up", "d");
        directionMap['d'] = DirectionInfo("down", "u");
        directionMap['f'] = DirectionInfo("f", "f"); // filler stays same
    }

    QString result;
    QString str;
    QString strAction;
    int count;
    const QChar* p = speedWalkString.constData();
    const QChar* end = p + speedWalkString.length();

    while (p < end) {
        // Preserve spaces (prepend to result since we're reversing)
        while (p < end && p->isSpace()) {
            if (*p == '\r') {
                // discard carriage returns
            } else if (*p == '\n') {
                result = "\n" + result;
            } else {
                result = *p + result;
            }
            p++;
        }

        if (p >= end)
            break;

        // Preserve comments {comment}
        if (*p == '{') {
            str.clear();
            while (p < end && *p != '}')
                str += *p++;

            if (p >= end || *p != '}')
                return QString("*Comment code of '{' not terminated by a '}'");

            p++; // skip } symbol
            str += "}";
            result = str + result;
            continue;
        }

        // Get counter, if any
        count = 0;
        while (p < end && p->isDigit()) {
            count = (count * 10) + (p->unicode() - '0');
            p++;
            if (count > 99)
                return QString("*Speed walk counter exceeds 99");
        }

        if (count == 0)
            count = 1;

        // Bypass spaces after counter
        while (p < end && p->isSpace())
            p++;

        if (count > 1 && p >= end)
            return QString("*Speed walk counter not followed by an action");

        if (count > 1 && *p == '{')
            return QString("*Speed walk counter may not be followed by a comment");

        if (p >= end)
            break;

        // Check for action codes (C, O, L, K)
        strAction.clear();
        if (p->toUpper() == 'C' || p->toUpper() == 'O' || p->toUpper() == 'L' ||
            p->toUpper() == 'K') {
            if (count > 1)
                return QString(
                    "*Action code of C, O, L or K must not follow a speed walk count (1-99)");

            strAction = *p;
            p++;

            while (p < end && p->isSpace())
                p++;

            if (p >= end || p->toUpper() == 'F' || *p == '{')
                return QString("*Action code of C, O, L or K must be followed by a direction");
        }

        if (p >= end)
            break;

        // Work out which direction and get reverse
        QChar dir = p->toLower();
        switch (p->toUpper().unicode()) {
            case 'N':
            case 'S':
            case 'E':
            case 'W':
            case 'U':
            case 'D':
            case 'F':
                str = directionMap[dir].m_sReverseDirection;
                break;

            case '(': // special string (e.g., (ne/sw))
            {
                // Static map for diagonal direction reverses (ne<->sw, nw<->se)
                static QMap<QString, QString> diagonalReverseMap;
                if (diagonalReverseMap.isEmpty()) {
                    diagonalReverseMap["ne"] = "sw";
                    diagonalReverseMap["sw"] = "ne";
                    diagonalReverseMap["nw"] = "se";
                    diagonalReverseMap["se"] = "nw";
                    // Also add the full direction names if needed
                    diagonalReverseMap["northeast"] = "southwest";
                    diagonalReverseMap["southwest"] = "northeast";
                    diagonalReverseMap["northwest"] = "southeast";
                    diagonalReverseMap["southeast"] = "northwest";
                }

                str.clear();
                p++; // skip '('
                while (p < end && *p != ')')
                    str += p->toLower(), p++;

                if (p >= end || *p != ')')
                    return QString("*Action code of '(' not terminated by a ')'");

                int iSlash = str.indexOf("/");
                if (iSlash == -1) {
                    // No slash - try to look up reverse (for simple directions like "ne")
                    if (diagonalReverseMap.contains(str))
                        str = diagonalReverseMap[str];
                    // Otherwise keep as-is
                } else {
                    // Swap left and right parts
                    QString left = str.left(iSlash);
                    QString right = str.mid(iSlash + 1);
                    str = right + "/" + left;
                }
                str = "(" + str + ")";
            } break;

            default:
                return QString("*Invalid direction '%1' in speed walk, must be N, S, E, W, U, D, "
                               "F, or (something)")
                    .arg(*p);
        }

        p++; // bypass direction char or closing bracket

        // Output it (prepend since reversing)
        if (count > 1)
            result = QString("%1%2%3").arg(count).arg(strAction).arg(str) + result;
        else
            result = strAction + str + result;
    }

    return result;
}

/**
 * RemoveBacktracks - Remove redundant back-and-forth movements from speedwalk
 *
 * Takes a speedwalk and removes movements that cancel each other out.
 * E.g., "nsen" becomes "2n" (the s-e cancel out... wait no, ns would cancel)
 * Actually: "nsew" - n and s cancel, e and w cancel = empty
 *
 * Based on methods_speedwalks.cpp in original MUSHclient.
 *
 * @param speedWalkString - Speedwalk notation string
 * @return Optimized speedwalk with backtracks removed
 */
QString WorldDocument::RemoveBacktracks(const QString& speedWalkString)
{
    // Initialize direction map with reverses
    static QMap<QString, QString> reverseMap;
    if (reverseMap.isEmpty()) {
        reverseMap["n"] = "s";
        reverseMap["s"] = "n";
        reverseMap["e"] = "w";
        reverseMap["w"] = "e";
        reverseMap["u"] = "d";
        reverseMap["d"] = "u";
        reverseMap["north"] = "south";
        reverseMap["south"] = "north";
        reverseMap["east"] = "west";
        reverseMap["west"] = "east";
        reverseMap["up"] = "down";
        reverseMap["down"] = "up";
    }

    // First expand the speedwalk
    QString expanded = DoEvaluateSpeedwalk(speedWalkString);

    // If error or empty, return as-is
    if (expanded.isEmpty() || expanded.startsWith('*'))
        return expanded;

    // Split into individual directions
    QStringList directions = expanded.split('\n', Qt::SkipEmptyParts);

    // Use a stack approach - push directions, pop when reverse found
    QList<QString> stack;

    for (const QString& dir : directions) {
        QString trimmed = dir.trimmed().toLower();
        if (trimmed.isEmpty())
            continue;

        // Convert full direction to single letter for comparison
        QString normalized = trimmed;
        if (trimmed == "north")
            normalized = "n";
        else if (trimmed == "south")
            normalized = "s";
        else if (trimmed == "east")
            normalized = "e";
        else if (trimmed == "west")
            normalized = "w";
        else if (trimmed == "up")
            normalized = "u";
        else if (trimmed == "down")
            normalized = "d";

        if (stack.isEmpty()) {
            stack.append(normalized);
        } else {
            QString top = stack.last();
            // Check if this direction is the reverse of the top
            if (reverseMap.contains(top) && reverseMap[top] == normalized) {
                stack.removeLast(); // Cancel out
            } else {
                stack.append(normalized);
            }
        }
    }

    // Empty result
    if (stack.isEmpty())
        return QString();

    // Compress consecutive identical directions
    QString result;
    QString prev;
    int count = 0;

    for (const QString& dir : stack) {
        if (dir.isEmpty())
            continue;

        QString formatted = dir;
        // Multi-char directions need brackets
        if (dir.length() > 1)
            formatted = "(" + dir + ")";

        if (formatted == prev && count < 99) {
            count++;
        } else {
            // Output previous
            if (!prev.isEmpty()) {
                if (count > 1)
                    result += QString("%1%2 ").arg(count).arg(prev);
                else
                    result += prev + " ";
            }
            prev = formatted;
            count = 1;
        }
    }

    // Output final
    if (!prev.isEmpty()) {
        if (count > 1)
            result += QString("%1%2 ").arg(count).arg(prev);
        else
            result += prev + " ";
    }

    return result.trimmed();
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
    Line* pLine = m_currentLine;

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
 * line buffer and ensures the buffer doesn't exceed m_maxlines by removing
 * the oldest lines.
 *
 * @param line The completed Line object to add (ownership transferred to buffer)
 */
void WorldDocument::AddLineToBuffer(Line* line)
{
    if (!line) {
        return; // Null check
    }

    // Add line to buffer
    m_lineList.append(line);

    // Trim buffer if too large
    while (m_lineList.count() > m_maxlines) {
        Line* oldLine = m_lineList.takeFirst();
        delete oldLine;
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
    memcpy(m_currentLine->text() + currentLen, sText, iLength);

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
            if (m_nWrapColumn == 0 || spacePos < static_cast<int>(m_nWrapColumn)) {
                m_lastSpace = spacePos;
            }
        }
    }

    // Check if line needs to wrap (exceeds wrap column)
    // Only wrap if m_nWrapColumn > 0 (0 means no wrapping)
    if (m_nWrapColumn > 0 && m_currentLine->len() >= static_cast<qint32>(m_nWrapColumn)) {
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
 * This is called when the current line length exceeds m_nWrapColumn.
 * Behavior depends on m_wrap setting:
 * - m_wrap = 0: Hard break at column boundary
 * - m_wrap = 1: Word-wrap at last space (if found within reasonable distance)
 *
 * Word-wrap logic:
 * 1. If m_wrap is enabled and we have a valid last space position
 * 2. And the text after the space isn't too long (< m_nWrapColumn)
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
    if (m_wrap != 0 && m_lastSpace >= 0) {
        // Check that the text after the space isn't too long
        // (if remaining text is >= wrap column, we'd have to break again immediately)
        qint32 remainingLen = lineLen - m_lastSpace;
        if (remainingLen < static_cast<qint32>(m_nWrapColumn)) {
            breakPoint = m_lastSpace;
        }
    }

    if (breakPoint >= 0) {
        // Word-wrap: break at the space
        // Original MUSHclient behavior (doc.cpp:1557-1562):
        // When m_indent_paras is false (normal case), KEEP the space at end of this line
        // This ensures soft-wrapped lines display correctly when joined

        // Save the text AFTER the space to carry over to the next line
        int carryOverStart = breakPoint + 1;  // Text starts after the space
        int carryOverLen = lineLen - carryOverStart;

        QByteArray carryOver;
        if (carryOverLen > 0) {
            carryOver = QByteArray(m_currentLine->text() + carryOverStart, carryOverLen);
        }

        // Truncate current line AFTER the space (include the space in this line)
        // This matches original MUSHclient when m_indent_paras is false
        int truncateAt = breakPoint + 1;  // Keep up to and including the space
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
            quint16 savedWrapColumn = m_nWrapColumn;
            m_nWrapColumn = 0;

            AddToLine(carryOver.constData(), carryOverLen);

            m_nWrapColumn = savedWrapColumn;

            // Re-check for wrap (in case carried text is still too long)
            if (m_nWrapColumn > 0 && m_currentLine->len() >= static_cast<qint32>(m_nWrapColumn)) {
                handleLineWrap();
            }
        }
    } else {
        // No space found before wrap column
        // Check if word-wrap is enabled and line truly has NO spaces at all
        bool hasAnySpace = false;
        if (m_wrap != 0) {
            const char* lineText = m_currentLine->text();
            for (qint32 i = 0; i < lineLen; i++) {
                if (lineText[i] == ' ') {
                    hasAnySpace = true;
                    break;
                }
            }
        }

        // If word-wrap enabled and NO spaces at all, let line extend (preserves ASCII art)
        // Otherwise, hard break at the wrap column
        if (m_wrap != 0 && !hasAnySpace) {
            // ASCII art with no spaces - don't break, let it extend
            return;
        }

        // Hard break at the wrap column
        int splitPoint = static_cast<int>(m_nWrapColumn);
        int carryOverLen = lineLen - splitPoint;

        QByteArray carryOver;
        if (carryOverLen > 0) {
            carryOver = QByteArray(m_currentLine->text() + splitPoint, carryOverLen);
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
            quint16 savedWrapColumn = m_nWrapColumn;
            m_nWrapColumn = 0;

            AddToLine(carryOver.constData(), carryOverLen);

            m_nWrapColumn = savedWrapColumn;

            // Re-check for wrap (in case carried text is still too long)
            if (m_nWrapColumn > 0 && m_currentLine->len() >= static_cast<qint32>(m_nWrapColumn)) {
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

/**
 * onWorldConnect - Called when connection to MUD succeeds
 *
 * Lua Function Callbacks
 * Based on CMUSHclientDoc::OnWorldConnect() from doc.cpp
 *
 * Calls the Lua function "OnWorldConnect" if it exists.
 * Scripts can use this to perform initialization when connecting.
 */
void WorldDocument::onWorldConnect()
{
    // Check if we have a script engine
    if (!m_ScriptEngine) {
        return;
    }

    // Get DISPID if we haven't already
    if (m_dispidWorldConnect == 0) {
        m_dispidWorldConnect = m_ScriptEngine->getLuaDispid("OnWorldConnect");
    }

    // Skip if function doesn't exist
    if (m_dispidWorldConnect == DISPID_UNKNOWN) {
        return;
    }

    // Prepare empty parameter lists (this callback takes no parameters)
    QList<double> nparams;
    QList<QString> sparams;
    long nInvocationCount = 0;
    bool result = false;

    // Call the Lua function
    bool error =
        m_ScriptEngine->executeLua(m_dispidWorldConnect, "OnWorldConnect", eWorldAction, "world",
                                   "world connect", nparams, sparams, nInvocationCount, &result);

    if (error) {
        qCDebug(lcWorld) << "Error calling OnWorldConnect callback";
    } else {
        qCDebug(lcWorld) << "OnWorldConnect callback executed successfully";
    }
}

/**
 * onWorldDisconnect - Called when connection to MUD closes
 *
 * Lua Function Callbacks
 * Based on CMUSHclientDoc::OnWorldDisconnect() from doc.cpp
 *
 * Calls the Lua function "OnWorldDisconnect" if it exists.
 * Scripts can use this to clean up when disconnecting.
 */
void WorldDocument::onWorldDisconnect()
{
    // Check if we have a script engine
    if (!m_ScriptEngine) {
        return;
    }

    // Get DISPID if we haven't already
    if (m_dispidWorldDisconnect == 0) {
        m_dispidWorldDisconnect = m_ScriptEngine->getLuaDispid("OnWorldDisconnect");
    }

    // Skip if function doesn't exist
    if (m_dispidWorldDisconnect == DISPID_UNKNOWN) {
        return;
    }

    // Prepare empty parameter lists (this callback takes no parameters)
    QList<double> nparams;
    QList<QString> sparams;
    long nInvocationCount = 0;
    bool result = false;

    // Call the Lua function
    bool error = m_ScriptEngine->executeLua(m_dispidWorldDisconnect, "OnWorldDisconnect",
                                            eWorldAction, "world", "world disconnect", nparams,
                                            sparams, nInvocationCount, &result);

    if (error) {
        qCDebug(lcWorld) << "Error calling OnWorldDisconnect callback";
    } else {
        qCDebug(lcWorld) << "OnWorldDisconnect callback executed successfully";
    }
}

// ========== Stub Methods (Future Implementation) ==========

void WorldDocument::OnConnectionDisconnect()
{
    qCDebug(lcWorld) << "OnConnectionDisconnect - disconnect detected";

    // Stop remote access server if running
    if (m_pRemoteServer && m_pRemoteServer->isRunning()) {
        qCDebug(lcWorld) << "Stopping remote access server";
        m_pRemoteServer->stop();
    }

    // Call Lua callback for world disconnect
    onWorldDisconnect();

    // Notify plugins of disconnection
    SendToAllPluginCallbacks(ON_PLUGIN_DISCONNECT);

    // Update connection state
    m_iConnectPhase = eConnectNotConnected;
    emit connectionStateChanged(false);

    // TODO: Additional cleanup:
    // - Stop timers
    // - Clean up telnet state
}

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

        // TODO: High-performance timer (will use QElapsedTimer in future)
        // Original: QueryPerformanceCounter(&(m_pCurrentLine->m_lineHighPerformanceTime))
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
            QString partialLine = QString::fromUtf8(m_currentLine->text(), m_currentLine->len());
            SendToAllPluginCallbacks(ON_PLUGIN_PARTIAL_LINE, partialLine, false);
        }

        // If this is a hard break (newline from MUD), evaluate triggers
        // Call trigger pattern matching engine
        if (bNewLine && !(iFlags & NOTE_OR_COMMAND)) {
            // Add line text to recent lines buffer for multi-line trigger matching
            // Based on ProcessPreviousLine.cpp in original MUSHclient
            QString lineText = QString::fromUtf8(m_currentLine->text(), m_currentLine->len());
            m_recentLines.push_back(lineText);
            m_newlines_received++;

            // Limit buffer size - remove oldest if too many
            if (static_cast<int>(m_recentLines.size()) > MAX_RECENT_LINES) {
                m_recentLines.pop_front();
            }

            evaluateTriggers(m_currentLine);
        }

        // Line-level logging integration
        // Log completed lines based on line type and logging flags
        if (bNewLine) {
            logCompletedLine(m_currentLine);
        }

        // Detect and linkify URLs in the completed line
        DetectAndLinkifyURLs(m_currentLine);

        // Notify plugins about the line being displayed (for accessibility/screen readers)
        // iType: 0 = MUD output, 1 = COMMENT (note), 2 = USER_INPUT (command)
        // iLog: whether line should be logged based on type
        if (bNewLine) {
            QString lineText = QString::fromUtf8(m_currentLine->text(), m_currentLine->len());
            unsigned char lineFlags = m_currentLine->flags;
            if (lineFlags & COMMENT) {
                Screendraw(COMMENT, m_bLogNotes, lineText);
            } else if (lineFlags & USER_INPUT) {
                Screendraw(USER_INPUT, m_log_input, lineText);
            } else {
                // Regular MUD output (type 0)
                // Log flag: m_bLogOutput AND not omitted
                Screendraw(0, m_bLogOutput && !m_bOmitCurrentLineFromLog, lineText);
            }
        }

        // Add completed line to buffer (this also handles buffer size limiting)
        AddLineToBuffer(m_currentLine);
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
        // User input uses echo color if configured
        // TODO: Check m_echo_colour setting
        // For now, use default style
    } else if (iFlags & COMMENT) {
        // Comments (notes) use note color if configured
        // TODO: Check m_iNoteTextColour setting
        // For now, use default style
    }

    // Create new Line object
    m_currentLine = new Line(m_total_lines, // line number
                             m_nWrapColumn, // wrap column
                             iFlags,        // line flags
                             initialFore,   // foreground color
                             initialBack,   // background color
                             m_bUTF_8);     // UTF-8 mode

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
    QString lineText = QString::fromUtf8(line->text(), line->len());

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
    if (m_bNoEcho && !m_bAlwaysRecordCommandHistory) {
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
    // Use m_nHistoryLines instead of m_maxCommandHistory (sendvw.cpp)
    while (m_commandHistory.count() > m_nHistoryLines) {
        m_commandHistory.removeFirst();
    }

    // Save as last command for consecutive duplicate check
    m_last_command = command;

    // Reset position to end (ready for Up arrow to recall)
    m_historyPosition = m_commandHistory.count();

    // Set history status to bottom (sendvw.cpp)
    m_iHistoryStatus = eAtBottom;

    qCDebug(lcWorld) << "Command history: added" << command
                     << "- history size:" << m_commandHistory.count()
                     << "/ max:" << m_nHistoryLines;
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
    m_iHistoryStatus = eAtBottom;

    // Clear last command (for consecutive duplicate check)
    m_last_command.clear();

    qCDebug(lcWorld) << "Command history cleared";
}

// ========== Script Output Methods ==========

/**
 * note - Display a note in the output window
 *
 * Prep: Required for world.Note Lua API
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::Note)
 *
 * Displays a note (script-generated message) in the output window using
 * the default note colors. Notes are marked with the COMMENT flag to
 * distinguish them from MUD output.
 *
 * @param text The text to display
 */
void WorldDocument::note(const QString& text)
{
    // Use default note colors (or white on black if not set) - BGR format
    QRgb foreColor = m_bNotesInRGB ? m_iNoteColourFore : BGR(255, 255, 255);
    QRgb backColor = m_bNotesInRGB ? m_iNoteColourBack : BGR(0, 0, 0);

    colourNote(foreColor, backColor, text);
}

/**
 * colourNote - Display a colored note in the output window
 *
 * Prep: Required for world.ColourNote Lua API
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::ColourNote)
 *
 * Displays a note with specified foreground and background colors.
 * The text is added to the output buffer with the COMMENT flag,
 * then a newline is started to complete the note line.
 *
 * This method temporarily changes the current style to display the note,
 * then restores the previous style so MUD output continues normally.
 *
 * @param foreColor RGB foreground color (QRgb format)
 * @param backColor RGB background color (QRgb format)
 * @param text The text to display
 */
void WorldDocument::colourNote(QRgb foreColor, QRgb backColor, const QString& text)
{
    // Don't output notes if disabled
    if (m_bNotesNotWantedNow) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_iFlags;
    QRgb savedFore = m_iForeColour;
    QRgb savedBack = m_iBackColour;

    // Set note style: RGB colors with note style flags
    m_iFlags = COLOUR_RGB | m_iNoteStyle; // Use configured note style (bold, underline, etc.)
    m_iForeColour = foreColor;
    m_iBackColour = backColor;

    // Split text on newlines and output each line separately
    // This matches original MUSHclient behavior for multi-line notes
    QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray utf8 = lines[i].toUtf8();
        AddToLine(utf8.constData(), utf8.length());

        // Complete each line (hard break with COMMENT flag)
        StartNewLine(true, COMMENT);
    }

    // Restore previous style for MUD output
    m_iFlags = savedFlags;
    m_iForeColour = savedFore;
    m_iBackColour = savedBack;

    qCDebug(lcWorld) << "note:" << text;
}

/**
 * colourTell - Display colored text without newline
 *
 * Prep: Optional for world.ColourTell Lua API
 * Source: doc.cpp (original MUSHclient) (CMUSHclientDoc::ColourTell)
 *
 * Similar to colourNote() but does NOT add a newline after the text.
 * This allows building up a line with multiple colored segments.
 *
 * @param foreColor RGB foreground color (QRgb format)
 * @param backColor RGB background color (QRgb format)
 * @param text The text to display
 */
void WorldDocument::colourTell(QRgb foreColor, QRgb backColor, const QString& text)
{
    // Don't output notes if disabled
    if (m_bNotesNotWantedNow) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_iFlags;
    QRgb savedFore = m_iForeColour;
    QRgb savedBack = m_iBackColour;

    // Set note style: RGB colors with note style flags
    m_iFlags = COLOUR_RGB | m_iNoteStyle;
    m_iForeColour = foreColor;
    m_iBackColour = backColor;

    // Handle embedded newlines but don't add final newline
    // This matches original MUSHclient behavior
    QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray utf8 = lines[i].toUtf8();
        AddToLine(utf8.constData(), utf8.length());

        // Add newline for all but the last segment
        if (i < lines.size() - 1) {
            StartNewLine(true, COMMENT);
        }
    }

    // Restore previous style (no final newline - caller must call note("") or StartNewLine)
    m_iFlags = savedFlags;
    m_iForeColour = savedFore;
    m_iBackColour = savedBack;
}

/**
 * hyperlink - Display a clickable hyperlink in the output window
 *
 * Creates a clickable link that executes an action when clicked.
 *
 * @param action What to send/execute when clicked
 * @param text Display text (if empty, uses action)
 * @param hint Tooltip text
 * @param foreColor Foreground color
 * @param backColor Background color
 * @param isUrl True for URL hyperlinks (opens browser), false for send actions
 */
void WorldDocument::hyperlink(const QString& action, const QString& text, const QString& hint,
                              QRgb foreColor, QRgb backColor, bool isUrl)
{
    // Don't output if disabled
    if (m_bNotesNotWantedNow) {
        return;
    }

    // Don't do anything for empty action
    if (action.isEmpty()) {
        return;
    }

    // Save current style state
    quint16 savedFlags = m_iFlags;
    QRgb savedFore = m_iForeColour;
    QRgb savedBack = m_iBackColour;
    auto savedAction = m_currentAction;

    // Create Action for this hyperlink
    QString hintText = hint.isEmpty() ? action : hint;
    m_currentAction = std::make_shared<Action>(action, hintText, QString(), this);

    // Set hyperlink style: RGB colors, underline, and action flag
    quint16 actionFlag = isUrl ? ACTION_HYPERLINK : ACTION_SEND;
    m_iFlags = COLOUR_RGB | actionFlag;
    if (m_bUnderlineHyperlinks) {
        m_iFlags |= UNDERLINE;
    }
    m_iForeColour = foreColor;
    m_iBackColour = backColor;

    // Output the link text (use action if text is empty)
    QString displayText = text.isEmpty() ? action : text;
    QByteArray utf8 = displayText.toUtf8();
    AddToLine(utf8.constData(), utf8.length());

    // Restore previous style and action
    m_iFlags = savedFlags;
    m_iForeColour = savedFore;
    m_iBackColour = savedBack;
    m_currentAction = savedAction;
}

/**
 * simulate - Process text as if received from MUD
 *
 * Processes text through the telnet state machine and trigger system
 * as if it came from the MUD connection. Useful for testing scripts.
 *
 * @param text Text to simulate (may contain ANSI codes)
 */
void WorldDocument::simulate(const QString& text)
{
    // Set flag so triggers know we're simulating
    m_bDoingSimulate = true;

    // Process each byte through the telnet state machine
    QByteArray utf8 = text.toUtf8();
    for (int i = 0; i < utf8.length(); i++) {
        ProcessIncomingByte((unsigned char)utf8[i]);
    }

    m_bDoingSimulate = false;

    // Check if we have an incomplete line (prompt)
    if (m_currentLine && m_currentLine->len() > 0) {
        emit incompleteLine();
    }
}

/**
 * noteHr - Display horizontal rule
 *
 * Outputs a horizontal rule line in the output window.
 */
void WorldDocument::noteHr()
{
    // Wrap up previous line if necessary
    if (m_currentLine && m_currentLine->len() > 0) {
        StartNewLine(true, 0);
    }

    // Mark line as HR line
    if (m_currentLine) {
        m_currentLine->flags = HORIZ_RULE;
    }

    // Finish this line
    StartNewLine(true, 0);
}

// ========== Script Loading and Parsing ==========

/**
 * loadScriptFile - Load and execute script file
 *
 * Script Loading and Parsing
 * Based on CMUSHclientDoc::LoadScriptFile() and Execute_Script_File()
 *
 * Reads the script file from disk and executes it using parseLua().
 * The script file path is stored in m_strScriptFilename.
 *
 * This is typically called:
 * - When world is opened
 * - When user manually reloads script (Ctrl+R)
 * - When script file is modified (if file watching enabled)
 */
void WorldDocument::loadScriptFile()
{
    // Check if script filename is set
    if (m_strScriptFilename.isEmpty()) {
        qCDebug(lcWorld) << "loadScriptFile: No script filename set";
        return;
    }

    // Check if scripting engine exists
    if (!m_ScriptEngine || !m_ScriptEngine->L) {
        qWarning() << "loadScriptFile: No scripting engine";
        note("Cannot load script file: Scripting engine not initialized");
        return;
    }

    qCDebug(lcWorld) << "loadScriptFile: Loading" << m_strScriptFilename;

    // Open script file
    QFile file(m_strScriptFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("Cannot open script file: %1").arg(m_strScriptFilename);
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
                   QString("Script file contains errors: %1").arg(m_strScriptFilename));
    } else {
        qCDebug(lcWorld) << "loadScriptFile: Script executed successfully";
        // Optional: notify user
        // note(QString("Script file loaded: %1").arg(m_strScriptFilename));
    }

    // Update file modification time for change detection
    QFileInfo fileInfo(m_strScriptFilename);
    if (fileInfo.exists()) {
        m_timeScriptFileMod = fileInfo.lastModified();
    }
}

/**
 * setupScriptFileWatcher - Set up file watcher for script file changes
 *
 * Creates or updates the QFileSystemWatcher to monitor the script file.
 * Called after setting m_strScriptFilename or loading a world.
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
    if (m_strScriptFilename.isEmpty()) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: No script file to watch";
        return;
    }

    if (m_nReloadOption == eReloadNever) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: Reload option is 'never', not watching";
        return;
    }

    // Check if script file exists
    QFileInfo fileInfo(m_strScriptFilename);
    if (!fileInfo.exists()) {
        qCDebug(lcWorld) << "setupScriptFileWatcher: Script file does not exist:"
                         << m_strScriptFilename;
        return;
    }

    // Create file watcher
    m_scriptFileWatcher = new QFileSystemWatcher(this);
    m_scriptFileWatcher->addPath(m_strScriptFilename);

    // Connect file change signal
    connect(m_scriptFileWatcher, &QFileSystemWatcher::fileChanged, this,
            &WorldDocument::onScriptFileChanged);

    // Store initial modification time
    m_timeScriptFileMod = fileInfo.lastModified();

    qCDebug(lcWorld) << "setupScriptFileWatcher: Watching" << m_strScriptFilename;
}

/**
 * onScriptFileChanged - Handle script file change notification
 *
 * Called when QFileSystemWatcher detects the script file has changed.
 * Behavior depends on m_nReloadOption:
 * - eReloadConfirm (0): Show dialog asking user
 * - eReloadAlways (1): Automatically reload
 * - eReloadNever (2): Ignore (but shouldn't get here since watcher not set up)
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

    qCDebug(lcWorld) << "onScriptFileChanged: Script file changed:" << m_strScriptFilename;

    // Check if file still exists (may have been deleted)
    QFileInfo fileInfo(m_strScriptFilename);
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
    switch (m_nReloadOption) {
        case eReloadAlways:
            // Automatically reload
            qCDebug(lcWorld) << "onScriptFileChanged: Auto-reloading script";
            note(QString("Script file changed, reloading: %1")
                     .arg(QFileInfo(m_strScriptFilename).fileName()));
            loadScriptFile();
            break;

        case eReloadConfirm: {
            // Show confirmation dialog
            QString filename = QFileInfo(m_strScriptFilename).fileName();
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

        case eReloadNever:
        default:
            // Ignore changes
            qCDebug(lcWorld) << "onScriptFileChanged: Ignoring (reload option is 'never')";
            m_timeScriptFileMod = newModTime;
            break;
    }

    // Re-add the file to the watcher (QFileSystemWatcher removes it after notification)
    if (m_scriptFileWatcher && fileInfo.exists()) {
        m_scriptFileWatcher->addPath(m_strScriptFilename);
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
    if (m_strScriptFilename.isEmpty()) {
        return;
    }

    // Open script file
    QFile file(m_strScriptFilename);
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
