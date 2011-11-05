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
#include "dfhack/VersionInfo.h"
#include "dfhack/PluginManager.h"
#include "ModuleFactory.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/modules/World.h"
using namespace DFHack;

#include "dfhack/SDL_fakes/events.h"

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <fstream>
#include "tinythread.h"
using namespace tthread;


struct Core::Cond
{
    Cond()
    {
        predicate = false;
        wakeup = new tthread::condition_variable();
    }
    ~Cond()
    {
        delete wakeup;
    }
    bool Lock(tthread::mutex * m)
    {
        while(!predicate)
        {
            wakeup->wait(*m);
        }
        predicate = false;
        return true;
    }
    bool Unlock()
    {
        predicate = true;
        wakeup->notify_one();
        return true;
    }
    tthread::condition_variable * wakeup;
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
void fHKthread(void * iodata)
{
    Core * core = ((IODATA*) iodata)->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;
    if(plug_mgr == 0 || core == 0)
    {
        cerr << "Hotkey thread has croaked." << endl;
        return;
    }
    while(1)
    {
        std::string stuff = core->getHotkeyCmd(); // waits on mutex!
        if(!stuff.empty())
        {
            vector <string> crap;
            command_result cr = plug_mgr->InvokeCommand(stuff, crap, false);
            if(cr == CR_WOULD_BREAK)
            {
                core->con.printerr("It isn't possible to run an interactive command outside the console.\n");
            }
        }
    }
}

// A thread function... for the interactive console.
void fIOthread(void * iodata)
{
    IODATA * iod = ((IODATA*) iodata);
    Core * core = iod->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;
    CommandHistory main_history;
    main_history.load("dfhack.history");
    Console & con = core->con;
    if(plug_mgr == 0 || core == 0)
    {
        con.printerr("Something horrible happened in Core's constructor...\n");
        return;
    }
    con.print("DFHack is ready. Have a nice day!\n"
              "Type in '?' or 'help' for general help, 'ls' to see all commands.\n");
    int clueless_counter = 0;
    while (true)
    {
        string command = "";
        int ret = con.lineedit("[DFHack]# ",command, main_history);
        if(ret == -2)
        {
            cerr << "Console is shutting down properly." << endl;
            return;
        }
        else if(ret == -1)
        {
            cerr << "Console caught an unspecified error." << endl;
            continue;
        }
        else if(ret)
        {
            // a proper, non-empty command was entered
            main_history.add(command);
            main_history.save("dfhack.history");
        }
        // cut the input into parts
        vector <string> parts;
        cheap_tokenise(command,parts);
        if(parts.size() == 0)
        {
            clueless_counter ++;
            continue;
        }
        string first = parts[0];
        parts.erase(parts.begin());
        cerr << "Invoking: " << command << endl;
        
        // let's see what we actually got
        if(first=="help" || first == "?")
        {
            if(!parts.size())
            {
                con.print("This is the DFHack console. You can type commands in and manage DFHack plugins from it.\n"
                          "Some basic editing capabilities are included (single-line text editing).\n"
                          "The console also has a command history - you can navigate it with Up and Down keys.\n"
                          "On Windows, you may have to resize your console window. The appropriate menu is accessible\n"
                          "by clicking on the program icon in the top bar of the window.\n\n"
                          "Basic commands:\n"
                          "  help|?                - This text.\n"
                          "  ls|dir [PLUGIN]       - List available commands. Optionally for single plugin.\n"
                          "  cls                   - Clear the console.\n"
                          "  fpause                - Force DF to pause.\n"
                          "  die                   - Force DF to close immediately\n"
                          "Plugin management (useful for developers):\n"
                          //"  belongs COMMAND       - Tell which plugin a command belongs to.\n"
                          "  plug [PLUGIN|v]       - List plugin state and description.\n"
                          "  load PLUGIN|all       - Load a plugin by name or load all possible plugins.\n"
                          "  unload PLUGIN|all     - Unload a plugin or all loaded plugins.\n"
                          "  reload PLUGIN|all     - Reload a plugin or all loaded plugins.\n"
                         );
            }
            else
            {
                con.printerr("not implemented yet\n");
            }
        }
        else if( first == "load" )
        {
            if(parts.size())
            {
                string & plugname = parts[0];
                if(plugname == "all")
                {
                    for(int i = 0; i < plug_mgr->size();i++)
                    {
                        Plugin * plug = (plug_mgr->operator[](i));
                        plug->load();
                    }
                }
                else
                {
                    Plugin * plug = plug_mgr->getPluginByName(plugname);
                    if(!plug) con.printerr("No such plugin\n");
                    plug->load();
                }
            }
        }
        else if( first == "reload" )
        {
            if(parts.size())
            {
                string & plugname = parts[0];
                if(plugname == "all")
                {
                    for(int i = 0; i < plug_mgr->size();i++)
                    {
                        Plugin * plug = (plug_mgr->operator[](i));
                        plug->reload();
                    }
                }
                else
                {
                    Plugin * plug = plug_mgr->getPluginByName(plugname);
                    if(!plug) con.printerr("No such plugin\n");
                    plug->reload();
                }
            }
        }
        else if( first == "unload" )
        {
            if(parts.size())
            {
                string & plugname = parts[0];
                if(plugname == "all")
                {
                    for(int i = 0; i < plug_mgr->size();i++)
                    {
                        Plugin * plug = (plug_mgr->operator[](i));
                        plug->unload();
                    }
                }
                else
                {
                    Plugin * plug = plug_mgr->getPluginByName(plugname);
                    if(!plug) con.printerr("No such plugin\n");
                    plug->unload();
                }
            }
        }
        else if(first == "ls" || first == "dir")
        {
            if(parts.size())
            {
                string & plugname = parts[0];
                const Plugin * plug = plug_mgr->getPluginByName(plugname);
                if(!plug)
                {
                    con.printerr("There's no plugin called %s!\n",plugname.c_str());
                }
                else for (int j = 0; j < plug->size();j++)
                {
                    const PluginCommand & pcmd = (plug->operator[](j));
                    con.print("  %-22s - %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                }
            }
            else
            {
                con.print(
                "builtin:\n"
                "  help|?                - This text or help specific to a plugin.\n"
                "  ls [PLUGIN]           - List available commands. Optionally for single plugin.\n"
                "  cls                   - Clear the console.\n"
                "  fpause                - Force DF to pause.\n"
                "  die                   - Force DF to close immediately\n"
                "  belongs COMMAND       - Tell which plugin a command belongs to.\n"
                "  plug [PLUGIN|v]       - List plugin state and detailed description.\n"
                "  load PLUGIN|all       - Load a plugin by name or load all possible plugins.\n"
                "  unload PLUGIN|all     - Unload a plugin or all loaded plugins.\n"
                "  reload PLUGIN|all     - Reload a plugin or all loaded plugins.\n"
                "\n"
                "plugins:\n"
                );
                for(int i = 0; i < plug_mgr->size();i++)
                {
                    const Plugin * plug = (plug_mgr->operator[](i));
                    if(!plug->size())
                        continue;
                    for (int j = 0; j < plug->size();j++)
                    {
                        const PluginCommand & pcmd = (plug->operator[](j));
                        con.print("  %-22s- %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                    }
                }
            }
        }
        else if(first == "plug")
        {
            for(int i = 0; i < plug_mgr->size();i++)
            {
                const Plugin * plug = (plug_mgr->operator[](i));
                if(!plug->size())
                    continue;
                con.print("%s\n", plug->getName().c_str());
            }
        }
        else if(first == "fpause")
        {
            World * w = core->getWorld();
            w->SetPauseState(true);
            con.print("The game was forced to pause!");
        }
        else if(first == "cls")
        {
            con.clear();
        }
        else if(first == "die")
        {
            _exit(666);
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
                    con.printerr("%s is not a recognized command.\n", first.c_str());
                    clueless_counter ++;
                }
                /*
                else if(res == CR_FAILURE)
                {
                    con.printerr("ERROR!\n");
                }
                */
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
    StackMutex = 0;
    core_cond = 0;
    // set up hotkey capture
    memset(hotkey_states,0,sizeof(hotkey_states));
    hotkey_set = false;
    HotkeyMutex = 0;
    HotkeyCond = 0;
    misc_data_mutex=0;
};

void Core::fatal (std::string output, bool deactivate)
{
    stringstream out;
    out << output ;
    if(deactivate)
        out << "DFHack will now deactivate.\n";
    if(con.isInited())
    {
        con.printerr("%s", out.str().c_str());
    }
    fprintf(stderr, "%s\n", out.str().c_str());
#ifndef LINUX_BUILD
    out << "Check file stderr.log for details\n";
    MessageBox(0,out.str().c_str(),"DFHack error!", MB_OK | MB_ICONERROR);
#endif
}

bool Core::Init()
{
    if(started)
        return true;
    if(errorstate)
        return false;

    // find out what we are...
    #ifdef LINUX_BUILD
        const char * path = "hack/Memory.xml";
    #else
        const char * path = "hack\\Memory.xml";
    #endif
    vif = new DFHack::VersionInfoFactory();
    cerr << "Identifying DF version.\n";
    try
    {
        vif->loadFile(path);
    }
    catch(Error::All & err)
    {
        std::stringstream out;
        out << "Error while reading Memory.xml:\n";
        out << err.what() << std::endl;
        delete vif;
        vif = nullptr;
        errorstate = true;
        fatal(out.str(), true);
        return false;
    }
    p = new DFHack::Process(vif);
    vinfo = p->getDescriptor();

    if(!vinfo || !p->isIdentified())
    {
        fatal ("Not a known DF version.\n", true);
        errorstate = true;
        delete p;
        p = NULL;
        return false;
    }
    cerr << "Version: " << vinfo->getVersion() << endl;

    cerr << "Initializing Console.\n";
    // init the console.
    Gui * g = getGui();
    bool is_text_mode = false;
    if(g->init && g->init->graphics.flags.is_set(GRAPHICS_TEXT))
    {
        is_text_mode = true;
    }
    if(con.init(is_text_mode))
        cerr << "Console is running.\n";
    else
        fatal ("Console has failed to initialize!\n", false);

    // dump offsets to a file
    std::ofstream dump("offsets.log");
    if(!dump.fail())
    {
        dump << vinfo->PrintOffsets();
        dump.close();
    }

    // create mutex for syncing with interactive tasks
    StackMutex = new mutex();
    AccessMutex = new mutex();
    misc_data_mutex=new mutex();
    core_cond = new Core::Cond();
    cerr << "Initializing Plugins.\n";
    // create plugin manager
    plug_mgr = new PluginManager(this);
    cerr << "Starting IO thread.\n";
    // create IO thread
    IODATA *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;
    thread * IO = new thread(fIOthread, (void *) temp);
    cerr << "Starting DF input capture thread.\n";
    // set up hotkey capture
    HotkeyMutex = new mutex();
    HotkeyCond = new condition_variable();
    thread * HK = new thread(fHKthread, (void *) temp);
    started = true;
    cerr << "DFHack is running.\n";
    return true;
}
/// sets the current hotkey command
bool Core::setHotkeyCmd( std::string cmd )
{
    // access command
    HotkeyMutex->lock();
    {
        hotkey_set = true;
        hotkey_cmd = cmd;
        HotkeyCond->notify_all();
    }
    HotkeyMutex->unlock();
    return true;
}
/// removes the hotkey command and gives it to the caller thread
std::string Core::getHotkeyCmd( void )
{
    string returner;
    HotkeyMutex->lock();
    while ( ! hotkey_set )
    {
        HotkeyCond->wait(*HotkeyMutex);
    }
    hotkey_set = false;
    returner = hotkey_cmd;
    hotkey_cmd.clear();
    HotkeyMutex->unlock();
    return returner;
}

void Core::RegisterData( void *p, std::string key )
{
    misc_data_mutex->lock();
    misc_data_map[key] = p;
    misc_data_mutex->unlock();
}

void *Core::GetData( std::string key )
{
    misc_data_mutex->lock();
    std::map<std::string,void*>::iterator it=misc_data_map.find(key);

    if ( it != misc_data_map.end() )
    {
        void *p=it->second;
        misc_data_mutex->unlock();
        return p;
    }
    else
    {
        misc_data_mutex->unlock();
        return 0;// or throw an error.
    }
}

void Core::Suspend()
{
    Core::Cond * nc = new Core::Cond();
    // put the condition on a stack
    StackMutex->lock();
        suspended_tools.push(nc);
    StackMutex->unlock();
    // wait until Core::Update() wakes up the tool
    AccessMutex->lock();
        nc->Lock(AccessMutex);
    AccessMutex->unlock();
}

void Core::Resume()
{
    AccessMutex->lock();
        core_cond->Unlock();
    AccessMutex->unlock();
}

// should always be from simulation thread!
int Core::Update()
{
    if(!started)
        Init();
    if(errorstate)
        return -1;

    // notify all the plugins that a game tick is finished
    plug_mgr->OnUpdate();
    // wake waiting tools
    // do not allow more tools to join in while we process stuff here
    StackMutex->lock();
    while (!suspended_tools.empty())
    {
        Core::Cond * nc = suspended_tools.top();
        suspended_tools.pop();
        AccessMutex->lock();
            // wake tool
            nc->Unlock();
            // wait for tool to wake us
            core_cond->Lock(AccessMutex);
        AccessMutex->unlock();
        // destroy condition
        delete nc;
    }
    StackMutex->unlock();
    return 0;
};

// FIXME: needs to terminate the IO threads and properly dismantle all the machinery involved.
int Core::Shutdown ( void )
{
    if(errorstate)
        return true;
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

// from ncurses
#define KEY_F0      0410        /* Function keys.  Space for 64 */
#define KEY_F(n)    (KEY_F0+(n))    /* Value of function key n */

bool Core::ncurses_wgetch(int in, int & out)
{
    if(!started)
    {
        out = in;
        return true;
    }
    if(in >= KEY_F(1) && in <= KEY_F(8))
    {
        int idx = in - KEY_F(1);
        // FIXME: copypasta, push into a method!
        Gui * g = getGui();
        if(g->hotkeys && g->df_interface && g->df_menu_state)
        {
            t_viewscreen * ws = g->GetCurrentScreen();
            // FIXME: put hardcoded values into memory.xml
            if(ws->getClassName() == "viewscreen_dwarfmodest" && *g->df_menu_state == 0x23)
            {
                out = in;
                return true;
            }
            else
            {
                t_hotkey & hotkey = (*g->hotkeys)[idx];
                setHotkeyCmd(hotkey.name);
                return false;
            }
        }
    }
    out = in;
    return true;
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
            if(g->hotkeys && g->df_interface && g->df_menu_state)
            {
                t_viewscreen * ws = g->GetCurrentScreen();
                // FIXME: put hardcoded values into memory.xml
                if(ws->getClassName() == "viewscreen_dwarfmodest" && *g->df_menu_state == 0x23)
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

////////////////
// ClassNamCheck
////////////////

// Since there is no Process.cpp, put ClassNamCheck stuff in Core.cpp

static std::set<std::string> known_class_names;
static std::map<std::string, void*> known_vptrs;

ClassNameCheck::ClassNameCheck(std::string _name) : name(_name), vptr(0)
{
    known_class_names.insert(name);
}

ClassNameCheck &ClassNameCheck::operator= (const ClassNameCheck &b)
{
    name = b.name; vptr = b.vptr; return *this;
}

bool ClassNameCheck::operator() (Process *p, void * ptr) const {
    if (vptr == 0 && p->readClassName(ptr) == name)
    {
        vptr = ptr;
        known_vptrs[name] = ptr;
    }
    return (vptr && vptr == ptr);
}

void ClassNameCheck::getKnownClassNames(std::vector<std::string> &names)
{
    std::set<std::string>::iterator it = known_class_names.begin();

    for (; it != known_class_names.end(); it++)
        names.push_back(*it);
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
MODULE_GETTER(Notes);
