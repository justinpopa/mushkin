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
#include "world_error.h"
#include "world_socket.h" // For m_pSocket->send()
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringDecoder>
#include <expected>
#include <zlib.h>

// ========== xterm 256-Color Palette ==========
// Port from: MUSHclient.cpp (Generate256colours)
// This is the standard xterm 256-color palette in BGR format (Windows COLORREF: 0x00BBGGRR)
// BGR format is used for MUSHclient compatibility - plugins may use hardcoded color values
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
 * Routes each byte to the appropriate phase handler.
 * Telnet-specific phases delegate to m_telnetParser->Phase_*().
 * MXP phases are handled here (MXP stays on WorldDocument pending MXPEngine extraction).
 *
 * Port from: telnet_phases.cpp
 */
void WorldDocument::ProcessIncomingByte(unsigned char c)
{
    auto& tp = *m_telnetParser;
    Phase& phase = tp.m_phase;

    // Special case: UTF-8 multibyte start in normal mode
    if (m_bUTF_8 && phase == NONE && (c & 0x80)) {
        if ((c & 0xE0) == 0xC0) {
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 1;
            phase = HAVE_UTF8_CHARACTER;
            return;
        } else if ((c & 0xF0) == 0xE0) {
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 2;
            phase = HAVE_UTF8_CHARACTER;
            return;
        } else if ((c & 0xF8) == 0xF0) {
            m_UTF8Sequence[0] = c;
            m_UTF8Sequence[1] = 0;
            m_iUTF8BytesLeft = 3;
            phase = HAVE_UTF8_CHARACTER;
            return;
        }
        // Invalid UTF-8 start byte - fall through to process as normal
    }

    // Dispatch based on current phase
    switch (phase) {
        case NONE:
            if (c == IAC) {
                phase = HAVE_IAC;
            } else if (c == 0x1B) { // ESC
                phase = HAVE_ESC;
            } else if (c == '<' && m_mxpEngine->m_bMXP && (MXP_Open() || MXP_Secure())) {
                m_mxpEngine->m_strMXPstring.clear();
                phase = HAVE_MXP_ELEMENT;
            } else if (c == '&' && m_mxpEngine->m_bMXP && (MXP_Open() || MXP_Secure())) {
                m_mxpEngine->m_strMXPstring.clear();
                phase = HAVE_MXP_ENTITY;
            } else if (c == '\n') {
                StartNewLine(true, 0);
            } else if (c == '\r') {
                // Carriage return without LF - ignore
            } else if (c >= 32 || c == '\t') {
                AddToLine(c);
            }
            break;

        case HAVE_ESC:
            tp.Phase_ESC(c);
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
            tp.Phase_ANSI(c);
            break;

        case HAVE_IAC:
            tp.Phase_IAC(c);
            // Phase_IAC may zero-out c (returning new_c=0 signals no further processing)
            if (c != 0) {
                ProcessIncomingByte(c);
            }
            return;

        case HAVE_WILL:
            tp.Phase_WILL(c);
            break;

        case HAVE_WONT:
            tp.Phase_WONT(c);
            break;

        case HAVE_DO:
            tp.Phase_DO(c);
            break;

        case HAVE_DONT:
            tp.Phase_DONT(c);
            break;

        case HAVE_SB:
            tp.Phase_SB(c);
            break;

        case HAVE_SUBNEGOTIATION:
            tp.Phase_SUBNEGOTIATION(c);
            break;

        case HAVE_SUBNEGOTIATION_IAC:
            tp.Phase_SUBNEGOTIATION_IAC(c);
            break;

        case HAVE_UTF8_CHARACTER:
            tp.Phase_UTF8(c);
            break;

        case HAVE_COMPRESS:
            tp.Phase_COMPRESS(c);
            break;

        case HAVE_COMPRESS_WILL:
            tp.Phase_COMPRESS_WILL(c);
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
            qCDebug(lcWorld) << "Unknown telnet phase:" << phase;
            phase = NONE;
            break;
    }
}

// Phase_ANSI — moved to TelnetParser::Phase_ANSI (telnet_parser.cpp)

