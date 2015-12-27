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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sstream>
#include <forward_list>
#include <type_traits>
#include <cstdarg>
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
#include "modules/EventManager.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Graphic.h"
#include "modules/Windows.h"
#include "RemoteServer.h"
#include "LuaTools.h"
#include "DFHackVersion.h"

#include "MiscUtils.h"

using namespace DFHack;

#include "df/ui.h"
#include "df/ui_sidebar_menus.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/interfacest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_game_cleanerst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_savegamest.h"
#include <df/graphic.h>

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <fstream>
#include "tinythread.h"
#include "md5wrapper.h"

#include "SDL_events.h"

using namespace tthread;
using namespace df::enums;
using df::global::init;
using df::global::world;

// FIXME: A lot of code in one file, all doing different things... there's something fishy about it.

static bool parseKeySpec(std::string keyspec, int *psym, int *pmod, std::string *pfocus = NULL);
size_t loadScriptFiles(Core* core, color_ostream& out, const vector<std::string>& prefix, const std::string& folder);

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

struct Core::Private
{
    tthread::mutex AccessMutex;
    tthread::mutex StackMutex;
    std::stack<Core::Cond*> suspended_tools;
    Core::Cond core_cond;
    thread::id df_suspend_thread;
    int df_suspend_depth;

    Private() {
        df_suspend_depth = 0;
    }
};

void Core::cheap_tokenise(string const& input, vector<string> &output)
{
    string *cur = NULL;
    size_t i = 0;

    // Check the first non-space character
    while (i < input.size() && isspace(input[i])) i++;

    // Special verbatim argument mode?
    if (i < input.size() && input[i] == ':')
    {
        // Read the command
        std::string cmd;
        i++;
        while (i < input.size() && !isspace(input[i]))
            cmd.push_back(input[i++]);
        if (!cmd.empty())
            output.push_back(cmd);

        // Find the argument
        while (i < input.size() && isspace(input[i])) i++;

        if (i < input.size())
            output.push_back(input.substr(i));

        return;
    }

    // Otherwise, parse in the regular quoted mode
    for (; i < input.size(); i++)
    {
        unsigned char c = input[i];
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
            color_ostream_proxy out(core->getConsole());

            auto rv = core->runCommand(out, stuff);

            if (rv == CR_NOT_IMPLEMENTED)
                out.printerr("Invalid hotkey command: '%s'\n", stuff.c_str());
        }
    }
}

struct sortable
{
    bool recolor;
    string name;
    string description;
    //FIXME: Nuke when MSVC stops failing at being C++11 compliant
    sortable(bool recolor_,const string& name_,const string & description_): recolor(recolor_), name(name_), description(description_){};
    bool operator <(const sortable & rhs) const
    {
        if( name < rhs.name )
            return true;
        return false;
    };
};

static string dfhack_version_desc()
{
    stringstream s;
    s << Version::dfhack_version() << " ";
    if (Version::is_release())
        s << "(release)";
    else
        s << "(development build " << Version::git_description() << ")";
    return s.str();
}

static std::string getScriptHelp(std::string path, std::string helpprefix)
{
    ifstream script(path.c_str());

    if (script.good())
    {
        std::string help;
        if (getline(script, help) &&
            help.substr(0,helpprefix.length()) == helpprefix)
        {
            help = help.substr(helpprefix.length());
            while (help.size() && help[0] == ' ')
                help = help.substr(1);
            return help;
        }
    }

    return "No help available.";
}

static void listScripts(PluginManager *plug_mgr, std::map<string,string> &pset, std::string path, bool all, std::string prefix = "")
{
    std::vector<string> files;
    Filesystem::listdir(path, files);

    for (size_t i = 0; i < files.size(); i++)
    {
        if (hasEnding(files[i], ".lua"))
        {
            std::string help = getScriptHelp(path + files[i], "--");

            pset[prefix + files[i].substr(0, files[i].size()-4)] = help;
        }
        else if (plug_mgr->ruby && plug_mgr->ruby->is_enabled() && hasEnding(files[i], ".rb"))
        {
            std::string help = getScriptHelp(path + files[i], "#");

            pset[prefix + files[i].substr(0, files[i].size()-3)] = help;
        }
        else if (all && !files[i].empty() && files[i][0] != '.')
        {
            listScripts(plug_mgr, pset, path+files[i]+"/", all, prefix+files[i]+"/");
        }
    }
}

static bool fileExists(std::string path)
{
    ifstream script(path.c_str());
    return script.good();
}

namespace {
    struct ScriptArgs {
        const string *pcmd;
        vector<string> *pargs;
    };
    struct ScriptEnableState {
        const string *pcmd;
        bool pstate;
    };
}

static bool init_run_script(color_ostream &out, lua_State *state, void *info)
{
    auto args = (ScriptArgs*)info;
    if (!lua_checkstack(state, args->pargs->size()+10))
        return false;
    Lua::PushDFHack(state);
    lua_getfield(state, -1, "run_script");
    lua_remove(state, -2);
    lua_pushstring(state, args->pcmd->c_str());
    for (size_t i = 0; i < args->pargs->size(); i++)
        lua_pushstring(state, (*args->pargs)[i].c_str());
    return true;
}

static command_result runLuaScript(color_ostream &out, std::string name, vector<string> &args)
{
    ScriptArgs data;
    data.pcmd = &name;
    data.pargs = &args;

    bool ok = Lua::RunCoreQueryLoop(out, Lua::Core::State, init_run_script, &data);

    return ok ? CR_OK : CR_FAILURE;
}

static bool init_enable_script(color_ostream &out, lua_State *state, void *info)
{
    auto args = (ScriptEnableState*)info;
    if (!lua_checkstack(state, 4))
        return false;
    Lua::PushDFHack(state);
    lua_getfield(state, -1, "enable_script");
    lua_remove(state, -2);
    lua_pushstring(state, args->pcmd->c_str());
    lua_pushboolean(state, args->pstate);
    return true;
}

static command_result enableLuaScript(color_ostream &out, std::string name, bool state)
{
    ScriptEnableState data;
    data.pcmd = &name;
    data.pstate = state;

    bool ok = Lua::RunCoreQueryLoop(out, Lua::Core::State, init_enable_script, &data);

    return ok ? CR_OK : CR_FAILURE;
}

static command_result runRubyScript(color_ostream &out, PluginManager *plug_mgr, std::string name, vector<string> &args)
{
    if (!plug_mgr->ruby || !plug_mgr->ruby->is_enabled())
        return CR_FAILURE;

    std::string rbcmd = "$script_args = [";
    for (size_t i = 0; i < args.size(); i++)
        rbcmd += "'" + args[i] + "', ";
    rbcmd += "]\n";

    rbcmd += "catch(:script_finished) { load './hack/scripts/" + name + ".rb' }";

    return plug_mgr->ruby->eval_ruby(out, rbcmd.c_str());
}

command_result Core::runCommand(color_ostream &out, const std::string &command)
{
    if (!command.empty())
    {
        vector <string> parts;
        Core::cheap_tokenise(command,parts);
        if(parts.size() == 0)
            return CR_NOT_IMPLEMENTED;

        string first = parts[0];
        parts.erase(parts.begin());

        if (first[0] == '#')
            return CR_OK;

        cerr << "Invoking: " << command << endl;
        return runCommand(out, first, parts);
    }
    else
        return CR_NOT_IMPLEMENTED;
}

static bool try_autocomplete(color_ostream &con, const std::string &first, std::string &completed)
{
    std::vector<std::string> possible;

    auto plug_mgr = Core::getInstance().getPluginManager();
    for (auto it = plug_mgr->begin(); it != plug_mgr->end(); ++it)
    {
        const Plugin * plug = it->second;
        for (size_t j = 0; j < plug->size(); j++)
        {
            const PluginCommand &pcmd = (*plug)[j];
            if (pcmd.isHotkeyCommand())
                continue;
            if (pcmd.name.substr(0, first.size()) == first)
                possible.push_back(pcmd.name);
        }
    }

    bool all = (first.find('/') != std::string::npos);

    std::map<string, string> scripts;
    listScripts(plug_mgr, scripts, Core::getInstance().getHackPath() + "scripts/", all);
    for (auto iter = scripts.begin(); iter != scripts.end(); ++iter)
        if (iter->first.substr(0, first.size()) == first)
            possible.push_back(iter->first);

    if (possible.size() == 1)
    {
        completed = possible[0];
        //fprintf(stderr, "Autocompleted %s to %s\n", , );
        con.printerr("%s is not recognized. Did you mean %s?\n", first.c_str(), completed.c_str());
        return true;
    }

    if (possible.size() > 1 && possible.size() < 8)
    {
        std::string out;
        for (size_t i = 0; i < possible.size(); i++)
            out += " " + possible[i];
        con.printerr("%s is not recognized. Possible completions:%s\n", first.c_str(), out.c_str());
        return true;
    }

    return false;
}

