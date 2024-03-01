#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/construction_type.h"
#include "df/coord.h"
#include "df/job.h"
#include "df/tile_designation.h"
#include "df/tile_occupancy.h"
#include "df/world.h"

#include <bitset>
#include <functional>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;
using std::views::transform;

using namespace DFHack;

DFHACK_PLUGIN("suspendmanager");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(suspendmanager, control, DebugCategory::LINFO);
    DBG_DECLARE(suspendmanager, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_PREVENT_BLOCKING = 1,
};


static const int32_t CYCLE_TICKS = 1213; // about one day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle



/////////////////////////////////////////////////////////////////////////////////
///                          Main Logic                                     /////
/////////////////////////////////////////////////////////////////////////////////

enum Reason {
    //The job is under water and dwarves will suspend the job when starting it
    UNDER_WATER = 1,
    // The job is planned by buildingplan, but not yet ready to start
    BUILDINGPLAN = 2,
    // Fuzzy risk detection of jobs blocking each other in shapes like corners
    RISK_BLOCKING = 3,
    // Building job on top of an erasable designation (smoothing, carving, ...)
    ERASE_DESIGNATION = 4,
    // Blocks a dead end (either a corridor or on top of a wall)
    DEADEND = 5,
    // Would cave in immediately on completion
    UNSUPPORTED = 6
};

inline bool isExternalReason(Reason reason) {
    return reason == Reason::BUILDINGPLAN || reason == Reason::UNDER_WATER;
}

static string reasonToString(Reason reason) {
  switch (reason) {
    case Reason::DEADEND:
        return "Blocks another build job";
    case Reason::RISK_BLOCKING:
        return "May block another build job";
    case Reason::UNSUPPORTED:
        return "Would collapse immediately";
    case Reason::ERASE_DESIGNATION:
        return "Waiting for carve/smooth/engrave";
    default:
        return "External reason";
    }
}

using df::coord;

// set() is constexpr starting with C++23

// impassible constructions
static const std::bitset<64> construction_impassible = std::bitset<64>()
    .set(construction_type::Wall)
    .set(construction_type::Fortification);

// constructions requiring same support as walls
static const std::bitset<64> construction_wall_support = std::bitset<64>()
    .set(construction_type::Wall)
    .set(construction_type::Fortification)
    .set(construction_type::UpStair)
    .set(construction_type::UpDownStair);

// constructions requiring same support as floors
static const std::bitset<64> construction_floor_support = std::bitset<64>()
    .set(construction_type::Floor)
    .set(construction_type::DownStair)
    .set(construction_type::Ramp)
    .set(construction_type::TrackN)
    .set(construction_type::TrackS)
    .set(construction_type::TrackE)
    .set(construction_type::TrackW)
    .set(construction_type::TrackNS)
    .set(construction_type::TrackNE)
    .set(construction_type::TrackSE)
    .set(construction_type::TrackSW)
    .set(construction_type::TrackEW)
    .set(construction_type::TrackNSE)
    .set(construction_type::TrackNSW)
    .set(construction_type::TrackNEW)
    .set(construction_type::TrackSEW)
    .set(construction_type::TrackNSEW)
    .set(construction_type::TrackRampN)
    .set(construction_type::TrackRampS)
    .set(construction_type::TrackRampE)
    .set(construction_type::TrackRampW)
    .set(construction_type::TrackRampNS)
    .set(construction_type::TrackRampNE)
    .set(construction_type::TrackRampNW)
    .set(construction_type::TrackRampSE)
    .set(construction_type::TrackRampSW)
    .set(construction_type::TrackRampEW)
    .set(construction_type::TrackRampNSE)
    .set(construction_type::TrackRampNSW)
    .set(construction_type::TrackRampNEW)
    .set(construction_type::TrackRampSEW)
    .set(construction_type::TrackRampNSEW);


static const std::bitset<64> shape_wall_support = std::bitset<64>()
    .set(tiletype_shape::WALL)
    .set(tiletype_shape::FORTIFICATION)
    .set(tiletype_shape::STAIR_UP)
    .set(tiletype_shape::STAIR_UPDOWN);

