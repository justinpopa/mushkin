#ifndef INFO_PAGE_H
#define INFO_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QTextEdit;
class QLabel;

/**
 * InfoPage - World information and notes
 *
 * View and edit world name, file path, unique ID,
 * and free-form notes about the world.
 */
class InfoPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit InfoPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Info"); }
    QString pageDescription() const override
    {
        return tr("View and edit world information and notes.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void markChanged();

  private:
    void setupUi();

    // World info (read-only)
    QLabel* m_worldIdLabel;
    QLabel* m_filePathLabel;

    // Editable fields
    QLineEdit* m_worldNameEdit;
    QTextEdit* m_notesEdit;

    // Track changes
    bool m_hasChanges = false;
};

#endif // INFO_PAGE_H
