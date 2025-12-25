#ifndef MAP_MOVE_DIALOG_H
#define MAP_MOVE_DIALOG_H

#include <QDialog>

// Forward declarations
class QLineEdit;
class QCheckBox;
class QPushButton;

/**
 * MapMoveDialog - Add special movement to the mapper
 *
 * Dialog for adding non-standard directions to the mapper like "climb tree".
 * Allows specifying the action, reverse action, and whether to send to MUD.
 */
class MapMoveDialog : public QDialog {
    Q_OBJECT

  public:
    explicit MapMoveDialog(QWidget* parent = nullptr);
    ~MapMoveDialog() override = default;

    // Get/Set action (command to send)
    QString action() const;
    void setAction(const QString& action);

    // Get/Set reverse action
    QString reverse() const;
    void setReverse(const QString& reverse);

    // Get/Set send to MUD flag
    bool sendToMud() const;
    void setSendToMud(bool send);

  private slots:
    void onActionTextChanged(const QString& text);

  private:
    void setupUi();

    // UI Components
    QLineEdit* m_actionEdit;
    QLineEdit* m_reverseEdit;
    QCheckBox* m_sendToMudCheckBox;
    QPushButton* m_okButton;
};

#endif // MAP_MOVE_DIALOG_H
