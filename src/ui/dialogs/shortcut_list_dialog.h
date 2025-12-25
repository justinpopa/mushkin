#ifndef SHORTCUT_LIST_DIALOG_H
#define SHORTCUT_LIST_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

// Forward declarations
class WorldDocument;

/**
 * ShortcutListDialog - Unified keyboard shortcut manager
 *
 * Displays all keyboard shortcuts (user, script, plugin) in a single view.
 * Features:
 * - Search/filter by key or action
 * - Conflict detection and indicator
 * - Add/Edit/Delete for user shortcuts
 * - Enable/Disable shortcuts
 * - Visual distinction between user and runtime shortcuts
 */
class ShortcutListDialog : public QDialog {
    Q_OBJECT

  public:
    explicit ShortcutListDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~ShortcutListDialog() override = default;

  private slots:
    /**
     * Search text changed - filter the list
     */
    void onSearchChanged(const QString& text);

    /**
     * Table selection changed - update button states
     */
    void onSelectionChanged();

    /**
     * Table item double-clicked - edit if user shortcut
     */
    void onItemDoubleClicked(QTableWidgetItem* item);

    /**
     * Add button clicked - open add dialog
     */
    void onAddClicked();

    /**
     * Edit button clicked - open edit dialog
     */
    void onEditClicked();

    /**
     * Delete button clicked - confirm and delete
     */
    void onDeleteClicked();

    /**
     * Enable button clicked
     */
    void onEnableClicked();

    /**
     * Disable button clicked
     */
    void onDisableClicked();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load shortcuts into the table
     */
    void loadShortcuts();

    /**
     * Apply current search filter
     */
    void applyFilter();

    /**
     * Update button enabled states based on selection
     */
    void updateButtonStates();

    /**
     * Update conflict indicator at bottom
     */
    void updateConflictIndicator();

    /**
     * Get the key string of the selected shortcut
     * @return Key string or empty if no selection
     */
    QString getSelectedKeyString() const;

    /**
     * Check if selected shortcut is user-editable
     */
    bool isSelectedUserShortcut() const;

    // Table column indices
    enum Column { ColShortcut = 0, ColAction, ColSendTo, ColSource, ColCount };

    // Member variables
    WorldDocument* m_doc;
    QString m_currentFilter;

    // UI Components
    QLineEdit* m_searchEdit;
    QTableWidget* m_table;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_enableButton;
    QPushButton* m_disableButton;
    QLabel* m_conflictLabel;
};

#endif // SHORTCUT_LIST_DIALOG_H
