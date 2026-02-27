// test_xml_roundtrip_gtest.cpp - GoogleTest version
// XML Round-trip Test
// Timer XML Round-trip Test
//
// Verifies that triggers, aliases, and timers save and load correctly with all
// compatibility fixes applied (user attribute, style decomposition, etc.)

#include "../src/automation/alias.h"
#include "../src/automation/timer.h"
#include "../src/automation/trigger.h"
#include "../src/world/color_utils.h"
#include "../src/world/world_document.h"
#include "../src/world/xml_serialization.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QUuid>
#include <gtest/gtest.h>

// Helper to clean up all files related to SaveWorldXML atomic save
// (main file, .tmp, and .bak files)
static void cleanupSaveFiles(const QString& filename)
{
    QFile::remove(filename);
    QFile::remove(filename + ".tmp");
    QFile::remove(filename + ".bak");
}

// Generate a unique temp filename without creating the file
// This avoids Windows file locking issues with QTemporaryFile
static QString generateTempFilename(const QString& prefix = "test")
{
    QString tempDir = QDir::tempPath();
    QString uuid = QUuid::createUuid().toString(QUuid::Id128);
    return tempDir + "/" + prefix + "_" + uuid + ".mcl";
}

// Test fixture for XML roundtrip tests
class XmlRoundtripTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc1 = nullptr;
        doc2 = nullptr;
    }

    void TearDown() override
    {
        if (doc1) {
            delete doc1;
            doc1 = nullptr;
        }
        if (doc2) {
            delete doc2;
            doc2 = nullptr;
        }
        if (!tmpFilename.isEmpty()) {
            cleanupSaveFiles(tmpFilename);
            tmpFilename.clear();
        }
    }

    // Helper to save and read back the XML file content
    QString saveAndReadXml(WorldDocument* doc, const QString& prefix = "roundtrip")
    {
        // Generate unique filename (avoids Windows file locking with QTemporaryFile)
        tmpFilename = generateTempFilename(prefix);

        EXPECT_TRUE(XmlSerialization::SaveWorldXML(doc, tmpFilename)) << "Failed to save XML";

        QFile file(tmpFilename);
        EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open saved file";
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        return content;
    }

    WorldDocument* doc1;
    WorldDocument* doc2;
    QString tmpFilename;
};

// Test 1: Trigger round-trip with style decomposition
TEST_F(XmlRoundtripTest, TriggerStyleDecomposition)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Round-trip Test";

    Trigger* trigger = new Trigger();
    trigger->label = "test_trigger";
    trigger->internal_name = "test_trigger";
    trigger->trigger = "You have * gold";
    trigger->contents = "say I have %1 gold!";
    trigger->procedure = "on_gold";
    trigger->enabled = true;
    trigger->send_to = 0; // SendToWorld
    trigger->sequence = 100;
    trigger->ignore_case = false;
    trigger->use_regexp = false;
    trigger->keep_evaluating = true;
    trigger->omit_from_output = false;
    trigger->omit_from_log = false;
    trigger->expand_variables = true;
    trigger->group = "Combat";

    // Set style attributes (these should decompose into individual XML attributes)
    trigger->style = 0x0001 | 0x0004; // HILITE | BLINK (make_bold | make_italic)

    doc1->addTrigger("test_trigger", std::unique_ptr<Trigger>(trigger));

    QString content = saveAndReadXml(doc1);

    // Verify decomposed style attributes (not raw style number)
    EXPECT_TRUE(content.contains("make_bold=\"y\"")) << "Missing make_bold attribute";
    EXPECT_TRUE(content.contains("make_italic=\"y\"")) << "Missing make_italic attribute";
    EXPECT_FALSE(content.contains("istyle="))
        << "Found raw istyle attribute (should be decomposed)";

    // Load into new document
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename)) << "Failed to load XML";

    // Verify trigger was loaded
    Trigger* loaded = doc2->getTrigger("test_trigger");
    ASSERT_NE(loaded, nullptr) << "Trigger not found after load";

    EXPECT_EQ(loaded->label, trigger->label);
    EXPECT_EQ(loaded->trigger, trigger->trigger);
    EXPECT_EQ(loaded->contents, trigger->contents);
    EXPECT_EQ(loaded->procedure, trigger->procedure);
    EXPECT_EQ(loaded->enabled, trigger->enabled);
    EXPECT_EQ(loaded->send_to, trigger->send_to);
    EXPECT_EQ(loaded->sequence, trigger->sequence);
    EXPECT_EQ(loaded->group, trigger->group);

    // Verify style was composed correctly from individual attributes
    EXPECT_EQ(loaded->style, trigger->style)
        << "style mismatch (make_bold/italic not composed correctly)";
}

