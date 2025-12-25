// ansi_formatter.h
// Converts Line/Style objects back to ANSI escape sequences for remote clients
//
// Part of Remote Access Server feature

#ifndef ANSI_FORMATTER_H
#define ANSI_FORMATTER_H

#include <QByteArray>
#include <QRgb>

// Forward declarations
class Line;
class Style;
class WorldDocument;

/** Converts Line/Style objects to ANSI escape sequences for terminal output. */
class AnsiFormatter {
  public:
    explicit AnsiFormatter(WorldDocument* doc = nullptr);

    QByteArray formatLine(Line* line, bool includeNewline = true);
    QByteArray formatIncompleteLine(Line* line);
    void reset();
    static QByteArray formatRaw(const QString& text, bool includeNewline = true);

  private:
    QByteArray styleToAnsi(Style* style);
    QByteArray colorToAnsi(QRgb color, quint16 colorType, bool isForeground);
    static QByteArray resetAnsi();

    WorldDocument* m_pDoc;
    quint16 m_currentFlags;
    QRgb m_currentForeColor;
    QRgb m_currentBackColor;
    quint16 m_currentForeColorType;
    quint16 m_currentBackColorType;
    bool m_stateValid;
};

#endif // ANSI_FORMATTER_H
