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
    trigger->strLabel = "test_trigger";
    trigger->strInternalName = "test_trigger";
    trigger->trigger = "You have * gold";
    trigger->contents = "say I have %1 gold!";
    trigger->strProcedure = "on_gold";
    trigger->bEnabled = true;
    trigger->iSendTo = 0; // SendToWorld
    trigger->iSequence = 100;
    trigger->ignore_case = false;
    trigger->bRegexp = false;
    trigger->bKeepEvaluating = true;
    trigger->bOmitFromOutput = false;
    trigger->omit_from_log = false;
    trigger->bExpandVariables = true;
    trigger->strGroup = "Combat";

    // Set style attributes (these should decompose into individual XML attributes)
    trigger->iStyle = 0x0001 | 0x0004; // HILITE | BLINK (make_bold | make_italic)

    doc1->addTrigger("test_trigger", std::unique_ptr<Trigger>(trigger));

    QString content = saveAndReadXml(doc1);

    // Verify decomposed style attributes (not raw iStyle number)
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

    EXPECT_EQ(loaded->strLabel, trigger->strLabel);
    EXPECT_EQ(loaded->trigger, trigger->trigger);
    EXPECT_EQ(loaded->contents, trigger->contents);
    EXPECT_EQ(loaded->strProcedure, trigger->strProcedure);
    EXPECT_EQ(loaded->bEnabled, trigger->bEnabled);
    EXPECT_EQ(loaded->iSendTo, trigger->iSendTo);
    EXPECT_EQ(loaded->iSequence, trigger->iSequence);
    EXPECT_EQ(loaded->strGroup, trigger->strGroup);

    // Verify iStyle was composed correctly from individual attributes
    EXPECT_EQ(loaded->iStyle, trigger->iStyle)
        << "iStyle mismatch (make_bold/italic not composed correctly)";
}

// Test 2: Trigger round-trip with match attribute decomposition
TEST_F(XmlRoundtripTest, TriggerMatchDecomposition)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Match Test";

    Trigger* trigger = new Trigger();
    trigger->strLabel = "match_trigger";
    trigger->strInternalName = "match_trigger";
    trigger->trigger = "test pattern";

    // Set match attributes (should decompose into text_colour, back_colour, bold, italic, etc.)
    // Bit layout: bits 0-3: style flags, bits 4-7: text_colour, bits 8-11: back_colour, bits
    // 12-15: match flags
    trigger->iMatch = (5 << 4) | (2 << 8) | 0x0001 | 0x4000; // text=5, back=2, bold, match_italic

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

    // Verify iMatch was composed correctly
    EXPECT_EQ(loaded->iMatch, trigger->iMatch)
        << "iMatch mismatch (text/back colour or style flags not composed correctly)";
}

// Test 3: Trigger round-trip with custom colors
TEST_F(XmlRoundtripTest, TriggerCustomColors)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Color Test";

    Trigger* trigger = new Trigger();
    trigger->strLabel = "color_trigger";
    trigger->strInternalName = "color_trigger";
    trigger->trigger = "color test";

    // Set custom color (should save as +1)
    trigger->colour = 42;

    // Set BGR colors (MUSHclient COLORREF format, should save as hex RGB names)
    trigger->iOtherForeground = BGR(255, 128, 64);  // Will save as #FF8040
    trigger->iOtherBackground = BGR(32, 64, 128);   // Will save as #204080

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
    EXPECT_EQ(loaded->iOtherForeground, trigger->iOtherForeground) << "iOtherForeground mismatch";
    EXPECT_EQ(loaded->iOtherBackground, trigger->iOtherBackground) << "iOtherBackground mismatch";
}

// Test 4: Trigger round-trip with user attribute
TEST_F(XmlRoundtripTest, TriggerUserAttribute)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger User Test";

    Trigger* trigger = new Trigger();
    trigger->strLabel = "user_trigger";
    trigger->strInternalName = "user_trigger";
    trigger->trigger = "user test";

    // Set user option (should save as "user" not "user_option")
    trigger->iUserOption = 123;

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
    EXPECT_EQ(loaded->iUserOption, trigger->iUserOption) << "iUserOption mismatch";
}

// Test 5: Complete trigger round-trip with all attributes
TEST_F(XmlRoundtripTest, TriggerCompleteRoundtrip)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Complete Trigger Test";

    Trigger* trigger = new Trigger();
    trigger->strLabel = "complete_trigger";
    trigger->strInternalName = "complete_trigger";
    trigger->trigger = "You have * gold";
    trigger->contents = "say I have %1 gold!";
    trigger->strProcedure = "on_gold";
    trigger->bEnabled = true;
    trigger->iSendTo = 0;
    trigger->iSequence = 100;
    trigger->ignore_case = false;
    trigger->bRegexp = false;
    trigger->bKeepEvaluating = true;
    trigger->bOmitFromOutput = false;
    trigger->omit_from_log = false;
    trigger->bExpandVariables = true;
    trigger->strGroup = "Combat";
    trigger->iStyle = 0x0001 | 0x0004;
    trigger->iMatch = (5 << 4) | (2 << 8) | 0x0001 | 0x4000;
    trigger->colour = 42;
    trigger->iOtherForeground = BGR(255, 128, 64);  // Stored as BGR/COLORREF
    trigger->iOtherBackground = BGR(32, 64, 128);   // Stored as BGR/COLORREF
    trigger->iUserOption = 123;

    doc1->addTrigger("complete_trigger", std::unique_ptr<Trigger>(trigger));

    saveAndReadXml(doc1);

    // Load and verify all attributes
    doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, tmpFilename));

    Trigger* loaded = doc2->getTrigger("complete_trigger");
    ASSERT_NE(loaded, nullptr);

    EXPECT_EQ(loaded->strLabel, trigger->strLabel);
    EXPECT_EQ(loaded->trigger, trigger->trigger);
    EXPECT_EQ(loaded->contents, trigger->contents);
    EXPECT_EQ(loaded->strProcedure, trigger->strProcedure);
    EXPECT_EQ(loaded->bEnabled, trigger->bEnabled);
    EXPECT_EQ(loaded->iSendTo, trigger->iSendTo);
    EXPECT_EQ(loaded->iSequence, trigger->iSequence);
    EXPECT_EQ(loaded->strGroup, trigger->strGroup);
    EXPECT_EQ(loaded->iStyle, trigger->iStyle);
    EXPECT_EQ(loaded->iMatch, trigger->iMatch);
    EXPECT_EQ(loaded->colour, trigger->colour);
    EXPECT_EQ(loaded->iOtherForeground, trigger->iOtherForeground);
    EXPECT_EQ(loaded->iOtherBackground, trigger->iOtherBackground);
    EXPECT_EQ(loaded->iUserOption, trigger->iUserOption);
}

