// test_world_properties_dialog_gtest.cpp - GoogleTest version
// World Properties Dialog parity test
//
// Verifies the "Character name" field (m_nameEdit) binds to the player name
// (m_name) rather than the world name (m_mush_name). This mirrors the original
// MUSHclient CPrefsP21::DoDataExchange, which maps IDC_CHARACTER to m_name
// (prefspropertypages.cpp:8087). Auto-login (connect_manager) sends
// "connect <m_name> <password>", so a dialog that stored the entered name into
// m_mush_name would make auto-login ignore the user-entered character name.

#include "../src/ui/dialogs/world_properties_dialog.h"
#include "../src/world/world_document.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <gtest/gtest.h>

#include <memory>

// Friend of WorldPropertiesDialog (declared in the dialog header) so the test
// can drive the private load/save round-trip directly.
class WorldPropertiesDialogParityTest : public ::testing::Test {
  protected:
    std::unique_ptr<WorldDocument> doc;
    std::unique_ptr<WorldPropertiesDialog> dlg;

    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    void makeDialog()
    {
        dlg = std::make_unique<WorldPropertiesDialog>(doc.get());
    }

    // Private-member accessors. Only this fixture is a friend of the dialog;
    // the TEST_F-generated subclasses are not, so all private access must live
    // in fixture methods.
    void loadSettings()
    {
        dlg->loadSettings();
    }
    void saveSettings()
    {
        dlg->saveSettings();
    }
    QString nameFieldText() const
    {
        return dlg->m_nameEdit->text();
    }
    void setNameFieldText(const QString& text)
    {
        dlg->m_nameEdit->setText(text);
    }

    // Spam-prevention field accessors (M77 UI parity).
    void setSpamFields(bool enabled, int lineCount, const QString& message)
    {
        dlg->m_enableSpamPreventionCheck->setChecked(enabled);
        dlg->m_spamLineCountSpin->setValue(lineCount);
        dlg->m_spamMessageEdit->setText(message);
    }
    bool spamEnabledChecked() const
    {
        return dlg->m_enableSpamPreventionCheck->isChecked();
    }
    int spamLineCountValue() const
    {
        return dlg->m_spamLineCountSpin->value();
    }
    QString spamMessageText() const
    {
        return dlg->m_spamMessageEdit->text();
    }

    // Language combo (M75: only Lua should be offered).
    int languageItemCount() const
    {
        return dlg->m_scriptLanguageCombo->count();
    }
    QString languageItem(int i) const
    {
        return dlg->m_scriptLanguageCombo->itemText(i);
    }

    // Connect-text line-count indicator (H105 LOW).
    void setConnectText(const QString& text)
    {
        dlg->m_connectTextEdit->setPlainText(text);
    }
    QString connectLineCountText() const
    {
        return dlg->m_connectLineCountLabel->text();
    }

    // Auto-connect validation (H105 MEDIUM).
    void setConnectMethodIndex(int idx)
    {
        dlg->m_connectMethodCombo->setCurrentIndex(idx);
    }
    // Use the non-modal predicate so the test never blocks on a message box.
    bool validate()
    {
        return dlg->isCharacterNameValid();
    }

    // M77: speed-walk filler / prefix accessors and non-modal validation predicates.
    QString speedwalkFillerText() const
    {
        return dlg->m_speedwalkFillerEdit->text();
    }
    void setSpeedwalk(bool enabled, const QString& prefix, const QString& filler)
    {
        dlg->m_enableSpeedwalkCheck->setChecked(enabled);
        dlg->m_speedwalkPrefixEdit->setText(prefix);
        dlg->m_speedwalkFillerEdit->setText(filler);
    }
    bool speedwalkPrefixValid() const
    {
        return dlg->isSpeedwalkPrefixValid();
    }

    // M77: command-stacking accessors and non-modal validation predicate.
    void setCommandStack(bool enabled, const QString& stackChar)
    {
        dlg->m_enableCommandStackCheck->setChecked(enabled);
        dlg->m_commandStackCharEdit->setText(stackChar);
    }
    bool commandStackCharValid() const
    {
        return dlg->isCommandStackCharacterValid();
    }

    // M78: terminal type edit max-length.
    int terminalTypeMaxLength() const
    {
        return dlg->m_terminalTypeEdit->maxLength();
    }

