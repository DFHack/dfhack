// automatically chop trees

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Burrows.h"
#include "modules/Designations.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/burrow.h"
#include "df/general_ref.h"
#include "df/item.h"
#include "df/map_block.h"
#include "df/material.h"
#include "df/plant.h"
#include "df/plant_raw.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

#include <map>
#include <unordered_map>

using std::map;
using std::multimap;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("autochop");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(autochop, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(autochop, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string BURROW_CONFIG_KEY_PREFIX = string(plugin_name) + "/burrow/";
static PersistentDataItem config;
static vector<PersistentDataItem> watched_burrows;
static unordered_map<int, size_t> watched_burrows_indices;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_MAX_LOGS = 1,
    CONFIG_MIN_LOGS = 2,
    CONFIG_WAITING_FOR_MIN = 3,
};

enum BurrowConfigValues {
    BURROW_CONFIG_ID = 0,
    BURROW_CONFIG_CHOP = 1,
    BURROW_CONFIG_CLEARCUT = 2,
    BURROW_CONFIG_PROTECT_BREWABLE = 3,
    BURROW_CONFIG_PROTECT_EDIBLE = 4,
    BURROW_CONFIG_PROTECT_COOKABLE = 5,
};

static PersistentDataItem & ensure_burrow_config(color_ostream &out, int id) {
    if (watched_burrows_indices.count(id))
        return watched_burrows[watched_burrows_indices[id]];
    string keyname = BURROW_CONFIG_KEY_PREFIX + int_to_string(id);
    DEBUG(control,out).print("creating new persistent key for burrow %d\n", id);
    watched_burrows.emplace_back(World::GetPersistentSiteData(keyname, true));
    size_t idx = watched_burrows.size()-1;
    watched_burrows_indices.emplace(id, idx);
    return watched_burrows[idx];
}
static void remove_burrow_config(color_ostream &out, int id) {
    if (!watched_burrows_indices.count(id))
        return;
    DEBUG(control,out).print("removing persistent key for burrow %d\n", id);
    size_t idx = watched_burrows_indices[id];
    World::DeletePersistentData(watched_burrows[idx]);
    watched_burrows.erase(watched_burrows.begin()+idx);
    watched_burrows_indices.erase(id);
}
static void validate_burrow_configs(color_ostream &out) {
    for (int32_t idx = watched_burrows.size()-1; idx >=0; --idx) {
        int id = watched_burrows[idx].get_int(BURROW_CONFIG_ID);
        if (!df::burrow::find(id)) {
            remove_burrow_config(out, id);
        }
    }
}

static const int32_t CYCLE_TICKS = 1181;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static int32_t do_cycle(color_ostream &out, bool force_designate = false);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Auto-harvest trees when low on stockpiled logs.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out, true);
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
        config.set_int(CONFIG_MAX_LOGS, 200);
        config.set_int(CONFIG_MIN_LOGS, 160);
        config.set_bool(CONFIG_WAITING_FOR_MIN, false);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");
    World::GetPersistentSiteData(&watched_burrows, BURROW_CONFIG_KEY_PREFIX, true);
    watched_burrows_indices.clear();
    const size_t num_watched_burrows = watched_burrows.size();
    for (size_t idx = 0; idx < num_watched_burrows; ++idx) {
        auto &c = watched_burrows[idx];
        watched_burrows_indices.emplace(c.get_int(BURROW_CONFIG_ID), idx);
    }
    validate_burrow_configs(out);

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
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
        int32_t designated = do_cycle(out);
        if (0 < designated)
            out.print("autochop: designated %d tree(s) for chopping\n", designated);
    }
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.autochop", "parse_commandline", parameters,
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

static bool is_accessible_item(df::item *item, const vector<df::unit *> &citizens) {
    const df::coord pos = Items::getPosition(item);
    for (auto &unit : citizens) {
        if (Maps::canWalkBetween(Units::getPosition(unit), pos))
            return true;
    }
    return false;
}

// at least one member of the fort can reach a position adjacent to the given pos
static bool is_accessible_tree(const df::coord &pos, const vector<df::unit *> &citizens) {
    for (auto &unit : citizens) {
        if (Maps::canWalkBetween(unit->pos, df::coord(pos.x-1, pos.y-1, pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x,   pos.y-1, pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x+1, pos.y-1, pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x-1, pos.y,   pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x+1, pos.y,   pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x-1, pos.y+1, pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x,   pos.y+1, pos.z))
                || Maps::canWalkBetween(unit->pos, df::coord(pos.x+1, pos.y+1, pos.z)))
            return true;
    }
    return false;
}

