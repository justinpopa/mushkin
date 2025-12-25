#ifndef RECALL_DIALOG_H
#define RECALL_DIALOG_H

#include <QColor>
#include <QDialog>
#include <QString>

// Forward declarations
class QPlainTextEdit;

/**
 * RecallDialog - Text recall/notepad window dialog
 *
 * A dialog for displaying recalled text in an editable notepad-style window.
 * Supports custom fonts, colors, and read-only mode. This is used for
 * displaying text recalled from the output window or other text sources.
 *
 * Features:
 * - Editable plain text area (can be made read-only)
 * - Custom font and size support
 * - Custom text and background colors
 * - Resizable window
 * - Optional filename association (for saving)
 *
 * Based on original MUSHclient's recall dialog
 */
class RecallDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Construct a recall dialog
     * @param title Dialog window title
     * @param backgroundColor Initial background color
     * @param parent Parent widget
     */
    explicit RecallDialog(const QString& title, const QColor& backgroundColor = Qt::white,
                          QWidget* parent = nullptr);

    // Get/set text
    QString text() const;
    void setText(const QString& text);

    // Make editor read-only
    void setReadOnly(bool readOnly);

    /**
     * Set the font for the text area
     * @param fontName Font family name
     * @param size Font size in points
     * @param weight Font weight (e.g., QFont::Normal, QFont::Bold)
     */
    void setFont(const QString& fontName, int size, int weight);

    /**
     * Set text and background colors
     * @param textColor Text color
     * @param backgroundColor Background color
     */
    void setColors(const QColor& textColor, const QColor& backgroundColor);

    /**
     * Set associated filename (for potential save operations)
     * @param filename The filename to associate with this text
     */
    void setFilename(const QString& filename);

    /**
     * Get the associated filename
     * @return The filename, or empty string if none set
     */
    QString filename() const
    {
        return m_filename;
    }

  private:
    void setupUi();

    // UI Components
    QPlainTextEdit* m_textEdit;

    // Data members (from original MFC dialog)
    QString m_filename;
};

#endif // RECALL_DIALOG_H