    // M78: output-page option accessors (beeps + the CPrefsP14 carryover fields).
    void setOutputOptions(bool beeps, bool lineInfo, bool startPaused, bool unpauseOnSend,
                          bool autoFreeze, bool disableCompression, bool useDefaultFont,
                          bool altInverse, int pixelOffset)
    {
        dlg->m_enableBeepsCheck->setChecked(beeps);
        dlg->m_lineInformationCheck->setChecked(lineInfo);
        dlg->m_startPausedCheck->setChecked(startPaused);
        dlg->m_unpauseOnSendCheck->setChecked(unpauseOnSend);
        dlg->m_autoFreezeCheck->setChecked(autoFreeze);
        dlg->m_disableCompressionCheck->setChecked(disableCompression);
        dlg->m_useDefaultOutputFontCheck->setChecked(useDefaultFont);
        dlg->m_alternativeInverseCheck->setChecked(altInverse);
        dlg->m_pixelOffsetSpin->setValue(pixelOffset);
    }
    bool beepsChecked() const
    {
        return dlg->m_enableBeepsCheck->isChecked();
    }
    bool lineInfoChecked() const
    {
        return dlg->m_lineInformationCheck->isChecked();
    }
    int pixelOffsetValue() const
    {
        return dlg->m_pixelOffsetSpin->value();
    }

    // L58: max-output-lines spin + validate-time memory warning predicate.
    void setMaxLines(int n)
    {
        dlg->m_maxLinesSpin->setValue(n);
    }
    static bool maxLinesWarningNeeded(int oldLines, int newLines, quint64 totalPhysicalBytes)
    {
        return WorldPropertiesDialog::isMaxLinesMemoryWarningNeeded(oldLines, newLines,
                                                                    totalPhysicalBytes);
    }

    // M75: script prefix / editor field accessors and MXP Scripts button presence.
    QString scriptPrefixText() const
    {
        return dlg->m_scriptPrefixEdit->text();
    }
    void setScriptPrefix(const QString& text)
    {
        dlg->m_scriptPrefixEdit->setText(text);
    }
    QString scriptEditorText() const
    {
        return dlg->m_scriptEditorEdit->text();
    }
    void setScriptEditor(const QString& text)
    {
        dlg->m_scriptEditorEdit->setText(text);
    }
    bool hasMxpScriptsButton() const
    {
        return dlg->m_mxpScriptsButton != nullptr;
    }

    // M149: applySettings must refresh the input view's wrap (original FixInputWrap()).
    void applySettings()
    {
        dlg->applySettings();
    }

    // M78: Adjust Width / Adjust to Width buttons (original IDC_ADJUST_WIDTH /
    // IDC_ADJUST_TO_WIDTH).
    bool hasAdjustWidthButton() const
    {
        return dlg->m_adjustWidthButton != nullptr;
    }
    bool hasAdjustToWidthButton() const
    {
        return dlg->m_adjustToWidthButton != nullptr;
    }
    void setWrapColumn(int n)
    {
        dlg->m_wrapColumnSpin->setValue(n);
    }
    int wrapColumnValue() const
    {
        return dlg->m_wrapColumnSpin->value();
    }
    static int computeAdjustedWrapColumn(int viewWidth, int pixelOffset, int avgCharWidth)
    {
        return WorldPropertiesDialog::computeAdjustedWrapColumn(viewWidth, pixelOffset,
                                                                avgCharWidth);
    }
};

// loadSettings() must populate the "Character name" field from m_name (player
// name), not m_mush_name (world name).
TEST_F(WorldPropertiesDialogParityTest, CharacterFieldLoadsFromPlayerName)
{
    doc->m_name = QStringLiteral("Gandalf");
    doc->m_mush_name = QStringLiteral("MiddleEarth MUD");

    makeDialog();
    loadSettings();

    EXPECT_EQ(nameFieldText(), QStringLiteral("Gandalf"));
    EXPECT_NE(nameFieldText(), QStringLiteral("MiddleEarth MUD"));
}

// saveSettings() must write the "Character name" field back into m_name and
// must leave m_mush_name (the world name) untouched.
TEST_F(WorldPropertiesDialogParityTest, CharacterFieldSavesToPlayerName)
{
    doc->m_name = QStringLiteral("OldName");
    doc->m_mush_name = QStringLiteral("MiddleEarth MUD");

    makeDialog();
    setNameFieldText(QStringLiteral("Frodo"));
    saveSettings();

    EXPECT_EQ(doc->m_name, QStringLiteral("Frodo"));
    EXPECT_EQ(doc->m_mush_name, QStringLiteral("MiddleEarth MUD"));
}

