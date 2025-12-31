#ifndef VARIABLES_PAGE_H
#define VARIABLES_PAGE_H

#include "../preferences_page_base.h"

class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QPushButton;

/**
 * VariablesPage - Script variables management
 *
 * View and manage script variables stored in the world file.
 */
class VariablesPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit VariablesPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Variables"); }
    QString pageDescription() const override
    {
        return tr("View and manage script variables.");
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

  private:
    void setupUi();
    void loadVariables();
    void applyFilter();
    void updateButtonStates();
    QString getSelectedVariableName() const;

    // Table column indices
    enum Column { ColName = 0, ColValue, ColCount };

    // UI components
    QLineEdit* m_searchEdit;
    QTableWidget* m_table;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;

    QString m_currentFilter;

    // Track changes
    bool m_hasChanges = false;
};

#endif // VARIABLES_PAGE_H
