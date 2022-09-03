//
// Created by josh on 7/28/21.
//

#include "Core.h"
#include <modules/Gui.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/World.h>
#include <modules/EventManager.h>
#include <modules/Job.h>
#include <modules/Units.h>
#include <df/job.h>
#include <df/unit.h>
#include <df/historical_figure.h>
#include <df/global_objects.h>
#include <df/world.h>
#include <df/viewscreen.h>

#include <map>
#include <set>
#include <random>

std::map<uint16_t,uint16_t> freq;
std::set<int32_t> job_tracker;
std::default_random_engine RNG;

//#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("spectate");
DFHACK_PLUGIN_IS_ENABLED(enabled);
bool dismiss_pause_events = false;
bool following_dwarf = false;
df::unit* our_dorf = nullptr;
void* job_watched = nullptr;
int32_t timestamp = -1;
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(pause_state);
REQUIRE_GLOBAL(d_init);

// todo: implement as user configurable variables
#define tick_span 50
#define base 0.99
Pausing::AnnouncementLock* pause_lock = nullptr;
bool lock_collision = false;

command_result spectate (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("spectate",
                                     "Automated spectator mode.",
                                     spectate,
                                     false,
                                     ""
                                     " spectate\n"
                                     "    displays plugin status\n"
                                     " spectate enable\n"
                                     " spectate disable\n"
                                     " spectate auto-unpause\n"
                                     "    toggle auto-dismissal of game pause events. e.g. a siege event pause\n"
                                     "\n"));
    pause_lock = World::AcquireAnnouncementPauseLock("spectate");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (enabled && world) {
        switch (event) {
            case SC_MAP_UNLOADED:
            case SC_BEGIN_UNLOAD:
            case SC_WORLD_UNLOADED:
                our_dorf = nullptr;
                job_watched = nullptr;
                following_dwarf = false;
            default:
                break;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    World::ReleasePauseLock(pause_lock);
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (lock_collision) {
        if (dismiss_pause_events) {
            // player asked for auto-unpause enabled
            World::SaveAnnouncementSettings();
            if (World::DisableAnnouncementPausing()){
                // now that we've got what we want, we can lock it down
                lock_collision = false;
                pause_lock->lock();
            }
        } else {
            if (World::RestoreAnnouncementSettings()) {
                lock_collision = false;
            }
        }
    }
    while (dismiss_pause_events && !world->status.popups.empty()) {
        // dismiss announcement popup(s)
        Gui::getCurViewscreen(true)->feed_key(interface_key::CLOSE_MEGA_ANNOUNCEMENT);
    }
    return DFHack::CR_OK;
}

void onTick(color_ostream& out, void* tick);
void onJobStart(color_ostream &out, void* job);
void onJobCompletion(color_ostream &out, void* job);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if (enable && !enabled) {
        out.print("Spectate mode enabled!\n");
        using namespace EM::EventType;
        EM::EventHandler ticking(onTick, 15);
        EM::EventHandler start(onJobStart, 0);
        EM::EventHandler complete(onJobCompletion, 0);
        EM::registerListener(EventType::TICK, ticking, plugin_self);
        EM::registerListener(EventType::JOB_STARTED, start, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, complete, plugin_self);
    } else if (!enable && enabled) {
        // warp 8, engage!
        out.print("Spectate mode disabled!\n");
        EM::unregisterAll(plugin_self);
        job_tracker.clear();
        freq.clear();
    }
    enabled = enable;
    return DFHack::CR_OK;
}

