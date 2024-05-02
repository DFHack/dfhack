#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/building_design.h"
#include "df/item.h"
#include "df/job_item.h"
#include "df/world.h"

#include <queue>
#include <string>
#include <vector>
#include <unordered_map>

using std::map;
using std::pair;
using std::queue;
using std::string;
using std::unordered_map;
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
static const string BLD_CONFIG_KEY = string(plugin_name) + "/building";

enum ConfigValues {
    CONFIG_BLOCKS = 1,
    CONFIG_BOULDERS = 2,
    CONFIG_LOGS = 3,
    CONFIG_BARS = 4,
};

enum BuildingConfigValues {
    BLD_CONFIG_ID = 0,
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

class PlannedBuilding {
public:
    const df::building::key_field_type id;

    PlannedBuilding(color_ostream &out, df::building *building) : id(building->id) {
        DEBUG(status,out).print("creating persistent data for building %d\n", id);
        bld_config = DFHack::World::AddPersistentData(BLD_CONFIG_KEY);
        set_config_val(bld_config, BLD_CONFIG_ID, id);
    }

    PlannedBuilding(DFHack::PersistentDataItem &bld_config)
            : id(get_config_val(bld_config, BLD_CONFIG_ID)), bld_config(bld_config) { }

    void remove(color_ostream &out);

    // Ensure the building still exists and is in a valid state. It can disappear
    // for lots of reasons, such as running the game with the buildingplan plugin
    // disabled, manually removing the building, modifying it via the API, etc.
    df::building * getBuildingIfValidOrRemoveIfNot(color_ostream &out) {
        auto bld = df::building::find(id);
        bool valid = bld && bld->getBuildStage() == 0;
        if (!valid) {
            remove(out);
            return NULL;
        }
        return bld;
    }

private:
    DFHack::PersistentDataItem bld_config;
};

static PersistentDataItem config;
// building id -> PlannedBuilding
unordered_map<int32_t, PlannedBuilding> planned_buildings;
// vector id -> filter bucket -> queue of (building id, job_item index)
map<df::job_item_vector_id, map<string, queue<pair<int32_t, int>>>> tasks;

// note that this just removes the PlannedBuilding. the tasks will get dropped
// as we discover them in the tasks queues and they fail to be found in planned_buildings.
// this "lazy" task cleaning algorithm works because there is no way to
// re-register a building once it has been removed -- if it has been booted out of
// planned_buildings, then it has either been built or desroyed. therefore there is
// no chance of duplicate tasks getting added to the tasks queues.
void PlannedBuilding::remove(color_ostream &out) {
    DEBUG(status,out).print("removing persistent data for building %d\n", id);
    DFHack::World::DeletePersistentData(config);
    if (planned_buildings.count(id) > 0)
        planned_buildings.erase(id);
}

static const int32_t CYCLE_TICKS = 600; // twice per game day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);
static bool registerPlannedBuilding(color_ostream &out, PlannedBuilding & pb);

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
    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(status,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
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
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(status,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(config, CONFIG_BLOCKS, true);
        set_config_bool(config, CONFIG_BOULDERS, true);
        set_config_bool(config, CONFIG_LOGS, true);
        set_config_bool(config, CONFIG_BARS, false);
    }

    DEBUG(status,out).print("loading persisted state\n");
    planned_buildings.clear();
    tasks.clear();
    vector<PersistentDataItem> building_configs;
    World::GetPersistentData(&building_configs, BLD_CONFIG_KEY);
    const size_t num_building_configs = building_configs.size();
    for (size_t idx = 0; idx < num_building_configs; ++idx) {
        PlannedBuilding pb(building_configs[idx]);
        registerPlannedBuilding(out, pb);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        DEBUG(status,out).print("world unloaded; clearing state for %s\n", plugin_name);
        planned_buildings.clear();
        tasks.clear();
    }
    return CR_OK;
}

static bool cycle_requested = false;

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Core::getInstance().isWorldLoaded())
        return CR_OK;

    if (is_enabled &&
            (cycle_requested || world->frame_counter - cycle_timestamp >= CYCLE_TICKS))
        do_cycle(out);
    return CR_OK;
}

