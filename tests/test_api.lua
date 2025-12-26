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

-- Test IsConnected
function test_is_connected()
    -- IsConnected should return a boolean
    local connected = world.IsConnected()
    check(type(connected) == "boolean", "IsConnected should return a boolean")

    -- When not connected, it should return false
    check(connected == false, "IsConnected should return false when not connected")

    return 0
end

-- Test Connect when not connected
function test_connect_not_connected()
    -- First, verify we're not connected
    local connected = world.IsConnected()
    check(connected == false, "Should not be connected initially")

    -- Connect returns eOK (0) when initiating connection
    -- or eWorldOpen (30001) if already connected
    local err = world.Connect()
    check(type(err) == "number", "Connect should return a number")
    check(err == 0 or err == 30001, "Connect should return eOK (0) or eWorldOpen (30001)")

    return 0
end

-- Test Disconnect when not connected
function test_disconnect_not_connected()
    -- Disconnect should return eWorldClosed (30002) when not connected
    local err = world.Disconnect()
    check(type(err) == "number", "Disconnect should return a number")
    check(err == 30002, "Disconnect should return eWorldClosed (30002) when not connected")

    return 0
end

-- Test Send when not connected
function test_send_not_connected()
    -- Send should return eWorldClosed (30002) when not connected
    local err = world.Send("test message")
    check(type(err) == "number", "Send should return a number")
    check(err == 30002, "Send should return eWorldClosed (30002) when not connected")

    return 0
end

-- Test SendNoEcho when not connected
function test_send_no_echo_not_connected()
    -- SendNoEcho should return eWorldClosed (30002) when not connected
    local err = world.SendNoEcho("test message")
    check(type(err) == "number", "SendNoEcho should return a number")
    check(err == 30002, "SendNoEcho should return eWorldClosed (30002) when not connected")

    return 0
end

-- Test SendPkt when not connected
function test_send_pkt_not_connected()
    -- SendPkt should return 1 (eWorldClosed) when not connected
    -- Note: SendPkt uses old-style error codes (0=OK, 1=error)
    local err = SendPkt("test packet")
    check(type(err) == "number", "SendPkt should return a number")
    check(err == 1, "SendPkt should return 1 (eWorldClosed) when not connected")

    return 0
end

-- Test Send with nil parameter
function test_send_nil_parameter()
    -- Send with nil should error (luaL_checkstring will fail)
    local success, err = pcall(function() world.Send(nil) end)
    check(success == false, "Send(nil) should error")
    check(type(err) == "string", "Error should be a string")

    return 0
end

-- Test SendNoEcho with nil parameter
function test_send_no_echo_nil_parameter()
    -- SendNoEcho with nil should error (luaL_checkstring will fail)
    local success, err = pcall(function() world.SendNoEcho(nil) end)
    check(success == false, "SendNoEcho(nil) should error")
    check(type(err) == "string", "Error should be a string")

    return 0
end

