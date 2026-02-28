/**
 * connection_manager.cpp — ConnectionManager companion object
 *
 * Owns connection state, network statistics, timing, and the command queue
 * for a single WorldDocument.  Methods moved from world_document.cpp.
 */

#include "connection_manager.h"

#include "../automation/plugin.h" // ON_PLUGIN_CONNECT, ON_PLUGIN_DISCONNECT
#include "../network/remote_access_server.h"
#include "../text/line.h"  // Line (std::make_unique<Line>)
#include "../text/style.h" // Style
#include "logging.h"
#include "script_engine.h"
#include "world_document.h"
#include "world_socket.h"

#include <QDateTime>
#include <QList>

// ========== Construction / destruction ==========

ConnectionManager::ConnectionManager(WorldDocument& doc) : m_doc(doc)
{
}

ConnectionManager::~ConnectionManager() = default;

// ========== Connection lifecycle ==========

/**
 * connectToMud — initiate connection to the MUD server.
 *
 * Called by user action (Game → Connect) or auto-connect on world open.
 * Ported from WorldDocument::connectToMud() (world_document.cpp).
 */
void ConnectionManager::connectToMud()
{
    if (!m_pSocket) {
        qCDebug(lcWorld) << "connectToMud: No socket available!";
        return;
    }

    if (m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "connectToMud: Already connected";
        return;
    }

    if (m_doc.m_server.isEmpty()) {
        qCDebug(lcWorld) << "connectToMud: No server specified";
        return;
    }

    qCDebug(lcWorld) << "connectToMud: Connecting to" << m_doc.m_server << ":" << m_doc.m_port;
    m_iConnectPhase = CONNECT_CONNECTING_TO_MUD;
    m_pSocket->connectToHost(m_doc.m_server, m_doc.m_port);
}

/**
 * disconnectFromMud — disconnect from the MUD server.
 *
 * Called by user action (Game → Disconnect) or on world close.
 * Ported from WorldDocument::disconnectFromMud() (world_document.cpp).
 */
void ConnectionManager::disconnectFromMud()
{
    if (!m_pSocket) {
        return;
    }

    if (!m_pSocket->isConnected()) {
        qCDebug(lcWorld) << "disconnectFromMud: Not connected";
        return;
    }

    qCDebug(lcWorld) << "disconnectFromMud: Disconnecting from" << m_doc.m_server;
    m_iConnectPhase = CONNECT_DISCONNECTING;
    m_pSocket->disconnectFromHost();
}

/**
 * onConnect — called by WorldSocket when connection succeeds or fails.
 *
 * errorCode == 0 means success; any other value is a QAbstractSocket::SocketError.
 * Ported from WorldDocument::OnConnect() (world_document.cpp).
 */
