#ifndef ALIAS_H
#define ALIAS_H

#include "constants.h"
#include "script_language.h"
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QVector>
#include <expected>
#include <memory>

/**
 * Alias Data Structure
 *
 * Based on CAlias from OtherTypes.h
 *
 * Represents an alias that matches user input and executes actions.
 * Contains ~30 fields organized into:
 * - Pattern matching (name pattern, regexp, case)
 * - Actions (contents to send, script procedure)
 * - Behavior (enabled, expand variables, keep evaluating)
 * - Display (echo alias, omit from log/output/history)
 * - Metadata (label, group, sequence, menu)
 * - Runtime state (DISPID, wildcards, statistics)
 */

class Alias : public QObject {
    Q_OBJECT

  public:
    /**
     * Constructor - initializes all fields to defaults
     * Based on CAlias::CAlias() from OtherTypes.h
     */
    explicit Alias(QObject* parent = nullptr);

    /**
     * Destructor - cleans up compiled regexp
     */
    ~Alias() override;

    // Equality operator for comparing aliases
    bool operator==(const Alias& rhs) const;

    // ========== Pattern Matching Fields ==========

    QString name;     // Alias pattern to match
    bool ignore_case; // If true, case-insensitive matching
    bool use_regexp;  // Use regular expressions

    // ========== Action Fields ==========

    QString contents;              // What to send when matched
    QString procedure;             // Script procedure to execute
    ScriptLanguage scriptLanguage; // Script language (Lua or YueScript)
    quint16 send_to;               // Where alias is sent (see SendTo enum)
    QString variable;              // Which variable to set (for send to variable)
    bool expand_variables;         // Expand variables (e.g., @food)

    // ========== Behavior Fields ==========

    bool enabled;         // If true, alias is enabled
    bool keep_evaluating; // If true, keep evaluating aliases after match

    // ========== Display Fields ==========

    bool omit_from_log;             // Omit from log file
    bool omit_from_output;          // Omit alias from output screen
    bool echo_alias;                // Echo alias itself to output window
    bool omit_from_command_history; // Omit from command history

    // ========== Metadata Fields ==========

    QString label;      // Alias label
    QString group;      // Group it belongs to
    quint16 sequence;   // Evaluation order (lower = sooner)
    bool menu;          // Make pop-up menu from this alias
    qint32 user_option; // User-settable flags
    bool one_shot;      // If true, alias only fires once

    // ========== Runtime State Fields ==========

    qint32 dispid;                              // Dispatch ID for calling script
    qint64 update_number;                       // For detecting update clashes
    qint32 invocation_count;                    // How many times procedure called
    qint32 matched;                             // How many times alias matched
    QVector<QString> wildcards;                 // Matching wildcards (MAX_WILDCARDS)
    QMap<QString, QString> namedWildcards;      // Named capture groups from regex
    std::unique_ptr<QRegularExpression> regexp; // Compiled regular expression
    QDateTime when_matched;                     // When last matched
    bool temporary;                             // If true, don't save it
    bool included;                              // If true, included from plugin
    bool selected;                              // If true, selected for use in plugin
    bool executing_script;                      // If true, executing script, cannot be deleted
    QString internal_name;                      // Name stored in alias map

    // ========== Helper Methods ==========

    /**
     * Compile regular expression if use_regexp is true
     * @return empty expected on success, error string on failure
     */
    [[nodiscard]] std::expected<void, QString> compileRegexp();

    /**
     * Match this alias against user input
     * @param text User input to match against
     * @return true if matched, false otherwise
     */
    bool match(const QString& text);

  private:
    void initializeDefaults();
};

#endif // ALIAS_H
