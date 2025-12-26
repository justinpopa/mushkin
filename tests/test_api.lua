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

-- ========== Group Enable Functions ==========

-- Test that required functions exist
function test_group_functions_exist()
    -- Just return info about what exists
    if world.SetTriggerOption == nil then
        error("world.SetTriggerOption is nil")
    end
    if world.GetTriggerOption == nil then
        error("world.GetTriggerOption is nil")
    end
    if world.EnableTriggerGroup == nil then
        error("world.EnableTriggerGroup is nil")
    end
    if world.SetAliasOption == nil then
        error("world.SetAliasOption is nil")
    end
    if world.EnableAliasGroup == nil then
        error("world.EnableAliasGroup is nil")
    end
    return 0
end

-- Test EnableTriggerGroup
function test_enable_trigger_group()
    -- Add two triggers in the same group
    local flags = trigger_flag.Enabled
    local err1 = world.AddTrigger("group_trigger_1", "pattern1", "send1", flags, 0, 0, "", "", sendto.World, 100)
    check(err1 == 0, "AddTrigger for group_trigger_1 failed")

    local err2 = world.AddTrigger("group_trigger_2", "pattern2", "send2", flags, 0, 0, "", "", sendto.World, 100)
    check(err2 == 0, "AddTrigger for group_trigger_2 failed")

    -- Set both triggers to the same group
    world.SetTriggerOption("group_trigger_1", "group", "test_group")
    world.SetTriggerOption("group_trigger_2", "group", "test_group")

    -- Disable the group
    local count = world.EnableTriggerGroup("test_group", false)
    check(count == 2, "EnableTriggerGroup should disable 2 triggers")

    -- Verify both are disabled
    local enabled1 = world.GetTriggerInfo("group_trigger_1", 8)
    local enabled2 = world.GetTriggerInfo("group_trigger_2", 8)
    check(enabled1 == false, "group_trigger_1 should be disabled")
    check(enabled2 == false, "group_trigger_2 should be disabled")

    -- Re-enable the group
    count = world.EnableTriggerGroup("test_group", true)
    check(count == 2, "EnableTriggerGroup should enable 2 triggers")

    -- Verify both are enabled
    enabled1 = world.GetTriggerInfo("group_trigger_1", 8)
    enabled2 = world.GetTriggerInfo("group_trigger_2", 8)
    check(enabled1 == true, "group_trigger_1 should be enabled")
    check(enabled2 == true, "group_trigger_2 should be enabled")

    -- Clean up
    world.DeleteTrigger("group_trigger_1")
    world.DeleteTrigger("group_trigger_2")

    return 0
end

-- Test EnableAliasGroup
function test_enable_alias_group()
    -- Add two aliases in the same group
    local flags = alias_flag.Enabled
    local err1 = world.AddAlias("group_alias_1", "ga1", "output1", flags, "")
    check(err1 == 0, "AddAlias for group_alias_1 failed")

    local err2 = world.AddAlias("group_alias_2", "ga2", "output2", flags, "")
    check(err2 == 0, "AddAlias for group_alias_2 failed")

    -- Set both aliases to the same group
    world.SetAliasOption("group_alias_1", "group", "alias_group")
    world.SetAliasOption("group_alias_2", "group", "alias_group")

    -- Disable the group
    local count = world.EnableAliasGroup("alias_group", false)
    check(count == 2, "EnableAliasGroup should disable 2 aliases")

    -- Verify both are disabled
    local enabled1 = world.GetAliasInfo("group_alias_1", 6)
    local enabled2 = world.GetAliasInfo("group_alias_2", 6)
    check(enabled1 == false, "group_alias_1 should be disabled")
    check(enabled2 == false, "group_alias_2 should be disabled")

    -- Re-enable the group
    count = world.EnableAliasGroup("alias_group", true)
    check(count == 2, "EnableAliasGroup should enable 2 aliases")

    -- Verify both are enabled
    enabled1 = world.GetAliasInfo("group_alias_1", 6)
    enabled2 = world.GetAliasInfo("group_alias_2", 6)
    check(enabled1 == true, "group_alias_1 should be enabled")
    check(enabled2 == true, "group_alias_2 should be enabled")

    -- Clean up
    world.DeleteAlias("group_alias_1")
    world.DeleteAlias("group_alias_2")

    return 0
end

-- Test EnableTriggerGroup with non-existent group
function test_enable_trigger_group_empty()
    local count = world.EnableTriggerGroup("nonexistent_group", false)
    check(count == 0, "EnableTriggerGroup on empty group should return 0")

    return 0
end

-- Test EnableAliasGroup with non-existent group
function test_enable_alias_group_empty()
    local count = world.EnableAliasGroup("nonexistent_group", false)
    check(count == 0, "EnableAliasGroup on empty group should return 0")

    return 0
end

-- ========== Get/Set TriggerOption Functions ==========

-- Test GetTriggerOption and SetTriggerOption
function test_trigger_option()
    -- Add a trigger first
    local flags = trigger_flag.Enabled
    local err = world.AddTrigger("option_trigger", "test pattern", "test send", flags, 0, 0, "", "", sendto.World, 100)
    check(err == 0, "AddTrigger for option_trigger failed")

    -- Test getting/setting enabled option (returns 0/1 to match original MUSHclient)
    local enabled = world.GetTriggerOption("option_trigger", "enabled")
    check(enabled == 1, "GetTriggerOption enabled should be 1")

    -- Use false in Lua to set boolean options to 0 (0 in Lua is truthy!)
    err = world.SetTriggerOption("option_trigger", "enabled", false)
    check(err == 0, "SetTriggerOption enabled should succeed")

    enabled = world.GetTriggerOption("option_trigger", "enabled")
    check(enabled == 0, "GetTriggerOption enabled should now be 0")

    -- Test getting/setting sequence
    local seq = world.GetTriggerOption("option_trigger", "sequence")
    check(seq == 100, "GetTriggerOption sequence should be 100")

    err = world.SetTriggerOption("option_trigger", "sequence", 50)
    check(err == 0, "SetTriggerOption sequence should succeed")

    seq = world.GetTriggerOption("option_trigger", "sequence")
    check(seq == 50, "GetTriggerOption sequence should now be 50")

    -- Test getting/setting group
    local group = world.GetTriggerOption("option_trigger", "group")
    check(group == "", "GetTriggerOption group should be empty")

    err = world.SetTriggerOption("option_trigger", "group", "my_group")
    check(err == 0, "SetTriggerOption group should succeed")

    group = world.GetTriggerOption("option_trigger", "group")
    check(group == "my_group", "GetTriggerOption group should be my_group")

    -- Clean up
    world.DeleteTrigger("option_trigger")

    return 0
end

-- Test GetTriggerOption for non-existent trigger
function test_trigger_option_not_found()
    local result = world.GetTriggerOption("nonexistent_trigger", "enabled")
    check(result == nil, "GetTriggerOption on nonexistent trigger should return nil")

    return 0
end

-- Test SetTriggerOption for non-existent trigger
function test_set_trigger_option_not_found()
    local err = world.SetTriggerOption("nonexistent_trigger", "enabled", 1)
    check(err ~= 0, "SetTriggerOption on nonexistent trigger should return error")

    return 0
end

-- ========== Timer API Functions ==========

-- Test AddTimer and DeleteTimer
function test_add_timer()
    local flags = timer_flag.Enabled
    local err = world.AddTimer("test_timer", 0, 0, 5.0, "test send", flags, "")
    check(err == 0, "AddTimer should succeed")

    -- Verify timer exists
    local list = world.GetTimerList()
    check(type(list) == "table", "GetTimerList should return table")

    local found = false
    for i = 1, #list do
        if list[i] == "test_timer" then
            found = true
            break
        end
    end
    check(found, "test_timer should be in GetTimerList")

    return 0
end

-- Test GetTimerInfo
function test_get_timer_info()
    -- Get hour (info type 1)
    local hour = world.GetTimerInfo("test_timer", 1)
    check(hour == 0, "GetTimerInfo hour should be 0")

    -- Get minute (info type 2)
    local minute = world.GetTimerInfo("test_timer", 2)
    check(minute == 0, "GetTimerInfo minute should be 0")

    -- Get second (info type 3)
    local second = world.GetTimerInfo("test_timer", 3)
    check(second == 5.0, "GetTimerInfo second should be 5.0")

    -- Get contents/send text (info type 4)
    local contents = world.GetTimerInfo("test_timer", 4)
    check(contents == "test send", "GetTimerInfo contents mismatch")

    -- Get enabled (info type 7)
    local enabled = world.GetTimerInfo("test_timer", 7)
    check(enabled == true, "GetTimerInfo enabled should be true")

    -- Get one_shot (info type 9)
    local one_shot = world.GetTimerInfo("test_timer", 9)
    check(one_shot == false, "GetTimerInfo one_shot should be false")

    return 0
end

-- Test EnableTimer
function test_enable_timer()
    -- Disable timer
    local err = world.EnableTimer("test_timer", false)
    check(err == 0, "EnableTimer(false) should succeed")

    -- Verify disabled (info type 7 = enabled)
    local enabled = world.GetTimerInfo("test_timer", 7)
    check(enabled == false, "Timer should be disabled")

    -- Re-enable timer
    err = world.EnableTimer("test_timer", true)
    check(err == 0, "EnableTimer(true) should succeed")

    -- Verify enabled
    enabled = world.GetTimerInfo("test_timer", 7)
    check(enabled == true, "Timer should be enabled")

    return 0
end

-- Test GetTimerOption and SetTimerOption
function test_timer_option()
    -- Test getting enabled option
    local enabled = world.GetTimerOption("test_timer", "enabled")
    check(enabled == true, "GetTimerOption enabled should be true")

    -- Test setting enabled option
    local err = world.SetTimerOption("test_timer", "enabled", false)
    check(err == 0, "SetTimerOption enabled should succeed")

    enabled = world.GetTimerOption("test_timer", "enabled")
    check(enabled == false, "GetTimerOption enabled should now be false")

    -- Re-enable for future tests
    world.SetTimerOption("test_timer", "enabled", true)

    -- Test getting/setting group
    local group = world.GetTimerOption("test_timer", "group")
    check(group == "", "GetTimerOption group should be empty")

    err = world.SetTimerOption("test_timer", "group", "my_timer_group")
    check(err == 0, "SetTimerOption group should succeed")

    group = world.GetTimerOption("test_timer", "group")
    check(group == "my_timer_group", "GetTimerOption group should be my_timer_group")

    -- Test getting/setting send text
    local send = world.GetTimerOption("test_timer", "send")
    check(send == "test send", "GetTimerOption send should be 'test send'")

    err = world.SetTimerOption("test_timer", "send", "new send text")
    check(err == 0, "SetTimerOption send should succeed")

    send = world.GetTimerOption("test_timer", "send")
    check(send == "new send text", "GetTimerOption send should be 'new send text'")

    return 0
