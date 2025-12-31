#ifndef COLORS_PAGE_H
#define COLORS_PAGE_H

#include "../preferences_page_base.h"

class QTableWidget;
class QPushButton;

/**
 * ColorsPage - Custom color mappings
 *
 * Configure the 16 custom color pairs (text and background)
 * used by triggers and other features.
 */
class ColorsPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit ColorsPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Colors"); }
    QString pageDescription() const override
    {
        return tr("Configure custom color mappings for triggers and display.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onTextColorClicked();
    void onBackColorClicked();
    void onNameChanged();
    void markChanged();

  private:
    void setupUi();
    void updateColorCell(int row, int col, QRgb color);

    // Custom colors table
    QTableWidget* m_table;

    // Color storage
    QRgb m_customText[16];
    QRgb m_customBack[16];
    QString m_customNames[16];

    // Track changes
    bool m_hasChanges = false;
};

#endif // COLORS_PAGE_H
