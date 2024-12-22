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

#include "Error.h"
#include "MemAccess.h"
#include "Core.h"
#include "DataDefs.h"
#include "Debug.h"
#include "Console.h"
#include "MiscUtils.h"
#include "Module.h"
#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "PluginManager.h"
#include "ModuleFactory.h"
#include "RemoteServer.h"
#include "RemoteTools.h"
#include "LuaTools.h"
#include "DFHackVersion.h"
#include "md5wrapper.h"

#include "modules/DFSDL.h"
#include "modules/DFSteam.h"
#include "modules/EventManager.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Textures.h"
#include "modules/World.h"
#include "modules/Persistence.h"

#include "df/init.h"
#include "df/gamest.h"
#include "df/graphic.h"
#include "df/interfacest.h"
#include "df/plotinfost.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_export_regionst.h"
#include "df/viewscreen_game_cleanerst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_savegamest.h"
#include "df/world.h"
#include "df/world_data.h"

#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
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
#include <SDL_events.h>

#ifdef LINUX_BUILD
#include <dlfcn.h>
#endif

using namespace DFHack;
using namespace df::enums;
using df::global::init;
using df::global::world;
using std::string;

// FIXME: A lot of code in one file, all doing different things... there's something fishy about it.

static bool parseKeySpec(std::string keyspec, int *psym, int *pmod, std::string *pfocus = NULL);
size_t loadScriptFiles(Core* core, color_ostream& out, const std::vector<std::string>& prefix, const std::string& folder);

namespace DFHack {

DBG_DECLARE(core,keybinding,DebugCategory::LINFO);
DBG_DECLARE(core,script,DebugCategory::LINFO);

static const std::string CONFIG_PATH = "dfhack-config/";
static const std::string CONFIG_DEFAULTS_PATH = "hack/data/dfhack-config-defaults/";

class MainThread {
public:
    //! MainThread::suspend keeps the main DF thread suspended from Core::Init to
    //! thread exit.
    static CoreSuspenderBase& suspend() {
        static thread_local CoreSuspenderBase lock{};
        return lock;
    }
};
}

struct Core::Private
{
    std::thread iothread;
    std::thread hotkeythread;

    bool last_autosave_request{false};
    bool last_manual_save_request{false};
    bool was_load_save{false};
};

void PerfCounters::reset(bool ignorePauseState) {
    *this = {};
    ignore_pause_state = ignorePauseState;
    baseline_elapsed_ms = Core::getInstance().p->getTickCount();
}

void PerfCounters::incCounter(uint32_t &counter, uint32_t baseline_ms) {
    if (!ignore_pause_state && (!World::isFortressMode() || World::ReadPauseState()))
        return;
    counter += Core::getInstance().p->getTickCount() - baseline_ms;
}

bool PerfCounters::getIgnorePauseState() {
    return ignore_pause_state;
}

void PerfCounters::registerTick(uint32_t baseline_ms) {
    if (!World::isFortressMode() || World::ReadPauseState()) {
        last_tick_baseline_ms = 0;
        return;
    }

    // only update when the tick counter has advanced
    if (!world || last_frame_counter == world->frame_counter)
        return;
    last_frame_counter = world->frame_counter;

    if (last_tick_baseline_ms == 0) {
        last_tick_baseline_ms = baseline_ms;
        return;
    }

    uint32_t elapsed_ms = baseline_ms - last_tick_baseline_ms;
    last_tick_baseline_ms = baseline_ms;

    recent_ticks.head_idx = (recent_ticks.head_idx + 1) % RECENT_TICKS_HISTORY_SIZE;

    if (recent_ticks.full)
        recent_ticks.sum_ms -= recent_ticks.history[recent_ticks.head_idx];
    else if (recent_ticks.head_idx == 0)
        recent_ticks.full = true;

    recent_ticks.history[recent_ticks.head_idx] = elapsed_ms;
    recent_ticks.sum_ms += elapsed_ms;
}

uint32_t PerfCounters::getUnpausedFps() {
    uint32_t seconds = recent_ticks.sum_ms / 1000;
    if (seconds == 0)
        return 0;
    size_t num_frames = recent_ticks.full ? RECENT_TICKS_HISTORY_SIZE : recent_ticks.head_idx;
    return num_frames / seconds;
}

struct CommandDepthCounter
{
    static const int MAX_DEPTH = 20;
    static thread_local int depth;
    CommandDepthCounter() { depth++; }
    ~CommandDepthCounter() { depth--; }
    bool ok() { return depth < MAX_DEPTH; }
};
thread_local int CommandDepthCounter::depth = 0;

void Core::cheap_tokenise(std::string const& input, std::vector<std::string>& output)
{
    std::string *cur = NULL;
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
        std::cerr << "Hotkey thread has croaked." << std::endl;
        return;
    }
    bool keep_going = true;
    while(keep_going)
    {
        std::string stuff = core->getHotkeyCmd(keep_going); // waits on mutex!
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
    std::string name;
    std::string description;
    //FIXME: Nuke when MSVC stops failing at being C++11 compliant
    sortable(bool recolor_,const std::string& name_,const std::string & description_): recolor(recolor_), name(name_), description(description_){};
    bool operator <(const sortable & rhs) const
    {
        if( name < rhs.name )
            return true;
        return false;
    };
};

static std::string dfhack_version_desc()
{
    std::stringstream s;
    s << Version::dfhack_version() << " ";
    if (Version::is_release())
        s << "(release)";
    else
        s << "(git: " << Version::git_commit(true) << ")";
    s << " on " << (sizeof(void*) == 8 ? "x86_64" : "x86");
    if (strlen(Version::dfhack_build_id()))
        s << " [build ID: " << Version::dfhack_build_id() << "]";
    return s.str();
}

namespace {
    struct ScriptArgs {
        const std::string *pcmd;
        std::vector<std::string> *pargs;
    };
    struct ScriptEnableState {
        const std::string *pcmd;
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

static command_result runLuaScript(color_ostream &out, std::string name, std::vector<std::string> &args)
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

command_result Core::runCommand(color_ostream &out, const std::string &command)
{
    if (!command.empty())
    {
        std::vector <std::string> parts;
        Core::cheap_tokenise(command,parts);
        if(parts.size() == 0)
            return CR_NOT_IMPLEMENTED;

        std::string first = parts[0];
        parts.erase(parts.begin());

        if (first[0] == '#')
            return CR_OK;

        std::cerr << "Invoking: " << command << std::endl;
        return runCommand(out, first, parts);
    }
    else
        return CR_NOT_IMPLEMENTED;
}

bool is_builtin(color_ostream &con, const std::string &command) {
    CoreSuspender suspend;
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 1) ||
        !Lua::PushModulePublic(con, L, "helpdb", "is_builtin")) {
        con.printerr("Failed to load helpdb Lua code\n");
        return false;
    }

    Lua::Push(L, command);

    if (!Lua::SafeCall(con, L, 1, 1)) {
        con.printerr("Failed Lua call to helpdb.is_builtin.\n");
        return false;
    }

    return lua_toboolean(L, -1);
}

void get_commands(color_ostream &con, std::vector<std::string> &commands) {
    ConditionalCoreSuspender suspend{};

    if (!suspend) {
        con.printerr("Cannot acquire core lock in helpdb.get_commands\n");
        commands.clear();
        return;
    }

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 1) ||
        !Lua::PushModulePublic(con, L, "helpdb", "get_commands")) {
        con.printerr("Failed to load helpdb Lua code\n");
        return;
    }

    if (!Lua::SafeCall(con, L, 0, 1)) {
        con.printerr("Failed Lua call to helpdb.get_commands.\n");
    }

    Lua::GetVector(L, commands, top + 1);
}