// M77: spam-prevention fields must load from and save to m_doc->m_spam.* so the
// (already-functional) DoSendMsg spam logic is configurable from the dialog.
TEST_F(WorldPropertiesDialogParityTest, SpamPreventionFieldsLoad)
{
    doc->m_spam.enabled = true;
    doc->m_spam.line_count = 42;
    doc->m_spam.message = QStringLiteral("score");

    makeDialog();
    loadSettings();

    EXPECT_TRUE(spamEnabledChecked());
    EXPECT_EQ(spamLineCountValue(), 42);
    EXPECT_EQ(spamMessageText(), QStringLiteral("score"));
}

TEST_F(WorldPropertiesDialogParityTest, SpamPreventionFieldsSave)
{
    makeDialog();
    setSpamFields(true, 17, QStringLiteral("look"));
    saveSettings();

    EXPECT_TRUE(doc->m_spam.enabled);
    EXPECT_EQ(doc->m_spam.line_count, 17);
    EXPECT_EQ(doc->m_spam.message, QStringLiteral("look"));
}

// M75: only Lua is supported, so the language combo must not offer the four
// non-functional languages (YueScript/MoonScript/Teal/Fennel) it used to.
TEST_F(WorldPropertiesDialogParityTest, LanguageComboOffersOnlyLua)
{
    makeDialog();
    EXPECT_EQ(languageItemCount(), 1);
    EXPECT_EQ(languageItem(0), QStringLiteral("Lua"));
}

// H105 LOW: the connect-text line-count indicator counts newlines, plus one for
// a trailing line with no newline (mirrors CPrefsP21::OnUpdateLineCount).
TEST_F(WorldPropertiesDialogParityTest, ConnectTextLineCountIndicator)
{
    makeDialog();

    setConnectText(QString());
    EXPECT_EQ(connectLineCountText(), QStringLiteral("(0 lines)"));

    setConnectText(QStringLiteral("one"));
    EXPECT_EQ(connectLineCountText(), QStringLiteral("(1 line)"));

    setConnectText(QStringLiteral("one\ntwo"));
    EXPECT_EQ(connectLineCountText(), QStringLiteral("(2 lines)"));

    setConnectText(QStringLiteral("one\ntwo\n"));
    EXPECT_EQ(connectLineCountText(), QStringLiteral("(2 lines)"));
}

// H105 MEDIUM: a blank character name must fail validation when an auto-connect
// method is selected, and pass when "No auto-connect" is selected.
TEST_F(WorldPropertiesDialogParityTest, BlankNameFailsValidationWithAutoConnect)
{
    makeDialog();

    // Index 1 = first auto-connect method (MUSH).
    setConnectMethodIndex(1);
    setNameFieldText(QString());
    EXPECT_FALSE(validate());

    // A whitespace-only name is also rejected (original trims first).
    setNameFieldText(QStringLiteral("   "));
    EXPECT_FALSE(validate());

    // A real name passes.
    setNameFieldText(QStringLiteral("Gandalf"));
    EXPECT_TRUE(validate());
}

TEST_F(WorldPropertiesDialogParityTest, BlankNameAllowedWithoutAutoConnect)
{
    makeDialog();

    // Index 0 = "No auto-connect".
    setConnectMethodIndex(0);
    setNameFieldText(QString());
    EXPECT_TRUE(validate());
}

// M77: the speed-walk filler field (IDC_SPEED_WALK_FILLER) round-trips through
// m_doc->m_speedwalk.filler.
TEST_F(WorldPropertiesDialogParityTest, SpeedwalkFillerFieldRoundTrips)
{
    doc->m_speedwalk.filler = QStringLiteral("look");
    makeDialog();
    loadSettings();
    EXPECT_EQ(speedwalkFillerText(), QStringLiteral("look"));

    setSpeedwalk(true, QStringLiteral("#"), QStringLiteral("scan"));
    saveSettings();
    EXPECT_EQ(doc->m_speedwalk.filler, QStringLiteral("scan"));
}

// M77: speed walking enabled with an empty prefix must fail validation; a
// non-empty prefix (or disabled speed walking) passes.
TEST_F(WorldPropertiesDialogParityTest, SpeedwalkPrefixValidation)
{
    makeDialog();

    setSpeedwalk(true, QString(), QString());
    EXPECT_FALSE(speedwalkPrefixValid());

    setSpeedwalk(true, QStringLiteral("#"), QString());
    EXPECT_TRUE(speedwalkPrefixValid());

    // Disabled => empty prefix is allowed.
    setSpeedwalk(false, QString(), QString());
    EXPECT_TRUE(speedwalkPrefixValid());
}

