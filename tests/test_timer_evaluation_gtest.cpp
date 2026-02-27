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

// Test fixture for timer evaluation tests
class TimerEvaluationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        doc->m_bEnableTimers = true;
        doc->m_connectionManager->m_iConnectPhase = eConnectConnectedToMud;
    }

    void TearDown() override
    {
        // Clean up any timers left in the map (unique_ptr handles deletion automatically)
        doc->m_automationRegistry->m_TimerMap.clear();
        delete doc;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Timers don't fire when m_bEnableTimers = false
TEST_F(TimerEvaluationTest, TimersDisabledCheck)
{
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->type = Timer::eInterval;
    timer->every_minute = 1;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test1"] = std::unique_ptr<Timer>(timer);

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
    Timer* timer = new Timer();
    timer->enabled = false; // DISABLED
    timer->type = Timer::eInterval;
    timer->every_minute = 1;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test2"] = std::unique_ptr<Timer>(timer);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;
    doc->checkTimerList();
    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount) << "Disabled timer should be skipped";
}

// Test 3: active_when_closed=false skips when disconnected
TEST_F(TimerEvaluationTest, ActiveWhenClosedRespectsConnectionState)
{
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = false; // Requires connection
    timer->type = Timer::eInterval;
    timer->every_minute = 1;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(-60); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test3"] = std::unique_ptr<Timer>(timer);

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
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->type = Timer::eInterval;
    timer->every_minute = 5;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(-10); // Fire time 10 seconds ago
    timer->matched = 0;
    doc->m_automationRegistry->m_TimerMap["test4"] = std::unique_ptr<Timer>(timer);

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
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->type = Timer::eInterval;
    timer->every_minute = 3; // 3 minutes
    timer->every_second = 0.0;

    QDateTime initialFireTime = QDateTime::currentDateTime().addSecs(-5); // 5 seconds ago
    timer->fire_time = initialFireTime;
    doc->m_automationRegistry->m_TimerMap["test5"] = std::unique_ptr<Timer>(timer);

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
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->type = Timer::eAtTime;
    timer->at_hour = 10;
    timer->at_minute = 30;
    timer->at_second = 0.0;

    QDateTime initialFireTime = QDateTime::currentDateTime().addSecs(-60); // 1 minute ago
    timer->fire_time = initialFireTime;
    doc->m_automationRegistry->m_TimerMap["test6"] = std::unique_ptr<Timer>(timer);

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
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->one_shot = true; // ONE-SHOT
    timer->type = Timer::eInterval;
    timer->every_minute = 1;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(-10); // Fire time in past
    doc->m_automationRegistry->m_TimerMap["test7"] = std::unique_ptr<Timer>(timer);

    doc->checkTimerList();

    // Timer should no longer exist (deleted after execution)
    EXPECT_FALSE(doc->m_automationRegistry->m_TimerMap.find("test7") != doc->m_automationRegistry->m_TimerMap.end())
        << "One-shot timer should be deleted after execution";
    // Note: timer pointer is now invalid, don't access it
}

// Test 8: Timer doesn't fire if fire_time > now
TEST_F(TimerEvaluationTest, TimerDoesNotFireWhenNotReady)
{
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->type = Timer::eInterval;
    timer->every_minute = 5;
    timer->every_second = 0.0;
    timer->fire_time = QDateTime::currentDateTime().addSecs(300); // Fire time 5 minutes in future
    timer->matched = 0;
    doc->m_automationRegistry->m_TimerMap["test8"] = std::unique_ptr<Timer>(timer);

    int beforeCount = doc->m_automationRegistry->m_iTimersFiredCount;

    doc->checkTimerList();

    int afterCount = doc->m_automationRegistry->m_iTimersFiredCount;

    EXPECT_EQ(afterCount, beforeCount) << "Timer count should not increment for future fire time";
    EXPECT_EQ(timer->matched, 0) << "matched should remain 0";
}

// Test 9: Clock change handling (fire time still in past)
TEST_F(TimerEvaluationTest, ClockChangeHandling)
{
    Timer* timer = new Timer();
    timer->enabled = true;
    timer->active_when_closed = true;
    timer->type = Timer::eInterval;
    timer->every_minute = 0;
    timer->every_second = 1.0; // 1 second interval

    // Set fire time way in the past (simulating clock change)
    timer->fire_time = QDateTime::currentDateTime().addSecs(-3600); // 1 hour ago
    doc->m_automationRegistry->m_TimerMap["test9"] = std::unique_ptr<Timer>(timer);

    doc->checkTimerList();

    // After checkTimerList, if fire time was still in past after adding interval,
    // it should have called resetOneTimer() which sets fire time to now + interval
    QDateTime now = QDateTime::currentDateTime();

    EXPECT_GT(timer->fire_time, now) << "Fire time should be reset to future after clock change";
}

// Test 10: Multiple timers fire in order
TEST_F(TimerEvaluationTest, MultipleTimersFireCorrectly)
{
    Timer* timer1 = new Timer();
    timer1->enabled = true;
    timer1->active_when_closed = true;
    timer1->type = Timer::eInterval;
    timer1->every_minute = 1;
    timer1->every_second = 0.0;
    timer1->fire_time = QDateTime::currentDateTime().addSecs(-10);
    timer1->matched = 0;

    Timer* timer2 = new Timer();
    timer2->enabled = true;
    timer2->active_when_closed = true;
    timer2->type = Timer::eInterval;
    timer2->every_minute = 2;
    timer2->every_second = 0.0;
    timer2->fire_time = QDateTime::currentDateTime().addSecs(-5);
    timer2->matched = 0;

    Timer* timer3 = new Timer();
    timer3->enabled = true;
    timer3->active_when_closed = true;
    timer3->type = Timer::eInterval;
    timer3->every_minute = 3;
    timer3->every_second = 0.0;
    timer3->fire_time = QDateTime::currentDateTime().addSecs(300); // Future
    timer3->matched = 0;

    doc->m_automationRegistry->m_TimerMap["timer1"] = std::unique_ptr<Timer>(timer1);
    doc->m_automationRegistry->m_TimerMap["timer2"] = std::unique_ptr<Timer>(timer2);
    doc->m_automationRegistry->m_TimerMap["timer3"] = std::unique_ptr<Timer>(timer3);

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
