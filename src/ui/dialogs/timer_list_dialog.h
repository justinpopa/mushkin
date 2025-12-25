#ifndef TIMER_LIST_DIALOG_H
#define TIMER_LIST_DIALOG_H

#include "item_list_dialog_base.h"

class WorldDocument;
class Timer;

/**
 * TimerListDialog - Dialog for viewing and managing all timers
 *
 * Provides a table view of all timers in the current world with:
 * - Add/Edit/Delete/Enable/Disable buttons
 * - Sortable columns
 * - Double-click to edit
 * - Group operations (enable/disable/delete group)
 */
class TimerListDialog : public ItemListDialogBase {
    Q_OBJECT

  public:
    explicit TimerListDialog(WorldDocument* doc, QWidget* parent = nullptr);

  protected:
    QString itemTypeName() const override
    {
        return QStringLiteral("timer");
    }
    QString itemTypeNamePlural() const override
    {
        return QStringLiteral("timers");
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
        return COL_TIMING;
    }

  private:
    QString formatTimerTiming(Timer* timer) const;

    enum Column {
        COL_ENABLED = 0,
        COL_LABEL,
        COL_TYPE,
        COL_TIMING,
        COL_GROUP,
        COL_SENDTO,
        COL_FIRED,
        COL_COUNT
    };
};

#endif // TIMER_LIST_DIALOG_H
