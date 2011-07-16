#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>




#include <luamain.h>

using std::vector;
using std::string;
using namespace DFHack;

static SDL::Mutex *mymutex=0;

DFhackCExport command_result dfusion (Core * c, vector <string> & parameters);


DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("DFusion","Init dfusion system.",dfusion));
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
    /*if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        c->con.print("Time delta = %d ms\n", delta);
    }
    return CR_OK;*/
	SDL_mutexP(mymutex); //TODO: make lua thread safe (somehow)...
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
	try{
		lua::glua::Get().loadfile("dfusion/init.lua"); //load script
		lua::glua::Get().pcall(0,0);// run it
		
	}
	catch(lua::exception &e)
	{
		con.printerr("Error:%s\n",e.what());
	}
	lua::glua::Get().settop(0);// clean up
	SDL_mutexV(mymutex);
	return CR_OK;
}
