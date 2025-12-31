#ifndef UNIFIED_PREFERENCES_DIALOG_H
#define UNIFIED_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QMap>

class QTreeWidget;
class QTreeWidgetItem;
class QStackedWidget;
class QDialogButtonBox;
class QLabel;
class WorldDocument;
class PreferencesPageBase;

/**
 * UnifiedPreferencesDialog - Single dialog for all world configuration
 *
 * Replaces the separate TriggerListDialog, AliasListDialog, TimerListDialog,
 * and WorldPropertiesDialog with a unified interface matching the original
 * MUSHclient's TreePropertySheet pattern.
 *
 * Layout:
 * - Left side: QTreeWidget with expandable groups (General, Appearance, etc.)
 * - Right side: QStackedWidget showing the selected page
 * - Bottom: OK/Cancel/Apply buttons
 *
 * Pages are organized into groups:
 * - General: Connection, Logging, Info
 * - Appearance: Output, Colors, MXP
 * - Automation: Triggers, Aliases, Timers, Macros
 * - Input: Commands, Keypad, Paste/Send
 * - Scripting: Script File, Variables
 */
class UnifiedPreferencesDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Page identifiers for navigation
     */
    enum class Page {
        // General group
        Connection,
        Logging,
        Info,

        // Appearance group
        Output,
        Colors,
        MXP,

        // Automation group
        Triggers,
        Aliases,
        Timers,
        Macros,

        // Input group
        Commands,
        Keypad,
        AutoSay,
        PasteSend,

        // Scripting group
        Scripting,
        Variables
    };

    /**
     * Constructor
     * @param doc WorldDocument to configure
     * @param initialPage Page to show initially
     * @param parent Parent widget
     */
    explicit UnifiedPreferencesDialog(WorldDocument* doc,
                                       Page initialPage = Page::Connection,
                                       QWidget* parent = nullptr);

    ~UnifiedPreferencesDialog() override;

    /**
     * Navigate to a specific page
     * @param page Page to show
     */
    void setCurrentPage(Page page);

    /**
     * Get the currently shown page
     * @return Current page
     */
    Page currentPage() const;

  private slots:
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    void onApplyClicked();
    void onOkClicked();
    void onCancelClicked();
    void onPageSettingsChanged();

  private:
    void setupUi();
    void setupTree();
    void setupPages();
    void connectSignals();

    void addPageItem(QTreeWidgetItem* parent, Page page, const QString& label);
    void selectTreeItem(Page page);
    void updateApplyButton();
    void saveAllPages();

    // UI components
    QTreeWidget* m_tree;
    QStackedWidget* m_stack;
    QDialogButtonBox* m_buttonBox;
    QLabel* m_pageTitle;
    QLabel* m_pageDescription;

    // Page management
    QMap<Page, PreferencesPageBase*> m_pages;
    QMap<Page, QTreeWidgetItem*> m_treeItems;
    QMap<QTreeWidgetItem*, Page> m_itemToPage;

    // Data
    WorldDocument* m_doc;
    Page m_currentPage;
    bool m_hasChanges;
};

#endif // UNIFIED_PREFERENCES_DIALOG_H
