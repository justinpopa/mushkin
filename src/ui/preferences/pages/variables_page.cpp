#include "variables_page.h"
#include "../../dialogs/variable_edit_dialog.h"
#include "automation/variable.h"
#include "world/world_document.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

VariablesPage::VariablesPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void VariablesPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel(tr("Search:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Filter by name or value..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit, 1);
    mainLayout->addLayout(searchLayout);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::Interactive);
    m_table->setColumnWidth(ColName, 200);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table, 1);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton(tr("&Add..."), this);
    m_editButton = new QPushButton(tr("&Edit..."), this);
    m_deleteButton = new QPushButton(tr("&Delete"), this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &VariablesPage::onSearchChanged);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &VariablesPage::onSelectionChanged);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &VariablesPage::onItemDoubleClicked);
    connect(m_addButton, &QPushButton::clicked, this, &VariablesPage::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &VariablesPage::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &VariablesPage::onDeleteClicked);
}

void VariablesPage::loadSettings()
{
    loadVariables();
    updateButtonStates();
    m_hasChanges = false;
}

void VariablesPage::saveSettings()
{
    // Changes are saved immediately when add/edit/delete is performed
    m_hasChanges = false;
}

bool VariablesPage::hasChanges() const
{
    return m_hasChanges;
}

void VariablesPage::loadVariables()
{
    if (!m_doc)
        return;

    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    const VariableMap& vars = m_doc->getVariableMap();

    for (const auto& pair : vars) {
        const QString& name = pair.first;
        const Variable* var = pair.second.get();
        if (!var)
            continue;

        int row = m_table->rowCount();
        m_table->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setData(Qt::UserRole, name);
        m_table->setItem(row, ColName, nameItem);

        QTableWidgetItem* valueItem = new QTableWidgetItem(var->strContents);
        // Truncate display of long values
        if (var->strContents.length() > 100) {
            valueItem->setText(var->strContents.left(100) + "...");
            valueItem->setToolTip(var->strContents);
        }
        m_table->setItem(row, ColValue, valueItem);
    }

    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ColName, Qt::AscendingOrder);
    applyFilter();
}

void VariablesPage::applyFilter()
{
    QString filter = m_currentFilter.toLower();

    for (int row = 0; row < m_table->rowCount(); ++row) {
        bool show = true;

        if (!filter.isEmpty()) {
            QString name = m_table->item(row, ColName)->text().toLower();
            QString value = m_table->item(row, ColValue)->text().toLower();
            show = name.contains(filter) || value.contains(filter);
        }

        m_table->setRowHidden(row, !show);
    }
}

void VariablesPage::onSearchChanged(const QString& text)
{
    m_currentFilter = text;
    applyFilter();
}

void VariablesPage::onSelectionChanged()
{
    updateButtonStates();
}

void VariablesPage::updateButtonStates()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

QString VariablesPage::getSelectedVariableName() const
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty())
        return QString();

    int row = selected.first()->row();
    QTableWidgetItem* nameItem = m_table->item(row, ColName);
    return nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
}

void VariablesPage::onItemDoubleClicked(QTableWidgetItem* item)
{
    Q_UNUSED(item);
    onEditClicked();
}

void VariablesPage::onAddClicked()
{
    VariableEditDialog dialog(m_doc, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Dialog saves directly to WorldDocument
        loadVariables();
        updateButtonStates();
        m_hasChanges = true;
        emit settingsChanged();
    }
}

void VariablesPage::onEditClicked()
{
    QString name = getSelectedVariableName();
    if (name.isEmpty())
        return;

    VariableEditDialog dialog(m_doc, name, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Dialog saves directly to WorldDocument
        loadVariables();
        updateButtonStates();
        m_hasChanges = true;
        emit settingsChanged();
    }
}

void VariablesPage::onDeleteClicked()
{
    QString name = getSelectedVariableName();
    if (name.isEmpty())
        return;

    int result = QMessageBox::question(this, tr("Confirm Delete"),
                                       tr("Delete variable '%1'?").arg(name),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_doc->deleteVariable(name);
        loadVariables();
        updateButtonStates();
        m_hasChanges = true;
        emit settingsChanged();
    }
}
