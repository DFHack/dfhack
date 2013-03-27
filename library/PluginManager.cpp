/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

#include "modules/EventManager.h"
#include "Internal.h"
#include "Core.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "RemoteServer.h"
#include "Console.h"
#include "Types.h"

#include "DataDefs.h"
#include "MiscUtils.h"

#include "LuaWrapper.h"
#include "LuaTools.h"

using namespace DFHack;

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "tinythread.h"
using namespace tthread;

#include <assert.h>

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
        if (--refcount == 0)
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

struct Plugin::RefAutoinc
{
    RefLock * lock;
    RefAutoinc(RefLock * lck):lock(lck){ lock->lock_add(); };
    ~RefAutoinc(){ lock->lock_sub(); };
};

struct Plugin::LuaCommand {
    Plugin *owner;
    std::string name;
    int (*command)(lua_State *state);

    LuaCommand(Plugin *owner, std::string name)
      : owner(owner), name(name), command(NULL) {}
};

struct Plugin::LuaFunction {
    Plugin *owner;
    std::string name;
    function_identity_base *identity;
    bool silent;

    LuaFunction(Plugin *owner, std::string name)
      : owner(owner), name(name), identity(NULL), silent(false) {}
};

struct Plugin::LuaEvent : public Lua::Event::Owner {
    LuaFunction handler;
    Lua::Notification *event;
    bool active;
    int count;

    LuaEvent(Plugin *owner, std::string name)
      : handler(owner,name), event(NULL), active(false), count(0)
    {
        handler.silent = true;
    }

    void on_count_changed(int new_cnt, int delta) {
        RefAutoinc lock(handler.owner->access);
        count = new_cnt;
        if (event)
            event->on_count_changed(new_cnt, delta);
    }
    void on_invoked(lua_State *state, int nargs, bool from_c) {
        RefAutoinc lock(handler.owner->access);
        if (event)
            event->on_invoked(state, nargs, from_c);
    }
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
    plugin_lib = 0;
    plugin_init = 0;
    plugin_shutdown = 0;
    plugin_status = 0;
    plugin_onupdate = 0;
    plugin_onstatechange = 0;
    plugin_rpcconnect = 0;
    state = PS_UNLOADED;
    access = new RefLock();
}

Plugin::~Plugin()
{
    if(state == PS_LOADED)
    {
        unload(Core::getInstance().getConsole());
    }
    delete access;
}

