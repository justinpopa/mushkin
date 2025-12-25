-- luacom.lua
-- Stub module for non-Windows platforms
--
-- The original MUSHclient includes luacom for Windows COM automation.
-- This stub allows plugins that try to require "luacom" to work gracefully
-- on macOS and Linux by returning false instead of throwing an error.
--
-- Plugins should check if luacom is available before using it:
--   local luacom = require "luacom"
--   if luacom then
--       -- Windows-specific code
--   end
--
-- Note: We return false (not nil) because Lua's require() treats nil returns
-- as "no return value" and substitutes true, which would break the if-check.

return false
