// ssh_server_session.cpp
// SSH transport for Remote Access Server
//
// Part of Remote Access Server feature

#include "ssh_server_session.h"

#include <QFile>
#include <QTextStream>

#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <netdb.h>
#    include <sys/socket.h>
#endif

#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/server.h>

SshServerSession::SshServerSession(ssh_session session, const QString& password,
                                   const QString& authorizedKeysFile, QObject* parent)
    : IRemoteTransport(parent), m_session(session), m_password(password),
      m_authorizedKeysFile(authorizedKeysFile)
{
    // Non-blocking mode for Qt integration.
    ssh_set_blocking(m_session, 0);

    // Set allowed auth methods.
    int methods = SSH_AUTH_METHOD_PASSWORD;
    if (!m_authorizedKeysFile.isEmpty() && QFile::exists(m_authorizedKeysFile)) {
        methods |= SSH_AUTH_METHOD_PUBLICKEY;
    }
    ssh_set_auth_methods(m_session, methods);

    // Message callback for auth + channel handshake.
    ssh_set_message_callback(m_session, &SshServerSession::cbMessage, this);

    // Create event loop and add session.
    m_event = ssh_event_new();
    ssh_event_add_session(m_event, m_session);

    // Wire QSocketNotifier on the session fd.
    auto fd = ssh_get_fd(m_session);
    m_notifier = new QSocketNotifier(static_cast<qintptr>(fd), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &SshServerSession::onSocketActivity);

    // Attempt the first step of key exchange immediately.
    advanceHandshake();
}

SshServerSession::~SshServerSession()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
    }
    if (m_channel) {
        ssh_channel_send_eof(m_channel);
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    if (m_event) {
        if (m_session) {
            ssh_event_remove_session(m_event, m_session);
        }
        ssh_event_free(m_event);
        m_event = nullptr;
    }
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
}

void SshServerSession::onSocketActivity()
{
    if (m_phase == Phase::Closing) {
        return;
    }
    ssh_event_dopoll(m_event, 0);
    advanceHandshake();
}

void SshServerSession::advanceHandshake()
{
    if (m_phase != Phase::KeyExchange) {
        return;
    }
    int rc = ssh_handle_key_exchange(m_session);
    if (rc == SSH_OK) {
        m_phase = Phase::Authentication;
        // Messages will be dispatched via cbMessage during subsequent dopoll calls.
    } else if (rc == SSH_ERROR) {
        m_phase = Phase::Closing;
        emit closed();
    }
    // SSH_AGAIN: wait for more data — notifier will fire again.
}

// static
int SshServerSession::cbMessage(ssh_session s, ssh_message msg, void* userdata)
{
    Q_UNUSED(s);
    return static_cast<SshServerSession*>(userdata)->handleMessage(msg);
}

int SshServerSession::handleMessage(ssh_message msg)
{
    switch (ssh_message_type(msg)) {
        case SSH_REQUEST_AUTH:
            return handleAuth(msg);
        case SSH_REQUEST_CHANNEL_OPEN:
            return handleChannelOpen(msg);
        case SSH_REQUEST_CHANNEL:
            return handleChannelRequest(msg);
        default:
            ssh_message_reply_default(msg);
            return 0;
    }
}

int SshServerSession::handleAuth(ssh_message msg)
{
    if (m_phase != Phase::Authentication) {
        ssh_message_reply_default(msg);
        return 0;
    }

    int subtype = ssh_message_subtype(msg);

    if (subtype == SSH_AUTH_METHOD_PASSWORD) {
        const char* pw = ssh_message_auth_password(msg);
        if (pw && checkPassword(pw)) {
            ssh_message_auth_reply_success(msg, 0);
            m_phase = Phase::WaitingChannel;
            return 0;
        }
    } else if (subtype == SSH_AUTH_METHOD_PUBLICKEY) {
        ssh_key key = ssh_message_auth_pubkey(msg);
        if (key && checkPublicKey(key)) {
            auto state = ssh_message_auth_publickey_state(msg);
            if (state == SSH_PUBLICKEY_STATE_NONE) {
                // Client is probing: "do you accept this key type?"
                ssh_message_auth_reply_pk_ok_simple(msg);
                return 0;
            } else if (state == SSH_PUBLICKEY_STATE_VALID) {
                // Client proved possession of the private key.
                ssh_message_auth_reply_success(msg, 0);
                m_phase = Phase::WaitingChannel;
                return 0;
            }
        }
    }

    ssh_message_reply_default(msg);
    return 0;
}

