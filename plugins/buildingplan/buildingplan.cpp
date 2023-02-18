#include "plannedbuilding.h"
#include "buildingplan.h"

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/World.h"

#include "df/item.h"
#include "df/job_item.h"
#include "df/world.h"

using std::map;
using std::string;
using std::unordered_map;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("buildingplan");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(buildingplan, status, DebugCategory::LINFO);
    DBG_DECLARE(buildingplan, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
const string BLD_CONFIG_KEY = string(plugin_name) + "/building";

int get_config_val(PersistentDataItem &c, int index) {
    if (!c.isValid())
        return -1;
    return c.ival(index);
}
bool get_config_bool(PersistentDataItem &c, int index) {
    return get_config_val(c, index) == 1;
}
void set_config_val(PersistentDataItem &c, int index, int value) {
    if (c.isValid())
        c.ival(index) = value;
}
void set_config_bool(PersistentDataItem &c, int index, bool value) {
    set_config_val(c, index, value ? 1 : 0);
}

// building type, subtype, custom
typedef std::tuple<df::building_type, int16_t, int32_t> BuildingTypeKey;

// rotates a size_t value left by count bits
// assumes count is not 0 or >= size_t_bits
// replace this with std::rotl when we move to C++20
static std::size_t rotl_size_t(size_t val, uint32_t count)
{
    static const int size_t_bits = CHAR_BIT * sizeof(std::size_t);
    return val << count | val >> (size_t_bits - count);
}

struct BuildingTypeKeyHash {
    std::size_t operator() (const BuildingTypeKey & key) const {
        // cast first param to appease gcc-4.8, which is missing the enum
        // specializations for std::hash
        std::size_t h1 = std::hash<int32_t>()(static_cast<int32_t>(std::get<0>(key)));
        std::size_t h2 = std::hash<int16_t>()(std::get<1>(key));
        std::size_t h3 = std::hash<int32_t>()(std::get<2>(key));

        return h1 ^ rotl_size_t(h2, 8) ^ rotl_size_t(h3, 16);
    }
};

static PersistentDataItem config;
// for use in counting available materials for the UI
static unordered_map<BuildingTypeKey, vector<df::job_item *>, BuildingTypeKeyHash> job_item_repo;
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
    DEBUG(status,out).print("removing persistent data for building %d\n", id);
    World::DeletePersistentData(bld_config);
    if (planned_buildings.count(id) > 0)
        planned_buildings.erase(id);
}

static const int32_t CYCLE_TICKS = 600; // twice per game day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
void buildingplan_cycle(color_ostream &out, Tasks &tasks,
        unordered_map<int32_t, PlannedBuilding> &planned_buildings);

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

