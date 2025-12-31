#ifndef SCRIPTING_PAGE_H
#define SCRIPTING_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;

/**
 * ScriptingPage - Script file configuration
 *
 * Configure the main script file, scripting language,
 * and script-related options.
 */
class ScriptingPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit ScriptingPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Script File"); }
    QString pageDescription() const override
    {
        return tr("Configure script file and scripting language settings.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onBrowseClicked();
    void markChanged();

  private:
    void setupUi();

    // Script settings
    QCheckBox* m_enableScriptCheck;
    QLineEdit* m_scriptFileEdit;
    QPushButton* m_browseButton;
    QComboBox* m_languageCombo;
    QCheckBox* m_autoReloadCheck;
    QCheckBox* m_warnIfNoHandlerCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // SCRIPTING_PAGE_H