int SshServerSession::handleChannelOpen(ssh_message msg)
{
    if (m_phase != Phase::WaitingChannel) {
        ssh_message_reply_default(msg);
        return 0;
    }

    if (ssh_message_subtype(msg) == SSH_CHANNEL_SESSION) {
        m_channel = ssh_message_channel_request_open_reply_accept(msg);
        if (m_channel) {
            // Set up channel callbacks for data flow.
            m_channelCb = std::make_unique<ssh_channel_callbacks_struct>();
            *m_channelCb = {};
            m_channelCb->size = sizeof(ssh_channel_callbacks_struct);
            m_channelCb->userdata = this;
            m_channelCb->channel_data_function = &SshServerSession::cbChannelData;
            m_channelCb->channel_eof_function = &SshServerSession::cbChannelEof;
            m_channelCb->channel_close_function = &SshServerSession::cbChannelClose;
            m_channelCb->channel_pty_request_function = &SshServerSession::cbChannelPtyRequest;
            m_channelCb->channel_pty_window_change_function =
                &SshServerSession::cbChannelPtyWindowChange;
            m_channelCb->channel_shell_request_function = &SshServerSession::cbChannelShellRequest;
            ssh_callbacks_init(m_channelCb.get());
            ssh_set_channel_callbacks(m_channel, m_channelCb.get());

            m_phase = Phase::WaitingShell;
            return 0;
        }
    }

    ssh_message_reply_default(msg);
    return 0;
}

int SshServerSession::handleChannelRequest(ssh_message msg)
{
    int subtype = ssh_message_subtype(msg);

    if (subtype == SSH_CHANNEL_REQUEST_PTY) {
        m_termWidth = ssh_message_channel_request_pty_width(msg);
        m_termHeight = ssh_message_channel_request_pty_height(msg);
        ssh_message_channel_request_reply_success(msg);
        return 0;
    }

    if (subtype == SSH_CHANNEL_REQUEST_SHELL) {
        ssh_message_channel_request_reply_success(msg);
        m_phase = Phase::Established;
        // QueuedConnection avoids re-entrancy from inside the message callback.
        QMetaObject::invokeMethod(this, [this]() { emit ready(); }, Qt::QueuedConnection);
        return 0;
    }

    if (subtype == SSH_CHANNEL_REQUEST_ENV) {
        // Accept environment variables silently.
        ssh_message_channel_request_reply_success(msg);
        return 0;
    }

    ssh_message_reply_default(msg);
    return 0;
}

// static
int SshServerSession::cbChannelData(ssh_session s, ssh_channel ch, void* data, uint32_t len,
                                    int is_stderr, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);
    Q_UNUSED(is_stderr);

    auto* self = static_cast<SshServerSession*>(userdata);
    if (self->m_phase != Phase::Established) {
        return 0;
    }

    // DoS prevention: cap the receive buffer.
    constexpr int kMaxBuffer = 65536;
    if (self->m_cleanBuffer.size() + static_cast<int>(len) > kMaxBuffer) {
        return 0;
    }

    self->m_cleanBuffer.append(static_cast<const char*>(data), static_cast<qsizetype>(len));
    QMetaObject::invokeMethod(self, [self]() { emit self->dataAvailable(); }, Qt::QueuedConnection);
    return static_cast<int>(len);
}

// static
void SshServerSession::cbChannelEof(ssh_session s, ssh_channel ch, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);

    auto* self = static_cast<SshServerSession*>(userdata);
    self->m_phase = Phase::Closing;
    QMetaObject::invokeMethod(self, [self]() { emit self->closed(); }, Qt::QueuedConnection);
}

// static
void SshServerSession::cbChannelClose(ssh_session s, ssh_channel ch, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);

    auto* self = static_cast<SshServerSession*>(userdata);
    if (self->m_phase != Phase::Closing) {
        self->m_phase = Phase::Closing;
        QMetaObject::invokeMethod(self, [self]() { emit self->closed(); }, Qt::QueuedConnection);
    }
}

// static
int SshServerSession::cbChannelPtyRequest(ssh_session s, ssh_channel ch, const char* term, int w,
                                          int h, int pw, int ph, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);
    Q_UNUSED(term);
    Q_UNUSED(pw);
    Q_UNUSED(ph);

    auto* self = static_cast<SshServerSession*>(userdata);
    self->m_termWidth = w;
    self->m_termHeight = h;
    return 0; // success
}

