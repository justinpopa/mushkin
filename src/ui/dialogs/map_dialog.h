#ifndef MAP_DIALOG_H
#define MAP_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

// Forward declarations
class WorldDocument;

/**
 * MapDialog - Mapper configuration dialog
 *
 * Provides controls for configuring the speedwalk/automapper feature:
 * - Enable/disable mapper
 * - Remove reverse directions automatically
 * - Failure detection pattern (text or regex)
 * - Display of forwards/backwards directions
 * - Mapper management buttons (Remove All, Remove Last, Special Move, Edit)
 *
 * Based on MUSHclient's mapper configuration dialog.
 */
class MapDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure mapper for
     * @param parent Parent widget
     */
    explicit MapDialog(WorldDocument* doc, QWidget* parent = nullptr);

    ~MapDialog() override = default;

  private slots:
    /**
     * OK button clicked - validate and save
     */
    void onAccepted();

    /**
     * Cancel button clicked
     */
    void onRejected();

    /**
     * Remove All button clicked - clear all mapper data
     */
    void onRemoveAllClicked();

    /**
     * Remove Last button clicked - remove last mapper entry
     */
    void onRemoveLastClicked();

    /**
     * Special Move button clicked - open special move dialog
     */
    void onSpecialMoveClicked();

    /**
     * Edit button clicked - open mapper editor
     */
    void onEditClicked();

    /**
     * Convert to Regexp button clicked - convert failure pattern to regex
     */
    void onConvertToRegexpClicked();

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

    /**
     * Update the direction displays with current mapper state
     */
    void updateDirectionDisplays();

    // Member variables
    WorldDocument* m_doc;

    // Enable/disable controls
    QCheckBox* m_enableMapperCheck;
    QCheckBox* m_removeMapReversesCheck;

    // Failure detection controls
    QLineEdit* m_failurePatternEdit;
    QCheckBox* m_failureRegexpCheck;
    QPushButton* m_convertToRegexpButton;

    // Direction displays
    QPlainTextEdit* m_forwardsDisplay;
    QPlainTextEdit* m_backwardsDisplay;

    // Action buttons
    QPushButton* m_removeAllButton;
    QPushButton* m_removeLastButton;
    QPushButton* m_specialMoveButton;
    QPushButton* m_editButton;
};

#endif // MAP_DIALOG_H
