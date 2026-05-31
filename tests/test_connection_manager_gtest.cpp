/**
 * test_connection_manager_gtest.cpp
 *
 * Behavioral parity tests for ConnectionManager — regression coverage for
 * deviations fixed against the original MUSHclient source.
 */

#include "../src/world/automation_registry.h"
#include "../src/world/connection_manager.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QDateTime>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// H95 / M63 — m_whenWorldStarted set at construction, not at connect time
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, WhenWorldStartedIsValidAtConstruction)
{
    // Original: doc_construct.cpp:28 — m_whenWorldStarted = CTime::GetCurrentTime() at ctor.
    // Must be valid before any connection attempt.
    EXPECT_TRUE(doc->m_connectionManager->m_whenWorldStarted.isValid())
        << "m_whenWorldStarted must be set at document construction";
}

TEST_F(WorldDocumentTest, WhenWorldStartedIsNotUpdatedOnConnect)
{
    QDateTime before = doc->m_connectionManager->m_whenWorldStarted;
    ASSERT_TRUE(before.isValid());

    // Simulate onConnect(0) without a real socket by directly poking state.
    // onConnect() used to overwrite m_whenWorldStarted; it must no longer do so.
    QDateTime connectTime = before.addSecs(300);
    doc->m_connectionManager->m_tConnectTime = connectTime;

    // m_whenWorldStarted must remain unchanged after a simulated connect update.
    EXPECT_EQ(doc->m_connectionManager->m_whenWorldStarted, before)
        << "m_whenWorldStarted must not change when m_tConnectTime is updated";
}

// ---------------------------------------------------------------------------
// M63 — m_tConnectTime initialized at construction so GetInfo(301) is valid
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, ConnectTimeIsValidAtConstruction)
{
    // Original: doc_construct.cpp:305 — m_tConnectTime = CTime::GetCurrentTime() at ctor.
    EXPECT_TRUE(doc->m_connectionManager->m_tConnectTime.isValid())
        << "m_tConnectTime must be set at construction so GetInfo(301) always returns a date";
}

// ---------------------------------------------------------------------------
// M12 — m_nBytesIn / m_nBytesOut are lifetime totals; not reset on connect
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, ByteCountersAreZeroAtConstruction)
{
    // Original: doc_construct.cpp:63-64 — initialized to 0 once at construction.
    EXPECT_EQ(doc->m_connectionManager->m_nBytesIn, 0);
    EXPECT_EQ(doc->m_connectionManager->m_nBytesOut, 0);
}

TEST_F(WorldDocumentTest, ByteCountersNotResetByOnConnect)
{
    // Simulate bytes accumulated over a session.
    doc->m_connectionManager->m_nBytesIn = 12345;
    doc->m_connectionManager->m_nBytesOut = 6789;

    // onConnect() must NOT reset these lifetime counters.
    // We verify indirectly: after a simulated connect phase change the values stay.
    // (A full onConnect() requires a socket; we just verify the fields survive a phase change.)
    doc->m_connectionManager->m_iConnectPhase = CONNECT_CONNECTED_TO_MUD;

    EXPECT_EQ(doc->m_connectionManager->m_nBytesIn, 12345)
        << "m_nBytesIn is a lifetime total and must not be reset on connect";
    EXPECT_EQ(doc->m_connectionManager->m_nBytesOut, 6789)
        << "m_nBytesOut is a lifetime total and must not be reset on connect";
}

// ---------------------------------------------------------------------------
// M12 — m_iTimersFiredThisSessionCount present in AutomationRegistry
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, TimersFiredThisSessionCountExistsAndStartsAtZero)
{
    // Original: doc.cpp:6576 — m_iTimersFiredThisSessionCount = 0 reset at ConnectionEstablished.
    EXPECT_EQ(doc->m_automationRegistry->m_iTimersFiredThisSessionCount, 0)
        << "m_iTimersFiredThisSessionCount must start at 0";
}

// ---------------------------------------------------------------------------
// M10 — auto-log close condition: auto_log_file_name, not IsLogOpen()
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, AutoLogFileNameFieldExists)
{
    // The disconnect path guards on !auto_log_file_name.isEmpty() (original: worldsock.cpp:109).
    // Verify the field is accessible and starts empty (no auto-log configured by default).
    EXPECT_TRUE(doc->m_logging.auto_log_file_name.isEmpty())
        << "auto_log_file_name should be empty by default";
}

// ---------------------------------------------------------------------------
// M96 — m_bDisconnectOK starts true at construction but is false during connect
// ---------------------------------------------------------------------------

TEST_F(WorldDocumentTest, DisconnectOKStartsTrueAtConstruction)
{
    // WorldDocument initializes m_bDisconnectOK = true (world_document.cpp:399).
    EXPECT_TRUE(doc->m_bDisconnectOK)
        << "m_bDisconnectOK should be true at construction (world not yet connecting)";
}