static const std::bitset<64> shape_floor_support = std::bitset<64>()
    .set(tiletype_shape::FLOOR)
    .set(tiletype_shape::STAIR_DOWN)
    .set(tiletype_shape::RAMP)
    .set(tiletype_shape::BOULDER)
    .set(tiletype_shape::PEBBLES)
    .set(tiletype_shape::SAPLING)
    .set(tiletype_shape::BROOK_BED)
    .set(tiletype_shape::BROOK_TOP)
    .set(tiletype_shape::SHRUB)
    .set(tiletype_shape::TWIG)
    .set(tiletype_shape::BRANCH)
    .set(tiletype_shape::TRUNK_BRANCH);

static const std::bitset<64> building_impassible = std::bitset<64>()
    .set(building_type::Floodgate)
    .set(building_type::Statue)
    .set(building_type::WindowGlass)
    .set(building_type::WindowGem)
    .set(building_type::GrateWall)
    .set(building_type::BarsVertical);


// using offset = std::tuple<int,int,int> woud be preferable
// However, tuple has non-public members and therefore tuples cannot be template arguments.
struct offset {
    int x,y,z;
    constexpr offset(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {};
};

inline coord operator+(coord pos, offset off) {
    return coord(pos.x + off.x, pos.y + off.y, pos.z + off.z);
}

// allows the idiom:
// "for (auto npos : neighbors | transform(around(pos)))"
static std::function<coord(offset)> around (coord pos) {
    return [pos](offset o){ return pos + o; };
}

/* offsets for various neighborhoods
 * using constexpr would be preferable,
 * but using arrays makes the length of the neighborhood part of its type
*/
static const vector<offset> neighbors {
    offset{ -1,  0,  0 },
    offset{ +1,  0,  0 },
    offset{  0, -1,  0 },
    offset{  0, +1,  0 }
};

static const vector<offset> neighboursWallSupportsWall {
    offset{-1, 0, 0}, // orthogonal same level
    offset{+1, 0, 0},
    offset{0, -1, 0},
    offset{0, +1, 0},
    offset{-1, 0, -1}, // orthogonal level below
    offset{+1, 0, -1},
    offset{0, -1, -1},
    offset{0, +1, -1},
    offset{-1, 0, +1}, // orthogonal level above
    offset{+1, 0, +1},
    offset{0, -1, +1},
    offset{0, +1, +1},
    offset{0, 0, -1}, // directly below
    offset{0, 0, +1}  // directly above
};

static const vector<offset> neighboursFloorSupportsWall {
    offset{-1, 0, 0},  // orthogonal same level
    offset{+1, 0, 0},
    offset{0, -1, 0},
    offset{0, +1, 0},
    offset{0, 0, +1},  // directly above
    offset{-1, 0, +1}, // orthogonal level above
    offset{+1, 0, +1},
    offset{0, -1, +1},
    offset{0, +1, +1}
};

static const vector<offset> neighboursWallSupportsFloor {
    offset{-1, 0, 0}, // orthogonal same level
    offset{+1, 0, 0},
    offset{0, -1, 0},
    offset{0, +1, 0},
    offset{-1, 0, -1}, // orthogonal level below
    offset{+1, 0, -1},
    offset{0, -1, -1},
    offset{0, +1, -1},
    offset{0, 0, -1}, // directly below
};


static const vector<offset> neighboursFloorSupportsFloor {
    offset{ -1,  0,  0 },
    offset{ +1,  0,  0 },
    offset{  0, -1,  0 },
    offset{  0, +1,  0 }
};

class SuspendManager {
private:
    static constexpr size_t max_deadend_depth = 1000;

    static void suspend(df::job* job) {
        job->flags.bits.suspend = true;
        job->flags.bits.working = false;
        Job::removeWorker(job, 0);
    }

    static void unsuspend(df::job* job) {
        job->flags.bits.suspend = false;
    }