static bool is_valid_tree(const df::plant *plant) {
    // Skip all non-trees immediately.
    if (!plant->tree_info)
        return false;

    // Skip plants with invalid tile.
    df::map_block *block = Maps::getTileBlock(plant->pos);
    if (!block)
        return false;

    int x = plant->pos.x % 16;
    int y = plant->pos.y % 16;

    // Skip all unrevealed plants.
    if (block->designation[x][y].bits.hidden)
        return false;

    if (tileMaterial(block->tiletype[x][y]) != tiletype_material::TREE)
        return false;

    return true;
}

static bool is_protected(const df::plant * plant, PersistentDataItem &c) {
    const df::plant_raw *plant_raw = df::plant_raw::find(plant->material);

    bool protect_brewable = c.get_bool(BURROW_CONFIG_PROTECT_BREWABLE);
    bool protect_edible = c.get_bool(BURROW_CONFIG_PROTECT_EDIBLE);
    bool protect_cookable = c.get_bool(BURROW_CONFIG_PROTECT_COOKABLE);

    if (protect_brewable && plant_raw->material_defs.type[plant_material_def::drink] != -1)
        return true;

    if (protect_edible || protect_cookable) {
        for (df::material * mat : plant_raw->material) {
            if (protect_edible && mat->flags.is_set(material_flags::EDIBLE_RAW))
                return true;

            if (protect_cookable && mat->flags.is_set(material_flags::EDIBLE_COOKED))
                return true;
        }
    }

    return false;
}

static int32_t estimate_logs(const df::plant *plant) {
    if (!plant->tree_info)
        return 0;

    df::plant_tree_tile** tiles = plant->tree_info->body;

    if (!tiles)
        return 0;

    MaterialInfo mi;
    mi.decode(MaterialInfo::PLANT_BASE, plant->material);
    bool is_shroom = mi.plant->flags.is_set(df::plant_raw_flags::TREE_HAS_MUSHROOM_CAP);

    int32_t trunks = 0, parent_dir = 0;
    const int32_t area = plant->tree_info->dim_y * plant->tree_info->dim_x;
    for (int z = 0; z < plant->tree_info->body_height; z++) {
        df::plant_tree_tile* tilesRow = tiles[z];
        if (!tilesRow)
            return 0; // tree data is corrupt; let's not touch it
        for (int i = 0; i < area; i++) {
            auto & tile = tilesRow[i];
            trunks += tile.bits.trunk;
            parent_dir += (tile.bits.parent_dir == 0) ? 0 : 1;
        }
    }
    return is_shroom ? parent_dir : trunks;
}

static void bucket_tree(df::plant *plant, bool designate_clearcut, bool *designated, bool *can_chop,
        map<int32_t, int32_t> *tree_counts, map<int32_t, int32_t> *designated_tree_counts,
        map<int, PersistentDataItem *> &clearcut_burrows,
        map<int, PersistentDataItem *> &chop_burrows) {
    for (auto &burrow : plotinfo->burrows.list) {
        if (!Burrows::isAssignedTile(burrow, plant->pos))
            continue;

        int id = burrow->id;
        if (tree_counts)
            ++(*tree_counts)[id];

        if (*designated) {
            if (designated_tree_counts)
                ++(*designated_tree_counts)[id];
        } else if (clearcut_burrows.count(id) && !is_protected(plant, *clearcut_burrows[id])) {
            if (designate_clearcut && Designations::markPlant(plant)) {
                *designated = true;
                if (designated_tree_counts)
                    ++(*designated_tree_counts)[id];
            }
        } else if (chop_burrows.count(id) && !is_protected(plant, *chop_burrows[id])) {
            *can_chop = true;
        }
    }

    if (!*designated && chop_burrows.empty())
        *can_chop = true;
}

