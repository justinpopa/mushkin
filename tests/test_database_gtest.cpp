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
 */

#include "../src/storage/database.h"
#include "fixtures/world_fixtures.h"
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

class DatabaseTest : public ::testing::Test {
  protected:
    DatabaseTest() : db(Database::instance())
    {
    }

    void TearDown() override
    {
        if (db.isOpen()) {
            (void)db.clearRecentFiles();
        }
    }

    Database& db;
};

TEST_F(DatabaseTest, SingletonWorks)
{
    Database& db1 = Database::instance();
    Database& db2 = Database::instance();

    EXPECT_EQ(&db1, &db2) << "Both instances should be the same (singleton)";
}

TEST_F(DatabaseTest, CanBeOpenedAndCreatesSchema)
{
    EXPECT_TRUE(db.open().has_value()) << "Database should open successfully";
    EXPECT_TRUE(db.isOpen()) << "Database should report as open";

    const QString path = db.databasePath();
    EXPECT_FALSE(path.isEmpty()) << "Database path should not be empty";

    qDebug() << "Database path:" << path;
}

TEST_F(DatabaseTest, FileExistsAfterOpen)
{
    EXPECT_TRUE(db.open().has_value());
    const QString path = db.databasePath();
    EXPECT_TRUE(QFile::exists(path)) << "Database file should exist at: " << path.toStdString();
}

TEST_F(DatabaseTest, AddRecentFile)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    const QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    const auto added = db.addRecentFile(testFile);
    EXPECT_TRUE(added.has_value()) << "Should successfully add recent file";

    const QStringList recent = db.getRecentFiles();
    EXPECT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0], testFile);
}

TEST_F(DatabaseTest, AddMultipleRecentFiles)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    QStringList testFiles;
    for (int i = 0; i < 5; ++i) {
        const QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        testFiles.append(testFile);
        EXPECT_TRUE(db.addRecentFile(testFile).has_value());
    }

    const QStringList recent = db.getRecentFiles();
    EXPECT_EQ(recent.size(), 5);

    for (const QString& testFile : testFiles) {
        EXPECT_TRUE(recent.contains(testFile))
            << "Recent files should contain: " << testFile.toStdString();
    }
}

TEST_F(DatabaseTest, RecentFilesLimitWorks)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    for (int i = 0; i < 15; ++i) {
        const QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        EXPECT_TRUE(db.addRecentFile(testFile).has_value());
    }

    const QStringList recent = db.getRecentFiles(10);
    EXPECT_EQ(recent.size(), 10);
}

TEST_F(DatabaseTest, DuplicateFileUpdatesTimestamp)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    const QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    EXPECT_TRUE(db.addRecentFile(testFile).has_value());
    EXPECT_TRUE(db.addRecentFile(testFile).has_value());

    const QStringList recent = db.getRecentFiles();
    EXPECT_EQ(recent.size(), 1);
}

TEST_F(DatabaseTest, RemoveRecentFile)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    const QString testFile = tempDir.path() + "/test_world.mcl";
    QFile file(testFile);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();

    EXPECT_TRUE(db.addRecentFile(testFile).has_value());
    EXPECT_EQ(db.getRecentFiles().size(), 1);

    const auto removed = db.removeRecentFile(testFile);
    EXPECT_TRUE(removed.has_value());
    EXPECT_EQ(db.getRecentFiles().size(), 0);
}

TEST_F(DatabaseTest, ClearAllRecentFiles)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    QTemporaryDir tempDir;
    EXPECT_TRUE(tempDir.isValid());

    for (int i = 0; i < 5; ++i) {
        const QString testFile = tempDir.path() + QString("/test_world_%1.mcl").arg(i);
        QFile file(testFile);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(QString("test content %1").arg(i).toUtf8());
        file.close();

        EXPECT_TRUE(db.addRecentFile(testFile).has_value());
    }

    EXPECT_EQ(db.getRecentFiles().size(), 5);

    const auto cleared = db.clearRecentFiles();
    EXPECT_TRUE(cleared.has_value());
    EXPECT_EQ(db.getRecentFiles().size(), 0);
}

TEST_F(DatabaseTest, NonExistentFilesFilteredOut)
{
    EXPECT_TRUE(db.open().has_value());
    EXPECT_TRUE(db.clearRecentFiles().has_value());

    const QString fakeFile = "/nonexistent/path/to/world.mcl";
    EXPECT_TRUE(db.addRecentFile(fakeFile).has_value());

    const QStringList recent = db.getRecentFiles();
    EXPECT_FALSE(recent.contains(fakeFile));
}

