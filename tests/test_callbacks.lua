-- test_callbacks.lua
-- Test script for Epic 4.4: Lua Function Callbacks
--
-- This script defines callback functions that will be invoked
-- by MUSHclient when world events occur.

-- Track callback invocations
callback_log = {}

-- OnWorldConnect callback
-- Called when connection to MUD succeeds
function OnWorldConnect()
    table.insert(callback_log, "OnWorldConnect called")
    world.Note("OnWorldConnect: Successfully connected to MUD!")
    return true
end

-- OnWorldDisconnect callback
-- Called when connection to MUD closes
function OnWorldDisconnect()
    table.insert(callback_log, "OnWorldDisconnect called")
    world.Note("OnWorldDisconnect: Disconnected from MUD")
    return true
end

-- Test nested function callbacks
utils = {}

function utils.OnWorldConnect()
    table.insert(callback_log, "utils.OnWorldConnect called")
    world.Note("Nested callback: utils.OnWorldConnect")
    return true
end

-- Helper function to display callback log
function ShowCallbackLog()
    world.Note("=== Callback Log ===")
    for i, msg in ipairs(callback_log) do
        world.Note(string.format("%d. %s", i, msg))
    end
    world.Note(string.format("Total callbacks: %d", #callback_log))
end

-- Print startup message
world.Note("Callback test script loaded successfully!")
world.Note("Callbacks defined: OnWorldConnect, OnWorldDisconnect, utils.OnWorldConnect")
world.Note("Use ShowCallbackLog() to see callback history")