// Phase_IAC — moved to TelnetParser::Phase_IAC (telnet_parser.cpp)

// Send_IAC_DO/DONT/WILL/WONT — moved to TelnetParser (telnet_parser.cpp)

// Phase_WILL/WONT/DO/DONT/SB/SUBNEGOTIATION/SUBNEGOTIATION_IAC/UTF8/COMPRESS/COMPRESS_WILL
// — all moved to TelnetParser (telnet_parser.cpp)

// Handle_Telnet_Request, Handle_IAC_GA — moved to TelnetParser (telnet_parser.cpp)

// Handle_TELOPT_COMPRESS2/MXP/CHARSET/TERMINAL_TYPE/ZMP/ATCP/MSP
// — moved to TelnetParser (telnet_parser.cpp)

// ========== Support Methods ==========

/**
 * SendPacket - Send raw bytes through the socket
 *
 * Port from: doc.cpp (various locations)
 */
void WorldDocument::SendPacket(std::span<const unsigned char> data)
{
    if (!m_connectionManager->m_pSocket) {
        qCDebug(lcWorld) << "SendPacket: No socket available";
        return;
    }

    // Ignore send errors — callers don't check return value for protocol packets
    (void)m_connectionManager->m_pSocket->send(
        {reinterpret_cast<const char*>(data.data()), data.size()});
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
        QByteArrayView singleByte(reinterpret_cast<const char*>(&m_UTF8Sequence[i]), 1);
        QString unicode = localDecoder(singleByte);

        // Convert Unicode to UTF-8 and add to line
        QByteArray utf8 = unicode.toUtf8();
        AddToLine(utf8.constData(), utf8.length());

        m_cLastChar = m_UTF8Sequence[i];
    }

    m_telnetParser->m_phase = NONE;
}

/**
 * PlayMSPSound - Forward to SoundManager::playMSPSound()
 *
 * Implementation lives in sound_manager.cpp; this is a thin wrapper kept for
 * source compatibility with telnet_parser.cpp callers.
 */
