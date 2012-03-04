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
#include "Core.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "Console.h"

#include "DataDefs.h"

using namespace DFHack;

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "tinythread.h"
using namespace tthread;

#ifdef LINUX_BUILD
    #include <dirent.h>
    #include <errno.h>
#else
    #include "wdirent.h"
#endif

#include <assert.h>

static int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
    files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

bool hasEnding (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() > ending.length())
    {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}
struct Plugin::RefLock
{
    RefLock()
    {
        refcount = 0;
        wakeup = new condition_variable();
        mut = new mutex();
    }
    ~RefLock()
    {
        delete wakeup;
        delete mut;
    }
    void lock()
    {
        mut->lock();
    }
    void unlock()
    {
        mut->unlock();
    }
    void lock_add()
    {
        mut->lock();
        refcount ++;
        mut->unlock();
    }
    void lock_sub()
    {
        mut->lock();
        refcount --;
        wakeup->notify_one();
        mut->unlock();
    }
    void wait()
    {
        while(refcount)
        {
            wakeup->wait(*mut);
        }
    }
    condition_variable * wakeup;
    mutex * mut;
    int refcount;
};

struct Plugin::RefAutolock
{
    RefLock * lock;
    RefAutolock(RefLock * lck):lock(lck){ lock->lock(); };
    ~RefAutolock(){ lock->unlock(); };
};

Plugin::Plugin(Core * core, const std::string & filepath, const std::string & _filename, PluginManager * pm)
{
    filename = filepath;
    parent = pm;
    name.reserve(_filename.size());
    for(size_t i = 0; i < _filename.size();i++)
    {
        char ch = _filename[i];
        if(ch == '.')
            break;
        name.append(1,ch);
    }
    Console & con = core->con;
    plugin_lib = 0;
    plugin_init = 0;
    plugin_shutdown = 0;
    plugin_status = 0;
    plugin_onupdate = 0;
    plugin_onstatechange = 0;
    state = PS_UNLOADED;
    access = new RefLock();
}

Plugin::~Plugin()
{
    if(state == PS_LOADED)
    {
        unload();
    }
    delete access;
}

bool Plugin::load()
{
    RefAutolock lock(access);
    if(state == PS_BROKEN)
    {
        return false;
    }
    else if(state == PS_LOADED)
    {
        return true;
    }
    Core & c = Core::getInstance();
    Console & con = c.con;
    DFLibrary * plug = OpenPlugin(filename.c_str());
    if(!plug)
    {
        con.printerr("Can't load plugin %s\n", filename.c_str());
        state = PS_BROKEN;
        return false;
    }
    const char ** plug_name =(const char ** ) LookupPlugin(plug, "name");
    const char ** plug_version =(const char ** ) LookupPlugin(plug, "version");
    if(!plug_name || !plug_version)
    {
        con.printerr("Plugin %s has no name or version.\n", filename.c_str());
        ClosePlugin(plug);
        state = PS_BROKEN;
        return false;
    }
    if(strcmp(DFHACK_VERSION, *plug_version) != 0)
    {
        con.printerr("Plugin %s was not built for this version of DFHack.\n"
                     "Plugin: %s, DFHack: %s\n", *plug_name, *plug_version, DFHACK_VERSION);
        ClosePlugin(plug);
        state = PS_BROKEN;
        return false;
    }
    plugin_init = (command_result (*)(Core *, std::vector <PluginCommand> &)) LookupPlugin(plug, "plugin_init");
    if(!plugin_init)
    {
        con.printerr("Plugin %s has no init function.\n", filename.c_str());
        ClosePlugin(plug);
        state = PS_BROKEN;
        return false;
    }
    plugin_status = (command_result (*)(Core *, std::string &)) LookupPlugin(plug, "plugin_status");
    plugin_onupdate = (command_result (*)(Core *)) LookupPlugin(plug, "plugin_onupdate");
    plugin_shutdown = (command_result (*)(Core *)) LookupPlugin(plug, "plugin_shutdown");
    plugin_onstatechange = (command_result (*)(Core *, state_change_event)) LookupPlugin(plug, "plugin_onstatechange");
    this->name = *plug_name;
    plugin_lib = plug;
    if(plugin_init(&c,commands) == CR_OK)
    {
        state = PS_LOADED;
        parent->registerCommands(this);
        return true;
    }
    else
    {
        con.printerr("Plugin %s has failed to initialize properly.\n", filename.c_str());
        ClosePlugin(plugin_lib);
        state = PS_BROKEN;
        return false;
    }
}

bool Plugin::unload()
{
    Core & c = Core::getInstance();
    Console & con = c.con;
    // get the mutex
    access->lock();
    // if we are actually loaded
    if(state == PS_LOADED)
    {
        // notify plugin about shutdown
        command_result cr = plugin_shutdown(&Core::getInstance());
        // wait for all calls to finish
        access->wait();
        // cleanup...
        parent->unregisterCommands(this);
        if(cr == CR_OK)
        {
            ClosePlugin(plugin_lib);
            state = PS_UNLOADED;
            access->unlock();
            return true;
        }
        else
        {
            con.printerr("Plugin %s has failed to shutdown!\n",name.c_str());
            state = PS_BROKEN;
            access->unlock();
            return false;
        }
    }
    else if(state == PS_UNLOADED)
    {
        access->unlock();
        return true;
    }
    access->unlock();
    return false;
}

bool Plugin::reload()
{
    if(state != PS_LOADED)
        return false;
    if(!unload())
        return false;
    if(!load())
        return false;
    return true;
}

