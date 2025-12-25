#ifndef ACTIVITY_WINDOW_H
#define ACTIVITY_WINDOW_H

#include <QDockWidget>
#include <QTableWidget>
#include <QTimer>

class MainWindow;
class WorldWidget;

/**
 * ActivityWindow - Dockable window showing all open worlds and their status
 *
 * Based on CActivityView from ActivityView.cpp in original MUSHclient.
 *
 * Displays a list of all open world connections with:
 * - Sequence number
 * - World name
 * - New (unread) lines count
 * - Total lines count
 * - Connection status
 * - Connected since time
 * - Connection duration
 *
 * Features:
 * - Sortable columns (click header to sort)
 * - Context menu for world actions
 * - Double-click to switch to world
 * - Auto-refresh on configurable interval
 */
class ActivityWindow : public QDockWidget {
    Q_OBJECT

  public:
    explicit ActivityWindow(MainWindow* mainWindow);
    ~ActivityWindow() override;

    /**
     * Refresh the world list
     * Called automatically by timer and when worlds change
     */
    void refresh();

  private slots:
    void onHeaderClicked(int column);
    void onDoubleClicked(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);

    // Context menu actions
    void switchToWorld();
    void configureWorld();
    void connectWorld();
    void disconnectWorld();
    void saveWorld();
    void saveWorldAs();
    void closeWorld();

  private:
    /**
     * Get the WorldWidget for the currently selected row
     * Returns nullptr if no selection or invalid
     */
    WorldWidget* getSelectedWorld();

    /**
     * Format duration as HH:MM:SS or D days HH:MM:SS
     */
    QString formatDuration(qint64 seconds);

    /**
     * Format timestamp as HH:MM:SS
     */
    QString formatTime(const QDateTime& time);

    // Column indices matching original MUSHclient
    enum Column {
        ColSeq = 0,
        ColWorld,
        ColNew,
        ColLines,
        ColStatus,
        ColSince,
        ColDuration,
        ColCount
    };

    MainWindow* m_mainWindow;
    QTableWidget* m_table;
    QTimer* m_refreshTimer;

    // Sorting state
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif // ACTIVITY_WINDOW_H
