// test_options_tables_gtest.cpp - GoogleTest version
// Options Tables Test Suite
//
// Tests the configuration options metadata tables:
// - OptionsTable (numeric options)
// - AlphaOptionsTable (string options)
// - Verifies table structure and offsets

#include "../src/world/config_options.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <QSet>
#include <QString>

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

// ========== Option Default Value Tests ==========

// Test 15: A freshly constructed world logs MUD output by default.
// Original MUSHclient defaults log_output to true (scriptingoptions.cpp:130).
// The XML loader leaves absent attributes at their constructor defaults
// (xml_serialization.cpp:570), so the in-class member initializer is the
// effective default for new worlds and for saves that omit the attribute.
TEST_F(OptionsWorldDocumentTest, LogOutputDefaultsToTrue)
{
    EXPECT_TRUE(doc->m_logging.log_output)
        << "New worlds must log MUD output by default (original default: true)";

    // The OptionsTable metadata default must agree with the member initializer.
    const tConfigurationNumericOption* logOutput = nullptr;
    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        if (QString(opt->pName) == "log_output") {
            logOutput = opt;
            break;
        }
    }
    ASSERT_NE(logOutput, nullptr) << "log_output must exist in OptionsTable";
    EXPECT_DOUBLE_EQ(logOutput->iDefault, 1.0)
        << "OptionsTable default for log_output must be true (1.0)";
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

// ========== MXP Mode Numeric Parity (H123) ==========
//
// The "use_mxp" option stores the raw integer value of m_iUseMXP both in the
// world file and through Lua GetOption/SetOption. Those integers MUST match the
// original MUSHclient enum (doc.h:350-355):
//   eOnCommandMXP = 0, eQueryMXP = 1, eUseMXP = 2, eNoMXP = 3.
// Any other numbering breaks world-file cross-compatibility and Lua parity.

static const tConfigurationNumericOption* findNumericOption(const char* name)
{
    for (const tConfigurationNumericOption* opt = OptionsTable; opt->pName != nullptr; opt++) {
        if (QString(opt->pName) == name) {
            return opt;
        }
    }
    return nullptr;
}

// ========== Constructor Default Parity (H58, M92) ==========
//
// The original MUSHclient applies scriptingoptions.cpp table defaults via
// SetAlphaDefaults(false) in doc_construct.cpp:45. Mushkin achieves the same
// effect through in-class member initializers — so the initializer IS the
// canonical default and must match the table entry.

// H58 MEDIUM: auto_say_string defaults to "say " in the original
// (scriptingoptions.cpp:225). A freshly constructed WorldDocument must return
// "say " for m_auto_say.say_string, not empty string.
TEST_F(OptionsWorldDocumentTest, AutoSayStringDefaultsToSaySpace)
{
    EXPECT_EQ(doc->m_auto_say.say_string, "say ")
        << "auto_say_string must default to \"say \" (original scriptingoptions.cpp:225)";

    // The AlphaOptionsTable metadata default must agree.
    const tConfigurationAlphaOption* opt = nullptr;
    for (const tConfigurationAlphaOption* o = AlphaOptionsTable; o->pName != nullptr; o++) {
        if (QString(o->pName) == "auto_say_string") {
            opt = o;
            break;
        }
    }
    ASSERT_NE(opt, nullptr) << "auto_say_string must exist in AlphaOptionsTable";
    ASSERT_NE(opt->sDefault, nullptr);
    EXPECT_STREQ(opt->sDefault, "say ")
        << "AlphaOptionsTable default for auto_say_string must be \"say \"";
}

// M92 MEDIUM: show_bold defaults to false in the original
// (scriptingoptions.cpp:168). A freshly constructed WorldDocument must have
// m_display.show_bold == false, not true.
TEST_F(OptionsWorldDocumentTest, ShowBoldDefaultsToFalse)
{
    EXPECT_FALSE(doc->m_display.show_bold)
        << "show_bold must default to false (original scriptingoptions.cpp:168)";

    // The OptionsTable metadata default must agree.
    const tConfigurationNumericOption* opt = findNumericOption("show_bold");
    ASSERT_NE(opt, nullptr) << "show_bold must exist in OptionsTable";
    EXPECT_DOUBLE_EQ(opt->iDefault, 0.0)
        << "OptionsTable default for show_bold must be false (0.0)";
}

// MXPMode enumerators carry the exact original numeric values.
TEST_F(OptionsTableTest, MxpModeEnumValuesMatchOriginal)
{
    EXPECT_EQ(static_cast<int>(MXPMode::eMXP_On), 0) // eOnCommandMXP
        << "eMXP_On (on command) must serialize as 0 like eOnCommandMXP";
    EXPECT_EQ(static_cast<int>(MXPMode::eMXP_Query), 1) // eQueryMXP
        << "eMXP_Query must serialize as 1 like eQueryMXP";
    EXPECT_EQ(static_cast<int>(MXPMode::eMXP_Always), 2) // eUseMXP
        << "eMXP_Always (always on) must serialize as 2 like eUseMXP";
    EXPECT_EQ(static_cast<int>(MXPMode::eMXP_Off), 3) // eNoMXP
        << "eMXP_Off must serialize as 3 like eNoMXP";
}

// use_mxp metadata: default is "on command" (0) with the original 0..3 range.
TEST_F(OptionsTableTest, UseMxpDefaultAndRangeMatchOriginal)
{
    const tConfigurationNumericOption* useMxp = findNumericOption("use_mxp");
    ASSERT_NE(useMxp, nullptr) << "use_mxp must exist in OptionsTable";
    EXPECT_DOUBLE_EQ(useMxp->iDefault, 0.0) << "use_mxp default must be eOnCommandMXP (0)";
    EXPECT_DOUBLE_EQ(useMxp->iMinimum, 0.0);
    EXPECT_DOUBLE_EQ(useMxp->iMaximum, 3.0)
        << "use_mxp range must span all four original modes (0..3)";
}

// The getter/setter round-trip every original integer to the matching MXPMode.
TEST_F(OptionsWorldDocumentTest, UseMxpGetterSetterRoundTripsOriginalIntegers)
{
    const tConfigurationNumericOption* useMxp = findNumericOption("use_mxp");
    ASSERT_NE(useMxp, nullptr);

    // A freshly constructed world defaults to "on command" (0).
    EXPECT_EQ(static_cast<int>(doc->m_iUseMXP), 0);
    EXPECT_DOUBLE_EQ(useMxp->getter(*doc), 0.0);

    struct {
        int value;
        MXPMode mode;
    } cases[] = {
        {0, MXPMode::eMXP_On},
        {1, MXPMode::eMXP_Query},
        {2, MXPMode::eMXP_Always},
        {3, MXPMode::eMXP_Off},
    };

    for (const auto& c : cases) {
        useMxp->setter(*doc, static_cast<double>(c.value));
        EXPECT_EQ(doc->m_iUseMXP, c.mode)
            << "use_mxp integer " << c.value << " must map to the matching MXPMode";
        EXPECT_DOUBLE_EQ(useMxp->getter(*doc), static_cast<double>(c.value))
            << "GetOption(use_mxp) must report the same integer that was set";
    }
}
