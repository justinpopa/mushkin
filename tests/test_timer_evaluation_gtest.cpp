/**
 * test_timer_evaluation_gtest.cpp - GoogleTest version
 * Test timer evaluation loop
 *
 * Tests the checkTimers() and checkTimerList() functions that evaluate
 * which timers are ready to fire and execute them.
 *
 * Verifies:
 * 1. Timers only check when m_bEnableTimers is true
 * 2. Disabled timers are skipped
 * 3. active_when_closed respected (skip when disconnected)
 * 4. Timers fire when fire_time <= now
 * 5. Fire time updates before execution
 * 6. At-time timers add 1 day
 * 7. Interval timers add interval
 * 8. Clock change handling (resetOneTimer called)
 * 9. One-shot timers disabled and deleted
 * 10. Multiple timers handled correctly
 * 11. matched, when_fired, m_iTimersFiredCount updated
 */

#include "../src/automation/timer.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QThread>
#include <gtest/gtest.h>
#include <memory>

// Test fixture for timer evaluation tests
class TimerEvaluationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
        doc->m_bEnableTimers = true;
        doc->m_connectionManager->m_iConnectPhase = eConnectConnectedToMud;
    }

    void TearDown() override
    {
        // Clean up any timers left in the map (unique_ptr handles deletion automatically)
        doc->m_automationRegistry->m_TimerMap.clear();
    }

    std::unique_ptr<WorldDocument> doc;
};

// Test 1: Timers don't fire when m_bEnableTimers = false
TEST_F(TimerEvaluationTest, TimersDisabledCheck)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 1;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test1"] = std::move(timerOwned);

    doc->m_bEnableTimers = false; // Disable timers
    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;

    doc->checkTimers();

    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount) << "Timers should be ignored when disabled";

    doc->m_bEnableTimers = true; // Re-enable for other tests
}

// Test 2: Disabled timer is skipped
TEST_F(TimerEvaluationTest, DisabledTimerSkipped)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = false; // DISABLED
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 1;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test2"] = std::move(timerOwned);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;
    doc->checkTimerList();
    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount) << "Disabled timer should be skipped";
}

// Test 3: active_when_closed=false skips when disconnected
TEST_F(TimerEvaluationTest, ActiveWhenClosedRespectsConnectionState)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = false; // Requires connection
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 1;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test3"] = std::move(timerOwned);

    doc->m_connectionManager->m_iConnectPhase = eConnectNotConnected; // Disconnected

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;
    doc->checkTimerList();
    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount)
        << "Timer with active_when_closed=false should be skipped when disconnected";

    doc->m_connectionManager->m_iConnectPhase = eConnectConnectedToMud; // Restore connection
}

// Test 4: Timer fires when fire_time <= now
TEST_F(TimerEvaluationTest, TimerFiresWhenReady)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 5;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-10); // Fire time 10 seconds ago
    timerOwned->matched = 0;
    Timer* timer = timerOwned.get();
    doc->m_automationRegistry->m_TimerMap["test4"] = std::move(timerOwned);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;

    doc->checkTimerList();

    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount + 1) << "m_iTimersFiredCount should increment";
    EXPECT_EQ(timer->matched, 1) << "matched should be incremented";
    // Note: when_fired is also updated when timer fires
}

// Test 5: Interval timer fire time updates correctly
TEST_F(TimerEvaluationTest, IntervalTimerFireTimeUpdate)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 3; // 3 minutes
    timerOwned->every_second = 0.0;

    QDateTime initialFireTime = QDateTime::currentDateTime().addSecs(-5); // 5 seconds ago
    timerOwned->fire_time = initialFireTime;
    Timer* timer = timerOwned.get();
    doc->m_automationRegistry->m_TimerMap["test5"] = std::move(timerOwned);

    doc->checkTimerList();

    // Fire time should be: initialFireTime + 3 minutes
    QDateTime expectedFireTime = initialFireTime.addSecs(3 * 60);
    qint64 diff = timer->fire_time.secsTo(expectedFireTime);

    EXPECT_GE(diff, -1) << "Fire time should be within 1 second of expected (lower bound)";
    EXPECT_LE(diff, 1) << "Fire time should be within 1 second of expected (upper bound)";
}

// Test 6: At-time timer fire time updates correctly (adds 1 day)
TEST_F(TimerEvaluationTest, AtTimeTimerFireTimeUpdate)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->type = Timer::TimerType::AtTime;
    timerOwned->at_hour = 10;
    timerOwned->at_minute = 30;
    timerOwned->at_second = 0.0;

    QDateTime initialFireTime = QDateTime::currentDateTime().addSecs(-60); // 1 minute ago
    timerOwned->fire_time = initialFireTime;
    Timer* timer = timerOwned.get();
    doc->m_automationRegistry->m_TimerMap["test6"] = std::move(timerOwned);

    doc->checkTimerList();

    // Fire time should be: initialFireTime + 1 day
    QDateTime expectedFireTime = initialFireTime.addDays(1);
    qint64 diff = timer->fire_time.secsTo(expectedFireTime);

    EXPECT_GE(diff, -1) << "Fire time should be within 1 second of expected (lower bound)";
    EXPECT_LE(diff, 1) << "Fire time should be within 1 second of expected (upper bound)";
}