static void validate_config(color_ostream &out, bool verbose = false) {
    if (get_config_bool(config, CONFIG_BLOCKS)
            || get_config_bool(config, CONFIG_BOULDERS)
            || get_config_bool(config, CONFIG_LOGS)
            || get_config_bool(config, CONFIG_BARS))
        return;

    if (verbose)
        out.printerr("all contruction materials disabled; resetting config\n");

    set_config_bool(config, CONFIG_BLOCKS, true);
    set_config_bool(config, CONFIG_BOULDERS, true);
    set_config_bool(config, CONFIG_LOGS, true);
    set_config_bool(config, CONFIG_BARS, false);
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

static void clear_state(color_ostream &out) {
    call_buildingplan_lua(&out, "signal_reset");
    planned_buildings.clear();
    tasks.clear();
    for (auto &entry : job_item_repo) {
        for (auto &jitem : entry.second) {
            delete jitem;
        }
    }
    job_item_repo.clear();
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(status,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
    }
    validate_config(out);

    DEBUG(status,out).print("loading persisted state\n");
    clear_state(out);
    vector<PersistentDataItem> building_configs;
    World::GetPersistentData(&building_configs, BLD_CONFIG_KEY);
    const size_t num_building_configs = building_configs.size();
    for (size_t idx = 0; idx < num_building_configs; ++idx) {
        PlannedBuilding pb(out, building_configs[idx]);
        registerPlannedBuilding(out, pb);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == SC_WORLD_UNLOADED) {
        DEBUG(status,out).print("world unloaded; clearing state for %s\n", plugin_name);
        clear_state(out);
    }
    return CR_OK;
}

static bool cycle_requested = false;

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;
    cycle_requested = false;

    buildingplan_cycle(out, tasks, planned_buildings);
    call_buildingplan_lua(&out, "signal_reset");
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Core::getInstance().isWorldLoaded())
        return CR_OK;

    if (is_enabled &&
            (cycle_requested || world->frame_counter - cycle_timestamp >= CYCLE_TICKS))
        do_cycle(out);
    return CR_OK;
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
vector<df::job_item_vector_id> getVectorIds(color_ostream &out, df::job_item *job_item) {
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
    if (isJobReady(out, job_items)) {
        // all items are already attached
        finalizeBuilding(out, bld);
        return true;
    }

    int num_job_items = job_items.size();
    int32_t id = bld->id;
    for (int job_item_idx = 0; job_item_idx < num_job_items; ++job_item_idx) {
        auto job_item = job_items[job_item_idx];
        auto bucket = getBucket(*job_item);

        // if there are multiple vector_ids, schedule duplicate tasks. after
        // the correct number of items are matched, the extras will get popped
        // as invalid
        for (auto vector_id : pb.vector_ids[job_item_idx]) {
            for (int item_num = 0; item_num < job_item->quantity; ++item_num) {
                tasks[vector_id][bucket].emplace_back(id, job_item_idx);
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

static string get_desc_string(color_ostream &out, df::job_item *jitem,
        const vector<df::job_item_vector_id> &vec_ids) {
    vector<string> descs;
    for (auto &vec_id : vec_ids) {
        df::job_item jitem_copy = *jitem;
        jitem_copy.vector_id = vec_id;
        call_buildingplan_lua(&out, "get_desc", 1, 1,
                [&](lua_State *L) { Lua::Push(L, &jitem_copy); },
                [&](lua_State *L) {
                    descs.emplace_back(lua_tostring(L, -1)); });
    }
    return join_strings(" or ", descs);
}

static void printStatus(color_ostream &out) {
    DEBUG(status,out).print("entering buildingplan_printStatus\n");
    out.print("buildingplan is %s\n\n", is_enabled ? "enabled" : "disabled");
    out.print("Current settings:\n");
    out.print("  use blocks:   %s\n", get_config_bool(config, CONFIG_BLOCKS) ? "yes" : "no");
    out.print("  use boulders: %s\n", get_config_bool(config, CONFIG_BOULDERS) ? "yes" : "no");
    out.print("  use logs:     %s\n", get_config_bool(config, CONFIG_LOGS) ? "yes" : "no");
    out.print("  use bars:     %s\n", get_config_bool(config, CONFIG_BARS) ? "yes" : "no");
    out.print("\n");

    map<string, int32_t> counts;
    int32_t total = 0;
    for (auto &entry : planned_buildings) {
        auto &pb = entry.second;
        auto bld = pb.getBuildingIfValidOrRemoveIfNot(out);
        if (!bld || bld->jobs.size() != 1)
            continue;
        auto &job_items = bld->jobs[0]->job_items;
        if (job_items.size() != pb.vector_ids.size())
            continue;
        int job_item_idx = 0;
        for (auto &vec_ids : pb.vector_ids) {
            auto &jitem = job_items[job_item_idx++];
            int32_t quantity = jitem->quantity;
            if (quantity) {
                counts[get_desc_string(out, jitem, vec_ids)] += quantity;
                total += quantity;
            }
        }
    }

    if (planned_buildings.size()) {
        out.print("Waiting for %d item(s) to be produced for %zd building(s):\n",
                total, planned_buildings.size());
        for (auto &count : counts)
            out.print("  %3d %s\n", count.second, count.first.c_str());
    } else {
        out.print("Currently no planned buildings\n");
    }
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

    validate_config(out, true);
    call_buildingplan_lua(&out, "signal_reset");
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

static int countAvailableItems(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    DEBUG(status,out).print("entering countAvailableItems\n");
    DEBUG(status,out).print(
            "entering countAvailableItems building_type=%d subtype=%d custom=%d index=%d\n",
            type, subtype, custom, index);
    BuildingTypeKey key(type, subtype, custom);
    auto &job_items = job_item_repo[key];
    if (index >= (int)job_items.size()) {
        for (int i = job_items.size(); i <= index; ++i) {
            bool failed = false;
            if (!call_buildingplan_lua(&out, "get_job_item", 4, 1,
                    [&](lua_State *L) {
                        Lua::Push(L, type);
                        Lua::Push(L, subtype);
                        Lua::Push(L, custom);
                        Lua::Push(L, index+1);
                    },
                    [&](lua_State *L) {
                        df::job_item *jitem = Lua::GetDFObject<df::job_item>(L, -1);
                        DEBUG(status,out).print("retrieving job_item for index=%d: %p\n",
                                index, jitem);
                        if (!jitem)
                            failed = true;
                        else
                            job_items.emplace_back(jitem);
                    }) || failed) {
                return 0;
            }
        }
    }

    auto &jitem = job_items[index];
    auto vector_ids = getVectorIds(out, jitem);

    int count = 0;
    for (auto vector_id : vector_ids) {
        auto other_id = ENUM_ATTR(job_item_vector_id, other, vector_id);
        for (auto &item : df::global::world->items.other[other_id]) {
            if (itemPassesScreen(item) && matchesFilters(item, jitem))
                ++count;
        }
    }

    DEBUG(status,out).print("found matches %d\n", count);
    return count;
}

static bool hasFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    DEBUG(status,out).print("entering hasFilter\n");
    return false;
}

static void setFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    DEBUG(status,out).print("entering setFilter\n");
}

static void clearFilter(color_ostream &out, df::building_type type, int16_t subtype, int32_t custom, int index) {
    DEBUG(status,out).print("entering clearFilter\n");
}

static bool validate_pb(color_ostream &out, df::building *bld, int index) {
    if (!isPlannedBuilding(out, bld) || bld->jobs.size() != 1)
        return false;

    auto &job_items = bld->jobs[0]->job_items;
    if ((int)job_items.size() <= index)
        return false;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    if ((int)pb.vector_ids.size() <= index)
        return false;

    return true;
}

static string getDescString(color_ostream &out, df::building *bld, int index) {
    DEBUG(status,out).print("entering getDescString\n");
    if (!validate_pb(out, bld, index))
        return 0;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto &jitem = bld->jobs[0]->job_items[index];
    return get_desc_string(out, jitem, pb.vector_ids[index]);
}

static int getQueuePosition(color_ostream &out, df::building *bld, int index) {
    DEBUG(status,out).print("entering getQueuePosition\n");
    if (!validate_pb(out, bld, index))
        return 0;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto &job_item = bld->jobs[0]->job_items[index];

    if (job_item->quantity <= 0)
        return 0;

    int min_pos = -1;
    for (auto &vec_id : pb.vector_ids[index]) {
        if (!tasks.count(vec_id))
            continue;
        auto &buckets = tasks.at(vec_id);
        string bucket_id = getBucket(*job_item);
        if (!buckets.count(bucket_id))
            continue;
        int bucket_pos = -1;
        for (auto &task : buckets.at(bucket_id)) {
            ++bucket_pos;
            if (bld->id == task.first && index == task.second)
                break;
        }
        if (bucket_pos++ >= 0)
            min_pos = min_pos < 0 ? bucket_pos : std::min(min_pos, bucket_pos);
    }

    return min_pos < 0 ? 0 : min_pos;
}

static void makeTopPriority(color_ostream &out, df::building *bld) {
    DEBUG(status,out).print("entering makeTopPriority\n");
    if (!validate_pb(out, bld, 0))
        return;

    PlannedBuilding &pb = planned_buildings.at(bld->id);
    auto &job_items = bld->jobs[0]->job_items;

    for (int index = 0; index < (int)job_items.size(); ++index) {
        for (auto &vec_id : pb.vector_ids[index]) {
            if (!tasks.count(vec_id))
                continue;
            auto &buckets = tasks.at(vec_id);
            string bucket_id = getBucket(*job_items[index]);
            if (!buckets.count(bucket_id))
                continue;
            auto &bucket = buckets.at(bucket_id);
            for (auto taskit = bucket.begin(); taskit != bucket.end(); ++taskit) {
                if (bld->id == taskit->first && index == taskit->second) {
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
    DFHACK_LUA_FUNCTION(isPlannableBuilding),
    DFHACK_LUA_FUNCTION(isPlannedBuilding),
    DFHACK_LUA_FUNCTION(addPlannedBuilding),
    DFHACK_LUA_FUNCTION(doCycle),
    DFHACK_LUA_FUNCTION(scheduleCycle),
    DFHACK_LUA_FUNCTION(countAvailableItems),
    DFHACK_LUA_FUNCTION(hasFilter),
    DFHACK_LUA_FUNCTION(setFilter),
    DFHACK_LUA_FUNCTION(clearFilter),
    DFHACK_LUA_FUNCTION(getDescString),
    DFHACK_LUA_FUNCTION(getQueuePosition),
    DFHACK_LUA_FUNCTION(makeTopPriority),
    DFHACK_LUA_END
};
