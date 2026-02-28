#pragma once

// telnet_parser.h
// TelnetParser — Companion object that owns all telnet protocol state and methods.
// WorldDocument creates and owns this via std::unique_ptr<TelnetParser>.
//
// This file contains:
// - Telnet protocol constants (converted from #define to inline constexpr)
// - Phase enum (parser state machine, moved from WorldDocument)
// - ZlibStream RAII guard for MCCP decompression state
// - TelnetParser class declaration

#include <QByteArray>
#include <array>
#include <cstdint>
#include <expected>
#include <vector>
#include <zlib.h>

// ========== ZlibStream — RAII guard for z_stream lifecycle ==========
// Ensures inflateEnd() is always called when the stream goes out of scope,
// even if TelnetParser is partially destroyed or an exception occurs.
struct ZlibStream {
    z_stream stream{};
    bool initialized = false;

    ZlibStream() = default;
    ~ZlibStream()
    {
        cleanup();
    }

    // Non-copyable, non-movable (owns opaque zlib state)
    ZlibStream(const ZlibStream&) = delete;
    ZlibStream& operator=(const ZlibStream&) = delete;
    ZlibStream(ZlibStream&&) = delete;
    ZlibStream& operator=(ZlibStream&&) = delete;

    // Returns Z_OK on success, a zlib error code otherwise.
    // Cleans up any existing stream before re-initializing.
    int init()
    {
        if (initialized) {
            cleanup();
        }
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = 0;
        stream.next_in = Z_NULL;
        const int rc = inflateInit(&stream);
        if (rc == Z_OK) {
            initialized = true;
        }
        return rc;
    }

    void cleanup()
    {
        if (initialized) {
            inflateEnd(&stream);
            initialized = false;
        }
    }
};

class WorldDocument; // forward declaration
class WorldError;    // forward declaration

// ========== Telnet Protocol Constants (RFC 854) ==========
// Converted from #define to inline constexpr for C++26 compliance.

// Telnet commands
inline constexpr unsigned char TELNET_IAC = 255;               // 0xFF - Interpret As Command
inline constexpr unsigned char TELNET_DONT = 254;              // 0xFE - Don't use this option
inline constexpr unsigned char TELNET_DO = 253;                // 0xFD - Please use this option
inline constexpr unsigned char TELNET_WONT = 252;              // 0xFC - I won't use this option
inline constexpr unsigned char TELNET_WILL = 251;              // 0xFB - I will use this option
inline constexpr unsigned char TELNET_SB = 250;                // 0xFA - Subnegotiation begin
inline constexpr unsigned char TELNET_GO_AHEAD = 249;          // 0xF9 - Go Ahead
inline constexpr unsigned char TELNET_ERASE_LINE = 248;        // 0xF8 - Erase Line
inline constexpr unsigned char TELNET_ERASE_CHARACTER = 247;   // 0xF7 - Erase Character
inline constexpr unsigned char TELNET_ARE_YOU_THERE = 246;     // 0xF6 - Are You There
inline constexpr unsigned char TELNET_ABORT_OUTPUT = 245;      // 0xF5 - Abort Output
inline constexpr unsigned char TELNET_INTERRUPT_PROCESS = 244; // 0xF4 - Interrupt Process
inline constexpr unsigned char TELNET_BREAK = 243;             // 0xF3 - Break
inline constexpr unsigned char TELNET_DATA_MARK = 242;         // 0xF2 - Data Mark
inline constexpr unsigned char TELNET_NOP = 241;               // 0xF1 - No Operation
inline constexpr unsigned char TELNET_SE = 240;                // 0xF0 - Subnegotiation End
inline constexpr unsigned char TELNET_EOR = 239;               // 0xEF - End Of Record

