// Dynamically enables and disables cooking restrictions for plants and seeds
// in order to limit the number of seeds available for each crop type

// With thanks to peterix for DFHack and Quietust for information
// http://www.bay12forums.com/smf/index.php?topic=91166.msg2605147#msg2605147

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Items.h"
#include "modules/Kitchen.h"
#include "modules/Maps.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/item_flags.h"
#include "df/items_other_id.h"
#include "df/plant_raw.h"
#include "df/world.h"

#include <unordered_map>

using namespace DFHack;
using namespace df::enums;

using std::map;
using std::string;
using std::unordered_map;
using std::vector;

DFHACK_PLUGIN("seedwatch");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(seedwatch, config, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(seedwatch, cycle, DebugCategory::LINFO);
}

// abbreviations for the standard plants
static unordered_map<string, const char *> abbreviations;
static map<string, int32_t> world_plant_ids;
static const int DEFAULT_TARGET = 30;
static const int TARGET_BUFFER = 20; // seed number buffer; 20 is reasonable

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string SEED_CONFIG_KEY_PREFIX = string(plugin_name) + "/seed/";
static PersistentDataItem config;
static unordered_map<int, PersistentDataItem> watched_seeds;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

enum SeedConfigValues {
    SEED_CONFIG_ID = 0,
    SEED_CONFIG_TARGET = 1,
};

static int get_config_val(PersistentDataItem &c, int index) {
    if (!c.isValid())
        return -1;
    return c.ival(index);
}
static bool get_config_bool(PersistentDataItem &c, int index) {
    return get_config_val(c, index) == 1;
}
static void set_config_val(PersistentDataItem &c, int index, int value) {
    if (c.isValid())
        c.ival(index) = value;
}
static void set_config_bool(PersistentDataItem &c, int index, bool value) {
    set_config_val(c, index, value ? 1 : 0);
}

static PersistentDataItem & ensure_seed_config(color_ostream &out, int id) {
    if (watched_seeds.count(id))
        return watched_seeds[id];
    string keyname = SEED_CONFIG_KEY_PREFIX + int_to_string(id);
    DEBUG(config,out).print("creating new persistent key for seed type %d\n", id);
    watched_seeds.emplace(id, World::GetPersistentData(keyname, NULL));
    set_config_val(watched_seeds[id], SEED_CONFIG_ID, id);
    return watched_seeds[id];
}
static void remove_seed_config(color_ostream &out, int id) {
    if (!watched_seeds.count(id))
        return;
    DEBUG(config,out).print("removing persistent key for seed type %d\n", id);
    World::DeletePersistentData(watched_seeds[id]);
    watched_seeds.erase(id);
}