static void bucket_watched_burrows(color_ostream & out,
        map<int, PersistentDataItem *> &clearcut_burrows,
        map<int, PersistentDataItem *> &chop_burrows) {
    for (auto &c : watched_burrows) {
        int id = c.get_int(BURROW_CONFIG_ID);
        if (c.get_bool(BURROW_CONFIG_CLEARCUT))
            clearcut_burrows.emplace(id, &c);
        else if (c.get_bool(BURROW_CONFIG_CHOP))
            chop_burrows.emplace(id, &c);
    }
}

typedef multimap<int, df::plant *, std::greater<int>> TreesBySize;

static int32_t scan_tree(color_ostream & out, df::plant *plant, int32_t *expected_yield,
        TreesBySize *designatable_trees_by_size, bool designate_clearcut,
        const vector<df::unit *> &citizens, int32_t *accessible_trees,
        int32_t *inaccessible_trees, int32_t *designated_trees, int32_t *accessible_yield,
        map<int32_t, int32_t> *tree_counts,
        map<int32_t, int32_t> *designated_tree_counts,
        map<int, PersistentDataItem *> &clearcut_burrows,
        map<int, PersistentDataItem *> &chop_burrows) {
    TRACE(cycle,out).print("  scanning tree at %d,%d,%d\n",
                            plant->pos.x, plant->pos.y, plant->pos.z);

    if (!is_valid_tree(plant))
        return 0;

    bool accessible = is_accessible_tree(plant->pos, citizens);
    int32_t yield = estimate_logs(plant);

    if (accessible) {
        if (accessible_trees)
            ++*accessible_trees;
        if (accessible_yield)
            *accessible_yield += yield;
    } else {
        if (inaccessible_trees)
            ++*inaccessible_trees;
    }

    bool can_chop = false;
    bool designated = Designations::isPlantMarked(plant);
    bool was_designated = designated;
    bucket_tree(plant, designate_clearcut, &designated, &can_chop, tree_counts,
            designated_tree_counts, clearcut_burrows, chop_burrows);

    int32_t ret = 0;
    if (designated) {
        if (!was_designated)
            ret = 1;
        if (designated_trees)
            ++*designated_trees;
        if (expected_yield)
            *expected_yield += yield;
    } else if (can_chop && accessible) {
        if (designatable_trees_by_size)
            designatable_trees_by_size->emplace(yield, plant);
    }

    return ret;
}

// returns the number of trees that were newly marked
static int32_t scan_trees(color_ostream & out, int32_t *expected_yield,
        TreesBySize *designatable_trees_by_size, bool designate_clearcut,
        const vector<df::unit *> &citizens, int32_t *accessible_trees = NULL,
        int32_t *inaccessible_trees = NULL, int32_t *designated_trees = NULL,
        int32_t *accessible_yield = NULL,
        map<int32_t, int32_t> *tree_counts = NULL,
        map<int32_t, int32_t> *designated_tree_counts = NULL) {
    TRACE(cycle,out).print("scanning trees\n");
    int32_t newly_marked = 0;

    if (accessible_trees)
        *accessible_trees = 0;
    if (inaccessible_trees)
        *inaccessible_trees = 0;
    if (designated_trees)
        *designated_trees = 0;
    if (expected_yield)
        *expected_yield = 0;
    if (accessible_yield)
        *accessible_yield = 0;
    if (tree_counts)
        tree_counts->clear();
    if (designated_tree_counts)
        designated_tree_counts->clear();

    map<int, PersistentDataItem *> clearcut_burrows, chop_burrows;
    bucket_watched_burrows(out, clearcut_burrows, chop_burrows);

    for (auto plant : world->plants.tree_dry)
        newly_marked += scan_tree(out, plant, expected_yield, designatable_trees_by_size,
                                  designate_clearcut, citizens, accessible_trees,
                                  inaccessible_trees, designated_trees, accessible_yield,
                                  tree_counts, designated_tree_counts,
                                  clearcut_burrows, chop_burrows);
    for (auto plant : world->plants.tree_wet)
        newly_marked += scan_tree(out, plant, expected_yield, designatable_trees_by_size,
                                  designate_clearcut, citizens, accessible_trees,
                                  inaccessible_trees, designated_trees, accessible_yield,
                                  tree_counts, designated_tree_counts,
                                  clearcut_burrows, chop_burrows);

    return newly_marked;
}

