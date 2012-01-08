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

#include "Error.h"
#include "MemAccess.h"
#include "Core.h"
#include "DataDefs.h"
#include "Console.h"
#include "Module.h"
#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "PluginManager.h"
#include "ModuleFactory.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Graphic.h"
using namespace DFHack;

#include "SDL_events.h"

#include "df/ui.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/interface.h"
#include "df/viewscreen_dwarfmodest.h"

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <fstream>
#include "tinythread.h"

using namespace tthread;
using namespace df::enums;

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
    string *cur = NULL;

    for (unsigned i = 0; i < input.size(); i++) {
        char c = input[i];
        if (isspace(c)) {
            cur = NULL;
        } else {
            if (!cur) {
                output.push_back("");
                cur = &output.back();
            }

            if (c == '"') {
                for (i++; i < input.size(); i++) {
                    c = input[i];
                    if (c == '"')
                        break;
                    else if (c == '\\') {
                        if (++i < input.size())
                            cur->push_back(input[i]);
                    }
                    else
                        cur->push_back(c);
                }
            } else {
                cur->push_back(c);
            }
        }
    }
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
            vector <string> args;
            cheap_tokenise(stuff, args);
            if (args.empty()) {
                core->con.printerr("Empty hotkey command.\n");
                continue;
            }
            
            string first = args[0];
            args.erase(args.begin());
            command_result cr = plug_mgr->InvokeCommand(first, args, false);

            if(cr == CR_WOULD_BREAK)
            {
                core->con.printerr("It isn't possible to run an interactive command outside the console.\n");
            }
        }
    }
}

