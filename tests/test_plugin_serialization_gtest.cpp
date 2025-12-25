/**
 * test_plugin_serialization_gtest.cpp - GoogleTest version
 * Plugin Save/Load to World Files Test
 *
 * Tests that plugins are properly saved to and loaded from world XML files,
 * including verification of <include> elements and plugin ordering by sequence.
 */

#include "../src/automation/plugin.h"
#include "../src/world/world_document.h"
#include "../src/world/xml_serialization.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>

// Test fixture for plugin serialization tests
class PluginSerializationTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create temporary directory for test files
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid()) << "Cannot create temporary directory";

        pluginPath = tempDir->path() + "/test_plugin.xml";
        worldPath = tempDir->path() + "/test_world.mcl";
    }

    void TearDown() override
    {
        delete tempDir;
    }

    // Helper to create a test plugin file
    void createPluginFile(const QString& path, const QString& name, const QString& id, int sequence)
    {
        QFile pluginFile(path);
        ASSERT_TRUE(pluginFile.open(QIODevice::WriteOnly | QIODevice::Text))
            << "Cannot create plugin file: " << path.toStdString();

        QString pluginXml = QString(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE muclient>
<muclient>
<plugin
   name="%1"
   author="Test Author"
   id="%2"
   language="Lua"
   purpose="Test plugin for serialization"
   save_state="y"
   date_written="2025-01-01"
   requires="5.00"
   version="1.0"
   sequence="%3"
>
<description trim="y">
Test plugin description
</description>

</plugin>
</muclient>
)")
                                .arg(name, id, QString::number(sequence));

        pluginFile.write(pluginXml.toUtf8());
        pluginFile.close();
    }

    QTemporaryDir* tempDir = nullptr;
    QString pluginPath;
    QString worldPath;
};

// Test 1: Create test plugin file
TEST_F(PluginSerializationTest, CreateTestPluginFile)
{
    createPluginFile(pluginPath, "TestPlugin", "12345678901234567890123456789012", 100);
    EXPECT_TRUE(QFile::exists(pluginPath)) << "Test plugin file should be created";
}

