#ifndef PLUGIN_DIALOG_H
#define PLUGIN_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

// Forward declarations
class WorldDocument;
class Plugin;

/**
 * PluginDialog - Dialog for managing plugins
 *
 * Plugin Management Dialog
 *
 * Provides a table view of all installed plugins with:
 * - 7 columns: Name, Purpose, Author, Language, File, Enabled, Version
 * - Add/Remove/Edit/Reload/Enable/Disable/Show Info buttons
 * - Sortable columns (click header to sort)
 * - Multi-select support
 * - Double-click to edit plugin source
 * - Right-click to edit plugin state file
 * - Persistent dialog size/position and column widths
 *
 * Based on MUSHclient's Plugin Management Dialog (PluginsDlg.cpp).
 */
class PluginDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param doc WorldDocument containing plugins
     * @param parent Parent widget
     */
    explicit PluginDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginDialog() override;

protected:
    /**
     * Save dialog settings on close
     */
    void closeEvent(QCloseEvent* event) override;

private slots:
    /**
     * Add new plugin(s) from file dialog
     */
    void onAddPlugin();

    /**
     * Remove selected plugin(s)
     */
    void onRemovePlugin();

    /**
     * Reload (reinstall) selected plugin(s)
     */
    void onReloadPlugin();

    /**
     * Edit selected plugin's source file
     */
    void onEditPlugin();

    /**
     * Enable selected plugin(s)
     */
    void onEnablePlugin();

    /**
     * Disable selected plugin(s)
     */
    void onDisablePlugin();

    /**
     * Show plugin description in text viewer
     */
    void onShowInfo();

    /**
     * Close dialog
     */
    void onClose();

    /**
     * Double-click on plugin to edit source
     */
    void onPluginDoubleClicked(int row, int column);

    /**
     * Right-click on plugin to show context menu (edit state file)
     */
    void onPluginRightClicked(const QPoint& pos);

    /**
     * Column header clicked - sort by that column
     */
    void onHeaderClicked(int column);

    /**
     * Selection changed - update button states
     */
    void onSelectionChanged();

private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load plugins from document into table
     */
    void loadPluginList();

    /**
     * Update button enabled states based on selection
     */
    void updateButtonStates();

    /**
     * Edit plugin source or state file
     * @param pluginPath Path to plugin file
     */
    void editPluginFile(const QString& pluginPath);

    /**
     * Get plugin state file path
     * @param plugin Plugin to get state file for
     * @return Path to state file
     */
    QString getPluginStateFilePath(Plugin* plugin) const;

    /**
     * Load dialog settings (size, position, column widths)
     */
    void loadSettings();

    /**
     * Save dialog settings (size, position, column widths)
     */
    void saveSettings();

    // Member variables
    WorldDocument* m_doc;
    QTableWidget* m_pluginTable;

    // Buttons - Top row
    QPushButton* m_addButton;
    QPushButton* m_reloadButton;
    QPushButton* m_enableButton;
    QPushButton* m_editButton;

    // Buttons - Bottom row
    QPushButton* m_removeButton;
    QPushButton* m_showInfoButton;
    QPushButton* m_disableButton;
    QPushButton* m_closeButton;

    // Info label
    QLabel* m_infoLabel;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    // Table columns
    enum Column {
        COL_NAME = 0,
        COL_PURPOSE,
        COL_AUTHOR,
        COL_LANGUAGE,
        COL_FILE,
        COL_ENABLED,
        COL_VERSION,
        COL_COUNT
    };
};

#endif // PLUGIN_DIALOG_H
