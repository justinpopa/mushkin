#include "aliases_page.h"
#include "automation/alias.h"
#include "automation/sendto.h"
#include "dialogs/alias_edit_dialog.h"
#include "world/world_document.h"

AliasesPage::AliasesPage(WorldDocument* doc, QWidget* parent) : ItemListPageBase(doc, parent)
{
    setupUi();
}

int AliasesPage::itemCount() const
{
    return static_cast<int>(m_doc->m_automationRegistry->m_AliasMap.size());
}

QStringList AliasesPage::itemNames() const
{
    QStringList names;
    for (const auto& [name, aliasPtr] : m_doc->m_automationRegistry->m_AliasMap) {
        names.append(name);
    }
    return names;
}

bool AliasesPage::itemExists(const QString& name) const
{
    return m_doc->getAlias(name) != nullptr;
}

void AliasesPage::deleteItem(const QString& name)
{
    (void)m_doc->deleteAlias(name); // UI: item selected for deletion; not-found is a no-op
}

QString AliasesPage::getItemGroup(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->group : QString();
}

bool AliasesPage::getItemEnabled(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->enabled : false;
}

void AliasesPage::setItemEnabled(const QString& name, bool enabled)
{
    Alias* alias = m_doc->getAlias(name);
    if (alias) {
        alias->enabled = enabled;
    }
}

void AliasesPage::populateRow(int row, const QString& name)
{
    Alias* alias = m_doc->getAlias(name);
    if (!alias)
        return;

    setCheckboxItem(row, COL_ENABLED, alias->enabled, name);
    setReadOnlyItem(row, COL_LABEL, alias->label);
    setReadOnlyItem(row, COL_MATCH, alias->name);
    setReadOnlyItem(row, COL_GROUP, alias->group);
    setReadOnlyItemWithData(row, COL_SEQUENCE, QString::number(alias->sequence), alias->sequence);
    setReadOnlyItem(row, COL_SENDTO, sendToDisplayName(alias->send_to));
    setReadOnlyItemWithData(row, COL_MATCHED, QString::number(alias->matched), alias->matched);
}

bool AliasesPage::openEditDialog(const QString& name)
{
    if (name.isEmpty()) {
        AliasEditDialog dialog(m_doc, this);
        return dialog.exec() == QDialog::Accepted;
    } else {
        AliasEditDialog dialog(m_doc, name, this);
        return dialog.exec() == QDialog::Accepted;
    }
}

QStringList AliasesPage::columnHeaders() const
{
    return {tr("Enabled"), tr("Label"),   tr("Match"),  tr("Group"),
            tr("Seq"),     tr("Send To"), tr("Matched")};
}