static bool try_autocomplete(color_ostream &con, const std::string &first, std::string &completed)
{
    std::vector<std::string> commands, possible;

    get_commands(con, commands);
    for (auto &command : commands)
        if (command.substr(0, first.size()) == first)
            possible.push_back(command);

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

bool Core::addScriptPath(std::string path, bool search_before)
{
    std::lock_guard<std::mutex> lock(script_path_mutex);
    std::vector<std::string> &vec = script_paths[search_before ? 0 : 1];
    if (std::find(vec.begin(), vec.end(), path) != vec.end())
        return false;
    if (!Filesystem::isdir(path))
        return false;
    vec.push_back(path);
    return true;
}

bool Core::setModScriptPaths(const std::vector<std::string> &mod_script_paths) {
    std::lock_guard<std::mutex> lock(script_path_mutex);
    script_paths[2] = mod_script_paths;
    return true;
}

bool Core::removeScriptPath(std::string path)
{
    std::lock_guard<std::mutex> lock(script_path_mutex);
    bool found = false;
    for (int i = 0; i < 2; i++)
    {
        std::vector<std::string> &vec = script_paths[i];
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
    std::lock_guard<std::mutex> lock(script_path_mutex);
    dest->clear();
    std::string df_path = this->p->getPath() + "/";
    for (auto & path : script_paths[0])
        dest->emplace_back(path);
    dest->push_back(df_path + CONFIG_PATH + "scripts");
    if (df::global::world && isWorldLoaded()) {
        std::string save = World::ReadWorldFolder();
        if (save.size())
            dest->emplace_back(df_path + "save/" + save + "/scripts");
    }
    dest->emplace_back(df_path + "hack/scripts");
    for (auto & path : script_paths[2])
        dest->emplace_back(path);
    for (auto & path : script_paths[1])
        dest->emplace_back(path);
}

std::string Core::findScript(std::string name)
{
    std::vector<std::string> paths;
    getScriptPaths(&paths);
    for (auto it = paths.begin(); it != paths.end(); ++it)
    {
        std::string path = *it + "/" + name;
        if (Filesystem::isfile(path))
            return path;
    }
    return "";
}

bool loadScriptPaths(color_ostream &out, bool silent = false)
{
    using namespace std;
    std::string filename(CONFIG_PATH + "script-paths.txt");
    ifstream file(filename);
    if (!file)
    {
        if (!silent)
            out.printerr("Could not load %s\n", filename.c_str());
        return false;
    }
    std::string raw;
    int line = 0;
    while (getline(file, raw))
    {
        ++line;
        istringstream ss(raw);
        char ch;
        ss >> skipws;
        if (!(ss >> ch) || ch == '#')
            continue;
        ss >> ws; // discard whitespace
        std::string path;
        getline(ss, path);
        if (ch == '+' || ch == '-')
        {
            if (!Core::getInstance().addScriptPath(path, ch == '+') && !silent)
                out.printerr("%s:%i: Failed to add path: %s\n", filename.c_str(), line, path.c_str());
        }
        else if (!silent)
            out.printerr("%s:%i: Illegal character: %c\n", filename.c_str(), line, ch);
    }
    return true;
}

static void loadModScriptPaths(color_ostream &out) {
    std::vector<std::string> mod_script_paths;
    Lua::CallLuaModuleFunction(out, "script-manager", "get_mod_script_paths", {}, 1,
            [&](lua_State *L) {
                Lua::GetVector(L, mod_script_paths);
            });
    DEBUG(script,out).print("final mod script paths:\n");
    for (auto & path : mod_script_paths)
        DEBUG(script,out).print("  %s\n", path.c_str());
    Core::getInstance().setModScriptPaths(mod_script_paths);
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

void help_helper(color_ostream &con, const std::string &entry_name) {
    ConditionalCoreSuspender suspend{};

    if (!suspend) {
        con.printerr("Failed Lua call to helpdb.help (could not acquire core lock).\n");
        return;
    }
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 2) ||
        !Lua::PushModulePublic(con, L, "helpdb", "help")) {
        con.printerr("Failed to load helpdb Lua code\n");
        return;
    }

    Lua::Push(L, entry_name);

    if (!Lua::SafeCall(con, L, 1, 0)) {
        con.printerr("Failed Lua call to helpdb.help.\n");
    }
}

void tags_helper(color_ostream &con, const std::string &tag) {
    CoreSuspender suspend;
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 1) ||
        !Lua::PushModulePublic(con, L, "helpdb", "tags")) {
        con.printerr("Failed to load helpdb Lua code\n");
        return;
    }

    Lua::Push(L, tag);

    if (!Lua::SafeCall(con, L, 1, 0)) {
        con.printerr("Failed Lua call to helpdb.tags.\n");
    }
}

void ls_helper(color_ostream &con, const std::vector<std::string> &params) {
    std::vector<std::string> filter;
    bool skip_tags = false;
    bool show_dev_commands = false;
    std::string exclude_strs = "";

    bool in_exclude = false;
    for (auto str : params) {
        if (in_exclude)
            exclude_strs = str;
        else if (str == "--notags")
            skip_tags = true;
        else if (str == "--dev")
            show_dev_commands = true;
        else if (str == "--exclude")
            in_exclude = true;
        else
            filter.push_back(str);
    }

    CoreSuspender suspend;
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 5) ||
        !Lua::PushModulePublic(con, L, "helpdb", "ls")) {
        con.printerr("Failed to load helpdb Lua code\n");
        return;
    }

    Lua::PushVector(L, filter);
    Lua::Push(L, skip_tags);
    Lua::Push(L, show_dev_commands);
    Lua::Push(L, exclude_strs);

    if (!Lua::SafeCall(con, L, 4, 0)) {
        con.printerr("Failed Lua call to helpdb.ls.\n");
    }
}

