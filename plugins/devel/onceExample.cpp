
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Once.h"

using namespace DFHack;
using namespace df::enums;

command_result onceExample (color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("onceExample");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "onceExample", "Test the doOnce command.",
        onceExample, false,
        "  This command tests the doOnce command..\n"
    ));
    return CR_OK;
}

command_result onceExample (color_ostream &out, std::vector <std::string> & parameters)
{
    out.print("Already done = %d.\n", DFHack::Once::alreadyDone("onceExample_1"));
    if ( DFHack::Once::doOnce("onceExample_1") ) {
        out.print("Printing this message once!\n");
    }
    return CR_OK;
}