command_result spectate (color_ostream &out, std::vector <std::string> & parameters) {
    // todo: parse parameters
    if(!parameters.empty()) {
        if (parameters[0] == "auto-unpause") {
            dismiss_pause_events = !dismiss_pause_events;

            // update the announcement settings if we can
            if (dismiss_pause_events) {
                if (World::SaveAnnouncementSettings()) {
                    World::DisableAnnouncementPausing();
                    pause_lock->lock();
                } else {
                    lock_collision = true;
                }
            } else {
                pause_lock->unlock();
                if (!World::RestoreAnnouncementSettings()) {
                    // this in theory shouldn't happen, if others use the lock like we do in spectate
                    lock_collision = true;
                }
            }

            // report to the user how things went
            if (!lock_collision){
                out.print(dismiss_pause_events ? "auto-unpause: on\n" : "auto-unpause: off\n");
            } else {
                out.print("auto-unpause: must wait for another Pausing::AnnouncementLock to be lifted. This setting will complete when the lock lifts.\n");
            }

            // probably a typo
            if (parameters.size() == 2) {
                out.print("If you want additional options open an issue on github, or mention it on discord.\n\n");
                return DFHack::CR_WRONG_USAGE;
            }
        } else {
            return DFHack::CR_WRONG_USAGE;
        }
    } else {
        out.print(enabled ? "Spectate is enabled.\n" : "Spectate is disabled.\n");
        if(enabled) {
            out.print(dismiss_pause_events ? "auto-unpause: on.\n" : "auto-unpause: off.\n");
        }
    }
    return DFHack::CR_OK;
}

// every tick check whether to decide to follow a dwarf
void onTick(color_ostream& out, void* ptr) {
    int32_t tick = df::global::world->frame_counter;
    if(our_dorf){
        if(!Units::isAlive(our_dorf)){
            following_dwarf = false;
            df::global::ui->follow_unit = -1;
        }
    }
    if (!following_dwarf || (tick - timestamp) > tick_span || job_watched == nullptr) {
        std::vector<df::unit*> dwarves;
        for (auto unit: df::global::world->units.active) {
            if (!Units::isCitizen(unit)) {
                continue;
            }
            dwarves.push_back(unit);
        }
        std::uniform_int_distribution<uint64_t> follow_any(0, dwarves.size() - 1);
        if (df::global::ui) {
            our_dorf = dwarves[follow_any(RNG)];
            df::global::ui->follow_unit = our_dorf->id;
            job_watched = our_dorf->job.current_job;
            following_dwarf = true;
            if (!job_watched) {
                timestamp = tick;
            }
        }
    }
}

// every new worked job needs to be considered
void onJobStart(color_ostream& out, void* job_ptr) {
    // todo: detect mood jobs
    int32_t tick = df::global::world->frame_counter;
    auto job = (df::job*) job_ptr;
    // don't forget about it
    int zcount = ++freq[job->pos.z];
    job_tracker.emplace(job->id);
    // if we're not doing anything~ then let's pick something
    if (job_watched == nullptr || (tick - timestamp) > tick_span) {
        following_dwarf = true;
        // todo: allow the user to configure b, and also revise the math
        const double b = base;
        double p = b * ((double) zcount / job_tracker.size());
        std::bernoulli_distribution follow_job(p);
        if (!job->flags.bits.special && follow_job(RNG)) {
            job_watched = job_ptr;
            df::unit* unit = Job::getWorker(job);
            if (df::global::ui && unit) {
                our_dorf = unit;
                df::global::ui->follow_unit = unit->id;
            }
        } else {
            timestamp = tick;
            std::vector<df::unit*> nonworkers;
            for (auto unit: df::global::world->units.active) {
                if (!Units::isCitizen(unit) || unit->job.current_job) {
                    continue;
                }
                nonworkers.push_back(unit);
            }
            std::uniform_int_distribution<> follow_drunk(0, nonworkers.size() - 1);
            if (df::global::ui) {
                df::global::ui->follow_unit = nonworkers[follow_drunk(RNG)]->id;
            }
        }
    }
}

// every job completed can be forgotten about
void onJobCompletion(color_ostream &out, void* job_ptr) {
    auto job = (df::job*)job_ptr;
    // forget about it
    freq[job->pos.z]--;
    freq[job->pos.z] = freq[job->pos.z] < 0 ? 0 : freq[job->pos.z];
    job_tracker.erase(job->id);
    // the job doesn't exist, so we definitely need to get rid of that
    if (job_watched == job_ptr) {
        job_watched = nullptr;
    }
}
