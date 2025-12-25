#ifndef SHORTCUT_EDIT_DIALOG_H
#define SHORTCUT_EDIT_DIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>

// Forward declarations
class WorldDocument;
class AcceleratorManager;

/**
 * ShortcutEditDialog - Dialog for adding/editing a keyboard shortcut
 *
 * Features:
 * - Record mode key capture using QKeySequenceEdit
 * - Action/command text field
 * - Send-to destination dropdown
 * - Real-time conflict detection
 *
 * Can operate in two modes:
 * - Add mode: Creates a new user shortcut
 * - Edit mode: Modifies an existing user shortcut
 */
class ShortcutEditDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor for adding a new shortcut
     * @param doc WorldDocument containing the AcceleratorManager
     * @param parent Parent widget
     */
    explicit ShortcutEditDialog(WorldDocument* doc, QWidget* parent = nullptr);

    /**
     * Constructor for editing an existing shortcut
     * @param doc WorldDocument containing the AcceleratorManager
     * @param keyString Original key string of the shortcut to edit
     * @param parent Parent widget
     */
    ShortcutEditDialog(WorldDocument* doc, const QString& keyString, QWidget* parent = nullptr);

    ~ShortcutEditDialog() override = default;

    /**
     * Get the configured key string
     */
    QString keyString() const;

    /**
     * Get the configured action
     */
    QString action() const;

    /**
     * Get the configured send-to destination
     */
    int sendTo() const;

  private slots:
    /**
     * Key sequence changed - check for conflicts
     */
    void onKeySequenceChanged(const QKeySequence& keySequence);

    /**
     * OK button clicked - validate and accept
     */
    void onOk();

    /**
     * Clear key sequence button clicked
     */
    void onClearKey();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    /**
     * Load existing shortcut data into form fields
     */
    void loadShortcutData();

    /**
     * Check for conflicts with existing shortcuts
     */
    void checkConflict();

    /**
     * Validate form data
     * @return true if valid
     */
    bool validateForm();

    // Member variables
    WorldDocument* m_doc;
    QString m_originalKeyString; // Empty for new shortcut, populated for edit
    bool m_isEditMode;

    // UI Components
    QKeySequenceEdit* m_keySequenceEdit;
    QLineEdit* m_actionEdit;
    QComboBox* m_sendToCombo;
    QLabel* m_conflictLabel;
};

#endif // SHORTCUT_EDIT_DIALOG_H
