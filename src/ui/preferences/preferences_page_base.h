#ifndef PREFERENCES_PAGE_BASE_H
#define PREFERENCES_PAGE_BASE_H

#include <QWidget>

class WorldDocument;

/**
 * PreferencesPageBase - Abstract base class for unified preferences dialog pages
 *
 * Each page in the unified preferences dialog inherits from this class.
 * Pages can be either settings pages (forms with fields) or list pages
 * (tables with CRUD operations for triggers, aliases, timers, etc.).
 *
 * Based on the original MUSHclient's TreePropertySheet pattern where all
 * world configuration is accessible from a single dialog with tree navigation.
 */
class PreferencesPageBase : public QWidget {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument this page configures
     * @param parent Parent widget
     */
    explicit PreferencesPageBase(WorldDocument* doc, QWidget* parent = nullptr);

    virtual ~PreferencesPageBase() = default;

    /**
     * Get the page name for display in tree and title
     * @return Page name (e.g., "Triggers", "Connection")
     */
    virtual QString pageName() const = 0;

    /**
     * Get a brief description of what this page configures
     * @return Description text
     */
    virtual QString pageDescription() const = 0;

    /**
     * Load settings from WorldDocument into the UI
     * Called when the page becomes visible
     */
    virtual void loadSettings() = 0;

    /**
     * Save settings from UI back to WorldDocument
     * Called when OK/Apply is clicked
     */
    virtual void saveSettings() = 0;

    /**
     * Check if the page has unsaved changes
     * @return true if there are unsaved changes
     */
    virtual bool hasChanges() const = 0;

    /**
     * Get the WorldDocument this page is editing
     * @return WorldDocument pointer
     */
    WorldDocument* document() const { return m_doc; }

  signals:
    /**
     * Emitted when settings on this page have been modified
     */
    void settingsChanged();

  protected:
    WorldDocument* m_doc;
};

#endif // PREFERENCES_PAGE_BASE_H
