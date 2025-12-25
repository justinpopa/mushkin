// function_list_dialog.cpp
// Function List Dialog
//
// Dialog for browsing and selecting Lua functions or other key-value items
// Based on original MUSHclient's function list dialog

#include "function_list_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

FunctionListDialog::FunctionListDialog(const QString& title, QWidget* parent)
    : QDialog(parent), m_title(title), m_filterEdit(nullptr), m_table(nullptr),
      m_copyButton(nullptr), m_buttonBox(nullptr), m_lastColumn(COL_NAME), m_reverseSort(false)
{
    setWindowTitle(m_title);
    setMinimumSize(500, 400);
    resize(700, 500);

    setupUi();
    setupConnections();
    updateButtonStates();
}

FunctionListDialog::~FunctionListDialog()
{
}

void FunctionListDialog::setupUi()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Filter section
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QLabel* filterLabel = new QLabel("Filter:", this);
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Type to filter...");
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(m_filterEdit);
    mainLayout->addLayout(filterLayout);

    // Table widget
    m_table = new QTableWidget(this);
    m_table->setColumnCount(COL_COUNT);
    m_table->setHorizontalHeaderLabels({"Name", "Description"});

    // Table settings
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(false); // Manual sorting
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    // Configure column widths
    m_table->setColumnWidth(COL_NAME, 200);
    m_table->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(m_table);

    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_copyButton = new QPushButton("Copy Name", this);
    m_copyButton->setToolTip("Copy selected function name to clipboard");
    actionLayout->addWidget(m_copyButton);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // Dialog buttons (OK/Cancel)
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
}

void FunctionListDialog::setupConnections()
{
    // Filter connections
    connect(m_filterEdit, &QLineEdit::textChanged, this, &FunctionListDialog::onFilterChanged);

    // Table connections
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            &FunctionListDialog::onItemDoubleClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            &FunctionListDialog::onSelectionChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &FunctionListDialog::onHeaderClicked);

    // Button connections
    connect(m_copyButton, &QPushButton::clicked, this, &FunctionListDialog::onCopyName);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void FunctionListDialog::addItem(const QString& name, const QString& description)
{
    m_items.append(qMakePair(name, description));
    populateTable();
}

void FunctionListDialog::setItems(const QList<QPair<QString, QString>>& items)
{
    m_items = items;
    populateTable();
}

QString FunctionListDialog::selectedName() const
{
    QList<QTableWidgetItem*> selectedItems = m_table->selectedItems();
    if (selectedItems.isEmpty())
        return QString();

    int row = selectedItems.first()->row();
    QTableWidgetItem* nameItem = m_table->item(row, COL_NAME);

    return nameItem ? nameItem->text() : QString();
}

QString FunctionListDialog::selectedDescription() const
{
    QList<QTableWidgetItem*> selectedItems = m_table->selectedItems();
    if (selectedItems.isEmpty())
        return QString();

    int row = selectedItems.first()->row();
    QTableWidgetItem* descItem = m_table->item(row, COL_DESCRIPTION);

    return descItem ? descItem->text() : QString();
}

void FunctionListDialog::setFilter(const QString& filter)
{
    m_filterEdit->setText(filter);
    // onFilterChanged will be called automatically via signal
}

void FunctionListDialog::populateTable()
{
    m_table->setRowCount(0);

    QString filter = m_filterEdit->text().toLower();

    int row = 0;
    for (const auto& item : m_items) {
        const QString& name = item.first;
        const QString& description = item.second;

        // Apply filter - check both name and description
        if (!filter.isEmpty()) {
            if (!name.toLower().contains(filter) && !description.toLower().contains(filter)) {
                continue;
            }
        }

        m_table->insertRow(row);

        // Name column
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        m_table->setItem(row, COL_NAME, nameItem);

        // Description column
        QTableWidgetItem* descItem = new QTableWidgetItem(description);
        m_table->setItem(row, COL_DESCRIPTION, descItem);

        row++;
    }

    // Apply sorting if we have items
    if (m_table->rowCount() > 0) {
        m_table->sortItems(m_lastColumn, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
    }

    // Update window title with count
    int totalCount = m_items.count();
    int displayedCount = m_table->rowCount();

    if (filter.isEmpty()) {
        setWindowTitle(QString("%1 (%2 items)").arg(m_title).arg(totalCount));
    } else {
        setWindowTitle(
            QString("%1 (%2 of %3 items)").arg(m_title).arg(displayedCount).arg(totalCount));
    }

    updateButtonStates();
}

void FunctionListDialog::updateButtonStates()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    m_copyButton->setEnabled(hasSelection);

    // OK button from button box
    QPushButton* okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setEnabled(hasSelection);
    }
}

void FunctionListDialog::onFilterChanged(const QString& filter)
{
    Q_UNUSED(filter);
    populateTable();
}

void FunctionListDialog::onCopyName()
{
    QString name = selectedName();
    if (!name.isEmpty()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(name);
    }
}

void FunctionListDialog::onItemDoubleClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);

    // Double-click accepts the dialog with the current selection
    if (!m_table->selectedItems().isEmpty()) {
        accept();
    }
}

void FunctionListDialog::onSelectionChanged()
{
    updateButtonStates();
}

void FunctionListDialog::onHeaderClicked(int column)
{
    // Toggle sort order if clicking same column
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
        m_lastColumn = column;
    }

    m_table->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}
