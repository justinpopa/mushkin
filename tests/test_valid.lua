-- test_valid.lua - Valid Lua script for Epic 4.3 testing

-- Define a simple function
function OnWorldConnect()
    world.Note("Hello from test script!")
    world.Note("Connection established")
end

-- Define another function
function OnWorldDisconnect()
    world.Note("Goodbye from test script!")
end

-- Test basic Lua functionality
local test_var = "This is a test"
local test_num = 42

-- Define a helper function
function greet(name)
    return "Hello, " .. name .. "!"
end

world.Note("Script loaded successfully!")
world.Note("Test variable: " .. test_var)
world.Note("Test number: " .. test_num)
world.Note(greet("World"))
