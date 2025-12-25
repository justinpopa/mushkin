// world_protocol.cpp
// Telnet Protocol State Machine and Related Protocol Handling
//
// Implements telnet protocol processing for Mushkin
// Port of: telnet_phases.cpp from original MUSHclient
//
// Provides:
// - ProcessIncomingByte() - Main state machine dispatcher
// - Phase_*() methods - State-specific byte handlers
// - Send_IAC_*() methods - Telnet negotiation with loop prevention
// - Handle_TELOPT_*() methods - Subnegotiation handlers
// - Support methods - SendPacket, Handle_IAC_GA, OutputBadUTF8characters, InitZlib
//
// Telnet protocol (RFC 854, RFC 855):
// - IAC (Interpret As Command) escape sequences
// - WILL/WONT/DO/DONT negotiation
// - Subnegotiation (SB...SE)
// - Special handling for MCCP, MXP, CHARSET, TERMINAL_TYPE
//
// ANSI escape sequences (ESC [ codes):
// - Color codes (30-37, 40-47, 90-97, 100-107)
// - 256-color (38;5;N, 48;5;N)
// - Truecolor (38;2;R;G;B, 48;2;R;G;B)
//
// Compression (MCCP v1 and v2):
// - zlib decompression of incoming data
// - Buffer management
//
// UTF-8 multibyte character handling:
// - Validation and fallback for invalid sequences

#include "../automation/plugin.h" // For plugin callback constants
#include "../text/style.h"        // For Style
#include "logging.h"              // For qCDebug(lcWorld)
#include "world_document.h"
#include "world_socket.h" // For m_pSocket->send()
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStringDecoder>
#include <zlib.h>

// ========== xterm 256-Color Palette ==========
// Port from: MUSHclient.cpp (Generate256colours)
// This is the standard xterm 256-color palette in BGR format (Windows COLORREF)
QRgb xterm_256_colours[256];

// BGR macro for Windows COLORREF compatibility (0x00BBGGRR)
#define BGR(r, g, b) (static_cast<QRgb>((r) | ((g) << 8) | ((b) << 16)))

// Initialize the xterm 256-color palette in BGR format
static void initializeXterm256Colors()
{
    // Colors 0-7: standard colors (normal intensity) - BGR format
    xterm_256_colours[0] = BGR(0, 0, 0);       // black
    xterm_256_colours[1] = BGR(128, 0, 0);     // maroon
    xterm_256_colours[2] = BGR(0, 128, 0);     // green
    xterm_256_colours[3] = BGR(128, 128, 0);   // olive
    xterm_256_colours[4] = BGR(0, 0, 128);     // navy
    xterm_256_colours[5] = BGR(128, 0, 128);   // purple
    xterm_256_colours[6] = BGR(0, 128, 128);   // teal
    xterm_256_colours[7] = BGR(192, 192, 192); // silver

    // Colors 8-15: bright colors (high intensity) - BGR format
    xterm_256_colours[8] = BGR(128, 128, 128);  // gray
    xterm_256_colours[9] = BGR(255, 0, 0);      // red
    xterm_256_colours[10] = BGR(0, 255, 0);     // lime
    xterm_256_colours[11] = BGR(255, 255, 0);   // yellow
    xterm_256_colours[12] = BGR(0, 0, 255);     // blue
    xterm_256_colours[13] = BGR(255, 0, 255);   // magenta
    xterm_256_colours[14] = BGR(0, 255, 255);   // cyan
    xterm_256_colours[15] = BGR(255, 255, 255); // white

    // Colors 16-231: 6x6x6 color cube - BGR format
    const int values[6] = {0, 95, 135, 175, 215, 255};
    for (int red = 0; red < 6; red++) {
        for (int green = 0; green < 6; green++) {
            for (int blue = 0; blue < 6; blue++) {
                xterm_256_colours[16 + (red * 36) + (green * 6) + blue] =
                    BGR(values[red], values[green], values[blue]);
            }
        }
    }

    // Colors 232-255: grayscale ramp (24 shades of gray) - BGR format
    // (grayscale is symmetric, so BGR vs RGB doesn't matter)
    for (int grey = 0; grey < 24; grey++) {
        quint8 value = 8 + (grey * 10);
        xterm_256_colours[232 + grey] = BGR(value, value, value);
    }
}

// Initialize the palette at program startup
static struct XtermColorInit {
    XtermColorInit()
    {
        initializeXterm256Colors();
    }
} xterm_color_init;

// ========== Telnet State Machine ==========

/**
 * ProcessIncomingByte - Main telnet protocol state machine dispatcher
 *
 * Routes each byte to the appropriate phase handler based on current m_phase.
 * This allows telnet sequences to span packet boundaries correctly.
 *
 * Port from: telnet_phases.cpp (conceptual - dispatching happens inline)
 */
void WorldDocument::ProcessIncomingByte(unsigned char c)
{
    // Special case: UTF-8 multibyte start in normal mode
    if (m_bUTF_8 && m_phase == NONE && (c & 0x80)) {
        // Check for UTF-8 start byte
        if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence: 110xxxxx
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 1;
            m_phase = HAVE_UTF8_CHARACTER;
            return;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence: 1110xxxx
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 2;
            m_phase = HAVE_UTF8_CHARACTER;
            return;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence: 11110xxx
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 3;
            m_phase = HAVE_UTF8_CHARACTER;
            return;
        }
        // Invalid UTF-8 start byte - fall through to process as normal
    }

    // Dispatch based on current phase
    switch (m_phase) {
        case NONE:
            if (c == IAC) {
                m_phase = HAVE_IAC;
            } else if (c == 0x1B) { // ESC
                m_phase = HAVE_ESC;
            } else if (c == '<' && m_bMXP && (MXP_Open() || MXP_Secure())) {
                // Start MXP element collection
                m_strMXPstring.clear();
                m_phase = HAVE_MXP_ELEMENT;
            } else if (c == '&' && m_bMXP && (MXP_Open() || MXP_Secure())) {
                // Start MXP entity collection
                m_strMXPstring.clear();
                m_phase = HAVE_MXP_ENTITY;
            } else if (c == '\n') {
                // Newline - complete current line
                StartNewLine(true, 0);
            } else if (c == '\r') {
                // Carriage return without LF - ignore for now
                // (Some MUDs use \r\n, we'll get the \n next)
            } else if (c >= 32 || c == '\t') {
                // Printable character or tab
                AddToLine(c);
            }
            // Else: control character (0-31 except \t) - ignore
            break;

        case HAVE_ESC:
            Phase_ESC(c);
            break;

        case DOING_CODE:
        case HAVE_FOREGROUND_256_START:
        case HAVE_FOREGROUND_256_FINISH:
        case HAVE_BACKGROUND_256_START:
        case HAVE_BACKGROUND_256_FINISH:
        case HAVE_FOREGROUND_24B_FINISH:
        case HAVE_FOREGROUND_24BR_FINISH:
        case HAVE_FOREGROUND_24BG_FINISH:
        case HAVE_FOREGROUND_24BB_FINISH:
        case HAVE_BACKGROUND_24B_FINISH:
        case HAVE_BACKGROUND_24BR_FINISH:
        case HAVE_BACKGROUND_24BG_FINISH:
        case HAVE_BACKGROUND_24BB_FINISH:
            Phase_ANSI(c);
            break;

        case HAVE_IAC:
            Phase_IAC(c);
            break;

        case HAVE_WILL:
            Phase_WILL(c);
            break;

        case HAVE_WONT:
            Phase_WONT(c);
            break;

        case HAVE_DO:
            Phase_DO(c);
            break;

        case HAVE_DONT:
            Phase_DONT(c);
            break;

        case HAVE_SB:
            Phase_SB(c);
            break;

        case HAVE_SUBNEGOTIATION:
            Phase_SUBNEGOTIATION(c);
            break;

        case HAVE_SUBNEGOTIATION_IAC:
            Phase_SUBNEGOTIATION_IAC(c);
            break;

        case HAVE_UTF8_CHARACTER:
            Phase_UTF8(c);
            break;

        case HAVE_COMPRESS:
            Phase_COMPRESS(c);
            break;

        case HAVE_COMPRESS_WILL:
            Phase_COMPRESS_WILL(c);
            break;

        case HAVE_MXP_ELEMENT:
            Phase_MXP_ELEMENT(c);
            break;

        case HAVE_MXP_COMMENT:
            Phase_MXP_COMMENT(c);
            break;

        case HAVE_MXP_QUOTE:
            Phase_MXP_QUOTE(c);
            break;

        case HAVE_MXP_ENTITY:
            Phase_MXP_ENTITY(c);
            break;

        default:
            // Unknown phase - reset to normal
            qCDebug(lcWorld) << "Unknown telnet phase:" << m_phase;
            m_phase = NONE;
            break;
    }
}