void ConnectionManager::onConnect(int errorCode)
{
    if (errorCode == 0) {
        // Connection succeeded.
        qCDebug(lcWorld) << "ConnectionManager::onConnect() - connected successfully to"
                         << m_doc.m_server << ":" << m_doc.m_port;
        m_iConnectPhase = CONNECT_CONNECTED_TO_MUD;
        m_tConnectTime = QDateTime::currentDateTime();
        m_whenWorldStarted = m_tConnectTime;

        // Reset telnet parser state.
        m_doc.m_telnetParser->reset();
        m_doc.m_code = 0;
        m_doc.m_UTF8Sequence.fill(0);
        m_doc.m_iUTF8BytesLeft = 0;

        // Create initial line if needed.
        if (!m_doc.m_currentLine) {
            m_doc.m_currentLine =
                std::make_unique<Line>(1, m_doc.m_nWrapColumn, m_doc.m_iFlags, m_doc.m_iForeColour,
                                       m_doc.m_iBackColour, m_doc.m_bUTF_8);

            auto initial_style = std::make_unique<Style>();
            initial_style->iLength = 0;
            initial_style->iFlags = m_doc.m_iFlags;
            initial_style->iForeColour = m_doc.m_iForeColour;
            initial_style->iBackColour = m_doc.m_iBackColour;
            initial_style->pAction = nullptr;
            m_doc.m_currentLine->styleList.push_back(std::move(initial_style));
        }

        // Lua callback: OnWorldConnect.
        onWorldConnect();

        // Notify plugins.
        m_doc.SendToAllPluginCallbacks(ON_PLUGIN_CONNECT);

        // Start remote access server if configured.
        qCDebug(lcWorld) << "Remote access check: enabled=" << m_doc.m_bEnableRemoteAccess
                         << "port=" << m_doc.m_iRemotePort
                         << "password_set=" << !m_doc.m_strRemotePassword.isEmpty();
        if (m_doc.m_bEnableRemoteAccess && m_doc.m_iRemotePort > 0 &&
            !m_doc.m_strRemotePassword.isEmpty()) {
            if (!m_doc.m_pRemoteServer) {
                m_doc.m_pRemoteServer = std::make_unique<RemoteAccessServer>(&m_doc);
            }
            m_doc.m_pRemoteServer->setPassword(m_doc.m_strRemotePassword);
            m_doc.m_pRemoteServer->setScrollbackLines(m_doc.m_iRemoteScrollbackLines);
            m_doc.m_pRemoteServer->setMaxClients(m_doc.m_iRemoteMaxClients);
            if (auto result = m_doc.m_pRemoteServer->start(m_doc.m_iRemotePort); result) {
                qCDebug(lcWorld) << "Remote access server started on port" << m_doc.m_iRemotePort;
            } else {
                qCWarning(lcWorld)
                    << "Remote access server FAILED to start on port" << m_doc.m_iRemotePort;
            }
        } else {
            qCDebug(lcWorld) << "Remote access server not starting (conditions not met)";
        }

        emit m_doc.connectionStateChanged(true);
    } else {
        // Connection failed.
        qCDebug(lcWorld) << "ConnectionManager::onConnect() - connection failed with error:"
                         << errorCode;
        m_iConnectPhase = CONNECT_NOT_CONNECTED;
        emit m_doc.connectionStateChanged(false);
    }
}

/**
 * onConnectionDisconnect — called by WorldSocket when the socket closes.
 *
 * Ported from WorldDocument::OnConnectionDisconnect() (world_document.cpp).
 */
void ConnectionManager::onConnectionDisconnect()
{
    qCDebug(lcWorld) << "ConnectionManager::onConnectionDisconnect - disconnect detected";

    // Stop remote access server if running.
    if (m_doc.m_pRemoteServer && m_doc.m_pRemoteServer->isRunning()) {
        qCDebug(lcWorld) << "Stopping remote access server";
        m_doc.m_pRemoteServer->stop();
    }

    // Lua callback: OnWorldDisconnect.
    onWorldDisconnect();

    // Notify plugins.
    m_doc.SendToAllPluginCallbacks(ON_PLUGIN_DISCONNECT);

    // Update connection state.
    m_iConnectPhase = CONNECT_NOT_CONNECTED;
    emit m_doc.connectionStateChanged(false);
}

// ========== Timing ==========

/**
 * connectedTime — seconds connected, or -1 if not connected.
 *
 * Ported from WorldDocument::connectedTime() (world_document.cpp).
 */
qint64 ConnectionManager::connectedTime() const
{
    if (!m_tConnectTime.isValid()) {
        return -1;
    }
    return m_tConnectTime.secsTo(QDateTime::currentDateTime());
}

/**
 * resetConnectedTime — reset the connection timer to now.
 *
 * Called when user double-clicks the time indicator in the status bar.
 * Ported from WorldDocument::resetConnectedTime() (world_document.cpp).
 */
void ConnectionManager::resetConnectedTime()
{
    m_tConnectTime = QDateTime::currentDateTime();
}

// ========== Command queue ==========

