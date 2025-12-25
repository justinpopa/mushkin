#ifndef ACTION_H
#define ACTION_H

#include <QString>
#include <QtGlobal>

// Forward declaration
class WorldDocument;

/**
 * Action - Hyperlink/clickable text data
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * WHY THIS EXISTS:
 * Actions are shared between multiple Style objects to save memory.
 * For example, a long hyperlink command might be repeated for every
 * character if they're in different colors. By sharing the Action object
 * via std::shared_ptr, we save significant memory.
 *
 * MEMORY MANAGEMENT:
 * Uses std::shared_ptr for automatic reference counting and cleanup.
 * No manual memory management required.
 */
class Action {
  public:
    /**
     * Constructor
     * @param action What to send/execute (pipe-delimited for multiple commands)
     * @param hint Tooltip text to display on hover
     * @param variable Which variable to set (for MXP FLAG attribute)
     * @param doc Back-pointer to owning document
     */
    Action(const QString& action, const QString& hint, const QString& variable, WorldDocument* doc);

    /**
     * Destructor
     */
    ~Action();

    // Public members (same as original for direct access)
    QString m_strAction;   // What to send - multiple commands delimited by |
    QString m_strHint;     // Hint - flyover tooltip, and prompts for actions
    QString m_strVariable; // Which variable to set (FLAG in MXP)
    quint32 m_iHash;       // For quick lookups - hash of action, hint, variable

  protected:
    WorldDocument* m_pDoc; // Which document this Action belongs to
};

#endif // ACTION_H
