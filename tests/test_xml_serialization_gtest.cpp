/**
 * test_xml_serialization_gtest.cpp - GoogleTest version
 * XML Serialization Tests
 *
 * Comprehensive tests for loading and saving MUSHclient world files (.mcl)
 */

#include "../src/automation/alias.h"
#include "../src/automation/trigger.h"
#include "../src/world/world_document.h"
#include "../src/world/xml_serialization.h"
#include <QCoreApplication>
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

// Test fixture for XML serialization tests
class XmlSerializationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Setup if needed
    }

    void TearDown() override
    {
        // Cleanup if needed
    }
};

// Tests for IsArchiveXML detection

TEST_F(XmlSerializationTest, IsArchiveXMLDetectsValidXMLWithDeclaration)
{
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open()) << "Failed to open temporary file";
    tmpFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<muclient><world/></muclient>");
    tmpFile.close();

    QFile file(tmpFile.fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    EXPECT_TRUE(XmlSerialization::IsArchiveXML(file)) << "Should detect valid XML with declaration";
    file.close();
}

TEST_F(XmlSerializationTest, IsArchiveXMLDetectsValidXMLWithMuclientTag)
{
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open()) << "Failed to open temporary file";
    tmpFile.write("<muclient><world name=\"test\"/></muclient>");
    tmpFile.close();

    QFile file(tmpFile.fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    EXPECT_TRUE(XmlSerialization::IsArchiveXML(file))
        << "Should detect valid XML with muclient tag";
    file.close();
}

TEST_F(XmlSerializationTest, IsArchiveXMLRejectsNonXMLContent)
{
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open()) << "Failed to open temporary file";
    tmpFile.write("This is not XML at all, just plain text");
    tmpFile.close();

    QFile file(tmpFile.fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    EXPECT_FALSE(XmlSerialization::IsArchiveXML(file)) << "Should reject non-XML content";
    file.close();
}

TEST_F(XmlSerializationTest, IsArchiveXMLDetectsUTF8BOM)
{
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open()) << "Failed to open temporary file";
    // Write UTF-8 BOM followed by XML
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    tmpFile.write(reinterpret_cast<const char*>(bom), 3);
    tmpFile.write("<muclient/>");
    tmpFile.close();

    QFile file(tmpFile.fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    EXPECT_TRUE(XmlSerialization::IsArchiveXML(file)) << "Should detect XML with UTF-8 BOM";
    file.close();
}

// Tests for SaveWorldXML and LoadWorldXML round-trip

TEST_F(XmlSerializationTest, BasicWorldPropertiesRoundTrip)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_mush_name = "Test World";
    doc1->m_server = "test.example.com";
    doc1->m_port = 4000;
    doc1->m_name = "TestPlayer";
    doc1->m_password = "SecretPassword";
    doc1->m_wrap = true;
    doc1->m_nWrapColumn = 80;

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("basic_roundtrip");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    // Load into new document
    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";

    // Verify fields match
    EXPECT_EQ(doc2->m_mush_name, doc1->m_mush_name) << "m_mush_name should match";
    EXPECT_EQ(doc2->m_server, doc1->m_server) << "m_server should match";
    EXPECT_EQ(doc2->m_port, doc1->m_port) << "m_port should match";
    EXPECT_EQ(doc2->m_name, doc1->m_name) << "m_name should match";
    EXPECT_EQ(doc2->m_password, doc1->m_password) << "m_password should match";
    EXPECT_EQ(doc2->m_wrap, doc1->m_wrap) << "m_wrap should match";
    EXPECT_EQ(doc2->m_nWrapColumn, doc1->m_nWrapColumn) << "m_nWrapColumn should match";

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

TEST_F(XmlSerializationTest, PasswordEncodingDecoding)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_mush_name = "Password Test";
    doc1->m_password = "Complex!P@ssw0rd#123";

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("password");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    // Verify password is base64 encoded in file
    QFile file(filename);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    QByteArray content = file.readAll();
    file.close();

    // Password should be base64 encoded (not plain text)
    EXPECT_FALSE(content.contains("Complex!P@ssw0rd#123"))
        << "Password should not appear in plain text";

    // Load and verify decoding works
    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";
    EXPECT_EQ(doc2->m_password, "Complex!P@ssw0rd#123") << "Password should be decoded correctly";

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

