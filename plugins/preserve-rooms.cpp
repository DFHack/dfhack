#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
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

// zone id -> unit ids (includes spouses)
static std::map<int32_t, vector<int32_t>> last_known_assignments;
// unit id -> zone ids
static std::map<int32_t, vector<int32_t>> pending_reassignment;

// as a "system" plugin, we do not persist plugin enabled state
enum ConfigValues {
    CONFIG_TRACK_RAIDS = 0,
    CONFIG_NOBLE_ROLES = 1,
};

static const int32_t CYCLE_TICKS = 109;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);
    commands.push_back(PluginCommand(
        plugin_name,
        "Manage room assignments for off-map units and noble roles.",
        do_command));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    is_enabled = enable;
    DEBUG(control, out).print("now %s\n", is_enabled ? "enabled" : "disabled");
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
        config.set_bool(CONFIG_TRACK_RAIDS, false);
        config.set_bool(CONFIG_NOBLE_ROLES, true);
    }

    last_known_assignments.clear();
    pending_reassignment.clear();

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
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
        return CR_OK;
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!World::isFortressMode() || !Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.preserve-rooms", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static void do_cycle(color_ostream &out) {
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);
}

/////////////////////////////////////////////////////
// Lua API
//

static void preserve_rooms_cycle(color_ostream &out) {
    DEBUG(control,out).print("entering preserve_rooms_cycle\n");
    do_cycle(out);
}

static bool preserve_rooms_setFeature(color_ostream &out, bool enabled, string feature) {
    DEBUG(control,out).print("entering preserve_rooms_setFeature (enabled=%d, feature=%s)\n",
        enabled, feature.c_str());
    if (feature == "track-raids") {
        config.set_bool(CONFIG_TRACK_RAIDS, enabled);
        if (is_enabled && enabled)
            do_cycle(out);
    } else if (feature == "noble-roles") {
        config.set_bool(CONFIG_NOBLE_ROLES, enabled);
    } else {
        return false;
    }

    return true;
}

static bool preserve_rooms_resetFeatureState(color_ostream &out, string feature) {
    DEBUG(control,out).print("entering preserve_rooms_resetFeatureState (feature=%s)\n", feature.c_str());
    if (feature == "track-raids") {
        // TODO: unpause reserved rooms
        last_known_assignments.clear();
        pending_reassignment.clear();
    } else if (feature == "noble-roles") {
        // TODO: reset state
    } else {
        return false;
    }

    return true;
}

static int preserve_rooms_getState(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering preserve_rooms_getState\n");

    unordered_map<string, bool> features;
    features.emplace("track-raids", config.get_bool(CONFIG_TRACK_RAIDS));
    features.emplace("noble-roles", config.get_bool(CONFIG_NOBLE_ROLES));
    Lua::Push(L, features);

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(preserve_rooms_cycle),
    DFHACK_LUA_FUNCTION(preserve_rooms_setFeature),
    DFHACK_LUA_FUNCTION(preserve_rooms_resetFeatureState),
    DFHACK_LUA_END};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(preserve_rooms_getState),
    DFHACK_LUA_END};