    static bool walkable (coord pos) { return Maps::getWalkableGroup(pos) > 0; }

    static bool buildingOnDesignation(df::building *building){
        CHECK_NULL_POINTER(building);
        auto z = building->z;
        for (auto x = building->x1; x <= building->x2; ++x){
            for (auto y = building->y1; x <= building->y2; ++x){
                auto flags = Maps::getTileDesignation(x,y,z);
                if (flags && (
                    flags->bits.dig != df::tile_dig_designation::No ||
                    flags->bits.smooth > 0))
                    return true;
                auto occupancy = Maps::getTileOccupancy(x,y,z);
                if (occupancy && (
                    occupancy->bits.carve_track_north ||
                    occupancy->bits.carve_track_east ||
                    occupancy->bits.carve_track_south ||
                    occupancy->bits.carve_track_west))
                    return true;
            }
        }
        return false;
    }

    // check if the tile is suitable tile to stand on for construction (walkable & not a tree branch)
    static bool isSuitableAccess (coord pos) {
        auto tile_type = Maps::getTileType(pos);
        if (!tile_type)
            return false; // no tiletype, likely out of bound
        auto shape = tileShape(*tile_type);
        if (shape == df::enums::tiletype_shape::BRANCH ||
            shape == df::enums::tiletype_shape::TRUNK_BRANCH)
            return false;

        return walkable(pos);
    }

    static bool tileHasSupportWall(coord pos) {
        auto tile_type = Maps::getTileType(pos);
        if (tile_type) {
            auto shape = tileShape(*tile_type);
            return shape == df::enums::tiletype_shape::NONE ? false : shape_wall_support[shape];
        }
        return false;
    }

    static bool tileHasSupportFloor(coord pos) {
        auto tile_type = Maps::getTileType(pos);
        if (tile_type) {
            auto shape = tileShape(*tile_type);
            return shape == df::enums::tiletype_shape::NONE ? false : shape_floor_support[shape];
        }
        return false;
    }

    static bool tileHasSupportBuilding(coord pos) {
        auto building = Buildings::findAtTile(pos);
        if (building)
            return building->getType() == df::building_type::Support && building->flags.bits.exists;
        return false;
    }


    static bool isImpassable(df::building* building) {
        auto type = building->getType();
        if (type == building_type::Construction) {
            return construction_impassible[building->getSubtype()];
        }
        else return building_impassible[type];
    }

    static bool hasWalkableNeighbor(coord pos) {
        return std::ranges::any_of(neighbors | transform(around(pos)), walkable);
    }


    static df::building* plannedImpassibleAt(coord pos) {
        auto building = Buildings::findAtTile(pos);
        if (!building)
            return nullptr;
        if (!building->flags.bits.exists && isImpassable(building))
            return building;
        return nullptr;
    }

    static int riskOfStuckConstructionAt(coord pos) {
        auto risk = 0;
        for (auto npos : neighbors | transform(around(pos))) {
            if (!isSuitableAccess(npos)) { // original used walkable
                ++risk; // one access blocked, increase danger
            } else if (!plannedImpassibleAt(npos)) {
                // walkable neighbour with no plan to build a wall, no danger
                return -1;
            }
        }
        return risk;
    }



    // return true if this job is at risk of blocking another one
    static bool riskBlocking(color_ostream &out, df::job* job) {
        if (job->job_type != job_type::ConstructBuilding)
            return false;
        TRACE(cycle,out).print("risk blocking: check construction job %d\n", job->id);

        auto building = Job::getHolder(job);
        if (!building || !isImpassable(building))
            return false; // not building a blocking construction, no risk

        coord pos(building->centerx,building->centery,building->z);
        if (!isSuitableAccess(pos))
            return false; // construction is on a non-walkable tile, can't block

        auto risk = riskOfStuckConstructionAt(pos);
        TRACE(cycle,out).print("  risk is %d\n", risk);

        // TOTHINK: on a large grid, this will compute the same risk up to 5 times
        for (auto npos : neighbors | transform(around(pos))) {
            if (plannedImpassibleAt(npos) && riskOfStuckConstructionAt(npos) > risk)
                return true; // neighbour job is at greater risk of getting stuck
        }

        return false;
    }