TEST_F(DatabaseTest, DatabaseVersionSetOnCreation)
{
    EXPECT_TRUE(db.open().has_value());

    const int version = db.getDatabaseVersion();
    EXPECT_EQ(version, CURRENT_DB_VERSION);
}

TEST_F(DatabaseTest, CanSetAndGetDatabaseVersion)
{
    EXPECT_TRUE(db.open().has_value());

    const auto set = db.setDatabaseVersion(99);
    EXPECT_TRUE(set.has_value());

    const int version = db.getDatabaseVersion();
    EXPECT_EQ(version, 99);

    EXPECT_TRUE(db.setDatabaseVersion(CURRENT_DB_VERSION).has_value());
}

TEST_F(DatabaseTest, CanSetAndGetControlInt)
{
    EXPECT_TRUE(db.open().has_value());

    const auto set = db.setControlInt("test_setting", 42);
    EXPECT_TRUE(set.has_value());

    const int value = db.getControlInt("test_setting", 0);
    EXPECT_EQ(value, 42);
}

TEST_F(DatabaseTest, ControlIntReturnsDefaultForNonExistentKey)
{
    EXPECT_TRUE(db.open().has_value());

    const int value = db.getControlInt("nonexistent_key", 999);
    EXPECT_EQ(value, 999);
}

TEST_F(DatabaseTest, CanUpdateExistingControlInt)
{
    EXPECT_TRUE(db.open().has_value());

    EXPECT_TRUE(db.setControlInt("update_test", 10).has_value());
    EXPECT_EQ(db.getControlInt("update_test"), 10);

    EXPECT_TRUE(db.setControlInt("update_test", 20).has_value());
    EXPECT_EQ(db.getControlInt("update_test"), 20);
}

TEST_F(DatabaseTest, CanSetAndGetStringPreference)
{
    EXPECT_TRUE(db.open().has_value());

    const auto set = db.setPreference("test_pref", "test_value");
    EXPECT_TRUE(set.has_value());

    const QString value = db.getPreference("test_pref", "");
    EXPECT_EQ(value, "test_value");
}

TEST_F(DatabaseTest, PreferenceReturnsDefaultForNonExistentKey)
{
    EXPECT_TRUE(db.open().has_value());

    const QString value = db.getPreference("nonexistent_pref", "default");
    EXPECT_EQ(value, "default");
}

TEST_F(DatabaseTest, CanUpdateExistingPreference)
{
    EXPECT_TRUE(db.open().has_value());

    EXPECT_TRUE(db.setPreference("update_pref", "first_value").has_value());
    EXPECT_EQ(db.getPreference("update_pref"), "first_value");

    EXPECT_TRUE(db.setPreference("update_pref", "second_value").has_value());
    EXPECT_EQ(db.getPreference("update_pref"), "second_value");
}

TEST_F(DatabaseTest, CanSetAndGetIntegerPreference)
{
    EXPECT_TRUE(db.open().has_value());

    const auto set = db.setPreferenceInt("int_pref", 12345);
    EXPECT_TRUE(set.has_value());

    const int value = db.getPreferenceInt("int_pref", 0);
    EXPECT_EQ(value, 12345);
}

TEST_F(DatabaseTest, IntPreferenceReturnsDefaultForNonExistentKey)
{
    EXPECT_TRUE(db.open().has_value());

    const int value = db.getPreferenceInt("nonexistent_int", 42);
    EXPECT_EQ(value, 42);
}

TEST_F(DatabaseTest, CanStoreVariousStringValues)
{
    EXPECT_TRUE(db.open().has_value());

    EXPECT_TRUE(db.setPreference("locale", "EN").has_value());
    EXPECT_TRUE(db.setPreference("theme", "dark").has_value());
    EXPECT_TRUE(db.setPreference("font_name", "Courier New").has_value());

    EXPECT_EQ(db.getPreference("locale"), "EN");
    EXPECT_EQ(db.getPreference("theme"), "dark");
    EXPECT_EQ(db.getPreference("font_name"), "Courier New");
}

TEST_F(DatabaseTest, CannotAddEmptyPath)
{
    EXPECT_TRUE(db.open().has_value());

    const auto added = db.addRecentFile("");
    EXPECT_FALSE(added.has_value());
}

TEST_F(DatabaseTest, OperationsSucceedWhenDatabaseOpen)
{
    EXPECT_TRUE(db.open().has_value());
}
