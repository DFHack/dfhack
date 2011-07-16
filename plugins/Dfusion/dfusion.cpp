#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>

#include <lua.hpp>

using std::vector;
using std::string;
using namespace DFHack;

lua_State *st;
DFhackCExport command_result dfusion (Core * c, vector <string> & parameters);


DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("DFusion","Init dfusion system.",dfusion));
	st=luaL_newstate();
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
	
	Console &con=c->con;

	con.print("Hello world!");
	luaL_dostring(st,parameters[0].c_str());
	const char* p=luaL_checkstring(st,1);
	con.print("ret=%s",p);
	return CR_OK;
}
