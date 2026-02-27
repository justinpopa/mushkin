/**
 * test_timer_fire_calculation_gtest.cpp - GoogleTest version
 * Test timer fire time calculation
 *
 * Verifies ResetOneTimer() and ResetAllTimers() correctly calculate fire times
 * for interval timers and at-time timers with various configurations.
 */

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
    timer1->enabled = false;
    timer1->type = Timer::eInterval;
    timer1->every_minute = 5;
    QDateTime oldFireTime = timer1->fire_time;

    doc->resetOneTimer(timer1);

    EXPECT_EQ(timer1->fire_time, oldFireTime) << "Disabled timer fire time should not change";
    delete timer1;
}

// Test 2: Interval timer - simple case (5 minutes from now)
TEST_F(TimerFireCalculationTest, IntervalTimerSimple)
{
    Timer* timer2 = new Timer();
    timer2->enabled = true;
    timer2->type = Timer::eInterval;
    timer2->every_minute = 5;
    timer2->every_second = 0.0;

    QDateTime beforeReset = QDateTime::currentDateTime();
    doc->resetOneTimer(timer2);
    QDateTime afterReset = QDateTime::currentDateTime();

    qint64 expectedSecs = 5 * 60; // 5 minutes in seconds
    qint64 actualSecs = beforeReset.secsTo(timer2->fire_time);

    EXPECT_GE(actualSecs, expectedSecs) << "Fire time should be at least 5 minutes from now";
    EXPECT_LE(actualSecs, expectedSecs + 1) << "Fire time should not exceed 5 minutes + 1 sec";
    EXPECT_GE(timer2->when_fired, beforeReset) << "when_fired should be >= beforeReset";
    EXPECT_LE(timer2->when_fired, afterReset) << "when_fired should be <= afterReset";

    delete timer2;
}

// Test 3: Interval timer with offset
TEST_F(TimerFireCalculationTest, IntervalTimerWithOffset)
{
    Timer* timer3 = new Timer();
    timer3->enabled = true;
    timer3->type = Timer::eInterval;
    timer3->every_minute = 10; // Every 10 minutes
    timer3->offset_minute = 2; // But offset by 2 minutes
    timer3->every_second = 0.0;
    timer3->offset_second = 0.0;

    QDateTime before3 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer3);

    // Expected: now + 10 minutes - 2 minutes = now + 8 minutes
    qint64 expected3 = (10 - 2) * 60;
    qint64 actual3 = before3.secsTo(timer3->fire_time);

    EXPECT_GE(actual3, expected3) << "Offset calculation should give 8 minutes";
    EXPECT_LE(actual3, expected3 + 1) << "Offset calculation tolerance";

    delete timer3;
}

// Test 4: Interval timer with complex time (1:23:45)
TEST_F(TimerFireCalculationTest, IntervalTimerComplex)
{
    Timer* timer4 = new Timer();
    timer4->enabled = true;
    timer4->type = Timer::eInterval;
    timer4->every_hour = 1;
    timer4->every_minute = 23;
    timer4->every_second = 45.5;

    QDateTime before4 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer4);

    qint64 expected4 = 1 * 3600 + 23 * 60 + 45; // 1:23:45 in seconds
    qint64 actual4 = before4.secsTo(timer4->fire_time);

    EXPECT_GE(actual4, expected4) << "Complex interval time should be calculated correctly";
    EXPECT_LE(actual4, expected4 + 1) << "Complex interval tolerance";

    delete timer4;
}

