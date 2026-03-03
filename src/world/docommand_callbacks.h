/**
 * docommand_callbacks.h - Callback interface for DoCommand UI dispatch
 *
 * Allows the world module to dispatch DoCommand() calls to MainWindow
 * without directly linking against the ui module.
 *
 * The ui module registers its implementation at startup via set(),
 * and world_commands.cpp calls through get() as a fallback after
 * document-tier and macro/keypad dispatch.
 */

#ifndef DOCOMMAND_CALLBACKS_H
#define DOCOMMAND_CALLBACKS_H

#include <functional>

namespace DoCommandCallbacks {

using ExecuteCommandFunc = std::function<int(const char* name)>;

void set(ExecuteCommandFunc cb);
ExecuteCommandFunc get();

} // namespace DoCommandCallbacks

#endif // DOCOMMAND_CALLBACKS_H
