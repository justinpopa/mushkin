#ifndef TAB_DEFAULTS_DIALOG_H
#define TAB_DEFAULTS_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QPlainTextEdit>
#include <QSpinBox>

// Forward declarations
class WorldDocument;

/**
 * TabDefaultsDialog - Dialog for configuring tab completion settings
 *
 * Provides controls for:
 * - Default words for tab completion (one per line)
 * - Number of lines to search (1-500000)
 * - Add space after completion option
 *
 * Based on MUSHclient's tab completion configuration dialog.
 */
class TabDefaultsDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure tab completion for
     * @param parent Parent widget
     */
    explicit TabDefaultsDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~TabDefaultsDialog() override = default;

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

    // UI Components
    QPlainTextEdit* m_defaultWords;
    QSpinBox* m_linesToSearch;
    QCheckBox* m_addSpace;
};

#endif // TAB_DEFAULTS_DIALOG_H
