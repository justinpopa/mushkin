#include "macros_page.h"
#include "../../dialogs/shortcut_edit_dialog.h"
#include "automation/sendto.h"
#include "world/accelerator_manager.h"
#include "world/world_document.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

MacrosPage::MacrosPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void MacrosPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel(tr("Search:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Filter by key or action..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit, 1);
    mainLayout->addLayout(searchLayout);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({tr("Shortcut"), tr("Action"), tr("Send To"), tr("Source")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColAction, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table, 1);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton(tr("&Add..."), this);
    m_editButton = new QPushButton(tr("&Edit..."), this);
    m_deleteButton = new QPushButton(tr("&Delete"), this);
    m_enableButton = new QPushButton(tr("E&nable"), this);
    m_disableButton = new QPushButton(tr("D&isable"), this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_enableButton);
    buttonLayout->addWidget(m_disableButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // Conflict indicator
    m_conflictLabel = new QLabel(this);
    m_conflictLabel->setStyleSheet("QLabel { color: #c0392b; font-weight: bold; padding: 5px; }");
    m_conflictLabel->hide();
    mainLayout->addWidget(m_conflictLabel);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MacrosPage::onSearchChanged);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &MacrosPage::onSelectionChanged);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &MacrosPage::onItemDoubleClicked);
    connect(m_addButton, &QPushButton::clicked, this, &MacrosPage::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &MacrosPage::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &MacrosPage::onDeleteClicked);
    connect(m_enableButton, &QPushButton::clicked, this, &MacrosPage::onEnableClicked);
    connect(m_disableButton, &QPushButton::clicked, this, &MacrosPage::onDisableClicked);
}

void MacrosPage::loadSettings()
{
    loadShortcuts();
    updateButtonStates();
    updateConflictIndicator();
    m_hasChanges = false;
}

void MacrosPage::saveSettings()
{
    // Changes are saved immediately when add/edit/delete is performed
    m_hasChanges = false;
}

bool MacrosPage::hasChanges() const
{
    return m_hasChanges;
}

void MacrosPage::loadShortcuts()
{
    if (!m_doc || !m_doc->m_acceleratorManager) {
        return;
    }

    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    QVector<AcceleratorEntry> accels = m_doc->m_acceleratorManager->acceleratorList();

    for (const auto& entry : accels) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Shortcut column
        QTableWidgetItem* keyItem = new QTableWidgetItem(entry.keyString);
        keyItem->setData(Qt::UserRole, entry.keyString);
        if (!entry.enabled) {
            keyItem->setForeground(Qt::gray);
        }
        m_table->setItem(row, ColShortcut, keyItem);

        // Action column
        QTableWidgetItem* actionItem = new QTableWidgetItem(entry.action);
        if (!entry.enabled) {
            actionItem->setForeground(Qt::gray);
        }
        m_table->setItem(row, ColAction, actionItem);

        // Send To column
        QTableWidgetItem* sendToItem = new QTableWidgetItem(sendToDisplayName(entry.sendTo));
        if (!entry.enabled) {
            sendToItem->setForeground(Qt::gray);
        }
        m_table->setItem(row, ColSendTo, sendToItem);

        // Source column
        QString sourceText;
        switch (entry.source) {
            case AcceleratorSource::User:
                sourceText = tr("User");
                break;
            case AcceleratorSource::Script:
                sourceText = tr("Script");
                break;
            case AcceleratorSource::Plugin:
                sourceText = tr("Plugin: %1").arg(entry.pluginId);
                break;
        }
        QTableWidgetItem* sourceItem = new QTableWidgetItem(sourceText);
        sourceItem->setData(Qt::UserRole, static_cast<int>(entry.source));

        if (entry.source != AcceleratorSource::User) {
            QFont font = sourceItem->font();
            font.setItalic(true);
            sourceItem->setFont(font);
            sourceItem->setForeground(QColor("#7f8c8d"));
        }
        if (!entry.enabled) {
            sourceItem->setForeground(Qt::gray);
        }
        m_table->setItem(row, ColSource, sourceItem);
    }

    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ColShortcut, Qt::AscendingOrder);
    applyFilter();
}

