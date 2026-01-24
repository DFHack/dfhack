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

#include "Core.h"

#include "Internal.h"

#include "Error.h"
#include "MemAccess.h"
#include "DataDefs.h"
#include "Debug.h"
#include "Console.h"
#include "MemoryPatcher.h"
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
#include "Format.h"

#include "Commands.h"

#include "modules/DFSDL.h"
#include "modules/DFSteam.h"
#include "modules/EventManager.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Hotkey.h"
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
#include <ranges>
#include <span>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <forward_list>
#include <type_traits>
#include <cstdarg>
#include <filesystem>
#include <SDL_events.h>


#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef LINUX_BUILD
#include <dlfcn.h>
#endif

using namespace DFHack;
using namespace df::enums;
using df::global::init;
using df::global::world;
using std::string;

// FIXME: A lot of code in one file, all doing different things... there's something fishy about it.

static size_t loadScriptFiles(Core* core, color_ostream& out, std::span<const std::string> prefix, const std::filesystem::path& folder);

namespace DFHack {
    DBG_DECLARE(core, keybinding, DebugCategory::LINFO);
    DBG_DECLARE(core, script, DebugCategory::LINFO);

    static const std::filesystem::path getConfigPath()
    {
        return Filesystem::getInstallDir() / "dfhack-config";
    };

