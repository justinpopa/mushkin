#include "activity_window.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "../../storage/database.h"
#include "../main_window.h"
#include "../views/world_widget.h"
#include "world_document.h"

ActivityWindow::ActivityWindow(MainWindow* mainWindow)
    : QDockWidget(tr("Activity List"), mainWindow), m_mainWindow(mainWindow), m_sortColumn(ColSeq),
      m_sortOrder(Qt::AscendingOrder)
{
    setObjectName("ActivityWindow");
    setAllowedAreas(Qt::AllDockWidgetAreas);

    // Create table widget
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({tr("Seq"), tr("World"), tr("New"), tr("Lines"),
                                        tr("Status"), tr("Since"), tr("Duration")});

    // Configure table appearance
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false); // We handle sorting manually
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->verticalHeader()->setVisible(false);

    // Set column widths (matching original)
    m_table->setColumnWidth(ColSeq, 40);
    m_table->setColumnWidth(ColWorld, 130);
    m_table->setColumnWidth(ColNew, 55);
    m_table->setColumnWidth(ColLines, 55);
    m_table->setColumnWidth(ColStatus, 80);
    m_table->setColumnWidth(ColSince, 105);
    m_table->setColumnWidth(ColDuration, 70);

    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionsClickable(true);

    setWidget(m_table);

    // Connect signals
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &ActivityWindow::onHeaderClicked);
    connect(m_table, &QTableWidget::doubleClicked, this, &ActivityWindow::onDoubleClicked);
    connect(m_table, &QTableWidget::customContextMenuRequested, this,
            &ActivityWindow::onContextMenu);

    // Setup refresh timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &ActivityWindow::refresh);

    // Get refresh interval from preferences (default 15 seconds)
    Database* db = Database::instance();
    int intervalSecs = db->getPreferenceInt("ActivityWindowRefreshInterval", 15);
    m_refreshTimer->start(intervalSecs * 1000);

    // Initial refresh
    refresh();
}

ActivityWindow::~ActivityWindow() = default;

