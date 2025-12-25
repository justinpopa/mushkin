#include "lua_choose_list_dialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

LuaChooseListDialog::LuaChooseListDialog(const QString& title, const QString& message,
                                         const QStringList& items, int defaultIndex,
                                         QWidget* parent)
    : QDialog(parent), m_title(title), m_message(message), m_items(items),
      m_defaultIndex(defaultIndex)
{
    setWindowTitle(m_title);
    setModal(true);
    setupUi();
}

void LuaChooseListDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label
    m_messageLabel = new QLabel(m_message, this);
    m_messageLabel->setWordWrap(true);
    mainLayout->addWidget(m_messageLabel);

    // List widget with items
    m_listWidget = new QListWidget(this);
    m_listWidget->addItems(m_items);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set default selection if valid
    if (m_defaultIndex >= 0 && m_defaultIndex < m_items.size()) {
        m_listWidget->setCurrentRow(m_defaultIndex);
    }

    // Double-click to accept
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
            &LuaChooseListDialog::onItemDoubleClicked);

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

int LuaChooseListDialog::selectedIndex() const
{
    return m_listWidget->currentRow();
}

QString LuaChooseListDialog::selectedText() const
{
    QListWidgetItem* item = m_listWidget->currentItem();
    return item ? item->text() : QString();
}

void LuaChooseListDialog::onItemDoubleClicked()
{
    accept();
}