void WorldDocument::PlayMSPSound(const QString& filename, const QString& url, bool loop,
                                 double volume, qint16 buffer)
{
    m_soundManager->playMSPSound(filename, url, loop, volume, buffer);
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
            m_telnetParser->m_phase = HAVE_FOREGROUND_256_START;
            return;
        case ANSI_BACK_256_COLOUR:
            m_telnetParser->m_phase = HAVE_BACKGROUND_256_START;
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


// MXP_On, MXP_Off, MXP_mode_change, Phase_MXP_* moved to mxp_engine.cpp.
// WorldDocument forwards these via inline wrappers in world_document.h.

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
    switch (m_telnetParser->m_phase) {
        case HAVE_FOREGROUND_256_START: // Got ESC[38;
            if (iCode == 5) {           // 8-bit color (256-color)
                m_code = 0;
                m_telnetParser->m_phase = HAVE_FOREGROUND_256_FINISH;
            } else if (iCode == 2) { // 24-bit RGB
                m_code = 0;
                m_telnetParser->m_phase = HAVE_FOREGROUND_24B_FINISH;
            } else {
                m_telnetParser->m_phase = NONE;
            }
            return;

        case HAVE_BACKGROUND_256_START: // Got ESC[48;
            if (iCode == 5) {           // 8-bit color (256-color)
                m_code = 0;
                m_telnetParser->m_phase = HAVE_BACKGROUND_256_FINISH;
            } else if (iCode == 2) { // 24-bit RGB
                m_code = 0;
                m_telnetParser->m_phase = HAVE_BACKGROUND_24B_FINISH;
            } else {
                m_telnetParser->m_phase = NONE;
            }
            return;

        default:
            // Other phases not handled by this function
            break;
    }

    // Validate color index (0-255)
    if (iCode < 0 || iCode > 255) {
        m_telnetParser->m_phase = DOING_CODE;
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
    bool iForegroundMode = (m_telnetParser->m_phase == HAVE_FOREGROUND_24B_FINISH ||
                            m_telnetParser->m_phase == HAVE_FOREGROUND_24BR_FINISH ||
                            m_telnetParser->m_phase == HAVE_FOREGROUND_24BG_FINISH ||
                            m_telnetParser->m_phase == HAVE_FOREGROUND_24BB_FINISH);

    // Build RGB color for 24-bit mode
    QRgb workingColor;
    if (iForegroundMode && (iFlags & INVERSE) || (!iForegroundMode && !(iFlags & INVERSE)))
        workingColor = iBackColour;
    else
        workingColor = iForeColour;

    // Handle 24-bit true color R,G,B components
    switch (m_telnetParser->m_phase) {
        case HAVE_FOREGROUND_24B_FINISH: // Got ESC[38;2; - waiting for R
            iFlags &= ~COLOURTYPE;
            iFlags |= COLOUR_RGB;
            workingColor = qRgb(iCode, 0, 0); // Set Red
            m_telnetParser->m_phase = HAVE_FOREGROUND_24BR_FINISH;
            break;

        case HAVE_FOREGROUND_24BR_FINISH:                      // Got ESC[38;2;<r>; - waiting for G
            workingColor = qRgb(qRed(workingColor), iCode, 0); // Set Green
            m_telnetParser->m_phase = HAVE_FOREGROUND_24BG_FINISH;
            break;

        case HAVE_FOREGROUND_24BG_FINISH: // Got ESC[38;2;<r>;<g>; - waiting for B
            workingColor = qRgb(qRed(workingColor), qGreen(workingColor), iCode); // Set Blue
            m_telnetParser->m_phase = HAVE_FOREGROUND_24BB_FINISH;
            break;

        case HAVE_BACKGROUND_24B_FINISH: // Got ESC[48;2; - waiting for R
            iFlags &= ~COLOURTYPE;
            iFlags |= COLOUR_RGB;
            workingColor = qRgb(iCode, 0, 0); // Set Red
            m_telnetParser->m_phase = HAVE_BACKGROUND_24BR_FINISH;
            break;

        case HAVE_BACKGROUND_24BR_FINISH:                      // Got ESC[48;2;<r>; - waiting for G
            workingColor = qRgb(qRed(workingColor), iCode, 0); // Set Green
            m_telnetParser->m_phase = HAVE_BACKGROUND_24BG_FINISH;
            break;

        case HAVE_BACKGROUND_24BG_FINISH: // Got ESC[48;2;<r>;<g>; - waiting for B
            workingColor = qRgb(qRed(workingColor), qGreen(workingColor), iCode); // Set Blue
            m_telnetParser->m_phase = HAVE_BACKGROUND_24BB_FINISH;
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

    // Handle 256-color palette lookups - always convert to RGB
    if (m_telnetParser->m_phase == HAVE_FOREGROUND_256_FINISH ||
        m_telnetParser->m_phase == HAVE_BACKGROUND_256_FINISH) {
        // Set COLOUR_RGB mode for 256-color sequences
        iFlags &= ~COLOURTYPE;
        iFlags |= COLOUR_RGB;

        // Look up the RGB value from the xterm 256-color palette
        switch (m_telnetParser->m_phase) {
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
    }

    // Check if we've completed a color sequence
    if (m_telnetParser->m_phase == HAVE_BACKGROUND_256_FINISH ||
        m_telnetParser->m_phase == HAVE_FOREGROUND_256_FINISH ||
        m_telnetParser->m_phase == HAVE_FOREGROUND_24BB_FINISH ||
        m_telnetParser->m_phase == HAVE_BACKGROUND_24BB_FINISH) {
        m_telnetParser->m_phase = DOING_CODE; // Ready for more ANSI codes
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

    qCDebug(lcWorld) << "256/24bit code" << iCode << "phase" << m_telnetParser->m_phase
                     << "- flags:" << Qt::hex << iFlags << "fore:" << iForeColour
                     << "back:" << iBackColour;
}


// InitZlib — moved to TelnetParser::initZlib() (telnet_parser.cpp)
// SendWindowSizes — moved to TelnetParser::sendWindowSizes(int width) (telnet_parser.cpp)
// MXP_On, MXP_Off, MXP_mode_change, Phase_MXP_* — moved to mxp_engine.cpp