command_result Core::runCommand(color_ostream &con, const std::string &first_, std::vector<std::string> &parts)
{
    std::string first = first_;
    CommandDepthCounter counter;
    if (!counter.ok())
    {
        con.printerr("Cannot invoke \"%s\": maximum command depth exceeded (%i)\n",
            first.c_str(), CommandDepthCounter::MAX_DEPTH);
        return CR_FAILURE;
    }

    if (first.empty())
        return CR_NOT_IMPLEMENTED;

    if (first.find('\\') != std::string::npos)
    {
        con.printerr("Replacing backslashes with forward slashes in \"%s\"\n", first.c_str());
        for (size_t i = 0; i < first.size(); i++)
        {
            if (first[i] == '\\')
                first[i] = '/';
        }
    }

    // let's see what we actually got
    command_result res;
    if (first == "help" || first == "man" || first == "?")
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
            con.print("Here are some basic commands to get you started:\n"
                      "  help|?|man         - This text.\n"
                      "  help <tool>        - Usage help for the given plugin, command, or script.\n"
                      "  tags               - List the tags that the DFHack tools are grouped by.\n"
                      "  ls|dir [<filter>]  - List commands, optionally filtered by a tag or substring.\n"
                      "                       Optional parameters:\n"
                      "                         --notags: skip printing tags for each command.\n"
                      "                         --dev:  include commands intended for developers and modders.\n"
                      "  cls|clear          - Clear the console.\n"
                      "  fpause             - Force DF to pause.\n"
                      "  die                - Force DF to close immediately, without saving.\n"
                      "  keybinding         - Modify bindings of commands to in-game key shortcuts.\n"
                      "\n"
                      "See more commands by running 'ls'.\n\n"
                     );

            con.print("DFHack version %s\n", dfhack_version_desc().c_str());
        }
        else
        {
            help_helper(con, parts[0]);
        }
    }
    else if (first == "tags")
    {
        tags_helper(con, parts.size() ? parts[0] : "");
    }
    else if (first == "load" || first == "unload" || first == "reload")
    {
        bool all = false;
        bool load = (first == "load");
        bool unload = (first == "unload");
        if (parts.size())
        {
            for (auto p = parts.begin(); p != parts.end(); p++)
            {
                if (p->size() && (*p)[0] == '-')
                {
                    if (p->find('a') != std::string::npos)
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
            con.printerr("%s: no arguments\n", first.c_str());
    }
    else if( first == "enable" || first == "disable" )
    {
        CoreSuspender suspend;
        bool enable = (first == "enable");

        if(parts.size())
        {
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

                part = GetAliasCommand(part, true);

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
                    con.printerr("Cannot %s plugin: %s\n", first.c_str(), part.c_str());
                }
                else
                {
                    res = plug->set_enabled(con, enable);

                    if (res != CR_OK || plug->is_enabled() != enable)
                        con.printerr("Could not %s plugin: %s\n", first.c_str(), part.c_str());
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
                    "%21s  %-3s%s\n",
                    (plug->getName()+":").c_str(),
                    plug->is_enabled() ? "on" : "off",
                    plug->can_set_enabled() ? "" : " (controlled internally)"
                );
            }

            Lua::CallLuaModuleFunction(con, "script-manager", "list");
        }
    }
    else if (first == "ls" || first == "dir")
    {
        ls_helper(con, parts);
    }
    else if (first == "plug")
    {
        const char *header_format = "%30s %10s %4s %8s\n";
        const char *row_format =    "%30s %10s %4i %8s\n";
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
    else if (first == "type")
    {
        if (!parts.size())
        {
            con.printerr("type: no argument\n");
            return CR_WRONG_USAGE;
        }
        con << parts[0];
        bool builtin = is_builtin(con, parts[0]);
        std::string lua_path = findScript(parts[0] + ".lua");
        Plugin *plug = plug_mgr->getPluginByCommand(parts[0]);
        if (builtin)
        {
            con << " is a built-in command";
            con << std::endl;
        }
        else if (IsAlias(parts[0]))
        {
            con << " is an alias: " << GetAliasCommand(parts[0]) << std::endl;
        }
        else if (plug)
        {
            con << " is a command implemented by the plugin " << plug->getName() << std::endl;
        }
        else if (lua_path.size())
        {
            con << " is a Lua script: " << lua_path << std::endl;
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
    else if (first == "keybinding")
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
                con << "No bindings." << std::endl;
            for (size_t i = 0; i < list.size(); i++)
                con << "  " << list[i] << std::endl;
        }
        else
        {
            con << "Usage:" << std::endl
                << "  keybinding list <key>" << std::endl
                << "  keybinding clear <key>[@context]..." << std::endl
                << "  keybinding set <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
                << "  keybinding add <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
                << "Later adds, and earlier items within one command have priority." << std::endl
                << "Supported keys: [Ctrl-][Alt-][Shift-](A-Z, 0-9, F1-F12, `, or Enter)." << std::endl
                << "Context may be used to limit the scope of the binding, by" << std::endl
                << "requiring the current context to have a certain prefix." << std::endl
                << "Current UI context is: " << std::endl
                << join_strings("\n", Gui::getCurFocus(true)) << std::endl;
        }
    }
    else if (first == "alias")
    {
        if (parts.size() >= 3 && (parts[0] == "add" || parts[0] == "replace"))
        {
            const std::string &name = parts[1];
            std::vector<std::string> cmd(parts.begin() + 2, parts.end());
            if (!AddAlias(name, cmd, parts[0] == "replace"))
            {
                con.printerr("Could not add alias %s - already exists\n", name.c_str());
                return CR_FAILURE;
            }
        }
        else if (parts.size() >= 2 && (parts[0] == "delete" || parts[0] == "clear"))
        {
            if (!RemoveAlias(parts[1]))
            {
                con.printerr("Could not remove alias %s\n", parts[1].c_str());
                return CR_FAILURE;
            }
        }
        else if (parts.size() >= 1 && (parts[0] == "list"))
        {
            auto aliases = ListAliases();
            for (auto p : aliases)
            {
                con << p.first << ": " << join_strings(" ", p.second) << std::endl;
            }
        }
        else
        {
            con << "Usage: " << std::endl
                << "  alias add|replace <name> <command...>" << std::endl
                << "  alias delete|clear <name> <command...>" << std::endl
                << "  alias list" << std::endl;
        }
    }
    else if (first == "fpause")
    {
        World::SetPauseState(true);
/* TODO: understand how this changes for v50
        if (auto scr = Gui::getViewscreenByType<df::viewscreen_new_regionst>())
        {
            scr->worldgen_paused = true;
        }
*/
        con.print("The game was forced to pause!\n");
    }
    else if (first == "cls" || first == "clear")
    {
        if (con.is_console())
            ((Console&)con).clear();
        else
        {
            con.printerr("No console to clear.\n");
            return CR_NEEDS_CONSOLE;
        }
    }
    else if (first == "die")
    {
#ifdef WIN32
        TerminateProcess(GetCurrentProcess(),666);
#else
        std::_Exit(666);
#endif
    }
    else if (first == "kill-lua")
    {
        bool force = false;
        for (auto it = parts.begin(); it != parts.end(); ++it)
        {
            if (*it == "force")
                force = true;
        }
        if (!Lua::Interrupt(force))
        {
            con.printerr(
                "Failed to register hook. This can happen if you have"
                " lua profiling or coverage monitoring enabled. Use"
                " 'kill-lua force' to force, but this may disable"
                " profiling and coverage monitoring.\n");
        }
    }
    else if (first == "script")
    {
        if(parts.size() == 1)
        {
            loadScriptFile(con, parts[0], false);
        }
        else
        {
            con << "Usage:" << std::endl
                << "  script <filename>" << std::endl;
            return CR_WRONG_USAGE;
        }
    }
    else if (first == "hide")
    {
        if (!getConsole().hide())
        {
            con.printerr("Could not hide console\n");
            return CR_FAILURE;
        }
        return CR_OK;
    }
    else if (first == "show")
    {
        if (!getConsole().show())
        {
            con.printerr("Could not show console\n");
            return CR_FAILURE;
        }
        return CR_OK;
    }
    else if (first == "sc-script")
    {
        if (parts.empty() || parts[0] == "help" || parts[0] == "?")
        {
            con << "Usage: sc-script add|remove|list|help SC_EVENT [path-to-script] [...]" << std::endl;
            con << "Valid event names (SC_ prefix is optional):" << std::endl;
            for (int i = SC_WORLD_LOADED; i <= SC_UNPAUSED; i++)
            {
                std::string name = sc_event_name((state_change_event)i);
                if (name != "SC_UNKNOWN")
                    con << "  " << name << std::endl;
            }
            return CR_OK;
        }
        else if (parts[0] == "list")
        {
            if(parts.size() < 2)
                parts.push_back("");
            if (parts[1].size() && sc_event_id(parts[1]) == SC_UNKNOWN)
            {
                con << "Unrecognized event name: " << parts[1] << std::endl;
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
                con << "Usage: sc-script add EVENT path-to-script [-save]" << std::endl;
                return CR_WRONG_USAGE;
            }
            state_change_event evt = sc_event_id(parts[1]);
            if (evt == SC_UNKNOWN)
            {
                con << "Unrecognized event: " << parts[1] << std::endl;
                return CR_FAILURE;
            }
            bool save_specific = (parts.size() >= 4 && parts[3] == "-save");
            StateChangeScript script(evt, parts[2], save_specific);
            for (auto it = state_change_scripts.begin(); it != state_change_scripts.end(); ++it)
            {
                if (script == *it)
                {
                    con << "Script already registered" << std::endl;
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
                con << "Usage: sc-script remove EVENT path-to-script [-save]" << std::endl;
                return CR_WRONG_USAGE;
            }
            state_change_event evt = sc_event_id(parts[1]);
            if (evt == SC_UNKNOWN)
            {
                con << "Unrecognized event: " << parts[1] << std::endl;
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
                con << "Unrecognized script" << std::endl;
                return CR_FAILURE;
            }
        }
        else
        {
            con << "Usage: sc-script add|remove|list|help SC_EVENT [path-to-script] [...]" << std::endl;
            return CR_WRONG_USAGE;
        }
    }
    else if (first == "devel/dump-rpc")
    {
        if (parts.size() == 1)
        {
            std::ofstream file(parts[0]);
            CoreService core;
            core.dumpMethods(file);

            for (auto & it : *plug_mgr)
            {
                Plugin * plug = it.second;
                if (!plug)
                    continue;

                std::unique_ptr<RPCService> svc(plug->rpc_connect(con));
                if (!svc)
                    continue;

                file << "// Plugin: " << plug->getName() << std::endl;
                svc->dumpMethods(file);
            }
        }
        else
        {
            con << "Usage: devel/dump-rpc \"filename\"" << std::endl;
            return CR_WRONG_USAGE;
        }
    }
    else if (RunAlias(con, first, parts, res))
    {
        return res;
    }
    else
    {
        res = plug_mgr->InvokeCommand(con, first, parts);
        if (res == CR_WRONG_USAGE)
        {
            help_helper(con, first);
        }
        else if (res == CR_NOT_IMPLEMENTED)
        {
            std::string completed;
            std::string filename = findScript(first + ".lua");
            bool lua = filename != "";
            if ( !lua ) {
                filename = findScript(first + ".rb");
            }
            if ( lua )
                res = runLuaScript(con, first, parts);
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
                        con.printerr("that implements %zi commands - see \"help %s\" for details\n",
                            p->size(), first.c_str());
                    else
                        con.printerr("but does not implement any commands\n");
                }
            }
        }
        else if (res == CR_NEEDS_CONSOLE)
            con.printerr("%s needs an interactive console to work.\n"
                          "Please run this command from the DFHack console.\n\n"
#ifdef WIN32
                          "You can show the console with the 'show' command."
#else
                          "The console is accessible when you run DF from the commandline\n"
                          "via the './dfhack' script."
#endif
                          "\n", first.c_str());
        return res;
    }

    return CR_OK;
}