/**
 * Phase_ESC - Handle ESC character
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_ESC(unsigned char c)
{
    if (c == '[') {
        m_phase = DOING_CODE;
        m_code = 0;
    } else {
        m_phase = NONE;
    }
}

/**
 * Phase_ANSI - Parse ANSI escape sequences
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_ANSI(unsigned char c)
{
    if (isdigit(c)) {
        m_code *= 10;
        m_code += c - '0';
    } else if (c == 'm') {
        // End of ANSI sequence
        if (m_phase != DOING_CODE) {
            Interpret256ANSIcode(m_code);
        } else {
            InterpretANSIcode(m_code);
        }
        m_phase = NONE;
    } else if (c == ';' || c == ':') {
        // Separator - process current code
        if (m_phase != DOING_CODE) {
            Interpret256ANSIcode(m_code);
        } else {
            InterpretANSIcode(m_code);
        }
        m_code = 0;
    } else if (c == 'z') {
        // MXP line security mode
        if (m_code == eMXP_reset) {
            MXP_Off();
        } else {
            MXP_mode_change(m_code);
        }
        m_phase = NONE;
    } else {
        m_phase = NONE;
    }
}

/**
 * Phase_IAC - Handle IAC commands
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_IAC(unsigned char& c)
{
    unsigned char new_c = 0; // returning zero stops further processing of c

    switch (c) {
        case EOR:
            m_phase = NONE;
            if (m_bConvertGAtoNewline) {
                new_c = '\n';
            }
            m_last_line_with_IAC_GA = m_total_lines;
            Handle_IAC_GA();
            break;

        case GO_AHEAD:
            m_phase = NONE;
            if (m_bConvertGAtoNewline) {
                new_c = '\n';
            }
            m_last_line_with_IAC_GA = m_total_lines;
            Handle_IAC_GA();
            break;

        case SE:
        case NOP:
        case DATA_MARK:
        case BREAK:
        case INTERRUPT_PROCESS:
        case ABORT_OUTPUT:
        case ARE_YOU_THERE:
        case ERASE_CHARACTER:
        case ERASE_LINE:
            m_phase = NONE;
            break;

        case SB:
            m_phase = HAVE_SB;
            break;

        case WILL:
            m_phase = HAVE_WILL;
            break;

        case WONT:
            m_phase = HAVE_WONT;
            break;

        case DO:
            m_phase = HAVE_DO;
            break;

        case DONT:
            m_phase = HAVE_DONT;
            break;

        case IAC:
            // Escaped IAC - treat as data
            m_phase = NONE;
            new_c = IAC;
            break;

        default:
            m_phase = NONE;
            break;
    }

    m_subnegotiation_type = 0; // no subnegotiation type yet
    c = new_c;
}

/**
 * Send_IAC_DO - Send IAC DO with loop prevention
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Send_IAC_DO(unsigned char c)
{
    // Prevent loops - don't send if already sent
    if (m_bClient_sent_IAC_DO[c]) {
        return;
    }

    unsigned char do_do_it[3] = {IAC, DO, c};
    SendPacket(do_do_it, sizeof(do_do_it));
    m_bClient_sent_IAC_DO[c] = true;
    m_bClient_sent_IAC_DONT[c] = false;
}

/**
 * Send_IAC_DONT - Send IAC DONT with loop prevention
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Send_IAC_DONT(unsigned char c)
{
    if (m_bClient_sent_IAC_DONT[c]) {
        return;
    }

    unsigned char dont_do_it[3] = {IAC, DONT, c};
    SendPacket(dont_do_it, sizeof(dont_do_it));
    m_bClient_sent_IAC_DONT[c] = true;
    m_bClient_sent_IAC_DO[c] = false;
}

/**
 * Send_IAC_WILL - Send IAC WILL with loop prevention
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Send_IAC_WILL(unsigned char c)
{
    if (m_bClient_sent_IAC_WILL[c]) {
        return;
    }

    unsigned char will_do_it[3] = {IAC, WILL, c};
    SendPacket(will_do_it, sizeof(will_do_it));
    m_bClient_sent_IAC_WILL[c] = true;
    m_bClient_sent_IAC_WONT[c] = false;
}

/**
 * Send_IAC_WONT - Send IAC WONT with loop prevention
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Send_IAC_WONT(unsigned char c)
{
    if (m_bClient_sent_IAC_WONT[c]) {
        return;
    }

    unsigned char wont_do_it[3] = {IAC, WONT, c};
    SendPacket(wont_do_it, sizeof(wont_do_it));
    m_bClient_sent_IAC_WONT[c] = true;
    m_bClient_sent_IAC_WILL[c] = false;
}

/**
 * Phase_WILL - Handle IAC WILL
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_WILL(unsigned char c)
{
    m_phase = NONE;
    m_nCount_IAC_WILL++;
    m_bClient_got_IAC_WILL[c] = true;

    switch (c) {
        case TELOPT_COMPRESS2:
        case TELOPT_COMPRESS:
            // MCCP compression handling - must allocate buffers BEFORE agreeing
            if (!m_bCompressInitOK && !m_bCompress) {
                m_bCompressInitOK = InitZlib(m_zCompress);
            }
            if (m_bCompressInitOK && !m_bDisableCompression) {
                // Allocate BOTH buffers before agreeing to compression
                if (!m_CompressOutput) {
                    m_CompressOutput = (Bytef*)malloc(m_nCompressionOutputBufferSize);
                }
                if (!m_CompressInput) {
                    m_CompressInput = (Bytef*)malloc(COMPRESS_BUFFER_LENGTH);
                }

                // Safety: If we have output buffer but not input, free output (keep both or
                // neither)
                if (m_CompressOutput && !m_CompressInput) {
                    free(m_CompressOutput);
                    m_CompressOutput = nullptr;
                }

                // Only agree if BOTH buffers allocated successfully
                if (m_CompressOutput && m_CompressInput &&
                    !(c == TELOPT_COMPRESS && m_bSupports_MCCP_2)) {
                    Send_IAC_DO(c);
                    if (c == TELOPT_COMPRESS2) {
                        m_bSupports_MCCP_2 = true;
                    }
                } else {
                    Send_IAC_DONT(c);
                }
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_SGA: // Suppress Go Ahead
            Send_IAC_DO(c);
            break;


        case TELOPT_ECHO:
            if (!m_bNoEchoOff) {
                m_bNoEcho = true;
                Send_IAC_DO(c);
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_MXP:
            if (m_iUseMXP == eMXP_Off) {
                Send_IAC_DONT(c);
            } else {
                Send_IAC_DO(c);
                if (m_iUseMXP == eMXP_Query) {
                    MXP_On();
                }
            }
            break;

        case WILL_END_OF_RECORD: // EOR
            if (m_bConvertGAtoNewline) {
                Send_IAC_DO(c);
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_CHARSET:
            Send_IAC_DO(c);
            break;

        case TELOPT_ZMP:
            // ZMP (Zenith MUD Protocol) - telnet option 93
            // Server offers ZMP support
            if (m_bUseZMP) {
                Send_IAC_DO(c);
                m_bZMP = true;
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_ATCP:
            // ATCP (Achaea Telnet Client Protocol) - telnet option 200
            // Server offers ATCP support
            if (m_bUseATCP) {
                Send_IAC_DO(c);
                m_bATCP = true;
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_MSP:
            // MSP (MUD Sound Protocol) - telnet option 90
            // Server offers MSP support
            if (m_bUseMSP) {
                Send_IAC_DO(c);
                m_bMSP = true;
            } else {
                Send_IAC_DONT(c);
            }
            break;

        default:
            // Query plugins for unknown options
            if (Handle_Telnet_Request(c, "WILL")) {
                Send_IAC_DO(c);
                Handle_Telnet_Request(c, "SENT_DO");
            } else {
                Send_IAC_DONT(c);
            }
            break;
    }
}

/**
 * Phase_WONT - Handle IAC WONT
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_WONT(unsigned char c)
{
    m_phase = NONE;
    m_nCount_IAC_WONT++;
    m_bClient_got_IAC_WONT[c] = true;

    switch (c) {
        case TELOPT_ECHO:
            if (!m_bNoEchoOff) {
                m_bNoEcho = false;
            }
            Send_IAC_DONT(c);
            break;


        default:
            Send_IAC_DONT(c);
            break;
    }
}

/**
 * Phase_DO - Handle IAC DO
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_DO(unsigned char c)
{
    m_phase = NONE;
    m_nCount_IAC_DO++;
    m_bClient_got_IAC_DO[c] = true;

    switch (c) {
        case TELOPT_SGA:
        case TELOPT_ECHO:
        case TELOPT_CHARSET:
            Send_IAC_WILL(c);
            break;

        case TELOPT_TERMINAL_TYPE:
            m_ttype_sequence = 0;
            Send_IAC_WILL(c);
            break;

        case TELOPT_NAWS:
            if (m_bNAWS) {
                Send_IAC_WILL(c);
                m_bNAWS_wanted = true;
                SendWindowSizes(m_nWrapColumn);
            } else {
                Send_IAC_WONT(c);
            }
            break;

        case TELOPT_MXP:
            if (m_iUseMXP == eMXP_Off) {
                Send_IAC_WONT(c);
            } else {
                Send_IAC_WILL(c);
                if (m_iUseMXP == eMXP_Query) {
                    MXP_On();
                }
            }
            break;

        default:
            if (Handle_Telnet_Request(c, "DO")) {
                Send_IAC_WILL(c);
                Handle_Telnet_Request(c, "SENT_WILL");
            } else {
                Send_IAC_WONT(c);
            }
            break;
    }
}

/**
 * Phase_DONT - Handle IAC DONT
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_DONT(unsigned char c)
{
    m_phase = NONE;
    Send_IAC_WONT(c);

    m_nCount_IAC_DONT++;
    m_bClient_got_IAC_DONT[c] = true;

    switch (c) {
        case TELOPT_MXP:
            if (m_bMXP) {
                MXP_Off(true);
            }
            break;

        case TELOPT_TERMINAL_TYPE:
            m_ttype_sequence = 0;
            break;

        default:
            break;
    }
}

/**
 * Phase_SB - Start subnegotiation
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_SB(unsigned char c)
{
    // Special case: TELOPT_COMPRESS (MCCP v1) doesn't use standard subnegotiation
    if (c == TELOPT_COMPRESS) {
        m_phase = HAVE_COMPRESS;
    } else {
        m_subnegotiation_type = c;
        m_IAC_subnegotiation_data.clear();
        m_phase = HAVE_SUBNEGOTIATION;
    }
}

/**
 * Phase_SUBNEGOTIATION - Collect subnegotiation data
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_SUBNEGOTIATION(unsigned char c)
{
    if (c == IAC) {
        // Got IAC inside subnegotiation
        m_phase = HAVE_SUBNEGOTIATION_IAC;
    } else {
        // Just collect the data
        m_IAC_subnegotiation_data.append(c);
    }
}

/**
 * Phase_SUBNEGOTIATION_IAC - Handle IAC within subnegotiation
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_SUBNEGOTIATION_IAC(unsigned char c)
{
    if (c == IAC) {
        // IAC IAC inside subnegotiation = escaped IAC (store single IAC)
        m_IAC_subnegotiation_data.append(c);
        m_phase = HAVE_SUBNEGOTIATION;
        return;
    }

    // Anything else (especially SE) ends the subnegotiation
    m_phase = NONE;
    m_nCount_IAC_SB++;

    // Dispatch to appropriate handler based on subnegotiation type
    switch (m_subnegotiation_type) {
        case TELOPT_COMPRESS2:
            Handle_TELOPT_COMPRESS2();
            break;

        case TELOPT_MXP:
            Handle_TELOPT_MXP();
            break;

        case TELOPT_TERMINAL_TYPE:
            Handle_TELOPT_TERMINAL_TYPE();
            break;

        case TELOPT_CHARSET:
            Handle_TELOPT_CHARSET();
            break;

        case TELOPT_ZMP:
            Handle_TELOPT_ZMP();
            break;

        case TELOPT_ATCP:
            Handle_TELOPT_ATCP();
            break;

        case TELOPT_MSP:
            Handle_TELOPT_MSP();
            break;

        case TELOPT_MUD_SPECIFIC:
            // Aardwolf telnet option 102 - call ON_PLUGIN_TELNET_OPTION with just data
            {
                QString subnegData = QString::fromLatin1(m_IAC_subnegotiation_data);
                SendToAllPluginCallbacks(ON_PLUGIN_TELNET_OPTION, subnegData, false);
            }
            // NOTE: no break, fall through to also call ON_PLUGIN_TELNET_SUBNEGOTIATION
            [[fallthrough]];

        default:
            // Notify plugins of unhandled telnet subnegotiation
            // This allows plugins to handle MUD-specific protocols
            {
                QString subnegData = QString::fromLatin1(m_IAC_subnegotiation_data);
                SendToAllPluginCallbacks(ON_PLUGIN_TELNET_SUBNEGOTIATION, m_subnegotiation_type,
                                         subnegData, false);
            }
            break;
    }
}

/**
 * Phase_UTF8 - Handle UTF-8 multibyte character continuation
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_UTF8(unsigned char c)
{
    // Append to our UTF-8 sequence
    int i = 0;
    while (m_UTF8Sequence[i]) {
        i++;
    }
    m_UTF8Sequence[i] = c;
    m_UTF8Sequence[i + 1] = 0; // null terminator

    // Check if continuation byte is valid (must be 10xxxxxx)
    if ((c & 0xC0) != 0x80) {
        // Invalid continuation - output as ANSI
        OutputBadUTF8characters();
        return;
    }

    // Decrement bytes remaining
    m_iUTF8BytesLeft--;

    // More to go?
    if (m_iUTF8BytesLeft > 0) {
        return;
    }

    // Sequence complete - validate it using Qt
    QString test = QString::fromUtf8((const char*)m_UTF8Sequence, i + 1);
    if (test.isEmpty() || test.contains(QChar::ReplacementCharacter)) {
        // Invalid UTF-8
        OutputBadUTF8characters();
        return;
    }

    // Valid - add to line
    AddToLine((const char*)m_UTF8Sequence, i + 1);
    m_phase = NONE;
}

/**
 * Phase_COMPRESS - MCCP v1 (IAC SB COMPRESS ...)
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_COMPRESS(unsigned char c)
{
    if (c == WILL) {
        m_phase = HAVE_COMPRESS_WILL;
    } else {
        m_phase = NONE; // Error
    }
}

/**
 * Phase_COMPRESS_WILL - MCCP v1 activation (IAC SB COMPRESS WILL SE)
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Phase_COMPRESS_WILL(unsigned char c)
{
    if (c == SE) {
        // MCCP v1 starts here
        m_iMCCP_type = 1;

        // Initialize zlib if not already done
        if (!m_bCompressInitOK && !m_bCompress) {
            m_bCompressInitOK = InitZlib(m_zCompress);
        }

        // Allocate BOTH buffers if needed
        if (!m_CompressOutput) {
            m_CompressOutput = (Bytef*)malloc(m_nCompressionOutputBufferSize);
            if (!m_CompressOutput) {
                qCDebug(lcWorld) << "Failed to allocate compression output buffer (MCCP v1)";
                OnConnectionDisconnect();
                m_phase = NONE;
                return;
            }
        }

        if (!m_CompressInput) {
            m_CompressInput = (Bytef*)malloc(COMPRESS_BUFFER_LENGTH);
            if (!m_CompressInput) {
                qCDebug(lcWorld) << "Failed to allocate compression input buffer (MCCP v1)";
                OnConnectionDisconnect();
                m_phase = NONE;
                return;
            }
        }

        // Check if we can compress (need BOTH buffers)
        if (!(m_bCompressInitOK && m_CompressOutput && m_CompressInput)) {
            qCDebug(lcWorld) << "Cannot process compressed output (MCCP v1) - closing connection";
            OnConnectionDisconnect();
            m_phase = NONE;
            return;
        }

        // Reset the decompression stream
        int izError = inflateReset(&m_zCompress);
        if (izError == Z_OK) {
            m_bCompress = true;
            qCDebug(lcWorld) << "MCCP v1 compression enabled";
        } else {
            qCDebug(lcWorld) << "Could not reset zlib for MCCP v1, error:" << izError;
            if (m_zCompress.msg) {
                qCDebug(lcWorld) << "  zlib message:" << m_zCompress.msg;
            }
            OnConnectionDisconnect();
        }
    }
    m_phase = NONE;
}

// ========== Support Methods ==========

/**
 * SendPacket - Send raw bytes through the socket
 *
 * Port from: doc.cpp (various locations)
 */
