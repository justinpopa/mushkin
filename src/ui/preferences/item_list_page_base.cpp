#include "item_list_page_base.h"
#include "world/world_document.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

ItemListPageBase::ItemListPageBase(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent), m_table(nullptr), m_infoLabel(nullptr),
      m_addButton(nullptr), m_editButton(nullptr), m_deleteButton(nullptr),
      m_enableButton(nullptr), m_disableButton(nullptr), m_enableGroupButton(nullptr),
      m_disableGroupButton(nullptr), m_deleteGroupButton(nullptr)
{
}

void ItemListPageBase::loadSettings()
{
    loadItems();
    updateButtonStates();
}

void ItemListPageBase::saveSettings()
{
    // List pages save changes immediately through edit dialogs
}

void ItemListPageBase::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Info label
    m_infoLabel = new QLabel(this);
    mainLayout->addWidget(m_infoLabel);

    // Item table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(columnCount());
    m_table->setHorizontalHeaderLabels(columnHeaders());
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(stretchColumn(), QHeaderView::Stretch);
    m_table->setAlternatingRowColors(true);

    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            &ItemListPageBase::onItemDoubleClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            &ItemListPageBase::onSelectionChanged);

    mainLayout->addWidget(m_table, 1);

    // Buttons layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // Single item operations
    QString typeName = itemTypeName();
    typeName[0] = typeName[0].toUpper(); // Capitalize

    QGroupBox* singleGroup = new QGroupBox(typeName + " Operations", this);
    QHBoxLayout* singleLayout = new QHBoxLayout(singleGroup);

    m_addButton = new QPushButton(tr("&Add..."), this);
    m_editButton = new QPushButton(tr("&Edit..."), this);
    m_deleteButton = new QPushButton(tr("&Delete"), this);
    m_enableButton = new QPushButton(tr("E&nable"), this);
    m_disableButton = new QPushButton(tr("D&isable"), this);

    connect(m_addButton, &QPushButton::clicked, this, &ItemListPageBase::onAddItem);
    connect(m_editButton, &QPushButton::clicked, this, &ItemListPageBase::onEditItem);
    connect(m_deleteButton, &QPushButton::clicked, this, &ItemListPageBase::onDeleteItem);
    connect(m_enableButton, &QPushButton::clicked, this, &ItemListPageBase::onEnableItem);
    connect(m_disableButton, &QPushButton::clicked, this, &ItemListPageBase::onDisableItem);

    singleLayout->addWidget(m_addButton);
    singleLayout->addWidget(m_editButton);
    singleLayout->addWidget(m_deleteButton);
    singleLayout->addWidget(m_enableButton);
    singleLayout->addWidget(m_disableButton);

    buttonLayout->addWidget(singleGroup);

    // Group operations
    QGroupBox* groupGroup = new QGroupBox(tr("Group Operations"), this);
    QHBoxLayout* groupLayout = new QHBoxLayout(groupGroup);

    m_enableGroupButton = new QPushButton(tr("Enable &Group"), this);
    m_disableGroupButton = new QPushButton(tr("Disable G&roup"), this);
    m_deleteGroupButton = new QPushButton(tr("Delete Gro&up"), this);

    connect(m_enableGroupButton, &QPushButton::clicked, this, &ItemListPageBase::onEnableGroup);
    connect(m_disableGroupButton, &QPushButton::clicked, this, &ItemListPageBase::onDisableGroup);
    connect(m_deleteGroupButton, &QPushButton::clicked, this, &ItemListPageBase::onDeleteGroup);

    groupLayout->addWidget(m_enableGroupButton);
    groupLayout->addWidget(m_disableGroupButton);
    groupLayout->addWidget(m_deleteGroupButton);

    buttonLayout->addWidget(groupGroup);

    mainLayout->addLayout(buttonLayout);
}

void ItemListPageBase::loadItems()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    int row = 0;
    for (const QString& name : itemNames()) {
        m_table->insertRow(row);
        populateRow(row, name);
        row++;
    }

    m_table->setSortingEnabled(true);

    // Update info label
    m_infoLabel->setText(QString("Total %1: %2").arg(itemTypeNamePlural()).arg(itemCount()));
}

void ItemListPageBase::updateButtonStates()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    QString groupName;

    if (hasSelection) {
        QString name = getSelectedItemName();
        if (!name.isEmpty()) {
            groupName = getItemGroup(name);
        }
    }

    m_editButton->setEnabled(m_table->selectedItems().count() > 0);
    m_deleteButton->setEnabled(hasSelection);
    m_enableButton->setEnabled(hasSelection);
    m_disableButton->setEnabled(hasSelection);

    bool hasGroup = !groupName.isEmpty();
    m_enableGroupButton->setEnabled(hasGroup);
    m_disableGroupButton->setEnabled(hasGroup);
    m_deleteGroupButton->setEnabled(hasGroup);
}