bool Core::loadScriptFile(color_ostream &out, std::string fname, bool silent)
{
    if(!silent) {
        INFO(script,out) << "Running script: " << fname << std::endl;
        std::cerr << "Running script: " << fname << std::endl;
    }
    std::ifstream script(fname.c_str());
    if ( !script.good() )
    {
        if(!silent)
            out.printerr("Error loading script: %s\n", fname.c_str());
        return false;
    }
    std::string command;
    while(script.good()) {
        std::string temp;
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
    CoreSuspender lock;
    if (!df::global::world || !df::global::plotinfo || !df::global::gview)
    {
        out.printerr("Key globals are missing, skipping loading dfhack.init.\n");
        return;
    }

    // load baseline defaults
    core->loadScriptFile(out, CONFIG_PATH + "init/default.dfhack.init", false);

    // load user overrides
    std::vector<std::string> prefixes(1, "dfhack");
    loadScriptFiles(core, out, prefixes, CONFIG_PATH + "init");

    // show the terminal if requested
    auto L = Lua::Core::State;
    Lua::CallLuaModuleFunction(out, L, "dfhack", "getHideConsoleOnStartup", 0, 1,
        Lua::DEFAULT_LUA_LAMBDA, [&](lua_State* L) {
            if (!lua_toboolean(L, -1))
                core->getConsole().show();
        }, false);
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
    static const std::string HISTORY_FILE = CONFIG_PATH + "dfhack.history";

    IODATA * iod = ((IODATA*) iodata);
    Core * core = iod->core;
    PluginManager * plug_mgr = ((IODATA*) iodata)->plug_mgr;

    CommandHistory main_history;
    main_history.load(HISTORY_FILE.c_str());

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

    if (getenv("DFHACK_DISABLE_CONSOLE"))
        return;

    while (true)
    {
        std::string command = "";
        int ret;
        while ((ret = con.lineedit("[DFHack]# ",command, main_history))
                == Console::RETRY);
        if(ret == Console::SHUTDOWN)
        {
            std::cerr << "Console is shutting down properly." << std::endl;
            return;
        }
        else if(ret == Console::FAILURE)
        {
            std::cerr << "Console caught an unspecified error." << std::endl;
            continue;
        }
        else if(ret)
        {
            // a proper, non-empty command was entered
            main_history.add(command);
            main_history.save(HISTORY_FILE.c_str());
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

Core::~Core()
{
    // we leak the memory in case ~Core is called after _exit
}

Core::Core() :
    d(std::make_unique<Private>()),
    script_path_mutex{},
    HotkeyMutex{},
    HotkeyCond{},
    alias_mutex{},
    started{false},
    misc_data_mutex{},
    CoreSuspendMutex{},
    CoreWakeup{},
    ownerThread{},
    toolCount{0}
{
    // init the console. This must be always the first step!
    plug_mgr = 0;
    errorstate = false;
    vinfo = 0;
    memset(&(s_mods), 0, sizeof(s_mods));

    // set up hotkey capture
    suppress_duplicate_keyboard_events = true;
    hotkey_set = NO;
    last_world_data_ptr = NULL;
    last_local_map_ptr = NULL;
    last_pause_state = false;
    top_viewscreen = NULL;

    color_ostream::log_errors_to_stderr = true;
};

void Core::fatal (std::string output, const char * title)
{
    errorstate = true;
    std::stringstream out;
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
    out << "Check file stderr.log for details.\n";
    std::cout << "DFHack fatal error: " << out.str() << std::endl;
    if (!title)
        title = "DFHack error!";
    DFSDL::DFSDL_ShowSimpleMessageBox(0x10 /* SDL_MESSAGEBOX_ERROR */, title, out.str().c_str(), NULL);

    bool is_headless = bool(getenv("DFHACK_HEADLESS"));
    if (is_headless)
    {
        exit('f');
    }
}

std::string Core::getHackPath()
{
#ifdef LINUX_BUILD
    return p->getPath() + "/hack/";
#else
    return p->getPath() + "\\hack\\";
#endif
}

df::viewscreen * Core::getTopViewscreen() {
    return getInstance().top_viewscreen;
}

bool Core::InitMainThread() {
    df_render_thread = std::this_thread::get_id();

    Filesystem::init();

    // Re-route stdout and stderr again - DF seems to set up stdout and
    // stderr.txt on Windows as of 0.43.05. Also, log before switching files to
    // make it obvious what's going on if someone checks the *.txt files.
    #ifndef LINUX_BUILD
        // Don't do this on Linux because it will break PRINT_MODE:TEXT
        // this is handled as appropriate in Console-posix.cpp
        fprintf(stdout, "dfhack: redirecting stdout to stdout.log (again)\n");
        if (!freopen("stdout.log", "w", stdout))
            std::cerr << "Could not redirect stdout to stdout.log" << std::endl;
    #endif
    fprintf(stderr, "dfhack: redirecting stderr to stderr.log\n");
    if (!freopen("stderr.log", "w", stderr))
        std::cerr << "Could not redirect stderr to stderr.log" << std::endl;

    std::cerr << "DFHack build: " << Version::git_description() << "\n"
         << "Starting with working directory: " << Filesystem::getcwd() << std::endl;

    std::cerr << "Binding to SDL.\n";
    if (!DFSDL::init(con)) {
        fatal("cannot bind SDL libraries");
        return false;
    }

    // find out what we are...
    #ifdef LINUX_BUILD
        const char * path = "hack/symbols.xml";
    #else
        const char * path = "hack\\symbols.xml";
    #endif
    auto local_vif = std::make_unique<DFHack::VersionInfoFactory>();
    std::cerr << "Identifying DF version.\n";
    try
    {
        local_vif->loadFile(path);
    }
    catch(Error::All & err)
    {
        std::stringstream out;
        out << "Error while reading symbols.xml:\n";
        out << err.what() << std::endl;
        errorstate = true;
        fatal(out.str());
        return false;
    }
    vif = std::move(local_vif);
    auto local_p = std::make_unique<DFHack::Process>(*vif);
    local_p->ValidateDescriptionOS();
    vinfo = local_p->getDescriptor();

    if(!vinfo || !local_p->isIdentified())
    {
        if (!Version::git_xml_match())
        {
            const char *msg = (
                "*******************************************************\n"
                "*               BIG, UGLY ERROR MESSAGE               *\n"
                "*******************************************************\n"
                "\n"
                "This DF version is missing from hack/symbols.xml, and\n"
                "you have compiled DFHack with a df-structures (xml)\n"
                "version that does *not* match the version tracked in git.\n"
                "\n"
                "If you are not actively working on df-structures and you\n"
                "expected DFHack to work, you probably forgot to run\n"
                "\n"
                "    git submodule update\n"
                "\n"
                "If this does not sound familiar, read Compile.rst and \n"
                "recompile.\n"
                "More details can be found in stderr.log in this folder.\n"
            );
            std::cout << msg << std::endl;
            std::cerr << msg << std::endl;
            fatal("Not a known DF version - XML version mismatch (see console or stderr.log)");
        }
        else
        {
            std::stringstream msg;
            msg << "Not a supported DF version.\n"
                   "\n"
                   "Please make sure that you have a version of DFHack installed that\n"
                   "matches the version of Dwarf Fortress.\n"
                   "\n"
                   "DFHack version: " << Version::dfhack_version() << "\n"
                   "\n";
            auto supported_versions = vif->getVersionInfosForCurOs();
            if (supported_versions.size()) {
                msg << "Dwarf Fortress releases supported by this version of DFHack:\n\n";
                for (auto & sv : supported_versions) {
                    string ver = sv->getVersion();
                    if (ver.starts_with("v0.")) {  // translate "v0.50" to the standard format: "v50"
                        ver = "v" + ver.substr(3);
                    }
                    msg << "    " << ver << "\n";
                }
                msg << "\n";
            }
            fatal(msg.str(), "DFHack version mismatch");
        }
        errorstate = true;
        return false;
    }
    std::cerr << "Version: " << vinfo->getVersion() << std::endl;
    p = std::move(local_p);

    // Init global object pointers
    df::global::InitGlobals();

    // check key game structure sizes against the global table
    // this check is (silently) skipped if either `game` or `global_table` is not defined
    // to faciliate the linux symbol discovery process (which runs without any symbols)
    // or if --skip-size-check is discovered on the command line

    if (!df::global::global_table || !df::global::game ||
        df::global::game->command_line.original.find("--skip-size-check") != std::string::npos)
    {
        std::cerr << "Skipping structure size verification check." << std::endl;
    } else {
        std::stringstream msg;
        bool gt_error = false;
        static const std::map<const std::string, const size_t> sizechecks{
            { "world", sizeof(df::world) },
            { "game", sizeof(df::gamest) },
            { "plotinfo", sizeof(df::plotinfost) },
        };

        for (auto& gte : *df::global::global_table)
        {
            // this will exit the loop when the terminator is hit, in the event the global table size in structures is incorrect
            if (gte.address == nullptr || gte.name == nullptr)
                break;
            std::string name{ gte.name };
            if (sizechecks.contains(name) && gte.size != sizechecks.at(name))
            {
                msg << "Global '" << name << "' size mismatch: is " << gte.size << ", expected " << sizechecks.at(name) << "\n";
                gt_error = true;
            }
        }

        if (gt_error)
        {
            msg << "DFHack cannot safely run under these conditions.\n";
            fatal(msg.str(), "DFHack fatal error");
            errorstate = true;
            return false;
        }

        std::cerr << "Structure size verification check passed." << std::endl;
    }

    perf_counters.reset();

    return true;
}

bool Core::InitSimulationThread()
{
    df_simulation_thread = std::this_thread::get_id();
    if(started)
        return true;
    if(errorstate)
        return false;

    // Lock the CoreSuspendMutex until the thread exits or call Core::Shutdown
    // Core::Update will temporary unlock when there is any commands queued
    MainThread::suspend().lock();

    std::cerr << "Initializing Console.\n";
    // init the console.
    bool is_text_mode = (init && init->display.flag.is_set(init_display_flags::TEXT));
    bool is_headless = bool(getenv("DFHACK_HEADLESS"));
    if (is_headless)
    {
#ifdef LINUX_BUILD
        if (is_text_mode)
        {
            auto endwin = (int(*)(void))dlsym(RTLD_DEFAULT, "endwin");
            if (endwin)
            {
                endwin();
            }
            else
            {
                std::cerr << "endwin(): bind failed" << std::endl;
            }
        }
        else
        {
            std::cerr << "Headless mode requires PRINT_MODE:TEXT" << std::endl;
        }
#else
        std::cerr << "Headless mode not supported on Windows" << std::endl;
#endif
    }
    if (is_text_mode && !is_headless)
    {
        std::cerr << "Console is not available. Use dfhack-run to send commands.\n";
        if (!is_text_mode)
        {
            std::cout << "Console disabled.\n";
        }
    }
    else if(con.init(false))
        std::cerr << "Console is running.\n";
    else
        std::cerr << "Console has failed to initialize!\n";
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

    // create config directory if it doesn't already exist
    if (!Filesystem::mkdir_recursive(CONFIG_PATH))
        con.printerr("Failed to create config directory: '%s'\n", CONFIG_PATH.c_str());

    // copy over default config files if necessary
    std::map<std::string, bool> config_files;
    std::map<std::string, bool> default_config_files;
    if (Filesystem::listdir_recursive(CONFIG_PATH, config_files, 10, false) != 0)
        con.printerr("Failed to list directory: '%s'\n", CONFIG_PATH.c_str());
    else if (Filesystem::listdir_recursive(CONFIG_DEFAULTS_PATH, default_config_files, 10, false) != 0)
        con.printerr("Failed to list directory: '%s'\n", CONFIG_DEFAULTS_PATH.c_str());
    else
    {
        // ensure all config file directories exist before we start copying files
        for (auto &entry : default_config_files) {
            // skip over files
            if (!entry.second)
                continue;
            std::string dirname = CONFIG_PATH + entry.first;
            if (!Filesystem::mkdir_recursive(dirname))
                con.printerr("Failed to create config directory: '%s'\n", dirname.c_str());
        }

        // copy files from the default tree that don't already exist in the config tree
        for (auto &entry : default_config_files) {
            // skip over directories
            if (entry.second)
                continue;
            std::string filename = entry.first;
            if (!config_files.count(filename)) {
                std::string src_file = CONFIG_DEFAULTS_PATH + filename;
                if (!Filesystem::isfile(src_file))
                    continue;
                std::string dest_file = CONFIG_PATH + filename;
                std::ifstream src(src_file, std::ios::binary);
                std::ofstream dest(dest_file, std::ios::binary);
                if (!src.good() || !dest.good()) {
                    con.printerr("Copy failed: '%s'\n", filename.c_str());
                    continue;
                }
                dest << src.rdbuf();
                src.close();
                dest.close();
            }
        }
    }

    loadScriptPaths(con);

    // initialize common lua context
    if (!Lua::Core::Init(con))
    {
        fatal("Lua failed to initialize");
        return false;
    }

    if (DFSteam::init(con)) {
        std::cerr << "Found Steam.\n";
        DFSteam::launchSteamDFHackIfNecessary(con);
    }
    std::cerr << "Initializing textures.\n";
    Textures::init(con);
    // create mutex for syncing with interactive tasks
    std::cerr << "Initializing plugins.\n";
    // create plugin manager
    plug_mgr = new PluginManager(this);
    plug_mgr->init();
    std::cerr << "Starting the TCP listener.\n";
    auto listen = ServerMain::listen(RemoteClient::GetDefaultPort());
    IODATA *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;

    if (!is_text_mode || is_headless)
    {
        std::cerr << "Starting IO thread.\n";
        // create IO thread
        d->iothread = std::thread{fIOthread, (void*)temp};
    }
    else
    {
        std::cerr << "Starting dfhack.init thread.\n";
        d->iothread = std::thread{fInitthread, (void*)temp};
    }

    std::cerr << "Starting DF input capture thread.\n";
    // set up hotkey capture
    d->hotkeythread = std::thread(fHKthread, (void *) temp);
    started = true;
    modstate = 0;

    if (!listen.get())
        std::cerr << "TCP listen failed.\n";

    if (df::global::game)
    {
        std::vector<std::string> args;
        const std::string & raw = df::global::game->command_line.original;
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
                if (next == std::string::npos)
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
            const std::string & first = *it;
            if (first.length() > 0 && first[0] == '+')
            {
                std::vector<std::string> cmd;
                for (it++; it != args.end(); it++) {
                    const std::string & arg = *it;
                    if (arg.length() > 0 && arg[0] == '+')
                    {
                        break;
                    }
                    cmd.push_back(arg);
                }

                if (runCommand(con, first.substr(1), cmd) != CR_OK)
                {
                    std::cerr << "Error running command: " << first.substr(1);
                    for (auto it2 = cmd.begin(); it2 != cmd.end(); it2++)
                    {
                        std::cerr << " \"" << *it2 << "\"";
                    }
                    std::cerr << "\n";
                }
            }
            else
            {
                it++;
            }
        }
    }

    std::cerr << "DFHack is running.\n";

    onStateChange(con, SC_CORE_INITIALIZED);

    return true;
}
/// sets the current hotkey command
bool Core::setHotkeyCmd( std::string cmd )
{
    // access command
    std::lock_guard<std::mutex> lock(HotkeyMutex);
    hotkey_set = SET;
    hotkey_cmd = cmd;
    HotkeyCond.notify_all();
    return true;
}
/// removes the hotkey command and gives it to the caller thread
std::string Core::getHotkeyCmd( bool &keep_going )
{
    std::string returner;
    std::unique_lock<std::mutex> lock(HotkeyMutex);
    HotkeyCond.wait(lock, [this]() -> bool {return this->hotkey_set;});
    if (hotkey_set == SHUTDOWN) {
        keep_going = false;
        return returner;
    }
    hotkey_set = NO;
    returner = hotkey_cmd;
    hotkey_cmd.clear();
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
    std::lock_guard<std::mutex> lock(misc_data_mutex);
    misc_data_map[key] = p;
}

void *Core::GetData( std::string key )
{
    std::lock_guard<std::mutex> lock(misc_data_mutex);
    std::map<std::string,void*>::iterator it=misc_data_map.find(key);

    if ( it != misc_data_map.end() )
    {
        void *p=it->second;
        return p;
    }
    else
    {
        return 0;// or throw an error.
    }
}

Core& Core::getInstance() {
    static Core instance;
    return instance;
}

bool Core::isSuspended(void)
{
    return ownerThread.load() == std::this_thread::get_id();
}

void Core::doUpdate(color_ostream &out)
{
    Lua::Core::Reset(out, "DF code execution");

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

    bool is_save = strict_virtual_cast<df::viewscreen_savegamest>(screen) ||
        strict_virtual_cast<df::viewscreen_export_regionst>(screen);
    bool is_load_save = is_save ||
        strict_virtual_cast<df::viewscreen_game_cleanerst>(screen) ||
        strict_virtual_cast<df::viewscreen_loadgamest>(screen);

    // save data (do this before updating last_world_data_ptr and triggering unload events)
    if ((df::global::game && df::global::game->main_interface.options.do_manual_save && !d->last_manual_save_request) ||
        (df::global::plotinfo && df::global::plotinfo->main.autosave_request && !d->last_autosave_request) ||
        (is_load_save && !d->was_load_save && is_save))
    {
        plug_mgr->doSaveData(out);
        Persistence::Internal::save(out);
    }

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
            onStateChange(out, new_mapdata ? SC_MAP_LOADED : SC_MAP_UNLOADED);
        }
    }

    if (vs_changed)
        onStateChange(out, SC_VIEWSCREEN_CHANGED);

    bool paused = World::ReadPauseState();
    if (paused != last_pause_state)
    {
        onStateChange(out, last_pause_state ? SC_UNPAUSED : SC_PAUSED);
        last_pause_state = paused;
    }

    // Execute per-frame handlers
    onUpdate(out);

    if (df::global::game && df::global::plotinfo) {
        d->last_autosave_request = df::global::plotinfo->main.autosave_request;
        d->last_manual_save_request = df::global::game->main_interface.options.do_manual_save;
    }
    d->was_load_save = is_load_save;

    out << std::flush;
}

// should always be from simulation thread!
int Core::Update()
{
    if (shutdown)
    {
        // release the suspender
        MainThread::suspend().unlock();
        return -1;
    }

    if(errorstate)
        return -1;

    color_ostream_proxy out(con);

    // Pretend this thread has suspended the core in the usual way,
    // and run various processing hooks.
    {
        if(!started)
        {
            // Initialize the core
            InitSimulationThread();
            if(errorstate)
                return -1;
        }

        uint32_t start_ms = p->getTickCount();
        perf_counters.registerTick(start_ms);
        doUpdate(out);
        perf_counters.incCounter(perf_counters.total_update_ms, start_ms);
    }

    // Let all commands run that require CoreSuspender
    CoreWakeup.wait(MainThread::suspend(),
            [this]() -> bool {return this->toolCount.load() == 0;});

    return 0;
};

extern bool buildings_do_onupdate;
void buildings_onStateChange(color_ostream &out, state_change_event event);
void buildings_onUpdate(color_ostream &out);

static int buildings_timer = 0;

void Core::onUpdate(color_ostream &out)
{
    Gui::clearFocusStringCache();

    uint32_t step_start_ms = p->getTickCount();
    EventManager::manageEvents(out);
    perf_counters.incCounter(perf_counters.update_event_manager_ms, step_start_ms);

    // convert building reagents
    if (buildings_do_onupdate && (++buildings_timer & 1))
        buildings_onUpdate(out);

    // notify all the plugins that a game tick is finished
    step_start_ms = p->getTickCount();
    plug_mgr->OnUpdate(out);
    perf_counters.incCounter(perf_counters.update_plugin_ms, step_start_ms);

    // process timers in lua
    step_start_ms = p->getTickCount();
    Lua::Core::onUpdate(out);
    perf_counters.incCounter(perf_counters.update_lua_ms, step_start_ms);
}

void getFilesWithPrefixAndSuffix(const std::string& folder, const std::string& prefix, const std::string& suffix, std::vector<std::string>& result) {
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

size_t loadScriptFiles(Core* core, color_ostream& out, const std::vector<std::string>& prefix, const std::string& folder) {
    static const std::string suffix = ".init";
    std::vector<std::string> scriptFiles;
    for ( size_t a = 0; a < prefix.size(); a++ ) {
        getFilesWithPrefixAndSuffix(folder, prefix[a], ".init", scriptFiles);
    }
    std::sort(scriptFiles.begin(), scriptFiles.end(),
              [&](const std::string &a, const std::string &b) {
        std::string a_base = a.substr(0, a.size() - suffix.size());
        std::string b_base = b.substr(0, b.size() - suffix.size());
        return a_base < b_base;
    });
    size_t result = 0;
    for ( size_t a = 0; a < scriptFiles.size(); a++ ) {
        result++;
        std::string path = "";
        if (folder != ".")
            path = folder + "/";
        core->loadScriptFile(out, path + scriptFiles[a], false);
    }
    return result;
}

namespace DFHack {
    namespace X {
        typedef state_change_event Key;
        typedef std::vector<std::string> Val;
        typedef std::pair<Key,Val> Entry;
        typedef std::vector<Entry> EntryVector;
        typedef std::map<Key,Val> InitVariationTable;

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
                    val.emplace_back(v);
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

    std::string rawFolder = !isWorldLoaded() ? "" : "save/" + World::ReadWorldFolder() + "/init";

    auto i = table.find(event);
    if ( i != table.end() ) {
        const std::vector<std::string>& set = i->second;

        // load baseline defaults
        this->loadScriptFile(out, CONFIG_PATH + "init/default." + set[0] + ".init", false);

        loadScriptFiles(this, out, set, CONFIG_PATH + "init");
        loadScriptFiles(this, out, set, rawFolder);
    }

    for (auto it = state_change_scripts.begin(); it != state_change_scripts.end(); ++it)
    {
        if (it->event == event)
        {
            if (!it->save_specific)
            {
                loadScriptFile(out, it->path, false);
            }
            else if (it->save_specific && isWorldLoaded())
            {
                loadScriptFile(out, rawFolder + it->path, false);
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
    case SC_CORE_INITIALIZED:
    {
        loadModScriptPaths(out);
        Lua::CallLuaModuleFunction(con, "helpdb", "refresh");
        Lua::CallLuaModuleFunction(con, "script-manager", "reload");
        break;
    }
    case SC_WORLD_LOADED:
    {
        perf_counters.reset();
        Persistence::Internal::load(out);
        plug_mgr->doLoadWorldData(out);
        loadModScriptPaths(out);
        auto L = Lua::Core::State;
        Lua::StackUnwinder top(L);
        Lua::CallLuaModuleFunction(con, "script-manager", "reload", std::make_tuple(true));
        if (world && world->cur_savegame.save_dir.size())
        {
            std::string save_dir = "save/" + world->cur_savegame.save_dir;
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
        if (Version::is_prerelease())
        {
            runCommand(out, "gui/prerelease-warning");
            std::cerr << "loaded world in prerelease build" << std::endl;
        }
        break;
    }
    case SC_MAP_LOADED:
        if (World::IsSiteLoaded())
            plug_mgr->doLoadSiteData(out);
        break;
    case SC_PAUSED:
        if (!perf_counters.getIgnorePauseState()) {
            perf_counters.elapsed_ms += p->getTickCount() - perf_counters.baseline_elapsed_ms;
            perf_counters.baseline_elapsed_ms = 0;
        }
        break;
    case SC_UNPAUSED:
        if (!perf_counters.getIgnorePauseState())
            perf_counters.baseline_elapsed_ms = p->getTickCount();
        break;
    default:
        break;
    }

    EventManager::onStateChange(out, event);

    buildings_onStateChange(out, event);

    plug_mgr->OnStateChange(out, event);

    Lua::Core::onStateChange(out, event);

    handleLoadAndUnloadScripts(out, event);

    if (event == SC_WORLD_UNLOADED)
    {
        Persistence::Internal::clear(out);
        loadModScriptPaths(out);
        Lua::CallLuaModuleFunction(con, "script-manager", "reload");
    }
}

int Core::Shutdown ( void )
{
    if(errorstate)
        return true;
    errorstate = 1;

    shutdown = true;

    // Make sure the console thread shutdowns before clean up to avoid any
    // unlikely data races.
    if (d->iothread.joinable()) {
        con.shutdown();
    }

    if (d->hotkeythread.joinable()) {
        std::unique_lock<std::mutex> hot_lock(HotkeyMutex);
        hotkey_set = SHUTDOWN;
        HotkeyCond.notify_one();
    }

    ServerMain::block();

    d->hotkeythread.join();
    d->iothread.join();

    if(plug_mgr)
    {
        delete plug_mgr;
        plug_mgr = 0;
    }
    // invalidate all modules
    allModules.clear();
    Textures::cleanup();
    DFSDL::cleanup();
    DFSteam::cleanup(getConsole());
    memset(&(s_mods), 0, sizeof(s_mods));
    d.reset();
    return -1;
}

// FIXME: this is HORRIBLY broken
// from ncurses
#define KEY_F0      0410        /* Function keys.  Space for 64 */
#define KEY_F(n)    (KEY_F0+(n))    /* Value of function key n */

// returns true if the event has been handled
bool Core::ncurses_wgetch(int in, int & out)
{
    if(!started)
    {
        out = in;
        return false;
    }
    if(in >= KEY_F(1) && in <= KEY_F(8))
    {
/* TODO: understand how this changes for v50
        int idx = in - KEY_F(1);
        // FIXME: copypasta, push into a method!
        if(df::global::plotinfo && df::global::gview)
        {
            df::viewscreen * ws = Gui::getCurViewscreen();
            if (strict_virtual_cast<df::viewscreen_dwarfmodest>(ws) &&
                df::global::plotinfo->main.mode != ui_sidebar_mode::Hotkeys &&
                df::global::plotinfo->main.hotkeys[idx].cmd == df::ui_hotkey::T_cmd::None)
            {
                setHotkeyCmd(df::global::plotinfo->main.hotkeys[idx].name);
                return true;
            }
            else
            {
                out = in;
                return false;
            }
        }
*/
    }
    out = in;
    return false;
}

bool Core::DFH_ncurses_key(int key)
{
    if (getenv("DFHACK_HEADLESS"))
        return true;
    int dummy;
    return ncurses_wgetch(key, dummy);
}

bool Core::getSuppressDuplicateKeyboardEvents() {
    return suppress_duplicate_keyboard_events;
}

void Core::setSuppressDuplicateKeyboardEvents(bool suppress) {
    DEBUG(keybinding).print("setting suppress_duplicate_keyboard_events to %s\n",
        suppress ? "true" : "false");
    suppress_duplicate_keyboard_events = suppress;
}

void Core::setMortalMode(bool value) {
    std::lock_guard<std::mutex> lock(HotkeyMutex);
    mortal_mode = value;
}

void Core::setArmokTools(const std::vector<std::string> &tool_names) {
    std::lock_guard<std::mutex> lock(HotkeyMutex);
    armok_tools.clear();
    armok_tools.insert(tool_names.begin(), tool_names.end());
}

// returns true if the event is handled
bool Core::DFH_SDL_Event(SDL_Event* ev) {
    uint32_t start_ms = p->getTickCount();
    bool ret = doSdlInputEvent(ev);
    perf_counters.incCounter(perf_counters.total_keybinding_ms, start_ms);
    return ret;
}

bool Core::doSdlInputEvent(SDL_Event* ev)
{
    // DF only calls the sdl input event hook from the render thread
    // we use this to identify the render thread
    //
    // first time this is called, set df_render_thread to calling thread
    // afterwards, assert that it's never changed
    assert(std::this_thread::get_id() == df_render_thread);

    static std::map<int, bool> hotkey_states;

    // do NOT process events before we are ready.
    if (!started || !ev)
        return false;

    if (ev->type == SDL_WINDOWEVENT && ev->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        // clear modstate when gaining focus in case alt-tab was used when
        // losing focus and modstate is now incorrectly set
        modstate = 0;
        return false;
    }

    if (ev->type == SDL_KEYDOWN || ev->type == SDL_KEYUP) {
        auto &ke = ev->key;
        auto &sym = ke.keysym.sym;

        if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT)
            modstate = (ev->type == SDL_KEYDOWN) ? modstate | DFH_MOD_SHIFT : modstate & ~DFH_MOD_SHIFT;
        else if (sym == SDLK_LCTRL || sym == SDLK_RCTRL)
            modstate = (ev->type == SDL_KEYDOWN) ? modstate | DFH_MOD_CTRL : modstate & ~DFH_MOD_CTRL;
        else if (sym == SDLK_LALT || sym == SDLK_RALT)
            modstate = (ev->type == SDL_KEYDOWN) ? modstate | DFH_MOD_ALT : modstate & ~DFH_MOD_ALT;
        else if (ke.state == SDL_PRESSED && !hotkey_states[sym])
        {
            // the check against hotkey_states[sym] ensures we only process keybindings once per keypress
            DEBUG(keybinding).print("key down: sym=%d (%c)\n", sym, sym);
            if (SelectHotkey(sym, modstate)) {
                hotkey_states[sym] = true;
                if (modstate & (DFH_MOD_CTRL | DFH_MOD_ALT)) {
                    DEBUG(keybinding).print("modifier key detected; not inhibiting SDL key down event\n");
                    return false;
                }
                DEBUG(keybinding).print("%sinhibiting SDL key down event\n",
                    suppress_duplicate_keyboard_events ? "" : "not ");
                return suppress_duplicate_keyboard_events;
            }
        }
        else if (ke.state == SDL_RELEASED)
        {
            DEBUG(keybinding).print("key up: sym=%d (%c)\n", sym, sym);
            hotkey_states[sym] = false;
        }
    } else if (ev->type == SDL_MOUSEBUTTONDOWN) {
        auto &but = ev->button;
        DEBUG(keybinding).print("mouse button down: button=%d\n", but.button);
        // don't mess with the first three buttons, which are critical elements of DF's control scheme
        if (but.button > 3) {
            SDL_Keycode sym = SDLK_F13 + but.button - 4;
            if (sym <= SDLK_F24 && SelectHotkey(sym, modstate))
                return suppress_duplicate_keyboard_events;
        }
    } else if (ev->type == SDL_TEXTINPUT) {
        auto &te = ev->text;
        DEBUG(keybinding).print("text input: '%s' (modifiers: %s%s%s)\n",
            te.text,
            modstate & DFH_MOD_SHIFT ? "Shift" : "",
            modstate & DFH_MOD_CTRL ? "Ctrl" : "",
            modstate & DFH_MOD_ALT ? "Alt" : "");
        if (strlen(te.text) == 1 && hotkey_states[te.text[0]]) {
            DEBUG(keybinding).print("%sinhibiting SDL text event\n",
                suppress_duplicate_keyboard_events ? "" : "not ");
            return suppress_duplicate_keyboard_events;
        }
    }

    return false;
}

bool Core::SelectHotkey(int sym, int modifiers)
{
    // Find the topmost viewscreen
    if (!df::global::gview || !df::global::plotinfo)
        return false;

    df::viewscreen *screen = &df::global::gview->view;
    while (screen->child)
        screen = screen->child;

    if (sym == SDLK_KP_ENTER)
        sym = SDLK_RETURN;

    std::string cmd;

    DEBUG(keybinding).print("checking hotkeys for sym=%d (%c), modifiers=%x\n", sym, sym, modifiers);

    {
        std::lock_guard<std::mutex> lock(HotkeyMutex);

        // Check the internal keybindings
        std::vector<KeyBinding> &bindings = key_bindings[sym];
        for (int i = bindings.size()-1; i >= 0; --i) {
            auto &binding = bindings[i];
            DEBUG(keybinding).print("examining hotkey with commandline: '%s'\n", binding.cmdline.c_str());

            if (binding.modifiers != modifiers) {
                DEBUG(keybinding).print("skipping keybinding due to modifiers mismatch: 0x%x != 0x%x\n",
                                        binding.modifiers, modifiers);
                continue;
            }
            if (!binding.focus.empty()) {
                if (!Gui::matchFocusString(binding.focus)) {
                    std::vector<std::string> focusStrings = Gui::getCurFocus(true);
                    DEBUG(keybinding).print("skipping keybinding due to focus string mismatch: '%s' != '%s'\n",
                        join_strings(", ", focusStrings).c_str(), binding.focus.c_str());
                    continue;
                }
            }
            if (!plug_mgr->CanInvokeHotkey(binding.command[0], screen)) {
                DEBUG(keybinding).print("skipping keybinding due to hotkey guard rejection (command: '%s')\n",
                                        binding.command[0].c_str());
                continue;
            }
            if (mortal_mode && armok_tools.contains(binding.command[0])) {
                DEBUG(keybinding).print("skipping keybinding due to mortal mode (command: '%s')\n",
                                        binding.command[0].c_str());
                continue;
            }

            cmd = binding.cmdline;
            DEBUG(keybinding).print("matched hotkey\n");
            break;
        }

        if (cmd.empty()) {
            // Check the hotkey keybindings
            int idx = sym - SDLK_F1;
            if(idx >= 0 && idx < 8)
            {
/* TODO: understand how this changes for v50
                if (modifiers & 1)
                    idx += 8;

                if (strict_virtual_cast<df::viewscreen_dwarfmodest>(screen) &&
                    df::global::plotinfo->main.mode != ui_sidebar_mode::Hotkeys &&
                    df::global::plotinfo->main.hotkeys[idx].cmd == df::ui_hotkey::T_cmd::None)
                {
                    cmd = df::global::plotinfo->main.hotkeys[idx].name;
                }
*/
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
        *psym = SDLK_a + (keyspec[0]-'A');
        return true;
    } else if (keyspec.size() == 1 && keyspec[0] == '`') {
        *psym = SDLK_BACKQUOTE;
        return true;
    } else if (keyspec.size() == 1 && keyspec[0] >= '0' && keyspec[0] <= '9') {
        *psym = SDLK_0 + (keyspec[0]-'0');
        return true;
    } else if (keyspec.size() == 2 && keyspec[0] == 'F' && keyspec[1] >= '1' && keyspec[1] <= '9') {
        *psym = SDLK_F1 + (keyspec[1]-'1');
        return true;
    } else if (keyspec.size() == 3 && keyspec.substr(0, 2) == "F1" && keyspec[2] >= '0' && keyspec[2] <= '2') {
        *psym = SDLK_F10 + (keyspec[2]-'0');
        return true;
    } else if (keyspec.size() == 6 && keyspec.substr(0, 5) == "MOUSE" && keyspec[5] >= '4' && keyspec[5] <= '9') {
        *psym = SDLK_F13 + (keyspec[5]-'4');
        return true;
    } else if (keyspec.size() == 7 && keyspec.substr(0, 6) == "MOUSE1" && keyspec[5] >= '0' && keyspec[5] <= '5') {
        *psym = SDLK_F19 + (keyspec[5]-'0');
        return true;
    } else if (keyspec == "Enter") {
        *psym = SDLK_RETURN;
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

    std::lock_guard<std::mutex> lock(HotkeyMutex);

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

    std::lock_guard<std::mutex> lock(HotkeyMutex);

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

    std::lock_guard<std::mutex> lock(HotkeyMutex);

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

bool Core::AddAlias(const std::string &name, const std::vector<std::string> &command, bool replace)
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    if (!IsAlias(name) || replace)
    {
        aliases[name] = command;
        return true;
    }
    return false;
}

bool Core::RemoveAlias(const std::string &name)
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    if (IsAlias(name))
    {
        aliases.erase(name);
        return true;
    }
    return false;
}

bool Core::IsAlias(const std::string &name)
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    return aliases.find(name) != aliases.end();
}

bool Core::RunAlias(color_ostream &out, const std::string &name,
    const std::vector<std::string> &parameters, command_result &result)
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    if (!IsAlias(name))
    {
        return false;
    }

    const std::string &first = aliases[name][0];
    std::vector<std::string> parts(aliases[name].begin() + 1, aliases[name].end());
    parts.insert(parts.end(), parameters.begin(), parameters.end());
    result = runCommand(out, first, parts);
    return true;
}

std::map<std::string, std::vector<std::string>> Core::ListAliases()
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    return aliases;
}

std::string Core::GetAliasCommand(const std::string &name, bool ignore_params)
{
    std::lock_guard<std::recursive_mutex> lock(alias_mutex);
    if (!IsAlias(name) || aliases[name].empty())
        return name;
    if (ignore_params)
        return aliases[name][0];
    return join_strings(" ", aliases[name]);
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
        p = Core::getInstance().p.get();
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
        std::unique_ptr<Module> mod = create##TYPE();\
        s_mods.p##TYPE = (TYPE *) mod.get();\
        allModules.push_back(std::move(mod));\
    }\
    return s_mods.p##TYPE;\
}

MODULE_GETTER(Materials);
MODULE_GETTER(Graphic);
