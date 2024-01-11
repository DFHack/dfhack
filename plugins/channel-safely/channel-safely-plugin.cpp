/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Dec. 8 2022
*/
/*
This skeletal logic has not been kept up-to-date since ~v0.5

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
#include <PluginManager.h>

#include <modules/EventManager.h>
#include <modules/Units.h>

#include <df/world.h>
#include <df/report.h>
#include <df/tile_traffic.h>
#include <df/block_square_event_designation_priorityst.h>

#include <cinttypes>
#include <unordered_map>
#include <unordered_set>

// Debugging
namespace DFHack {
    DBG_DECLARE(channelsafely, plugin, DebugCategory::LINFO);
    DBG_DECLARE(channelsafely, monitor, DebugCategory::LERROR);
    DBG_DECLARE(channelsafely, manager, DebugCategory::LERROR);
    DBG_DECLARE(channelsafely, groups, DebugCategory::LERROR);
    DBG_DECLARE(channelsafely, jobs, DebugCategory::LERROR);
}

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);

namespace EM = EventManager;
using namespace DFHack;
using namespace EM::EventType;

int32_t mapx, mapy, mapz;
Configuration config;
PersistentDataItem psetting;
PersistentDataItem pfeature;
const std::string FCONFIG_KEY = std::string(plugin_name) + "/feature";
const std::string SCONFIG_KEY = std::string(plugin_name) + "/setting";

enum FeatureConfigData {
    VISION,
    MONITOR,
    RESURRECT,
    INSTADIG,
    RISKAVERSE
};

enum SettingConfigData {
    REFRESH_RATE,
    MONITOR_RATE,
    IGNORE_THRESH,
    FALL_THRESH
};

// dig-now.cpp
df::coord simulate_fall(const df::coord &pos) {
    if unlikely(!Maps::isValidTilePos(pos)) {
        ERR(plugin).print("Error: simulate_fall(" COORD ") - invalid coordinate\n", COORDARGS(pos));
        return {};
    }
    df::coord resting_pos(pos);

    while (Maps::ensureTileBlock(resting_pos)) {
        df::tiletype tt = *Maps::getTileType(resting_pos);
        if (isWalkable(tt))
            break;
        --resting_pos.z;
    }

    return resting_pos;
}

df::coord simulate_area_fall(const df::coord &pos) {
    df::coord neighbours[8]{};
    get_neighbours(pos, neighbours);
    df::coord lowest = simulate_fall(pos);
    for (auto p : neighbours) {
        if unlikely(!Maps::isValidTilePos(p)) continue;
        auto nlow = simulate_fall(p);
        if (nlow.z < lowest.z) {
            lowest = nlow;
        }
    }
    return lowest;
}

namespace CSP {
    std::unordered_map<df::unit*, int32_t> endangered_units;
    std::unordered_map<df::job*, int32_t> job_id_map;
    std::unordered_map<int32_t, df::job*> active_jobs;
    std::unordered_map<int32_t, df::unit*> active_workers;


    std::unordered_map<int32_t, df::coord> last_safe;
    std::unordered_set<df::coord> dignow_queue;

    void ClearData() {
        ChannelManager::Get().destroy_groups();
        dignow_queue.clear();
        last_safe.clear();
        endangered_units.clear();
        active_workers.clear();
        active_jobs.clear();
        job_id_map.clear();
    }

    void SaveSettings() {
        if (pfeature.isValid() && psetting.isValid()) {
            try {
                pfeature.ival(MONITOR) = config.monitoring;
                pfeature.ival(VISION) = config.require_vision;
                pfeature.ival(INSTADIG) = false; //config.insta_dig;
                pfeature.ival(RESURRECT) = config.resurrect;
                pfeature.ival(RISKAVERSE) = config.riskaverse;

                psetting.ival(REFRESH_RATE) = config.refresh_freq;
                psetting.ival(MONITOR_RATE) = config.monitor_freq;
                psetting.ival(IGNORE_THRESH) = config.ignore_threshold;
                psetting.ival(FALL_THRESH) = config.fall_threshold;
            } catch (std::exception &e) {
                ERR(plugin).print("%s\n", e.what());
            }
        }
    }

    void LoadSettings() {
        pfeature = World::GetPersistentSiteData(FCONFIG_KEY);
        psetting = World::GetPersistentSiteData(SCONFIG_KEY);

        if (!pfeature.isValid() || !psetting.isValid()) {
            pfeature = World::AddPersistentSiteData(FCONFIG_KEY);
            psetting = World::AddPersistentSiteData(SCONFIG_KEY);
            SaveSettings();
        } else {
            try {
                config.monitoring = pfeature.ival(MONITOR);
                config.require_vision = pfeature.ival(VISION);
                config.insta_dig = false; //pfeature.ival(INSTADIG);
                config.resurrect = pfeature.ival(RESURRECT);
                config.riskaverse = pfeature.ival(RISKAVERSE);

                config.ignore_threshold = psetting.ival(IGNORE_THRESH);
                config.fall_threshold = psetting.ival(FALL_THRESH);
                config.refresh_freq = psetting.ival(REFRESH_RATE);
                config.monitor_freq = psetting.ival(MONITOR_RATE);
            } catch (std::exception &e) {
                ERR(plugin).print("%s\n", e.what());
            }
        }
        active_workers.clear();
    }

    void UnpauseEvent(bool full_scan = false){
        CoreSuspender suspend; // we need exclusive access to df memory and this call stack doesn't already have a lock
        DEBUG(plugin).print("UnpauseEvent()\n");
        ChannelManager::Get().build_groups(full_scan);
        ChannelManager::Get().manage_groups();
        DEBUG(plugin).print("UnpauseEvent() exits\n");
    }

    void JobStartedEvent(color_ostream &out, void* j) {
        if (enabled && World::isFortressMode() && Maps::IsValid()) {
            TRACE(jobs).print("JobStartedEvent()\n");
            auto job = (df::job*) j;
            // validate job type
            if (ChannelManager::Get().exists(job->pos)) {
                DEBUG(jobs).print(" valid channel job:\n");
                df::unit* worker = Job::getWorker(job);
                // there is a valid worker (living citizen) on the job? right..
                if (worker && Units::isAlive(worker) && Units::isCitizen(worker)) {
                    ChannelManager::Get().jobs.mark_active(job->pos);
                    if (config.riskaverse) {
                        if (ChannelManager::Get().jobs.possible_cavein(job->pos)) {
                            cancel_job(job);
                            ChannelManager::Get().manage_one(job->pos, true, true);
                        } else {
                            ChannelManager::Get().manage_group(job->pos, true, false);
                        }
                    }
                    DEBUG(jobs).print("  valid worker:\n");
                    // track workers on jobs
                    df::coord &pos = job->pos;
                    TRACE(jobs).print(" -> Starting job at (" COORD ")\n", COORDARGS(pos));
                    if (config.monitoring || config.resurrect) {
                        job_id_map.emplace(job, job->id);
                        active_jobs.emplace(job->id, job);
                        active_workers[job->id] = worker;
                        if (config.resurrect) {
                            // this is the only place we can be 100% sure of "safety"
                            // (excluding deadly enemies that will have arrived)
                            last_safe[worker->id] = worker->pos;
                        }
                    }
                    // set tile to restricted
                    TRACE(jobs).print("   setting job tile to restricted\n");
                    Maps::getTileDesignation(job->pos)->bits.traffic = df::tile_traffic::Restricted;
                }
            }
            TRACE(jobs).print(" <- JobStartedEvent() exits normally\n");
        }
    }

    void JobCompletedEvent(color_ostream &out, void* j) {
        if (enabled && World::isFortressMode() && Maps::IsValid()) {
            TRACE(jobs).print("JobCompletedEvent()\n");
            auto job = (df::job*) j;
            // we only care if the job is a channeling one
            if (ChannelManager::Get().exists(job->pos)) {
                ChannelManager::Get().manage_group(job->pos, true, false);
                // check job outcome
                auto block = Maps::getTileBlock(job->pos);
                df::coord local(job->pos);
                local.x = local.x % 16;
                local.y = local.y % 16;
                // verify completion
                if (TileCache::Get().hasChanged(job->pos, block->tiletype[Coord(local)])) {
                    // the job can be considered done
                    df::coord below(job->pos);
                    below.z--;
                    DEBUG(jobs).print(" -> (" COORD ") is marked done, managing group below.\n", COORDARGS(job->pos));
                    // mark done and manage below (and the rest of the group, if there were cavein candidates)
                    block->designation[Coord(local)].bits.traffic = df::tile_traffic::Normal;
                    ChannelManager::Get().mark_done(job->pos);
                    ChannelManager::Get().manage_group(below);
                    if (config.resurrect) {
                        // this is the only place we can be 100% sure of "safety"
                        // (excluding deadly enemies that may have arrived, and future digging)
                        if (active_workers.count(job->id)) {
                            df::unit* worker = active_workers[job->id];
                            last_safe[worker->id] = worker->pos;
                        }
                    }
                }
                // clean up
                auto jp = active_jobs[job->id];
                job_id_map.erase(jp);
                active_workers.erase(job->id);
                active_jobs.erase(job->id);
            }
            TRACE(jobs).print("JobCompletedEvent() exits\n");
        }
    }

    void NewReportEvent(color_ostream &out, void* r) {
        int32_t tick = df::global::world->frame_counter;
        auto report_id = (int32_t)(intptr_t(r));
        if (df::global::world) {
            df::report* report = df::report::find(report_id);
            if (!report) {
                WARN(plugin).print("Error: NewReportEvent() received an invalid report_id - a report* cannot be found\n");
                return;
            }
            switch (report->type) {
                case announcement_type::CANCEL_JOB:
                    if (config.insta_dig) {
                        if (report->text.find("cancels Dig") != std::string::npos ||
                            report->text.find("path") != std::string::npos) {

                            dignow_queue.emplace(report->pos);
                        }
                        DEBUG(plugin).print("%d, pos: " COORD ", pos2: " COORD "\n%s\n", report_id, COORDARGS(report->pos),
                                            COORDARGS(report->pos2), report->text.c_str());
                    }
                    break;
                case announcement_type::CAVE_COLLAPSE:
                    if (config.resurrect) {
                        DEBUG(plugin).print("CAVE IN\n%d, pos: " COORD ", pos2: " COORD "\n%s\n", report_id, COORDARGS(report->pos),
                                            COORDARGS(report->pos2), report->text.c_str());

                        df::coord below = report->pos;
                        below.z -= 1;
                        below = simulate_area_fall(below);
                        df::coord areaMin{report->pos};
                        df::coord areaMax{areaMin};
                        areaMin.x -= 15;
                        areaMin.y -= 15;
                        areaMax.x += 15;
                        areaMax.y += 15;
                        areaMin.z = below.z;
                        areaMax.z += 1;
                        std::vector<df::unit*> units;
                        Units::getUnitsInBox(units, COORDARGS(areaMin), COORDARGS(areaMax));
                        for (auto unit: units) {
                            endangered_units[unit] = tick;
                            DEBUG(plugin).print(" [id %d] was near a cave in.\n", unit->id);
                        }
                        for (auto unit : world->units.all) {
                            if (last_safe.count(unit->id)) {
                                endangered_units[unit] = tick;
                                DEBUG(plugin).print(" [id %d] is/was a worker, we'll track them too.\n", unit->id);
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    void OnUpdate(color_ostream &out) {
        CoreSuspender suspend;
        if (enabled && World::isFortressMode() && Maps::IsValid() && !World::ReadPauseState()) {
            static int32_t last_tick = df::global::world->frame_counter;
            static int32_t last_monitor_tick = df::global::world->frame_counter;
            static int32_t last_refresh_tick = df::global::world->frame_counter;
            static int32_t last_resurrect_tick = df::global::world->frame_counter;
            int32_t tick = df::global::world->frame_counter;

            // Refreshing the group data with full scanning
            if (tick - last_refresh_tick >= config.refresh_freq) {
                last_refresh_tick = tick;
                TRACE(monitor).print("OnUpdate() refreshing now\n");
                if (config.insta_dig) {
                    TRACE(monitor).print(" -> evaluate dignow queue\n");
                    for (auto iter = dignow_queue.begin(); iter != dignow_queue.end();) {
                        auto map_pos = *iter;
                        dig_now(out, map_pos); // teleports units to the bottom of a simulated fall
                        ChannelManager::Get().mark_done(map_pos);
                        iter = dignow_queue.erase(iter);
                    }
                }
                UnpauseEvent(false);
                TRACE(monitor).print("OnUpdate() refresh done\n");
            }

            // Clean up stale df::job*
            if ((config.monitoring || config.resurrect) && tick - last_tick >= 1) {
                last_tick = tick;
                // make note of valid jobs
                std::unordered_map<int32_t, df::job*> valid_jobs;
                for (df::job_list_link* link = &df::global::world->jobs.list; link != nullptr; link = link->next) {
                    df::job* job = link->item;
                    if (job && active_jobs.count(job->id)) {
                        valid_jobs.emplace(job->id, job);
                    }
                }

                // erase the active jobs that aren't valid
                std::unordered_set<df::job*> erase;
                map_value_difference(active_jobs, valid_jobs, erase);
                for (auto j : erase) {
                    auto id = job_id_map[j];
                    job_id_map.erase(j);
                    active_jobs.erase(id);
                    active_workers.erase(id);
                }
            }

            // Monitoring Active and Resurrecting Dead
            if (config.monitoring && tick - last_monitor_tick >= config.monitor_freq) {
                last_monitor_tick = tick;
                TRACE(monitor).print("OnUpdate() monitoring now\n");

                // iterate active jobs
                for (auto pair: active_jobs) {
                    df::job* job = pair.second;
                    df::unit* unit = active_workers[job->id];
                    if (!unit) continue;
                    if (!Maps::isValidTilePos(job->pos)) continue;
                    TRACE(monitor).print(" -> check for job in tracking\n");
                    if (Units::isAlive(unit)) {
                        if (!config.monitoring) continue;
                        TRACE(monitor).print(" -> compare positions of worker and job\n");

                        // check for fall safety
                        if (unit->pos == job->pos && !is_safe_fall(job->pos)) {
                            // unsafe
                            WARN(monitor).print(" -> unsafe job\n");
                            Job::removeWorker(job);

                            // decide to insta-dig or marker mode
                            if (config.insta_dig) {
                                // delete the job
                                Job::removeJob(job);
                                // queue digging the job instantly
                                dignow_queue.emplace(job->pos);
                                DEBUG(monitor).print(" -> insta-dig\n");
                            } else if (config.resurrect) {
                                endangered_units.emplace(unit, tick);
                            } else {
                                // set marker mode
                                Maps::getTileOccupancy(job->pos)->bits.dig_marked = true;

                                // prevent algorithm from re-enabling designation
                                for (auto &be: Maps::getBlock(job->pos)->block_events) {
                                    if (auto bsedp = virtual_cast<df::block_square_event_designation_priorityst>(
                                            be)) {
                                        df::coord local(job->pos);
                                        local.x = local.x % 16;
                                        local.y = local.y % 16;
                                        bsedp->priority[Coord(local)] = config.ignore_threshold * 1000 + 1;
                                        break;
                                    }
                                }
                                DEBUG(monitor).print(" -> set marker mode\n");
                            }
                        }
                    } else if (config.resurrect) {
                        resurrect(out, unit->id);
                        if (last_safe.count(unit->id)) {
                            df::coord lowest = simulate_fall(last_safe[unit->id]);
                            Units::teleport(unit, lowest);
                        }
                    }
                }
                TRACE(monitor).print("OnUpdate() monitoring done\n");
            }

            // Resurrect Dead Workers
            if (config.resurrect && tick - last_resurrect_tick >= 1) {
                last_resurrect_tick = tick;

                // clean up any "endangered" workers that have been tracked 100 ticks or more
                for (auto iter = endangered_units.begin(); iter != endangered_units.end();) {
                    if (tick - iter->second >= 1200) { //keep watch 1 day
                        DEBUG(plugin).print("It has been one day since [id %d]'s last incident.\n", iter->first->id);
                        iter = endangered_units.erase(iter);
                        continue;
                    }
                    ++iter;
                }

                // resurrect any dead units
                for (auto pair : endangered_units) {
                    auto unit = pair.first;
                    if (!Units::isAlive(unit)) {
                        resurrect(out, unit->id);
                        if (last_safe.count(unit->id)) {
                            df::coord lowest = simulate_fall(last_safe[unit->id]);
                            Units::teleport(unit, lowest);
                        }
                    }
                }
            }
        }
    }
}

command_result channel_safely(color_ostream &out, std::vector<std::string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "Automatically manage channel designations.",
                                     channel_safely,
                                     false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    EM::unregisterAll(plugin_self);
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    CSP::LoadSettings();
    if (enabled) {
        std::vector<std::string> params;
        channel_safely(out, params);
    }
    return DFHack::CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot enable %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable && !enabled) {
        // register events to check jobs / update tracking
        EM::EventHandler jobStartHandler(CSP::JobStartedEvent, 0);
        EM::EventHandler jobCompletionHandler(CSP::JobCompletedEvent, 0);
        EM::EventHandler reportHandler(CSP::NewReportEvent, 0);
        EM::registerListener(EventType::REPORT, reportHandler, plugin_self);
        EM::registerListener(EventType::JOB_STARTED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        // manage designations to start off (first time building groups [very important])
        out.print("channel-safely: enabled!\n");
        CSP::UnpauseEvent(true);
    } else if (!enable) {
        // don't need the groups if the plugin isn't going to be enabled
        EM::unregisterAll(plugin_self);
        out.print("channel-safely: disabled!\n");
    }
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    switch (event) {
        case SC_UNPAUSED:
            if (enabled && World::isFortressMode() && Maps::IsValid()) {
                // manage all designations on unpause
                CSP::UnpauseEvent(true);
            }
            break;
        case SC_MAP_LOADED:
            // cache the map size
            Maps::getSize(mapx, mapy, mapz);
            CSP::ClearData();
            ChannelManager::Get().build_groups(true);
            break;
        case SC_WORLD_UNLOADED:
        case SC_MAP_UNLOADED:
            CSP::ClearData();
            break;
        default:
            return DFHack::CR_OK;
    }
    return DFHack::CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out, state_change_event event) {
    CSP::OnUpdate(out);
    return DFHack::CR_OK;
}

command_result channel_safely(color_ostream &out, std::vector<std::string> &parameters) {
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    if (!parameters.empty()) {
        if (parameters[0] == "runonce") {
            CSP::UnpauseEvent(true);
            return DFHack::CR_OK;
        } else if (parameters[0] == "rebuild") {
            ChannelManager::Get().destroy_groups();
            ChannelManager::Get().build_groups(true);
        }
        if (parameters.size() >= 2 && parameters.size() <= 3) {
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
                if(parameters[1] == "monitoring"){
                    if (state != config.monitoring) {
                        config.monitoring = state;
                        // if this is a fresh start
                        if (state && !config.resurrect) {
                            // we need a fresh start
                            CSP::active_workers.clear();
                        }
                    }
                } else if (parameters[1] == "risk-averse") {
                    config.riskaverse = state;
                } else if (parameters[1] == "require-vision") {
                    config.require_vision = state;
                } else if (parameters[1] == "insta-dig") {
                    //config.insta_dig = state;
                    config.insta_dig = false;
                } else if (parameters[1] == "resurrect") {
                    if (state != config.resurrect) {
                        config.resurrect = state;
                        // if this is a fresh start
                        if (state && !config.monitoring) {
                            // we need a fresh start
                            CSP::active_workers.clear();
                        }
                    }
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
        out.print(" FEATURES:\n");
        out.print("  %-20s\t%s\n", "risk-averse: ", config.riskaverse ? "on." : "off.");
        out.print("  %-20s\t%s\n", "monitoring: ", config.monitoring ? "on." : "off.");
        out.print("  %-20s\t%s\n", "require-vision: ", config.require_vision ? "on." : "off.");
        //out.print("  %-20s\t%s\n", "insta-dig: ", config.insta_dig ? "on." : "off.");
        out.print("  %-20s\t%s\n", "resurrect: ", config.resurrect ? "on." : "off.");
        out.print(" SETTINGS:\n");
        out.print("  %-20s\t%" PRIi32 "\n", "refresh-freq: ", config.refresh_freq);
        out.print("  %-20s\t%" PRIi32 "\n", "monitor-freq: ", config.monitor_freq);
        out.print("  %-20s\t%" PRIu8 "\n", "ignore-threshold: ", config.ignore_threshold);
        out.print("  %-20s\t%" PRIu8 "\n", "fall-threshold: ", config.fall_threshold);
    }
    CSP::SaveSettings();
    return DFHack::CR_OK;
}
