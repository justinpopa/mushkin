/**
 * test_variable_expansion_gtest.cpp - GoogleTest version of Variable Expansion Test
 *
 * Verifies that the expandVariables() function works correctly:
 * - @variablename → variable contents
 * - @@ → literal @
 * - @!variablename → variable without regex escaping
 * - Missing variables → left as-is
 * - Case-insensitive variable names
 *
 * Migrated from standalone test to GoogleTest for better test organization,
 * fixtures, and assertion messages.
 */

#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"

// Test fixture for variable expansion tests
// Provides a fresh WorldDocument for each test
class VariableExpansionTest : public WorldDocumentTest {};

// Test 1: Basic Variable Expansion
TEST_F(VariableExpansionTest, BasicVariableExpansion)
{
    doc->setVariable("target", "goblin");
    doc->setVariable("spell", "fireball");

    QString result = doc->expandVariables("cast @spell at @target");

    EXPECT_EQ(result, "cast fireball at goblin") << "Basic variable expansion should work";
}

// Test 2: Double @@ Escape Sequence
TEST_F(VariableExpansionTest, DoubleAtEscapeSequence)
{
    doc->setVariable("price", "100");

    QString result = doc->expandVariables("Cost: @@price is @price gold");

    EXPECT_EQ(result, "Cost: @price is 100 gold") << "@@ should become literal @";
}

// Test 3: Regex Escaping ON (@var)
TEST_F(VariableExpansionTest, RegexEscapingEnabled)
{
    doc->setVariable("pattern", ".*test.*");

    QString result = doc->expandVariables("match @pattern", true); // escapeRegex=true

    // . * should become \. \*
    EXPECT_EQ(result, "match \\.\\*test\\.\\*")
        << "Regex metacharacters should be escaped when escapeRegex=true";
}

// Test 4: Regex Escaping OFF (@!var)
TEST_F(VariableExpansionTest, RegexEscapingDisabledWithExclamation)
{
    doc->setVariable("pattern", ".*test.*");

    QString result =
        doc->expandVariables("match @!pattern", true); // escapeRegex=true, but @! disables it

    // . * should remain as-is because of @!
    EXPECT_EQ(result, "match .*test.*") << "@! prefix should disable regex escaping";
}

// Test 5: escapeRegex Parameter = false
TEST_F(VariableExpansionTest, RegexEscapingParameterFalse)
{
    doc->setVariable("pattern", ".*test.*");

    QString result = doc->expandVariables("match @pattern", false); // escapeRegex=false

    // . * should remain as-is because escapeRegex=false
    EXPECT_EQ(result, "match .*test.*") << "escapeRegex=false should prevent escaping";
}

// Test 6: All Regex Metacharacters
TEST_F(VariableExpansionTest, AllRegexMetacharacters)
{
    doc->setVariable("meta", "\\^$.|?*+()[]{}");

    QString result = doc->expandVariables("chars: @meta", true);

    // All metacharacters should be escaped
    EXPECT_EQ(result, "chars: \\\\\\^\\$\\.\\|\\?\\*\\+\\(\\)\\[\\]\\{\\}")
        << "All regex metacharacters should be escaped";
}

// Test 7: Case-Insensitive Variable Names
TEST_F(VariableExpansionTest, CaseInsensitiveVariableNames)
{
    doc->setVariable("MyVar", "value123");

    QString result = doc->expandVariables("lowercase: @myvar, uppercase: @MYVAR, mixed: @MyVaR");

    EXPECT_EQ(result, "lowercase: value123, uppercase: value123, mixed: value123")
        << "Variable names should be case-insensitive";
}

// Test 8: Missing Variables (silently dropped — original behaviour)
TEST_F(VariableExpansionTest, MissingVariablesSilentlyDropped)
{
    // Don't set any variables

    QString result = doc->expandVariables("Value: @missing_var end");

    EXPECT_EQ(result, "Value:  end")
        << "Missing variables should be silently dropped (original MUSHclient behaviour)";
}

