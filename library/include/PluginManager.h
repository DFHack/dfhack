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

#pragma once

#include "Export.h"
#include "Hooks.h"
#include <map>
#include <string>
#include <vector>
struct DFLibrary;
namespace tthread
{
    class mutex;
    class condition_variable;
}
namespace df
{
    struct viewscreen;
}
namespace DFHack
{
    class Core;
    class PluginManager;
    struct virtual_identity;

    enum command_result
    {
        CR_WOULD_BREAK = -2,
        CR_NOT_IMPLEMENTED = -1,
        CR_FAILURE = 0,
        CR_OK = 1
    };
    enum state_change_event
    {
        SC_GAME_LOADED,
        SC_GAME_UNLOADED,
        SC_VIEWSCREEN_CHANGED
    };
    struct DFHACK_EXPORT PluginCommand
    {
        typedef command_result (*command_function)(Core *, std::vector <std::string> &);
        typedef bool (*command_hotkey_guard)(Core *, df::viewscreen *);
        
        /// create a command with a name, description, function pointer to its code
        /// and saying if it needs an interactive terminal
        /// Most commands shouldn't require an interactive terminal!
        PluginCommand(const char * _name,
                      const char * _description,
                      command_function function_,
                      bool interactive_ = false
                     )
            : name(_name), description(_description),
              function(function_), interactive(interactive_),
              guard(NULL), viewscreen_type(NULL)
        {
        }

        PluginCommand(const char * _name,
                      const char * _description,
                      command_function function_,
                      command_hotkey_guard guard_,
                      virtual_identity *viewscreen_type_ = NULL)
            : name(_name), description(_description),
              function(function_), interactive(false),
              guard(guard_), viewscreen_type(viewscreen_type_)
        {
        }

        std::string name;
        std::string description;
        command_function function;
        bool interactive;
        command_hotkey_guard guard;
        virtual_identity *viewscreen_type;
    };
    class Plugin
    {
        struct RefLock;
        enum plugin_state
        {
            PS_UNLOADED,
            PS_LOADED,
            PS_BROKEN
        };
        friend class PluginManager;
        Plugin(DFHack::Core* core, const std::string& filepath, const std::string& filename, PluginManager * pm);
        ~Plugin();
        command_result on_update();
        command_result on_state_change(state_change_event event);
    public:
        bool load();
        bool unload();
        bool reload();
        command_result invoke( std::string & command, std::vector <std::string> & parameters, bool interactive );
        bool can_invoke_hotkey( std::string & command, df::viewscreen *top );
        plugin_state getState () const;
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
    private:
        RefLock * access;
        std::vector <PluginCommand> commands;
        std::string filename;
        std::string name;
        DFLibrary * plugin_lib;
        PluginManager * parent;
        plugin_state state;
        command_result (*plugin_init)(Core *, std::vector <PluginCommand> &);
        command_result (*plugin_status)(Core *, std::string &);
        command_result (*plugin_shutdown)(Core *);
        command_result (*plugin_onupdate)(Core *);
        command_result (*plugin_onstatechange)(Core *, state_change_event);
    };
    class DFHACK_EXPORT PluginManager
    {
    // PRIVATE METHODS
        friend class Core;
        friend class Plugin;
        PluginManager(Core * core);
        ~PluginManager();
        void OnUpdate( void );
        void OnStateChange( state_change_event event );
        void registerCommands( Plugin * p );
        void unregisterCommands( Plugin * p );
    // PUBLIC METHODS
    public:
        Plugin *getPluginByName (const std::string & name);
        Plugin *getPluginByCommand (const std::string &command);
        command_result InvokeCommand( std::string & command, std::vector <std::string> & parameters, bool interactive = true );
        bool CanInvokeHotkey(std::string &command, df::viewscreen *top);
        Plugin* operator[] (std::size_t index)
        {
            if(index >= all_plugins.size())
                return 0;
            return all_plugins[index];
        };
        std::size_t size()
        {
            return all_plugins.size();
        }
    // DATA
    private:
        tthread::mutex * cmdlist_mutex;
        std::map <std::string, Plugin *> belongs;
        std::vector <Plugin *> all_plugins;
        std::string plugin_path;
    };

    DFHACK_EXPORT bool default_hotkey(Core *, df::viewscreen *);
}

