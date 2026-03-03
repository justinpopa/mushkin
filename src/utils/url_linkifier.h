#pragma once

#include <QString>
#include <cstddef>

class Line;
class WorldDocument;

namespace URLLinkifier {

void detectAndLinkifyURLs(Line* line, WorldDocument* doc);
void splitStyleForURL(Line* line, size_t styleIdx, int urlStart, int urlLength, const QString& url,
                      WorldDocument* doc);

} // namespace URLLinkifier
