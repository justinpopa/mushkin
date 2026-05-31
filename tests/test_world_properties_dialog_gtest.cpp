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
#include <QLineEdit>
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
