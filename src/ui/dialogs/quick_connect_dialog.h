#ifndef QUICK_CONNECT_DIALOG_H
#define QUICK_CONNECT_DIALOG_H

#include <QDialog>

// Forward declarations
class QLineEdit;
class QSpinBox;

/**
 * QuickConnectDialog - Quick connection to MUD server
 *
 * Simple dialog for quickly connecting to a MUD server without
 * needing to create a full world configuration first.
 * Allows user to specify world name, server address, and port.
 */
class QuickConnectDialog : public QDialog {
    Q_OBJECT

  public:
    explicit QuickConnectDialog(QWidget* parent = nullptr);
    ~QuickConnectDialog() override = default;

    // Accessors for connection parameters
    QString worldName() const;
    QString serverAddress() const;
    int port() const;

  private slots:
    void onAccept();

  private:
    void setupUi();
    bool validateInput();

    // UI Components
    QLineEdit* m_worldNameEdit;
    QLineEdit* m_serverAddressEdit;
    QSpinBox* m_portSpinBox;
};

#endif // QUICK_CONNECT_DIALOG_H
