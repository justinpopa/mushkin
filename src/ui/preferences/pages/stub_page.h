#ifndef STUB_PAGE_H
#define STUB_PAGE_H

#include "../preferences_page_base.h"

/**
 * StubPage - Placeholder page for unimplemented preferences pages
 *
 * Shows the page name and a "Coming soon" message. Used during development
 * to allow the dialog navigation to work before all pages are implemented.
 */
class StubPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    StubPage(WorldDocument* doc, const QString& name, const QString& description,
             QWidget* parent = nullptr);

    QString pageName() const override { return m_name; }
    QString pageDescription() const override { return m_description; }
    void loadSettings() override {}
    void saveSettings() override {}
    bool hasChanges() const override { return false; }

  private:
    QString m_name;
    QString m_description;
};

#endif // STUB_PAGE_H