void WorldDocument::SendPacket(const unsigned char* data, qint64 len)
{
    if (!m_pSocket) {
        qCDebug(lcWorld) << "SendPacket: No socket available";
        return;
    }

    m_pSocket->send((const char*)data, len);
}

/**
 * Handle_Telnet_Request - Query plugins for unknown telnet options
 *
 * Port from: telnet_phases.cpp
 *
 * Called when the server sends WILL/DO for an unhandled telnet option.
 * Plugins can return true to indicate they want to handle the option.
 *
 * For "WILL" and "DO": Stop on first true (we just need to know if ANY plugin wants it)
 * For "SENT_DO" and "SENT_WILL": Call ALL plugins (these are notifications, all interested
 *                                 plugins need to receive them to do their initialization)
 *
 * @param iNumber Telnet option number (e.g., 201 for GMCP)
 * @param sType Type of request: "WILL", "DO", "SENT_WILL", "SENT_DO", etc.
 * @return true if a plugin wants to handle this option, false otherwise
 */
bool WorldDocument::Handle_Telnet_Request(int iNumber, const QString& sType)
{
    // For SENT_* notifications, call ALL plugins (don't stop on first true)
    // Multiple plugins may need to initialize when a telnet option is negotiated
    bool bStopOnTrue = !sType.startsWith("SENT_");
    return SendToAllPluginCallbacks(ON_PLUGIN_TELNET_REQUEST, iNumber, sType, bStopOnTrue);
}

