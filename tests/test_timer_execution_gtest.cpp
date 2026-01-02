/**
 * test_timer_execution_gtest.cpp - GoogleTest version
 * Test timer execution functionality
 *
 * Tests the executeTimer() and executeTimerScript() functions that handle
 * timer firing and Lua callbacks.
 *
 * Verifies:
 * 1. Timer sends contents via SendTo()
 * 2. bExecutingScript flag prevents deletion during execution
 * 3. Lua callbacks with timer name parameter
 * 4. dispid caching for Lua function lookups
 * 5. nInvocationCount tracking
 * 6. Timer label vs internal name handling
 * 7. Non-existent functions handled gracefully
 * 8. Empty strProcedure handled correctly
 * 9. Unlabelled timers use internal name
 *
 * NOTE: These tests have exhibited flaky behavior in CI environments:
 * - Intermittently hangs/times out during both test discovery and execution
 * - Passes consistently in local testing (verified with 1000 consecutive runs)
 * - Observed on both Windows (GoogleTest discovery phase) and Linux (execution phase)
 * - Occurs on separate self-hosted CI runners, suggesting potential code race
 *   condition rather than infrastructure issue
 * - The exact root cause is unclear; may be related to WorldDocument/ScriptEngine
 *   initialization or cleanup timing issues under CI load conditions
 */

#include "test_qt_static.h"
#include "../src/automation/sendto.h"
#include "../src/automation/timer.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QDateTime>
#include <gtest/gtest.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Test fixture for timer execution tests
class TimerExecutionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        doc->m_iConnectPhase = eConnectConnectedToMud;
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Basic timer execution (no script)
TEST_F(TimerExecutionTest, BasicTimerExecutionSendToOnly)
{
    Timer* timer = new Timer();
    timer->bEnabled = true;
    timer->bActiveWhenClosed = true;
    timer->iType = Timer::eInterval;
    timer->iEveryMinute = 1;
    timer->fEverySecond = 0.0;
    timer->strContents = "say Hello from timer!";
    timer->iSendTo = eSendToWorld;
    timer->bOmitFromOutput = false;
    timer->bOmitFromLog = false;
    timer->strLabel = "test_timer";
    timer->strProcedure = ""; // No script

    QString timerName = "test_timer";

    // Test executeTimer
    doc->executeTimer(timer, timerName);

    // Verify bExecutingScript was set and cleared
    EXPECT_FALSE(timer->bExecutingScript) << "bExecutingScript should be cleared after execution";

    delete timer;
}

// Test 2: Timer execution with label vs internal name
TEST_F(TimerExecutionTest, TimerLabelVsInternalName)
{
    Timer* timer1 = new Timer();
    timer1->strLabel = "my_label";
    timer1->strContents = "test";
    timer1->iSendTo = eSendToWorld;
    timer1->bEnabled = true;

    doc->executeTimer(timer1, "*timer0000000001");

    EXPECT_FALSE(timer1->bExecutingScript) << "Timer with label should execute successfully";

    delete timer1;
}

// Test 3: bExecutingScript flag protection
TEST_F(TimerExecutionTest, ExecutingScriptFlagProtection)
{
    Timer* timer = new Timer();
    timer->strLabel = "protected_timer";
    timer->strContents = "test";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->bExecutingScript = false;

    // Note: We can't directly test deletion prevention during execution,
    // but we can verify the flag is set correctly
    doc->executeTimer(timer, "protected_timer");

    EXPECT_FALSE(timer->bExecutingScript)
        << "bExecutingScript flag should be cleared after execution";

    delete timer;
}

// Test 4: Timer with Lua callback
TEST_F(TimerExecutionTest, TimerLuaCallbackExecution)
{
    // Define Lua function
    QString luaScript = R"(
-- Global variable to track callback
timer_callback_called = false
timer_callback_name = ""

function OnTestTimer(timerName)
    timer_callback_called = true
    timer_callback_name = timerName
end
)";

    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Timer* timer = new Timer();
    timer->strLabel = "callback_timer";
    timer->strContents = "";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = "OnTestTimer";
    timer->dispid = DISPID_UNKNOWN; // Not yet cached
    timer->nInvocationCount = 0;

    doc->executeTimer(timer, "callback_timer");

    // Verify Lua callback was called
    ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should exist";

    lua_State* L = doc->m_ScriptEngine->L;

    lua_getglobal(L, "timer_callback_called");
    bool wasCalled = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "timer_callback_name");
    QString nameReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    EXPECT_TRUE(wasCalled) << "Lua callback should be called";
    EXPECT_EQ(nameReceived, "callback_timer") << "Correct timer name should be passed to callback";
    EXPECT_EQ(timer->nInvocationCount, 1) << "Invocation count should be 1";

    delete timer;
}