QString ItemListPageBase::getSelectedItemName() const
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty())
        return QString();

    int row = selected.first()->row();
    QTableWidgetItem* item = m_table->item(row, 0); // First column has internal name
    if (item)
        return item->data(Qt::UserRole).toString();

    return QString();
}

QStringList ItemListPageBase::getSelectedItemNames() const
{
    QStringList names;
    QSet<int> rows;

    for (QTableWidgetItem* item : m_table->selectedItems()) {
        rows.insert(item->row());
    }

    for (int row : rows) {
        QTableWidgetItem* tableItem = m_table->item(row, 0);
        if (tableItem) {
            names.append(tableItem->data(Qt::UserRole).toString());
        }
    }

    return names;
}

void ItemListPageBase::setReadOnlyItem(int row, int col, const QString& text)
{
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, col, item);
}

void ItemListPageBase::setReadOnlyItemWithData(int row, int col, const QString& text,
                                                const QVariant& data)
{
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setData(Qt::DisplayRole, data);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, col, item);
}

void ItemListPageBase::setCheckboxItem(int row, int col, bool checked, const QString& internalName)
{
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, internalName);
    m_table->setItem(row, col, item);
}

void ItemListPageBase::onAddItem()
{
    if (openEditDialog()) {
        loadItems();
        updateButtonStates();
    }
}

void ItemListPageBase::onEditItem()
{
    QString name = getSelectedItemName();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Edit %1").arg(itemTypeName()),
                             tr("Please select a %1 to edit.").arg(itemTypeName()));
        return;
    }

    if (openEditDialog(name)) {
        loadItems();
        updateButtonStates();
    }
}

void ItemListPageBase::onDeleteItem()
{
    QStringList names = getSelectedItemNames();
    if (names.isEmpty())
        return;

    QString typeName = itemTypeName();
    QString message =
        (names.size() == 1)
            ? tr("Delete %1 '%2'?").arg(typeName, names.first())
            : tr("Delete %1 selected %2?").arg(names.size()).arg(itemTypeNamePlural());

    int ret = QMessageBox::question(this, tr("Delete %1").arg(typeName), message,
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        for (const QString& name : names) {
            deleteItem(name);
        }
        loadItems();
    }
}

void ItemListPageBase::onEnableItem()
{
    for (const QString& name : getSelectedItemNames()) {
        setItemEnabled(name, true);
    }
    loadItems();
}

void ItemListPageBase::onDisableItem()
{
    for (const QString& name : getSelectedItemNames()) {
        setItemEnabled(name, false);
    }
    loadItems();
}

void ItemListPageBase::onEnableGroup()
{
    QString name = getSelectedItemName();
    if (name.isEmpty())
        return;

    QString groupName = getItemGroup(name);
    if (groupName.isEmpty())
        return;

    int count = 0;
    for (const QString& itemName : itemNames()) {
        if (getItemGroup(itemName) == groupName) {
            setItemEnabled(itemName, true);
            count++;
        }
    }

    loadItems();
    QMessageBox::information(
        this, tr("Enable Group"),
        tr("Enabled %1 %2 in group '%3'").arg(count).arg(itemTypeNamePlural()).arg(groupName));
}

void ItemListPageBase::onDisableGroup()
{
    QString name = getSelectedItemName();
    if (name.isEmpty())
        return;

    QString groupName = getItemGroup(name);
    if (groupName.isEmpty())
        return;

    int count = 0;
    for (const QString& itemName : itemNames()) {
        if (getItemGroup(itemName) == groupName) {
            setItemEnabled(itemName, false);
            count++;
        }
    }

    loadItems();
    QMessageBox::information(
        this, tr("Disable Group"),
        tr("Disabled %1 %2 in group '%3'").arg(count).arg(itemTypeNamePlural()).arg(groupName));
}

void ItemListPageBase::onDeleteGroup()
{
    QString name = getSelectedItemName();
    if (name.isEmpty())
        return;

    QString groupName = getItemGroup(name);
    if (groupName.isEmpty())
        return;

    int ret = QMessageBox::question(
        this, tr("Delete Group"),
        tr("Delete all %1 in group '%2'?").arg(itemTypeNamePlural()).arg(groupName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QStringList toDelete;
        for (const QString& itemName : itemNames()) {
            if (getItemGroup(itemName) == groupName) {
                toDelete.append(itemName);
            }
        }

        for (const QString& itemName : toDelete) {
            deleteItem(itemName);
        }

        loadItems();
        QMessageBox::information(
            this, tr("Delete Group"),
            tr("Deleted %1 %2 from group '%3'").arg(toDelete.size()).arg(itemTypeNamePlural()).arg(groupName));
    }
}

void ItemListPageBase::onItemDoubleClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    onEditItem();
}

void ItemListPageBase::onSelectionChanged()
{
    updateButtonStates();
}
