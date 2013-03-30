
#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"

#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/syndrome.h"
#include "df/unit.h"
#include "df/unit_syndrome.h"
#include "df/world.h"

#include <cstdlib>

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN("trueTransformation");

void syndromeHandler(color_ostream& out, void* ptr);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    EventManager::EventHandler syndrome(syndromeHandler, 1);
    EventManager::registerListener(EventManager::EventType::SYNDROME, syndrome, plugin_self);

    return CR_OK;
}

void syndromeHandler(color_ostream& out, void* ptr) {
    EventManager::SyndromeData* data = (EventManager::SyndromeData*)ptr;
    //out.print("Syndrome started: unit %d, syndrome %d.\n", data->unitId, data->syndromeIndex);

    df::unit* unit = df::unit::find(data->unitId);
    if (!unit) {
        out.print("%s, line %d: couldn't find unit.\n", __FILE__, __LINE__);
        return;
    }

    df::unit_syndrome* unit_syndrome = unit->syndromes.active[data->syndromeIndex];
    df::syndrome* syndrome = df::global::world->raws.syndromes.all[unit_syndrome->type];
    
    bool foundIt = false;
    int32_t raceId = -1;
    df::creature_raw* creatureRaw = NULL;
    int32_t casteId = -1;
    for ( size_t a = 0; a < syndrome->syn_class.size(); a++ ) {
        if ( *syndrome->syn_class[a] == "\\PERMANENT" ) {
            foundIt = true;
        }
        if ( foundIt && raceId == -1 ) {
            //find the race with the name
            string& name = *syndrome->syn_class[a];
            for ( size_t b = 0; b < df::global::world->raws.creatures.all.size(); b++ ) {
                df::creature_raw* creature = df::global::world->raws.creatures.all[b];
                if ( creature->creature_id != name )
                    continue;
                raceId = b;
                creatureRaw = creature;
                break;
            }
            continue;
        }
        if ( foundIt && raceId != -1 ) {
            string& name = *syndrome->syn_class[a];
            for ( size_t b = 0; b < creatureRaw->caste.size(); b++ ) {
                df::caste_raw* caste = creatureRaw->caste[b];
                if ( caste->caste_id != name )
                    continue;
                casteId = b;
                break;
            }
            break;
        }
    }
    if ( !foundIt || raceId == -1 || casteId == -1 )
        return;

    unit->enemy.normal_race = raceId;
    unit->enemy.normal_caste = casteId;
    //that's it!
}

