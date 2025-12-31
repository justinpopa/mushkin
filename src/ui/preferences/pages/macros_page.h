#ifndef MACROS_PAGE_H
#define MACROS_PAGE_H

#include "../preferences_page_base.h"

class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;

/**
 * MacrosPage - Keyboard macro/accelerator management
 *
 * Displays and manages keyboard shortcuts/macros with add/edit/delete
 * support for user-defined shortcuts and conflict detection.
 */
class MacrosPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit MacrosPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Macros"); }
    QString pageDescription() const override
    {
        return tr("Manage keyboard macros and accelerators.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void onSearchChanged(const QString& text);
    void onSelectionChanged();
    void onItemDoubleClicked(QTableWidgetItem* item);
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onEnableClicked();
    void onDisableClicked();

  private:
    void setupUi();
    void loadShortcuts();
    void applyFilter();
    void updateButtonStates();
    void updateConflictIndicator();
    QString getSelectedKeyString() const;
    bool isSelectedUserShortcut() const;

    // Table column indices
    enum Column { ColShortcut = 0, ColAction, ColSendTo, ColSource, ColCount };

    // UI components
    QLineEdit* m_searchEdit;
    QTableWidget* m_table;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_enableButton;
    QPushButton* m_disableButton;
    QLabel* m_conflictLabel;

    QString m_currentFilter;

    // Track changes
    bool m_hasChanges = false;
};

#endif // MACROS_PAGE_H
