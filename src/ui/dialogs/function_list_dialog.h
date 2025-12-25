#ifndef FUNCTION_LIST_DIALOG_H
#define FUNCTION_LIST_DIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

// Forward declarations
class QTableWidget;
class QLineEdit;
class QDialogButtonBox;
class QPushButton;

/**
 * FunctionListDialog - Display a filterable list of Lua functions
 *
 * This dialog displays a list of function names and descriptions in a table format.
 * Users can:
 * - Filter the list by typing in the search box
 * - Double-click to select a function
 * - Copy function names to clipboard
 * - Sort by clicking column headers
 *
 * Based on the original MUSHclient's function list dialog.
 * Used for browsing available Lua functions and other key-value items.
 */
class FunctionListDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param title Dialog window title
     * @param parent Parent widget
     */
    explicit FunctionListDialog(const QString& title, QWidget* parent = nullptr);
    ~FunctionListDialog() override;

    /**
     * Add a single item to the list
     * @param name Function/item name
     * @param description Function/item description
     */
    void addItem(const QString& name, const QString& description);

    /**
     * Set all items at once (replaces existing items)
     * @param items List of name-description pairs
     */
    void setItems(const QList<QPair<QString, QString>>& items);

    /**
     * Get the name of the currently selected item
     * @return Selected item name, or empty string if none selected
     */
    QString selectedName() const;

    /**
     * Get the description of the currently selected item
     * @return Selected item description, or empty string if none selected
     */
    QString selectedDescription() const;

    /**
     * Set the filter text
     * @param filter Filter string to apply
     */
    void setFilter(const QString& filter);

  private slots:
    /**
     * Filter text changed - update visible items
     */
    void onFilterChanged(const QString& filter);

    /**
     * Copy button clicked - copy selected function name to clipboard
     */
    void onCopyName();

    /**
     * Item double-clicked - accept dialog with selection
     */
    void onItemDoubleClicked(int row, int column);

    /**
     * Selection changed - update button states
     */
    void onSelectionChanged();

    /**
     * Column header clicked - sort by that column
     */
    void onHeaderClicked(int column);

  private:
    // Setup methods
    void setupUi();
    void setupConnections();

    // Data methods
    void populateTable();
    void updateButtonStates();

    // Member variables
    QString m_title;
    QList<QPair<QString, QString>> m_items;

    // UI components
    QLineEdit* m_filterEdit;
    QTableWidget* m_table;
    QPushButton* m_copyButton;
    QDialogButtonBox* m_buttonBox;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    // Table columns
    enum Column { COL_NAME = 0, COL_DESCRIPTION, COL_COUNT };
};

#endif // FUNCTION_LIST_DIALOG_H
