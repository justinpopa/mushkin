/**
 * trigger.cpp - Trigger Data Structure
 *
 * Based on CTrigger from OtherTypes.h
 */

#include "trigger.h"
#include "sendto.h"
#include <QDebug>

// Default trigger sequence (OtherTypes.h)
#define DEFAULT_TRIGGER_SEQUENCE 100

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

/**
 * Constructor - Initialize all fields to defaults
 *
 * Based on CTrigger::CTrigger() from OtherTypes.h
 */
Trigger::Trigger(QObject* parent) : QObject(parent), regexp(nullptr)
{
    initializeDefaults();
}

/**
 * Destructor - Clean up regexp
 */
Trigger::~Trigger()
{
    delete regexp;
}

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
    bOmitFromOutput = false;
    bKeepEvaluating = true; // MUSHclient default: continue evaluating other triggers after match
    bExpandVariables = false;
    bEnabled = true;
    bRegexp = false;
    bRepeat = false;
    iSequence = DEFAULT_TRIGGER_SEQUENCE;
    iMatch = 0;
    iStyle = 0;
    bSoundIfInactive = false;
    bLowercaseWildcard = false;
    bMultiLine = false;
    iLinesToMatch = 0;

    // Actions
    iSendTo = eSendToWorld;
    iClipboardArg = 0;
    scriptLanguage = ScriptLanguage::Lua;

    // Display
    colour = 0; // Custom color 1
    iOtherForeground = 0;
    iOtherBackground = 0;
    iColourChangeType = TRIGGER_COLOUR_CHANGE_BOTH;

    // Metadata
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
           bOmitFromOutput == rhs.bOmitFromOutput && bKeepEvaluating == rhs.bKeepEvaluating &&
           bEnabled == rhs.bEnabled && strLabel == rhs.strLabel &&
           strProcedure == rhs.strProcedure && scriptLanguage == rhs.scriptLanguage &&
           iClipboardArg == rhs.iClipboardArg && iSendTo == rhs.iSendTo &&
           bRegexp == rhs.bRegexp && bRepeat == rhs.bRepeat && iSequence == rhs.iSequence &&
           iMatch == rhs.iMatch && iStyle == rhs.iStyle &&
           bSoundIfInactive == rhs.bSoundIfInactive && bExpandVariables == rhs.bExpandVariables &&
           bLowercaseWildcard == rhs.bLowercaseWildcard && strGroup == rhs.strGroup &&
           strVariable == rhs.strVariable && iUserOption == rhs.iUserOption &&
           iOtherForeground == rhs.iOtherForeground && iOtherBackground == rhs.iOtherBackground &&
           bMultiLine == rhs.bMultiLine && iLinesToMatch == rhs.iLinesToMatch &&
           iColourChangeType == rhs.iColourChangeType && bOneShot == rhs.bOneShot;
}

/**
 * Compile regular expression
 *
 * Compiles the trigger pattern into a QRegularExpression.
 * Handles case-sensitivity based on ignore_case flag.
 * For multi-line triggers, enables MultiLineOption so ^ and $ match line boundaries.
 *
 * @return true on success, false on error
 */
bool Trigger::compileRegexp()
{
    if (!bRegexp) {
        // Not a regexp trigger, nothing to compile
        return true;
    }

    // Clean up old regexp if exists
    delete regexp;
    regexp = nullptr;

    // Compile new regexp
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ignore_case) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    // For multi-line triggers, ^ and $ match at line boundaries within the text
    if (bMultiLine) {
        options |= QRegularExpression::MultilineOption;
    }

    regexp = new QRegularExpression(trigger, options);

    if (!regexp->isValid()) {
        qWarning() << "Failed to compile trigger regexp:" << trigger;
        qWarning() << "Error:" << regexp->errorString();
        delete regexp;
        regexp = nullptr;
        return false;
    }

    return true;
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
    if (!bEnabled) {
        return false;
    }

    // TODO: Implement color/style matching (iMatch, iStyle)
    Q_UNUSED(foreColor);
    Q_UNUSED(backColor);
    Q_UNUSED(style);

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
            for (int i = 0; i < MAX_WILDCARDS && i <= match.lastCapturedIndex(); ++i) {
                QString captured = match.captured(i);

                // Apply lowercase conversion if requested
                if (bLowercaseWildcard && i > 0) { // Don't lowercase %0 (whole match)
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

        matched = searchText.contains(searchPattern);

        if (matched) {
            // For literal matches, wildcard 0 is the matched text
            wildcards[0] = trigger;
        }
    }

    if (matched) {
        // Update statistics
        nMatched++;
        tWhenMatched = QDateTime::currentDateTime();
    }

    return matched;
}
