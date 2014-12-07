#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

//#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("skeleton2");
//REQUIRE_GLOBAL(world);

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "skeleton2",
        "shortHelpString",
        skeleton2,
        false, //allow non-interactive use
        "longHelpString"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    out.print("blah");
    return CR_OK;
}

