#ifndef TRIGGER_H
#define TRIGGER_H

#include "constants.h"
#include "script_language.h"
#include "sendto.h"
#include <QColor> // for QRgb typedef
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QVector>
#include <expected>
#include <memory>

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

// Trigger match bit masks (from original MUSHclient OtherTypes.h)
// match_type is a bitmask that packs multiple values:
//   Bits [7:4]  — ANSI text colour index (0-15)
//   Bits [11:8] — ANSI back colour index (0-15)
//   Bit  12     — Match bold (hilite)
//   Bit  13     — Match underline
//   Bit  14     — Match italic (blink)
//   Bit  15     — Match inverse
//   Bit  7      — Match text colour (high order of text nibble)
//   Bit  11     — Match back colour (high order of back nibble)
// Because this is a compound bitmask (not a closed set of named values),
// it stays as quint16 with named bit-mask constants rather than enum class.
inline constexpr quint16 TRIGGER_MATCH_TEXT = 0x0080; // high order of text nibble
inline constexpr quint16 TRIGGER_MATCH_BACK = 0x0800; // high order of back nibble
inline constexpr quint16 TRIGGER_MATCH_HILITE = 0x1000;
inline constexpr quint16 TRIGGER_MATCH_UNDERLINE = 0x2000;
inline constexpr quint16 TRIGGER_MATCH_BLINK = 0x4000; // italic
inline constexpr quint16 TRIGGER_MATCH_INVERSE = 0x8000;

// Trigger color change types (from original MUSHclient OtherTypes.h)
// At XML/Lua boundaries, cast quint16 to/from this enum.
enum class ColourChangeType : quint16 {
    Both = 0,       // Change both foreground and background
    Foreground = 1, // Change foreground only
    Background = 2, // Change background only
};

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

    QString trigger;        // Pattern to match
    bool ignore_case;       // If true, case-insensitive matching
    bool use_regexp;        // Use regular expressions
    bool repeat;            // Repeat on same line until no matches
    quint16 match_type;     // Match on color/bold/italic (see TRIGGER_MATCH_* defines)
    quint16 style;          // Underline, italic, bold
    bool multi_line;        // Do multi-line match
    quint16 lines_to_match; // How many lines to match (if multi-line)

    // ========== Action Fields ==========

    QString contents;              // What to send when triggered
    QString sound_to_play;         // Sound file to play
    QString procedure;             // Script procedure to execute
    ScriptLanguage scriptLanguage; // Script language (Lua or YueScript)
    SendTo send_to;                // Where trigger is sent (see SendTo enum)
    QString variable;              // Which variable to set (for send to variable)
    quint16 clipboard_arg;         // If non-zero, copy matching wildcard to clipboard

    // ========== Behavior Fields ==========

    bool enabled;            // If true, trigger is enabled
    bool keep_evaluating;    // If true, keep evaluating triggers after match
    bool expand_variables;   // Expand variables in trigger (e.g., @food)
    bool sound_if_inactive;  // Only play sound if window inactive
    bool lowercase_wildcard; // Convert wildcards to lowercase

    // ========== Display Fields ==========

    quint16 colour;                      // User color to display in
    bool omit_from_log;                  // If true, do not log triggered line
    bool omit_from_output;               // If true, do not put triggered line in output
    QRgb other_foreground;               // "Other" foreground color
    QRgb other_background;               // "Other" background color
    ColourChangeType colour_change_type; // Color change type

    // ========== Metadata Fields ==========

    QString label;      // Trigger label
    QString group;      // Group it belongs to
    quint16 sequence;   // Evaluation order (lower = sooner)
    qint32 user_option; // User-settable flags
    bool one_shot;      // If true, trigger only fires once

#ifdef PANE
    QString pane; // Which pane to send to (for send to pane)
#endif

    // ========== Runtime State Fields ==========

    qint32 dispid;                              // Dispatch ID for calling script
    qint64 update_number;                       // For detecting update clashes
    qint32 invocation_count;                    // How many times procedure called
    qint32 matched;                             // How many times trigger fired
    QVector<QString> wildcards;                 // Matching wildcards (MAX_WILDCARDS)
    QMap<QString, QString> namedWildcards;      // Named capture groups from regex
    std::unique_ptr<QRegularExpression> regexp; // Compiled regular expression
    QDateTime when_matched;                     // When last matched
    bool temporary;                             // If true, don't save it
    bool included;                              // If true, included from plugin
    bool selected;                              // If true, selected for use in plugin
    bool executing_script;                      // If true, executing script, cannot be deleted
    QString internal_name;                      // Name stored in trigger map
    Plugin* owningPlugin; // Plugin that owns this trigger (nullptr for world triggers)

    // ========== Helper Methods ==========

    /**
     * Compile regular expression if use_regexp is true
     * @return empty expected on success, error string on failure
     */
    [[nodiscard]] std::expected<void, QString> compileRegexp();

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