TEST_F(XmlSerializationTest, BooleanValuesSerializeCorrectly)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_wrap = true;
    doc1->m_enable_triggers = true;
    doc1->m_enable_aliases = false;

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("booleans");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";

    EXPECT_EQ(doc2->m_wrap, true) << "m_wrap should be true";
    EXPECT_EQ(doc2->m_enable_triggers, true) << "m_enable_triggers should be true";
    EXPECT_EQ(doc2->m_enable_aliases, false) << "m_enable_aliases should be false";

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

// Tests for loading real Aardwolf.mcl file

TEST_F(XmlSerializationTest, LoadRealAardwolfFile)
{
    WorldDocument* doc = new WorldDocument();

    // Try multiple possible paths relative to build directory
    QStringList possiblePaths = {"../../tests/fixtures/Aardwolf.mcl",
                                 "../tests/fixtures/Aardwolf.mcl",
                                 "tests/fixtures/Aardwolf.mcl",
                                 "./fixtures/Aardwolf.mcl"};

    QString filename;
    for (const auto& path : possiblePaths) {
        if (QFile::exists(path)) {
            filename = path;
            break;
        }
    }

    if (filename.isEmpty()) {
        delete doc;
        GTEST_SKIP() << "Aardwolf.mcl fixture not found (tried multiple paths)";
    }

    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc, filename))
        << "LoadWorldXML failed for Aardwolf.mcl";

    // Verify key properties from real file
    EXPECT_EQ(doc->m_mush_name, "Aardwolf") << "m_mush_name should be 'Aardwolf'";
    EXPECT_EQ(doc->m_server, "aardmud.org") << "m_server should be 'aardmud.org'";
    EXPECT_EQ(doc->m_port, 23) << "m_port should be 23";
    EXPECT_EQ(doc->m_name, "TestPlayer") << "m_name should be 'TestPlayer'";
    EXPECT_EQ(doc->m_password, "TestPassword123") << "m_password should be 'TestPassword123'";
    EXPECT_EQ(doc->m_strWorldID, "e0eb198d8d5698e3b2f61483") << "m_strWorldID should match";
    EXPECT_EQ(doc->m_strLanguage, "Lua") << "m_strLanguage should be 'Lua'";
    EXPECT_EQ(doc->m_bUTF_8, true) << "m_bUTF_8 should be true";
    EXPECT_EQ(doc->m_wrap, true) << "m_wrap should be true";
    EXPECT_EQ(doc->m_nWrapColumn, 124) << "m_nWrapColumn should be 124";
    EXPECT_EQ(doc->m_enable_triggers, true) << "m_enable_triggers should be true";
    EXPECT_EQ(doc->m_enable_aliases, true) << "m_enable_aliases should be true";
    EXPECT_EQ(doc->m_font_name, "Fira Code") << "m_font_name should be 'Fira Code'";
    EXPECT_EQ(doc->m_input_font_name, "Fira Code") << "m_input_font_name should be 'Fira Code'";
    EXPECT_EQ(doc->m_strTerminalIdentification, "MUSHclient-Aard")
        << "m_strTerminalIdentification should be 'MUSHclient-Aard'";

    delete doc;
}

// Tests for SaveWorldXML XML structure validation

