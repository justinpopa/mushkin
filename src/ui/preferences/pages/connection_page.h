#ifndef CONNECTION_PAGE_H
#define CONNECTION_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QSpinBox;
class QCheckBox;

/**
 * ConnectionPage - Server connection settings
 *
 * Configure server address, port, character name, password, and
 * connection behavior for the world.
 */
class ConnectionPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit ConnectionPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Connection"); }
    QString pageDescription() const override
    {
        return tr("Configure server address, port, and connection options.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private:
    void setupUi();

    // UI elements
    QLineEdit* m_serverEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_nameEdit;
    QLineEdit* m_passwordEdit;
    QCheckBox* m_autoConnectCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // CONNECTION_PAGE_H
