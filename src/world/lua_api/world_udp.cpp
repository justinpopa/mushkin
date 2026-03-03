/**
 * world_udp.cpp - UDP Networking and Host Resolution Functions
 */

#include "lua_common.h"
#include <QHostAddress>
#include <QHostInfo>

// Socket includes for getnameinfo (reverse DNS)
#ifdef Q_OS_WIN
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

/**
 * world.GetUdpPort(first, last)
 *
 * Gets a UDP port number (deprecated - UDP support removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param first First port in range
 * @param last Last port in range
 * @return Always returns 0 (UDP support removed)
 */
int L_GetUdpPort(lua_State* L)
{
    // UDP support was removed from MUSHclient
    // This stub exists for backward compatibility with old scripts
    Q_UNUSED(L);
    lua_pushnumber(L, 0);
    return 1;
}

/**
 * world.UdpSend(IP, Port, Text)
 *
 * Sends a UDP packet (deprecated - UDP support removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param IP Destination IP address
 * @param Port Destination port
 * @param Text Data to send
 * @return Always returns -1 (UDP support removed)
 */
int L_UdpSend(lua_State* L)
{
    Q_UNUSED(L);
    lua_pushnumber(L, -1);
    return 1;
}

/**
 * world.UdpListen(IP, Port, Script)
 *
 * Listens for UDP packets (deprecated - UDP support removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @param IP IP address to bind to
 * @param Port Port to listen on
 * @param Script Callback script function
 * @return Always returns -1 (UDP support removed)
 */
int L_UdpListen(lua_State* L)
{
    Q_UNUSED(L);
    lua_pushnumber(L, -1);
    return 1;
}

/**
 * world.UdpPortList()
 *
 * Lists UDP listening ports (deprecated - UDP support removed).
 * This function is a stub for backward compatibility.
 *
 * Based on methods_utilities.cpp
 *
 * @return Always returns empty table (UDP support removed)
 */
int L_UdpPortList(lua_State* L)
{
    lua_newtable(L);
    return 1;
}

/**
 * world.GetHostAddress(hostname)
 *
 * Looks up IP addresses for a given hostname (DNS resolution).
 * Returns IPv4 addresses only to match original MUSHclient behavior.
 *
 * @param hostname (string) Hostname to look up (e.g., "example.com")
 *
 * @return (table) Array of IP address strings (1-indexed), empty if not found
 *
 * @example
 * local addrs = GetHostAddress("google.com")
 * for i, ip in ipairs(addrs) do
 *     Note("IP " .. i .. ": " .. ip)
 * end
 *
 * -- Check if hostname resolves
 * local ips = GetHostAddress("myserver.com")
 * if #ips == 0 then
 *     Note("Could not resolve hostname")
 * end
 *
 * @see GetHostName
 */
int L_GetHostAddress(lua_State* L)
{
    const char* hostname = luaL_checkstring(L, 1);

    lua_newtable(L);

    if (strlen(hostname) == 0) {
        return 1; // Return empty table for empty hostname
    }

    QHostInfo info = QHostInfo::fromName(QString::fromUtf8(hostname));
    QList<QHostAddress> addresses = info.addresses();

    int index = 1;
    for (const QHostAddress& addr : addresses) {
        // Only include IPv4 addresses to match original behavior
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            QByteArray addrStr = addr.toString().toUtf8();
            lua_pushlstring(L, addrStr.constData(), addrStr.length());
            lua_rawseti(L, -2, index++);
        }
    }

    return 1;
}

/**
 * world.GetHostName(ipAddress)
 *
 * Looks up hostname for a given IP address (reverse DNS lookup).
 * Returns the hostname string or empty string if not found.
 *
 * @param ipAddress (string) IPv4 address to look up (e.g., "8.8.8.8")
 *
 * @return (string) Hostname, or empty string if not found
 *
 * @example
 * local name = GetHostName("8.8.8.8")
 * if name ~= "" then
 *     Note("Hostname: " .. name)  -- e.g., "dns.google"
 * else
 *     Note("No reverse DNS entry")
 * end
 *
 * @see GetHostAddress
 */
int L_GetHostName(lua_State* L)
{
    const char* ipAddress = luaL_checkstring(L, 1);

    if (strlen(ipAddress) == 0) {
        lua_pushstring(L, "");
        return 1;
    }

    QHostAddress addr(QString::fromUtf8(ipAddress));
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) {
        lua_pushstring(L, "");
        return 1;
    }

    // Use getnameinfo for proper reverse DNS lookup (like original gethostbyaddr)
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(addr.toIPv4Address());

    char hostname[NI_MAXHOST];
    int result = getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa), hostname,
                             sizeof(hostname), nullptr, 0, NI_NAMEREQD);

    if (result == 0) {
        lua_pushstring(L, hostname);
    } else {
        lua_pushstring(L, "");
    }
    return 1;
}

/**
 * world.ResetIP()
 *
 * Resets IP address cache (deprecated).
 * This function is a stub for backward compatibility.
 * Proxy support has been re-implemented via QNetworkProxy (m_proxy in WorldDocument).
 *
 * Based on methods_utilities.cpp
 */
int L_ResetIP(lua_State* L)
{
    Q_UNUSED(L);
    // No-op: proxy support is now handled via QNetworkProxy set per-socket in
    // WorldSocket::connectToHost()
    return 0;
}
