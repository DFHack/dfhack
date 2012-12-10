#include "PluginManager.h"
#include "Export.h"
#include "DataDefs.h"
#include "Core.h"
#include "df/job.h"
#include "df/global_objects.h"
#include "df/ui.h"
#include "df/job_type.h"
#include "df/reaction.h"
#include "df/reaction_product.h"
#include "df/reaction_product_type.h"
#include "df/reaction_product_itemst.h"
#include "df/syndrome.h"
#include "df/unit_syndrome.h"
#include "df/unit.h"
#include "df/general_ref.h"
#include "df/general_ref_type.h"
#include "df/general_ref_unit_workerst.h"
#include "modules/Maps.h"
#include "modules/Job.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace DFHack;

/*
Example usage:

//////////////////////////////////////////////
//in file interaction_duck.txt
interaction_duck

[OBJECT:INTERACTION]

[INTERACTION:DUMMY_DUCK_INTERACTION]
	[I_SOURCE:CREATURE_ACTION]
	[I_TARGET:A:CREATURE]
		[IT_LOCATION:CONTEXT_CREATURE]
		[IT_MANUAL_INPUT:target]
		[IT_IMMUNE_CREATURE:BIRD_DUCK:MALE]
	[I_EFFECT:ADD_SYNDROME]
		[IE_TARGET:A]
		[IE_IMMEDIATE]
		[SYNDROME]
            [SYN_NAME:chronicDuckSyndrome]
			[CE_BODY_TRANSFORMATION:PROB:100:START:0]
				[CE:CREATURE:BIRD_DUCK:MALE]
//////////////////////////////////////////////
//In file inorganic_duck.txt
inorganic_stone_duck

[OBJECT:INORGANIC]

[INORGANIC:DUCK_ROCK]
[USE_MATERIAL_TEMPLATE:STONE_TEMPLATE]
[STATE_NAME_ADJ:ALL_SOLID:drakium][DISPLAY_COLOR:0:7:0][TILE:'.']
[IS_STONE]
[SOLID_DENSITY:1][MELTING_POINT:25000]
[CAUSE_SYNDROME:chronicDuckSyndrome]
[BOILING_POINT:50000]
///////////////////////////////////////////////
//In file building_duck.txt
building_duck

[OBJECT:BUILDING]

[BUILDING_WORKSHOP:DUCK_WORKSHOP]
	[NAME:Duck Workshop]
	[NAME_COLOR:7:0:1]
	[DIM:1:1]
	[WORK_LOCATION:1:1]
	[BLOCK:1:0:0:0]
	[TILE:0:1:236]
	[COLOR:0:1:0:0:1]
	[TILE:1:1:' ']
	[COLOR:1:1:0:0:0]
	[TILE:2:1:8]
	[COLOR:2:1:0:0:1]
	[TILE:3:1:8]
	[COLOR:3:2:0:4:1]
	[BUILD_ITEM:1:NONE:NONE:NONE:NONE]
	[BUILDMAT]
	[WORTHLESS_STONE_ONLY]
	[CAN_USE_ARTIFACT]
///////////////////////////////////////////////
//In file reaction_duck.txt
reaction_duck

[OBJECT:REACTION]

[REACTION:DUCKIFICATION]
[NAME:become a duck]
[BUILDING:DUCK_WORKSHOP:NONE]
[PRODUCT:100:100:STONE:NO_SUBTYPE:STONE:DUCK_ROCK]
//////////////////////////////////////////////
//Add the following lines to your entity in entity_default.txt (or wherever it is)
	[PERMITTED_BUILDING:DUCK_WORKSHOP]
	[PERMITTED_REACTION:DUCKIFICATION]
//////////////////////////////////////////////

Next, start a new fort in a new world, build a duck workshop, then have someone become a duck.
*/

const int32_t ticksPerYear = 403200;
int32_t lastRun = 0;
unordered_map<int32_t, df::job*> prevJobs;
unordered_map<int32_t, int32_t> jobWorkers;
bool enabled = true;

DFHACK_PLUGIN("autoSyndrome");

