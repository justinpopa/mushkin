#ifndef WORLD_PROPERTIES_DIALOG_H
#define WORLD_PROPERTIES_DIALOG_H

#include <QDialog>
#include <QFont>

// Forward declarations
class WorldDocument;
class QTabWidget;
class QPushButton;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QLabel;
class QComboBox;

/**
 * WorldPropertiesDialog - World configuration dialog
 *
 * Provides a tabbed interface for configuring all world settings:
 * - Connection (server, port, credentials)
 * - Output (font, colors)
 * - Input (input font, echo settings)
 * - Logging (log file, format)
 * - Scripting (script file, language)
 *
 * This is the PRIMARY configuration interface for MUSHclient.
 * Without it, users must manually edit XML files.
 */
class WorldPropertiesDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc WorldDocument to configure
     * @param parent Parent widget
     */
    explicit WorldPropertiesDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~WorldPropertiesDialog() override;

  private slots:
    /**
     * OK button clicked - save and close
     */
    void onOkClicked();

    /**
     * Cancel button clicked - discard changes and close
     */
    void onCancelClicked();

    /**
     * Apply button clicked - save without closing
     */
    void onApplyClicked();

    /**
     * Output font button clicked - open font picker
     */
    void onOutputFontButtonClicked();

    /**
     * Color button clicked - open color picker
     */
    void onColorButtonClicked();

    /**
     * Input font button clicked - open font picker
     */
    void onInputFontButtonClicked();

  private:
    // Setup methods (to be implemented in later tasks)
    void setupUi();
    void setupConnectionTab();
    void setupOutputTab();
    void setupInputTab();
    void setupLoggingTab();
    void setupScriptingTab();
    void setupPasteToWorldTab();
    void setupSendFileTab();
    void setupRemoteAccessTab();

    // Data methods (to be implemented in later tasks)
    void loadSettings();
    void saveSettings();
    void applySettings();

    // Member variables
    WorldDocument* m_doc;
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;

    // Connection tab widgets
    QLineEdit* m_serverEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_nameEdit;
    QLineEdit* m_passwordEdit;
    QCheckBox* m_autoConnectCheck;

    // Output tab widgets
    QPushButton* m_outputFontButton;
    QLabel* m_outputFontLabel;
    QFont m_outputFont;
    QPushButton* m_colorButtons[16]; // 8 normal + 8 bright ANSI colors
    QRgb m_ansiColors[16];
    QCheckBox* m_flashIconCheck;

    // Helper for Output tab
    void updateColorButton(int index);

    // Input tab widgets
    QPushButton* m_inputFontButton;
    QLabel* m_inputFontLabel;
    QFont m_inputFont;
    QCheckBox* m_echoInputCheck;
    QComboBox* m_echoColorCombo;
    QSpinBox* m_historySizeSpin; // Command history size (20-5000)

    // Logging tab widgets
    QCheckBox* m_enableLogCheck;
    QLineEdit* m_logFileEdit;
    QPushButton* m_logFileBrowse;
    QComboBox* m_logFormatCombo;

    // Scripting tab widgets
    QCheckBox* m_enableScriptCheck;
    QLineEdit* m_scriptFileEdit;
    QPushButton* m_scriptFileBrowse;
    QComboBox* m_scriptLanguageCombo;

    // Paste to World tab widgets
    QLineEdit* m_pastePreambleEdit;
    QLineEdit* m_pastePostambleEdit;
    QLineEdit* m_pasteLinePreambleEdit;
    QLineEdit* m_pasteLinePostambleEdit;
    QSpinBox* m_pasteDelaySpin;
    QSpinBox* m_pasteDelayPerLinesSpin;
    QCheckBox* m_pasteCommentedSoftcodeCheck;
    QCheckBox* m_pasteEchoCheck;
    QCheckBox* m_pasteConfirmCheck;

    // Send File tab widgets
    QLineEdit* m_filePreambleEdit;
    QLineEdit* m_filePostambleEdit;
    QLineEdit* m_fileLinePreambleEdit;
    QLineEdit* m_fileLinePostambleEdit;
    QSpinBox* m_fileDelaySpin;
    QSpinBox* m_fileDelayPerLinesSpin;
    QCheckBox* m_fileCommentedSoftcodeCheck;
    QCheckBox* m_fileEchoCheck;
    QCheckBox* m_fileConfirmCheck;

    // Remote Access tab widgets
    QCheckBox* m_enableRemoteAccessCheck;
    QSpinBox* m_remotePortSpin;
    QLineEdit* m_remotePasswordEdit;
    QSpinBox* m_remoteScrollbackSpin;
    QSpinBox* m_remoteMaxClientsSpin;
};

#endif // WORLD_PROPERTIES_DIALOG_H
