/**
 * test_config_introspection_gtest.cpp - GoogleTest version
 * Test config introspection Lua API: GetDefaultValue, GetCurrentValue, GetLoadedValue
 *
 * Verifies:
 * 1. GetDefaultValue returns the static default from OptionsTable / AlphaOptionsTable
 * 2. GetCurrentValue mirrors world.GetOption for numeric options
 * 3. GetCurrentValue returns the live string field for alpha options
 * 4. GetCurrentValue reflects direct C++ mutations
 * 5. GetLoadedValue returns nil when no XML snapshot exists
 * 6. GetLoadedValue returns the snapshot value after manual population
 * 7. GetLoadedValue and GetCurrentValue can differ when the field is mutated post-load
 * 8. Unknown option names return nil
 * 9. Option name matching is case-insensitive
 */

#include "../src/world/config_options.h"
#include "fixtures/world_fixtures.h"

class ConfigIntrospectionTest : public ConnectedWorldTest {};

// Test 1: GetDefaultValue returns the static default for a numeric option
TEST_F(ConfigIntrospectionTest, GetDefaultValueNumeric)
{
    executeLua("default_port = world.GetDefaultValue('port')");
    double defaultPort = getGlobalNumber("default_port");
    EXPECT_EQ(defaultPort, 4000.0) << "GetDefaultValue('port') should return 4000";
}

// Test 2: GetDefaultValue returns the static default for an alpha option
TEST_F(ConfigIntrospectionTest, GetDefaultValueAlpha)
{
    executeLua("default_name = world.GetDefaultValue('name')");
    QString defaultName = getGlobalString("default_name");
    EXPECT_EQ(defaultName, "") << "GetDefaultValue('name') should return empty string";
}

// Test 3: GetCurrentValue matches world.GetOption for numeric options
TEST_F(ConfigIntrospectionTest, GetCurrentValueMatchesGetOption)
{
    executeLua("opt_port = world.GetOption('port')");
    executeLua("cur_port = world.GetCurrentValue('port')");
    double optPort = getGlobalNumber("opt_port");
    double curPort = getGlobalNumber("cur_port");
    EXPECT_EQ(curPort, optPort) << "GetCurrentValue('port') should match GetOption('port')";
}

// Test 4: GetCurrentValue returns the live string field for alpha options
TEST_F(ConfigIntrospectionTest, GetCurrentValueAlpha)
{
    executeLua("cur_name = world.GetCurrentValue('name')");
    QString curName = getGlobalString("cur_name");
    EXPECT_EQ(curName, doc->m_mush_name)
        << "GetCurrentValue('name') should return the world name set in SetUp";
}

// Test 5: GetCurrentValue reflects direct C++ mutations to the field
TEST_F(ConfigIntrospectionTest, GetCurrentValueReflectsSetOption)
{
    doc->m_port = 9999;
    executeLua("cur_port = world.GetCurrentValue('port')");
    double curPort = getGlobalNumber("cur_port");
    EXPECT_EQ(curPort, 9999.0)
        << "GetCurrentValue('port') should return 9999 after directly setting m_port";
}

// Test 6: GetLoadedValue returns nil when no XML snapshot has been taken
TEST_F(ConfigIntrospectionTest, GetLoadedValueNilWithoutLoad)
{
    executeLua("loaded_port = world.GetLoadedValue('port')");
    EXPECT_TRUE(isGlobalNil("loaded_port"))
        << "GetLoadedValue('port') should be nil when no snapshot exists";
}

// Test 7: GetLoadedValue returns the snapshot value after manual population
TEST_F(ConfigIntrospectionTest, GetLoadedValueAfterSnapshot)
{
    doc->m_loadedNumericOptions["port"] = 4000.0;
    executeLua("loaded_port = world.GetLoadedValue('port')");
    double loadedPort = getGlobalNumber("loaded_port");
    EXPECT_EQ(loadedPort, 4000.0) << "GetLoadedValue('port') should return 4000 from the snapshot";
}

