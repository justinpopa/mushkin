#ifndef HIGHLIGHT_PHRASE_DIALOG_H
#define HIGHLIGHT_PHRASE_DIALOG_H

#include <QColor>
#include <QDialog>

// Forward declarations
class QLineEdit;
class QCheckBox;
class QPushButton;
class QLabel;
class QDialogButtonBox;

/**
 * HighlightPhraseDialog - Configure text highlighting
 *
 * Provides a dialog for configuring text phrase highlighting in the output window.
 * Allows users to specify:
 * - Text to highlight
 * - Match options (whole word, case sensitivity)
 * - Text color
 * - Background color
 *
 * This is a standalone dialog that doesn't require a WorldDocument.
 */
class HighlightPhraseDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit HighlightPhraseDialog(QWidget* parent = nullptr);
    ~HighlightPhraseDialog() override;

    /**
     * Get the text to highlight
     * @return Text phrase
     */
    QString text() const;

    /**
     * Get match whole word setting
     * @return true if should match whole words only
     */
    bool matchWholeWord() const;

    /**
     * Get match case setting
     * @return true if matching should be case-sensitive
     */
    bool matchCase() const;

    /**
     * Get the text color
     * @return Selected text color
     */
    QColor textColor() const;

    /**
     * Get the background color
     * @return Selected background color
     */
    QColor backgroundColor() const;

  private slots:
    /**
     * Text color button clicked - open color picker
     */
    void onTextColorButtonClicked();

    /**
     * Background color button clicked - open color picker
     */
    void onBackgroundColorButtonClicked();

  private:
    // Setup methods
    void setupUi();

    /**
     * Update color preview widget
     * @param preview The label widget to update
     * @param color The color to display
     */
    void updateColorPreview(QLabel* preview, const QColor& color);

    // UI widgets
    QLineEdit* m_textEdit;
    QCheckBox* m_matchWholeWordCheck;
    QCheckBox* m_matchCaseCheck;
    QPushButton* m_textColorButton;
    QPushButton* m_backgroundColorButton;
    QLabel* m_textColorPreview;
    QLabel* m_backgroundColorPreview;
    QDialogButtonBox* m_buttonBox;

    // Color values
    QColor m_textColor;
    QColor m_backgroundColor;
};

#endif // HIGHLIGHT_PHRASE_DIALOG_H
