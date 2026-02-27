/**
 * trigger.cpp - Trigger Data Structure
 *
 * Based on CTrigger from OtherTypes.h
 */

#include "trigger.h"
#include "sendto.h"
#include <QDebug>
#include <expected>

// Default trigger sequence (OtherTypes.h)
#define DEFAULT_TRIGGER_SEQUENCE 100

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

/**
 * Constructor - Initialize all fields to defaults
 *
 * Based on CTrigger::CTrigger() from OtherTypes.h
 */
Trigger::Trigger(QObject* parent) : QObject(parent)
{
    initializeDefaults();
}

/**
 * Destructor - unique_ptr handles regexp cleanup automatically
 */
Trigger::~Trigger() = default;

/**
 * Initialize all fields to default values
 *
 * Based on CTrigger constructor from OtherTypes.h
 */
void Trigger::initializeDefaults()
{
    // Pattern matching
    ignore_case = false;
    omit_from_log = false;
    omit_from_output = false;
    keep_evaluating = true; // MUSHclient default: continue evaluating other triggers after match
    expand_variables = false;
    enabled = true;
    use_regexp = false;
    repeat = false;
    sequence = DEFAULT_TRIGGER_SEQUENCE;
    match_type = 0;
    style = 0;
    sound_if_inactive = false;
    lowercase_wildcard = false;
    multi_line = false;
    lines_to_match = 0;

    // Actions
    send_to = eSendToWorld;
    clipboard_arg = 0;
    scriptLanguage = ScriptLanguage::Lua;

    // Display
    colour = 0; // Custom color 1
    other_foreground = 0;
    other_background = 0;
    colour_change_type = TRIGGER_COLOUR_CHANGE_BOTH;

    // Metadata
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
    owningPlugin = nullptr;

    // Resize wildcards vector
    wildcards.resize(MAX_WILDCARDS);
}

/**
 * Equality operator
 *
 * Compares all fields except runtime state for equality
 */
bool Trigger::operator==(const Trigger& rhs) const
{
    return trigger == rhs.trigger && contents == rhs.contents &&
           sound_to_play == rhs.sound_to_play && ignore_case == rhs.ignore_case &&
           colour == rhs.colour && omit_from_log == rhs.omit_from_log &&
           omit_from_output == rhs.omit_from_output && keep_evaluating == rhs.keep_evaluating &&
           enabled == rhs.enabled && label == rhs.label && procedure == rhs.procedure &&
           scriptLanguage == rhs.scriptLanguage && clipboard_arg == rhs.clipboard_arg &&
           send_to == rhs.send_to && use_regexp == rhs.use_regexp && repeat == rhs.repeat &&
           sequence == rhs.sequence && match_type == rhs.match_type && style == rhs.style &&
           sound_if_inactive == rhs.sound_if_inactive && expand_variables == rhs.expand_variables &&
           lowercase_wildcard == rhs.lowercase_wildcard && group == rhs.group &&
           variable == rhs.variable && user_option == rhs.user_option &&
           other_foreground == rhs.other_foreground && other_background == rhs.other_background &&
           multi_line == rhs.multi_line && lines_to_match == rhs.lines_to_match &&
           colour_change_type == rhs.colour_change_type && one_shot == rhs.one_shot;
}

/**
 * Compile regular expression
 *
 * Compiles the trigger pattern into a QRegularExpression.
 * Handles case-sensitivity based on ignore_case flag.
 * For multi-line triggers, enables MultiLineOption so ^ and $ match line boundaries.
 *
 * @return empty expected on success, error string on failure
 */
std::expected<void, QString> Trigger::compileRegexp()
{
    if (!use_regexp) {
        // Not a regexp trigger, nothing to compile
        return {};
    }

    // Compile new regexp (reset releases old one)
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignore_case) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    // For multi-line triggers, ^ and $ match at line boundaries within the text
    if (multi_line) {
        options |= QRegularExpression::MultilineOption;
    }

    auto compiled = std::make_unique<QRegularExpression>(trigger, options);

    if (!compiled->isValid()) {
        QString error = compiled->errorString();
        qWarning() << "Failed to compile trigger regexp:" << trigger;
        qWarning() << "Error:" << error;
        regexp.reset();
        return std::unexpected(error);
    }

    regexp = std::move(compiled);
    return {};
}

/**
 * Match trigger against text
 *
 * Performs pattern matching based on trigger settings:
 * - Regular expression or literal text
 * - Case sensitivity
 * - Color/style matching
 * - Multi-line matching (TODO: )
 *
 * Populates wildcards vector on successful match.
 *
 * @param text Text to match against
 * @param foreColor Foreground color (for color matching)
 * @param backColor Background color (for color matching)
 * @param style Style flags (for style matching)
 * @return true if matched, false otherwise
 */
bool Trigger::match(const QString& text, QRgb foreColor, QRgb backColor, quint16 style)
{
    // Check if trigger is enabled
    if (!enabled) {
        return false;
    }

    // TODO: Implement color/style matching (match_type, style)
    Q_UNUSED(foreColor);
    Q_UNUSED(backColor);
    Q_UNUSED(style);

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
            for (int i = 0; i < MAX_WILDCARDS && i <= match.lastCapturedIndex(); ++i) {
                QString captured = match.captured(i);

                // Apply lowercase conversion if requested
                if (lowercase_wildcard && i > 0) { // Don't lowercase %0 (whole match)
                    captured = captured.toLower();
                }

                wildcards[i] = captured;
            }
        }
    } else {
        // Literal text matching
        QString searchText = text;
        QString searchPattern = trigger;

        if (ignore_case) {
            searchText = searchText.toLower();
            searchPattern = searchPattern.toLower();
        }

        did_match = searchText.contains(searchPattern);

        if (did_match) {
            // For literal matches, wildcard 0 is the matched text
            wildcards[0] = trigger;
        }
    }

    if (did_match) {
        // Update statistics
        matched++;
        when_matched = QDateTime::currentDateTime();
    }

    return did_match;
}
