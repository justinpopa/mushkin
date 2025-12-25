#include "trigger_list_dialog.h"
#include "../../automation/sendto.h"
#include "../../automation/trigger.h"
#include "../../world/world_document.h"
#include "trigger_edit_dialog.h"

TriggerListDialog::TriggerListDialog(WorldDocument* doc, QWidget* parent)
    : ItemListDialogBase(doc, parent)
{
    setWindowTitle("Configure Triggers - " + doc->m_mush_name);
    resize(900, 600);

    setupUi();
    loadItems();
    updateButtonStates();
}

int TriggerListDialog::itemCount() const
{
    return static_cast<int>(m_doc->m_TriggerMap.size());
}

QStringList TriggerListDialog::itemNames() const
{
    QStringList names;
    for (const auto& [name, triggerPtr] : m_doc->m_TriggerMap) {
        names.append(name);
    }
    return names;
}

bool TriggerListDialog::itemExists(const QString& name) const
{
    return m_doc->getTrigger(name) != nullptr;
}

void TriggerListDialog::deleteItem(const QString& name)
{
    m_doc->deleteTrigger(name);
}

QString TriggerListDialog::getItemGroup(const QString& name) const
{
    Trigger* trigger = m_doc->getTrigger(name);
    return trigger ? trigger->strGroup : QString();
}

bool TriggerListDialog::getItemEnabled(const QString& name) const
{
    Trigger* trigger = m_doc->getTrigger(name);
    return trigger ? trigger->bEnabled : false;
}

void TriggerListDialog::setItemEnabled(const QString& name, bool enabled)
{
    Trigger* trigger = m_doc->getTrigger(name);
    if (trigger) {
        trigger->bEnabled = enabled;
    }
}

void TriggerListDialog::populateRow(int row, const QString& name)
{
    Trigger* trigger = m_doc->getTrigger(name);
    if (!trigger)
        return;

    setCheckboxItem(row, COL_ENABLED, trigger->bEnabled, name);
    setReadOnlyItem(row, COL_LABEL, trigger->strLabel);
    setReadOnlyItem(row, COL_PATTERN, trigger->trigger);
    setReadOnlyItem(row, COL_GROUP, trigger->strGroup);
    setReadOnlyItemWithData(row, COL_SEQUENCE, QString::number(trigger->iSequence),
                            trigger->iSequence);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(trigger->iSendTo));
    setReadOnlyItemWithData(row, COL_MATCHED, QString::number(trigger->nMatched),
                            trigger->nMatched);
}

bool TriggerListDialog::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        TriggerEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        TriggerEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList TriggerListDialog::columnHeaders() const
{
    return {"Enabled", "Label", "Pattern", "Group", "Seq", "Send To", "Matched"};
}
