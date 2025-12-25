#ifndef GENERATE_NAME_DIALOG_H
#define GENERATE_NAME_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

/**
 * GenerateNameDialog - Shows generated character name with regenerate/copy options
 *
 * Simple dialog that displays a randomly generated fantasy character name.
 * User can regenerate to get a different name or copy to clipboard.
 */
class GenerateNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GenerateNameDialog(QWidget *parent = nullptr);

private slots:
    void onRegenerate();
    void onCopyToClipboard();

private:
    void generateAndDisplay();

    QLineEdit *m_nameEdit;
    QPushButton *m_regenerateButton;
    QPushButton *m_copyButton;
    QPushButton *m_closeButton;
};

#endif // GENERATE_NAME_DIALOG_H