bool Core::addScriptPath(string path, bool search_before)
{
    lock_guard<mutex> lock(*script_path_mutex);
    vector<string> &vec = script_paths[search_before ? 0 : 1];
    if (std::find(vec.begin(), vec.end(), path) != vec.end())
        return false;
    if (!Filesystem::isdir(path))
        return false;
    vec.push_back(path);
    return true;
}

bool Core::removeScriptPath(string path)
{
    lock_guard<mutex> lock(*script_path_mutex);
    bool found = false;
    for (int i = 0; i < 2; i++)
    {
        vector<string> &vec = script_paths[i];
        while (1)
        {
            auto it = std::find(vec.begin(), vec.end(), path);
            if (it == vec.end())
                break;
            vec.erase(it);
            found = true;
        }
    }
    return found;
}

void Core::getScriptPaths(std::vector<std::string> *dest)
{
    lock_guard<mutex> lock(*script_path_mutex);
    dest->clear();
    string df_path = this->p->getPath();
    for (auto it = script_paths[0].begin(); it != script_paths[0].end(); ++it)
        dest->push_back(*it);
    if (df::global::world && isWorldLoaded()) {
        string save = World::ReadWorldFolder();
        if (save.size())
            dest->push_back(df_path + "/data/save/" + save + "/raw/scripts");
    }
    dest->push_back(df_path + "/raw/scripts");
    dest->push_back(df_path + "/hack/scripts");
    for (auto it = script_paths[1].begin(); it != script_paths[1].end(); ++it)
        dest->push_back(*it);
}


string Core::findScript(string name)
{
    vector<string> paths;
    getScriptPaths(&paths);
    for (auto it = paths.begin(); it != paths.end(); ++it)
    {
        string path = *it + "/" + name;
        if (Filesystem::isfile(path))
            return path;
    }
    return "";
}

static std::map<std::string, state_change_event> state_change_event_map;
static void sc_event_map_init() {
    if (!state_change_event_map.size())
    {
        #define insert(name) state_change_event_map.insert(std::pair<std::string, state_change_event>(#name, name))
        insert(SC_WORLD_LOADED);
        insert(SC_WORLD_UNLOADED);
        insert(SC_MAP_LOADED);
        insert(SC_MAP_UNLOADED);
        insert(SC_VIEWSCREEN_CHANGED);
        insert(SC_PAUSED);
        insert(SC_UNPAUSED);
        #undef insert
    }
}

static state_change_event sc_event_id (std::string name) {
    sc_event_map_init();
    auto it = state_change_event_map.find(name);
    if (it != state_change_event_map.end())
        return it->second;
    if (name.find("SC_") != 0)
        return sc_event_id(std::string("SC_") + name);
    return SC_UNKNOWN;
}

static std::string sc_event_name (state_change_event id) {
    sc_event_map_init();
    for (auto it = state_change_event_map.begin(); it != state_change_event_map.end(); ++it)
    {
        if (it->second == id)
            return it->first;
    }
    return "SC_UNKNOWN";
}

string getBuiltinCommand(std::string cmd)
{
    std::string builtin = "";
    if (cmd == "ls" ||
        cmd == "help" ||
        cmd == "type" ||
        cmd == "load" ||
        cmd == "unload" ||
        cmd == "reload" ||
        cmd == "enable" ||
        cmd == "disable" ||
        cmd == "plug" ||
        cmd == "keybinding" ||
        cmd == "fpause" ||
        cmd == "cls" ||
        cmd == "die" ||
        cmd == "kill-lua" ||
        cmd == "script" ||
        cmd == "hide" ||
        cmd == "show" ||
        cmd == "sc-script"
    )
        builtin = cmd;

    else if (cmd == "?" || cmd == "man")
        builtin = "help";

    else if (cmd == "dir")
        builtin = "ls";

    else if (cmd == "clear")
        builtin = "cls";

    return builtin;
}