static const int32_t CYCLE_TICKS = 1200;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out, int32_t *num_enabled_seeds = NULL, int32_t *num_disabled_seeds = NULL);
static void seedwatch_setTarget(color_ostream &out, string name, int32_t num);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(config,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically toggle seed cooking based on quantity available.",
        do_command));

    // fill in the abbreviations map
    abbreviations["bs"] = "SLIVER_BARB";
    abbreviations["bt"] = "TUBER_BLOATED";
    abbreviations["bw"] = "WEED_BLADE";
    abbreviations["cw"] = "GRASS_WHEAT_CAVE";
    abbreviations["dc"] = "MUSHROOM_CUP_DIMPLE";
    abbreviations["fb"] = "BERRIES_FISHER";
    abbreviations["hr"] = "ROOT_HIDE";
    abbreviations["kb"] = "BULB_KOBOLD";
    abbreviations["lg"] = "GRASS_LONGLAND";
    abbreviations["mr"] = "ROOT_MUCK";
    abbreviations["pb"] = "BERRIES_PRICKLE";
    abbreviations["ph"] = "MUSHROOM_HELMET_PLUMP";
    abbreviations["pt"] = "GRASS_TAIL_PIG";
    abbreviations["qb"] = "BUSH_QUARRY";
    abbreviations["rr"] = "REED_ROPE";
    abbreviations["rw"] = "WEED_RAT";
    abbreviations["sb"] = "BERRY_SUN";
    abbreviations["sp"] = "POD_SWEET";
    abbreviations["vh"] = "HERB_VALLEY";
    abbreviations["ws"] = "BERRIES_STRAW";
    abbreviations["wv"] = "VINE_WHIP";

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(config,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(config,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(config,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    cycle_timestamp = 0;

    world_plant_ids.clear();
    for (size_t i = 0; i < world->raws.plants.all.size(); ++i) {
        auto & plant = world->raws.plants.all[i];
        if (plant->material_defs.type[plant_material_def::seed] != -1)
            world_plant_ids[plant->id] = i;
    }

    watched_seeds.clear();
    vector<PersistentDataItem> seed_configs;
    World::GetPersistentData(&seed_configs, SEED_CONFIG_KEY_PREFIX, true);
    const size_t num_seed_configs = seed_configs.size();
    for (size_t idx = 0; idx < num_seed_configs; ++idx) {
        auto &c = seed_configs[idx];
        watched_seeds.emplace(get_config_val(c, SEED_CONFIG_ID), c);
    }

    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(config,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
        seedwatch_setTarget(out, "all", DEFAULT_TARGET);
    }

    is_enabled = get_config_bool(config, CONFIG_IS_ENABLED);
    DEBUG(config,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");
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
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
        int32_t num_enabled_seeds, num_disabled_seeds;
        do_cycle(out, &num_enabled_seeds, &num_disabled_seeds);
        if (0 < num_enabled_seeds)
            out.print("%s: enabled %d seed types for cooking\n",
                    plugin_name, num_enabled_seeds);
        if (0 < num_disabled_seeds)
            out.print("%s: protected %d seed types from cooking\n",
                    plugin_name, num_disabled_seeds);
    }
    return CR_OK;
}

static bool call_seedwatch_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    DEBUG(config,*out).print("calling %s lua function: '%s'\n", plugin_name, fn_name);

    return Lua::CallLuaModuleFunction(*out, L, "plugins.seedwatch", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_seedwatch_lua(&out, "parse_commandline", parameters.size(), 1,
            [&](lua_State *L) {
                for (const string &param : parameters)
                    Lua::Push(L, param);
            },
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

struct BadFlags {
    uint32_t whole;

    BadFlags() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
        F(in_job); F(owned); F(in_chest); F(removed);
        F(encased); F(spider_web);
        #undef F
        whole = flags.whole;
    }
};

static bool is_accessible_item(df::item *item, const vector<df::unit *> &citizens) {
    const df::coord pos = Items::getPosition(item);
    for (auto &unit : citizens) {
        if (Maps::canWalkBetween(Units::getPosition(unit), pos))
            return true;
    }
    return false;
}

static void scan_seeds(color_ostream &out, unordered_map<int32_t, int32_t> *accessible_counts,
        unordered_map<int32_t, int32_t> *inaccessible_counts = NULL) {
    static const BadFlags bad_flags;

    vector<df::unit *> citizens;
    Units::getCitizens(citizens);

    for (auto &item : world->items.other[items_other_id::SEEDS]) {
        MaterialInfo mat(item);
        if (mat.plant->index < 0 || !mat.isPlant())
            continue;
        if ((bad_flags.whole & item->flags.whole) || !is_accessible_item(item, citizens)) {
            if (inaccessible_counts)
                ++(*inaccessible_counts)[mat.plant->index];
        } else {
            if (accessible_counts)
                ++(*accessible_counts)[mat.plant->index];
        }
    }
}

static void do_cycle(color_ostream &out, int32_t *num_enabled_seed_types, int32_t *num_disabled_seed_types) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    if (num_enabled_seed_types)
        *num_enabled_seed_types = 0;
    if (num_disabled_seed_types)
        *num_disabled_seed_types = 0;

    unordered_map<int32_t, int32_t> accessible_counts;
    scan_seeds(out, &accessible_counts);

    for (auto &entry : watched_seeds) {
        int32_t id = entry.first;
        if (id < 0 || (size_t)id >= world->raws.plants.all.size())
            continue;
        int32_t target = get_config_val(entry.second, SEED_CONFIG_TARGET);
        if (accessible_counts[id] <= target &&
                (Kitchen::isPlantCookeryAllowed(id) ||
                 Kitchen::isSeedCookeryAllowed(id))) {
            DEBUG(cycle,out).print("disabling seed mat: %d\n", id);
            if (num_disabled_seed_types)
                ++*num_disabled_seed_types;
            Kitchen::denyPlantSeedCookery(id);
        } else if (target + TARGET_BUFFER < accessible_counts[id] &&
                (!Kitchen::isPlantCookeryAllowed(id) ||
                 !Kitchen::isSeedCookeryAllowed(id))) {
            DEBUG(cycle,out).print("enabling seed mat: %d\n", id);
            if (num_enabled_seed_types)
                ++*num_enabled_seed_types;
            Kitchen::allowPlantSeedCookery(id);
        }
    }
}

/////////////////////////////////////////////////////
// Lua API
// core will already be suspended when coming in through here
//

static void set_target(color_ostream &out, int32_t id, int32_t target) {
    if (target == 0)
        remove_seed_config(out, id);
    else {
        if (id < 0 || (size_t)id >= world->raws.plants.all.size()) {
            WARN(config,out).print(
                    "cannot set target for unknown plant id: %d\n", id);
            return;
        }
        PersistentDataItem &c = ensure_seed_config(out, id);
        set_config_val(c, SEED_CONFIG_TARGET, target);
    }
}

// searches abbreviations, returns expansion if so, returns original if not
static string searchAbbreviations(string in) {
    if(abbreviations.count(in) > 0)
        return abbreviations[in];
    return in;
};

static void seedwatch_setTarget(color_ostream &out, string name, int32_t num) {
    DEBUG(config,out).print("entering seedwatch_setTarget\n");

    if (num < 0) {
        out.printerr("target must be at least 0\n");
        return;
    }

    if (name == "all") {
        for (auto &entry : world_plant_ids) {
            set_target(out, entry.second, num);
        }
        return;
    }

    string token = searchAbbreviations(name);
    if (!world_plant_ids.count(token)) {
        out.printerr("%s has not been found as a material.\n", token.c_str());
        return;
    }

    set_target(out, world_plant_ids[token], num);
}

static int seedwatch_getData(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(config,*out).print("entering seedwatch_getData\n");
    unordered_map<int32_t, int32_t> watch_map, accessible_counts;
    scan_seeds(*out, &accessible_counts);
    for (auto &entry : watched_seeds) {
        watch_map.emplace(entry.first, get_config_val(entry.second, SEED_CONFIG_TARGET));
    }
    Lua::Push(L, watch_map);
    Lua::Push(L, accessible_counts);
    return 2;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(seedwatch_setTarget),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(seedwatch_getData),
    DFHACK_LUA_END
};
