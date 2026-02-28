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
#include <memory>

// Test fixture for timer fire calculation tests
class TimerFireCalculationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
        testStart = QDateTime::currentDateTime();
    }

    void TearDown() override
    {
    }

    std::unique_ptr<WorldDocument> doc;
    QDateTime testStart;
};

// Test 1: Disabled timer should not have fire time updated
TEST_F(TimerFireCalculationTest, DisabledTimerUnchanged)
{
    auto timer1 = std::make_unique<Timer>();
    timer1->enabled = false;
    timer1->type = Timer::TimerType::Interval;
    timer1->every_minute = 5;
    QDateTime oldFireTime = timer1->fire_time;

    doc->resetOneTimer(timer1.get());

    EXPECT_EQ(timer1->fire_time, oldFireTime) << "Disabled timer fire time should not change";
}

// Test 2: Interval timer - simple case (5 minutes from now)
TEST_F(TimerFireCalculationTest, IntervalTimerSimple)
{
    auto timer2 = std::make_unique<Timer>();
    timer2->enabled = true;
    timer2->type = Timer::TimerType::Interval;
    timer2->every_minute = 5;
    timer2->every_second = 0.0;

    QDateTime beforeReset = QDateTime::currentDateTime();
    doc->resetOneTimer(timer2.get());
    QDateTime afterReset = QDateTime::currentDateTime();

    qint64 expectedSecs = 5 * 60; // 5 minutes in seconds
    qint64 actualSecs = beforeReset.secsTo(timer2->fire_time);

    EXPECT_GE(actualSecs, expectedSecs) << "Fire time should be at least 5 minutes from now";
    EXPECT_LE(actualSecs, expectedSecs + 1) << "Fire time should not exceed 5 minutes + 1 sec";
    EXPECT_GE(timer2->when_fired, beforeReset) << "when_fired should be >= beforeReset";
    EXPECT_LE(timer2->when_fired, afterReset) << "when_fired should be <= afterReset";
}

// Test 3: Interval timer with offset
// The offset shortens the initial delay so that firings land on a phase within the period.
// Formula: delay = interval - offset (clamped to >= 1 ms to prevent busy-loops).
// Example: every=10 min, offset=2 min -> first fire at now + 8 min, then every 10 min.
TEST_F(TimerFireCalculationTest, IntervalTimerWithOffset)
{
    auto timer3 = std::make_unique<Timer>();
    timer3->enabled = true;
    timer3->type = Timer::TimerType::Interval;
    timer3->every_minute = 10; // Every 10 minutes
    timer3->offset_minute = 2; // Offset: shortens first delay by 2 minutes
    timer3->every_second = 0.0;
    timer3->offset_second = 0.0;

    QDateTime before3 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer3.get());

    // Expected: now + (10 - 2) minutes = now + 8 minutes
    qint64 expected3 = (10 - 2) * 60;
    qint64 actual3 = before3.secsTo(timer3->fire_time);

    EXPECT_GE(actual3, expected3) << "Offset calculation should give 8 minutes";
    EXPECT_LE(actual3, expected3 + 1) << "Offset calculation tolerance";
}

// Test 4: Interval timer with complex time (1:23:45.5) — fractional seconds preserved
TEST_F(TimerFireCalculationTest, IntervalTimerComplex)
{
    auto timer4 = std::make_unique<Timer>();
    timer4->enabled = true;
    timer4->type = Timer::TimerType::Interval;
    timer4->every_hour = 1;
    timer4->every_minute = 23;
    timer4->every_second = 45.5; // 45 seconds + 500 ms

    QDateTime before4 = QDateTime::currentDateTime();
    doc->resetOneTimer(timer4.get());

    // Whole-second part of the delay (1:23:45)
    qint64 expectedWholeSecs = 1 * 3600 + 23 * 60 + 45;
    qint64 actualSecs = before4.secsTo(timer4->fire_time);

    EXPECT_GE(actualSecs, expectedWholeSecs)
        << "Complex interval whole-second component should be >= 1:23:45";
    EXPECT_LE(actualSecs, expectedWholeSecs + 1) << "Complex interval whole-second tolerance";

    // Millisecond part: the 0.5 s fraction must survive as 500 ms in fire_time
    qint64 expectedMs = (static_cast<qint64>(1) * 3600000LL + 23LL * 60000LL + 45500LL);
    qint64 actualMs = before4.msecsTo(timer4->fire_time);

    EXPECT_GE(actualMs, expectedMs) << "Fractional second (500 ms) must be preserved";
    EXPECT_LE(actualMs, expectedMs + 50) << "Fractional second tolerance (50 ms)";
}

// Test 5: At-time timer - time in the future today
TEST_F(TimerFireCalculationTest, AtTimeTimerFutureToday)
{
    auto timer5 = std::make_unique<Timer>();
    timer5->enabled = true;
    timer5->type = Timer::TimerType::AtTime;

    // Set to 1 hour from now using QDateTime (handles midnight correctly)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime futureDateTime = now.addSecs(3600); // 1 hour from now

    // If adding 1 hour crosses midnight, skip this test (handled by next test)
    if (futureDateTime.date() == now.date()) {
        timer5->at_hour = futureDateTime.time().hour();
        timer5->at_minute = futureDateTime.time().minute();
        timer5->at_second = futureDateTime.time().second();

        doc->resetOneTimer(timer5.get());

        EXPECT_EQ(timer5->fire_time.date(), now.date())
            << "At-time timer should fire today (time hasn't passed)";
        EXPECT_GT(timer5->fire_time, now) << "Fire time should be in the future";
    }
}

