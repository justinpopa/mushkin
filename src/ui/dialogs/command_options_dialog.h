#ifndef COMMAND_OPTIONS_DIALOG_H
#define COMMAND_OPTIONS_DIALOG_H

#include <QCheckBox>
#include <QDialog>

// Forward declarations
class WorldDocument;

/**
 * CommandOptionsDialog - Dialog for configuring command input behavior options
 *
 * Provides controls for:
 * - Double-click behavior (insert word, send command)
 * - Arrow key behavior (wrap history, change history, recall partial)
 * - Input options (escape clears input, save deleted commands, confirm before replacing)
 * - Keyboard shortcuts (Ctrl+Z, Ctrl+P, Ctrl+N)
 *
 * Based on MUSHclient's command input configuration dialog.
 */
class CommandOptionsDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure command options for
     * @param parent Parent widget
     */
    explicit CommandOptionsDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~CommandOptionsDialog() override = default;

  private slots:
    /**
     * OK button clicked - validate and save
     */
    void onAccepted();

    /**
     * Cancel button clicked
     */
    void onRejected();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load settings from WorldDocument
     */
    void loadSettings();

    /**
     * Save settings to WorldDocument
     */
    void saveSettings();

    // Member variables
    WorldDocument* m_doc;

    // Double-click behavior group
    QCheckBox* m_doubleClickInserts;
    QCheckBox* m_doubleClickSends;

    // Arrow key behavior group
    QCheckBox* m_arrowKeysWrap;
    QCheckBox* m_arrowsChangeHistory;
    QCheckBox* m_arrowRecallsPartial;
    QCheckBox* m_altArrowRecallsPartial;

    // Input options group
    QCheckBox* m_escapeDeletesInput;
    QCheckBox* m_saveDeletedCommand;
    QCheckBox* m_confirmBeforeReplacingTyping;

    // Keyboard shortcuts group
    QCheckBox* m_ctrlZGoesToEndOfBuffer;
    QCheckBox* m_ctrlPGoesToPreviousCommand;
    QCheckBox* m_ctrlNGoesToNextCommand;
};

#endif // COMMAND_OPTIONS_DIALOG_H