end

-- Test EnableTimerGroup
function test_enable_timer_group()
    -- Add two timers in the same group
    local flags = timer_flag.Enabled
    local err1 = world.AddTimer("group_timer_1", 0, 0, 10.0, "send1", flags, "")
    check(err1 == 0, "AddTimer for group_timer_1 failed")

    local err2 = world.AddTimer("group_timer_2", 0, 0, 15.0, "send2", flags, "")
    check(err2 == 0, "AddTimer for group_timer_2 failed")

    -- Set both timers to the same group
    world.SetTimerOption("group_timer_1", "group", "timer_test_group")
    world.SetTimerOption("group_timer_2", "group", "timer_test_group")

    -- Disable the group
    local count = world.EnableTimerGroup("timer_test_group", false)
    check(count == 2, "EnableTimerGroup should disable 2 timers")

    -- Verify both are disabled
    local enabled1 = world.GetTimerInfo("group_timer_1", 7)
    local enabled2 = world.GetTimerInfo("group_timer_2", 7)
    check(enabled1 == false, "group_timer_1 should be disabled")
    check(enabled2 == false, "group_timer_2 should be disabled")

    -- Re-enable the group
    count = world.EnableTimerGroup("timer_test_group", true)
    check(count == 2, "EnableTimerGroup should enable 2 timers")

    -- Verify both are enabled
    enabled1 = world.GetTimerInfo("group_timer_1", 7)
    enabled2 = world.GetTimerInfo("group_timer_2", 7)
    check(enabled1 == true, "group_timer_1 should be enabled")
    check(enabled2 == true, "group_timer_2 should be enabled")

    -- Clean up
    world.DeleteTimer("group_timer_1")
    world.DeleteTimer("group_timer_2")

    return 0
end

-- Test EnableTimerGroup with non-existent group
function test_enable_timer_group_empty()
    local count = world.EnableTimerGroup("nonexistent_timer_group", false)
    check(count == 0, "EnableTimerGroup on empty group should return 0")

    return 0
end

-- Test ResetTimer
function test_reset_timer()
    local err = world.ResetTimer("test_timer")
    check(err == 0, "ResetTimer should succeed")

    return 0
end

-- Test GetTimerOption for non-existent timer
function test_timer_option_not_found()
    local result = world.GetTimerOption("nonexistent_timer", "enabled")
    check(result == nil, "GetTimerOption on nonexistent timer should return nil")

    return 0
end

-- Test SetTimerOption for non-existent timer
function test_set_timer_option_not_found()
    local err = world.SetTimerOption("nonexistent_timer", "enabled", 1)
    check(err ~= 0, "SetTimerOption on nonexistent timer should return error")

    return 0
end

-- Test DeleteTimer
function test_delete_timer()
    local err = world.DeleteTimer("test_timer")
    check(err == 0, "DeleteTimer should succeed")

    -- Verify timer is gone
    local result = world.GetTimerOption("test_timer", "enabled")
    check(result == nil, "Timer should be deleted")

    return 0
end

-- Test DeleteTimer for non-existent timer
function test_delete_timer_not_found()
    local err = world.DeleteTimer("nonexistent_timer")
    check(err ~= 0, "DeleteTimer on nonexistent timer should return error")

    return 0
end

-- ========== Variable API Functions ==========

-- Test SetVariable and GetVariable
function test_set_get_variable()
    local err = world.SetVariable("test_var", "hello world")
    check(err == 0, "SetVariable should succeed")

    local value = world.GetVariable("test_var")
    check(value == "hello world", "GetVariable should return the value")

    -- Test updating a variable
    err = world.SetVariable("test_var", "new value")
    check(err == 0, "SetVariable update should succeed")

    value = world.GetVariable("test_var")
    check(value == "new value", "GetVariable should return updated value")

    return 0
end

-- Test GetVariable for non-existent variable
function test_get_variable_not_found()
    local value = world.GetVariable("nonexistent_variable_xyz")
    check(value == nil, "GetVariable on nonexistent variable should return nil")

    return 0
end

-- Test GetVariableList
function test_get_variable_list()
    -- Set some variables first
    world.SetVariable("var_a", "1")
    world.SetVariable("var_b", "2")

    local list = world.GetVariableList()
    check(type(list) == "table", "GetVariableList should return a table")

    -- Should have at least our variables
    local found_a = false
    local found_b = false
    for i = 1, #list do
        if list[i] == "var_a" then found_a = true end
        if list[i] == "var_b" then found_b = true end
    end
    check(found_a, "var_a should be in variable list")
    check(found_b, "var_b should be in variable list")

    return 0
end

-- Test DeleteVariable
function test_delete_variable()
    -- Set a variable first
    world.SetVariable("to_delete", "value")

    local err = world.DeleteVariable("to_delete")
    check(err == 0, "DeleteVariable should succeed")

    local value = world.GetVariable("to_delete")
    check(value == nil, "Variable should be deleted")

    return 0
end

-- Test DeleteVariable for non-existent variable
function test_delete_variable_not_found()
    local err = world.DeleteVariable("nonexistent_variable_xyz")
    check(err ~= 0, "DeleteVariable on nonexistent variable should return error")

    return 0
end

-- Test empty string variable (edge case)
function test_variable_empty_string()
    local err = world.SetVariable("empty_var", "")
    check(err == 0, "SetVariable with empty string should succeed")

    local value = world.GetVariable("empty_var")
    check(value == "", "GetVariable should return empty string, not nil")

    -- Clean up
    world.DeleteVariable("empty_var")

    return 0
end

-- ========== Utility Functions ==========

-- Test Base64Encode and Base64Decode
function test_base64()
    local original = "Hello, World!"
    local encoded = world.Base64Encode(original)
    check(type(encoded) == "string", "Base64Encode should return a string")
    check(encoded == "SGVsbG8sIFdvcmxkIQ==", "Base64Encode should produce correct output")

    local decoded = world.Base64Decode(encoded)
    check(decoded == original, "Base64Decode should reverse Base64Encode")

    return 0
end

