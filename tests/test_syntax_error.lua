-- test_syntax_error.lua - Script with syntax error for Epic 4.3 testing

-- Valid code
world.Note("Starting script...")

-- Valid function
function OnWorldConnect()
    world.Note("Connected")
end

-- SYNTAX ERROR on line 13: Missing 'end' keyword
function broken_function()
    world.Note("This function is missing its end keyword")
-- Missing 'end' here!

-- More code after the error
world.Note("This won't execute due to syntax error")
