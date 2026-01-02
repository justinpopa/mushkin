// test_url_detection_gtest.cpp - GoogleTest version
// URL Detection and Linkification Test Suite
//
// Tests automatic URL detection in text output:
// - Detecting various URL patterns (http://, https://, ftp://, mailto:)
// - Creating hyperlink Actions for detected URLs
// - Splitting styles at URL boundaries
// - Preserving original text and style properties

#include "test_qt_static.h"
#include "../src/text/action.h"
#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/color_utils.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <gtest/gtest.h>

// Test fixture for URL detection tests
class URLDetectionTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create a WorldDocument instance for testing
        m_doc = new WorldDocument();
    }

    void TearDown() override
    {
        delete m_doc;
        m_doc = nullptr;
    }

    // Helper: Create a line with text and a single style
    Line* createTestLine(const QString& text)
    {
        Line* line = new Line(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);

        // Add text to line's textBuffer
        QByteArray textBytes = text.toUtf8();
        line->textBuffer.clear();
        for (char c : textBytes) {
            line->textBuffer.push_back(c);
        }
        // Add null terminator
        line->textBuffer.push_back('\0');

        // Create a single style covering all text
        auto style = std::make_unique<Style>();
        style->iLength = line->len();
        style->iFlags = NORMAL;
        style->iForeColour = qRgb(255, 255, 255);
        style->iBackColour = qRgb(0, 0, 0);
        style->pAction = nullptr;
        line->styleList.push_back(std::move(style));

        return line;
    }

    // Helper: Count hyperlink styles in a line
    int countHyperlinkStyles(Line* line)
    {
        int count = 0;
        for (const auto& style : line->styleList) {
            if (style->iFlags & ACTION_HYPERLINK) {
                count++;
            }
        }
        return count;
    }

    // Helper: Get text covered by a specific style
    QString getStyleText(Line* line, size_t styleIdx)
    {
        if (styleIdx >= line->styleList.size()) {
            return QString();
        }

        quint16 currentPos = 0;
        for (size_t i = 0; i < styleIdx; ++i) {
            currentPos += line->styleList[i]->iLength;
        }

        Style* style = line->styleList[styleIdx].get();
        return QString::fromUtf8(line->text() + currentPos, style->iLength);
    }

    WorldDocument* m_doc = nullptr;
};

// Test 1: Detect simple HTTP URL
TEST_F(URLDetectionTest, SimpleHTTPURL)
{
    Line* line = createTestLine("Visit http://example.com for more info");

    ASSERT_EQ(line->styleList.size(), 1); // One style before detection
    EXPECT_GT(line->len(), 0);            // Just check that there's text

    // Run URL detection
    m_doc->DetectAndLinkifyURLs(line);

    // Should split into 3 styles: before, URL, after
    ASSERT_EQ(line->styleList.size(), 3);

    // Check first style (before URL)
    EXPECT_EQ(getStyleText(line, 0), "Visit ");
    EXPECT_EQ(line->styleList[0]->iFlags & ACTION_HYPERLINK, 0);

    // Check second style (URL)
    EXPECT_EQ(getStyleText(line, 1), "http://example.com");
    EXPECT_NE(line->styleList[1]->iFlags & ACTION_HYPERLINK, 0);
    ASSERT_NE(line->styleList[1]->pAction, nullptr);
    EXPECT_EQ(line->styleList[1]->pAction->m_strAction, "http://example.com");

    // Check third style (after URL)
    EXPECT_EQ(getStyleText(line, 2), " for more info");
    EXPECT_EQ(line->styleList[2]->iFlags & ACTION_HYPERLINK, 0);

    delete line;
}

// Test 2: Detect HTTPS URL
TEST_F(URLDetectionTest, HTTPSURL)
{
    Line* line = createTestLine("Secure site: https://secure.example.com/path");

    m_doc->DetectAndLinkifyURLs(line);

    ASSERT_GE(line->styleList.size(), 2);
    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    // Find the hyperlink style
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            EXPECT_TRUE(getStyleText(line, i).startsWith("https://"));
            ASSERT_NE(line->styleList[i]->pAction, nullptr);
            EXPECT_TRUE(line->styleList[i]->pAction->m_strAction.startsWith("https://"));
            break;
        }
    }

    delete line;
}

// Test 3: Detect FTP URL
TEST_F(URLDetectionTest, FTPURL)
{
    Line* line = createTestLine("Download from ftp://files.example.com/file.zip");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    delete line;
}

// Test 4: Detect mailto: URL
TEST_F(URLDetectionTest, MailtoURL)
{
    Line* line = createTestLine("Contact mailto:support@example.com");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    // Find the mailto style
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            QString text = getStyleText(line, i);
            EXPECT_TRUE(text.startsWith("mailto:"));
            break;
        }
    }

    delete line;
}