// static
int SshServerSession::cbChannelPtyWindowChange(ssh_session s, ssh_channel ch, int w, int h, int pw,
                                               int ph, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);
    Q_UNUSED(pw);
    Q_UNUSED(ph);

    auto* self = static_cast<SshServerSession*>(userdata);
    self->m_termWidth = w;
    self->m_termHeight = h;
    QMetaObject::invokeMethod(
        self, [self, w, h]() { emit self->terminalSizeChanged(w, h); }, Qt::QueuedConnection);
    return 0;
}

// static
int SshServerSession::cbChannelShellRequest(ssh_session s, ssh_channel ch, void* userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(ch);

    auto* self = static_cast<SshServerSession*>(userdata);
    // This fires when the shell request arrives via the channel callbacks path
    // (after channel is already open). Only act if we are still waiting for the
    // shell — the message-callback path may have already advanced the phase.
    if (self->m_phase == Phase::WaitingShell) {
        self->m_phase = Phase::Established;
        QMetaObject::invokeMethod(self, [self]() { emit self->ready(); }, Qt::QueuedConnection);
    }
    return 0;
}

// IRemoteTransport implementations

QByteArray SshServerSession::processIncoming()
{
    QByteArray result;
    result.swap(m_cleanBuffer);
    return result;
}

QByteArray SshServerSession::escapeOutgoing(const QByteArray& data)
{
    // SSH handles framing; no application-level escaping needed.
    return data;
}

void SshServerSession::writeRaw(const QByteArray& data)
{
    if (m_channel && m_phase == Phase::Established) {
        ssh_channel_write(m_channel, data.constData(), static_cast<uint32_t>(data.size()));
    }
}

void SshServerSession::close()
{
    if (m_phase == Phase::Closing) {
        return;
    }
    m_phase = Phase::Closing;
    if (m_notifier) {
        m_notifier->setEnabled(false);
    }
    if (m_channel) {
        ssh_channel_send_eof(m_channel);
        ssh_channel_close(m_channel);
    }
    emit closed();
}

QString SshServerSession::peerAddress() const
{
    if (!m_session) {
        return {};
    }
    auto fd = static_cast<int>(ssh_get_fd(m_session));
    struct sockaddr_storage addr{};
    socklen_t addrlen = sizeof(addr);
    if (getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) != 0) {
        return QStringLiteral("unknown");
    }
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    if (getnameinfo(reinterpret_cast<struct sockaddr*>(&addr), addrlen, host, sizeof(host), port,
                    sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        return QStringLiteral("unknown");
    }
    return QString("%1:%2").arg(host).arg(port);
}

int SshServerSession::terminalWidth() const
{
    return m_termWidth;
}

int SshServerSession::terminalHeight() const
{
    return m_termHeight;
}

bool SshServerSession::checkPassword(const char* password) const
{
    return m_password == QString::fromUtf8(password);
}

bool SshServerSession::checkPublicKey(ssh_key key) const
{
    auto keys = loadAuthorizedKeys();
    bool found = false;
    for (auto* authKey : keys) {
        if (!found && ssh_key_cmp(key, authKey, SSH_KEY_CMP_PUBLIC) == 0) {
            found = true;
        }
        ssh_key_free(authKey);
    }
    return found;
}

std::vector<ssh_key> SshServerSession::loadAuthorizedKeys() const
{
    std::vector<ssh_key> keys;
    if (m_authorizedKeysFile.isEmpty()) {
        return keys;
    }

    QFile file(m_authorizedKeysFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return keys;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Format: key-type base64-key [comment]
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            continue;
        }

        enum ssh_keytypes_e keyType = SSH_KEYTYPE_UNKNOWN;
        if (parts[0] == "ssh-ed25519") {
            keyType = SSH_KEYTYPE_ED25519;
        } else if (parts[0] == "ssh-rsa") {
            keyType = SSH_KEYTYPE_RSA;
        } else if (parts[0] == "ecdsa-sha2-nistp256") {
            keyType = SSH_KEYTYPE_ECDSA_P256;
        } else if (parts[0] == "ecdsa-sha2-nistp384") {
            keyType = SSH_KEYTYPE_ECDSA_P384;
        } else if (parts[0] == "ecdsa-sha2-nistp521") {
            keyType = SSH_KEYTYPE_ECDSA_P521;
        } else {
            continue; // unknown key type
        }

        ssh_key pubkey = nullptr;
        if (ssh_pki_import_pubkey_base64(parts[1].toUtf8().constData(), keyType, &pubkey) ==
            SSH_OK) {
            keys.push_back(pubkey);
        }
    }

    return keys;
}
