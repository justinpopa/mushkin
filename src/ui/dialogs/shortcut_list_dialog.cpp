/**
 * shortcut_list_dialog.cpp - Unified keyboard shortcut manager dialog
 *
 * Displays all keyboard shortcuts in a searchable, filterable table with
 * add/edit/delete support for user-defined shortcuts and conflict detection.
 */

#include "shortcut_list_dialog.h"
#include "../../automation/sendto.h"
#include "../../world/accelerator_manager.h"
#include "../../world/world_document.h"
#include "shortcut_edit_dialog.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

ShortcutListDialog::ShortcutListDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc)
{
    setupUi();
    loadShortcuts();
    updateButtonStates();
    updateConflictIndicator();

    setWindowTitle("Keyboard Shortcuts");
    resize(700, 500);
}

void ShortcutListDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search:", this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filter by key or action...");
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit, 1);
    mainLayout->addLayout(searchLayout);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({"Shortcut", "Action", "Send To", "Source"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColAction, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table, 1);

    // Buttons group
    QGroupBox* buttonGroup = new QGroupBox("Shortcut Operations", this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonGroup);

    m_addButton = new QPushButton("&Add...", this);
    m_editButton = new QPushButton("&Edit...", this);
    m_deleteButton = new QPushButton("&Delete", this);
    m_enableButton = new QPushButton("E&nable", this);
    m_disableButton = new QPushButton("D&isable", this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_enableButton);
    buttonLayout->addWidget(m_disableButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(buttonGroup);

    // Conflict indicator
    m_conflictLabel = new QLabel(this);
    m_conflictLabel->setStyleSheet("QLabel { color: #c0392b; font-weight: bold; padding: 5px; }");
    m_conflictLabel->hide();
    mainLayout->addWidget(m_conflictLabel);

    // Close button
    QDialogButtonBox* dialogButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mainLayout->addWidget(dialogButtons);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ShortcutListDialog::onSearchChanged);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            &ShortcutListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::itemDoubleClicked, this,
            &ShortcutListDialog::onItemDoubleClicked);
    connect(m_addButton, &QPushButton::clicked, this, &ShortcutListDialog::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &ShortcutListDialog::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ShortcutListDialog::onDeleteClicked);
    connect(m_enableButton, &QPushButton::clicked, this, &ShortcutListDialog::onEnableClicked);
    connect(m_disableButton, &QPushButton::clicked, this, &ShortcutListDialog::onDisableClicked);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

void ShortcutListDialog::loadShortcuts()
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
        keyItem->setData(Qt::UserRole, entry.keyString); // Store for retrieval
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
                sourceText = "User";
                break;
            case AcceleratorSource::Script:
                sourceText = "Script";
                break;
            case AcceleratorSource::Plugin:
                sourceText = QString("Plugin: %1").arg(entry.pluginId);
                break;
        }
        QTableWidgetItem* sourceItem = new QTableWidgetItem(sourceText);
        sourceItem->setData(Qt::UserRole, static_cast<int>(entry.source));

        // Style runtime shortcuts differently
        if (entry.source != AcceleratorSource::User) {
            QFont font = sourceItem->font();
            font.setItalic(true);
            sourceItem->setFont(font);
            sourceItem->setForeground(QColor("#7f8c8d")); // Gray for runtime
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

void ShortcutListDialog::applyFilter()
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

void ShortcutListDialog::onSearchChanged(const QString& text)
{
    m_currentFilter = text;
    applyFilter();
}

void ShortcutListDialog::onSelectionChanged()
{
    updateButtonStates();
}

void ShortcutListDialog::updateButtonStates()
{
    bool hasSelection = !m_table->selectedItems().isEmpty();
    bool isUserShortcut = isSelectedUserShortcut();

    m_editButton->setEnabled(hasSelection && isUserShortcut);
    m_deleteButton->setEnabled(hasSelection && isUserShortcut);
    m_enableButton->setEnabled(hasSelection);
    m_disableButton->setEnabled(hasSelection);
}

void ShortcutListDialog::updateConflictIndicator()
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
            conflictKeys.append(QString("%1 (%2 bindings)").arg(it.key()).arg(it.value().size()));
        }
        m_conflictLabel->setText(QString("Conflicts: %1").arg(conflictKeys.join(", ")));
        m_conflictLabel->show();
    }
}

QString ShortcutListDialog::getSelectedKeyString() const
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }

    int row = selected.first()->row();
    QTableWidgetItem* keyItem = m_table->item(row, ColShortcut);
    return keyItem ? keyItem->data(Qt::UserRole).toString() : QString();
}

bool ShortcutListDialog::isSelectedUserShortcut() const
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

void ShortcutListDialog::onItemDoubleClicked(QTableWidgetItem* item)
{
    Q_UNUSED(item);
    if (isSelectedUserShortcut()) {
        onEditClicked();
    }
}

void ShortcutListDialog::onAddClicked()
{
    ShortcutEditDialog dialog(m_doc, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Add the new shortcut
        m_doc->m_acceleratorManager->addKeyBinding(dialog.keyString(), dialog.action(),
                                                   dialog.sendTo());

        // Reload the list
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
    }
}

void ShortcutListDialog::onEditClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    ShortcutEditDialog dialog(m_doc, keyString, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Remove old shortcut if key changed
        if (dialog.keyString() != keyString) {
            m_doc->m_acceleratorManager->removeKeyBinding(keyString);
        }

        // Add/update the shortcut
        m_doc->m_acceleratorManager->addKeyBinding(dialog.keyString(), dialog.action(),
                                                   dialog.sendTo());

        // Reload the list
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
    }
}

void ShortcutListDialog::onDeleteClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    int result = QMessageBox::question(this, "Confirm Delete",
                                       QString("Delete shortcut '%1'?").arg(keyString),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_doc->m_acceleratorManager->removeKeyBinding(keyString);
        loadShortcuts();
        updateButtonStates();
        updateConflictIndicator();
    }
}

void ShortcutListDialog::onEnableClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    m_doc->m_acceleratorManager->setAcceleratorEnabled(keyString, true);
    loadShortcuts();
    updateButtonStates();
}

void ShortcutListDialog::onDisableClicked()
{
    QString keyString = getSelectedKeyString();
    if (keyString.isEmpty()) {
        return;
    }

    m_doc->m_acceleratorManager->setAcceleratorEnabled(keyString, false);
    loadShortcuts();
    updateButtonStates();
}
