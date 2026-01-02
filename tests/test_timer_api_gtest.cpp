/**
 * test_timer_api_gtest.cpp - GoogleTest version
 * Test Timer Lua API functions
 *
 * Comprehensive test for all Timer Lua API functions:
 * - AddTimer (interval and at-time)
 * - IsTimer
 * - GetTimerInfo
 * - EnableTimer
 * - GetTimerList
 * - GetTimerOption / SetTimerOption
 * - ResetTimer
 * - DoAfter
 * - DoAfterNote
 * - EnableTimerGroup
 * - DeleteTimerGroup
 * - DeleteTemporaryTimers
 * - DeleteTimer
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

// Test fixture for timer API tests
class TimerApiTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        doc->m_mush_name = "Test World";
        doc->m_server = "localhost";
        doc->m_port = 4000;

        // Verify script engine is available
        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "Script engine should be initialized";
        ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should be initialized";
    }

    void TearDown() override
    {
        delete doc;
    }

    // Helper to execute Lua code
    void executeLua(const QString& code)
    {
        bool hasError = doc->m_ScriptEngine->parseLua(code, "test");
        ASSERT_FALSE(hasError) << "Lua execution should succeed";
    }

    // Helper to get Lua global number
    double getLuaNumber(const QString& varName)
    {
        lua_State* L = doc->m_ScriptEngine->L;
        lua_getglobal(L, varName.toUtf8().constData());
        double value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }

    // Helper to get Lua global string
    QString getLuaString(const QString& varName)
    {
        lua_State* L = doc->m_ScriptEngine->L;
        lua_getglobal(L, varName.toUtf8().constData());
        const char* value = lua_tostring(L, -1);
        QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 1);
        return result;
    }

    // Helper to get Lua global boolean
    bool getLuaBoolean(const QString& varName)
    {
        lua_State* L = doc->m_ScriptEngine->L;
        lua_getglobal(L, varName.toUtf8().constData());
        bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: AddTimer - Create interval timer
TEST_F(TimerApiTest, AddTimerInterval)
{
    executeLua(R"(
        -- Create a 5-second interval timer
        result = world.AddTimer("test_timer1", 0, 0, 5.0, "look",
            timer_flag.Enabled, "")
    )");

    double result = getLuaNumber("result");
    EXPECT_EQ(result, 0.0) << "AddTimer should return eOK (0)";

    // Verify timer was created
    Timer* timer = doc->getTimer("test_timer1");
    ASSERT_NE(timer, nullptr) << "Timer should exist";
    EXPECT_EQ(timer->iType, Timer::eInterval) << "Timer should be interval type";
    EXPECT_EQ(timer->iEveryHour, 0) << "Hour should be 0";
    EXPECT_EQ(timer->iEveryMinute, 0) << "Minute should be 0";
    EXPECT_EQ(timer->fEverySecond, 5.0) << "Second should be 5.0";
    EXPECT_EQ(timer->strContents, QString("look")) << "Contents should be 'look'";
    EXPECT_TRUE(timer->bEnabled) << "Timer should be enabled";
}

// Test 2: AddTimer - Create at-time timer
TEST_F(TimerApiTest, AddTimerAtTime)
{
    executeLua(R"(
        -- Create a timer that fires at 15:30:00 each day
        result = world.AddTimer("test_timer2", 15, 30, 0.0, "check mail",
            timer_flag.Enabled + timer_flag.AtTime, "")
    )");

    double result = getLuaNumber("result");
    EXPECT_EQ(result, 0.0) << "AddTimer should return eOK (0)";

    // Verify timer was created
    Timer* timer = doc->getTimer("test_timer2");
    ASSERT_NE(timer, nullptr) << "Timer should exist";
    EXPECT_EQ(timer->iType, Timer::eAtTime) << "Timer should be at-time type";
    EXPECT_EQ(timer->iAtHour, 15) << "Hour should be 15";
    EXPECT_EQ(timer->iAtMinute, 30) << "Minute should be 30";
    EXPECT_EQ(timer->fAtSecond, 0.0) << "Second should be 0.0";
    EXPECT_EQ(timer->strContents, QString("check mail")) << "Contents should be 'check mail'";
}

// Test 3: IsTimer
TEST_F(TimerApiTest, IsTimer)
{
    // Create a timer first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
    )");

    executeLua(R"(
        result1 = world.IsTimer("test_timer1")
        result2 = world.IsTimer("nonexistent_timer")
    )");

    double result1 = getLuaNumber("result1");
    double result2 = getLuaNumber("result2");

    EXPECT_EQ(result1, 0.0) << "IsTimer should return eOK (0) for existing timer";
    EXPECT_EQ(result2, 30017.0) << "IsTimer should return eTimerNotFound for nonexistent timer";
}

// Test 4: GetTimerInfo
TEST_F(TimerApiTest, GetTimerInfo)
{
    // Create a timer first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
    )");

    executeLua(R"(
        hour = world.GetTimerInfo("test_timer1", 1)
        minute = world.GetTimerInfo("test_timer1", 2)
        second = world.GetTimerInfo("test_timer1", 3)
        contents = world.GetTimerInfo("test_timer1", 4)
        enabled = world.GetTimerInfo("test_timer1", 7)
        at_time = world.GetTimerInfo("test_timer1", 8)
    )");

    double hour = getLuaNumber("hour");
    double minute = getLuaNumber("minute");
    double second = getLuaNumber("second");
    QString contents = getLuaString("contents");
    bool enabled = getLuaBoolean("enabled");
    bool at_time = getLuaBoolean("at_time");

    EXPECT_EQ(hour, 0.0) << "Hour should be 0";
    EXPECT_EQ(minute, 0.0) << "Minute should be 0";
    EXPECT_EQ(second, 5.0) << "Second should be 5.0";
    EXPECT_EQ(contents, QString("look")) << "Contents should be 'look'";
    EXPECT_TRUE(enabled) << "Timer should be enabled";
    EXPECT_FALSE(at_time) << "Timer should not be at-time type";
}

// Test 5: EnableTimer
TEST_F(TimerApiTest, EnableTimer)
{
    // Create a timer first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
    )");

    executeLua(R"(
        -- Disable the timer
        result1 = world.EnableTimer("test_timer1", false)
        enabled1 = world.GetTimerInfo("test_timer1", 7)

        -- Enable it again
        result2 = world.EnableTimer("test_timer1", true)
        enabled2 = world.GetTimerInfo("test_timer1", 7)
    )");

    double result1 = getLuaNumber("result1");
    bool enabled1 = getLuaBoolean("enabled1");
    double result2 = getLuaNumber("result2");
    bool enabled2 = getLuaBoolean("enabled2");

    EXPECT_EQ(result1, 0.0) << "EnableTimer should return eOK";
    EXPECT_FALSE(enabled1) << "Timer should be disabled";
    EXPECT_EQ(result2, 0.0) << "EnableTimer should return eOK";
    EXPECT_TRUE(enabled2) << "Timer should be enabled";
}

// Test 6: GetTimerList
TEST_F(TimerApiTest, GetTimerList)
{
    // Create timers first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
        world.AddTimer("test_timer2", 15, 30, 0.0, "check mail",
            timer_flag.Enabled + timer_flag.AtTime, "")
    )");

    executeLua(R"(
        list = world.GetTimerList()
        count = #list
    )");

    double count = getLuaNumber("count");
    EXPECT_EQ(count, 2.0) << "Should have 2 timers (test_timer1 and test_timer2)";
}

// Test 7: GetTimerOption and SetTimerOption
TEST_F(TimerApiTest, TimerOptions)
{
    // Create a timer first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
    )");

    executeLua(R"(
        -- Get current hour
        hour1 = world.GetTimerOption("test_timer1", "hour")

        -- Set new hour
        result = world.SetTimerOption("test_timer1", "hour", 1)

        -- Get updated hour
        hour2 = world.GetTimerOption("test_timer1", "hour")
    )");

    double hour1 = getLuaNumber("hour1");
    double result = getLuaNumber("result");
    double hour2 = getLuaNumber("hour2");

    EXPECT_EQ(hour1, 0.0) << "Initial hour should be 0";
    EXPECT_EQ(result, 0.0) << "SetTimerOption should return eOK";
    EXPECT_EQ(hour2, 1.0) << "Updated hour should be 1";
}

// Test 8: ResetTimer
TEST_F(TimerApiTest, ResetTimer)
{
    // Create a timer first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
    )");

    executeLua(R"(
        result = world.ResetTimer("test_timer1")
    )");

    double result = getLuaNumber("result");
    EXPECT_EQ(result, 0.0) << "ResetTimer should return eOK";

    // Verify fire time was recalculated
    Timer* timer = doc->getTimer("test_timer1");
    ASSERT_NE(timer, nullptr) << "Timer should exist";
    EXPECT_TRUE(timer->tFireTime.isValid()) << "Fire time should be valid";
}

// Test 9: DoAfter
TEST_F(TimerApiTest, DoAfter)
{
    executeLua(R"(
        result = world.DoAfter(3.5, "north")
    )");

    double result = getLuaNumber("result");
    EXPECT_EQ(result, 0.0) << "DoAfter should return eOK";

    // Find the doafter timer (name starts with "doafter_")
    Timer* doafterTimer = nullptr;
    for (const auto& [name, timerPtr] : doc->m_TimerMap) {
        if (name.startsWith("doafter_")) {
            doafterTimer = timerPtr.get();
            break;
        }
    }

    ASSERT_NE(doafterTimer, nullptr) << "DoAfter timer should be created";
    EXPECT_EQ(doafterTimer->iType, Timer::eInterval) << "DoAfter timer should be interval type";
    EXPECT_EQ(doafterTimer->fEverySecond, 3.5) << "DoAfter timer should fire after 3.5 seconds";
    EXPECT_EQ(doafterTimer->strContents, QString("north")) << "DoAfter contents should be 'north'";
    EXPECT_TRUE(doafterTimer->bOneShot) << "DoAfter timer should be one-shot";
    EXPECT_TRUE(doafterTimer->bTemporary) << "DoAfter timer should be temporary";
}

// Test 10: DoAfterNote
TEST_F(TimerApiTest, DoAfterNote)
{
    executeLua(R"(
        result = world.DoAfterNote(2.0, "Timer fired!")
    )");

    double result = getLuaNumber("result");
    EXPECT_EQ(result, 0.0) << "DoAfterNote should return eOK";

    // Find the doafternote timer
    Timer* noteTimer = nullptr;
    for (const auto& [name, timerPtr] : doc->m_TimerMap) {
        if (name.startsWith("doafternote_")) {
            noteTimer = timerPtr.get();
            break;
        }
    }

    ASSERT_NE(noteTimer, nullptr) << "DoAfterNote timer should be created";
    EXPECT_EQ(noteTimer->iSendTo, (quint16)eSendToOutput) << "DoAfterNote should send to output";
}

// Test 11: EnableTimerGroup
TEST_F(TimerApiTest, EnableTimerGroup)
{
    // Create timers in a group
    executeLua(R"(
        world.AddTimer("group_timer1", 0, 0, 10.0, "cmd1", timer_flag.Enabled, "")
        world.SetTimerOption("group_timer1", "group", "testgroup")

        world.AddTimer("group_timer2", 0, 0, 20.0, "cmd2", timer_flag.Enabled, "")
        world.SetTimerOption("group_timer2", "group", "testgroup")

        -- Disable the group
        count = world.EnableTimerGroup("testgroup", false)

        -- Check if timers are disabled
        enabled1 = world.GetTimerInfo("group_timer1", 7)
        enabled2 = world.GetTimerInfo("group_timer2", 7)
    )");

    double count = getLuaNumber("count");
    bool enabled1 = getLuaBoolean("enabled1");
    bool enabled2 = getLuaBoolean("enabled2");

    EXPECT_EQ(count, 2.0) << "Should have disabled 2 timers";
    EXPECT_FALSE(enabled1) << "group_timer1 should be disabled";
    EXPECT_FALSE(enabled2) << "group_timer2 should be disabled";
}

// Test 12: DeleteTimerGroup
TEST_F(TimerApiTest, DeleteTimerGroup)
{
    // Create timers in a group (reusing from previous test)
    executeLua(R"(
        world.AddTimer("group_timer1", 0, 0, 10.0, "cmd1", timer_flag.Enabled, "")
        world.SetTimerOption("group_timer1", "group", "testgroup")

        world.AddTimer("group_timer2", 0, 0, 20.0, "cmd2", timer_flag.Enabled, "")
        world.SetTimerOption("group_timer2", "group", "testgroup")
    )");

    executeLua(R"(
        -- Delete the group
        count = world.DeleteTimerGroup("testgroup")

        -- Verify timers are gone
        result1 = world.IsTimer("group_timer1")
        result2 = world.IsTimer("group_timer2")
    )");

    double count = getLuaNumber("count");
    double result1 = getLuaNumber("result1");
    double result2 = getLuaNumber("result2");

    EXPECT_EQ(count, 2.0) << "Should have deleted 2 timers";
    EXPECT_EQ(result1, 30017.0) << "group_timer1 should not exist";
    EXPECT_EQ(result2, 30017.0) << "group_timer2 should not exist";
}

// Test 13: DeleteTemporaryTimers
TEST_F(TimerApiTest, DeleteTemporaryTimers)
{
    // Create some temporary timers
    executeLua(R"(
        world.DoAfter(1.0, "test1")
        world.DoAfterNote(2.0, "test2")
    )");

    executeLua(R"(
        -- Count how many temporary timers exist (from DoAfter/DoAfterNote)
        count = world.DeleteTemporaryTimers()
    )");

    double count = getLuaNumber("count");
    EXPECT_GE(count, 2.0) << "Should have deleted at least 2 temporary timers";
}

// Test 14: DeleteTimer
TEST_F(TimerApiTest, DeleteTimer)
{
    // Create timers first
    executeLua(R"(
        world.AddTimer("test_timer1", 0, 0, 5.0, "look", timer_flag.Enabled, "")
        world.AddTimer("test_timer2", 15, 30, 0.0, "check mail",
            timer_flag.Enabled + timer_flag.AtTime, "")
    )");

    executeLua(R"(
        result1 = world.DeleteTimer("test_timer1")
        result2 = world.DeleteTimer("test_timer2")
        result3 = world.DeleteTimer("nonexistent_timer")
    )");

    double result1 = getLuaNumber("result1");
    double result2 = getLuaNumber("result2");
    double result3 = getLuaNumber("result3");

    EXPECT_EQ(result1, 0.0) << "DeleteTimer should return eOK for test_timer1";
    EXPECT_EQ(result2, 0.0) << "DeleteTimer should return eOK for test_timer2";
    EXPECT_EQ(result3, 30017.0) << "DeleteTimer should return eTimerNotFound";

    // Verify timers are deleted
    EXPECT_EQ(doc->getTimer("test_timer1"), nullptr) << "test_timer1 should be deleted";
    EXPECT_EQ(doc->getTimer("test_timer2"), nullptr) << "test_timer2 should be deleted";
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
