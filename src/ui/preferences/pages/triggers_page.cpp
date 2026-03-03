#include "triggers_page.h"
#include "automation/sendto.h"
#include "automation/trigger.h"
#include "dialogs/trigger_edit_dialog.h"
#include "world/world_document.h"

TriggersPage::TriggersPage(WorldDocument* doc, QWidget* parent) : ItemListPageBase(doc, parent)
{
    setupUi();
}

int TriggersPage::itemCount() const
{
    return static_cast<int>(m_doc->m_automationRegistry->m_TriggerMap.size());
}

QStringList TriggersPage::itemNames() const
{
    QStringList names;
    for (const auto& [name, triggerPtr] : m_doc->m_automationRegistry->m_TriggerMap) {
        names.append(name);
    }
    return names;
}

bool TriggersPage::itemExists(const QString& name) const
{
    return m_doc->getTrigger(name) != nullptr;
}

void TriggersPage::deleteItem(const QString& name)
{
    (void)m_doc->deleteTrigger(name); // UI: item selected for deletion; not-found is a no-op
}

QString TriggersPage::getItemGroup(const QString& name) const
{
    Trigger* trigger = m_doc->getTrigger(name);
    return trigger ? trigger->group : QString();
}

bool TriggersPage::getItemEnabled(const QString& name) const
{
    Trigger* trigger = m_doc->getTrigger(name);
    return trigger ? trigger->enabled : false;
}

void TriggersPage::setItemEnabled(const QString& name, bool enabled)
{
    Trigger* trigger = m_doc->getTrigger(name);
    if (trigger) {
        trigger->enabled = enabled;
    }
}

void TriggersPage::populateRow(int row, const QString& name)
{
    Trigger* trigger = m_doc->getTrigger(name);
    if (!trigger)
        return;

    setCheckboxItem(row, COL_ENABLED, trigger->enabled, name);
    setReadOnlyItem(row, COL_LABEL, trigger->label);
    setReadOnlyItem(row, COL_PATTERN, trigger->trigger);
    setReadOnlyItem(row, COL_GROUP, trigger->group);
    setReadOnlyItemWithData(row, COL_SEQUENCE, QString::number(trigger->sequence),
                            trigger->sequence);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(trigger->send_to));
    setReadOnlyItemWithData(row, COL_MATCHED, QString::number(trigger->matched), trigger->matched);
}

bool TriggersPage::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        TriggerEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        TriggerEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList TriggersPage::columnHeaders() const
{
    return {tr("Enabled"), tr("Label"),   tr("Pattern"), tr("Group"),
            tr("Seq"),     tr("Send To"), tr("Matched")};
}
