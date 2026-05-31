// test_recall_search_dialog_gtest.cpp - GoogleTest version
// Recall Search Dialog parity test
//
// Verifies that the recall line-type flags (Output/Commands/Notes) and the
// recall line preamble are per-world DOCUMENT state, mirroring the original
// MUSHclient CMUSHclientDoc::DoRecallText (doc.cpp:4930-4949):
//   - The dialog is SEEDED from the document (m_bRecallOutput/Commands/Notes,
//     m_strRecallLinePreamble) on load.
//   - On OK, the flags and preamble are COMMITTED back to the document, and a
//     changed preamble marks the document modified.
// Previously the Mushkin dialog read/wrote only global Database preferences,
// leaving the per-world document members orphaned.

#include "../src/ui/dialogs/recall_search_dialog.h"
#include "../src/world/world_document.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <gtest/gtest.h>

#include <memory>

// Friend of RecallSearchDialog (declared in the dialog header) so the test can
// drive the private load/save round-trip and inspect the widgets directly.
class RecallSearchDialogParityTest : public ::testing::Test {
  protected:
    std::unique_ptr<WorldDocument> doc;
    std::unique_ptr<RecallSearchDialog> dlg;

    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    void makeDialog()
    {
        // The constructor calls setupUi() + loadSettings(), seeding from the doc.
        dlg = std::make_unique<RecallSearchDialog>(doc.get());
    }

    void loadSettings()
    {
        dlg->loadSettings();
    }
    void clickOk()
    {
        dlg->onOkClicked();
    }

    bool outputChecked() const
    {
        return dlg->m_includeOutputCheck->isChecked();
    }
    bool commandsChecked() const
    {
        return dlg->m_includeCommandsCheck->isChecked();
    }
    bool notesChecked() const
    {
        return dlg->m_includeNotesCheck->isChecked();
    }
    QString preambleText() const
    {
        return dlg->m_linePreambleEdit->text();
    }

    void setOutputChecked(bool v)
    {
        dlg->m_includeOutputCheck->setChecked(v);
    }
    void setCommandsChecked(bool v)
    {
        dlg->m_includeCommandsCheck->setChecked(v);
    }
    void setNotesChecked(bool v)
    {
        dlg->m_includeNotesCheck->setChecked(v);
    }
    void setPreambleText(const QString& v)
    {
        dlg->m_linePreambleEdit->setText(v);
    }
    void setSearchText(const QString& v)
    {
        dlg->m_searchTextCombo->setCurrentText(v);
    }
};

// loadSettings() must seed the line-type checkboxes and preamble from the
// per-world document, not from global preferences.
TEST_F(RecallSearchDialogParityTest, LoadsLineTypesAndPreambleFromDocument)
{
    doc->m_bRecallOutput = false;
    doc->m_bRecallCommands = true;
    doc->m_bRecallNotes = true;
    doc->m_output.recall_line_preamble = QStringLiteral("[%H:%M:%S] ");

    makeDialog();

    EXPECT_FALSE(outputChecked());
    EXPECT_TRUE(commandsChecked());
    EXPECT_TRUE(notesChecked());
    EXPECT_EQ(preambleText(), QStringLiteral("[%H:%M:%S] "));
}

// onOkClicked() must commit the line-type flags back into the document.
TEST_F(RecallSearchDialogParityTest, OkCommitsLineTypesToDocument)
{
    doc->m_bRecallOutput = true;
    doc->m_bRecallCommands = true;
    doc->m_bRecallNotes = true;
    doc->m_output.recall_line_preamble.clear();

    makeDialog();

    // OK requires a search string and at least one line type selected.
    setSearchText(QStringLiteral("damage"));
    setOutputChecked(false);
    setCommandsChecked(true);
    setNotesChecked(false);

    clickOk();

    EXPECT_FALSE(doc->m_bRecallOutput);
    EXPECT_TRUE(doc->m_bRecallCommands);
    EXPECT_FALSE(doc->m_bRecallNotes);
}

// Changing the preamble on OK must write it back and mark the document modified.
TEST_F(RecallSearchDialogParityTest, OkCommitsPreambleAndMarksModified)
{
    doc->m_output.recall_line_preamble = QStringLiteral("old");
    doc->setModified(false);

    makeDialog();

    setSearchText(QStringLiteral("damage"));
    setOutputChecked(true);
    setPreambleText(QStringLiteral("new-preamble"));

    clickOk();

    EXPECT_EQ(doc->m_output.recall_line_preamble, QStringLiteral("new-preamble"));
    EXPECT_TRUE(doc->isModified());
}

// An unchanged preamble must NOT spuriously mark the document modified on OK.
TEST_F(RecallSearchDialogParityTest, OkUnchangedPreambleDoesNotMarkModified)
{
    doc->m_output.recall_line_preamble = QStringLiteral("same");
    doc->setModified(false);

    makeDialog();

    setSearchText(QStringLiteral("damage"));
    setOutputChecked(true);
    // Leave the preamble field exactly as loaded ("same").

    clickOk();

    EXPECT_EQ(doc->m_output.recall_line_preamble, QStringLiteral("same"));
    EXPECT_FALSE(doc->isModified());
}
