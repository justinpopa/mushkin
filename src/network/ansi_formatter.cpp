// ansi_formatter.cpp
// Converts Line/Style objects back to ANSI escape sequences for remote clients
//
// Part of Remote Access Server feature

#include "ansi_formatter.h"
#include "../text/line.h"
#include "../text/style.h"
#include "../world/world_document.h"

// ANSI escape sequence constants
static const char ESC = '\x1b';
static const char* CSI = "\x1b["; // Control Sequence Introducer

AnsiFormatter::AnsiFormatter(WorldDocument* doc)
    : m_pDoc(doc), m_currentFlags(0), m_currentForeColor(0), m_currentBackColor(0),
      m_currentForeColorType(COLOUR_ANSI), m_currentBackColorType(COLOUR_ANSI), m_stateValid(false)
{
}

void AnsiFormatter::reset()
{
    m_currentFlags = 0;
    m_currentForeColor = 0;
    m_currentBackColor = 0;
    m_currentForeColorType = COLOUR_ANSI;
    m_currentBackColorType = COLOUR_ANSI;
    m_stateValid = false;
}

QByteArray AnsiFormatter::formatRaw(const QString& text, bool includeNewline)
{
    QByteArray result = text.toUtf8();
    if (includeNewline) {
        result.append("\r\n");
    }
    return result;
}

QByteArray AnsiFormatter::resetAnsi()
{
    return QByteArray("\x1b[0m");
}

QByteArray AnsiFormatter::formatLine(Line* line, bool includeNewline)
{
    if (!line) {
        return includeNewline ? QByteArray("\r\n") : QByteArray();
    }

    QByteArray result;
    const char* text = line->text();
    qint32 textLen = line->len();

    // Track position in text buffer
    qint32 pos = 0;

    // Iterate through style runs
    for (const auto& stylePtr : line->styleList) {
        Style* style = stylePtr.get();
        if (!style || style->iLength == 0) {
            continue;
        }

        // Emit ANSI codes for this style
        result.append(styleToAnsi(style));

        // Emit the text covered by this style
        qint32 styleLen = qMin(static_cast<qint32>(style->iLength), textLen - pos);
        if (styleLen > 0 && pos < textLen) {
            result.append(text + pos, styleLen);
            pos += styleLen;
        }
    }

    // Emit any remaining text (shouldn't happen if styles are correct)
    if (pos < textLen) {
        result.append(text + pos, textLen - pos);
    }

    // Reset at end of line to avoid color bleeding
    if (m_stateValid) {
        result.append(resetAnsi());
        m_stateValid = false;
    }

    if (includeNewline) {
        result.append("\r\n");
    }

    return result;
}

QByteArray AnsiFormatter::formatIncompleteLine(Line* line)
{
    return formatLine(line, false);
}

QByteArray AnsiFormatter::styleToAnsi(Style* style)
{
    if (!style) {
        return QByteArray();
    }

    QByteArray result;

    // Extract color type from flags
    quint16 colorType = style->iFlags & COLOURTYPE;

    // Check if we need to emit anything
    bool needsUpdate = !m_stateValid;

    // Check style flags (text attributes)
    quint16 textFlags = style->iFlags & TEXT_STYLE;
    if (textFlags != (m_currentFlags & TEXT_STYLE)) {
        needsUpdate = true;
    }

    // Check foreground color
    if (style->iForeColour != m_currentForeColor || colorType != m_currentForeColorType) {
        needsUpdate = true;
    }

    // Check background color
    if (style->iBackColour != m_currentBackColor || colorType != m_currentBackColorType) {
        needsUpdate = true;
    }

    if (!needsUpdate) {
        return result;
    }

    // For simplicity, reset and reapply all attributes
    // This is less efficient than tracking individual changes but more reliable
    result.append(resetAnsi());

    // Apply text attributes
    quint16 flags = style->iFlags;

    if (flags & HILITE) {
        result.append(CSI);
        result.append("1m"); // Bold
    }

    if (flags & UNDERLINE) {
        result.append(CSI);
        result.append("4m"); // Underline
    }

    if (flags & BLINK) {
        result.append(CSI);
        result.append("3m"); // Italic (BLINK repurposed)
    }

    if (flags & INVERSE) {
        result.append(CSI);
        result.append("7m"); // Inverse/reverse
    }

    if (flags & STRIKEOUT) {
        result.append(CSI);
        result.append("9m"); // Strikethrough
    }

    // Apply foreground color
    result.append(colorToAnsi(style->iForeColour, colorType, true));

    // Apply background color
    result.append(colorToAnsi(style->iBackColour, colorType, false));

    // Update current state
    m_currentFlags = flags;
    m_currentForeColor = style->iForeColour;
    m_currentBackColor = style->iBackColour;
    m_currentForeColorType = colorType;
    m_currentBackColorType = colorType;
    m_stateValid = true;

    return result;
}

QByteArray AnsiFormatter::colorToAnsi(QRgb color, quint16 colorType, bool isForeground)
{
    QByteArray result;

    // Base codes differ for foreground vs background
    const int baseStd = isForeground ? 30 : 40;       // Standard: 30-37 or 40-47
    const int baseBright = isForeground ? 90 : 100;   // Bright: 90-97 or 100-107
    const char* extCode = isForeground ? "38" : "48"; // Extended color prefix

    switch (colorType) {
        case COLOUR_ANSI: {
            int index = color & 0xFF;
            if (index < 8) {
                result.append(CSI);
                result.append(QByteArray::number(baseStd + index));
                result.append('m');
            } else if (index < 16) {
                result.append(CSI);
                result.append(QByteArray::number(baseBright + (index - 8)));
                result.append('m');
            } else {
                // 256-color mode
                result.append(CSI);
                result.append(extCode);
                result.append(";5;");
                result.append(QByteArray::number(index));
                result.append('m');
            }
            break;
        }

        case COLOUR_CUSTOM: {
            if (m_pDoc) {
                int index = color & 0x0F;
                QRgb rgb = isForeground ? m_pDoc->m_customtext[index] : m_pDoc->m_customback[index];
                result.append(CSI);
                result.append(extCode);
                result.append(";2;");
                result.append(QByteArray::number(qRed(rgb)));
                result.append(';');
                result.append(QByteArray::number(qGreen(rgb)));
                result.append(';');
                result.append(QByteArray::number(qBlue(rgb)));
                result.append('m');
            }
            break;
        }

        case COLOUR_RGB: {
            result.append(CSI);
            result.append(extCode);
            result.append(";2;");
            result.append(QByteArray::number(qRed(color)));
            result.append(';');
            result.append(QByteArray::number(qGreen(color)));
            result.append(';');
            result.append(QByteArray::number(qBlue(color)));
            result.append('m');
            break;
        }

        default:
            break;
    }

    return result;
}