// Test 6: At-time timer - time that has already passed (should be tomorrow)
TEST_F(TimerFireCalculationTest, AtTimeTimerPastTimeTomorrow)
{
    auto timer6 = std::make_unique<Timer>();
    timer6->enabled = true;
    timer6->type = Timer::TimerType::AtTime;

    // Set to a time 1 second ago
    QDateTime now6 = QDateTime::currentDateTime();
    QDateTime pastDateTime = now6.addSecs(-1);
    timer6->at_hour = pastDateTime.time().hour();
    timer6->at_minute = pastDateTime.time().minute();
    timer6->at_second = pastDateTime.time().second();

    doc->resetOneTimer(timer6.get());

    QDate tomorrow = now6.date().addDays(1);

    EXPECT_EQ(timer6->fire_time.date(), tomorrow)
        << "At-time timer should move to tomorrow (time has passed)";
    EXPECT_GT(timer6->fire_time, now6) << "Fire time should be in the future";
}

// Test 7: At-time timer with fractional seconds
TEST_F(TimerFireCalculationTest, AtTimeTimerWithMilliseconds)
{
    auto timer7 = std::make_unique<Timer>();
    timer7->enabled = true;
    timer7->type = Timer::TimerType::AtTime;

    QDateTime now7 = QDateTime::currentDateTime();
    QDateTime futureDateTime7 = now7.addSecs(3600); // 1 hour from now
    timer7->at_hour = futureDateTime7.time().hour();
    timer7->at_minute = futureDateTime7.time().minute();
    timer7->at_second = 30.750; // 30 seconds and 750 milliseconds

    doc->resetOneTimer(timer7.get());

    int expectedMs = 750;
    int actualMs = timer7->fire_time.time().msec();

    EXPECT_EQ(actualMs, expectedMs)
        << "Fractional seconds should be converted to milliseconds correctly";
}

// Test 8: ResetAllTimers - batch reset
TEST_F(TimerFireCalculationTest, ResetAllTimersBatch)
{
    // Add some timers to the document
    auto batch1 = std::make_unique<Timer>();
    batch1->enabled = true;
    batch1->type = Timer::TimerType::Interval;
    batch1->every_minute = 1;
    doc->m_automationRegistry->m_TimerMap["batch1"] = std::move(batch1);

    auto batch2 = std::make_unique<Timer>();
    batch2->enabled = true;
    batch2->type = Timer::TimerType::Interval;
    batch2->every_minute = 2;
    doc->m_automationRegistry->m_TimerMap["batch2"] = std::move(batch2);

    auto batch3 = std::make_unique<Timer>();
    batch3->enabled = true;
    batch3->type = Timer::TimerType::Interval;
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

// Test 9: Offset >= interval must not produce a past fire_time (busy-loop guard)
// Before the fix: intervalSecs - offsetSecs could be <= 0, scheduling fire_time in the past
// which caused the timer to re-fire on every checkTimers() tick.
TEST_F(TimerFireCalculationTest, OffsetEqualToIntervalClamped)
{
    auto timer = std::make_unique<Timer>();
    timer->enabled = true;
    timer->type = Timer::TimerType::Interval;
    timer->every_minute = 5;  // 5-minute interval
    timer->offset_minute = 5; // offset == interval: delay would be 0 without clamping
    timer->every_second = 0.0;
    timer->offset_second = 0.0;

    QDateTime before = QDateTime::currentDateTime();
    doc->resetOneTimer(timer.get());

    EXPECT_GT(timer->fire_time, before)
        << "fire_time must be strictly in the future even when offset equals interval";
}

TEST_F(TimerFireCalculationTest, OffsetExceedsIntervalClamped)
{
    auto timer = std::make_unique<Timer>();
    timer->enabled = true;
    timer->type = Timer::TimerType::Interval;
    timer->every_minute = 3;  // 3-minute interval
    timer->offset_minute = 7; // offset > interval: delay would be negative without clamping
    timer->every_second = 0.0;
    timer->offset_second = 0.0;

    QDateTime before = QDateTime::currentDateTime();
    doc->resetOneTimer(timer.get());

    EXPECT_GT(timer->fire_time, before)
        << "fire_time must be strictly in the future even when offset exceeds interval";

    // The clamped delay must not exceed the interval itself
    qint64 intervalMs = 3LL * 60000LL;
    qint64 actualMs = before.msecsTo(timer->fire_time);
    EXPECT_LE(actualMs, intervalMs + 50) << "Clamped delay must not exceed the interval duration";
}

// Test 11: Sub-second interval timer (every_second = 0.5) must produce a future fire_time
// and preserve the 500 ms precision — before the fix this degenerated to intervalMs = 0
// and the timer fired every tick.
TEST_F(TimerFireCalculationTest, SubSecondIntervalPreservesMilliseconds)
{
    auto timer = std::make_unique<Timer>();
    timer->enabled = true;
    timer->type = Timer::TimerType::Interval;
    timer->every_hour = 0;
    timer->every_minute = 0;
    timer->every_second = 0.5; // 500 ms interval

    QDateTime before = QDateTime::currentDateTime();
    doc->resetOneTimer(timer.get());

    qint64 actualMs = before.msecsTo(timer->fire_time);

    EXPECT_GT(timer->fire_time, before) << "Sub-second timer fire_time must be in the future";
    EXPECT_GE(actualMs, 500LL) << "500 ms interval must produce >= 500 ms delay";
    EXPECT_LE(actualMs, 550LL) << "500 ms interval must produce <= 550 ms delay";
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
