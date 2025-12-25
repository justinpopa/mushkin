/**
 * alias.cpp - Alias Data Structure
 *
 * Based on CAlias from OtherTypes.h
 */

#include "alias.h"
#include "sendto.h"
#include <QDebug>

// Default alias sequence (OtherTypes.h)
#define DEFAULT_ALIAS_SEQUENCE 100

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

/**
 * Constructor - Initialize all fields to defaults
 *
 * Based on CAlias::CAlias() from OtherTypes.h
 */
Alias::Alias(QObject* parent) : QObject(parent), regexp(nullptr)
{
    initializeDefaults();
}

/**
 * Destructor - Clean up regexp
 */
Alias::~Alias()
{
    delete regexp;
}

/**
 * Initialize all fields to default values
 *
 * Based on CAlias constructor from OtherTypes.h
 */
void Alias::initializeDefaults()
{
    // Pattern matching
    bIgnoreCase = false;
    bRegexp = false;

    // Actions
    bExpandVariables = false;
    iSendTo = eSendToWorld;

    // Behavior
    bEnabled = true;
    bKeepEvaluating = true; // MUSHclient default: continue evaluating other aliases after match

    // Display
    bOmitFromLog = false;
    bOmitFromOutput = false;
    bEchoAlias = false;
    bOmitFromCommandHistory = false;

    // Metadata
    iSequence = DEFAULT_ALIAS_SEQUENCE;
    bMenu = false;
    iUserOption = 0;
    bOneShot = false;

    // Runtime state
    dispid = DISPID_UNKNOWN;
    nUpdateNumber = 0;
    nInvocationCount = 0;
    nMatched = 0;
    tWhenMatched = QDateTime();
    bTemporary = false;
    bIncluded = false;
    bSelected = false;
    bExecutingScript = false;

    // Resize wildcards vector
    wildcards.resize(MAX_WILDCARDS);
}

/**
 * Equality operator
 *
 * Compares all fields except runtime state for equality
 */
bool Alias::operator==(const Alias& rhs) const
{
    return name == rhs.name && contents == rhs.contents && bIgnoreCase == rhs.bIgnoreCase &&
           strLabel == rhs.strLabel && strProcedure == rhs.strProcedure &&
           bEnabled == rhs.bEnabled && bExpandVariables == rhs.bExpandVariables &&
           bOmitFromLog == rhs.bOmitFromLog && bRegexp == rhs.bRegexp &&
           bOmitFromOutput == rhs.bOmitFromOutput && iSequence == rhs.iSequence &&
           bMenu == rhs.bMenu && strGroup == rhs.strGroup && strVariable == rhs.strVariable &&
           iSendTo == rhs.iSendTo && bKeepEvaluating == rhs.bKeepEvaluating &&
           bEchoAlias == rhs.bEchoAlias && iUserOption == rhs.iUserOption &&
           bOmitFromCommandHistory == rhs.bOmitFromCommandHistory && bOneShot == rhs.bOneShot;
}

/**
 * Compile regular expression
 *
 * Compiles the alias pattern into a QRegularExpression.
 * Handles case-sensitivity based on bIgnoreCase flag.
 *
 * @return true on success, false on error
 */
bool Alias::compileRegexp()
{
    if (!bRegexp) {
        // Not a regexp alias, nothing to compile
        return true;
    }

    // Clean up old regexp if exists
    delete regexp;
    regexp = nullptr;

    // Compile new regexp
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (bIgnoreCase) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    regexp = new QRegularExpression(name, options);

    if (!regexp->isValid()) {
        qWarning() << "Failed to compile alias regexp:" << name;
        qWarning() << "Error:" << regexp->errorString();
        delete regexp;
        regexp = nullptr;
        return false;
    }

    return true;
}

/**
 * wildcardToRegex - Convert wildcard pattern to regex
 *
 * Converts simple * wildcard patterns to QRegularExpression.
 * Example: "n*" → "^n(.*?)$" or "n(.*?)" depending on match mode
 *
 * This is the same pattern used for triggers in world_trigger_matching.cpp.
 *
 * @param pattern The wildcard pattern (e.g., "n*" or "l*")
 * @param ignoreCase If true, create case-insensitive regex
 * @param exact If true, anchor to start and end (exact match mode)
 * @return QRegularExpression ready for matching
 */
static QRegularExpression wildcardToRegex(const QString& pattern, bool ignoreCase,
                                          bool exact = true)
{
    // Escape all regex special characters EXCEPT *
    QString escaped = QRegularExpression::escape(pattern);

    // Replace escaped asterisks with non-greedy capture group
    // QRegularExpression::escape() turns * into \\*
    escaped.replace("\\*", "(.*?)");

    // Anchor to start (and optionally end) of line
    QString fullPattern = exact ? ("^" + escaped + "$") : ("^" + escaped);

    // Set case sensitivity
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignoreCase) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    return QRegularExpression(fullPattern, options);
}

/**
 * Match alias against user input
 *
 * Performs pattern matching based on alias settings:
 * - Regular expression matching (if bRegexp is true)
 * - Wildcard pattern matching (* wildcards converted to regex)
 * - Case sensitivity (if bIgnoreCase is true)
 *
 * Populates wildcards vector on successful match.
 *
 * @param text User input to match against
 * @return true if matched, false otherwise
 */
bool Alias::match(const QString& text)
{
    // Check if alias is enabled
    if (!bEnabled) {
        return false;
    }

    bool matched = false;

    if (bRegexp) {
        // Regular expression matching
        if (!regexp) {
            // Regexp not compiled yet
            if (!compileRegexp()) {
                return false;
            }
        }

        QRegularExpressionMatch match = regexp->match(text);
        matched = match.hasMatch();

        if (matched) {
            // Extract wildcards (captured groups)
            wildcards.clear();
            wildcards.resize(match.lastCapturedIndex() + 1);
            for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
                wildcards[i] = match.captured(i);
            }

            // Extract named capture groups
            namedWildcards.clear();
            const QStringList namedGroups = regexp->namedCaptureGroups();
            for (const QString& groupName : namedGroups) {
                if (!groupName.isEmpty()) {
                    namedWildcards[groupName] = match.captured(groupName);
                }
            }
        }
    } else {
        // Wildcard pattern matching
        // Convert wildcard pattern (e.g., "n*" or "l*") to regex
        // This matches the pattern used for triggers
        QRegularExpression re = wildcardToRegex(name, bIgnoreCase, true);
        QRegularExpressionMatch match = re.match(text);
        matched = match.hasMatch();

        if (matched) {
            // Extract wildcards from pattern match
            wildcards.clear();
            wildcards.resize(match.lastCapturedIndex() + 1);
            for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
                wildcards[i] = match.captured(i);
            }
        }
    }

    if (matched) {
        // Update statistics
        nMatched++;
        tWhenMatched = QDateTime::currentDateTime();
    }

    return matched;
}
