/**
 * world_triggers.cpp - Trigger and Alias Management
 *
 * Implements trigger and alias management methods for WorldDocument.
 * Handles adding, deleting, and retrieving triggers and aliases.
 */

#include "../automation/alias.h"
#include "../automation/trigger.h"
#include "logging.h"
#include "world_document.h"
#include <QDebug>
#include <algorithm>

// ========== Trigger Management Methods ==========

/**
 * addTrigger - Add trigger to map and array
 *
 * Adds a trigger to both the TriggerMap (for fast lookup by name)
 * and TriggerArray (for sequence-based evaluation).
 *
 * @param name Trigger name (must be unique)
 * @param trigger Pointer to Trigger object (ownership transferred)
 * @return true if added, false if name already exists
 */
bool WorldDocument::addTrigger(const QString& name, std::unique_ptr<Trigger> trigger)
{
    // Check for duplicate name
    if (m_TriggerMap.find(name) != m_TriggerMap.end()) {
        qWarning() << "Trigger already exists with name:" << name;
        return false;
    }

    // Set internal name
    trigger->strInternalName = name;

    // Get raw pointer before moving
    Trigger* rawPtr = trigger.get();

    // Add to map (takes ownership)
    m_TriggerMap[name] = std::move(trigger);

    // Add to array (non-owning pointer)
    m_TriggerArray.append(rawPtr);

    // Mark array as needing rebuild (lazy sorting)
    m_triggersNeedSorting = true;

    qCDebug(lcWorld) << "Added trigger:" << name << "sequence:" << rawPtr->iSequence;

    return true;
}

/**
 * deleteTrigger - Delete trigger by name
 *
 * Removes trigger from both map and array, and deletes the object.
 *
 * @param name Trigger name to delete
 * @return true if deleted, false if not found
 */
bool WorldDocument::deleteTrigger(const QString& name)
{
    // Find in map
    auto it = m_TriggerMap.find(name);
    if (it == m_TriggerMap.end()) {
        qWarning() << "Trigger not found:" << name;
        return false;
    }

    Trigger* trigger = it->second.get();

    // Cannot delete trigger while script is executing
    if (trigger->bExecutingScript) {
        qCDebug(lcWorld) << "Cannot delete trigger while script is executing:" << name;
        return false;
    }

    // Remove from array (must be done before erasing from map)
    m_TriggerArray.removeOne(trigger);

    // Remove from map (unique_ptr automatically deletes the object)
    m_TriggerMap.erase(it);

    // Mark array as needing rebuild
    m_triggersNeedSorting = true;

    qCDebug(lcWorld) << "Deleted trigger:" << name;

    return true;
}

/**
 * getTrigger - Get trigger by name
 *
 * @param name Trigger name to find
 * @return Pointer to Trigger, or nullptr if not found
 */
Trigger* WorldDocument::getTrigger(const QString& name)
{
    auto it = m_TriggerMap.find(name);
    return (it != m_TriggerMap.end()) ? it->second.get() : nullptr;
}

// ========== Alias Management Methods ==========

/**
 * addAlias - Add alias to map and array
 *
 * Adds an alias to both the AliasMap (for fast lookup by name)
 * and AliasArray (for sequence-based evaluation).
 *
 * @param name Alias name (must be unique)
 * @param alias Pointer to Alias object (ownership transferred)
 * @return true if added, false if name already exists
 */
bool WorldDocument::addAlias(const QString& name, std::unique_ptr<Alias> alias)
{
    // Check for duplicate name
    if (m_AliasMap.find(name) != m_AliasMap.end()) {
        qWarning() << "Alias already exists with name:" << name;
        return false;
    }

    // Set internal name
    alias->strInternalName = name;

    // Get raw pointer before moving
    Alias* rawPtr = alias.get();

    // Add to map (takes ownership)
    m_AliasMap[name] = std::move(alias);

    // Add to array (non-owning pointer)
    m_AliasArray.append(rawPtr);

    // Mark array as needing rebuild (lazy sorting)
    m_aliasesNeedSorting = true;

    qCDebug(lcWorld) << "Added alias:" << name << "sequence:" << rawPtr->iSequence;

    return true;
}

/**
 * deleteAlias - Delete alias by name
 *
 * Removes alias from both map and array, and deletes the object.
 *
 * @param name Alias name to delete
 * @return true if deleted, false if not found
 */
bool WorldDocument::deleteAlias(const QString& name)
{
    // Find in map
    auto it = m_AliasMap.find(name);
    if (it == m_AliasMap.end()) {
        qWarning() << "Alias not found:" << name;
        return false;
    }

    Alias* alias = it->second.get();

    // Cannot delete alias while script is executing
    if (alias->bExecutingScript) {
        qCDebug(lcWorld) << "Cannot delete alias while script is executing:" << name;
        return false;
    }

    // Remove from array (must be done before erasing from map)
    m_AliasArray.removeOne(alias);

    // Remove from map (unique_ptr automatically deletes the object)
    m_AliasMap.erase(it);

    // Mark array as needing rebuild
    m_aliasesNeedSorting = true;

    qCDebug(lcWorld) << "Deleted alias:" << name;

    return true;
}

/**
 * getAlias - Get alias by name
 *
 * @param name Alias name to find
 * @return Pointer to Alias, or nullptr if not found
 */
Alias* WorldDocument::getAlias(const QString& name)
{
    auto it = m_AliasMap.find(name);
    return (it != m_AliasMap.end()) ? it->second.get() : nullptr;
}