command_result Core::runCommand(color_ostream &con, const std::string &first_, vector<string> &parts)
{
    std::string first = first_;
    if (!first.empty())
    {
        if(first.find('\\') != std::string::npos)
        {
            con.printerr("Replacing backslashes with forward slashes in \"%s\"\n", first.c_str());
            for (size_t i = 0; i < first.size(); i++)
            {
                if (first[i] == '\\')
                    first[i] = '/';
            }
        }

        // let's see what we actually got
        string builtin = getBuiltinCommand(first);
        if (builtin == "help")
        {
            if(!parts.size())
            {
                if (con.is_console())
                {
                    con.print("This is the DFHack console. You can type commands in and manage DFHack plugins from it.\n"
                              "Some basic editing capabilities are included (single-line text editing).\n"
                              "The console also has a command history - you can navigate it with Up and Down keys.\n"
                              "On Windows, you may have to resize your console window. The appropriate menu is accessible\n"
                              "by clicking on the program icon in the top bar of the window.\n\n");
                }
                con.print("Basic commands:\n"
                          "  help|?|man            - This text.\n"
                          "  help COMMAND          - Usage help for the given command.\n"
                          "  ls|dir [-a] [PLUGIN]  - List available commands. Optionally for single plugin.\n"
                          "  cls|clear             - Clear the console.\n"
                          "  fpause                - Force DF to pause.\n"
                          "  die                   - Force DF to close immediately\n"
                          "  keybinding            - Modify bindings of commands to keys\n"
                          "Plugin management (useful for developers):\n"
                          "  plug [PLUGIN|v]       - List plugin state and description.\n"
                          "  load PLUGIN|-all      - Load a plugin by name or load all possible plugins.\n"
                          "  unload PLUGIN|-all    - Unload a plugin or all loaded plugins.\n"
                          "  reload PLUGIN|-all    - Reload a plugin or all loaded plugins.\n"
                         );

                con.print("\nDFHack version %s\n", dfhack_version_desc().c_str());
            }
            else if (parts.size() == 1)
            {
                if (getBuiltinCommand(parts[0]).size())
                {
                    con << parts[0] << ": built-in command; Use `ls`, `help`, or check hack/Readme.html for more information" << std::endl;
                    return CR_NOT_IMPLEMENTED;
                }
                Plugin *plug = plug_mgr->getPluginByCommand(parts[0]);
                if (plug) {
                    for (size_t j = 0; j < plug->size();j++)
                    {
                        const PluginCommand & pcmd = (plug->operator[](j));
                        if (pcmd.name != parts[0])
                            continue;

                        if (pcmd.isHotkeyCommand())
                            con.color(COLOR_CYAN);
                        con.print("%s: %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                        con.reset_color();
                        if (!pcmd.usage.empty())
                            con << "Usage:\n" << pcmd.usage << flush;
                        return CR_OK;
                    }
                }
                string file = findScript(parts[0] + ".lua");
                if ( file != "" ) {
                    string help = getScriptHelp(file, "--");
                    con.print("%s: %s\n", parts[0].c_str(), help.c_str());
                    return CR_OK;
                }
                if (plug_mgr->ruby && plug_mgr->ruby->is_enabled() ) {
                    file = findScript(parts[0] + ".rb");
                    if ( file != "" ) {
                        string help = getScriptHelp(file, "#");
                        con.print("%s: %s\n", parts[0].c_str(), help.c_str());
                        return CR_OK;
                    }
                }
                con.printerr("Unknown command: %s\n", parts[0].c_str());
                return CR_FAILURE;
            }
            else
            {
                con.printerr("not implemented yet\n");
                return CR_NOT_IMPLEMENTED;
            }
        }
        else if (builtin == "load" || builtin == "unload" || builtin == "reload")
        {
            bool all = false;
            bool load = (builtin == "load");
            bool unload = (builtin == "unload");
            if (parts.size())
            {
                for (auto p = parts.begin(); p != parts.end(); p++)
                {
                    if (p->size() && (*p)[0] == '-')
                    {
                        if (p->find('a') != string::npos)
                            all = true;
                    }
                }
                if (all)
                {
                    if (load)
                        plug_mgr->loadAll();
                    else if (unload)
                        plug_mgr->unloadAll();
                    else
                        plug_mgr->reloadAll();
                    return CR_OK;
                }
                for (auto p = parts.begin(); p != parts.end(); p++)
                {
                    if (!p->size() || (*p)[0] == '-')
                        continue;
                    if (load)
                        plug_mgr->load(*p);
                    else if (unload)
                        plug_mgr->unload(*p);
                    else
                        plug_mgr->reload(*p);
                }
            }
            else
                con.printerr("%s: no arguments\n", builtin.c_str());
        }
        else if( builtin == "enable" || builtin == "disable" )
        {
            CoreSuspender suspend;
            bool enable = (builtin == "enable");

            if(parts.size())
            {
                command_result res = CR_OK;

                for (size_t i = 0; i < parts.size(); i++)
                {
                    std::string part = parts[i];
                    if (part.find('\\') != std::string::npos)
                    {
                        con.printerr("Replacing backslashes with forward slashes in \"%s\"\n", part.c_str());
                        for (size_t j = 0; j < part.size(); j++)
                        {
                            if (part[j] == '\\')
                                part[j] = '/';
                        }
                    }

                    Plugin * plug = (*plug_mgr)[part];

                    if(!plug)
                    {
                        std::string lua = findScript(part + ".lua");
                        if (lua.size())
                        {
                            res = enableLuaScript(con, part, enable);
                        }
                        else
                        {
                            res = CR_NOT_FOUND;
                            con.printerr("No such plugin or Lua script: %s\n", part.c_str());
                        }
                    }
                    else if (!plug->can_set_enabled())
                    {
                        res = CR_NOT_IMPLEMENTED;
                        con.printerr("Cannot %s plugin: %s\n", builtin.c_str(), part.c_str());
                    }
                    else
                    {
                        res = plug->set_enabled(con, enable);

                        if (res != CR_OK || plug->is_enabled() != enable)
                            con.printerr("Could not %s plugin: %s\n", builtin.c_str(), part.c_str());
                    }
                }

                return res;
            }
            else
            {
                for (auto it = plug_mgr->begin(); it != plug_mgr->end(); ++it)
                {
                    Plugin * plug = it->second;
                    if (!plug->can_be_enabled()) continue;

                    con.print(
                        "%20s\t%-3s%s\n",
                        (plug->getName()+":").c_str(),
                        plug->is_enabled() ? "on" : "off",
                        plug->can_set_enabled() ? "" : " (controlled elsewhere)"
                    );
                }
            }
        }
        else if (builtin == "ls" || builtin == "dir")
        {
            bool all = false;
            if (parts.size() && parts[0] == "-a")
            {
                all = true;
                vector_erase_at(parts, 0);
            }
            if(parts.size())
            {
                string & plugname = parts[0];
                const Plugin * plug = (*plug_mgr)[plugname];
                if(!plug)
                {
                    con.printerr("There's no plugin called %s!\n", plugname.c_str());
                }
                else if (plug->getState() != Plugin::PS_LOADED)
                {
                    con.printerr("Plugin %s is not loaded.\n", plugname.c_str());
                }
                else if (!plug->size())
                {
                    con.printerr("Plugin %s is loaded but does not implement any commands.\n", plugname.c_str());
                }
                else for (size_t j = 0; j < plug->size();j++)
                {
                    const PluginCommand & pcmd = (plug->operator[](j));
                    if (pcmd.isHotkeyCommand())
                        con.color(COLOR_CYAN);
                    con.print("  %-22s - %s\n",pcmd.name.c_str(), pcmd.description.c_str());
                    con.reset_color();
                }
            }
            else
            {
                con.print(
                "builtin:\n"
                "  help|?|man                  - This text or help specific to a plugin.\n"
                "  ls|dir [-a] [PLUGIN]        - List available commands. Optionally for single plugin.\n"
                "  cls|clear                   - Clear the console.\n"
                "  fpause                      - Force DF to pause.\n"
                "  die                         - Force DF to close immediately\n"
                "  kill-lua                    - Stop an active Lua script\n"
                "  keybinding                  - Modify bindings of commands to keys\n"
                "  script FILENAME             - Run the commands specified in a file.\n"
                "  sc-script                   - Automatically run specified scripts on state change events\n"
                "  plug [PLUGIN|v]             - List plugin state and detailed description.\n"
                "  load PLUGIN|-all [...]      - Load a plugin by name or load all possible plugins.\n"
                "  unload PLUGIN|-all [...]    - Unload a plugin or all loaded plugins.\n"
                "  reload PLUGIN|-all [...]    - Reload a plugin or all loaded plugins.\n"
                "  enable/disable PLUGIN [...] - Enable or disable a plugin if supported.\n"
                "  type COMMAND                - Display information about where a command is implemented\n"
                "\n"
                "plugins:\n"
                );
                std::set <sortable> out;
                for (auto it = plug_mgr->begin(); it != plug_mgr->end(); ++it)
                {
                    const Plugin * plug = it->second;
                    if(!plug->size())
                        continue;
                    for (size_t j = 0; j < plug->size();j++)
                    {
                        const PluginCommand & pcmd = (plug->operator[](j));
                        out.insert(sortable(pcmd.isHotkeyCommand(),pcmd.name,pcmd.description));
                    }
                }
                for(auto iter = out.begin();iter != out.end();iter++)
                {
                    if ((*iter).recolor)
                        con.color(COLOR_CYAN);
                    con.print("  %-22s- %s\n",(*iter).name.c_str(), (*iter).description.c_str());
                    con.reset_color();
                }
                std::map<string, string> scripts;
                listScripts(plug_mgr, scripts, getHackPath() + "scripts/", all);
                if (!scripts.empty())
                {
                    con.print("\nscripts:\n");
                    for (auto iter = scripts.begin(); iter != scripts.end(); ++iter)
                        con.print("  %-22s- %s\n", iter->first.c_str(), iter->second.c_str());
                }
            }
        }
        else if (builtin == "plug")
        {
            const char *header_format = "%25s %10s %4s %8s\n";
            const char *row_format =    "%25s %10s %4i %8s\n";
            con.print(header_format, "Name", "State", "Cmds", "Enabled");

            plug_mgr->refresh();
            for (auto it = plug_mgr->begin(); it != plug_mgr->end(); ++it)
            {
                Plugin * plug = it->second;
                if (!plug)
                    continue;
                if (parts.size() && std::find(parts.begin(), parts.end(), plug->getName()) == parts.end())
                    continue;
                color_value color;
                switch (plug->getState())
                {
                    case Plugin::PS_LOADED:
                        color = COLOR_RESET;
                        break;
                    case Plugin::PS_UNLOADED:
                    case Plugin::PS_UNLOADING:
                        color = COLOR_YELLOW;
                        break;
                    case Plugin::PS_LOADING:
                        color = COLOR_LIGHTBLUE;
                        break;
                    case Plugin::PS_BROKEN:
                        color = COLOR_LIGHTRED;
                        break;
                    default:
                        color = COLOR_LIGHTMAGENTA;
                        break;
                }
                con.color(color);
                con.print(row_format,
                    plug->getName().c_str(),
                    Plugin::getStateDescription(plug->getState()),
                    plug->size(),
                    (plug->can_be_enabled()
                        ? (plug->is_enabled() ? "enabled" : "disabled")
                        : "n/a")
                );
                con.color(COLOR_RESET);
            }
        }
        else if (builtin == "type")
        {
            if (!parts.size())
            {
                con.printerr("type: no argument\n");
                return CR_WRONG_USAGE;
            }
            con << parts[0];
            string builtin_cmd = getBuiltinCommand(parts[0]);
            string lua_path = findScript(parts[0] + ".lua");
            string ruby_path = findScript(parts[0] + ".rb");
            Plugin *plug = plug_mgr->getPluginByCommand(parts[0]);
            if (builtin_cmd.size())
            {
                con << " is a built-in command";
                if (builtin_cmd != parts[0])
                    con << " (aliased to " << builtin_cmd << ")";
                con << std::endl;
            }
            else if (plug)
            {
                con << " is a command implemented by the plugin " << plug->getName() << std::endl;
            }
            else if (lua_path.size())
            {
                con << " is a Lua script: " << lua_path << std::endl;
            }
            else if (ruby_path.size())
            {
                con << " is a Ruby script: " << ruby_path << std::endl;
            }
            else
            {
                con << " is not a recognized command." << std::endl;
                plug = plug_mgr->getPluginByName(parts[0]);
                if (plug)
                    con << "Plugin " << parts[0] << " exists and implements " << plug->size() << " commands." << std::endl;
                return CR_FAILURE;
            }
        }
        else if (builtin == "keybinding")
        {
            if (parts.size() >= 3 && (parts[0] == "set" || parts[0] == "add"))
            {
                std::string keystr = parts[1];
                if (parts[0] == "set")
                    ClearKeyBindings(keystr);
                for (int i = parts.size()-1; i >= 2; i--)
                {
                    if (!AddKeyBinding(keystr, parts[i])) {
                        con.printerr("Invalid key spec: %s\n", keystr.c_str());
                        break;
                    }
                }
            }
            else if (parts.size() >= 2 && parts[0] == "clear")
            {
                for (size_t i = 1; i < parts.size(); i++)
                {
                    if (!ClearKeyBindings(parts[i])) {
                        con.printerr("Invalid key spec: %s\n", parts[i].c_str());
                        break;
                    }
                }
            }
            else if (parts.size() == 2 && parts[0] == "list")
            {
                std::vector<std::string> list = ListKeyBindings(parts[1]);
                if (list.empty())
                    con << "No bindings." << endl;
                for (size_t i = 0; i < list.size(); i++)
                    con << "  " << list[i] << endl;
            }
            else
            {
                con << "Usage:" << endl
                    << "  keybinding list <key>" << endl
                    << "  keybinding clear <key>[@context]..." << endl
                    << "  keybinding set <key>[@context] \"cmdline\" \"cmdline\"..." << endl
                    << "  keybinding add <key>[@context] \"cmdline\" \"cmdline\"..." << endl
                    << "Later adds, and earlier items within one command have priority." << endl
                    << "Supported keys: [Ctrl-][Alt-][Shift-](A-Z, or F1-F9, or Enter)." << endl
                    << "Context may be used to limit the scope of the binding, by" << endl
                    << "requiring the current context to have a certain prefix." << endl
                    << "Current UI context is: "
                    << Gui::getFocusString(Core::getTopViewscreen()) << endl;
            }
        }
        else if (builtin == "fpause")
        {
            World::SetPauseState(true);
            con.print("The game was forced to pause!\n");
        }
        else if (builtin == "cls")
        {
            if (con.is_console())
                ((Console&)con).clear();
            else
            {
                con.printerr("No console to clear.\n");
                return CR_NEEDS_CONSOLE;
            }
        }
        else if (builtin == "die")
        {
            _exit(666);
        }
        else if (builtin == "kill-lua")
        {
            bool force = false;
            for (auto it = parts.begin(); it != parts.end(); ++it)
            {
                if (*it == "force")
                    force = true;
            }
            if (!Lua::Interrupt(force))
                con.printerr("Failed to register hook - use 'kill-lua force' to force\n");
        }
        else if (builtin == "script")
        {
            if(parts.size() == 1)
            {
                loadScriptFile(con, parts[0], false);
            }
            else
            {
                con << "Usage:" << endl
                    << "  script <filename>" << endl;
                return CR_WRONG_USAGE;
            }
        }
        else if (builtin=="hide")
        {
            if (!getConsole().hide())
            {
                con.printerr("Could not hide console\n");
                return CR_FAILURE;
            }
            return CR_OK;
        }
        else if (builtin=="show")
        {
            if (!getConsole().show())
            {
                con.printerr("Could not show console\n");
                return CR_FAILURE;
            }
            return CR_OK;
        }
        else if (builtin == "sc-script")
        {
            if (parts.size() < 1)
            {
                con << "Usage: sc-script add|remove|list|help SC_EVENT [path-to-script] [...]" << endl;
                return CR_WRONG_USAGE;
            }
            if (parts[0] == "help" || parts[0] == "?")
            {
                con << "Valid event names (SC_ prefix is optional):" << endl;
                for (int i = SC_WORLD_LOADED; i <= SC_UNPAUSED; i++)
                {
                    std::string name = sc_event_name((state_change_event)i);
                    if (name != "SC_UNKNOWN")
                        con << "  " << name << endl;
                }
                return CR_OK;
            }
            else if (parts[0] == "list")
            {
                if(parts.size() < 2)
                    parts.push_back("");
                if (parts[1].size() && sc_event_id(parts[1]) == SC_UNKNOWN)
                {
                    con << "Unrecognized event name: " << parts[1] << endl;
                    return CR_WRONG_USAGE;
                }
                for (auto it = state_change_scripts.begin(); it != state_change_scripts.end(); ++it)
                {
                    if (!parts[1].size() || (it->event == sc_event_id(parts[1])))
                    {
                        con.print("%s (%s): %s%s\n", sc_event_name(it->event).c_str(),
                            it->save_specific ? "save-specific" : "global",
                            it->save_specific ? "<save folder>/raw/" : "<DF folder>/",
                            it->path.c_str());
                    }
                }
                return CR_OK;
            }
            else if (parts[0] == "add")
            {
                if (parts.size() < 3 || (parts.size() >= 4 && parts[3] != "-save"))
                {
                    con << "Usage: sc-script add EVENT path-to-script [-save]" << endl;
                    return CR_WRONG_USAGE;
                }
                state_change_event evt = sc_event_id(parts[1]);
                if (evt == SC_UNKNOWN)
                {
                    con << "Unrecognized event: " << parts[1] << endl;
                    return CR_FAILURE;
                }
                bool save_specific = (parts.size() >= 4 && parts[3] == "-save");
                StateChangeScript script(evt, parts[2], save_specific);
                for (auto it = state_change_scripts.begin(); it != state_change_scripts.end(); ++it)
                {
                    if (script == *it)
                    {
                        con << "Script already registered" << endl;
                        return CR_FAILURE;
                    }
                }
                state_change_scripts.push_back(script);
                return CR_OK;
            }
            else if (parts[0] == "remove")
            {
                if (parts.size() < 3 || (parts.size() >= 4 && parts[3] != "-save"))
                {
                    con << "Usage: sc-script remove EVENT path-to-script [-save]" << endl;
                    return CR_WRONG_USAGE;
                }
                state_change_event evt = sc_event_id(parts[1]);
                if (evt == SC_UNKNOWN)
                {
                    con << "Unrecognized event: " << parts[1] << endl;
                    return CR_FAILURE;
                }
                bool save_specific = (parts.size() >= 4 && parts[3] == "-save");
                StateChangeScript tmp(evt, parts[2], save_specific);
                auto it = std::find(state_change_scripts.begin(), state_change_scripts.end(), tmp);
                if (it != state_change_scripts.end())
                {
                    state_change_scripts.erase(it);
                    return CR_OK;
                }
                else
                {
                    con << "Unrecognized script" << endl;
                    return CR_FAILURE;
                }
            }
            else
            {
                con << "Usage: sc-script add|remove|list|help SC_EVENT [path-to-script] [...]" << endl;
                return CR_WRONG_USAGE;
            }
        }
        else
        {
            command_result res = plug_mgr->InvokeCommand(con, first, parts);
            if(res == CR_NOT_IMPLEMENTED)
            {
                string completed;
                string filename = findScript(first + ".lua");
                bool lua = filename != "";
                if ( !lua ) {
                    filename = findScript(first + ".rb");
                }
                if ( lua )
                    res = runLuaScript(con, first, parts);
                else if ( filename != "" && plug_mgr->ruby && plug_mgr->ruby->is_enabled() )
                    res = runRubyScript(con, plug_mgr, first, parts);
                else if ( try_autocomplete(con, first, completed) )
                    res = CR_NOT_IMPLEMENTED;
                else
                    con.printerr("%s is not a recognized command.\n", first.c_str());
                if (res == CR_NOT_IMPLEMENTED)
                {
                    Plugin *p = plug_mgr->getPluginByName(first);
                    if (p)
                    {
                        con.printerr("%s is a plugin ", first.c_str());
                        if (p->getState() == Plugin::PS_UNLOADED)
                            con.printerr("that is not loaded - try \"load %s\" or check stderr.log\n",
                                first.c_str());
                        else if (p->size())
                            con.printerr("that implements %i commands - see \"ls %s\" for details\n",
                                p->size(), first.c_str());
                        else
                            con.printerr("but does not implement any commands\n");
                    }
                }
            }
            else if (res == CR_NEEDS_CONSOLE)
                con.printerr("%s needs interactive console to work.\n", first.c_str());
            return res;
        }

        return CR_OK;
    }

    return CR_NOT_IMPLEMENTED;
}

bool Core::loadScriptFile(color_ostream &out, string fname, bool silent)
{
    if(!silent)
        out << "Loading script at " << fname << std::endl;
    ifstream script(fname.c_str());
    if ( !script.good() )
    {
        if(!silent)
            out.printerr("Error loading script\n");
        return false;
    }
    string command;
    while(script.good()) {
        string temp;
        getline(script,temp);
        bool doMore = false;
        if ( temp.length() > 0 ) {
            if ( temp[0] == '#' )
                continue;
            if ( temp[temp.length()-1] == '\r' )
                temp = temp.substr(0,temp.length()-1);
            if ( temp.length() > 0 ) {
                if ( temp[temp.length()-1] == '\\' ) {
                    temp = temp.substr(0,temp.length()-1);
                    doMore = true;
                }
            }
        }
        command = command + temp;
        if ( (!doMore || !script.good()) && !command.empty() ) {
            runCommand(out, command);
            command = "";
        }
    }
    return true;
}

static void run_dfhack_init(color_ostream &out, Core *core)
{
    if (!df::global::world || !df::global::ui || !df::global::gview)
    {
        out.printerr("Key globals are missing, skipping loading dfhack.init.\n");
        return;
    }

    std::vector<std::string> prefixes(1, "dfhack");
    size_t count = loadScriptFiles(core, out, prefixes, ".");
    if (!count || !Filesystem::isfile("dfhack.init"))
    {
        core->runCommand(out, "gui/no-dfhack-init");
        core->loadScriptFile(out, "dfhack.init-example", true);
    }
}

// Load dfhack.init in a dedicated thread (non-interactive console mode)
void fInitthread(void * iodata)
{
    IODATA * iod = ((IODATA*) iodata);
    Core * core = iod->core;
    color_ostream_proxy out(core->getConsole());

    run_dfhack_init(out, core);
}

// A thread function... for the interactive console.
void fIOthread(void * iodata)
{
    IODATA * iod = ((IODATA*) iodata);
    Core * core = iod->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;

    CommandHistory main_history;
    main_history.load("dfhack.history");

    Console & con = core->getConsole();
    if (plug_mgr == 0)
    {
        con.printerr("Something horrible happened in Core's constructor...\n");
        return;
    }

    run_dfhack_init(con, core);

    con.print("DFHack is ready. Have a nice day!\n"
              "DFHack version %s\n"
              "Type in '?' or 'help' for general help, 'ls' to see all commands.\n",
              dfhack_version_desc().c_str());

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

        auto rv = core->runCommand(con, command);

        if (rv == CR_NOT_IMPLEMENTED)
            clueless_counter++;

        if(clueless_counter == 3)
        {
            con.print("Run 'help' or '?' for the list of available commands.\n");
            clueless_counter = 0;
        }
    }
}

