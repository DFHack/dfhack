#include "PluginManager.h"
#include "Export.h"
#include "DataDefs.h"
#include "Core.h"

#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/Maps.h"

#include "df/building.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/item_boulderst.h"
#include "df/job.h"
#include "df/job_type.h"
#include "df/reaction.h"
#include "df/reaction_product.h"
#include "df/reaction_product_type.h"
#include "df/reaction_product_itemst.h"
#include "df/syndrome.h"
#include "df/unit_syndrome.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_type.h"
#include "df/general_ref_unit_workerst.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace DFHack;

/*
Example usage:

//////////////////////////////////////////////
//In file inorganic_duck.txt
inorganic_stone_duck

[OBJECT:INORGANIC]

[INORGANIC:DUCK_ROCK]
[USE_MATERIAL_TEMPLATE:STONE_TEMPLATE]
[STATE_NAME_ADJ:ALL_SOLID:drakium][DISPLAY_COLOR:0:7:0][TILE:'.']
[IS_STONE]
[SOLID_DENSITY:1][MELTING_POINT:25000]
[BOILING_POINT:9999] //This is the critical line: boiling point must be <= 10000
[SYNDROME]
    [SYN_NAME:Chronic Duck Syndrome]
    [CE_BODY_TRANSFORMATION:PROB:100:START:0]
        [CE:CREATURE:BIRD_DUCK:MALE] //even though we don't have SYN_INHALED, the plugin will add it
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

bool enabled = false;

DFHACK_PLUGIN("autoSyndrome");

command_result autoSyndrome(color_ostream& out, vector<string>& parameters);
void processJob(color_ostream& out, void* jobPtr);
int32_t giveSyndrome(color_ostream& out, int32_t workerId, df::syndrome* syndrome);

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
        "  3) The stone must have a boiling temperature less than or equal to 9000.\n"
        "\n"
        "When these conditions are met, the unit that completed the job will immediately become afflicted with all applicable syndromes associated with the inorganic material of the stone, or stones. It should correctly check for whether the creature or caste is affected or immune, and it should also correctly account for affected and immune creature classes.\n"
        "Multiple syndromes per stone, or multiple boiling rocks produced with the same reaction should work fine.\n"
        ));
    
    
    EventManager::EventHandler handle(processJob, 5);
    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handle, plugin_self);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    return CR_OK;
}

/*DFhackCExport command_result plugin_onstatechange(color_ostream& out, state_change_event e) {
    return CR_OK;
}*/

command_result autoSyndrome(color_ostream& out, vector<string>& parameters) {
    if ( parameters.size() > 1 )
        return CR_WRONG_USAGE;

    bool wasEnabled = enabled;
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
    if ( enabled == wasEnabled )
        return CR_OK;

    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("autoSyndrome");
    if ( enabled ) {
        EventManager::EventHandler handle(processJob, 5);
        EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handle, me);
    } else {
        EventManager::unregisterAll(me);
    }
    return CR_OK;
}

