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
#include "lua_VersionInfo.h"
#include "functioncall.h"
#include "lua_FunctionCall.h"
#include "lua_Offsets.h"
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
command_result lua_run (color_ostream &out, std::vector <std::string> &parameters);
command_result lua_run_file (color_ostream &out, std::vector <std::string> &parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "dfusion";
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
	lua::state st=lua::glua::Get();

	//maybe remake it to run automaticaly
    Lua::Open(out, st);

	lua::RegisterProcess(st);
	lua::RegisterHexsearch(st);
	lua::RegisterMisc(st);
	lua::RegisterVersionInfo(st);
	lua::RegisterFunctionCall(st);
	lua::RegisterEngine(st);

	#ifdef LINUX_BUILD
		st.push(1);
		st.setglobal("LINUX");
	#else
		st.push(1);
		st.setglobal("WINDOWS");
	#endif

    commands.push_back(PluginCommand("dfusion","Run dfusion system (interactive i.e. can input further commands).",dfusion,true));
	commands.push_back(PluginCommand("dfuse","Init dfusion system (not interactive).",dfuse,false));
	commands.push_back(PluginCommand("lua", "Run interactive interpreter. Use 'lua <filename>' to run <filename> instead.",lua_run,true));
	commands.push_back(PluginCommand("runlua", "Run non-interactive interpreter. Use 'runlua <filename>' to run <filename>.",lua_run_file,false));
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
command_result lua_run_file (color_ostream &out, std::vector <std::string> &parameters)
{
	if(parameters.size()==0)
	{
		out.printerr("runlua without file to run!");
		return CR_FAILURE;
	}
	return lua_run(out,parameters);
}
command_result lua_run (color_ostream &out, std::vector <std::string> &parameters)
{
    if (!parameters.empty())
    {
        if (parameters[0] == "--core-context")
        {
            Lua::InterpreterLoop(out, Lua::Core::State, "core lua");
            return CR_OK;
        }
        else if (parameters[0] == "--core-reload")
        {
            CoreSuspender suspend;

            for (size_t i = 1; i < parameters.size(); i++)
            {
                lua_getglobal(Lua::Core::State, "reload");
                lua_pushstring(Lua::Core::State, parameters[i].c_str());
                Lua::SafeCall(out, Lua::Core::State, 1, 0);
            }

            return CR_OK;
        }
    }

	mymutex->lock();
	lua::state s=lua::glua::Get();

	if(parameters.size()>0)
	{
		try{
			s.loadfile(parameters[0]); //load file
			for(size_t i=1;i<parameters.size();i++)
				s.push(parameters[i]);
            Lua::SafeCall(out, s, parameters.size()-1,0);// run it
		}
		catch(lua::exception &e)
		{
			out.printerr("Error:%s\n",e.what());
            out.printerr("%s\n",lua::DebugDump(lua::glua::Get()).c_str());
		}
	}
	else
	{
		Lua::InterpreterLoop(out, s);
	}
	s.settop(0);// clean up
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
