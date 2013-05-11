#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/Once.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_type.h"
#include "df/general_ref_unit_workerst.h"
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

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace DFHack;

bool enabled = true;

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
        "autoSyndrome looks for recently completed jobs matching certain conditions, and if it finds one, then it will give the unit that finished that job the syndrome specified in the raw files. See Readme.rst for full details.\n"
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

    EventManager::unregisterAll(plugin_self);
    if ( enabled ) {
        EventManager::EventHandler handle(processJob, 5);
        EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handle, plugin_self);
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
        if ( DFHack::Once::doOnce("autoSyndrome: different affected creature/caste sizes.") ) {
            out.print("%s, line %d: different affected creature/caste sizes.\n", __FILE__, __LINE__);
        }
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
    CoreSuspender suspender;
    df::job* job = (df::job*)jobPtr;
    if ( job == NULL ) {
        if ( DFHack::Once::doOnce("autoSyndrome_processJob_null job") )
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
        if ( DFHack::Once::doOnce("autoSyndrome processJob couldNotFind") )
            out.print("%s, line %d: could not find reaction \"%s\".\n", __FILE__, __LINE__, job->reaction_name.c_str() );
        return;
    }
    
    int32_t workerId = -1;
    for ( size_t a = 0; a < job->general_refs.size(); a++ ) {
        if ( job->general_refs[a]->getType() != df::enums::general_ref_type::UNIT_WORKER )
            continue;
        if ( workerId != -1 ) {
            if ( DFHack::Once::doOnce("autoSyndrome processJob two workers same job") )
                out.print("%s, line %d: Found two workers on the same job.\n", __FILE__, __LINE__);
        }
        workerId = ((df::general_ref_unit_workerst*)job->general_refs[a])->unit_id;
        if (workerId == -1) {
            if ( DFHack::Once::doOnce("autoSyndrome processJob invalid worker") )
                out.print("%s, line %d: invalid worker.\n", __FILE__, __LINE__);
            continue;
        }
    }
    
    df::unit* worker = df::unit::find(workerId);
    if ( worker == NULL ) {
        //out.print("%s, line %d: invalid worker.\n", __FILE__, __LINE__);
        //this probably means that it finished before EventManager could get a copy of the job while the job was running
        //TODO: consider printing a warning once
        return;
    }
    
    //find the building that made it
    int32_t buildingId = -1;
    for ( size_t a = 0; a < job->general_refs.size(); a++ ) {
        if ( job->general_refs[a]->getType() != df::enums::general_ref_type::BUILDING_HOLDER )
            continue;
        if ( buildingId != -1 ) {
            if ( DFHack::Once::doOnce("autoSyndrome processJob two buildings same job") )
                out.print("%s, line %d: Found two buildings for the same job.\n", __FILE__, __LINE__);
        }
        buildingId = ((df::general_ref_building_holderst*)job->general_refs[a])->building_id;
        if (buildingId == -1) {
            if ( DFHack::Once::doOnce("autoSyndrome processJob invalid building") )
                out.print("%s, line %d: invalid building.\n", __FILE__, __LINE__);
            continue;
        }
    }
    
    df::building* building = df::building::find(buildingId);
    if ( building == NULL ) {
        if ( DFHack::Once::doOnce("autoSyndrome processJob couldn't find building") )
            out.print("%s, line %d: error: couldn't find building %d.\n", __FILE__, __LINE__, buildingId);
        return;
    }
    
    //find all of the products it makes. Look for a stone with a low boiling point.
    for ( size_t a = 0; a < reaction->products.size(); a++ ) {
        bool appliedSomething = false;
        df::reaction_product_type type = reaction->products[a]->getType();
        //out.print("type = %d\n", (int32_t)type);
        if ( type != df::enums::reaction_product_type::item )
            continue;
        df::reaction_product_itemst* bob = (df::reaction_product_itemst*)reaction->products[a];
        //out.print("item_type = %d\n", (int32_t)bob->item_type);
        if ( bob->item_type != df::enums::item_type::BOULDER )
            continue;
        
        if ( bob->mat_index < 0 )
            continue;
        
        //for now don't worry about subtype
        df::inorganic_raw* inorganic = df::global::world->raws.inorganics[bob->mat_index];
        
        for ( size_t b = 0; b < inorganic->material.syndrome.size(); b++ ) {
            //add each syndrome to the guy who did the job
            df::syndrome* syndrome = inorganic->material.syndrome[b];
            bool workerOnly = true;
            bool allowMultipleTargets = false;
            bool foundCommand = false;
            bool destroyRock = true;
            bool foundAutoSyndrome = false;
            string commandStr;
            vector<string> args;
            for ( size_t c = 0; c < syndrome->syn_class.size(); c++ ) {
                std::string& clazz = *syndrome->syn_class[c];
                if ( clazz == "\\AUTO_SYNDROME" ) {
                    foundAutoSyndrome = true;
                    continue;
                } else if ( clazz == "\\ALLOW_NONWORKER_TARGETS" ) {
                    workerOnly = false;
                    continue;
                } else if ( clazz == "\\ALLOW_MULTIPLE_TARGETS" ) {
                    allowMultipleTargets = true;
                    continue;
                } else if ( clazz == "\\PRESERVE_ROCK" ) {
                    destroyRock = false;
                    continue;
                }
                if ( foundCommand ) {
                    if ( commandStr == "" ) {
                        commandStr = clazz;
                    } else {
                        stringstream bob;
                        if ( clazz == "\\LOCATION" ) {
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
                        } else if ( clazz == "\\WORKER_ID" ) {
                            bob << workerId;
                            args.push_back(bob.str());
                        } else if ( clazz == "\\REACTION_INDEX" ) {
                            bob << reaction->index;
                            args.push_back(bob.str());
                        } else {
                            args.push_back(clazz);
                        }
                    }
                } else if ( clazz == "\\COMMAND" ) {
                    foundCommand = true;
                }
            }
            if ( !foundAutoSyndrome ) {
                continue;
            }
            
            if ( commandStr != "" ) {
                Core::getInstance().runCommand(out, commandStr, args);
                continue;
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
                    
                    boulder->flags.bits.hidden = true;
                    boulder->flags.bits.forbid = true;
                    boulder->flags.bits.garbage_collect = true;
                }
            }

            if ( maybeApply(out, syndrome, workerId, worker) ) {
                appliedSomething = true;
            }

            if ( workerOnly )
                continue;

            if ( appliedSomething && !allowMultipleTargets )
                continue;
            
            //now try applying it to everybody inside the building
            for ( size_t a = 0; a < df::global::world->units.active.size(); a++ ) {
                df::unit* unit = df::global::world->units.active[a];
                if ( unit == worker )
                    continue; //we already tried giving it to him, so no doubling up
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
        if ( DFHack::Once::doOnce("autoSyndrome giveSyndrome couldn't find unit") )
            out.print("%s line %d: Couldn't find unit %d.\n", __FILE__, __LINE__, workerId);
        return -1;
    }
    df::unit* unit = df::global::world->units.all[index];

    df::unit_syndrome* unitSyndrome = new df::unit_syndrome();
    unitSyndrome->type = syndrome->id;
    unitSyndrome->year = DFHack::World::ReadCurrentYear();
    unitSyndrome->year_time = DFHack::World::ReadCurrentTick();
//    unitSyndrome->year = 0;
//    unitSyndrome->year_time = 0;
    unitSyndrome->ticks = 0;
    unitSyndrome->unk1 = 0;
    unitSyndrome->flags = 0; //typecast
    
    for ( size_t a = 0; a < syndrome->ce.size(); a++ ) {
        df::unit_syndrome::T_symptoms* symptom = new df::unit_syndrome::T_symptoms();
        symptom->unk1 = 0;
        symptom->unk2 = 0;
        symptom->ticks = 0;
        symptom->flags = 2; //TODO: ???
        unitSyndrome->symptoms.push_back(symptom);
    }
    unit->syndromes.active.push_back(unitSyndrome);
    return 0;
}

