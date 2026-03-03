/**
 * test_speedwalk_gtest.cpp - GoogleTest version
 * Speed Walking - Test Suite
 *
 * Tests for speedwalk::evaluate() function.
 *
 * Based on methods_speedwalks.cpp:34-154 in original MUSHclient.
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/world/speedwalk_engine.h"
#include "fixtures/world_fixtures.h"

// Default filler (empty) for most tests
static const QString kNoFiller;

// Test 1: Basic directions
// Input: "3n2w"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST(SpeedwalkTest, BasicDirections)
{
    QString result = speedwalk::evaluate("3n2w", kNoFiller);

    // Should expand to 3 norths and 2 wests
    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 2: Single direction without count
// Input: "n"
// Expected: "north\n"
TEST(SpeedwalkTest, SingleDirection)
{
    QString result = speedwalk::evaluate("n", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\n");
}

// Test 3: All standard directions
// Input: "nsewud"
// Expected: "north\nsouth\neast\nwest\nup\ndown\n"
TEST(SpeedwalkTest, AllDirections)
{
    QString result = speedwalk::evaluate("nsewud", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nsouth\neast\nwest\nup\ndown\n");
}

// Test 4: Open action
// Input: "On3e"
// Expected: "open north\neast\neast\neast\n"
TEST(SpeedwalkTest, OpenAction)
{
    QString result = speedwalk::evaluate("On3e", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "open north\neast\neast\neast\n");
}

// Test 5: Close action
// Input: "Cw2n"
// Expected: "close west\nnorth\nnorth\n"
TEST(SpeedwalkTest, CloseAction)
{
    QString result = speedwalk::evaluate("Cw2n", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "close west\nnorth\nnorth\n");
}

// Test 6: Lock and Unlock actions
// Input: "LnKs"
// Expected: "lock north\nunlock south\n"
TEST(SpeedwalkTest, LockUnlockActions)
{
    QString result = speedwalk::evaluate("LnKs", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "lock north\nunlock south\n");
}

// Test 7: Custom direction with slash
// Input: "2(ne/sw)"
// Expected: "ne\nne\n"  (only uses part before slash)
TEST(SpeedwalkTest, CustomDirectionWithSlash)
{
    QString result = speedwalk::evaluate("2(ne/sw)", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "ne\nne\n");
}

// Test 8: Custom direction without slash
// Input: "(portal)"
// Expected: "portal\n"
TEST(SpeedwalkTest, CustomDirectionNoSlash)
{
    QString result = speedwalk::evaluate("(portal)", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "portal\n");
}

// Test 9: Comments ignored
// Input: "{comment}3n{another}2w"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST(SpeedwalkTest, CommentsIgnored)
{
    QString result = speedwalk::evaluate("{comment}3n{another}2w", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 10: Whitespace handling
// Input: "  3n  2w  "
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST(SpeedwalkTest, WhitespaceHandling)
{
    QString result = speedwalk::evaluate("  3n  2w  ", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 11: Error - counter exceeds 99
// Input: "100n"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorCounterTooLarge)
{
    QString result = speedwalk::evaluate("100n", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("exceeds 99"));
}

// Test 12: Error - unterminated comment
// Input: "{comment"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorUnterminatedComment)
{
    QString result = speedwalk::evaluate("{comment", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not terminated"));
}

// Test 13: Error - unterminated custom direction
// Input: "(portal"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorUnterminatedParen)
{
    QString result = speedwalk::evaluate("(portal", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not terminated"));
}

// Test 14: Error - invalid direction
// Input: "3x"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorInvalidDirection)
{
    QString result = speedwalk::evaluate("3x", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("Invalid direction"));
}

// Test 15: Error - counter not followed by action
// Input: "3"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorCounterNoAction)
{
    QString result = speedwalk::evaluate("3", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not followed by an action"));
}

// Test 16: Error - action without direction
// Input: "O"
// Expected: Error message starting with "*"
TEST(SpeedwalkTest, ErrorActionNoDirection)
{
    QString result = speedwalk::evaluate("O", kNoFiller);

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("must be followed by a direction"));
}

// Test 17: Filler command
// Input: "2f" (with filler set to "look")
// Expected: "look\nlook\n"
TEST(SpeedwalkTest, FillerCommand)
{
    QString result = speedwalk::evaluate("2f", "look");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "look\nlook\n");
}

// Test 18: Case insensitivity
// Input: "3N2W"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST(SpeedwalkTest, CaseInsensitivity)
{
    QString result = speedwalk::evaluate("3N2W", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 19: Empty string
// Input: ""
// Expected: Empty string (no error)
TEST(SpeedwalkTest, EmptyString)
{
    QString result = speedwalk::evaluate("", kNoFiller);

    EXPECT_TRUE(result.isEmpty()) << "Empty input should return empty string";
}

// Test 20: Integration test - complex speedwalk
// Input: "On3e{to the forest}Cs2(portal/entrance)u"
// Expected: "open north\neast\neast\neast\nclose south\nportal\nportal\nup\n"
TEST(SpeedwalkTest, ComplexSpeedwalk)
{
    QString result = speedwalk::evaluate("On3e{to the forest}Cs2(portal/entrance)u", kNoFiller);

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "open north\neast\neast\neast\nclose south\nportal\nportal\nup\n");
}