// Test 5: Multiple URLs in one line
TEST_F(URLDetectionTest, MultipleURLs)
{
    Line* line = createTestLine("Visit http://example.com or https://other.com");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 2);

    delete line;
}

// Test 6: URL at start of line
TEST_F(URLDetectionTest, URLAtStart)
{
    Line* line = createTestLine("http://example.com is our website");

    m_doc->DetectAndLinkifyURLs(line);

    // Should split into 2 styles: URL, after
    ASSERT_GE(line->styleList.size(), 2);

    // First style should be the URL
    EXPECT_EQ(getStyleText(line, 0), "http://example.com");
    EXPECT_NE(line->styleList[0]->iFlags & ACTION_HYPERLINK, 0);

    delete line;
}

// Test 7: URL at end of line
TEST_F(URLDetectionTest, URLAtEnd)
{
    Line* line = createTestLine("Visit us at http://example.com");

    m_doc->DetectAndLinkifyURLs(line);

    // Should split into 2 styles: before, URL
    ASSERT_GE(line->styleList.size(), 2);

    // Last style should be the URL
    size_t lastIdx = line->styleList.size() - 1;
    EXPECT_EQ(getStyleText(line, lastIdx), "http://example.com");
    EXPECT_NE(line->styleList[lastIdx]->iFlags & ACTION_HYPERLINK, 0);

    delete line;
}

// Test 8: No URLs in line
TEST_F(URLDetectionTest, NoURLs)
{
    Line* line = createTestLine("This line has no URLs at all");

    size_t originalStyleCount = line->styleList.size();
    m_doc->DetectAndLinkifyURLs(line);

    // Should not change style count
    EXPECT_EQ(line->styleList.size(), originalStyleCount);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 0);

    delete line;
}

// Test 9: URL with query parameters
TEST_F(URLDetectionTest, URLWithQueryParams)
{
    Line* line = createTestLine("Search: https://example.com/search?q=test&lang=en");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    // Find the URL and verify it includes query params
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            QString url = getStyleText(line, i);
            EXPECT_TRUE(url.contains("?"));
            EXPECT_TRUE(url.contains("&"));
            break;
        }
    }

    delete line;
}

// Test 10: URL with port number
TEST_F(URLDetectionTest, URLWithPort)
{
    Line* line = createTestLine("Connect to http://example.com:8080/api");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    // Find the URL and verify it includes port
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            QString url = getStyleText(line, i);
            EXPECT_TRUE(url.contains(":8080"));
            break;
        }
    }

    delete line;
}

// Test 11: URL surrounded by punctuation
TEST_F(URLDetectionTest, URLWithPunctuation)
{
    Line* line = createTestLine("Visit (http://example.com).");

    m_doc->DetectAndLinkifyURLs(line);

    int hyperlinkCount = countHyperlinkStyles(line);
    EXPECT_EQ(hyperlinkCount, 1);

    // The URL should not include surrounding punctuation
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            QString url = getStyleText(line, i);
            // Should be just the URL, not including the )
            EXPECT_FALSE(url.endsWith(")"));
            EXPECT_FALSE(url.startsWith("("));
            break;
        }
    }

    delete line;
}

// Test 12: Empty line
TEST_F(URLDetectionTest, EmptyLine)
{
    Line* line = new Line(1, 80, 0, qRgb(255, 255, 255), qRgb(0, 0, 0), false);

    m_doc->DetectAndLinkifyURLs(line);

    // Should handle empty line gracefully
    EXPECT_EQ(line->len(), 0);

    delete line;
}

// Test 13: Hyperlink style attributes
TEST_F(URLDetectionTest, HyperlinkStyleAttributes)
{
    Line* line = createTestLine("Link: http://example.com");

    m_doc->DetectAndLinkifyURLs(line);

    // Find the hyperlink style
    for (size_t i = 0; i < line->styleList.size(); ++i) {
        if (line->styleList[i]->iFlags & ACTION_HYPERLINK) {
            // Should have ACTION_HYPERLINK and UNDERLINE flags
            EXPECT_NE(line->styleList[i]->iFlags & ACTION_HYPERLINK, 0);
            EXPECT_NE(line->styleList[i]->iFlags & UNDERLINE, 0);

            // Should be blue color (stored as BGR/COLORREF)
            EXPECT_EQ(line->styleList[i]->iForeColour, BGR(0, 0, 255));

            // Should have an Action object
            ASSERT_NE(line->styleList[i]->pAction, nullptr);
            break;
        }
    }

    delete line;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