// Test 8b: Empty variable name (@ alone or @! alone) silently dropped
TEST_F(VariableExpansionTest, EmptyVariableNameSilentlyDropped)
{
    // @ followed by non-alnum/non-underscore: empty name — silently drop
    QString result = doc->expandVariables("before @ after");
    EXPECT_EQ(result, "before  after") << "@ with no variable name should be silently dropped";

    // @! followed by non-alnum: also silently drop
    QString result2 = doc->expandVariables("before @! after");
    EXPECT_EQ(result2, "before  after") << "@! with no variable name should be silently dropped";
}

// Test 9: Multiple Variables in One String
TEST_F(VariableExpansionTest, MultipleVariablesInOneString)
{
    doc->setVariable("a", "first");
    doc->setVariable("b", "second");
    doc->setVariable("c", "third");

    QString result = doc->expandVariables("@a and @b and @c");

    EXPECT_EQ(result, "first and second and third") << "Multiple variables should all be expanded";
}

// Test 10: Empty Variable Value
TEST_F(VariableExpansionTest, EmptyVariableValue)
{
    doc->setVariable("empty", "");

    QString result = doc->expandVariables("before @empty after");

    EXPECT_EQ(result, "before  after") << "Empty variables should expand to empty string";
}

// Test 11: Missing variable in email-like text — silently dropped
// 'example' is a valid identifier but undefined, so it is dropped per original behaviour.
TEST_F(VariableExpansionTest, AtSignNotFollowedByIdentifierSilentlyDropped)
{
    // user@example.com: 'example' is looked up, not found, silently dropped.
    QString result = doc->expandVariables("Email: user@example.com");
    EXPECT_EQ(result, "Email: user.com") << "Missing variable 'example' should be silently dropped";

    // @ followed immediately by '.': no identifier at all — silently drop
    QString result2 = doc->expandVariables("foo@.bar");
    EXPECT_EQ(result2, "foo.bar") << "@ with no identifier should be silently dropped";
}

// Test 12: Adjacent Variables
TEST_F(VariableExpansionTest, AdjacentVariables)
{
    doc->setVariable("x", "foo");
    doc->setVariable("y", "bar");

    QString result = doc->expandVariables("@x@y");

    EXPECT_EQ(result, "foobar") << "Adjacent variables should be expanded correctly";
}

// Test 13: Variable Name with Underscore
TEST_F(VariableExpansionTest, VariableNamesWithUnderscore)
{
    doc->setVariable("my_var", "works");
    doc->setVariable("_private", "also_works");

    QString result = doc->expandVariables("@my_var and @_private");

    EXPECT_EQ(result, "works and also_works") << "Variable names with underscores should work";
}

// Test 14: Non-regexp mode drops asterisks instead of escaping them
TEST_F(VariableExpansionTest, NonRegexpModeDropsAsterisks)
{
    doc->setVariable("pat", "a*b");

    // isRegexp=true (default): asterisk is escaped to \*
    QString resultRegexp = doc->expandVariables("match @pat", true, true);
    EXPECT_EQ(resultRegexp, "match a\\*b") << "Regexp mode should escape asterisks as \\*";

    // isRegexp=false: asterisk is dropped, not escaped
    QString resultNonRegexp = doc->expandVariables("match @pat", true, false);
    EXPECT_EQ(resultNonRegexp, "match ab")
        << "Non-regexp mode should drop asterisks from variable values";
}

// Test 15: SetVariable preserves original label case
TEST_F(VariableExpansionTest, SetVariablePreservesLabelCase)
{
    doc->setVariable("MyVariable", "value");

    // Lookup is case-insensitive
    EXPECT_EQ(doc->getVariable("myvariable"), "value");
    EXPECT_EQ(doc->getVariable("MYVARIABLE"), "value");
    EXPECT_EQ(doc->getVariable("MyVariable"), "value");

    // But the label stored internally should preserve original case
    const auto& varMap = doc->getVariableMap();
    auto it = varMap.find("myvariable");
    ASSERT_NE(it, varMap.end());
    EXPECT_EQ(it->second->label, "MyVariable")
        << "Variable label should preserve original case supplied to setVariable";
}
