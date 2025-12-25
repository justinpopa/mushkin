#include "notepad_widget.h"
#include "../world/world_document.h"
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextStream>
#include <QVBoxLayout>

NotepadWidget::NotepadWidget(WorldDocument* parent, const QString& title, const QString& contents,
                             QWidget* mdiParent)
    : QWidget(mdiParent), m_strTitle(title), m_pRelatedWorld(parent),
      m_iUniqueDocumentNumber(parent ? parent->m_iUniqueDocumentNumber : 0),
      m_pMdiSubWindow(nullptr), m_iFontSize(10), m_iFontWeight(QFont::Normal), m_iFontCharset(0),
      m_bFontItalic(false), m_bFontUnderline(false), m_bFontStrikeout(false),
      m_textColour(qRgb(0, 0, 0)), m_backColour(qRgb(255, 255, 255)), m_bReadOnly(false),
      m_iSaveOnChange(eNotepadSaveDefault), m_iNotepadType(eNotepadScript), m_pTextEdit(nullptr)
{
    setupUi();

    // Set initial content
    m_pTextEdit->setPlainText(contents);

    // Apply default font/colors from world if available
    if (parent) {
        m_strFontName = parent->m_input_font_name;
        m_iFontSize = parent->m_input_font_height;
        m_iFontWeight = parent->m_input_font_weight;
        m_iFontCharset = parent->m_input_font_charset;
        m_textColour = parent->m_input_text_colour;
        m_backColour = parent->m_input_background_colour;

        ApplyFont();
        ApplyColours();

        // Register with world
        parent->RegisterNotepad(this);
    }
}

NotepadWidget::~NotepadWidget()
{
    if (m_pRelatedWorld) {
        m_pRelatedWorld->UnregisterNotepad(this);
    }
}

void NotepadWidget::setupUi()
{
    // Create layout with no margins
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create text edit widget
    m_pTextEdit = new QTextEdit(this);
    layout->addWidget(m_pTextEdit);

    setLayout(layout);
}

void NotepadWidget::AppendText(const QString& text)
{
    // Get current cursor position
    QTextCursor cursor = m_pTextEdit->textCursor();

    // Move to end
    cursor.movePosition(QTextCursor::End);
    m_pTextEdit->setTextCursor(cursor);

    // Insert text
    m_pTextEdit->insertPlainText(text);

    // Scroll to end
    m_pTextEdit->ensureCursorVisible();
}

void NotepadWidget::ReplaceText(const QString& text)
{
    m_pTextEdit->setPlainText(text);
}

QString NotepadWidget::GetText() const
{
    return m_pTextEdit->toPlainText();
}

qint32 NotepadWidget::GetLength() const
{
    return m_pTextEdit->toPlainText().length();
}

void NotepadWidget::SetReadOnly(bool readOnly)
{
    m_bReadOnly = readOnly;
    m_pTextEdit->setReadOnly(readOnly);
}

void NotepadWidget::SetFont(const QString& name, qint32 size, qint32 style, qint32 charset)
{
    // Update members
    if (!name.isEmpty())
        m_strFontName = name;

    if (size > 0)
        m_iFontSize = size;

    // Parse style flags
    if (style & 1) // Bold
        m_iFontWeight = QFont::Bold;
    else
        m_iFontWeight = QFont::Normal;

    m_bFontItalic = (style & 2) != 0;
    m_bFontUnderline = (style & 4) != 0;
    m_bFontStrikeout = (style & 8) != 0;

    m_iFontCharset = charset;

    ApplyFont();
}

void NotepadWidget::ApplyFont()
{
    QFont font(m_strFontName, m_iFontSize);
    font.setWeight(static_cast<QFont::Weight>(m_iFontWeight));
    font.setItalic(m_bFontItalic);
    font.setUnderline(m_bFontUnderline);
    font.setStrikeOut(m_bFontStrikeout);

    m_pTextEdit->setFont(font);
}

void NotepadWidget::SetColours(QRgb text, QRgb back)
{
    m_textColour = text;
    m_backColour = back;

    ApplyColours();
}

void NotepadWidget::ApplyColours()
{
    // Build stylesheet
    QString style = QString("QTextEdit { "
                            "  color: %1; "
                            "  background-color: %2; "
                            "}")
                        .arg(QColor(m_textColour).name())
                        .arg(QColor(m_backColour).name());

    m_pTextEdit->setStyleSheet(style);
}

bool NotepadWidget::SaveToFile(const QString& filename, bool replaceExisting)
{
    // Check if file exists and we shouldn't replace
    if (!replaceExisting && QFile::exists(filename)) {
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << m_pTextEdit->toPlainText();

    file.close();

    // Update stored filename
    m_strFilename = filename;

    // Mark as not modified
    m_pTextEdit->document()->setModified(false);

    return true;
}