command_result autoSyndrome(color_ostream& out, vector<string>& parameters);
int32_t processJob(color_ostream& out, int32_t id);
int32_t giveSyndrome(color_ostream& out, int32_t workerId, df::syndrome* syndrome);

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream& out) {
    if ( !enabled )
        return CR_OK;
    if(DFHack::Maps::IsValid() == false) {
        return CR_OK;
    }
    
    //don't run more than once per tick
    int32_t time = (*df::global::cur_year)*ticksPerYear + (*df::global::cur_year_tick);
    if ( time <= lastRun )
        return CR_OK;
    lastRun = time;

    //keep track of all queued jobs. When one completes (and is not cancelled), check if it's a boiling rock job, and if so, give the worker the appropriate syndrome
    unordered_map<int32_t, df::job*> jobs;
    df::job_list_link* link = &df::global::world->job_list;
    for( ; link != NULL; link = link->next ) {
        df::job* item = link->item;
        if ( item == NULL )
            continue;
        //-1 usually means it hasn't been assigned yet.
        if ( item->completion_timer < 0 )
            continue;
        //if the completion timer is more than one, then the job will never disappear next tick unless it's cancelled
        if ( item->completion_timer > 1 )
            continue;

        //only consider jobs that have been started
        int32_t workerId = -1;
        for ( size_t a = 0; a < item->references.size(); a++ ) {
            if ( item->references[a]->getType() != df::enums::general_ref_type::UNIT_WORKER )
                continue;
            if ( workerId != -1 ) {
                out.print("%s, line %d: Found two workers on the same job.\n", __FILE__, __LINE__);
            }
            workerId = ((df::general_ref_unit_workerst*)item->references[a])->unit_id;
        }
        if ( workerId == -1 )
            continue;

        jobs[item->id] = item;
        jobWorkers[item->id] = workerId;
    }
    
    //if it's not on the job list anymore, and its completion timer was 0, then it probably finished and was not cancelled, so process the job.
    for ( unordered_map<int32_t, df::job*>::iterator i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        int32_t id = (*i).first;
        df::job* completion = (*i).second;
        if ( jobs.find(id) != jobs.end() )
            continue;
        if ( completion->completion_timer > 0 )
            continue;
        if ( processJob(out, id) < 0 ) {
            //enabled = false;
            return CR_FAILURE;
        }
    }

    //delete obselete job copies
    for ( unordered_map<int32_t, df::job*>::iterator i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        int32_t id = (*i).first;
        df::job* oldJob = (*i).second;
        DFHack::Job::deleteJobStruct(oldJob);
        if ( jobs.find(id) == jobs.end() )
            jobWorkers.erase(id);
    }
    prevJobs.clear();

    //make copies of the jobs we looked at this tick in case they disappear next frame.
    for ( unordered_map<int32_t, df::job*>::iterator i = jobs.begin(); i != jobs.end(); i++ ) {
        int32_t id = (*i).first;
        df::job* oldJob = (*i).second;
        df::job* jobCopy = DFHack::Job::cloneJobStruct(oldJob);
        prevJobs[id] = jobCopy;
    }
    
    return CR_OK;
}

/*DFhackCExport command_result plugin_onstatechange(color_ostream& out, state_change_event e) {
    return CR_OK;
}*/

DFhackCExport command_result plugin_init(color_ostream& out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("autoSyndrome", "Automatically give units syndromes when they complete jobs, as configured in the raw files.\n", &autoSyndrome, false,
        "autoSyndrome:\n"
        "  autoSyndrome 0 //disable\n"
        "  autoSyndrome 1 //enable\n"
        "  autoSyndrome disable //disable\n"
        "  autoSyndrome enable //enable\n"
        "\n"
        "autoSyndrome looks for recently completed jobs matching certain conditions, and if it finds one, then it will give the dwarf that finished that job the syndrome specified in the raw files.\n"
        "\n"
        "Requirements:\n"
        "  1) The job must be a custom reaction.\n"
        "  2) The job must produce a stone of some inorganic material.\n"
        "  3) The material of one of the stones produced must have a token in its raw file of the form [CAUSE_SYNDROME:syndrome_name].\n"
        "\n"
        "If a syndrome with the tag [SYN_NAME:syndrome_name] exists, then the unit that completed the job will become afflicted with that syndrome as soon as the job is completed.\n"));
    
    
    return CR_OK;
}

command_result autoSyndrome(color_ostream& out, vector<string>& parameters) {
    if ( parameters.size() > 1 )
        return CR_WRONG_USAGE;

    if ( parameters.size() == 1 ) {
        if ( parameters[0] == "enable" ) {
            enabled = true;
        } else if ( parameters[0] == "disable" ) {
            enabled = false;
        } else {
            int32_t a = atoi(parameters[0].c_str());
            if ( a < 0 || a > 1 )
                return CR_WRONG_USAGE;

            enabled = (bool)a;
        }
    }

    out.print("autoSyndrome is %s\n", enabled ? "enabled" : "disabled");
    return CR_OK;
}