// Telnet options (TELOPT)
inline constexpr unsigned char TELOPT_ECHO_OPT = 1;           // Echo
inline constexpr unsigned char TELOPT_SGA_OPT = 3;            // Suppress Go Ahead
inline constexpr unsigned char TELOPT_TERMINAL_TYPE_OPT = 24; // Terminal Type
inline constexpr unsigned char TELOPT_NAWS_OPT = 31;          // Negotiate About Window Size
inline constexpr unsigned char TELOPT_CHARSET_OPT = 42;       // Character Set
inline constexpr unsigned char TELOPT_COMPRESS_OPT = 85;      // MCCP v1
inline constexpr unsigned char TELOPT_COMPRESS2_OPT = 86;     // MCCP v2
inline constexpr unsigned char TELOPT_MSP_OPT = 90;           // MUD Sound Protocol
inline constexpr unsigned char TELOPT_MXP_OPT = 91;           // MUD eXtension Protocol
inline constexpr unsigned char TELOPT_ZMP_OPT = 93;           // Zenith MUD Protocol
inline constexpr unsigned char TELOPT_MUD_SPECIFIC_OPT = 102; // Aardwolf telnet option
inline constexpr unsigned char TELOPT_ATCP_OPT = 200;         // ATCP
inline constexpr unsigned char TELOPT_GMCP_OPT = 201;         // GMCP

// Telnet subnegotiation opcodes
inline constexpr unsigned char TELNET_WILL_END_OF_RECORD = 25; // Will send End Of Record

// MCCP constants
inline constexpr int TELNET_COMPRESS_BUFFER_LENGTH = 20000;

// ========== Phase Enum — Telnet/ANSI/MXP Parser State Machine ==========
// Moved from WorldDocument (was: enum Phase in world_document.h).
// The EXACT values are preserved to maintain binary compatibility with any
// serialized state and to make grep-based verification straightforward.
enum class Phase : int {
    NONE = 0,                // Normal text
    HAVE_ESC,                // Just received ESC (0x1B)
    DOING_CODE,              // Processing ANSI escape sequence ESC[...m
    HAVE_IAC,                // Just received IAC (0xFF)
    HAVE_WILL,               // Got IAC WILL
    HAVE_WONT,               // Got IAC WONT
    HAVE_DO,                 // Got IAC DO
    HAVE_DONT,               // Got IAC DONT
    HAVE_SB,                 // Got IAC SB (subnegotiation begin)
    HAVE_SUBNEGOTIATION,     // In subnegotiation, collecting data
    HAVE_SUBNEGOTIATION_IAC, // Got IAC inside subnegotiation
    HAVE_COMPRESS,           // Got IAC SB COMPRESS (MCCP v1)
    HAVE_COMPRESS_WILL,      // Got IAC SB COMPRESS WILL (MCCP v1)

    // 256-color support
    HAVE_FOREGROUND_256_START,
    HAVE_FOREGROUND_256_FINISH,
    HAVE_BACKGROUND_256_START,
    HAVE_BACKGROUND_256_FINISH,

    // 24-bit true color support
    HAVE_FOREGROUND_24B_FINISH,
    HAVE_FOREGROUND_24BR_FINISH,
    HAVE_FOREGROUND_24BG_FINISH,
    HAVE_FOREGROUND_24BB_FINISH,
    HAVE_BACKGROUND_24B_FINISH,
    HAVE_BACKGROUND_24BR_FINISH,
    HAVE_BACKGROUND_24BG_FINISH,
    HAVE_BACKGROUND_24BB_FINISH,

    // UTF-8 multibyte character handling
    HAVE_UTF8_CHARACTER,

    // MXP (MUD eXtension Protocol) modes — these phases stay on WorldDocument
    // but the enum values must remain here since TelnetParser owns m_phase.
    HAVE_MXP_ELEMENT,
    HAVE_MXP_COMMENT,
    HAVE_MXP_QUOTE,
    HAVE_MXP_ENTITY,

    HAVE_MXP_ROOM_NAME,
    HAVE_MXP_ROOM_DESCRIPTION,
    HAVE_MXP_ROOM_EXITS,
    HAVE_MXP_WELCOME,
};

// ========== TelnetParser Class ==========
class TelnetParser {
  public:
    explicit TelnetParser(WorldDocument& doc);
    ~TelnetParser();

    // Non-copyable, non-movable (owns zlib state)
    TelnetParser(const TelnetParser&) = delete;
    TelnetParser& operator=(const TelnetParser&) = delete;
    TelnetParser(TelnetParser&&) = delete;
    TelnetParser& operator=(TelnetParser&&) = delete;

    // ========== Phase access ==========
    // ProcessIncomingByte and MXP phase handlers on WorldDocument read/write m_phase directly
    // via the public member (same pattern as original — direct access through companion pointer).
    Phase m_phase = Phase::NONE;

