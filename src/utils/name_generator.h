#ifndef NAME_GENERATOR_H
#define NAME_GENERATOR_H

#include <QString>

/**
 * Generate a random fantasy character name
 *
 * Reads syllables from the embedded names.txt resource file
 * and combines them to create names like "Drakmorwyn" or "Zarethion"
 *
 * @return Generated character name, or empty string on error
 */
QString generateCharacterName();

/**
 * Generate a cryptographically unique identifier
 *
 * Creates a 24-character hex string suitable for plugin IDs,
 * based on QUuid with SHA-1 hashing (matching original MUSHclient).
 *
 * @return 24-character hex ID like "3e7dedcf168620e8f3e7d3a6"
 */
QString generateUniqueID();

/**
 * Create a GUID in standard format
 *
 * Returns a UUID/GUID string with dashes in the format:
 * XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
 *
 * @return GUID string like "550e8400-e29b-41d4-a716-446655440000"
 */
QString createGUID();

#endif // NAME_GENERATOR_H