bool maybeApply(color_ostream& out, df::syndrome* syndrome, int32_t workerId, df::unit* unit) {
    df::creature_raw* creature = df::global::world->raws.creatures.all[unit->race];
    df::caste_raw* caste = creature->caste[unit->caste];
    std::string& creature_name = creature->creature_id;
    std::string& creature_caste = caste->caste_id;
    //check that the syndrome applies to that guy
    /*
     * If there is no affected class or affected creature, then anybody who isn't immune is fair game.
     *
     * Otherwise, it works like this:
     *  add all the affected class creatures
     *  add all the affected creatures
     *  remove all the immune class creatures
     *  remove all the immune creatures
     *  you're affected if and only if you're in the remaining list after all of that
     **/
    bool applies = syndrome->syn_affected_class.size() == 0 && syndrome->syn_affected_creature.size() == 0;
    for ( size_t c = 0; c < syndrome->syn_affected_class.size(); c++ ) {
        if ( applies )
            break;
        for ( size_t d = 0; d < caste->creature_class.size(); d++ ) {
            if ( *syndrome->syn_affected_class[c] == *caste->creature_class[d] ) {
                applies = true;
                break;
            }
        }
    }
    for ( size_t c = 0; c < syndrome->syn_immune_class.size(); c++ ) {
        if ( !applies )
            break;
        for ( size_t d = 0; d < caste->creature_class.size(); d++ ) {
            if ( *syndrome->syn_immune_class[c] == *caste->creature_class[d] ) {
                applies = false;
                return false;
            }
        }
    }

    if ( syndrome->syn_affected_creature.size() != syndrome->syn_affected_caste.size() ) {
        out.print("%s, line %d: different affected creature/caste sizes.\n", __FILE__, __LINE__);
        return false;
    }
    for ( size_t c = 0; c < syndrome->syn_affected_creature.size(); c++ ) {
        if ( creature_name != *syndrome->syn_affected_creature[c] )
            continue;
        if ( *syndrome->syn_affected_caste[c] == "ALL" ||
             *syndrome->syn_affected_caste[c] == creature_caste ) {
            applies = true;
            break;
        }
    }
    for ( size_t c = 0; c < syndrome->syn_immune_creature.size(); c++ ) {
        if ( creature_name != *syndrome->syn_immune_creature[c] )
            continue;
        if ( *syndrome->syn_immune_caste[c] == "ALL" ||
             *syndrome->syn_immune_caste[c] == creature_caste ) {
            applies = false;
            return false;
        }
    }
    if ( !applies ) {
        return false;
    }
    if ( giveSyndrome(out, workerId, syndrome) < 0 )
        return false;
    return true;
}

