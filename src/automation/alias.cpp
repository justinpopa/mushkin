/**
 * alias.cpp - Alias Data Structure
 *
 * Based on CAlias from OtherTypes.h
 */

#include "alias.h"
#include "sendto.h"
#include <QDebug>
#include <expected>

// Default alias sequence (OtherTypes.h)
#define DEFAULT_ALIAS_SEQUENCE 100

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

/**
 * Constructor - Initialize all fields to defaults
 *
 * Based on CAlias::CAlias() from OtherTypes.h
 */
Alias::Alias(QObject* parent) : QObject(parent)
{
    initializeDefaults();
}

/**
 * Destructor - unique_ptr handles regexp cleanup automatically
 */
Alias::~Alias() = default;

/**
 * Initialize all fields to default values
 *
 * Based on CAlias constructor from OtherTypes.h
 */
void Alias::initializeDefaults()
{
    // Pattern matching
    ignore_case = false;
    use_regexp = false;

    // Actions
    expand_variables = false;
    send_to = eSendToWorld;
    scriptLanguage = ScriptLanguage::Lua;

    // Behavior
    enabled = true;
    keep_evaluating = true; // MUSHclient default: continue evaluating other aliases after match

    // Display
    omit_from_log = false;
    omit_from_output = false;
    echo_alias = false;
    omit_from_command_history = false;

    // Metadata
    sequence = DEFAULT_ALIAS_SEQUENCE;
    menu = false;
    user_option = 0;
    one_shot = false;

    // Runtime state
    dispid = DISPID_UNKNOWN;
    update_number = 0;
    invocation_count = 0;
    matched = 0;
    when_matched = QDateTime();
    temporary = false;
    included = false;
    selected = false;
    executing_script = false;

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
    return name == rhs.name && contents == rhs.contents && ignore_case == rhs.ignore_case &&
           label == rhs.label && procedure == rhs.procedure &&
           scriptLanguage == rhs.scriptLanguage && enabled == rhs.enabled &&
           expand_variables == rhs.expand_variables && omit_from_log == rhs.omit_from_log &&
           use_regexp == rhs.use_regexp && omit_from_output == rhs.omit_from_output &&
           sequence == rhs.sequence && menu == rhs.menu && group == rhs.group &&
           variable == rhs.variable && send_to == rhs.send_to &&
           keep_evaluating == rhs.keep_evaluating && echo_alias == rhs.echo_alias &&
           user_option == rhs.user_option &&
           omit_from_command_history == rhs.omit_from_command_history && one_shot == rhs.one_shot;
}

/**
 * Compile regular expression
 *
 * Compiles the alias pattern into a QRegularExpression.
 * Handles case-sensitivity based on ignore_case flag.
 *
 * @return empty expected on success, error string on failure
 */
std::expected<void, QString> Alias::compileRegexp()
{
    if (!use_regexp) {
        // Not a regexp alias, nothing to compile
        return {};
    }

    // Compile new regexp (reset releases old one)
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignore_case) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    auto compiled = std::make_unique<QRegularExpression>(name, options);

    if (!compiled->isValid()) {
        QString error = compiled->errorString();
        qWarning() << "Failed to compile alias regexp:" << name;
        qWarning() << "Error:" << error;
        regexp.reset();
        return std::unexpected(error);
    }

    regexp = std::move(compiled);
    return {};
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
 * - Regular expression matching (if use_regexp is true)
 * - Wildcard pattern matching (* wildcards converted to regex)
 * - Case sensitivity (if ignore_case is true)
 *
 * Populates wildcards vector on successful match.
 *
 * @param text User input to match against
 * @return true if matched, false otherwise
 */
bool Alias::match(const QString& text)
{
    // Check if alias is enabled
    if (!enabled) {
        return false;
    }

    bool did_match = false;

    if (use_regexp) {
        // Regular expression matching
        if (!regexp) {
            // Regexp not compiled yet
            if (!compileRegexp().has_value()) {
                return false;
            }
        }

        QRegularExpressionMatch match = regexp->match(text);
        did_match = match.hasMatch();

        if (did_match) {
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
        QRegularExpression re = wildcardToRegex(name, ignore_case, true);
        QRegularExpressionMatch match = re.match(text);
        did_match = match.hasMatch();

        if (did_match) {
            // Extract wildcards from pattern match
            wildcards.clear();
            wildcards.resize(match.lastCapturedIndex() + 1);
            for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
                wildcards[i] = match.captured(i);
            }
        }
    }

    if (did_match) {
        // Update statistics
        matched++;
        when_matched = QDateTime::currentDateTime();
    }

    return did_match;
}
