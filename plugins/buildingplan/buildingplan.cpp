#include "buildingplan.h"
#include "buildingtypekey.h"
#include "defaultitemfilters.h"
#include "plannedbuilding.h"

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Burrows.h"
#include "modules/World.h"

#include "df/construction_type.h"
#include "df/burrow.h"
#include "df/item.h"
#include "df/job_item.h"
#include "df/organic_mat_category.h"
#include "df/world.h"

#if __has_cpp_attribute(no_dangling)
#define NO_DANGLING_REFERENCE [[gnu::no_dangling]]
#else
#define NO_DANGLING_REFERENCE
#endif

using std::map;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("buildingplan");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(buildingplan, control, DebugCategory::LINFO);
    DBG_DECLARE(buildingplan, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
const string FILTER_CONFIG_KEY = string(plugin_name) + "/filter";
const string BLD_CONFIG_KEY = string(plugin_name) + "/building";

static PersistentDataItem config;
// for use in counting available materials for the UI
static map<string, std::pair<MaterialInfo, string>> mat_cache;
static unordered_map<BuildingTypeKey, vector<const df::job_item *>, BuildingTypeKeyHash> job_item_cache;
static unordered_map<BuildingTypeKey, HeatSafety, BuildingTypeKeyHash> cur_heat_safety;
static unordered_map<BuildingTypeKey, DefaultItemFilters, BuildingTypeKeyHash> cur_item_filters;
// building id -> PlannedBuilding
static unordered_map<int32_t, PlannedBuilding> planned_buildings;
// vector id -> filter bucket -> queue of (building id, job_item index)
static Tasks tasks;

// note that this just removes the PlannedBuilding. the tasks will get dropped
// as we discover them in the tasks queues and they fail to be found in planned_buildings.
// this "lazy" task cleaning algorithm works because there is no way to
// re-register a building once it has been removed -- if it has been booted out of
// planned_buildings, then it has either been built or desroyed. therefore there is
// no chance of duplicate tasks getting added to the tasks queues.
void PlannedBuilding::remove(color_ostream &out) {
    DEBUG(control,out).print("removing persistent data for building %d\n", id);
    World::DeletePersistentData(bld_config);
    planned_buildings.erase(id);
}

static const int32_t CYCLE_TICKS = 599; // twice per game day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle
int32_t walkability_timestamp = -1; // world->frame_counter at last update of walkability groups

static int get_num_filters(color_ostream &out, BuildingTypeKey key) {
    int num_filters = 0;
    if (!Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "get_num_filters",
            std::make_tuple(std::get<0>(key), std::get<1>(key), std::get<2>(key)),
            1, [&](lua_State *L) {
                num_filters = lua_tonumber(L, -1);
            })) {
        return 0;
    }
    return num_filters;
}

NO_DANGLING_REFERENCE
static const vector<const df::job_item *> & get_job_items(color_ostream &out, BuildingTypeKey key) {
    if (job_item_cache.count(key))
        return job_item_cache[key];
    const int num_filters = get_num_filters(out, key);
    auto &jitems = job_item_cache[key];
    for (int index = 0; index < num_filters; ++index) {
        bool failed = false;
        if (!Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "get_job_item",
                std::make_tuple(std::get<0>(key), std::get<1>(key), std::get<2>(key), index+1),
                1, [&](lua_State *L) {
                    df::job_item *jitem = Lua::GetDFObject<df::job_item>(L, -1);
                    DEBUG(control,out).print("retrieving job_item for (%d, %d, %d) index=%d: 0x%p\n",
                            std::get<0>(key), std::get<1>(key), std::get<2>(key), index, jitem);
                    if (!jitem)
                        failed = true;
                    else
                        jitems.emplace_back(jitem);
                }) || failed) {
            jitems.clear();
            break;
        }
    }
    return jitems;
}

static const df::dfhack_material_category stone_cat(df::dfhack_material_category::mask_stone);
static const df::dfhack_material_category wood_cat(df::dfhack_material_category::mask_wood);
static const df::dfhack_material_category metal_cat(df::dfhack_material_category::mask_metal);
static const df::dfhack_material_category glass_cat(df::dfhack_material_category::mask_glass);
static const df::dfhack_material_category gem_cat(df::dfhack_material_category::mask_gem);
static const df::dfhack_material_category clay_cat(df::dfhack_material_category::mask_clay);
static const df::dfhack_material_category cloth_cat(df::dfhack_material_category::mask_cloth);
static const df::dfhack_material_category silk_cat(df::dfhack_material_category::mask_silk);
static const df::dfhack_material_category yarn_cat(df::dfhack_material_category::mask_yarn);

