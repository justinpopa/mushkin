#ifndef TRIGGER_H
#define TRIGGER_H

#include <QColor> // for QRgb typedef
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QVector>

// Forward declarations
class WorldDocument;
class Plugin;

/**
 * Trigger Data Structure
 *
 * Based on CTrigger from OtherTypes.h
 *
 * Represents a trigger that matches incoming MUD text and executes actions.
 * Contains ~35 fields organized into:
 * - Pattern matching (trigger text, regexp, case, style matching)
 * - Actions (send content, script, sound)
 * - Behavior (enabled, keep evaluating, expand variables)
 * - Display (colors, omit from output/log)
 * - Metadata (label, group, sequence)
 * - Runtime state (DISPID, wildcards, statistics)
 */

// Trigger color change types (OtherTypes.h)
#define TRIGGER_COLOUR_CHANGE_BOTH 0
#define TRIGGER_COLOUR_CHANGE_FOREGROUND 1
#define TRIGGER_COLOUR_CHANGE_BACKGROUND 2

// Maximum wildcards for trigger matching
#define MAX_WILDCARDS 10

class Trigger : public QObject {
    Q_OBJECT

  public:
    /**
     * Constructor - initializes all fields to defaults
     * Based on CTrigger::CTrigger() from OtherTypes.h
     */
    explicit Trigger(QObject* parent = nullptr);

    /**
     * Destructor - cleans up compiled regexp
     */
    ~Trigger() override;

    // Equality operator for comparing triggers
    bool operator==(const Trigger& rhs) const;

    // ========== Pattern Matching Fields ==========

    QString trigger;       // Pattern to match
    quint16 ignore_case;   // If true, case-insensitive matching
    quint16 bRegexp;       // Use regular expressions
    quint16 bRepeat;       // Repeat on same line until no matches
    quint16 iMatch;        // Match on color/bold/italic (see TRIGGER_MATCH_* defines)
    quint16 iStyle;        // Underline, italic, bold
    quint16 bMultiLine;    // Do multi-line match
    quint16 iLinesToMatch; // How many lines to match (if multi-line)

    // ========== Action Fields ==========

    QString contents;      // What to send when triggered
    QString sound_to_play; // Sound file to play
    QString strProcedure;  // Lua procedure to execute
    quint16 iSendTo;       // Where trigger is sent (see SendTo enum)
    QString strVariable;   // Which variable to set (for send to variable)
    quint16 iClipboardArg; // If non-zero, copy matching wildcard to clipboard

    // ========== Behavior Fields ==========

    quint16 bEnabled;         // If true, trigger is enabled
    quint16 bKeepEvaluating;  // If true, keep evaluating triggers after match
    quint16 bExpandVariables; // Expand variables in trigger (e.g., @food)
    quint16 bSoundIfInactive; // Only play sound if window inactive
    bool bLowercaseWildcard;  // Convert wildcards to lowercase

    // ========== Display Fields ==========

    quint16 colour;            // User color to display in
    quint16 omit_from_log;     // If true, do not log triggered line
    quint16 bOmitFromOutput;   // If true, do not put triggered line in output
    QRgb iOtherForeground;     // "Other" foreground color
    QRgb iOtherBackground;     // "Other" background color
    quint16 iColourChangeType; // Color change type (see TRIGGER_COLOUR_CHANGE_* defines)

    // ========== Metadata Fields ==========

    QString strLabel;   // Trigger label
    QString strGroup;   // Group it belongs to
    quint16 iSequence;  // Evaluation order (lower = sooner)
    qint32 iUserOption; // User-settable flags
    bool bOneShot;      // If true, trigger only fires once

#ifdef PANE
    QString strPane; // Which pane to send to (for send to pane)
#endif

    // ========== Runtime State Fields ==========

    qint32 dispid;                         // Dispatch ID for calling script
    qint64 nUpdateNumber;                  // For detecting update clashes
    qint32 nInvocationCount;               // How many times procedure called
    qint32 nMatched;                       // How many times trigger fired
    QVector<QString> wildcards;            // Matching wildcards (MAX_WILDCARDS)
    QMap<QString, QString> namedWildcards; // Named capture groups from regex
    QRegularExpression* regexp;            // Compiled regular expression
    QDateTime tWhenMatched;                // When last matched
    bool bTemporary;                       // If true, don't save it
    bool bIncluded;                        // If true, included from plugin
    bool bSelected;                        // If true, selected for use in plugin
    bool bExecutingScript;                 // If true, executing script, cannot be deleted
    QString strInternalName;               // Name stored in trigger map
    Plugin* owningPlugin; // Plugin that owns this trigger (nullptr for world triggers)

    // ========== Helper Methods ==========

    /**
     * Compile regular expression if bRegexp is true
     * @return true on success, false on error
     */
    bool compileRegexp();

    /**
     * Match this trigger against a line of text
     * @param text Text to match against
     * @param foreColor Foreground color (for color matching)
     * @param backColor Background color (for color matching)
     * @param style Style flags (for style matching)
     * @return true if matched, false otherwise
     */
    bool match(const QString& text, QRgb foreColor = qRgb(255, 255, 255),
               QRgb backColor = qRgb(0, 0, 0), quint16 style = 0);

  private:
    void initializeDefaults();
};

#endif // TRIGGER_H
