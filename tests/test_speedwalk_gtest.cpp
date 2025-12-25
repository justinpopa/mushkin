/**
 * test_speedwalk_gtest.cpp - GoogleTest version
 * Speed Walking - Test Suite
 *
 * Tests for DoEvaluateSpeedwalk() function.
 *
 * Based on methods_speedwalks.cpp:34-154 in original MUSHclient.
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for speedwalk tests
// Provides common setup with WorldDocument
class SpeedwalkTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete doc;
    }

    WorldDocument* doc = nullptr;
};

// Test 1: Basic directions
// Input: "3n2w"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST_F(SpeedwalkTest, BasicDirections)
{
    QString result = doc->DoEvaluateSpeedwalk("3n2w");

    // Should expand to 3 norths and 2 wests
    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 2: Single direction without count
// Input: "n"
// Expected: "north\n"
TEST_F(SpeedwalkTest, SingleDirection)
{
    QString result = doc->DoEvaluateSpeedwalk("n");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\n");
}

// Test 3: All standard directions
// Input: "nsewud"
// Expected: "north\nsouth\neast\nwest\nup\ndown\n"
TEST_F(SpeedwalkTest, AllDirections)
{
    QString result = doc->DoEvaluateSpeedwalk("nsewud");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nsouth\neast\nwest\nup\ndown\n");
}

// Test 4: Open action
// Input: "On3e"
// Expected: "open north\neast\neast\neast\n"
TEST_F(SpeedwalkTest, OpenAction)
{
    QString result = doc->DoEvaluateSpeedwalk("On3e");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "open north\neast\neast\neast\n");
}

// Test 5: Close action
// Input: "Cw2n"
// Expected: "close west\nnorth\nnorth\n"
TEST_F(SpeedwalkTest, CloseAction)
{
    QString result = doc->DoEvaluateSpeedwalk("Cw2n");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "close west\nnorth\nnorth\n");
}

// Test 6: Lock and Unlock actions
// Input: "LnKs"
// Expected: "lock north\nunlock south\n"
TEST_F(SpeedwalkTest, LockUnlockActions)
{
    QString result = doc->DoEvaluateSpeedwalk("LnKs");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "lock north\nunlock south\n");
}

// Test 7: Custom direction with slash
// Input: "2(ne/sw)"
// Expected: "ne\nne\n"  (only uses part before slash)
TEST_F(SpeedwalkTest, CustomDirectionWithSlash)
{
    QString result = doc->DoEvaluateSpeedwalk("2(ne/sw)");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "ne\nne\n");
}

// Test 8: Custom direction without slash
// Input: "(portal)"
// Expected: "portal\n"
TEST_F(SpeedwalkTest, CustomDirectionNoSlash)
{
    QString result = doc->DoEvaluateSpeedwalk("(portal)");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "portal\n");
}

// Test 9: Comments ignored
// Input: "{comment}3n{another}2w"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST_F(SpeedwalkTest, CommentsIgnored)
{
    QString result = doc->DoEvaluateSpeedwalk("{comment}3n{another}2w");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 10: Whitespace handling
// Input: "  3n  2w  "
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST_F(SpeedwalkTest, WhitespaceHandling)
{
    QString result = doc->DoEvaluateSpeedwalk("  3n  2w  ");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 11: Error - counter exceeds 99
// Input: "100n"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorCounterTooLarge)
{
    QString result = doc->DoEvaluateSpeedwalk("100n");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("exceeds 99"));
}

// Test 12: Error - unterminated comment
// Input: "{comment"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorUnterminatedComment)
{
    QString result = doc->DoEvaluateSpeedwalk("{comment");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not terminated"));
}

// Test 13: Error - unterminated custom direction
// Input: "(portal"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorUnterminatedParen)
{
    QString result = doc->DoEvaluateSpeedwalk("(portal");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not terminated"));
}

// Test 14: Error - invalid direction
// Input: "3x"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorInvalidDirection)
{
    QString result = doc->DoEvaluateSpeedwalk("3x");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("Invalid direction"));
}

// Test 15: Error - counter not followed by action
// Input: "3"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorCounterNoAction)
{
    QString result = doc->DoEvaluateSpeedwalk("3");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("not followed by an action"));
}

// Test 16: Error - action without direction
// Input: "O"
// Expected: Error message starting with "*"
TEST_F(SpeedwalkTest, ErrorActionNoDirection)
{
    QString result = doc->DoEvaluateSpeedwalk("O");

    EXPECT_TRUE(result.startsWith("*")) << "Should return an error";
    EXPECT_TRUE(result.contains("must be followed by a direction"));
}

// Test 17: Filler command
// Input: "2f" (with filler set to "look")
// Expected: "look\nlook\n"
TEST_F(SpeedwalkTest, FillerCommand)
{
    doc->m_strSpeedWalkFiller = "look";
    QString result = doc->DoEvaluateSpeedwalk("2f");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "look\nlook\n");
}

// Test 18: Case insensitivity
// Input: "3N2W"
// Expected: "north\nnorth\nnorth\nwest\nwest\n"
TEST_F(SpeedwalkTest, CaseInsensitivity)
{
    QString result = doc->DoEvaluateSpeedwalk("3N2W");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "north\nnorth\nnorth\nwest\nwest\n");
}

// Test 19: Empty string
// Input: ""
// Expected: Empty string (no error)
TEST_F(SpeedwalkTest, EmptyString)
{
    QString result = doc->DoEvaluateSpeedwalk("");

    EXPECT_TRUE(result.isEmpty()) << "Empty input should return empty string";
}

// Test 20: Integration test - complex speedwalk
// Input: "On3e{to the forest}Cs2(portal/entrance)u"
// Expected: "open north\neast\neast\neast\nclose south\nportal\nportal\nup\n"
TEST_F(SpeedwalkTest, ComplexSpeedwalk)
{
    QString result = doc->DoEvaluateSpeedwalk("On3e{to the forest}Cs2(portal/entrance)u");

    EXPECT_FALSE(result.startsWith("*")) << "Should not return an error";
    EXPECT_EQ(result, "open north\neast\neast\neast\nclose south\nportal\nportal\nup\n");
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects like WorldDocument)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