bool Plugin::load(color_ostream &con)
{
    {
        RefAutolock lock(access);
        if(state == PS_LOADED)
        {
            return true;
        }
        else if(state != PS_UNLOADED)
        {
            return false;
        }
        state = PS_LOADING;
    }
    // enter suspend
    CoreSuspender suspend;
    // open the library, etc
    DFLibrary * plug = OpenPlugin(filename.c_str());
    if(!plug)
    {
        con.printerr("Can't load plugin %s\n", filename.c_str());
        RefAutolock lock(access);
        state = PS_BROKEN;
        return false;
    }
    const char ** plug_name =(const char ** ) LookupPlugin(plug, "name");
    const char ** plug_version =(const char ** ) LookupPlugin(plug, "version");
    Plugin **plug_self = (Plugin**)LookupPlugin(plug, "plugin_self");
    if(!plug_name || !plug_version || !plug_self)
    {
        con.printerr("Plugin %s has no name, version or self pointer.\n", filename.c_str());
        ClosePlugin(plug);
        RefAutolock lock(access);
        state = PS_BROKEN;
        return false;
    }
    if(strcmp(DFHACK_VERSION, *plug_version) != 0)
    {
        con.printerr("Plugin %s was not built for this version of DFHack.\n"
                     "Plugin: %s, DFHack: %s\n", *plug_name, *plug_version, DFHACK_VERSION);
        ClosePlugin(plug);
        RefAutolock lock(access);
        state = PS_BROKEN;
        return false;
    }
    *plug_self = this;
    RefAutolock lock(access);
    plugin_init = (command_result (*)(color_ostream &, std::vector <PluginCommand> &)) LookupPlugin(plug, "plugin_init");
    if(!plugin_init)
    {
        con.printerr("Plugin %s has no init function.\n", filename.c_str());
        ClosePlugin(plug);
        state = PS_BROKEN;
        return false;
    }
    plugin_status = (command_result (*)(color_ostream &, std::string &)) LookupPlugin(plug, "plugin_status");
    plugin_onupdate = (command_result (*)(color_ostream &)) LookupPlugin(plug, "plugin_onupdate");
    plugin_shutdown = (command_result (*)(color_ostream &)) LookupPlugin(plug, "plugin_shutdown");
    plugin_onstatechange = (command_result (*)(color_ostream &, state_change_event)) LookupPlugin(plug, "plugin_onstatechange");
    plugin_rpcconnect = (RPCService* (*)(color_ostream &)) LookupPlugin(plug, "plugin_rpcconnect");
    plugin_eval_ruby = (command_result (*)(color_ostream &, const char*)) LookupPlugin(plug, "plugin_eval_ruby");
    index_lua(plug);
    this->name = *plug_name;
    plugin_lib = plug;
    commands.clear();
    if(plugin_init(con,commands) == CR_OK)
    {
        state = PS_LOADED;
        parent->registerCommands(this);
        return true;
    }
    else
    {
        con.printerr("Plugin %s has failed to initialize properly.\n", filename.c_str());
        reset_lua();
        ClosePlugin(plugin_lib);
        state = PS_BROKEN;
        return false;
    }
}

bool Plugin::unload(color_ostream &con)
{
    // get the mutex
    access->lock();
    // if we are actually loaded
    if(state == PS_LOADED)
    {
        EventManager::unregisterAll(this);
        // notify the plugin about an attempt to shutdown
        if (plugin_onstatechange &&
            plugin_onstatechange(con, SC_BEGIN_UNLOAD) == CR_NOT_FOUND)
        {
            con.printerr("Plugin %s has refused to be unloaded.\n", name.c_str());
            access->unlock();
            return false;
        }
        // wait for all calls to finish
        access->wait();
        state = PS_UNLOADING;
        access->unlock();
        // enter suspend
        CoreSuspender suspend;
        access->lock();
        // notify plugin about shutdown, if it has a shutdown function
        command_result cr = CR_OK;
        if(plugin_shutdown)
            cr = plugin_shutdown(con);
        // cleanup...
        reset_lua();
        parent->unregisterCommands(this);
        commands.clear();
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

bool Plugin::reload(color_ostream &out)
{
    if(state != PS_LOADED)
        return false;
    if(!unload(out))
        return false;
    if(!load(out))
        return false;
    return true;
}

command_result Plugin::invoke(color_ostream &out, const std::string & command, std::vector <std::string> & parameters)
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
                if(!out.is_console() && cmd.interactive)
                    cr = CR_NEEDS_CONSOLE;
                else if (cmd.guard)
                {
                    // Execute hotkey commands in a way where they can
                    // expect their guard conditions to be matched,
                    // so as to avoid duplicating checks.
                    // This means suspending the core beforehand.
                    CoreSuspender suspend(&c);
                    df::viewscreen *top = c.getTopViewscreen();

                    if (!cmd.guard(top))
                    {
                        out.printerr("Could not invoke %s: unsuitable UI state.\n", command.c_str());
                        cr = CR_WRONG_USAGE;
                    }
                    else
                    {
                        cr = cmd.function(out, parameters);
                    }
                }
                else
                {
                    cr = cmd.function(out, parameters);
                }
                if (cr == CR_WRONG_USAGE && !cmd.usage.empty())
                    out << "Usage:\n" << cmd.usage << flush;
                break;
            }
        }
    }
    access->lock_sub();
    return cr;
}