void processJob(color_ostream& out, void* jobPtr) {
    df::job* job = (df::job*)jobPtr;
    if ( job == NULL ) {
        out.print("Error %s line %d: null job.\n", __FILE__, __LINE__);
        return;
    }
    if ( job->completion_timer > 0 )
        return;
    
    if ( job->job_type != df::job_type::CustomReaction )
        return;

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
        return;
    }
    
    int32_t workerId = -1;
    for ( size_t a = 0; a < job->general_refs.size(); a++ ) {
        if ( job->general_refs[a]->getType() != df::enums::general_ref_type::UNIT_WORKER )
            continue;
        if ( workerId != -1 ) {
            out.print("%s, line %d: Found two workers on the same job.\n", __FILE__, __LINE__);
        }
        workerId = ((df::general_ref_unit_workerst*)job->general_refs[a])->unit_id;
        if (workerId == -1) {
            out.print("%s, line %d: invalid worker.\n", __FILE__, __LINE__);
            continue;
        }
    }

    if ( workerId == -1 )
        return;

    int32_t workerIndex = df::unit::binsearch_index(df::global::world->units.all, workerId);
    if ( workerIndex < 0 ) {
        out.print("%s line %d: Couldn't find unit %d.\n", __FILE__, __LINE__, workerId);
        return;
    }
    df::unit* worker = df::global::world->units.all[workerIndex];
    //find the building that made it
    int32_t buildingId = -1;
    for ( size_t a = 0; a < job->general_refs.size(); a++ ) {
        if ( job->general_refs[a]->getType() != df::enums::general_ref_type::BUILDING_HOLDER )
            continue;
        if ( buildingId != -1 ) {
            out.print("%s, line %d: Found two buildings for the same job.\n", __FILE__, __LINE__);
        }
        buildingId = ((df::general_ref_building_holderst*)job->general_refs[a])->building_id;
        if (buildingId == -1) {
            out.print("%s, line %d: invalid building.\n", __FILE__, __LINE__);
            continue;
        }
    }
    df::building* building;
    {
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, buildingId);
        if ( index == -1 ) {
            out.print("%s, line %d: error: couldn't find building %d.\n", __FILE__, __LINE__, buildingId);
            return;
        }
        building = df::global::world->buildings.all[index];
    }

    //find all of the products it makes. Look for a stone with a low boiling point.
    bool appliedSomething = false;
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

        //must be a boiling rock syndrome
        df::inorganic_raw* inorganic = df::global::world->raws.inorganics[bob->mat_index];
        if ( inorganic->material.heat.boiling_point > 9000 ) {
            continue;
        }

        for ( size_t b = 0; b < inorganic->material.syndrome.size(); b++ ) {
            //add each syndrome to the guy who did the job
            df::syndrome* syndrome = inorganic->material.syndrome[b];
            bool workerOnly = false;
            bool allowMultipleTargets = false;
            bool foundCommand = false;
            bool destroyRock = true;
            string commandStr;
            vector<string> args;
            for ( size_t c = 0; c < syndrome->syn_class.size(); c++ ) {
                std::string* clazz = syndrome->syn_class[c];
                if ( foundCommand ) {
                    if ( commandStr == "" ) {
                        if ( *clazz == "\\WORKER_ONLY" ) {
                            workerOnly = true;
                        } else if ( *clazz == "\\ALLOW_MULTIPLE_TARGETS" ) {
                            allowMultipleTargets = true;
                        } else if ( *clazz == "\\PRESERVE_ROCK" ) {
                            destroyRock = false;
                        }
                        else {
                            commandStr = *clazz;
                        }
                    } else {
                        stringstream bob;
                        if ( *clazz == "\\LOCATION" ) {
                            bob << job->pos.x;
                            args.push_back(bob.str());
                            bob.str("");
                            bob.clear();

                            bob << job->pos.y;
                            args.push_back(bob.str());
                            bob.str("");
                            bob.clear();

                            bob << job->pos.z;
                            args.push_back(bob.str());
                            bob.str("");
                            bob.clear();
                        } else if ( *clazz == "\\WORKER_ID" ) {
                            bob << workerId;
                            args.push_back(bob.str());
                        } else if ( *clazz == "\\REACTION_INDEX" ) {
                            bob << reaction->index;
                            args.push_back(bob.str());
                        } else {
                            args.push_back(*clazz);
                        }
                    }
                } else if ( *clazz == "\\COMMAND" ) {
                    foundCommand = true;
                }
            }
            if ( commandStr != "" ) {
                Core::getInstance().runCommand(out, commandStr, args);
            }

            if ( destroyRock ) {
                //find the rock and kill it before it can boil and cause problems and ugliness
                for ( size_t c = 0; c < df::global::world->items.all.size(); c++ ) {
                    df::item* item = df::global::world->items.all[c];
                    if ( item->pos.z != building->z )
                        continue;
                    if ( item->pos.x < building->x1 || item->pos.x > building->x2 )
                        continue;
                    if ( item->pos.y < building->y1 || item->pos.y > building->y2 )
                        continue;
                    if ( item->getType() != df::enums::item_type::BOULDER )
                        continue;
                    //make sure it's the right type of boulder
                    df::item_boulderst* boulder = (df::item_boulderst*)item;
                    if ( boulder->mat_index != bob->mat_index )
                        continue;
                    
                    boulder->flags.bits.garbage_collect = true;
                    boulder->flags.bits.forbid = true;
                    boulder->flags.bits.hidden = true;
                }
            }

            //only one syndrome per reaction will be applied, unless multiples are allowed.
            if ( appliedSomething && !allowMultipleTargets )
                continue;

            if ( maybeApply(out, syndrome, workerId, worker) ) {
                appliedSomething = true;
            }

            if ( workerOnly )
                continue;
            
            //now try applying it to everybody inside the building
            for ( size_t a = 0; a < df::global::world->units.active.size(); a++ ) {
                df::unit* unit = df::global::world->units.active[a];
                if ( unit == worker )
                    continue;
                if ( unit->pos.z != building->z )
                    continue;
                if ( unit->pos.x < building->x1 || unit->pos.x > building->x2 )
                    continue;
                if ( unit->pos.y < building->y1 || unit->pos.y > building->y2 )
                    continue;
                if ( maybeApply(out, syndrome, unit->id, unit) ) {
                    appliedSomething = true;
                    if ( !allowMultipleTargets )
                        break;
                }
            }
        }
    }

    return;
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