static bool call_buildingplan_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(status).print("calling buildingplan lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.buildingplan", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot configure %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_buildingplan_lua(&out, "parse_commandline", parameters.size(), 1,
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
        F(in_building); F(construction); F(in_job);
        F(owned); F(in_chest); F(removed); F(encased);
        F(spider_web);
        #undef F
        whole = flags.whole;
    }
};

static bool itemPassesScreen(df::item * item) {
    static const BadFlags bad_flags;
    return !(item->flags.whole & bad_flags.whole)
        && !item->isAssignedToStockpile();
}

static bool matchesFilters(df::item * item, df::job_item * job_item) {
    // check the properties that are not checked by Job::isSuitableItem()
    if (job_item->item_type > -1 && job_item->item_type != item->getType())
        return false;

    if (job_item->item_subtype > -1 &&
        job_item->item_subtype != item->getSubtype())
        return false;

    if (job_item->flags2.bits.building_material && !item->isBuildMat())
        return false;

    if (job_item->metal_ore > -1 && !item->isMetalOre(job_item->metal_ore))
        return false;

    if (job_item->has_tool_use > df::tool_uses::NONE
        && !item->hasToolUse(job_item->has_tool_use))
        return false;

    return DFHack::Job::isSuitableItem(
            job_item, item->getType(), item->getSubtype())
        && DFHack::Job::isSuitableMaterial(
            job_item, item->getMaterial(), item->getMaterialIndex(),
            item->getType());
}

static bool isJobReady(color_ostream &out, df::job * job) {
    int needed_items = 0;
    for (auto job_item : job->job_items) { needed_items += job_item->quantity; }
    if (needed_items) {
        DEBUG(cycle,out).print("building needs %d more item(s)\n", needed_items);
        return false;
    }
    return true;
}

static bool job_item_idx_lt(df::job_item_ref *a, df::job_item_ref *b) {
    // we want the items in the opposite order of the filters
    return a->job_item_idx > b->job_item_idx;
}

// this function does not remove the job_items since their quantity fields are
// now all at 0, so there is no risk of having extra items attached. we don't
// remove them to keep the "finalize with buildingplan active" path as similar
// as possible to the "finalize with buildingplan disabled" path.
static void finalizeBuilding(color_ostream &out, df::building * bld) {
    DEBUG(cycle,out).print("finalizing building %d\n", bld->id);
    auto job = bld->jobs[0];

    // sort the items so they get added to the structure in the correct order
    std::sort(job->items.begin(), job->items.end(), job_item_idx_lt);

    // derive the material properties of the building and job from the first
    // applicable item. if any boulders are involved, it makes the whole
    // structure "rough".
    bool rough = false;
    for (auto attached_item : job->items) {
        df::item *item = attached_item->item;
        rough = rough || item->getType() == df::item_type::BOULDER;
        if (bld->mat_type == -1) {
            bld->mat_type = item->getMaterial();
            job->mat_type = bld->mat_type;
        }
        if (bld->mat_index == -1) {
            bld->mat_index = item->getMaterialIndex();
            job->mat_index = bld->mat_index;
        }
    }

    if (bld->needsDesign()) {
        auto act = (df::building_actual *)bld;
        if (!act->design)
            act->design = new df::building_design();
        act->design->flags.bits.rough = rough;
    }

    // we're good to go!
    job->flags.bits.suspend = false;
    Job::checkBuildingsNow();
}

