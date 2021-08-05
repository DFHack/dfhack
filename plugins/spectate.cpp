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
bool watching_job = false;
void* job_watched = nullptr;
//REQUIRE_GLOBAL(world);

command_result spectate (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("spectate",
                                     "Automated spectator mode.",
                                     spectate,
                                     false,
                                     ""
                                     " spectate\n"
                                     "    toggles spectator mode\n"
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
}

void onJobStart(color_ostream &out, void* job);
void onJobCompletion(color_ostream &out, void* job);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if (enable && !enabled) {
        using namespace EM::EventType;
        EM::EventHandler start(onJobStart, 0);
        EM::EventHandler complete(onJobCompletion, 0);
        EM::registerListener(EventType::JOB_INITIATED, start, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, complete, plugin_self);
        //out.print("spectator mode enabled!\n");
    } else if (!enable && enabled) {
        EM::unregisterAll(plugin_self);
        job_tracker.clear();
        freq.clear();
        //out.print("spectator mode disabled!\n");
    } else {
        return CR_FAILURE;
    }
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if(enabled && watching_job) {
        int32_t id = Job::getWorker((df::job*) job_watched)->id;
        if (df::global::ui->follow_unit != id) {
            df::global::ui->follow_unit = id;
        }
    }
    return CR_OK;
}


command_result spectate (color_ostream &out, std::vector <std::string> & parameters) {
    return plugin_enable(out, !enabled);
}

void onJobStart(color_ostream& out, void* job_ptr) {
    df::job* job = (df::job*)job_ptr;
    int zcount = freq[job->pos.z]++;
    job_tracker.emplace(job->id);
    if (!watching_job) {
        double p = 0.99 * ((double)zcount / job_tracker.size());
        std::bernoulli_distribution follow_job(p);
        if(follow_job(RNG)){
            df::unit* unit = Job::getWorker(job);
            df::global::ui->follow_unit = unit->id;
            job_watched = job_ptr;
        } else {
            // choose a dwarf that isn't working, and follow them until <condition?>
        }
    }
}

void onJobCompletion(color_ostream &out, void* job_ptr) {
    df::job* job = (df::job*)job_ptr;
    freq[job->pos.z]--;
    job_tracker.erase(job->id);
    if (watching_job && job_ptr == job_watched) {
        watching_job = false;
        job_watched = nullptr;
    }
}