static void cache_matched(int16_t type, int32_t index) {
    MaterialInfo mi;
    mi.decode(type, index);
    if (mi.matches(stone_cat)) {
        DEBUG(control).print("cached stone material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "stone"));
    } else if (mi.matches(wood_cat)) {
        DEBUG(control).print("cached wood material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "wood"));
    } else if (mi.matches(metal_cat)) {
        DEBUG(control).print("cached metal material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "metal"));
    } else if (mi.matches(glass_cat)) {
        DEBUG(control).print("cached glass material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "glass"));
    } else if (mi.matches(gem_cat)) {
        DEBUG(control).print("cached gem material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "gem"));
    } else if (mi.matches(clay_cat)) {
        DEBUG(control).print("cached clay material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "clay"));
    } else if (mi.matches(cloth_cat)) {
        DEBUG(control).print("cached cloth material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "cloth"));
    } else if (mi.matches(silk_cat)) {
        DEBUG(control).print("cached silk material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "silk"));
    } else if (mi.matches(yarn_cat)) {
        DEBUG(control).print("cached yarn material: %s (%d, %d)\n", mi.toString().c_str(), type, index);
        mat_cache.emplace(mi.toString(), std::make_pair(mi, "yarn"));
    }
    else
        TRACE(control).print("not matched: %s\n", mi.toString().c_str());
}

static void load_organic_material_cache(df::organic_mat_category cat) {
    auto& mat_tab = world->raws.mat_table;
    auto& cat_vec = mat_tab.organic_types[cat];
    auto& cat_indices = mat_tab.organic_indexes[cat];
    for (size_t i = 0; i < cat_vec.size(); i++) {
        cache_matched(cat_vec[i], cat_indices[i]);
    }
}

static void load_material_cache() {
    df::world_raws &raws = world->raws;
    for (int i = 1; i < DFHack::MaterialInfo::NUM_BUILTIN; ++i)
        if (raws.mat_table.builtin[i])
            cache_matched(i, -1);

    for (size_t i = 0; i < raws.inorganics.size(); i++)
        cache_matched(0, i);

    load_organic_material_cache(df::organic_mat_category::Wood);
    load_organic_material_cache(df::organic_mat_category::PlantFiber);
    load_organic_material_cache(df::organic_mat_category::Silk);
    load_organic_material_cache(df::organic_mat_category::Yarn);
}

static HeatSafety get_heat_safety_filter(const BuildingTypeKey &key) {
    if (cur_heat_safety.count(key))
        return cur_heat_safety.at(key);
    return HEAT_SAFETY_ANY;
}

static DefaultItemFilters & get_item_filters(color_ostream &out, const BuildingTypeKey &key) {
    if (cur_item_filters.count(key))
        return cur_item_filters.at(key);
    cur_item_filters.emplace(key, DefaultItemFilters(out, key, get_job_items(out, key)));
    return cur_item_filters.at(key);
}

static command_result do_command(color_ostream &out, vector<string> &parameters);
void update_walkability_groups();
void buildingplan_cycle(color_ostream &out, Tasks &tasks,
        unordered_map<int32_t, PlannedBuilding> &planned_buildings, bool unsuspend_on_finalize);

static bool registerPlannedBuilding(color_ostream &out, PlannedBuilding & pb, bool unsuspend_on_finalize);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Plan building placement before you have materials.",
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

    for (auto &entry : job_item_cache ) {
        for (auto &jitem : entry.second) {
            delete jitem;
        }
    }
    job_item_cache.clear();

    return CR_OK;
}

static void validate_materials_config(color_ostream &out, bool verbose = false) {
    if (config.get_bool(CONFIG_BLOCKS)
            || config.get_bool(CONFIG_BOULDERS)
            || config.get_bool(CONFIG_LOGS)
            || config.get_bool(CONFIG_BARS))
        return;

    if (verbose)
        out.printerr("all contruction materials disabled; resetting config\n");

    config.set_bool(CONFIG_BLOCKS, true);
    config.set_bool(CONFIG_BOULDERS, true);
    config.set_bool(CONFIG_LOGS, true);
    config.set_bool(CONFIG_BARS, false);
}

static void reset_filters(color_ostream &out) {
    cur_heat_safety.clear();
    cur_item_filters.clear();
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

static int16_t get_subtype(df::building *bld) {
    if (!bld)
        return -1;

    int16_t subtype = bld->getSubtype();
    if (bld->getType() == df::building_type::Construction &&
            subtype >= df::construction_type::UpStair &&
            subtype <= df::construction_type::UpDownStair)
        subtype = df::construction_type::UpDownStair;
    return subtype;
}

static bool is_suspendmanager_enabled(color_ostream &out) {
    bool suspendmanager_enabled = false;
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "is_suspendmanager_enabled", {},
        1, [&](lua_State *L){
                suspendmanager_enabled = lua_toboolean(L, -1);
            });
    return suspendmanager_enabled;
}

DFhackCExport command_result plugin_load_world_data (color_ostream &out) {
    mat_cache.clear();
    load_material_cache();
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "reload_pens");
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    walkability_timestamp = -1;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
    }
    if (config.get_int(CONFIG_RECONSTRUCT) == -1)
        config.set_bool(CONFIG_RECONSTRUCT, true);
    validate_materials_config(out);

    DEBUG(control,out).print("loading persisted state\n");

    planned_buildings.clear();
    tasks.clear();
    reset_filters(out);

    vector<PersistentDataItem> filter_configs;
    World::GetPersistentSiteData(&filter_configs, FILTER_CONFIG_KEY);
    for (auto &cfg : filter_configs) {
        BuildingTypeKey key = DefaultItemFilters::getKey(cfg);
        cur_item_filters.emplace(key, DefaultItemFilters(out, cfg, get_job_items(out, key)));
    }

    vector<PersistentDataItem> building_configs;
    World::GetPersistentSiteData(&building_configs, BLD_CONFIG_KEY);
    const size_t num_building_configs = building_configs.size();
    bool unsuspend_on_finalize = !is_suspendmanager_enabled(out);
    for (size_t idx = 0; idx < num_building_configs; ++idx) {
        PlannedBuilding pb(out, building_configs[idx]);
        df::building *bld = df::building::find(pb.id);
        if (!bld) {
            DEBUG(control,out).print("building %d no longer exists; skipping\n", pb.id);
            pb.remove(out);
            continue;
        }
        BuildingTypeKey key(bld->getType(), get_subtype(bld), bld->getCustomType());
        if (pb.item_filters.size() != get_item_filters(out, key).getItemFilters().size()) {
            WARN(control).print("loaded state for building %d doesn't match world\n", pb.id);
            pb.remove(out);
            continue;
        }
        registerPlannedBuilding(out, pb, unsuspend_on_finalize);
    }

    return CR_OK;
}

static bool cycle_requested = false;

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;
    cycle_requested = false;

    bool unsuspend_on_finalize = !is_suspendmanager_enabled(out);
    buildingplan_cycle(out, tasks, planned_buildings, unsuspend_on_finalize);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
        return CR_OK;

    if (cycle_requested || world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot configure %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "parse_commandline", parameters,
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// Lua API
// core will already be suspended when coming in through here
//

static string getBucket(const df::job_item & ji, const PlannedBuilding & pb, int idx) {
    if (idx < 0 || (size_t)idx >= pb.item_filters.size())
        return "INVALID";

    std::ostringstream ser;

    // put elements in front that significantly affect the difficulty of matching
    // the filter. ensure the lexicographically "less" value is the pickier value.
    const ItemFilter & item_filter = pb.item_filters[idx];

    if (item_filter.getDecoratedOnly())
        ser << "Da";
    else
        ser << "Db";

    if (ji.flags2.bits.magma_safe || pb.heat_safety == HEAT_SAFETY_MAGMA)
        ser << "Ha";
    else if (ji.flags2.bits.fire_safe || pb.heat_safety == HEAT_SAFETY_FIRE)
        ser << "Hb";
    else
        ser << "Hc";

    size_t num_materials = item_filter.getMaterials().size();
    if (num_materials == 0 || num_materials >= 9 || !item_filter.getMaterialMask().whole)
        ser << "M9";
    else
        ser << "M" << num_materials;

    // pull out and serialize only known relevant fields. if we miss a few, then
    // the filter bucket will be slighly less specific than it could be, but
    // that's probably ok. we'll just end up bucketing slightly different items
    // together. this is only a problem if the different filter at the front of
    // the queue doesn't match any available items and blocks filters behind it
    // that could be matched.
    ser << ji.item_type << ':' << ji.item_subtype << ':' << ji.mat_type << ':'
        << ji.mat_index << ':' << ji.flags1.whole << ':' << ji.flags2.whole
        << ':' << ji.flags3.whole << ':' << ji.flags4 << ':' << ji.flags5 << ':'
        << ji.metal_ore << ':' << ji.has_tool_use;

    ser << ':' << item_filter.serialize();

    for (auto &special : pb.specials)
        ser << ':' << special;

    return ser.str();
}

// get a list of item vectors that we should search for matches
vector<df::job_item_vector_id> getVectorIds(color_ostream &out, const df::job_item *job_item, bool ignore_filters) {
    std::vector<df::job_item_vector_id> ret;

    // if the filter already has the vector_id set to something specific, use it
    if (job_item->vector_id > df::job_item_vector_id::IN_PLAY)
    {
        DEBUG(control,out).print("using vector_id from job_item: %s\n",
              ENUM_KEY_STR(job_item_vector_id, job_item->vector_id).c_str());
        ret.push_back(job_item->vector_id);
        return ret;
    }

    // if the filter is for building material, refer to our global settings for
    // which vectors to search
    if (job_item->flags2.bits.building_material)
    {
        if (ignore_filters || config.get_bool(CONFIG_BLOCKS))
            ret.push_back(df::job_item_vector_id::BLOCKS);
        if (ignore_filters || config.get_bool(CONFIG_BOULDERS))
            ret.push_back(df::job_item_vector_id::BOULDER);
        if (ignore_filters || config.get_bool(CONFIG_LOGS))
            ret.push_back(df::job_item_vector_id::WOOD);
        if (ignore_filters || config.get_bool(CONFIG_BARS))
            ret.push_back(df::job_item_vector_id::BAR);
    }

    // fall back to IN_PLAY if no other vector was appropriate
    if (ret.empty())
        ret.push_back(df::job_item_vector_id::IN_PLAY);
    return ret;
}

static bool registerPlannedBuilding(color_ostream &out, PlannedBuilding & pb, bool unsuspend_on_finalize) {
    df::building * bld = pb.getBuildingIfValidOrRemoveIfNot(out);
    if (!bld)
        return false;

    if (bld->jobs.size() != 1) {
        DEBUG(control,out).print("unexpected number of jobs: want 1, got %zu\n", bld->jobs.size());
        return false;
    }

    // suspend jobs
    for (auto job : bld->jobs)
        job->flags.bits.suspend = true;

    auto job_items = bld->jobs[0]->job_items.elements;
    if (isJobReady(out, job_items)) {
        // all items are already attached
        finalizeBuilding(out, bld, unsuspend_on_finalize);
        pb.remove(out);
        return true;
    }

    int num_job_items = (int)job_items.size();
    int32_t id = bld->id;
    for (int job_item_idx = 0; job_item_idx < num_job_items; ++job_item_idx) {
        int rev_jitem_index = num_job_items - (job_item_idx+1);
        auto job_item = job_items[rev_jitem_index];
        auto bucket = getBucket(*job_item, pb, job_item_idx);

        // if there are multiple vector_ids, schedule duplicate tasks. after
        // the correct number of items are matched, the extras will get popped
        // as invalid
        for (auto vector_id : pb.vector_ids[job_item_idx]) {
            for (int item_num = 0; item_num < job_item->quantity; ++item_num) {
                tasks[vector_id][bucket].emplace_back(id, rev_jitem_index);
                DEBUG(control,out).print("added task: %s/%s/%d,%d; "
                      "%zu vector(s), %zu filter bucket(s), %zu task(s) in bucket\n",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket.c_str(), id, rev_jitem_index, tasks.size(),
                      tasks[vector_id].size(), tasks[vector_id][bucket].size());
            }
        }
    }

    // add the planned buildings to our register
    planned_buildings.emplace(bld->id, pb);

    return true;
}

static string get_desc_string(color_ostream &out, df::job_item *jitem,
        const vector<df::job_item_vector_id> &vec_ids) {
    vector<string> descs;
    for (auto &vec_id : vec_ids) {
        df::job_item jitem_copy = *jitem;
        jitem_copy.vector_id = vec_id;
        Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "get_desc", std::make_tuple(&jitem_copy),
                1, [&](lua_State *L) {
                    descs.emplace_back(lua_tostring(L, -1));
                });
    }
    return join_strings(" or ", descs);
}

static void printStatus(color_ostream &out) {
    DEBUG(control,out).print("entering buildingplan_printStatus\n");
    out.print("buildingplan is %s\n\n", is_enabled ? "enabled" : "disabled");
    out.print("Current settings:\n");
    out.print("  use blocks:   %s\n", config.get_bool(CONFIG_BLOCKS) ? "yes" : "no");
    out.print("  use boulders: %s\n", config.get_bool(CONFIG_BOULDERS) ? "yes" : "no");
    out.print("  use logs:     %s\n", config.get_bool(CONFIG_LOGS) ? "yes" : "no");
    out.print("  use bars:     %s\n", config.get_bool(CONFIG_BARS) ? "yes" : "no");
    out.print("  plan constructions on tiles with existing constructed floors/ramps when using box select: %s\n",
        config.get_bool(CONFIG_RECONSTRUCT) ? "yes" : "no");
    auto burrow = df::burrow::find(config.get_int(CONFIG_BURROW));
    out.print("  ignore building materials in burrow: %s\n", burrow ? burrow->name.c_str() : "none");
    out.print("\n");

    size_t bld_count = 0;
    map<string, int32_t> counts;
    int32_t total = 0;
    for (auto &entry : planned_buildings) {
        auto &pb = entry.second;
        // don't actually remove bad buildings from the list while we're
        // actively iterating through that list
        auto bld = pb.getBuildingIfValidOrRemoveIfNot(out, true);
        if (!bld || bld->jobs.size() != 1)
            continue;
        auto &job_items = bld->jobs[0]->job_items.elements;
        const size_t num_job_items = job_items.size();
        if (num_job_items != pb.vector_ids.size())
            continue;
        ++bld_count;
        int job_item_idx = 0;
        for (auto &vec_ids : pb.vector_ids) {
            auto &jitem = job_items[num_job_items - (job_item_idx+1)];
            int32_t quantity = jitem->quantity;
            if (quantity) {
                counts[get_desc_string(out, jitem, vec_ids)] += quantity;
                total += quantity;
            }
            ++job_item_idx;
        }
    }

    if (bld_count) {
        out.print("Waiting for %d item(s) to be produced for %zd building(s):\n",
                total, bld_count);
        for (auto &count : counts)
            out.print("  %3d %s%s\n", count.second, count.first.c_str(), count.second == 1 ? "" : "s");
    } else {
        out.print("Currently no planned buildings\n");
    }
    out.print("\n");
}

static bool setSetting(color_ostream &out, string name, bool value) {
    DEBUG(control,out).print("entering setSetting (%s -> %s)\n", name.c_str(), value ? "true" : "false");
    if (name == "blocks")
        config.set_bool(CONFIG_BLOCKS, value);
    else if (name == "boulders")
        config.set_bool(CONFIG_BOULDERS, value);
    else if (name == "logs")
        config.set_bool(CONFIG_LOGS, value);
    else if (name == "bars")
        config.set_bool(CONFIG_BARS, value);
    else if (name == "reconstruct")
        config.set_bool(CONFIG_RECONSTRUCT, value);
    else {
        out.printerr("unrecognized setting: '%s'\n", name.c_str());
        return false;
    }

    validate_materials_config(out, true);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
    return true;
}

static void resetFilters(color_ostream &out) {
    DEBUG(control,out).print("entering resetFilters\n");
    reset_filters(out);
}

static void setIgnoreBurrow(color_ostream &out, string burrow_name){
    DEBUG(control,out).print("entering setIgnoreBurrow\n");
    auto burrow = Burrows::findByName(burrow_name);
    if (burrow) {
        config.set_int(CONFIG_BURROW, burrow->id);
    } else {
        config.set_int(CONFIG_BURROW, -1);
    }
}

df::burrow *getIgnoreBurrow(){
    int id = config.get_int(CONFIG_BURROW);
    return id < 0 ? nullptr : df::burrow::find(id);
}

static bool isPlannableBuilding(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom) {
    DEBUG(control,out).print("entering isPlannableBuilding\n");
    return get_num_filters(out, BuildingTypeKey(type, subtype, custom)) >= 1;
}

static bool isPlannedBuilding(color_ostream &out, df::building *bld) {
    TRACE(control,out).print("entering isPlannedBuilding\n");
    return bld && planned_buildings.count(bld->id);
}

static bool addPlannedBuilding(color_ostream &out, df::building *bld) {
    DEBUG(control,out).print("entering addPlannedBuilding\n");
    if (!bld || planned_buildings.count(bld->id))
        return false;

    int16_t subtype = get_subtype(bld);

    if (!isPlannableBuilding(out, bld->getType(), subtype, bld->getCustomType()))
        return false;

    BuildingTypeKey key(bld->getType(), subtype, bld->getCustomType());
    PlannedBuilding pb(out, bld, get_heat_safety_filter(key), get_item_filters(out, key));

    bool unsuspend_on_finalize = !is_suspendmanager_enabled(out);
    return registerPlannedBuilding(out, pb, unsuspend_on_finalize);
}

static void doCycle(color_ostream &out) {
    DEBUG(control,out).print("entering doCycle\n");
    do_cycle(out);
}

static void scheduleCycle(color_ostream &out) {
    DEBUG(control,out).print("entering scheduleCycle\n");
    cycle_requested = true;
}

static int scanAvailableItems(color_ostream &out, df::building_type type, int16_t subtype,
        int32_t custom, int index, bool ignore_filters, bool ignore_quality, HeatSafety *heat_override = NULL,
        vector<int> *item_ids = NULL, map<MaterialInfo, int32_t> *counts = NULL)
{
    DEBUG(control,out).print(
            "entering scanAvailableItems building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    HeatSafety heat = heat_override ? *heat_override : get_heat_safety_filter(key);
    auto &job_items = get_job_items(out, key);
    if (index < 0 || job_items.size() <= (size_t)index)
        return 0;
    auto &item_filters = get_item_filters(out, key);
    auto &filters = item_filters.getItemFilters();
    auto &specials = item_filters.getSpecials();

    auto &jitem = job_items[index];
    auto vector_ids = getVectorIds(out, jitem, ignore_filters);

    ItemFilter filter = filters[index];
    set<string> special = specials;
    if (ignore_filters || counts) {
        // don't filter by material; we want counts for all materials
        filter.setMaterialMask(0);
        filter.setMaterials(set<MaterialInfo>());
        special.clear();
    }
    if (ignore_quality) {
        filter.setMinQuality(df::item_quality::Ordinary);
        filter.setMaxQuality(df::item_quality::Artifact);
    }

    update_walkability_groups(); // ensure that itemPassesScreen is accurate

    int count = 0;
    for (auto vector_id : vector_ids) {
        auto other_id = ENUM_ATTR(job_item_vector_id, other, vector_id);
        for (auto &item : df::global::world->items.other[other_id]) {
            if (itemPassesScreen(out, item) && matchesFilters(item, jitem, heat, filter, special)) {
                if (item_ids)
                    item_ids->emplace_back(item->id);
                if (counts) {
                    MaterialInfo mi;
                    mi.decode(item);
                    (*counts)[mi]++;
                }
                ++count;
            }
        }
    }

    DEBUG(control,out).print("found matches %d\n", count);
    return count;
}

static int getAvailableItems(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering getAvailableItems building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    vector<int> item_ids;
    scanAvailableItems(*out, type, subtype, custom, index, true, false, NULL, &item_ids);
    Lua::PushVector(L, item_ids);
    return 1;
}

static int getAvailableItemsByHeat(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    HeatSafety heat = (HeatSafety)luaL_checkint(L, 5);
    DEBUG(control,*out).print(
            "entering getAvailableItemsByHeat building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    vector<int> item_ids;
    scanAvailableItems(*out, type, subtype, custom, index, true, true, &heat, &item_ids);
    Lua::PushVector(L, item_ids);
    return 1;
}

static int getGlobalSettings(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering getGlobalSettings\n");
    map<string, bool> settings;
    settings.emplace("blocks", config.get_bool(CONFIG_BLOCKS));
    settings.emplace("logs", config.get_bool(CONFIG_LOGS));
    settings.emplace("boulders", config.get_bool(CONFIG_BOULDERS));
    settings.emplace("bars", config.get_bool(CONFIG_BARS));
    settings.emplace("reconstruct", config.get_bool(CONFIG_RECONSTRUCT));
    Lua::Push(L, settings);
    return 1;
}

static int countAvailableItems(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    DEBUG(control,out).print(
            "entering countAvailableItems building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    int count = scanAvailableItems(out, type, subtype, custom, index, false, false);
    if (count)
        return count;

    // nothing in stock; return how many are waiting in line as a negative
    BuildingTypeKey key(type, subtype, custom);
    auto &job_items = get_job_items(out, key);
    if (index < 0 || job_items.size() <= (size_t)index)
        return 0;
    auto &jitem = job_items[index];

    for (auto &entry : planned_buildings) {
        auto &pb = entry.second;
        // don't actually remove bad buildings from the list while we're
        // actively iterating through that list
        auto bld = pb.getBuildingIfValidOrRemoveIfNot(out, true);
        if (!bld || bld->jobs.size() != 1)
            continue;
        for (auto pb_jitem : bld->jobs[0]->job_items.elements) {
            if (pb_jitem->item_type == jitem->item_type && pb_jitem->item_subtype == jitem->item_subtype)
                count -= pb_jitem->quantity;
        }
    }
    return count;
}

static bool hasFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    TRACE(control,out).print("entering hasFilter\n");
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
        return false;
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(out, key);
    if (index < 0 || filters.getItemFilters().size() <= (size_t)index)
        return false;
    return !filters.getItemFilters()[index].isEmpty();
}

static void clearFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    TRACE(control,out).print("entering clearFilter\n");
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(out, key);
    if (index < 0 || filters.getItemFilters().size() <= (size_t)index)
        return;
    ItemFilter filter = filters.getItemFilters()[index];
    filter.clear();
    filters.setItemFilter(out, filter, index);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

static int setMaterialMaskFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering setMaterialMaskFilter building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(*out, key).getItemFilters();
    if (index < 0 || filters.size() <= (size_t)index)
        return 0;
    uint32_t mask = 0;
    vector<string> cats;
    Lua::GetVector(L, cats, 5);
    for (auto &cat : cats) {
        if (cat == "stone")
            mask |= stone_cat.whole;
        else if (cat == "wood")
            mask |= wood_cat.whole;
        else if (cat == "metal")
            mask |= metal_cat.whole;
        else if (cat == "glass")
            mask |= glass_cat.whole;
        else if (cat == "gem")
            mask |= gem_cat.whole;
        else if (cat == "clay")
            mask |= clay_cat.whole;
        else if (cat == "cloth")
            mask |= cloth_cat.whole;
        else if (cat == "silk")
            mask |= silk_cat.whole;
        else if (cat == "yarn")
            mask |= yarn_cat.whole;
    }
    DEBUG(control,*out).print(
            "setting material mask filter for building_type=%d subtype=%d custom=%d index=%d to %x\n",
            type, subtype, custom, index, mask);
    ItemFilter filter = filters[index];
    filter.setMaterialMask(mask);
    set<MaterialInfo> new_mats;
    if (mask) {
        // remove materials from the list that don't match the mask
        const auto &mats = filter.getMaterials();
        const df::dfhack_material_category mat_mask(mask);
        for (auto & mat : mats) {
            if (mat.matches(mat_mask))
                new_mats.emplace(mat);
        }
    }
    filter.setMaterials(new_mats);
    get_item_filters(*out, key).setItemFilter(*out, filter, index);
    Lua::CallLuaModuleFunction(*out, "plugins.buildingplan", "signal_reset");
    return 0;
}

static int getMaterialMaskFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering getMaterialFilter building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(*out, key);
    if (index < 0 || filters.getItemFilters().size() <= (size_t)index)
        return 0;
    map<string, bool> ret;
    uint32_t bits = filters.getItemFilters()[index].getMaterialMask().whole;
    ret.emplace("unset", !bits);
    ret.emplace("stone", !bits || bits & stone_cat.whole);
    ret.emplace("wood", !bits || bits & wood_cat.whole);
    ret.emplace("metal", !bits || bits & metal_cat.whole);
    ret.emplace("glass", !bits || bits & glass_cat.whole);
    ret.emplace("gem", !bits || bits & gem_cat.whole);
    ret.emplace("clay", !bits || bits & clay_cat.whole);
    ret.emplace("cloth", !bits || bits & cloth_cat.whole);
    ret.emplace("silk", !bits || bits & silk_cat.whole);
    ret.emplace("yarn", !bits || bits & yarn_cat.whole);
    Lua::Push(L, ret);
    return 1;
}

static int setMaterialFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering setMaterialFilter building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(*out, key).getItemFilters();
    if (index < 0 || filters.size() <= (size_t)index)
        return 0;
    set<MaterialInfo> mats;
    vector<string> matstrs;
    Lua::GetVector(L, matstrs, 5);
    for (auto &mat : matstrs) {
        if (mat_cache.count(mat))
            mats.emplace(mat_cache.at(mat).first);
    }
    DEBUG(control,*out).print(
            "setting material filter for building_type=%d subtype=%d custom=%d index=%d to %zd materials\n",
            type, subtype, custom, index, mats.size());
    ItemFilter filter = filters[index];
    filter.setMaterials(mats);
    // ensure relevant masks are explicitly enabled
    df::dfhack_material_category mask = filter.getMaterialMask();
    if (!mats.size())
        mask.whole = 0; // if all materials are disabled, reset the mask
    for (auto & mat : mats) {
        if (mat.matches(stone_cat))
            mask.whole |= stone_cat.whole;
        else if (mat.matches(wood_cat))
            mask.whole |= wood_cat.whole;
        else if (mat.matches(metal_cat))
            mask.whole |= metal_cat.whole;
        else if (mat.matches(glass_cat))
            mask.whole |= glass_cat.whole;
        else if (mat.matches(gem_cat))
            mask.whole |= gem_cat.whole;
        else if (mat.matches(clay_cat))
            mask.whole |= clay_cat.whole;
        else if (mat.matches(cloth_cat))
            mask.whole |= cloth_cat.whole;
        else if (mat.matches(silk_cat))
            mask.whole |= silk_cat.whole;
        else if (mat.matches(yarn_cat))
            mask.whole |= yarn_cat.whole;
    }
    filter.setMaterialMask(mask.whole);
    get_item_filters(*out, key).setItemFilter(*out, filter, index);
    Lua::CallLuaModuleFunction(*out, "plugins.buildingplan", "signal_reset");
    return 0;
}

static int getMaterialFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering getMaterialFilter building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(*out, key).getItemFilters();
    if (index < 0 || filters.size() <= (size_t)index)
        return 0;
    const auto &mat_filter = filters[index].getMaterials();
    map<MaterialInfo, int32_t> counts;
    scanAvailableItems(*out, type, subtype, custom, index, false, false, NULL, NULL, &counts);
    HeatSafety heat = get_heat_safety_filter(key);
    const df::job_item *jitem = get_job_items(*out, key)[index];
    // name -> {count=int, enabled=bool, category=string, heat=string}
    map<string, map<string, string>> ret;
    for (auto & entry : mat_cache) {
        auto &mat = entry.second.first;
        if (!mat.matches(jitem))
            continue;
        if (!matchesHeatSafety(mat.type, mat.index, heat))
            continue;
        string heat_safety = "";
        if (matchesHeatSafety(mat.type, mat.index, HEAT_SAFETY_MAGMA))
            heat_safety = "magma-safe";
        else if (matchesHeatSafety(mat.type, mat.index, HEAT_SAFETY_FIRE))
            heat_safety = "fire-safe";
        auto &name = entry.first;
        auto &cat = entry.second.second;
        map<string, string> props;
        string count = "0";
        if (counts.count(mat))
            count = int_to_string(counts.at(mat));
        props.emplace("count", count);
        props.emplace("enabled", (!mat_filter.size() || mat_filter.count(mat)) ? "true" : "false");
        props.emplace("category", cat);
        ret.emplace(name, props);
    }
    Lua::Push(L, ret);
    return 1;
}

static void setChooseItems(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int choose) {
    DEBUG(control,out).print(
            "entering setChooseItems building_type=%d subtype=%d custom=%d choose=%d\n",
            type, subtype, custom, choose);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(out, key);
    filters.setChooseItems(choose);
    // no need to reset signal; no change to the state of any other UI element
}

static int getChooseItems(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    DEBUG(control,*out).print(
            "entering getChooseItems building_type=%d subtype=%d custom=%d\n",
            type, subtype, custom);
    BuildingTypeKey key(type, subtype, custom);
    Lua::Push(L, get_item_filters(*out, key).getChooseItems());
    return 1;
}

static void setHeatSafetyFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int heat) {
    DEBUG(control,out).print("entering setHeatSafetyFilter\n");
    BuildingTypeKey key(type, subtype, custom);
    if (heat == HEAT_SAFETY_FIRE || heat == HEAT_SAFETY_MAGMA)
        cur_heat_safety[key] = (HeatSafety)heat;
    else
        cur_heat_safety.erase(key);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

static int getHeatSafetyFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    DEBUG(control,*out).print(
            "entering getHeatSafetyFilter building_type=%d subtype=%d custom=%d\n",
            type, subtype, custom);
    BuildingTypeKey key(type, subtype, custom);
    HeatSafety heat = get_heat_safety_filter(key);
    Lua::Push(L, heat);
    return 1;
}

static void setSpecial(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, string special, bool val) {
    DEBUG(control,out).print("entering setSpecial\n");
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(out, key);
    filters.setSpecial(special, val);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

static int getSpecials(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    DEBUG(control,*out).print(
            "entering getSpecials building_type=%d subtype=%d custom=%d\n",
            type, subtype, custom);
    BuildingTypeKey key(type, subtype, custom);
    Lua::Push(L, get_item_filters(*out, key).getSpecials());
    return 1;
}

static void setQualityFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index,
        int decorated, int min_quality, int max_quality) {
    DEBUG(control,out).print("entering setQualityFilter\n");
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(out, key).getItemFilters();
    if (index < 0 || filters.size() <= (size_t)index)
        return;
    ItemFilter filter = filters[index];
    filter.setDecoratedOnly(decorated != 0);
    filter.setMinQuality(min_quality);
    filter.setMaxQuality(max_quality);
    get_item_filters(out, key).setItemFilter(out, filter, index);
    Lua::CallLuaModuleFunction(out, "plugins.buildingplan", "signal_reset");
}

static int getQualityFilter(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    df::building_type type = (df::building_type)luaL_checkint(L, 1);
    int16_t subtype = luaL_checkint(L, 2);
    int32_t custom = luaL_checkint(L, 3);
    int index = luaL_checkint(L, 4);
    DEBUG(control,*out).print(
            "entering getQualityFilter building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &filters = get_item_filters(*out, key).getItemFilters();
    if (index < 0 || filters.size() <= (size_t)index)
        return 0;
    auto &filter = filters[index];
    map<string, int> ret;
    ret.emplace("decorated", filter.getDecoratedOnly());
    ret.emplace("min_quality", filter.getMinQuality());
    ret.emplace("max_quality", filter.getMaxQuality());
    Lua::Push(L, ret);
    return 1;
}

static bool validate_pb(color_ostream &out, df::building *bld, int index) {
    if (!isPlannedBuilding(out, bld) || bld->jobs.size() != 1)
        return false;

    auto &job_items = bld->jobs[0]->job_items.elements;
    if ((int)job_items.size() <= index)
        return false;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    if ((int)pb.vector_ids.size() <= index)
        return false;

    return true;
}

static string getDescString(color_ostream &out, df::building *bld, int index) {
    DEBUG(control,out).print("entering getDescString\n");
    if (!validate_pb(out, bld, index))
        return "INVALID";

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto & jitems = bld->jobs[0]->job_items.elements;
    const size_t num_job_items = jitems.size();
    int rev_index = num_job_items - (index + 1);
    auto &jitem = jitems[rev_index];
    return int_to_string(jitem->quantity) + " " + get_desc_string(out, jitem, pb.vector_ids[index]);
}

static int getQueuePosition(color_ostream &out, df::building *bld, int index) {
    TRACE(control,out).print("entering getQueuePosition\n");
    if (!validate_pb(out, bld, index))
        return 0;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto & jitems = bld->jobs[0]->job_items.elements;
    const size_t num_job_items = jitems.size();
    int rev_index = num_job_items - (index + 1);
    auto &job_item = jitems[rev_index];

    if (job_item->quantity <= 0)
        return 0;

    int min_pos = -1;
    for (auto &vec_id : pb.vector_ids[index]) {
        if (!tasks.count(vec_id))
            continue;
        auto &buckets = tasks.at(vec_id);
        string bucket_id = getBucket(*job_item, pb, index);
        if (!buckets.count(bucket_id))
            continue;
        int bucket_pos = -1;
        for (auto &task : buckets.at(bucket_id)) {
            ++bucket_pos;
            if (bld->id == task.first && rev_index == task.second)
                break;
        }
        if (bucket_pos++ >= 0)
            min_pos = min_pos < 0 ? bucket_pos : std::min(min_pos, bucket_pos);
    }

    return min_pos < 0 ? 0 : min_pos;
}

static void makeTopPriority(color_ostream &out, df::building *bld) {
    DEBUG(control,out).print("entering makeTopPriority\n");
    if (!validate_pb(out, bld, 0))
        return;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto &job_items = bld->jobs[0]->job_items.elements;
    const int num_job_items = (int)job_items.size();

    for (int index = 0; index < num_job_items; ++index) {
        int rev_index = num_job_items - (index + 1);
        for (auto &vec_id : pb.vector_ids[index]) {
            if (!tasks.count(vec_id))
                continue;
            auto &buckets = tasks.at(vec_id);
            string bucket_id = getBucket(*job_items[rev_index], pb, index);
            if (!buckets.count(bucket_id))
                continue;
            auto &bucket = buckets.at(bucket_id);
            for (auto taskit = bucket.begin(); taskit != bucket.end(); ++taskit) {
                if (bld->id == taskit->first && rev_index == taskit->second) {
                    auto task_bld_id = taskit->first;
                    auto task_job_item_idx = taskit->second;
                    bucket.erase(taskit);
                    bucket.emplace_front(task_bld_id, task_job_item_idx);
                    break;
                }
            }
        }
    }
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(printStatus),
    DFHACK_LUA_FUNCTION(setSetting),
    DFHACK_LUA_FUNCTION(resetFilters),
    DFHACK_LUA_FUNCTION(isPlannableBuilding),
    DFHACK_LUA_FUNCTION(isPlannedBuilding),
    DFHACK_LUA_FUNCTION(addPlannedBuilding),
    DFHACK_LUA_FUNCTION(doCycle),
    DFHACK_LUA_FUNCTION(scheduleCycle),
    DFHACK_LUA_FUNCTION(countAvailableItems),
    DFHACK_LUA_FUNCTION(hasFilter),
    DFHACK_LUA_FUNCTION(clearFilter),
    DFHACK_LUA_FUNCTION(setChooseItems),
    DFHACK_LUA_FUNCTION(setHeatSafetyFilter),
    DFHACK_LUA_FUNCTION(setSpecial),
    DFHACK_LUA_FUNCTION(setQualityFilter),
    DFHACK_LUA_FUNCTION(getDescString),
    DFHACK_LUA_FUNCTION(getQueuePosition),
    DFHACK_LUA_FUNCTION(makeTopPriority),
    DFHACK_LUA_FUNCTION(setIgnoreBurrow),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getGlobalSettings),
    DFHACK_LUA_COMMAND(getAvailableItems),
    DFHACK_LUA_COMMAND(getAvailableItemsByHeat),
    DFHACK_LUA_COMMAND(setMaterialMaskFilter),
    DFHACK_LUA_COMMAND(getMaterialMaskFilter),
    DFHACK_LUA_COMMAND(setMaterialFilter),
    DFHACK_LUA_COMMAND(getMaterialFilter),
    DFHACK_LUA_COMMAND(getChooseItems),
    DFHACK_LUA_COMMAND(getHeatSafetyFilter),
    DFHACK_LUA_COMMAND(getSpecials),
    DFHACK_LUA_COMMAND(getQualityFilter),
    DFHACK_LUA_END
};
