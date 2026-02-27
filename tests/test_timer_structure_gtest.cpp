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
    EXPECT_EQ(timer->type, Timer::eInterval) << "Default type should be eInterval (0)";
    EXPECT_TRUE(timer->enabled) << "Timer should be enabled by default";
    EXPECT_EQ(timer->send_to, 0) << "Default send-to should be 0";
    EXPECT_EQ(timer->matched, 0) << "Match count should be 0";
    EXPECT_GE(timer->create_sequence, 0) << "Create sequence should be valid";

    delete timer;
}

// Test 2: Setting timer fields (interval timer)
TEST_F(TimerStructureTest, IntervalTimerFields)
{
    Timer* timer = new Timer();

    // Set interval timer properties
    timer->label = "test_timer";
    timer->type = Timer::eInterval;
    timer->every_minute = 5;
    timer->every_second = 30.5;
    timer->contents = "say Timer fired!";
    timer->procedure = "on_timer";
    timer->group = "Combat";
    timer->one_shot = false;
    timer->temporary = true;

    // Verify fields were set correctly
    EXPECT_EQ(timer->label, "test_timer");
    EXPECT_EQ(timer->type, Timer::eInterval);
    EXPECT_EQ(timer->every_minute, 5);
    EXPECT_DOUBLE_EQ(timer->every_second, 30.5);
    EXPECT_EQ(timer->contents, "say Timer fired!");
    EXPECT_EQ(timer->procedure, "on_timer");
    EXPECT_EQ(timer->group, "Combat");
    EXPECT_FALSE(timer->one_shot);
    EXPECT_TRUE(timer->temporary);

    delete timer;
}

// Test 3: At-time timer creation
TEST_F(TimerStructureTest, AtTimeTimer)
{
    Timer* timer = new Timer();

    // Set at-time timer properties
    timer->label = "daily_reminder";
    timer->type = Timer::eAtTime;
    timer->at_hour = 15;
    timer->at_minute = 30;
    timer->at_second = 0.0;
    timer->contents = "say It's 3:30 PM!";
    timer->active_when_closed = true;

    // Verify at-time fields
    EXPECT_EQ(timer->label, "daily_reminder");
    EXPECT_EQ(timer->type, Timer::eAtTime);
    EXPECT_EQ(timer->at_hour, 15);
    EXPECT_EQ(timer->at_minute, 30);
    EXPECT_DOUBLE_EQ(timer->at_second, 0.0);
    EXPECT_TRUE(timer->active_when_closed);
    EXPECT_GE(timer->create_sequence, 0);

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
    EXPECT_LT(timer1->create_sequence, timer2->create_sequence)
        << "Sequence should increment for timer2";
    EXPECT_LT(timer2->create_sequence, timer3->create_sequence)
        << "Sequence should increment for timer3";
    EXPECT_LT(timer3->create_sequence, timer4->create_sequence)
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
    timer->offset_hour = 0;
    timer->offset_minute = 2;
    timer->offset_second = 0.0;

    // Action fields
    timer->variable = "last_timer";
    timer->contents = "test";
    timer->procedure = "test_proc";

    // Flags
    timer->omit_from_output = true;
    timer->omit_from_log = false;
    timer->executing_script = false;
    timer->included = false;
    timer->selected = true;

    // Metadata
    timer->user_option = 42;
    timer->dispid = QVariant(123);

    // Runtime tracking
    timer->update_number = 456;
    timer->invocation_count = 10;
    timer->matched = 5;
    timer->fire_time = QDateTime::currentDateTime().addSecs(300);
    timer->when_fired = QDateTime::currentDateTime();

    // Verify all fields are set correctly
    EXPECT_EQ(timer->offset_minute, 2);
    EXPECT_EQ(timer->variable, "last_timer");
    EXPECT_TRUE(timer->omit_from_output);
    EXPECT_FALSE(timer->omit_from_log);
    EXPECT_TRUE(timer->selected);
    EXPECT_EQ(timer->user_option, 42);
    EXPECT_EQ(timer->dispid.toInt(), 123);
    EXPECT_EQ(timer->update_number, 456);
    EXPECT_EQ(timer->invocation_count, 10);
    EXPECT_EQ(timer->matched, 5);
    EXPECT_TRUE(timer->fire_time.isValid());
    EXPECT_TRUE(timer->when_fired.isValid());

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
