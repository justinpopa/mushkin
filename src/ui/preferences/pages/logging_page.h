#ifndef LOGGING_PAGE_H
#define LOGGING_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;

/**
 * LoggingPage - Logging configuration settings
 *
 * Configure automatic logging to file, log format,
 * and related options.
 */
class LoggingPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit LoggingPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Logging"); }
    QString pageDescription() const override
    {
        return tr("Configure log file settings and automatic logging.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onBrowseClicked();
    void markChanged();

  private:
    void setupUi();

    // Logging options
    QCheckBox* m_enableLogCheck;
    QLineEdit* m_logFileEdit;
    QPushButton* m_browseButton;
    QComboBox* m_logFormatCombo;
    QCheckBox* m_appendLogCheck;
    QCheckBox* m_logInputCheck;
    QCheckBox* m_logNotesCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // LOGGING_PAGE_H