/**
 * Handle_IAC_GA - Handle Go Ahead or End of Record
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Handle_IAC_GA()
{
    // Notify plugins of IAC GA/EOR (Go Ahead/End of Record)
    // This is commonly used for prompt detection
    SendToAllPluginCallbacks(ON_PLUGIN_IAC_GA);
    qCDebug(lcWorld) << "IAC GA/EOR received";
}

/**
 * OutputBadUTF8characters - Fallback for invalid UTF-8
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::OutputBadUTF8characters()
{
    // Convert each byte from local encoding to UTF-8
    auto localDecoder = QStringDecoder(QStringDecoder::System);

    for (int i = 0; m_UTF8Sequence[i]; i++) {
        // Convert single byte from local encoding to Unicode
        QByteArrayView singleByte((const char*)&m_UTF8Sequence[i], 1);
        QString unicode = localDecoder(singleByte);

        // Convert Unicode to UTF-8 and add to line
        QByteArray utf8 = unicode.toUtf8();
        AddToLine(utf8.constData(), utf8.length());

        m_cLastChar = m_UTF8Sequence[i];
    }

    m_phase = NONE;
}

// ========== Protocol-Specific Handlers (Subnegotiation) ==========

/**
 * Handle_TELOPT_COMPRESS2 - MCCP v2 activation
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Handle_TELOPT_COMPRESS2()
{
    m_iMCCP_type = 2;

    // Initialize zlib if not already done
    if (!m_bCompressInitOK && !m_bCompress) {
        m_bCompressInitOK = InitZlib(m_zCompress);
    }

    // Allocate BOTH buffers if needed
    if (!m_CompressOutput) {
        m_CompressOutput = (Bytef*)malloc(m_nCompressionOutputBufferSize);
        if (!m_CompressOutput) {
            qCDebug(lcWorld) << "Failed to allocate compression output buffer (MCCP v2)";
            OnConnectionDisconnect();
            return;
        }
    }

    if (!m_CompressInput) {
        m_CompressInput = (Bytef*)malloc(COMPRESS_BUFFER_LENGTH);
        if (!m_CompressInput) {
            qCDebug(lcWorld) << "Failed to allocate compression input buffer (MCCP v2)";
            OnConnectionDisconnect();
            return;
        }
    }

    // Check if we can compress (need BOTH buffers)
    if (!(m_bCompressInitOK && m_CompressOutput && m_CompressInput)) {
        qCDebug(lcWorld) << "Cannot process compressed output (MCCP v2) - closing connection";
        OnConnectionDisconnect();
        return;
    }

    // Reset the decompression stream
    int izError = inflateReset(&m_zCompress);
    if (izError == Z_OK) {
        m_bCompress = true;
        qCDebug(lcWorld) << "MCCP v2 compression enabled";
        return;
    }

    // Error resetting zlib
    qCDebug(lcWorld) << "Could not reset zlib, error:" << izError;
    if (m_zCompress.msg) {
        qCDebug(lcWorld) << "  zlib message:" << m_zCompress.msg;
    }
    OnConnectionDisconnect();
}

/**
 * Handle_TELOPT_MXP - MXP on command
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Handle_TELOPT_MXP()
{
    if (m_iUseMXP == eMXP_On) {
        MXP_On();
    }
}

/**
 * Handle_TELOPT_CHARSET - Character set negotiation
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Handle_TELOPT_CHARSET()
{
    // Must have at least REQUEST DELIM NAME
    if (m_IAC_subnegotiation_data.length() < 3) {
        return;
    }

    int tt = (unsigned char)m_IAC_subnegotiation_data[0];
    if (tt != 1) { // 1 = REQUEST
        return;
    }

    char delim = m_IAC_subnegotiation_data[1];
    QList<QByteArray> charsets = m_IAC_subnegotiation_data.mid(2).split(delim);

    QByteArray strCharset = "US-ASCII"; // default
    if (m_bUTF_8) {
        strCharset = "UTF-8";
    }

    bool found = charsets.contains(strCharset);

    if (found) {
        // Build: IAC SB CHARSET ACCEPTED <name> IAC SE
        unsigned char p1[] = {IAC, SB, TELOPT_CHARSET, 2}; // 2 = ACCEPTED
        unsigned char p2[] = {IAC, SE};
        unsigned char sResponse[40];
        int iLength = 0;

        memcpy(sResponse, p1, sizeof(p1));
        iLength += sizeof(p1);

        QByteArray ba = strCharset.left(20);
        memcpy(&sResponse[iLength], ba.constData(), ba.length());
        iLength += ba.length();

        memcpy(&sResponse[iLength], p2, sizeof(p2));
        iLength += sizeof(p2);

        SendPacket(sResponse, iLength);
    } else {
        // Build: IAC SB CHARSET REJECTED IAC SE
        unsigned char p[] = {IAC, SB, TELOPT_CHARSET, 3, IAC, SE}; // 3 = REJECTED
        SendPacket(p, sizeof(p));
    }
}

/**
 * Handle_TELOPT_TERMINAL_TYPE - Terminal type / MTTS
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::Handle_TELOPT_TERMINAL_TYPE()
{
    if (m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

    int tt = (unsigned char)m_IAC_subnegotiation_data[0];
    if (tt != 1) { // 1 = SEND
        return;
    }

    unsigned char p1[4] = {IAC, SB, TELOPT_TERMINAL_TYPE, 0}; // 0 = IS
    unsigned char p2[2] = {IAC, SE};
    unsigned char sResponse[40];
    int iLength = 0;

    memcpy(sResponse, p1, 4);
    iLength += 4;

    QString strTemp;
    switch (m_ttype_sequence) {
        case 0:
            strTemp = m_strTerminalIdentification.left(20); // "ANSI" or custom
            m_ttype_sequence++;
            break;

        case 1:
            strTemp = "ANSI";
            m_ttype_sequence++;
            break;

        case 2:
            if (m_bUTF_8) {
                strTemp = "MTTS 269"; // ANSI=1, UTF-8=4, 256=8, TRUECOLOR=256
            } else {
                strTemp = "MTTS 265"; // ANSI=1, 256=8, TRUECOLOR=256
            }
            break;
    }

    QByteArray ba = strTemp.toUtf8();
    memcpy(&sResponse[iLength], ba.constData(), ba.length());
    iLength += ba.length();

    memcpy(&sResponse[iLength], p2, 2);
    iLength += 2;

    SendPacket(sResponse, iLength);
}

/**
 * Handle_TELOPT_ZMP - ZMP (Zenith MUD Protocol) subnegotiation
 *
 * ZMP uses NUL-terminated strings in its subnegotiation data.
 * Format: IAC SB ZMP <command>\0<arg1>\0<arg2>\0...<argN>\0 IAC SE
 *
 * The first string is the ZMP command (package.command format).
 * Subsequent strings are arguments.
 *
 * Reference: http://zmp.sourcemud.org/spec.shtml
 */
void WorldDocument::Handle_TELOPT_ZMP()
{
    if (!m_bZMP || m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

    // Parse NUL-terminated strings from subnegotiation data
    QList<QByteArray> parts = m_IAC_subnegotiation_data.split('\0');

    // Remove empty parts (trailing NULs create empty entries)
    while (!parts.isEmpty() && parts.last().isEmpty()) {
        parts.removeLast();
    }

    if (parts.isEmpty()) {
        return;
    }

    // First part is the command (e.g., "zmp.ping", "zmp.ident")
    QString command = QString::fromUtf8(parts.at(0));

    // Remaining parts are arguments
    QStringList args;
    for (int i = 1; i < parts.size(); i++) {
        args.append(QString::fromUtf8(parts.at(i)));
    }

    // Handle built-in ZMP commands
    if (command == "zmp.ping") {
        // Respond with zmp.time
        QDateTime now = QDateTime::currentDateTimeUtc();
        QString timeStr = now.toString("yyyy-MM-dd HH:mm:ss");
        QByteArray response;
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SB));
        response.append(static_cast<char>(TELOPT_ZMP));
        response.append("zmp.time");
        response.append('\0');
        response.append(timeStr.toUtf8());
        response.append('\0');
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SE));
        SendPacket(reinterpret_cast<const unsigned char*>(response.constData()), response.size());
    } else if (command == "zmp.check") {
        // Server asking if we support a package
        // For now, we only support core zmp.* commands
        if (!args.isEmpty()) {
            QString package = args.at(0);
            bool supported = package.startsWith("zmp.");
            QByteArray response;
            response.append(static_cast<char>(IAC));
            response.append(static_cast<char>(SB));
            response.append(static_cast<char>(TELOPT_ZMP));
            response.append(supported ? "zmp.support" : "zmp.no-support");
            response.append('\0');
            response.append(package.toUtf8());
            response.append('\0');
            response.append(static_cast<char>(IAC));
            response.append(static_cast<char>(SE));
            SendPacket(reinterpret_cast<const unsigned char*>(response.constData()),
                       response.size());
        }
    } else if (command == "zmp.ident") {
        // Server asking for client identification
        QByteArray response;
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SB));
        response.append(static_cast<char>(TELOPT_ZMP));
        response.append("zmp.ident");
        response.append('\0');
        response.append("Mushkin");
        response.append('\0');
        response.append("1.0");
        response.append('\0');
        response.append("Cross-platform MUD client");
        response.append('\0');
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SE));
        SendPacket(reinterpret_cast<const unsigned char*>(response.constData()), response.size());
    }

    // Send to Lua callback for custom handling
    // Format: "command arg1 arg2 ..." (space-separated)
    QString callbackData = command;
    for (const QString& arg : args) {
        callbackData += " " + arg;
    }
    SendToAllPluginCallbacks("OnPluginZMP", callbackData, false);
}

/**
 * Handle_TELOPT_ATCP - ATCP (Achaea Telnet Client Protocol) subnegotiation
 *
 * ATCP sends data as text in the format: <package>.<command> <data>
 * Example: "Char.Vitals HP/100 MP/50"
 *
 * Reference: http://www.ironrealms.com/rapture/manual/files/FeatATCP-txt.html
 */
void WorldDocument::Handle_TELOPT_ATCP()
{
    if (!m_bATCP || m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

    // ATCP data is plain text, typically "Package.Command data"
    QString data = QString::fromUtf8(m_IAC_subnegotiation_data);

    // Split into message type and value at first space
    int spacePos = data.indexOf(' ');
    QString msgType;
    QString msgValue;

    if (spacePos > 0) {
        msgType = data.left(spacePos);
        msgValue = data.mid(spacePos + 1);
    } else {
        msgType = data;
        msgValue = QString();
    }

    // Handle special ATCP messages
    if (msgType == "Auth.Request") {
        // Server requesting authentication
        // Send client identification
        QByteArray response;
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SB));
        response.append(static_cast<char>(TELOPT_ATCP));
        response.append("hello Mushkin 1.0");
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SE));
        SendPacket(reinterpret_cast<const unsigned char*>(response.constData()), response.size());
    }

    // Send to Lua callback for custom handling
    // Format: "msgType msgValue" (space-separated)
    SendToAllPluginCallbacks("OnPluginATCP", data, false);
}

