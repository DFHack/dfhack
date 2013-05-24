
#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Once.h"

#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/syndrome.h"
#include "df/unit.h"
#include "df/unit_syndrome.h"
#include "df/world.h"

#include <cstdlib>

using namespace DFHack;
using namespace std;

static bool enabled = false;

DFHACK_PLUGIN("syndromeTrigger");

void syndromeHandler(color_ostream& out, void* ptr);

command_result syndromeTrigger(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("syndromeTrigger", "Run commands and enable true transformations, configured by the raw files.\n", &syndromeTrigger, false,
        "syndromeTrigger:\n"
        "  syndromeTrigger 0 //disable\n"
        "  syndromeTrigger 1 //enable\n"
        "  syndromeTrigger disable //disable\n"
        "  syndromeTrigger enable //enable\n"
        "\n"
        "See Readme.rst for details.\n"
        ));

    return CR_OK;
}

command_result syndromeTrigger(color_ostream& out, vector<string>& parameters) {
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

    out.print("syndromeTrigger is %s\n", enabled ? "enabled" : "disabled");
    if ( enabled == wasEnabled )
        return CR_OK;

    EventManager::unregisterAll(plugin_self);
    if ( enabled ) {
        EventManager::EventHandler handle(syndromeHandler, 1);
        EventManager::registerListener(EventManager::EventType::SYNDROME, handle, plugin_self);
    }
    return CR_OK;
    
}

void syndromeHandler(color_ostream& out, void* ptr) {
    EventManager::SyndromeData* data = (EventManager::SyndromeData*)ptr;
    
    if ( !ptr ) {
        if ( DFHack::Once::doOnce("syndromeTrigger_null data") ) {
            out.print("%s, %d: null pointer from EventManager.\n", __FILE__, __LINE__);
        }
        return;
    }
    //out.print("Syndrome started: unit %d, syndrome %d.\n", data->unitId, data->syndromeIndex);

    df::unit* unit = df::unit::find(data->unitId);
    if (!unit) {
        if ( DFHack::Once::doOnce("syndromeTrigger_no find unit" ) )
            out.print("%s, line %d: couldn't find unit.\n", __FILE__, __LINE__);
        return;
    }

    df::unit_syndrome* unit_syndrome = unit->syndromes.active[data->syndromeIndex];
    //out.print("  syndrome type %d\n", unit_syndrome->type);
    df::syndrome* syndrome = df::global::world->raws.syndromes.all[unit_syndrome->type];
    
    bool foundPermanent = false;
    bool foundCommand = false;
    bool foundAutoSyndrome = false;
    string commandStr;
    vector<string> args;
    int32_t raceId = -1;
    df::creature_raw* creatureRaw = NULL;
    int32_t casteId = -1;
    for ( size_t a = 0; a < syndrome->syn_class.size(); a++ ) {
        std::string& clazz = *syndrome->syn_class[a];
        //out.print("  clazz %d = %s\n", a, clazz.c_str());
        if ( foundCommand ) {
            if ( commandStr == "" ) {
                commandStr = clazz;
                continue;
            }
            stringstream bob;
            if ( clazz == "\\LOCATION" ) {
                bob << unit->pos.x;
                args.push_back(bob.str());
                bob.str("");
                bob.clear();
                
                bob << unit->pos.y;
                args.push_back(bob.str());
                bob.str("");
                bob.clear();
                
                bob << unit->pos.z;
                args.push_back(bob.str());
                bob.str("");
                bob.clear();
            } else if ( clazz == "\\UNIT_ID" ) {
                bob << unit->id;
                args.push_back(bob.str());
                bob.str("");
                bob.clear();
            } else if ( clazz == "\\SYNDROME_ID" ) {
                bob << unit_syndrome->type;
                args.push_back(bob.str());
                bob.str("");
                bob.clear();
            } else {
                args.push_back(clazz);
            }
            continue;
        }
        if ( clazz == "\\AUTO_SYNDROME" ) {
            foundAutoSyndrome = true;
            continue;
        }
        if ( clazz == "\\COMMAND" ) {
            foundCommand = true;
            continue;
        }
        if ( clazz == "\\PERMANENT" ) {
            foundPermanent = true;
            continue;
        }
        if ( !foundPermanent )
            continue;
        if ( raceId == -1 ) {
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
        if ( raceId != -1 && casteId == -1 ) {
            string& name = *syndrome->syn_class[a];
            for ( size_t b = 0; b < creatureRaw->caste.size(); b++ ) {
                df::caste_raw* caste = creatureRaw->caste[b];
                if ( caste->caste_id != name )
                    continue;
                casteId = b;
                break;
            }
            continue;
        }
    }
    
    if ( !foundAutoSyndrome && commandStr != "" ) {
        Core::getInstance().runCommand(out, commandStr, args);
    }
    
    if ( !foundPermanent || raceId == -1 || casteId == -1 )
        return;

    unit->enemy.normal_race = raceId;
    unit->enemy.normal_caste = casteId;
    //that's it!
}

