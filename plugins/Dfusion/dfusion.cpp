#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include <vector>
#include <string>


#include "tinythread.h"


#include "luamain.h"
#include "lua_Process.h"
#include "lua_Hexsearch.h"
#include "lua_Misc.h"


#include "DataDefs.h"
#include "LuaTools.h"

using std::vector;
using std::string;
using namespace DFHack;

static tthread::mutex* mymutex=0;
static tthread::thread* thread_dfusion=0;
uint64_t timeLast=0;

DFHACK_PLUGIN("dfusion")

command_result dfusion (color_ostream &out, std::vector <std::string> &parameters);
command_result dfuse (color_ostream &out, std::vector <std::string> &parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
	lua::state st=lua::glua::Get();

	//maybe remake it to run automatically
    Lua::Open(out, st);

	lua::RegisterProcess(st);
	lua::RegisterHexsearch(st);
	lua::RegisterMisc(st);

	#ifdef LINUX_BUILD
		st.push(1);
		st.setglobal("LINUX");
	#else
		st.push(1);
		st.setglobal("WINDOWS");
	#endif

    commands.push_back(PluginCommand("dfusion","Run dfusion system (interactive i.e. can input further commands).",dfusion,true));
	commands.push_back(PluginCommand("dfuse","Init dfusion system (not interactive).",dfuse,false));
	mymutex=new tthread::mutex;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{

// shutdown stuff
	if(thread_dfusion)
		delete thread_dfusion;
	delete mymutex;
	return CR_OK;
}

DFhackCExport command_result plugin_onupdate_DISABLED ( Core * c )
{
	uint64_t time2 = GetTimeMs64();
	uint64_t delta = time2-timeLast;
	if(delta<100)
		return CR_OK;
	timeLast = time2;
	mymutex->lock();
	lua::state s=lua::glua::Get();
	s.getglobal("OnTick");
	if(s.is<lua::function>())
	{
		try{
			s.pcall();
		}
		catch(lua::exception &e)
		{
			c->getConsole().printerr("Error OnTick:%s\n",e.what());
            c->getConsole().printerr("%s\n",lua::DebugDump(lua::glua::Get()).c_str());
			c->getConsole().msleep(1000);
		}
	}
	s.settop(0);
	mymutex->unlock();
	return CR_OK;
}
void RunDfusion(color_ostream &out, std::vector <std::string> &parameters)
{
	mymutex->lock();
	lua::state s=lua::glua::Get();
	try{
		s.loadfile("dfusion/init.lua"); //load script
		for(size_t i=0;i<parameters.size();i++)
				s.push(parameters[i]);
        Lua::SafeCall(out, s, parameters.size(),0);
	}
	catch(lua::exception &e)
	{
		out.printerr("Error:%s\n",e.what());
        out.printerr("%s\n",lua::DebugDump(lua::glua::Get()).c_str());
	}
	s.settop(0);// clean up
	mymutex->unlock();
}
command_result dfuse(color_ostream &out, std::vector <std::string> &parameters)
{
	lua::state s=lua::glua::Get();
	s.push(1);
	s.setglobal("INIT");
	RunDfusion(out,parameters);
	return CR_OK;
}
command_result dfusion (color_ostream &out, std::vector <std::string> &parameters)
{
	lua::state s=lua::glua::Get();
	s.push();
	s.setglobal("INIT");
	RunDfusion(out,parameters);
	return CR_OK;
}