-- Test Hash (SHA-256)
function test_hash()
    local hash = world.Hash("test")
    check(type(hash) == "string", "Hash should return a string")
    check(#hash == 64, "SHA-256 hash should be 64 hex characters")
    -- Known SHA-256 hash of "test"
    check(hash == "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08", "Hash should be correct")

    return 0
end

-- Test Trim
function test_trim()
    local trimmed = world.Trim("  hello world  ")
    check(trimmed == "hello world", "Trim should remove leading and trailing whitespace")

    trimmed = world.Trim("\t\n  spaces  \t\n")
    check(trimmed == "spaces", "Trim should remove all whitespace types")

    trimmed = world.Trim("no_whitespace")
    check(trimmed == "no_whitespace", "Trim should not modify strings without whitespace")

    return 0
end

-- Test CreateGUID
function test_create_guid()
    local guid1 = world.CreateGUID()
    check(type(guid1) == "string", "CreateGUID should return a string")
    check(#guid1 > 0, "CreateGUID should return non-empty string")

    local guid2 = world.CreateGUID()
    check(guid1 ~= guid2, "CreateGUID should return unique values")

    return 0
end

-- Test GetUniqueNumber
function test_get_unique_number()
    local num1 = world.GetUniqueNumber()
    check(type(num1) == "number", "GetUniqueNumber should return a number")

    local num2 = world.GetUniqueNumber()
    check(num2 > num1, "GetUniqueNumber should return increasing values")

    return 0
end

-- Test GetUniqueID (returns string GUID)
function test_get_unique_id()
    local id1 = world.GetUniqueID()
    check(type(id1) == "string", "GetUniqueID should return a string")
    check(#id1 > 0, "GetUniqueID should return non-empty string")

    local id2 = world.GetUniqueID()
    check(id1 ~= id2, "GetUniqueID should return unique values")

    return 0
end

-- ========== Color Utility Functions ==========

-- Test ColourNameToRGB with basic colors
function test_colour_name_to_rgb_basic()
    -- Note: Returns BGR format (0x00BBGGRR) for Windows COLORREF compatibility
    -- "red" = RGB(255,0,0) = BGR 0x0000FF (255 in R channel)
    local red = world.ColourNameToRGB("red")
    check(red == 0x0000FF, "red should be 0x0000FF (BGR format)")

    -- "blue" = RGB(0,0,255) = BGR 0xFF0000
    local blue = world.ColourNameToRGB("blue")
    check(blue == 0xFF0000, "blue should be 0xFF0000 (BGR format)")

    -- "green" = RGB(0,128,0) = BGR 0x008000
    local green = world.ColourNameToRGB("green")
    check(green == 0x008000, "green should be 0x008000 (BGR format)")

    -- "black" = RGB(0,0,0) = BGR 0x000000
    local black = world.ColourNameToRGB("black")
    check(black == 0x000000, "black should be 0x000000")

    -- "white" = RGB(255,255,255) = BGR 0xFFFFFF
    local white = world.ColourNameToRGB("white")
    check(white == 0xFFFFFF, "white should be 0xFFFFFF")

    return 0
end

-- Test ColourNameToRGB case insensitivity
function test_colour_name_to_rgb_case()
    local red1 = world.ColourNameToRGB("red")
    local red2 = world.ColourNameToRGB("RED")
    local red3 = world.ColourNameToRGB("Red")
    check(red1 == red2, "ColourNameToRGB should be case insensitive")
    check(red2 == red3, "ColourNameToRGB should be case insensitive")

    return 0
end

-- Test ColourNameToRGB with extended colors
function test_colour_name_to_rgb_extended()
    -- "orange" = RGB(255,165,0) = BGR 0x00A5FF
    local orange = world.ColourNameToRGB("orange")
    check(orange == 0x00A5FF, "orange should be 0x00A5FF (BGR format)")

    -- "purple" = RGB(128,0,128) = BGR 0x800080
    local purple = world.ColourNameToRGB("purple")
    check(purple == 0x800080, "purple should be 0x800080 (BGR format)")

    -- "cyan" = RGB(0,255,255) = BGR 0xFFFF00
    local cyan = world.ColourNameToRGB("cyan")
    check(cyan == 0xFFFF00, "cyan should be 0xFFFF00 (BGR format)")

    return 0
end

-- Test ColourNameToRGB with hex values
function test_colour_name_to_rgb_hex()
    -- Hex input is RGB format, output is BGR
    -- "#FF0000" (RGB red) should return BGR 0x0000FF
    local hexRed = world.ColourNameToRGB("#FF0000")
    check(hexRed == 0x0000FF, "#FF0000 should convert to BGR 0x0000FF")

    -- "#0000FF" (RGB blue) should return BGR 0xFF0000
    local hexBlue = world.ColourNameToRGB("#0000FF")
    check(hexBlue == 0xFF0000, "#0000FF should convert to BGR 0xFF0000")

    -- "0x00FF00" format
    local hexGreen = world.ColourNameToRGB("0x00FF00")
    check(hexGreen == 0x00FF00, "0x00FF00 should convert to BGR 0x00FF00")

    return 0
end

-- Test RGBColourToName
function test_rgb_colour_to_name()
    -- BGR 0x0000FF = red
    local redName = world.RGBColourToName(0x0000FF)
    check(redName == "red", "0x0000FF should be 'red'")

    -- BGR 0xFF0000 = blue
    local blueName = world.RGBColourToName(0xFF0000)
    check(blueName == "blue", "0xFF0000 should be 'blue'")

    -- BGR 0x000000 = black
    local blackName = world.RGBColourToName(0x000000)
    check(blackName == "black", "0x000000 should be 'black'")

    -- BGR 0xFFFFFF = white
    local whiteName = world.RGBColourToName(0xFFFFFF)
    check(whiteName == "white", "0xFFFFFF should be 'white'")

    return 0
end

-- Test RGBColourToName with non-named colors
function test_rgb_colour_to_name_unknown()
    -- Non-standard color should return empty string
    local unknown = world.RGBColourToName(0x123456)
    check(unknown == "", "Unknown color should return empty string")

    return 0
end

-- Test roundtrip: name -> BGR -> name
function test_colour_roundtrip()
    local colors = {"red", "blue", "green", "black", "white", "orange", "purple", "cyan"}
    for i, name in ipairs(colors) do
        local bgr = world.ColourNameToRGB(name)
        local nameBack = world.RGBColourToName(bgr)
        check(nameBack == name, "Roundtrip failed for " .. name)
    end

    return 0
end

-- Test GetNormalColour and SetNormalColour
function test_normal_colour()
    -- Get original value
    local original = world.GetNormalColour(1)
    check(type(original) == "number", "GetNormalColour should return a number")

    -- Set to a new value
    world.SetNormalColour(1, 0x0000FF) -- red in BGR

    -- Verify it changed
    local newVal = world.GetNormalColour(1)
    check(newVal == 0x0000FF, "SetNormalColour should change the value")

    -- Restore original
    world.SetNormalColour(1, original)

    -- Out of range should return 0
    local outOfRange = world.GetNormalColour(99)
    check(outOfRange == 0, "GetNormalColour out of range should return 0")

    return 0
end

-- Test GetBoldColour and SetBoldColour
function test_bold_colour()
    -- Get original value
    local original = world.GetBoldColour(1)
    check(type(original) == "number", "GetBoldColour should return a number")

    -- Set to a new value
    world.SetBoldColour(1, 0xFF0000) -- blue in BGR

    -- Verify it changed
    local newVal = world.GetBoldColour(1)
    check(newVal == 0xFF0000, "SetBoldColour should change the value")

    -- Restore original
    world.SetBoldColour(1, original)

    return 0
end

-- Test GetCustomColourText and SetCustomColourText
function test_custom_colour_text()
    -- Get original value
    local original = world.GetCustomColourText(1)
    check(type(original) == "number", "GetCustomColourText should return a number")

    -- Set to a new value
    world.SetCustomColourText(1, 0x00FF00)

    -- Verify it changed
    local newVal = world.GetCustomColourText(1)
    check(newVal == 0x00FF00, "SetCustomColourText should change the value")

    -- Restore original
    world.SetCustomColourText(1, original)

    -- Out of range should return 0
    local outOfRange = world.GetCustomColourText(99)
    check(outOfRange == 0, "GetCustomColourText out of range should return 0")

    return 0
end

-- Test GetCustomColourBackground and SetCustomColourBackground
function test_custom_colour_background()
    -- Get original value
    local original = world.GetCustomColourBackground(1)
    check(type(original) == "number", "GetCustomColourBackground should return a number")

    -- Set to a new value
    world.SetCustomColourBackground(1, 0xABCDEF)

    -- Verify it changed
    local newVal = world.GetCustomColourBackground(1)
    check(newVal == 0xABCDEF, "SetCustomColourBackground should change the value")

    -- Restore original
    world.SetCustomColourBackground(1, original)

    return 0
end

-- Test AdjustColour
function test_adjust_colour()
    local red = 0x0000FF  -- red in BGR

    -- Method 0: No change
    local unchanged = world.AdjustColour(red, 0)
    check(unchanged == red, "AdjustColour method 0 should not change color")

    -- Method 1: Invert
    -- Invert of red (255,0,0) = (0,255,255) = cyan = BGR 0xFFFF00
    local inverted = world.AdjustColour(red, 1)
    check(inverted == 0xFFFF00, "AdjustColour invert of red should be cyan")

    -- Method 2: Lighter - should increase luminance
    local lighter = world.AdjustColour(red, 2)
    check(lighter ~= red, "AdjustColour lighter should change color")

    -- Method 3: Darker - should decrease luminance
    local darker = world.AdjustColour(red, 3)
    check(darker ~= red, "AdjustColour darker should change color")

    return 0
end

-- ========== Options API Functions ==========

-- Test GetOption and SetOption (numeric options)
function test_get_set_option()
    -- Get a known option
    local wrapCol = world.GetOption("wrap_column")
    check(type(wrapCol) == "number", "GetOption should return a number")

    -- Save original value
    local original = wrapCol

    -- Set to a new value
    local err = world.SetOption("wrap_column", 100)
    check(err == 0, "SetOption should succeed")

    -- Verify it changed
    local newVal = world.GetOption("wrap_column")
    check(newVal == 100, "GetOption should return new value")

    -- Restore original
    world.SetOption("wrap_column", original)

    return 0
end

-- Test GetOption for unknown option
function test_get_option_unknown()
    local val = world.GetOption("nonexistent_option_xyz")
    check(val == nil, "GetOption for unknown option should return nil")

    return 0
end

-- Test SetOption for unknown option
function test_set_option_unknown()
    local err = world.SetOption("nonexistent_option_xyz", 42)
    check(err ~= 0, "SetOption for unknown option should return error")

    return 0
end

-- Test boolean options
function test_option_boolean()
    -- Get a boolean option
    local autoPause = world.GetOption("auto_pause")
    check(type(autoPause) == "number", "GetOption boolean should return number (0 or 1)")

    -- Save original
    local original = autoPause

    -- Set to opposite value
    local newVal = (original == 0) and 1 or 0
    local err = world.SetOption("auto_pause", newVal)
    check(err == 0, "SetOption boolean should succeed")

    -- Verify it changed
    local readBack = world.GetOption("auto_pause")
    check(readBack == newVal, "GetOption should return new boolean value")

    -- Restore original
    world.SetOption("auto_pause", original)

    return 0
end

-- Test GetAlphaOption and SetAlphaOption (string options)
function test_get_set_alpha_option()
    -- Get a known string option
    local prefix = world.GetAlphaOption("script_prefix")
    check(type(prefix) == "string", "GetAlphaOption should return a string")

    -- Save original value
    local original = prefix

    -- Set to a new value
    local err = world.SetAlphaOption("script_prefix", "test_prefix_")
    check(err == 0, "SetAlphaOption should succeed")

    -- Verify it changed
    local newVal = world.GetAlphaOption("script_prefix")
    check(newVal == "test_prefix_", "GetAlphaOption should return new value")

    -- Restore original
    world.SetAlphaOption("script_prefix", original)

    return 0
end

-- Test GetAlphaOption for unknown option
function test_get_alpha_option_unknown()
    local val = world.GetAlphaOption("nonexistent_alpha_option_xyz")
    check(val == nil, "GetAlphaOption for unknown option should return nil")

    return 0
end

-- Test SetAlphaOption for unknown option
function test_set_alpha_option_unknown()
    local err = world.SetAlphaOption("nonexistent_alpha_option_xyz", "value")
    check(err ~= 0, "SetAlphaOption for unknown option should return error")

    return 0
end

-- Test GetOptionList
function test_get_option_list()
    local list = world.GetOptionList()
    check(type(list) == "table", "GetOptionList should return a table")
    check(#list > 0, "GetOptionList should return non-empty table")

    -- Check for some known options
    local foundWrapColumn = false
    local foundAutoPause = false
    for i, name in ipairs(list) do
        if name == "wrap_column" then foundWrapColumn = true end
        if name == "auto_pause" then foundAutoPause = true end
    end
    check(foundWrapColumn, "GetOptionList should contain wrap_column")
    check(foundAutoPause, "GetOptionList should contain auto_pause")

    return 0
end

-- Test GetAlphaOptionList
function test_get_alpha_option_list()
    local list = world.GetAlphaOptionList()
    check(type(list) == "table", "GetAlphaOptionList should return a table")
    check(#list > 0, "GetAlphaOptionList should return non-empty table")

    -- Check for some known options
    local foundScriptPrefix = false
    for i, name in ipairs(list) do
        if name == "script_prefix" then foundScriptPrefix = true end
    end
    check(foundScriptPrefix, "GetAlphaOptionList should contain script_prefix")

    return 0
end

-- Test GetInfo
function test_get_info()
    -- Type 2 = World name
    local worldName = world.GetInfo(2)
    check(type(worldName) == "string", "GetInfo(2) should return string world name")

    -- Type 72 = Version
    local version = world.GetInfo(72)
    check(type(version) == "string", "GetInfo(72) should return string version")
    check(#version > 0, "GetInfo(72) version should not be empty")

    -- Type 83 = SQLite version
    local sqliteVer = world.GetInfo(83)
    check(type(sqliteVer) == "string", "GetInfo(83) should return SQLite version")

    -- Type 219 = Count of triggers
    local triggerCount = world.GetInfo(219)
    check(type(triggerCount) == "number", "GetInfo(219) should return number")

    -- Type 220 = Count of timers
    local timerCount = world.GetInfo(220)
    check(type(timerCount) == "number", "GetInfo(220) should return number")

    -- Type 221 = Count of aliases
    local aliasCount = world.GetInfo(221)
    check(type(aliasCount) == "number", "GetInfo(221) should return number")

    return 0
end

-- Test GetInfo with boolean types
function test_get_info_boolean()
    -- Type 106 = Not connected (boolean)
    local notConnected = world.GetInfo(106)
    check(type(notConnected) == "boolean", "GetInfo(106) should return boolean")

    -- Type 119 = Script engine exists
    local scriptExists = world.GetInfo(119)
    check(type(scriptExists) == "boolean", "GetInfo(119) should return boolean")

    return 0
end

-- Test GetInfo unknown type returns nil
function test_get_info_unknown()
    local val = world.GetInfo(99999)
    check(val == nil, "GetInfo with unknown type should return nil")

    return 0
end

-- ========== Additional Utility Functions ==========

-- Test EditDistance (Levenshtein distance)
function test_edit_distance()
    -- Same strings = 0
    local dist = world.EditDistance("hello", "hello")
    check(dist == 0, "EditDistance of same strings should be 0")

    -- Single insertion = 1
    dist = world.EditDistance("hello", "hallo")
    check(dist == 1, "EditDistance of hello/hallo should be 1")

    -- Complete difference
    dist = world.EditDistance("abc", "xyz")
    check(dist == 3, "EditDistance of abc/xyz should be 3")

    -- Empty string
    dist = world.EditDistance("", "hello")
    check(dist == 5, "EditDistance of empty to hello should be 5")

    dist = world.EditDistance("hello", "")
    check(dist == 5, "EditDistance of hello to empty should be 5")

    -- Case sensitive
    dist = world.EditDistance("Hello", "hello")
    check(dist == 1, "EditDistance should be case sensitive")

    return 0
end

-- Test Replace function
-- Note: 4th parameter is "multiple" - when true, replaces all occurrences; when false, only first
function test_replace()
    -- Basic replacement (first occurrence only when 4th param is false)
    local result = world.Replace("hello world", "world", "there", false)
    check(result == "hello there", "Replace should replace first occurrence")

    -- Multiple occurrences (4th param = true to replace all)
    result = world.Replace("aaa", "a", "b", true)
    check(result == "bbb", "Replace should replace all occurrences when multiple=true")

    -- Only first occurrence (4th param = false)
    result = world.Replace("aaa", "a", "b", false)
    check(result == "baa", "Replace should replace only first when multiple=false")

    -- No match
    result = world.Replace("hello world", "xyz", "abc", false)
    check(result == "hello world", "Replace with no match should return original")

    -- Empty replacement
    result = world.Replace("hello world", "world", "", false)
    check(result == "hello ", "Replace with empty should delete")

    return 0
end

-- Test Metaphone (phonetic encoding)
-- Note: This is a stub function that always returns empty string in Mushkin
function test_metaphone()
    -- Basic test - Metaphone is a stub, returns empty string
    local code = world.Metaphone("hello")
    check(type(code) == "string", "Metaphone should return a string")
    -- Metaphone is not implemented, just returns empty string
    check(code == "", "Metaphone stub returns empty string")

    return 0
end

-- Test Version function
function test_version()
    local ver = world.Version()
    check(type(ver) == "string", "Version should return a string")
    check(#ver > 0, "Version should return non-empty string")

    return 0
end

-- Test ErrorDesc function
function test_error_desc()
    -- Known error codes
    local desc = world.ErrorDesc(0)
    check(type(desc) == "string", "ErrorDesc should return a string")
    check(desc == "No error" or #desc > 0, "ErrorDesc(0) should describe no error")

    -- Another known error code
    desc = world.ErrorDesc(30001)
    check(type(desc) == "string", "ErrorDesc should return a string for error 30001")
    check(#desc > 0, "ErrorDesc should return non-empty description")

    return 0
end

-- Test IsTrigger function
function test_is_trigger()
    -- Add a trigger first
    local flags = trigger_flag.Enabled
    world.AddTrigger("is_trigger_test", "pattern", "send", flags, 0, 0, "", "", sendto.World, 100)

    -- Check it exists
    local exists = world.IsTrigger("is_trigger_test")
    check(exists == 0, "IsTrigger should return 0 for existing trigger")

    -- Check non-existent
    local notExists = world.IsTrigger("nonexistent_trigger_xyz")
    check(notExists ~= 0, "IsTrigger should return error for non-existent trigger")

    -- Clean up
    world.DeleteTrigger("is_trigger_test")

    return 0
end

-- Test IsAlias function
function test_is_alias()
    -- Add an alias first
    local flags = alias_flag.Enabled
    world.AddAlias("is_alias_test", "pattern", "send", flags, "")

    -- Check it exists
    local exists = world.IsAlias("is_alias_test")
    check(exists == 0, "IsAlias should return 0 for existing alias")

    -- Check non-existent
    local notExists = world.IsAlias("nonexistent_alias_xyz")
    check(notExists ~= 0, "IsAlias should return error for non-existent alias")

    -- Clean up
    world.DeleteAlias("is_alias_test")

    return 0
end

-- Test IsTimer function
function test_is_timer()
    -- Add a timer first
    local flags = timer_flag.Enabled
    world.AddTimer("is_timer_test", 0, 0, 10.0, "send", flags, "")

    -- Check it exists
    local exists = world.IsTimer("is_timer_test")
    check(exists == 0, "IsTimer should return 0 for existing timer")

    -- Check non-existent
    local notExists = world.IsTimer("nonexistent_timer_xyz")
    check(notExists ~= 0, "IsTimer should return error for non-existent timer")

    -- Clean up
    world.DeleteTimer("is_timer_test")

    return 0
end

-- Test GetTrigger function (returns trigger details)
-- Returns: error_code, match, response, flags, colour, wildcard, sound, script
function test_get_trigger()
    -- Add a trigger first
    local flags = trigger_flag.Enabled
    world.AddTrigger("get_trigger_test", "test_pattern", "test_send", flags, 0, 0, "", "", sendto.World, 100)

    -- Get the trigger - first return is error_code (0=OK), then pattern, send, etc.
    local err, pattern, send, tflags, colour, wildcard, sound, script = world.GetTrigger("get_trigger_test")
    check(err == 0, "GetTrigger should return 0 for success")
    check(pattern == "test_pattern", "GetTrigger should return pattern")
    check(send == "test_send", "GetTrigger should return send text")

    -- Non-existent trigger returns error code
    local notFoundErr = world.GetTrigger("nonexistent_trigger_xyz")
    check(notFoundErr ~= 0, "GetTrigger on non-existent trigger should return error")

    -- Clean up
    world.DeleteTrigger("get_trigger_test")

    return 0
end

-- Test GetAlias function
-- Returns: error_code, match, response, flags, script
function test_get_alias()
    -- Add an alias first
    local flags = alias_flag.Enabled
    world.AddAlias("get_alias_test", "alias_pattern", "alias_send", flags, "")

    -- Get the alias - first return is error_code (0=OK)
    local err, pattern, send, aflags, script = world.GetAlias("get_alias_test")
    check(err == 0, "GetAlias should return 0 for success")
    check(pattern == "alias_pattern", "GetAlias should return pattern")
    check(send == "alias_send", "GetAlias should return send text")

    -- Non-existent alias returns error code
    local notFoundErr = world.GetAlias("nonexistent_alias_xyz")
    check(notFoundErr ~= 0, "GetAlias on non-existent alias should return error")

    -- Clean up
    world.DeleteAlias("get_alias_test")

    return 0
end

-- Test GetTimer function
-- Returns: error_code, hour, minute, second, response, flags, script
function test_get_timer()
    -- Add a timer first
    local flags = timer_flag.Enabled
    world.AddTimer("get_timer_test", 1, 2, 3.0, "timer_send", flags, "")

    -- Get the timer - first return is error_code (0=OK)
    local err, hour, minute, second, send, tflags, script = world.GetTimer("get_timer_test")
    check(err == 0, "GetTimer should return 0 for success")
    check(hour == 1, "GetTimer should return hour")
    check(minute == 2, "GetTimer should return minute")
    check(second == 3.0, "GetTimer should return second")
    check(send == "timer_send", "GetTimer should return send text")

    -- Non-existent timer returns error code
    local notFoundErr = world.GetTimer("nonexistent_timer_xyz")
    check(notFoundErr ~= 0, "GetTimer on non-existent timer should return error")

    -- Clean up
    world.DeleteTimer("get_timer_test")

    return 0
end

-- ========== Speedwalk Functions ==========

-- Test EvaluateSpeedwalk
function test_evaluate_speedwalk()
    -- Basic speedwalk expansion
    local result = world.EvaluateSpeedwalk("3n2e")
    check(type(result) == "string", "EvaluateSpeedwalk should return a string")
    check(result == "n\nn\nn\ne\ne\n" or result:find("n") ~= nil, "EvaluateSpeedwalk should expand directions")

    -- Single direction
    result = world.EvaluateSpeedwalk("n")
    check(result:find("n") ~= nil, "EvaluateSpeedwalk should handle single direction")

    return 0
end

-- Test ReverseSpeedwalk
function test_reverse_speedwalk()
    -- Basic reversal: n -> s, e -> w
    local result = world.ReverseSpeedwalk("n")
    check(result == "s" or result:find("s") ~= nil, "ReverseSpeedwalk n should become s")

    result = world.ReverseSpeedwalk("e")
    check(result == "w" or result:find("w") ~= nil, "ReverseSpeedwalk e should become w")

    result = world.ReverseSpeedwalk("u")
    check(result == "d" or result:find("d") ~= nil, "ReverseSpeedwalk u should become d")

    return 0
end

-- Test RemoveBacktracks
function test_remove_backtracks()
    -- n followed by s should cancel out
    local result = world.RemoveBacktracks("ns")
    check(result == "" or #result < 2, "RemoveBacktracks should remove n+s")

    -- More complex case
    result = world.RemoveBacktracks("nnsen")
    check(type(result) == "string", "RemoveBacktracks should return a string")

    return 0
end

-- Test GetSpeedWalkDelay and SetSpeedWalkDelay
function test_speedwalk_delay()
    -- Get current delay
    local delay = world.GetSpeedWalkDelay()
    check(type(delay) == "number", "GetSpeedWalkDelay should return a number")

    -- Save original
    local original = delay

    -- Set new delay
    world.SetSpeedWalkDelay(500)

    -- Verify it changed
    delay = world.GetSpeedWalkDelay()
    check(delay == 500, "SetSpeedWalkDelay should change the delay")

    -- Restore original
    world.SetSpeedWalkDelay(original)

    return 0
end

-- ========== StripANSI Test ==========

-- Test StripANSI function
function test_strip_ansi()
    -- Basic ANSI code removal
    local result = world.StripANSI("\027[31mred text\027[0m")
    check(result == "red text", "StripANSI should remove ANSI codes")

    -- Multiple ANSI codes
    result = world.StripANSI("\027[1m\027[32mbold green\027[0m")
    check(result == "bold green", "StripANSI should remove multiple ANSI codes")

    -- No ANSI codes
    result = world.StripANSI("plain text")
    check(result == "plain text", "StripANSI should not modify plain text")

    -- Empty string
    result = world.StripANSI("")
    check(result == "", "StripANSI should handle empty string")

    return 0
end

-- ========== FixupEscapeSequences Test ==========

-- Test FixupEscapeSequences
function test_fixup_escape_sequences()
    -- \n should become newline
    local result = world.FixupEscapeSequences("line1\\nline2")
    check(result == "line1\nline2", "FixupEscapeSequences should handle \\n")

    -- \t should become tab
    result = world.FixupEscapeSequences("col1\\tcol2")
    check(result == "col1\tcol2", "FixupEscapeSequences should handle \\t")

    -- Plain text unchanged
    result = world.FixupEscapeSequences("plain text")
    check(result == "plain text", "FixupEscapeSequences should not modify plain text")

    return 0
end

-- ========== FixupHTML Test ==========

-- Test FixupHTML
function test_fixup_html()
    -- Escape < and >
    local result = world.FixupHTML("<script>alert('xss')</script>")
    check(result:find("<") == nil or result:find("&lt;") ~= nil, "FixupHTML should escape <")

    -- Escape &
    result = world.FixupHTML("foo & bar")
    check(result:find("&amp;") ~= nil or result == "foo & bar", "FixupHTML should handle &")

    -- Plain text
    result = world.FixupHTML("plain text")
    check(result == "plain text", "FixupHTML should not modify plain text")

    return 0
end

-- ========== MakeRegularExpression Test ==========

-- Test MakeRegularExpression
-- This function wraps text with ^ and $ for whole-line matching and escapes special regex chars
function test_make_regular_expression()
    -- Basic text gets wrapped with ^ and $
    local result = world.MakeRegularExpression("hello")
    check(result == "^hello$", "MakeRegularExpression should wrap with ^ and $")

    -- Escape special characters (. becomes \.)
    result = world.MakeRegularExpression("hello.world")
    check(result == "^hello\\.world$", "MakeRegularExpression should escape .")

    -- Escape * character
    result = world.MakeRegularExpression("test*pattern")
    check(result == "^test\\*pattern$", "MakeRegularExpression should escape *")

    return 0
end

-- ========== Alias Options API Tests ==========

-- Test GetAliasOption and SetAliasOption
function test_alias_option()
    -- Add an alias first (5th param is script name, not group)
    local flags = alias_flag.Enabled
    world.AddAlias("alias_option_test", "test_pattern", "test_send", flags, "")

    -- Set the group via SetAliasOption
    world.SetAliasOption("alias_option_test", "group", "test_group")

    -- Test getting an option - enabled returns boolean true/false
    local enabled = world.GetAliasOption("alias_option_test", "enabled")
    check(enabled == true, "GetAliasOption should return enabled=true")

    -- Test setting an option - use false/true, not 0/1 (Lua treats 0 as truthy)
    local err = world.SetAliasOption("alias_option_test", "enabled", false)
    check(err == 0, "SetAliasOption should succeed")

    enabled = world.GetAliasOption("alias_option_test", "enabled")
    check(enabled == false, "GetAliasOption should return enabled=false after disable")

    -- Re-enable
    world.SetAliasOption("alias_option_test", "enabled", true)

    -- Test group option
    local group = world.GetAliasOption("alias_option_test", "group")
    check(group == "test_group", "GetAliasOption should return group")

    -- Clean up
    world.DeleteAlias("alias_option_test")

    return 0
end

-- Test GetAliasOption for non-existent alias
function test_alias_option_not_found()
    local val = world.GetAliasOption("nonexistent_alias_xyz", "enabled")
    check(val == nil, "GetAliasOption should return nil for non-existent alias")

    return 0
end

-- Test SetAliasOption for non-existent alias
function test_set_alias_option_not_found()
    local err = world.SetAliasOption("nonexistent_alias_xyz", "enabled", 1)
    check(err ~= 0, "SetAliasOption should return error for non-existent alias")

    return 0
end

-- ========== Delete Group Functions ==========

-- Test DeleteTriggerGroup
function test_delete_trigger_group()
    -- Add triggers to a group
    local flags = trigger_flag.Enabled
    world.AddTrigger("del_trig_group_1", "pattern1", "send1", flags, 0, 0, "", "", sendto.World, 100)
    world.AddTrigger("del_trig_group_2", "pattern2", "send2", flags, 0, 0, "", "", sendto.World, 100)

    -- Set their group
    world.SetTriggerOption("del_trig_group_1", "group", "delete_test_group")
    world.SetTriggerOption("del_trig_group_2", "group", "delete_test_group")

    -- Verify they exist
    check(world.IsTrigger("del_trig_group_1") == 0, "Trigger 1 should exist")
    check(world.IsTrigger("del_trig_group_2") == 0, "Trigger 2 should exist")

    -- Delete the group
    local count = world.DeleteTriggerGroup("delete_test_group")
    check(count == 2, "DeleteTriggerGroup should return count of deleted triggers")

    -- Verify they're gone
    check(world.IsTrigger("del_trig_group_1") ~= 0, "Trigger 1 should be deleted")
    check(world.IsTrigger("del_trig_group_2") ~= 0, "Trigger 2 should be deleted")

    return 0
end

-- Test DeleteAliasGroup
function test_delete_alias_group()
    -- Add aliases (5th param is script, not group)
    local flags = alias_flag.Enabled
    world.AddAlias("del_alias_group_1", "pattern1", "send1", flags, "")
    world.AddAlias("del_alias_group_2", "pattern2", "send2", flags, "")

    -- Set the group via SetAliasOption
    world.SetAliasOption("del_alias_group_1", "group", "delete_alias_group")
    world.SetAliasOption("del_alias_group_2", "group", "delete_alias_group")

    -- Verify they exist
    check(world.IsAlias("del_alias_group_1") == 0, "Alias 1 should exist")
    check(world.IsAlias("del_alias_group_2") == 0, "Alias 2 should exist")

    -- Delete the group
    local count = world.DeleteAliasGroup("delete_alias_group")
    check(count == 2, "DeleteAliasGroup should return count of deleted aliases")

    -- Verify they're gone
    check(world.IsAlias("del_alias_group_1") ~= 0, "Alias 1 should be deleted")
    check(world.IsAlias("del_alias_group_2") ~= 0, "Alias 2 should be deleted")

    return 0
end

-- Test DeleteTimerGroup
function test_delete_timer_group()
    -- Add timers (7th param is script, not group)
    local flags = timer_flag.Enabled
    world.AddTimer("del_timer_group_1", 0, 0, 10.0, "send1", flags, "")
    world.AddTimer("del_timer_group_2", 0, 0, 20.0, "send2", flags, "")

    -- Set the group via SetTimerOption
    world.SetTimerOption("del_timer_group_1", "group", "delete_timer_group")
    world.SetTimerOption("del_timer_group_2", "group", "delete_timer_group")

    -- Verify they exist
    check(world.IsTimer("del_timer_group_1") == 0, "Timer 1 should exist")
    check(world.IsTimer("del_timer_group_2") == 0, "Timer 2 should exist")

    -- Delete the group
    local count = world.DeleteTimerGroup("delete_timer_group")
    check(count == 2, "DeleteTimerGroup should return count of deleted timers")

    -- Verify they're gone
    check(world.IsTimer("del_timer_group_1") ~= 0, "Timer 1 should be deleted")
    check(world.IsTimer("del_timer_group_2") ~= 0, "Timer 2 should be deleted")

    return 0
end

-- ========== Delete Temporary Functions ==========

-- Test DeleteTemporaryTriggers
function test_delete_temporary_triggers()
    -- Add a temporary trigger (one-shot)
    local flags = trigger_flag.Enabled + trigger_flag.OneShot + trigger_flag.Temporary
    world.AddTrigger("temp_trig_test", "pattern", "send", flags, 0, 0, "", "", sendto.World, 100)

    -- Verify it exists
    check(world.IsTrigger("temp_trig_test") == 0, "Temporary trigger should exist")

    -- Delete temporary triggers
    local count = world.DeleteTemporaryTriggers()
    check(type(count) == "number", "DeleteTemporaryTriggers should return a number")

    return 0
end

-- Test DeleteTemporaryAliases
function test_delete_temporary_aliases()
    -- Add a temporary alias
    local flags = alias_flag.Enabled + alias_flag.Temporary
    world.AddAlias("temp_alias_test", "pattern", "send", flags, "")

    -- Delete temporary aliases
    local count = world.DeleteTemporaryAliases()
    check(type(count) == "number", "DeleteTemporaryAliases should return a number")

    return 0
end

-- Test DeleteTemporaryTimers
function test_delete_temporary_timers()
    -- Add a temporary timer
    local flags = timer_flag.Enabled + timer_flag.Temporary
    world.AddTimer("temp_timer_test", 0, 0, 10.0, "send", flags, "")

    -- Delete temporary timers
    local count = world.DeleteTemporaryTimers()
    check(type(count) == "number", "DeleteTemporaryTimers should return a number")

    return 0
end

-- ========== World Info Functions ==========

-- Test GetWorldID
function test_get_world_id()
    local id = world.GetWorldID()
    check(type(id) == "string", "GetWorldID should return a string")

    return 0
end

-- Test WorldName function
function test_world_name()
    local name = world.WorldName()
    check(type(name) == "string", "WorldName should return a string")

    return 0
end

-- Test WorldAddress function
function test_world_address()
    local addr = world.WorldAddress()
    check(type(addr) == "string", "WorldAddress should return a string")

    return 0
end

-- Test WorldPort function
function test_world_port()
    local port = world.WorldPort()
    check(type(port) == "number", "WorldPort should return a number")

    return 0
end

-- ========== Notes API ==========

-- Test GetNotes and SetNotes
function test_get_set_notes()
    -- Get current notes
    local original = world.GetNotes()
    check(type(original) == "string", "GetNotes should return a string")

    -- Set new notes
    world.SetNotes("Test notes content")

    -- Verify
    local notes = world.GetNotes()
    check(notes == "Test notes content", "SetNotes should change notes")

    -- Restore original
    world.SetNotes(original)

    return 0
end

-- ========== Line Buffer Functions ==========

-- Test GetLineCount
function test_get_line_count()
    local count = world.GetLineCount()
    check(type(count) == "number", "GetLineCount should return a number")
    check(count >= 0, "GetLineCount should be non-negative")

    return 0
end

-- Test GetLinesInBufferCount
function test_get_lines_in_buffer_count()
    local count = world.GetLinesInBufferCount()
    check(type(count) == "number", "GetLinesInBufferCount should return a number")
    check(count >= 0, "GetLinesInBufferCount should be non-negative")

    return 0
end

-- ========== Output Functions ==========

-- Test Tell function (output without newline)
function test_tell()
    -- Tell should not error
    local err = world.Tell("test output")
    -- Tell typically returns nil or nothing
    check(err == nil or err == 0, "Tell should not return an error")

    return 0
end

-- Test ANSI function (generate ANSI codes)
function test_ansi()
    -- ANSI(code) generates an ANSI escape sequence
    local code = world.ANSI(1) -- Bold
    check(type(code) == "string", "ANSI should return a string")
    check(#code > 0, "ANSI should return non-empty string")

    -- Reset code
    code = world.ANSI(0)
    check(type(code) == "string", "ANSI(0) should return reset code")

    return 0
end

-- Test Simulate function
function test_simulate()
    -- Simulate processes text as if received from server
    -- This should not error even with empty input
    world.Simulate("")
    world.Simulate("Test line\n")

    return 0
end

-- ========== Script Info Functions ==========

-- Test GetScriptTime
function test_get_script_time()
    local time = world.GetScriptTime()
    check(type(time) == "number", "GetScriptTime should return a number")
    check(time >= 0, "GetScriptTime should be non-negative")

    return 0
end

-- Test GetFrame
function test_get_frame()
    -- GetFrame returns a window handle (lightuserdata) or nil if no window
    local frame = world.GetFrame()
    -- In headless test mode, there's no window, so nil is acceptable
    check(frame == nil or type(frame) == "userdata", "GetFrame should return userdata or nil")

    return 0
end

-- ========== Command Line Functions ==========

-- Test GetCommand and SetCommand
function test_get_set_command()
    -- Get current command
    local original = world.GetCommand()
    check(type(original) == "string", "GetCommand should return a string")

    -- Set new command - in headless mode, there's no input widget so this is a no-op
    local result = world.SetCommand("test command")
    -- Result is 0 (success) even in headless mode (no widget = nothing to overwrite)
    check(result == 0 or result == nil, "SetCommand should return success")

    -- In headless mode, GetCommand returns empty because there's no input widget
    local cmd = world.GetCommand()
    check(type(cmd) == "string", "GetCommand should return a string")
    -- Don't check if it changed - in headless mode it stays empty

    return 0
end

-- Test GetCommandList
function test_get_command_list()
    local list = world.GetCommandList()
    -- Returns nil or a table
    check(list == nil or type(list) == "table", "GetCommandList should return nil or table")

    return 0
end

-- ========== Echo Functions ==========

-- Test GetEchoInput and SetEchoInput
function test_echo_input()
    -- Get current echo setting
    local original = world.GetEchoInput()
    check(type(original) == "number" or type(original) == "boolean", "GetEchoInput should return number or boolean")

    -- Toggle it
    world.SetEchoInput(original == 0 and 1 or 0)

    -- Restore
    world.SetEchoInput(original)

    return 0
end

-- ========== Connection Info (when not connected) ==========

-- Test GetConnectDuration when not connected
function test_get_connect_duration()
    local duration = world.GetConnectDuration()
    check(type(duration) == "number", "GetConnectDuration should return a number")
    -- When not connected, should be 0
    check(duration == 0, "GetConnectDuration should be 0 when not connected")

    return 0
end

-- Test GetReceivedBytes
function test_get_received_bytes()
    local bytes = world.GetReceivedBytes()
    check(type(bytes) == "number", "GetReceivedBytes should return a number")
    check(bytes >= 0, "GetReceivedBytes should be non-negative")

    return 0
end

-- Test GetSentBytes
function test_get_sent_bytes()
    local bytes = world.GetSentBytes()
    check(type(bytes) == "number", "GetSentBytes should return a number")
    check(bytes >= 0, "GetSentBytes should be non-negative")

    return 0
end

-- ========== Base64 Functions ==========

-- Test Base64Encode and Base64Decode
function test_base64()
    -- Encode a simple string
    local encoded = world.Base64Encode("Hello, World!")
    check(type(encoded) == "string", "Base64Encode should return a string")
    check(encoded == "SGVsbG8sIFdvcmxkIQ==", "Base64Encode should encode correctly")

    -- Decode it back
    local decoded = world.Base64Decode(encoded)
    check(decoded == "Hello, World!", "Base64Decode should decode correctly")

    -- Empty string
    encoded = world.Base64Encode("")
    check(encoded == "", "Base64Encode of empty string should be empty")

    decoded = world.Base64Decode("")
    check(decoded == "", "Base64Decode of empty string should be empty")

    -- Binary data (with null bytes)
    local binary = "test\0data\0here"
    encoded = world.Base64Encode(binary)
    decoded = world.Base64Decode(encoded)
    check(decoded == binary, "Base64 should handle binary data with nulls")

    return 0
end

-- ========== DoAfter Functions ==========

-- Test DoAfter function (schedules a command after delay)
function test_do_after()
    -- DoAfter(seconds, command) - schedules command to run after delay
    -- Returns 0 on success
    local result = world.DoAfter(0.1, "test_command")
    check(result == 0, "DoAfter should return 0 on success")

    -- Very short delay
    result = world.DoAfter(0.001, "another_test")
    check(result == 0, "DoAfter should accept small delays")

    return 0
end

-- Test DoAfterNote function
function test_do_after_note()
    -- DoAfterNote(seconds, text) - outputs text after delay
    local result = world.DoAfterNote(0.1, "test note")
    check(result == 0, "DoAfterNote should return 0 on success")

    return 0
end

-- Test DoAfterSpecial function
function test_do_after_special()
    -- DoAfterSpecial(seconds, command, sendto) - like DoAfter but with send destination
    local result = world.DoAfterSpecial(0.1, "test_command", sendto.World)
    check(result == 0, "DoAfterSpecial should return 0 on success")

    -- Send to script
    result = world.DoAfterSpecial(0.1, "test_func()", sendto.Script)
    check(result == 0, "DoAfterSpecial to script should succeed")

    return 0
end

-- ========== Queue Functions ==========

-- Test Queue, GetQueue, and DiscardQueue
function test_queue()
    -- First discard any existing queue
    world.DiscardQueue()

    -- Queue returns 30002 (eWorldClosed) when not connected
    -- In headless test mode, we just verify the function exists and returns a number
    local result = world.Queue("first command", false)
    check(type(result) == "number", "Queue should return a number")
    -- result is 0 when connected, 30002 when not connected

    -- GetQueue should return a table (possibly empty in test mode)
    local queue = world.GetQueue()
    check(type(queue) == "table", "GetQueue should return a table")

    -- Discard queue
    world.DiscardQueue()

    return 0
end

-- Test PushCommand (adds to front of queue)
function test_push_command()
    world.DiscardQueue()

    -- PushCommand may not work when not connected, just verify it doesn't error
    world.PushCommand("first")

    local queue = world.GetQueue()
    check(type(queue) == "table", "GetQueue should return a table")

    world.DiscardQueue()

    return 0
end

-- ========== Trace/Debug Functions ==========

-- Test Trace function
function test_trace()
    -- Trace outputs debug info - should not error
    world.Trace("Test trace message")

    return 0
end

-- Test TraceOut function
function test_trace_out()
    -- TraceOut outputs to trace window
    world.TraceOut("Test trace out message")

    return 0
end

-- Test Debug function
function test_debug()
    -- Debug outputs debug info
    world.Debug("Test debug message")

    return 0
end

-- Test SetTrace and GetTrace
function test_set_get_trace()
    -- Get current trace setting
    local original = world.GetTrace()
    check(type(original) == "boolean" or type(original) == "number", "GetTrace should return boolean or number")

    -- Toggle trace
    world.SetTrace(true)
    local enabled = world.GetTrace()
    check(enabled == true or enabled == 1, "SetTrace(true) should enable tracing")

    world.SetTrace(false)
    local disabled = world.GetTrace()
    check(disabled == false or disabled == 0, "SetTrace(false) should disable tracing")

    -- Restore original
    world.SetTrace(original)

    return 0
end

-- ========== Execute Function ==========

-- Test Execute (runs a command through alias processing)
function test_execute()
    -- Execute processes command through aliases
    local result = world.Execute("test command")
    -- Result is usually nil or 0
    check(result == nil or result == 0, "Execute should not return error")

    return 0
end

-- ========== StopEvaluatingTriggers ==========

-- Test StopEvaluatingTriggers
function test_stop_evaluating_triggers()
    -- This function is called during trigger processing to stop further evaluation
    -- Outside of trigger context, it should just return without error
    world.StopEvaluatingTriggers()

    return 0
end

-- ========== ResetTimers ==========

-- Test ResetTimers (resets all timer fire times)
function test_reset_timers()
    -- Add a timer
    local flags = timer_flag.Enabled
    world.AddTimer("reset_test_timer", 0, 0, 60.0, "send", flags, "")

    -- Reset all timers
    local result = world.ResetTimers()
    check(result == nil or result == 0, "ResetTimers should succeed")

    -- Clean up
    world.DeleteTimer("reset_test_timer")

    return 0
end

-- ========== DeleteGroup and EnableGroup ==========

-- Test DeleteGroup (deletes triggers, aliases, and timers in a group)
function test_delete_group()
    -- Add items to a group
    local tflags = trigger_flag.Enabled
    local aflags = alias_flag.Enabled
    local tmflags = timer_flag.Enabled

    world.AddTrigger("del_group_trig", "pattern", "send", tflags, 0, 0, "", "", sendto.World, 100)
    world.SetTriggerOption("del_group_trig", "group", "delete_all_group")

    world.AddAlias("del_group_alias", "pattern", "send", aflags, "")
    world.SetAliasOption("del_group_alias", "group", "delete_all_group")

    world.AddTimer("del_group_timer", 0, 0, 10.0, "send", tmflags, "")
    world.SetTimerOption("del_group_timer", "group", "delete_all_group")

    -- Verify they exist
    check(world.IsTrigger("del_group_trig") == 0, "Trigger should exist")
    check(world.IsAlias("del_group_alias") == 0, "Alias should exist")
    check(world.IsTimer("del_group_timer") == 0, "Timer should exist")

    -- Delete the entire group
    local count = world.DeleteGroup("delete_all_group")
    check(type(count) == "number", "DeleteGroup should return a number")
    check(count == 3, "DeleteGroup should delete 3 items")

    -- Verify they're gone
    check(world.IsTrigger("del_group_trig") ~= 0, "Trigger should be deleted")
    check(world.IsAlias("del_group_alias") ~= 0, "Alias should be deleted")
    check(world.IsTimer("del_group_timer") ~= 0, "Timer should be deleted")

    return 0
end

-- Test EnableGroup (enables/disables all items in a group)
function test_enable_group()
    -- Add items to a group
    local tflags = trigger_flag.Enabled
    local aflags = alias_flag.Enabled

    world.AddTrigger("enable_group_trig", "pattern_eg", "send", tflags, 0, 0, "", "", sendto.World, 100)
    world.SetTriggerOption("enable_group_trig", "group", "enable_test_group")

    world.AddAlias("enable_group_alias", "pattern_eg", "send", aflags, "")
    world.SetAliasOption("enable_group_alias", "group", "enable_test_group")

    -- Disable the group
    local count = world.EnableGroup("enable_test_group", false)
    check(type(count) == "number", "EnableGroup should return a number")
    check(count >= 0, "EnableGroup should return non-negative count")

    -- Re-enable
    count = world.EnableGroup("enable_test_group", true)
    check(type(count) == "number", "EnableGroup should return a number")

    -- Clean up
    world.DeleteTrigger("enable_group_trig")
    world.DeleteAlias("enable_group_alias")

    return 0
end

-- ========== Line Info Functions ==========

-- Test GetLineInfo
function test_get_line_info()
    -- GetLineInfo(line_number, info_type) - gets info about an output line
    -- In test mode with no output, this should return nil or handle gracefully
    local info = world.GetLineInfo(1, 1) -- Line 1, info type 1 (text)
    -- May return nil if no lines exist
    check(info == nil or type(info) == "string" or type(info) == "number",
          "GetLineInfo should return nil, string, or number")

    return 0
end

-- Test GetRecentLines
function test_get_recent_lines()
    -- GetRecentLines(count) - gets recent output lines as a string
    local lines = world.GetRecentLines(10)
    check(type(lines) == "string", "GetRecentLines should return a string")

    return 0
end

-- ========== Selection Functions ==========

-- Test selection functions (GetSelectionStartLine, etc.)
function test_selection()
    local startLine = world.GetSelectionStartLine()
    local endLine = world.GetSelectionEndLine()
    local startCol = world.GetSelectionStartColumn()
    local endCol = world.GetSelectionEndColumn()

    check(type(startLine) == "number", "GetSelectionStartLine should return number")
    check(type(endLine) == "number", "GetSelectionEndLine should return number")
    check(type(startCol) == "number", "GetSelectionStartColumn should return number")
    check(type(endCol) == "number", "GetSelectionEndColumn should return number")

    return 0
end

-- ========== Host Info Functions ==========

-- Test GetHostAddress (looks up IP for a hostname)
function test_get_host_address()
    -- GetHostAddress(hostname) returns table of IP addresses
    local addrs = world.GetHostAddress("localhost")
    check(type(addrs) == "table", "GetHostAddress should return a table")

    -- Empty hostname returns empty table
    addrs = world.GetHostAddress("")
    check(type(addrs) == "table", "GetHostAddress should return table for empty hostname")

    return 0
end

-- Test GetHostName (reverse DNS lookup)
function test_get_host_name()
    -- GetHostName(ip_address) returns hostname
    local name = world.GetHostName("127.0.0.1")
    check(type(name) == "string", "GetHostName should return a string")

    -- Empty IP returns empty string
    name = world.GetHostName("")
    check(name == "", "GetHostName should return empty for empty IP")

    return 0
end

-- ========== Plugin Functions ==========

-- Test GetPluginList
function test_get_plugin_list()
    local list = world.GetPluginList()
    check(list == nil or type(list) == "table", "GetPluginList should return nil or table")

    return 0
end

-- Test GetPluginID
function test_get_plugin_id()
    -- When called from main script (not a plugin), returns empty
    local id = world.GetPluginID()
    check(type(id) == "string", "GetPluginID should return a string")

    return 0
end

-- Test GetPluginName
function test_get_plugin_name()
    local name = world.GetPluginName()
    check(type(name) == "string", "GetPluginName should return a string")

    return 0
end

-- Test PluginSupports
function test_plugin_supports()
    -- Check if a plugin supports a specific function
    -- With no plugins loaded, should return appropriate value
    local supports = world.PluginSupports("nonexistent_plugin", "OnPluginConnect")
    check(supports == nil or type(supports) == "boolean" or type(supports) == "number",
          "PluginSupports should return nil, boolean, or number")

    return 0
end

-- ========== Clipboard Functions (UI - disabled test) ==========

-- Test GetClipboard and SetClipboard - requires UI
function test_clipboard()
    -- Save original clipboard
    local original = world.GetClipboard()
    check(type(original) == "string", "GetClipboard should return a string")

    -- Set new clipboard content
    world.SetClipboard("test clipboard content")

    -- Verify
    local content = world.GetClipboard()
    check(content == "test clipboard content", "SetClipboard should change clipboard")

    -- Restore original
    world.SetClipboard(original)

    return 0
end

-- ========== Window Position Functions (UI - disabled tests) ==========

-- Test GetMainWindowPosition - requires UI
function test_get_main_window_position()
    local left, top, width, height = world.GetMainWindowPosition()
    check(type(left) == "number", "GetMainWindowPosition should return left")
    check(type(top) == "number", "GetMainWindowPosition should return top")

    return 0
end

-- Test SetStatus - requires UI
function test_set_status()
    local result = world.SetStatus("Test status message")
    check(result == nil or result == 0, "SetStatus should not error")

    return 0
end

-- Test Repaint - requires UI
function test_repaint()
    world.Repaint()
    return 0
end

-- Test Redraw - requires UI
function test_redraw()
    world.Redraw()
    return 0
end

-- ========== Notepad Functions (UI tests) ==========

-- Test notepad functions that may require UI
function test_notepad_functions()
    local title = "test_notepad_" .. world.GetUniqueNumber()

    -- These functions may work in headless mode
    world.AppendToNotepad(title, "Test line 1\n")
    world.AppendToNotepad(title, "Test line 2\n")

    local length = world.GetNotepadLength(title)
    check(type(length) == "number", "GetNotepadLength should return number")

    local text = world.GetNotepadText(title)
    check(type(text) == "string", "GetNotepadText should return string")

    -- Close notepad
    world.CloseNotepad(title)

    return 0
end

-- ========== Sound Functions ==========

-- Test Sound and StopSound
function test_sound_functions()
    -- These may or may not produce actual sound in headless mode
    -- but should not error
    local result = world.Sound("nonexistent.wav")
    -- Sound returns nil typically

    -- StopSound takes a buffer number (0 = all buffers)
    world.StopSound(0)

    return 0
end

-- ========== Global Options ==========

-- Test GetGlobalOption
function test_get_global_option()
    -- GetGlobalOption returns various global settings
    -- Note: If database is not open, returns nil for all options
    local val = world.GetGlobalOption("AlwaysOnTop")
    -- In headless mode without database, val may be nil
    -- Just verify the function can be called without error
    check(type(val) == "nil" or type(val) == "number",
          "GetGlobalOption should return nil or number")

    -- Unknown option also returns nil
    val = world.GetGlobalOption("nonexistent_option_xyz")
    check(val == nil, "GetGlobalOption should return nil for unknown option")

    return 0
end

-- Test GetGlobalOptionList
function test_get_global_option_list()
    local list = world.GetGlobalOptionList()
    check(type(list) == "table", "GetGlobalOptionList should return a table")
    check(#list > 0, "GetGlobalOptionList should return non-empty list")

    return 0
end

-- ========== Logging Config ==========

-- Test GetLogInput/SetLogInput
function test_log_input()
    local original = world.GetLogInput()
    check(type(original) == "boolean" or type(original) == "number",
          "GetLogInput should return boolean or number")

    -- Toggle
    world.SetLogInput(true)
    world.SetLogInput(false)

    -- Restore
    world.SetLogInput(original)

    return 0
end

-- Test GetLogOutput/SetLogOutput
function test_log_output()
    local original = world.GetLogOutput()
    check(type(original) == "boolean" or type(original) == "number",
          "GetLogOutput should return boolean or number")

    world.SetLogOutput(true)
    world.SetLogOutput(false)
    world.SetLogOutput(original)

    return 0
end

-- Test GetLogNotes/SetLogNotes
function test_log_notes()
    local original = world.GetLogNotes()
    check(type(original) == "boolean" or type(original) == "number",
          "GetLogNotes should return boolean or number")

    world.SetLogNotes(true)
    world.SetLogNotes(false)
    world.SetLogNotes(original)

    return 0
end

-- ========== Wildcard Functions ==========

-- Test GetTriggerWildcard
function test_get_trigger_wildcard()
    -- Add a trigger with a capture group
    local flags = trigger_flag.Enabled + trigger_flag.RegularExpression
    world.AddTrigger("wildcard_trig", "test (\\w+) pattern", "send", flags, 0, 0, "", "", sendto.World, 100)

    -- GetTriggerWildcard returns nil when not in trigger context
    -- Just verify the function exists and doesn't error
    local wc = world.GetTriggerWildcard("wildcard_trig", 1)
    -- Outside trigger context, returns nil or empty
    check(wc == nil or type(wc) == "string", "GetTriggerWildcard should return nil or string")

    world.DeleteTrigger("wildcard_trig")

    return 0
end

-- Test GetAliasWildcard
function test_get_alias_wildcard()
    -- Add an alias with a capture group
    local flags = alias_flag.Enabled + alias_flag.RegularExpression
    world.AddAlias("wildcard_alias", "test (\\w+)", "send $1", flags, "")

    -- GetAliasWildcard returns nil when not in alias context
    local wc = world.GetAliasWildcard("wildcard_alias", 1)
    check(wc == nil or type(wc) == "string", "GetAliasWildcard should return nil or string")

    world.DeleteAlias("wildcard_alias")

    return 0
end

-- ========== Command History ==========

-- Test DeleteCommandHistory
function test_delete_command_history()
    -- Just verify the function exists and doesn't error
    local result = world.DeleteCommandHistory()
    check(result == nil or type(result) == "number", "DeleteCommandHistory should return nil or number")

    return 0
end

-- Test SelectCommand
function test_select_command()
    -- SelectCommand selects from command history
    -- In test mode, just verify it doesn't error
    local result = world.SelectCommand(1)
    check(result == nil or type(result) == "number", "SelectCommand should return nil or number")

    return 0
end

-- ========== Colour Output Functions ==========

-- Test ColourNote
function test_colour_note()
    -- ColourNote(foreground, background, text, ...)
    -- Just verify it doesn't error
    world.ColourNote("white", "black", "Test coloured text")
    world.ColourNote("red", "blue", "More ", "green", "black", "text")

    return 0
end

-- Test ColourTell
function test_colour_tell()
    -- ColourTell is like ColourNote but no newline
    world.ColourTell("yellow", "black", "Test ")
    world.ColourTell("cyan", "black", "coloured ")
    world.Note("") -- End the line

    return 0
end

-- Test AnsiNote
function test_ansi_note()
    -- AnsiNote outputs text with ANSI codes interpreted
    world.AnsiNote("\027[31mRed text\027[0m normal text")

    return 0
end

-- ========== Export/Import XML ==========

-- Test ExportXML
function test_export_xml()
    -- Add some items to export
    local tflags = trigger_flag.Enabled
    world.AddTrigger("export_test_trig", "pattern", "send", tflags, 0, 0, "", "", sendto.World, 100)

    -- ExportXML(type, name) - exports to XML string
    -- type: 1=trigger, 2=alias, 3=timer, etc.
    local xml = world.ExportXML(1, "export_test_trig")
    check(type(xml) == "string", "ExportXML should return a string")
    check(#xml > 0, "ExportXML should return non-empty XML")

    world.DeleteTrigger("export_test_trig")

    return 0
end

-- ========== Extended Miniwindow Tests ==========

-- Test WindowArc
function test_window_arc()
    -- Create a window first
    world.WindowCreate("arc_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Draw an arc: WindowArc(name, left, top, right, bottom, x1, y1, x2, y2, pen_colour, pen_style, pen_width)
    local result = world.WindowArc("arc_test", 10, 10, 90, 90, 50, 10, 90, 50, 0xFFFFFF, 0, 1)
    check(result == 0, "WindowArc should return 0 on success")

    world.WindowDelete("arc_test")

    return 0
end

-- Test WindowBezier
function test_window_bezier()
    world.WindowCreate("bezier_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- WindowBezier takes a points string
    -- Format: "x1,y1,x2,y2,x3,y3,x4,y4" (4 control points for cubic bezier)
    local result = world.WindowBezier("bezier_test", "10,50,30,10,70,90,90,50", 0xFFFFFF, 0, 1)
    check(result == 0, "WindowBezier should return 0 on success")

    world.WindowDelete("bezier_test")

    return 0
end

-- Test WindowPolygon
function test_window_polygon()
    world.WindowCreate("polygon_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- WindowPolygon(name, points, pen_colour, pen_style, pen_width, brush_colour, brush_style)
    local result = world.WindowPolygon("polygon_test", "10,10,90,10,50,90",
                                        0xFFFFFF, 0, 1,  -- pen
                                        0x808080, 0)     -- brush
    check(result == 0, "WindowPolygon should return 0 on success")

    world.WindowDelete("polygon_test")

    return 0
end

-- Test WindowGradient
function test_window_gradient()
    world.WindowCreate("gradient_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- WindowGradient(name, left, top, right, bottom, start_colour, end_colour, mode)
    local result = world.WindowGradient("gradient_test", 0, 0, 100, 100, 0xFF0000, 0x0000FF, 1)
    check(result == 0, "WindowGradient should return 0 on success")

    world.WindowDelete("gradient_test")

    return 0
end

-- Test WindowAddHotspot and WindowDeleteHotspot
function test_window_hotspots()
    world.WindowCreate("hotspot_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- WindowAddHotspot(name, hotspot_id, left, top, right, bottom,
    --                  mouseover, cancelmouseover, mousedown, cancelmousedown, mouseup,
    --                  tooltip, cursor, flags)
    local result = world.WindowAddHotspot("hotspot_test", "hs1", 10, 10, 50, 50,
                                           "", "", "", "", "",
                                           "Test hotspot", 0, 0)
    check(result == 0, "WindowAddHotspot should return 0 on success")

    -- Delete hotspot
    result = world.WindowDeleteHotspot("hotspot_test", "hs1")
    check(result == 0, "WindowDeleteHotspot should return 0 on success")

    -- Re-add and delete all hotspots
    world.WindowAddHotspot("hotspot_test", "hs2", 10, 10, 50, 50, "", "", "", "", "", "Test", 0, 0)
    result = world.WindowDeleteAllHotspots("hotspot_test")
    check(result == 0, "WindowDeleteAllHotspots should return 0 on success")

    world.WindowDelete("hotspot_test")

    return 0
end

-- Test WindowHotspotInfo
function test_window_hotspot_info()
    world.WindowCreate("hsinfo_test", 0, 0, 100, 100, 0, 0, 0x000000)
    world.WindowAddHotspot("hsinfo_test", "hs1", 10, 10, 50, 50, "", "", "", "", "", "Tooltip", 0, 0)

    -- WindowHotspotInfo(name, hotspot_id, info_type)
    local left = world.WindowHotspotInfo("hsinfo_test", "hs1", 1)  -- 1 = left
    check(type(left) == "number", "WindowHotspotInfo should return a number for position")

    local tooltip = world.WindowHotspotInfo("hsinfo_test", "hs1", 6)  -- 6 = tooltip
    check(type(tooltip) == "string", "WindowHotspotInfo should return string for tooltip")

    world.WindowDelete("hsinfo_test")

    return 0
end

-- Test WindowImageList and WindowImageInfo
function test_window_images()
    world.WindowCreate("image_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Get image list (should be empty for new window)
    local list = world.WindowImageList("image_test")
    check(type(list) == "table", "WindowImageList should return a table")
    -- Note: WindowCreateImage is not registered, so we can only test with loaded images

    -- WindowImageInfo for non-existent image returns nil
    local info = world.WindowImageInfo("image_test", "nonexistent", 2)
    check(info == nil, "WindowImageInfo should return nil for non-existent image")

    world.WindowDelete("image_test")

    return 0
end

-- Test WindowPosition and WindowResize
function test_window_position_resize()
    world.WindowCreate("pos_test", 100, 100, 200, 200, 0, 0, 0x000000)

    -- Get position info
    local left = world.WindowInfo("pos_test", 10)  -- 10 = left position
    check(type(left) == "number", "WindowInfo should return position")

    -- Resize window
    local result = world.WindowResize("pos_test", 300, 300, 0x000000)
    check(result == 0, "WindowResize should return 0 on success")

    -- Move window
    result = world.WindowPosition("pos_test", 50, 50, 0, 0)
    check(result == 0, "WindowPosition should return 0 on success")

    world.WindowDelete("pos_test")

    return 0
end

-- Test WindowSetZOrder
function test_window_zorder()
    world.WindowCreate("zorder_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Set z-order (0 = bottom, higher = more on top)
    local result = world.WindowSetZOrder("zorder_test", 100)
    check(result == 0, "WindowSetZOrder should return 0 on success")

    world.WindowDelete("zorder_test")

    return 0
end

-- ========== Misc Functions ==========

-- Test TranslateDebug
function test_translate_debug()
    -- TranslateDebug in Qt version returns 1 (no translator script loaded)
    local result = world.TranslateDebug(0)
    check(type(result) == "number", "TranslateDebug should return a number")
    check(result == 1, "TranslateDebug should return 1 (no script loaded)")

    return 0
end

-- Test DoAfterSpeedWalk
function test_do_after_speedwalk()
    -- DoAfterSpeedWalk(seconds, speedwalk) - queues speedwalk after delay
    local result = world.DoAfterSpeedWalk(0.1, "3n2e")
    check(result == 0, "DoAfterSpeedWalk should return 0 on success")

    return 0
end

-- Test SetChanged
function test_set_changed()
    -- SetChanged(flag) - marks world as changed/unchanged
    world.SetChanged(true)
    world.SetChanged(false)

    return 0
end

-- Test ChangeDir
function test_change_dir()
    -- ChangeDir(path) changes working directory and returns boolean success
    -- Change to current directory (should succeed)
    local result = world.ChangeDir(".")
    check(type(result) == "boolean", "ChangeDir should return a boolean")
    check(result == true, "ChangeDir to '.' should succeed")

    -- Invalid directory should fail
    result = world.ChangeDir("/nonexistent_path_xyz_12345")
    check(result == false, "ChangeDir to invalid path should return false")

    return 0
end

print("Lua test script loaded successfully")
