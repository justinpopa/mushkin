#ifndef ALIAS_LIST_DIALOG_H
#define ALIAS_LIST_DIALOG_H

#include "item_list_dialog_base.h"

class WorldDocument;

/**
 * AliasListDialog - Dialog for viewing and managing all aliases
 *
 * Provides a table view of all aliases in the current world with:
 * - Add/Edit/Delete/Enable/Disable buttons
 * - Sortable columns
 * - Double-click to edit
 * - Group operations (enable/disable/delete group)
 */
class AliasListDialog : public ItemListDialogBase {
    Q_OBJECT

  public:
    explicit AliasListDialog(WorldDocument* doc, QWidget* parent = nullptr);

  protected:
    QString itemTypeName() const override
    {
        return QStringLiteral("alias");
    }
    QString itemTypeNamePlural() const override
    {
        return QStringLiteral("aliases");
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
        return COL_MATCH;
    }

  private:
    enum Column {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_MATCH,
        COL_GROUP,
        COL_SEQUENCE,
        COL_SENDTO,
        COL_MATCHED,
        COL_COUNT
    };
};

#endif // ALIAS_LIST_DIALOG_H
