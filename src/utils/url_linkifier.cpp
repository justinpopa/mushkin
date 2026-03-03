#include "url_linkifier.h"
#include "../text/action.h"
#include "../text/line.h"
#include "../text/style.h"
#include <QList>
#include <QRegularExpression>
#include <QString>

// BGR(0, 0, 255) — blue in Windows COLORREF byte order (0x00BBGGRR)
static constexpr QRgb kUrlBlue = 255 << 16;

namespace URLLinkifier {

void detectAndLinkifyURLs(Line* line, WorldDocument* doc)
{
    if (!line || line->len() == 0) {
        return;
    }

    // Convert line text to QString for regex matching
    QString lineText = QString::fromUtf8(line->text().data(), line->text().size());

    // URL regex pattern - matches http://, https://, ftp://, mailto:
    // Excludes common punctuation that shouldn't be part of URLs
    static QRegularExpression urlPattern(R"((https?://|ftp://|mailto:)[^\s<>"{}|\\^`\[\]]+)",
                                         QRegularExpression::CaseInsensitiveOption);

    // Find all URL matches
    QRegularExpressionMatchIterator matches = urlPattern.globalMatch(lineText);
    if (!matches.hasNext()) {
        return; // No URLs found
    }

    // Collect all matches first (we'll process them in reverse to avoid offset issues)
    QList<QRegularExpressionMatch> matchList;
    while (matches.hasNext()) {
        matchList.append(matches.next());
    }

    // Process URLs in reverse order (right to left) to maintain correct positions
    // when splitting styles
    for (int i = matchList.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& match = matchList[i];
        int urlStart = match.capturedStart();
        int urlLength = match.capturedLength();
        QString url = match.captured();

        // Find which style(s) this URL spans
        quint16 currentPos = 0;
        for (size_t styleIdx = 0; styleIdx < line->styleList.size(); ++styleIdx) {
            Style* style = line->styleList[styleIdx].get();
            quint16 styleEnd = currentPos + style->iLength;

            // Check if URL overlaps with this style
            if (urlStart < styleEnd && urlStart + urlLength > currentPos) {
                // URL overlaps this style - we need to split it

                // Calculate positions relative to this style
                int relativeStart = urlStart - currentPos;
                int relativeEnd = urlStart + urlLength - currentPos;

                // Clamp to style boundaries
                if (relativeStart < 0)
                    relativeStart = 0;
                if (relativeEnd > style->iLength)
                    relativeEnd = style->iLength;

                // Split style into three parts: before URL, URL, after URL
                splitStyleForURL(line, styleIdx, relativeStart, relativeEnd - relativeStart, url,
                                 doc);

                // After splitting, we've inserted new styles, so we need to adjust our iteration
                // The split function handles updating the style list
                break; // Move to next URL
            }

            currentPos = styleEnd;
        }
    }
}

void splitStyleForURL(Line* line, size_t styleIdx, int urlStart, int urlLength, const QString& url,
                      WorldDocument* doc)
{
    Style* originalStyle = line->styleList[styleIdx].get();

    // Create Action object for the URL
    auto action = std::make_shared<Action>(url,       // action = URL to open
                                           url,       // hint = show URL in tooltip
                                           QString(), // variable = none for auto-detected URLs
                                           doc);      // world document

    // Three cases:
    // 1. URL at start: [URL][rest]
    // 2. URL in middle: [before][URL][after]
    // 3. URL at end: [before][URL]

    std::vector<std::unique_ptr<Style>> newStyles;

    // Part 1: Before URL (if URL doesn't start at position 0)
    if (urlStart > 0) {
        auto beforeStyle = std::make_unique<Style>();
        beforeStyle->iLength = urlStart;
        beforeStyle->iFlags = originalStyle->iFlags;
        beforeStyle->iForeColour = originalStyle->iForeColour;
        beforeStyle->iBackColour = originalStyle->iBackColour;
        beforeStyle->pAction = originalStyle->pAction; // Keep original action if any
        newStyles.push_back(std::move(beforeStyle));
    }

    // Part 2: URL section with hyperlink
    auto urlStyle = std::make_unique<Style>();
    urlStyle->iLength = urlLength;
    urlStyle->iFlags = originalStyle->iFlags | ACTION_HYPERLINK | UNDERLINE;
    urlStyle->iForeColour = kUrlBlue;
    urlStyle->iBackColour = originalStyle->iBackColour;
    urlStyle->pAction = action;
    newStyles.push_back(std::move(urlStyle));

    // Part 3: After URL (if there's text after the URL)
    int afterLength = originalStyle->iLength - (urlStart + urlLength);
    if (afterLength > 0) {
        auto afterStyle = std::make_unique<Style>();
        afterStyle->iLength = afterLength;
        afterStyle->iFlags = originalStyle->iFlags;
        afterStyle->iForeColour = originalStyle->iForeColour;
        afterStyle->iBackColour = originalStyle->iBackColour;
        afterStyle->pAction = originalStyle->pAction; // Keep original action if any
        newStyles.push_back(std::move(afterStyle));
    }

    // Replace the original style with the split styles
    // Erase the original
    line->styleList.erase(line->styleList.begin() + styleIdx);

    // Insert the new styles at the same position
    line->styleList.insert(line->styleList.begin() + styleIdx,
                           std::make_move_iterator(newStyles.begin()),
                           std::make_move_iterator(newStyles.end()));
}

} // namespace URLLinkifier
