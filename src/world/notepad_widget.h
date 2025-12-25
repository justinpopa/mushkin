#ifndef NOTEPAD_WIDGET_H
#define NOTEPAD_WIDGET_H

#include <QRgb>
#include <QTextEdit>
#include <QWidget>

class WorldDocument;
class QMdiSubWindow;

// Notepad type enumeration
enum {
    eNotepadScript = 0,  // Created by script
    eNotepadRecall = 1,  // Recall buffer
    eNotepadCommand = 2, // Command history
    eNotepadOutput = 3,  // Output buffer
    eNotepadLog = 4,     // Log file view
};

// Save behavior enumeration
enum {
    eNotepadSaveDefault = 0, // Ask user on close
    eNotepadSaveAlways = 1,  // Auto-save
    eNotepadSaveNever = 2,   // Never save
};

/**
 * NotepadWidget - Text display widget for MDI child windows
 *
 * Displays text in a separate MDI child window within the main application.
 * Used for logs, stats displays, debugging output, and plugin help text.
 *
 * Architecture:
 * - QWidget (not QMainWindow) - designed to be wrapped in QMdiSubWindow
 * - Managed by WorldDocument (tracks all notepads for a world)
 * - Contains QTextEdit for text display/editing
 *
 * Based on original MUSHclient's CTextDocument (MFC Document class)
 */
class NotepadWidget : public QWidget {
    Q_OBJECT

  public:
    explicit NotepadWidget(WorldDocument* parent, const QString& title, const QString& contents,
                           QWidget* mdiParent = nullptr);
    ~NotepadWidget() override;

    // Text operations
    void AppendText(const QString& text);
    void ReplaceText(const QString& text);
    QString GetText() const;
    qint32 GetLength() const;

    // Formatting
    void SetReadOnly(bool readOnly);
    void SetFont(const QString& name, qint32 size, qint32 style, qint32 charset);
    void SetColours(QRgb text, QRgb back);
    void ApplyFont();
    void ApplyColours();

    // File operations
    bool SaveToFile(const QString& filename, bool replaceExisting);

    // Public members (matching original MUSHclient's public member access pattern)
    QString m_strTitle;             // Window title
    WorldDocument* m_pRelatedWorld; // Parent world
    qint64 m_iUniqueDocumentNumber; // World instance ID
    QMdiSubWindow* m_pMdiSubWindow; // Parent MDI window

    // Font and colors
    QString m_strFontName;
    qint32 m_iFontSize;
    qint32 m_iFontWeight;
    quint32 m_iFontCharset;
    bool m_bFontItalic;
    bool m_bFontUnderline;
    bool m_bFontStrikeout;
    QRgb m_textColour;
    QRgb m_backColour;

    // Behavior
    bool m_bReadOnly;      // Editable?
    int m_iSaveOnChange;   // Auto-save mode
    int m_iNotepadType;    // eNotepadScript, eNotepadRecall, etc.
    QString m_strFilename; // Save file path

    // UI
    QTextEdit* m_pTextEdit; // Text widget

  signals:
    void titleChanged(const QString& title);

  private:
    void setupUi();
};

#endif // NOTEPAD_WIDGET_H