int32_t processJob(color_ostream& out, int32_t jobId) {
    df::job* job = prevJobs[jobId];
    if ( job == NULL ) {
        out.print("Error %s line %d: couldn't find job %d.\n", __FILE__, __LINE__, jobId);
        return -1;
    }
    
    if ( job->job_type != df::job_type::CustomReaction )
        return 0;
    
    //find the custom reaction raws and see if we have any special tags there
    //out.print("job: \"%s\"\n", job->reaction_name.c_str());

    df::reaction* reaction = NULL;
    for ( size_t a = 0; a < df::global::world->raws.reactions.size(); a++ ) {
        df::reaction* candidate = df::global::world->raws.reactions[a];
        if ( candidate->code != job->reaction_name )
            continue;
        reaction = candidate;
        break;
    }
    if ( reaction == NULL ) {
        out.print("%s, line %d: could not find reaction \"%s\".\n", __FILE__, __LINE__, job->reaction_name.c_str() );
        return -1;
    }

    //find all of the products it makes. Look for a stone with a material with special tags.
    bool foundIt = false;
    for ( size_t a = 0; a < reaction->products.size(); a++ ) {
        df::reaction_product_type type = reaction->products[a]->getType();
        //out.print("type = %d\n", (int32_t)type);
        if ( type != df::enums::reaction_product_type::item )
            continue;
        df::reaction_product_itemst* bob = (df::reaction_product_itemst*)reaction->products[a];
        //out.print("item_type = %d\n", (int32_t)bob->item_type);
        if ( bob->item_type != df::enums::item_type::BOULDER )
            continue;
        //for now don't worry about subtype

        df::inorganic_raw* inorganic = df::global::world->raws.inorganics[bob->mat_index];
        const char* helper = "CAUSE_SYNDROME:";
        for ( size_t b = 0; b < inorganic->str.size(); b++ ) {
            //out.print("inorganic str = \"%s\"\n", inorganic->str[b]->c_str());
            size_t c = inorganic->str[b]->find(helper);
            if ( c == string::npos )
                continue;
            string tail = inorganic->str[b]->substr(c + strlen(helper), inorganic->str[b]->length() - strlen(helper) - 2);
            //out.print("tail = %s\n", tail.c_str());

            //find the syndrome with this name, and give apply it to the dwarf working on the job
            //first find out who completed the job
            if ( jobWorkers.find(jobId) == jobWorkers.end() ) {
                out.print("%s, line %d: could not find job worker for jobs %d.\n", __FILE__, __LINE__, jobId);
                return -1;
            }
            int32_t workerId = jobWorkers[jobId];
            
            //find the syndrome
            df::syndrome* syndrome = NULL;
            for ( size_t d = 0; d < df::global::world->raws.syndromes.all.size(); d++ ) {
                df::syndrome* candidate = df::global::world->raws.syndromes.all[d];
                if ( candidate->syn_name != tail )
                    continue;
                syndrome = candidate;
                break;
            }
            if ( syndrome == NULL )
                return 0;

            if ( giveSyndrome(out, workerId, syndrome) < 0 )
                return -1;
            //out.print("Gave syndrome.\n");
        }
    }
    if ( !foundIt )
        return 0;

    return -2;
}

/*
 * Heavily based on https://gist.github.com/4061959/
 **/
int32_t giveSyndrome(color_ostream& out, int32_t workerId, df::syndrome* syndrome) {
    int32_t index = df::unit::binsearch_index(df::global::world->units.all, workerId);
    if ( index < 0 ) {
        out.print("%s line %d: Couldn't find unit %d.\n", __FILE__, __LINE__, workerId);
        return -1;
    }
    df::unit* unit = df::global::world->units.all[index];

    df::unit_syndrome* unitSyndrome = new df::unit_syndrome();
    unitSyndrome->type = syndrome->id;
    unitSyndrome->year = 0;
    unitSyndrome->year_time = 0;
    unitSyndrome->ticks = 1;
    unitSyndrome->unk1 = 1;
    unitSyndrome->flags = 0; //typecast
    
    for ( size_t a = 0; a < syndrome->ce.size(); a++ ) {
        df::unit_syndrome::T_symptoms* symptom = new df::unit_syndrome::T_symptoms();
        symptom->unk1 = 0;
        symptom->unk2 = 0;
        symptom->ticks = 1;
        symptom->flags = 2; //TODO: ???
        unitSyndrome->symptoms.push_back(symptom);
    }
    unit->syndromes.active.push_back(unitSyndrome);
    return 0;
}

