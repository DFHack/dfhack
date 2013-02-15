
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include <iostream>

using namespace DFHack;
using namespace std;

command_result printArgs (color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("printArgs");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "printArgs", "Print the arguments given.",
        printArgs, false
    ));
    return CR_OK;
}

command_result printArgs (color_ostream &out, std::vector <std::string> & parameters)
{
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        out << "Argument " << (a+1) << ": \"" << parameters[a] << "\"" << endl;
    }
    return CR_OK;
}