Core::Core()
{
    d = new Private();

    // init the console. This must be always the first step!
    plug_mgr = 0;
    vif = 0;
    p = 0;
    errorstate = false;
    vinfo = 0;
    started = false;
    memset(&(s_mods), 0, sizeof(s_mods));

    // set up hotkey capture
    hotkey_set = false;
    HotkeyMutex = 0;
    HotkeyCond = 0;
    misc_data_mutex=0;
    last_world_data_ptr = NULL;
    last_local_map_ptr = NULL;
    last_pause_state = false;
    top_viewscreen = NULL;
    screen_window = NULL;
    server = NULL;

    color_ostream::log_errors_to_stderr = true;

    script_path_mutex = new mutex();
};

void Core::fatal (std::string output)
{
    errorstate = true;
    stringstream out;
    out << output ;
    if (output[output.size() - 1] != '\n')
        out << '\n';
    out << "DFHack will now deactivate.\n";
    if(con.isInited())
    {
        con.printerr("%s", out.str().c_str());
        con.reset_color();
        con.print("\n");
    }
    fprintf(stderr, "%s\n", out.str().c_str());
#ifndef LINUX_BUILD
    out << "Check file stderr.log for details\n";
    MessageBox(0,out.str().c_str(),"DFHack error!", MB_OK | MB_ICONERROR);
#else
    cout << "DFHack fatal error: " << out.str() << std::endl;
#endif
}

