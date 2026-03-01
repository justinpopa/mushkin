// telnet_parser.cpp
// TelnetParser — Companion object that owns all telnet protocol state.
//
// This file contains the implementation of TelnetParser, extracted from:
//   - world_protocol.cpp (Phase_* handlers, Send_IAC_*, Handle_TELOPT_*, support methods)
//
// Every method that was WorldDocument::Phase_FOO() or WorldDocument::Handle_FOO()
// is now TelnetParser::Phase_FOO() / TelnetParser::Handle_FOO().
// Fields that remain on WorldDocument are accessed via m_doc.field.
//
// The #define constants (IAC, DONT, DO, etc.) from world_document.h are still included
// transitively via world_document.h — TelnetParser methods use the original macro names
// so that diffs against world_protocol.cpp remain minimal.

#include "telnet_parser.h"

#include "../automation/plugin.h" // For plugin callback constants (ON_PLUGIN_*)
#include "../text/style.h"        // For Style
#include "logging.h"              // For qCDebug(lcWorld)
#include "world_document.h"       // Full definition of WorldDocument
#include "world_error.h"          // WorldError, WorldErrorType
#include "world_socket.h"         // For m_doc.m_pSocket->send()
#include <QDateTime>
#include <QStringDecoder>
#include <zlib.h>

// ========== Constructor / Destructor ==========

TelnetParser::TelnetParser(WorldDocument& doc) : m_doc(doc)
{
    m_bClient_sent_IAC_DO.fill(false);
    m_bClient_sent_IAC_DONT.fill(false);
    m_bClient_sent_IAC_WILL.fill(false);
    m_bClient_sent_IAC_WONT.fill(false);
    m_bClient_got_IAC_DO.fill(false);
    m_bClient_got_IAC_DONT.fill(false);
    m_bClient_got_IAC_WILL.fill(false);
    m_bClient_got_IAC_WONT.fill(false);
}

TelnetParser::~TelnetParser()
{
    // m_zlibStream destructor calls inflateEnd() automatically.
    m_CompressOutput.clear();
    m_CompressInput.clear();
}

// ========== Reset ==========

void TelnetParser::reset()
{
    m_phase = Phase::NONE;
    m_bClient_sent_IAC_DO.fill(false);
    m_bClient_sent_IAC_DONT.fill(false);
    m_bClient_sent_IAC_WILL.fill(false);
    m_bClient_sent_IAC_WONT.fill(false);
    m_bClient_got_IAC_DO.fill(false);
    m_bClient_got_IAC_DONT.fill(false);
    m_bClient_got_IAC_WILL.fill(false);
    m_bClient_got_IAC_WONT.fill(false);
    m_bCompress = false;
    m_subnegotiation_type = 0;
    m_IAC_subnegotiation_data.clear();
}

// ========== zlib lifecycle (internal helper) ==========
// Returns true on success. On failure, logs the error and returns false.
// Callers use this result to set m_bCompress guards before agreeing to MCCP.

static bool initZlibStream(ZlibStream& zs)
{
    const int rc = zs.init();
    if (rc == Z_OK) {
        qCDebug(lcWorld) << "zlib initialized successfully for MCCP";
        return true;
    }
    const QString msg =
        zs.stream.msg ? QString::fromLatin1(zs.stream.msg) : QString("zlib error code %1").arg(rc);
    qCDebug(lcWorld) << "zlib init failed with error:" << msg;
    return false;
}

// ========== Phase Handlers ==========

/**
 * Phase_ESC - Handle ESC character
 */
void TelnetParser::Phase_ESC(unsigned char c)
{
    if (c == '[') {
        m_phase = Phase::DOING_CODE;
        m_doc.m_code = 0;
    } else {
        m_phase = Phase::NONE;
    }
}

/**
 * Phase_ANSI - Parse ANSI escape sequences
 */
void TelnetParser::Phase_ANSI(unsigned char c)
{
    if (isdigit(c)) {
        m_doc.m_code *= 10;
        m_doc.m_code += c - '0';
    } else if (c == 'm') {
        // End of ANSI sequence
        if (m_phase != Phase::DOING_CODE) {
            m_doc.Interpret256ANSIcode(m_doc.m_code);
        } else {
            m_doc.InterpretANSIcode(m_doc.m_code);
        }
        m_phase = Phase::NONE;
    } else if (c == ';' || c == ':') {
        // Separator - process current code
        if (m_phase != Phase::DOING_CODE) {
            m_doc.Interpret256ANSIcode(m_doc.m_code);
        } else {
            m_doc.InterpretANSIcode(m_doc.m_code);
        }
        m_doc.m_code = 0;
    } else if (c == 'z') {
        // MXP line security mode
        if (m_doc.m_code == eMXP_reset) {
            m_doc.MXP_Off();
        } else {
            m_doc.MXP_mode_change(m_doc.m_code);
        }
        m_phase = Phase::NONE;
    } else {
        m_phase = Phase::NONE;
    }
}