// Test 2: Trigger round-trip with match attribute decomposition
TEST_F(XmlRoundtripTest, TriggerMatchDecomposition)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Match Test";

    Trigger* trigger = new Trigger();
    trigger->label = "match_trigger";
    trigger->internal_name = "match_trigger";
    trigger->trigger = "test pattern";

    // Set match attributes (should decompose into text_colour, back_colour, bold, italic, etc.)
    // Bit layout: bits 0-3: style flags, bits 4-7: text_colour, bits 8-11: back_colour, bits
    // 12-15: match flags
    trigger->match_type = (5 << 4) | (2 << 8) | 0x0001 | 0x4000; // text=5, back=2, bold, match_italic

    doc1->addTrigger("match_trigger", std::unique_ptr<Trigger>(trigger));

    QString content = saveAndReadXml(doc1);

    // Verify decomposed match attributes
    EXPECT_TRUE(content.contains("text_colour=\"5\"")) << "Missing text_colour attribute";
    EXPECT_TRUE(content.contains("back_colour=\"2\"")) << "Missing back_colour attribute";
    EXPECT_TRUE(content.contains("bold=\"y\"")) << "Missing bold attribute";
    EXPECT_TRUE(content.contains("italic=\"y\"")) << "Missing italic attribute";
    EXPECT_FALSE(content.contains("imatch="))
        << "Found raw imatch attribute (should be decomposed)";

    // Load and verify
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Trigger* loaded = doc2->getTrigger("match_trigger");
    ASSERT_NE(loaded, nullptr);

    // Verify match_type was composed correctly
    EXPECT_EQ(loaded->match_type, trigger->match_type)
        << "match_type mismatch (text/back colour or style flags not composed correctly)";
}

// Test 3: Trigger round-trip with custom colors
TEST_F(XmlRoundtripTest, TriggerCustomColors)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Color Test";

    Trigger* trigger = new Trigger();
    trigger->label = "color_trigger";
    trigger->internal_name = "color_trigger";
    trigger->trigger = "color test";

    // Set custom color (should save as +1)
    trigger->colour = 42;

    // Set BGR colors (MUSHclient COLORREF format, should save as hex RGB names)
    trigger->other_foreground = BGR(255, 128, 64);  // Will save as #FF8040
    trigger->other_background = BGR(32, 64, 128);   // Will save as #204080

    doc1->addTrigger("color_trigger", std::unique_ptr<Trigger>(trigger));

    QString content = saveAndReadXml(doc1);

    // Verify custom_colour is +1
    EXPECT_TRUE(content.contains("custom_colour=\"43\"")) << "custom_colour should be 43 (42+1)";

    // Verify RGB colors are hex names
    EXPECT_TRUE(content.contains("other_text_colour=\"#FF8040\""))
        << "other_text_colour should be #FF8040";
    EXPECT_TRUE(content.contains("other_back_colour=\"#204080\""))
        << "other_back_colour should be #204080";

    // Load and verify
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Trigger* loaded = doc2->getTrigger("color_trigger");
    ASSERT_NE(loaded, nullptr);

    // Verify custom_colour was loaded correctly (decremented)
    EXPECT_EQ(loaded->colour, trigger->colour)
        << "colour mismatch (custom_colour not decremented correctly)";

    // Verify RGB colors
    EXPECT_EQ(loaded->other_foreground, trigger->other_foreground) << "other_foreground mismatch";
    EXPECT_EQ(loaded->other_background, trigger->other_background) << "other_background mismatch";
}

// Test 4: Trigger round-trip with user attribute
TEST_F(XmlRoundtripTest, TriggerUserAttribute)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger User Test";

    Trigger* trigger = new Trigger();
    trigger->label = "user_trigger";
    trigger->internal_name = "user_trigger";
    trigger->trigger = "user test";

    // Set user option (should save as "user" not "user_option")
    trigger->user_option = 123;

    doc1->addTrigger("user_trigger", std::unique_ptr<Trigger>(trigger));

    QString content = saveAndReadXml(doc1);

    // Verify "user" attribute (not "user_option")
    EXPECT_TRUE(content.contains("user=\"123\"")) << "Missing user attribute";
    EXPECT_FALSE(content.contains("user_option=")) << "Found user_option (should be user)";

    // Load and verify
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Trigger* loaded = doc2->getTrigger("user_trigger");
    ASSERT_NE(loaded, nullptr);

    // Verify user option
    EXPECT_EQ(loaded->user_option, trigger->user_option) << "user_option mismatch";
}