    // ========== Parser state ==========
    qint32 m_ttype_sequence = 0; // MTTS terminal type sequence counter

    // ========== Subnegotiation ==========
    qint32 m_subnegotiation_type = 0;
    QByteArray m_IAC_subnegotiation_data;

    // ========== IAC tracking arrays ==========
    std::array<bool, 256> m_bClient_sent_IAC_DO{};
    std::array<bool, 256> m_bClient_sent_IAC_DONT{};
    std::array<bool, 256> m_bClient_sent_IAC_WILL{};
    std::array<bool, 256> m_bClient_sent_IAC_WONT{};
    std::array<bool, 256> m_bClient_got_IAC_DO{};
    std::array<bool, 256> m_bClient_got_IAC_DONT{};
    std::array<bool, 256> m_bClient_got_IAC_WILL{};
    std::array<bool, 256> m_bClient_got_IAC_WONT{};

    // ========== MCCP Compression ==========
    ZlibStream m_zlibStream; // RAII guard — owns the z_stream and its inflateEnd lifecycle
    bool m_bCompress = false;
    std::vector<unsigned char> m_CompressInput;
    std::vector<unsigned char> m_CompressOutput;
    qint64 m_nTotalUncompressed = 0;
    qint64 m_nTotalCompressed = 0;
    qint64 m_iCompressionTimeTaken = 0;
    qint32 m_iMCCP_type = 0; // 0=none, 1=v1, 2=v2
    bool m_bSupports_MCCP_2 = false;

    // ========== IAC counters ==========
    qint32 m_nCount_IAC_DO = 0;
    qint32 m_nCount_IAC_DONT = 0;
    qint32 m_nCount_IAC_WILL = 0;
    qint32 m_nCount_IAC_WONT = 0;
    qint32 m_nCount_IAC_SB = 0;

    // ========== Protocol activation flags ==========
    bool m_bNAWS_wanted = false;    // server wants NAWS messages
    bool m_bATCP = false;           // ATCP negotiated and active
    bool m_bZMP = false;            // ZMP negotiated and active
    bool m_bMSP = false;            // MSP negotiated and active
    bool m_bCHARSET_wanted = false; // server wants CHARSET messages
    bool m_bNoEcho = false;         // set if IAC WILL ECHO

    // ========== Reset ==========
    // Reset telnet state for reconnection (does NOT re-initialize zlib).
    void reset();

    // ========== Public API called from WorldDocument ==========
    void sendWindowSizes(int width);

    // ========== Phase handlers — called from WorldDocument::ProcessIncomingByte ==========
    void Phase_ESC(unsigned char c);
    void Phase_ANSI(unsigned char c);
    void Phase_IAC(unsigned char& c);
    void Phase_WILL(unsigned char c);
    void Phase_WONT(unsigned char c);
    void Phase_DO(unsigned char c);
    void Phase_DONT(unsigned char c);
    void Phase_SB(unsigned char c);
    void Phase_SUBNEGOTIATION(unsigned char c);
    void Phase_SUBNEGOTIATION_IAC(unsigned char c);
    void Phase_UTF8(unsigned char c);
    void Phase_COMPRESS(unsigned char c);
    void Phase_COMPRESS_WILL(unsigned char c);

    // ========== IAC negotiation (with loop prevention) ==========
    void Send_IAC_DO(unsigned char c);
    void Send_IAC_DONT(unsigned char c);
    void Send_IAC_WILL(unsigned char c);
    void Send_IAC_WONT(unsigned char c);

    // ========== Protocol-specific subnegotiation handlers ==========
    void Handle_TELOPT_COMPRESS2();
    void Handle_TELOPT_MXP();
    void Handle_TELOPT_CHARSET();
    void Handle_TELOPT_TERMINAL_TYPE();
    void Handle_TELOPT_ZMP();
    void Handle_TELOPT_ATCP();
    void Handle_TELOPT_MSP();

    // ========== Support methods ==========
    bool Handle_Telnet_Request(int iNumber, const QString& sType);
    void Handle_IAC_GA();

  private:
    WorldDocument& m_doc; // back-reference (non-owning)
};
