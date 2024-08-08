// This template is appropriate for plugins that periodically check game state
// and make some sort of automated change. These types of plugins typically
// provide a command that can be used to configure the plugin behavior and
// require a map to be loaded before they can function. This kind of plugin
// should persist its state in the savegame and auto-re-enable itself when a
// savegame that had this plugin enabled is loaded.

#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"

#include "modules/World.h"

#include "df/world.h"

#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("persistent_per_save_example");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
// the names "config" and "cycle" are arbitrary and are just used to categorize
// your log messages.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(persistent_per_save_example, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(persistent_per_save_example, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string ELEM_CONFIG_KEY_PREFIX = string(plugin_name) + "/elem/";
static PersistentDataItem config;
static unordered_map<int, PersistentDataItem> elems;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_SOMETHING_ELSE = 1,
};

enum ElemConfigValues {
    ELEM_CONFIG_ID = 0,
    ELEM_CONFIG_SOMETHING_ELSE = 1,
};

static PersistentDataItem & ensure_elem_config(color_ostream &out, int id) {
    if (elems.count(id))
        return elems[id];
    string keyname = ELEM_CONFIG_KEY_PREFIX + int_to_string(id);
    DEBUG(control,out).print("creating new persistent key for elem id %d\n", id);
    elems.emplace(id, World::GetPersistentSiteData(keyname, true));
    return elems[id];
}
static void remove_elem_config(color_ostream &out, int id) {
    if (!elems.count(id))
        return;
    DEBUG(control,out).print("removing persistent key for elem id %d\n", id);
    World::DeletePersistentData(elems[id]);
    elems.erase(id);
}

// When choosing a cycle timer, think about performance -- how often do you
// really need to run? Also, don't choose exactly one day or hour. Choose a
// number of ticks close to your target, but choose it from this list of prime
// numbers: https://www-users.york.ac.uk/~ss44/cyc/p/prime100.htm
// Do a search in the codebase for CYCLE_TICKS and try to choose a number that
// isn't there yet. This will help prevent too many tools from running on the
// same tick and causing noticeable FPS stuttering.
static const int32_t CYCLE_TICKS = 1217; // about one day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(config,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Short (~54 character) description of command.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(config,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(config,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_int(CONFIG_SOMETHING_ELSE, 6000);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(config,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    // load other config elements, if applicable
    elems.clear();
    vector<PersistentDataItem> elem_configs;
    World::GetPersistentSiteData(&elem_configs, ELEM_CONFIG_KEY_PREFIX, true);
    const size_t num_elem_configs = elem_configs.size();
    for (size_t idx = 0; idx < num_elem_configs; ++idx) {
        auto &c = elem_configs[idx];
        elems.emplace(c.get_int(ELEM_CONFIG_ID), c);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(config,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    // be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    // TODO: configuration logic
    // simple commandline parsing can be done in C++, but there are lua libraries
    // that can easily handle more complex commandlines. see the seedwatch plugin
    // for a simple example.

    return CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // TODO: logic that runs every CYCLE_TICKS ticks
}