    static bool constructionIsUnsupported(color_ostream &out, df::job* job)
    {
        if (!isConstructionJob(job))
            return false;

        auto building = Job::getHolder(job);
        if (!building || building->getType() != df::building_type::Construction)
            return false;

        TRACE(cycle,out).print("check (construction) construction job %d for support\n", job->id);

        coord pos(building->centerx,building->centery,building->z);

        // if no neighbour is walkable, it can't be constructed now anyways
        if (!hasWalkableNeighbor(pos))
            return false;



        auto constr_type = building->getSubtype();

        const vector<offset> *wall_would_support, *floor_would_support;
        vector<offset> supportbld_would_support;

        if (construction_floor_support[constr_type]){
            wall_would_support = &neighboursWallSupportsFloor;
            floor_would_support = &neighboursFloorSupportsFloor;
            supportbld_would_support = vector<offset>{offset(0,0,-1)};
        } else if (construction_wall_support[constr_type]){
            wall_would_support = &neighboursWallSupportsWall;
            floor_would_support = &neighboursFloorSupportsWall;
            supportbld_would_support = vector<offset>{offset(0,0,-1), offset(0,0,+1)};
        } else {
            return false; // some unknown construction - don't suspend
        }

        for (auto npos : *wall_would_support | transform(around(pos)))
            if (tileHasSupportWall(npos)) return false;
        for (auto npos : *floor_would_support | transform(around(pos)))
            if (tileHasSupportFloor(npos)) return false;
        for (auto npos : supportbld_would_support | transform(around(pos)))
            if (tileHasSupportBuilding(npos)) return false;

        return true;
    }

    void suspendDeadend (color_ostream &out, df::job* job) {
        auto building = Job::getHolder(job);
        if (!building) return;
        coord pos(building->centerx,building->centery,building->z);

        for (size_t count = 0; count < max_deadend_depth; ++count){

            df::building* exit = nullptr;
            for (auto npos : neighbors | transform(around(pos))) {
                if (!isSuitableAccess(npos))
                    continue; // non walkable neighbour, not an exit

                auto impassiblePlan = plannedImpassibleAt(npos);
                if (!impassiblePlan)
                    // walkable neighbour with no building scheduled, not in a dead end
                    return;

                if (leadsToDeadend.contains(impassiblePlan->id))
                    continue; // already visited, not an exit

                if (exit)
                    return; // more than one exit, not in a dead end

                exit = impassiblePlan;
            }

            if (!exit) return;

            // exit is the single exit point of this corridor, suspend its construction job...
            for (auto exit_job : exit->jobs) {
                if (exit_job->job_type == df::job_type::ConstructBuilding) {
                    suspensions[exit_job->id] = Reason::DEADEND;
                }
            }
            // ...mark the current tile of the corridor as leading to a dead-end...
            leadsToDeadend.insert(building->id);

            // ...and continue the exploration from its position
            building = exit;
            pos = coord(building->centerx,building->centery,building->z);
        }
    }

    void preserveDesigations (df::job* job){
        CHECK_NULL_POINTER(job)
        if (job->job_type == df::job_type::CarveTrack ||
            job->job_type == df::job_type::SmoothFloor ||
            job->job_type == df::job_type::DetailFloor)
        {
            auto building = Buildings::findAtTile(job->pos);
            if (building) {
                for (auto building_job : building->jobs) {
                    if (building_job->job_type == df::job_type::ConstructBuilding) {
                        suspensions[building_job->id] = Reason::ERASE_DESIGNATION;
                    }
                }
            }
        }
    }

    // try to fetch suspension reason for a job without modifying the map
    // (std::map::operator[] inserts the key if it isn't found)
    bool tryGetReason (df::job* job, Reason &reason) {
        auto reason_it = suspensions.find(job->id);
        if (reason_it != suspensions.end()) {
                reason = reason_it->second;
                return true;
        }
        return false;
    }

