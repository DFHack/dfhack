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
#include "modules/Filesystem.h"
#include "Internal.h"
#include "Core.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "RemoteServer.h"
#include "Console.h"
#include "Types.h"
#include "VersionInfo.h"

#include "DataDefs.h"
#include "MiscUtils.h"
#include "DFHackVersion.h"

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

#define MUTEX_GUARD(lock) auto lock_##__LINE__ = make_mutex_guard(lock);
template <typename T>
tthread::lock_guard<T> make_mutex_guard (T *mutex)
{
    return tthread::lock_guard<T>(*mutex);
}

#if defined(_LINUX)
    static const string plugin_suffix = ".plug.so";
#elif defined(_DARWIN)
    static const string plugin_suffix = ".plug.dylib";
#else
    static const string plugin_suffix = ".plug.dll";
#endif

static string getPluginPath()
{
    return Core::getInstance().getHackPath() + "plugins/";
}

static string getPluginPath (std::string name)
{
    return getPluginPath() + name + plugin_suffix;
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

Plugin::Plugin(Core * core, const std::string & path,
    const std::string &name, PluginManager * pm)
    :name(name),
     path(path),
     parent(pm)
{
    plugin_lib = 0;
    plugin_init = 0;
    plugin_globals = 0;
    plugin_shutdown = 0;
    plugin_status = 0;
    plugin_onupdate = 0;
    plugin_onstatechange = 0;
    plugin_rpcconnect = 0;
    plugin_enable = 0;
    plugin_is_enabled = 0;
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

const char *Plugin::getStateDescription (plugin_state state)
{
    switch (state)
    {
#define map(s, desc) case s: return desc; break;
        map(PS_LOADED, "loaded")
        map(PS_UNLOADED, "unloaded")
        map(PS_BROKEN, "broken")
        map(PS_LOADING, "loading")
        map(PS_UNLOADING, "unloading")
        map(PS_DELETED, "deleted")
#undef map
        default:
            return "unknown";
            break;
    }
}

bool Plugin::load(color_ostream &con)
{
    {
        RefAutolock lock(access);
        if(state == PS_LOADED)
        {
            return true;
        }
        else if(state != PS_UNLOADED && state != PS_DELETED)
        {
            if (state == PS_BROKEN)
                con.printerr("Plugin %s is broken - cannot be loaded\n", name.c_str());
            return false;
        }
        state = PS_LOADING;
    }
    // enter suspend
    CoreSuspender suspend;
    // open the library, etc
    fprintf(stderr, "loading plugin %s\n", name.c_str());
    DFLibrary * plug = OpenPlugin(path.c_str());
    if(!plug)
    {
        RefAutolock lock(access);
        if (!Filesystem::isfile(path))
        {
            con.printerr("Plugin %s does not exist on disk\n", name.c_str());
            state = PS_DELETED;
            return false;
        }
        else {
            con.printerr("Can't load plugin %s\n", name.c_str());
            state = PS_UNLOADED;
            return false;
        }
    }
    #define plugin_abort_load ClosePlugin(plug); RefAutolock lock(access); state = PS_UNLOADED
    #define plugin_check_symbol(sym) \
        if (!LookupPlugin(plug, sym)) \
        { \
            con.printerr("Plugin %s: missing symbol: %s\n", name.c_str(), sym); \
            plugin_abort_load; \
            return false; \
        }
    #define plugin_check_symbols(sym1,sym2) \
        if (!LookupPlugin(plug, sym1) && !LookupPlugin(plug, sym2)) \
        { \
            con.printerr("Plugin %s: missing symbols: %s & %s\n", name.c_str(), sym1, sym2); \
            plugin_abort_load; \
            return false; \
        }

    plugin_check_symbols("plugin_name", "name")                 // allow r3 plugins
    plugin_check_symbols("plugin_version", "version")           // allow r3 plugins
    plugin_check_symbol("plugin_self")
    plugin_check_symbol("plugin_init")
    plugin_check_symbol("plugin_globals")
    const char ** plug_name =(const char ** ) LookupPlugin(plug, "plugin_name");
    if (!plug_name)                                            // allow r3 plugin naming
        plug_name = (const char ** )LookupPlugin(plug, "name");

    if (name != *plug_name)
    {
        con.printerr("Plugin %s: name mismatch, claims to be %s\n", name.c_str(), *plug_name);
        plugin_abort_load;
        return false;
    }
    const char ** plug_version =(const char ** ) LookupPlugin(plug, "plugin_version");
    if (!plug_version)                                         // allow r3 plugin version
        plug_version =(const char ** ) LookupPlugin(plug, "version");

    const char ** plug_git_desc_ptr = (const char**) LookupPlugin(plug, "plugin_git_description");
    Plugin **plug_self = (Plugin**)LookupPlugin(plug, "plugin_self");
    const char *dfhack_version = Version::dfhack_version();
    const char *dfhack_git_desc = Version::git_description();
    const char *plug_git_desc = plug_git_desc_ptr ? *plug_git_desc_ptr : "unknown";
    if (strcmp(dfhack_version, *plug_version) != 0)
    {
        con.printerr("Plugin %s was not built for this version of DFHack.\n"
                     "Plugin: %s, DFHack: %s\n", *plug_name, *plug_version, dfhack_version);
        plugin_abort_load;
        return false;
    }
    if (plug_git_desc_ptr)
    {
        if (strcmp(dfhack_git_desc, plug_git_desc) != 0)
            con.printerr("Warning: Plugin %s compiled for DFHack %s, running DFHack %s\n",
                *plug_name, plug_git_desc, dfhack_git_desc);
    }
    else
        con.printerr("Warning: Plugin %s missing git information\n", *plug_name);
    bool *plug_dev = (bool*)LookupPlugin(plug, "plugin_dev");
    if (plug_dev && *plug_dev && getenv("DFHACK_NO_DEV_PLUGINS"))
    {
        con.print("Skipping dev plugin: %s\n", *plug_name);
        plugin_abort_load;
        return false;
    }
    *plug_self = this;
    plugin_init = (command_result (*)(color_ostream &, std::vector <PluginCommand> &)) LookupPlugin(plug, "plugin_init");
    std::vector<std::string>* plugin_globals = *((std::vector<std::string>**) LookupPlugin(plug, "plugin_globals"));
    if (plugin_globals->size())
    {
        std::vector<std::string> missing_globals;
        for (auto it = plugin_globals->begin(); it != plugin_globals->end(); ++it)
        {
            if (!Core::getInstance().vinfo->getAddress(it->c_str()))
                missing_globals.push_back(*it);
        }
        if (missing_globals.size())
        {
            con.printerr("Plugin %s is missing required globals: %s\n",
                *plug_name, join_strings(", ", missing_globals).c_str());
            plugin_abort_load;
            return false;
        }
    }
    plugin_status = (command_result (*)(color_ostream &, std::string &)) LookupPlugin(plug, "plugin_status");
    plugin_onupdate = (command_result (*)(color_ostream &)) LookupPlugin(plug, "plugin_onupdate");
    plugin_shutdown = (command_result (*)(color_ostream &)) LookupPlugin(plug, "plugin_shutdown");
    plugin_onstatechange = (command_result (*)(color_ostream &, state_change_event)) LookupPlugin(plug, "plugin_onstatechange");
    plugin_rpcconnect = (RPCService* (*)(color_ostream &)) LookupPlugin(plug, "plugin_rpcconnect");
    plugin_enable = (command_result (*)(color_ostream &,bool)) LookupPlugin(plug, "plugin_enable");
    plugin_is_enabled = (bool*) LookupPlugin(plug, "plugin_is_enabled");
    plugin_eval_ruby = (command_result (*)(color_ostream &, const char*)) LookupPlugin(plug, "plugin_eval_ruby");
    index_lua(plug);
    plugin_lib = plug;
    commands.clear();
    if(plugin_init(con,commands) == CR_OK)
    {
        RefAutolock lock(access);
        state = PS_LOADED;
        parent->registerCommands(this);
        if ((plugin_onupdate || plugin_enable) && !plugin_is_enabled)
            con.printerr("Plugin %s has no enabled var!\n", name.c_str());
        fprintf(stderr, "loaded plugin %s; DFHack build %s\n", name.c_str(), plug_git_desc);
        fflush(stderr);
        return true;
    }
    else
    {
        con.printerr("Plugin %s has failed to initialize properly.\n", name.c_str());
        plugin_is_enabled = 0;
        plugin_onupdate = 0;
        reset_lua();
        plugin_abort_load;
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
            plugin_onstatechange(con, SC_BEGIN_UNLOAD) != CR_OK)
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
        plugin_is_enabled = 0;
        plugin_onupdate = 0;
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
    else if(state == PS_UNLOADED || state == PS_DELETED)
    {
        access->unlock();
        return true;
    }
    else if (state == PS_BROKEN)
        con.printerr("Plugin %s is broken - cannot be unloaded\n", name.c_str());
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
    // Check things that are implicitly protected by the suspend lock
    if (!plugin_onupdate)
        return CR_NOT_IMPLEMENTED;
    if (plugin_is_enabled && !*plugin_is_enabled)
        return CR_OK;
    // Grab mutex and call the thing
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

command_result Plugin::set_enabled(color_ostream &out, bool enable)
{
    command_result cr = CR_NOT_IMPLEMENTED;
    access->lock_add();
    if(state == PS_LOADED && plugin_is_enabled && plugin_enable)
    {
        cr = plugin_enable(out, enable);

        if (cr == CR_OK && enable != is_enabled())
            cr = CR_FAILURE;
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

int Plugin::lua_is_enabled(lua_State *state)
{
    auto obj = (Plugin*)lua_touserdata(state, lua_upvalueindex(1));

    RefAutoinc lock(obj->access);
    if (obj->state == PS_LOADED && obj->plugin_is_enabled)
        lua_pushboolean(state, obj->is_enabled());
    else
        lua_pushnil(state);

    return 1;
}

int Plugin::lua_set_enabled(lua_State *state)
{
    lua_settop(state, 1);
    bool val = lua_toboolean(state, 1);

    auto obj = (Plugin*)lua_touserdata(state, lua_upvalueindex(1));
    RefAutoinc lock(obj->access);

    color_ostream *out = Lua::GetOutput(state);

    if (obj->state == PS_LOADED && obj->plugin_enable)
        lua_pushboolean(state, obj->set_enabled(*out, val) == CR_OK);
    else
        luaL_error(state, "plugin %s unloaded, cannot enable or disable", obj->name.c_str());

    return 1;
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

    if (plugin_is_enabled)
    {
        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, lua_is_enabled, 1);
        lua_setfield(state, table, "isEnabled");
    }
    if (plugin_enable)
    {
        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, lua_set_enabled, 1);
        lua_setfield(state, table, "setEnabled");
    }

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

PluginManager::PluginManager(Core * core) : core(core)
{
    plugin_mutex = new recursive_mutex();
    cmdlist_mutex = new mutex();
    ruby = NULL;
}

PluginManager::~PluginManager()
{
    for (auto it = begin(); it != end(); ++it)
    {
        Plugin *p = it->second;
        delete p;
    }
    all_plugins.clear();
    delete plugin_mutex;
    delete cmdlist_mutex;
}

void PluginManager::init()
{
    loadAll();
}

bool PluginManager::addPlugin(string name)
{
    if (all_plugins.find(name) != all_plugins.end())
    {
        Core::printerr("Plugin already exists: %s\n", name.c_str());
        return false;
    }
    string path = getPluginPath(name);
    if (!Filesystem::isfile(path))
    {
        Core::printerr("Plugin does not exist: %s\n", name.c_str());
        return false;
    }
    Plugin * p = new Plugin(core, path, name, this);
    all_plugins[name] = p;
    return true;
}

vector<string> PluginManager::listPlugins()
{
    vector<string> results;
    vector<string> files;
    Filesystem::listdir(getPluginPath(), files);
    for (auto file = files.begin(); file != files.end(); ++file)
    {
        if (hasEnding(*file, plugin_suffix))
        {
            string shortname = file->substr(0, file->find(plugin_suffix));
            results.push_back(shortname);
        }
    }
    return results;
}

void PluginManager::refresh()
{
    MUTEX_GUARD(plugin_mutex);
    auto files = listPlugins();
    for (auto f = files.begin(); f != files.end(); ++f)
    {
        if (!(*this)[*f])
            addPlugin(*f);
    }
}

bool PluginManager::load (const string &name)
{
    MUTEX_GUARD(plugin_mutex);
    if (!(*this)[name] && !addPlugin(name))
        return false;
    Plugin *p = (*this)[name];
    if (!p)
    {
        Core::printerr("Plugin failed to register: %s\n", name.c_str());
        return false;
    }
    return p->load(core->getConsole());
}

bool PluginManager::loadAll()
{
    MUTEX_GUARD(plugin_mutex);
    auto files = listPlugins();
    bool ok = true;
    // load all plugins in hack/plugins
    for (auto f = files.begin(); f != files.end(); ++f)
    {
        if (!load(*f))
            ok = false;
    }
    return ok;
}

bool PluginManager::unload (const string &name)
{
    MUTEX_GUARD(plugin_mutex);
    if (!(*this)[name])
    {
        Core::printerr("Plugin does not exist: %s\n", name.c_str());
        return false;
    }
    return (*this)[name]->unload(core->getConsole());
}

bool PluginManager::unloadAll()
{
    MUTEX_GUARD(plugin_mutex);
    bool ok = true;
    // only try to unload plugins that are in all_plugins
    for (auto it = begin(); it != end(); ++it)
    {
        if (!unload(it->first))
            ok = false;
    }
    return ok;
}

bool PluginManager::reload (const string &name)
{
    // equivalent to "unload(name); load(name);" if plugin is recognized,
    // "load(name);" otherwise
    MUTEX_GUARD(plugin_mutex);
    if (!(*this)[name])
        return load(name);
    if (!unload(name))
        return false;
    return load(name);
}

bool PluginManager::reloadAll()
{
    MUTEX_GUARD(plugin_mutex);
    bool ok = true;
    if (!unloadAll())
        ok = false;
    if (!loadAll())
        ok = false;
    return ok;
}

Plugin *PluginManager::getPluginByCommand(const std::string &command)
{
    tthread::lock_guard<tthread::mutex> lock(*cmdlist_mutex);
    map <string, Plugin *>::iterator iter = command_map.find(command);
    if (iter != command_map.end())
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
    for (auto it = begin(); it != end(); ++it)
        it->second->on_update(out);
}

void PluginManager::OnStateChange(color_ostream &out, state_change_event event)
{
    for (auto it = begin(); it != end(); ++it)
        it->second->on_state_change(out, event);
}

void PluginManager::registerCommands( Plugin * p )
{
    cmdlist_mutex->lock();
    vector <PluginCommand> & cmds = p->commands;
    for (size_t i = 0; i < cmds.size();i++)
    {
        std::string name = cmds[i].name;
        if (command_map.find(name) != command_map.end())
        {
            core->printerr("Plugin %s re-implements command \"%s\" (from plugin %s)\n",
                p->getName().c_str(), name.c_str(), command_map[name]->getName().c_str());
            continue;
        }
        command_map[name] = p;
    }
    if (p->plugin_eval_ruby)
        ruby = p;
    cmdlist_mutex->unlock();
}

void PluginManager::unregisterCommands( Plugin * p )
{
    cmdlist_mutex->lock();
    vector <PluginCommand> & cmds = p->commands;
    for(size_t i = 0; i < cmds.size();i++)
    {
        command_map.erase(cmds[i].name);
    }
    if (p->plugin_eval_ruby)
        ruby = NULL;
    cmdlist_mutex->unlock();
}

Plugin *PluginManager::operator[] (std::string name)
{
    MUTEX_GUARD(plugin_mutex);
    if (all_plugins.find(name) == all_plugins.end())
    {
        if (Filesystem::isfile(getPluginPath(name)))
            addPlugin(name);
    }
    return (all_plugins.find(name) != all_plugins.end()) ? all_plugins[name] : NULL;
}

size_t PluginManager::size()
{
    return all_plugins.size();
}

std::map<std::string, Plugin*>::iterator PluginManager::begin()
{
    return all_plugins.begin();
}

std::map<std::string, Plugin*>::iterator PluginManager::end()
{
    return all_plugins.end();
}
