/**
 * test_database_gtest.cpp - GoogleTest version of Database Test
 *
 * Test suite for Database (SQLite Preferences Database)
 *
 * Tests the global preferences database for:
 * - Database creation and opening
 * - Schema creation (control, prefs, worlds, recent_files tables)
 * - Control table operations (database version, metadata)
 * - Prefs table operations (global preferences)
 * - Recent files CRUD operations
 * - Error handling
 *
 * Migrated from Catch2 to GoogleTest for consistency with other tests.
 */

#include "test_qt_static.h"
#include "../src/storage/database.h"
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <gtest/gtest.h>

// Test fixture for database tests
// Provides common setup/teardown and helper methods
class DatabaseTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        db = Database::instance();
    }

    void TearDown() override
    {
        // Clean up any test data
        if (db && db->isOpen()) {
            db->clearRecentFiles();
        }
    }

    Database* db = nullptr;
};

// Test 1: Database singleton works
TEST_F(DatabaseTest, SingletonWorks)
{
    Database* db1 = Database::instance();
    Database* db2 = Database::instance();

    ASSERT_NE(db1, nullptr) << "Database instance should not be null";
    EXPECT_EQ(db1, db2) << "Both instances should be the same (singleton)";
}

// Test 2: Database can be opened and creates schema
TEST_F(DatabaseTest, CanBeOpenedAndCreatesSchema)
{
    bool opened = db->open();
    ASSERT_TRUE(opened) << "Database should open successfully";
    EXPECT_TRUE(db->isOpen()) << "Database should report as open";

    // Verify database path is set
    QString path = db->databasePath();
    EXPECT_FALSE(path.isEmpty()) << "Database path should not be empty";

    qDebug() << "Database path:" << path;
}

// Test 3: Database file exists after open
TEST_F(DatabaseTest, FileExistsAfterOpen)
{
    db->open();
    QString path = db->databasePath();
    EXPECT_TRUE(QFile::exists(path)) << "Database file should exist at: " << path.toStdString();
}

// Test 4: Add recent file
TEST_F(DatabaseTest, AddRecentFile)
{
    ASSERT_TRUE(db->open());

    // Clear any existing recent files
    db->clearRecentFiles();

    // Create a temporary test file
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    // Add to recent files
    bool added = db->addRecentFile(testFile);
    EXPECT_TRUE(added) << "Should successfully add recent file";

    // Verify it's in the list
    QStringList recent = db->getRecentFiles();
    EXPECT_EQ(recent.size(), 1) << "Should have 1 recent file";
    EXPECT_EQ(recent[0], testFile) << "Recent file should match added file";
}

// Test 5: Add multiple recent files
TEST_F(DatabaseTest, AddMultipleRecentFiles)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Create multiple test files
    QStringList testFiles;
    for (int i = 0; i < 5; ++i) {
        QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        testFiles.append(testFile);
        db->addRecentFile(testFile);
    }

    // Verify all are in the list
    QStringList recent = db->getRecentFiles();
    EXPECT_EQ(recent.size(), 5) << "Should have 5 recent files";

    // Verify all test files are in the recent list
    // Note: We don't check specific order because files added in the same second
    // may have identical timestamps, making order undefined
    for (const QString& testFile : testFiles) {
        EXPECT_TRUE(recent.contains(testFile))
            << "Recent files should contain: " << testFile.toStdString();
    }
}

// Test 6: Recent files limit works
TEST_F(DatabaseTest, RecentFilesLimitWorks)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Create 15 test files
    for (int i = 0; i < 15; ++i) {
        QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        db->addRecentFile(testFile);
    }

    // Get only 10 most recent
    QStringList recent = db->getRecentFiles(10);
    EXPECT_EQ(recent.size(), 10) << "Should return only 10 most recent files";
}

// Test 7: Duplicate file updates timestamp
TEST_F(DatabaseTest, DuplicateFileUpdatesTimestamp)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    // Add file twice
    db->addRecentFile(testFile);
    db->addRecentFile(testFile);

    // Should only have one entry
    QStringList recent = db->getRecentFiles();
    EXPECT_EQ(recent.size(), 1) << "Duplicate file should not create multiple entries";
}

// Test 8: Remove recent file
TEST_F(DatabaseTest, RemoveRecentFile)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    // Add and then remove
    db->addRecentFile(testFile);
    EXPECT_EQ(db->getRecentFiles().size(), 1) << "Should have 1 file after adding";

    bool removed = db->removeRecentFile(testFile);
    EXPECT_TRUE(removed) << "Should successfully remove recent file";
    EXPECT_EQ(db->getRecentFiles().size(), 0) << "Should have 0 files after removal";
}

// Test 9: Clear all recent files
TEST_F(DatabaseTest, ClearAllRecentFiles)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Add multiple files
    for (int i = 0; i < 5; ++i) {
        QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        db->addRecentFile(testFile);
    }

    EXPECT_EQ(db->getRecentFiles().size(), 5) << "Should have 5 files before clearing";

    // Clear all
    bool cleared = db->clearRecentFiles();
    EXPECT_TRUE(cleared) << "Should successfully clear recent files";
    EXPECT_EQ(db->getRecentFiles().size(), 0) << "Should have 0 files after clearing";
}

// Test 10: Non-existent files are filtered out
TEST_F(DatabaseTest, NonExistentFilesFilteredOut)
{
    ASSERT_TRUE(db->open());
    db->clearRecentFiles();

    // Add a file that doesn't exist
    QString fakeFile = "/nonexistent/path/to/world.mcl";
    db->addRecentFile(fakeFile);

    // It shouldn't appear in results (because getRecentFiles filters non-existent files)
    QStringList recent = db->getRecentFiles();

    // Should be empty or not contain the fake file
    bool containsFake = recent.contains(fakeFile);
    EXPECT_FALSE(containsFake) << "Non-existent files should be filtered out";
}