void ActivityWindow::refresh()
{
    if (!m_mainWindow)
        return;

    // Find the MDI area
    QMdiArea* mdiArea = m_mainWindow->findChild<QMdiArea*>();
    if (!mdiArea)
        return;

    QList<QMdiSubWindow*> windows = mdiArea->subWindowList();

    // Store current selection
    int selectedRow = m_table->currentRow();
    QString selectedWorld;
    if (selectedRow >= 0 && selectedRow < m_table->rowCount()) {
        QTableWidgetItem* item = m_table->item(selectedRow, ColWorld);
        if (item)
            selectedWorld = item->text();
    }

    // Block signals during update
    m_table->blockSignals(true);
    m_table->setRowCount(0);

    int seq = 1;
    for (QMdiSubWindow* subWindow : windows) {
        WorldWidget* ww = qobject_cast<WorldWidget*>(subWindow->widget());
        if (!ww || !ww->document())
            continue;

        WorldDocument* doc = ww->document();
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Store WorldWidget pointer in first column's data
        QTableWidgetItem* seqItem = new QTableWidgetItem();
        seqItem->setData(Qt::DisplayRole, seq);
        seqItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(ww)));
        seqItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, ColSeq, seqItem);

        // World name
        QTableWidgetItem* worldItem = new QTableWidgetItem(doc->worldName());
        m_table->setItem(row, ColWorld, worldItem);

        // New lines (unread) - lines they haven't read yet
        int newLines = doc->m_new_lines;
        QTableWidgetItem* newItem = new QTableWidgetItem();
        newItem->setData(Qt::DisplayRole, newLines);
        newItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (newLines > 0) {
            newItem->setForeground(Qt::blue);
            QFont font = newItem->font();
            font.setBold(true);
            newItem->setFont(font);
        }
        m_table->setItem(row, ColNew, newItem);

        // Total lines
        QTableWidgetItem* linesItem = new QTableWidgetItem();
        linesItem->setData(Qt::DisplayRole, static_cast<int>(doc->m_lineList.count()));
        linesItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, ColLines, linesItem);

        // Status
        QString status;
        if (doc->m_iConnectPhase == eConnectConnectedToMud)
            status = tr("Connected");
        else if (doc->m_iConnectPhase == eConnectNotConnected)
            status = tr("Disconnected");
        else
            status = tr("Connecting...");
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        if (doc->m_iConnectPhase == eConnectConnectedToMud) {
            statusItem->setForeground(QColor(0, 128, 0)); // Green
        } else if (doc->m_iConnectPhase == eConnectNotConnected) {
            statusItem->setForeground(Qt::gray);
        } else {
            statusItem->setForeground(QColor(255, 165, 0)); // Orange
        }
        m_table->setItem(row, ColStatus, statusItem);

        // Since (connection start time)
        QString sinceStr;
        if (doc->m_iConnectPhase == eConnectConnectedToMud && doc->m_tConnectTime.isValid()) {
            sinceStr = formatTime(doc->m_tConnectTime);
        }
        QTableWidgetItem* sinceItem = new QTableWidgetItem(sinceStr);
        m_table->setItem(row, ColSince, sinceItem);

        // Duration - includes previous connections plus current session
        // m_tsConnectDuration is in milliseconds
        qint64 totalSecs = doc->m_tsConnectDuration / 1000;
        if (doc->m_iConnectPhase == eConnectConnectedToMud && doc->m_tConnectTime.isValid()) {
            totalSecs += doc->m_tConnectTime.secsTo(QDateTime::currentDateTime());
        }
        QString durationStr = (totalSecs > 0) ? formatDuration(totalSecs) : QString();
        QTableWidgetItem* durationItem = new QTableWidgetItem(durationStr);
        durationItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        // Store raw seconds for proper sorting
        durationItem->setData(Qt::UserRole, totalSecs);
        m_table->setItem(row, ColDuration, durationItem);

        seq++;
    }

    // Restore sorting
    if (m_sortColumn >= 0 && m_sortColumn < ColCount) {
        m_table->sortItems(m_sortColumn, m_sortOrder);
    }

    // Restore selection
    if (!selectedWorld.isEmpty()) {
        for (int i = 0; i < m_table->rowCount(); ++i) {
            QTableWidgetItem* item = m_table->item(i, ColWorld);
            if (item && item->text() == selectedWorld) {
                m_table->selectRow(i);
                break;
            }
        }
    }

    m_table->blockSignals(false);
}

void ActivityWindow::onHeaderClicked(int column)
{
    if (column == m_sortColumn) {
        // Toggle sort order
        m_sortOrder =
            (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = column;
        m_sortOrder = Qt::AscendingOrder;
    }

    m_table->sortItems(m_sortColumn, m_sortOrder);
}

void ActivityWindow::onDoubleClicked(const QModelIndex& index)
{
    Q_UNUSED(index);
    switchToWorld();
}

void ActivityWindow::onContextMenu(const QPoint& pos)
{
    WorldWidget* ww = getSelectedWorld();

    QMenu menu(this);

    QAction* switchAction = menu.addAction(tr("&Switch to World"));
    switchAction->setEnabled(ww != nullptr);
    connect(switchAction, &QAction::triggered, this, &ActivityWindow::switchToWorld);

    menu.addSeparator();

    QAction* configAction = menu.addAction(tr("&Configure World..."));
    configAction->setEnabled(ww != nullptr);
    connect(configAction, &QAction::triggered, this, &ActivityWindow::configureWorld);

    menu.addSeparator();

    QAction* connectAction = menu.addAction(tr("C&onnect"));
    connectAction->setEnabled(ww && ww->document() &&
                              ww->document()->m_iConnectPhase == eConnectNotConnected);
    connect(connectAction, &QAction::triggered, this, &ActivityWindow::connectWorld);

    QAction* disconnectAction = menu.addAction(tr("&Disconnect"));
    disconnectAction->setEnabled(ww && ww->document() &&
                                 ww->document()->m_iConnectPhase == eConnectConnectedToMud);
    connect(disconnectAction, &QAction::triggered, this, &ActivityWindow::disconnectWorld);

    menu.addSeparator();

    QAction* saveAction = menu.addAction(tr("&Save"));
    saveAction->setEnabled(ww != nullptr);
    connect(saveAction, &QAction::triggered, this, &ActivityWindow::saveWorld);

    QAction* saveAsAction = menu.addAction(tr("Save &As..."));
    saveAsAction->setEnabled(ww != nullptr);
    connect(saveAsAction, &QAction::triggered, this, &ActivityWindow::saveWorldAs);

    QAction* closeAction = menu.addAction(tr("&Close"));
    closeAction->setEnabled(ww != nullptr);
    connect(closeAction, &QAction::triggered, this, &ActivityWindow::closeWorld);

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

WorldWidget* ActivityWindow::getSelectedWorld()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount())
        return nullptr;

    QTableWidgetItem* item = m_table->item(row, ColSeq);
    if (!item)
        return nullptr;

    return static_cast<WorldWidget*>(item->data(Qt::UserRole).value<void*>());
}

