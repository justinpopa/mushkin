#ifndef TIMERS_PAGE_H
#define TIMERS_PAGE_H

#include "../item_list_page_base.h"

class Timer;

/**
 * TimersPage - Timer list management in unified preferences
 *
 * Displays all timers for the world with CRUD operations.
 * Based on TimerListDialog, adapted for the unified preferences dialog.
 */
class TimersPage : public ItemListPageBase {
    Q_OBJECT

  public:
    explicit TimersPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Timers"); }
    QString pageDescription() const override
    {
        return tr("Manage timed actions that fire at intervals or specific times.");
    }

  protected:
    QString itemTypeName() const override { return tr("timer"); }
    QString itemTypeNamePlural() const override { return tr("timers"); }
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
    int stretchColumn() const override { return COL_TIMING; }

  private:
    QString formatTimerTiming(Timer* timer) const;

    enum Columns {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_TYPE,
        COL_TIMING,
        COL_GROUP,
        COL_SENDTO,
        COL_FIRED
    };
};

#endif // TIMERS_PAGE_H