/**
 * Handle_TELOPT_MSP - MSP (MUD Sound Protocol) subnegotiation
 *
 * MSP allows MUDs to trigger sound playback. The subnegotiation data format is:
 *   <command> <filename> [V=<volume>] [L=<loops>] [P=<priority>] [T=<type>] [U=<url>]
 *
 * Commands:
 *   SOUND - Play a sound effect
 *   MUSIC - Play background music (typically loops)
 *   STOP  - Stop all sounds
 *
 * Parameters:
 *   V=<volume>   - Volume 0-100 (100 = full volume)
 *   L=<loops>    - Number of loops (-1 = infinite, 1 = once)
 *   P=<priority> - Priority for interruption (ignored)
 *   T=<type>     - Sound type (for categorization, ignored)
 *   U=<url>      - URL base for downloading sounds
 *
 * If the sound file is not found locally and U= is provided, the file will be
 * downloaded from url+filename and cached in sounds/cached/ for future use.
 *
 * Reference: http://www.zuggsoft.com/zmud/msp.htm
 */
void WorldDocument::Handle_TELOPT_MSP()
{
    if (!m_bMSP || m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

    // MSP data is plain text: "COMMAND filename [params...]"
    QString data = QString::fromUtf8(m_IAC_subnegotiation_data);

    // Parse command and filename
    QStringList parts = data.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        qWarning() << "MSP: Invalid data (need command and filename):" << data;
        return;
    }

    QString command = parts[0].toUpper();
    QString filename = parts[1];

    // Parse optional parameters
    int volume = 100; // Default: full volume
    int loops = 1;    // Default: play once
    QString url;      // Base URL for downloading

    for (int i = 2; i < parts.size(); i++) {
        QString param = parts[i];
        if (param.startsWith("V=", Qt::CaseInsensitive)) {
            volume = param.mid(2).toInt();
            volume = qBound(0, volume, 100);
        } else if (param.startsWith("L=", Qt::CaseInsensitive)) {
            loops = param.mid(2).toInt();
        } else if (param.startsWith("U=", Qt::CaseInsensitive)) {
            url = param.mid(2);
        }
        // P= (priority) and T= (type) are ignored
    }

    // Convert volume 0-100 to our API scale (-100 to 0)
    // MSP: 100 = full volume, 0 = silent
    // Our API: 0 = full volume, -100 = silent
    double volumeApi = static_cast<double>(volume) - 100.0;

    bool loop = (loops < 0 || loops > 1); // -1 = infinite, >1 = multiple loops

    // Handle commands
    if (command == "SOUND") {
        qDebug() << "MSP SOUND:" << filename << "volume:" << volume << "loops:" << loops;

        // Use buffer 0 for auto-select
        PlayMSPSound(filename, url, loop, volumeApi, 0);
    } else if (command == "MUSIC") {
        qDebug() << "MSP MUSIC:" << filename << "volume:" << volume << "loops:" << loops;

        // Music uses buffer 1 (dedicated), loops by default unless L=1
        bool musicLoop = (loops != 1);
        PlayMSPSound(filename, url, musicLoop, volumeApi, 1);
    } else if (command == "STOP") {
        qDebug() << "MSP STOP";
        StopSound(0); // Stop all sounds
    } else {
        qWarning() << "MSP: Unknown command:" << command;
    }

    // Send to Lua callback for custom handling
    SendToAllPluginCallbacks("OnPluginMSP", data, false);
}

/**
 * PlayMSPSound - Play a sound for MSP, downloading if necessary
 *
 * @param filename Sound filename
 * @param url Base URL for downloading (empty = no download)
 * @param loop Loop the sound?
 * @param volume Volume (-100 to 0)
 * @param buffer Buffer number (0 = auto, 1-10 = specific)
 */
void WorldDocument::PlayMSPSound(const QString& filename, const QString& url, bool loop,
                                 double volume, qint16 buffer)
{
    // First, try to resolve the file locally
    QString fullPath = ResolveFilePath(filename);

    if (QFile::exists(fullPath)) {
        // File exists locally, play it
        PlaySound(buffer, fullPath, loop, volume, 0.0);
        return;
    }

    // Check if we have it cached
    QString cacheDir = QDir::current().filePath("sounds/cached");
    QString cachedPath = QDir(cacheDir).filePath(filename);

    if (QFile::exists(cachedPath)) {
        // Found in cache, play it
        PlaySound(buffer, cachedPath, loop, volume, 0.0);
        return;
    }

    // File not found locally - try to download if URL provided
    if (url.isEmpty()) {
        qWarning() << "MSP: Sound file not found and no URL provided:" << filename;
        return;
    }

    // Build the download URL
    QString downloadUrl = url;
    if (!downloadUrl.endsWith('/'))
        downloadUrl += '/';
    downloadUrl += filename;

    qDebug() << "MSP: Downloading sound from:" << downloadUrl;

    // Create cache directory if needed
    QDir().mkpath(cacheDir);

    // Create network manager for this download (will be deleted when done)
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl downloadQUrl(downloadUrl);
    QNetworkRequest netRequest(downloadQUrl);
    netRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                            QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager->get(netRequest);

    // Store parameters for when download completes
    // Use QObject properties to pass data to the lambda
    reply->setProperty("msp_filename", filename);
    reply->setProperty("msp_cached_path", cachedPath);
    reply->setProperty("msp_buffer", buffer);
    reply->setProperty("msp_loop", loop);
    reply->setProperty("msp_volume", volume);

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        // Retrieve stored parameters
        QString filename = reply->property("msp_filename").toString();
        QString cachedPath = reply->property("msp_cached_path").toString();
        qint16 buffer = static_cast<qint16>(reply->property("msp_buffer").toInt());
        bool loop = reply->property("msp_loop").toBool();
        double volume = reply->property("msp_volume").toDouble();

        if (reply->error() == QNetworkReply::NoError) {
            // Save to cache
            QFile cacheFile(cachedPath);
            if (cacheFile.open(QIODevice::WriteOnly)) {
                cacheFile.write(reply->readAll());
                cacheFile.close();
                qDebug() << "MSP: Downloaded and cached:" << cachedPath;

                // Now play the cached file
                PlaySound(buffer, cachedPath, loop, volume, 0.0);
            } else {
                qWarning() << "MSP: Failed to cache file:" << cachedPath;
            }
        } else {
            qWarning() << "MSP: Download failed:" << reply->errorString();
        }

        // Clean up
        reply->deleteLater();
        manager->deleteLater();
    });
}


// ========== Style Management ==========
void WorldDocument::RememberStyle(const Style* pStyle)
{
    if (!pStyle)
        return;

    // Save current style attributes
    m_iFlags = pStyle->iFlags & STYLE_BITS;
    m_iForeColour = pStyle->iForeColour;
    m_iBackColour = pStyle->iBackColour;

    // Note: We don't save pAction here - it's managed separately via m_currentAction
}

/**
 * GetStyleRGB - Convert style colors to RGB values
 *
 * Style Tracking
 *
 * This converts color values from ANSI indices (0-7) to actual RGB colors
 * using the lookup tables (m_normalcolour[], m_boldcolour[]). If already
 * in RGB mode, returns colors as-is.
 *
 * @param pOldStyle Style to get colors from (nullptr = use current style)
 * @param iForeColour Output: RGB foreground color
 * @param iBackColour Output: RGB background color
 */
void WorldDocument::GetStyleRGB(const Style* pOldStyle, QRgb& iForeColour, QRgb& iBackColour)
{
    quint16 flags;
    QRgb fore, back;

    if (pOldStyle) {
        // Use provided style
        flags = pOldStyle->iFlags;
        fore = pOldStyle->iForeColour;
        back = pOldStyle->iBackColour;
    } else {
        // Use current document style
        flags = m_iFlags;
        fore = m_iForeColour;
        back = m_iBackColour;
    }

    // Check color mode
    if ((flags & COLOURTYPE) == COLOUR_ANSI) {
        // Convert ANSI indices (0-7) to RGB using lookup tables
        int foreIndex = fore & 0xFF;
        int backIndex = back & 0xFF;

        if (foreIndex < 8) {
            // Use bold table if HILITE flag set, otherwise normal table
            bool bold = (flags & HILITE) != 0;
            iForeColour = bold ? m_boldcolour[foreIndex] : m_normalcolour[foreIndex];
        } else {
            // Invalid index, use as-is
            iForeColour = fore;
        }

        if (backIndex < 8) {
            // Background always uses normal table (never bold)
            iBackColour = m_normalcolour[backIndex];
        } else {
            // Invalid index, use as-is
            iBackColour = back;
        }
    } else if ((flags & COLOURTYPE) == COLOUR_CUSTOM) {
        // Custom color - use custom color tables
        int foreIndex = fore & 0xFF;
        int backIndex = back & 0xFF;

        if (foreIndex < MAX_CUSTOM) {
            iForeColour = m_customtext[foreIndex];
        } else {
            iForeColour = fore;
        }

        if (backIndex < MAX_CUSTOM) {
            iBackColour = m_customback[backIndex];
        } else {
            iBackColour = back;
        }
    } else {
        // Already RGB (COLOUR_RGB mode) - use directly
        iForeColour = fore;
        iBackColour = back;
    }
}


// ========== ANSI Parser Implementation ==========

/**
 * InterpretANSIcode - Process ANSI color and style codes
 *
 * Handles standard ANSI codes (0-49) for:
 * - Reset (0)
 * - Bold, underline, blink, inverse, strikeout (1-9)
 * - Cancel codes (22-29)
 * - Foreground colors (30-37, 39)
 * - Background colors (40-47, 49)
 *
 * Port from: ansi.cpp
 */