    std::unordered_map<int,Reason> suspensions;
    std::unordered_set<int> leadsToDeadend;
    size_t num_suspend = 0, num_unsuspend = 0;

public:
    bool prevent_blocking = true;

    // gather some statistics about the last call to do_cycle()
    string getStatus (color_ostream &out) {
        std::map<Reason,int> stats;

        for (auto [_ , reason] : suspensions) {
            stats[reason == Reason::DEADEND ? Reason::RISK_BLOCKING : reason]++;
        }

        std::stringstream res;
        res << "suspended " << num_suspend << " and unsuspend " << num_unsuspend <<  " jobs\n";
        res << "maintaining " << suspensions.size() << " suspension reasons\n";
        for (auto stat : stats) {
            res << std::setw(5) << stat.second << " " << reasonToString(stat.first) << std::endl;
        }

        return res.str();
    }

    void refresh(color_ostream &out)
    {
        DEBUG(cycle,out).print("starting refresh, prevent blocking is %s\n",
                               prevent_blocking ? "true" : "false");
        suspensions.clear();
        leadsToDeadend.clear();

        for (auto job : df::global::world->jobs.list) {

            // check carving/detailing jobs and suspend buildings over them
            preserveDesigations(job);

            // remaining checks only apply to construction jobs
            if (!isConstructionJob(job)) continue;

            // may suspend other jobs, must always be called
            if (prevent_blocking) suspendDeadend(out, job);

            if (suspensions.contains(job->id))
                continue; // we already have a reason to suspend this job

            // external reasons
            if (Maps::getTileDesignation(job->pos)->bits.flow_size > 1) {
                suspensions[job->id]=Reason::UNDER_WATER;
                continue;
            } else if (isBuildingPlanJob(job)) {
                suspensions[job->id]=Reason::BUILDINGPLAN;
                continue;
            }

            if (constructionIsUnsupported(out, job))
                suspensions[job->id]=Reason::UNSUPPORTED;

            if (!prevent_blocking) continue;

            if (riskBlocking(out, job)) {
                suspensions[job->id]=Reason::RISK_BLOCKING;
                continue;
            }

            // protect (unprocessed) designations
            auto building = Job::getHolder(job);
            if (building && buildingOnDesignation(building))
                suspensions[job->id]=Reason::ERASE_DESIGNATION;
        }
        DEBUG(cycle,out).print("finished refresh: found %zu reasons for suspension\n",suspensions.size());
    }

    void do_cycle (color_ostream &out) {
        refresh(out);
        num_suspend = 0, num_unsuspend = 0;

        Reason reason;


        for (auto job : df::global::world->jobs.list) {
            if (!isConstructionJob(job)) continue;

            if (job->flags.bits.suspend && !suspensions.contains(job->id)) {
                unsuspend(job); // suspended for no reason
                ++num_unsuspend;
            } else if (!job->flags.bits.suspend && tryGetReason(job,reason)) {
                if (!isExternalReason(reason)) {
                    suspend(job); // has internal reason to be suspended
                    ++num_suspend;
                }
            }
        }
        DEBUG(cycle,out).print("suspended %zu constructions and unsuspend %zu constructions\n",
                              num_suspend, num_unsuspend);
    }

    static bool isConstructionJob(df::job *job) {
        return job->job_type == job_type::ConstructBuilding;
    }

    bool keptSuspended(df::job *job) {
        Reason reason;
        if (tryGetReason(job,reason) && !isExternalReason(reason))
            return true;
        else
            return false;
    }

    std::string suspensionDescription (df::job *job) {
        Reason reason;
        if (tryGetReason(job,reason)) {
            return reasonToString(reason);
        }
        return "not suspended by suspendmanager";
    }

