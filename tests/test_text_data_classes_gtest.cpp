// test_text_data_classes_gtest.cpp - GoogleTest version
// Text Data Classes Test Suite
//
// Tests the core text representation classes:
// - Action: Shared pointer managed actions for hyperlinks
// - Style: Text formatting and styling
// - Line: Text lines with embedded styles and actions

#include "test_qt_static.h"
#include "../src/text/action.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for Action tests
class ActionTest : public ::testing::Test {
  protected:
    // No special setup needed - Actions are simple value objects
};

// Test 1: Action basic creation and hash
TEST_F(ActionTest, BasicCreation)
{
    auto testAction =
        std::make_shared<Action>("look at sword", "Click to examine sword", "", nullptr);

    EXPECT_NE(testAction, nullptr);
    EXPECT_GT(testAction->m_iHash, 0);
    EXPECT_EQ(testAction->m_strAction, "look at sword");
    EXPECT_EQ(testAction->m_strHint, "Click to examine sword");
}

// Test 2: Action shared ownership
TEST_F(ActionTest, SharedOwnership)
{
    auto testAction = std::make_shared<Action>("test", "test", "", nullptr);

    // Create second shared_ptr - both point to same Action
    auto testAction2 = testAction;

    EXPECT_EQ(testAction.use_count(), 2);
    EXPECT_EQ(testAction.get(), testAction2.get());
}

// Test fixture for Style tests
class StyleTest : public ::testing::Test {
  protected:
    // No special setup needed
};

// Test 3: Default Style initialization
TEST_F(StyleTest, DefaultInitialization)
{
    auto normalStyle = std::make_unique<Style>();

    EXPECT_NE(normalStyle, nullptr);
    EXPECT_EQ(normalStyle->iLength, 0);
    EXPECT_EQ(normalStyle->pAction, nullptr);
}

// Test 4: Normal text style attributes
TEST_F(StyleTest, NormalTextStyle)
{
    auto normalStyle = std::make_unique<Style>();
    normalStyle->iLength = 11;
    normalStyle->iForeColour = qRgb(255, 255, 255);
    normalStyle->iBackColour = qRgb(0, 0, 0);
    normalStyle->iFlags = NORMAL;

    EXPECT_EQ(normalStyle->iLength, 11);
    EXPECT_EQ(normalStyle->iForeColour, qRgb(255, 255, 255));
    EXPECT_EQ(normalStyle->iBackColour, qRgb(0, 0, 0));
    EXPECT_EQ(normalStyle->iFlags, NORMAL);
}

// Test 5: Style with formatting flags
TEST_F(StyleTest, FormattingFlags)
{
    auto normalStyle = std::make_unique<Style>();
    normalStyle->iFlags = HILITE | UNDERLINE | COLOUR_RGB;

    EXPECT_NE((normalStyle->iFlags & HILITE), 0);
    EXPECT_NE((normalStyle->iFlags & UNDERLINE), 0);
    EXPECT_NE((normalStyle->iFlags & COLOUR_RGB), 0);
}

// Test 6: Style holds reference to Action
TEST_F(StyleTest, StyleHoldsActionReference)
{
    auto testAction = std::make_shared<Action>("look at sword", "Click to examine", "", nullptr);

    EXPECT_EQ(testAction.use_count(), 1);

    auto linkStyle = std::make_unique<Style>();
    linkStyle->iLength = 5;
    linkStyle->iForeColour = qRgb(0, 255, 255);
    linkStyle->iBackColour = qRgb(0, 0, 0);
    linkStyle->iFlags = HILITE | UNDERLINE | ACTION_SEND;
    linkStyle->pAction = testAction;

    EXPECT_EQ(testAction.use_count(), 2);

    // When linkStyle is destroyed, action ref count decreases
    linkStyle.reset();
    EXPECT_EQ(testAction.use_count(), 1);
}

// Test fixture for Line tests
class LineTest : public ::testing::Test {
  protected:
    // No special setup needed
};

