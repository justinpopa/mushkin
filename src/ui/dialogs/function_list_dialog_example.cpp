// Example usage of FunctionListDialog
// This file is for reference only and is not compiled into the project

#include "function_list_dialog.h"
#include <QApplication>

/*
 * Example 1: Simple usage with a list of Lua functions
 */
void example1()
{
    FunctionListDialog dialog("Available Lua Functions", nullptr);

    // Add individual items
    dialog.addItem("print", "Outputs a message to the output window");
    dialog.addItem("Note", "Displays a note in the output window");
    dialog.addItem("Send", "Sends text to the MUD");
    dialog.addItem("GetVariable", "Retrieves a variable value");
    dialog.addItem("SetVariable", "Sets a variable value");

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedFunc = dialog.selectedName();
        QString description = dialog.selectedDescription();

        qDebug() << "Selected function:" << selectedFunc;
        qDebug() << "Description:" << description;
    }
}

/*
 * Example 2: Using setItems() to populate the list all at once
 */
void example2()
{
    QList<QPair<QString, QString>> functions;

    // Build list of function name -> description pairs
    functions.append(qMakePair("AddAlias", "Creates a new alias"));
    functions.append(qMakePair("AddTimer", "Creates a new timer"));
    functions.append(qMakePair("AddTrigger", "Creates a new trigger"));
    functions.append(qMakePair("DeleteAlias", "Removes an alias"));
    functions.append(qMakePair("DeleteTimer", "Removes a timer"));
    functions.append(qMakePair("DeleteTrigger", "Removes a trigger"));
    functions.append(qMakePair("EnableAlias", "Enables or disables an alias"));
    functions.append(qMakePair("EnableTimer", "Enables or disables a timer"));
    functions.append(qMakePair("EnableTrigger", "Enables or disables a trigger"));

    FunctionListDialog dialog("MUSHclient Functions", nullptr);
    dialog.setItems(functions);

    // Pre-filter to show only "Add" functions
    dialog.setFilter("Add");

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedFunc = dialog.selectedName();
        qDebug() << "Selected:" << selectedFunc;
    }
}

/*
 * Example 3: Using for any key-value pairs (not just functions)
 */
void example3()
{
    QList<QPair<QString, QString>> variables;

    variables.append(qMakePair("player_name", "Gandalf"));
    variables.append(qMakePair("player_level", "50"));
    variables.append(qMakePair("player_hp", "1234"));
    variables.append(qMakePair("player_mana", "567"));
    variables.append(qMakePair("current_room", "Town Square"));

    FunctionListDialog dialog("World Variables", nullptr);
    dialog.setItems(variables);

    if (dialog.exec() == QDialog::Accepted) {
        QString varName = dialog.selectedName();
        QString varValue = dialog.selectedDescription();

        qDebug() << "Variable:" << varName << "=" << varValue;
    }
}

/*
 * Example 4: Integration in a real world scenario
 * This shows how it might be used from MainWindow or another class
 */
void example4_RealWorldUsage()
{
    // Suppose we want to show all available world methods to the user
    FunctionListDialog dialog("World Scripting Functions", nullptr);

    QList<QPair<QString, QString>> worldFunctions;
    worldFunctions.append(qMakePair("world.Note", "Display a note in the output window"));
    worldFunctions.append(qMakePair("world.Send", "Send text to the MUD"));
    worldFunctions.append(qMakePair("world.GetVariable", "Get a variable's value"));
    worldFunctions.append(qMakePair("world.SetVariable", "Set a variable's value"));
    worldFunctions.append(qMakePair("world.AddTrigger", "Add a new trigger"));
    worldFunctions.append(qMakePair("world.AddAlias", "Add a new alias"));
    worldFunctions.append(qMakePair("world.AddTimer", "Add a new timer"));
    worldFunctions.append(qMakePair("world.ColourNote", "Display colored text"));

    dialog.setItems(worldFunctions);

    // User can filter, sort by clicking columns, double-click to select
    if (dialog.exec() == QDialog::Accepted) {
        QString functionName = dialog.selectedName();

        // Insert the function name into a script editor or command line
        // For example: m_scriptEditor->insertText(functionName);
        qDebug() << "Inserting function:" << functionName;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Run one of the examples
    example1();
    // example2();
    // example3();
    // example4_RealWorldUsage();

    return 0;
}
