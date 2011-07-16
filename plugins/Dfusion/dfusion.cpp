#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>


#include "luamain.h"
#include "lua_Console.h"

using std::vector;
using std::string;
using namespace DFHack;

static SDL::Mutex* mymutex=0;

DFhackCExport command_result dfusion (Core * c, vector <string> & parameters);


DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
	//maybe remake it to run automaticaly
	lua::RegisterConsole(lua::glua::Get(),&c->con);

    commands.push_back(PluginCommand("dfusion","Init dfusion system.",dfusion));
	mymutex=SDL_CreateMutex();
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
	
// shutdown stuff
	return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    /*if(timering == true) //TODO maybe reuse this to make it run less often.
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        c->con.print("Time delta = %d ms\n", delta);
    }
    return CR_OK;*/
	SDL_mutexP(mymutex); 
	lua::state s=lua::glua::Get();
	s.getglobal("OnTick");
	if(s.is<lua::function>())
	{
		s.pcall();
	}
	s.settop(0);
	SDL_mutexV(mymutex);
	return CR_OK;
}


DFhackCExport command_result dfusion (Core * c, vector <string> & parameters)
{
   // do stuff
	Console &con=c->con;
	SDL_mutexP(mymutex);
	lua::state s=lua::glua::Get();
	
	try{
		s.loadfile("dfusion/init.lua"); //load script
		s.pcall(0,0);// run it
		
	}
	catch(lua::exception &e)
	{
		con.printerr("Error:%s\n",e.what());
	}
	s.settop(0);// clean up
	SDL_mutexV(mymutex);
	return CR_OK;
}
