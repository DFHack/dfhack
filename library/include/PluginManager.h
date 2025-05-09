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

#pragma once

#include "Export.h"
#include "Hooks.h"
#include "ColorText.h"
#include "MiscUtils.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "Core.h"

typedef struct lua_State lua_State;

namespace df
{
    struct viewscreen;
}
namespace DFHack
{
    class Core;
    class PluginManager;
    class virtual_identity;
    class RPCService;
    class function_identity_base;
    namespace Lua {
        class Notification;
    }

    namespace Version {
        const char *dfhack_version();
        const char *git_description();
        int dfhack_abi_version();
    }

    // anon type, pretty much
    struct DFLibrary;

    // DFLibrary* that can be used to resolve global names
    extern DFLibrary* GLOBAL_NAMES;
    // Open a plugin library
    DFHACK_EXPORT DFLibrary * OpenPlugin (std::filesystem::path filename);
    // find a symbol inside plugin
    DFHACK_EXPORT void * LookupPlugin (DFLibrary * plugin ,const char * function);
    // Close a plugin library. returns true on success, false on failure
    DFHACK_EXPORT bool ClosePlugin (DFLibrary * plugin);

    struct DFHACK_EXPORT CommandReg {
        const char *name;
        int (*command)(lua_State*);
    };
    struct DFHACK_EXPORT FunctionReg {
        const char *name;
        function_identity_base *identity;
    };
    struct DFHACK_EXPORT EventReg {
        const char *name;
        Lua::Notification *event;
    };
    struct DFHACK_EXPORT PluginCommand
    {
        typedef command_result (*command_function)(color_ostream &out, std::vector <std::string> &);
        typedef bool (*command_hotkey_guard)(df::viewscreen *);

        /// create a command with a name, description, function pointer to its code
        /// and saying if it needs an interactive terminal
        /// Most commands shouldn't require an interactive terminal!
        /// Note that the description and usage fields are only used for
        /// out-of-tree plugins that do not have rendered help installed in
        /// the hack/docs directory. Help for all internal plugins comes from
        /// the rendered .rst files.
        PluginCommand(const char * _name,
                      const char * _description,
                      command_function function_,
                      bool interactive_ = false,
                      bool unlocked_ = false,
                      const char * usage_ = ""
                     )
            : name{_name}, description{_description}, function{function_},
              interactive{interactive_}, unlocked{unlocked_}, guard{NULL},
              usage{usage_}
        {
            fix_usage();
        }

        PluginCommand(const char * _name,
                      const char * _description,
                      command_function function_,
                      command_hotkey_guard guard_,
                      const char * usage_ = "")
            : name{_name}, description{_description}, function{function_},
              interactive{false}, unlocked{false}, guard{guard_},
              usage{usage_}
        {
            fix_usage();
        }

        void fix_usage()
        {
            if (usage.size() && usage[usage.size() - 1] != '\n')
                usage.push_back('\n');
        }

        bool isHotkeyCommand() const { return guard != NULL; }

        const std::string name;
        const std::string description;
        const command_function function;
        const bool interactive;
        const bool unlocked;
        const command_hotkey_guard guard;
        std::string usage;
    };
    class Plugin
    {
        struct RefLock;
        struct RefAutolock;
        struct RefAutoinc;
        friend class PluginManager;
        friend class RPCService;
        Plugin(DFHack::Core* core, const std::filesystem::path& filepath,
            const std::string &plug_name, PluginManager * pm);
        ~Plugin();
        command_result on_update(color_ostream &out);
        command_result on_state_change(color_ostream &out, state_change_event event);
        command_result save_world_data(color_ostream &out);
        command_result save_site_data(color_ostream &out);
        command_result load_world_data(color_ostream &out);
        command_result load_site_data(color_ostream &out);
        void detach_connection(RPCService *svc);
    public:
        enum plugin_state
        {
            PS_UNLOADED,
            PS_LOADED,
            PS_BROKEN,
            PS_LOADING,
            PS_UNLOADING,
            PS_DELETED
        };
        static const char *getStateDescription (plugin_state state);
        bool load(color_ostream &out);
        bool unload(color_ostream &out);
        bool reload(color_ostream &out);

        bool can_be_enabled() { return plugin_is_enabled != 0; }
        bool is_enabled() { return plugin_is_enabled && *plugin_is_enabled; }
        bool can_set_enabled() { return plugin_is_enabled != 0 && plugin_enable; }
        command_result set_enabled(color_ostream &out, bool enable);

        command_result invoke(color_ostream &out, const std::string & command, std::vector <std::string> & parameters);
        bool can_invoke_hotkey(const std::string & command, df::viewscreen *top );
        plugin_state getState () const;

        RPCService *rpc_connect(color_ostream &out);

        const PluginCommand& operator[] (std::size_t index) const
        {
            return commands[index];
        };
        std::size_t size() const
        {
            return commands.size();
        }
        const std::string & getName() const
        {
            return name;
        }
        plugin_state getState()
        {
            return state;
        }

        void open_lua(lua_State *state, int table);

    private:
        RefLock * access;
        std::vector <PluginCommand> commands;
        std::vector <RPCService*> services;
        std::filesystem::path path;
        std::string name;
        DFLibrary * plugin_lib;
        PluginManager * parent;
        plugin_state state;

        struct LuaCommand;
        std::map<std::string, LuaCommand*> lua_commands;
        static int lua_cmd_wrapper(lua_State *state);