std::string Core::getHackPath()
{
#ifdef LINUX_BUILD
    return p->getPath() + "/hack/";
#else
    return p->getPath() + "\\hack\\";
#endif
}

void init_screen_module(Core *);

bool Core::Init()
{
    if(started)
        return true;
    if(errorstate)
        return false;

    fprintf(stderr, "DFHack build: %s\n", Version::git_description());

    // find out what we are...
    #ifdef LINUX_BUILD
        const char * path = "hack/symbols.xml";
    #else
        const char * path = "hack\\symbols.xml";
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
        out << "Error while reading symbols.xml:\n";
        out << err.what() << std::endl;
        delete vif;
        vif = NULL;
        errorstate = true;
        fatal(out.str());
        return false;
    }
    p = new DFHack::Process(vif);
    vinfo = p->getDescriptor();

    if(!vinfo || !p->isIdentified())
    {
        fatal("Not a known DF version.\n");
        errorstate = true;
        delete p;
        p = NULL;
        return false;
    }
    cerr << "Version: " << vinfo->getVersion() << endl;

    // Init global object pointers
    df::global::InitGlobals();

    cerr << "Initializing Console.\n";
    // init the console.
    bool is_text_mode = (init && init->display.flag.is_set(init_display_flags::TEXT));
    if (is_text_mode || getenv("DFHACK_DISABLE_CONSOLE"))
    {
        con.init(true);
        cerr << "Console is not available. Use dfhack-run to send commands.\n";
        if (!is_text_mode)
        {
            cout << "Console disabled.\n";
        }
    }
    else if(con.init(false))
        cerr << "Console is running.\n";
    else
        cerr << "Console has failed to initialize!\n";
/*
    // dump offsets to a file
    std::ofstream dump("offsets.log");
    if(!dump.fail())
    {
        //dump << vinfo->PrintOffsets();
        dump.close();
    }
    */
    // initialize data defs
    virtual_identity::Init(this);
    init_screen_module(this);

    // copy over default config files if necessary
    std::vector<std::string> config_files;
    std::vector<std::string> default_config_files;
    if (Filesystem::listdir("dfhack-config", config_files) != 0)
        con.printerr("Failed to list directory: dfhack-config");
    else if (Filesystem::listdir("dfhack-config/default", default_config_files) != 0)
        con.printerr("Failed to list directory: dfhack-config/default");
    else
    {
        for (auto it = default_config_files.begin(); it != default_config_files.end(); ++it)
        {
            std::string filename = *it;
            if (std::find(config_files.begin(), config_files.end(), filename) == config_files.end())
            {
                std::string src_file = std::string("dfhack-config/default/") + filename;
                if (!Filesystem::isfile(src_file))
                    continue;
                std::string dest_file = std::string("dfhack-config/") + filename;
                std::ifstream src(src_file, std::ios::binary);
                std::ofstream dest(dest_file, std::ios::binary);
                if (!src.good() || !dest.good())
                {
                    con.printerr("Copy failed: %s\n", filename.c_str());
                    continue;
                }
                dest << src.rdbuf();
                src.close();
                dest.close();
            }
        }
    }

    // initialize common lua context
    if (!Lua::Core::Init(con))
    {
        fatal("Lua failed to initialize");
        return false;
    }

    // create mutex for syncing with interactive tasks
    misc_data_mutex=new mutex();
    cerr << "Initializing Plugins.\n";
    // create plugin manager
    plug_mgr = new PluginManager(this);
    plug_mgr->init();
    IODATA *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;

    HotkeyMutex = new mutex();
    HotkeyCond = new condition_variable();

    if (!is_text_mode)
    {
        cerr << "Starting IO thread.\n";
        // create IO thread
        thread * IO = new thread(fIOthread, (void *) temp);
    }
    else
    {
        cerr << "Starting dfhack.init thread.\n";
        thread * init = new thread(fInitthread, (void *) temp);
    }

    cerr << "Starting DF input capture thread.\n";
    // set up hotkey capture
    thread * HK = new thread(fHKthread, (void *) temp);
    screen_window = new Windows::top_level_window();
    screen_window->addChild(new Windows::dfhack_dummy(5,10));
    started = true;
    modstate = 0;

    cerr << "Starting the TCP listener.\n";
    server = new ServerMain();
    if (!server->listen(RemoteClient::GetDefaultPort()))
        cerr << "TCP listen failed.\n";

    if (df::global::ui_sidebar_menus)
    {
        vector<string> args;
        const string & raw = df::global::ui_sidebar_menus->command_line.raw;
        size_t offset = 0;
        while (offset < raw.size())
        {
            if (raw[offset] == '"')
            {
                offset++;
                size_t next = raw.find("\"", offset);
                args.push_back(raw.substr(offset, next - offset));
                offset = next + 2;
            }
            else
            {
                size_t next = raw.find(" ", offset);
                if (next == string::npos)
                {
                    args.push_back(raw.substr(offset));
                    offset = raw.size();
                }
                else
                {
                    args.push_back(raw.substr(offset, next - offset));
                    offset = next + 1;
                }
            }
        }
        for (auto it = args.begin(); it != args.end(); )
        {
            const string & first = *it;
            if (first.length() > 0 && first[0] == '+')
            {
                vector<string> cmd;
                for (it++; it != args.end(); it++) {
                    const string & arg = *it;
                    if (arg.length() > 0 && arg[0] == '+')
                    {
                        break;
                    }
                    cmd.push_back(arg);
                }

                if (runCommand(con, first.substr(1), cmd) != CR_OK)
                {
                    cerr << "Error running command: " << first.substr(1);
                    for (auto it2 = cmd.begin(); it2 != cmd.end(); it2++)
                    {
                        cerr << " \"" << *it2 << "\"";
                    }
                    cerr << "\n";
                }
            }
            else
            {
                it++;
            }
        }
    }

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