// M77: command stacking enabled requires a non-empty, printable stack character.
TEST_F(WorldPropertiesDialogParityTest, CommandStackCharacterValidation)
{
    makeDialog();

    setCommandStack(true, QString());
    EXPECT_FALSE(commandStackCharValid()); // empty

    setCommandStack(true, QStringLiteral("\t"));
    EXPECT_FALSE(commandStackCharValid()); // non-printable control char

    setCommandStack(true, QStringLiteral(";"));
    EXPECT_TRUE(commandStackCharValid()); // printable

    // Disabled => no requirement.
    setCommandStack(false, QString());
    EXPECT_TRUE(commandStackCharValid());
}

// M77: when stacking is disabled and the character is blank, saveSettings defaults
// it to a single space (mirrors original prefspropertypages.cpp:4720-4721).
TEST_F(WorldPropertiesDialogParityTest, DisabledStackCharacterDefaultsToSpace)
{
    makeDialog();
    setCommandStack(false, QString());
    saveSettings();
    EXPECT_EQ(doc->m_input.command_stack_character, QStringLiteral(" "));
}

// M78: terminal type edit enforces the original DDV_MaxChars(20).
TEST_F(WorldPropertiesDialogParityTest, TerminalTypeMaxLength)
{
    makeDialog();
    EXPECT_EQ(terminalTypeMaxLength(), 20);
}

// M78: the output-page options carried over from CPrefsP14 round-trip through
// their WorldDocument backing fields.
TEST_F(WorldPropertiesDialogParityTest, OutputOptionsLoad)
{
    doc->m_sound.enable_beeps = false;
    doc->m_display.line_information = false;
    doc->m_display.pixel_offset = 7;

    makeDialog();
    loadSettings();

    EXPECT_FALSE(beepsChecked());
    EXPECT_FALSE(lineInfoChecked());
    EXPECT_EQ(pixelOffsetValue(), 7);
}

TEST_F(WorldPropertiesDialogParityTest, OutputOptionsSave)
{
    makeDialog();
    setOutputOptions(/*beeps=*/false, /*lineInfo=*/false, /*startPaused=*/true,
                     /*unpauseOnSend=*/false, /*autoFreeze=*/false,
                     /*disableCompression=*/true, /*useDefaultFont=*/true,
                     /*altInverse=*/true, /*pixelOffset=*/12);
    saveSettings();

    EXPECT_FALSE(doc->m_sound.enable_beeps);
    EXPECT_FALSE(doc->m_display.line_information);
    EXPECT_TRUE(doc->m_bStartPaused);
    EXPECT_FALSE(doc->m_bUnpauseOnSend);
    EXPECT_FALSE(doc->m_display.auto_freeze);
    EXPECT_TRUE(doc->m_bDisableCompression);
    EXPECT_NE(doc->m_bUseDefaultOutputFont, 0);
    EXPECT_TRUE(doc->m_bAlternativeInverse);
    EXPECT_EQ(doc->m_display.pixel_offset, 12);
}

// L58: saveSettings must mark the document modified when at least one committed value
// actually changed (mirrors CMUSHclientDoc::OnApply / SetModifiedFlag(TRUE),
// configuration.cpp:1973-2016). It must NOT mark modified when nothing changed.
TEST_F(WorldPropertiesDialogParityTest, SaveMarksDocumentModifiedOnChange)
{
    doc->m_name = QStringLiteral("OldName");
    makeDialog();
    loadSettings();

    // Settle widget/doc into a fixed point first: a few load/save fields default
    // asymmetrically (e.g. remote port 0 -> 4001), so the very first save can
    // legitimately report a change. After one save the dialog and doc agree.
    saveSettings();
    doc->setModified(false);

    // A second save with no edits must leave the document clean.
    saveSettings();
    EXPECT_FALSE(doc->isModified());

    // An actual edit must flip the modified flag.
    setNameFieldText(QStringLiteral("NewName"));
    saveSettings();
    EXPECT_TRUE(doc->isModified());
}

// L58: once modified, a subsequent no-op save must not clear the flag.
TEST_F(WorldPropertiesDialogParityTest, SaveNeverClearsModifiedFlag)
{
    makeDialog();
    loadSettings();
    doc->setModified(true);
    saveSettings(); // no edits
    EXPECT_TRUE(doc->isModified());
}

