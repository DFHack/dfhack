#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result vdig (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "vein digger";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("vdig","Dig a whole vein.",vdig));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result vdig (Core * c, vector <string> & parameters)
{
    return CR_OK;
}
