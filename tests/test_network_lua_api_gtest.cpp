/**
 * test_network_lua_api_gtest.cpp
 * Network-related Lua API Tests
 *
 * Tests network API functions including:
 * - Connection status: GetHostAddress, GetHostName, IsConnected
 * - Network statistics: GetConnectDuration, GetReceivedBytes, GetSentBytes
 * - Connection operations: Connect, Disconnect, Send functions
 * - UDP operations: UdpSend, UdpListen, UdpPortList
 */

#include "lua_api_test_fixture.h"

// ========== Network Information Functions ==========

// Test: GetHostAddress
TEST_F(LuaApiTest, GetHostAddress)
{
    lua_getglobal(L, "test_get_host_address");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_host_address should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_host_address should succeed";
}

// Test: GetHostName
TEST_F(LuaApiTest, GetHostName)
{
    lua_getglobal(L, "test_get_host_name");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_host_name should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_host_name should succeed";
}

// ========== Network Statistics Functions ==========

// Test: GetConnectDuration
TEST_F(LuaApiTest, GetConnectDuration)
{
    lua_getglobal(L, "test_get_connect_duration");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_connect_duration should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_connect_duration should succeed";
}

// Test: GetReceivedBytes
TEST_F(LuaApiTest, GetReceivedBytes)
{
    lua_getglobal(L, "test_get_received_bytes");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_received_bytes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_received_bytes should succeed";
}

// Test: GetSentBytes
TEST_F(LuaApiTest, GetSentBytes)
{
    lua_getglobal(L, "test_get_sent_bytes");
    int pcall_result = lua_pcall(L, 0, 1, 0);

    ASSERT_EQ(pcall_result, 0) << "test_get_sent_bytes should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");

    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);

    EXPECT_EQ(result, 0) << "test_get_sent_bytes should succeed";
}

// ========== Connection Status and Control Functions ==========

// Test: IsConnected
TEST_F(LuaApiTest, IsConnected)
{
    lua_getglobal(L, "test_is_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_is_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_is_connected should succeed";
}

// Test: Connect when not connected (disabled)
TEST_F(LuaApiTest, DISABLED_ConnectNotConnected)
{
    lua_getglobal(L, "test_connect_not_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_connect_not_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_connect_not_connected should succeed";
}

// Test: Disconnect when not connected
TEST_F(LuaApiTest, DisconnectNotConnected)
{
    lua_getglobal(L, "test_disconnect_not_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_disconnect_not_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_disconnect_not_connected should succeed";
}

// ========== Send Functions (Not Connected) ==========

// Test: Send when not connected
TEST_F(LuaApiTest, SendNotConnected)
{
    lua_getglobal(L, "test_send_not_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_not_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_not_connected should succeed";
}

// Test: SendNoEcho when not connected
TEST_F(LuaApiTest, SendNoEchoNotConnected)
{
    lua_getglobal(L, "test_send_no_echo_not_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_no_echo_not_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_no_echo_not_connected should succeed";
}

// Test: SendPkt when not connected
TEST_F(LuaApiTest, SendPktNotConnected)
{
    lua_getglobal(L, "test_send_pkt_not_connected");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_send_pkt_not_connected should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_send_pkt_not_connected should succeed";
}

// ========== UDP Functions ==========

// Test: UdpSend
TEST_F(LuaApiTest, UdpSend)
{
    lua_getglobal(L, "test_udp_send");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_udp_send should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_udp_send should succeed";
}

// Test: UdpListen
TEST_F(LuaApiTest, UdpListen)
{
    lua_getglobal(L, "test_udp_listen");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_udp_listen should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_udp_listen should succeed";
}

// Test: UdpPortList
TEST_F(LuaApiTest, UdpPortList)
{
    lua_getglobal(L, "test_udp_port_list");
    int pcall_result = lua_pcall(L, 0, 1, 0);
    ASSERT_EQ(pcall_result, 0) << "test_udp_port_list should not error: "
                               << (pcall_result != 0 ? lua_tostring(L, -1) : "");
    int result = lua_tointeger(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(result, 0) << "test_udp_port_list should succeed";
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt GUI application (required for WorldDocument)
    QGuiApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
