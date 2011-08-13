#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/Process.h>
#include "dfhack/extra/stopwatch.h"
#include <vector>
#include <string>


#include "tinythread.h"


#include "luamain.h"
#include "lua_Console.h"
#include "lua_Process.h"
#include "lua_Hexsearch.h"
#include "lua_Misc.h"
#include "lua_VersionInfo.h"
#include "functioncall.h"

using std::vector;
using std::string;
using namespace DFHack;

static tthread::mutex* mymutex=0;
uint64_t timeLast=0;

DFhackCExport command_result dfusion (Core * c, vector <string> & parameters);
DFhackCExport command_result lua_run (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
	//maybe remake it to run automaticaly
	lua::RegisterConsole(lua::glua::Get(),&c->con);
	lua::RegisterProcess(lua::glua::Get(),c->p);
	lua::RegisterHexsearch(lua::glua::Get());
	lua::RegisterMisc(lua::glua::Get());
	lua::RegisterVersionInfo(lua::glua::Get());
    commands.push_back(PluginCommand("dfusion","Init dfusion system.",dfusion));
	commands.push_back(PluginCommand("lua", "Run interactive interpreter.\
\n              Options: <filename> = run <filename> instead",lua_run));

	mymutex=new tthread::mutex;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{

// shutdown stuff
	delete mymutex;
	return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
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
			c->con.printerr("Error OnTick:%s\n",e.what());
            c->con.printerr("%s",lua::DebugDump(lua::glua::Get()).c_str());
			c->con.msleep(1000);
		}
	}
	s.settop(0);
	mymutex->unlock();
	return CR_OK;
}

void InterpreterLoop(Core* c)
{
	Console &con=c->con;
	DFHack::CommandHistory hist;
	lua::state s=lua::glua::Get();
	string curline;
	con.print("Type quit to exit interactive mode\n");
	con.lineedit(">>",curline,hist);

	while (curline!="quit") {
		hist.add(curline);
		try
		{
			s.loadstring(curline);
			s.pcall();
		}
		catch(lua::exception &e)
		{
			con.printerr("Error:%s\n",e.what());
			c->con.printerr("%s",lua::DebugDump(lua::glua::Get()).c_str());
			s.settop(0);
		}
		con.lineedit(">>",curline,hist);
	}
	s.settop(0);
}
DFhackCExport command_result lua_run (Core * c, vector <string> & parameters)
{
	Console &con=c->con;
	mymutex->lock();
	lua::state s=lua::glua::Get();
	if(parameters.size()>0)
	{
		try{
			s.loadfile(parameters[0]); //load file
			s.pcall(0,0);// run it
		}
		catch(lua::exception &e)
		{
			con.printerr("Error:%s\n",e.what());
			c->con.printerr("%s",lua::DebugDump(lua::glua::Get()).c_str());
		}
	}
	else
	{
		InterpreterLoop(c);
	}
	s.settop(0);// clean up
	mymutex->unlock();
	return CR_OK;
}
DFhackCExport command_result dfusion (Core * c, vector <string> & parameters)
{

	Console &con=c->con;
	mymutex->lock();
	lua::state s=lua::glua::Get();

	try{
		s.loadfile("dfusion/init.lua"); //load script
		s.pcall(0,0);// run it
	}
	catch(lua::exception &e)
	{
		con.printerr("Error:%s\n",e.what());
		c->con.printerr("%s",lua::DebugDump(lua::glua::Get()).c_str());
	}
	s.settop(0);// clean up
	mymutex->unlock();
	return CR_OK;
}
