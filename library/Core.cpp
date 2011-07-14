/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sstream>
using namespace std;

#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "dfhack/Core.h"
#include "dfhack/Console.h"
#include "dfhack/Module.h"
#include "dfhack/VersionInfoFactory.h"
#include "dfhack/PluginManager.h"
#include "ModuleFactory.h"
#include "dfhack/modules/Gui.h"

#include "dfhack/SDL_fakes/events.h"
#include "linenoise.h"

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
using namespace DFHack;

    struct Core::Cond
    {
        Cond()
        {
            predicate = false;
            wakeup = SDL_CreateCond();
        }
        ~Cond()
        {
            SDL_DestroyCond(wakeup);
        }
        bool Lock(SDL::Mutex * m)
        {
            while(!predicate)
            {
                SDL_CondWait(wakeup,m);
            }
            predicate = false;
            return true;
        }
        bool Unlock()
        {
            predicate = true;
            SDL_CondSignal(wakeup);
            return true;
        }
        SDL::Cond * wakeup;
        bool predicate;
    };

void cheap_tokenise(string const& input, vector<string> &output)
{
    istringstream str(input);
    istream_iterator<string> cur(str), end;
    output.assign(cur, end);
}

struct IODATA
{
    Core * core;
    PluginManager * plug_mgr;
};

// A thread function... for handling hotkeys. This is needed because
// all the plugin commands are expected to be run from foreign threads.
// Running them from one of the main DF threads will result in deadlock!
int fHKthread(void * iodata)
{
    Core * core = ((IODATA*) iodata)->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;
    if(plug_mgr == 0 || core == 0)
    {
        cerr << "Hotkey thread has croaked." << endl;
        return 0;
    }
    while(1)
    {
        std::string stuff = core->getHotkeyCmd(); // waits on mutex!
        if(!stuff.empty())
        {
            vector <string> crap;
            plug_mgr->InvokeCommand(stuff, crap);
        }
    }
}

// A thread function... for the interactive console.
int fIOthread(void * iodata)
{
    IODATA * iod = ((IODATA*) iodata);
    Core * core = iod->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;
    Console & con = core->con;
    if(plug_mgr == 0 || core == 0)
    {
        con.print("Something horrible happened in Core's constructor...\n");
        return 0;
    }
    con.print("DFHack is ready. Have a nice day! Type in '?' or 'help' for help.\n");
    //dfterm <<  << endl;
    int clueless_counter = 0;
    while (true)
    {
        string command = "";
        con.lineedit("[DFHack]# ",command);
        con.history_add(command);
        //con <<"[DFHack]# ";
        //char * line = linenoise("[DFHack]# ", core->con.dfout_C);
        //        dfout <<"[DFHack]# ";
        /*
        if (line)
        {
            command=line;
            linenoiseHistoryAdd(line);
            free(line);
        }*/
        //getline(cin, command);
        if(command=="help" || command == "?")
        {
            con.print("Available commands\n");
            con.print("------------------\n");
            for(int i = 0; i < plug_mgr->size();i++)
            {
                const Plugin * plug = (plug_mgr->operator[](i));
                if(!plug->size())
                    continue;
                con.print("Plugin %s :\n", plug->getName().c_str());
                for (int j = 0; j < plug->size();j++)
                {
                    const PluginCommand & pcmd = (plug->operator[](j));
                    con.print("%12s| %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                    //con << setw(12) << pcmd.name << "| " << pcmd.description << endl;
                }
                con.print("\n");
            }
        }
        else if( command == "" )
        {
            clueless_counter++;
        }
        else
        {
            vector <string> parts;
            cheap_tokenise(command,parts);
            if(parts.size() == 0)
            {
                clueless_counter++;
            }
            else
            {
                string first = parts[0];
                parts.erase(parts.begin());
                command_result res = plug_mgr->InvokeCommand(first, parts);
                if(res == CR_NOT_IMPLEMENTED)
                {
                    con.print("Invalid command.\n");
                    clueless_counter ++;
                }
                else if(res == CR_FAILURE)
                {
                    con.print("ERROR!\n");
                }
            }
        }
        if(clueless_counter == 3)
        {
            con.print("Do 'help' or '?' for the list of available commands.\n");
            clueless_counter = 0;
        }
    }
}

Core::Core()
{
    // init the console. This must be always the first step!
    plug_mgr = 0;
    vif = 0;
    p = 0;
    errorstate = false;
    vinfo = 0;
    started = false;
    memset(&(s_mods), 0, sizeof(s_mods));

    // create mutex for syncing with interactive tasks
    AccessMutex = 0;
    core_cond = 0;
    // set up hotkey capture
    memset(hotkey_states,0,sizeof(hotkey_states));
    hotkey_set = false;
    HotkeyMutex = 0;
    HotkeyCond = 0;
};

bool Core::Init()
{
    // init the console. This must be always the first step!
    con.init();
    // find out what we are...
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        con.print("Couldn't identify this version of DF.\n");
        errorstate = true;
        delete p;
        p = NULL;
        return false;
    }
    vinfo = p->getDescriptor();

    // create mutex for syncing with interactive tasks
    AccessMutex = SDL_CreateMutex();
    if(!AccessMutex)
    {
        con.print("Mutex creation failed\n");
        errorstate = true;
        return false;
    }
    core_cond = new Core::Cond();
    // create plugin manager
    plug_mgr = new PluginManager(this);
    if(!plug_mgr)
    {
        con.print("Failed to create the Plugin Manager.\n");
        errorstate = true;
        return false;
    }
    // look for all plugins, 
    // create IO thread
    IODATA *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;
    SDL::Thread * IO = SDL_CreateThread(fIOthread, (void *) temp);
    // set up hotkey capture
    HotkeyMutex = SDL_CreateMutex();
    HotkeyCond = SDL_CreateCond();
    SDL::Thread * HK = SDL_CreateThread(fHKthread, (void *) temp);
    started = true;
    return true;
}
/// sets the current hotkey command
bool Core::setHotkeyCmd( std::string cmd )
{
    // access command
    SDL_mutexP(HotkeyMutex);
    {
        hotkey_set = true;
        hotkey_cmd = cmd;
        SDL_CondSignal(HotkeyCond);
    }
    SDL_mutexV(HotkeyMutex);
    return true;
}
/// removes the hotkey command and gives it to the caller thread
std::string Core::getHotkeyCmd( void )
{
    string returner;
    SDL_mutexP(HotkeyMutex);
    while ( ! hotkey_set )
    {
        SDL_CondWait(HotkeyCond, HotkeyMutex);
    }
    hotkey_set = false;
    returner = hotkey_cmd;
    hotkey_cmd.clear();
    SDL_mutexV(HotkeyMutex);
    return returner;
}