void Core::print(const char *format, ...)
{
    color_ostream_proxy proxy(getInstance().con);

    va_list args;
    va_start(args,format);
    proxy.vprint(format,args);
    va_end(args);
}

void Core::printerr(const char *format, ...)
{
    color_ostream_proxy proxy(getInstance().con);

    va_list args;
    va_start(args,format);
    proxy.vprinterr(format,args);
    va_end(args);
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

bool Core::isSuspended(void)
{
    lock_guard<mutex> lock(d->AccessMutex);

    return (d->df_suspend_depth > 0 && d->df_suspend_thread == this_thread::get_id());
}

void Core::Suspend()
{
    auto tid = this_thread::get_id();

    // If recursive, just increment the count
    {
        lock_guard<mutex> lock(d->AccessMutex);

        if (d->df_suspend_depth > 0 && d->df_suspend_thread == tid)
        {
            d->df_suspend_depth++;
            return;
        }
    }

    // put the condition on a stack
    Core::Cond *nc = new Core::Cond();

    {
        lock_guard<mutex> lock2(d->StackMutex);

        d->suspended_tools.push(nc);
    }

    // wait until Core::Update() wakes up the tool
    {
        lock_guard<mutex> lock(d->AccessMutex);

        nc->Lock(&d->AccessMutex);

        assert(d->df_suspend_depth == 0);
        d->df_suspend_thread = tid;
        d->df_suspend_depth = 1;
    }
}

void Core::Resume()
{
    auto tid = this_thread::get_id();
    lock_guard<mutex> lock(d->AccessMutex);

    assert(d->df_suspend_depth > 0 && d->df_suspend_thread == tid);

    if (--d->df_suspend_depth == 0)
        d->core_cond.Unlock();
}

int Core::TileUpdate()
{
    if(!started)
        return false;
    screen_window->paint();
    return true;
}

int Core::ClaimSuspend(bool force_base)
{
    auto tid = this_thread::get_id();
    lock_guard<mutex> lock(d->AccessMutex);

    if (force_base || d->df_suspend_depth <= 0)
    {
        assert(d->df_suspend_depth == 0);

        d->df_suspend_thread = tid;
        d->df_suspend_depth = 1000000;
        return 1000000;
    }
    else
    {
        assert(d->df_suspend_thread == tid);
        return ++d->df_suspend_depth;
    }
}

void Core::DisclaimSuspend(int level)
{
    auto tid = this_thread::get_id();
    lock_guard<mutex> lock(d->AccessMutex);

    assert(d->df_suspend_depth == level && d->df_suspend_thread == tid);

    if (level == 1000000)
        d->df_suspend_depth = 0;
    else
        --d->df_suspend_depth;
}

void Core::doUpdate(color_ostream &out, bool first_update)
{
    Lua::Core::Reset(out, "DF code execution");

    if (first_update)
        onStateChange(out, SC_CORE_INITIALIZED);

    // find the current viewscreen
    df::viewscreen *screen = NULL;
    if (df::global::gview)
    {
        screen = &df::global::gview->view;
        while (screen->child)
            screen = screen->child;
    }

    // detect if the viewscreen changed, and trigger events later
    bool vs_changed = false;
    if (screen != top_viewscreen)
    {
        top_viewscreen = screen;
        vs_changed = true;
    }

    bool is_load_save =
        strict_virtual_cast<df::viewscreen_game_cleanerst>(screen) ||
        strict_virtual_cast<df::viewscreen_loadgamest>(screen) ||
        strict_virtual_cast<df::viewscreen_savegamest>(screen);

    // detect if the game was loaded or unloaded in the meantime
    void *new_wdata = NULL;
    void *new_mapdata = NULL;
    if (df::global::world && !is_load_save)
    {
        df::world_data *wdata = df::global::world->world_data;
        // when the game is unloaded, world_data isn't deleted, but its contents are
        // regions work to detect arena too
        if (wdata && !wdata->regions.empty())
            new_wdata = wdata;
        new_mapdata = df::global::world->map.block_index;
    }

    // if the world changes
    if (new_wdata != last_world_data_ptr)
    {
        // we check for map change too
        bool had_map = isMapLoaded();
        last_world_data_ptr = new_wdata;
        last_local_map_ptr = new_mapdata;

        World::ClearPersistentCache();

        // and if the world is going away, we report the map change first
        if(had_map)
            onStateChange(out, SC_MAP_UNLOADED);
        // and if the world is appearing, we report map change after that
        onStateChange(out, new_wdata ? SC_WORLD_LOADED : SC_WORLD_UNLOADED);
        if(isMapLoaded())
            onStateChange(out, SC_MAP_LOADED);
    }
    // otherwise just check for map change...
    else if (new_mapdata != last_local_map_ptr)
    {
        bool had_map = isMapLoaded();
        last_local_map_ptr = new_mapdata;

        if (isMapLoaded() != had_map)
        {
            World::ClearPersistentCache();
            onStateChange(out, new_mapdata ? SC_MAP_LOADED : SC_MAP_UNLOADED);
        }
    }

    if (vs_changed)
        onStateChange(out, SC_VIEWSCREEN_CHANGED);

    if (df::global::pause_state)
    {
        if (*df::global::pause_state != last_pause_state)
        {
            onStateChange(out, last_pause_state ? SC_UNPAUSED : SC_PAUSED);
            last_pause_state = *df::global::pause_state;
        }
    }

    // Execute per-frame handlers
    onUpdate(out);

    out << std::flush;
}

// should always be from simulation thread!
int Core::Update()
{
    if(errorstate)
        return -1;

    color_ostream_proxy out(con);

    // Pretend this thread has suspended the core in the usual way,
    // and run various processing hooks.
    {
        CoreSuspendClaimer suspend(true);

        // Initialize the core
        bool first_update = false;

        if(!started)
        {
            first_update = true;
            Init();
            if(errorstate)
                return -1;
            Lua::Core::Reset(con, "core init");
        }

        doUpdate(out, first_update);
    }

    // wake waiting tools
    // do not allow more tools to join in while we process stuff here
    lock_guard<mutex> lock_stack(d->StackMutex);

    while (!d->suspended_tools.empty())
    {
        Core::Cond * nc = d->suspended_tools.top();
        d->suspended_tools.pop();

        lock_guard<mutex> lock(d->AccessMutex);
        // wake tool
        nc->Unlock();
        // wait for tool to wake us
        d->core_cond.Lock(&d->AccessMutex);
        // verify
        assert(d->df_suspend_depth == 0);
        // destroy condition
        delete nc;
        // check lua stack depth
        Lua::Core::Reset(out, "suspend");
    }

    return 0;
};

extern bool buildings_do_onupdate;
void buildings_onStateChange(color_ostream &out, state_change_event event);
void buildings_onUpdate(color_ostream &out);

static int buildings_timer = 0;

void Core::onUpdate(color_ostream &out)
{
    EventManager::manageEvents(out);

    // convert building reagents
    if (buildings_do_onupdate && (++buildings_timer & 1))
        buildings_onUpdate(out);

    // notify all the plugins that a game tick is finished
    plug_mgr->OnUpdate(out);

    // process timers in lua
    Lua::Core::onUpdate(out);
}

void getFilesWithPrefixAndSuffix(const std::string& folder, const std::string& prefix, const std::string& suffix, std::vector<std::string>& result) {
    //DFHACK_EXPORT int listdir (std::string dir, std::vector<std::string> &files);
    std::vector<std::string> files;
    DFHack::Filesystem::listdir(folder, files);
    for ( size_t a = 0; a < files.size(); a++ ) {
        if ( prefix.length() > files[a].length() )
            continue;
        if ( suffix.length() > files[a].length() )
            continue;
        if ( files[a].compare(0, prefix.length(), prefix) != 0 )
            continue;
        if ( files[a].compare(files[a].length()-suffix.length(), suffix.length(), suffix) != 0 )
            continue;
        result.push_back(files[a]);
    }
    return;
}

size_t loadScriptFiles(Core* core, color_ostream& out, const vector<std::string>& prefix, const std::string& folder) {
    vector<std::string> scriptFiles;
    for ( size_t a = 0; a < prefix.size(); a++ ) {
        getFilesWithPrefixAndSuffix(folder, prefix[a], ".init", scriptFiles);
    }
    std::sort(scriptFiles.begin(), scriptFiles.end());
    size_t result = 0;
    for ( size_t a = 0; a < scriptFiles.size(); a++ ) {
        result++;
        core->loadScriptFile(out, folder + "/" + scriptFiles[a], true);
    }
    return result;
}

namespace DFHack {
    namespace X {
        typedef state_change_event Key;
        typedef vector<string> Val;
        typedef pair<Key,Val> Entry;
        typedef vector<Entry> EntryVector;
        typedef map<Key,Val> InitVariationTable;

        EntryVector computeInitVariationTable(void* none, ...) {
            va_list list;
            va_start(list,none);
            EntryVector result;
            while(true) {
                Key key = (Key)va_arg(list,int);
                if ( key == SC_UNKNOWN )
                    break;
                Val val;
                while (true) {
                    const char *v = va_arg(list, const char *);
                    if (!v || !v[0])
                        break;
                    val.push_back(string(v));
                }
                result.push_back(Entry(key,val));
            }
            va_end(list);
            return result;
        }

        InitVariationTable getTable(const EntryVector& vec) {
            return InitVariationTable(vec.begin(),vec.end());
        }
    }
}

void Core::handleLoadAndUnloadScripts(color_ostream& out, state_change_event event) {
    static const X::InitVariationTable table = X::getTable(X::computeInitVariationTable(0,
        (int)SC_WORLD_LOADED, "onLoad", "onLoadWorld", "onWorldLoaded", "",
        (int)SC_WORLD_UNLOADED, "onUnload", "onUnloadWorld", "onWorldUnloaded", "",
        (int)SC_MAP_LOADED, "onMapLoad", "onLoadMap", "",
        (int)SC_MAP_UNLOADED, "onMapUnload", "onUnloadMap", "",
        (int)SC_UNKNOWN
    ));

    if (!df::global::world)
        return;
    std::string rawFolder = "data/save/" + (df::global::world->cur_savegame.save_dir) + "/raw/";

    auto i = table.find(event);
    if ( i != table.end() ) {
        const std::vector<std::string>& set = i->second;
        loadScriptFiles(this, out, set, "."      );
        loadScriptFiles(this, out, set, rawFolder);
        loadScriptFiles(this, out, set, rawFolder + "objects/");
    }

    for (auto it = state_change_scripts.begin(); it != state_change_scripts.end(); ++it)
    {
        if (it->event == event)
        {
            if (!it->save_specific)
            {
                if (!loadScriptFile(out, it->path, true))
                    out.printerr("Could not load script: %s\n", it->path.c_str());
            }
            else if (it->save_specific && isWorldLoaded())
            {
                loadScriptFile(out, rawFolder + it->path, true);
            }
        }
    }
}

void Core::onStateChange(color_ostream &out, state_change_event event)
{
    using df::global::gametype;
    static md5wrapper md5w;
    static std::string ostype = "";

    if (!ostype.size())
    {
        ostype = "unknown OS";
        if (vinfo) {
            switch (vinfo->getOS())
            {
            case OS_WINDOWS:
                ostype = "Windows";
                break;
            case OS_APPLE:
                ostype = "OS X";
                break;
            case OS_LINUX:
                ostype = "Linux";
                break;
            default:
                break;
            }
        }
    }

    switch (event)
    {
    case SC_WORLD_LOADED:
    case SC_WORLD_UNLOADED:
    case SC_MAP_LOADED:
    case SC_MAP_UNLOADED:
        if (world && world->cur_savegame.save_dir.size())
        {
            std::string save_dir = "data/save/" + world->cur_savegame.save_dir;
            std::string evtlogpath = save_dir + "/events-dfhack.log";
            std::ofstream evtlog;
            evtlog.open(evtlogpath, std::ios_base::app);  // append
            if (evtlog.fail())
            {
                if (DFHack::Filesystem::isdir(save_dir))
                    out.printerr("Could not append to %s\n", evtlogpath.c_str());
            }
            else
            {
                char timebuf[30];
                time_t rawtime = time(NULL);
                struct tm * timeinfo = localtime(&rawtime);
                strftime(timebuf, sizeof(timebuf), "[%Y-%m-%dT%H:%M:%S%z] ", timeinfo);
                evtlog << timebuf;
                evtlog << "DFHack " << Version::git_description() << " on " << ostype << "; ";
                evtlog << "cwd md5: " << md5w.getHashFromString(getHackPath()).substr(0, 10) << "; ";
                evtlog << "save: " << world->cur_savegame.save_dir << "; ";
                evtlog << sc_event_name(event) << "; ";
                if (gametype)
                    evtlog << "game type " << ENUM_KEY_STR(game_type, *gametype) << " (" << *gametype << ")";
                else
                    evtlog << "game type unavailable";
                evtlog << std::endl;
            }
        }
    default:
        break;
    }

    if (event == SC_WORLD_LOADED && Version::is_prerelease())
    {
        runCommand(out, "gui/prerelease-warning");
        std::cerr << "loaded map in prerelease build" << std::endl;
    }

    EventManager::onStateChange(out, event);

    buildings_onStateChange(out, event);

    plug_mgr->OnStateChange(out, event);

    Lua::Core::onStateChange(out, event);

    handleLoadAndUnloadScripts(out, event);
}

// FIXME: needs to terminate the IO threads and properly dismantle all the machinery involved.
int Core::Shutdown ( void )
{
    if(errorstate)
        return true;
    errorstate = 1;
    CoreSuspendClaimer suspend;
    if(plug_mgr)
    {
        delete plug_mgr;
        plug_mgr = 0;
    }
    // invalidate all modules
    for(size_t i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));
    con.shutdown();
    return -1;
}

