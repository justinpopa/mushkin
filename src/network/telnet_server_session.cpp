// telnet_server_session.cpp
// Server-side telnet protocol handling for Remote Access Server
//
// Part of Remote Access Server feature

#include "telnet_server_session.h"
#include "../world/world_document.h" // For telnet constants
#include <QTcpSocket>

TelnetServerSession::TelnetServerSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_pSocket(socket), m_clientWidth(80), m_clientHeight(24),
      m_negotiationComplete(false), m_echoNegotiated(false), m_sgaNegotiated(false),
      m_nawsNegotiated(false), m_state(State::Normal), m_subnegOption(0)
{
}

void TelnetServerSession::initiateNegotiation()
{
    // Server-side negotiation:
    // WILL ECHO - We will handle echoing (client should not echo locally)
    // WILL SGA  - We will suppress go-ahead (modern line mode)
    // DO NAWS   - Please send us your window size

    sendCommand(WILL, TELOPT_ECHO);
    sendCommand(WILL, TELOPT_SGA);
    sendCommand(DO, TELOPT_NAWS);
}

bool TelnetServerSession::isNegotiationComplete() const
{
    return m_negotiationComplete;
}

int TelnetServerSession::clientWidth() const
{
    return m_clientWidth;
}

int TelnetServerSession::clientHeight() const
{
    return m_clientHeight;
}

void TelnetServerSession::sendRaw(const QByteArray& data)
{
    if (m_pSocket && m_pSocket->isOpen()) {
        m_pSocket->write(data);
    }
}

void TelnetServerSession::sendCommand(unsigned char command, unsigned char option)
{
    QByteArray cmd;
    cmd.append(static_cast<char>(IAC));
    cmd.append(static_cast<char>(command));
    cmd.append(static_cast<char>(option));
    sendRaw(cmd);
}

QByteArray TelnetServerSession::escapeOutgoing(const QByteArray& data)
{
    QByteArray result;
    result.reserve(data.size() * 2); // Worst case: all IAC bytes

    for (int i = 0; i < data.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        result.append(static_cast<char>(c));
        if (c == IAC) {
            // Double the IAC byte
            result.append(static_cast<char>(IAC));
        }
    }

    return result;
}

QByteArray TelnetServerSession::processIncoming(const QByteArray& data)
{
    QByteArray clean;
    clean.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);

        switch (m_state) {
            case State::Normal:
                if (c == IAC) {
                    m_state = State::HaveIAC;
                } else {
                    clean.append(static_cast<char>(c));
                }
                break;

            case State::HaveIAC:
                switch (c) {
                    case IAC:
                        // Escaped IAC - literal 0xFF
                        clean.append(static_cast<char>(c));
                        m_state = State::Normal;
                        break;
                    case WILL:
                        m_state = State::HaveWILL;
                        break;
                    case WONT:
                        m_state = State::HaveWONT;
                        break;
                    case DO:
                        m_state = State::HaveDO;
                        break;
                    case DONT:
                        m_state = State::HaveDONT;
                        break;
                    case SB:
                        m_state = State::HaveSB;
                        break;
                    case SE:
                        // SE without SB - ignore
                        m_state = State::Normal;
                        break;
                    case NOP:
                    case GO_AHEAD:
                    case BREAK:
                    case INTERRUPT_PROCESS:
                    case ABORT_OUTPUT:
                    case ARE_YOU_THERE:
                    case ERASE_CHARACTER:
                    case ERASE_LINE:
                    case DATA_MARK:
                        // Single-byte commands - ignore
                        m_state = State::Normal;
                        break;
                    default:
                        // Unknown command - ignore
                        m_state = State::Normal;
                        break;
                }
                break;

            case State::HaveWILL:
                handleCommand(WILL, c);
                m_state = State::Normal;
                break;

            case State::HaveWONT:
                handleCommand(WONT, c);
                m_state = State::Normal;
                break;

            case State::HaveDO:
                handleCommand(DO, c);
                m_state = State::Normal;
                break;

            case State::HaveDONT:
                handleCommand(DONT, c);
                m_state = State::Normal;
                break;

            case State::HaveSB:
                m_subnegOption = c;
                m_subnegBuffer.clear();
                m_state = State::InSB;
                break;

            case State::InSB:
                if (c == IAC) {
                    m_state = State::InSB_IAC;
                } else {
                    m_subnegBuffer.append(static_cast<char>(c));
                }
                break;

            case State::InSB_IAC:
                if (c == IAC) {
                    // Escaped IAC in subnegotiation
                    m_subnegBuffer.append(static_cast<char>(IAC));
                    m_state = State::InSB;
                } else if (c == SE) {
                    // End of subnegotiation
                    handleSubnegotiation(m_subnegOption, m_subnegBuffer);
                    m_state = State::Normal;
                } else {
                    // Malformed - treat as end of subnegotiation
                    handleSubnegotiation(m_subnegOption, m_subnegBuffer);
                    m_state = State::Normal;
                }
                break;
        }
    }

    return clean;
}