bool Plugin::can_invoke_hotkey(const std::string & command, df::viewscreen *top )
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
                    cr = cmd.guard(top);
                else
                    cr = Gui::default_hotkey(top);
                break;
            }
        }
    }
    access->lock_sub();
    return cr;
}

command_result Plugin::on_update(color_ostream &out)
{
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED && plugin_onupdate)
    {
        cr = plugin_onupdate(out);
        Lua::Core::Reset(out, "plugin_onupdate");
    }
    access->lock_sub();
    return cr;
}

command_result Plugin::on_state_change(color_ostream &out, state_change_event event)
{
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED && plugin_onstatechange)
    {
        cr = plugin_onstatechange(out, event);
        Lua::Core::Reset(out, "plugin_onstatechange");
    }
    access->lock_sub();
    return cr;
}

RPCService *Plugin::rpc_connect(color_ostream &out)
{
    RPCService *rv = NULL;

    access->lock_add();

    if(state == PS_LOADED && plugin_rpcconnect)
    {
        rv = plugin_rpcconnect(out);
    }

    if (rv)
    {
        // Retain the access reference
        assert(!rv->holder);
        services.push_back(rv);
        rv->holder = this;
        return rv;
    }
    else
    {
        access->lock_sub();
        return NULL;
    }
}

void Plugin::detach_connection(RPCService *svc)
{
    int idx = linear_index(services, svc);

    assert(svc->holder == this && idx >= 0);

    vector_erase_at(services, idx);
    access->lock_sub();
}

Plugin::plugin_state Plugin::getState() const
{
    return state;
}

void Plugin::index_lua(DFLibrary *lib)
{
    if (auto cmdlist = (CommandReg*)LookupPlugin(lib, "plugin_lua_commands"))
    {
        for (; cmdlist->name; ++cmdlist)
        {
            auto &cmd = lua_commands[cmdlist->name];
            if (!cmd) cmd = new LuaCommand(this,cmdlist->name);
            cmd->command = cmdlist->command;
        }
    }
    if (auto funlist = (FunctionReg*)LookupPlugin(lib, "plugin_lua_functions"))
    {
        for (; funlist->name; ++funlist)
        {
            auto &cmd = lua_functions[funlist->name];
            if (!cmd) cmd = new LuaFunction(this,funlist->name);
            cmd->identity = funlist->identity;
        }
    }
    if (auto evlist = (EventReg*)LookupPlugin(lib, "plugin_lua_events"))
    {
        for (; evlist->name; ++evlist)
        {
            auto &cmd = lua_events[evlist->name];
            if (!cmd) cmd = new LuaEvent(this,evlist->name);
            cmd->handler.identity = evlist->event->get_handler();
            cmd->event = evlist->event;
            if (cmd->active)
            {
                cmd->event->bind(Lua::Core::State, cmd);
                if (cmd->count > 0)
                    cmd->event->on_count_changed(cmd->count, 0);
            }
        }
    }
}

void Plugin::reset_lua()
{
    for (auto it = lua_commands.begin(); it != lua_commands.end(); ++it)
        it->second->command = NULL;
    for (auto it = lua_functions.begin(); it != lua_functions.end(); ++it)
        it->second->identity = NULL;
    for (auto it = lua_events.begin(); it != lua_events.end(); ++it)
    {
        it->second->handler.identity = NULL;
        it->second->event = NULL;
    }
}

int Plugin::lua_cmd_wrapper(lua_State *state)
{
    auto cmd = (LuaCommand*)lua_touserdata(state, lua_upvalueindex(1));

    RefAutoinc lock(cmd->owner->access);

    if (!cmd->command)
        luaL_error(state, "plugin command %s() has been unloaded",
                   (cmd->owner->name+"."+cmd->name).c_str());

    return Lua::CallWithCatch(state, cmd->command, cmd->name.c_str());
}