// Test 7: One-shot timer is disabled and deleted after execution
TEST_F(TimerEvaluationTest, OneShotTimerDeleted)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->one_shot = true; // ONE-SHOT
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 1;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-10); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test7"] = std::move(timerOwned);

    doc->checkTimerList();

    // Timer should no longer exist (deleted after execution)
    EXPECT_FALSE(doc->m_automationRegistry->m_TimerMap.find("test7") !=
                 doc->m_automationRegistry->m_TimerMap.end())
        << "One-shot timer should be deleted after execution";
    // Note: timer pointer is now invalid, don't access it
}

// Test 8: Timer doesn't fire if fire_time > now
TEST_F(TimerEvaluationTest, TimerDoesNotFireWhenNotReady)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 5;
    timerOwned->every_second = 0.0;
    timerOwned->fire_time =
        QDateTime::currentDateTime().addSecs(300); // Fire time 5 minutes in future
    timerOwned->matched = 0;
    Timer* timer = timerOwned.get();
    doc->m_automationRegistry->m_TimerMap["test8"] = std::move(timerOwned);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;

    doc->checkTimerList();

    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount) << "Timer count should not increment for future fire time";
    EXPECT_EQ(timer->matched, 0) << "matched should remain 0";
}

// Test 9: Clock change handling (fire time still in past)
TEST_F(TimerEvaluationTest, ClockChangeHandling)
{
    auto timerOwned = std::make_unique<Timer>();
    timerOwned->enabled = true;
    timerOwned->active_when_closed = true;
    timerOwned->type = Timer::TimerType::Interval;
    timerOwned->every_minute = 0;
    timerOwned->every_second = 1.0; // 1 second interval

    // Set fire time way in the past (simulating clock change)
    timerOwned->fire_time = QDateTime::currentDateTime().addSecs(-3600); // 1 hour ago
    Timer* timer = timerOwned.get();
    doc->m_automationRegistry->m_TimerMap["test9"] = std::move(timerOwned);

    doc->checkTimerList();

    // After checkTimerList, if fire time was still in past after adding interval,
    // it should have called resetOneTimer() which sets fire time to now + interval
    QDateTime now = QDateTime::currentDateTime();

    EXPECT_GT(timer->fire_time, now) << "Fire time should be reset to future after clock change";
}

// Test 10: Multiple timers fire in order
TEST_F(TimerEvaluationTest, MultipleTimersFireCorrectly)
{
    auto timer1Owned = std::make_unique<Timer>();
    timer1Owned->enabled = true;
    timer1Owned->active_when_closed = true;
    timer1Owned->type = Timer::TimerType::Interval;
    timer1Owned->every_minute = 1;
    timer1Owned->every_second = 0.0;
    timer1Owned->fire_time = QDateTime::currentDateTime().addSecs(-10);
    timer1Owned->matched = 0;

    auto timer2Owned = std::make_unique<Timer>();
    timer2Owned->enabled = true;
    timer2Owned->active_when_closed = true;
    timer2Owned->type = Timer::TimerType::Interval;
    timer2Owned->every_minute = 2;
    timer2Owned->every_second = 0.0;
    timer2Owned->fire_time = QDateTime::currentDateTime().addSecs(-5);
    timer2Owned->matched = 0;

    auto timer3Owned = std::make_unique<Timer>();
    timer3Owned->enabled = true;
    timer3Owned->active_when_closed = true;
    timer3Owned->type = Timer::TimerType::Interval;
    timer3Owned->every_minute = 3;
    timer3Owned->every_second = 0.0;
    timer3Owned->fire_time = QDateTime::currentDateTime().addSecs(300); // Future
    timer3Owned->matched = 0;

    Timer* timer1 = timer1Owned.get();
    Timer* timer2 = timer2Owned.get();
    Timer* timer3 = timer3Owned.get();
    doc->m_automationRegistry->m_TimerMap["timer1"] = std::move(timer1Owned);
    doc->m_automationRegistry->m_TimerMap["timer2"] = std::move(timer2Owned);
    doc->m_automationRegistry->m_TimerMap["timer3"] = std::move(timer3Owned);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;

    doc->checkTimerList();

    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount + 2) << "Two timers should have fired";
    EXPECT_EQ(timer1->matched, 1) << "timer1 (past) should have fired";
    EXPECT_EQ(timer2->matched, 1) << "timer2 (past) should have fired";
    EXPECT_EQ(timer3->matched, 0) << "timer3 (future) should not have fired";
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
