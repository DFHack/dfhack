#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"

#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/world.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("buildingplan");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(buildingplan, status, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(buildingplan, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_BLOCKS = 1,
    CONFIG_BOULDERS = 2,
    CONFIG_LOGS = 3,
    CONFIG_BARS = 4,
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

static const int32_t CYCLE_TICKS = 600; // twice per game day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(status,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Plan building placement before you have materials.",
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
        set_config_bool(CONFIG_BLOCKS, true);
        set_config_bool(CONFIG_BOULDERS, true);
        set_config_bool(CONFIG_LOGS, true);
        set_config_bool(CONFIG_BARS, false);
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
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(status,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    // be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    // TODO: configuration logic
    // simple commandline parsing can be done in C++, but there are lua libraries
    // that can easily handle more complex commandlines. see the blueprint plugin
    // for an example.

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