// FIXME: this is HORRIBLY broken
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
        if(df::global::ui && df::global::gview)
        {
            df::viewscreen * ws = Gui::getCurViewscreen();
            if (strict_virtual_cast<df::viewscreen_dwarfmodest>(ws) &&
                df::global::ui->main.mode != ui_sidebar_mode::Hotkeys &&
                df::global::ui->main.hotkeys[idx].cmd == df::ui_hotkey::T_cmd::None)
            {
                setHotkeyCmd(df::global::ui->main.hotkeys[idx].name);
                return false;
            }
            else
            {
                out = in;
                return true;
            }
        }
    }
    out = in;
    return true;
}

int UnicodeAwareSym(const SDL::KeyboardEvent& ke)
{
    // Assume keyboard layouts don't change the order of numbers:
    if( '0' <= ke.ksym.sym && ke.ksym.sym <= '9') return ke.ksym.sym;
    if(SDL::K_F1 <= ke.ksym.sym && ke.ksym.sym <= SDL::K_F12) return ke.ksym.sym;

    // These keys are mapped to the same control codes as Ctrl-?
    switch (ke.ksym.sym)
    {
        case SDL::K_RETURN:
        case SDL::K_KP_ENTER:
        case SDL::K_TAB:
        case SDL::K_ESCAPE:
        case SDL::K_DELETE:
            return ke.ksym.sym;
        default:
            break;
    }

    int unicode = ke.ksym.unicode;

    // convert Ctrl characters to their 0x40-0x5F counterparts:
    if (unicode < ' ')
    {
        unicode += 'A' - 1;
    }

    // convert A-Z to their a-z counterparts:
    if('A' <= unicode && unicode <= 'Z')
    {
        unicode += 'a' - 'A';
    }

    // convert various other punctuation marks:
    if('\"' == unicode) unicode = '\'';
    if('+' == unicode) unicode = '=';
    if(':' == unicode) unicode = ';';
    if('<' == unicode) unicode = ',';
    if('>' == unicode) unicode = '.';
    if('?' == unicode) unicode = '/';
    if('{' == unicode) unicode = '[';
    if('|' == unicode) unicode = '\\';
    if('}' == unicode) unicode = ']';
    if('~' == unicode) unicode = '`';

    return unicode;
}


