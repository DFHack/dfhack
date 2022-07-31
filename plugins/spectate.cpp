//
// Created by josh on 7/28/21.
//

#include "Core.h"
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
#include <df/ui.h>

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
bool following_dwarf = false;
void* job_watched = nullptr;
int32_t timestamp = -1;
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

command_result spectate (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("spectate",
                                     "Automated spectator mode.",
                                     spectate));
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
        out.print("Spectate mode disabled!\n");
        EM::unregisterAll(plugin_self);
        job_tracker.clear();
        freq.clear();
    }
    enabled = enable;
    return CR_OK;
}

command_result spectate (color_ostream &out, std::vector <std::string> & parameters) {
    return plugin_enable(out, !enabled);
}

void onTick(color_ostream& out, void* ptr) {
    int32_t tick = df::global::world->frame_counter;
    // || seems to be redundant as the first always evaluates true when job_watched is nullptr.. todo: figure what is supposed to happen
    if (!following_dwarf || (job_watched == nullptr && (tick - timestamp) > 50)) {
        std::vector<df::unit*> dwarves;
        for (auto unit: df::global::world->units.active) {
            if (!Units::isCitizen(unit)) {
                continue;
            }
            dwarves.push_back(unit);
        }
        std::uniform_int_distribution<uint64_t> follow_any(0, dwarves.size() - 1);
        if (df::global::ui) {
            df::unit* unit = dwarves[follow_any(RNG)];
            df::global::ui->follow_unit = unit->id;
            job_watched = unit->job.current_job;
            following_dwarf = true;
            if (!job_watched) {
                timestamp = tick;
            }
        }
    }
}

void onJobStart(color_ostream& out, void* job_ptr) {
    int32_t tick = df::global::world->frame_counter;
    auto job = (df::job*) job_ptr;
    int zcount = ++freq[job->pos.z];
    job_tracker.emplace(job->id);
    if (!following_dwarf || (job_watched == nullptr && (tick - timestamp) > 50)) {
        following_dwarf = true;
        double p = 0.99 * ((double) zcount / job_tracker.size());
        std::bernoulli_distribution follow_job(p);
        if (!job->flags.bits.special && follow_job(RNG)) {
            job_watched = job_ptr;
            df::unit* unit = Job::getWorker(job);
            if (df::global::ui && unit) {
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

void onJobCompletion(color_ostream &out, void* job_ptr) {
    auto job = (df::job*)job_ptr;
    freq[job->pos.z]--;
    freq[job->pos.z] = freq[job->pos.z] < 0 ? 0 : freq[job->pos.z];
    job_tracker.erase(job->id);
    if (following_dwarf && job_ptr == job_watched) {
        following_dwarf = false;
        job_watched = nullptr;
    }
}