static df::building * popInvalidTasks(color_ostream &out, queue<pair<int32_t, int>> & task_queue) {
    while (!task_queue.empty()) {
        auto & task = task_queue.front();
        auto id = task.first;
        if (planned_buildings.count(id) > 0) {
            auto bld = planned_buildings.at(id).getBuildingIfValidOrRemoveIfNot(out);
            if (bld && bld->jobs[0]->job_items[task.second]->quantity)
                return bld;
        }
        DEBUG(cycle,out).print("discarding invalid task: bld=%d, job_item_idx=%d\n", id, task.second);
        task_queue.pop();
    }
    return NULL;
}

static void doVector(color_ostream &out, df::job_item_vector_id vector_id,
        map<string, queue<pair<int32_t, int>>> & buckets) {
    auto other_id = ENUM_ATTR(job_item_vector_id, other, vector_id);
    auto item_vector = df::global::world->items.other[other_id];
    DEBUG(cycle,out).print("matching %zu item(s) in vector %s against %zu filter bucket(s)\n",
          item_vector.size(),
          ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
          buckets.size());
    for (auto item_it = item_vector.rbegin();
            item_it != item_vector.rend();
            ++item_it) {
        auto item = *item_it;
        if (!itemPassesScreen(item))
            continue;
        for (auto bucket_it = buckets.begin(); bucket_it != buckets.end(); ) {
            auto & task_queue = bucket_it->second;
            auto bld = popInvalidTasks(out, task_queue);
            if (!bld) {
                DEBUG(cycle,out).print("removing empty bucket: %s/%s; %zu bucket(s) left\n",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str(),
                      buckets.size() - 1);
                bucket_it = buckets.erase(bucket_it);
                continue;
            }
            auto & task = task_queue.front();
            auto id = task.first;
            auto job = bld->jobs[0];
            auto filter_idx = task.second;
            if (matchesFilters(item, job->job_items[filter_idx])
               && DFHack::Job::attachJobItem(job, item,
                        df::job_item_ref::Hauled, filter_idx))
            {
                MaterialInfo material;
                material.decode(item);
                ItemTypeInfo item_type;
                item_type.decode(item);
                DEBUG(cycle,out).print("attached %s %s to filter %d for %s(%d): %s/%s\n",
                      material.toString().c_str(),
                      item_type.toString().c_str(),
                      filter_idx,
                      ENUM_KEY_STR(building_type, bld->getType()).c_str(),
                      id,
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str());
                // keep quantity aligned with the actual number of remaining
                // items so if buildingplan is turned off, the building will
                // be completed with the correct number of items.
                --job->job_items[filter_idx]->quantity;
                task_queue.pop();
                if (isJobReady(out, job)) {
                    finalizeBuilding(out, bld);
                    planned_buildings.at(id).remove(out);
                }
                if (task_queue.empty()) {
                    DEBUG(cycle,out).print(
                        "removing empty item bucket: %s/%s; %zu left\n",
                        ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                        bucket_it->first.c_str(),
                        buckets.size() - 1);
                    buckets.erase(bucket_it);
                }
                // we found a home for this item; no need to look further
                break;
            }
            ++bucket_it;
        }
        if (buckets.empty())
            break;
    }
}

struct VectorsToScanLast {
    std::vector<df::job_item_vector_id> vectors;
    VectorsToScanLast() {
        // order is important here. we want to match boulders before wood and
        // everything before bars. blocks are not listed here since we'll have
        // already scanned them when we did the first pass through the buckets.
        vectors.push_back(df::job_item_vector_id::BOULDER);
        vectors.push_back(df::job_item_vector_id::WOOD);
        vectors.push_back(df::job_item_vector_id::BAR);
    }
};