//MEMO: return false if event is consumed
int Core::DFH_SDL_Event(SDL::Event* ev)
{
    //static bool alt = 0;

    // do NOT process events before we are ready.
    if(!started) return true;
    if(!ev)
        return true;
    if(ev && (ev->type == SDL::ET_KEYDOWN || ev->type == SDL::ET_KEYUP))
    {
        auto ke = (SDL::KeyboardEvent *)ev;

        if (ke->ksym.sym == SDL::K_LSHIFT || ke->ksym.sym == SDL::K_RSHIFT)
            modstate = (ev->type == SDL::ET_KEYDOWN) ? modstate | DFH_MOD_SHIFT : modstate & ~DFH_MOD_SHIFT;
        else if (ke->ksym.sym == SDL::K_LCTRL || ke->ksym.sym == SDL::K_RCTRL)
            modstate = (ev->type == SDL::ET_KEYDOWN) ? modstate | DFH_MOD_CTRL : modstate & ~DFH_MOD_CTRL;
        else if (ke->ksym.sym == SDL::K_LALT || ke->ksym.sym == SDL::K_RALT)
            modstate = (ev->type == SDL::ET_KEYDOWN) ? modstate | DFH_MOD_ALT : modstate & ~DFH_MOD_ALT;
        else if(ke->state == SDL::BTN_PRESSED && !hotkey_states[ke->ksym.sym])
        {
            hotkey_states[ke->ksym.sym] = true;

            // Use unicode so Windows gives the correct value for the
            // user's Input Language
            if(ke->ksym.unicode && ((ke->ksym.unicode & 0xff80) == 0))
            {
                int key = UnicodeAwareSym(*ke);
                SelectHotkey(key, modstate);
            }
            else
            {
                // Pretend non-ascii characters don't happen:
                SelectHotkey(ke->ksym.sym, modstate);
            }
        }
        else if(ke->state == SDL::BTN_RELEASED)
        {
            hotkey_states[ke->ksym.sym] = false;
        }
    }
    return true;
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

    if (sym == SDL::K_KP_ENTER)
        sym = SDL::K_RETURN;

    std::string cmd;

    {
        tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

        // Check the internal keybindings
        std::vector<KeyBinding> &bindings = key_bindings[sym];
        for (int i = bindings.size()-1; i >= 0; --i) {
            if (bindings[i].modifiers != modifiers)
                continue;
            if (!bindings[i].focus.empty() &&
                !prefix_matches(bindings[i].focus, Gui::getFocusString(screen)))
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
                    df::global::ui->main.mode != ui_sidebar_mode::Hotkeys &&
                    df::global::ui->main.hotkeys[idx].cmd == df::ui_hotkey::T_cmd::None)
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

static bool parseKeySpec(std::string keyspec, int *psym, int *pmod, std::string *pfocus)
{
    *pmod = 0;

    if (pfocus)
    {
        *pfocus = "";

        size_t idx = keyspec.find('@');
        if (idx != std::string::npos)
        {
            *pfocus = keyspec.substr(idx+1);
            keyspec = keyspec.substr(0, idx);
        }
    }

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
    } else if (keyspec.size() == 1 && keyspec[0] >= '0' && keyspec[0] <= '9') {
        *psym = SDL::K_0 + (keyspec[0]-'0');
        return true;
    } else if (keyspec.size() == 2 && keyspec[0] == 'F' && keyspec[1] >= '1' && keyspec[1] <= '9') {
        *psym = SDL::K_F1 + (keyspec[1]-'1');
        return true;
    } else if (keyspec.size() == 3 && keyspec.substr(0, 2) == "F1" && keyspec[2] >= '0' && keyspec[2] <= '2') {
        *psym = SDL::K_F10 + (keyspec[2]-'0');
        return true;
    } else if (keyspec == "Enter") {
        *psym = SDL::K_RETURN;
        return true;
    } else
        return false;
}

bool Core::ClearKeyBindings(std::string keyspec)
{
    int sym, mod;
    std::string focus;
    if (!parseKeySpec(keyspec, &sym, &mod, &focus))
        return false;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (bindings[i].modifiers == mod && prefix_matches(focus, bindings[i].focus))
            bindings.erase(bindings.begin()+i);
    }

    return true;
}

bool Core::AddKeyBinding(std::string keyspec, std::string cmdline)
{
    size_t at_pos = keyspec.find('@');
    if (at_pos != std::string::npos)
    {
        std::string raw_spec = keyspec.substr(0, at_pos);
        std::string raw_focus = keyspec.substr(at_pos + 1);
        if (raw_focus.find('|') != std::string::npos)
        {
            std::vector<std::string> focus_strings;
            split_string(&focus_strings, raw_focus, "|");
            for (size_t i = 0; i < focus_strings.size(); i++)
            {
                if (!AddKeyBinding(raw_spec + "@" + focus_strings[i], cmdline))
                    return false;
            }
            return true;
        }
    }
    int sym;
    KeyBinding binding;
    if (!parseKeySpec(keyspec, &sym, &binding.modifiers, &binding.focus))
        return false;

    cheap_tokenise(cmdline, binding.command);
    if (binding.command.empty())
        return false;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    // Don't add duplicates
    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (bindings[i].modifiers == binding.modifiers &&
            bindings[i].cmdline == cmdline &&
            bindings[i].focus == binding.focus)
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
    std::string focus;
    if (!parseKeySpec(keyspec, &sym, &mod, &focus))
        return rv;

    tthread::lock_guard<tthread::mutex> lock(*HotkeyMutex);

    std::vector<KeyBinding> &bindings = key_bindings[sym];
    for (int i = bindings.size()-1; i >= 0; --i) {
        if (focus.size() && focus != bindings[i].focus)
            continue;
        if (bindings[i].modifiers == mod)
        {
            std::string cmd = bindings[i].cmdline;
            if (!bindings[i].focus.empty())
                cmd = "@" + bindings[i].focus + ": " + cmd;
            rv.push_back(cmd);
        }
    }

    return rv;
}


/////////////////
// ClassNameCheck
/////////////////

// Since there is no Process.cpp, put ClassNameCheck stuff in Core.cpp

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

MemoryPatcher::MemoryPatcher(Process *p_) : p(p_)
{
    if (!p)
        p = Core::getInstance().p;
}

MemoryPatcher::~MemoryPatcher()
{
    close();
}

bool MemoryPatcher::verifyAccess(void *target, size_t count, bool write)
{
    uint8_t *sptr = (uint8_t*)target;
    uint8_t *eptr = sptr + count;

    // Find the valid memory ranges
    if (ranges.empty())
        p->getMemRanges(ranges);

    // Find the ranges that this area spans
    unsigned start = 0;
    while (start < ranges.size() && ranges[start].end <= sptr)
        start++;
    if (start >= ranges.size() || ranges[start].start > sptr)
        return false;

    unsigned end = start+1;
    while (end < ranges.size() && ranges[end].start < eptr)
    {
        if (ranges[end].start != ranges[end-1].end)
            return false;
        end++;
    }
    if (ranges[end-1].end < eptr)
        return false;

    // Verify current permissions
    for (unsigned i = start; i < end; i++)
        if (!ranges[i].valid || !(ranges[i].read || ranges[i].execute) || ranges[i].shared)
            return false;

    // Apply writable permissions & update
    for (unsigned i = start; i < end; i++)
    {
        auto &perms = ranges[i];
        if ((perms.write || !write) && perms.read)
            continue;

        save.push_back(perms);
        perms.write = perms.read = true;
        if (!p->setPermisions(perms, perms))
            return false;
    }

    return true;
}

bool MemoryPatcher::write(void *target, const void *src, size_t size)
{
    if (!makeWritable(target, size))
        return false;

    memmove(target, src, size);
    return true;
}

void MemoryPatcher::close()
{
    for (size_t i  = 0; i < save.size(); i++)
        p->setPermisions(save[i], save[i]);

    save.clear();
    ranges.clear();
};


bool Process::patchMemory(void *target, const void* src, size_t count)
{
    MemoryPatcher patcher(this);

    return patcher.write(target, src, count);
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

MODULE_GETTER(Materials);
MODULE_GETTER(Notes);
MODULE_GETTER(Graphic);
