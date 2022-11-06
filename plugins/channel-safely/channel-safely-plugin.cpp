/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Nov. 6 2022

 Enable plugin:
 -> build groups
 -> manage designations

 Unpause event:
 -> build groups
 -> manage designations

 Manage Designation(s):
 -> for each group in groups:
    -> does any tile in group have a group above
        -> Yes: set entire group to marker mode
        -> No: activate entire group (still checks is_safe_to_dig_down before activating each designation)

 Job started event:
 -> validate job type (channel)
 -> check pathing:
    -> Can: add job/worker to tracking
    -> Can: set tile to restricted
    -> Cannot: remove worker
    -> Cannot: insta-dig & delete job
    -> Cannot: set designation to Marker Mode (no insta-digging)

 OnUpdate:
 -> check worker location:
    -> CanFall: check if a fall would be safe:
        -> Safe: do nothing
        -> Unsafe: remove worker
        -> Unsafe: insta-dig & delete job (presumes the job is only accessible from directly on the tile)
        -> Unsafe: set designation to Marker Mode (no insta-digging)
 -> check tile occupancy:
    -> HasUnit: check if a fall would be safe:
        -> Safe: do nothing, let them fall
        -> Unsafe: remove worker for 1 tick (test if this "pauses" or cancels the job)
        -> Unsafe: Add feature to teleport unit?

 Job completed event:
 -> validate job type (channel)
 -> verify completion:
    -> IsOpenSpace: mark done
    -> IsOpenSpace: manage tile below
    -> NotOpenSpace: check for designation
        -> HasDesignation: do nothing
        -> NoDesignation: mark done (erases from group)
        -> NoDesignation: manage tile below
*/

#include <plugin.h>
#include <inlines.h>
#include <channel-manager.h>
#include <tile-cache.h>

#include <Debug.h>
#include <LuaTools.h>
#include <LuaWrapper.h>
#include <PluginManager.h>
#include <modules/EventManager.h>

#include <cinttypes>
#include <unordered_map>
#include <unordered_set>
#include <modules/Units.h>
#include <df/report.h>
#include <df/tile_traffic.h>
#include <df/world.h>

// Debugging
namespace DFHack {
    DBG_DECLARE(channelsafely, monitor, DebugCategory::LINFO);
    DBG_DECLARE(channelsafely, manager, DebugCategory::LINFO);
    DBG_DECLARE(channelsafely, groups, DebugCategory::LINFO);
    DBG_DECLARE(channelsafely, jobs, DebugCategory::LINFO);
}

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);

namespace EM = EventManager;
using namespace DFHack;
using namespace EM::EventType;

int32_t mapx, mapy, mapz;
Configuration config;
PersistentDataItem pconfig;
const std::string CONFIG_KEY = std::string(plugin_name) + "/config";
//std::unordered_set<int32_t> active_jobs;

#include <df/block_square_event_designation_priorityst.h>

enum ConfigurationData {
    MONITOR,
    VISION,
    INSTADIG,
    IGNORE_THRESH,
    FALL_THRESH,
    REFRESH_RATE,
    MONITOR_RATE
};

inline void saveConfig() {
    if (pconfig.isValid()) {
        pconfig.ival(MONITOR) = config.monitor_active;
        pconfig.ival(VISION) = config.require_vision;
        pconfig.ival(INSTADIG) = config.insta_dig;
        pconfig.ival(REFRESH_RATE) = config.refresh_freq;
        pconfig.ival(MONITOR_RATE) = config.monitor_freq;
        pconfig.ival(IGNORE_THRESH) = config.ignore_threshold;
        pconfig.ival(FALL_THRESH) = config.fall_threshold;
    }
}

// executes dig designations for the specified tile coordinates
inline bool dig_now(color_ostream &out, const df::coord &map_pos) {
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 2) ||
        !Lua::PushModulePublic(out, L, "plugins.dig-now", "dig_now_tile"))
        return false;

    Lua::Push(L, map_pos);

    if (!Lua::SafeCall(out, L, 1, 1))
        return false;

    return lua_toboolean(L, -1);

}

namespace CSP {
    std::unordered_map<int32_t, int32_t> active_workers;
    std::unordered_map<int32_t, df::coord> last_safe;
    std::unordered_set<df::coord> dignow_queue;

    void UnpauseEvent(){
        INFO(monitor).print("UnpauseEvent()\n");
        ChannelManager::Get().build_groups();
        ChannelManager::Get().manage_groups();
        ChannelManager::Get().debug();
        INFO(monitor).print("UnpauseEvent() exits\n");
    }

