// telnet_server_session.h
// Server-side telnet protocol handling for Remote Access Server
//
// Part of Remote Access Server feature

#ifndef TELNET_SERVER_SESSION_H
#define TELNET_SERVER_SESSION_H

#include <QByteArray>
#include <QObject>

class QTcpSocket;

/** Server-side telnet protocol handler for negotiation and IAC processing. */
class TelnetServerSession : public QObject {
    Q_OBJECT

  public:
    explicit TelnetServerSession(QTcpSocket* socket, QObject* parent = nullptr);

    void initiateNegotiation();
    bool isNegotiationComplete() const;
    QByteArray processIncoming(const QByteArray& data);
    static QByteArray escapeOutgoing(const QByteArray& data);
    int clientWidth() const;
    int clientHeight() const;

  signals:
    void windowSizeChanged(int width, int height);
    void negotiationComplete();

  private:
    void sendRaw(const QByteArray& data);
    void sendCommand(unsigned char command, unsigned char option);
    void handleCommand(unsigned char command, unsigned char option);
    void handleSubnegotiation(unsigned char option, const QByteArray& data);

    enum class State {
        Normal,
        HaveIAC,
        HaveWILL,
        HaveWONT,
        HaveDO,
        HaveDONT,
        HaveSB,
        InSB,
        InSB_IAC
    };

    QTcpSocket* m_pSocket;
    int m_clientWidth;
    int m_clientHeight;
    bool m_negotiationComplete;
    bool m_echoNegotiated;
    bool m_sgaNegotiated;
    bool m_nawsNegotiated;
    State m_state;
    unsigned char m_subnegOption;
    QByteArray m_subnegBuffer;
};

#endif // TELNET_SERVER_SESSION_H