void WorldDocument::InterpretANSIcode(const int iCode)
{
    // Handle 256-color/24-bit extended sequences first
    switch (iCode) {
        case ANSI_TEXT_256_COLOUR:
            m_phase = HAVE_FOREGROUND_256_START;
            return;
        case ANSI_BACK_256_COLOUR:
            m_phase = HAVE_BACKGROUND_256_START;
            return;
    }

    // For now, without the full line/style system, we'll just update our current colors
    // This will be connected to the real style system
    // The logic below follows the original exactly but operates on
    // m_iFlags/m_iForeColour/m_iBackColour

    quint16 iFlags = m_iFlags & STYLE_BITS;
    QRgb iForeColour = m_iForeColour;
    QRgb iBackColour = m_iBackColour;
    void* pAction = nullptr; // No actions yet

    // If in custom mode, switch to RGB for ANSI colors
    if ((iFlags & COLOURTYPE) == COLOUR_CUSTOM) {
        if ((iCode >= ANSI_TEXT_BLACK && iCode <= ANSI_TEXT_WHITE) ||
            (iCode >= ANSI_BACK_BLACK && iCode <= ANSI_BACK_WHITE) ||
            (iCode == ANSI_SET_FOREGROUND_DEFAULT) || (iCode == ANSI_SET_BACKGROUND_DEFAULT)) {
            // Switch from custom to RGB
            GetStyleRGB(nullptr, iForeColour, iBackColour);
            iFlags &= ~COLOURTYPE;
            iFlags |= COLOUR_RGB;
        }
    }

    // If in RGB mode, do RGB conversion now (not at display time)
    if ((iFlags & COLOURTYPE) == COLOUR_RGB) {
        int i;

        // Foreground color change
        if ((iCode >= ANSI_TEXT_BLACK && iCode <= ANSI_TEXT_WHITE) ||
            (iCode == ANSI_SET_FOREGROUND_DEFAULT)) {
            if (iCode == ANSI_SET_FOREGROUND_DEFAULT)
                i = WHITE;
            else
                i = iCode - ANSI_TEXT_BLACK;

            if (iFlags & INVERSE) {
                // Inverse: foreground code affects background
                if (m_bAlternativeInverse) {
                    if (iFlags & HILITE)
                        iBackColour = m_boldcolour[i];
                    else
                        iBackColour = m_normalcolour[i];
                } else {
                    iBackColour = m_normalcolour[i];
                }
            } else {
                // Normal: foreground code affects foreground
                if (m_bCustom16isDefaultColour && (iCode == ANSI_SET_FOREGROUND_DEFAULT))
                    iForeColour = m_customtext[15];
                else {
                    if (iFlags & HILITE)
                        iForeColour = m_boldcolour[i];
                    else
                        iForeColour = m_normalcolour[i];
                }
            }
        }
        // Background color change
        else if ((iCode >= ANSI_BACK_BLACK && iCode <= ANSI_BACK_WHITE) ||
                 (iCode == ANSI_SET_BACKGROUND_DEFAULT)) {
            if (iCode == ANSI_SET_BACKGROUND_DEFAULT)
                i = BLACK;
            else
                i = iCode - ANSI_BACK_BLACK;

            if (iFlags & INVERSE) {
                // Inverse: background code affects foreground
                if (m_bAlternativeInverse) {
                    if (iFlags & HILITE)
                        iForeColour = m_boldcolour[i];
                    else
                        iForeColour = m_normalcolour[i];
                } else {
                    iForeColour = m_normalcolour[i];
                }
            } else {
                // Normal: background code affects background
                if (m_bCustom16isDefaultColour && (iCode == ANSI_SET_BACKGROUND_DEFAULT))
                    iBackColour = m_customback[15];
                else
                    iBackColour = m_normalcolour[i];
            }
        }
    } else {
        // Not RGB - just store ANSI color indices (0-7)
        switch (iCode) {
            case ANSI_TEXT_BLACK:
                iForeColour = ANSI_BLACK;
                break;
            case ANSI_TEXT_RED:
                iForeColour = ANSI_RED;
                break;
            case ANSI_TEXT_GREEN:
                iForeColour = ANSI_GREEN;
                break;
            case ANSI_TEXT_YELLOW:
                iForeColour = ANSI_YELLOW;
                break;
            case ANSI_TEXT_BLUE:
                iForeColour = ANSI_BLUE;
                break;
            case ANSI_TEXT_MAGENTA:
                iForeColour = ANSI_MAGENTA;
                break;
            case ANSI_TEXT_CYAN:
                iForeColour = ANSI_CYAN;
                break;
            case ANSI_TEXT_WHITE:
                iForeColour = ANSI_WHITE;
                break;

            case ANSI_SET_FOREGROUND_DEFAULT:
                if (m_bCustom16isDefaultColour) {
                    iForeColour = 15; // custom color 16
                    iFlags |= COLOUR_CUSTOM;
                } else {
                    iForeColour = ANSI_WHITE;
                }
                break;

            case ANSI_BACK_BLACK:
                iBackColour = ANSI_BLACK;
                break;
            case ANSI_BACK_RED:
                iBackColour = ANSI_RED;
                break;
            case ANSI_BACK_GREEN:
                iBackColour = ANSI_GREEN;
                break;
            case ANSI_BACK_YELLOW:
                iBackColour = ANSI_YELLOW;
                break;
            case ANSI_BACK_BLUE:
                iBackColour = ANSI_BLUE;
                break;
            case ANSI_BACK_MAGENTA:
                iBackColour = ANSI_MAGENTA;
                break;
            case ANSI_BACK_CYAN:
                iBackColour = ANSI_CYAN;
                break;
            case ANSI_BACK_WHITE:
                iBackColour = ANSI_WHITE;
                break;

            case ANSI_SET_BACKGROUND_DEFAULT:
                if (m_bCustom16isDefaultColour) {
                    iBackColour = 15;
                    iFlags |= COLOUR_CUSTOM;
                } else {
                    iBackColour = ANSI_BLACK;
                }
                break;
        }
    }

    // Handle formatting codes (work in both ANSI and RGB mode)
    switch (iCode) {
        case ANSI_RESET: // Reset everything
            iFlags &= ~(STYLE_BITS & ~ACTIONTYPE);
            if (m_bCustom16isDefaultColour) {
                iForeColour = 15;
                iBackColour = 15;
                iFlags |= COLOUR_CUSTOM;
            } else {
                iForeColour = WHITE;
                iBackColour = BLACK;
            }
            break;

        case ANSI_BOLD:
            // If in RGB mode with custom16 default, manually brighten color
            if (m_bCustom16isDefaultColour && (iFlags & COLOURTYPE) == COLOUR_RGB &&
                !(iFlags & HILITE)) {
                for (int i = 0; i < 7; i++) {
                    if (iForeColour == m_normalcolour[i]) {
                        iForeColour = m_boldcolour[i];
                        break;
                    }
                }
            }
            iFlags |= HILITE;
            break;

        case ANSI_BLINK:
            iFlags |= BLINK;
            break; // Also used for italic
        case ANSI_UNDERLINE:
            iFlags |= UNDERLINE;
            break;
        case ANSI_SLOW_BLINK:
            iFlags |= BLINK;
            break;
        case ANSI_FAST_BLINK:
            iFlags |= BLINK;
            break;
        case ANSI_INVERSE:
            iFlags |= INVERSE;
            break;
        case ANSI_STRIKEOUT:
            iFlags |= STRIKEOUT;
            break;

        // Cancel codes
        case ANSI_CANCEL_BOLD:
            // If in RGB mode with custom16 default, manually dim color
            if (m_bCustom16isDefaultColour && (iFlags & COLOURTYPE) == COLOUR_RGB &&
                (iFlags & HILITE)) {
                for (int i = 0; i < 7; i++) {
                    if (iForeColour == m_boldcolour[i]) {
                        iForeColour = m_normalcolour[i];
                        break;
                    }
                }
            }
            iFlags &= ~HILITE;
            break;

        case ANSI_CANCEL_BLINK:
            iFlags &= ~BLINK;
            break;
        case ANSI_CANCEL_UNDERLINE:
            iFlags &= ~UNDERLINE;
            break;
        case ANSI_CANCEL_SLOW_BLINK:
            iFlags &= ~BLINK;
            break;
        case ANSI_CANCEL_INVERSE:
            iFlags &= ~INVERSE;
            break;
        case ANSI_CANCEL_STRIKEOUT:
            iFlags &= ~STRIKEOUT;
            break;
    }

    // If nothing changed, don't create a new style
    if (iFlags == m_iFlags && iForeColour == m_iForeColour && iBackColour == m_iBackColour)
        return;

    // Update current style state using RememberStyle
    Style tempStyle;
    tempStyle.iFlags = iFlags;
    tempStyle.iForeColour = iForeColour;
    tempStyle.iBackColour = iBackColour;
    RememberStyle(&tempStyle);

    qCDebug(lcWorld) << "ANSI code" << iCode << "- flags:" << Qt::hex << iFlags
                     << "fore:" << iForeColour << "back:" << iBackColour;
}


/**
 * MXP_On - Turn on MXP (MUD eXtension Protocol)
 *
 * Port from: mxp/mxpOnOff.cpp
 *
 * MXP allows servers to send HTML-like tags for clickable links, images, sounds, etc.
 * This simplified version sets the basic MXP state. Full tag parsing will be .
 */