void ActivityWindow::switchToWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (!ww)
        return;

    // Find the MDI subwindow containing this world
    QMdiArea* mdiArea = m_mainWindow->findChild<QMdiArea*>();
    if (!mdiArea)
        return;

    for (QMdiSubWindow* subWindow : mdiArea->subWindowList()) {
        if (subWindow->widget() == ww) {
            mdiArea->setActiveSubWindow(subWindow);
            subWindow->showNormal();
            ww->setFocus();
            break;
        }
    }
}

void ActivityWindow::configureWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (!ww)
        return;

    // Switch to the world first, then open properties
    switchToWorld();

    // Trigger the world properties action in main window
    QAction* propsAction = m_mainWindow->findChild<QAction*>("worldPropertiesAction");
    if (propsAction)
        propsAction->trigger();
}

void ActivityWindow::connectWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (ww)
        ww->connectToMud();
}

void ActivityWindow::disconnectWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (ww)
        ww->disconnectFromMud();
}

void ActivityWindow::saveWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (!ww)
        return;

    QString filename = ww->filename();
    if (!filename.isEmpty()) {
        ww->saveToFile(filename);
    } else {
        // No filename - use Save As
        saveWorldAs();
    }
}

void ActivityWindow::saveWorldAs()
{
    WorldWidget* ww = getSelectedWorld();
    if (!ww)
        return;

    QString filename = QFileDialog::getSaveFileName(
        this, tr("Save World As"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("MUSHclient World Files (*.mcl);;All Files (*)"));

    if (!filename.isEmpty()) {
        ww->saveToFile(filename);
    }
}

void ActivityWindow::closeWorld()
{
    WorldWidget* ww = getSelectedWorld();
    if (!ww)
        return;

    // Find and close the MDI subwindow
    QMdiArea* mdiArea = m_mainWindow->findChild<QMdiArea*>();
    if (!mdiArea)
        return;

    for (QMdiSubWindow* subWindow : mdiArea->subWindowList()) {
        if (subWindow->widget() == ww) {
            subWindow->close();
            break;
        }
    }
}

QString ActivityWindow::formatDuration(qint64 seconds)
{
    // Match original: adaptive format like "2d 3h 4m 5s" or "3h 4m 5s" etc.
    if (seconds < 0)
        return QString();

    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int mins = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (days > 0) {
        return QString("%1d %2h %3m %4s").arg(days).arg(hours).arg(mins).arg(secs);
    } else if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(mins).arg(secs);
    } else if (mins > 0) {
        return QString("%1m %2s").arg(mins).arg(secs);
    } else {
        return QString("%1s").arg(secs);
    }
}

QString ActivityWindow::formatTime(const QDateTime& time)
{
    // Match original: "%#I:%M %p, %d %b" e.g., "2 PM, 17 Dec"
    return time.toString("h:mm AP, d MMM");
}