TEST_F(XmlSerializationTest, SaveWorldXMLCreatesValidXMLStructure)
{
    WorldDocument* doc = new WorldDocument();
    doc->m_mush_name = "Structure Test";
    doc->m_server = "test.example.com";
    doc->m_port = 4001; // Use non-default port so it appears in XML (4000 is default, skipped)

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("structure");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc, filename)) << "SaveWorldXML failed";

    // Read file and verify XML structure
    QFile file(filename);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // Check for required XML elements
    EXPECT_TRUE(content.contains("<?xml version=\"1.0\"")) << "Should contain XML declaration";
    EXPECT_TRUE(content.contains("<!DOCTYPE muclient>")) << "Should contain DOCTYPE";
    EXPECT_TRUE(content.contains("<muclient>")) << "Should contain opening muclient tag";
    EXPECT_TRUE(content.contains("<world")) << "Should contain opening world tag";
    EXPECT_TRUE(content.contains("</world>")) << "Should contain closing world tag";
    EXPECT_TRUE(content.contains("</muclient>")) << "Should contain closing muclient tag";
    EXPECT_TRUE(content.contains("name=\"Structure Test\"")) << "Should contain world name";
    EXPECT_TRUE(content.contains("site=\"test.example.com\"")) << "Should contain server address";
    EXPECT_TRUE(content.contains("port=\"4001\"")) << "Should contain port number";

    delete doc;
    cleanupSaveFiles(filename);
}

// Tests for Trigger/Alias XML serialization round-trip

TEST_F(XmlSerializationTest, TriggersSaveAndLoadCorrectly)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_mush_name = "Trigger Test";

    // Create a trigger
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

    doc1->addTrigger("test_trigger", std::unique_ptr<Trigger>(trigger));

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("triggers");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    // Verify XML contains trigger
    QFile file(filename);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    EXPECT_TRUE(content.contains("<triggers>")) << "Should contain triggers section";
    EXPECT_TRUE(content.contains("<trigger")) << "Should contain trigger element";
    EXPECT_TRUE(content.contains("name=\"test_trigger\"")) << "Should contain trigger name";
    EXPECT_TRUE(content.contains("match=\"You have * gold\"")) << "Should contain trigger match";
    EXPECT_TRUE(content.contains("script=\"on_gold\"")) << "Should contain script name";
    EXPECT_TRUE(content.contains("<send><![CDATA[say I have %1 gold!]]></send>"))
        << "Should contain trigger send text";

    // Load into new document
    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";

    // Verify trigger was loaded
    Trigger* loaded = doc2->getTrigger("test_trigger");
    ASSERT_NE(loaded, nullptr) << "Trigger should be loaded";
    EXPECT_EQ(loaded->strLabel, "test_trigger") << "strLabel should match";
    EXPECT_EQ(loaded->trigger, "You have * gold") << "trigger should match";
    EXPECT_EQ(loaded->contents, "say I have %1 gold!") << "contents should match";
    EXPECT_EQ(loaded->strProcedure, "on_gold") << "strProcedure should match";
    EXPECT_EQ(loaded->bEnabled, true) << "bEnabled should match";
    EXPECT_EQ(loaded->iSendTo, 0) << "iSendTo should match";
    EXPECT_EQ(loaded->iSequence, 100) << "iSequence should match";
    EXPECT_EQ(loaded->ignore_case, false) << "ignore_case should match";
    EXPECT_EQ(loaded->bKeepEvaluating, true) << "bKeepEvaluating should match";
    EXPECT_EQ(loaded->strGroup, "Combat") << "strGroup should match";

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

