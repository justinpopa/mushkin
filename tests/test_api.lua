-- test_api.lua - Epic 5.5: Trigger/Alias API Tests
-- Tests all world.* API functions

-- Test helper function
-- Note: Don't use tostring() in message - it crashes with LuaJIT
function check(condition, message)
    if not condition then
        error(message or "check failed")
    end
end

-- Test AddTrigger
function test_add_trigger()
    local flags = trigger_flag.Enabled
    local err = world.AddTrigger("test_trigger", "You have * gold", "say I have %1 gold", flags, 0, 0, "", "", sendto.World, 100)

    if err ~= 0 then
        error("AddTrigger returned error")
    end

    return 0  -- success
end

-- Test GetTriggerInfo
function test_get_trigger_info()
    -- Get match pattern (info type 1)
    local pattern = world.GetTriggerInfo("test_trigger", 1)
    check(pattern == "You have * gold", "GetTriggerInfo pattern mismatch")

    -- Get send text (info type 2)
    local send = world.GetTriggerInfo("test_trigger", 2)
    check(send == "say I have %1 gold", "GetTriggerInfo send mismatch")

    -- Get sound file (info type 3)
    local sound = world.GetTriggerInfo("test_trigger", 3)
    check(sound == "", "GetTriggerInfo sound mismatch")

    -- Get script name (info type 4)
    local script = world.GetTriggerInfo("test_trigger", 4)
    check(script == "", "GetTriggerInfo script mismatch")

    -- Get omit_from_output (info type 6)
    local omit_out = world.GetTriggerInfo("test_trigger", 6)
    check(omit_out == false, "GetTriggerInfo omit_from_output mismatch")

    -- Get enabled (info type 8) - CORRECTED from type 6!
    local enabled = world.GetTriggerInfo("test_trigger", 8)
    check(enabled == true, "GetTriggerInfo enabled mismatch")

    -- Get ignore_case (info type 10)
    local ignore_case = world.GetTriggerInfo("test_trigger", 10)
    check(ignore_case == false, "GetTriggerInfo ignore_case mismatch")

    -- Get send_to (info type 15)
    local send_to = world.GetTriggerInfo("test_trigger", 15)
    check(send_to == sendto.World, "GetTriggerInfo send_to mismatch")

    -- Get sequence (info type 16) - CORRECTED from type 20!
    local sequence = world.GetTriggerInfo("test_trigger", 16)
    check(sequence == 100, "GetTriggerInfo sequence mismatch")

    -- Get invocation count (info type 20) - CORRECTED!
    local invocation = world.GetTriggerInfo("test_trigger", 20)
    check(invocation == 0, "GetTriggerInfo invocation count mismatch")

    -- Get times matched (info type 21) - CORRECTED from type 11!
    local matched = world.GetTriggerInfo("test_trigger", 21)
    check(matched == 0, "GetTriggerInfo matched count mismatch")

    return 0
end

