#ifndef TRIGGERS_PAGE_H
#define TRIGGERS_PAGE_H

#include "../item_list_page_base.h"

/**
 * TriggersPage - Trigger list management in unified preferences
 *
 * Displays all triggers for the world with CRUD operations.
 * Based on TriggerListDialog, adapted for the unified preferences dialog.
 */
class TriggersPage : public ItemListPageBase {
    Q_OBJECT

  public:
    explicit TriggersPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Triggers"); }
    QString pageDescription() const override
    {
        return tr("Manage triggers that respond to MUD output with actions, colors, or scripts.");
    }

  protected:
    QString itemTypeName() const override { return tr("trigger"); }
    QString itemTypeNamePlural() const override { return tr("triggers"); }
    int itemCount() const override;
    QStringList itemNames() const override;
    bool itemExists(const QString& name) const override;
    void deleteItem(const QString& name) override;
    QString getItemGroup(const QString& name) const override;
    bool getItemEnabled(const QString& name) const override;
    void setItemEnabled(const QString& name, bool enabled) override;
    void populateRow(int row, const QString& name) override;
    bool openEditDialog(const QString& name = QString()) override;
    int columnCount() const override { return 7; }
    QStringList columnHeaders() const override;
    int stretchColumn() const override { return COL_PATTERN; }

  private:
    enum Columns {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_PATTERN,
        COL_GROUP,
        COL_SEQUENCE,
        COL_SENDTO,
        COL_MATCHED
    };
};

#endif // TRIGGERS_PAGE_H