// Test 7: Basic line creation
TEST_F(LineTest, BasicCreation)
{
    Line* testLine = new Line(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);

    EXPECT_NE(testLine, nullptr);
    EXPECT_EQ(testLine->m_nLineNumber, 1);
    EXPECT_GT(testLine->iMemoryAllocated(), 0);
    EXPECT_EQ(testLine->len(), 0); // Empty line (len() doesn't count null terminator)
    EXPECT_EQ(testLine->styleList.size(), 0);

    delete testLine;
}

// Test 8: Line with text and styles
TEST_F(LineTest, LineWithTextAndStyles)
{
    Line* testLine = new Line(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);

    auto normalStyle = std::make_unique<Style>();
    normalStyle->iLength = 11;
    normalStyle->iForeColour = qRgb(255, 255, 255);
    normalStyle->iBackColour = qRgb(0, 0, 0);
    normalStyle->iFlags = NORMAL;

    auto boldStyle = std::make_unique<Style>();
    boldStyle->iLength = 6;
    boldStyle->iForeColour = qRgb(255, 255, 0);
    boldStyle->iBackColour = qRgb(0, 0, 0);
    boldStyle->iFlags = HILITE;

    testLine->styleList.push_back(std::move(normalStyle));
    testLine->styleList.push_back(std::move(boldStyle));

    EXPECT_EQ(testLine->styleList.size(), 2);

    const char* testText = "Hello world bold!";
    int textLen = qstrlen(testText);

    // Resize buffer and copy text
    testLine->textBuffer.resize(textLen);
    memcpy(testLine->text(), testText, textLen);
    testLine->textBuffer.push_back('\0');

    EXPECT_EQ(testLine->len(), textLen); // Text length (len() doesn't count null terminator)
    EXPECT_STREQ(testLine->text(), "Hello world bold!");

    delete testLine;
}

// Test 9: Action lifecycle with Line
TEST_F(LineTest, ActionLifecycleWithLine)
{
    Line* testLine = new Line(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);

    auto testAction =
        std::make_shared<Action>("look at sword", "Click to examine sword", "", nullptr);

    EXPECT_EQ(testAction.use_count(), 1);

    auto normalStyle = std::make_unique<Style>();
    normalStyle->iLength = 11;
    normalStyle->iForeColour = qRgb(255, 255, 255);
    normalStyle->iBackColour = qRgb(0, 0, 0);
    normalStyle->iFlags = NORMAL;

    auto linkStyle = std::make_unique<Style>();
    linkStyle->iLength = 5;
    linkStyle->iForeColour = qRgb(0, 255, 255);
    linkStyle->iBackColour = qRgb(0, 0, 0);
    linkStyle->iFlags = HILITE | UNDERLINE | ACTION_SEND;
    linkStyle->pAction = testAction;

    EXPECT_EQ(testAction.use_count(), 2);

    testLine->styleList.push_back(std::move(normalStyle));
    testLine->styleList.push_back(std::move(linkStyle));

    const char* testText = "Hello world sword";
    int textLen = qstrlen(testText);
    testLine->textBuffer.resize(textLen);
    memcpy(testLine->text(), testText, textLen);
    testLine->textBuffer.push_back('\0');

    EXPECT_EQ(testLine->styleList.size(), 2);

    delete testLine;

    // After line deletion, action ref count should be back to 1
    EXPECT_EQ(testAction.use_count(), 1);
}

// Test 10: Multiple styles sharing Action
TEST_F(ActionTest, MultipleStylesSharingAction)
{
    auto testAction = std::make_shared<Action>("examine object", "Click to examine", "", nullptr);

    EXPECT_EQ(testAction.use_count(), 1);

    auto style1 = std::make_unique<Style>();
    style1->iLength = 5;
    style1->pAction = testAction;

    auto style2 = std::make_unique<Style>();
    style2->iLength = 6;
    style2->pAction = testAction;

    EXPECT_EQ(testAction.use_count(), 3);

    style1.reset();
    EXPECT_EQ(testAction.use_count(), 2);

    style2.reset();
    EXPECT_EQ(testAction.use_count(), 1);
}

// GoogleTest main entry point
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