        struct LuaFunction;
        std::map<std::string, LuaFunction*> lua_functions;
        static int lua_fun_wrapper(lua_State *state);
        void push_function(lua_State *state, LuaFunction *fn);

        static int lua_is_enabled(lua_State *state);
        static int lua_set_enabled(lua_State *state);

        struct LuaEvent;
        std::map<std::string, LuaEvent*> lua_events;

        void index_lua(DFLibrary *lib);
        void reset_lua();

        bool *plugin_is_enabled;
        std::vector<std::string>* plugin_globals;
        command_result (*plugin_init)(color_ostream &, std::vector <PluginCommand> &);
        command_result (*plugin_status)(color_ostream &, std::string &);
        command_result (*plugin_shutdown)(color_ostream &);
        command_result (*plugin_onupdate)(color_ostream &);
        command_result (*plugin_onstatechange)(color_ostream &, state_change_event);
        command_result (*plugin_enable)(color_ostream &, bool);
        RPCService* (*plugin_rpcconnect)(color_ostream &);
        command_result (*plugin_save_world_data)(color_ostream &);
        command_result (*plugin_save_site_data)(color_ostream &);
        command_result (*plugin_load_world_data)(color_ostream &);
        command_result (*plugin_load_site_data)(color_ostream &);
    };
    class DFHACK_EXPORT PluginManager
    {
    // PRIVATE METHODS
        friend class Core;
        friend class Plugin;
        PluginManager(Core * core);
        ~PluginManager();
        void init();
        void OnUpdate(color_ostream &out);
        void OnStateChange(color_ostream &out, state_change_event event);
        void registerCommands( Plugin * p );
        void unregisterCommands( Plugin * p );
        void doSaveData(color_ostream &out);
        void doLoadWorldData(color_ostream &out);
        void doLoadSiteData(color_ostream &out);
    // PUBLIC METHODS
    public:
        // list names of all plugins present in hack/plugins
        std::vector<std::string> listPlugins();
        // create Plugin instances for any plugins in hack/plugins that aren't present in all_plugins
        void refresh();

        bool load (const std::string &name);
        bool loadAll();
        bool unload (const std::string &name);
        bool unloadAll();
        bool reload (const std::string &name);
        bool reloadAll();

        Plugin *getPluginByName (const std::string &name) { return (*this)[name]; }
        Plugin *getPluginByCommand (const std::string &command);
        command_result InvokeCommand(color_ostream &out, const std::string & command, std::vector <std::string> & parameters);
        bool CanInvokeHotkey(const std::string &command, df::viewscreen *top);
        Plugin* operator[] (const std::string name);
        std::size_t size();

        std::map<std::string, Plugin*>::iterator begin();
        std::map<std::string, Plugin*>::iterator end();
    // DATA
    private:
        Core *core;
        bool addPlugin(std::string name);
        std::recursive_mutex * plugin_mutex;
        std::mutex * cmdlist_mutex;
        std::map <std::string, Plugin*> command_map;
        std::map <std::string, Plugin*> all_plugins;
        std::string plugin_path;
    };

    namespace Gui
    {
        // Predefined hotkey guards
        DFHACK_EXPORT bool default_hotkey(df::viewscreen *);
        DFHACK_EXPORT bool anywhere_hotkey(df::viewscreen *);
        DFHACK_EXPORT bool dwarfmode_hotkey(df::viewscreen *);
        DFHACK_EXPORT bool cursor_hotkey(df::viewscreen *);
    }
};

#define DFHACK_PLUGIN_AUX(m_plugin_name, is_dev) \
extern "C" { \
    DFhackDataExport const char * plugin_name = m_plugin_name;\
    DFhackDataExport const char * plugin_version = DFHack::Version::dfhack_version();\
    DFhackDataExport const char * plugin_git_description = DFHack::Version::git_description();\
    DFhackDataExport int plugin_abi_version = DFHack::Version::dfhack_abi_version();\
    DFhackDataExport DFHack::Plugin *plugin_self = NULL;\
    std::vector<std::string> plugin_globals_noptr;\
    DFhackDataExport std::vector<std::string>* plugin_globals = &plugin_globals_noptr; \
    DFhackDataExport bool plugin_dev = is_dev; \
}

/// You have to include DFHACK_PLUGIN("plugin_name") in every plugin you write - just once. Ideally at the top of the main file.
#ifdef DEV_PLUGIN
#define DFHACK_PLUGIN(m_plugin_name) DFHACK_PLUGIN_AUX(m_plugin_name, true)
#else
// Expose the plugin name to the DFHack core, as well as metadata like the DFHack version.
#define DFHACK_PLUGIN(m_plugin_name) DFHACK_PLUGIN_AUX(m_plugin_name, false)
#endif

#define DFHACK_PLUGIN_IS_ENABLED(varname) \
    DFhackDataExport bool plugin_is_enabled = false; \
    bool &varname = plugin_is_enabled;

#define REQUIRE_GLOBAL_NO_USE(global_name) \
    static int VARIABLE_IS_NOT_USED CONCAT_TOKENS(required_globals_, __LINE__) = \
        (plugin_globals->push_back(#global_name), 0);

#define REQUIRE_GLOBAL(global_name) \
    using df::global::global_name; \
    REQUIRE_GLOBAL_NO_USE(global_name)
