// Example usage of DebugLuaDialog
// This file demonstrates how to use the Lua debugger dialog
// This is NOT compiled - it's just for reference

#include "debug_lua_dialog.h"
#include <QApplication>

void exampleUsage()
{
    // Create the dialog
    DebugLuaDialog* debugDialog = new DebugLuaDialog();

    // Set debug information from Lua debug hooks
    debugDialog->setCurrentLine("42");
    debugDialog->setFunctionName("myFunction");
    debugDialog->setSource("my_script.lua");
    debugDialog->setWhat("Lua");
    debugDialog->setLines("1-100");
    debugDialog->setNups("2");

    // Connect signals to handle debug commands
    QObject::connect(debugDialog, &DebugLuaDialog::executeCommand, [](const QString& cmd) {
        // Execute custom debug command in Lua
        // Example: lua_getfield, lua_setfield, etc.
    });

    QObject::connect(debugDialog, &DebugLuaDialog::showLocals, []() {
        // Enumerate and display local variables
        // Use lua_getlocal to iterate through locals
    });

    QObject::connect(debugDialog, &DebugLuaDialog::showUpvalues, []() {
        // Enumerate and display upvalues
        // Use lua_getupvalue to iterate through upvalues
    });

    QObject::connect(debugDialog, &DebugLuaDialog::showTraceback, []() {
        // Generate and display stack traceback
        // Use lua_Debug and lua_getstack to walk the stack
    });

    QObject::connect(debugDialog, &DebugLuaDialog::abortExecution, []() {
        // Abort the current Lua execution
        // Use lua_error or similar mechanism
    });

    QObject::connect(debugDialog, &DebugLuaDialog::continueExecution, []() {
        // Continue execution from breakpoint
        // Return from debug hook
    });

    // Show the dialog (modal)
    int result = debugDialog->exec();

    // Check if user aborted
    if (debugDialog->wasAborted()) {
        // Handle abort
    }

    // Clean up
    delete debugDialog;
}

// Integration with Lua debug hook:
//
// static void lua_debug_hook(lua_State *L, lua_Debug *ar)
// {
//     lua_getinfo(L, "Slnu", ar);
//
//     DebugLuaDialog* dialog = new DebugLuaDialog();
//     dialog->setCurrentLine(QString::number(ar->currentline));
//     dialog->setFunctionName(ar->name ? ar->name : "<anonymous>");
//     dialog->setSource(ar->source);
//     dialog->setWhat(ar->what);
//     dialog->setLines(QString("%1-%2").arg(ar->linedefined).arg(ar->lastlinedefined));
//     dialog->setNups(QString::number(ar->nups));
//
//     // Handle dialog signals...
//
//     dialog->exec();
//     delete dialog;
// }
