/**
 * output_formatter.h - Output display formatting companion
 *
 * Manages note/color/hyperlink output to the line buffer.
 * Extracted from WorldDocument for composability.
 */

#ifndef OUTPUT_FORMATTER_H
#define OUTPUT_FORMATTER_H

#include <QRgb>
#include <QString>

class WorldDocument;

class OutputFormatter {
  public:
    explicit OutputFormatter(WorldDocument& doc);

    void note(const QString& text);
    void colourNote(QRgb foreColor, QRgb backColor, const QString& text);
    void colourTell(QRgb foreColor, QRgb backColor, const QString& text);
    void hyperlink(const QString& action, const QString& text, const QString& hint, QRgb foreColor,
                   QRgb backColor, bool isUrl);
    void noteHr();
    void simulate(const QString& text);

  private:
    WorldDocument& m_doc;
};

#endif // OUTPUT_FORMATTER_H