    void JobStartedEvent(color_ostream &out, void* p) {
        if (enabled && World::isFortressMode() && Maps::IsValid()) {
            INFO(monitor).print("JobStartedEvent()\n");
            auto job = (df::job*) p;
            // validate job type
            if (is_channel_job(job)) {
                DEBUG(monitor).print(" valid channel job:\n");
                df::unit* worker = Job::getWorker(job);
                // there is a valid worker (living citizen) on the job? right..
                if (worker && Units::isAlive(worker) && Units::isCitizen(worker)) {
                    DEBUG(monitor).print("  valid worker:\n");
                    df::coord local(job->pos);
                    local.x = local.x % 16;
                    local.y = local.y % 16;
                    // check pathing exists to job
                    if (can_reach_designation(worker->pos, job->pos)) {
                        DEBUG(monitor).print("   can path from (" COORD ") to (" COORD ")\n",
                                             COORDARGS(worker->pos), COORDARGS(job->pos));
                        // track workers on jobs
                        active_workers.emplace(job->id, Units::findIndexById(Job::getWorker(job)->id));
                        // set tile to restricted
                        TRACE(monitor).print("   setting job tile to restricted\n");
                        Maps::getTileDesignation(job->pos)->bits.traffic = df::tile_traffic::Restricted;
                    } else {
                        DEBUG(monitor).print("   no path exists to job:\n");
                        // if we can't get there, then we should remove the worker and cancel the job (restore tile designation)
                        Job::removeWorker(job);
                        cancel_job(job);
                        if (!config.insta_dig) {
                            TRACE(monitor).print("    setting marker mode for (" COORD ")\n", COORDARGS(job->pos));
                            // set to marker mode
                            auto occupancy = Maps::getTileOccupancy(job->pos);
                            if (!occupancy) {
                                WARN(monitor).print(" <X> Could not acquire tile occupancy*\n");
                                return;
                            }
                            occupancy->bits.dig_marked = true;
                            // prevent algorithm from re-enabling designation
                            df::map_block* block = Maps::getTileBlock(job->pos);
                            if (!block) {
                                WARN(monitor).print(" <X> Could not acquire block*\n");
                                return;
                            }
                            for (auto &be: block->block_events) { ;
                                if (auto bsedp = virtual_cast<df::block_square_event_designation_priorityst>(be)) {
                                    TRACE(monitor).print("     re-setting priority\n");
                                    bsedp->priority[Coord(local)] = config.ignore_threshold * 1000 + 1;
                                }
                            }
                        } else {
                            TRACE(monitor).print("    deleting job, and queuing insta-dig)\n");
                            // queue digging the job instantly
                            dignow_queue.emplace(job->pos);
                        }
                    }
                }
            }
            INFO(monitor).print(" <- JobStartedEvent() exits normally\n");
        }
    }

    void JobCompletedEvent(color_ostream &out, void* job_ptr) {
        if (enabled && World::isFortressMode() && Maps::IsValid()) {
            INFO(monitor).print("JobCompletedEvent()\n");
            auto job = (df::job*) job_ptr;
            // we only care if the job is a channeling one
            if (ChannelManager::Get().groups.count(job->pos)) {
                // untrack job/worker
                active_workers.erase(job->id);
                // check job outcome
                auto block = Maps::getTileBlock(job->pos);
                df::coord local(job->pos);
                local.x = local.x % 16;
                local.y = local.y % 16;
                const auto &type = block->tiletype[Coord(local)];
                // verify completion
                if (TileCache::Get().hasChanged(job->pos, type)) {
                    // the job can be considered done
                    df::coord below(job->pos);
                    below.z--;
                    WARN(monitor).print(" -> Marking tile done and managing the group below.\n");
                    // mark done and manage below
                    block->designation[Coord(local)].bits.traffic = df::tile_traffic::Normal;
                    ChannelManager::Get().mark_done(job->pos);
                    ChannelManager::Get().manage_group(below);
                    ChannelManager::Get().debug();
                }
            }
            INFO(monitor).print("JobCompletedEvent() exits\n");
        }
    }