void TelnetServerSession::handleCommand(unsigned char command, unsigned char option)
{
    switch (command) {
        case DO:
            // Client says DO (agrees to our WILL or requests we do something)
            switch (option) {
                case TELOPT_ECHO:
                    m_echoNegotiated = true;
                    break;
                case TELOPT_SGA:
                    m_sgaNegotiated = true;
                    break;
                default:
                    // Client wants us to do something we didn't offer - refuse
                    sendCommand(WONT, option);
                    break;
            }
            break;

        case DONT:
            // Client says DONT (refuses our WILL)
            switch (option) {
                case TELOPT_ECHO:
                    m_echoNegotiated = true; // Negotiation complete, even if refused
                    break;
                case TELOPT_SGA:
                    m_sgaNegotiated = true;
                    break;
                default:
                    break;
            }
            break;

        case WILL:
            // Client says WILL (agrees to our DO or offers to do something)
            switch (option) {
                case TELOPT_NAWS:
                    m_nawsNegotiated = true;
                    // Client will send window size via subnegotiation
                    break;
                default:
                    // Client offers something we didn't ask for - refuse
                    sendCommand(DONT, option);
                    break;
            }
            break;

        case WONT:
            // Client says WONT (refuses our DO)
            switch (option) {
                case TELOPT_NAWS:
                    m_nawsNegotiated = true; // Negotiation complete, even if refused
                    break;
                default:
                    break;
            }
            break;
    }

    // Check if initial negotiation is complete
    if (!m_negotiationComplete && m_echoNegotiated && m_sgaNegotiated) {
        m_negotiationComplete = true;
        emit negotiationComplete();
    }
}

void TelnetServerSession::handleSubnegotiation(unsigned char option, const QByteArray& data)
{
    switch (option) {
        case TELOPT_NAWS:
            // Window size: 2 bytes width (MSB first), 2 bytes height (MSB first)
            if (data.size() >= 4) {
                unsigned char w1 = static_cast<unsigned char>(data[0]);
                unsigned char w2 = static_cast<unsigned char>(data[1]);
                unsigned char h1 = static_cast<unsigned char>(data[2]);
                unsigned char h2 = static_cast<unsigned char>(data[3]);

                int newWidth = (w1 << 8) | w2;
                int newHeight = (h1 << 8) | h2;

                // Sanity check
                constexpr int kMaxTerminalDimension = 10000;
                if (newWidth > 0 && newWidth < kMaxTerminalDimension && newHeight > 0 &&
                    newHeight < kMaxTerminalDimension) {
                    m_clientWidth = newWidth;
                    m_clientHeight = newHeight;
                    emit windowSizeChanged(m_clientWidth, m_clientHeight);
                }
            }
            break;

        default:
            // Unknown subnegotiation - ignore
            break;
    }
}
