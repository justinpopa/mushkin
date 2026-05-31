#ifndef WORLD_PROPERTIES_DIALOG_H
#define WORLD_PROPERTIES_DIALOG_H

#include <QDialog>
#include <QFont>
#include <QTextEdit>
#include <array>

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
class QGroupBox;

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

    // Grants the parity gtest access to private load/save members so it can
    // verify the "Character name" field binds to m_name (player name), not
    // m_mush_name (world name). See test_world_properties_dialog_gtest.cpp.
    friend class WorldPropertiesDialogParityTest;

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

    /**
     * "Adjust Width" clicked - set the wrap column from the output window's
     * current pixel width (original CPrefsP14::OnAdjustWidth,
     * prefspropertypages.cpp:5893-5952).
     */
    void onAdjustWidthClicked();

    /**
     * "Adjust to Width" clicked - resize the output window so it is exactly wide
     * enough for the current wrap column (original CPrefsP14::OnAdjustToWidth,
     * prefspropertypages.cpp:8571-8623).
     */
    void onAdjustToWidthClicked();

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

    // Pure predicate: is the character-name field valid given the connect method?
    // Separated from validateSettings() so it can be exercised without a modal box.
    bool isCharacterNameValid() const;

    // Non-modal predicates mirroring the original CPrefsP9 DDV validation
    // (prefspropertypages.cpp:4695-4715). Separated from validateSettings() so
    // they can be exercised in tests without a modal message box.
    bool isSpeedwalkPrefixValid() const;
    bool isCommandStackCharacterValid() const;

    // Pure predicate mirroring the original CPrefsP14::DoDataExchange memory check
    // (prefspropertypages.cpp:5789-5813): warn when the output-buffer line count
    // changed, exceeds 1000, and the estimated allocation (16 MB OS + 60 bytes per
    // line) exceeds the machine's physical RAM. Kept pure (RAM passed in) so it can
    // be exercised in tests without depending on the host's memory.
    static bool isMaxLinesMemoryWarningNeeded(int oldLines, int newLines,
                                              quint64 totalPhysicalBytes);

    // Returns the machine's total physical RAM in bytes (0 if it cannot be queried).
    static quint64 totalPhysicalMemoryBytes();

    // Pure computation mirroring CPrefsP14::OnAdjustWidth (prefspropertypages.cpp:5937-5943):
    // wrap column = (output window pixel width − pixel offset) / average character width,
    // clamped to [20, MAX_LINE_WIDTH]. Kept pure so it can be exercised in tests without a
    // live output view. avgCharWidth <= 0 yields the minimum (20) to avoid divide-by-zero.
    static int computeAdjustedWrapColumn(int viewWidth, int pixelOffset, int avgCharWidth);

    // Validate UI fields before applying. Returns false (and shows a message box)
    // if a required field is invalid; mirrors original DDX/DDV validation.
    bool validateSettings();

    // Member variables
    WorldDocument* m_doc;
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;

    // Connection tab widgets
    QLineEdit* m_serverEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_nameEdit;
    QLineEdit* m_passwordEdit;
    QComboBox* m_connectMethodCombo;
    QTextEdit* m_connectTextEdit;
    QLabel* m_connectLineCountLabel; // "(N lines)" indicator for connect text

    // Proxy widgets
    QComboBox* m_proxyTypeCombo;
    QLineEdit* m_proxyServerEdit;
    QSpinBox* m_proxyPortSpin;
    QLineEdit* m_proxyUsernameEdit;
    QLineEdit* m_proxyPasswordEdit;

    // Output tab widgets
    QPushButton* m_outputFontButton;
    QLabel* m_outputFontLabel;
    QFont m_outputFont;
    std::array<QPushButton*, 16> m_colorButtons{}; // 8 normal + 8 bright ANSI colors
    std::array<QRgb, 16> m_ansiColors{};
    // Display options
    QCheckBox* m_wrapCheck;
    QSpinBox* m_wrapColumnSpin;
    QPushButton* m_adjustWidthButton;   // sets wrap column from output window width
    QPushButton* m_adjustToWidthButton; // resizes output window to fit wrap column
    QSpinBox* m_maxLinesSpin;
    QCheckBox* m_utf8Check;
    QCheckBox* m_nawsCheck;
    QLineEdit* m_terminalTypeEdit;
    QCheckBox* m_indentParasCheck;
    QCheckBox* m_showBoldCheck;
    QCheckBox* m_showItalicCheck;
    QCheckBox* m_showUnderlineCheck;
    QSpinBox* m_lineSpacingSpin;
    // Output-page options absent from the redesigned dialog but present in the
    // original CPrefsP14 (prefspropertypages.cpp:5748-5783) and fully backed by
    // WorldDocument. Without these controls the settings are unreachable from the UI.
    QCheckBox* m_enableBeepsCheck;
    QCheckBox* m_lineInformationCheck;
    QCheckBox* m_startPausedCheck;
    QSpinBox* m_pixelOffsetSpin;
    QCheckBox* m_autoFreezeCheck;
    QCheckBox* m_disableCompressionCheck;
    QCheckBox* m_useDefaultOutputFontCheck;
    QCheckBox* m_unpauseOnSendCheck;
    QCheckBox* m_alternativeInverseCheck;
    // Activity
    QCheckBox* m_flashIconCheck;

    // Helper for Output tab
    void updateColorButton(int index);

    // Helper for Connection tab: refresh the "(N lines)" indicator for connect text.
    void updateConnectLineCount();

    // Input tab widgets
    QPushButton* m_inputFontButton;
    QLabel* m_inputFontLabel;
    QFont m_inputFont;
    QCheckBox* m_echoInputCheck;
    QComboBox* m_echoColorCombo;
    QSpinBox* m_historySizeSpin; // Command history size (20-5000)
    QCheckBox* m_enableCommandStackCheck;
    QLineEdit* m_commandStackCharEdit;
    QCheckBox* m_enableSpeedwalkCheck;
    QLineEdit* m_speedwalkPrefixEdit;
    QLineEdit* m_speedwalkFillerEdit; // 'F' token filler (original IDC_SPEED_WALK_FILLER)
    QSpinBox* m_speedwalkDelaySpin;
    QCheckBox* m_escapeDeletesInputCheck;
    QCheckBox* m_noEchoOffCheck;
    // Spam prevention (original CPrefsP4: IDC_ENABLE_SPAM_PREVENTION / IDC_SPAM_LINE_COUNT /
    // IDC_SPAM_FILLER)
    QCheckBox* m_enableSpamPreventionCheck;
    QSpinBox* m_spamLineCountSpin;
    QLineEdit* m_spamMessageEdit;

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
    // Script prefix / editor fields (original CPrefsP17: IDC_SCRIPT_PREFIX /
    // IDC_SCRIPT_EDITOR, prefspropertypages.cpp:7007-7009). Backed by
    // m_scripting.prefix and m_scripting.editor.
    QLineEdit* m_scriptPrefixEdit;
    QLineEdit* m_scriptEditorEdit;
    QPushButton* m_scriptEditorBrowse;
    // Opens the MXP script-routines sub-dialog (original IDC_MXP_SCRIPTS /
    // CPrefsP17::OnMxpScripts, prefspropertypages.cpp:7069, 7300-7321).
    QPushButton* m_mxpScriptsButton;
    QLineEdit* m_onWorldOpenEdit;
    QLineEdit* m_onWorldCloseEdit;
    QLineEdit* m_onWorldConnectEdit;
    QLineEdit* m_onWorldDisconnectEdit;
    QLineEdit* m_onWorldGetFocusEdit;
    QLineEdit* m_onWorldLoseFocusEdit;
    QLineEdit* m_onWorldSaveEdit;
    // Previous script engine state, captured by saveSettings so applySettings can
    // reconcile the running engine (mirrors original SavePrefsP17 filename/enable check).
    QString m_prevScriptFilename;
    bool m_prevScriptEnabled = false;

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
    QLineEdit* m_remoteAuthorizedKeysEdit;
    QLabel* m_hostKeyFingerprintLabel;
    QSpinBox* m_remoteScrollbackSpin;
    QSpinBox* m_remoteMaxClientsSpin;
};

#endif // WORLD_PROPERTIES_DIALOG_H