    void OnUpdate(color_ostream &out) {
        if (enabled && World::isFortressMode() && Maps::IsValid() && !World::ReadPauseState()) {
            static int32_t last_monitor_tick = df::global::world->frame_counter;
            static int32_t last_refresh_tick = df::global::world->frame_counter;
            int32_t tick = df::global::world->frame_counter;
            if (tick - last_refresh_tick >= config.refresh_freq) {
                last_refresh_tick = tick;
                TRACE(monitor).print("OnUpdate()\n");
                UnpauseEvent();

                if (config.insta_dig) {
                    TRACE(monitor).print(" -> evaluate dignow queue\n");
                    for (const df::coord &pos: dignow_queue) {
                        if (!has_unit(Maps::getTileOccupancy(pos))) {
                            out.print("channel-safely: insta-dig: Digging now!\n");
                            dig_now(out, pos);
                        } else {
                            // todo: teleport?
                            //Units::teleport()
                        }
                    }
                }
            }
            if (config.monitor_active && tick - last_monitor_tick >= config.monitor_freq) {
                last_monitor_tick = tick;
                TRACE(monitor).print("OnUpdate()\n");
                for (df::job_list_link* link = &df::global::world->jobs.list; link != nullptr; link = link->next) {
                    df::job* job = link->item;
                    if (job) {
                        auto iter = active_workers.find(job->id);
                        TRACE(monitor).print(" -> check for job in tracking\n");
                        if (iter != active_workers.end()) {
                            df::unit* unit = df::global::world->units.active[iter->second];
                            TRACE(monitor).print(" -> compare positions of worker and job\n");
                            // check if fall is possible
                            if (unit->pos == job->pos) {
                                // can fall, is safe?
                                TRACE(monitor).print("  equal -> check if safe fall\n");
                                if (!is_safe_fall(job->pos)) {
                                    // unsafe
                                    Job::removeWorker(job);
                                    if (config.insta_dig) {
                                        TRACE(monitor).print(" -> insta-dig\n");
                                        // delete the job
                                        Job::removeJob(job);
                                        // queue digging the job instantly
                                        dignow_queue.emplace(job->pos);
                                        // worker is currently in the air
                                        Units::teleport(unit, last_safe[unit->id]);
                                        last_safe.erase(unit->id);
                                    } else {
                                        TRACE(monitor).print(" -> set marker mode\n");
                                        // set to marker mode
                                        Maps::getTileOccupancy(job->pos)->bits.dig_marked = true;
                                        // prevent algorithm from re-enabling designation
                                        for (auto &be: Maps::getBlock(job->pos)->block_events) { ;
                                            if (auto bsedp = virtual_cast<df::block_square_event_designation_priorityst>(
                                                    be)) {
                                                df::coord local(job->pos);
                                                local.x = local.x % 16;
                                                local.y = local.y % 16;
                                                bsedp->priority[Coord(local)] = config.ignore_threshold * 1000 + 1;
                                                break;
                                            }
                                        }
                                    }
                                }
                            } else {
                                TRACE(monitor).print(" -> save safe position\n");
                                // worker is perfectly safe right now
                                last_safe[unit->id] = unit->pos;
                            }
                        }
                    }
                }
            }
            TRACE(monitor).print("OnUpdate() exits\n");
        }
    }
}

command_result channel_safely(color_ostream &out, std::vector<std::string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "Automatically manage channel designations.",
                                     channel_safely,
                                     false));
    DBG_NAME(monitor).allowed(DFHack::DebugCategory::LERROR);
    DBG_NAME(manager).allowed(DFHack::DebugCategory::LERROR);
    DBG_NAME(groups).allowed(DFHack::DebugCategory::LERROR);
    DBG_NAME(jobs).allowed(DFHack::DebugCategory::LERROR);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    EM::unregisterAll(plugin_self);
    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    pconfig = World::GetPersistentData(CONFIG_KEY);

    if (!pconfig.isValid()) {
        pconfig = World::AddPersistentData(CONFIG_KEY);
        saveConfig();
    } else {
        config.monitor_active = pconfig.ival(MONITOR);
        config.require_vision = pconfig.ival(VISION);
        config.insta_dig = pconfig.ival(INSTADIG);
        config.refresh_freq = pconfig.ival(REFRESH_RATE);
        config.monitor_freq = pconfig.ival(MONITOR_RATE);
        config.ignore_threshold = pconfig.ival(IGNORE_THRESH);
        config.fall_threshold = pconfig.ival(FALL_THRESH);
    }
    return DFHack::CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (enable && !enabled) {
        // register events to check jobs / update tracking
        EM::EventHandler jobStartHandler(CSP::JobStartedEvent, 0);
        EM::EventHandler jobCompletionHandler(CSP::JobCompletedEvent, 0);
        EM::registerListener(EventType::JOB_STARTED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        // manage designations to start off (first time building groups [very important])
        out.print("channel-safely: enabled!\n");
        CSP::UnpauseEvent();
    } else if (!enable) {
        // don't need the groups if the plugin isn't going to be enabled
        EM::unregisterAll(plugin_self);
        out.print("channel-safely: disabled!\n");
    }
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        switch (event) {
            case SC_MAP_LOADED:
                // cache the map size
                Maps::getSize(mapx, mapy, mapz);
            case SC_UNPAUSED:
                // manage all designations on unpause
                CSP::UnpauseEvent();
            default:
                return DFHack::CR_OK;
        }
    }
    switch (event) {
        case SC_WORLD_LOADED:
        case SC_WORLD_UNLOADED:
        case SC_MAP_UNLOADED:
            // destroy any old group data
            out.print("channel-safely: unloading data!\n");
            ChannelManager::Get().destroy_groups();
        case SC_MAP_LOADED:
            // cache the map size
            Maps::getSize(mapx, mapy, mapz);
        default:
            return DFHack::CR_OK;
    }
}