// TODO: does this actually catch anything above the bad_flag check?
static bool is_valid_item(df::item *item) {
    for (size_t i = 0; i < item->general_refs.size(); i++) {
        df::general_ref *ref = item->general_refs[i];

        switch (ref->getType()) {
        case general_ref_type::CONTAINED_IN_ITEM:
            return false;

        case general_ref_type::UNIT_HOLDER:
            return false;

        case general_ref_type::BUILDING_HOLDER:
            return false;

        default:
            break;
        }
    }

    for (size_t i = 0; i < item->specific_refs.size(); i++) {
        df::specific_ref *ref = item->specific_refs[i];

        if (ref->type == specific_ref_type::JOB) {
            // Ignore any items assigned to a job
            return false;
        }
    }

    return true;
}

struct BadFlags
{
    uint32_t whole;

    BadFlags()
    {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
        F(in_job); F(owned); F(removed);
        F(encased); F(spider_web);
        #undef F
        whole = flags.whole;
    }
};

static void scan_logs(color_ostream &out, int32_t *usable_logs,
                      const vector<df::unit *> &citizens, int32_t *inaccessible_logs = NULL) {
    static const BadFlags bad_flags;

    TRACE(cycle,out).print("scanning logs\n");
    if (usable_logs)
        *usable_logs = 0;
    if (inaccessible_logs)
        *inaccessible_logs = 0;

    for (auto &item : world->items.other[items_other_id::IN_PLAY]) {
        TRACE(cycle,out).print("  scanning log %d\n", item->id);
        if (item->flags.whole & bad_flags.whole)
            continue;

        if (item->getType() != item_type::WOOD)
            continue;

        if (!is_valid_item(item))
            continue;

        if (!is_accessible_item(item, citizens)) {
            if (inaccessible_logs)
                ++*inaccessible_logs;
        } else if (usable_logs) {
            ++*usable_logs;
        }
    }
}

static int32_t do_cycle(color_ostream &out, bool force_designate) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    validate_burrow_configs(out);

    // scan trees and clearcut marked burrows
    int32_t expected_yield;
    TreesBySize designatable_trees_by_size;
    vector<df::unit *> citizens;
    Units::getCitizens(citizens, true);
    int32_t newly_marked = scan_trees(out, &expected_yield,
            &designatable_trees_by_size, true, citizens);

    // check how many logs we have already
    int32_t usable_logs;
    scan_logs(out, &usable_logs, citizens);

    if (config.get_bool(CONFIG_WAITING_FOR_MIN)
            && usable_logs <= config.get_int(CONFIG_MIN_LOGS)) {
        DEBUG(cycle,out).print("minimum threshold crossed\n");
        config.set_bool(CONFIG_WAITING_FOR_MIN, false);
    }
    else if (!config.get_bool(CONFIG_WAITING_FOR_MIN)
            && usable_logs > config.get_int(CONFIG_MAX_LOGS)) {
        DEBUG(cycle,out).print("maximum threshold crossed\n");
        config.set_bool(CONFIG_WAITING_FOR_MIN, true);
    }

    // if we already have designated enough, we're done
    int32_t limit = force_designate || !config.get_bool(CONFIG_WAITING_FOR_MIN) ?
        config.get_int(CONFIG_MAX_LOGS) :
        config.get_int(CONFIG_MIN_LOGS);
    if (usable_logs + expected_yield > limit) {
        return newly_marked;
    }

    // designate until the expected yield gets us to our target or we run out
    // of accessible trees
    int32_t needed = config.get_int(CONFIG_MAX_LOGS) -
            (usable_logs + expected_yield);
    DEBUG(cycle,out).print("needed logs for this cycle: %d\n", needed);
    for (auto & entry : designatable_trees_by_size) {
        if (!Designations::markPlant(entry.second))
            continue;
        ++newly_marked;
        needed -= entry.first;
        if (needed <= 0) {
            return newly_marked;
        }
    }
    out.print("autochop: insufficient accessible trees to reach log target! Still need %d logs!\n",
            needed);
    return newly_marked;
}

/////////////////////////////////////////////////////
// Lua API
// core will already be suspended when coming in through here
//

