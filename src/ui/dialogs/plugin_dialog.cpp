// plugin_dialog.cpp
// Plugin Management Dialog
//
// Dialog for managing plugins - view, add, remove, enable, disable, edit, reload
// Based on MUSHclient's PluginsDlg (dialogs/plugins/PluginsDlg.cpp)

#include "plugin_dialog.h"
#include "plugin.h"
#include "world_document.h"

#include "logging.h"
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

PluginDialog::PluginDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_pluginTable(nullptr), m_lastColumn(COL_NAME),
      m_reverseSort(false)
{
    setWindowTitle(tr("Plugin Management"));
    // Note: WA_DeleteOnClose is NOT set because this dialog is stack-allocated
    // in MainWindow::onPluginManagement(). Setting it would cause a double-delete crash.

    setupUi();
    loadSettings();
    loadPluginList();
    updateButtonStates();
}

PluginDialog::~PluginDialog()
{
    // Settings are saved in closeEvent() which is called before destruction.
    // We don't save settings here because Qt may have already destroyed child widgets.
}

void PluginDialog::setupUi()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Plugin table
    m_pluginTable = new QTableWidget(this);
    m_pluginTable->setColumnCount(COL_COUNT);
    m_pluginTable->setHorizontalHeaderLabels({tr("Name"), tr("Purpose"), tr("Author"),
                                              tr("Language"), tr("File"), tr("Enabled"),
                                              tr("Ver")});

    // Table settings
    m_pluginTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTable->setSortingEnabled(false); // We do manual sorting
    m_pluginTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pluginTable->horizontalHeader()->setStretchLastSection(false);
    m_pluginTable->verticalHeader()->setVisible(false);

    // Set column widths
    m_pluginTable->setColumnWidth(COL_NAME, 100);
    m_pluginTable->setColumnWidth(COL_PURPOSE, 200);
    m_pluginTable->setColumnWidth(COL_AUTHOR, 100);
    m_pluginTable->setColumnWidth(COL_LANGUAGE, 70);
    m_pluginTable->setColumnWidth(COL_FILE, 200);
    m_pluginTable->setColumnWidth(COL_ENABLED, 60);
    m_pluginTable->setColumnWidth(COL_VERSION, 50);

    mainLayout->addWidget(m_pluginTable);

    // Info label (shows plugin count)
    m_infoLabel = new QLabel(this);
    mainLayout->addWidget(m_infoLabel);

    // Buttons layout - 2 rows of 4 buttons
    QGridLayout* buttonLayout = new QGridLayout();

    // Top row: Add, Reinstall, Enable, Edit
    m_addButton = new QPushButton(tr("&Add"), this);
    m_reloadButton = new QPushButton(tr("&Reinstall"), this);
    m_enableButton = new QPushButton(tr("E&nable"), this);
    m_editButton = new QPushButton(tr("&Edit"), this);

    buttonLayout->addWidget(m_addButton, 0, 0);
    buttonLayout->addWidget(m_reloadButton, 0, 1);
    buttonLayout->addWidget(m_enableButton, 0, 2);
    buttonLayout->addWidget(m_editButton, 0, 3);

    // Bottom row: Remove, Show Info, Disable, Close
    m_removeButton = new QPushButton(tr("&Remove"), this);
    m_showInfoButton = new QPushButton(tr("&Show Info"), this);
    m_disableButton = new QPushButton(tr("&Disable"), this);
    m_closeButton = new QPushButton(tr("&Close"), this);

    buttonLayout->addWidget(m_removeButton, 1, 0);
    buttonLayout->addWidget(m_showInfoButton, 1, 1);
    buttonLayout->addWidget(m_disableButton, 1, 2);
    buttonLayout->addWidget(m_closeButton, 1, 3);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_addButton, &QPushButton::clicked, this, &PluginDialog::onAddPlugin);
    connect(m_removeButton, &QPushButton::clicked, this, &PluginDialog::onRemovePlugin);
    connect(m_reloadButton, &QPushButton::clicked, this, &PluginDialog::onReloadPlugin);
    connect(m_editButton, &QPushButton::clicked, this, &PluginDialog::onEditPlugin);
    connect(m_enableButton, &QPushButton::clicked, this, &PluginDialog::onEnablePlugin);
    connect(m_disableButton, &QPushButton::clicked, this, &PluginDialog::onDisablePlugin);
    connect(m_showInfoButton, &QPushButton::clicked, this, &PluginDialog::onShowInfo);
    connect(m_closeButton, &QPushButton::clicked, this, &PluginDialog::onClose);

    connect(m_pluginTable, &QTableWidget::cellDoubleClicked, this,
            &PluginDialog::onPluginDoubleClicked);
    connect(m_pluginTable, &QTableWidget::customContextMenuRequested, this,
            &PluginDialog::onPluginRightClicked);
    connect(m_pluginTable->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &PluginDialog::onHeaderClicked);
    connect(m_pluginTable, &QTableWidget::itemSelectionChanged, this,
            &PluginDialog::onSelectionChanged);

    // Set minimum size
    setMinimumSize(800, 400);
}