    static const std::filesystem::path getConfigDefaultsPath()
    {
        return Filesystem::getInstallDir() / "hack" / "data" / "dfhack-config-defaults";
    };

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

uint32_t PerfCounters::registerTick(uint32_t baseline_ms) {
    if (!World::isFortressMode() || World::ReadPauseState()) {
        last_tick_baseline_ms = 0;
        return 0;
    }

    // only update when the tick counter has advanced
    if (!world || last_frame_counter == world->frame_counter)
        return 0;
    last_frame_counter = world->frame_counter;

    if (last_tick_baseline_ms == 0) {
        last_tick_baseline_ms = baseline_ms;
        return 0;
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

    return elapsed_ms;
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
    static constexpr int MAX_DEPTH = 20;
    static thread_local int depth;
    CommandDepthCounter() { depth++; }
    ~CommandDepthCounter() { depth--; }
    bool ok() { return depth < MAX_DEPTH; }
};
thread_local int CommandDepthCounter::depth = 0;

void Core::cheap_tokenise(std::string const& input, std::vector<std::string>& output)
{
    std::string *cur = nullptr;
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
            cur = nullptr;
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

std::string DFHack::dfhack_version_desc()
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

static bool init_run_script(color_ostream &out, lua_State *state, const std::string_view pcmd, const std::span<const std::string> pargs)
{
    if (!lua_checkstack(state, pargs.size()+10))
        return false;
    Lua::PushDFHack(state);
    lua_getfield(state, -1, "run_script");
    lua_remove(state, -2);
    lua_pushlstring(state, pcmd.data(), pcmd.size());
    for (const auto& arg : pargs)
        lua_pushstring(state, arg.c_str());
    return true;
}

static command_result runLuaScript(color_ostream &out, const std::string_view name, const std::span<const std::string> args)
{
    auto init_fn = [name, args](color_ostream& out, lua_State* state) -> bool {
        return init_run_script(out, state, name, args);
    };

    bool ok = Lua::RunCoreQueryLoop(out, DFHack::Core::getInstance().getLuaState(true), init_fn);

    return ok ? CR_OK : CR_FAILURE;
}

static bool init_enable_script(color_ostream &out, lua_State *state, const std::string_view name, bool enable)
{
    if (!lua_checkstack(state, 4))
        return false;
    Lua::PushDFHack(state);
    lua_getfield(state, -1, "enable_script");
    lua_remove(state, -2);
    lua_pushlstring(state, name.data(), name.size());
    lua_pushboolean(state, enable);
    return true;
}

command_result Core::enableLuaScript(color_ostream &out, const std::string_view name, bool enabled)
{
    auto init_fn = [name, enabled](color_ostream& out, lua_State* state) -> bool {
        return init_enable_script(out, state, name, enabled);
    };

    bool ok = Lua::RunCoreQueryLoop(out, DFHack::Core::getInstance().getLuaState(), init_fn);

    return ok ? CR_OK : CR_FAILURE;
}

command_result Core::runCommand(color_ostream& out, const std::string& command)
{
    if (!command.empty())
    {
        std::vector <std::string> parts;
        Core::cheap_tokenise(command, parts);
        if (parts.size() == 0)
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

bool DFHack::is_builtin(color_ostream& con, const std::string& command)
{
    CoreSuspender suspend;
    auto L = DFHack::Core::getInstance().getLuaState();
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 1) ||
        !Lua::PushModulePublic(con, L, "helpdb", "is_builtin"))
    {
        con.printerr("Failed to load helpdb Lua code\n");
        return false;
    }

    Lua::Push(L, command);

    if (!Lua::SafeCall(con, L, 1, 1))
    {
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

    auto L = DFHack::Core::getInstance().getLuaState();
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
        con.printerr("{} is not recognized. Did you mean {}?\n", first, completed);
        return true;
    }

    if (possible.size() > 1 && possible.size() < 8)
    {
        std::string out;
        for (size_t i = 0; i < possible.size(); i++)
            out += " " + possible[i];
        con.printerr("{} is not recognized. Possible completions:{}\n", first, out);
        return true;
    }

    return false;
}

bool Core::addScriptPath(std::filesystem::path path, bool search_before)
{
    std::lock_guard<std::mutex> lock(script_path_mutex);
    auto &vec = script_paths[search_before ? 0 : 1];
    if (std::find(vec.begin(), vec.end(), path) != vec.end())
        return false;
    if (!Filesystem::isdir(path))
        return false;
    vec.push_back(path);
    return true;
}

bool Core::setModScriptPaths(const std::vector<std::filesystem::path> &mod_script_paths) {
    std::lock_guard<std::mutex> lock(script_path_mutex);
    script_paths[2] = mod_script_paths;
    return true;
}

bool Core::removeScriptPath(std::filesystem::path path)
{
    std::lock_guard<std::mutex> lock(script_path_mutex);
    bool found = false;
    for (int i = 0; i < 2; i++)
    {
        auto &vec = script_paths[i];
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

void Core::getScriptPaths(std::vector<std::filesystem::path> *dest)
{
    std::lock_guard<std::mutex> lock(script_path_mutex);
    dest->clear();
    std::filesystem::path df_pref_path = Filesystem::getBaseDir();
    std::filesystem::path df_install_path = Filesystem::getInstallDir();
    for (auto & path : script_paths[0])
        dest->emplace_back(path);
    // should this be df_pref_path? probably
    dest->push_back(getConfigPath() / "scripts");
    if (df::global::world && isWorldLoaded()) {
        std::string save = World::ReadWorldFolder();
        if (save.size())
            dest->emplace_back(df_pref_path / "save" / save / "scripts");
    }
    dest->emplace_back(df_install_path / "hack" / "scripts");
    for (auto & path : script_paths[2])
        dest->emplace_back(path);
    for (auto & path : script_paths[1])
        dest->emplace_back(path);
}

std::filesystem::path Core::findScript(std::string name)
{
    std::vector<std::filesystem::path> paths;
    getScriptPaths(&paths);
    for (auto& path : paths)
    {
        std::error_code ec;
        auto raw_path = path / name;
        std::filesystem::path load_path = std::filesystem::weakly_canonical(raw_path, ec);
        if (ec)
        {
            con.printerr("Error loading '{}' ({})\n", raw_path, ec.message());
            continue;
        }

        if (Filesystem::isfile(load_path))
            return load_path;
    }
    return {};
}

bool loadScriptPaths(color_ostream &out, bool silent = false)
{
    std::filesystem::path filename{ getConfigPath() / "script-paths.txt" };
    std::ifstream file(filename);
    if (!file)
    {
        if (!silent)
            out.printerr("Could not load {}\n", filename);
        return false;
    }
    std::string raw;
    int line = 0;
    while (getline(file, raw))
    {
        ++line;
        std::istringstream ss(raw);
        char ch;
        ss >> std::skipws;
        if (!(ss >> ch) || ch == '#')
            continue;
        ss >> std::ws; // discard whitespace
        std::string path;
        getline(ss, path);
        if (ch == '+' || ch == '-')
        {
            if (!Core::getInstance().addScriptPath(path, ch == '+') && !silent)
                out.printerr("{}:{}: Failed to add path: {}\n", filename, line, path);
        }
        else if (!silent)
            out.printerr("{}:{}: Illegal character: {}\n", filename, line, ch);
    }
    return true;
}

static void loadModScriptPaths(color_ostream &out) {
    std::vector<std::string> mod_script_paths_str;
    std::vector<std::filesystem::path> mod_script_paths;
    Lua::CallLuaModuleFunction(out, "script-manager", "get_mod_script_paths", {}, 1,
            [&](lua_State *L) {
                Lua::GetVector(L, mod_script_paths_str);
            });
    DEBUG(script,out).print("final mod script paths:\n");
    for (auto& path : mod_script_paths_str)
    {
        DEBUG(script, out).print("  {}\n", path);
        mod_script_paths.push_back(std::filesystem::weakly_canonical(std::filesystem::path{ path }));
    }
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

state_change_event DFHack::sc_event_id (std::string name) {
    sc_event_map_init();
    auto it = state_change_event_map.find(name);
    if (it != state_change_event_map.end())
        return it->second;
    if (name.find("SC_") != 0)
        return sc_event_id(std::string("SC_") + name);
    return SC_UNKNOWN;
}

std::string DFHack::sc_event_name (state_change_event id) {
    sc_event_map_init();
    for (auto it = state_change_event_map.begin(); it != state_change_event_map.end(); ++it)
    {
        if (it->second == id)
            return it->first;
    }
    return "SC_UNKNOWN";
}

void DFHack::help_helper(color_ostream &con, const std::string &entry_name) {
    ConditionalCoreSuspender suspend{};

    if (!suspend) {
        con.printerr("Failed Lua call to helpdb.help (could not acquire core lock).\n");
        return;
    }

    auto L = DFHack::Core::getInstance().getLuaState();
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
    ConditionalCoreSuspender suspend{};

    if (!suspend) {
        con.printerr("Failed Lua call to helpdb.help (could not acquire core lock).\n");
        return;
    }

    auto L = DFHack::Core::getInstance().getLuaState();
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

static void ls_helper(color_ostream &con, const std::span<const std::string> params) {
    std::vector<std::string> filter;
    bool skip_tags = false;
    bool show_dev_commands = false;
    std::string_view exclude_strs;

    bool in_exclude = false;
    for (const auto& str : params) {
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

    ConditionalCoreSuspender suspend{};

    if (!suspend) {
        con.printerr("Failed Lua call to helpdb.help (could not acquire core lock).\n");
        return;
    }

    auto L = DFHack::Core::getInstance().getLuaState();
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

command_result Core::runCommand(color_ostream &con, const std::string &first_, std::vector<std::string> &parts, bool no_autocomplete)
{
    std::string first = first_;
    CommandDepthCounter counter;
    if (!counter.ok())
    {
        con.printerr("Cannot invoke \"{}\": maximum command depth exceeded ({})\n",
            first, CommandDepthCounter::MAX_DEPTH);
        return CR_FAILURE;
    }

    if (first.empty())
        return CR_NOT_IMPLEMENTED;

    if (has_backslashes(first))
    {
        con.printerr("Replacing backslashes with forward slashes in \"{}\"\n", first);
        replace_backslashes_with_forwardslashes(first);
    }

    // let's see what we actually got
    command_result res;
    if (first == "help" || first == "man" || first == "?")
    {
        return Commands::help(con, *this, first, parts);
    }
    else if (first == "tags")
    {
        tags_helper(con, parts.size() ? parts[0] : "");
    }
    else if (first == "load" || first == "unload" || first == "reload")
    {
        return Commands::load(con, *this, first, parts);
    }
    else if( first == "enable" || first == "disable" )
    {
        return Commands::enable(con, *this, first, parts);
    }
    else if (first == "ls" || first == "dir")
    {
        ls_helper(con, parts);
    }
    else if (first == "plug")
    {
        return Commands::plug(con, *this, first, parts);
    }
    else if (first == "type")
    {
        return Commands::type(con, *this, first, parts);
    }
    else if (first == "keybinding")
    {
        return Commands::keybinding(con, *this, first, parts);
    }
    else if (first == "alias")
    {
        return Commands::alias(con, *this, first, parts);
    }
    else if (first == "fpause")
    {
        return Commands::fpause(con, *this, first, parts);
    }
    else if (first == "cls" || first == "clear")
    {
        return Commands::clear(con, *this, first, parts);
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
        return Commands::kill_lua(con, *this, first, parts);
    }
    else if (first == "script")
    {
        return Commands::script(con, *this, first, parts);
    }
    else if (first == "hide")
    {
        return Commands::hide(con, *this, first, parts);
    }
    else if (first == "show")
    {
        return Commands::show(con, *this, first, parts);
    }
    else if (first == "sc-script")
    {
        return Commands::sc_script(con, *this, first, parts);
    }
    else if (first == "devel/dump-rpc")
    {
        return Commands::dump_rpc(con, *this, first, parts);
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
            DFHack::help_helper(con, first);
        }
        else if (res == CR_NOT_IMPLEMENTED)
        {
            std::string completed;
            std::filesystem::path filename = findScript(first + ".lua");
            bool lua = !filename.empty();
            if ( lua )
                res = runLuaScript(con, first, parts);
            else if (!no_autocomplete && try_autocomplete(con, first, completed))
                res = CR_NOT_IMPLEMENTED;
            else
                con.printerr("{} is not a recognized command.\n", first);
            if (res == CR_NOT_IMPLEMENTED)
            {
                Plugin *p = plug_mgr->getPluginByName(first);
                if (p)
                {
                    con.printerr("{} is a plugin ", first);
                    if (p->getState() == Plugin::PS_UNLOADED)
                        con.printerr("that is not loaded - try \"load {}\" or check stderr.log\n",
                            first);
                    else if (p->size())
                        con.printerr("that implements {} commands - see \"help {}\" for details\n",
                            p->size(), first);
                    else
                        con.printerr("but does not implement any commands\n");
                }
            }
        }
        else if (res == CR_NEEDS_CONSOLE)
            con.printerr("{} needs an interactive console to work.\n"
                          "Please run this command from the DFHack console.\n\n"
#ifdef WIN32
                          "You can show the console with the 'show' command."
#else
                          "The console is accessible when you run DF from the commandline\n"
                          "via the './dfhack' script."
#endif
                          "\n", first);
        return res;
    }

    return CR_OK;
}

bool Core::loadScriptFile(color_ostream &out, std::filesystem::path fname, bool silent)
{
    if(!silent) {
        INFO(script,out) << "Running script: " << fname << std::endl;
        std::cerr << "Running script: " << fname << std::endl;
    }
    std::ifstream script{ fname.c_str() };
    if ( !script )
    {
        if(!silent)
            out.printerr("Error loading script: {}\n", fname);
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
    core->loadScriptFile(out, getConfigPath() / "init" / "default.dfhack.init", false);

    // load user overrides
    std::vector<std::string> prefixes(1, "dfhack");
    loadScriptFiles(core, out, prefixes, getConfigPath() / "init");

    // show the terminal if requested
    auto L = DFHack::Core::getInstance().getLuaState();
    Lua::CallLuaModuleFunction(out, L, "dfhack", "getHideConsoleOnStartup", 0, 1,
        Lua::DEFAULT_LUA_LAMBDA, [&](lua_State* L) {
            if (!lua_toboolean(L, -1))
                core->getConsole().show();
        }, false);
}

// Load dfhack.init in a dedicated thread (non-interactive console mode)
static void fInitthread(IODATA * iod)
{
    Core * core = iod->core;
    color_ostream_proxy out(core->getConsole());

    run_dfhack_init(out, core);
}

// A thread function... for the interactive console.
static void fIOthread(IODATA * iod)
{
    static const std::filesystem::path HISTORY_FILE = getConfigPath() / "dfhack.history";

    Core * core = iod->core;
    PluginManager * plug_mgr = iod->plug_mgr;

    CommandHistory main_history;
    main_history.load(HISTORY_FILE.c_str());

    Console & con = core->getConsole();
    if (plug_mgr == nullptr)
    {
        con.printerr("Something horrible happened in Core's constructor...\n");
        return;
    }

    run_dfhack_init(con, core);

    con.print("DFHack is ready. Have a nice day!\n"
              "DFHack version {}\n"
              "Type in '?' or 'help' for general help, 'ls' to see all commands.\n",
              dfhack_version_desc());

    int clueless_counter = 0;

    if (getenv("DFHACK_DISABLE_CONSOLE"))
        return;

    while (true)
    {
        std::string command;
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
            main_history.save(HISTORY_FILE);
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
    armok_mutex{},
    alias_mutex{},
    started{false},
    CoreSuspendMutex{},
    CoreWakeup{},
    ownerThread{},
    toolCount{0}
{
    // init the console. This must be always the first step!
    plug_mgr = nullptr;
    errorstate = false;
    vinfo = 0;
    memset(&(s_mods), 0, sizeof(s_mods));

    // set up hotkey capture
    suppress_duplicate_keyboard_events = true;
    last_world_data_ptr = nullptr;
    last_local_map_ptr = nullptr;
    last_pause_state = false;
    top_viewscreen = nullptr;

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
        con.printerr("{}", out.str());
        con.reset_color();
        con.print("\n");
    }
    fmt::print(stderr, "{}\n", out.str());
    out << "Check file stderr.log for details.\n";
    std::cout << "DFHack fatal error: " << out.str() << std::endl;
    if (!title)
        title = "DFHack error!";
    DFSDL::DFSDL_ShowSimpleMessageBox(0x10 /* SDL_MESSAGEBOX_ERROR */, title, out.str().c_str(), nullptr);

    bool is_headless = bool(getenv("DFHACK_HEADLESS"));
    if (is_headless)
    {
        exit('f');
    }
}

std::filesystem::path Core::getHackPath()
{
    return p->getPath() / "hack";
}

df::viewscreen * Core::getTopViewscreen() {
    return getInstance().top_viewscreen;
}

bool Core::InitMainThread() {
    // this hook is always called from DF's main (render) thread, so capture this thread id
    df_render_thread = std::this_thread::get_id();

    Filesystem::init();

    #ifdef LINUX_BUILD
        extern void dfhack_crashlog_init();
        dfhack_crashlog_init();
    #endif

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

    std::cerr << "DFHack build: " << Version::git_description() << std::endl;
    if (strlen(Version::dfhack_run_url())) {
        std::cerr << "Build url: " << Version::dfhack_run_url() << std::endl;
    }
    std::cerr << "Starting with working directory: " << Filesystem::getcwd() << std::endl;

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
            constexpr auto msg = (
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
            { "gps", sizeof(df::graphic) },
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
    unpaused_ms = 0;

    return true;
}

bool Core::InitSimulationThread()
{
    // the update hook is only called from the simulation thread, so capture this thread id
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
    if (!Filesystem::mkdir_recursive(getConfigPath()))
        con.printerr("Failed to create config directory: '{}'\n", getConfigPath());

    // copy over default config files if necessary
    std::map<std::filesystem::path, bool> config_files;
    std::map<std::filesystem::path, bool> default_config_files;
    if (Filesystem::listdir_recursive(getConfigPath(), config_files, 10, false) != 0)
        con.printerr("Failed to list directory: '{}'\n", getConfigPath());
    else if (Filesystem::listdir_recursive(getConfigDefaultsPath(), default_config_files, 10, false) != 0)
        con.printerr("Failed to list directory: '{}'\n", getConfigDefaultsPath());
    else
    {
        // ensure all config file directories exist before we start copying files
        for (auto &entry : default_config_files) {
            // skip over files
            if (!entry.second)
                continue;
            std::filesystem::path dirname = getConfigPath() / entry.first;
            if (!Filesystem::mkdir_recursive(dirname))
                con.printerr("Failed to create config directory: '{}'\n", dirname);
        }

        // copy files from the default tree that don't already exist in the config tree
        for (auto &entry : default_config_files) {
            // skip over directories
            if (entry.second)
                continue;
            std::filesystem::path filename = entry.first;
            if (!config_files.contains(filename)) {
                std::filesystem::path src_file = getConfigDefaultsPath() / filename;
                if (!Filesystem::isfile(src_file))
                    continue;
                std::filesystem::path dest_file = getConfigPath() / filename;
                std::ifstream src(src_file, std::ios::binary);
                std::ofstream dest(dest_file, std::ios::binary);
                if (!src.good() || !dest.good()) {
                    con.printerr("Copy failed: '{}'\n", filename);
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
    // Calls InitCoreContext after checking IsCoreContext
    State = luaL_newstate();
    State = Lua::Open(con, State);
    if (!State)
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
    this->hotkey_mgr = new HotkeyManager();

    auto *temp = new IODATA;
    temp->core = this;
    temp->plug_mgr = plug_mgr;

    if (!is_text_mode || is_headless)
    {
        std::cerr << "Starting IO thread.\n";
        // create IO thread
        d->iothread = std::thread{fIOthread, temp};
    }
    else
    {
        std::cerr << "Starting dfhack.init thread.\n";
        d->iothread = std::thread{fInitthread, temp};
    }

    std::cerr << "Starting DF input capture thread.\n";
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
                size_t next = raw.find('"', offset);
                args.push_back(raw.substr(offset, next - offset));
                offset = next + 2;
            }
            else
            {
                size_t next = raw.find(' ', offset);
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
                for (const auto& arg : args)
                {
                    if (!arg.empty() && arg[0] == '+')
                    {
                        break;
                    }
                    cmd.push_back(arg);
                }

                if (runCommand(con, first.substr(1), cmd) != CR_OK)
                {
                    std::cerr << "Error running command: " << first.substr(1);
                    for (const auto& arg : cmd)
                    {
                        std::cerr << " \"" << arg << "\"";
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
    df::viewscreen *screen = nullptr;
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
    df::world_data* new_wdata = nullptr;
    df::map_block**** new_mapdata = nullptr;
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
        unpaused_ms += perf_counters.registerTick(start_ms);
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

static void getFilesWithPrefixAndSuffix(const std::filesystem::path& folder, const std::string& prefix, const std::string& suffix, std::vector<std::filesystem::path>& result) {
    std::vector<std::filesystem::path> files;
    DFHack::Filesystem::listdir(folder, files);
    for ( const auto& f : files) {
        if (f.stem().string().starts_with(prefix) && f.extension() == suffix)
            result.push_back(f);
    }
}

size_t loadScriptFiles(Core* core, color_ostream& out, const std::span<const std::string> prefix, const std::filesystem::path& folder) {
    static const std::string suffix = ".init";
    std::vector<std::filesystem::path> scriptFiles;
    for ( const auto& p : prefix ) {
        getFilesWithPrefixAndSuffix(folder, p, ".init", scriptFiles);
    }

    std::ranges::sort(scriptFiles,
                      [](const std::filesystem::path& a, const std::filesystem::path& b) {
                          return a.stem() < b.stem();
                      });

    size_t result = 0;
    for ( const auto& file : scriptFiles) {
        result++;
        core->loadScriptFile(out, folder / file, false);
    }
    return result;
}

namespace DFHack {
    namespace X {
        using Key = state_change_event;
        using Val = std::vector<std::string>;
        using Entry = std::pair<Key,Val>;
        using EntryVector =  std::vector<Entry>;
        using InitVariationTable = std::map<Key,Val>;

        template <typename T>
        concept VariationTableTypes = std::same_as<T, Key> || std::convertible_to<T, std::string_view>;

        template <VariationTableTypes... Ts>
        static EntryVector computeInitVariationTable(Ts&&... ts) {
            using Arg = std::variant<Key, std::string_view>;
            std::vector<Arg> args{ Arg{std::forward<Ts>(ts)}... };

            Key current_key = SC_UNKNOWN;
            EntryVector result;
            Val val;
            for (auto& arg : args) {
                if (std::holds_alternative<Key>(arg)) {
                    if (current_key != SC_UNKNOWN && !val.empty()) {
                        result.emplace_back(current_key, val);
                        val.clear();
                    }
                    current_key = std::get<Key>(arg);
                } else if (std::holds_alternative<std::string_view>(arg)) {
                    auto str = std::get<std::string_view>(arg);
                    if (!str.empty())
                        val.emplace_back(str);
                }
            }

            return result;
        }

        static InitVariationTable getTable(const EntryVector& vec) {
            return InitVariationTable(vec.begin(),vec.end());
        }
    }
}

void Core::handleLoadAndUnloadScripts(color_ostream& out, state_change_event event) {
    static const X::InitVariationTable table = X::getTable(X::computeInitVariationTable(
        SC_WORLD_LOADED, "onLoad", "onLoadWorld", "onWorldLoaded",
        SC_WORLD_UNLOADED, "onUnload", "onUnloadWorld", "onWorldUnloaded",
        SC_MAP_LOADED, "onMapLoad", "onLoadMap",
        SC_MAP_UNLOADED, "onMapUnload", "onUnloadMap",
        SC_UNKNOWN
    ));

    if (!df::global::world)
        return;

    std::filesystem::path rawFolder = !isWorldLoaded() ? Filesystem::getInstallDir() : Filesystem::getBaseDir() / "save" / World::ReadWorldFolder() / "init";

    auto i = table.find(event);
    if ( i != table.end() ) {
        const std::vector<std::string>& set = i->second;

        // load baseline defaults
        this->loadScriptFile(out, getConfigPath() / "init" / ("default." + set[0] + ".init"), false);

        loadScriptFiles(this, out, set, getConfigPath() / "init");
        loadScriptFiles(this, out, set, rawFolder);
    }

    for (const auto& script : state_change_scripts)
    {
        if (script.event == event)
        {
            if (!script.save_specific)
            {
                loadScriptFile(out, script.path, false);
            }
            else if (script.save_specific && isWorldLoaded())
            {
                loadScriptFile(out, rawFolder / script.path, false);
            }
        }
    }
}

void Core::onStateChange(color_ostream &out, state_change_event event)
{
    using df::global::gametype;
    static md5wrapper md5w;
    static std::string ostype;

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
        unpaused_ms = 0;
        Persistence::Internal::load(out);
        plug_mgr->doLoadWorldData(out);
        loadModScriptPaths(out);
        auto L = DFHack::Core::getInstance().getLuaState();
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
                    out.printerr("Could not append to {}\n", evtlogpath);
            }
            else
            {
                char timebuf[30];
                time_t rawtime = time(nullptr);
                struct tm * timeinfo = localtime(&rawtime);
                strftime(timebuf, sizeof(timebuf), "[%Y-%m-%dT%H:%M:%S%z] ", timeinfo);
                evtlog << timebuf;
                evtlog << "DFHack " << Version::git_description() << " on " << ostype << "; ";
                evtlog << "cwd md5: " << md5w.getHashFromString(getHackPath().string().c_str()).substr(0, 10) << "; ";
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
    #ifdef LINUX_BUILD
        extern void dfhack_crashlog_shutdown();
        dfhack_crashlog_shutdown();
    #endif

    if(errorstate)
        return true;
    errorstate = 1;

    shutdown = true;

    // Make sure the console thread shutdowns before clean up to avoid any
    // unlikely data races.
    if (d->iothread.joinable()) {
        con.shutdown();
    }

    ServerMain::block();

    d->iothread.join();

    if (hotkey_mgr) {
        delete hotkey_mgr;
    }

    if(plug_mgr)
    {
        delete plug_mgr;
        plug_mgr = nullptr;
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

bool Core::getSuppressDuplicateKeyboardEvents() const {
    return suppress_duplicate_keyboard_events;
}

void Core::setSuppressDuplicateKeyboardEvents(bool suppress) {
    DEBUG(keybinding).print("setting suppress_duplicate_keyboard_events to {}\n",
        suppress);
    suppress_duplicate_keyboard_events = suppress;
}

void Core::setMortalMode(bool value) {
    mortal_mode.exchange(value);
}

bool Core::getMortalMode() {
    return mortal_mode.load();
}

void Core::setArmokTools(const std::vector<std::string> &tool_names) {
    std::lock_guard<std::mutex> lock(armok_mutex);
    armok_tools.clear();
    armok_tools.insert(tool_names.begin(), tool_names.end());
}

bool Core::isArmokTool(const std::string& tool) {
    std::lock_guard<std::mutex> lock(armok_mutex);
    return armok_tools.contains(tool);
}

// returns true if the event is handled
bool Core::DFH_SDL_Event(SDL_Event* ev) {
    uint32_t start_ms = p->getTickCount();
    bool ret = doSdlInputEvent(ev);
    perf_counters.incCounter(perf_counters.total_keybinding_ms, start_ms);
    return ret;
}

void Core::DFH_SDL_Loop() {
    DFHack::runRenderThreadCallbacks();
}

bool Core::doSdlInputEvent(SDL_Event* ev)
{
    // this should only ever be called from the render thread
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
        else if (sym == SDLK_LGUI || sym == SDLK_RGUI) // Renamed to LMETA/RMETA in SDL3
            modstate = (ev->type == SDL_KEYDOWN) ? modstate | DFH_MOD_SUPER : modstate & ~DFH_MOD_SUPER;
        else if (ke.state == SDL_PRESSED && !hotkey_states[sym])
        {
            // the check against hotkey_states[sym] ensures we only process keybindings once per keypress
            DEBUG(keybinding).print("key down: sym={}, char={}\n", sym, static_cast<char>(sym));
            if ((hotkey_mgr->handleKeybind(sym, modstate))) {
                hotkey_states[sym] = true;
                if (modstate & (DFH_MOD_CTRL | DFH_MOD_ALT)) {
                    DEBUG(keybinding).print("modifier key detected; not inhibiting SDL key down event\n");
                    return false;
                }
                DEBUG(keybinding).print("{}inhibiting SDL key down event\n",
                    suppress_duplicate_keyboard_events ? "" : "not ");
                return suppress_duplicate_keyboard_events;
            }
        }
        else if (ke.state == SDL_RELEASED)
        {
            DEBUG(keybinding).print("key up: sym={}, char={}\n", sym, static_cast<char>(sym));
            hotkey_states[sym] = false;
        }
    } else if (ev->type == SDL_MOUSEBUTTONDOWN) {
        auto &but = ev->button;
        DEBUG(keybinding).print("mouse button down: button={}\n", but.button);
        // don't mess with the first three buttons, which are critical elements of DF's control scheme
        if (but.button > 3) {
            // We represent mouse buttons as a negative number, permitting buttons 4-15
            SDL_Keycode sym = -but.button;
            if (sym >= -15 && sym <= -4  && hotkey_mgr->handleKeybind(sym, modstate))
                return suppress_duplicate_keyboard_events;
        }
    } else if (ev->type == SDL_TEXTINPUT) {
        auto &te = ev->text;
        DEBUG(keybinding).print("text input: '{}' (modifiers: {}{}{})\n",
            te.text,
            modstate & DFH_MOD_SHIFT ? "Shift" : "",
            modstate & DFH_MOD_CTRL ? "Ctrl" : "",
            modstate & DFH_MOD_ALT ? "Alt" : "");
        if (strlen(te.text) == 1 && hotkey_states[te.text[0]]) {
            DEBUG(keybinding).print("{}inhibiting SDL text event\n",
                suppress_duplicate_keyboard_events ? "" : "not ");
            return suppress_duplicate_keyboard_events;
        }
    }

    return false;
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
    return aliases.contains(name);
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

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/

#define MODULE_GETTER(TYPE) \
TYPE * Core::get##TYPE() \
{ \
    if(errorstate) return nullptr;\
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