static const char * get_protect_str(bool protect_brewable, bool protect_edible, bool protect_cookable) {
    if (!protect_brewable && !protect_edible && !protect_cookable)
        return "   ";
    if (!protect_brewable && !protect_edible && protect_cookable)
        return "  z";
    if (!protect_brewable && protect_edible && !protect_cookable)
        return " e ";
    if (!protect_brewable && protect_edible && protect_cookable)
        return " ez";
    if (protect_brewable && !protect_edible && !protect_cookable)
        return "b  ";
    if (protect_brewable && !protect_edible && protect_cookable)
        return "b z";
    if (protect_brewable && protect_edible && !protect_cookable)
        return "be ";
    if (protect_brewable && protect_edible && protect_cookable)
        return "bez";
    return "";
}

static void autochop_printStatus(color_ostream &out) {
    DEBUG(control,out).print("entering autochop_printStatus\n");
    validate_burrow_configs(out);
    out.print("autochop is %s\n\n", is_enabled ? "enabled" : "disabled");
    out.print("  keeping log counts between %d and %d\n",
            config.get_int(CONFIG_MIN_LOGS), config.get_int(CONFIG_MAX_LOGS));
    if (config.get_bool(CONFIG_WAITING_FOR_MIN))
        out.print("  currently waiting for min threshold to be crossed before designating more trees\n");
    else
        out.print("  currently designating trees until max threshold is crossed\n");
    out.print("\n");

    int32_t usable_logs, inaccessible_logs;
    int32_t accessible_trees, inaccessible_trees;
    int32_t designated_trees, expected_yield, accessible_yield;
    map<int32_t, int32_t> tree_counts, designated_tree_counts;
    vector<df::unit *> citizens;
    Units::getCitizens(citizens, true);
    scan_logs(out, &usable_logs, citizens, &inaccessible_logs);
    scan_trees(out, &expected_yield, NULL, false, citizens, &accessible_trees, &inaccessible_trees,
            &designated_trees, &accessible_yield, &tree_counts, &designated_tree_counts);

    out.print("summary:\n");
    out.print("           accessible logs (usable stock): %d\n", usable_logs);
    out.print("                        inaccessible logs: %d\n", inaccessible_logs);
    out.print("                       total visible logs: %d\n", usable_logs + inaccessible_logs);
    out.print("\n");
    out.print("                         accessible trees: %d\n", accessible_trees);
    out.print("                       inaccessible trees: %d\n", inaccessible_trees);
    out.print("                      total visible trees: %d\n", accessible_trees + inaccessible_trees);
    out.print("\n");
    out.print("                         designated trees: %d\n", designated_trees);
    out.print("      expected logs from designated trees: %d\n", expected_yield);
    out.print("  expected logs from all accessible trees: %d\n", accessible_yield);
    out.print("\n");
    out.print("                    total trees harvested: %d\n", plotinfo->trees_removed);
    out.print("\n");

    if (!plotinfo->burrows.list.size()) {
        out.print("no burrows defined\n");
        return;
    }

    out.print("\n");

    int name_width = 11;
    for (auto &burrow : plotinfo->burrows.list) {
        name_width = std::max(name_width, (int)burrow->name.size());
    }
    name_width = -name_width; // left justify

    const char *fmt = "%*s  %4s  %4s  %8s  %5s  %6s  %7s\n";
    out.print(fmt, name_width, "burrow name", " id ", "chop", "clearcut", "trees", "marked", "protect");
    out.print(fmt, name_width, "-----------", "----", "----", "--------", "-----", "------", "-------");

    for (auto &burrow : plotinfo->burrows.list) {
        bool chop = false;
        bool clearcut = false;
        bool protect_brewable = false;
        bool protect_edible = false;
        bool protect_cookable = false;
        if (watched_burrows_indices.count(burrow->id)) {
            auto &c = watched_burrows[watched_burrows_indices[burrow->id]];
            chop = c.get_bool(BURROW_CONFIG_CHOP);
            clearcut = c.get_bool(BURROW_CONFIG_CLEARCUT);
            protect_brewable = c.get_bool(BURROW_CONFIG_PROTECT_BREWABLE);
            protect_edible = c.get_bool(BURROW_CONFIG_PROTECT_EDIBLE);
            protect_cookable = c.get_bool(BURROW_CONFIG_PROTECT_COOKABLE);
        }
        out.print(fmt, name_width, burrow->name.c_str(), int_to_string(burrow->id).c_str(),
                chop ? "[x]" : "[ ]", clearcut ? "[x]" : "[ ]",
                int_to_string(tree_counts[burrow->id]).c_str(),
                int_to_string(designated_tree_counts[burrow->id]).c_str(),
                get_protect_str(protect_brewable, protect_edible, protect_cookable));
    }
}

