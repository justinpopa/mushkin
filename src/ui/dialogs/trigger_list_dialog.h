#ifndef TRIGGER_LIST_DIALOG_H
#define TRIGGER_LIST_DIALOG_H

#include "item_list_dialog_base.h"

class WorldDocument;

/**
 * TriggerListDialog - Dialog for viewing and managing all triggers
 *
 * Provides a table view of all triggers in the current world with:
 * - Add/Edit/Delete/Enable/Disable buttons
 * - Sortable columns
 * - Double-click to edit
 * - Group operations (enable/disable/delete group)
 */
class TriggerListDialog : public ItemListDialogBase {
    Q_OBJECT

  public:
    explicit TriggerListDialog(WorldDocument* doc, QWidget* parent = nullptr);

  protected:
    QString itemTypeName() const override
    {
        return QStringLiteral("trigger");
    }
    QString itemTypeNamePlural() const override
    {
        return QStringLiteral("triggers");
    }
    int itemCount() const override;
    QStringList itemNames() const override;
    bool itemExists(const QString& name) const override;
    void deleteItem(const QString& name) override;
    QString getItemGroup(const QString& name) const override;
    bool getItemEnabled(const QString& name) const override;
    void setItemEnabled(const QString& name, bool enabled) override;
    void populateRow(int row, const QString& name) override;
    bool openEditDialog(const QString& name = QString()) override;
    int columnCount() const override
    {
        return COL_COUNT;
    }
    QStringList columnHeaders() const override;
    int stretchColumn() const override
    {
        return COL_PATTERN;
    }

  private:
    enum Column {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_PATTERN,
        COL_GROUP,
        COL_SEQUENCE,
        COL_SENDTO,
        COL_MATCHED,
        COL_COUNT
    };
};

#endif // TRIGGER_LIST_DIALOG_H
