#ifndef COMPLETE_WORD_DIALOG_H
#define COMPLETE_WORD_DIALOG_H

#include <QDialog>
#include <QPoint>
#include <QString>
#include <QStringList>

// Forward declarations
class QListWidget;
class QLineEdit;
class QLabel;

/**
 * CompleteWordDialog - Word/Function completion dialog
 *
 * Provides word and function completion suggestions from a list.
 * Used for tab completion in the command input field.
 *
 * Features:
 * - Filter field to narrow down choices
 * - QListWidget to display completions
 * - Double-click or Enter selects item
 * - Support for function arguments display
 * - Compact size suitable for popup near cursor
 *
 * Based on original MFC dialog for word completion.
 */
class CompleteWordDialog : public QDialog {
    Q_OBJECT

  public:
    explicit CompleteWordDialog(QWidget* parent = nullptr);
    ~CompleteWordDialog() override = default;

    // Configuration methods
    void setItems(const QStringList& items);
    void setFilter(const QString& filter);
    void setDefaultSelection(const QString& defaultItem);
    void setPosition(const QPoint& pos);
    void setLuaMode(bool isLua);
    void setFunctionsMode(bool isFunctions);
    void setExtraItems(const QStringList& items);
    void setCommandHistoryItems(const QStringList& items);

    // Accessors
    QString selectedItem() const;
    QString selectedArgs() const;

  private slots:
    void onFilterTextChanged(const QString& text);
    void onItemDoubleClicked();
    void onItemSelectionChanged();

  private:
    void setupUi();
    void updateFilteredList();
    void selectDefaultItem();

    // UI Components
    QLineEdit* m_filterEdit;
    QListWidget* m_listWidget;
    QLabel* m_argsLabel;

    // Data
    QStringList m_allItems;
    QStringList m_extraItems;
    QStringList m_commandHistoryItems;
    QString m_filter;
    QString m_defaultSelection;
    QString m_currentArgs;
    QPoint m_position;
    bool m_isLuaMode;
    bool m_isFunctionsMode;
};

#endif // COMPLETE_WORD_DIALOG_H
