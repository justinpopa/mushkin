-- test_runtime_error.lua - Script with runtime error for Epic 4.3 testing

-- Valid code
world.Note("Starting script with runtime error...")

-- Valid function
function OnWorldConnect()
    world.Note("Connected")
end

-- This will compile fine but fail at runtime
-- RUNTIME ERROR on line 14: Attempt to call nil value
local result = non_existent_function("test")

-- This won't execute due to runtime error
world.Note("This line won't execute")