void PluginDialog::loadPluginList()
{
    m_pluginTable->setRowCount(0);

    if (!m_doc) {
        m_infoLabel->setText(tr("No plugins"));
        return;
    }

    int row = 0;
    for (const auto& plugin : m_doc->m_PluginList) {
        if (!plugin) {
            continue;
        }

        m_pluginTable->insertRow(row);

        // COL_NAME
        QTableWidgetItem* nameItem = new QTableWidgetItem(plugin->m_strName);
        nameItem->setData(Qt::UserRole, QVariant::fromValue<void*>(plugin.get()));
        m_pluginTable->setItem(row, COL_NAME, nameItem);

        // COL_PURPOSE
        m_pluginTable->setItem(row, COL_PURPOSE, new QTableWidgetItem(plugin->m_strPurpose));

        // COL_AUTHOR
        m_pluginTable->setItem(row, COL_AUTHOR, new QTableWidgetItem(plugin->m_strAuthor));

        // COL_LANGUAGE
        m_pluginTable->setItem(row, COL_LANGUAGE, new QTableWidgetItem(plugin->m_strLanguage));

        // COL_FILE
        m_pluginTable->setItem(row, COL_FILE, new QTableWidgetItem(plugin->m_strSource));

        // COL_ENABLED
        m_pluginTable->setItem(row, COL_ENABLED,
                               new QTableWidgetItem(plugin->m_bEnabled ? tr("Yes") : tr("No")));

        // COL_VERSION
        m_pluginTable->setItem(row, COL_VERSION,
                               new QTableWidgetItem(QString::number(plugin->m_dVersion, 'f', 2)));

        row++;
    }

    // Sort by last sorted column
    if (m_pluginTable->rowCount() > 0) {
        m_pluginTable->sortItems(m_lastColumn,
                                 m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
    }

    // Update info label
    m_infoLabel->setText(tr("%n plugin(s)", "", m_pluginTable->rowCount()));
}

void PluginDialog::updateButtonStates()
{
    bool hasSelection = m_pluginTable->selectionModel()->hasSelection();
    bool hasDescription = false;

    // Check if any selected plugin has a description
    if (hasSelection) {
        QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
        for (const QModelIndex& index : selected) {
            QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
            if (nameItem) {
                Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
                if (plugin && !plugin->m_strDescription.isEmpty()) {
                    hasDescription = true;
                    break;
                }
            }
        }
    }

    // Update button states
    m_removeButton->setEnabled(hasSelection);
    m_reloadButton->setEnabled(hasSelection);
    m_enableButton->setEnabled(hasSelection);
    m_disableButton->setEnabled(hasSelection);
    m_editButton->setEnabled(hasSelection);
    m_showInfoButton->setEnabled(hasSelection && hasDescription);
}

void PluginDialog::onAddPlugin()
{
    QSettings settings;
    QString pluginDir = settings.value("PluginsDirectory", QDir::homePath()).toString();

    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Add Plugin"), pluginDir,
        tr("Plugin files (*.xml);;Text files (*.txt);;All files (*.*)"));

    if (files.isEmpty()) {
        return;
    }

    // Save directory for next time
    if (!files.isEmpty()) {
        QFileInfo fi(files.first());
        settings.setValue("PluginsDirectory", fi.absolutePath());
    }

    bool anyLoaded = false;

    for (const QString& file : files) {
        QString errorMsg;
        Plugin* plugin = m_doc->LoadPlugin(file, errorMsg);

        if (!plugin) {
            QMessageBox::warning(this, tr("Plugin Error"),
                                 tr("Failed to load plugin:\n%1\n\nError: %2").arg(file, errorMsg));
        } else {
            anyLoaded = true;
        }
    }

    if (anyLoaded) {
        loadPluginList();
        updateButtonStates();
    }
}

