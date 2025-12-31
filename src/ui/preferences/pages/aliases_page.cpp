#include "aliases_page.h"
#include "automation/alias.h"
#include "automation/sendto.h"
#include "dialogs/alias_edit_dialog.h"
#include "world/world_document.h"

AliasesPage::AliasesPage(WorldDocument* doc, QWidget* parent)
    : ItemListPageBase(doc, parent)
{
    setupUi();
}

int AliasesPage::itemCount() const
{
    return static_cast<int>(m_doc->m_AliasMap.size());
}

QStringList AliasesPage::itemNames() const
{
    QStringList names;
    for (const auto& [name, aliasPtr] : m_doc->m_AliasMap) {
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
    m_doc->deleteAlias(name);
}

QString AliasesPage::getItemGroup(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->strGroup : QString();
}

bool AliasesPage::getItemEnabled(const QString& name) const
{
    Alias* alias = m_doc->getAlias(name);
    return alias ? alias->bEnabled : false;
}

void AliasesPage::setItemEnabled(const QString& name, bool enabled)
{
    Alias* alias = m_doc->getAlias(name);
    if (alias) {
        alias->bEnabled = enabled;
    }
}

void AliasesPage::populateRow(int row, const QString& name)
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
    return {tr("Enabled"), tr("Label"), tr("Match"), tr("Group"),
            tr("Seq"),     tr("Send To"), tr("Matched")};
}