static void do_cycle(color_ostream &out) {
    static const VectorsToScanLast vectors_to_scan_last;

    // mark that we have recently run
    cycle_timestamp = world->frame_counter;
    cycle_requested = false;

    DEBUG(cycle,out).print("running %s cycle for %zu registered buildings\n",
            plugin_name, planned_buildings.size());

    for (auto it = tasks.begin(); it != tasks.end(); ) {
        auto vector_id = it->first;
        // we could make this a set, but it's only three elements
        if (std::find(vectors_to_scan_last.vectors.begin(),
                      vectors_to_scan_last.vectors.end(),
                      vector_id) != vectors_to_scan_last.vectors.end()) {
            ++it;
            continue;
        }

        auto & buckets = it->second;
        doVector(out, vector_id, buckets);
        if (buckets.empty()) {
            DEBUG(cycle,out).print("removing empty vector: %s; %zu vector(s) left\n",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            it = tasks.erase(it);
        }
        else
            ++it;
    }
    for (auto vector_id : vectors_to_scan_last.vectors) {
        if (tasks.count(vector_id) == 0)
            continue;
        auto & buckets = tasks[vector_id];
        doVector(out, vector_id, buckets);
        if (buckets.empty()) {
            DEBUG(cycle,out).print("removing empty vector: %s; %zu vector(s) left\n",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            tasks.erase(vector_id);
        }
    }
    DEBUG(cycle,out).print("cycle done; %zu registered building(s) left\n",
          planned_buildings.size());
}

/////////////////////////////////////////////////////
// Lua API
// core will already be suspended when coming in through here
//

static string getBucket(const df::job_item & ji) {
    std::ostringstream ser;

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

    return ser.str();
}

// get a list of item vectors that we should search for matches
static vector<df::job_item_vector_id> getVectorIds(color_ostream &out, df::job_item *job_item)
{
    std::vector<df::job_item_vector_id> ret;

    // if the filter already has the vector_id set to something specific, use it
    if (job_item->vector_id > df::job_item_vector_id::IN_PLAY)
    {
        DEBUG(status,out).print("using vector_id from job_item: %s\n",
              ENUM_KEY_STR(job_item_vector_id, job_item->vector_id).c_str());
        ret.push_back(job_item->vector_id);
        return ret;
    }

    // if the filer is for building material, refer to our global settings for
    // which vectors to search
    if (job_item->flags2.bits.building_material)
    {
        if (get_config_bool(config, CONFIG_BLOCKS))
            ret.push_back(df::job_item_vector_id::BLOCKS);
        if (get_config_bool(config, CONFIG_BOULDERS))
            ret.push_back(df::job_item_vector_id::BOULDER);
        if (get_config_bool(config, CONFIG_LOGS))
            ret.push_back(df::job_item_vector_id::WOOD);
        if (get_config_bool(config, CONFIG_BARS))
            ret.push_back(df::job_item_vector_id::BAR);
    }

    // fall back to IN_PLAY if no other vector was appropriate
    if (ret.empty())
        ret.push_back(df::job_item_vector_id::IN_PLAY);
    return ret;
}
static bool registerPlannedBuilding(color_ostream &out, PlannedBuilding & pb) {
    df::building * bld = pb.getBuildingIfValidOrRemoveIfNot(out);
    if (!bld)
        return false;

    if (bld->jobs.size() != 1) {
        DEBUG(status,out).print("unexpected number of jobs: want 1, got %zu\n", bld->jobs.size());
        return false;
    }
    auto job_items = bld->jobs[0]->job_items;
    int num_job_items = job_items.size();
    if (num_job_items < 1) {
        DEBUG(status,out).print("unexpected number of job items: want >0, got %d\n", num_job_items);
        return false;
    }
    int32_t id = bld->id;
    for (int job_item_idx = 0; job_item_idx < num_job_items; ++job_item_idx) {
        auto job_item = job_items[job_item_idx];
        auto bucket = getBucket(*job_item);
        auto vector_ids = getVectorIds(out, job_item);

        // if there are multiple vector_ids, schedule duplicate tasks. after
        // the correct number of items are matched, the extras will get popped
        // as invalid
        for (auto vector_id : vector_ids) {
            for (int item_num = 0; item_num < job_item->quantity; ++item_num) {
                tasks[vector_id][bucket].push(std::make_pair(id, job_item_idx));
                DEBUG(status,out).print("added task: %s/%s/%d,%d; "
                      "%zu vector(s), %zu filter bucket(s), %zu task(s) in bucket",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket.c_str(), id, job_item_idx, tasks.size(),
                      tasks[vector_id].size(), tasks[vector_id][bucket].size());
            }
        }
    }

    // suspend jobs
    for (auto job : bld->jobs)
        job->flags.bits.suspend = true;

    // add the planned buildings to our register
    planned_buildings.emplace(bld->id, pb);

    return true;
}

static void printStatus(color_ostream &out) {
    DEBUG(status,out).print("entering buildingplan_printStatus\n");
    out.print("buildingplan is %s\n\n", is_enabled ? "enabled" : "disabled");
    out.print("  finding materials for %zd buildings\n", planned_buildings.size());
    out.print("Current settings:\n");
    out.print("  use blocks:   %s\n", get_config_bool(config, CONFIG_BLOCKS) ? "yes" : "no");
    out.print("  use boulders: %s\n", get_config_bool(config, CONFIG_BOULDERS) ? "yes" : "no");
    out.print("  use logs:     %s\n", get_config_bool(config, CONFIG_LOGS) ? "yes" : "no");
    out.print("  use bars:     %s\n", get_config_bool(config, CONFIG_BARS) ? "yes" : "no");
    out.print("\n");
}

static bool setSetting(color_ostream &out, string name, bool value) {
    DEBUG(status,out).print("entering setSetting (%s -> %s)\n", name.c_str(), value ? "true" : "false");
    if (name == "blocks")
        set_config_bool(config, CONFIG_BLOCKS, value);
    else if (name == "boulders")
        set_config_bool(config, CONFIG_BOULDERS, value);
    else if (name == "logs")
        set_config_bool(config, CONFIG_LOGS, value);
    else if (name == "bars")
        set_config_bool(config, CONFIG_BARS, value);
    else {
        out.printerr("unrecognized setting: '%s'\n", name.c_str());
        return false;
    }
    return true;
}

static bool isPlannableBuilding(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom) {
    DEBUG(status,out).print("entering isPlannableBuilding\n");
    int num_filters = 0;
    if (!call_buildingplan_lua(&out, "get_num_filters", 3, 1,
            [&](lua_State *L) {
                Lua::Push(L, type);
                Lua::Push(L, subtype);
                Lua::Push(L, custom);
            },
            [&](lua_State *L) {
                num_filters = lua_tonumber(L, -1);
            })) {
        return false;
    }
    return num_filters >= 1;
}

static bool isPlannedBuilding(color_ostream &out, df::building *bld) {
    TRACE(status,out).print("entering isPlannedBuilding\n");
    return bld && planned_buildings.count(bld->id) > 0;
}

static bool addPlannedBuilding(color_ostream &out, df::building *bld) {
    DEBUG(status,out).print("entering addPlannedBuilding\n");
    if (!bld || planned_buildings.count(bld->id)
            || !isPlannableBuilding(out, bld->getType(), bld->getSubtype(),
                                    bld->getCustomType()))
        return false;
    PlannedBuilding pb(out, bld);
    return registerPlannedBuilding(out, pb);
}

static void doCycle(color_ostream &out) {
    DEBUG(status,out).print("entering doCycle\n");
    do_cycle(out);
}

static void scheduleCycle(color_ostream &out) {
    DEBUG(status,out).print("entering scheduleCycle\n");
    cycle_requested = true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(printStatus),
    DFHACK_LUA_FUNCTION(setSetting),
    DFHACK_LUA_FUNCTION(isPlannableBuilding),
    DFHACK_LUA_FUNCTION(isPlannedBuilding),
    DFHACK_LUA_FUNCTION(addPlannedBuilding),
    DFHACK_LUA_FUNCTION(doCycle),
    DFHACK_LUA_FUNCTION(scheduleCycle),
    DFHACK_LUA_END
};
