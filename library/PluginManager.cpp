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
#include "dfhack/Core.h"
#include "dfhack/Process.h"
#include "dfhack/PluginManager.h"
#include "dfhack/Console.h"
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
Plugin::Plugin(Core * core, const std::string & filepath, const std::string & _filename, PluginManager * pm)
{
    filename = filepath;
    parent = pm;
    name.reserve(_filename.size());
    for(int i = 0; i < _filename.size();i++)
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
    access->lock();
    if(state == PS_BROKEN)
    {
        access->unlock();
        return false;
    }
    else if(state == PS_LOADED)
    {
        access->unlock();
        return true;
    }
    Core & c = Core::getInstance();
    Console & con = c.con;
    DFLibrary * plug = OpenPlugin(filename.c_str());
    if(!plug)
    {
        con.printerr("Can't load plugin %s\n", filename.c_str());
        state = PS_BROKEN;
        access->unlock();
        return false;
    }
    const char * (*_PlugName)() =(const char * (*)()) LookupPlugin(plug, "plugin_name");
    if(!_PlugName)
    {
        con.printerr("Plugin %s has no name.\n", filename.c_str());
        ClosePlugin(plug);
        state = PS_BROKEN;
        access->unlock();
        return false;
    }
    plugin_init = (command_result (*)(Core *, std::vector <PluginCommand> &)) LookupPlugin(plug, "plugin_init");
    if(!plugin_init)
    {
        con.printerr("Plugin %s has no init function.\n", filename.c_str());
        ClosePlugin(plug);
        state = PS_BROKEN;
        access->unlock();
        return false;
    }
    plugin_status = (command_result (*)(Core *, std::string &)) LookupPlugin(plug, "plugin_status");
    plugin_onupdate = (command_result (*)(Core *)) LookupPlugin(plug, "plugin_onupdate");
    plugin_shutdown = (command_result (*)(Core *)) LookupPlugin(plug, "plugin_shutdown");
    //name = _PlugName();
    plugin_lib = plug;
    if(plugin_init(&c,commands) == CR_OK)
    {
        state = PS_LOADED;
        parent->registerCommands(this);
        access->unlock();
        return true;
    }
    else
    {
        con.printerr("Plugin %s has failed to initialize properly.\n", filename.c_str());
        ClosePlugin(plugin_lib);
        state = PS_BROKEN;
        access->unlock();
        return false;
    }
    // not reachable
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
            return false;
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
        for (int i = 0; i < commands.size();i++)
        {
            if(commands[i].name == command)
            {
                // running interactive things from some other source than the console would break it
                if(!interactive_ && commands[i].interactive)
                    cr = CR_WOULD_BREAK;
                else
                    cr = commands[i].function(&c, parameters);
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
    for(int i = 0; i < filez.size();i++)
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
    for(int i = 0; i < all_plugins.size();i++)
    {
        delete all_plugins[i];
    }
    all_plugins.clear();
    delete cmdlist_mutex;
}

Plugin *PluginManager::getPluginByName (const std::string & name)
{
    for(int i = 0; i < all_plugins.size(); i++)
    {
        if(name == all_plugins[i]->name)
            return all_plugins[i];
    }
    return 0;
}

// FIXME: handle name collisions...
command_result PluginManager::InvokeCommand( std::string & command, std::vector <std::string> & parameters, bool interactive)
{
    command_result cr = CR_NOT_IMPLEMENTED;
    Core * c = &Core::getInstance();
    cmdlist_mutex->lock();
    map <string, Plugin *>::iterator iter = belongs.find(command);
    if(iter != belongs.end())
    {
        cr = iter->second->invoke(command, parameters, interactive);
    }
    cmdlist_mutex->unlock();
    return cr;
}

void PluginManager::OnUpdate( void )
{
    for(int i = 0; i < all_plugins.size(); i++)
    {
        all_plugins[i]->on_update();
    }
}
// FIXME: doesn't check name collisions!
void PluginManager::registerCommands( Plugin * p )
{
    cmdlist_mutex->lock();
    vector <PluginCommand> & cmds = p->commands;
    for(int i = 0; i < cmds.size();i++)
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
    for(int i = 0; i < cmds.size();i++)
    {
        belongs.erase(cmds[i].name);
    }
    cmdlist_mutex->unlock();
}