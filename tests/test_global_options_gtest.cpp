// test_global_options_gtest.cpp - GoogleTest version
//
// Behavioral-parity tests for GlobalOptions (global_options.cpp), covering
// deviations fixed in task t002:
//
//  * H13 MEDIUM - save() must round-trip the RAW relative directory defaults
//    (e.g. "./logs/") rather than the machine-specific absolute path that the
//    getters resolve to. The original LoadGlobalsFromDatabase keeps the raw
//    string and SaveGlobalsToDatabase writes it back verbatim.
//  * H13 LOW    - resolveDirectory("") must resolve to the working directory,
//    matching the original Make_Absolute_Path (Utilities.cpp:2483-2508).
//  * M25 MEDIUM - DB key names must match the original GlobalOptionsTable
//    verbatim, including the spaces in "Icon Placement" / "Tray Icon" and the
//    trailing space on "DefaultInputFontItalic " (globalregistryoptions.cpp).

#include "../src/storage/database.h"
#include "../src/storage/global_options.h"
#include "fixtures/world_fixtures.h"
#include <QDir>

class GlobalOptionsTest : public ::testing::Test {
  protected:
    GlobalOptionsTest() : db(Database::instance()), opts(GlobalOptions::instance())
    {
    }

    void SetUp() override
    {
        ASSERT_TRUE(db.open().has_value()) << "preferences database must open";
    }

    Database& db;
    GlobalOptions& opts;
};

// H13 LOW: an empty directory string resolves to the working directory, not "".
TEST_F(GlobalOptionsTest, EmptyDirectoryResolvesToWorkingDirectory)
{
    opts.setDefaultLogFileDirectory("");
    const QString resolved = opts.defaultLogFileDirectory();

    EXPECT_FALSE(resolved.isEmpty())
        << "empty directory must resolve to the working directory, not empty";
    EXPECT_EQ(resolved, QDir::currentPath() + "/")
        << "empty directory must resolve to the captured working directory";
}

// H13 MEDIUM: the getter resolves a relative path to an absolute one, but the
// raw accessor (what save() persists) keeps the original relative string.
TEST_F(GlobalOptionsTest, RelativeDirectoryResolvesButRawStaysRelative)
{
    opts.setDefaultLogFileDirectory("./logs/");

    // Resolved form is absolute and ends in the logs directory.
    const QString resolved = opts.defaultLogFileDirectory();
    EXPECT_TRUE(QDir::isAbsolutePath(resolved))
        << "getter must resolve a relative directory to an absolute path";
    EXPECT_TRUE(resolved.endsWith("/logs/"))
        << "resolved log directory should end in /logs/, got: " << resolved.toStdString();

    // Raw form is untouched and still relative.
    EXPECT_EQ(opts.rawDefaultLogFileDirectory(), "./logs/")
        << "raw stored value must remain the relative default";
}

// H13 MEDIUM: save() persists the RAW relative path, not the resolved absolute
// path. Loading a fresh value back must still be the relative default.
TEST_F(GlobalOptionsTest, SavePersistsRawRelativePathNotResolved)
{
    opts.setDefaultLogFileDirectory("./logs/");
    opts.setDefaultWorldFileDirectory("./worlds/");
    opts.setPluginsDirectory("./worlds/plugins/");
    opts.setStateFilesDirectory("./worlds/plugins/state/");
    opts.save();

    EXPECT_EQ(db.getPreference("DefaultLogFileDirectory", QString()), "./logs/")
        << "save() must write the raw relative path, not a resolved absolute path";
    EXPECT_EQ(db.getPreference("DefaultWorldFileDirectory", QString()), "./worlds/");
    EXPECT_EQ(db.getPreference("PluginsDirectory", QString()), "./worlds/plugins/");
    EXPECT_EQ(db.getPreference("StateFilesDirectory", QString()), "./worlds/plugins/state/");

    // And the persisted value must NOT contain a machine-specific absolute prefix.
    const QString stored = db.getPreference("DefaultLogFileDirectory", QString());
    EXPECT_FALSE(QDir::isAbsolutePath(stored))
        << "persisted directory must stay relative: " << stored.toStdString();
}

// An absolute directory set by the user is preserved verbatim through save/load.
TEST_F(GlobalOptionsTest, AbsoluteDirectoryRoundTripsUnchanged)
{
    const QString abs = QDir::tempPath() + "/mushkin_logs/";
    opts.setDefaultLogFileDirectory(abs);

    EXPECT_EQ(opts.defaultLogFileDirectory(), abs)
        << "absolute directory must be returned unchanged by the getter";
    EXPECT_EQ(opts.rawDefaultLogFileDirectory(), abs);

    opts.save();
    EXPECT_EQ(db.getPreference("DefaultLogFileDirectory", QString()), abs);
}

// M25 MEDIUM: save() writes the original spaced / trailing-space key names so
// the database is drop-in compatible with original MUSHclient prefs.
TEST_F(GlobalOptionsTest, SaveUsesOriginalSpacedKeyNames)
{
    opts.setIconPlacement(2);
    opts.setTrayIcon(7);
    opts.setDefaultInputFontItalic(1);
    opts.save();

    EXPECT_EQ(db.getPreferenceInt("Icon Placement", -1), 2)
        << "Icon placement must persist under the original key \"Icon Placement\"";
    EXPECT_EQ(db.getPreferenceInt("Tray Icon", -1), 7)
        << "Tray icon must persist under the original key \"Tray Icon\"";
    EXPECT_EQ(db.getPreferenceInt("DefaultInputFontItalic ", -1), 1)
        << "Italic flag must persist under the original key with a trailing space";
}

// M25 MEDIUM: load() reads the original spaced key names. A value stored under
// the original key must be picked up.
TEST_F(GlobalOptionsTest, LoadReadsOriginalSpacedKeyNames)
{
    ASSERT_TRUE(db.setPreferenceInt("Icon Placement", 3).has_value());
    ASSERT_TRUE(db.setPreferenceInt("Tray Icon", 5).has_value());
    ASSERT_TRUE(db.setPreferenceInt("DefaultInputFontItalic ", 1).has_value());

    opts.load();

    EXPECT_EQ(opts.iconPlacement(), 3) << "load() must read icon placement from \"Icon Placement\"";
    EXPECT_EQ(opts.trayIcon(), 5) << "load() must read tray icon from \"Tray Icon\"";
    EXPECT_EQ(opts.defaultInputFontItalic(), 1)
        << "load() must read italic flag from \"DefaultInputFontItalic \"";
}