/**
 * Phase_IAC - Handle IAC commands
 */
void TelnetParser::Phase_IAC(unsigned char& c)
{
    unsigned char new_c = 0; // returning zero stops further processing of c

    switch (c) {
        case EOR:
            m_phase = Phase::NONE;
            if (m_doc.m_spam.convert_ga_to_newline) {
                new_c = '\n';
            }
            m_doc.m_last_line_with_IAC_GA = m_doc.m_total_lines;
            Handle_IAC_GA();
            break;

        case GO_AHEAD:
            m_phase = Phase::NONE;
            if (m_doc.m_spam.convert_ga_to_newline) {
                new_c = '\n';
            }
            m_doc.m_last_line_with_IAC_GA = m_doc.m_total_lines;
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
            m_phase = Phase::NONE;
            break;

        case SB:
            m_phase = Phase::HAVE_SB;
            break;

        case WILL:
            m_phase = Phase::HAVE_WILL;
            break;

        case WONT:
            m_phase = Phase::HAVE_WONT;
            break;

        case DO:
            m_phase = Phase::HAVE_DO;
            break;

        case DONT:
            m_phase = Phase::HAVE_DONT;
            break;

        case IAC:
            // Escaped IAC - treat as data
            m_phase = Phase::NONE;
            new_c = IAC;
            break;

        default:
            m_phase = Phase::NONE;
            break;
    }

    m_subnegotiation_type = 0; // no subnegotiation type yet
    c = new_c;
}

/**
 * Send_IAC_DO - Send IAC DO with loop prevention
 */
void TelnetParser::Send_IAC_DO(unsigned char c)
{
    if (m_bClient_sent_IAC_DO[c]) {
        return;
    }

    unsigned char do_do_it[3] = {IAC, DO, c};
    m_doc.SendPacket(do_do_it);
    m_bClient_sent_IAC_DO[c] = true;
    m_bClient_sent_IAC_DONT[c] = false;
}

/**
 * Send_IAC_DONT - Send IAC DONT with loop prevention
 */
void TelnetParser::Send_IAC_DONT(unsigned char c)
{
    if (m_bClient_sent_IAC_DONT[c]) {
        return;
    }

    unsigned char dont_do_it[3] = {IAC, DONT, c};
    m_doc.SendPacket(dont_do_it);
    m_bClient_sent_IAC_DONT[c] = true;
    m_bClient_sent_IAC_DO[c] = false;
}

/**
 * Send_IAC_WILL - Send IAC WILL with loop prevention
 */
void TelnetParser::Send_IAC_WILL(unsigned char c)
{
    if (m_bClient_sent_IAC_WILL[c]) {
        return;
    }

    unsigned char will_do_it[3] = {IAC, WILL, c};
    m_doc.SendPacket(will_do_it);
    m_bClient_sent_IAC_WILL[c] = true;
    m_bClient_sent_IAC_WONT[c] = false;
}

/**
 * Send_IAC_WONT - Send IAC WONT with loop prevention
 */
void TelnetParser::Send_IAC_WONT(unsigned char c)
{
    if (m_bClient_sent_IAC_WONT[c]) {
        return;
    }

    unsigned char wont_do_it[3] = {IAC, WONT, c};
    m_doc.SendPacket(wont_do_it);
    m_bClient_sent_IAC_WONT[c] = true;
    m_bClient_sent_IAC_WILL[c] = false;
}

/**
 * Phase_WILL - Handle IAC WILL
 */