static void autochop_designate(color_ostream &out) {
    DEBUG(control,out).print("entering autochop_designate\n");
    out.print("designated %d tree(s) for chopping\n", do_cycle(out, true));
}

static void autochop_undesignate(color_ostream &out) {
    DEBUG(control,out).print("entering autochop_undesignate\n");
    int32_t count = 0;
    for (auto plant : world->plants.all) {
        if (is_valid_tree(plant) && Designations::unmarkPlant(plant))
            ++count;
    }
    out.print("undesignated %d tree(s)\n", count);
}

static void autochop_setTargets(color_ostream &out, int32_t max_logs, int32_t min_logs) {
    DEBUG(control,out).print("entering autochop_setTargets\n");
    if (max_logs < min_logs || min_logs < 0) {
        out.printerr("max and min must be at least 0 and max must be greater than min\n");
        return;
    }
    config.set_int(CONFIG_MAX_LOGS, max_logs);
    config.set_int(CONFIG_MIN_LOGS, min_logs);

    // check limits and designate up to the new maximum
    autochop_designate(out);
}

static int autochop_getTargets(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering autochop_getTargets\n");
    Lua::Push(L, config.get_int(CONFIG_MAX_LOGS));
    Lua::Push(L, config.get_int(CONFIG_MIN_LOGS));
    return 2;
}

static int autochop_getLogCounts(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering autochop_getNumLogs\n");
    int32_t usable_logs, inaccessible_logs;
    vector<df::unit *> citizens;
    Units::getCitizens(citizens, true);
    scan_logs(*out, &usable_logs, citizens, &inaccessible_logs);
    Lua::Push(L, usable_logs);
    Lua::Push(L, inaccessible_logs);
    return 2;
}

static void push_burrow_config(lua_State *L, int id, bool chop = false,
        bool clearcut = false, bool protect_brewable = false,
        bool protect_edible = false, bool protect_cookable = false) {
    map<string, int32_t> burrow_config;
    burrow_config.emplace("id", id);
    burrow_config.emplace("chop", chop);
    burrow_config.emplace("clearcut", clearcut);
    burrow_config.emplace("protect_brewable", protect_brewable);
    burrow_config.emplace("protect_edible", protect_edible);
    burrow_config.emplace("protect_cookable", protect_cookable);
    Lua::Push(L, burrow_config);
}

static void push_burrow_config(lua_State *L, PersistentDataItem &c) {
    push_burrow_config(L, c.get_int(BURROW_CONFIG_ID),
            c.get_bool(BURROW_CONFIG_CHOP),
            c.get_bool(BURROW_CONFIG_CLEARCUT),
            c.get_bool(BURROW_CONFIG_PROTECT_BREWABLE),
            c.get_bool(BURROW_CONFIG_PROTECT_EDIBLE),
            c.get_bool(BURROW_CONFIG_PROTECT_COOKABLE));
}

static void emplace_bulk_burrow_config(lua_State *L,  map<int32_t, map<string, int32_t>> &burrows, int id, bool chop = false,
        bool clearcut = false, bool protect_brewable = false,
        bool protect_edible = false, bool protect_cookable = false) {

    map<string, int32_t> burrow_config;
    burrow_config.emplace("id", id);
    burrow_config.emplace("chop", chop);
    burrow_config.emplace("clearcut", clearcut);
    burrow_config.emplace("protect_brewable", protect_brewable);
    burrow_config.emplace("protect_edible", protect_edible);
    burrow_config.emplace("protect_cookable", protect_cookable);

    burrows.emplace(id, burrow_config);
}

static void emplace_bulk_burrow_config(lua_State *L, map<int32_t, map<string, int32_t>> &burrows, PersistentDataItem &c) {
    emplace_bulk_burrow_config(L, burrows, c.get_int(BURROW_CONFIG_ID),
            c.get_bool(BURROW_CONFIG_CHOP),
            c.get_bool(BURROW_CONFIG_CLEARCUT),
            c.get_bool(BURROW_CONFIG_PROTECT_BREWABLE),
            c.get_bool(BURROW_CONFIG_PROTECT_EDIBLE),
            c.get_bool(BURROW_CONFIG_PROTECT_COOKABLE));
}

