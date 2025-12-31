#include "timers_page.h"
#include "automation/sendto.h"
#include "automation/timer.h"
#include "dialogs/timer_edit_dialog.h"
#include "world/world_document.h"

TimersPage::TimersPage(WorldDocument* doc, QWidget* parent)
    : ItemListPageBase(doc, parent)
{
    setupUi();
}

int TimersPage::itemCount() const
{
    return static_cast<int>(m_doc->m_TimerMap.size());
}

QStringList TimersPage::itemNames() const
{
    QStringList names;
    for (const auto& [name, timerPtr] : m_doc->m_TimerMap) {
        names.append(name);
    }
    return names;
}

bool TimersPage::itemExists(const QString& name) const
{
    return m_doc->getTimer(name) != nullptr;
}

void TimersPage::deleteItem(const QString& name)
{
    m_doc->deleteTimer(name);
}

QString TimersPage::getItemGroup(const QString& name) const
{
    Timer* timer = m_doc->getTimer(name);
    return timer ? timer->strGroup : QString();
}

bool TimersPage::getItemEnabled(const QString& name) const
{
    Timer* timer = m_doc->getTimer(name);
    return timer ? timer->bEnabled : false;
}

void TimersPage::setItemEnabled(const QString& name, bool enabled)
{
    Timer* timer = m_doc->getTimer(name);
    if (timer) {
        timer->bEnabled = enabled;
    }
}

QString TimersPage::formatTimerTiming(Timer* timer) const
{
    if (timer->iType == Timer::eAtTime) {
        return QString("At %1:%2:%3")
            .arg(timer->iAtHour, 2, 10, QChar('0'))
            .arg(timer->iAtMinute, 2, 10, QChar('0'))
            .arg(timer->fAtSecond, 4, 'f', 1, QChar('0'));
    } else {
        QString result = "Every ";
        if (timer->iEveryHour > 0)
            result += QString("%1h ").arg(timer->iEveryHour);
        if (timer->iEveryMinute > 0)
            result += QString("%1m ").arg(timer->iEveryMinute);
        if (timer->fEverySecond > 0)
            result += QString("%1s").arg(timer->fEverySecond, 0, 'f', 1);
        return result.trimmed();
    }
}

void TimersPage::populateRow(int row, const QString& name)
{
    Timer* timer = m_doc->getTimer(name);
    if (!timer)
        return;

    setCheckboxItem(row, COL_ENABLED, timer->bEnabled, name);
    setReadOnlyItem(row, COL_LABEL, timer->strLabel);
    setReadOnlyItem(row, COL_TYPE, timer->iType == Timer::eAtTime ? tr("At Time") : tr("Interval"));
    setReadOnlyItem(row, COL_TIMING, formatTimerTiming(timer));
    setReadOnlyItem(row, COL_GROUP, timer->strGroup);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(timer->iSendTo));
    setReadOnlyItemWithData(row, COL_FIRED, QString::number(timer->nMatched), timer->nMatched);
}

bool TimersPage::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        TimerEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        TimerEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList TimersPage::columnHeaders() const
{
    return {tr("Enabled"), tr("Label"), tr("Type"), tr("Timing"),
            tr("Group"),   tr("Send To"), tr("Fired")};
}