void MacrosPage::applyFilter()
{
    QString filter = m_currentFilter.toLower();

    for (int row = 0; row < m_table->rowCount(); ++row) {
        bool show = true;

        if (!filter.isEmpty()) {
            QString key = m_table->item(row, ColShortcut)->text().toLower();
            QString action = m_table->item(row, ColAction)->text().toLower();
            show = key.contains(filter) || action.contains(filter);
        }

        m_table->setRowHidden(row, !show);
    }
}

void MacrosPage::onSearchChanged(const QString& text)
{
    m_currentFilter = text;
    applyFilter();
}

void MacrosPage::onSelectionChanged()
{
    updateButtonStates();
}

void MacrosPage::updateButtonStates()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    bool isUserShortcut = isSelectedUserShortcut();

    m_editButton->setEnabled(hasSelection && isUserShortcut);
    m_deleteButton->setEnabled(hasSelection && isUserShortcut);
    m_enableButton->setEnabled(hasSelection);
    m_disableButton->setEnabled(hasSelection);
}

void MacrosPage::updateConflictIndicator()
{
    if (!m_doc || !m_doc->m_acceleratorManager) {
        m_conflictLabel->hide();
        return;
    }

    auto conflicts = m_doc->m_acceleratorManager->findConflicts();

    if (conflicts.isEmpty()) {
        m_conflictLabel->hide();
    } else {
        QStringList conflictKeys;
        for (auto it = conflicts.constBegin(); it != conflicts.constEnd(); ++it) {
            conflictKeys.append(tr("%1 (%2 bindings)").arg(it.key()).arg(it.value().size()));
        }
        m_conflictLabel->setText(tr("Conflicts: %1").arg(conflictKeys.join(", ")));
        m_conflictLabel->show();
    }
}

QString MacrosPage::getSelectedKeyString() const
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }

    int row = selected.first()->row();
    QTableWidgetItem* keyItem = m_table->item(row, ColShortcut);
    return keyItem ? keyItem->data(Qt::UserRole).toString() : QString();
}

bool MacrosPage::isSelectedUserShortcut() const
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return false;
    }

    int row = selected.first()->row();
    QTableWidgetItem* sourceItem = m_table->item(row, ColSource);
    if (!sourceItem) {
        return false;
    }

    int source = sourceItem->data(Qt::UserRole).toInt();
    return source == static_cast<int>(AcceleratorSource::User);
}

void MacrosPage::onItemDoubleClicked(QTableWidgetItem* item)
{
    Q_UNUSED(item);
    if (isSelectedUserShortcut()) {
        onEditClicked();
    }
}

void MacrosPage::onAddClicked()
{
    ShortcutEditDialog dialog(m_doc, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_doc->m_acceleratorManager->addKeyBinding(dialog.keyString(), dialog.action(),
                                                   dialog.sendTo());
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
        m_hasChanges = true;
        emit settingsChanged();
    }
}

void MacrosPage::onEditClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    ShortcutEditDialog dialog(m_doc, keyString, this);
    if (dialog.exec() == QDialog::Accepted) {
        if (dialog.keyString() != keyString) {
            m_doc->m_acceleratorManager->removeKeyBinding(keyString);
        }
        m_doc->m_acceleratorManager->addKeyBinding(dialog.keyString(), dialog.action(),
                                                   dialog.sendTo());
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
        m_hasChanges = true;
        emit settingsChanged();
    }
}

void MacrosPage::onDeleteClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    int result = QMessageBox::question(this, tr("Confirm Delete"),
                                       tr("Delete shortcut '%1'?").arg(keyString),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_doc->m_acceleratorManager->removeKeyBinding(keyString);
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
        m_hasChanges = true;
        emit settingsChanged();
    }
}

void MacrosPage::onEnableClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    m_doc->m_acceleratorManager->setAcceleratorEnabled(keyString, true);
    loadShortcuts();
    updateButtonStates();
    m_hasChanges = true;
    emit settingsChanged();
}

void MacrosPage::onDisableClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    m_doc->m_acceleratorManager->setAcceleratorEnabled(keyString, false);
    loadShortcuts();
    updateButtonStates();
    m_hasChanges = true;
    emit settingsChanged();
}