TEST_F(XmlSerializationTest, AliasesSaveAndLoadCorrectly)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_mush_name = "Alias Test";

    // Create an alias
    auto alias = std::make_unique<Alias>();
    alias->strLabel = "test_alias";
    alias->strInternalName = "test_alias";
    alias->name = "n";
    alias->contents = "north";
    alias->strProcedure = "on_north";
    alias->bEnabled = true;
    alias->iSendTo = 0; // SendToWorld
    alias->iSequence = 100;
    alias->bIgnoreCase = true;
    alias->bRegexp = false;
    alias->bKeepEvaluating = false;
    alias->bExpandVariables = true;
    alias->bEchoAlias = true;
    alias->strGroup = "Movement";

    doc1->addAlias("test_alias", std::move(alias));

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("aliases");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    // Verify XML contains alias
    QFile file(filename);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open file for reading";
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    EXPECT_TRUE(content.contains("<aliases>")) << "Should contain aliases section";
    EXPECT_TRUE(content.contains("<alias")) << "Should contain alias element";
    EXPECT_TRUE(content.contains("name=\"test_alias\"")) << "Should contain alias name";
    EXPECT_TRUE(content.contains("match=\"n\"")) << "Should contain alias match";
    EXPECT_TRUE(content.contains("script=\"on_north\"")) << "Should contain script name";
    EXPECT_TRUE(content.contains("<send><![CDATA[north]]></send>"))
        << "Should contain alias send text";

    // Load into new document
    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";

    // Verify alias was loaded
    Alias* loaded = doc2->getAlias("test_alias");
    ASSERT_NE(loaded, nullptr) << "Alias should be loaded";
    EXPECT_EQ(loaded->strLabel, "test_alias") << "strLabel should match";
    EXPECT_EQ(loaded->name, "n") << "name should match";
    EXPECT_EQ(loaded->contents, "north") << "contents should match";
    EXPECT_EQ(loaded->strProcedure, "on_north") << "strProcedure should match";
    EXPECT_EQ(loaded->bEnabled, true) << "bEnabled should match";
    EXPECT_EQ(loaded->iSendTo, 0) << "iSendTo should match";
    EXPECT_EQ(loaded->iSequence, 100) << "iSequence should match";
    EXPECT_EQ(loaded->bIgnoreCase, true) << "bIgnoreCase should match";
    EXPECT_EQ(loaded->bEchoAlias, true) << "bEchoAlias should match";
    EXPECT_EQ(loaded->strGroup, "Movement") << "strGroup should match";

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

TEST_F(XmlSerializationTest, MultipleTriggersAndAliases)
{
    WorldDocument* doc1 = new WorldDocument();
    doc1->m_mush_name = "Multi Test";

    // Create multiple triggers
    for (int i = 0; i < 3; i++) {
        Trigger* trigger = new Trigger();
        trigger->strLabel = QString("trigger_%1").arg(i);
        trigger->strInternalName = trigger->strLabel;
        trigger->trigger = QString("Pattern %1").arg(i);
        trigger->contents = QString("Response %1").arg(i);
        trigger->bEnabled = true;
        trigger->iSequence = 100 + i;
        doc1->addTrigger(trigger->strLabel, std::unique_ptr<Trigger>(trigger));
    }

    // Create multiple aliases
    for (int i = 0; i < 3; i++) {
        auto alias = std::make_unique<Alias>();
        alias->strLabel = QString("alias_%1").arg(i);
        alias->strInternalName = alias->strLabel;
        alias->name = QString("cmd%1").arg(i);
        alias->contents = QString("command%1").arg(i);
        alias->bEnabled = true;
        alias->iSequence = 100 + i;
        QString label = alias->strLabel;
        doc1->addAlias(label, std::move(alias));
    }

    // Generate unique filename (avoids Windows file locking with QTemporaryFile)
    QString filename = generateTempFilename("multi");

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(doc1, filename)) << "SaveWorldXML failed";

    // Load into new document
    WorldDocument* doc2 = new WorldDocument();
    ASSERT_TRUE(XmlSerialization::LoadWorldXML(doc2, filename)) << "LoadWorldXML failed";

    // Verify all triggers loaded
    for (int i = 0; i < 3; i++) {
        QString name = QString("trigger_%1").arg(i);
        Trigger* trigger = doc2->getTrigger(name);
        ASSERT_NE(trigger, nullptr) << "Trigger " << name.toStdString() << " should be loaded";
        EXPECT_EQ(trigger->trigger, QString("Pattern %1").arg(i))
            << "Trigger pattern should match for " << name.toStdString();
        EXPECT_EQ(trigger->iSequence, 100 + i)
            << "Trigger sequence should match for " << name.toStdString();
    }

    // Verify all aliases loaded
    for (int i = 0; i < 3; i++) {
        QString name = QString("alias_%1").arg(i);
        Alias* alias = doc2->getAlias(name);
        ASSERT_NE(alias, nullptr) << "Alias " << name.toStdString() << " should be loaded";
        EXPECT_EQ(alias->name, QString("cmd%1").arg(i))
            << "Alias name should match for " << name.toStdString();
        EXPECT_EQ(alias->iSequence, 100 + i)
            << "Alias sequence should match for " << name.toStdString();
    }

    delete doc1;
    delete doc2;
    cleanupSaveFiles(filename);
}

// GoogleTest main function
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
