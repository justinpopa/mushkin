#ifndef COMMAND_HISTORY_DIALOG_H
#define COMMAND_HISTORY_DIALOG_H

#include <QDialog>

// Forward declarations
class WorldDocument;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QDialogButtonBox;

/**
 * CommandHistoryDialog - View and manage command history
 *
 * While arrow keys provide quick access to recent commands, this dialog
 * allows users to:
 * - View entire command history at once
 * - Search through old commands
 * - Edit commands before re-sending
 * - Delete commands from history
 * - Clear all history
 * - Save command sequences to file
 *
 * Based on: cmdhist.h/cpp in original MUSHclient
 * Accessed via: Game → Command History (Ctrl+H)
 */
class CommandHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param doc World document containing command history
     * @param parent Parent widget
     */
    explicit CommandHistoryDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~CommandHistoryDialog() override;

private slots:
    /**
     * Send button clicked or Enter pressed - send selected command to MUD
     */
    void sendCommand();

    /**
     * Edit button clicked or F2 pressed - edit command before sending
     */
    void editCommand();

    /**
     * Delete button clicked or Delete key pressed - remove command from history
     */
    void deleteCommand();

    /**
     * Clear All button clicked - empty entire history (with confirmation)
     */
    void clearAll();

    /**
     * Filter text changed - update list to show only matching commands
     */
    void filterChanged(const QString& filter);

    /**
     * Save button clicked - export history to text file
     */
    void saveToFile();

    /**
     * Command double-clicked - send to MUD
     */
    void commandDoubleClicked(QListWidgetItem* item);

    /**
     * Selection changed - update button states
     */
    void selectionChanged();

private:
    // Setup methods
    void setupUi();
    void setupConnections();

    // Data methods
    void populateList();
    void updateButtons();

    // Member variables
    WorldDocument* m_doc;

    QListWidget* m_commandList;
    QLineEdit* m_filterEdit;

    QPushButton* m_sendButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_clearButton;
    QPushButton* m_saveButton;

    QDialogButtonBox* m_buttonBox;
};

#endif // COMMAND_HISTORY_DIALOG_H
