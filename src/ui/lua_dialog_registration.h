/**
 * lua_dialog_registration.h - Register UI dialogs with Lua callback system
 *
 * This module provides the registration function that connects the ui module's
 * dialog implementations to the world module's Lua callback system.
 */

#ifndef LUA_DIALOG_REGISTRATION_H
#define LUA_DIALOG_REGISTRATION_H

namespace LuaDialogRegistration {

/**
 * Register all dialog callbacks
 *
 * Call this at application startup (e.g., in main.cpp or MainWindow constructor)
 * to connect the ui module's dialogs to the Lua utils API.
 */
void registerDialogCallbacks();

} // namespace LuaDialogRegistration

#endif // LUA_DIALOG_REGISTRATION_H
