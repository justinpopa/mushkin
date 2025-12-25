#ifndef GENERATE_ID_DIALOG_H
#define GENERATE_ID_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

/**
 * GenerateIDDialog - Shows generated unique ID with regenerate/copy options
 *
 * Simple dialog that displays a cryptographically unique 40-character hex ID
 * suitable for use as plugin IDs. User can regenerate or copy to clipboard.
 */
class GenerateIDDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GenerateIDDialog(QWidget *parent = nullptr);

private slots:
    void onRegenerate();
    void onCopyToClipboard();

private:
    void generateAndDisplay();

    QLineEdit *m_idEdit;
    QPushButton *m_regenerateButton;
    QPushButton *m_copyButton;
    QPushButton *m_closeButton;
};

#endif // GENERATE_ID_DIALOG_H
