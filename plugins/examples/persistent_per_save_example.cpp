// This template is appropriate for plugins that periodically check game state
// and make some sort of automated change. These types of plugins typically
// provide a command that can be used to configure the plugin behavior and
// require a world to be loaded before they can function. This kind of plugin
// should persist its state in the savegame and auto-re-enable itself when a
// savegame that had this plugin enabled is loaded.

#include <string>
#include <vector>

#include "df/world.h"

#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Persistence.h"
#include "modules/World.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("persistent_per_save_example");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(persistent_per_save_example, status, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(persistent_per_save_example, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_CYCLE_TICKS = 1,
};
static int get_config_val(int index) {
    if (!config.isValid())
        return -1;
    return config.ival(index);
}
static bool get_config_bool(int index) {
    return get_config_val(index) == 1;
}
static void set_config_val(int index, int value) {
    if (config.isValid())
        config.ival(index) = value;
}
static void set_config_bool(int index, bool value) {
    set_config_val(index, value ? 1 : 0);
}

static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

// define the structure that will represent the possible commandline options
struct command_options {
    // whether to display help
    bool help = false;

    // whether to run a cycle right now
    bool now = false;

    // how many ticks to wait between cycles when enabled, -1 means unset
    int32_t ticks = -1;

    // example params of different types
    df::coord start;
    string format;
    vector<string*> list; // note this must be a vector of pointers, not objects

    static struct_identity _identity;
};
static const struct_field_info command_options_fields[] = {
    { struct_field_info::PRIMITIVE,      "help",   offsetof(command_options, help),  &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "now",    offsetof(command_options, now),   &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE,      "ticks",  offsetof(command_options, ticks), &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::SUBSTRUCT,      "start",  offsetof(command_options, start), &df::coord::_identity,                   0, 0 },
    { struct_field_info::PRIMITIVE,      "format", offsetof(command_options, format), df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::STL_VECTOR_PTR, "list",   offsetof(command_options, list),   df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::END }
};
struct_identity command_options::_identity(sizeof(command_options), &df::allocator_fn<command_options>, NULL, "command_options", NULL, command_options_fields);

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(status,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Short (~54 character) description of command.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(status,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
    } else {
        DEBUG(status,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(status,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(status,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
        set_config_val(CONFIG_CYCLE_TICKS, 6000);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = get_config_bool(CONFIG_IS_ENABLED);
    DEBUG(status,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_MAP_UNLOADED) {
        if (is_enabled) {
            DEBUG(status,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= get_config_val(CONFIG_CYCLE_TICKS))
        do_cycle(out);
    return CR_OK;
}

// load the lua module associated with the plugin and parse the commandline
// in lua (which has better facilities than C++ for string parsing).
static bool get_options(color_ostream &out,
                        command_options &opts,
                        const vector<string> &parameters)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, ("plugins." + string(plugin_name)).c_str(),
            "parse_commandline")) {
        out.printerr("Failed to load %s Lua code\n", plugin_name);
        return false;
    }

    Lua::Push(L, &opts);
    for (const string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    // be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    command_options opts;
    if (!get_options(out, opts, parameters) || opts.help)
        return CR_WRONG_USAGE;

    if (opts.ticks > -1) {
        set_config_val(CONFIG_CYCLE_TICKS, opts.ticks);
        INFO(status,out).print("New cycle timer: %d ticks.\n", opts.ticks);
    }
    else if (opts.now) {
        do_cycle(out);
    }
    else {
        out.print("%s is %srunning\n", plugin_name, (is_enabled ? "" : "not "));
    }
    return CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // TODO: logic that runs every get_config_val(CONFIG_CYCLE_TICKS) ticks
}
