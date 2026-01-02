/**
 * test_timer_fire_calculation_gtest.cpp - GoogleTest version
 * Test timer fire time calculation
 *
 * Verifies ResetOneTimer() and ResetAllTimers() correctly calculate fire times
 * for interval timers and at-time timers with various configurations.
 */

#include "test_qt_static.h"
#include "../src/automation/timer.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QTime>
#include <gtest/gtest.h>

// Test fixture for timer fire calculation tests
class TimerFireCalculationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
        testStart = QDateTime::currentDateTime();
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
    QDateTime testStart;
};

// Test 1: Disabled timer should not have fire time updated
TEST_F(TimerFireCalculationTest, DisabledTimerUnchanged)
{
    Timer* timer1 = new Timer();
    timer1->bEnabled = false;
    timer1->iType = Timer::eInterval;
    timer1->iEveryMinute = 5;
    QDateTime oldFireTime = timer1->tFireTime;

    doc->resetOneTimer(timer1);

    EXPECT_EQ(timer1->tFireTime, oldFireTime) << "Disabled timer fire time should not change";
    delete timer1;
}

// Test 2: Interval timer - simple case (5 minutes from now)
TEST_F(TimerFireCalculationTest, IntervalTimerSimple)
{
    Timer* timer2 = new Timer();
    timer2->bEnabled = true;
    timer2->iType = Timer::eInterval;
    timer2->iEveryMinute = 5;
    timer2->fEverySecond = 0.0;

    QDateTime beforeReset = QDateTime::currentDateTime();
    doc->resetOneTimer(timer2);
    QDateTime afterReset = QDateTime::currentDateTime();

    qint64 expectedSecs = 5 * 60; // 5 minutes in seconds
    qint64 actualSecs = beforeReset.secsTo(timer2->tFireTime);

    EXPECT_GE(actualSecs, expectedSecs) << "Fire time should be at least 5 minutes from now";
    EXPECT_LE(actualSecs, expectedSecs + 1) << "Fire time should not exceed 5 minutes + 1 sec";
    EXPECT_GE(timer2->tWhenFired, beforeReset) << "tWhenFired should be >= beforeReset";
    EXPECT_LE(timer2->tWhenFired, afterReset) << "tWhenFired should be <= afterReset";

    delete timer2;
}

// Test 3: Interval timer with offset
TEST_F(TimerFireCalculationTest, IntervalTimerWithOffset)
{
    Timer* timer3 = new Timer();
    timer3->bEnabled = true;
    timer3->iType = Timer::eInterval;
    timer3->iEveryMinute = 10; // Every 10 minutes
    timer3->iOffsetMinute = 2; // But offset by 2 minutes
    timer3->fEverySecond = 0.0;
    timer3->fOffsetSecond = 0.0;

    QDateTime before3 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer3);

    // Expected: now + 10 minutes - 2 minutes = now + 8 minutes
    qint64 expected3 = (10 - 2) * 60;
    qint64 actual3 = before3.secsTo(timer3->tFireTime);

    EXPECT_GE(actual3, expected3) << "Offset calculation should give 8 minutes";
    EXPECT_LE(actual3, expected3 + 1) << "Offset calculation tolerance";

    delete timer3;
}

// Test 4: Interval timer with complex time (1:23:45)
TEST_F(TimerFireCalculationTest, IntervalTimerComplex)
{
    Timer* timer4 = new Timer();
    timer4->bEnabled = true;
    timer4->iType = Timer::eInterval;
    timer4->iEveryHour = 1;
    timer4->iEveryMinute = 23;
    timer4->fEverySecond = 45.5;

    QDateTime before4 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer4);

    qint64 expected4 = 1 * 3600 + 23 * 60 + 45; // 1:23:45 in seconds
    qint64 actual4 = before4.secsTo(timer4->tFireTime);

    EXPECT_GE(actual4, expected4) << "Complex interval time should be calculated correctly";
    EXPECT_LE(actual4, expected4 + 1) << "Complex interval tolerance";

    delete timer4;
}