// Test 6: Alias round-trip with user attribute
TEST_F(XmlRoundtripTest, AliasUserAttribute)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Alias Round-trip Test";

    auto alias = std::make_unique<Alias>();
    alias->strLabel = "test_alias";
    alias->strInternalName = "test_alias";
    alias->name = "n";
    alias->contents = "north";
    alias->strProcedure = "on_north";
    alias->bEnabled = true;
    alias->iSendTo = 0;
    alias->iSequence = 100;
    alias->bIgnoreCase = true;
    alias->bRegexp = false;
    alias->bKeepEvaluating = false;
    alias->bExpandVariables = true;
    alias->bEchoAlias = true;
    alias->strGroup = "Movement";
    alias->iUserOption = 456;

    // Save expected values before move
    QString expectedLabel = alias->strLabel;
    QString expectedName = alias->name;
    QString expectedContents = alias->contents;
    QString expectedProcedure = alias->strProcedure;
    bool expectedEnabled = alias->bEnabled;
    int expectedSendTo = alias->iSendTo;
    int expectedSequence = alias->iSequence;
    bool expectedIgnoreCase = alias->bIgnoreCase;
    bool expectedEchoAlias = alias->bEchoAlias;
    QString expectedGroup = alias->strGroup;
    int expectedUserOption = alias->iUserOption;

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

    EXPECT_EQ(loaded->strLabel, expectedLabel);
    EXPECT_EQ(loaded->name, expectedName);
    EXPECT_EQ(loaded->contents, expectedContents);
    EXPECT_EQ(loaded->strProcedure, expectedProcedure);
    EXPECT_EQ(loaded->bEnabled, expectedEnabled);
    EXPECT_EQ(loaded->iSendTo, expectedSendTo);
    EXPECT_EQ(loaded->iSequence, expectedSequence);
    EXPECT_EQ(loaded->bIgnoreCase, expectedIgnoreCase);
    EXPECT_EQ(loaded->bEchoAlias, expectedEchoAlias);
    EXPECT_EQ(loaded->strGroup, expectedGroup);
    EXPECT_EQ(loaded->iUserOption, expectedUserOption);
}

// Test 7: Timer round-trip with interval type
TEST_F(XmlRoundtripTest, TimerIntervalRoundtrip)
{
    doc1 = new WorldDocument();
    doc1->m_mush_name = "Timer Round-trip Test";

    Timer* timer = new Timer();
    timer->strLabel = "test_timer";
    timer->bEnabled = true;
    timer->iType = Timer::eInterval;
    timer->iEveryHour = 0;
    timer->iEveryMinute = 5;
    timer->fEverySecond = 30.5;
    timer->iOffsetHour = 0;
    timer->iOffsetMinute = 2;
    timer->fOffsetSecond = 15.25;
    timer->strContents = "say Timer fired!";
    timer->iSendTo = 0;
    timer->strProcedure = "onTimerFire";
    timer->strVariable = "";
    timer->bOneShot = false;
    timer->bActiveWhenClosed = true;
    timer->bOmitFromOutput = false;
    timer->bOmitFromLog = false;
    timer->strGroup = "Maintenance";
    timer->iUserOption = 789;

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

    EXPECT_EQ(loaded->strLabel, timer->strLabel);
    EXPECT_EQ(loaded->bEnabled, timer->bEnabled);
    EXPECT_EQ(loaded->iType, timer->iType);
    EXPECT_EQ(loaded->iEveryMinute, timer->iEveryMinute);
    EXPECT_DOUBLE_EQ(loaded->fEverySecond, timer->fEverySecond);
    EXPECT_EQ(loaded->iOffsetMinute, timer->iOffsetMinute);
    EXPECT_DOUBLE_EQ(loaded->fOffsetSecond, timer->fOffsetSecond);
    EXPECT_EQ(loaded->strContents, timer->strContents);
    EXPECT_EQ(loaded->iSendTo, timer->iSendTo);
    EXPECT_EQ(loaded->strProcedure, timer->strProcedure);
    EXPECT_EQ(loaded->bOneShot, timer->bOneShot);
    EXPECT_EQ(loaded->bActiveWhenClosed, timer->bActiveWhenClosed);
    EXPECT_EQ(loaded->bOmitFromOutput, timer->bOmitFromOutput);
    EXPECT_EQ(loaded->bOmitFromLog, timer->bOmitFromLog);
    EXPECT_EQ(loaded->strGroup, timer->strGroup);
    EXPECT_EQ(loaded->iUserOption, timer->iUserOption);
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
