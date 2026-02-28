/**
 * docommand_callbacks.cpp - Static storage for DoCommand UI callback
 */

#include "docommand_callbacks.h"

namespace DoCommandCallbacks {

static ExecuteCommandFunc s_callback = nullptr;

void set(ExecuteCommandFunc cb)
{
    s_callback = std::move(cb);
}

ExecuteCommandFunc get()
{
    return s_callback;
}

} // namespace DoCommandCallbacks
