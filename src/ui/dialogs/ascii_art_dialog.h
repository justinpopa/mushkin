#ifndef ASCII_ART_DIALOG_H
#define ASCII_ART_DIALOG_H

#include <QDialog>

// Forward declarations
class QLineEdit;
class QComboBox;
class QPlainTextEdit;

/**
 * AsciiArtDialog - ASCII art text generator
 *
 * Standalone dialog that converts text into ASCII art using various fonts.
 * Provides a text input, font selection, and preview of the generated art.
 */
class AsciiArtDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit AsciiArtDialog(QWidget* parent = nullptr);
    ~AsciiArtDialog() override;

    /**
     * Get the input text
     * @return The text entered by the user
     */
    QString text() const;

    /**
     * Get the selected font name
     * @return The name of the selected ASCII art font
     */
    QString fontName() const;

    /**
     * Get the generated ASCII art
     * @return The generated ASCII art text (currently placeholder)
     */
    QString generatedArt() const;

  private slots:
    /**
     * Text or font changed - update preview
     */
    void updatePreview();

  private:
    void setupUi();

    // UI widgets
    QLineEdit* m_textEdit;
    QComboBox* m_fontCombo;
    QPlainTextEdit* m_previewEdit;
};

#endif // ASCII_ART_DIALOG_H
