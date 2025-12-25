#ifndef IMPORT_XML_DIALOG_H
#define IMPORT_XML_DIALOG_H

#include <QCheckBox>
#include <QDialog>

class WorldDocument;

/**
 * ImportXmlDialog - Dialog for selecting which data types to import from XML
 *
 * Allows users to selectively import different types of data from MUSHclient XML files:
 * - General settings
 * - Triggers
 * - Aliases
 * - Timers
 * - Macros
 * - Variables
 * - Colors
 * - Keypad settings
 * - Printing settings
 *
 * Provides convenient "Select All" and "Select None" buttons, plus options to
 * import from file or clipboard.
 *
 * Based on MUSHclient's XML import dialog.
 */
class ImportXmlDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to import into
     * @param parent Parent widget
     */
    explicit ImportXmlDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~ImportXmlDialog() override = default;

    // Get import flags based on checkbox selections
    int importFlags() const;

    // Accessor methods for import selections
    bool importGeneral() const
    {
        return m_general->isChecked();
    }
    bool importTriggers() const
    {
        return m_triggers->isChecked();
    }
    bool importAliases() const
    {
        return m_aliases->isChecked();
    }
    bool importTimers() const
    {
        return m_timers->isChecked();
    }
    bool importMacros() const
    {
        return m_macros->isChecked();
    }
    bool importVariables() const
    {
        return m_variables->isChecked();
    }
    bool importColours() const
    {
        return m_colours->isChecked();
    }
    bool importKeypad() const
    {
        return m_keypad->isChecked();
    }
    bool importPrinting() const
    {
        return m_printing->isChecked();
    }

  private slots:
    /**
     * Select All button clicked
     */
    void onSelectAll();

    /**
     * Select None button clicked
     */
    void onSelectNone();

    /**
     * Import from File button clicked
     */
    void onImportFromFile();

    /**
     * Import from Clipboard button clicked
     */
    void onImportFromClipboard();

  private:
    /**
     * Setup UI components
     */
    void setupUi();

    // WorldDocument to import into
    WorldDocument* m_doc;

    // Checkboxes for import options
    QCheckBox* m_general;
    QCheckBox* m_triggers;
    QCheckBox* m_aliases;
    QCheckBox* m_timers;
    QCheckBox* m_macros;
    QCheckBox* m_variables;
    QCheckBox* m_colours;
    QCheckBox* m_keypad;
    QCheckBox* m_printing;
};

#endif // IMPORT_XML_DIALOG_H
