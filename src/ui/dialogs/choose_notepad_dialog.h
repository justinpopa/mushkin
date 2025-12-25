#ifndef CHOOSE_NOTEPAD_DIALOG_H
#define CHOOSE_NOTEPAD_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QListWidget;

/**
 * ChooseNotepadDialog - Let users select from a list of available notepads
 *
 * A simple dialog for Lua scripts to present a list of notepad choices
 * to the user via a list widget.
 *
 * Features:
 * - Displays a list of notepad names
 * - Single selection from available notepads
 * - Returns selected notepad name
 * - Double-click accepts selection
 * - OK/Cancel buttons
 *
 * Used by Lua API ChooseNotepad function for interactive notepad selection.
 */
class ChooseNotepadDialog : public QDialog {
    Q_OBJECT

  public:
    explicit ChooseNotepadDialog(const QString& title, const QStringList& notepadNames,
                                 QWidget* parent = nullptr);
    ~ChooseNotepadDialog() override = default;

    // Accessor
    QString selectedNotepad() const;

  private slots:
    void onItemDoubleClicked();

  private:
    void setupUi();

    // UI Components
    QListWidget* m_listWidget;

    // Data
    QString m_title;
    QStringList m_notepadNames;
};

#endif // CHOOSE_NOTEPAD_DIALOG_H
