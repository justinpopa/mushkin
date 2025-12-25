#ifndef TEXT_ATTRIBUTES_DIALOG_H
#define TEXT_ATTRIBUTES_DIALOG_H

#include <QCheckBox>
#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

/**
 * TextAttributesDialog - Displays text attributes and styling information
 *
 * Shows comprehensive styling information for a character or selection including:
 * - Text and background colors (names and RGB values)
 * - Color swatches showing actual colors
 * - Bold, italic, and inverse attributes
 * - Custom color information
 * - The character being inspected
 * - Modification status
 *
 * This is a display-only dialog that shows information without allowing edits.
 */
class TextAttributesDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for displaying text attributes
     * @param parent Parent widget
     */
    explicit TextAttributesDialog(QWidget* parent = nullptr);

    ~TextAttributesDialog() override = default;

    // Setters to populate the display fields
    void setTextColour(const QString& colourName);
    void setBackColour(const QString& colourName);
    void setTextColourRGB(const QString& rgb);
    void setBackgroundColourRGB(const QString& rgb);
    void setCustomColour(const QString& customInfo);
    void setLetter(const QString& letter);
    void setBold(bool bold);
    void setInverse(bool inverse);
    void setItalic(bool italic);
    void setModified(const QString& modifiedInfo);

    // Convenience method to set colors from QColor objects
    void setTextColour(const QColor& color);
    void setBackColour(const QColor& color);

  private slots:
    /**
     * Placeholder for future line info functionality
     */
    void onLineInfo();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Update color swatch with the given color
     */
    void updateColorSwatch(QLabel* swatch, const QColor& color);

    // UI Components - Color information
    QLabel* m_textColourLabel;
    QLabel* m_textColourSwatch;
    QLabel* m_backColourLabel;
    QLabel* m_backColourSwatch;
    QLabel* m_textColourRGBLabel;
    QLabel* m_backgroundColourRGBLabel;
    QLabel* m_customColourLabel;

    // UI Components - Character and attributes
    QLabel* m_letterLabel;
    QCheckBox* m_boldCheckBox;
    QCheckBox* m_inverseCheckBox;
    QCheckBox* m_italicCheckBox;
    QLabel* m_modifiedLabel;

    // UI Components - Buttons
    QPushButton* m_lineInfoButton;

    // Stored color values for swatches
    QColor m_textColor;
    QColor m_backColor;
};

#endif // TEXT_ATTRIBUTES_DIALOG_H