// Test 5: Complete trigger round-trip with all attributes
TEST_F(XmlRoundtripTest, TriggerCompleteRoundtrip)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Complete Trigger Test";

    Trigger* trigger = new Trigger();
    trigger->label = "complete_trigger";
    trigger->internal_name = "complete_trigger";
    trigger->trigger = "You have * gold";
    trigger->contents = "say I have %1 gold!";
    trigger->procedure = "on_gold";
    trigger->enabled = true;
    trigger->send_to = 0;
    trigger->sequence = 100;
    trigger->ignore_case = false;
    trigger->use_regexp = false;
    trigger->keep_evaluating = true;
    trigger->omit_from_output = false;
    trigger->omit_from_log = false;
    trigger->expand_variables = true;
    trigger->group = "Combat";
    trigger->style = 0x0001 | 0x0004;
    trigger->match_type = (5 << 4) | (2 << 8) | 0x0001 | 0x4000;
    trigger->colour = 42;
    trigger->other_foreground = BGR(255, 128, 64);  // Stored as BGR/COLORREF
    trigger->other_background = BGR(32, 64, 128);   // Stored as BGR/COLORREF
    trigger->user_option = 123;

    doc1->addTrigger("complete_trigger", std::unique_ptr<Trigger>(trigger));

    saveAndReadXml(doc1);

    // Load and verify all attributes
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Trigger* loaded = doc2->getTrigger("complete_trigger");
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->label, trigger->label);
    EXPECT_EQ(loaded->trigger, trigger->trigger);
    EXPECT_EQ(loaded->contents, trigger->contents);
    EXPECT_EQ(loaded->procedure, trigger->procedure);
    EXPECT_EQ(loaded->enabled, trigger->enabled);
    EXPECT_EQ(loaded->send_to, trigger->send_to);
    EXPECT_EQ(loaded->sequence, trigger->sequence);
    EXPECT_EQ(loaded->group, trigger->group);
    EXPECT_EQ(loaded->style, trigger->style);
    EXPECT_EQ(loaded->match_type, trigger->match_type);
    EXPECT_EQ(loaded->colour, trigger->colour);
    EXPECT_EQ(loaded->other_foreground, trigger->other_foreground);
    EXPECT_EQ(loaded->other_background, trigger->other_background);
    EXPECT_EQ(loaded->user_option, trigger->user_option);
}

// Test 6: Alias round-trip with user attribute
TEST_F(XmlRoundtripTest, AliasUserAttribute)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Alias Round-trip Test";

    auto alias = std::make_unique<Alias>();
    alias->label = "test_alias";
    alias->internal_name = "test_alias";
    alias->name = "n";
    alias->contents = "north";
    alias->procedure = "on_north";
    alias->enabled = true;
    alias->send_to = 0;
    alias->sequence = 100;
    alias->ignore_case = true;
    alias->use_regexp = false;
    alias->keep_evaluating = false;
    alias->expand_variables = true;
    alias->echo_alias = true;
    alias->group = "Movement";
    alias->user_option = 456;

    // Save expected values before move
    QString expectedLabel = alias->label;
    QString expectedName = alias->name;
    QString expectedContents = alias->contents;
    QString expectedProcedure = alias->procedure;
    bool expectedEnabled = alias->enabled;
    int expectedSendTo = alias->send_to;
    int expectedSequence = alias->sequence;
    bool expectedIgnoreCase = alias->ignore_case;
    bool expectedEchoAlias = alias->echo_alias;
    QString expectedGroup = alias->group;
    int expectedUserOption = alias->user_option;

    doc1->addAlias("test_alias", std::move(alias));

    QString content = saveAndReadXml(doc1);

    // Verify "user" attribute (not "user_option")
    EXPECT_TRUE(content.contains("user=\"456\"")) << "Missing user attribute in alias";
    EXPECT_FALSE(content.contains("user_option=")) << "Found user_option in alias (should be user)";

    // Load and verify
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Alias* loaded = doc2->getAlias("test_alias");
    ASSERT_NE(loaded, nullptr) << "Alias not found after load";

    EXPECT_EQ(loaded->label, expectedLabel);
    EXPECT_EQ(loaded->name, expectedName);
    EXPECT_EQ(loaded->contents, expectedContents);
    EXPECT_EQ(loaded->procedure, expectedProcedure);
    EXPECT_EQ(loaded->enabled, expectedEnabled);
    EXPECT_EQ(loaded->send_to, expectedSendTo);
    EXPECT_EQ(loaded->sequence, expectedSequence);
    EXPECT_EQ(loaded->ignore_case, expectedIgnoreCase);
    EXPECT_EQ(loaded->echo_alias, expectedEchoAlias);
    EXPECT_EQ(loaded->group, expectedGroup);
    EXPECT_EQ(loaded->user_option, expectedUserOption);
}

