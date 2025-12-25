#include "lua_choose_list_multi_dialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

LuaChooseListMultiDialog::LuaChooseListMultiDialog(const QString& title, const QString& message,
                                                   const QStringList& items,
                                                   const QList<int>& defaultIndices,
                                                   QWidget* parent)
    : QDialog(parent), m_title(title), m_message(message), m_items(items),
      m_defaultIndices(defaultIndices)
{
    setWindowTitle(m_title);
    setModal(true);
    setupUi();
}

void LuaChooseListMultiDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label
    m_messageLabel = new QLabel(m_message, this);
    m_messageLabel->setWordWrap(true);
    mainLayout->addWidget(m_messageLabel);

    // List widget with items
    m_listWidget = new QListWidget(this);
    m_listWidget->addItems(m_items);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Set default selections if valid
    for (int index : m_defaultIndices) {
        if (index >= 0 && index < m_items.size()) {
            m_listWidget->item(index)->setSelected(true);
        }
    }

    // Double-click to accept
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
            &LuaChooseListMultiDialog::onItemDoubleClicked);

    mainLayout->addWidget(m_listWidget);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on list widget
    m_listWidget->setFocus();

    // Set minimum size
    setMinimumSize(300, 400);
    adjustSize();
}

QList<int> LuaChooseListMultiDialog::selectedIndices() const
{
    QList<int> indices;
    QList<QListWidgetItem*> selectedItems = m_listWidget->selectedItems();

    for (QListWidgetItem* item : selectedItems) {
        indices.append(m_listWidget->row(item));
    }

    return indices;
}

QStringList LuaChooseListMultiDialog::selectedTexts() const
{
    QStringList texts;
    QList<QListWidgetItem*> selectedItems = m_listWidget->selectedItems();

    for (QListWidgetItem* item : selectedItems) {
        texts.append(item->text());
    }

    return texts;
}

void LuaChooseListMultiDialog::onItemDoubleClicked()
{
    accept();
}
