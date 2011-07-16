#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>


using std::vector;
using std::string;
using namespace DFHack;


DFhackCExport command_result dfusion (Core * c, vector <string> & parameters);


DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("DFusion","Init dfusion system.",dfusion));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
// shutdown stuff
	return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    /*if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        c->con.print("Time delta = %d ms\n", delta);
    }
    return CR_OK;*/
	return CR_OK;
}

DFhackCExport command_result dfusion (Core * c, vector <string> & parameters)
{
   // do stuff
	return CR_OK;
}