-- Test GetTriggerList
function test_get_trigger_list()
    local list = world.GetTriggerList()
    check(type(list) == "table", "GetTriggerList didn't return table")
    check(#list >= 1, "GetTriggerList returned empty list")

    -- Find our trigger (use numeric for loop instead of ipairs to avoid crash)
    local found = false
    for i = 1, #list do
        if list[i] == "test_trigger" then
            found = true
            break
        end
    end
    check(found, "test_trigger not in GetTriggerList")

    return 0
end

-- Test EnableTrigger
function test_enable_trigger()
    -- Disable trigger
    local err = world.EnableTrigger("test_trigger", false)
    check(err == 0, "EnableTrigger(false) returned error")

    -- Verify disabled (info type 8 = enabled)
    local enabled = world.GetTriggerInfo("test_trigger", 8)
    check(enabled == false, "Trigger not disabled")

    -- Re-enable trigger
    err = world.EnableTrigger("test_trigger", true)
    check(err == 0, "EnableTrigger(true) returned error")

    -- Verify enabled (info type 8 = enabled)
    enabled = world.GetTriggerInfo("test_trigger", 8)
    check(enabled == true, "Trigger not re-enabled")

    return 0
end

-- Test DeleteTrigger
function test_delete_trigger()
    local err = world.DeleteTrigger("test_trigger")
    check(err == 0, "DeleteTrigger returned error")

    -- Verify deleted (GetTriggerInfo should return nil)
    local pattern = world.GetTriggerInfo("test_trigger", 1)
    check(pattern == nil, "Trigger not deleted, pattern")

    return 0
end

-- Test AddAlias
function test_add_alias()
    local flags = alias_flag.Enabled + alias_flag.IgnoreCase
    local err = world.AddAlias("test_alias", "n*", "north", flags, "")

    check(err == 0, "AddAlias returned error")

    return 0
end

-- Test GetAliasInfo
function test_get_alias_info()
    -- Get match pattern (info type 1)
    local pattern = world.GetAliasInfo("test_alias", 1)
    check(pattern == "n*", "GetAliasInfo pattern mismatch")

    -- Get send text (info type 2)
    local send = world.GetAliasInfo("test_alias", 2)
    check(send == "north", "GetAliasInfo send mismatch")

    -- Get script name (info type 3)
    local script = world.GetAliasInfo("test_alias", 3)
    check(script == "", "GetAliasInfo script mismatch")

    -- Get omit_from_log (info type 4)
    local omit_log = world.GetAliasInfo("test_alias", 4)
    check(omit_log == false, "GetAliasInfo omit_from_log mismatch")

    -- Get enabled (info type 6)
    local enabled = world.GetAliasInfo("test_alias", 6)
    check(enabled == true, "GetAliasInfo enabled mismatch")

    -- Get ignore_case (info type 8)
    local ignore_case = world.GetAliasInfo("test_alias", 8)
    check(ignore_case == true, "GetAliasInfo ignore_case mismatch")

    -- Get invocation count (info type 10)
    local invocation = world.GetAliasInfo("test_alias", 10)
    check(invocation == 0, "GetAliasInfo invocation count mismatch")

    -- Get times matched (info type 11)
    local matched = world.GetAliasInfo("test_alias", 11)
    check(matched == 0, "GetAliasInfo matched count mismatch")

    -- Get send_to (info type 18)
    local send_to = world.GetAliasInfo("test_alias", 18)
    check(send_to == sendto.World, "GetAliasInfo send_to mismatch")

    -- Get sequence (info type 20)
    local sequence = world.GetAliasInfo("test_alias", 20)
    check(sequence == 100, "GetAliasInfo sequence mismatch")

    return 0
end

-- Test GetAliasList
function test_get_alias_list()
    local list = world.GetAliasList()
    check(type(list) == "table", "GetAliasList didn't return table")
    check(#list >= 1, "GetAliasList returned empty list")

    -- Find our alias (use numeric for loop instead of ipairs to avoid crash)
    local found = false
    for i = 1, #list do
        if list[i] == "test_alias" then
            found = true
            break
        end
    end
    check(found, "test_alias not in GetAliasList")

    return 0
end

-- Test EnableAlias
function test_enable_alias()
    -- Disable alias
    local err = world.EnableAlias("test_alias", false)
    check(err == 0, "EnableAlias(false) returned error")

    -- Verify disabled
    local enabled = world.GetAliasInfo("test_alias", 6)
    check(enabled == false, "Alias not disabled")

    -- Re-enable alias
    err = world.EnableAlias("test_alias", true)
    check(err == 0, "EnableAlias(true) returned error")

    -- Verify enabled
    enabled = world.GetAliasInfo("test_alias", 6)
    check(enabled == true, "Alias not re-enabled")

    return 0
end

-- Test DeleteAlias
function test_delete_alias()
    local err = world.DeleteAlias("test_alias")
    check(err == 0, "DeleteAlias returned error")

    -- Verify deleted (GetAliasInfo should return nil)
    local pattern = world.GetAliasInfo("test_alias", 1)
    check(pattern == nil, "Alias not deleted, pattern")

    return 0
end

-- ========== Command Window Functions ==========

-- Test GetCommand (requires InputView to be set up by C++ test)
function test_get_command()
    local cmd = world.GetCommand()
    check(type(cmd) == "string", "GetCommand should return string")

    return 0
end

-- Test SetCommand with valid input
function test_set_command_valid()
    -- This assumes InputView is empty (set up by C++ test)
    local err = world.SetCommand("test command")
    check(err == error_code.eOK, "SetCommand should succeed on empty input")

    -- Verify it was set
    local cmd = world.GetCommand()
    check(cmd == "test command", "Command should be set correctly")

    return 0
end

-- Test SetCommand when input not empty
function test_set_command_not_empty()
    -- This assumes InputView has text (set up by C++ test)
    local err = world.SetCommand("other command")
    check(err == 30011, "SetCommand should return eCommandNotEmpty (30011)")

    return 0
end

-- Test SetCommandSelection
function test_set_command_selection()
    -- Assumes InputView has "test command" (set up by C++ test)
    -- Select "test" (chars 1-4, 1-based)
    local err = world.SetCommandSelection(1, 4)
    check(err == error_code.eOK, "SetCommandSelection should succeed")

    return 0
end

-- Test SetCommandSelection with -1 (end of text)
function test_set_command_selection_end()
    -- Select from position 6 to end
    local err = world.SetCommandSelection(6, -1)
    check(err == error_code.eOK, "SetCommandSelection with -1 should succeed")

    return 0
end

-- ========== Color Functions ==========

-- Test SetCustomColourName with valid input
function test_set_custom_colour_name_valid()
    local err = world.SetCustomColourName(1, "MyRed")
    check(err == error_code.eOK, "SetCustomColourName should succeed")

    return 0
end

-- Test SetCustomColourName with out of range index
function test_set_custom_colour_name_out_of_range()
    -- Test below range
    local err = world.SetCustomColourName(0, "Test")
    check(err == 30009, "SetCustomColourName should return eOptionOutOfRange (30009) for index 0")

    -- Test above range (MAX_CUSTOM = 16)
    err = world.SetCustomColourName(17, "Test")
    check(err == 30009, "SetCustomColourName should return eOptionOutOfRange (30009) for index 17")

    return 0
end

-- Test SetCustomColourName with empty name
function test_set_custom_colour_name_empty()
    local err = world.SetCustomColourName(1, "")
    check(err == 30003, "SetCustomColourName should return eNoNameSpecified (30003)")

    return 0
end

-- Test SetCustomColourName with name too long
function test_set_custom_colour_name_too_long()
    -- Name with 31 characters (max is 30)
    local long_name = "1234567890123456789012345678901"
    local err = world.SetCustomColourName(1, long_name)
    check(err == 30008, "SetCustomColourName should return eInvalidObjectLabel (30008)")

    return 0
end

-- Test SetCustomColourName with max length name (30 chars)
function test_set_custom_colour_name_max_length()
    -- Name with exactly 30 characters
    local max_name = "123456789012345678901234567890"
    local err = world.SetCustomColourName(1, max_name)
    check(err == error_code.eOK, "SetCustomColourName should accept 30 character name")

    return 0
end

-- Test SetCustomColourName modifies document
function test_set_custom_colour_name_different_values()
    -- Set name for color 1
    local err = world.SetCustomColourName(1, "Color1")
    check(err == error_code.eOK, "SetCustomColourName(1) should succeed")

    -- Set name for color 2
    err = world.SetCustomColourName(2, "Color2")
    check(err == error_code.eOK, "SetCustomColourName(2) should succeed")

    -- Set name for color 16 (MAX_CUSTOM)
    err = world.SetCustomColourName(16, "Color16")
    check(err == error_code.eOK, "SetCustomColourName(16) should succeed")

    return 0
end

-- ========== Utility Functions ==========

-- Test GetUdpPort (deprecated, always returns 0)
function test_get_udp_port()
    local port = world.GetUdpPort(5000, 6000)
    check(port == 0, "GetUdpPort should return 0 (deprecated)")

    -- Test with different parameters (should still return 0)
    port = world.GetUdpPort(1, 65535)
    check(port == 0, "GetUdpPort should always return 0")

    return 0
end

print("Lua test script loaded successfully")
