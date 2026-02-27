#ifndef WORLD_ERROR_H
#define WORLD_ERROR_H

#include <QString>

/**
 * WorldErrorType - Failure modes for fallible WorldDocument operations
 *
 * Used with std::expected<T, WorldError> to signal precise failure reasons
 * without relying on bool or integer sentinel values in C++ internal code.
 */
enum class WorldErrorType {
    AlreadyExists,   // Item with that name already exists in the registry
    NotFound,        // Item with that name was not found
    ScriptExecuting, // Cannot mutate item while its script is executing
    InitFailed,      // Low-level initialisation failed (e.g. zlib)
    NotepadNotFound, // No notepad window with that title exists
};

struct WorldError {
    WorldErrorType type;
    QString message;
};

#endif // WORLD_ERROR_H