    /**
     * This is a proxy, since there is currently no (easy) way for C++ plugins
     * to communicate with each other.
     * NOTE: If buidingplan is changed to set the material early
     * (e.g., to solve the issue of some planned buildings not rendering)
     * this will need to be adapted.
     */
    static bool isBuildingPlanJob (df::job* job) {
        auto building = Job::getHolder(job);
        return building && building->mat_type == -1;
    }
};

/////////////////////////////////////////////////////////////////////////////////
///                      Interface for Plugin Manager                       /////
/////////////////////////////////////////////////////////////////////////////////


std::unique_ptr<SuspendManager> suspendmanager_instance;
std::unique_ptr<EventManager::EventHandler> eventhandler_instance;


static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);
static void jobCompletedHandler(color_ostream& out, void* ptr);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    suspendmanager_instance = std::make_unique<SuspendManager>();
    eventhandler_instance = std::make_unique<EventManager::EventHandler>(jobCompletedHandler,1);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
         "Intelligently suspend and unsuspend jobs.",
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
        if (enable) {
             EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, *eventhandler_instance, plugin_self);
             do_cycle(out);
        } else {
             EventManager::unregister(EventManager::EventType::JOB_COMPLETED, *eventhandler_instance, plugin_self);
        }

    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);
    suspendmanager_instance.release();
    eventhandler_instance.release();
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
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
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

    if (parameters[0] == "now") {
        do_cycle(out);
        return CR_OK;
    } else if (parameters[0] == "enable") {
        return plugin_enable(out,true);
    } else if (parameters[0] == "disable") {
        return plugin_enable(out,false);
    } else if (parameters[0] == "set" && parameters[1] == "preventblocking") {
        if (parameters[3] == "true") {
            suspendmanager_instance->prevent_blocking = true;
            if (is_enabled) do_cycle(out);
            return CR_OK;
        } else if (parameters[3] == "false") {
            suspendmanager_instance->prevent_blocking = false;
            if (is_enabled) do_cycle(out);
            return CR_OK;
        } else
            return CR_WRONG_USAGE;
    } else {
        return CR_WRONG_USAGE;
    }

}

static void jobCompletedHandler(color_ostream& out, void* ptr) {
    DEBUG(cycle,out).print("job completed; updating suspensions\n");
    df::job* job = static_cast<df::job*>(ptr);
    if (SuspendManager::isConstructionJob(job))
        do_cycle(out);
}

/////////////////////////////////////////////////////
// cycle logic
//

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    suspendmanager_instance->do_cycle(out);
}


/////////////////////////////////////////////////////
// Lua exports
//

// Return a human readable description of why suspendmanager keeps a job suspended
static std::string suspendmanager_suspensionDescription(df::job *job) {
    if (is_enabled) {
        CHECK_NULL_POINTER(suspendmanager_instance);
        return suspendmanager_instance->suspensionDescription(job);
    } else {
        return "suspendmanager disabled";
    }
}

// check whether suspendmanager keeps a job suspended
static bool suspendmanager_isKeptSuspended(df::job *job) {
    if (is_enabled) {
        CHECK_NULL_POINTER(suspendmanager_instance);
        return suspendmanager_instance->keptSuspended(job);
    } else {
        return false;
    }
}

static void suspendmanager_runOnce(color_ostream &out, bool blocking) {
    auto save = suspendmanager_instance->prevent_blocking;
    suspendmanager_instance->prevent_blocking = blocking;
    do_cycle(out);
    suspendmanager_instance->prevent_blocking = save;
}

static string suspendmanager_getStatus(color_ostream &out) {
    return suspendmanager_instance->getStatus(out);
}

static bool suspendmanager_isBuildingPlanJob(df::job *job) {
    return suspendmanager_instance->isBuildingPlanJob(job);
}


DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(suspendmanager_suspensionDescription),
    DFHACK_LUA_FUNCTION(suspendmanager_isKeptSuspended),
    DFHACK_LUA_FUNCTION(suspendmanager_runOnce),
    DFHACK_LUA_FUNCTION(suspendmanager_getStatus),
    DFHACK_LUA_FUNCTION(suspendmanager_isBuildingPlanJob),
    DFHACK_LUA_END
};
