// test_timer_structure_gtest.cpp - GoogleTest version
// Timer Data Structure Test
//
// Verifies Timer class fields and basic operations

#include "../src/automation/timer.h"
#include <QCoreApplication>
#include <QDateTime>
#include <gtest/gtest.h>

// Test fixture for timer structure tests
class TimerStructureTest : public ::testing::Test {
  protected:
    // No special setup needed - timers are simple value objects
};

// Test 1: Timer construction with defaults
TEST_F(TimerStructureTest, DefaultConstruction)
{
    Timer* timer = new Timer();

    // Verify default values (based on actual defaults, not assumptions)
    EXPECT_EQ(timer->iType, Timer::eInterval) << "Default type should be eInterval (0)";
    EXPECT_TRUE(timer->bEnabled) << "Timer should be enabled by default";
    EXPECT_EQ(timer->iSendTo, 0) << "Default send-to should be 0";
    EXPECT_EQ(timer->nMatched, 0) << "Match count should be 0";
    EXPECT_GE(timer->nCreateSequence, 0) << "Create sequence should be valid";

    delete timer;
}

// Test 2: Setting timer fields (interval timer)
TEST_F(TimerStructureTest, IntervalTimerFields)
{
    Timer* timer = new Timer();

    // Set interval timer properties
    timer->strLabel = "test_timer";
    timer->iType = Timer::eInterval;
    timer->iEveryMinute = 5;
    timer->fEverySecond = 30.5;
    timer->strContents = "say Timer fired!";
    timer->strProcedure = "on_timer";
    timer->strGroup = "Combat";
    timer->bOneShot = false;
    timer->bTemporary = true;

    // Verify fields were set correctly
    EXPECT_EQ(timer->strLabel, "test_timer");
    EXPECT_EQ(timer->iType, Timer::eInterval);
    EXPECT_EQ(timer->iEveryMinute, 5);
    EXPECT_DOUBLE_EQ(timer->fEverySecond, 30.5);
    EXPECT_EQ(timer->strContents, "say Timer fired!");
    EXPECT_EQ(timer->strProcedure, "on_timer");
    EXPECT_EQ(timer->strGroup, "Combat");
    EXPECT_FALSE(timer->bOneShot);
    EXPECT_TRUE(timer->bTemporary);

    delete timer;
}

// Test 3: At-time timer creation
TEST_F(TimerStructureTest, AtTimeTimer)
{
    Timer* timer = new Timer();

    // Set at-time timer properties
    timer->strLabel = "daily_reminder";
    timer->iType = Timer::eAtTime;
    timer->iAtHour = 15;
    timer->iAtMinute = 30;
    timer->fAtSecond = 0.0;
    timer->strContents = "say It's 3:30 PM!";
    timer->bActiveWhenClosed = true;

    // Verify at-time fields
    EXPECT_EQ(timer->strLabel, "daily_reminder");
    EXPECT_EQ(timer->iType, Timer::eAtTime);
    EXPECT_EQ(timer->iAtHour, 15);
    EXPECT_EQ(timer->iAtMinute, 30);
    EXPECT_DOUBLE_EQ(timer->fAtSecond, 0.0);
    EXPECT_TRUE(timer->bActiveWhenClosed);
    EXPECT_GE(timer->nCreateSequence, 0);

    delete timer;
}

// Test 4: Sequence counter increments
TEST_F(TimerStructureTest, SequenceCounter)
{
    Timer* timer1 = new Timer();
    Timer* timer2 = new Timer();
    Timer* timer3 = new Timer();
    Timer* timer4 = new Timer();

    // Verify sequences increment
    EXPECT_LT(timer1->nCreateSequence, timer2->nCreateSequence)
        << "Sequence should increment for timer2";
    EXPECT_LT(timer2->nCreateSequence, timer3->nCreateSequence)
        << "Sequence should increment for timer3";
    EXPECT_LT(timer3->nCreateSequence, timer4->nCreateSequence)
        << "Sequence should increment for timer4";

    delete timer1;
    delete timer2;
    delete timer3;
    delete timer4;
}

// Test 5: All field types accessible
TEST_F(TimerStructureTest, AllFieldsAccessible)
{
    Timer* timer = new Timer();

    // Set all field types to verify they're accessible
    // Timing fields
    timer->iOffsetHour = 0;
    timer->iOffsetMinute = 2;
    timer->fOffsetSecond = 0.0;

    // Action fields
    timer->strVariable = "last_timer";
    timer->strContents = "test";
    timer->strProcedure = "test_proc";

    // Flags
    timer->bOmitFromOutput = true;
    timer->bOmitFromLog = false;
    timer->bExecutingScript = false;
    timer->bIncluded = false;
    timer->bSelected = true;

    // Metadata
    timer->iUserOption = 42;
    timer->dispid = QVariant(123);

    // Runtime tracking
    timer->nUpdateNumber = 456;
    timer->nInvocationCount = 10;
    timer->nMatched = 5;
    timer->tFireTime = QDateTime::currentDateTime().addSecs(300);
    timer->tWhenFired = QDateTime::currentDateTime();

    // Verify all fields are set correctly
    EXPECT_EQ(timer->iOffsetMinute, 2);
    EXPECT_EQ(timer->strVariable, "last_timer");
    EXPECT_TRUE(timer->bOmitFromOutput);
    EXPECT_FALSE(timer->bOmitFromLog);
    EXPECT_TRUE(timer->bSelected);
    EXPECT_EQ(timer->iUserOption, 42);
    EXPECT_EQ(timer->dispid.toInt(), 123);
    EXPECT_EQ(timer->nUpdateNumber, 456);
    EXPECT_EQ(timer->nInvocationCount, 10);
    EXPECT_EQ(timer->nMatched, 5);
    EXPECT_TRUE(timer->tFireTime.isValid());
    EXPECT_TRUE(timer->tWhenFired.isValid());

    delete timer;
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