void PluginDialog::onRemovePlugin()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    int result = QMessageBox::question(this, tr("Remove Plugin"),
                                       tr("Remove %n selected plugin(s)?", "", selected.size()),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes) {
        return;
    }

    // Collect plugins to remove (don't modify list while iterating)
    QList<Plugin*> pluginsToRemove;

    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin) {
                pluginsToRemove.append(plugin);
            }
        }
    }

    // Remove plugins
    for (Plugin* plugin : pluginsToRemove) {
        QString pluginID = plugin->m_strID;
        QString pluginName = plugin->m_strName;

        if (m_doc->UnloadPlugin(pluginID)) {
            qCDebug(lcPlugin) << "Removed plugin:" << pluginName << "ID:" << pluginID;
        } else {
            QMessageBox::warning(this, tr("Remove Error"),
                                 tr("Failed to remove plugin: %1").arg(pluginName));
        }
    }

    loadPluginList();
    updateButtonStates();
}

void PluginDialog::onReloadPlugin()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    // Collect plugin info before unloading
    struct PluginInfo {
        QString id;
        QString name;
        QString source;
    };
    QList<PluginInfo> pluginsToReload;

    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin) {
                PluginInfo info;
                info.id = plugin->m_strID;
                info.name = plugin->m_strName;
                info.source = plugin->m_strSource;
                pluginsToReload.append(info);
            }
        }
    }

    // Reload each plugin
    for (const PluginInfo& info : pluginsToReload) {
        // Unload
        if (!m_doc->UnloadPlugin(info.id)) {
            QMessageBox::warning(this, tr("Reload Error"),
                                 tr("Failed to unload plugin: %1").arg(info.name));
            continue;
        }

        // Reload
        QString errorMsg;
        Plugin* plugin = m_doc->LoadPlugin(info.source, errorMsg);

        if (!plugin) {
            QMessageBox::warning(
                this, tr("Reload Error"),
                tr("Failed to reload plugin: %1\n\nError: %2").arg(info.name, errorMsg));
        } else {
            qCDebug(lcPlugin) << "Reinstalled plugin:" << info.name << "ID:" << info.id;
        }
    }

    loadPluginList();
    updateButtonStates();
}

void PluginDialog::onEditPlugin()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    // Edit all selected plugins
    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin) {
                editPluginFile(plugin->m_strSource);
            }
        }
    }
}

void PluginDialog::onEnablePlugin()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin) {
                m_doc->EnablePlugin(plugin->m_strID, true);
                qCDebug(lcPlugin) << "Enabled plugin:" << plugin->m_strName;
            }
        }
    }

    loadPluginList();
    updateButtonStates();
}

void PluginDialog::onDisablePlugin()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin) {
                m_doc->EnablePlugin(plugin->m_strID, false);
                qCDebug(lcPlugin) << "Disabled plugin:" << plugin->m_strName;
            }
        }
    }

    loadPluginList();
    updateButtonStates();
}

void PluginDialog::onShowInfo()
{
    QModelIndexList selected = m_pluginTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    // Show description for all selected plugins
    for (const QModelIndex& index : selected) {
        QTableWidgetItem* nameItem = m_pluginTable->item(index.row(), COL_NAME);
        if (nameItem) {
            Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
            if (plugin && !plugin->m_strDescription.isEmpty()) {
                // Create a simple dialog to show the description
                QDialog* descDialog = new QDialog(this);
                descDialog->setWindowTitle(tr("%1 - Description").arg(plugin->m_strName));
                descDialog->setAttribute(Qt::WA_DeleteOnClose);

                QVBoxLayout* layout = new QVBoxLayout(descDialog);

                QTextEdit* textEdit = new QTextEdit(descDialog);
                textEdit->setPlainText(plugin->m_strDescription);
                textEdit->setReadOnly(true);
                layout->addWidget(textEdit);

                QPushButton* closeBtn = new QPushButton(tr("Close"), descDialog);
                connect(closeBtn, &QPushButton::clicked, descDialog, &QDialog::accept);
                layout->addWidget(closeBtn);

                descDialog->resize(600, 400);
                descDialog->show();
            }
        }
    }
}