// Test 5: At-time timer - time in the future today
TEST_F(TimerFireCalculationTest, AtTimeTimerFutureToday)
{
    Timer* timer5 = new Timer();
    timer5->enabled = true;
    timer5->type = Timer::eAtTime;

    // Set to 1 hour from now using QDateTime (handles midnight correctly)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime futureDateTime = now.addSecs(3600); // 1 hour from now

    // If adding 1 hour crosses midnight, skip this test (handled by next test)
    if (futureDateTime.date() == now.date()) {
        timer5->at_hour = futureDateTime.time().hour();
        timer5->at_minute = futureDateTime.time().minute();
        timer5->at_second = futureDateTime.time().second();

        doc->resetOneTimer(timer5);

        EXPECT_EQ(timer5->fire_time.date(), now.date())
            << "At-time timer should fire today (time hasn't passed)";
        EXPECT_GT(timer5->fire_time, now) << "Fire time should be in the future";
    }

    delete timer5;
}

// Test 6: At-time timer - time that has already passed (should be tomorrow)
TEST_F(TimerFireCalculationTest, AtTimeTimerPastTimeTomorrow)
{
    Timer* timer6 = new Timer();
    timer6->enabled = true;
    timer6->type = Timer::eAtTime;

    // Set to a time 1 second ago
    QDateTime now6 = QDateTime::currentDateTime();
    QDateTime pastDateTime = now6.addSecs(-1);
    timer6->at_hour = pastDateTime.time().hour();
    timer6->at_minute = pastDateTime.time().minute();
    timer6->at_second = pastDateTime.time().second();

    doc->resetOneTimer(timer6);

    QDate tomorrow = now6.date().addDays(1);

    EXPECT_EQ(timer6->fire_time.date(), tomorrow)
        << "At-time timer should move to tomorrow (time has passed)";
    EXPECT_GT(timer6->fire_time, now6) << "Fire time should be in the future";

    delete timer6;
}

// Test 7: At-time timer with fractional seconds
TEST_F(TimerFireCalculationTest, AtTimeTimerWithMilliseconds)
{
    Timer* timer7 = new Timer();
    timer7->enabled = true;
    timer7->type = Timer::eAtTime;

    QDateTime now7 = QDateTime::currentDateTime();
    QDateTime futureDateTime7 = now7.addSecs(3600); // 1 hour from now
    timer7->at_hour = futureDateTime7.time().hour();
    timer7->at_minute = futureDateTime7.time().minute();
    timer7->at_second = 30.750; // 30 seconds and 750 milliseconds

    doc->resetOneTimer(timer7);

    int expectedMs = 750;
    int actualMs = timer7->fire_time.time().msec();

    EXPECT_EQ(actualMs, expectedMs)
        << "Fractional seconds should be converted to milliseconds correctly";

    delete timer7;
}

// Test 8: ResetAllTimers - batch reset
TEST_F(TimerFireCalculationTest, ResetAllTimersBatch)
{
    // Add some timers to the document
    auto batch1 = std::make_unique<Timer>();
    batch1->enabled = true;
    batch1->type = Timer::eInterval;
    batch1->every_minute = 1;
    doc->m_automationRegistry->m_TimerMap["batch1"] = std::move(batch1);

    auto batch2 = std::make_unique<Timer>();
    batch2->enabled = true;
    batch2->type = Timer::eInterval;
    batch2->every_minute = 2;
    doc->m_automationRegistry->m_TimerMap["batch2"] = std::move(batch2);

    auto batch3 = std::make_unique<Timer>();
    batch3->enabled = true;
    batch3->type = Timer::eInterval;
    batch3->every_minute = 3;
    doc->m_automationRegistry->m_TimerMap["batch3"] = std::move(batch3);

    QDateTime batchBefore = QDateTime::currentDateTime();
    doc->resetAllTimers();
    QDateTime batchAfter = QDateTime::currentDateTime();

    bool allReset = true;
    for (const auto& [name, timerPtr] : doc->m_automationRegistry->m_TimerMap) {
        Timer* t = timerPtr.get();
        if (t->when_fired < batchBefore || t->when_fired > batchAfter) {
            allReset = false;
        }
    }

    EXPECT_TRUE(allReset) << "ResetAllTimers() should reset all timers";
    EXPECT_EQ(doc->m_automationRegistry->m_TimerMap.size(), 3) << "Should have 3 timers";
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