static int autochop_getTreeCountsAndBurrowConfigs(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering autochop_getTreeCountsAndBurrowConfigs\n");
    validate_burrow_configs(*out);
    int32_t accessible_trees, inaccessible_trees;
    int32_t designated_trees, expected_yield, accessible_yield;
    map<int32_t, int32_t> tree_counts, designated_tree_counts;
    vector<df::unit *> citizens;
    Units::getCitizens(citizens, true);
    scan_trees(*out, &expected_yield, NULL, false, citizens, &accessible_trees, &inaccessible_trees,
            &designated_trees, &accessible_yield, &tree_counts, &designated_tree_counts);

    map<string, int32_t> summary;

    map<int32_t, map<string, int32_t>> burrow_config_map;

    summary.emplace("accessible_trees", accessible_trees);
    summary.emplace("inaccessible_trees", inaccessible_trees);
    summary.emplace("designated_trees", designated_trees);
    summary.emplace("expected_yield", expected_yield);
    summary.emplace("accessible_yield", accessible_yield);
    Lua::Push(L, summary);

    Lua::Push(L, tree_counts);
    Lua::Push(L, designated_tree_counts);

    for (auto &burrow : plotinfo->burrows.list) {
        int id = burrow->id;
        if (watched_burrows_indices.count(id))
            emplace_bulk_burrow_config(L, burrow_config_map,
                    watched_burrows[watched_burrows_indices[id]]);
        else
            emplace_bulk_burrow_config(L, burrow_config_map, id);
    }

    Lua::Push(L, burrow_config_map);

    return 4;
}

static int autochop_getBurrowConfig(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering autochop_getBurrowConfig\n");
    validate_burrow_configs(*out);
    // param can be a name or an id
    int id;
    if (lua_isnumber(L, -1)) {
        id = lua_tointeger(L, -1);
        if (!df::burrow::find(id))
            return 0;
    } else {
        const char * name = lua_tostring(L, -1);
        if (!name)
            return 0;

        string nameStr = name;
        bool found = false;
        for (auto &burrow : plotinfo->burrows.list) {
            if (nameStr == burrow->name) {
                id = burrow->id;
                found = true;
                break;
            }
        }
        if (!found)
            return 0;
    }

    if (watched_burrows_indices.count(id)) {
        push_burrow_config(L, watched_burrows[watched_burrows_indices[id]]);
    } else {
        push_burrow_config(L, id);
    }
    return 1;
}

static void autochop_setBurrowConfig(color_ostream &out, int id, bool chop,
        bool clearcut, bool protect_brewable, bool protect_edible,
        bool protect_cookable) {
    DEBUG(control,out).print("entering autochop_setBurrowConfig\n");
    validate_burrow_configs(out);

    bool isInvalidBurrow = !df::burrow::find(id);
    bool hasNoData = !chop && !clearcut && !protect_brewable && !protect_edible
            && !protect_cookable;

    if (isInvalidBurrow || hasNoData) {
        remove_burrow_config(out, id);
        return;
    }

    PersistentDataItem &c = ensure_burrow_config(out, id);
    c.set_int(BURROW_CONFIG_ID, id);
    c.set_bool(BURROW_CONFIG_CHOP, chop);
    c.set_bool(BURROW_CONFIG_CLEARCUT, clearcut);
    c.set_bool(BURROW_CONFIG_PROTECT_BREWABLE, protect_brewable);
    c.set_bool(BURROW_CONFIG_PROTECT_EDIBLE, protect_edible);
    c.set_bool(BURROW_CONFIG_PROTECT_COOKABLE, protect_cookable);
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autochop_printStatus),
    DFHACK_LUA_FUNCTION(autochop_designate),
    DFHACK_LUA_FUNCTION(autochop_undesignate),
    DFHACK_LUA_FUNCTION(autochop_setTargets),
    DFHACK_LUA_FUNCTION(autochop_setBurrowConfig),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(autochop_getTargets),
    DFHACK_LUA_COMMAND(autochop_getLogCounts),
    DFHACK_LUA_COMMAND(autochop_getBurrowConfig),
    DFHACK_LUA_COMMAND(autochop_getTreeCountsAndBurrowConfigs),
    DFHACK_LUA_END
};