// M75: the script prefix and editor fields round-trip through m_scripting.prefix /
// m_scripting.editor (original IDC_SCRIPT_PREFIX / IDC_SCRIPT_EDITOR).
TEST_F(WorldPropertiesDialogParityTest, ScriptPrefixAndEditorRoundTrip)
{
    doc->m_scripting.prefix = QStringLiteral("@");
    doc->m_scripting.editor = QStringLiteral("/usr/bin/vim");
    makeDialog();
    loadSettings();
    EXPECT_EQ(scriptPrefixText(), QStringLiteral("@"));
    EXPECT_EQ(scriptEditorText(), QStringLiteral("/usr/bin/vim"));

    setScriptPrefix(QStringLiteral("!"));
    setScriptEditor(QStringLiteral("/usr/local/bin/nano"));
    saveSettings();
    EXPECT_EQ(doc->m_scripting.prefix, QStringLiteral("!"));
    EXPECT_EQ(doc->m_scripting.editor, QStringLiteral("/usr/local/bin/nano"));
}

// M75: the MXP Scripts button (original IDC_MXP_SCRIPTS) must exist on the dialog.
TEST_F(WorldPropertiesDialogParityTest, MxpScriptsButtonPresent)
{
    makeDialog();
    EXPECT_TRUE(hasMxpScriptsButton());
}

// M78: the output-lines memory-warning predicate mirrors prefspropertypages.cpp:5789-5811.
TEST_F(WorldPropertiesDialogParityTest, MaxLinesMemoryWarningPredicate)
{
    // 1 GB of RAM.
    const quint64 oneGb = 1024ULL * 1024ULL * 1024ULL;

    // Unchanged count: never warns even if huge.
    EXPECT_FALSE(maxLinesWarningNeeded(5'000'000, 5'000'000, oneGb));

    // Changed but <= 1000: never warns.
    EXPECT_FALSE(maxLinesWarningNeeded(500, 1000, oneGb));

    // Changed, > 1000, but allocation (16 MB + 60 B/line) fits in RAM: no warning.
    EXPECT_FALSE(maxLinesWarningNeeded(1000, 50'000, oneGb));

    // Changed, > 1000, allocation exceeds RAM: warns. 16 MB + 60 B/line > 1 GB needs
    // ~17.9M lines; use a generous count to clear the threshold.
    EXPECT_TRUE(maxLinesWarningNeeded(1000, 30'000'000, oneGb));

    // A zero RAM reading (query failed) suppresses the warning.
    EXPECT_FALSE(maxLinesWarningNeeded(1000, 30'000'000, 0));
}

// M149: applySettings() must emit inputSettingsChanged so the input view re-applies its
// word-wrap, mirroring the original's unconditional FixInputWrap() call after committing
// the wrap column (configuration.cpp:1373).
TEST_F(WorldPropertiesDialogParityTest, ApplyRefreshesInputWrap)
{
    makeDialog();
    loadSettings();

    int emissions = 0;
    QObject::connect(doc.get(), &WorldDocument::inputSettingsChanged,
                     [&emissions]() { ++emissions; });

    applySettings();

    EXPECT_GE(emissions, 1);
}

// M78: the Adjust Width / Adjust to Width buttons (IDC_ADJUST_WIDTH / IDC_ADJUST_TO_WIDTH)
// must exist on the dialog.
TEST_F(WorldPropertiesDialogParityTest, AdjustWidthButtonsPresent)
{
    makeDialog();
    EXPECT_TRUE(hasAdjustWidthButton());
    EXPECT_TRUE(hasAdjustToWidthButton());
}

// M78: computeAdjustedWrapColumn mirrors CPrefsP14::OnAdjustWidth
// (prefspropertypages.cpp:5937-5943): column = (width - offset) / avgCharWidth, clamped.
TEST_F(WorldPropertiesDialogParityTest, AdjustWidthComputation)
{
    // 800px window, 1px offset, 8px char => (800-1)/8 = 99 columns.
    EXPECT_EQ(computeAdjustedWrapColumn(800, 1, 8), 99);

    // Clamps to the minimum of 20 when the window is tiny.
    EXPECT_EQ(computeAdjustedWrapColumn(80, 1, 8), 20);

    // A zero (or negative) average char width is degenerate => minimum, no divide-by-zero.
    EXPECT_EQ(computeAdjustedWrapColumn(800, 1, 0), 20);

    // Clamps to MAX_LINE_WIDTH (32000) for absurd widths.
    EXPECT_EQ(computeAdjustedWrapColumn(1'000'000, 0, 1), 32000);
}