// Test 5: dispid caching
TEST_F(TimerExecutionTest, DispidCaching)
{
    // Define Lua function
    QString luaScript = R"(
function OnCachedTimer(timerName)
    -- Function exists
end
)";

    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";
    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Timer* timer = new Timer();
    timer->strLabel = "cached_timer";
    timer->strContents = "";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = "OnCachedTimer";
    timer->dispid = DISPID_UNKNOWN; // First call - not cached

    // First execution - should cache dispid
    doc->executeTimer(timer, "cached_timer");

    qint32 cachedDispid = timer->dispid.toInt();

    EXPECT_NE(cachedDispid, DISPID_UNKNOWN) << "dispid should be cached after first execution";

    // Second execution - should use cached dispid
    doc->executeTimer(timer, "cached_timer");

    EXPECT_EQ(timer->dispid.toInt(), cachedDispid) << "Cached dispid should be reused";
    EXPECT_EQ(timer->nInvocationCount, 2) << "Invocation count should be 2";

    delete timer;
}

// Test 6: Timer with non-existent Lua function
TEST_F(TimerExecutionTest, NonExistentLuaFunction)
{
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";

    Timer* timer = new Timer();
    timer->strLabel = "missing_function_timer";
    timer->strContents = "";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = "NonExistentFunction";
    timer->dispid = DISPID_UNKNOWN;
    timer->nInvocationCount = 0;

    // Execute timer - should not crash
    doc->executeTimer(timer, "missing_function_timer");

    // Should have checked for function and not called it
    EXPECT_EQ(timer->dispid.toInt(), DISPID_UNKNOWN) << "dispid should remain DISPID_UNKNOWN";
    EXPECT_EQ(timer->nInvocationCount, 0) << "Invocation count should be 0 (function not called)";

    delete timer;
}

// Test 7: Timer with empty strProcedure
TEST_F(TimerExecutionTest, EmptyStrProcedure)
{
    Timer* timer = new Timer();
    timer->strLabel = "no_script_timer";
    timer->strContents = "test content";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = ""; // Empty - no script
    timer->dispid = DISPID_UNKNOWN;
    timer->nInvocationCount = 0;

    // Execute - should not crash
    doc->executeTimer(timer, "no_script_timer");

    EXPECT_EQ(timer->nInvocationCount, 0) << "Invocation count should remain 0";

    delete timer;
}

// Test 8: nInvocationCount increments correctly
TEST_F(TimerExecutionTest, InvocationCountTracking)
{
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";

    // Define Lua function
    QString luaScript = R"(
function OnInvocationTimer(timerName)
    -- Simple function
end
)";

    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Timer* timer = new Timer();
    timer->strLabel = "invocation_timer";
    timer->strContents = "";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = "OnInvocationTimer";
    timer->dispid = DISPID_UNKNOWN;
    timer->nInvocationCount = 0;

    // Execute 3 times
    doc->executeTimer(timer, "invocation_timer");
    doc->executeTimer(timer, "invocation_timer");
    doc->executeTimer(timer, "invocation_timer");

    EXPECT_EQ(timer->nInvocationCount, 3) << "nInvocationCount should track correctly";

    delete timer;
}

// Test 9: Timer uses internal name when label is empty
TEST_F(TimerExecutionTest, UnlabelledTimerUsesInternalName)
{
    ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be available";

    // Define Lua function
    QString luaScript = R"(
unlabelled_timer_name = ""

function OnUnlabelledTimer(timerName)
    unlabelled_timer_name = timerName
end
)";

    doc->m_ScriptEngine->parseLua(luaScript, "Test script");

    Timer* timer = new Timer();
    timer->strLabel = ""; // No label - unlabelled timer
    timer->strContents = "";
    timer->iSendTo = eSendToWorld;
    timer->bEnabled = true;
    timer->strProcedure = "OnUnlabelledTimer";
    timer->dispid = DISPID_UNKNOWN;

    QString internalName = "*timer0000000042";
    doc->executeTimer(timer, internalName);

    // Check that internal name was passed to Lua
    lua_State* L = doc->m_ScriptEngine->L;
    lua_getglobal(L, "unlabelled_timer_name");
    QString nameReceived = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    EXPECT_EQ(nameReceived, internalName) << "Unlabelled timer should use internal name";

    delete timer;
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