void WorldDocument::MXP_On()
{
    // Do nothing if already on
    if (m_bMXP) {
        return;
    }

    qCDebug(lcWorld) << "MXP turned on";

    // Basic MXP initialization
    m_bMXP = true;
    m_bPuebloActive = false; // Not Pueblo mode
    m_bMXP_script = false;
    m_bPreMode = false;
    m_iMXP_mode = eMXP_open;        // Start in open mode
    m_iMXP_defaultMode = eMXP_open; // Default is open mode
    m_iListMode = 0;                // No list mode (eNoList)
    m_iListCount = 0;
    m_iLastOutstandingTagCount = 0;
    m_iMXPerrors = 0;
    m_iMXPtags = 0;
    m_iMXPentities = 0;

    // Initialize MXP elements and entities
    InitializeMXPElements();
    InitializeMXPEntities();

    // TODO: Execute OnMXP_Start script
    // TODO: SendToAllPluginCallbacks(ON_PLUGIN_MXP_START)
}


/**
 * Interpret256ANSIcode - Process 256-color and 24-bit true color codes
 *
 * Handles extended color sequences:
 * - ESC[38;5;<n>m  - 256-color foreground (n = 0-255)
 * - ESC[48;5;<n>m  - 256-color background (n = 0-255)
 * - ESC[38;2;<r>;<g>;<b>m - 24-bit RGB foreground
 * - ESC[48;2;<r>;<g>;<b>m - 24-bit RGB background
 *
 * Port from: ansi.cpp
 */
void WorldDocument::Interpret256ANSIcode(const int iCode)
{
    // Handle phase transitions for 256-color and 24-bit
    switch (m_phase) {
        case HAVE_FOREGROUND_256_START: // Got ESC[38;
            if (iCode == 5) {           // 8-bit color (256-color)
                m_code = 0;
                m_phase = HAVE_FOREGROUND_256_FINISH;
            } else if (iCode == 2) { // 24-bit RGB
                m_code = 0;
                m_phase = HAVE_FOREGROUND_24B_FINISH;
            } else {
                m_phase = NONE;
            }
            return;

        case HAVE_BACKGROUND_256_START: // Got ESC[48;
            if (iCode == 5) {           // 8-bit color (256-color)
                m_code = 0;
                m_phase = HAVE_BACKGROUND_256_FINISH;
            } else if (iCode == 2) { // 24-bit RGB
                m_code = 0;
                m_phase = HAVE_BACKGROUND_24B_FINISH;
            } else {
                m_phase = NONE;
            }
            return;

        default:
            // Other phases not handled by this function
            break;
    }

    // Validate color index (0-255)
    if (iCode < 0 || iCode > 255) {
        m_phase = DOING_CODE;
        return;
    }

    // Get current style
    quint16 iFlags = m_iFlags & STYLE_BITS;
    QRgb iForeColour = m_iForeColour;
    QRgb iBackColour = m_iBackColour;
    void* pAction = nullptr;

    // If in custom or ANSI mode, switch to RGB for extended colors
    if ((iFlags & COLOURTYPE) == COLOUR_CUSTOM || (iFlags & COLOURTYPE) == COLOUR_ANSI) {
        GetStyleRGB(nullptr, iForeColour, iBackColour);
        iFlags &= ~COLOURTYPE;
        iFlags |= COLOUR_RGB;
    }

    // Determine if we're setting foreground or background
    bool iForegroundMode =
        (m_phase == HAVE_FOREGROUND_24B_FINISH || m_phase == HAVE_FOREGROUND_24BR_FINISH ||
         m_phase == HAVE_FOREGROUND_24BG_FINISH || m_phase == HAVE_FOREGROUND_24BB_FINISH);

    // Build RGB color for 24-bit mode
    QRgb workingColor;
    if (iForegroundMode && (iFlags & INVERSE) || (!iForegroundMode && !(iFlags & INVERSE)))
        workingColor = iBackColour;
    else
        workingColor = iForeColour;

    // Handle 24-bit true color R,G,B components
    switch (m_phase) {
        case HAVE_FOREGROUND_24B_FINISH: // Got ESC[38;2; - waiting for R
            iFlags &= ~COLOURTYPE;
            iFlags |= COLOUR_RGB;
            workingColor = qRgb(iCode, 0, 0); // Set Red
            m_phase = HAVE_FOREGROUND_24BR_FINISH;
            break;

        case HAVE_FOREGROUND_24BR_FINISH:                      // Got ESC[38;2;<r>; - waiting for G
            workingColor = qRgb(qRed(workingColor), iCode, 0); // Set Green
            m_phase = HAVE_FOREGROUND_24BG_FINISH;
            break;

        case HAVE_FOREGROUND_24BG_FINISH: // Got ESC[38;2;<r>;<g>; - waiting for B
            workingColor = qRgb(qRed(workingColor), qGreen(workingColor), iCode); // Set Blue
            m_phase = HAVE_FOREGROUND_24BB_FINISH;
            break;

        case HAVE_BACKGROUND_24B_FINISH: // Got ESC[48;2; - waiting for R
            iFlags &= ~COLOURTYPE;
            iFlags |= COLOUR_RGB;
            workingColor = qRgb(iCode, 0, 0); // Set Red
            m_phase = HAVE_BACKGROUND_24BR_FINISH;
            break;

        case HAVE_BACKGROUND_24BR_FINISH:                      // Got ESC[48;2;<r>; - waiting for G
            workingColor = qRgb(qRed(workingColor), iCode, 0); // Set Green
            m_phase = HAVE_BACKGROUND_24BG_FINISH;
            break;

        case HAVE_BACKGROUND_24BG_FINISH: // Got ESC[48;2;<r>;<g>; - waiting for B
            workingColor = qRgb(qRed(workingColor), qGreen(workingColor), iCode); // Set Blue
            m_phase = HAVE_BACKGROUND_24BB_FINISH;
            break;

        default:
            // Other phases not handled by this function
            break;
    }

    // Apply the working color back (handle INVERSE flag)
    if (iForegroundMode && (iFlags & INVERSE) || (!iForegroundMode && !(iFlags & INVERSE)))
        iBackColour = workingColor;
    else
        iForeColour = workingColor;

    // Handle 256-color palette lookups
    if ((iFlags & COLOURTYPE) == COLOUR_RGB &&
        (m_phase == HAVE_FOREGROUND_256_FINISH || m_phase == HAVE_BACKGROUND_256_FINISH)) {
        switch (m_phase) {
            case HAVE_FOREGROUND_256_FINISH:
                if (iFlags & INVERSE)
                    iBackColour = xterm_256_colours[iCode];
                else
                    iForeColour = xterm_256_colours[iCode];
                break;

            case HAVE_BACKGROUND_256_FINISH:
                if (iFlags & INVERSE)
                    iForeColour = xterm_256_colours[iCode];
                else
                    iBackColour = xterm_256_colours[iCode];
                break;

            default:
                // Other phases not handled by this function
                break;
        }
    } else if (m_phase == HAVE_FOREGROUND_256_FINISH || m_phase == HAVE_BACKGROUND_256_FINISH) {
        // Must be in ANSI mode - store color index directly
        switch (m_phase) {
            case HAVE_FOREGROUND_256_FINISH:
                if (iFlags & INVERSE)
                    iBackColour = iCode;
                else
                    iForeColour = iCode;
                break;

            case HAVE_BACKGROUND_256_FINISH:
                if (iFlags & INVERSE)
                    iForeColour = iCode;
                else
                    iBackColour = iCode;
                break;

            default:
                // Other phases not handled by this function
                break;
        }
    }

    // Check if we've completed a color sequence
    if (m_phase == HAVE_BACKGROUND_256_FINISH || m_phase == HAVE_FOREGROUND_256_FINISH ||
        m_phase == HAVE_FOREGROUND_24BB_FINISH || m_phase == HAVE_BACKGROUND_24BB_FINISH) {
        m_phase = DOING_CODE; // Ready for more ANSI codes
    }

    // If nothing changed, don't create new style
    if (iFlags == m_iFlags && iForeColour == m_iForeColour && iBackColour == m_iBackColour)
        return;

    // Update current style state using RememberStyle
    Style tempStyle;
    tempStyle.iFlags = iFlags;
    tempStyle.iForeColour = iForeColour;
    tempStyle.iBackColour = iBackColour;
    RememberStyle(&tempStyle);

    qCDebug(lcWorld) << "256/24bit code" << iCode << "phase" << m_phase << "- flags:" << Qt::hex
                     << iFlags << "fore:" << iForeColour << "back:" << iBackColour;
}


/**
 * MXP_Off - Turn off MXP or reset it
 *
 * Port from: mxp/mxpOnOff.cpp
 *
 * @param force If true, completely turn off MXP. If false, just reset to normal behavior.
 */
