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

DFHACK_PLUGIN("preserve-rooms");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(persistent_per_save_example, control, DebugCategory::LINFO);
    DBG_DECLARE(persistent_per_save_example, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

static std::map<int32_t, int32_t> last_known_assignments;
static std::map<int32_t, vector<int32_t>> pending_reassignment;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

static const int32_t CYCLE_TICKS = 109;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);
    commands.push_back(PluginCommand(
        plugin_name,
        "Prevent room assignments from being lost.",
        do_command));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!World::isFortressMode() || !Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
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
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                             is_enabled ? "true" : "false");

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
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
    CoreSuspender suspend;

    if (!World::isFortressMode() || !Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
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
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
}