int Plugin::lua_fun_wrapper(lua_State *state)
{
    auto cmd = (LuaFunction*)lua_touserdata(state, UPVAL_CONTAINER_ID);

    RefAutoinc lock(cmd->owner->access);

    if (!cmd->identity)
    {
        if (cmd->silent)
            return 0;

        luaL_error(state, "plugin function %s() has been unloaded",
                   (cmd->owner->name+"."+cmd->name).c_str());
    }

    return LuaWrapper::method_wrapper_core(state, cmd->identity);
}

void Plugin::open_lua(lua_State *state, int table)
{
    table = lua_absindex(state, table);

    RefAutolock lock(access);

    for (auto it = lua_commands.begin(); it != lua_commands.end(); ++it)
    {
        lua_pushlightuserdata(state, it->second);
        lua_pushcclosure(state, lua_cmd_wrapper, 1);
        lua_setfield(state, table, it->first.c_str());
    }

    for (auto it = lua_functions.begin(); it != lua_functions.end(); ++it)
    {
        push_function(state, it->second);
        lua_setfield(state, table, it->first.c_str());
    }

    if (Lua::IsCoreContext(state))
    {
        for (auto it = lua_events.begin(); it != lua_events.end(); ++it)
        {
            Lua::Event::Make(state, it->second, it->second);

            push_function(state, &it->second->handler);
            Lua::Event::SetPrivateCallback(state, -2);

            it->second->active = true;
            if (it->second->event)
                it->second->event->bind(Lua::Core::State, it->second);

            lua_setfield(state, table, it->first.c_str());
        }
    }
}

void Plugin::push_function(lua_State *state, LuaFunction *fn)
{
    lua_rawgetp(state, LUA_REGISTRYINDEX, &LuaWrapper::DFHACK_TYPETABLE_TOKEN);
    lua_pushlightuserdata(state, NULL);
    lua_pushfstring(state, "%s.%s()", name.c_str(), fn->name.c_str());
    lua_pushlightuserdata(state, fn);
    lua_pushcclosure(state, lua_fun_wrapper, 4);
}

PluginManager::PluginManager(Core * core)
{
    cmdlist_mutex = new mutex();
    eval_ruby = NULL;
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

void PluginManager::init(Core * core)
{
#ifdef LINUX_BUILD
    string path = core->getHackPath() + "plugins/";
    const string searchstr = ".plug.so";
#else
    string path = core->getHackPath() + "plugins\\";
    const string searchstr = ".plug.dll";
#endif
    vector <string> filez;
    getdir(path, filez);
    for(size_t i = 0; i < filez.size();i++)
    {
        if(hasEnding(filez[i],searchstr))
        {
            Plugin * p = new Plugin(core, path + filez[i], filez[i], this);
            all_plugins.push_back(p);
            // make all plugins load by default (until a proper design emerges).
            p->load(core->getConsole());
        }
    }
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
command_result PluginManager::InvokeCommand(color_ostream &out, const std::string & command, std::vector <std::string> & parameters)
{
    Plugin *plugin = getPluginByCommand(command);
    return plugin ? plugin->invoke(out, command, parameters) : CR_NOT_IMPLEMENTED;
}

bool PluginManager::CanInvokeHotkey(const std::string &command, df::viewscreen *top)
{
    Plugin *plugin = getPluginByCommand(command);
    return plugin ? plugin->can_invoke_hotkey(command, top) : true;
}

void PluginManager::OnUpdate(color_ostream &out)
{
    for(size_t i = 0; i < all_plugins.size(); i++)
    {
        all_plugins[i]->on_update(out);
    }
}

void PluginManager::OnStateChange(color_ostream &out, state_change_event event)
{
    for(size_t i = 0; i < all_plugins.size(); i++)
    {
        all_plugins[i]->on_state_change(out, event);
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
    if (p->plugin_eval_ruby)
        eval_ruby = p->plugin_eval_ruby;
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
    if (p->plugin_eval_ruby)
        eval_ruby = NULL;
    cmdlist_mutex->unlock();
}