void PluginDialog::onClose()
{
    close();
}

void PluginDialog::onPluginDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    QTableWidgetItem* nameItem = m_pluginTable->item(row, COL_NAME);
    if (nameItem) {
        Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
        if (plugin) {
            editPluginFile(plugin->m_strSource);
        }
    }
}

void PluginDialog::onPluginRightClicked(const QPoint& pos)
{
    QTableWidgetItem* item = m_pluginTable->itemAt(pos);
    if (!item) {
        return;
    }

    int row = item->row();
    QTableWidgetItem* nameItem = m_pluginTable->item(row, COL_NAME);
    if (!nameItem) {
        return;
    }

    Plugin* plugin = static_cast<Plugin*>(nameItem->data(Qt::UserRole).value<void*>());
    if (!plugin) {
        return;
    }

    QMenu contextMenu(this);

    QAction* editAction = contextMenu.addAction(tr("Edit Plugin Source"));
    QAction* editStateAction = contextMenu.addAction(tr("Edit Plugin State File"));

    QAction* selected = contextMenu.exec(m_pluginTable->viewport()->mapToGlobal(pos));

    if (selected == editAction) {
        editPluginFile(plugin->m_strSource);
    } else if (selected == editStateAction) {
        QString stateFile = getPluginStateFilePath(plugin);
        editPluginFile(stateFile);
    }
}

void PluginDialog::onHeaderClicked(int column)
{
    // Toggle sort order if clicking same column
    if (column == m_lastColumn) {
        m_reverseSort = !m_reverseSort;
    } else {
        m_reverseSort = false;
        m_lastColumn = column;
    }

    m_pluginTable->sortItems(column, m_reverseSort ? Qt::DescendingOrder : Qt::AscendingOrder);
}

void PluginDialog::onSelectionChanged()
{
    updateButtonStates();
}

void PluginDialog::editPluginFile(const QString& pluginPath)
{
    // Use the system default application to open the file
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pluginPath))) {
        QMessageBox::warning(this, tr("Edit Error"),
                             tr("Failed to open file:\n%1").arg(pluginPath));
    }
}

QString PluginDialog::getPluginStateFilePath(Plugin* plugin) const
{
    if (!plugin) {
        return QString();
    }

    // State file: <plugin_directory>/<plugin_id>.state
    return plugin->m_strDirectory + "/" + plugin->m_strID + ".state";
}

void PluginDialog::loadSettings()
{
    QSettings settings;

    // Restore window geometry
    restoreGeometry(settings.value("PluginDialog/geometry").toByteArray());

    // Restore column widths
    for (int col = 0; col < COL_COUNT; ++col) {
        int width = settings.value(QString("PluginDialog/column%1").arg(col), -1).toInt();
        if (width > 0) {
            m_pluginTable->setColumnWidth(col, width);
        }
    }

    // Restore sort settings
    m_lastColumn = settings.value("PluginDialog/sortColumn", COL_NAME).toInt();
    m_reverseSort = settings.value("PluginDialog/sortReverse", false).toBool();
}

void PluginDialog::saveSettings()
{
    if (!m_pluginTable) {
        return; // Table already deleted
    }

    QSettings settings;

    // Save window geometry
    settings.setValue("PluginDialog/geometry", saveGeometry());

    // Save column widths
    for (int col = 0; col < COL_COUNT; ++col) {
        settings.setValue(QString("PluginDialog/column%1").arg(col),
                          m_pluginTable->columnWidth(col));
    }

    // Save sort settings
    settings.setValue("PluginDialog/sortColumn", m_lastColumn);
    settings.setValue("PluginDialog/sortReverse", m_reverseSort);
}

void PluginDialog::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QDialog::closeEvent(event);
}