DFhackCExport command_result plugin_onupdate(color_ostream &out, state_change_event event) {
    CSP::OnUpdate(out);
    return DFHack::CR_OK;
}

command_result channel_safely(color_ostream &out, std::vector<std::string> &parameters) {
    if (!parameters.empty()) {
        if (parameters.size() >= 2 && parameters.size() <= 3) {
            if (parameters[0] == "run" && parameters[1] == "once") {
                CSP::UnpauseEvent();
                return DFHack::CR_OK;
            }
            bool state = false;
            bool set = false;
            if (parameters[0] == "enable") {
                state = true;
            } else if (parameters[0] == "disable") {
                state = false;
            } else if (parameters[0] == "set") {
                set = true;
            } else {
                return DFHack::CR_WRONG_USAGE;
            }
            try {
                if (parameters[1] == "debug") {
                    auto level = std::abs(std::stol(parameters[2]));
                    config.debug = true;
                    switch (level) {
                        case 1:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LDEBUG);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LINFO);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LINFO);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LINFO);
                            break;
                        case 2:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LINFO);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LDEBUG);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LDEBUG);
                            break;
                        case 3:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LINFO);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LDEBUG);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LTRACE);
                            break;
                        case 4:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LINFO);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LTRACE);
                            break;
                        case 5:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LDEBUG);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LTRACE);
                            break;
                        case 6:
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LTRACE);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LTRACE);
                            break;
                        case 0:
                        default:
                            DBG_NAME(monitor).allowed(DFHack::DebugCategory::LERROR);
                            DBG_NAME(manager).allowed(DFHack::DebugCategory::LERROR);
                            DBG_NAME(groups).allowed(DFHack::DebugCategory::LERROR);
                            DBG_NAME(jobs).allowed(DFHack::DebugCategory::LERROR);
                    }
                } else if(parameters[1] == "monitor-active"){
                    config.monitor_active = state;
                } else if (parameters[1] == "require-vision") {
                    config.require_vision = state;
                } else if (parameters[1] == "insta-dig") {
                    config.insta_dig = state;
                } else if (parameters[1] == "refresh-freq" && set && parameters.size() == 3) {
                    config.refresh_freq = std::abs(std::stol(parameters[2]));
                } else if (parameters[1] == "monitor-freq" && set && parameters.size() == 3) {
                    config.monitor_freq = std::abs(std::stol(parameters[2]));
                } else if (parameters[1] == "ignore-threshold" && set && parameters.size() == 3) {
                    config.ignore_threshold = std::abs(std::stol(parameters[2]));
                } else if (parameters[1] == "fall-threshold" && set && parameters.size() == 3) {
                    uint8_t t = std::abs(std::stol(parameters[2]));
                    if (t > 0) {
                        config.fall_threshold = t;
                    } else {
                        out.printerr("fall-threshold must have a value greater than 0 or the plugin does a lot of nothing.\n");
                        return DFHack::CR_FAILURE;
                    }
                } else {
                    return DFHack::CR_WRONG_USAGE;
                }
            } catch (const std::exception &e) {
                out.printerr("%s\n", e.what());
                return DFHack::CR_FAILURE;
            }
        }
    } else {
        out.print("Channel-Safely is %s\n", enabled ? "ENABLED." : "DISABLED.");
        out.print("monitor-active: %s\n", config.monitor_active ? "on." : "off.");
        out.print("require-vision: %s\n", config.require_vision ? "on." : "off.");
        out.print("insta-dig: %s\n", config.insta_dig ? "on." : "off.");
        out.print("refresh-freq: %" PRIi32 "\n", config.refresh_freq);
        out.print("monitor-freq: %" PRIi32 "\n", config.monitor_freq);
        out.print("ignore-threshold: %" PRIu8 "\n", config.ignore_threshold);
        out.print("fall-threshold: %" PRIu8 "\n", config.fall_threshold);
    }
    saveConfig();
    return DFHack::CR_OK;
}