// Test 2: Load plugin into world document
TEST_F(PluginSerializationTest, LoadPluginIntoWorld)
{
    createPluginFile(pluginPath, "TestPlugin", "12345678901234567890123456789012", 100);

    WorldDocument doc;
    QString errorMsg;
    Plugin* plugin = doc.LoadPlugin(pluginPath, errorMsg);

    ASSERT_NE(plugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();
    EXPECT_EQ(doc.m_PluginList.size(), 1) << "Expected 1 plugin in document";
    EXPECT_EQ(plugin->m_strName, "TestPlugin") << "Plugin name should be 'TestPlugin'";
    EXPECT_EQ(plugin->m_strID, "12345678901234567890123456789012") << "Plugin ID should match";
}

// Test 3: Save world with plugin
TEST_F(PluginSerializationTest, SaveWorldWithPlugin)
{
    createPluginFile(pluginPath, "TestPlugin", "12345678901234567890123456789012", 100);

    WorldDocument doc;
    QString errorMsg;
    Plugin* plugin = doc.LoadPlugin(pluginPath, errorMsg);
    ASSERT_NE(plugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();

    bool saveResult = XmlSerialization::SaveWorldXML(&doc, worldPath);
    EXPECT_TRUE(saveResult) << "SaveWorldXML should succeed";
    EXPECT_TRUE(QFile::exists(worldPath)) << "World file should be created";
}

// Test 4: Verify <include> element in saved world file
TEST_F(PluginSerializationTest, VerifyIncludeElementInWorldFile)
{
    createPluginFile(pluginPath, "TestPlugin", "12345678901234567890123456789012", 100);

    WorldDocument doc;
    QString errorMsg;
    Plugin* plugin = doc.LoadPlugin(pluginPath, errorMsg);
    ASSERT_NE(plugin, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();

    ASSERT_TRUE(XmlSerialization::SaveWorldXML(&doc, worldPath)) << "SaveWorldXML should succeed";

    // Read world file content
    QFile worldFile(worldPath);
    ASSERT_TRUE(worldFile.open(QIODevice::ReadOnly | QIODevice::Text))
        << "Cannot open world file for reading";

    QString worldContent = worldFile.readAll();
    worldFile.close();

    // Verify <include> element exists
    EXPECT_TRUE(worldContent.contains("<include")) << "World file should contain <include> element";

    // Verify plugin="y" attribute
    EXPECT_TRUE(worldContent.contains("plugin=\"y\""))
        << "World file should contain plugin=\"y\" attribute";

    // Verify plugin path is included
    EXPECT_TRUE(worldContent.contains(pluginPath)) << "World file should contain plugin path";
}

// Test 5: Load world file and verify plugin is reloaded
TEST_F(PluginSerializationTest, LoadWorldAndVerifyPlugin)
{
    createPluginFile(pluginPath, "TestPlugin", "12345678901234567890123456789012", 100);

    // Create and save world with plugin
    WorldDocument doc1;
    QString errorMsg;
    Plugin* plugin1 = doc1.LoadPlugin(pluginPath, errorMsg);
    ASSERT_NE(plugin1, nullptr) << "LoadPlugin failed: " << errorMsg.toStdString();
    ASSERT_TRUE(XmlSerialization::SaveWorldXML(&doc1, worldPath)) << "SaveWorldXML should succeed";

    // Load world from file
    WorldDocument doc2;
    bool loadResult = XmlSerialization::LoadWorldXML(&doc2, worldPath);
    ASSERT_TRUE(loadResult) << "LoadWorldXML should succeed";

    ASSERT_EQ(doc2.m_PluginList.size(), 1) << "Expected 1 plugin after loading world";

    Plugin* loadedPlugin = doc2.m_PluginList.front().get();
    ASSERT_NE(loadedPlugin, nullptr) << "Loaded plugin should not be null";
    EXPECT_EQ(loadedPlugin->m_strName, "TestPlugin") << "Loaded plugin name should be 'TestPlugin'";
    EXPECT_EQ(loadedPlugin->m_strID, "12345678901234567890123456789012")
        << "Loaded plugin ID should match";
}

// Test 6: Verify plugin sorting by sequence
TEST_F(PluginSerializationTest, VerifyPluginSortingBySequence)
{
    // Create two plugins with different sequences
    QString plugin1Path = tempDir->path() + "/test_plugin1.xml";
    QString plugin2Path = tempDir->path() + "/test_plugin2.xml";

    createPluginFile(plugin1Path, "TestPlugin1", "11111111111111111111111111111111", 100);
    createPluginFile(plugin2Path, "TestPlugin2", "22222222222222222222222222222222", -100);

    // Load both plugins
    WorldDocument doc1;
    QString errorMsg;
    Plugin* plugin1 = doc1.LoadPlugin(plugin1Path, errorMsg);
    ASSERT_NE(plugin1, nullptr) << "LoadPlugin failed for plugin1: " << errorMsg.toStdString();

    Plugin* plugin2 = doc1.LoadPlugin(plugin2Path, errorMsg);
    ASSERT_NE(plugin2, nullptr) << "LoadPlugin failed for plugin2: " << errorMsg.toStdString();

    ASSERT_EQ(doc1.m_PluginList.size(), 2) << "Expected 2 plugins in document";

    // Save world with both plugins
    ASSERT_TRUE(XmlSerialization::SaveWorldXML(&doc1, worldPath))
        << "SaveWorldXML should succeed with 2 plugins";

    // Load world and verify plugin order
    WorldDocument doc2;
    bool loadResult = XmlSerialization::LoadWorldXML(&doc2, worldPath);
    ASSERT_TRUE(loadResult) << "LoadWorldXML should succeed for 2 plugins";

    ASSERT_EQ(doc2.m_PluginList.size(), 2) << "Expected 2 plugins after loading world";

    // Verify plugins are sorted by sequence (negative first)
    EXPECT_LT(doc2.m_PluginList[0]->m_iSequence, doc2.m_PluginList[1]->m_iSequence)
        << "Plugins should be sorted by sequence (lower first)";

    EXPECT_EQ(doc2.m_PluginList[0]->m_strName, "TestPlugin2")
        << "First plugin should be TestPlugin2 (sequence -100)";

    EXPECT_EQ(doc2.m_PluginList[1]->m_strName, "TestPlugin1")
        << "Second plugin should be TestPlugin1 (sequence 100)";

    EXPECT_EQ(doc2.m_PluginList[0]->m_iSequence, -100) << "First plugin sequence should be -100";

    EXPECT_EQ(doc2.m_PluginList[1]->m_iSequence, 100) << "Second plugin sequence should be 100";
}

// GoogleTest main function
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
