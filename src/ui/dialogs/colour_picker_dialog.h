#ifndef COLOUR_PICKER_DIALOG_H
#define COLOUR_PICKER_DIALOG_H

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVector>

// Forward declarations
class WorldDocument;

/**
 * ColourPickerDialog - Color selection dialog
 *
 * Provides a dialog for selecting colors from:
 * - 16 standard ANSI colors (8 normal + 8 bold)
 * - 16 custom color slots
 * - Custom color picker (QColorDialog)
 *
 * Based on MUSHclient's ColourPickerDlg - allows users to pick colors
 * for triggers, text, and other UI elements.
 */
class ColourPickerDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument (can be nullptr if not using custom colors)
     * @param initialColor Initial color to show/select
     * @param parent Parent widget
     */
    explicit ColourPickerDialog(WorldDocument* doc, QRgb initialColor = qRgb(255, 255, 255),
                                QWidget* parent = nullptr);

    ~ColourPickerDialog() override = default;

    /**
     * Get the selected color
     * @return Selected RGB color value
     */
    QRgb getSelectedColor() const
    {
        return m_selectedColor;
    }

  private slots:
    /**
     * Called when an ANSI color button is clicked
     */
    void onAnsiColorClicked();

    /**
     * Called when a custom color button is clicked
     */
    void onCustomColorClicked();

    /**
     * Called when "Pick Color" button is clicked - opens color picker
     */
    void onPickColorClicked();

    /**
     * Called when OK is clicked
     */
    void onOk();

    /**
     * Called when Cancel is clicked
     */
    void onCancel();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Create a color button with the given color
     * @param color RGB color value
     * @param tooltip Tooltip text
     * @return QPushButton configured as a color swatch
     */
    QPushButton* createColorButton(QRgb color, const QString& tooltip);

    /**
     * Update the current color swatch display
     */
    void updateColorSwatch();

    /**
     * Select a color (updates swatch and stores selection)
     * @param color RGB color value
     */
    void selectColor(QRgb color);

    // Member variables
    WorldDocument* m_doc;
    QRgb m_selectedColor;

    // UI Components
    QLabel* m_colorSwatch;    // Large swatch showing current color
    QLabel* m_colorInfoLabel; // Shows RGB values and color name
    QPushButton* m_pickColorButton;
    QVector<QPushButton*> m_ansiButtons;   // ANSI color buttons
    QVector<QPushButton*> m_customButtons; // Custom color buttons
};

#endif // COLOUR_PICKER_DIALOG_H
