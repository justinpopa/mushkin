#include "complete_word_dialog.h"
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

CompleteWordDialog::CompleteWordDialog(QWidget* parent)
    : QDialog(parent), m_filterEdit(nullptr), m_listWidget(nullptr), m_argsLabel(nullptr),
      m_isLuaMode(false), m_isFunctionsMode(false)
{
    setWindowTitle(tr("Complete Word"));
    setModal(true);
    setupUi();
}

void CompleteWordDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Filter label and input
    QLabel* filterLabel = new QLabel(tr("Filter:"), this);
    mainLayout->addWidget(filterLabel);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Type to filter..."));
    connect(m_filterEdit, &QLineEdit::textChanged, this, &CompleteWordDialog::onFilterTextChanged);
    mainLayout->addWidget(m_filterEdit);

    // List widget with completions
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // Double-click to accept
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
            &CompleteWordDialog::onItemDoubleClicked);

    // Selection changed to update arguments
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this,
            &CompleteWordDialog::onItemSelectionChanged);

    mainLayout->addWidget(m_listWidget);

    // Arguments label (for functions)
    m_argsLabel = new QLabel(this);
    m_argsLabel->setWordWrap(true);
    m_argsLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    m_argsLabel->setVisible(false);
    mainLayout->addWidget(m_argsLabel);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set compact size suitable for popup
    setMinimumSize(300, 400);
    setMaximumSize(500, 600);
    resize(300, 400);

    // Set focus on filter field
    m_filterEdit->setFocus();
}

void CompleteWordDialog::setItems(const QStringList& items)
{
    m_allItems = items;
    updateFilteredList();
}

void CompleteWordDialog::setFilter(const QString& filter)
{
    m_filter = filter;
    m_filterEdit->setText(filter);
    updateFilteredList();
}

void CompleteWordDialog::setDefaultSelection(const QString& defaultItem)
{
    m_defaultSelection = defaultItem;
    selectDefaultItem();
}

void CompleteWordDialog::setPosition(const QPoint& pos)
{
    m_position = pos;
    if (!pos.isNull()) {
        move(pos);
    }
}

void CompleteWordDialog::setLuaMode(bool isLua)
{
    m_isLuaMode = isLua;
    if (isLua) {
        setWindowTitle(tr("Complete Lua Function"));
    } else {
        setWindowTitle(tr("Complete Word"));
    }
}

void CompleteWordDialog::setFunctionsMode(bool isFunctions)
{
    m_isFunctionsMode = isFunctions;
    m_argsLabel->setVisible(isFunctions);
}

void CompleteWordDialog::setExtraItems(const QStringList& items)
{
    m_extraItems = items;
    updateFilteredList();
}

void CompleteWordDialog::setCommandHistoryItems(const QStringList& items)
{
    m_commandHistoryItems = items;
    updateFilteredList();
}

QString CompleteWordDialog::selectedItem() const
{
    QListWidgetItem* item = m_listWidget->currentItem();
    return item ? item->text() : QString();
}

QString CompleteWordDialog::selectedArgs() const
{
    return m_currentArgs;
}

void CompleteWordDialog::onFilterTextChanged(const QString& text)
{
    m_filter = text;
    updateFilteredList();
}

void CompleteWordDialog::onItemDoubleClicked()
{
    accept();
}

void CompleteWordDialog::onItemSelectionChanged()
{
    // Update arguments display if in functions mode
    if (m_isFunctionsMode) {
        QString selectedText = selectedItem();
        if (!selectedText.isEmpty()) {
            // Extract function arguments from parentheses
            int parenStart = selectedText.indexOf('(');
            int parenEnd = selectedText.lastIndexOf(')');

            if (parenStart != -1 && parenEnd != -1 && parenEnd > parenStart) {
                m_currentArgs = selectedText.mid(parenStart + 1, parenEnd - parenStart - 1);
                m_argsLabel->setText(tr("Arguments: %1").arg(m_currentArgs));
                m_argsLabel->setVisible(true);
            } else {
                m_currentArgs.clear();
                m_argsLabel->setVisible(false);
            }
        }
    }
}

void CompleteWordDialog::updateFilteredList()
{
    m_listWidget->clear();

    // Combine all item sources
    QStringList combinedItems;

    // Add extra items first
    combinedItems.append(m_extraItems);

    // Add main items
    combinedItems.append(m_allItems);

    // Add command history items
    combinedItems.append(m_commandHistoryItems);

    // Remove duplicates while preserving order
    QStringList uniqueItems;
    QSet<QString> seen;
    for (const QString& item : combinedItems) {
        if (!seen.contains(item)) {
            uniqueItems.append(item);
            seen.insert(item);
        }
    }

    // Filter items based on filter text
    QString filterLower = m_filter.toLower();
    QStringList filteredItems;

    for (const QString& item : uniqueItems) {
        if (filterLower.isEmpty() || item.toLower().contains(filterLower)) {
            filteredItems.append(item);
        }
    }

    // Add filtered items to list
    m_listWidget->addItems(filteredItems);

    // Select default item or first item
    selectDefaultItem();
}

void CompleteWordDialog::selectDefaultItem()
{
    if (m_listWidget->count() == 0) {
        return;
    }

    // Try to select the default item
    if (!m_defaultSelection.isEmpty()) {
        QList<QListWidgetItem*> matches =
            m_listWidget->findItems(m_defaultSelection, Qt::MatchExactly);

        if (!matches.isEmpty()) {
            m_listWidget->setCurrentItem(matches.first());
            m_listWidget->scrollToItem(matches.first());
            return;
        }
    }

    // If no default or default not found, select first item
    m_listWidget->setCurrentRow(0);
}
