#ifndef SEND_TO_ALL_DIALOG_H
#define SEND_TO_ALL_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QLineEdit;
class QListWidget;
class QCheckBox;
class QDialogButtonBox;

/**
 * SendToAllDialog - Send a command to all open worlds
 *
 * Provides a dialog for sending a single command or text to multiple
 * open worlds simultaneously. Useful for:
 * - Broadcasting commands to all connected worlds
 * - Synchronizing actions across multiple sessions
 * - Batch operations on selected worlds
 *
 * The dialog allows selecting which worlds to send to and whether
 * to echo the command to each world's output.
 *
 * Based on: SendToAllDlg.h/cpp in original MUSHclient
 * Original dialog: IDD_SENDTOALL
 */
class SendToAllDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit SendToAllDialog(QWidget* parent = nullptr);
    ~SendToAllDialog() override;

    /**
     * Get the text to send
     * @return The text entered in the send text field
     */
    QString sendText() const;

    /**
     * Get the list of selected world names
     * @return List of world names that were checked
     */
    QStringList selectedWorlds() const;

    /**
     * Get the echo checkbox state
     * @return True if commands should be echoed to output
     */
    bool echo() const;

    /**
     * Populate the world list with available worlds
     * @param worlds List of world names to display
     */
    void setWorlds(const QStringList& worlds);

  private slots:
    /**
     * OK button clicked - accept the dialog
     */
    void onAccepted();

    /**
     * Cancel button clicked - reject the dialog
     */
    void onRejected();

  private:
    // Setup methods
    void setupUi();

    // UI Components
    QLineEdit* m_sendText;
    QListWidget* m_worldList;
    QCheckBox* m_echo;
    QDialogButtonBox* m_buttonBox;
};

#endif // SEND_TO_ALL_DIALOG_H
