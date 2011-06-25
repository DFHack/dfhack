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

#include "dfhack/Export.h"
#include <map>
#include <string>
#include <vector>
struct DFLibrary;
namespace DFHack
{
    class Core;
    class PluginManager;
    enum command_result
    {
        CR_NOT_IMPLEMENTED = -1,
        CR_FAILURE = 0,
        CR_OK = 1
    };
    struct PluginCommand
    {
        PluginCommand(const char * _name,
                      const char * _description,
                      command_result (*function_)(Core *, std::vector <std::string> &)
                     )
        {
            name = _name;
            description = _description;
            function = function_;
        }
        PluginCommand (const PluginCommand & rhs)
        {
            name = rhs.name;
            description = rhs.description;
            function = rhs.function;
        }
        std::string name;
        std::string description;
        command_result (*function)(Core *, std::vector <std::string> &);
    };
    class Plugin
    {
        friend class PluginManager;
    public:
        Plugin(DFHack::Core* core, const std::string& file);
        ~Plugin();
        bool isLoaded ();
        /*
        bool Load ();
        bool Unload ();
        std::string Status ();
        */
    private:
        std::vector <PluginCommand> commands;
        std::string filename;
        std::string name;
        DFLibrary * plugin_lib;
        bool loaded;
        command_result (*plugin_init)(Core *, std::vector <PluginCommand> &);
        command_result (*plugin_status)(Core *, std::string &);
        command_result (*plugin_shutdown)(Core *);
    };
    class DFHACK_EXPORT PluginManager
    {
    public:
        PluginManager(Core * core);
        ~PluginManager();
        Plugin *getPluginByName (const std::string & name);
        command_result InvokeCommand( std::string & command, std::vector <std::string> & parameters);
    private:
        std::map <std::string, PluginCommand *> commands;
        std::vector <Plugin *> all_plugins;
        std::string plugin_path;
    };
}