command_result Plugin::invoke( std::string & command, std::vector <std::string> & parameters, bool interactive_)
{
    Core & c = Core::getInstance();
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED)
    {
        for (size_t i = 0; i < commands.size();i++)
        {
            PluginCommand &cmd = commands[i];
            if(cmd.name == command)
            {
                // running interactive things from some other source than the console would break it
                if(!interactive_ && cmd.interactive)
                    cr = CR_WOULD_BREAK;
                else if (cmd.guard)
                {
                    // Execute hotkey commands in a way where they can
                    // expect their guard conditions to be matched,
                    // so as to avoid duplicating checks.
                    // This means suspending the core beforehand.
                    CoreSuspender suspend(&c);
                    df::viewscreen *top = c.getTopViewscreen();

                    if (!cmd.guard(&c, top))
                    {
                        c.con.printerr("Could not invoke %s: unsuitable UI state.\n", command.c_str());
                        cr = CR_WRONG_USAGE;
                    }
                    else
                    {
                        cr = cmd.function(&c, parameters);
                    }
                }
                else
                {
                    cr = cmd.function(&c, parameters);
                }
                if (cr == CR_WRONG_USAGE && !cmd.usage.empty())
                    c.con << "Usage:\n" << cmd.usage << flush;
                break;
            }
        }
    }
    access->lock_sub();
    return cr;
}

bool Plugin::can_invoke_hotkey( std::string & command, df::viewscreen *top )
{
    Core & c = Core::getInstance();
    bool cr = false;
    access->lock_add();
    if(state == PS_LOADED)
    {
        for (size_t i = 0; i < commands.size();i++)
        {
            PluginCommand &cmd = commands[i];
            if(cmd.name == command)
            {
                if (cmd.interactive)
                    cr = false;
                else if (cmd.guard)
                    cr = cmd.guard(&c, top);
                else
                    cr = Gui::default_hotkey(&c, top);
                break;
            }
        }
    }
    access->lock_sub();
    return cr;
}

command_result Plugin::on_update()
{
    Core & c = Core::getInstance();
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED && plugin_onupdate)
    {
        cr = plugin_onupdate(&c);
    }
    access->lock_sub();
    return cr;
}

command_result Plugin::on_state_change(state_change_event event)
{
    Core & c = Core::getInstance();
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED && plugin_onstatechange)
    {
        cr = plugin_onstatechange(&c, event);
    }
    access->lock_sub();
    return cr;
}

Plugin::plugin_state Plugin::getState() const
{
    return state;
}

PluginManager::PluginManager(Core * core)
{
#ifdef LINUX_BUILD
    string path = core->p->getPath() + "/hack/plugins/";
    const string searchstr = ".plug.so";
#else
    string path = core->p->getPath() + "\\hack\\plugins\\";
    const string searchstr = ".plug.dll";
#endif
    cmdlist_mutex = new mutex();
    vector <string> filez;
    getdir(path, filez);
    for(size_t i = 0; i < filez.size();i++)
    {
        if(hasEnding(filez[i],searchstr))
        {
            Plugin * p = new Plugin(core, path + filez[i], filez[i], this);
            all_plugins.push_back(p);
            // make all plugins load by default (until a proper design emerges).
            p->load();
        }
    }
}

PluginManager::~PluginManager()
{
    for(size_t i = 0; i < all_plugins.size();i++)
    {
        delete all_plugins[i];
    }
    all_plugins.clear();
    delete cmdlist_mutex;
}

Plugin *PluginManager::getPluginByName (const std::string & name)
{
    for(size_t i = 0; i < all_plugins.size(); i++)
    {
        if(name == all_plugins[i]->name)
            return all_plugins[i];
    }
    return 0;
}

Plugin *PluginManager::getPluginByCommand(const std::string &command)
{
    tthread::lock_guard<tthread::mutex> lock(*cmdlist_mutex);
    map <string, Plugin *>::iterator iter = belongs.find(command);
    if (iter != belongs.end())
        return iter->second;
    else
        return NULL;    
}

// FIXME: handle name collisions...
command_result PluginManager::InvokeCommand( std::string & command, std::vector <std::string> & parameters, bool interactive)
{
    Plugin *plugin = getPluginByCommand(command);
    return plugin ? plugin->invoke(command, parameters, interactive) : CR_NOT_IMPLEMENTED;
}

bool PluginManager::CanInvokeHotkey(std::string &command, df::viewscreen *top)
{
    Plugin *plugin = getPluginByCommand(command);
    return plugin ? plugin->can_invoke_hotkey(command, top) : false;
}

void PluginManager::OnUpdate( void )
{
    for(size_t i = 0; i < all_plugins.size(); i++)
    {
        all_plugins[i]->on_update();
    }
}

void PluginManager::OnStateChange( state_change_event event )
{
    for(size_t i = 0; i < all_plugins.size(); i++)
    {
        all_plugins[i]->on_state_change(event);
    }
}

// FIXME: doesn't check name collisions!
void PluginManager::registerCommands( Plugin * p )
{
    cmdlist_mutex->lock();
    vector <PluginCommand> & cmds = p->commands;
    for(size_t i = 0; i < cmds.size();i++)
    {
        belongs[cmds[i].name] = p;
    }
    cmdlist_mutex->unlock();
}

// FIXME: doesn't check name collisions!
void PluginManager::unregisterCommands( Plugin * p )
{
    cmdlist_mutex->lock();
    vector <PluginCommand> & cmds = p->commands;
    for(size_t i = 0; i < cmds.size();i++)
    {
        belongs.erase(cmds[i].name);
    }
    cmdlist_mutex->unlock();
}