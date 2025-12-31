#ifndef AUTOSAY_PAGE_H
#define AUTOSAY_PAGE_H

#include "../preferences_page_base.h"

class QCheckBox;
class QLineEdit;

/**
 * AutoSayPage - Auto-say configuration
 *
 * Configure automatic "say" command prefixing for commands that don't
 * start with a special character.
 */
class AutoSayPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit AutoSayPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Auto Say"); }
    QString pageDescription() const override
    {
        return tr("Configure automatic say command prefixing.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void markChanged();

  private:
    void setupUi();

    // UI components
    QCheckBox* m_enableCheck;
    QLineEdit* m_sayStringEdit;
    QLineEdit* m_overridePrefixEdit;
    QCheckBox* m_excludeMacrosCheck;
    QCheckBox* m_excludeNonAlphaCheck;
    QCheckBox* m_reEvaluateCheck;

    bool m_hasChanges = false;
};

#endif // AUTOSAY_PAGE_H
