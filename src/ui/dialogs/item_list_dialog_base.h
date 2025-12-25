/**
 * item_list_dialog_base.h - Base class for item list management dialogs
 *
 * Provides common functionality for Alias/Timer/Trigger list dialogs
 * to reduce code duplication.
 */

#ifndef ITEM_LIST_DIALOG_BASE_H
#define ITEM_LIST_DIALOG_BASE_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QStringList>
#include <QTableWidget>

class WorldDocument;

/**
 * Base class for item list management dialogs (triggers, aliases, timers).
 *
 * Subclasses must implement:
 * - itemTypeName() - returns "alias", "timer", or "trigger"
 * - itemTypeNamePlural() - returns "aliases", "timers", or "triggers"
 * - getItemMap() - returns const reference to item map
 * - getItem(name) - returns item by name
 * - deleteItem(name) - deletes item by name
 * - getItemGroup(name) - returns group name for item
 * - setItemEnabled(name, enabled) - enables/disables item
 * - populateRow(row, name, item) - fills table row with item data
 * - createEditDialog(name) - creates appropriate edit dialog
 * - columnCount() / columnHeaders() - table column configuration
 */
class ItemListDialogBase : public QDialog {
    Q_OBJECT

  public:
    explicit ItemListDialogBase(WorldDocument* doc, QWidget* parent = nullptr);
    virtual ~ItemListDialogBase() = default;

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

    WorldDocument* m_doc;
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
    QPushButton* m_closeButton;

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
    void onClose();
};

#endif // ITEM_LIST_DIALOG_BASE_H
