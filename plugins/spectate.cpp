//
// Created by josh on 7/28/21.
//

#include "Core.h"
#include <modules/Gui.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/EventManager.h>
#include <modules/Job.h>
#include <modules/Units.h>
#include <df/job.h>
#include <df/unit.h>
#include <df/historical_figure.h>
#include <df/global_objects.h>
#include <df/world.h>
#include <df/viewscreen.h>
#include <df/ui.h>
#include <df/interface_key.h>

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

command_result spectate (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("spectate",
                                     "Automated spectator mode.",
                                     spectate));
    return CR_OK;
}

void refresh_camera(color_ostream &out) {
    if (our_dorf) {
        df::global::ui->follow_unit = our_dorf->id;
        const int16_t &x = our_dorf->pos.x;
        const int16_t &y = our_dorf->pos.y;
        const int16_t &z = our_dorf->pos.z;
        Gui::setViewCoords(x, y, z);
    }
}

void unpause(color_ostream &out) {
    if (!world) return;
    while (!world->status.popups.empty()) {
        // dismiss?
        Gui::getCurViewscreen(true)->feed_key(interface_key::CLOSE_MEGA_ANNOUNCEMENT);
    }
    if (*pause_state) {
        // unpause?
        out.print("unpause? %d", df::global::world->frame_counter);
        Gui::getCurViewscreen(true)->feed_key(interface_key::D_PAUSE);
    }
    refresh_camera(out);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (enabled) {
        switch (event) {
            case SC_PAUSED:
                out.print("  ===  This is a pause event: %d", world->frame_counter);
                if(dismiss_pause_events){
                    unpause(out);
                    out.print("spectate: May the deities bless your dwarves.");
                }
                break;
            case SC_UNPAUSED:
                out.print("  ===  This is an pause event: %d", world->frame_counter);
                break;
            case SC_MAP_UNLOADED:
            case SC_BEGIN_UNLOAD:
            case SC_WORLD_UNLOADED:
                our_dorf = nullptr;
            default:
                break;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
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
    return CR_OK;
}

command_result spectate (color_ostream &out, std::vector <std::string> & parameters) {
    // todo: parse parameters
    if(!parameters.empty()) {
        if (parameters[0] == "spectate") {
            return plugin_enable(out, !enabled);
        } else if (parameters[0] == "disable") {
            return plugin_enable(out, false);
        } else if (parameters[0] == "enable") {
            return plugin_enable(out, true);
        } else if (parameters[0] == "godmode") {
            out.print("todo?"); // todo: adventure as deity?
        } else if (parameters[0] == "auto-unpause") {
            dismiss_pause_events = !dismiss_pause_events;
            out.print(dismiss_pause_events ? "auto-dismiss: on" : "auto-dismiss: off");
            if (parameters.size() == 2) {
                out.print("If you want additional options open an issue on github, or mention it on discord.");
                return DFHack::CR_WRONG_USAGE;
            }
        }
    }
    return DFHack::CR_WRONG_USAGE;
}

// every tick check whether to decide to follow a dwarf
void onTick(color_ostream& out, void* ptr) {
    if (!df::global::ui) return;
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