void WorldDocument::MXP_Off(bool force)
{
    // Reset colors to white on black if we have a current line
    // (When we have m_pCurrentLine implemented)
    // if (m_pCurrentLine) {
    //     InterpretANSIcode(0);
    // }

    // Do nothing else if already off
    if (!m_bMXP && !force) {
        return;
    }

    if (force) {
        qCDebug(lcWorld) << "Closing down MXP";
    }

    // Reset MXP state
    m_bInParagraph = false;
    m_bMXP_script = false;
    m_bPreMode = false;
    m_iListMode = 0;
    m_iListCount = 0;

    // Close all open tags
    MXP_CloseOpenTags();

    if (force) {
        // Clean up MXP resources
        CleanupMXP();
        // Change back to open mode
        MXP_mode_change(eMXP_open);

        // If in MXP collection phase, reset to NONE
        if (m_phase == HAVE_MXP_ELEMENT || m_phase == HAVE_MXP_COMMENT ||
            m_phase == HAVE_MXP_QUOTE || m_phase == HAVE_MXP_ENTITY ||
            m_phase == HAVE_MXP_ROOM_NAME || m_phase == HAVE_MXP_ROOM_DESCRIPTION ||
            m_phase == HAVE_MXP_ROOM_EXITS || m_phase == HAVE_MXP_WELCOME) {
            m_phase = NONE;
        }

        if (m_bPuebloActive) {
            qCDebug(lcWorld) << "Pueblo turned off";
        } else {
            qCDebug(lcWorld) << "MXP turned off";
        }

        m_bPuebloActive = false;
        m_bMXP = false;

        // TODO: Execute OnMXP_Stop script
        // TODO: SendToAllPluginCallbacks(ON_PLUGIN_MXP_STOP)
    }
}


/**
 * MXP_mode_change - Change MXP line security mode
 *
 * Port from: mxp/mxpMode.cpp
 *
 * MXP has different security modes that control what tags are allowed:
 * - Open: All tags allowed
 * - Secure: Limited tags
 * - Locked: No tags
 * - Permanent modes: Stay in that mode until changed
 *
 * @param mode New MXP mode (-1 means use default after newline)
 */
void WorldDocument::MXP_mode_change(int mode)
{
    // Change to default mode if -1
    if (mode == -1) {
        mode = m_iMXP_defaultMode;
    }

    // Debug output for mode changes
    const char* modeNames[] = {
        "open",                 // 0
        "secure",               // 1
        "locked",               // 2
        "reset",                // 3
        "secure next tag only", // 4
        "permanently open",     // 5
        "permanently secure",   // 6
        "permanently locked"    // 7
    };

    if (mode != m_iMXP_mode &&
        (mode == eMXP_perm_open || mode == eMXP_perm_secure || mode == eMXP_perm_locked ||
         m_iMXP_mode == eMXP_perm_open || m_iMXP_mode == eMXP_perm_secure ||
         m_iMXP_mode == eMXP_perm_locked)) {
        QString oldMode = (m_iMXP_mode >= 0 && m_iMXP_mode < 8)
                              ? QString(modeNames[m_iMXP_mode])
                              : QString("unknown mode %1").arg(m_iMXP_mode);
        QString newMode = (mode >= 0 && mode < 8) ? QString(modeNames[mode])
                                                  : QString("unknown mode %1").arg(mode);
        qCDebug(lcWorld) << "MXP mode change from" << oldMode << "to" << newMode;
    }

    // TODO: Close open tags when moving from open to not-open
    // if (MXP_Open() && mode != eMXP_open && mode != eMXP_perm_open) {
    //     MXP_CloseOpenTags();
    // }

    // Set default mode based on new mode
    switch (mode) {
        case eMXP_open:
        case eMXP_secure:
        case eMXP_locked:
            m_iMXP_defaultMode = eMXP_open; // These make open the default
            break;

        case eMXP_secure_once:
            m_iMXP_previousMode = m_iMXP_mode; // Remember previous mode
            break;

        case eMXP_perm_open:
        case eMXP_perm_secure:
        case eMXP_perm_locked:
            m_iMXP_defaultMode = mode; // Permanent mode becomes the default
            break;
    }

    // Set the new mode
    m_iMXP_mode = mode;
}


// Moved to AddToLine implementation below (after SendWindowSizes)

/**
 * InitZlib - Initialize zlib decompression stream
 *
 * Port from: original MUSHclient (search for "InitZlib")
 */
bool WorldDocument::InitZlib(z_stream& zInfo)
{
    zInfo.zalloc = Z_NULL;
    zInfo.zfree = Z_NULL;
    zInfo.opaque = Z_NULL;
    zInfo.avail_in = 0;
    zInfo.next_in = Z_NULL;

    int izError = inflateInit(&zInfo);
    if (izError == Z_OK) {
        qCDebug(lcWorld) << "zlib initialized successfully for MCCP";
        return true;
    } else {
        qCDebug(lcWorld) << "zlib init failed with error:" << izError;
        if (zInfo.msg) {
            qCDebug(lcWorld) << "  Error message:" << zInfo.msg;
        }
        return false;
    }
}


/**
 * SendWindowSizes - Send NAWS (Negotiate About Window Size) to server
 *
 * Port from: doc.cpp
 *
 * RFC 1073: NAWS sends window dimensions to the server
 * Format: IAC SB TELOPT_NAWS <width_hi> <width_lo> <height_hi> <height_lo> IAC SE
 *
 * @param width The window width in characters
 */
void WorldDocument::SendWindowSizes(int width)
{
    // Can't send if not connected or NAWS not negotiated
    if (m_iConnectPhase != eConnectConnectedToMud || !m_pSocket || !m_bNAWS_wanted) {
        return;
    }

    // For now, assume height = width (will be calculated from actual view)
    // In the original, height is calculated from m_FontHeight and view rectangle
    quint16 height = static_cast<quint16>(width);

    // Build NAWS packet: IAC SB TELOPT_NAWS <width> <height> IAC SE
    // Width and height are sent as 2 bytes each (big-endian: high byte, low byte)
    // Note: If a byte value is 255 (IAC), it must be escaped as IAC IAC

    unsigned char packet[20]; // Max: IAC SB TELOPT + 4 doubled bytes + IAC SE = 13 bytes
    int length = 0;

    // Preamble: IAC SB TELOPT_NAWS
    packet[length++] = IAC;
    packet[length++] = SB;
    packet[length++] = TELOPT_NAWS;

    // Width (high byte, low byte)
    quint8 width_hi = (width >> 8) & 0xFF;
    quint8 width_lo = width & 0xFF;

    packet[length++] = width_hi;
    if (width_hi == IAC) { // Escape IAC as IAC IAC
        packet[length++] = IAC;
    }

    packet[length++] = width_lo;
    if (width_lo == IAC) {
        packet[length++] = IAC;
    }

    // Height (high byte, low byte)
    quint8 height_hi = (height >> 8) & 0xFF;
    quint8 height_lo = height & 0xFF;

    packet[length++] = height_hi;
    if (height_hi == IAC) {
        packet[length++] = IAC;
    }

    packet[length++] = height_lo;
    if (height_lo == IAC) {
        packet[length++] = IAC;
    }

    // Postamble: IAC SE
    packet[length++] = IAC;
    packet[length++] = SE;

    SendPacket(packet, length);
    qCDebug(lcWorld) << "Sent NAWS:" << width << "x" << height;
}

// ========================================================================
// MXP Phase Handlers
// ========================================================================

/**
 * Phase_MXP_ELEMENT - Collect MXP element characters
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters for an MXP element starting with '<' until '>'.
 * Handles quotes and comments within elements.
 */
void WorldDocument::Phase_MXP_ELEMENT(unsigned char c)
{
    switch (c) {
        case '>':
            // End of element - process it
            MXP_collected_element();
            m_phase = NONE;
            break;

        case '<':
            // Shouldn't have < inside <
            qCWarning(lcMXP) << "Got \"<\" inside \"<\" - discarding previous element";
            m_strMXPstring.clear(); // Start again
            break;

        case '\'':
        case '\"':
            // Quote inside element - switch to quote mode
            m_cMXPquoteTerminator = c;
            m_phase = HAVE_MXP_QUOTE;
            m_strMXPstring += static_cast<char>(c); // Collect this character
            break;

        case '-':
            // May be a comment? Check for <!--
            m_strMXPstring += static_cast<char>(c); // Collect this character
            if (m_strMXPstring.left(3) == "!--") {
                m_phase = HAVE_MXP_COMMENT;
            }
            break;

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}

/**
 * Phase_MXP_COMMENT - Collect MXP comment
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters within an MXP comment <!-- ... -->
 */
void WorldDocument::Phase_MXP_COMMENT(unsigned char c)
{
    switch (c) {
        case '>':
            // End of comment if preceded by --
            if (m_strMXPstring.right(2) == "--") {
                // Discard comment, switch phase back to none
                m_phase = NONE;
                break;
            }
            // If > is not preceded by -- then just collect it
            // FALL THROUGH

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}

/**
 * Phase_MXP_QUOTE - Collect quoted string within MXP element
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters within quotes inside an MXP element.
 */
void WorldDocument::Phase_MXP_QUOTE(unsigned char c)
{
    // Closing quote? Change phase back to element collection
    if (c == m_cMXPquoteTerminator) {
        m_phase = HAVE_MXP_ELEMENT;
    }

    m_strMXPstring += static_cast<char>(c); // Collect this character
}

/**
 * Phase_MXP_ENTITY - Collect MXP entity
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters for an MXP entity starting with '&' until ';'.
 */
void WorldDocument::Phase_MXP_ENTITY(unsigned char c)
{
    switch (c) {
        case ';':
            // End of entity - process it
            m_phase = NONE;
            MXP_collected_entity();
            break;

        case '&':
            // Shouldn't have & inside &
            qCWarning(lcMXP) << "Got \"&\" inside \"&\" - discarding previous entity";
            m_strMXPstring.clear(); // Start again
            break;

        case '<':
            // Shouldn't have < inside &, but we are now collecting an element
            qCWarning(lcMXP) << "Got \"<\" inside \"&\" - switching to element collection";
            m_phase = HAVE_MXP_ELEMENT;
            m_strMXPstring.clear(); // Start again
            break;

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}
