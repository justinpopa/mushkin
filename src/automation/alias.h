#ifndef ALIAS_H
#define ALIAS_H

#include "script_language.h"
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QVector>

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

// Maximum wildcards for alias matching
#define MAX_WILDCARDS 10

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

    QString name;        // Alias pattern to match
    quint16 bIgnoreCase; // If true, case-insensitive matching
    quint16 bRegexp;     // Use regular expressions

    // ========== Action Fields ==========

    QString contents;              // What to send when matched
    QString strProcedure;          // Script procedure to execute
    ScriptLanguage scriptLanguage; // Script language (Lua or YueScript)
    quint16 iSendTo;               // Where alias is sent (see SendTo enum)
    QString strVariable;      // Which variable to set (for send to variable)
    quint16 bExpandVariables; // Expand variables (e.g., @food)

    // ========== Behavior Fields ==========

    quint16 bEnabled;        // If true, alias is enabled
    quint16 bKeepEvaluating; // If true, keep evaluating aliases after match

    // ========== Display Fields ==========

    quint16 bOmitFromLog;            // Omit from log file
    quint16 bOmitFromOutput;         // Omit alias from output screen
    quint16 bEchoAlias;              // Echo alias itself to output window
    quint16 bOmitFromCommandHistory; // Omit from command history

    // ========== Metadata Fields ==========

    QString strLabel;   // Alias label
    QString strGroup;   // Group it belongs to
    quint16 iSequence;  // Evaluation order (lower = sooner)
    quint16 bMenu;      // Make pop-up menu from this alias
    qint32 iUserOption; // User-settable flags
    bool bOneShot;      // If true, alias only fires once

    // ========== Runtime State Fields ==========

    qint32 dispid;                         // Dispatch ID for calling script
    qint64 nUpdateNumber;                  // For detecting update clashes
    qint32 nInvocationCount;               // How many times procedure called
    qint32 nMatched;                       // How many times alias matched
    QVector<QString> wildcards;            // Matching wildcards (MAX_WILDCARDS)
    QMap<QString, QString> namedWildcards; // Named capture groups from regex
    QRegularExpression* regexp;            // Compiled regular expression
    QDateTime tWhenMatched;                // When last matched
    bool bTemporary;                       // If true, don't save it
    bool bIncluded;                        // If true, included from plugin
    bool bSelected;                        // If true, selected for use in plugin
    bool bExecutingScript;                 // If true, executing script, cannot be deleted
    QString strInternalName;               // Name stored in alias map

    // ========== Helper Methods ==========

    /**
     * Compile regular expression if bRegexp is true
     * @return true on success, false on error
     */
    bool compileRegexp();

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