// Test 5: At-time timer - time in the future today
TEST_F(TimerFireCalculationTest, AtTimeTimerFutureToday)
{
    Timer* timer5 = new Timer();
    timer5->bEnabled = true;
    timer5->iType = Timer::eAtTime;

    // Set to 1 hour from now using QDateTime (handles midnight correctly)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime futureDateTime = now.addSecs(3600); // 1 hour from now

    // If adding 1 hour crosses midnight, skip this test (handled by next test)
    if (futureDateTime.date() == now.date()) {
        timer5->iAtHour = futureDateTime.time().hour();
        timer5->iAtMinute = futureDateTime.time().minute();
        timer5->fAtSecond = futureDateTime.time().second();

        doc->resetOneTimer(timer5);

        EXPECT_EQ(timer5->tFireTime.date(), now.date())
            << "At-time timer should fire today (time hasn't passed)";
        EXPECT_GT(timer5->tFireTime, now) << "Fire time should be in the future";
    }

    delete timer5;
}

// Test 6: At-time timer - time that has already passed (should be tomorrow)
TEST_F(TimerFireCalculationTest, AtTimeTimerPastTimeTomorrow)
{
    Timer* timer6 = new Timer();
    timer6->bEnabled = true;
    timer6->iType = Timer::eAtTime;

    // Set to a time 1 second ago
    QDateTime now6 = QDateTime::currentDateTime();
    QDateTime pastDateTime = now6.addSecs(-1);
    timer6->iAtHour = pastDateTime.time().hour();
    timer6->iAtMinute = pastDateTime.time().minute();
    timer6->fAtSecond = pastDateTime.time().second();

    doc->resetOneTimer(timer6);

    QDate tomorrow = now6.date().addDays(1);

    EXPECT_EQ(timer6->tFireTime.date(), tomorrow)
        << "At-time timer should move to tomorrow (time has passed)";
    EXPECT_GT(timer6->tFireTime, now6) << "Fire time should be in the future";

    delete timer6;
}

// Test 7: At-time timer with fractional seconds
TEST_F(TimerFireCalculationTest, AtTimeTimerWithMilliseconds)
{
    Timer* timer7 = new Timer();
    timer7->bEnabled = true;
    timer7->iType = Timer::eAtTime;

    QDateTime now7 = QDateTime::currentDateTime();
    QDateTime futureDateTime7 = now7.addSecs(3600); // 1 hour from now
    timer7->iAtHour = futureDateTime7.time().hour();
    timer7->iAtMinute = futureDateTime7.time().minute();
    timer7->fAtSecond = 30.750; // 30 seconds and 750 milliseconds

    doc->resetOneTimer(timer7);

    int expectedMs = 750;
    int actualMs = timer7->tFireTime.time().msec();

    EXPECT_EQ(actualMs, expectedMs)
        << "Fractional seconds should be converted to milliseconds correctly";

    delete timer7;
}

// Test 8: ResetAllTimers - batch reset
TEST_F(TimerFireCalculationTest, ResetAllTimersBatch)
{
    // Add some timers to the document
    auto batch1 = std::make_unique<Timer>();
    batch1->bEnabled = true;
    batch1->iType = Timer::eInterval;
    batch1->iEveryMinute = 1;
    doc->m_TimerMap["batch1"] = std::move(batch1);

    auto batch2 = std::make_unique<Timer>();
    batch2->bEnabled = true;
    batch2->iType = Timer::eInterval;
    batch2->iEveryMinute = 2;
    doc->m_TimerMap["batch2"] = std::move(batch2);

    auto batch3 = std::make_unique<Timer>();
    batch3->bEnabled = true;
    batch3->iType = Timer::eInterval;
    batch3->iEveryMinute = 3;
    doc->m_TimerMap["batch3"] = std::move(batch3);

    QDateTime batchBefore = QDateTime::currentDateTime();
    doc->resetAllTimers();
    QDateTime batchAfter = QDateTime::currentDateTime();

    bool allReset = true;
    for (const auto& [name, timerPtr] : doc->m_TimerMap) {
        Timer* t = timerPtr.get();
        if (t->tWhenFired < batchBefore || t->tWhenFired > batchAfter) {
            allReset = false;
        }
    }

    EXPECT_TRUE(allReset) << "ResetAllTimers() should reset all timers";
    EXPECT_EQ(doc->m_TimerMap.size(), 3) << "Should have 3 timers";
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
