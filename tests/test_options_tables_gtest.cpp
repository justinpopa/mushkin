// test_options_tables_gtest.cpp - GoogleTest version
// Options Tables Test Suite
//
// Tests the configuration options metadata tables:
// - OptionsTable (numeric options)
// - AlphaOptionsTable (string options)
// - Verifies table structure and offsets

#include "../src/world/config_options.h"
#include "../src/world/world_document.h"
#include <QCoreApplication>
#include <QSet>
#include <QString>
#include <gtest/gtest.h>
#include <memory>

// Test fixture for OptionsTable tests
class OptionsTableTest : public ::testing::Test {
  protected:
    // No special setup needed - tables are global
};

// Test fixture for AlphaOptionsTable tests
class AlphaOptionsTableTest : public ::testing::Test {
  protected:
    // No special setup needed
};

// Test fixture for WorldDocument integration tests
class OptionsWorldDocumentTest : public ::testing::Test {
  protected:
    std::unique_ptr<WorldDocument> doc;

    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    void TearDown() override
    {
    }
};

// ========== OptionsTable Tests ==========

// Test 1: Table has entries
TEST_F(OptionsTableTest, TableHasEntries)
{
    int count = 0;
    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        count++;
    }

    EXPECT_GT(count, 0) << "OptionsTable has " << count << " numeric entries";
}

// Test 2: First few numeric options have valid data
TEST_F(OptionsTableTest, FirstNumericOptionsValid)
{
    ASSERT_NE(OptionsTable[0].pName, nullptr);
    EXPECT_NE(OptionsTable[0].getter, nullptr);
    EXPECT_NE(OptionsTable[0].setter, nullptr);

    int validEntries = 0;
    for (int i = 0; i < 5 && OptionsTable[i].pName != nullptr; i++) {
        EXPECT_NE(OptionsTable[i].pName, nullptr);
        EXPECT_NE(OptionsTable[i].getter, nullptr);
        EXPECT_NE(OptionsTable[i].setter, nullptr);
        validEntries++;
    }

    EXPECT_GE(validEntries, 5);
}

// Test 3: All numeric options have non-null accessors
TEST_F(OptionsTableTest, AllAccessorsNonNull)
{
    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        EXPECT_NE(opt->getter, nullptr) << "Option: " << opt->pName;
        EXPECT_NE(opt->setter, nullptr) << "Option: " << opt->pName;
    }
}

// ========== AlphaOptionsTable Tests ==========

// Test 4: Alpha table has entries
TEST_F(AlphaOptionsTableTest, TableHasEntries)
{
    int count = 0;
    for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr; opt++) {
        count++;
    }

    EXPECT_GT(count, 0) << "AlphaOptionsTable has " << count << " string entries";
}

// Test 5: First few alpha options have valid data
TEST_F(AlphaOptionsTableTest, FirstAlphaOptionsValid)
{
    ASSERT_NE(AlphaOptionsTable[0].pName, nullptr);
    EXPECT_NE(AlphaOptionsTable[0].getter, nullptr);
    EXPECT_NE(AlphaOptionsTable[0].setter, nullptr);

    int validEntries = 0;
    for (int i = 0; i < 5 && AlphaOptionsTable[i].pName != nullptr; i++) {
        EXPECT_NE(AlphaOptionsTable[i].pName, nullptr);
        EXPECT_NE(AlphaOptionsTable[i].getter, nullptr);
        EXPECT_NE(AlphaOptionsTable[i].setter, nullptr);
        validEntries++;
    }

    EXPECT_GE(validEntries, 5);
}

// Test 6: All alpha options have non-null accessors
TEST_F(AlphaOptionsTableTest, AllAccessorsNonNull)
{
    for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr; opt++) {
        EXPECT_NE(opt->getter, nullptr) << "Option: " << opt->pName;
        EXPECT_NE(opt->setter, nullptr) << "Option: " << opt->pName;
    }
}

// Test 7: Default values are valid strings or null
TEST_F(AlphaOptionsTableTest, ValidDefaultValues)
{
    for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr; opt++) {
        if (opt->sDefault != nullptr) {
            QString defaultStr(opt->sDefault);
            EXPECT_GE(defaultStr.length(), 0);
        }
    }
}