void Core::Suspend()
{
    Core::Cond * nc = new Core::Cond();
    // put the condition on a stack
    SDL_mutexP(StackMutex);
        suspended_tools.push(nc);
    SDL_mutexV(StackMutex);
    // wait until Core::Update() wakes up the tool
    SDL_mutexP(AccessMutex);
        nc->Lock(AccessMutex);
    SDL_mutexV(AccessMutex);
}

void Core::Resume()
{
    SDL_mutexP(AccessMutex);
        core_cond->Unlock();
    SDL_mutexV(AccessMutex);
}

// should always be from simulation thread!
int Core::Update()
{
    if(!started) Init();
    if(errorstate)
        return -1;

    // notify all the plugins that a game tick is finished
    plug_mgr->OnUpdate();
    // wake waiting tools
    // do not allow more tools to join in while we process stuff here
    SDL_mutexP(StackMutex);
    while (!suspended_tools.empty())
    {
        Core::Cond * nc = suspended_tools.top();
        suspended_tools.pop();
        SDL_mutexP(AccessMutex);
            // wake tool
            nc->Unlock();
            // wait for tool to wake us
            core_cond->Lock(AccessMutex);
        SDL_mutexV(AccessMutex);
        // destroy condition
        delete nc;
    }
    SDL_mutexV(StackMutex);
    return 0;
};

// FIXME: needs to terminate the IO threads and properly dismantle all the machinery involved.
int Core::Shutdown ( void )
{
    errorstate = 1;
    if(plug_mgr)
    {
        delete plug_mgr;
        plug_mgr = 0;
    }
    // invalidate all modules
    for(unsigned int i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));
    con.shutdown();
    return -1;
}

int Core::SDL_Event(SDL::Event* ev, int orig_return)
{
    // do NOT process events before we are ready.
    if(!started) return orig_return;
    if(!ev)
        return orig_return;
    if(ev && ev->type == SDL::ET_KEYDOWN || ev->type == SDL::ET_KEYUP)
    {
        SDL::KeyboardEvent * ke = (SDL::KeyboardEvent *)ev;
        bool shift = ke->ksym.mod & SDL::KMOD_SHIFT;
        // consuming F1 .. F8
        int idx = ke->ksym.sym - SDL::K_F1;
        if(idx < 0 || idx > 7)
            return orig_return;
        idx += 8*shift;
        // now we have the real index...
        if(ke->state == SDL::BTN_PRESSED && !hotkey_states[idx])
        {
            hotkey_states[idx] = 1;
            Gui * g = getGui();
            if(g->hotkeys && g->interface && g->menu_state)
            {
                t_viewscreen * ws = g->GetCurrentScreen();
                if(ws->getClassName() == "viewscreen_dwarfmodest" && *g->menu_state == 0x23)
                    return orig_return;
                else
                {
                    t_hotkey & hotkey = (*g->hotkeys)[idx];
                    setHotkeyCmd(hotkey.name);
                }
            }
        }
        else if(ke->state == SDL::BTN_RELEASED)
        {
            hotkey_states[idx] = 0;
        }
    }
    return orig_return;
    // do stuff with the events...
}

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/

#define MODULE_GETTER(TYPE) \
TYPE * Core::get##TYPE() \
{ \
    if(errorstate) return NULL;\
    if(!s_mods.p##TYPE)\
    {\
        Module * mod = create##TYPE();\
        s_mods.p##TYPE = (TYPE *) mod;\
        allModules.push_back(mod);\
    }\
    return s_mods.p##TYPE;\
}

MODULE_GETTER(Creatures);
MODULE_GETTER(Engravings);
MODULE_GETTER(Maps);
MODULE_GETTER(Gui);
MODULE_GETTER(World);
MODULE_GETTER(Materials);
MODULE_GETTER(Items);
MODULE_GETTER(Translation);
MODULE_GETTER(Vegetation);
MODULE_GETTER(Buildings);
MODULE_GETTER(Constructions);
MODULE_GETTER(Vermin);