// Test 11: Database version is set on creation
TEST_F(DatabaseTest, DatabaseVersionSetOnCreation)
{
    ASSERT_TRUE(db->open());

    int version = db->getDatabaseVersion();
    EXPECT_EQ(version, CURRENT_DB_VERSION)
        << "Database version should be set to CURRENT_DB_VERSION";
}

// Test 12: Can set and get database version
TEST_F(DatabaseTest, CanSetAndGetDatabaseVersion)
{
    ASSERT_TRUE(db->open());

    bool set = db->setDatabaseVersion(99);
    EXPECT_TRUE(set) << "Should successfully set database version";

    int version = db->getDatabaseVersion();
    EXPECT_EQ(version, 99) << "Database version should be 99";

    // Restore to current version
    db->setDatabaseVersion(CURRENT_DB_VERSION);
}

// Test 13: Can set and get control int values
TEST_F(DatabaseTest, CanSetAndGetControlInt)
{
    ASSERT_TRUE(db->open());

    bool set = db->setControlInt("test_setting", 42);
    EXPECT_TRUE(set) << "Should successfully set control int";

    int value = db->getControlInt("test_setting", 0);
    EXPECT_EQ(value, 42) << "Control int value should be 42";
}

// Test 14: Control int returns default for non-existent key
TEST_F(DatabaseTest, ControlIntReturnsDefaultForNonExistentKey)
{
    ASSERT_TRUE(db->open());

    int value = db->getControlInt("nonexistent_key", 999);
    EXPECT_EQ(value, 999) << "Should return default value for non-existent key";
}

// Test 15: Can update existing control int value
TEST_F(DatabaseTest, CanUpdateExistingControlInt)
{
    ASSERT_TRUE(db->open());

    db->setControlInt("update_test", 10);
    EXPECT_EQ(db->getControlInt("update_test"), 10) << "Initial value should be 10";

    db->setControlInt("update_test", 20);
    EXPECT_EQ(db->getControlInt("update_test"), 20) << "Updated value should be 20";
}

// Test 16: Can set and get string preference
TEST_F(DatabaseTest, CanSetAndGetStringPreference)
{
    ASSERT_TRUE(db->open());

    bool set = db->setPreference("test_pref", "test_value");
    EXPECT_TRUE(set) << "Should successfully set preference";

    QString value = db->getPreference("test_pref", "");
    EXPECT_EQ(value, "test_value") << "Preference value should be 'test_value'";
}

// Test 17: Preference returns default for non-existent key
TEST_F(DatabaseTest, PreferenceReturnsDefaultForNonExistentKey)
{
    ASSERT_TRUE(db->open());

    QString value = db->getPreference("nonexistent_pref", "default");
    EXPECT_EQ(value, "default") << "Should return default value for non-existent preference";
}

// Test 18: Can update existing preference
TEST_F(DatabaseTest, CanUpdateExistingPreference)
{
    ASSERT_TRUE(db->open());

    db->setPreference("update_pref", "first_value");
    EXPECT_EQ(db->getPreference("update_pref"), "first_value")
        << "Initial value should be 'first_value'";

    db->setPreference("update_pref", "second_value");
    EXPECT_EQ(db->getPreference("update_pref"), "second_value")
        << "Updated value should be 'second_value'";
}

// Test 19: Can set and get integer preference
TEST_F(DatabaseTest, CanSetAndGetIntegerPreference)
{
    ASSERT_TRUE(db->open());

    bool set = db->setPreferenceInt("int_pref", 12345);
    EXPECT_TRUE(set) << "Should successfully set integer preference";

    int value = db->getPreferenceInt("int_pref", 0);
    EXPECT_EQ(value, 12345) << "Integer preference value should be 12345";
}

// Test 20: Integer preference returns default for non-existent key
TEST_F(DatabaseTest, IntPreferenceReturnsDefaultForNonExistentKey)
{
    ASSERT_TRUE(db->open());

    int value = db->getPreferenceInt("nonexistent_int", 42);
    EXPECT_EQ(value, 42) << "Should return default value for non-existent integer preference";
}

// Test 21: Can store various string values
TEST_F(DatabaseTest, CanStoreVariousStringValues)
{
    ASSERT_TRUE(db->open());

    db->setPreference("locale", "EN");
    db->setPreference("theme", "dark");
    db->setPreference("font_name", "Courier New");

    EXPECT_EQ(db->getPreference("locale"), "EN") << "Locale should be 'EN'";
    EXPECT_EQ(db->getPreference("theme"), "dark") << "Theme should be 'dark'";
    EXPECT_EQ(db->getPreference("font_name"), "Courier New") << "Font name should be 'Courier New'";
}

// Test 22: Cannot add empty path
TEST_F(DatabaseTest, CannotAddEmptyPath)
{
    ASSERT_TRUE(db->open());

    bool added = db->addRecentFile("");
    EXPECT_FALSE(added) << "Should not add empty path to recent files";
}

// Test 23: Operations succeed when database is open
TEST_F(DatabaseTest, OperationsSucceedWhenDatabaseOpen)
{
    // Note: In practice, the singleton should always be opened early in main()
    // This test documents the expected behavior

    bool opened = db->open();
    EXPECT_TRUE(opened) << "Database should open successfully";
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