void TelnetParser::Phase_WILL(unsigned char c)
{
    m_phase = Phase::NONE;
    m_nCount_IAC_WILL++;
    m_bClient_got_IAC_WILL[c] = true;

    switch (c) {
        case TELOPT_COMPRESS2:
        case TELOPT_COMPRESS:
            // MCCP compression handling - must allocate buffers BEFORE agreeing
            if (!m_zlibStream.initialized && !m_bCompress) {
                initZlibStream(m_zlibStream);
            }
            if (m_zlibStream.initialized && !m_doc.m_bDisableCompression) {
                // Allocate BOTH buffers before agreeing to compression
                if (m_CompressOutput.empty()) {
                    m_CompressOutput.assign(COMPRESS_BUFFER_LENGTH, 0);
                }
                if (m_CompressInput.empty()) {
                    m_CompressInput.assign(COMPRESS_BUFFER_LENGTH, 0);
                }

                // Safety: If we have output buffer but not input, clear output
                if (!m_CompressOutput.empty() && m_CompressInput.empty()) {
                    m_CompressOutput.clear();
                }

                // Only agree if BOTH buffers allocated successfully
                if (!m_CompressOutput.empty() && !m_CompressInput.empty() &&
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
            if (!m_doc.m_input.no_echo_off) {
                m_bNoEcho = true;
                Send_IAC_DO(c);
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_MXP:
            if (m_doc.m_iUseMXP == MXPMode::eMXP_Off) {
                Send_IAC_DONT(c);
            } else {
                Send_IAC_DO(c);
                if (m_doc.m_iUseMXP == MXPMode::eMXP_Query) {
                    m_doc.MXP_On();
                }
            }
            break;

        case WILL_END_OF_RECORD: // EOR
            if (m_doc.m_spam.convert_ga_to_newline) {
                Send_IAC_DO(c);
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_CHARSET:
            Send_IAC_DO(c);
            break;

        case TELOPT_ZMP:
            if (m_doc.m_bUseZMP) {
                Send_IAC_DO(c);
                m_bZMP = true;
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_ATCP:
            if (m_doc.m_bUseATCP) {
                Send_IAC_DO(c);
                m_bATCP = true;
            } else {
                Send_IAC_DONT(c);
            }
            break;

        case TELOPT_MSP:
            if (m_doc.m_sound.use_msp) {
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
 */
void TelnetParser::Phase_WONT(unsigned char c)
{
    m_phase = Phase::NONE;
    m_nCount_IAC_WONT++;
    m_bClient_got_IAC_WONT[c] = true;

    switch (c) {
        case TELOPT_ECHO:
            if (!m_doc.m_input.no_echo_off) {
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
 */
void TelnetParser::Phase_DO(unsigned char c)
{
    m_phase = Phase::NONE;
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
            if (m_doc.m_bNAWS) {
                Send_IAC_WILL(c);
                m_bNAWS_wanted = true;
                sendWindowSizes(m_doc.m_display.wrap_column);
            } else {
                Send_IAC_WONT(c);
            }
            break;

        case TELOPT_MXP:
            if (m_doc.m_iUseMXP == MXPMode::eMXP_Off) {
                Send_IAC_WONT(c);
            } else {
                Send_IAC_WILL(c);
                if (m_doc.m_iUseMXP == MXPMode::eMXP_Query) {
                    m_doc.MXP_On();
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
 */
void TelnetParser::Phase_DONT(unsigned char c)
{
    m_phase = Phase::NONE;
    Send_IAC_WONT(c);

    m_nCount_IAC_DONT++;
    m_bClient_got_IAC_DONT[c] = true;

    switch (c) {
        case TELOPT_MXP:
            if (m_doc.m_mxpEngine->m_bMXP) {
                m_doc.MXP_Off(true);
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
 */
void TelnetParser::Phase_SB(unsigned char c)
{
    // Special case: TELOPT_COMPRESS (MCCP v1) doesn't use standard subnegotiation
    if (c == TELOPT_COMPRESS) {
        m_phase = Phase::HAVE_COMPRESS;
    } else {
        m_subnegotiation_type = c;
        m_IAC_subnegotiation_data.clear();
        m_phase = Phase::HAVE_SUBNEGOTIATION;
    }
}

/**
 * Phase_SUBNEGOTIATION - Collect subnegotiation data
 *
 * Security: cap buffer at 8KB to prevent OOM from malicious servers
 * sending unbounded data without IAC SE.
 */
void TelnetParser::Phase_SUBNEGOTIATION(unsigned char c)
{
    if (c == IAC) {
        m_phase = Phase::HAVE_SUBNEGOTIATION_IAC;
    } else {
        constexpr int kMaxSubnegotiationSize = 8192;
        if (m_IAC_subnegotiation_data.size() < kMaxSubnegotiationSize) {
            m_IAC_subnegotiation_data.append(c);
        } else {
            // Buffer full — discard subnegotiation and reset
            m_IAC_subnegotiation_data.clear();
            m_phase = Phase::NONE;
        }
    }
}

/**
 * Phase_SUBNEGOTIATION_IAC - Handle IAC within subnegotiation
 */
void TelnetParser::Phase_SUBNEGOTIATION_IAC(unsigned char c)
{
    if (c == IAC) {
        // IAC IAC inside subnegotiation = escaped IAC (store single IAC)
        m_IAC_subnegotiation_data.append(c);
        m_phase = Phase::HAVE_SUBNEGOTIATION;
        return;
    }

    // Anything else (especially SE) ends the subnegotiation
    m_phase = Phase::NONE;
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
                m_doc.SendToAllPluginCallbacks(ON_PLUGIN_TELNET_OPTION, subnegData, false);
            }
            // NOTE: no break, fall through to also call ON_PLUGIN_TELNET_SUBNEGOTIATION
            [[fallthrough]];

        default:
            // Notify plugins of unhandled telnet subnegotiation
            {
                QString subnegData = QString::fromLatin1(m_IAC_subnegotiation_data);
                m_doc.SendToAllPluginCallbacks(ON_PLUGIN_TELNET_SUBNEGOTIATION,
                                               m_subnegotiation_type, subnegData, false);
            }
            break;
    }
}

/**
 * Phase_UTF8 - Handle UTF-8 multibyte character continuation
 */
void TelnetParser::Phase_UTF8(unsigned char c)
{
    // Append to our UTF-8 sequence.
    // Find the first null slot, capped at size()-2 to leave room for the byte and
    // its null terminator. If the buffer is already full this indicates a corrupt
    // state (e.g. m_iUTF8BytesLeft was wrong); treat it as a bad UTF-8 sequence.
    int i = 0;
    const int maxWriteIndex = static_cast<int>(m_doc.m_UTF8Sequence.size()) - 2;
    while (i <= maxWriteIndex && m_doc.m_UTF8Sequence[i]) {
        i++;
    }
    if (i > maxWriteIndex) {
        m_doc.OutputBadUTF8characters();
        return;
    }
    m_doc.m_UTF8Sequence[i] = c;
    m_doc.m_UTF8Sequence[i + 1] = 0; // null terminator

    // Check if continuation byte is valid (must be 10xxxxxx)
    if ((c & 0xC0) != 0x80) {
        // Invalid continuation - output as ANSI
        m_doc.OutputBadUTF8characters();
        return;
    }

    // Decrement bytes remaining
    m_doc.m_iUTF8BytesLeft--;

    // More to go?
    if (m_doc.m_iUTF8BytesLeft > 0) {
        return;
    }

    // Sequence complete - validate it using Qt
    QString test =
        QString::fromUtf8(reinterpret_cast<const char*>(m_doc.m_UTF8Sequence.data()), i + 1);
    if (test.isEmpty() || test.contains(QChar::ReplacementCharacter)) {
        // Invalid UTF-8
        m_doc.OutputBadUTF8characters();
        return;
    }

    // Valid - add to line
    m_doc.AddToLine(reinterpret_cast<const char*>(m_doc.m_UTF8Sequence.data()), i + 1);
    m_phase = Phase::NONE;
}

/**
 * Phase_COMPRESS - MCCP v1 (IAC SB COMPRESS ...)
 */
void TelnetParser::Phase_COMPRESS(unsigned char c)
{
    if (c == WILL) {
        m_phase = Phase::HAVE_COMPRESS_WILL;
    } else {
        m_phase = Phase::NONE; // Error
    }
}

/**
 * Phase_COMPRESS_WILL - MCCP v1 activation (IAC SB COMPRESS WILL SE)
 */
void TelnetParser::Phase_COMPRESS_WILL(unsigned char c)
{
    if (c == SE) {
        // MCCP v1 starts here
        m_iMCCP_type = 1;

        // Initialize zlib if not already done
        if (!m_zlibStream.initialized && !m_bCompress) {
            initZlibStream(m_zlibStream);
        }

        // Allocate BOTH buffers if needed
        if (m_CompressOutput.empty()) {
            m_CompressOutput.assign(COMPRESS_BUFFER_LENGTH, 0);
        }

        if (m_CompressInput.empty()) {
            m_CompressInput.assign(COMPRESS_BUFFER_LENGTH, 0);
        }

        // Check if we can compress (need BOTH buffers)
        if (!(m_zlibStream.initialized && !m_CompressOutput.empty() && !m_CompressInput.empty())) {
            qCDebug(lcWorld) << "Cannot process compressed output (MCCP v1) - closing connection";
            m_doc.OnConnectionDisconnect();
            m_phase = Phase::NONE;
            return;
        }

        // Reset the decompression stream
        int izError = inflateReset(&m_zlibStream.stream);
        if (izError == Z_OK) {
            m_bCompress = true;
            qCDebug(lcWorld) << "MCCP v1 compression enabled";
        } else {
            qCDebug(lcWorld) << "Could not reset zlib for MCCP v1, error:" << izError;
            if (m_zlibStream.stream.msg) {
                qCDebug(lcWorld) << "  zlib message:" << m_zlibStream.stream.msg;
            }
            m_doc.OnConnectionDisconnect();
        }
    }
    m_phase = Phase::NONE;
}

// ========== Support Methods ==========

/**
 * Handle_Telnet_Request - Query plugins for unknown telnet options
 */
bool TelnetParser::Handle_Telnet_Request(int iNumber, const QString& sType)
{
    // For SENT_* notifications, call ALL plugins (don't stop on first true)
    bool bStopOnTrue = !sType.startsWith("SENT_");
    return m_doc.SendToAllPluginCallbacks(ON_PLUGIN_TELNET_REQUEST, iNumber, sType, bStopOnTrue);
}

/**
 * Handle_IAC_GA - Handle Go Ahead or End of Record
 */
void TelnetParser::Handle_IAC_GA()
{
    m_doc.SendToAllPluginCallbacks(ON_PLUGIN_IAC_GA);
    qCDebug(lcWorld) << "IAC GA/EOR received";
}

// ========== Protocol-Specific Handlers (Subnegotiation) ==========

/**
 * Handle_TELOPT_COMPRESS2 - MCCP v2 activation
 */
void TelnetParser::Handle_TELOPT_COMPRESS2()
{
    m_iMCCP_type = 2;

    // Initialize zlib if not already done
    if (!m_zlibStream.initialized && !m_bCompress) {
        initZlibStream(m_zlibStream);
    }

    // Allocate BOTH buffers if needed
    if (m_CompressOutput.empty()) {
        m_CompressOutput.assign(COMPRESS_BUFFER_LENGTH, 0);
    }

    if (m_CompressInput.empty()) {
        m_CompressInput.assign(COMPRESS_BUFFER_LENGTH, 0);
    }

    // Check if we can compress (need BOTH buffers)
    if (!(m_zlibStream.initialized && !m_CompressOutput.empty() && !m_CompressInput.empty())) {
        qCDebug(lcWorld) << "Cannot process compressed output (MCCP v2) - closing connection";
        m_doc.OnConnectionDisconnect();
        return;
    }

    // Reset the decompression stream
    int izError = inflateReset(&m_zlibStream.stream);
    if (izError == Z_OK) {
        m_bCompress = true;
        qCDebug(lcWorld) << "MCCP v2 compression enabled";
        return;
    }

    // Error resetting zlib
    qCDebug(lcWorld) << "Could not reset zlib, error:" << izError;
    if (m_zlibStream.stream.msg) {
        qCDebug(lcWorld) << "  zlib message:" << m_zlibStream.stream.msg;
    }
    m_doc.OnConnectionDisconnect();
}

/**
 * Handle_TELOPT_MXP - MXP on command
 */
void TelnetParser::Handle_TELOPT_MXP()
{
    if (m_doc.m_iUseMXP == MXPMode::eMXP_On) {
        m_doc.MXP_On();
    }
}

/**
 * Handle_TELOPT_CHARSET - Character set negotiation
 */
void TelnetParser::Handle_TELOPT_CHARSET()
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
    if (m_doc.m_display.utf8) {
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

        m_doc.SendPacket({sResponse, static_cast<std::size_t>(iLength)});
    } else {
        // Build: IAC SB CHARSET REJECTED IAC SE
        unsigned char p[] = {IAC, SB, TELOPT_CHARSET, 3, IAC, SE}; // 3 = REJECTED
        m_doc.SendPacket(p);
    }
}

/**
 * Handle_TELOPT_TERMINAL_TYPE - Terminal type / MTTS
 */
void TelnetParser::Handle_TELOPT_TERMINAL_TYPE()
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
            strTemp = m_doc.m_strTerminalIdentification.left(20); // "ANSI" or custom
            m_ttype_sequence++;
            break;

        case 1:
            strTemp = "ANSI";
            m_ttype_sequence++;
            break;

        case 2:
            if (m_doc.m_display.utf8) {
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

    m_doc.SendPacket({sResponse, static_cast<std::size_t>(iLength)});
}

/**
 * Handle_TELOPT_ZMP - ZMP (Zenith MUD Protocol) subnegotiation
 */
void TelnetParser::Handle_TELOPT_ZMP()
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
        m_doc.SendPacket({reinterpret_cast<const unsigned char*>(response.constData()),
                          static_cast<std::size_t>(response.size())});
    } else if (command == "zmp.check") {
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
            m_doc.SendPacket({reinterpret_cast<const unsigned char*>(response.constData()),
                              static_cast<std::size_t>(response.size())});
        }
    } else if (command == "zmp.ident") {
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
        m_doc.SendPacket({reinterpret_cast<const unsigned char*>(response.constData()),
                          static_cast<std::size_t>(response.size())});
    }

    // Send to Lua callback for custom handling
    QString callbackData = command;
    for (const QString& arg : args) {
        callbackData += " " + arg;
    }
    m_doc.SendToAllPluginCallbacks("OnPluginZMP", callbackData, false);
}

/**
 * Handle_TELOPT_ATCP - ATCP (Achaea Telnet Client Protocol) subnegotiation
 */
void TelnetParser::Handle_TELOPT_ATCP()
{
    if (!m_bATCP || m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

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
        QByteArray response;
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SB));
        response.append(static_cast<char>(TELOPT_ATCP));
        response.append("hello Mushkin 1.0");
        response.append(static_cast<char>(IAC));
        response.append(static_cast<char>(SE));
        m_doc.SendPacket({reinterpret_cast<const unsigned char*>(response.constData()),
                          static_cast<std::size_t>(response.size())});
    }

    m_doc.SendToAllPluginCallbacks("OnPluginATCP", data, false);
}

/**
 * Handle_TELOPT_MSP - MSP (MUD Sound Protocol) subnegotiation
 */
void TelnetParser::Handle_TELOPT_MSP()
{
    if (!m_bMSP || m_IAC_subnegotiation_data.isEmpty()) {
        return;
    }

    QString data = QString::fromUtf8(m_IAC_subnegotiation_data);

    QStringList parts = data.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        qWarning() << "MSP: Invalid data (need command and filename):" << data;
        return;
    }

    QString command = parts[0].toUpper();
    QString filename = parts[1];

    int volume = 100;
    int loops = 1;
    QString url;

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
    }

    double volumeApi = static_cast<double>(volume) - 100.0;
    bool loop = (loops < 0 || loops > 1);

    if (command == "SOUND") {
        qDebug() << "MSP SOUND:" << filename << "volume:" << volume << "loops:" << loops;
        m_doc.PlayMSPSound(filename, url, loop, volumeApi, 0);
    } else if (command == "MUSIC") {
        qDebug() << "MSP MUSIC:" << filename << "volume:" << volume << "loops:" << loops;
        bool musicLoop = (loops != 1);
        m_doc.PlayMSPSound(filename, url, musicLoop, volumeApi, 1);
    } else if (command == "STOP") {
        qDebug() << "MSP STOP";
        m_doc.StopSound(0);
    } else {
        qWarning() << "MSP: Unknown command:" << command;
    }

    m_doc.SendToAllPluginCallbacks("OnPluginMSP", data, false);
}

/**
 * sendWindowSizes - Send NAWS (Negotiate About Window Size) to server
 */
void TelnetParser::sendWindowSizes(int width)
{
    // Can't send if not connected or NAWS not negotiated
    if (m_doc.m_connectionManager->m_iConnectPhase != eConnectConnectedToMud ||
        !m_doc.m_connectionManager->m_pSocket || !m_bNAWS_wanted) {
        return;
    }

    quint16 height = static_cast<quint16>(width);

    // Build NAWS packet: IAC SB TELOPT_NAWS <width> <height> IAC SE
    unsigned char packet[20];
    int length = 0;

    packet[length++] = IAC;
    packet[length++] = SB;
    packet[length++] = TELOPT_NAWS;

    quint8 width_hi = (width >> 8) & 0xFF;
    quint8 width_lo = width & 0xFF;

    packet[length++] = width_hi;
    if (width_hi == IAC) {
        packet[length++] = IAC;
    }

    packet[length++] = width_lo;
    if (width_lo == IAC) {
        packet[length++] = IAC;
    }

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

    packet[length++] = IAC;
    packet[length++] = SE;

    m_doc.SendPacket({packet, static_cast<std::size_t>(length)});
    qCDebug(lcWorld) << "Sent NAWS:" << width << "x" << height;
}
