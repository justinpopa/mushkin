#ifndef ALIASES_PAGE_H
#define ALIASES_PAGE_H

#include "../item_list_page_base.h"

/**
 * AliasesPage - Alias list management in unified preferences
 *
 * Displays all aliases for the world with CRUD operations.
 * Based on AliasListDialog, adapted for the unified preferences dialog.
 */
class AliasesPage : public ItemListPageBase {
    Q_OBJECT

  public:
    explicit AliasesPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Aliases"); }
    QString pageDescription() const override
    {
        return tr("Manage command aliases that expand to other commands or scripts.");
    }

  protected:
    QString itemTypeName() const override { return tr("alias"); }
    QString itemTypeNamePlural() const override { return tr("aliases"); }
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
    int stretchColumn() const override { return COL_MATCH; }

  private:
    enum Columns {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_MATCH,
        COL_GROUP,
        COL_SEQUENCE,
        COL_SENDTO,
        COL_MATCHED
    };
};

#endif // ALIASES_PAGE_H