/**
 * getCommandQueue — return a copy of the current command queue.
 *
 * Ported from WorldDocument::GetCommandQueue() (world_document.cpp).
 */
QStringList ConnectionManager::getCommandQueue() const
{
    return m_CommandQueue;
}

/**
 * queue — add a command to the command queue.
 *
 * Ported from WorldDocument::Queue() (world_document.cpp).
 *
 * @param message Command to queue.
 * @param echo    Whether to echo the command when sent.
 * @return std::expected<void, WorldError>: success, or a WorldError describing the failure.
 *         Lua boundary: WorldDocument::Queue() converts this back to a qint32 error code.
 */
std::expected<void, WorldError> ConnectionManager::queue(const QString& message, bool echo)
{
    if (m_iConnectPhase != CONNECT_CONNECTED_TO_MUD) {
        return std::unexpected(WorldError{WorldErrorType::NotConnected, "World is not connected"});
    }

    if (m_doc.m_bPluginProcessingSent) {
        return std::unexpected(
            WorldError{WorldErrorType::ItemInUse, "Plugin is processing sent text"});
    }

    // Delegate to WorldDocument::SendMsg (queue=true).
    m_doc.SendMsg(message, echo, true, false);
    return {};
}

/**
 * discardQueue — clear the command queue.
 *
 * Ported from WorldDocument::DiscardQueue() (world_document.cpp).
 *
 * @return Number of commands that were discarded.
 */
qint32 ConnectionManager::discardQueue()
{
    qint32 count = m_CommandQueue.size();
    m_CommandQueue.clear();
    return count;
}

// ========== Private — Lua callbacks ==========

/**
 * onWorldConnect — call the Lua OnWorldConnect handler if registered.
 *
 * Ported from WorldDocument::onWorldConnect() (world_document.cpp).
 */
void ConnectionManager::onWorldConnect()
{
    if (!m_doc.m_ScriptEngine) {
        return;
    }

    if (m_doc.m_dispidWorldConnect == 0) {
        m_doc.m_dispidWorldConnect = m_doc.m_ScriptEngine->getLuaDispid("OnWorldConnect");
    }

    if (m_doc.m_dispidWorldConnect == DISPID_UNKNOWN) {
        return;
    }

    QList<double> nparams;
    QList<QString> sparams;
    qint32 invocation_count = 0;
    bool result = false;

    bool error = m_doc.m_ScriptEngine->executeLua(
        m_doc.m_dispidWorldConnect, "OnWorldConnect", ActionSource::eWorldAction, "world",
        "world connect", nparams, sparams, invocation_count, &result);
    if (error) {
        qCDebug(lcWorld) << "Error calling OnWorldConnect callback";
    } else {
        qCDebug(lcWorld) << "OnWorldConnect callback executed successfully";
    }
}

/**
 * onWorldDisconnect — call the Lua OnWorldDisconnect handler if registered.
 *
 * Ported from WorldDocument::onWorldDisconnect() (world_document.cpp).
 */
void ConnectionManager::onWorldDisconnect()
{
    if (!m_doc.m_ScriptEngine) {
        return;
    }

    if (m_doc.m_dispidWorldDisconnect == 0) {
        m_doc.m_dispidWorldDisconnect = m_doc.m_ScriptEngine->getLuaDispid("OnWorldDisconnect");
    }

    if (m_doc.m_dispidWorldDisconnect == DISPID_UNKNOWN) {
        return;
    }

    QList<double> nparams;
    QList<QString> sparams;
    qint32 invocation_count = 0;
    bool result = false;

    bool error = m_doc.m_ScriptEngine->executeLua(
        m_doc.m_dispidWorldDisconnect, "OnWorldDisconnect", ActionSource::eWorldAction, "world",
        "world disconnect", nparams, sparams, invocation_count, &result);
    if (error) {
        qCDebug(lcWorld) << "Error calling OnWorldDisconnect callback";
    } else {
        qCDebug(lcWorld) << "OnWorldDisconnect callback executed successfully";
    }
}
