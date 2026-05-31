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