static void runInteractiveCommand(Core *core, PluginManager *plug_mgr, int &clueless_counter, const string &command)
{
    Console & con = core->con;
    
    if (!command.empty())
    {
        // cut the input into parts
        vector <string> parts;
        cheap_tokenise(command,parts);
        if(parts.size() == 0)
        {
            clueless_counter ++;
            return;
        }
        string first = parts[0];
        parts.erase(parts.begin());

        if (first[0] == '#') return;

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
                          "  keybinding            - Modify bindings of commands to keys\n"
                          "Plugin management (useful for developers):\n"
                          //"  belongs COMMAND       - Tell which plugin a command belongs to.\n"
                          "  plug [PLUGIN|v]       - List plugin state and description.\n"
                          "  load PLUGIN|all       - Load a plugin by name or load all possible plugins.\n"
                          "  unload PLUGIN|all     - Unload a plugin or all loaded plugins.\n"
                          "  reload PLUGIN|all     - Reload a plugin or all loaded plugins.\n"
                         );
            }
            else if (parts.size() == 1)
            {
                Plugin *plug = plug_mgr->getPluginByCommand(parts[0]);
                if (plug) {
                    for (int j = 0; j < plug->size();j++)
                    {
                        const PluginCommand & pcmd = (plug->operator[](j));
                        if (pcmd.name != parts[0])
                            continue;

                        if (pcmd.isHotkeyCommand())
                            con.color(Console::COLOR_CYAN);
                        con.print("%s: %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                        con.reset_color();
                        if (!pcmd.usage.empty())
                            con << "Usage:\n" << pcmd.usage << flush;
                        return;
                    }
                }
                con.printerr("Unknown command: %s\n", parts[0].c_str());
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
                    if(!plug)
                    {
                        con.printerr("No such plugin\n");
                    }
                    else
                    {
                        plug->load();
                    }
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
                    if(!plug)
                    {
                        con.printerr("No such plugin\n");
                    }
                    else
                    {
                        plug->reload();
                    }
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
                    if(!plug)
                    {
                        con.printerr("No such plugin\n");
                    }
                    else
                    {
                        plug->unload();
                    }
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
                    if (pcmd.isHotkeyCommand())
                        con.color(Console::COLOR_CYAN);
                    con.print("  %-22s - %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                    con.reset_color();
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
                        if (pcmd.isHotkeyCommand())
                            con.color(Console::COLOR_CYAN);
                        con.print("  %-22s- %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                        con.reset_color();
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
        else if(first == "keybinding")
        {
            if (parts.size() >= 3 && (parts[0] == "set" || parts[0] == "add"))
            {
                std::string keystr = parts[1];
                if (parts[0] == "set")
                    core->ClearKeyBindings(keystr);
                for (int i = parts.size()-1; i >= 2; i--) 
                {
                    if (!core->AddKeyBinding(keystr, parts[i])) {
                        con.printerr("Invalid key spec: %s\n", keystr.c_str());
                        break;
                    }
                }
            }
            else if (parts.size() >= 2 && parts[0] == "clear")
            {
                for (unsigned i = 1; i < parts.size(); i++)
                {
                    if (!core->ClearKeyBindings(parts[i])) {
                        con.printerr("Invalid key spec: %s\n", parts[i].c_str());
                        break;
                    }
                }
            }
            else if (parts.size() == 2 && parts[0] == "list")
            {
                std::vector<std::string> list = core->ListKeyBindings(parts[1]);
                if (list.empty())
                    con << "No bindings." << endl;
                for (unsigned i = 0; i < list.size(); i++)
                    con << "  " << list[i] << endl;
            }
            else
            {
                con << "Usage:" << endl
                    << "  keybinding list <key>" << endl
                    << "  keybinding clear <key> <key>..." << endl
                    << "  keybinding set <key> \"cmdline\" \"cmdline\"..." << endl
                    << "  keybinding add <key> \"cmdline\" \"cmdline\"..." << endl
                    << "Later adds, and earlier items within one command have priority." << endl;
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
    }
}

static void loadInitFile(Core *core, PluginManager *plug_mgr, string fname)
{
    ifstream init(fname);
    if (init.bad())
        return;

    int tmp = 0;
    string command;
    while (getline(init, command))
    {
        if (!command.empty())
            runInteractiveCommand(core, plug_mgr, tmp, command);
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

    loadInitFile(core, plug_mgr, "dfhack.init");

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

        runInteractiveCommand(core, plug_mgr, clueless_counter, command);

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
    hotkey_set = false;
    HotkeyMutex = 0;
    HotkeyCond = 0;
    misc_data_mutex=0;
    last_world_data_ptr = NULL;
    top_viewscreen = NULL;
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
        vif = NULL;
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
    
    // initialize data defs
    virtual_identity::Init(this);
    InitDataDefGlobals(this);

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

    // detect if the game was loaded or unloaded in the meantime
    void *new_wdata = NULL;
    if (df::global::world) {
        df::world_data *wdata = df::global::world->world_data;
        // when the game is unloaded, world_data isn't deleted, but its contents are
        if (wdata && !wdata->sites.empty())
            new_wdata = wdata;
    }
    
    if (new_wdata != last_world_data_ptr) {
        last_world_data_ptr = new_wdata;
        plug_mgr->OnStateChange(new_wdata ? SC_GAME_LOADED : SC_GAME_UNLOADED);
    }

    // detect if the viewscreen changed
    if (df::global::gview) 
    {
        df::viewscreen *screen = &df::global::gview->view;
        while (screen->child)
            screen = screen->child;
        if (screen != top_viewscreen) 
        {
            top_viewscreen = screen;
            plug_mgr->OnStateChange(SC_VIEWSCREEN_CHANGED);
        }
    }

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
        
        if(ke->state == SDL::BTN_PRESSED && !hotkey_states[ke->ksym.sym])
        {
            hotkey_states[ke->ksym.sym] = true;

            int mod = 0;
            if (ke->ksym.mod & SDL::KMOD_SHIFT) mod |= 1;
            if (ke->ksym.mod & SDL::KMOD_CTRL) mod |= 2;
            if (ke->ksym.mod & SDL::KMOD_ALT) mod |= 4;

            SelectHotkey(ke->ksym.sym, mod);
        }
        else if(ke->state == SDL::BTN_RELEASED)
        {
            hotkey_states[ke->ksym.sym] = false;
        }
    }
    return orig_return;
    // do stuff with the events...
}

bool Core::SelectHotkey(int sym, int modifiers)
{
    // Find the topmost viewscreen
    if (!df::global::gview || !df::global::ui)
        return false;

    df::viewscreen *screen = &df::global::gview->view;
    while (screen->child)
        screen = screen->child;

    std::string cmd;
    
    {
        tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);
    
        // Check the internal keybindings
        std::vector<KeyBinding> &bindings = key_bindings[sym];
        for (int i = bindings.size()-1; i >= 0; --i) {
            if (bindings[i].modifiers != modifiers)
                continue;
            if (!plug_mgr->CanInvokeHotkey(bindings[i].command[0], screen))
                continue;
            cmd = bindings[i].cmdline;
            break;
        }

        if (cmd.empty()) {
            // Check the hotkey keybindings
            int idx = sym - SDL::K_F1;
            if(idx >= 0 && idx < 8)
            {
                if (modifiers & 1)
                    idx += 8;

                if (strict_virtual_cast<df::viewscreen_dwarfmodest>(screen) &&
                    df::global::ui->main.mode != ui_sidebar_mode::Hotkeys)
                {
                    cmd = df::global::ui->main.hotkeys[idx].name;
                }
            }
        }
    }

    if (!cmd.empty()) {
        setHotkeyCmd(cmd);
        return true;
    }
    else
        return false;
}

static bool parseKeySpec(std::string keyspec, int *psym, int *pmod)
{
    *pmod = 0;

    // ugh, ugly
    for (;;) {
        if (keyspec.size() > 6 && keyspec.substr(0, 6) == "Shift-") {
            *pmod |= 1;
            keyspec = keyspec.substr(6);
        } else if (keyspec.size() > 5 && keyspec.substr(0, 5) == "Ctrl-") {
            *pmod |= 2;
            keyspec = keyspec.substr(5);
        } else if (keyspec.size() > 4 && keyspec.substr(0, 4) == "Alt-") {
            *pmod |= 4;
            keyspec = keyspec.substr(4);
        } else 
            break;
    }

    if (keyspec.size() == 1 && keyspec[0] >= 'A' && keyspec[0] <= 'Z') {
        *psym = SDL::K_a + (keyspec[0]-'A');
        return true;
    } else if (keyspec.size() == 2 && keyspec[0] == 'F' && keyspec[1] >= '1' && keyspec[1] <= '9') {
        *psym = SDL::K_F1 + (keyspec[1]-'1');
        return true;
    } else
        return false;    
}

bool Core::ClearKeyBindings(std::string keyspec)
{
    int sym, mod;
    if (!parseKeySpec(keyspec, &sym, &mod))
        return false;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (bindings[i].modifiers == mod)
            bindings.erase(bindings.begin()+i);
    }

    return true;
}

bool Core::AddKeyBinding(std::string keyspec, std::string cmdline)
{
    int sym;
    KeyBinding binding;
    if (!parseKeySpec(keyspec, &sym, &binding.modifiers))
        return false;

    cheap_tokenise(cmdline, binding.command);
    if (binding.command.empty())
        return false;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    // Don't add duplicates
    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (bindings[i].modifiers == binding.modifiers &&
            bindings[i].cmdline == cmdline)
            return true;
    }

    binding.cmdline = cmdline;
    bindings.push_back(binding);
    return true;
}

std::vector<std::string> Core::ListKeyBindings(std::string keyspec)
{
    int sym, mod;
    std::vector<std::string> rv;
    if (!parseKeySpec(keyspec, &sym, &mod))
        return rv;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (bindings[i].modifiers == mod)
            rv.push_back(bindings[i].cmdline);
    }

    return rv;
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

MODULE_GETTER(Units);
MODULE_GETTER(Engravings);
MODULE_GETTER(Maps);
MODULE_GETTER(Gui);
MODULE_GETTER(World);
MODULE_GETTER(Materials);
MODULE_GETTER(Items);
MODULE_GETTER(Translation);
MODULE_GETTER(Vegetation);
MODULE_GETTER(Constructions);
MODULE_GETTER(Notes);
MODULE_GETTER(Graphic);