// Test 7: Timer round-trip with interval type
TEST_F(XmlRoundtripTest, TimerIntervalRoundtrip)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Timer Round-trip Test";

    Timer* timer = new Timer();
    timer->label = "test_timer";
    timer->enabled = true;
    timer->type = Timer::eInterval;
    timer->every_hour = 0;
    timer->every_minute = 5;
    timer->every_second = 30.5;
    timer->offset_hour = 0;
    timer->offset_minute = 2;
    timer->offset_second = 15.25;
    timer->contents = "say Timer fired!";
    timer->send_to = 0;
    timer->procedure = "onTimerFire";
    timer->variable = "";
    timer->one_shot = false;
    timer->active_when_closed = true;
    timer->omit_from_output = false;
    timer->omit_from_log = false;
    timer->group = "Maintenance";
    timer->user_option = 789;

    doc1->addTimer("test_timer", std::unique_ptr<Timer>(timer));

    QString content = saveAndReadXml(doc1);

    // Verify timer element exists
    EXPECT_TRUE(content.contains("<timers>")) << "Missing <timers> element";
    EXPECT_TRUE(content.contains("<timer")) << "Missing <timer> element";
    EXPECT_TRUE(content.contains("name=\"test_timer\"")) << "Missing timer name";

    // Verify timing attributes (original MUSHclient compatible format)
    EXPECT_TRUE(content.contains("at_time=\"n\""))
        << "Missing at_time attribute (n = interval timer)";
    EXPECT_TRUE(content.contains("minute=\"5\"")) << "Missing minute attribute";
    EXPECT_TRUE(content.contains("second=\"30.5")) << "Missing second attribute";
    EXPECT_TRUE(content.contains("offset_minute=\"2\"")) << "Missing offset_minute attribute";

    // Verify "user" attribute (not "user_option")
    EXPECT_TRUE(content.contains("user=\"789\"")) << "Missing user attribute in timer";
    EXPECT_FALSE(content.contains("user_option=")) << "Found user_option in timer (should be user)";

    // Load and verify
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Timer* loaded = doc2->getTimer("test_timer");
    ASSERT_NE(loaded, nullptr) << "Timer not found after load";

    EXPECT_EQ(loaded->label, timer->label);
    EXPECT_EQ(loaded->enabled, timer->enabled);
    EXPECT_EQ(loaded->type, timer->type);
    EXPECT_EQ(loaded->every_minute, timer->every_minute);
    EXPECT_DOUBLE_EQ(loaded->every_second, timer->every_second);
    EXPECT_EQ(loaded->offset_minute, timer->offset_minute);
    EXPECT_DOUBLE_EQ(loaded->offset_second, timer->offset_second);
    EXPECT_EQ(loaded->contents, timer->contents);
    EXPECT_EQ(loaded->send_to, timer->send_to);
    EXPECT_EQ(loaded->procedure, timer->procedure);
    EXPECT_EQ(loaded->one_shot, timer->one_shot);
    EXPECT_EQ(loaded->active_when_closed, timer->active_when_closed);
    EXPECT_EQ(loaded->omit_from_output, timer->omit_from_output);
    EXPECT_EQ(loaded->omit_from_log, timer->omit_from_log);
    EXPECT_EQ(loaded->group, timer->group);
    EXPECT_EQ(loaded->user_option, timer->user_option);
}

// Main function required for GoogleTest
int main(int argc, char** argv)
{
    // Initialize Qt (required for Qt objects)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