// Test 8: GetLoadedValue and GetCurrentValue can diverge after mutation
TEST_F(ConfigIntrospectionTest, GetLoadedValueDiffersFromCurrent)
{
    doc->m_loadedNumericOptions["port"] = 4000.0;
    doc->m_port = 9999;

    executeLua("cur_port = world.GetCurrentValue('port')");
    executeLua("loaded_port = world.GetLoadedValue('port')");

    double curPort = getGlobalNumber("cur_port");
    double loadedPort = getGlobalNumber("loaded_port");

    EXPECT_EQ(curPort, 9999.0) << "GetCurrentValue('port') should reflect the mutated value 9999";
    EXPECT_EQ(loadedPort, 4000.0)
        << "GetLoadedValue('port') should still return the snapshot value 4000";
}

// Test 9: Unknown option names return nil
TEST_F(ConfigIntrospectionTest, UnknownOptionReturnsNil)
{
    executeLua("unknown_val = world.GetCurrentValue('nonexistent_option_xyz')");
    EXPECT_TRUE(isGlobalNil("unknown_val"))
        << "GetCurrentValue with an unknown option name should return nil";
}

// Test 10: Option name matching is case-insensitive
TEST_F(ConfigIntrospectionTest, CaseInsensitiveMatching)
{
    executeLua("lower_port = world.GetCurrentValue('port')");
    executeLua("upper_port = world.GetCurrentValue('Port')");
    double lowerPort = getGlobalNumber("lower_port");
    double upperPort = getGlobalNumber("upper_port");
    EXPECT_EQ(upperPort, lowerPort)
        << "GetCurrentValue should match option names case-insensitively";
}

// Test 11: GetInfo(212) respects line_spacing override when non-zero
// Original doc.cpp:3253-3256: m_FontHeight = iLineSpacing when non-zero.
// Only tests the override path here — the font-metrics fallback (line_spacing == 0)
// requires a QApplication and belongs in a GUI test suite.
TEST_F(ConfigIntrospectionTest, GetInfo212RespectsLineSpacing)
{
    // Set a line_spacing override and verify GetInfo(212) returns it exactly.
    doc->m_display.line_spacing = 20;
    executeLua("h_override = world.GetInfo(212)");
    double hOverride = getGlobalNumber("h_override");
    EXPECT_EQ(hOverride, 20.0)
        << "GetInfo(212) must return line_spacing when non-zero (original: doc.cpp:3253-3254)";
}

// Test 12: GetInfo(241) respects line_spacing override when non-zero
// GetInfo(241) is the same metric as GetInfo(212) and must follow the same override.
// Only tests the override path — font-metrics fallback needs a QApplication.
TEST_F(ConfigIntrospectionTest, GetInfo241RespectsLineSpacing)
{
    doc->m_display.line_spacing = 25;
    executeLua("h241 = world.GetInfo(241)");
    double h241 = getGlobalNumber("h241");
    EXPECT_EQ(h241, 25.0)
        << "GetInfo(241) must return line_spacing when non-zero (mirrors GetInfo(212))";
}

// Test 13: SetOption("max_output_lines") trims buffer AND refreshes view
// Original doc.cpp:4161-4171: FixUpOutputBuffer() calls addedstuff() on each view.
// In Mushkin, trimLineBuffer() + linesAdded() signal must both fire.
// max_output_lines minimum is 200 (config_options.cpp), so use 300 → 200.
TEST_F(ConfigIntrospectionTest, SetOptionMaxOutputLinesTrimsThenSignals)
{
    // Start with max_output_lines at 300 so we can legally lower it.
    doc->m_display.max_lines = 300;

    // Populate 250 lines (exceeds the 200 we'll set below).
    for (int i = 0; i < 250; ++i) {
        auto line = std::make_unique<Line>(i, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        doc->m_lineList.push_back(std::move(line));
    }
    ASSERT_EQ(static_cast<int>(doc->m_lineList.size()), 250);

    // Track linesAdded signal emissions via a local counter.
    int signalCount = 0;
    QObject::connect(doc.get(), &WorldDocument::linesAdded, [&signalCount]() { ++signalCount; });

    // Reduce max_output_lines to 200 (valid minimum) via SetOption — should trim and signal.
    executeLua("world.SetOption('max_output_lines', 200)");

    EXPECT_LE(static_cast<int>(doc->m_lineList.size()), 200)
        << "Buffer should be trimmed to at most max_output_lines lines";
    EXPECT_GT(signalCount, 0)
        << "linesAdded must be emitted after buffer trim so view refreshes (H96)";
}