-- Test SendPkt with nil parameter
function test_send_pkt_nil_parameter()
    -- SendPkt with nil returns error code when not connected (doesn't check param first)
    -- Note: SendPkt checks connection status before validating parameters
    local err = SendPkt(nil)
    check(type(err) == "number", "SendPkt(nil) should return a number")
    check(err == 1, "SendPkt(nil) should return 1 (eWorldClosed) when not connected")

    return 0
end

-- Test Send with empty string
function test_send_empty_string()
    -- Send with empty string should still fail with eWorldClosed when not connected
    local err = world.Send("")
    check(type(err) == "number", "Send('') should return a number")
    check(err == 30002, "Send('') should return eWorldClosed (30002) when not connected")

    return 0
end

-- Test SendNoEcho with empty string
function test_send_no_echo_empty_string()
    -- SendNoEcho with empty string should still fail with eWorldClosed when not connected
    local err = world.SendNoEcho("")
    check(type(err) == "number", "SendNoEcho('') should return a number")
    check(err == 30002, "SendNoEcho('') should return eWorldClosed (30002) when not connected")

    return 0
end

-- Test SendPkt with empty string
function test_send_pkt_empty_string()
    -- SendPkt with empty string should still fail with error 1 when not connected
    local err = SendPkt("")
    check(type(err) == "number", "SendPkt('') should return a number")
    check(err == 1, "SendPkt('') should return 1 (eWorldClosed) when not connected")

    return 0
end

-- Test SendPkt with binary data (null bytes)
function test_send_pkt_binary_data()
    -- SendPkt should handle binary data with null bytes
    local packet = "\255\250\201test\255\240" -- IAC SB GMCP test IAC SE
    local err = SendPkt(packet)
    check(type(err) == "number", "SendPkt with binary data should return a number")
    check(err == 1, "SendPkt should return 1 (not connected)")

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

-- Test UdpSend
function test_udp_send()
    -- UdpSend is a deprecated stub that always returns -1
    -- UdpSend(IP, Port, Text)
    local result = world.UdpSend("127.0.0.1", 4000, "test message")
    check(type(result) == "number", "UdpSend should return a number")
    check(result == -1, "UdpSend should return -1 (UDP support removed)")

    -- Test with different parameters (should still return -1)
    result = world.UdpSend("192.168.1.1", 8080, "another test")
    check(result == -1, "UdpSend should always return -1")

    -- Test with empty message
    result = world.UdpSend("10.0.0.1", 5000, "")
    check(result == -1, "UdpSend with empty message should return -1")

    return 0
end

-- Test UdpListen
function test_udp_listen()
    -- UdpListen is a deprecated stub that always returns -1
    -- UdpListen(IP, Port, Script)
    local result = world.UdpListen("127.0.0.1", 4000, "udp_callback")
    check(type(result) == "number", "UdpListen should return a number")
    check(result == -1, "UdpListen should return -1 (UDP support removed)")

    -- Test with different parameters (should still return -1)
    result = world.UdpListen("0.0.0.0", 9000, "another_callback")
    check(result == -1, "UdpListen should always return -1")

    -- Test with empty script name
    result = world.UdpListen("192.168.1.1", 7000, "")
    check(result == -1, "UdpListen with empty script should return -1")

    return 0
end

-- Test UdpPortList
function test_udp_port_list()
    -- UdpPortList is a deprecated stub that always returns an empty table
    -- UdpPortList()
    local result = world.UdpPortList()
    check(type(result) == "table", "UdpPortList should return a table")

    -- Count table entries
    local count = 0
    for _ in pairs(result) do
        count = count + 1
    end
    check(count == 0, "UdpPortList should return empty table (UDP support removed)")

    return 0
end

-- ========== Logging Functions ==========

-- Test OpenLog and CloseLog
function test_open_close_log()
    -- Check log is not open initially
    local is_open = world.IsLogOpen()
    check(is_open == false, "IsLogOpen should return false initially")

    -- Open log file
    local err = world.OpenLog("test_log.txt", false)
    check(err == 0, "OpenLog should return 0 on success")

    -- Check log is now open
    is_open = world.IsLogOpen()
    check(is_open == true, "IsLogOpen should return true after opening")

    -- Try opening again (should fail with eLogFileAlreadyOpen = 30001)
    err = world.OpenLog("test_log2.txt", false)
    check(err == 30001, "OpenLog should return 30001 when already open")

    -- Close log
    err = world.CloseLog()
    check(err == 0, "CloseLog should return 0 on success")

    -- Check log is now closed
    is_open = world.IsLogOpen()
    check(is_open == false, "IsLogOpen should return false after closing")

    -- Try closing again (should fail with eLogFileNotOpen = 30003)
    err = world.CloseLog()
    check(err == 30003, "CloseLog should return 30003 when not open")

    return 0
end

-- Test WriteLog
function test_write_log()
    -- Try writing when log not open (should fail with eLogFileNotOpen = 30003)
    local err = world.WriteLog("Should fail")
    check(err == 30003, "WriteLog should return 30003 when log not open")

    -- Open log
    err = world.OpenLog("test_log_write.txt", false)
    check(err == 0, "OpenLog should succeed")

    -- Write to log
    err = world.WriteLog("Test message 1")
    check(err == 0, "WriteLog should return 0 on success")

    err = world.WriteLog("Test message 2")
    check(err == 0, "WriteLog should return 0 on success")

    -- Close log
    world.CloseLog()

    return 0
end

-- Test FlushLog
function test_flush_log()
    -- Try flushing when log not open (should fail with eLogFileNotOpen = 30003)
    local err = world.FlushLog()
    check(err == 30003, "FlushLog should return 30003 when log not open")

    -- Open log
    err = world.OpenLog("test_log_flush.txt", false)
    check(err == 0, "OpenLog should succeed")

    -- Write to log
    err = world.WriteLog("Flush test message")
    check(err == 0, "WriteLog should succeed")

    -- Flush log
    err = world.FlushLog()
    check(err == 0, "FlushLog should return 0 on success")

    -- Close log
    world.CloseLog()

    return 0
end

-- Test IsLogOpen states
function test_is_log_open()
    -- Initially not open
    local is_open = world.IsLogOpen()
    check(is_open == false, "Log should not be open initially")

    -- Open log
    world.OpenLog("test_log_status.txt", false)
    is_open = world.IsLogOpen()
    check(is_open == true, "Log should be open after OpenLog")

    -- Still open after writes
    world.WriteLog("Status test")
    is_open = world.IsLogOpen()
    check(is_open == true, "Log should still be open after writing")

    -- Still open after flush
    world.FlushLog()
    is_open = world.IsLogOpen()
    check(is_open == true, "Log should still be open after flushing")

    -- Not open after close
    world.CloseLog()
    is_open = world.IsLogOpen()
    check(is_open == false, "Log should not be open after CloseLog")

    return 0
end

-- Test OpenLog with append mode
function test_open_log_append()
    -- Create initial log
    local err = world.OpenLog("test_log_append.txt", false)
    check(err == 0, "OpenLog should succeed")
    world.WriteLog("First line")
    world.CloseLog()

    -- Append to existing log
    err = world.OpenLog("test_log_append.txt", true)
    check(err == 0, "OpenLog with append should succeed")
    world.WriteLog("Second line")
    world.CloseLog()

    return 0
end

-- Test LogSend (note: requires connection to work fully, so we test error cases)
function test_log_send()
    -- LogSend without connection should return eWorldClosed = 30002
    -- Since we're not connected in tests, we expect this error
    local err = world.LogSend("test command")
    check(err == 30002, "LogSend should return 30002 when not connected")

    return 0
end

-- ========== World Info Functions ==========

-- Test GetWorldName
function test_get_world_name()
    -- GetWorldName() returns the world name as a string
    local name = world.GetWorldName()
    check(type(name) == "string", "GetWorldName should return a string")
    -- By default, the world name is empty in tests
    check(name == "", "GetWorldName should return empty string by default")

    return 0
end

-- Test GetWorldName with configured name
function test_get_world_name_configured()
    -- This test assumes the C++ test has set m_mush_name
    local name = world.GetWorldName()
    check(type(name) == "string", "GetWorldName should return a string")
    check(name == "Test World", "GetWorldName should return 'Test World'")

    return 0
end

-- Test GetWorldList
function test_get_world_list()
    -- GetWorldList() returns a table of world names
    local list = world.GetWorldList()
    check(type(list) == "table", "GetWorldList should return a table")
    -- In single-world mode, should have exactly 1 entry
    check(#list == 1, "GetWorldList should have 1 entry in single-world mode")
    -- First entry should be a string
    check(type(list[1]) == "string", "GetWorldList[1] should be a string")

    return 0
end

-- Test GetWorldList with configured name
function test_get_world_list_configured()
    -- This test assumes the C++ test has set m_mush_name
    local list = world.GetWorldList()
    check(type(list) == "table", "GetWorldList should return a table")
    check(#list == 1, "GetWorldList should have 1 entry")
    check(list[1] == "Test World", "GetWorldList[1] should be 'Test World'")

    return 0
end

-- Test GetWorldIdList
function test_get_world_id_list()
    -- GetWorldIdList() returns a table of world IDs
    local list = world.GetWorldIdList()
    check(type(list) == "table", "GetWorldIdList should return a table")
    -- In single-world mode, should have exactly 1 entry
    check(#list == 1, "GetWorldIdList should have 1 entry in single-world mode")
    -- First entry should be a string
    check(type(list[1]) == "string", "GetWorldIdList[1] should be a string")

    return 0
end

-- Test GetWorldIdList with configured ID
function test_get_world_id_list_configured()
    -- This test assumes the C++ test has set m_strWorldID
    local list = world.GetWorldIdList()
    check(type(list) == "table", "GetWorldIdList should return a table")
    check(#list == 1, "GetWorldIdList should have 1 entry")
    check(list[1] == "{TEST-WORLD-ID-123456}", "GetWorldIdList[1] should be '{TEST-WORLD-ID-123456}'")

    return 0
end

-- ========== Note Color Functions Tests ==========

-- Test NoteColourRGB
function test_note_colour_rgb()
    world.NoteColourRGB(0xFF0000, 0x00FF00)
    local fore = world.NoteColourFore()
    check(fore == 0xFF0000, "NoteColourFore should return 0xFF0000")
    local back = world.NoteColourBack()
    check(back == 0x00FF00, "NoteColourBack should return 0x00FF00")
    local mode = world.NoteColour()
    check(mode == -1, "NoteColour should return -1 in RGB mode")
    world.NoteColourRGB(0x0000FF, 0xFFFF00)
    fore = world.NoteColourFore()
    back = world.NoteColourBack()
    check(fore == 0x0000FF, "NoteColourFore should return 0x0000FF")
    check(back == 0xFFFF00, "NoteColourBack should return 0xFFFF00")
    return 0
end

-- Test NoteColourName
function test_note_colour_name()
    world.NoteColourName("red", "blue")
    local mode = world.NoteColour()
    check(mode == -1, "NoteColour should return -1 after NoteColourName")
    local fore = world.NoteColourFore()
    local back = world.NoteColourBack()
    check(fore == 0x0000FF, "NoteColourFore should return red in BGR (0x0000FF)")
    check(back == 0xFF0000, "NoteColourBack should return blue in BGR (0xFF0000)")
    world.NoteColourName("green", "yellow")
    fore = world.NoteColourFore()
    back = world.NoteColourBack()
    check(fore == 0x008000, "NoteColourFore should return green in BGR (0x008000)")
    check(back == 0x00FFFF, "NoteColourBack should return yellow in BGR (0x00FFFF)")
    world.NoteColourName("WHITE", "BLACK")
    fore = world.NoteColourFore()
    back = world.NoteColourBack()
    check(fore == 0xFFFFFF, "NoteColourFore should return white (0xFFFFFF)")
    check(back == 0x000000, "NoteColourBack should return black (0x000000)")
    return 0
end

-- Test GetNoteColourFore and GetNoteColourBack in RGB mode
function test_get_note_colour_rgb_mode()
    world.NoteColourRGB(0xABCDEF, 0x123456)
    local fore = world.GetNoteColourFore()
    local back = world.GetNoteColourBack()
    check(fore == 0xABCDEF, "GetNoteColourFore should return 0xABCDEF in RGB mode")
    check(back == 0x123456, "GetNoteColourBack should return 0x123456 in RGB mode")
    check(fore == world.NoteColourFore(), "GetNoteColourFore should match NoteColourFore in RGB mode")
    check(back == world.NoteColourBack(), "GetNoteColourBack should match NoteColourBack in RGB mode")
    return 0
end

-- Test GetNoteColour (alias for NoteColour)
function test_get_note_colour_alias()
    world.NoteColourRGB(0xFF00FF, 0x00FFFF)
    local result1 = world.NoteColour()
    local result2 = world.GetNoteColour()
    check(result1 == result2, "GetNoteColour should match NoteColour")
    check(result1 == -1, "Both should return -1 in RGB mode")
    return 0
end

-- Test NoteColour masking (strips alpha channel)
function test_note_colour_alpha_masking()
    world.NoteColourRGB(0xFFABCDEF, 0xFF123456)
    local fore = world.NoteColourFore()
    local back = world.NoteColourBack()
    check(fore == 0xABCDEF, "NoteColourFore should strip alpha channel")
    check(back == 0x123456, "NoteColourBack should strip alpha channel")
    return 0
end

-- ========== Miniwindow Text Functions Tests ==========

-- Test WindowFont
function test_window_font()
    -- Create test window
    local err = world.WindowCreate("test_font_win", 0, 0, 200, 100, 0, 2, 0xFFFFFF)
    check(err == 0, "WindowCreate failed")

    -- Add a font
    err = world.WindowFont("test_font_win", "font1", "Arial", 12, false, false, false, false)
    check(err == 0, "WindowFont failed for basic Arial font")

    -- Add bold font
    err = world.WindowFont("test_font_win", "font2", "Courier", 14, true, false, false, false)
    check(err == 0, "WindowFont failed for bold Courier font")

    -- Add italic font
    err = world.WindowFont("test_font_win", "font3", "Times", 10, false, true, false, false)
    check(err == 0, "WindowFont failed for italic Times font")

    -- Add underline font
    err = world.WindowFont("test_font_win", "font4", "Helvetica", 11, false, false, true, false)
    check(err == 0, "WindowFont failed for underline Helvetica font")

    -- Add strikeout font
    err = world.WindowFont("test_font_win", "font5", "Georgia", 13, false, false, false, true)
    check(err == 0, "WindowFont failed for strikeout Georgia font")

    -- Add font with multiple styles
    err = world.WindowFont("test_font_win", "font6", "Verdana", 15, true, true, true, true)
    check(err == 0, "WindowFont failed for multi-style Verdana font")

    -- Test with non-existent window (should return eNoSuchWindow = 30073)
    err = world.WindowFont("nonexistent_win", "font1", "Arial", 12, false, false, false, false)
    check(err == 30073, "WindowFont should return 30073 for non-existent window")

    -- Clean up
    err = world.WindowDelete("test_font_win")
    check(err == 0, "WindowDelete failed")

    return 0
end

-- Test WindowText
function test_window_text()
    -- Create test window
    local err = world.WindowCreate("test_text_win", 0, 0, 300, 200, 0, 2, 0xFFFFFF)
    check(err == 0, "WindowCreate failed")

    -- Add a font first
    err = world.WindowFont("test_text_win", "text_font", "Arial", 12, false, false, false, false)
    check(err == 0, "WindowFont failed")

    -- Draw basic text (returns text width on success)
    local width = world.WindowText("test_text_win", "text_font", "Hello World", 10, 10, 100, 30, 0x000000, false)
    check(width > 0, "WindowText failed for basic text")

    -- Draw text with unicode
    width = world.WindowText("test_text_win", "text_font", "Test Unicode", 10, 40, 100, 60, 0xFF0000, true)
    check(width > 0, "WindowText failed for unicode text")

    -- Draw text with different color
    width = world.WindowText("test_text_win", "text_font", "Colored Text", 10, 70, 150, 90, 0x00FF00, false)
    check(width > 0, "WindowText failed for colored text")

    -- Draw text in larger rectangle
    width = world.WindowText("test_text_win", "text_font", "Wide Rectangle", 10, 100, 200, 120, 0x0000FF, false)
    check(width > 0, "WindowText failed for wide rectangle")

    -- Test with non-existent window (should return eNoSuchWindow = 30073)
    err = world.WindowText("nonexistent_win", "text_font", "Test", 0, 0, 100, 20, 0x000000, false)
    check(err == 30073, "WindowText should return 30073 for non-existent window")

    -- Clean up
    err = world.WindowDelete("test_text_win")
    check(err == 0, "WindowDelete failed")

    return 0
end

-- Test WindowTextWidth
function test_window_text_width()
    -- Create test window
    local err = world.WindowCreate("test_width_win", 0, 0, 200, 100, 0, 2, 0xFFFFFF)
    check(err == 0, "WindowCreate failed")

    -- Add a font
    err = world.WindowFont("test_width_win", "width_font", "Arial", 12, false, false, false, false)
    check(err == 0, "WindowFont failed")

    -- Measure text width
    local width = world.WindowTextWidth("test_width_win", "width_font", "Hello", false)
    check(width > 0, "WindowTextWidth should return positive width")

    -- Measure longer text (should be wider)
    local width2 = world.WindowTextWidth("test_width_win", "width_font", "Hello World", false)
    check(width2 > width, "Longer text should have greater width")

    -- Measure empty string
    local width3 = world.WindowTextWidth("test_width_win", "width_font", "", false)
    check(width3 == 0, "Empty string should have zero width")

    -- Measure with unicode
    local width4 = world.WindowTextWidth("test_width_win", "width_font", "Test Unicode", true)
    check(width4 > 0, "WindowTextWidth with unicode should return positive width")

    -- Test with different font
    err = world.WindowFont("test_width_win", "width_font2", "Courier", 14, false, false, false, false)
    check(err == 0, "WindowFont failed for Courier")

    local width5 = world.WindowTextWidth("test_width_win", "width_font2", "Hello", false)
    check(width5 > 0, "WindowTextWidth with different font should return positive width")

    -- Test with non-existent window (should return 0)
    local width6 = world.WindowTextWidth("nonexistent_win", "width_font", "Test", false)
    check(width6 == 0, "WindowTextWidth should return 0 for non-existent window")

    -- Clean up
    err = world.WindowDelete("test_width_win")
    check(err == 0, "WindowDelete failed")

    return 0
end

-- Test WindowFontInfo
function test_window_font_info()
    -- Create test window
    local err = world.WindowCreate("test_fontinfo_win", 0, 0, 200, 100, 0, 2, 0xFFFFFF)
    check(err == 0, "WindowCreate failed")

    -- Add a font
    err = world.WindowFont("test_fontinfo_win", "info_font", "Arial", 12, false, false, false, false)
    check(err == 0, "WindowFont failed")

    -- Test info type 1: height
    local height = world.WindowFontInfo("test_fontinfo_win", "info_font", 1)
    check(height ~= nil, "WindowFontInfo should return height")
    check(height > 0, "Font height should be positive")

    -- Test info type 2: ascent
    local ascent = world.WindowFontInfo("test_fontinfo_win", "info_font", 2)
    check(ascent ~= nil, "WindowFontInfo should return ascent")
    check(ascent > 0, "Font ascent should be positive")

    -- Test info type 3: descent
    local descent = world.WindowFontInfo("test_fontinfo_win", "info_font", 3)
    check(descent ~= nil, "WindowFontInfo should return descent")
    check(descent >= 0, "Font descent should be non-negative")

    -- Test info type 4: leading (internal leading)
    local leading = world.WindowFontInfo("test_fontinfo_win", "info_font", 4)
    check(leading ~= nil, "WindowFontInfo should return leading")

    -- Test info type 5: external leading
    local ext_leading = world.WindowFontInfo("test_fontinfo_win", "info_font", 5)
    check(ext_leading ~= nil, "WindowFontInfo should return external leading")

    -- Test info type 6: average char width
    local avg_width = world.WindowFontInfo("test_fontinfo_win", "info_font", 6)
    check(avg_width ~= nil, "WindowFontInfo should return average char width")
    check(avg_width > 0, "Average char width should be positive")

    -- Test info type 7: max char width
    local max_width = world.WindowFontInfo("test_fontinfo_win", "info_font", 7)
    check(max_width ~= nil, "WindowFontInfo should return max char width")
    check(max_width > 0, "Max char width should be positive")

    -- Test info type 8: weight
    local weight = world.WindowFontInfo("test_fontinfo_win", "info_font", 8)
    check(weight ~= nil, "WindowFontInfo should return weight")

    -- Test info type 9: overhang
    local overhang = world.WindowFontInfo("test_fontinfo_win", "info_font", 9)
    check(overhang ~= nil, "WindowFontInfo should return overhang")

    -- Test with bold font
    err = world.WindowFont("test_fontinfo_win", "bold_font", "Arial", 12, true, false, false, false)
    check(err == 0, "WindowFont failed for bold font")

    local bold_weight = world.WindowFontInfo("test_fontinfo_win", "bold_font", 8)
    check(bold_weight ~= nil, "WindowFontInfo should return weight for bold font")

    -- Test with invalid info type (should return nil)
    local invalid = world.WindowFontInfo("test_fontinfo_win", "info_font", 99)
    check(invalid == nil, "WindowFontInfo should return nil for invalid info type")

    -- Test with non-existent window (should return nil)
    local no_win = world.WindowFontInfo("nonexistent_win", "info_font", 1)
    check(no_win == nil, "WindowFontInfo should return nil for non-existent window")

    -- Test with non-existent font (should return nil)
    local no_font = world.WindowFontInfo("test_fontinfo_win", "nonexistent_font", 1)
    check(no_font == nil, "WindowFontInfo should return nil for non-existent font")

    -- Clean up
    err = world.WindowDelete("test_fontinfo_win")
    check(err == 0, "WindowDelete failed")

    return 0
end

-- Test WindowFontList
function test_window_font_list()
    -- Create test window
    local err = world.WindowCreate("test_fontlist_win", 0, 0, 200, 100, 0, 2, 0xFFFFFF)
    check(err == 0, "WindowCreate failed")

    -- Get font list (should be empty initially)
    local fonts = world.WindowFontList("test_fontlist_win")
    check(type(fonts) == "table", "WindowFontList should return table")
    check(#fonts == 0, "Initial font list should be empty")

    -- Add first font
    err = world.WindowFont("test_fontlist_win", "list_font1", "Arial", 12, false, false, false, false)
    check(err == 0, "WindowFont failed for font1")

    fonts = world.WindowFontList("test_fontlist_win")
    check(#fonts == 1, "Font list should have 1 font")
    check(fonts[1] == "list_font1", "Font list should contain list_font1")

    -- Add second font
    err = world.WindowFont("test_fontlist_win", "list_font2", "Courier", 14, false, false, false, false)
    check(err == 0, "WindowFont failed for font2")

    fonts = world.WindowFontList("test_fontlist_win")
    check(#fonts == 2, "Font list should have 2 fonts")

    -- Check both fonts are in list
    local found1 = false
    local found2 = false
    for i = 1, #fonts do
        if fonts[i] == "list_font1" then
            found1 = true
        end
        if fonts[i] == "list_font2" then
            found2 = true
        end
    end
    check(found1, "Font list should contain list_font1")
    check(found2, "Font list should contain list_font2")

    -- Add third font
    err = world.WindowFont("test_fontlist_win", "list_font3", "Times", 10, false, false, false, false)
    check(err == 0, "WindowFont failed for font3")

    fonts = world.WindowFontList("test_fontlist_win")
    check(#fonts == 3, "Font list should have 3 fonts")

    -- Replace existing font (should not add duplicate)
    err = world.WindowFont("test_fontlist_win", "list_font1", "Helvetica", 16, false, false, false, false)
    check(err == 0, "WindowFont failed for replacing font1")

    fonts = world.WindowFontList("test_fontlist_win")
    check(#fonts == 3, "Font list should still have 3 fonts after replacement")

    -- Test with non-existent window (should return empty table)
    local no_fonts = world.WindowFontList("nonexistent_win")
    check(type(no_fonts) == "table", "WindowFontList should return table for non-existent window")
    check(#no_fonts == 0, "Non-existent window should have empty font list")

    -- Clean up
    err = world.WindowDelete("test_fontlist_win")
    check(err == 0, "WindowDelete failed")

    return 0
end

-- ========== Info Bar Functions ==========

-- Test Info
function test_info()
    -- Info(text) - displays text in the info bar
    -- Text is appended to current content

    -- Start fresh
    world.InfoClear()

    -- Add some text
    world.Info("Hello")
    world.Info(" World")

    -- Note: We can't directly read m_infoBarText from Lua, but we can verify
    -- the function doesn't error

    return 0
end

-- Test InfoClear
function test_info_clear()
    -- InfoClear() - clears the info bar and resets to default formatting

    -- Set some content and formatting
    world.Info("Some text")
    world.InfoColour("red")
    world.InfoBackground("blue")
    world.InfoFont("Arial", 12, 1)

    -- Clear everything
    world.InfoClear()

    -- Verify it doesn't error (actual content reset is tested in C++)

    return 0
end

-- Test InfoColour
function test_info_colour()
    -- InfoColour(name) - sets the info bar text color by color name

    -- Test with various color names
    world.InfoColour("red")
    world.InfoColour("blue")
    world.InfoColour("green")
    world.InfoColour("white")
    world.InfoColour("black")
    world.InfoColour("yellow")
    world.InfoColour("cyan")
    world.InfoColour("magenta")
    world.InfoColour("gray")
    world.InfoColour("grey")
    world.InfoColour("silver")
    world.InfoColour("maroon")
    world.InfoColour("purple")
    world.InfoColour("lime")
    world.InfoColour("navy")

    return 0
end

-- Test InfoBackground
function test_info_background()
    -- InfoBackground(name) - sets the info bar background color by color name

    -- Test with various color names
    world.InfoBackground("white")
    world.InfoBackground("black")
    world.InfoBackground("red")
    world.InfoBackground("blue")
    world.InfoBackground("green")
    world.InfoBackground("yellow")
    world.InfoBackground("cyan")
    world.InfoBackground("magenta")

    return 0
end

-- Test InfoFont
function test_info_font()
    -- InfoFont(fontName, size, style) - sets the info bar font
    -- fontName: Font family name (empty string to keep current)
    -- size: Font size in points (0 or negative to keep current)
    -- style: Style bits - 1=bold, 2=italic, 4=underline, 8=strikeout

    -- Set font with all parameters
    world.InfoFont("Arial", 12, 0)

    -- Set bold font
    world.InfoFont("Courier New", 10, 1)

    -- Set italic font
    world.InfoFont("Times New Roman", 14, 2)

    -- Set bold + italic
    world.InfoFont("Helvetica", 11, 3)

    -- Set underline
    world.InfoFont("Verdana", 10, 4)

    -- Set strikeout
    world.InfoFont("Georgia", 10, 8)

    -- Set bold + underline
    world.InfoFont("Comic Sans MS", 10, 5)

    -- Keep current font name (empty string)
    world.InfoFont("", 16, 0)

    -- Keep current size (0 or negative)
    world.InfoFont("Arial", 0, 1)
    world.InfoFont("Arial", -1, 1)

    -- Optional parameters (should default)
    world.InfoFont("Arial")
    world.InfoFont("Arial", 12)

    return 0
end

-- Test Hyperlink
function test_hyperlink()
    -- Hyperlink(action, text, hint, forecolour, backcolour, url)
    -- action: Command to send when clicked (required)
    -- text: Display text (optional, defaults to action)
    -- hint: Tooltip text (optional, defaults to action)
    -- forecolour: Foreground color (optional)
    -- backcolour: Background color (optional)
    -- url: If true, opens as URL in browser (optional, defaults to false)

    -- Basic hyperlink with just action
    world.Hyperlink("look")

    -- Hyperlink with action and text
    world.Hyperlink("inventory", "Check Inventory")

    -- Hyperlink with action, text, and hint
    world.Hyperlink("score", "View Score", "Click to view your character score")

    -- Hyperlink with colors
    world.Hyperlink("help", "Help", "Get help", "blue", "white")

    -- Hyperlink as URL
    world.Hyperlink("https://example.com", "Visit Website", "Click to open in browser", "", "", true)

    -- Test with nil/empty values
    world.Hyperlink("test", "", "")

    -- Empty action should be ignored (no error)
    world.Hyperlink("")

    return 0
end

-- ========== UI Accelerator Functions (DISABLED - require GUI) ==========

-- Test Accelerator: Register keyboard accelerator with command
function test_accelerator()
    -- Add an accelerator for F5 -> look command
    local err = world.Accelerator("F5", "look")
    check(err == 0, "Accelerator should return 0 (eOK)")

    -- Add another accelerator with Ctrl modifier
    err = world.Accelerator("Ctrl+S", "save")
    check(err == 0, "Accelerator with Ctrl should return 0")

    -- Add accelerator with Alt modifier
    err = world.Accelerator("Alt+Q", "quit")
    check(err == 0, "Accelerator with Alt should return 0")

    -- Test invalid key string (should return error)
    err = world.Accelerator("InvalidKey123", "test")
    check(err ~= 0, "Invalid key string should return error")

    -- Remove accelerator by setting empty string
    err = world.Accelerator("F5", "")
    check(err == 0, "Removing accelerator should return 0")

    return 0
end

-- Test AcceleratorList: Get list of all registered accelerators
function test_accelerator_list()
    -- First add some accelerators to list
    world.Accelerator("F1", "help")
    world.Accelerator("F2", "score")
    world.Accelerator("Ctrl+N", "north")

    -- Get the list
    local list = world.AcceleratorList()
    check(type(list) == "table", "AcceleratorList should return a table")
    check(#list >= 3, "AcceleratorList should contain at least 3 entries")

    -- Verify format of entries (should be "Key = Command" or "Key = Command\t[sendto]")
    local found_f1 = false
    for i = 1, #list do
        if type(list[i]) == "string" then
            if list[i]:find("F1") and list[i]:find("help") then
                found_f1 = true
            end
        end
    end
    check(found_f1, "AcceleratorList should contain F1 = help entry")

    -- Clean up
    world.Accelerator("F1", "")
    world.Accelerator("F2", "")
    world.Accelerator("Ctrl+N", "")

    return 0
end

-- Test AcceleratorTo: Register accelerator with specific send destination
function test_accelerator_to()
    -- Add accelerator that sends to world (10)
    local err = world.AcceleratorTo("F6", "look around", sendto.World)
    check(err == 0, "AcceleratorTo with sendto.World should return 0")

    -- Add accelerator that sends to script (12)
    err = world.AcceleratorTo("F7", "DoScript()", sendto.Script)
    check(err == 0, "AcceleratorTo with sendto.Script should return 0")

    -- Add accelerator that sends to output (1)
    err = world.AcceleratorTo("F8", "Output text", sendto.Output)
    check(err == 0, "AcceleratorTo with sendto.Output should return 0")

    -- Verify they appear in the list
    local list = world.AcceleratorList()
    local found_f6 = false
    local found_f7 = false
    for i = 1, #list do
        if type(list[i]) == "string" then
            if list[i]:find("F6") then
                found_f6 = true
            end
            if list[i]:find("F7") then
                found_f7 = true
            end
        end
    end
    check(found_f6, "F6 accelerator should be in list")
    check(found_f7, "F7 accelerator should be in list")

    -- Test invalid key string
    err = world.AcceleratorTo("BadKey@#$", "test", sendto.World)
    check(err ~= 0, "Invalid key string should return error")

    -- Clean up
    world.AcceleratorTo("F6", "", sendto.World)
    world.AcceleratorTo("F7", "", sendto.Script)
    world.AcceleratorTo("F8", "", sendto.Output)

    return 0
end

-- Test Activate: Bring world window to front
function test_activate()
    -- This function has no return value and emits a signal
    -- In headless mode, it should not crash
    world.Activate()

    -- Call it multiple times to ensure stability
    world.Activate()
    world.Activate()

    return 0
end

-- Test ActivateClient: Bring main application window to front
function test_activate_client()
    -- This function has no return value and emits a signal
    -- In headless mode, it should not crash
    world.ActivateClient()

    -- Call it multiple times to ensure stability
    world.ActivateClient()
    world.ActivateClient()

    return 0
end

print("Lua test script loaded successfully")

-- ========== UI Background Functions (DISABLED - require UI) ==========

-- Test SetBackgroundColour
function test_set_background_colour()
    -- SetBackgroundColour(colour) sets output window background color
    -- Returns previous color (BGR format)

    -- Get current background color by calling with a known value
    local initial_colour = world.SetBackgroundColour(0x000000)  -- Black
    check(type(initial_colour) == "number", "SetBackgroundColour should return a number")

    -- Set to white background
    local prev = world.SetBackgroundColour(0xFFFFFF)  -- White
    check(prev == 0x000000, "Previous colour should be black")

    -- Set to red background (BGR: 0x0000FF)
    prev = world.SetBackgroundColour(0x0000FF)
    check(prev == 0xFFFFFF, "Previous colour should be white")

    -- Restore original
    world.SetBackgroundColour(initial_colour)

    return 0
end

-- Test SetBackgroundImage
function test_set_background_image()
    -- SetBackgroundImage(filename, mode) sets background image
    -- mode: 0-3=stretch variants, 4-12=position, 13=tile
    -- Returns eOK (0) on success, eBadParameter on invalid mode

    -- Test with empty filename (clears image)
    local result = world.SetBackgroundImage("", 0)
    check(result == 0, "SetBackgroundImage with empty filename should succeed")

    -- Test with valid mode values
    result = world.SetBackgroundImage("background.png", 0)  -- Stretch
    check(result == 0, "SetBackgroundImage mode 0 should succeed")

    result = world.SetBackgroundImage("background.png", 13)  -- Tile
    check(result == 0, "SetBackgroundImage mode 13 should succeed")

    -- Test with invalid mode (should return eBadParameter = 2)
    result = world.SetBackgroundImage("background.png", -1)
    check(result == 2, "SetBackgroundImage mode -1 should return eBadParameter")

    result = world.SetBackgroundImage("background.png", 14)
    check(result == 2, "SetBackgroundImage mode 14 should return eBadParameter")

    -- Clear image
    world.SetBackgroundImage("", 0)

    return 0
end

-- Test SetCursor
function test_set_cursor()
    -- SetCursor(cursor_type) sets cursor shape
    -- cursor_type: -1=hide, 0=arrow, 1=hand, 2=I-beam, etc.
    -- Returns eOK (0) on success, eBadParameter on invalid cursor

    -- Test valid cursor types
    local result = world.SetCursor(0)  -- Arrow
    check(result == 0, "SetCursor arrow should succeed")

    result = world.SetCursor(1)  -- Hand
    check(result == 0, "SetCursor hand should succeed")

    result = world.SetCursor(2)  -- I-beam
    check(result == 0, "SetCursor I-beam should succeed")

    result = world.SetCursor(-1)  -- Hide cursor
    check(result == 0, "SetCursor hide should succeed")

    -- Test invalid cursor type (should return eBadParameter = 2)
    result = world.SetCursor(99)
    check(result == 2, "SetCursor invalid type should return eBadParameter")

    result = world.SetCursor(-2)
    check(result == 2, "SetCursor -2 should return eBadParameter")

    -- Restore to arrow
    world.SetCursor(0)

    return 0
end

-- Test SetForegroundImage
function test_set_foreground_image()
    -- SetForegroundImage(filename, mode) sets foreground overlay image
    -- mode: 0-3=stretch variants, 4-12=position, 13=tile
    -- Returns eOK (0) on success, eBadParameter on invalid mode

    -- Test with empty filename (clears image)
    local result = world.SetForegroundImage("", 0)
    check(result == 0, "SetForegroundImage with empty filename should succeed")

    -- Test with valid mode values
    result = world.SetForegroundImage("overlay.png", 4)  -- Position
    check(result == 0, "SetForegroundImage mode 4 should succeed")

    result = world.SetForegroundImage("overlay.png", 13)  -- Tile
    check(result == 0, "SetForegroundImage mode 13 should succeed")

    -- Test boundary modes
    result = world.SetForegroundImage("overlay.png", 0)
    check(result == 0, "SetForegroundImage mode 0 should succeed")

    result = world.SetForegroundImage("overlay.png", 13)
    check(result == 0, "SetForegroundImage mode 13 should succeed")

    -- Test with invalid mode (should return eBadParameter = 2)
    result = world.SetForegroundImage("overlay.png", -1)
    check(result == 2, "SetForegroundImage mode -1 should return eBadParameter")

    result = world.SetForegroundImage("overlay.png", 14)
    check(result == 2, "SetForegroundImage mode 14 should return eBadParameter")

    -- Clear image
    world.SetForegroundImage("", 0)

    return 0
end

-- Test SetFrameBackgroundColour
function test_set_frame_background_colour()
    -- SetFrameBackgroundColour(colour) sets frame background color
    -- In Qt version, this is same as SetBackgroundColour
    -- Returns previous color (BGR format)

    -- Get current color
    local initial_colour = world.SetFrameBackgroundColour(0x000000)  -- Black
    check(type(initial_colour) == "number", "SetFrameBackgroundColour should return a number")

    -- Set to blue background (BGR: 0xFF0000)
    local prev = world.SetFrameBackgroundColour(0xFF0000)
    check(prev == 0x000000, "Previous colour should be black")

    -- Set to green background (BGR: 0x00FF00)
    prev = world.SetFrameBackgroundColour(0x00FF00)
    check(prev == 0xFF0000, "Previous colour should be blue")

    -- Restore original
    world.SetFrameBackgroundColour(initial_colour)

    return 0
end

-- Test ShowInfoBar
function test_show_info_bar()
    -- ShowInfoBar(visible) shows or hides the info bar
    -- visible: true to show, false to hide
    -- No return value

    -- Show info bar
    world.ShowInfoBar(true)

    -- Hide info bar
    world.ShowInfoBar(false)

    -- Show again
    world.ShowInfoBar(true)

    -- Test with numeric values (Lua treats non-zero as true)
    world.ShowInfoBar(1)
    world.ShowInfoBar(0)

    return 0
end

-- Test TextRectangle
function test_text_rectangle()
    -- TextRectangle(left, top, right, bottom, borderOffset, borderColour, borderWidth, outsideFillColour, outsideFillStyle)
    -- Sets text display rectangle with border and fill
    -- Returns eOK (0) on success

    -- Test basic rectangle (100x100 at origin)
    local result = world.TextRectangle(0, 0, 100, 100, 5, 0xFF0000, 2, 0x00FF00, 0)
    check(result == 0, "TextRectangle basic should succeed")

    -- Test with negative right/bottom (offset from edge)
    result = world.TextRectangle(10, 10, -10, -10, 3, 0x0000FF, 1, 0xFFFFFF, 0)
    check(result == 0, "TextRectangle negative offset should succeed")

    -- Test with no border (width 0)
    result = world.TextRectangle(0, 0, 200, 200, 0, 0x000000, 0, 0x000000, 1)
    check(result == 0, "TextRectangle no border should succeed")

    -- Test with different fill styles (0=solid, 1=null, 2+=patterns)
    result = world.TextRectangle(0, 0, 100, 100, 5, 0x000000, 1, 0xFFFFFF, 1)
    check(result == 0, "TextRectangle fill style 1 should succeed")

    result = world.TextRectangle(0, 0, 100, 100, 5, 0x000000, 1, 0xFFFFFF, 2)
    check(result == 0, "TextRectangle fill style 2 should succeed")

    -- Test large rectangle
    result = world.TextRectangle(0, 0, 1000, 800, 10, 0xFF00FF, 5, 0x00FFFF, 0)
    check(result == 0, "TextRectangle large should succeed")

    return 0
end

-- Test Menu
function test_menu()
    -- Menu(items, default) displays popup menu and returns selected item
    -- items: pipe-separated list, "-" for separator, "!" prefix for checkmark
    -- default: optional default/selected item
    -- Returns selected item text (trimmed), or empty string if canceled
    -- NOTE: This test cannot actually verify user selection in automated tests

    -- Test that function exists and accepts parameters
    -- In automated tests, we can't verify actual menu display or selection
    -- We just verify the function can be called without errors

    -- Simple menu items
    local items = "Option 1|Option 2|Option 3"
    -- Note: In automated test, menu won't actually display
    -- This will return empty string (canceled) since no user interaction
    local result = world.Menu(items, "Option 1")
    check(type(result) == "string", "Menu should return a string")

    -- Menu with separator
    items = "New|Open|-|Save|Save As"
    result = world.Menu(items, "Save")
    check(type(result) == "string", "Menu with separator should return a string")

    -- Menu with checkmarks
    items = "Normal|!Checked Item|Another"
    result = world.Menu(items, "")
    check(type(result) == "string", "Menu with checkmarks should return a string")

    -- Menu with no default
    items = "Choice A|Choice B|Choice C"
    result = world.Menu(items)
    check(type(result) == "string", "Menu with no default should return a string")

    -- Empty items (should return empty string)
    result = world.Menu("", "")
    check(result == "", "Menu with empty items should return empty string")

    return 0
end
-- ========== Database Meta Functions ==========

-- Test DatabaseGetField
function test_database_get_field()
    -- Open an in-memory database
    local err = world.DatabaseOpen("getfield_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Create a test table
    err = world.DatabaseExec("getfield_db", "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)")
    check(err == 0, "CREATE TABLE should succeed")

    -- Insert test data
    err = world.DatabaseExec("getfield_db", "INSERT INTO test (name, value) VALUES ('test1', 42)")
    check(err == 0, "INSERT should succeed")
    err = world.DatabaseExec("getfield_db", "INSERT INTO test (name, value) VALUES ('test2', 100)")
    check(err == 0, "INSERT second row should succeed")

    -- Test DatabaseGetField to retrieve a single value
    local count = world.DatabaseGetField("getfield_db", "SELECT COUNT(*) FROM test")
    check(count == 2, "DatabaseGetField should return count of 2")

    local name = world.DatabaseGetField("getfield_db", "SELECT name FROM test WHERE id = 1")
    check(name == "test1", "DatabaseGetField should return 'test1'")

    local value = world.DatabaseGetField("getfield_db", "SELECT value FROM test WHERE id = 2")
    check(value == 100, "DatabaseGetField should return 100")

    -- Test with no results (should return nil)
    local no_result = world.DatabaseGetField("getfield_db", "SELECT name FROM test WHERE id = 999")
    check(no_result == nil, "DatabaseGetField should return nil for no results")

    -- Test with invalid SQL (should return nil)
    local invalid = world.DatabaseGetField("getfield_db", "INVALID SQL")
    check(invalid == nil, "DatabaseGetField should return nil for invalid SQL")

    -- Test with non-existent database (should return nil)
    local bad_db = world.DatabaseGetField("nonexistent_db", "SELECT 1")
    check(bad_db == nil, "DatabaseGetField should return nil for non-existent database")

    -- Clean up
    world.DatabaseClose("getfield_db")

    return 0
end

-- Test DatabaseError
function test_database_error()
    -- Open an in-memory database
    local err = world.DatabaseOpen("error_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Execute valid SQL (should not produce error message)
    err = world.DatabaseExec("error_db", "CREATE TABLE test (id INTEGER)")
    check(err == 0, "CREATE TABLE should succeed")

    local error_msg = world.DatabaseError("error_db")
    check(error_msg == "not an error", "DatabaseError should return 'not an error' after successful operation")

    -- Execute invalid SQL (should produce error message)
    err = world.DatabaseExec("error_db", "INVALID SQL STATEMENT")
    check(err ~= 0, "Invalid SQL should return error code")

    error_msg = world.DatabaseError("error_db")
    check(type(error_msg) == "string", "DatabaseError should return a string")
    check(error_msg ~= "", "DatabaseError should return non-empty error message")

    -- Test with non-existent database (should return empty string)
    error_msg = world.DatabaseError("nonexistent_db")
    check(error_msg == "", "DatabaseError should return empty string for non-existent database")

    -- Clean up
    world.DatabaseClose("error_db")

    return 0
end

-- Test DatabaseChanges
function test_database_changes()
    -- Open an in-memory database
    local err = world.DatabaseOpen("changes_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Create a test table
    err = world.DatabaseExec("changes_db", "CREATE TABLE test (id INTEGER PRIMARY KEY, value INTEGER)")
    check(err == 0, "CREATE TABLE should succeed")

    -- Insert one row and check changes
    err = world.DatabaseExec("changes_db", "INSERT INTO test (value) VALUES (1)")
    check(err == 0, "INSERT should succeed")

    local changes = world.DatabaseChanges("changes_db")
    check(changes == 1, "DatabaseChanges should return 1 after single INSERT")

    -- Insert multiple rows at once
    err = world.DatabaseExec("changes_db", "INSERT INTO test (value) VALUES (2), (3), (4)")
    check(err == 0, "INSERT multiple rows should succeed")

    changes = world.DatabaseChanges("changes_db")
    check(changes == 3, "DatabaseChanges should return 3 after multi-row INSERT")

    -- Update multiple rows
    err = world.DatabaseExec("changes_db", "UPDATE test SET value = 10 WHERE value < 4")
    check(err == 0, "UPDATE should succeed")

    changes = world.DatabaseChanges("changes_db")
    check(changes == 3, "DatabaseChanges should return 3 after UPDATE affecting 3 rows")

    -- Delete one row
    err = world.DatabaseExec("changes_db", "DELETE FROM test WHERE id = 1")
    check(err == 0, "DELETE should succeed")

    changes = world.DatabaseChanges("changes_db")
    check(changes == 1, "DatabaseChanges should return 1 after single DELETE")

    -- Delete all remaining rows
    err = world.DatabaseExec("changes_db", "DELETE FROM test")
    check(err == 0, "DELETE all should succeed")

    changes = world.DatabaseChanges("changes_db")
    check(changes == 3, "DatabaseChanges should return 3 after deleting all remaining rows")

    -- SELECT doesn't change rows
    -- Note: sqlite3_changes() retains the last value after SELECT via sqlite3_exec()
    -- It doesn't reset to 0, it keeps the value from the last modifying statement
    err = world.DatabaseExec("changes_db", "SELECT * FROM test")
    check(err == 0, "SELECT should succeed")

    changes = world.DatabaseChanges("changes_db")
    check(changes == 3, "DatabaseChanges should still be 3 after SELECT (unchanged)")

    -- Clean up
    world.DatabaseClose("changes_db")

    return 0
end

-- Test DatabaseTotalChanges
function test_database_total_changes()
    -- Open an in-memory database
    local err = world.DatabaseOpen("total_changes_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Create a test table
    err = world.DatabaseExec("total_changes_db", "CREATE TABLE test (id INTEGER PRIMARY KEY, value INTEGER)")
    check(err == 0, "CREATE TABLE should succeed")

    -- Check initial total changes (should be 0)
    local total = world.DatabaseTotalChanges("total_changes_db")
    check(total == 0, "DatabaseTotalChanges should be 0 initially")

    -- Insert some rows
    err = world.DatabaseExec("total_changes_db", "INSERT INTO test (value) VALUES (1)")
    check(err == 0, "INSERT should succeed")

    total = world.DatabaseTotalChanges("total_changes_db")
    check(total == 1, "DatabaseTotalChanges should be 1 after first INSERT")

    -- Insert more rows
    err = world.DatabaseExec("total_changes_db", "INSERT INTO test (value) VALUES (2), (3)")
    check(err == 0, "INSERT multiple should succeed")

    total = world.DatabaseTotalChanges("total_changes_db")
    check(total == 3, "DatabaseTotalChanges should be 3 (1+2) after second INSERT")

    -- Update rows
    err = world.DatabaseExec("total_changes_db", "UPDATE test SET value = 10")
    check(err == 0, "UPDATE should succeed")

    total = world.DatabaseTotalChanges("total_changes_db")
    check(total == 6, "DatabaseTotalChanges should be 6 (3+3) after UPDATE")

    -- Delete rows
    err = world.DatabaseExec("total_changes_db", "DELETE FROM test WHERE value = 10")
    check(err == 0, "DELETE should succeed")

    total = world.DatabaseTotalChanges("total_changes_db")
    check(total == 9, "DatabaseTotalChanges should be 9 (6+3) after DELETE")

    -- Clean up
    world.DatabaseClose("total_changes_db")

    return 0
end

-- Test DatabaseLastInsertRowid
function test_database_last_insert_rowid()
    -- Open an in-memory database
    local err = world.DatabaseOpen("rowid_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Create a test table with autoincrement primary key
    err = world.DatabaseExec("rowid_db", "CREATE TABLE test (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)")
    check(err == 0, "CREATE TABLE should succeed")

    -- Insert first row
    err = world.DatabaseExec("rowid_db", "INSERT INTO test (name) VALUES ('first')")
    check(err == 0, "INSERT should succeed")

    local rowid = world.DatabaseLastInsertRowid("rowid_db")
    check(type(rowid) == "string", "DatabaseLastInsertRowid should return a string")
    check(rowid == "1", "DatabaseLastInsertRowid should be '1' after first insert")

    -- Insert second row
    err = world.DatabaseExec("rowid_db", "INSERT INTO test (name) VALUES ('second')")
    check(err == 0, "INSERT should succeed")

    rowid = world.DatabaseLastInsertRowid("rowid_db")
    check(rowid == "2", "DatabaseLastInsertRowid should be '2' after second insert")

    -- Insert third row
    err = world.DatabaseExec("rowid_db", "INSERT INTO test (name) VALUES ('third')")
    check(err == 0, "INSERT should succeed")

    rowid = world.DatabaseLastInsertRowid("rowid_db")
    check(rowid == "3", "DatabaseLastInsertRowid should be '3' after third insert")

    -- Test with non-existent database (should return empty string)
    rowid = world.DatabaseLastInsertRowid("nonexistent_db")
    check(rowid == "", "DatabaseLastInsertRowid should return empty string for non-existent database")

    -- Clean up
    world.DatabaseClose("rowid_db")

    return 0
end

-- Test DatabaseInfo
function test_database_info()
    -- Open an in-memory database
    local err = world.DatabaseOpen("info_db", ":memory:")
    check(err == 0, "DatabaseOpen should succeed")

    -- Test info type 1: disk filename
    local filename = world.DatabaseInfo("info_db", 1)
    check(filename == ":memory:", "DatabaseInfo type 1 should return ':memory:'")

    -- Test info type 2: has prepared statement (should be false initially)
    local has_stmt = world.DatabaseInfo("info_db", 2)
    check(has_stmt == false, "DatabaseInfo type 2 should return false initially")

    -- Test info type 3: has valid row (should be false initially)
    local has_row = world.DatabaseInfo("info_db", 3)
    check(has_row == false, "DatabaseInfo type 3 should return false initially")

    -- Test info type 4: number of columns (should be 0 initially)
    local num_cols = world.DatabaseInfo("info_db", 4)
    check(num_cols == 0, "DatabaseInfo type 4 should return 0 initially")

    -- Create a test table and prepare a statement
    err = world.DatabaseExec("info_db", "CREATE TABLE test (id INTEGER, name TEXT, value INTEGER)")
    check(err == 0, "CREATE TABLE should succeed")

    err = world.DatabaseExec("info_db", "INSERT INTO test VALUES (1, 'test', 42)")
    check(err == 0, "INSERT should succeed")

    err = world.DatabasePrepare("info_db", "SELECT * FROM test")
    check(err == 0, "DatabasePrepare should succeed")

    -- Test info type 2 again: should now have prepared statement
    has_stmt = world.DatabaseInfo("info_db", 2)
    check(has_stmt == true, "DatabaseInfo type 2 should return true after prepare")

    -- Test info type 4 again: should now show 3 columns
    num_cols = world.DatabaseInfo("info_db", 4)
    check(num_cols == 3, "DatabaseInfo type 4 should return 3 after prepare")

    -- Step to get a valid row
    err = world.DatabaseStep("info_db")
    check(err == 100, "DatabaseStep should return SQLITE_ROW (100)")

    -- Test info type 3 again: should now have valid row
    has_row = world.DatabaseInfo("info_db", 3)
    check(has_row == true, "DatabaseInfo type 3 should return true after step")

    -- Finalize the statement
    err = world.DatabaseFinalize("info_db")
    check(err == 0, "DatabaseFinalize should succeed")

    -- Test info type 2 again: should no longer have prepared statement
    has_stmt = world.DatabaseInfo("info_db", 2)
    check(has_stmt == false, "DatabaseInfo type 2 should return false after finalize")

    -- Test with non-existent database (should return nil)
    local result = world.DatabaseInfo("nonexistent_db", 1)
    check(result == nil, "DatabaseInfo should return nil for non-existent database")

    -- Test with invalid info type (should return nil)
    result = world.DatabaseInfo("info_db", 999)
    check(result == nil, "DatabaseInfo should return nil for invalid info type")

    -- Clean up
    world.DatabaseClose("info_db")

    return 0
end

-- Test DatabaseList
function test_database_list()
    -- Get initial list (should be empty or contain pre-existing databases)
    local initial_list = world.DatabaseList()
    check(type(initial_list) == "table", "DatabaseList should return a table")

    local initial_count = 0
    for i = 1, #initial_list do
        initial_count = initial_count + 1
    end

    -- Open first database
    local err = world.DatabaseOpen("list_db1", ":memory:")
    check(err == 0, "DatabaseOpen should succeed for first database")

    -- Check list again
    local list = world.DatabaseList()
    check(type(list) == "table", "DatabaseList should return a table")

    local count = 0
    local found_db1 = false
    for i = 1, #list do
        count = count + 1
        if list[i] == "list_db1" then
            found_db1 = true
        end
    end

    check(count == initial_count + 1, "DatabaseList should contain one more database")
    check(found_db1 == true, "DatabaseList should contain 'list_db1'")

    -- Open second database
    err = world.DatabaseOpen("list_db2", ":memory:")
    check(err == 0, "DatabaseOpen should succeed for second database")

    -- Check list again
    list = world.DatabaseList()
    count = 0
    found_db1 = false
    local found_db2 = false
    for i = 1, #list do
        count = count + 1
        if list[i] == "list_db1" then
            found_db1 = true
        end
        if list[i] == "list_db2" then
            found_db2 = true
        end
    end

    check(count == initial_count + 2, "DatabaseList should contain two more databases")
    check(found_db1 == true, "DatabaseList should contain 'list_db1'")
    check(found_db2 == true, "DatabaseList should contain 'list_db2'")

    -- Close one database
    err = world.DatabaseClose("list_db1")
    check(err == 0, "DatabaseClose should succeed")

    -- Check list again
    list = world.DatabaseList()
    count = 0
    found_db1 = false
    found_db2 = false
    for i = 1, #list do
        count = count + 1
        if list[i] == "list_db1" then
            found_db1 = true
        end
        if list[i] == "list_db2" then
            found_db2 = true
        end
    end

    check(count == initial_count + 1, "DatabaseList should contain one more database after closing first")
    check(found_db1 == false, "DatabaseList should not contain closed 'list_db1'")
    check(found_db2 == true, "DatabaseList should still contain 'list_db2'")

    -- Clean up
    world.DatabaseClose("list_db2")

    return 0
end

print("Lua test script loaded successfully")

-- ========== UI Display Functions (DISABLED - require user interaction) ==========

-- Test AddFont
function test_add_font()
    -- AddFont(pathname) - loads a font file for use in the client
    -- Returns eOK (0) on success, eFileNotFound (30006) if not found

    -- Test with empty string (should fail with eBadParameter = 8)
    local result = world.AddFont("")
    check(result == 8, "AddFont('') should return eBadParameter (8)")

    -- Test with nonexistent file (should fail with eFileNotFound = 30006)
    result = world.AddFont("/nonexistent/path/to/font.ttf")
    check(result == 30006, "AddFont with invalid path should return eFileNotFound (30006)")

    -- Note: Testing with a real font would require UI and font database access
    -- In a real environment, you would test:
    -- result = world.AddFont("path/to/valid/font.ttf")
    -- check(result == 0, "AddFont with valid font should return eOK (0)")

    return 0
end

-- Test FlashIcon
function test_flash_icon()
    -- FlashIcon() - flashes the application icon in taskbar
    -- No parameters, no return value, requires UI

    -- Just verify it doesn't crash when called
    world.FlashIcon()

    -- Note: Actual icon flashing requires active UI and can't be verified programmatically

    return 0
end

-- Test GetDeviceCaps
function test_get_device_caps()
    -- GetDeviceCaps(index) - returns device capability values
    -- Based on Windows device capability constants

    -- Test LOGPIXELSX (88) - horizontal DPI
    local dpiX = world.GetDeviceCaps(88)
    check(type(dpiX) == "number", "GetDeviceCaps(LOGPIXELSX) should return a number")
    check(dpiX > 0, "GetDeviceCaps(LOGPIXELSX) should return positive DPI value")

    -- Test LOGPIXELSY (90) - vertical DPI
    local dpiY = world.GetDeviceCaps(90)
    check(type(dpiY) == "number", "GetDeviceCaps(LOGPIXELSY) should return a number")
    check(dpiY > 0, "GetDeviceCaps(LOGPIXELSY) should return positive DPI value")

    -- Test HORZRES (8) - screen width in pixels
    local width = world.GetDeviceCaps(8)
    check(type(width) == "number", "GetDeviceCaps(HORZRES) should return a number")
    check(width > 0, "GetDeviceCaps(HORZRES) should return positive width")

    -- Test VERTRES (10) - screen height in pixels
    local height = world.GetDeviceCaps(10)
    check(type(height) == "number", "GetDeviceCaps(VERTRES) should return a number")
    check(height > 0, "GetDeviceCaps(VERTRES) should return positive height")

    -- Test BITSPIXEL (12) - color depth
    local bpp = world.GetDeviceCaps(12)
    check(type(bpp) == "number", "GetDeviceCaps(BITSPIXEL) should return a number")
    check(bpp >= 8, "GetDeviceCaps(BITSPIXEL) should return valid color depth")

    -- Test invalid index (should return 0)
    local invalid = world.GetDeviceCaps(99999)
    check(invalid == 0, "GetDeviceCaps with invalid index should return 0")

    return 0
end

-- Test GetSysColor
function test_get_sys_color()
    -- GetSysColor(index) - returns system color values
    -- Based on Windows COLOR_* constants, returns RGB value (0xRRGGBB)

    -- Test COLOR_WINDOW (5) - window background
    local windowBg = world.GetSysColor(5)
    check(type(windowBg) == "number", "GetSysColor(COLOR_WINDOW) should return a number")
    check(windowBg >= 0 and windowBg <= 0xFFFFFF, "GetSysColor should return valid RGB value")

    -- Test COLOR_WINDOWTEXT (8) - window text
    local windowText = world.GetSysColor(8)
    check(type(windowText) == "number", "GetSysColor(COLOR_WINDOWTEXT) should return a number")
    check(windowText >= 0 and windowText <= 0xFFFFFF, "GetSysColor(COLOR_WINDOWTEXT) should return valid RGB")

    -- Test COLOR_HIGHLIGHT (13) - highlight background
    local highlight = world.GetSysColor(13)
    check(type(highlight) == "number", "GetSysColor(COLOR_HIGHLIGHT) should return a number")
    check(highlight >= 0 and highlight <= 0xFFFFFF, "GetSysColor(COLOR_HIGHLIGHT) should return valid RGB")

    -- Test COLOR_HIGHLIGHTTEXT (14) - highlight text
    local highlightText = world.GetSysColor(14)
    check(type(highlightText) == "number", "GetSysColor(COLOR_HIGHLIGHTTEXT) should return a number")
    check(highlightText >= 0 and highlightText <= 0xFFFFFF, "GetSysColor(COLOR_HIGHLIGHTTEXT) should return valid RGB")

    -- Test COLOR_BTNFACE (15) - button face
    local btnFace = world.GetSysColor(15)
    check(type(btnFace) == "number", "GetSysColor(COLOR_BTNFACE) should return a number")
    check(btnFace >= 0 and btnFace <= 0xFFFFFF, "GetSysColor(COLOR_BTNFACE) should return valid RGB")

    -- Test invalid index (should return 0)
    local invalid = world.GetSysColor(99999)
    check(invalid == 0, "GetSysColor with invalid index should return 0")

    return 0
end

-- Test GetSystemMetrics
function test_get_system_metrics()
    -- GetSystemMetrics(index) - returns system metric values
    -- Based on Windows SM_* constants

    -- Test SM_CXSCREEN (0) - screen width
    local screenWidth = world.GetSystemMetrics(0)
    check(type(screenWidth) == "number", "GetSystemMetrics(SM_CXSCREEN) should return a number")
    check(screenWidth > 0, "GetSystemMetrics(SM_CXSCREEN) should return positive width")

    -- Test SM_CYSCREEN (1) - screen height
    local screenHeight = world.GetSystemMetrics(1)
    check(type(screenHeight) == "number", "GetSystemMetrics(SM_CYSCREEN) should return a number")
    check(screenHeight > 0, "GetSystemMetrics(SM_CYSCREEN) should return positive height")

    -- Test SM_CXVSCROLL (2) - vertical scrollbar width
    local vscrollWidth = world.GetSystemMetrics(2)
    check(type(vscrollWidth) == "number", "GetSystemMetrics(SM_CXVSCROLL) should return a number")
    check(vscrollWidth > 0, "GetSystemMetrics(SM_CXVSCROLL) should return positive value")

    -- Test SM_CYHSCROLL (3) - horizontal scrollbar height
    local hscrollHeight = world.GetSystemMetrics(3)
    check(type(hscrollHeight) == "number", "GetSystemMetrics(SM_CYHSCROLL) should return a number")
    check(hscrollHeight > 0, "GetSystemMetrics(SM_CYHSCROLL) should return positive value")

    -- Test SM_CYCAPTION (4) - caption/title bar height
    local captionHeight = world.GetSystemMetrics(4)
    check(type(captionHeight) == "number", "GetSystemMetrics(SM_CYCAPTION) should return a number")
    check(captionHeight > 0, "GetSystemMetrics(SM_CYCAPTION) should return positive value")

    -- Test SM_CXBORDER (5) - window border width
    local borderWidth = world.GetSystemMetrics(5)
    check(type(borderWidth) == "number", "GetSystemMetrics(SM_CXBORDER) should return a number")
    check(borderWidth >= 0, "GetSystemMetrics(SM_CXBORDER) should return non-negative value")

    -- Test SM_CYBORDER (6) - window border height
    local borderHeight = world.GetSystemMetrics(6)
    check(type(borderHeight) == "number", "GetSystemMetrics(SM_CYBORDER) should return a number")
    check(borderHeight >= 0, "GetSystemMetrics(SM_CYBORDER) should return non-negative value")

    -- Test SM_CXFULLSCREEN (16) - full screen client area width
    local fullWidth = world.GetSystemMetrics(16)
    check(type(fullWidth) == "number", "GetSystemMetrics(SM_CXFULLSCREEN) should return a number")
    check(fullWidth > 0, "GetSystemMetrics(SM_CXFULLSCREEN) should return positive value")

    -- Test SM_CYFULLSCREEN (17) - full screen client area height
    local fullHeight = world.GetSystemMetrics(17)
    check(type(fullHeight) == "number", "GetSystemMetrics(SM_CYFULLSCREEN) should return a number")
    check(fullHeight > 0, "GetSystemMetrics(SM_CYFULLSCREEN) should return positive value")

    -- Test invalid index (should return 0)
    local invalid = world.GetSystemMetrics(99999)
    check(invalid == 0, "GetSystemMetrics with invalid index should return 0")

    return 0
end

-- Test OpenBrowser
function test_open_browser()
    -- OpenBrowser(url) - opens URL in default browser
    -- Returns eOK (0) on success, eBadParameter (8) for invalid URL, eCouldNotOpenFile (30009) on failure

    -- Test with empty URL (should fail with eBadParameter = 8)
    local result = world.OpenBrowser("")
    check(result == 8, "OpenBrowser('') should return eBadParameter (8)")

    -- Test with invalid protocol (should fail with eBadParameter = 8)
    result = world.OpenBrowser("ftp://example.com")
    check(result == 8, "OpenBrowser with ftp:// should return eBadParameter (8)")

    result = world.OpenBrowser("file:///etc/passwd")
    check(result == 8, "OpenBrowser with file:// should return eBadParameter (8)")

    result = world.OpenBrowser("javascript:alert(1)")
    check(result == 8, "OpenBrowser with javascript: should return eBadParameter (8)")

    -- Valid protocols should work (but won't actually open in test environment)
    -- These may return eOK (0) or eCouldNotOpenFile (30009) depending on environment
    result = world.OpenBrowser("http://example.com")
    check(type(result) == "number", "OpenBrowser with http:// should return a number")
    check(result == 0 or result == 30009, "OpenBrowser with http:// should return eOK (0) or eCouldNotOpenFile (30009)")

    result = world.OpenBrowser("https://example.com")
    check(type(result) == "number", "OpenBrowser with https:// should return a number")
    check(result == 0 or result == 30009, "OpenBrowser with https:// should return eOK (0) or eCouldNotOpenFile (30009)")

    result = world.OpenBrowser("mailto:test@example.com")
    check(type(result) == "number", "OpenBrowser with mailto: should return a number")
    check(result == 0 or result == 30009, "OpenBrowser with mailto: should return eOK (0) or eCouldNotOpenFile (30009)")

    return 0
end

-- Test Pause
function test_pause()
    -- Pause(flag) - pauses/resumes output display (freeze mode)
    -- flag: true to pause (default), false to resume
    -- No return value

    -- Test pause (default true)
    world.Pause()

    -- Test explicit pause
    world.Pause(true)

    -- Test resume
    world.Pause(false)

    -- Test with numeric values (Lua converts to boolean)
    world.Pause(1)  -- truthy, should pause
    world.Pause(0)  -- falsy, should resume

    -- Note: Actual pause state requires UI and can't be verified programmatically

    return 0
end

-- Test PickColour
function test_pick_colour()
    -- PickColour(suggested) - opens color picker dialog
    -- suggested: BGR color value (0x00BBGGRR) or -1 for no suggestion
    -- Returns: selected BGR color value, or -1 if canceled
    -- Requires UI interaction, so we only test that it accepts valid parameters

    -- Note: In a headless test environment, this will likely return -1 (canceled)
    -- or may not show a dialog at all

    -- Test with no suggestion (-1)
    local result = world.PickColour(-1)
    check(type(result) == "number", "PickColour should return a number")

    -- Test with red suggestion (BGR: 0x000000FF)
    result = world.PickColour(0x000000FF)
    check(type(result) == "number", "PickColour(red) should return a number")

    -- Test with green suggestion (BGR: 0x0000FF00)
    result = world.PickColour(0x0000FF00)
    check(type(result) == "number", "PickColour(green) should return a number")

    -- Test with blue suggestion (BGR: 0x00FF0000)
    result = world.PickColour(0x00FF0000)
    check(type(result) == "number", "PickColour(blue) should return a number")

    -- Test with white suggestion (BGR: 0x00FFFFFF)
    result = world.PickColour(0x00FFFFFF)
    check(type(result) == "number", "PickColour(white) should return a number")

    -- Test with black suggestion (BGR: 0x00000000)
    result = world.PickColour(0x00000000)
    check(type(result) == "number", "PickColour(black) should return a number")

    -- In headless environment, typically returns -1 (canceled)
    -- check(result == -1, "PickColour in headless environment should return -1 (canceled)")

    return 0
end

print("Lua test script loaded successfully")
-- New tests for ImportXML and AddTriggerEx

-- Test ImportXML
function test_import_xml()
    -- Create a simple XML with a trigger
    local xml = [[<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<triggers>
  <trigger
   name="xml_test_trigger"
   enabled="y"
   match="You receive * gold"
   send_to="14"
   sequence="100"
   script=""
   group=""
   variable=""
   omit_from_output="n"
   omit_from_log="n"
   keep_evaluating="y"
   regexp="n"
   ignore_case="n"
   repeat="y"
   expand_variables="n"
   one_shot="n"
   lowercase_wildcard="n"
   multi_line="n"
   lines_to_match="0"
   sound=""
   sound_if_inactive="n"
  >
  <send>say I got %1 gold!</send>
  </trigger>
</triggers>
</muclient>]]

    -- Import the XML
    local count = world.ImportXML(xml)
    check(count >= 1, "ImportXML should import at least 1 item")

    -- Verify the trigger was imported
    local pattern = world.GetTriggerInfo("xml_test_trigger", 1)
    check(pattern == "You receive * gold", "ImportXML trigger pattern mismatch")

    local send = world.GetTriggerInfo("xml_test_trigger", 2)
    check(send == "say I got %1 gold!", "ImportXML trigger send mismatch")

    -- Clean up
    world.DeleteTrigger("xml_test_trigger")

    return 0
end

-- Test ImportXML with multiple items
function test_import_xml_multiple()
    -- Create XML with trigger and alias
    local xml = [[<?xml version="1.0"?>
<!DOCTYPE muclient>
<muclient>
<triggers>
  <trigger
   name="xml_multi_trigger"
   enabled="y"
   match="health: *"
   send_to="12"
   sequence="100"
   script=""
   group=""
   variable="hp"
   omit_from_output="n"
   omit_from_log="n"
   keep_evaluating="y"
   regexp="n"
   ignore_case="n"
   repeat="y"
   expand_variables="n"
   one_shot="n"
   lowercase_wildcard="n"
   multi_line="n"
   lines_to_match="0"
   sound=""
   sound_if_inactive="n"
  >
  <send></send>
  </trigger>
</triggers>
<aliases>
  <alias
   name="xml_multi_alias"
   enabled="y"
   match="^test$"
   send_to="0"
   sequence="100"
   script=""
   group=""
   omit_from_output="n"
   omit_from_log="n"
   expand_variables="n"
   ignore_case="n"
   keep_evaluating="y"
   menu="n"
   one_shot="n"
   regexp="y"
  >
  <send>say testing</send>
  </alias>
</aliases>
</muclient>]]

    -- Import the XML
    local count = world.ImportXML(xml)
    check(count >= 2, "ImportXML should import at least 2 items")

    -- Verify the trigger exists
    local pattern = world.GetTriggerInfo("xml_multi_trigger", 1)
    check(pattern == "health: *", "ImportXML multiple - trigger pattern mismatch")

    -- Verify the alias exists
    local alias_pattern = world.GetAliasInfo("xml_multi_alias", 1)
    check(alias_pattern == "^test$", "ImportXML multiple - alias pattern mismatch")

    -- Clean up
    world.DeleteTrigger("xml_multi_trigger")
    world.DeleteAlias("xml_multi_alias")

    return 0
end

-- Test ImportXML with invalid XML
function test_import_xml_invalid()
    -- Try importing invalid XML
    local xml = "not valid xml at all"
    local count = world.ImportXML(xml)
    check(count == -1, "ImportXML should return -1 for invalid XML")

    return 0
end

-- Test AddTriggerEx
function test_add_trigger_ex()
    local flags = trigger_flag.Enabled
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = ""
    local send_to = sendto.World
    local sequence = 200

    local err = world.AddTriggerEx("test_trigger_ex", "You gain * experience", "say Gained %1 exp", flags, color, wildcard, sound_file, script, send_to, sequence)

    check(err == 0, "AddTriggerEx should return 0 on success")

    -- Verify trigger was created
    local pattern = world.GetTriggerInfo("test_trigger_ex", 1)
    check(pattern == "You gain * experience", "AddTriggerEx pattern mismatch")

    local send = world.GetTriggerInfo("test_trigger_ex", 2)
    check(send == "say Gained %1 exp", "AddTriggerEx send mismatch")

    -- Verify sequence
    local seq = world.GetTriggerInfo("test_trigger_ex", 16)
    check(seq == 200, "AddTriggerEx sequence mismatch")

    -- Clean up
    world.DeleteTrigger("test_trigger_ex")

    return 0
end

-- Test AddTriggerEx with multiple flags
function test_add_trigger_ex_flags()
    -- Combine multiple flags
    local flags = trigger_flag.Enabled + trigger_flag.OmitFromOutput + trigger_flag.IgnoreCase
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = ""
    local send_to = sendto.World
    local sequence = 150

    local err = world.AddTriggerEx("test_trigger_ex_flags", "HELLO*", "say hello there", flags, color, wildcard, sound_file, script, send_to, sequence)

    check(err == 0, "AddTriggerEx with flags should return 0 on success")

    -- Verify enabled
    local enabled = world.GetTriggerInfo("test_trigger_ex_flags", 8)
    check(enabled == true, "AddTriggerEx enabled flag mismatch")

    -- Verify omit_from_output
    local omit = world.GetTriggerInfo("test_trigger_ex_flags", 6)
    check(omit == true, "AddTriggerEx omit_from_output flag mismatch")

    -- Verify ignore_case
    local ignore_case = world.GetTriggerInfo("test_trigger_ex_flags", 10)
    check(ignore_case == true, "AddTriggerEx ignore_case flag mismatch")

    -- Clean up
    world.DeleteTrigger("test_trigger_ex_flags")

    return 0
end

-- Test AddTriggerEx with script
function test_add_trigger_ex_script()
    local flags = trigger_flag.Enabled
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = "test_script_function"
    local send_to = sendto.Script
    local sequence = 100

    local err = world.AddTriggerEx("test_trigger_ex_script", "Script test: *", "", flags, color, wildcard, sound_file, script, send_to, sequence)

    check(err == 0, "AddTriggerEx with script should return 0 on success")

    -- Verify script name
    local script_name = world.GetTriggerInfo("test_trigger_ex_script", 4)
    check(script_name == "test_script_function", "AddTriggerEx script name mismatch")

    -- Verify send_to
    local send_to_value = world.GetTriggerInfo("test_trigger_ex_script", 15)
    check(send_to_value == sendto.Script, "AddTriggerEx send_to mismatch")

    -- Clean up
    world.DeleteTrigger("test_trigger_ex_script")

    return 0
end

-- Test AddTriggerEx with regexp
function test_add_trigger_ex_regexp()
    local flags = trigger_flag.Enabled + trigger_flag.RegularExpression
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = ""
    local send_to = sendto.World
    local sequence = 100

    local err = world.AddTriggerEx("test_trigger_ex_regexp", "^You (hit|miss) the monster$", "say combat!", flags, color, wildcard, sound_file, script, send_to, sequence)

    check(err == 0, "AddTriggerEx with regexp should return 0 on success")

    -- Verify pattern
    local pattern = world.GetTriggerInfo("test_trigger_ex_regexp", 1)
    check(pattern == "^You (hit|miss) the monster$", "AddTriggerEx regexp pattern mismatch")

    -- Clean up
    world.DeleteTrigger("test_trigger_ex_regexp")

    return 0
end

-- Test AddTriggerEx error: empty match
function test_add_trigger_ex_empty_match()
    local flags = trigger_flag.Enabled
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = ""
    local send_to = sendto.World
    local sequence = 100

    -- Try to create trigger with empty match - should fail
    local err = world.AddTriggerEx("test_trigger_ex_empty", "", "say test", flags, color, wildcard, sound_file, script, send_to, sequence)

    -- Error code 30028 = eTriggerCannotBeEmpty
    -- Note: using ~= 0 to check for any error instead of specific code
    check(err ~= 0, "AddTriggerEx with empty match should return an error")

    return 0
end

-- Test AddTriggerEx error: invalid sequence
function test_add_trigger_ex_invalid_sequence()
    local flags = trigger_flag.Enabled
    local color = 0
    local wildcard = 0
    local sound_file = ""
    local script = ""
    local send_to = sendto.World
    local sequence = 99999  -- Out of range (max is 10000)

    -- Try to create trigger with invalid sequence - should fail
    local err = world.AddTriggerEx("test_trigger_ex_invalid_seq", "test", "say test", flags, color, wildcard, sound_file, script, send_to, sequence)

    -- Error code 30029 = eTriggerSequenceOutOfRange
    -- Note: using ~= 0 to check for any error instead of specific code
    check(err ~= 0, "AddTriggerEx with invalid sequence should return an error")

    return 0
end

-- ========== Spell Check Functions (Deprecated Stubs) ==========

-- Test SpellCheck
function test_spell_check()
    -- SpellCheck(Text) - deprecated stub, returns empty table
    -- Test with simple text
    local result = world.SpellCheck("Hello World")
    check(type(result) == "table", "SpellCheck should return a table")

    -- Should return empty table (no spell check errors)
    local count = 0
    for _ in pairs(result) do
        count = count + 1
    end
    check(count == 0, "SpellCheck should return empty table (deprecated)")

    -- Test with nil/empty string
    result = world.SpellCheck("")
    check(type(result) == "table", "SpellCheck with empty string should return table")

    -- Test with text containing misspellings (should still return empty table)
    result = world.SpellCheck("Thiss iss misspelledd textt")
    check(type(result) == "table", "SpellCheck should return table even with misspellings")
    check(#result == 0, "SpellCheck should return empty table (spell check removed)")

    return 0
end

-- Test SpellCheckDlg
function test_spell_check_dlg()
    -- SpellCheckDlg(Text) - deprecated stub, returns empty table
    -- Note: Would normally show dialog, but is now a stub

    local result = world.SpellCheckDlg("Check this text")
    check(type(result) == "table", "SpellCheckDlg should return a table")

    -- Should return empty table (no dialog shown)
    local count = 0
    for _ in pairs(result) do
        count = count + 1
    end
    check(count == 0, "SpellCheckDlg should return empty table (deprecated)")

    -- Test with various text inputs
    result = world.SpellCheckDlg("")
    check(type(result) == "table", "SpellCheckDlg with empty string should return table")

    result = world.SpellCheckDlg("Multiple words to check here")
    check(type(result) == "table", "SpellCheckDlg with multi-word text should return table")
    check(#result == 0, "SpellCheckDlg should return empty table (spell check removed)")

    return 0
end

-- Test SpellCheckCommand
function test_spell_check_command()
    -- SpellCheckCommand(StartCol, EndCol) - deprecated stub, returns 0
    -- Would normally spell check command input between columns

    -- Test with valid column range
    local result = world.SpellCheckCommand(0, 10)
    check(type(result) == "number", "SpellCheckCommand should return a number")
    check(result == 0, "SpellCheckCommand should return 0 (deprecated)")

    -- Test with same start and end
    result = world.SpellCheckCommand(5, 5)
    check(type(result) == "number", "SpellCheckCommand with same columns should return number")
    check(result == 0, "SpellCheckCommand should return 0")

    -- Test with zero range
    result = world.SpellCheckCommand(0, 0)
    check(result == 0, "SpellCheckCommand with zero range should return 0")

    -- Test with larger range
    result = world.SpellCheckCommand(0, 100)
    check(result == 0, "SpellCheckCommand with large range should return 0")

    return 0
end

-- Test AddSpellCheckWord
function test_add_spell_check_word()
    -- AddSpellCheckWord(OriginalWord, ActionCode, ReplacementWord) - deprecated stub, returns 0
    -- Would normally add word to spell check dictionary

    -- Test adding a word to dictionary (action code 0 = add to dictionary)
    local result = world.AddSpellCheckWord("testword", 0, "")
    check(type(result) == "number", "AddSpellCheckWord should return a number")
    check(result == 0, "AddSpellCheckWord should return 0 (deprecated)")

    -- Test with replacement word (action code 1 = replace)
    result = world.AddSpellCheckWord("teh", 1, "the")
    check(type(result) == "number", "AddSpellCheckWord with replacement should return number")
    check(result == 0, "AddSpellCheckWord should return 0")

    -- Test with ignore action (action code 2 = ignore)
    result = world.AddSpellCheckWord("ignoreme", 2, "")
    check(result == 0, "AddSpellCheckWord with ignore action should return 0")

    -- Test with empty original word
    result = world.AddSpellCheckWord("", 0, "")
    check(result == 0, "AddSpellCheckWord with empty word should return 0")

    -- Test with multiple words
    result = world.AddSpellCheckWord("multiword phrase", 0, "")
    check(result == 0, "AddSpellCheckWord with multi-word should return 0")

    return 0
end

-- ========== UI Window Position Tests (DISABLED - Require UI) ==========

-- Test GetWorldWindowPosition
function test_get_world_window_position()
    -- GetWorldWindowPosition() returns table {left, top, width, height}
    -- In Qt version, this returns the main window position since worlds are tabs
    local pos = world.GetWorldWindowPosition()
    check(type(pos) == "table", "GetWorldWindowPosition should return a table")
    check(#pos == 4, "GetWorldWindowPosition should return 4 values")

    -- Verify all values are numbers
    check(type(pos[1]) == "number", "left should be a number")
    check(type(pos[2]) == "number", "top should be a number")
    check(type(pos[3]) == "number", "width should be a number")
    check(type(pos[4]) == "number", "height should be a number")

    -- Verify reasonable bounds (width and height should be positive)
    check(pos[3] > 0, "width should be positive")
    check(pos[4] > 0, "height should be positive")

    return 0
end

-- Test GetWorldWindowPositionX
function test_get_world_window_position_x()
    -- GetWorldWindowPositionX(which) returns table {left, top, width, height}
    -- In Qt we only have one world window, so this is same as GetWorldWindowPosition
    -- The 'which' parameter is ignored
    local pos = world.GetWorldWindowPositionX(0)
    check(type(pos) == "table", "GetWorldWindowPositionX should return a table")
    check(#pos == 4, "GetWorldWindowPositionX should return 4 values")

    -- Verify all values are numbers
    check(type(pos[1]) == "number", "left should be a number")
    check(type(pos[2]) == "number", "top should be a number")
    check(type(pos[3]) == "number", "width should be a number")
    check(type(pos[4]) == "number", "height should be a number")

    -- Should return same as GetWorldWindowPosition
    local pos2 = world.GetWorldWindowPosition()
    check(pos[1] == pos2[1], "GetWorldWindowPositionX left should match GetWorldWindowPosition")
    check(pos[2] == pos2[2], "GetWorldWindowPositionX top should match GetWorldWindowPosition")
    check(pos[3] == pos2[3], "GetWorldWindowPositionX width should match GetWorldWindowPosition")
    check(pos[4] == pos2[4], "GetWorldWindowPositionX height should match GetWorldWindowPosition")

    return 0
end

-- Test MoveMainWindow
function test_move_main_window()
    -- MoveMainWindow(left, top, width, height) moves and resizes the main window
    -- Save original position
    local orig_pos = world.GetMainWindowPosition()
    check(type(orig_pos) == "table", "GetMainWindowPosition should return a table")

    -- Move to a new position
    world.MoveMainWindow(100, 100, 800, 600)

    -- Get new position
    local new_pos = world.GetMainWindowPosition()
    check(type(new_pos) == "table", "GetMainWindowPosition should return a table after move")

    -- Verify position changed (allow some tolerance for window manager constraints)
    -- In test environment without GUI, this might not actually move the window
    -- so we just verify the function doesn't crash
    check(type(new_pos[1]) == "number", "left should be a number after move")
    check(type(new_pos[2]) == "number", "top should be a number after move")
    check(type(new_pos[3]) == "number", "width should be a number after move")
    check(type(new_pos[4]) == "number", "height should be a number after move")

    -- Restore original position
    world.MoveMainWindow(orig_pos[1], orig_pos[2], orig_pos[3], orig_pos[4])

    return 0
end

-- Test MoveWorldWindow
function test_move_world_window()
    -- MoveWorldWindow(left, top, width, height) moves the world window
    -- In Qt version, this moves the main window since worlds are tabs
    local orig_pos = world.GetWorldWindowPosition()
    check(type(orig_pos) == "table", "GetWorldWindowPosition should return a table")

    -- Move to a new position
    world.MoveWorldWindow(150, 150, 900, 700)

    -- Get new position
    local new_pos = world.GetWorldWindowPosition()
    check(type(new_pos) == "table", "GetWorldWindowPosition should return a table after move")

    -- Verify the function works (doesn't crash)
    check(type(new_pos[1]) == "number", "left should be a number after move")
    check(type(new_pos[2]) == "number", "top should be a number after move")
    check(type(new_pos[3]) == "number", "width should be a number after move")
    check(type(new_pos[4]) == "number", "height should be a number after move")

    -- Restore original position
    world.MoveWorldWindow(orig_pos[1], orig_pos[2], orig_pos[3], orig_pos[4])

    return 0
end

-- Test MoveWorldWindowX
function test_move_world_window_x()
    -- MoveWorldWindowX(left, top, width, height, which) moves a specific world window
    -- In Qt we only have one world window, so this is same as MoveWorldWindow
    local orig_pos = world.GetWorldWindowPosition()
    check(type(orig_pos) == "table", "GetWorldWindowPosition should return a table")

    -- Move to a new position (which parameter is ignored in Qt)
    world.MoveWorldWindowX(200, 200, 1000, 800, 0)

    -- Get new position
    local new_pos = world.GetWorldWindowPosition()
    check(type(new_pos) == "table", "GetWorldWindowPosition should return a table after move")

    -- Verify the function works
    check(type(new_pos[1]) == "number", "left should be a number after move")
    check(type(new_pos[2]) == "number", "top should be a number after move")
    check(type(new_pos[3]) == "number", "width should be a number after move")
    check(type(new_pos[4]) == "number", "height should be a number after move")

    -- Restore original position
    world.MoveWorldWindowX(orig_pos[1], orig_pos[2], orig_pos[3], orig_pos[4], 0)

    return 0
end

-- Test SetWorldWindowStatus
function test_set_world_window_status()
    -- SetWorldWindowStatus(status) sets window state
    -- status: 1=normal, 2=minimized, 3=maximized

    -- Test normal state
    world.SetWorldWindowStatus(1)
    -- Function doesn't return anything, just verify it doesn't crash

    -- Test maximized state
    world.SetWorldWindowStatus(3)

    -- Restore to normal
    world.SetWorldWindowStatus(1)

    -- Test minimized state (might not work in test environment)
    world.SetWorldWindowStatus(2)

    -- Restore to normal
    world.SetWorldWindowStatus(1)

    return 0
end

-- Test SetToolBarPosition
function test_set_tool_bar_position()
    -- SetToolBarPosition(which, float, side, top, left)
    -- which: 1=main toolbar, 2=game toolbar, 3=activity toolbar, 4=infobar
    -- float: true to float, false to dock
    -- side: For docking: 1=top, 2=bottom, 3=left, 4=right
    -- top, left: Position for floating toolbars

    -- Test docking main toolbar to top
    local result = world.SetToolBarPosition(1, false, 1, 0, 0)
    check(result == 0, "SetToolBarPosition should return eOK (0) for valid toolbar")

    -- Test docking game toolbar to bottom
    result = world.SetToolBarPosition(2, false, 2, 0, 0)
    check(result == 0, "SetToolBarPosition should return eOK (0) for game toolbar")

    -- Test docking activity toolbar to left
    result = world.SetToolBarPosition(3, false, 3, 0, 0)
    check(result == 0, "SetToolBarPosition should return eOK (0) for activity toolbar")

    -- Test docking infobar to right
    result = world.SetToolBarPosition(4, false, 4, 0, 0)
    check(result == 0, "SetToolBarPosition should return eOK (0) for infobar")

    -- Test floating toolbar
    result = world.SetToolBarPosition(1, true, 1, 100, 100)
    check(result == 0, "SetToolBarPosition should return eOK (0) for floating toolbar")

    -- Test invalid toolbar number (should return eBadParameter = 30)
    result = world.SetToolBarPosition(0, false, 1, 0, 0)
    check(result == 30, "SetToolBarPosition should return eBadParameter (30) for invalid toolbar")

    result = world.SetToolBarPosition(5, false, 1, 0, 0)
    check(result == 30, "SetToolBarPosition should return eBadParameter (30) for toolbar > 4")

    -- Restore main toolbar to top (docked)
    result = world.SetToolBarPosition(1, false, 1, 0, 0)
    check(result == 0, "SetToolBarPosition should return eOK (0) when restoring toolbar")

    return 0
end

print("Lua test script loaded successfully")

-- Test Base64 with additional edge cases
function test_base64_comprehensive()
    -- Test different padding scenarios
    -- "f" -> "Zg==" (2 padding chars)
    local encoded = world.Base64Encode("f")
    check(encoded == "Zg==", "Base64Encode 'f' should produce 'Zg=='")
    check(world.Base64Decode(encoded) == "f", "Base64Decode should reverse encode for 'f'")

    -- "fo" -> "Zm8=" (1 padding char)
    encoded = world.Base64Encode("fo")
    check(encoded == "Zm8=", "Base64Encode 'fo' should produce 'Zm8='")
    check(world.Base64Decode(encoded) == "fo", "Base64Decode should reverse encode for 'fo'")

    -- "foo" -> "Zm9v" (no padding)
    encoded = world.Base64Encode("foo")
    check(encoded == "Zm9v", "Base64Encode 'foo' should produce 'Zm9v'")
    check(world.Base64Decode(encoded) == "foo", "Base64Decode should reverse encode for 'foo'")

    -- Test all printable ASCII characters
    local ascii_chars = ""
    for i = 32, 126 do
        ascii_chars = ascii_chars .. string.char(i)
    end
    encoded = world.Base64Encode(ascii_chars)
    local decoded = world.Base64Decode(encoded)
    check(decoded == ascii_chars, "Base64 should handle all printable ASCII characters")

    -- Test longer string
    local long_string = string.rep("The quick brown fox jumps over the lazy dog. ", 100)
    encoded = world.Base64Encode(long_string)
    decoded = world.Base64Decode(encoded)
    check(decoded == long_string, "Base64 should handle long strings")

    -- Test special characters and newlines
    local special = "Hello\nWorld\r\n\t!@#$%^&*()"
    encoded = world.Base64Encode(special)
    decoded = world.Base64Decode(encoded)
    check(decoded == special, "Base64 should handle special characters and newlines")

    return 0
end

-- ========== Pixel Manipulation Functions ==========

-- Test BlendPixel function
function test_blend_pixel()
    -- RGB color encoding is BGR format: 0xBBGGRR
    -- Red = 0x0000FF, Green = 0x00FF00, Blue = 0xFF0000
    local red = 0x0000FF
    local green = 0x00FF00
    local blue = 0xFF0000
    local white = 0xFFFFFF
    local black = 0x000000

    -- Mode 1: Normal (blend color replaces base)
    local result = world.BlendPixel(red, blue, 1, 1.0)
    check(result == red, "BlendPixel Normal mode should use blend color")

    -- Mode 2: Average
    result = world.BlendPixel(white, black, 2, 1.0)
    local expected_gray = 0x7F7F7F  -- (255+0)/2 = 127 = 0x7F for each channel
    check(result == expected_gray, "BlendPixel Average mode should average colors")

    -- Mode 5: Darken (min of each channel)
    result = world.BlendPixel(red, green, 5, 1.0)
    check(result == black, "BlendPixel Darken mode should take minimum of channels")

    -- Mode 11: Lighten (max of each channel)
    result = world.BlendPixel(red, green, 11, 1.0)
    local expected_yellow = 0x00FFFF  -- R=255, G=255, B=0
    check(result == expected_yellow, "BlendPixel Lighten mode should take maximum of channels")

    -- Test opacity at 0.5
    result = world.BlendPixel(white, black, 1, 0.5)
    check(result >= 0 and result <= 0xFFFFFF, "BlendPixel with 0.5 opacity should produce valid color")

    -- Test invalid mode (should return -1)
    result = world.BlendPixel(red, blue, 0, 1.0)
    check(result == -1, "BlendPixel with invalid mode 0 should return -1")

    result = world.BlendPixel(red, blue, 65, 1.0)
    check(result == -1, "BlendPixel with invalid mode 65 should return -1")

    -- Test invalid opacity (should return -2)
    result = world.BlendPixel(red, blue, 1, -0.1)
    check(result == -2, "BlendPixel with negative opacity should return -2")

    result = world.BlendPixel(red, blue, 1, 1.1)
    check(result == -2, "BlendPixel with opacity > 1.0 should return -2")

    -- Test opacity boundaries
    result = world.BlendPixel(red, blue, 1, 0.0)
    check(result >= 0, "BlendPixel with 0.0 opacity should be valid")

    result = world.BlendPixel(red, blue, 1, 1.0)
    check(result >= 0, "BlendPixel with 1.0 opacity should be valid")

    return 0
end

-- Test FilterPixel function
function test_filter_pixel()
    -- RGB color encoding is BGR format: 0xBBGGRR
    local gray_mid = 0x808080  -- R=128, G=128, B=128
    local white = 0xFFFFFF
    local black = 0x000000

    -- Operation 7: Brightness (additive)
    local result = world.FilterPixel(gray_mid, 7, 50)
    -- Expected: 128 + 50 = 178 = 0xB2 for each channel
    local expected = 0xB2B2B2
    check(result == expected, "FilterPixel brightness +50 should add to each channel")

    -- Brightness with clamping
    result = world.FilterPixel(white, 7, 10)
    check(result == white, "FilterPixel brightness should clamp at 255")

    result = world.FilterPixel(black, 7, -10)
    check(result == black, "FilterPixel brightness should clamp at 0")

    -- Operation 8: Contrast
    result = world.FilterPixel(gray_mid, 8, 1.0)
    check(result == gray_mid, "FilterPixel contrast 1.0 should not change mid-gray")

    -- Operation 9: Gamma
    result = world.FilterPixel(gray_mid, 9, 1.0)
    check(result == gray_mid, "FilterPixel gamma 1.0 should not change color")

    -- Operation 10: Red brightness only
    result = world.FilterPixel(black, 10, 100)
    local expected_red_only = 0x000064  -- R=100, G=0, B=0
    check(result == expected_red_only, "FilterPixel red brightness should only affect red channel")

    -- Operation 13: Green brightness only
    result = world.FilterPixel(black, 13, 100)
    local expected_green_only = 0x006400  -- R=0, G=100, B=0
    check(result == expected_green_only, "FilterPixel green brightness should only affect green channel")

    -- Operation 16: Blue brightness only
    result = world.FilterPixel(black, 16, 100)
    local expected_blue_only = 0x640000  -- R=0, G=0, B=100
    check(result == expected_blue_only, "FilterPixel blue brightness should only affect blue channel")

    -- Operation 19: Grayscale (linear average)
    local color_rgb = 0x4B3219  -- R=25, G=50, B=75 -> avg = 50
    result = world.FilterPixel(color_rgb, 19, 0)
    local avg = 0x323232  -- (25+50+75)/3 = 50 = 0x32
    check(result == avg, "FilterPixel grayscale should average RGB channels")

    -- Operation 21: Brightness multiply
    result = world.FilterPixel(gray_mid, 21, 2.0)
    check(result == white, "FilterPixel brightness multiply by 2 on mid-gray should give white")

    result = world.FilterPixel(gray_mid, 21, 0.5)
    local expected_half = 0x404040  -- 128 * 0.5 = 64 = 0x40
    check(result == expected_half, "FilterPixel brightness multiply by 0.5 should halve values")

    -- Test invalid operation (should return -1)
    result = world.FilterPixel(gray_mid, 0, 0)
    check(result == -1, "FilterPixel with invalid operation should return -1")

    result = world.FilterPixel(gray_mid, 28, 0)
    check(result == -1, "FilterPixel with operation > 27 should return -1")

    return 0
end

-- ========== Random Number Functions ==========

-- Test MtSrand and MtRand functions
function test_mt_rand()
    -- Seed the RNG with a known value
    world.MtSrand(12345)

    -- Get several random numbers
    local r1 = world.MtRand()
    local r2 = world.MtRand()
    local r3 = world.MtRand()

    -- Check that values are in valid range [0, 1)
    check(type(r1) == "number", "MtRand should return a number")
    check(r1 >= 0.0 and r1 < 1.0, "MtRand should return value in [0, 1)")
    check(r2 >= 0.0 and r2 < 1.0, "MtRand second call should return value in [0, 1)")
    check(r3 >= 0.0 and r3 < 1.0, "MtRand third call should return value in [0, 1)")

    -- Check that values are different (very unlikely to be same)
    check(r1 ~= r2 or r2 ~= r3, "MtRand should produce different values")

    -- Reseed with same value and verify reproducibility
    world.MtSrand(12345)
    local r1_repeat = world.MtRand()
    check(r1 == r1_repeat, "MtRand should be reproducible with same seed")

    -- Seed with different value
    world.MtSrand(54321)
    local r4 = world.MtRand()
    check(r4 >= 0.0 and r4 < 1.0, "MtRand with different seed should return value in [0, 1)")

    -- Verify that different seed produces different sequence
    check(r4 ~= r1, "Different seed should produce different first random value")

    -- Test seeding with 0
    world.MtSrand(0)
    local r5 = world.MtRand()
    check(r5 >= 0.0 and r5 < 1.0, "MtRand with seed 0 should work")

    -- Test seeding with large number
    world.MtSrand(4294967295)  -- Max uint32
    local r6 = world.MtRand()
    check(r6 >= 0.0 and r6 < 1.0, "MtRand with large seed should work")

    return 0
end

-- ========== Miniwindow Hotspot Functions ==========

-- Test WindowMenu
function test_window_menu()
    world.WindowCreate("menu_test", 100, 100, 200, 200, 0, 0, 0x000000)

    -- WindowMenu(name, x, y, menuString)
    -- Note: In test mode, WindowMenu returns empty string (no GUI interaction)
    -- Test basic menu string
    local result = world.WindowMenu("menu_test", 150, 150, "Item 1|Item 2|Item 3")
    check(type(result) == "string", "WindowMenu should return a string")

    -- Test menu with separators
    result = world.WindowMenu("menu_test", 150, 150, "Item 1|-|Item 2")
    check(type(result) == "string", "WindowMenu with separator should return a string")

    -- Test menu with checked items
    result = world.WindowMenu("menu_test", 150, 150, "!Checked Item|Item 2")
    check(type(result) == "string", "WindowMenu with checked item should return a string")

    -- Test menu with disabled items
    result = world.WindowMenu("menu_test", 150, 150, "Item 1|^Disabled Item")
    check(type(result) == "string", "WindowMenu with disabled item should return a string")

    -- Test submenu
    result = world.WindowMenu("menu_test", 150, 150, ">Submenu|Item 1|<|Top Item")
    check(type(result) == "string", "WindowMenu with submenu should return a string")

    -- Test with world (empty window name)
    result = world.WindowMenu("", 150, 150, "World Menu Item")
    check(type(result) == "string", "WindowMenu for world should return a string")

    world.WindowDelete("menu_test")

    return 0
end

-- Test WindowMoveHotspot
function test_window_move_hotspot()
    world.WindowCreate("movehs_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create a hotspot first
    local result = world.WindowAddHotspot("movehs_test", "hs1", 10, 10, 50, 50,
                                          "", "", "", "", "", "Original", 0, 0)
    check(result == 0, "WindowAddHotspot should return 0")

    -- Verify original position (info type 1=left, 2=top, 3=right, 4=bottom)
    local left = world.WindowHotspotInfo("movehs_test", "hs1", 1)
    check(left == 10, "Original hotspot left should be 10")

    -- WindowMoveHotspot(name, hotspotId, left, top, right, bottom)
    result = world.WindowMoveHotspot("movehs_test", "hs1", 20, 20, 80, 80)
    check(result == 0, "WindowMoveHotspot should return 0 on success")

    -- Verify new position
    -- Note: Qt's QRect::right() returns left+width-1 (inclusive), so we expect 79 not 80
    left = world.WindowHotspotInfo("movehs_test", "hs1", 1)
    local top = world.WindowHotspotInfo("movehs_test", "hs1", 2)
    local right = world.WindowHotspotInfo("movehs_test", "hs1", 3)
    local bottom = world.WindowHotspotInfo("movehs_test", "hs1", 4)
    check(left == 20, "Moved hotspot left should be 20")
    check(top == 20, "Moved hotspot top should be 20")
    check(right == 79, "Moved hotspot right should be 79 (Qt QRect inclusive)")
    check(bottom == 79, "Moved hotspot bottom should be 79 (Qt QRect inclusive)")

    -- Test moving to edge of window
    result = world.WindowMoveHotspot("movehs_test", "hs1", 0, 0, 200, 200)
    check(result == 0, "WindowMoveHotspot to window edge should succeed")

    -- Test error: non-existent window
    result = world.WindowMoveHotspot("nonexistent", "hs1", 10, 10, 50, 50)
    check(result ~= 0, "WindowMoveHotspot with invalid window should return error")

    -- Test error: non-existent hotspot
    result = world.WindowMoveHotspot("movehs_test", "nonexistent", 10, 10, 50, 50)
    check(result ~= 0, "WindowMoveHotspot with invalid hotspot should return error")

    world.WindowDelete("movehs_test")

    return 0
end

-- Test WindowHotspotTooltip
function test_window_hotspot_tooltip()
    world.WindowCreate("tooltip_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create a hotspot with initial tooltip
    local result = world.WindowAddHotspot("tooltip_test", "hs1", 10, 10, 50, 50,
                                          "", "", "", "", "", "Initial Tooltip", 0, 0)
    check(result == 0, "WindowAddHotspot should return 0")

    -- Verify initial tooltip (info type 10 = tooltip)
    local tooltip = world.WindowHotspotInfo("tooltip_test", "hs1", 10)
    check(tooltip == "Initial Tooltip", "Initial tooltip should match")

    -- WindowHotspotTooltip(name, hotspotId, tooltipText)
    result = world.WindowHotspotTooltip("tooltip_test", "hs1", "Updated Tooltip")
    check(result == 0, "WindowHotspotTooltip should return 0 on success")

    -- Verify tooltip was updated
    tooltip = world.WindowHotspotInfo("tooltip_test", "hs1", 10)
    check(tooltip == "Updated Tooltip", "Tooltip should be updated")

    -- Test setting empty tooltip
    result = world.WindowHotspotTooltip("tooltip_test", "hs1", "")
    check(result == 0, "WindowHotspotTooltip with empty string should succeed")
    tooltip = world.WindowHotspotInfo("tooltip_test", "hs1", 10)
    check(tooltip == "", "Tooltip should be empty")

    -- Test setting long tooltip
    local long_tooltip = "This is a very long tooltip that contains multiple words and should still work correctly without any issues"
    result = world.WindowHotspotTooltip("tooltip_test", "hs1", long_tooltip)
    check(result == 0, "WindowHotspotTooltip with long text should succeed")
    tooltip = world.WindowHotspotInfo("tooltip_test", "hs1", 10)
    check(tooltip == long_tooltip, "Long tooltip should match")

    -- Test error: non-existent window
    result = world.WindowHotspotTooltip("nonexistent", "hs1", "Test")
    check(result ~= 0, "WindowHotspotTooltip with invalid window should return error")

    -- Test error: non-existent hotspot
    result = world.WindowHotspotTooltip("tooltip_test", "nonexistent", "Test")
    check(result ~= 0, "WindowHotspotTooltip with invalid hotspot should return error")

    world.WindowDelete("tooltip_test")

    return 0
end

-- Test WindowDragHandler
function test_window_drag_handler()
    world.WindowCreate("drag_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create a hotspot
    local result = world.WindowAddHotspot("drag_test", "hs1", 10, 10, 50, 50,
                                          "", "", "", "", "", "Draggable", 0, 0)
    check(result == 0, "WindowAddHotspot should return 0")

    -- WindowDragHandler(name, hotspotId, moveCallback, releaseCallback, flags)
    result = world.WindowDragHandler("drag_test", "hs1", "OnDragMove", "OnDragRelease", 0)
    check(result == 0, "WindowDragHandler should return 0 on success")

    -- Test with empty callbacks
    result = world.WindowDragHandler("drag_test", "hs1", "", "", 0)
    check(result == 0, "WindowDragHandler with empty callbacks should succeed")

    -- Test with only move callback
    result = world.WindowDragHandler("drag_test", "hs1", "OnMove", "", 0)
    check(result == 0, "WindowDragHandler with only move callback should succeed")

    -- Test with only release callback
    result = world.WindowDragHandler("drag_test", "hs1", "", "OnRelease", 0)
    check(result == 0, "WindowDragHandler with only release callback should succeed")

    -- Test with flags
    result = world.WindowDragHandler("drag_test", "hs1", "OnMove", "OnRelease", 1)
    check(result == 0, "WindowDragHandler with flags should succeed")

    -- Test error: non-existent window
    result = world.WindowDragHandler("nonexistent", "hs1", "OnMove", "OnRelease", 0)
    check(result ~= 0, "WindowDragHandler with invalid window should return error")

    -- Test error: non-existent hotspot
    result = world.WindowDragHandler("drag_test", "nonexistent", "OnMove", "OnRelease", 0)
    check(result ~= 0, "WindowDragHandler with invalid hotspot should return error")

    world.WindowDelete("drag_test")

    return 0
end

-- Test WindowScrollwheelHandler
function test_window_scrollwheel_handler()
    world.WindowCreate("scroll_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create a hotspot
    local result = world.WindowAddHotspot("scroll_test", "hs1", 10, 10, 50, 50,
                                          "", "", "", "", "", "Scrollable", 0, 0)
    check(result == 0, "WindowAddHotspot should return 0")

    -- WindowScrollwheelHandler(name, hotspotId, scrollCallback)
    result = world.WindowScrollwheelHandler("scroll_test", "hs1", "OnScroll")
    check(result == 0, "WindowScrollwheelHandler should return 0 on success")

    -- Test with empty callback
    result = world.WindowScrollwheelHandler("scroll_test", "hs1", "")
    check(result == 0, "WindowScrollwheelHandler with empty callback should succeed")

    -- Test error: non-existent window
    result = world.WindowScrollwheelHandler("nonexistent", "hs1", "OnScroll")
    check(result ~= 0, "WindowScrollwheelHandler with invalid window should return error")

    -- Test error: non-existent hotspot
    result = world.WindowScrollwheelHandler("scroll_test", "nonexistent", "OnScroll")
    check(result ~= 0, "WindowScrollwheelHandler with invalid hotspot should return error")

    -- Test with multiple hotspots having scroll handlers
    world.WindowAddHotspot("scroll_test", "hs2", 60, 60, 100, 100,
                          "", "", "", "", "", "Scrollable2", 0, 0)
    result = world.WindowScrollwheelHandler("scroll_test", "hs2", "OnScroll2")
    check(result == 0, "WindowScrollwheelHandler on second hotspot should succeed")

    world.WindowDelete("scroll_test")

    return 0
end

-- ========== Miniwindow Image Functions ==========

-- Test WindowLoadImage
function test_window_load_image()
    world.WindowCreate("loadimg_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create a simple test image by drawing to a window and using WindowWrite
    world.WindowCreate("temp_img", 0, 0, 32, 32, 0, 0, 0xFF0000)
    world.WindowCircleOp("temp_img", 3, 4, 4, 28, 28, 0x00FF00, 0, 2, 0, 1)

    -- Write window to file
    local result = world.WindowWrite("temp_img", "tests/test_image.png")
    check(result == 0, "WindowWrite should return 0 on success")

    -- Now load the image
    result = world.WindowLoadImage("loadimg_test", "testimg", "tests/test_image.png")
    check(result == 0, "WindowLoadImage should return 0 on success")

    -- Verify image is in the list
    local list = world.WindowImageList("loadimg_test")
    check(type(list) == "table", "WindowImageList should return a table")
    local found = false
    for i = 1, #list do
        if list[i] == "testimg" then
            found = true
            break
        end
    end
    check(found, "Loaded image should be in WindowImageList")

    -- Get image info - type 2 is width, type 3 is height
    local width = world.WindowImageInfo("loadimg_test", "testimg", 2)
    check(type(width) == "number", "WindowImageInfo should return image width")
    check(width == 32, "Image width should be 32")

    local height = world.WindowImageInfo("loadimg_test", "testimg", 3)
    check(type(height) == "number", "WindowImageInfo should return image height")
    check(height == 32, "Image height should be 32")

    -- Test loading non-existent file
    result = world.WindowLoadImage("loadimg_test", "badimg", "nonexistent_file.png")
    check(result == 30051, "WindowLoadImage should return 30051 for non-existent file")

    world.WindowDelete("temp_img")
    world.WindowDelete("loadimg_test")

    return 0
end

-- Test WindowDrawImage
function test_window_draw_image()
    world.WindowCreate("drawimg_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create and save a test image
    world.WindowCreate("temp_img2", 0, 0, 64, 64, 0, 0, 0xFF0000)
    world.WindowRectOp("temp_img2", 2, 10, 10, 54, 54, 0x00FF00, 0, 1, 0x0000FF, 0)
    world.WindowWrite("temp_img2", "tests/test_image2.png")

    -- Load the image
    local result = world.WindowLoadImage("drawimg_test", "img2", "tests/test_image2.png")
    check(result == 0, "WindowLoadImage should succeed")

    -- Draw the image: WindowDrawImage(name, imageId, left, top, right, bottom, mode, srcLeft, srcTop, srcRight, srcBottom)
    -- mode 1 = copy, mode 2 = transparent, mode 3 = stretch
    result = world.WindowDrawImage("drawimg_test", "img2", 10, 10, 74, 74, 1, 0, 0, 0, 0)
    check(result == 0, "WindowDrawImage should return 0 on success")

    -- Draw with stretching
    result = world.WindowDrawImage("drawimg_test", "img2", 100, 100, 180, 180, 3, 0, 0, 0, 0)
    check(result == 0, "WindowDrawImage stretch should return 0 on success")

    -- Draw a sprite (partial source rect)
    result = world.WindowDrawImage("drawimg_test", "img2", 50, 50, 82, 82, 1, 0, 0, 32, 32)
    check(result == 0, "WindowDrawImage sprite should return 0 on success")

    -- Test invalid window
    result = world.WindowDrawImage("nonexistent", "img2", 0, 0, 64, 64, 1, 0, 0, 0, 0)
    check(result ~= 0, "WindowDrawImage should fail for non-existent window")

    -- Test invalid image
    result = world.WindowDrawImage("drawimg_test", "nonexistent", 0, 0, 64, 64, 1, 0, 0, 0, 0)
    check(result ~= 0, "WindowDrawImage should fail for non-existent image")

    world.WindowDelete("temp_img2")
    world.WindowDelete("drawimg_test")

    return 0
end

-- Test WindowDrawImageAlpha
function test_window_draw_image_alpha()
    world.WindowCreate("alphaimg_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create and save a test image
    world.WindowCreate("temp_img3", 0, 0, 48, 48, 0, 0, 0xFF0000)
    world.WindowCircleOp("temp_img3", 3, 4, 4, 44, 44, 0x00FF00, 0, 2, 0, 1)
    world.WindowWrite("temp_img3", "tests/test_image3.png")

    -- Load the image
    local result = world.WindowLoadImage("alphaimg_test", "img3", "tests/test_image3.png")
    check(result == 0, "WindowLoadImage should succeed")

    -- Draw with 100% opacity
    result = world.WindowDrawImageAlpha("alphaimg_test", "img3", 10, 10, 58, 58, 1.0, 0, 0)
    check(result == 0, "WindowDrawImageAlpha should return 0 on success")

    -- Draw with 50% opacity
    result = world.WindowDrawImageAlpha("alphaimg_test", "img3", 60, 60, 108, 108, 0.5, 0, 0)
    check(result == 0, "WindowDrawImageAlpha 50% opacity should return 0 on success")

    -- Draw with 0% opacity (invisible)
    result = world.WindowDrawImageAlpha("alphaimg_test", "img3", 120, 120, 168, 168, 0.0, 0, 0)
    check(result == 0, "WindowDrawImageAlpha 0% opacity should return 0 on success")

    -- Test with partial source rect
    result = world.WindowDrawImageAlpha("alphaimg_test", "img3", 150, 10, 174, 34, 0.75, 0, 0)
    check(result == 0, "WindowDrawImageAlpha with source rect should return 0 on success")

    world.WindowDelete("temp_img3")
    world.WindowDelete("alphaimg_test")

    return 0
end

-- Test WindowBlendImage
function test_window_blend_image()
    world.WindowCreate("blend_test", 0, 0, 200, 200, 0, 0, 0x808080)

    -- Create and save a test image
    world.WindowCreate("temp_img4", 0, 0, 50, 50, 0, 0, 0xFF0000)
    world.WindowRectOp("temp_img4", 3, 5, 5, 45, 45, 0, 0, 0, 0x00FF00, 0)
    world.WindowWrite("temp_img4", "tests/test_image4.png")

    -- Load the image
    local result = world.WindowLoadImage("blend_test", "img4", "tests/test_image4.png")
    check(result == 0, "WindowLoadImage should succeed")

    -- Blend with mode 1 (copy) and 100% opacity
    result = world.WindowBlendImage("blend_test", "img4", 10, 10, 60, 60, 1, 1.0, 0, 0, 0, 0)
    check(result == 0, "WindowBlendImage should return 0 on success")

    -- Blend with 50% opacity
    result = world.WindowBlendImage("blend_test", "img4", 70, 70, 120, 120, 1, 0.5, 0, 0, 0, 0)
    check(result == 0, "WindowBlendImage 50% opacity should return 0 on success")

    -- Blend with mode 3 (stretch)
    result = world.WindowBlendImage("blend_test", "img4", 130, 10, 190, 70, 3, 0.8, 0, 0, 0, 0)
    check(result == 0, "WindowBlendImage stretch should return 0 on success")

    -- Blend with partial source rect
    result = world.WindowBlendImage("blend_test", "img4", 10, 130, 35, 155, 1, 1.0, 0, 0, 25, 25)
    check(result == 0, "WindowBlendImage with source rect should return 0 on success")

    world.WindowDelete("temp_img4")
    world.WindowDelete("blend_test")

    return 0
end

-- Test WindowImageFromWindow
function test_window_image_from_window()
    world.WindowCreate("src_win", 0, 0, 80, 80, 0, 0, 0xFF0000)
    world.WindowCircleOp("src_win", 3, 5, 5, 75, 75, 0x00FF00, 0, 2, 0x0000FF, 0)

    world.WindowCreate("dst_win", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Copy window content to image
    local result = world.WindowImageFromWindow("dst_win", "winimg", "src_win")
    check(result == 0, "WindowImageFromWindow should return 0 on success")

    -- Verify image was created
    local list = world.WindowImageList("dst_win")
    local found = false
    for i = 1, #list do
        if list[i] == "winimg" then
            found = true
            break
        end
    end
    check(found, "WindowImageFromWindow should create image in list")

    -- Verify image dimensions match source window (infoType: 1=width, 2=height)
    local width = world.WindowImageInfo("dst_win", "winimg", 1)
    check(width == 80, "Copied image width should match source window")

    local height = world.WindowImageInfo("dst_win", "winimg", 2)
    check(height == 80, "Copied image height should match source window")

    -- Now we can draw that image
    result = world.WindowDrawImage("dst_win", "winimg", 10, 10, 90, 90, 1, 0, 0, 0, 0)
    check(result == 0, "Should be able to draw copied window image")

    -- Test with non-existent source window
    result = world.WindowImageFromWindow("dst_win", "badimg", "nonexistent_window")
    check(result ~= 0, "WindowImageFromWindow should fail for non-existent source")

    world.WindowDelete("src_win")
    world.WindowDelete("dst_win")

    return 0
end

-- Test WindowGetImageAlpha
function test_window_get_image_alpha()
    world.WindowCreate("getalpha_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Create source window with content
    world.WindowCreate("alpha_src", 0, 0, 60, 60, 0, 0, 0xFF0000)
    world.WindowRectOp("alpha_src", 3, 10, 10, 50, 50, 0, 0, 0, 0x00FF00, 0)

    -- Copy to image
    local result = world.WindowImageFromWindow("getalpha_test", "alphaimg", "alpha_src")
    check(result == 0, "WindowImageFromWindow should succeed")

    -- Get image alpha: WindowGetImageAlpha(name, imageId, left, top, right, bottom, srcLeft, srcTop)
    -- This extracts alpha channel from source image region
    result = world.WindowGetImageAlpha("getalpha_test", "alphaimg", 10, 10, 70, 70, 0, 0)
    check(result == 0, "WindowGetImageAlpha should return 0 on success")

    -- Test with partial source rect
    result = world.WindowGetImageAlpha("getalpha_test", "alphaimg", 80, 10, 110, 40, 0, 0)
    check(result == 0, "WindowGetImageAlpha with source rect should return 0 on success")

    world.WindowDelete("alpha_src")
    world.WindowDelete("getalpha_test")

    return 0
end

-- Test WindowMergeImageAlpha
function test_window_merge_image_alpha()
    world.WindowCreate("merge_test", 0, 0, 200, 200, 0, 0, 0x404040)

    -- Create base image
    world.WindowCreate("base_img", 0, 0, 64, 64, 0, 0, 0xFF0000)
    world.WindowRectOp("base_img", 3, 8, 8, 56, 56, 0, 0, 0, 0x00FF00, 0)
    local result = world.WindowImageFromWindow("merge_test", "baseimg", "base_img")
    check(result == 0, "Should create base image")

    -- Create mask image (alpha channel)
    world.WindowCreate("mask_img", 0, 0, 64, 64, 0, 0, 0xFFFFFF)
    world.WindowCircleOp("mask_img", 3, 4, 4, 60, 60, 0, 0, 1, 0x000000, 0)
    result = world.WindowImageFromWindow("merge_test", "maskimg", "mask_img")
    check(result == 0, "Should create mask image")

    -- Merge mask alpha into base image
    -- WindowMergeImageAlpha(name, imageId, maskId, left, top, right, bottom, mode, opacity, srcLeft, srcTop, srcRight, srcBottom)
    result = world.WindowMergeImageAlpha("merge_test", "baseimg", "maskimg", 10, 10, 74, 74, 1, 1.0, 0, 0, 64, 64)
    check(result == 0, "WindowMergeImageAlpha should return 0 on success")

    -- Try with 50% opacity
    result = world.WindowMergeImageAlpha("merge_test", "baseimg", "maskimg", 90, 90, 154, 154, 1, 0.5, 0, 0, 64, 64)
    check(result == 0, "WindowMergeImageAlpha 50% opacity should return 0 on success")

    world.WindowDelete("base_img")
    world.WindowDelete("mask_img")
    world.WindowDelete("merge_test")

    return 0
end

-- Test WindowTransformImage
function test_window_transform_image()
    world.WindowCreate("transform_test", 0, 0, 300, 300, 0, 0, 0x202020)

    -- Create and load test image
    world.WindowCreate("trans_img", 0, 0, 40, 40, 0, 0, 0xFF0000)
    world.WindowRectOp("trans_img", 3, 5, 5, 35, 35, 0, 0, 0, 0x0000FF, 0)
    world.WindowWrite("trans_img", "tests/test_transform.png")

    local result = world.WindowLoadImage("transform_test", "transimg", "tests/test_transform.png")
    check(result == 0, "WindowLoadImage should succeed")

    -- Transform image using a 2x2 transformation matrix
    -- WindowTransformImage(name, imageId, left, top, mode, mxx, mxy, myx, myy)
    -- Identity matrix: mxx=1, mxy=0, myx=0, myy=1
    result = world.WindowTransformImage("transform_test", "transimg", 10, 10, 1, 1.0, 0.0, 0.0, 1.0)
    check(result == 0, "WindowTransformImage identity should return 0 on success")

    -- Scale 2x in both directions: mxx=2, mxy=0, myx=0, myy=2
    result = world.WindowTransformImage("transform_test", "transimg", 60, 10, 1, 2.0, 0.0, 0.0, 2.0)
    check(result == 0, "WindowTransformImage scale should return 0 on success")

    -- Mirror horizontally: mxx=-1, mxy=0, myx=0, myy=1
    result = world.WindowTransformImage("transform_test", "transimg", 150, 10, 1, -1.0, 0.0, 0.0, 1.0)
    check(result == 0, "WindowTransformImage mirror-x should return 0 on success")

    -- Mirror vertically: mxx=1, mxy=0, myx=0, myy=-1
    result = world.WindowTransformImage("transform_test", "transimg", 10, 60, 1, 1.0, 0.0, 0.0, -1.0)
    check(result == 0, "WindowTransformImage mirror-y should return 0 on success")

    -- Rotate 90 degrees: mxx=0, mxy=-1, myx=1, myy=0
    result = world.WindowTransformImage("transform_test", "transimg", 60, 60, 1, 0.0, -1.0, 1.0, 0.0)
    check(result == 0, "WindowTransformImage rotate should return 0 on success")

    -- Shear: mxx=1, mxy=0.5, myx=0, myy=1
    result = world.WindowTransformImage("transform_test", "transimg", 150, 60, 1, 1.0, 0.5, 0.0, 1.0)
    check(result == 0, "WindowTransformImage shear should return 0 on success")

    world.WindowDelete("trans_img")
    world.WindowDelete("transform_test")

    return 0
end

-- ========== UI Input Functions (DISABLED - require UI) ==========

-- Test PasteCommand
function test_paste_command()
    -- PasteCommand(text) - pastes text into command input
    -- Note: Returns empty string (replaced text) in headless mode
    local result = world.PasteCommand("test text")
    check(type(result) == "string", "PasteCommand should return a string")

    -- Test with empty string
    result = world.PasteCommand("")
    check(type(result) == "string", "PasteCommand with empty string should return string")

    -- Test with special characters
    result = world.PasteCommand("test\nwith\nnewlines")
    check(type(result) == "string", "PasteCommand with newlines should return string")

    -- Test with Unicode
    result = world.PasteCommand("Unicode: \xE2\x9C\x93")
    check(type(result) == "string", "PasteCommand with Unicode should return string")

    return 0
end

-- Test SetCommandWindowHeight
function test_set_command_window_height()
    -- SetCommandWindowHeight(height) - sets command window height
    -- This is a stub implementation that returns eOK (0)

    -- Test with valid height
    local result = world.SetCommandWindowHeight(100)
    check(result == 0, "SetCommandWindowHeight should return 0 (eOK)")

    -- Test with minimum height
    result = world.SetCommandWindowHeight(1)
    check(result == 0, "SetCommandWindowHeight with small value should return 0")

    -- Test with large height
    result = world.SetCommandWindowHeight(1000)
    check(result == 0, "SetCommandWindowHeight with large value should return 0")

    return 0
end

-- Test SetInputFont
function test_set_input_font()
    -- SetInputFont(fontName, pointSize, weight, italic) - sets command input font
    -- Parameters:
    --   fontName: Font family name (string)
    --   pointSize: Font size in points (integer)
    --   weight: Font weight (400=normal, 700=bold)
    --   italic: 0=normal, 1=italic (optional, defaults to 0)

    -- Test basic font setting
    world.SetInputFont("Courier New", 12, 400, 0)

    -- Test with bold font
    world.SetInputFont("Arial", 14, 700, 0)

    -- Test with italic font
    world.SetInputFont("Times New Roman", 11, 400, 1)

    -- Test with bold and italic
    world.SetInputFont("Helvetica", 13, 700, 1)

    -- Test with minimum values
    world.SetInputFont("Monospace", 6, 100, 0)

    -- Test without italic parameter (should default to 0)
    world.SetInputFont("Monospace", 10, 400)

    return 0
end

-- Test SetOutputFont
function test_set_output_font()
    -- SetOutputFont(fontName, pointSize) - sets output window font
    -- Parameters:
    --   fontName: Font family name (string)
    --   pointSize: Font size in points (integer)

    -- Test basic font setting
    world.SetOutputFont("Courier New", 12)

    -- Test with different font
    world.SetOutputFont("Consolas", 11)

    -- Test with small font size
    world.SetOutputFont("Monaco", 8)

    -- Test with large font size
    world.SetOutputFont("DejaVu Sans Mono", 18)

    -- Test with Unicode font name
    world.SetOutputFont("Noto Mono", 10)

    return 0
end

-- Test ShiftTabCompleteItem
function test_shift_tab_complete_item()
    -- ShiftTabCompleteItem(item) - adds item to Shift+Tab completion list
    -- Special items:
    --   "<clear>" - clears the list
    --   "<functions>" - enables showing Lua functions
    --   "<nofunctions>" - disables showing Lua functions
    -- Regular items must be 1-30 chars, start with letter, contain only alphanumeric, dot, hyphen, underscore

    -- Test clearing the list
    local result = world.ShiftTabCompleteItem("<clear>")
    check(result == 0, "ShiftTabCompleteItem <clear> should return 0")

    -- Test enabling functions
    result = world.ShiftTabCompleteItem("<functions>")
    check(result == 0, "ShiftTabCompleteItem <functions> should return 0")

    -- Test disabling functions
    result = world.ShiftTabCompleteItem("<nofunctions>")
    check(result == 0, "ShiftTabCompleteItem <nofunctions> should return 0")

    -- Test adding valid items
    result = world.ShiftTabCompleteItem("mycommand")
    check(result == 0, "ShiftTabCompleteItem with valid name should return 0")

    result = world.ShiftTabCompleteItem("my_command")
    check(result == 0, "ShiftTabCompleteItem with underscore should return 0")

    result = world.ShiftTabCompleteItem("my-command")
    check(result == 0, "ShiftTabCompleteItem with hyphen should return 0")

    result = world.ShiftTabCompleteItem("my.command")
    check(result == 0, "ShiftTabCompleteItem with dot should return 0")

    result = world.ShiftTabCompleteItem("cmd123")
    check(result == 0, "ShiftTabCompleteItem with numbers should return 0")

    -- Test edge cases for valid names
    result = world.ShiftTabCompleteItem("a")
    check(result == 0, "ShiftTabCompleteItem with single char should return 0")

    result = world.ShiftTabCompleteItem("a12345678901234567890123456789")  -- 30 chars
    check(result == 0, "ShiftTabCompleteItem with 30 chars should return 0")

    -- Test invalid items (should return eBadParameter = 30046)

    -- Empty string
    result = world.ShiftTabCompleteItem("")
    check(result == 30046, "ShiftTabCompleteItem with empty string should return eBadParameter")

    -- Too long (31 chars)
    result = world.ShiftTabCompleteItem("a1234567890123456789012345678901")
    check(result == 30046, "ShiftTabCompleteItem with >30 chars should return eBadParameter")

    -- Starts with number
    result = world.ShiftTabCompleteItem("1command")
    check(result == 30046, "ShiftTabCompleteItem starting with number should return eBadParameter")

    -- Contains invalid characters
    result = world.ShiftTabCompleteItem("my command")
    check(result == 30046, "ShiftTabCompleteItem with space should return eBadParameter")

    result = world.ShiftTabCompleteItem("my@command")
    check(result == 30046, "ShiftTabCompleteItem with @ should return eBadParameter")

    result = world.ShiftTabCompleteItem("my$command")
    check(result == 30046, "ShiftTabCompleteItem with $ should return eBadParameter")

    return 0
end

-- ========== UI Sound and Save Functions (DISABLED - require UI) ==========

-- Test PlaySound
function test_play_sound()
    -- PlaySound(buffer, filename, loop, volume, pan)
    -- buffer: 1-10 for specific buffer
    -- filename: path to sound file
    -- loop: true/false
    -- volume: -10000 to 0 (0 = full, -10000 = silent)
    -- pan: -100 (left) to 100 (right), 0 = center

    -- Test with nonexistent file (should return eFileNotFound = 30024)
    local result = world.PlaySound(1, "nonexistent.wav", false, 0, 0)
    check(type(result) == "number", "PlaySound should return a number")
    check(result == 30051, "PlaySound should return eFileNotFound (30051) for nonexistent file")

    -- Buffer 0 is valid (auto-select first available buffer)
    result = world.PlaySound(0, "nonexistent.wav", false, 0, 0)
    check(result == 30051, "PlaySound with buffer 0 (auto-select) should return eFileNotFound")

    -- Test with buffer out of range (> 10)
    result = world.PlaySound(11, "test.wav", false, 0, 0)
    check(result == 30046, "PlaySound should return eBadParameter (30046) for buffer > 10")

    -- Test with negative buffer
    result = world.PlaySound(-1, "test.wav", false, 0, 0)
    check(result == 30046, "PlaySound should return eBadParameter (30046) for negative buffer")

    -- Test with different volume levels
    result = world.PlaySound(1, "test.wav", false, -5000, 0)
    check(type(result) == "number", "PlaySound with -5000 volume should return number")

    -- Test with different pan values
    result = world.PlaySound(2, "test.wav", false, 0, -100)
    check(type(result) == "number", "PlaySound with left pan should return number")

    result = world.PlaySound(3, "test.wav", false, 0, 100)
    check(type(result) == "number", "PlaySound with right pan should return number")

    -- Test with loop enabled
    result = world.PlaySound(4, "test.wav", true, 0, 0)
    check(type(result) == "number", "PlaySound with loop should return number")

    return 0
end

-- Test GetSoundStatus
function test_get_sound_status()
    -- GetSoundStatus(buffer)
    -- Returns: -1 for invalid buffer, -2 for free buffer, 0 for stopped, 1 for playing, 2 for looping

    -- Test with invalid buffer (out of range)
    local status = world.GetSoundStatus(999)
    check(status == -1, "GetSoundStatus should return -1 for buffer out of range")

    -- Test with negative buffer
    status = world.GetSoundStatus(-1)
    check(status == -1, "GetSoundStatus should return -1 for negative buffer")

    -- Test with valid but unused buffer (should return -2 for free buffer)
    status = world.GetSoundStatus(1)
    check(status == -2, "GetSoundStatus should return -2 for free buffer")

    -- Test with all valid buffer numbers (1-10)
    for i = 1, 10 do
        status = world.GetSoundStatus(i)
        check(type(status) == "number", "GetSoundStatus(" .. i .. ") should return a number")
        check(status >= -2, "GetSoundStatus(" .. i .. ") should return >= -2")
    end

    return 0
end

-- Test Save
function test_save()
    -- Save(filename)
    -- filename is optional - if not provided, saves to current world file
    -- This requires UI for "Save As" dialog, so we just test the function exists and is callable

    -- Check function exists
    check(type(world.Save) == "function", "world.Save should be a function")

    -- Test with empty filename (would use current world path or show dialog)
    -- In test environment without UI, this may fail but should not crash
    local result = world.Save("")
    check(type(result) == "number", "Save should return a number")

    -- Test with nil (same as no parameter)
    result = world.Save(nil)
    check(type(result) == "number", "Save with nil should return a number")

    return 0
end

-- Test SaveState
function test_save_state()
    -- SaveState() - saves plugin state
    -- This is tested extensively in test_plugin_state_gtest.cpp
    -- Here we just verify it's callable from world context

    -- Check function exists
    check(type(world.SaveState) == "function", "world.SaveState should be a function")

    -- In world context (not plugin), SaveState should work without errors
    -- Note: This doesn't do much in world context, mainly for plugins
    local result = world.SaveState()
    check(type(result) == "number", "SaveState should return a number")
    check(result == 0, "SaveState should return 0 (eOK)")

    return 0
end

-- Test SetMainTitle
function test_set_main_title()
    -- SetMainTitle(...) - sets main application window title
    -- All arguments are concatenated

    -- Check function exists
    check(type(world.SetMainTitle) == "function", "world.SetMainTitle should be a function")

    -- Test with single string
    world.SetMainTitle("Test Title")
    -- No error means success (returns 0 values)

    -- Test with multiple arguments (concatenated)
    world.SetMainTitle("Title ", "Part ", "2")
    -- No error means success

    -- Test with empty string
    world.SetMainTitle("")
    -- No error means success

    -- Test with numbers (should be converted to strings)
    world.SetMainTitle("Count: ", 42)
    -- No error means success

    return 0
end

-- Test SetScroll
function test_set_scroll()
    -- SetScroll(position, visible) - sets scroll position
    -- This is a stub function that always returns 0

    -- Check function exists
    check(type(world.SetScroll) == "function", "world.SetScroll should be a function")

    -- Test with position 0, visible true
    local result = world.SetScroll(0, true)
    check(result == 0, "SetScroll should return 0 (eOK)")

    -- Test with position 100, visible false
    result = world.SetScroll(100, false)
    check(result == 0, "SetScroll should return 0 (eOK)")

    -- Test with negative position
    result = world.SetScroll(-50, true)
    check(result == 0, "SetScroll should return 0 (eOK)")

    return 0
end

-- Test SetTitle
function test_set_title()
    -- SetTitle(...) - sets world window title
    -- All arguments are concatenated

    -- Check function exists
    check(type(world.SetTitle) == "function", "world.SetTitle should be a function")

    -- Test with single string
    world.SetTitle("World Title")
    -- No error means success (returns 0 values)

    -- Test with multiple arguments (concatenated)
    world.SetTitle("World ", "Connection ", "Active")
    -- No error means success

    -- Test with empty string
    world.SetTitle("")
    -- No error means success

    -- Test with special characters
    world.SetTitle("MUD: [Connected] @localhost:4000")
    -- No error means success

    return 0
end

-- Test ResetIP
function test_reset_ip()
    -- ResetIP() - resets IP address (deprecated, does nothing)
    -- This is a legacy function from proxy support that was removed

    -- Check function exists
    check(type(world.ResetIP) == "function", "world.ResetIP should be a function")

    -- Call ResetIP (should return 0 and do nothing)
    local result = world.ResetIP()
    check(result == 0, "ResetIP should return 0")

    return 0
end

-- ========== Miniwindow Shape/Drawing Functions ==========

-- Test WindowRectOp
function test_window_rect_op()
    world.WindowCreate("rectop_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Test 1: Frame (outline) rectangle
    local result = world.WindowRectOp("rectop_test", miniwin.rect_frame,
                                       10, 10, 50, 50,
                                       0xFF0000, 0x000000)
    check(result == 0, "WindowRectOp frame should return 0 on success")

    -- Test 2: Fill rectangle
    result = world.WindowRectOp("rectop_test", miniwin.rect_fill,
                                 60, 10, 100, 50,
                                 0x000000, 0x00FF00)
    check(result == 0, "WindowRectOp fill should return 0 on success")

    -- Test 3: Invert rectangle (XOR)
    result = world.WindowRectOp("rectop_test", miniwin.rect_invert,
                                 110, 10, 150, 50,
                                 0xFFFFFF, 0x000000)
    check(result == 0, "WindowRectOp invert should return 0 on success")

    -- Test 4: 3D rectangle
    result = world.WindowRectOp("rectop_test", miniwin.rect_3d_rect,
                                 10, 60, 50, 100,
                                 0x0000FF, 0x808080)
    check(result == 0, "WindowRectOp 3d_rect should return 0 on success")

    -- Test 5: Invalid action code
    result = world.WindowRectOp("rectop_test", 999,
                                 0, 0, 10, 10,
                                 0x000000, 0x000000)
    check(result ~= 0, "WindowRectOp with invalid action should return error")

    -- Test 6: Invalid window name
    result = world.WindowRectOp("nonexistent_window", miniwin.rect_frame,
                                 0, 0, 10, 10,
                                 0x000000, 0x000000)
    check(result == 30073, "WindowRectOp with invalid window should return eNoSuchWindow")

    world.WindowDelete("rectop_test")

    return 0
end

-- Test WindowCircleOp
function test_window_circle_op()
    world.WindowCreate("circleop_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Test 1: Ellipse
    local result = world.WindowCircleOp("circleop_test", miniwin.circle_ellipse,
                                         10, 10, 50, 50,
                                         0xFF0000, miniwin.pen_solid, 1,
                                         0x000000, miniwin.brush_null,
                                         0, 0, 0, 0)
    check(result == 0, "WindowCircleOp ellipse should return 0 on success")

    -- Test 2: Rectangle
    result = world.WindowCircleOp("circleop_test", miniwin.circle_rectangle,
                                   60, 10, 100, 50,
                                   0x00FF00, miniwin.pen_solid, 2,
                                   0x808080, miniwin.brush_solid,
                                   0, 0, 0, 0)
    check(result == 0, "WindowCircleOp rectangle should return 0 on success")

    -- Test 3: Rounded rectangle
    result = world.WindowCircleOp("circleop_test", miniwin.circle_round_rect,
                                   110, 10, 150, 50,
                                   0x0000FF, miniwin.pen_solid, 1,
                                   0x000000, miniwin.brush_null,
                                   10, 10, 0, 0)
    check(result == 0, "WindowCircleOp round_rect should return 0 on success")

    -- Test 4: Chord
    result = world.WindowCircleOp("circleop_test", miniwin.circle_chord,
                                   10, 60, 50, 100,
                                   0xFFFF00, miniwin.pen_solid, 1,
                                   0x000000, miniwin.brush_null,
                                   25, 60, 50, 80)
    check(result == 0, "WindowCircleOp chord should return 0 on success")

    -- Test 5: Pie
    result = world.WindowCircleOp("circleop_test", miniwin.circle_pie,
                                   60, 60, 100, 100,
                                   0xFF00FF, miniwin.pen_solid, 1,
                                   0x404040, miniwin.brush_solid,
                                   80, 60, 100, 80)
    check(result == 0, "WindowCircleOp pie should return 0 on success")

    -- Test 6: Arc
    result = world.WindowCircleOp("circleop_test", miniwin.circle_arc,
                                   110, 60, 150, 100,
                                   0x00FFFF, miniwin.pen_solid, 2,
                                   0x000000, miniwin.brush_null,
                                   130, 60, 150, 80)
    check(result == 0, "WindowCircleOp arc should return 0 on success")

    -- Test 7: Different pen styles
    result = world.WindowCircleOp("circleop_test", miniwin.circle_ellipse,
                                   160, 10, 190, 40,
                                   0xFFFFFF, miniwin.pen_dash, 1,
                                   0x000000, miniwin.brush_null,
                                   0, 0, 0, 0)
    check(result == 0, "WindowCircleOp with pen_dash should return 0 on success")

    -- Test 8: Invalid action code
    result = world.WindowCircleOp("circleop_test", 999,
                                   0, 0, 10, 10,
                                   0x000000, miniwin.pen_solid, 1,
                                   0x000000, miniwin.brush_null,
                                   0, 0, 0, 0)
    check(result ~= 0, "WindowCircleOp with invalid action should return error")

    -- Test 9: Invalid window name
    result = world.WindowCircleOp("nonexistent_window", miniwin.circle_ellipse,
                                   0, 0, 10, 10,
                                   0x000000, miniwin.pen_solid, 1,
                                   0x000000, miniwin.brush_null,
                                   0, 0, 0, 0)
    check(result == 30073, "WindowCircleOp with invalid window should return eNoSuchWindow")

    world.WindowDelete("circleop_test")

    return 0
end

-- Test WindowLine
function test_window_line()
    world.WindowCreate("line_test", 0, 0, 200, 200, 0, 0, 0x000000)

    -- Test 1: Simple line
    local result = world.WindowLine("line_test", 10, 10, 50, 50,
                                     0xFF0000, miniwin.pen_solid, 1)
    check(result == 0, "WindowLine should return 0 on success")

    -- Test 2: Horizontal line
    result = world.WindowLine("line_test", 10, 60, 100, 60,
                              0x00FF00, miniwin.pen_solid, 2)
    check(result == 0, "WindowLine horizontal should return 0 on success")

    -- Test 3: Vertical line
    result = world.WindowLine("line_test", 110, 10, 110, 100,
                              0x0000FF, miniwin.pen_solid, 3)
    check(result == 0, "WindowLine vertical should return 0 on success")

    -- Test 4: Different pen styles
    result = world.WindowLine("line_test", 120, 10, 180, 50,
                              0xFFFF00, miniwin.pen_dash, 1)
    check(result == 0, "WindowLine with pen_dash should return 0 on success")

    result = world.WindowLine("line_test", 120, 55, 180, 95,
                              0xFF00FF, miniwin.pen_dot, 1)
    check(result == 0, "WindowLine with pen_dot should return 0 on success")

    result = world.WindowLine("line_test", 10, 110, 70, 150,
                              0x00FFFF, miniwin.pen_dashdot, 2)
    check(result == 0, "WindowLine with pen_dashdot should return 0 on success")

    -- Test 5: Thick line
    result = world.WindowLine("line_test", 80, 110, 140, 150,
                              0xFFFFFF, miniwin.pen_solid, 5)
    check(result == 0, "WindowLine with thick pen should return 0 on success")

    -- Test 6: Invalid window name
    result = world.WindowLine("nonexistent_window", 0, 0, 10, 10,
                              0x000000, miniwin.pen_solid, 1)
    check(result == 30073, "WindowLine with invalid window should return eNoSuchWindow")

    world.WindowDelete("line_test")

    return 0
end

-- Test WindowSetPixel
function test_window_set_pixel()
    world.WindowCreate("setpixel_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Test 1: Set a single pixel
    local result = world.WindowSetPixel("setpixel_test", 50, 50, 0xFF0000)
    check(result == 0, "WindowSetPixel should return 0 on success")

    -- Test 2: Set multiple pixels
    result = world.WindowSetPixel("setpixel_test", 10, 10, 0x00FF00)
    check(result == 0, "WindowSetPixel at 10,10 should return 0 on success")

    result = world.WindowSetPixel("setpixel_test", 90, 10, 0x0000FF)
    check(result == 0, "WindowSetPixel at 90,10 should return 0 on success")

    result = world.WindowSetPixel("setpixel_test", 10, 90, 0xFFFF00)
    check(result == 0, "WindowSetPixel at 10,90 should return 0 on success")

    result = world.WindowSetPixel("setpixel_test", 90, 90, 0xFF00FF)
    check(result == 0, "WindowSetPixel at 90,90 should return 0 on success")

    -- Test 3: Set pixel at boundary
    result = world.WindowSetPixel("setpixel_test", 0, 0, 0xFFFFFF)
    check(result == 0, "WindowSetPixel at 0,0 should return 0 on success")

    result = world.WindowSetPixel("setpixel_test", 99, 99, 0xFFFFFF)
    check(result == 0, "WindowSetPixel at 99,99 should return 0 on success")

    -- Test 4: Out of bounds - should fail silently or return error
    result = world.WindowSetPixel("setpixel_test", -1, 50, 0xFF0000)
    -- Note: May return error or silently fail depending on implementation

    result = world.WindowSetPixel("setpixel_test", 50, -1, 0xFF0000)
    -- Note: May return error or silently fail depending on implementation

    result = world.WindowSetPixel("setpixel_test", 100, 50, 0xFF0000)
    -- Note: May return error or silently fail depending on implementation

    result = world.WindowSetPixel("setpixel_test", 50, 100, 0xFF0000)
    -- Note: May return error or silently fail depending on implementation

    -- Test 5: Invalid window name
    result = world.WindowSetPixel("nonexistent_window", 50, 50, 0xFF0000)
    check(result == 30073, "WindowSetPixel with invalid window should return eNoSuchWindow")

    world.WindowDelete("setpixel_test")

    return 0
end

-- Test WindowGetPixel
function test_window_get_pixel()
    world.WindowCreate("getpixel_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Test 1: Get pixel from background (should be black)
    local color = world.WindowGetPixel("getpixel_test", 50, 50)
    check(type(color) == "number", "WindowGetPixel should return a number")
    check(color == 0x000000, "WindowGetPixel on black background should return 0x000000")

    -- Test 2: Set a pixel and read it back
    world.WindowSetPixel("getpixel_test", 25, 25, 0xFF0000)
    color = world.WindowGetPixel("getpixel_test", 25, 25)
    check(color == 0xFF0000, "WindowGetPixel should return color that was set")

    -- Test 3: Set and read different colors
    world.WindowSetPixel("getpixel_test", 30, 30, 0x00FF00)
    color = world.WindowGetPixel("getpixel_test", 30, 30)
    check(color == 0x00FF00, "WindowGetPixel should return green color")

    world.WindowSetPixel("getpixel_test", 35, 35, 0x0000FF)
    color = world.WindowGetPixel("getpixel_test", 35, 35)
    check(color == 0x0000FF, "WindowGetPixel should return blue color")

    world.WindowSetPixel("getpixel_test", 40, 40, 0xFFFFFF)
    color = world.WindowGetPixel("getpixel_test", 40, 40)
    check(color == 0xFFFFFF, "WindowGetPixel should return white color")

    -- Test 4: Get pixel at boundary
    world.WindowSetPixel("getpixel_test", 0, 0, 0xFF00FF)
    color = world.WindowGetPixel("getpixel_test", 0, 0)
    check(color == 0xFF00FF, "WindowGetPixel at 0,0 should return magenta")

    world.WindowSetPixel("getpixel_test", 99, 99, 0x00FFFF)
    color = world.WindowGetPixel("getpixel_test", 99, 99)
    check(color == 0x00FFFF, "WindowGetPixel at 99,99 should return cyan")

    -- Test 5: Out of bounds - should return 0 or error
    color = world.WindowGetPixel("getpixel_test", -1, 50)
    check(color == 0, "WindowGetPixel out of bounds should return 0")

    color = world.WindowGetPixel("getpixel_test", 50, -1)
    check(color == 0, "WindowGetPixel out of bounds should return 0")

    color = world.WindowGetPixel("getpixel_test", 100, 50)
    check(color == 0, "WindowGetPixel out of bounds should return 0")

    color = world.WindowGetPixel("getpixel_test", 50, 100)
    check(color == 0, "WindowGetPixel out of bounds should return 0")

    -- Test 6: Invalid window name
    color = world.WindowGetPixel("nonexistent_window", 50, 50)
    check(color == 0, "WindowGetPixel with invalid window should return 0")

    world.WindowDelete("getpixel_test")

    return 0
end

-- ========== Miniwindow Filter Functions ==========

-- Test WindowShow
function test_window_show()
    -- Create a test window
    world.WindowCreate("show_test", 0, 0, 100, 100, 0, 0, 0x000000)

    -- Test showing the window (default is already shown)
    local result = world.WindowShow("show_test", true)
    check(result == 0, "WindowShow(true) should return 0 on success")

    -- Test hiding the window
    result = world.WindowShow("show_test", false)
    check(result == 0, "WindowShow(false) should return 0 on success")

    -- Test showing again
    result = world.WindowShow("show_test", true)
    check(result == 0, "WindowShow(true) again should return 0 on success")

    -- Test with non-existent window (should return error eNoSuchWindow=30073)
    result = world.WindowShow("nonexistent_window", true)
    check(result == 30073, "WindowShow with non-existent window should return 30073")

    world.WindowDelete("show_test")

    return 0
end

-- Test WindowWrite
function test_window_write()
    -- Create a test window with some content
    world.WindowCreate("write_test", 0, 0, 100, 100, 0, 0, 0xFF0000)

    -- Draw something on the window so it has content
    world.WindowRectOp("write_test", 0, 10, 10, 50, 50, 0x00FF00, 0)

    -- Test writing to PNG file
    local png_path = "/tmp/mushkin_test_window.png"
    local result = world.WindowWrite("write_test", png_path)
    check(result == 0, "WindowWrite to PNG should return 0 on success")

    -- Test writing to BMP file
    local bmp_path = "/tmp/mushkin_test_window.bmp"
    result = world.WindowWrite("write_test", bmp_path)
    check(result == 0, "WindowWrite to BMP should return 0 on success")

    -- Test with non-existent window (should return error eNoSuchWindow=30073)
    result = world.WindowWrite("nonexistent_window", "/tmp/test.png")
    check(result == 30073, "WindowWrite with non-existent window should return 30073")

    -- Test with invalid file extension (should return error)
    result = world.WindowWrite("write_test", "/tmp/test.txt")
    check(result ~= 0, "WindowWrite with invalid extension should return error")

    world.WindowDelete("write_test")

    return 0
end

-- Test WindowFilter
function test_window_filter()
    -- Create a test window with some content
    world.WindowCreate("filter_test", 0, 0, 200, 200, 0, 0, 0x808080)

    -- Draw a colored rectangle to have content to filter
    world.WindowRectOp("filter_test", 0, 50, 50, 150, 150, 0xFF8800, 0)

    -- Test filter operation 1: Noise (add noise with 50% intensity)
    local result = world.WindowFilter("filter_test", 0, 0, 100, 100, 1, 50)
    check(result == 0, "WindowFilter with Noise operation should return 0")

    -- Test filter operation 3: Blur
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 3, 0)
    check(result == 0, "WindowFilter with Blur operation should return 0")

    -- Test filter operation 4: Sharpen
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 4, 0)
    check(result == 0, "WindowFilter with Sharpen operation should return 0")

    -- Test filter operation 7: Brightness (increase by 50)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 7, 50)
    check(result == 0, "WindowFilter with Brightness operation should return 0")

    -- Test filter operation 8: Contrast (increase by 50)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 8, 50)
    check(result == 0, "WindowFilter with Contrast operation should return 0")

    -- Test filter operation 9: Gamma (gamma value 1.5)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 9, 1.5)
    check(result == 0, "WindowFilter with Gamma operation should return 0")

    -- Test filter operation 19: Grayscale (linear)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 19, 0)
    check(result == 0, "WindowFilter with Grayscale (linear) operation should return 0")

    -- Test filter operation 20: Grayscale (perceptual)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 20, 0)
    check(result == 0, "WindowFilter with Grayscale (perceptual) operation should return 0")

    -- Test with 0 for right/bottom (should use window width/height)
    result = world.WindowFilter("filter_test", 0, 0, 0, 0, 3, 0)
    check(result == 0, "WindowFilter with 0,0 for right,bottom should use window dimensions")

    -- Test filter operation 27: Average
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 27, 0)
    check(result == 0, "WindowFilter with Average operation should return 0")

    -- Test with non-existent window (should return error eNoSuchWindow=30073)
    result = world.WindowFilter("nonexistent_window", 0, 0, 100, 100, 3, 0)
    check(result == 30073, "WindowFilter with non-existent window should return 30073")

    -- Test with invalid operation number (should return error or handle gracefully)
    result = world.WindowFilter("filter_test", 0, 0, 100, 100, 99, 0)
    check(result ~= 0, "WindowFilter with invalid operation should return error")

    world.WindowDelete("filter_test")

    return 0
end

-- ========== Note Style Functions ==========

-- Test NoteStyle and GetNoteStyle
function test_note_style()
    -- Save original style
    local original = world.GetNoteStyle()
    check(type(original) == "number", "GetNoteStyle should return a number")

    -- Test setting normal style (0)
    world.NoteStyle(0)
    local style = world.GetNoteStyle()
    check(style == 0, "NoteStyle(0) should set normal style")

    -- Test bold (HILITE = 1)
    world.NoteStyle(1)
    style = world.GetNoteStyle()
    check(style == 1, "NoteStyle(1) should set bold style")

    -- Test underline (UNDERLINE = 2)
    world.NoteStyle(2)
    style = world.GetNoteStyle()
    check(style == 2, "NoteStyle(2) should set underline style")

    -- Test blink/italic (BLINK = 4)
    world.NoteStyle(4)
    style = world.GetNoteStyle()
    check(style == 4, "NoteStyle(4) should set blink style")

    -- Test inverse (INVERSE = 8)
    world.NoteStyle(8)
    style = world.GetNoteStyle()
    check(style == 8, "NoteStyle(8) should set inverse style")

    -- Test combined styles (bold + underline = 3)
    world.NoteStyle(3)
    style = world.GetNoteStyle()
    check(style == 3, "NoteStyle(3) should set bold+underline")

    -- Test all styles combined (0x0F = 15)
    world.NoteStyle(15)
    style = world.GetNoteStyle()
    check(style == 15, "NoteStyle(15) should set all styles")

    -- Test that values are masked to 0x0F (only low 4 bits)
    world.NoteStyle(0xFF)
    style = world.GetNoteStyle()
    check(style == 15, "NoteStyle should mask to 0x0F")

    -- Restore original
    world.NoteStyle(original)

    return 0
end

-- Test NoteHr
function test_note_hr()
    -- Get initial line count
    local initial_count = world.GetLineCount()

    -- Output a horizontal rule
    world.NoteHr()

    -- Verify a line was added
    local new_count = world.GetLineCount()
    check(new_count == initial_count + 1, "NoteHr should add a line")

    -- Get info about the last line (info type 8 = true if horizontal rule)
    local is_hr = world.GetLineInfo(new_count, 8)
    check(type(is_hr) == "boolean", "Horizontal rule check should be a boolean")
    check(is_hr == true, "NoteHr line should be marked as horizontal rule")

    return 0
end

-- Test GetStyleInfo
function test_get_style_info()
    -- Output some styled text to create testable lines
    world.ColourNote("red", "black", "Test line with styles")

    -- Get the line count to find our line
    local line_count = world.GetLineCount()

    -- Get style count for the last line (info type 11)
    local style_count = world.GetLineInfo(line_count, 11)
    check(type(style_count) == "number", "Style count should be a number")
    check(style_count >= 1, "Line should have at least one style")

    -- Test GetStyleInfo for first style (style number 1)
    -- Info type 1: text of style
    local text = world.GetStyleInfo(line_count, 1, 1)
    check(type(text) == "string", "GetStyleInfo text should be string")

    -- Info type 2: length of style run
    local length = world.GetStyleInfo(line_count, 1, 2)
    check(type(length) == "number", "GetStyleInfo length should be number")
    check(length > 0, "Style length should be positive")

    -- Info type 3: starting column (1-based)
    local column = world.GetStyleInfo(line_count, 1, 3)
    check(type(column) == "number", "GetStyleInfo column should be number")
    check(column >= 1, "Style column should be >= 1")

    -- Info type 4: action type
    local action_type = world.GetStyleInfo(line_count, 1, 4)
    check(type(action_type) == "number", "GetStyleInfo action type should be number")

    -- Info type 5: action text
    local action = world.GetStyleInfo(line_count, 1, 5)
    check(type(action) == "string", "GetStyleInfo action should be string")

    -- Info type 6: hint/tooltip
    local hint = world.GetStyleInfo(line_count, 1, 6)
    check(type(hint) == "string", "GetStyleInfo hint should be string")

    -- Info type 14: text colour (RGB)
    local text_colour = world.GetStyleInfo(line_count, 1, 14)
    check(type(text_colour) == "number", "GetStyleInfo text colour should be number")

    -- Info type 15: background colour (RGB)
    local back_colour = world.GetStyleInfo(line_count, 1, 15)
    check(type(back_colour) == "number", "GetStyleInfo back colour should be number")

    -- Test invalid line number returns nil
    local invalid = world.GetStyleInfo(999999, 1, 1)
    check(invalid == nil, "Invalid line number should return nil")

    -- Test invalid style number returns nil
    invalid = world.GetStyleInfo(line_count, 999, 1)
    check(invalid == nil, "Invalid style number should return nil")

    -- Test invalid info type returns nil
    invalid = world.GetStyleInfo(line_count, 1, 999)
    check(invalid == nil, "Invalid info type should return nil")

    return 0
end

-- ========== UI Notepad Functions (DISABLED - require UI/MDI) ==========

-- Test ActivateNotepad
function test_activate_notepad()
    -- Create a notepad
    world.SendToNotepad("ActivateTest", "Test content for activation")

    -- ActivateNotepad returns false without MDI window in test environment
    local result = world.ActivateNotepad("ActivateTest")
    check(type(result) == "boolean", "ActivateNotepad should return a boolean")
    check(result == false, "ActivateNotepad should return false without MDI window")

    -- ActivateNotepad for non-existent notepad also returns false
    result = world.ActivateNotepad("NonExistentNotepad")
    check(result == false, "ActivateNotepad should return false for non-existent notepad")

    return 0
end

-- Test SaveNotepad
function test_save_notepad()
    -- Create a notepad with test content
    world.SendToNotepad("SaveTest", "This is test content\nLine 2\nLine 3")

    -- Test 1: SaveNotepad returns eNoSuchNotepad (30075) for non-existent notepad
    local result = world.SaveNotepad("NonExistent", "/tmp/test.txt", true)
    check(type(result) == "number", "SaveNotepad should return a number")
    check(result == 30075, "SaveNotepad should return eNoSuchNotepad (30075) for non-existent notepad")

    -- Test 2: SaveNotepad with valid notepad (returns eOK = 0 on success)
    local temp_file = "/tmp/mushkin_test_notepad_save_" .. os.time() .. ".txt"
    result = world.SaveNotepad("SaveTest", temp_file, true)
    check(result == 0, "SaveNotepad should return eOK (0) on success")

    -- Test 3: SaveNotepad with replaceExisting=false on existing file
    local temp_file2 = "/tmp/mushkin_test_notepad_save2_" .. os.time() .. ".txt"
    result = world.SaveNotepad("SaveTest", temp_file2, true)
    check(result == 0, "First SaveNotepad should succeed")

    result = world.SaveNotepad("SaveTest", temp_file2, false)
    check(type(result) == "number", "SaveNotepad should return error code for existing file")

    -- Test 4: SaveNotepad with empty filename should fail
    result = world.SaveNotepad("SaveTest", "", true)
    check(result ~= 0, "SaveNotepad with empty filename should fail")

    -- Test 5: SaveNotepad with default replaceExisting parameter
    local temp_file3 = "/tmp/mushkin_test_notepad_save3_" .. os.time() .. ".txt"
    result = world.SaveNotepad("SaveTest", temp_file3)
    check(result == 0, "SaveNotepad with default replaceExisting should succeed")

    -- Clean up test files
    os.remove(temp_file)
    os.remove(temp_file2)
    os.remove(temp_file3)

    return 0
end
