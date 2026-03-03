#include "timer_list_dialog.h"
#include "../../automation/sendto.h"
#include "../../automation/timer.h"
#include "../../world/world_document.h"
#include "timer_edit_dialog.h"

TimerListDialog::TimerListDialog(WorldDocument* doc, QWidget* parent)
    : ItemListDialogBase(doc, parent)
{
    setWindowTitle("Configure Timers - " + doc->m_mush_name);
    resize(900, 600);

    setupUi();
    loadItems();
    updateButtonStates();
}

int TimerListDialog::itemCount() const
{
    return static_cast<int>(m_doc->m_automationRegistry->m_TimerMap.size());
}

QStringList TimerListDialog::itemNames() const
{
    QStringList names;
    for (const auto& [name, timerPtr] : m_doc->m_automationRegistry->m_TimerMap) {
        names.append(name);
    }
    return names;
}

bool TimerListDialog::itemExists(const QString& name) const
{
    return m_doc->getTimer(name) != nullptr;
}

void TimerListDialog::deleteItem(const QString& name)
{
    (void)m_doc->deleteTimer(name); // UI: item selected for deletion; not-found is a no-op
}

QString TimerListDialog::getItemGroup(const QString& name) const
{
    Timer* timer = m_doc->getTimer(name);
    return timer ? timer->group : QString();
}

bool TimerListDialog::getItemEnabled(const QString& name) const
{
    Timer* timer = m_doc->getTimer(name);
    return timer ? timer->enabled : false;
}

void TimerListDialog::setItemEnabled(const QString& name, bool enabled)
{
    Timer* timer = m_doc->getTimer(name);
    if (timer) {
        timer->enabled = enabled;
    }
}

QString TimerListDialog::formatTimerTiming(Timer* timer) const
{
    if (timer->type == Timer::TimerType::AtTime) {
        return QString("At %1:%2:%3")
            .arg(timer->at_hour, 2, 10, QChar('0'))
            .arg(timer->at_minute, 2, 10, QChar('0'))
            .arg(timer->at_second, 4, 'f', 1, QChar('0'));
    } else {
        QString result = "Every ";
        if (timer->every_hour > 0)
            result += QString("%1h ").arg(timer->every_hour);
        if (timer->every_minute > 0)
            result += QString("%1m ").arg(timer->every_minute);
        if (timer->every_second > 0)
            result += QString("%1s").arg(timer->every_second, 0, 'f', 1);
        return result.trimmed();
    }
}

void TimerListDialog::populateRow(int row, const QString& name)
{
    Timer* timer = m_doc->getTimer(name);
    if (!timer)
        return;

    setCheckboxItem(row, COL_ENABLED, timer->enabled, name);
    setReadOnlyItem(row, COL_LABEL, timer->label);
    setReadOnlyItem(row, COL_TYPE,
                    timer->type == Timer::TimerType::AtTime ? "At Time" : "Interval");
    setReadOnlyItem(row, COL_TIMING, formatTimerTiming(timer));
    setReadOnlyItem(row, COL_GROUP, timer->group);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(timer->send_to));
    setReadOnlyItemWithData(row, COL_FIRED, QString::number(timer->matched), timer->matched);
}

bool TimerListDialog::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        TimerEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        TimerEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList TimerListDialog::columnHeaders() const
{
    return {"Enabled", "Label", "Type", "Timing", "Group", "Send To", "Fired"};
}
