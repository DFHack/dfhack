#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("cromulate");

command_result cromulate (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("cromulate",
                                     "in-cpp plugin short desc", //to use one line in the ``[DFHack]# ls`` output
                                     cromulate));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
}

command_result cromulate (color_ostream &out, std::vector <std::string> &parameters) {
    return CR_OK;
}
