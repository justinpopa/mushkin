#ifndef INPUT_PAGE_H
#define INPUT_PAGE_H

#include "../preferences_page_base.h"
#include <QFont>

class QLabel;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QComboBox;

/**
 * InputPage - Command input settings
 *
 * Configure input font, echo settings, command history,
 * and input behavior options.
 *
 * Matches original MUSHclient Page 9 (Input) with sub-dialogs for:
 * - Command Options (double-click, arrow keys, keyboard shortcuts)
 * - Tab Completion Defaults (word list, lines to search)
 */
class InputPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit InputPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Commands"); }
    QString pageDescription() const override
    {
        return tr("Configure command input behavior, history, and display options.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onInputFontButtonClicked();
    void onCommandOptionsClicked();
    void onTabDefaultsClicked();
    void markChanged();

  private:
    void setupUi();

    // Font selection
    QPushButton* m_inputFontButton;
    QLabel* m_inputFontLabel;
    QFont m_inputFont;

    // Echo settings
    QCheckBox* m_echoInputCheck;
    QComboBox* m_echoColorCombo;

    // History settings
    QSpinBox* m_historySizeSpin;
    QCheckBox* m_duplicateHistoryCheck;
    QCheckBox* m_arrowHistoryCheck;

    // Input behavior
    QCheckBox* m_autoRepeatCheck;
    QCheckBox* m_escClearCheck;
    QCheckBox* m_doubleClickSelectCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // INPUT_PAGE_H
