#include "alias_list_dialog.h"
#include "../../automation/alias.h"
#include "../../automation/sendto.h"
#include "../../world/world_document.h"
#include "alias_edit_dialog.h"

AliasListDialog::AliasListDialog(WorldDocument* doc, QWidget* parent)
    : ItemListDialogBase(doc, parent)
{
    setWindowTitle("Configure Aliases - " + doc->m_mush_name);
    resize(900, 600);

    setupUi();
    loadItems();
    updateButtonStates();
}

int AliasListDialog::itemCount() const
{
    return static_cast<int>(m_doc->m_AliasMap.size());
}

QStringList AliasListDialog::itemNames() const
{
    QStringList names;
    for (const auto& [name, aliasPtr] : m_doc->m_AliasMap) {
        names.append(name);
    }
    return names;
}

bool AliasListDialog::itemExists(const QString& name) const
{
    return m_doc->getAlias(name) != nullptr;
}

void AliasListDialog::deleteItem(const QString& name)
{
    m_doc->deleteAlias(name);
}

QString AliasListDialog::getItemGroup(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->strGroup : QString();
}

bool AliasListDialog::getItemEnabled(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->bEnabled : false;
}

void AliasListDialog::setItemEnabled(const QString& name, bool enabled)
{
    Alias* alias = m_doc->getAlias(name);
    if (alias) {
        alias->bEnabled = enabled;
    }
}

void AliasListDialog::populateRow(int row, const QString& name)
{
    Alias* alias = m_doc->getAlias(name);
    if (!alias)
        return;

    setCheckboxItem(row, COL_ENABLED, alias->bEnabled, name);
    setReadOnlyItem(row, COL_LABEL, alias->strLabel);
    setReadOnlyItem(row, COL_MATCH, alias->name);
    setReadOnlyItem(row, COL_GROUP, alias->strGroup);
    setReadOnlyItemWithData(row, COL_SEQUENCE, QString::number(alias->iSequence), alias->iSequence);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(alias->iSendTo));
    setReadOnlyItemWithData(row, COL_MATCHED, QString::number(alias->nMatched), alias->nMatched);
}

bool AliasListDialog::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        AliasEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        AliasEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList AliasListDialog::columnHeaders() const
{
    return {"Enabled", "Label", "Match", "Group", "Seq", "Send To", "Matched"};
}
