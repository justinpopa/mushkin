#include "action.h"
#include <QHash>

/**
 * Constructor
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * Initializes the Action with the provided strings and calculates a hash
 * for fast lookup.
 */
Action::Action(const QString& action, const QString& hint, const QString& variable,
               WorldDocument* doc)
    : m_strAction(action), m_strHint(hint), m_strVariable(variable), m_pDoc(doc)
{
    // Calculate hash for fast lookup
    // Combine all three strings to create a unique hash
    QString combined = action + hint + variable;
    m_iHash = qHash(combined);
}

/**
 * Destructor
 *
 * Source: OtherTypes.h (implicit destructor)
 *
 * Nothing special to clean up since QString handles its own memory.
 */
Action::~Action()
{
    // Nothing to do - QString cleans up automatically
}
