#ifndef OUTPUT_PAGE_H
#define OUTPUT_PAGE_H

#include "../preferences_page_base.h"
#include <QFont>

class QLabel;
class QPushButton;
class QCheckBox;
class QSpinBox;

/**
 * OutputPage - Output window display settings
 *
 * Configure output font, ANSI colors, wrap settings,
 * and activity notification options.
 */
class OutputPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit OutputPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Output"); }
    QString pageDescription() const override
    {
        return tr("Configure output window appearance, fonts, and display options.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onOutputFontButtonClicked();
    void onColorButtonClicked();

  private:
    void setupUi();
    void updateColorButton(int index);
    void markChanged();

    // Font selection
    QPushButton* m_outputFontButton;
    QLabel* m_outputFontLabel;
    QFont m_outputFont;

    // ANSI color palette (16 colors: 8 normal + 8 bright)
    QPushButton* m_colorButtons[16];
    QRgb m_ansiColors[16];

    // Display options
    QCheckBox* m_wordWrapCheck;   // Enable word-wrap at spaces (m_wrap)
    QSpinBox* m_wrapColumnSpin;   // Wrap column width (m_nWrapColumn)
    QCheckBox* m_showBoldCheck;
    QCheckBox* m_showItalicCheck;
    QCheckBox* m_showUnderlineCheck;

    // Activity notification
    QCheckBox* m_flashIconCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // OUTPUT_PAGE_H