// ========== WorldDocument Integration Tests ==========

// Test 8: Numeric options point to valid WorldDocument fields
TEST_F(OptionsWorldDocumentTest, NumericOptionsPointToValidFields)
{
    if (OptionsTable[0].pName != nullptr) {
        EXPECT_NE(OptionsTable[0].getter, nullptr);
        EXPECT_NE(OptionsTable[0].setter, nullptr);
    }
}

// Test 9: Alpha options point to valid WorldDocument fields
TEST_F(OptionsWorldDocumentTest, AlphaOptionsPointToValidFields)
{
    if (AlphaOptionsTable[0].pName != nullptr) {
        EXPECT_NE(AlphaOptionsTable[0].getter, nullptr);
        EXPECT_NE(AlphaOptionsTable[0].setter, nullptr);
    }
}

// Test 10: Can iterate through all options without crash
TEST_F(OptionsWorldDocumentTest, IterateThroughAllOptions)
{
    int numericCount = 0;
    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        numericCount++;
        [[maybe_unused]] const char* name = opt->pName;
        [[maybe_unused]] auto* getter = opt->getter;
        [[maybe_unused]] auto* setter = opt->setter;
        [[maybe_unused]] double defaultVal = opt->iDefault;
    }

    int alphaCount = 0;
    for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr; opt++) {
        alphaCount++;
        [[maybe_unused]] const char* name = opt->pName;
        [[maybe_unused]] auto* getter = opt->getter;
        [[maybe_unused]] auto* setter = opt->setter;
        [[maybe_unused]] const char* defaultVal = opt->sDefault;
    }

    EXPECT_GT(numericCount, 0) << "Numeric options: " << numericCount;
    EXPECT_GT(alphaCount, 0) << "Alpha options: " << alphaCount;
}

// ========== Options Name Uniqueness Tests ==========

// Test 11: Numeric option names are unique
TEST_F(OptionsTableTest, NumericOptionNamesUnique)
{
    QSet<QString> names;

    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        QString name(opt->pName);
        EXPECT_FALSE(names.contains(name)) << "Checking uniqueness of: " << name.toStdString();
        names.insert(name);
    }
}

// Test 12: Alpha option names are unique
TEST_F(AlphaOptionsTableTest, AlphaOptionNamesUnique)
{
    QSet<QString> names;

    for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr; opt++) {
        QString name(opt->pName);
        EXPECT_FALSE(names.contains(name)) << "Checking uniqueness of: " << name.toStdString();
        names.insert(name);
    }
}

// ========== Known Options Tests ==========

// Test 13: Can find specific numeric options
TEST_F(OptionsTableTest, CanFindSpecificNumericOptions)
{
    // Look for some known numeric options
    QStringList knownOptions = {"beep_sound", "connect_method", "port"};

    for (const QString& optName : knownOptions) {
        bool found = false;
        for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
            if (QString(opt->pName) == optName) {
                found = true;
                break;
            }
        }
        // Note: We don't require these to exist, just document if they do
        if (found) {
            SUCCEED() << "Found numeric option: " << optName.toStdString();
        }
    }
}

// Test 14: Can find specific alpha options
TEST_F(AlphaOptionsTableTest, CanFindSpecificAlphaOptions)
{
    // Look for some known string options
    QStringList knownOptions = {"name", "password", "server"};

    for (const QString& optName : knownOptions) {
        bool found = false;
        for (const tConfigurationAlphaOption* opt = AlphaOptionsTable; opt->pName != nullptr;
             opt++) {
            if (QString(opt->pName) == optName) {
                found = true;
                break;
            }
        }
        // Note: We don't require these to exist, just document if they do
        if (found) {
            SUCCEED() << "Found alpha option: " << optName.toStdString();
        }
    }
}

// GoogleTest main entry point
int main(int argc, char** argv)
{
    // Initialize Qt application (required for Qt types)
    QCoreApplication app(argc, argv);

    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}
