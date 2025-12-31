#ifndef ITEM_LIST_PAGE_BASE_H
#define ITEM_LIST_PAGE_BASE_H

#include "preferences_page_base.h"
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QStringList>
#include <QTableWidget>

/**
 * ItemListPageBase - Base class for list-based preferences pages
 *
 * Provides common functionality for Triggers, Aliases, Timers, and Macros pages.
 * Each page displays a table of items with CRUD operations.
 *
 * Subclasses must implement:
 * - itemTypeName() - singular name ("trigger", "alias", etc.)
 * - itemTypeNamePlural() - plural name ("triggers", "aliases", etc.)
 * - itemCount() / itemNames() / itemExists() - item enumeration
 * - deleteItem() / getItemGroup() / getItemEnabled() / setItemEnabled()
 * - populateRow() - fill table row with item data
 * - openEditDialog() - open editor for item
 * - columnCount() / columnHeaders() / stretchColumn() - table configuration
 */
class ItemListPageBase : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit ItemListPageBase(WorldDocument* doc, QWidget* parent = nullptr);
    ~ItemListPageBase() override = default;

    // PreferencesPageBase interface
    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override { return false; } // List pages save immediately

  protected:
    // Subclasses must implement these
    virtual QString itemTypeName() const = 0;
    virtual QString itemTypeNamePlural() const = 0;
    virtual int itemCount() const = 0;
    virtual QStringList itemNames() const = 0;
    virtual bool itemExists(const QString& name) const = 0;
    virtual void deleteItem(const QString& name) = 0;
    virtual QString getItemGroup(const QString& name) const = 0;
    virtual bool getItemEnabled(const QString& name) const = 0;
    virtual void setItemEnabled(const QString& name, bool enabled) = 0;
    virtual void populateRow(int row, const QString& name) = 0;
    virtual bool openEditDialog(const QString& name = QString()) = 0;
    virtual int columnCount() const = 0;
    virtual QStringList columnHeaders() const = 0;
    virtual int stretchColumn() const = 0;

    // Shared implementation
    void setupUi();
    void loadItems();
    void updateButtonStates();
    QString getSelectedItemName() const;
    QStringList getSelectedItemNames() const;

    // Helper for subclasses
    void setReadOnlyItem(int row, int col, const QString& text);
    void setReadOnlyItemWithData(int row, int col, const QString& text, const QVariant& data);
    void setCheckboxItem(int row, int col, bool checked, const QString& internalName);

    QTableWidget* m_table;
    QLabel* m_infoLabel;

    // Buttons
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_enableButton;
    QPushButton* m_disableButton;
    QPushButton* m_enableGroupButton;
    QPushButton* m_disableGroupButton;
    QPushButton* m_deleteGroupButton;

  protected slots:
    void onAddItem();
    void onEditItem();
    void onDeleteItem();
    void onEnableItem();
    void onDisableItem();
    void onEnableGroup();
    void onDisableGroup();
    void onDeleteGroup();
    void onItemDoubleClicked(int row, int column);
    void onSelectionChanged();
};

#endif // ITEM_LIST_PAGE_BASE_H
