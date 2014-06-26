
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Maps.h"
#include "modules/Once.h"

#include "df/coord.h"
#include "df/building.h"
#include "df/building_def.h"
#include "df/map_block.h"
#include "df/tile_designation.h"

#include <map>
#include <string>
//TODO: check if building becomes inside/outside later

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN_IS_ENABLED(enabled);
DFHACK_PLUGIN("outsideOnly");

static map<std::string, int32_t> registeredBuildings;
const int32_t OUTSIDE_ONLY = 1;
const int32_t EITHER = 0;
const int32_t INSIDE_ONLY = -1;
int32_t checkEvery = -1;

void buildingCreated(color_ostream& out, void* data);
void checkBuildings(color_ostream& out, void* data);
command_result outsideOnly(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("outsideOnly", "Register buildings as inside/outside only. If the player attempts to construct them in an inapproprate place, the building plan automatically deconstructs.\n", &outsideOnly, false,
        "outsideOnly:\n"
        "  outsideOnly outside [custom building name]\n"
        "    registers [custom building name] as outside-only\n"
        "  outsideOnly inside [custom building name]\n"
        "    registers [custom building name] as inside-only\n"
        "  outsideOnly either [custom building name]\n"
        "    unregisters [custom building name]\n"
        "  outsideOnly checkEvery [n]\n"
        "    checks for buildings that were previously in appropriate inside/outsideness but are not anymore every [n] ticks. If [n] is negative, disables checking.\n"
        "  outsideOnly clear\n"
        "    unregisters all custom buildings\n"
        "  enable outsideOnly\n"
        "    enables the plugin. Plugin must be enabled to function!\n"
        "  disable outsideOnly\n"
        "    disables the plugin\n"
        "  outsideOnly clear outside BUILDING_1 BUILDING_2 inside BUILDING_3\n"
        "    equivalent to:\n"
        "      outsideOnly clear\n"
        "      outsideOnly outside BUILDING_1\n"
        "      outsideOnly outside BUILDING_2\n"
        "      outsideOnly inside BUILDING_3\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_UNLOADED:
        registeredBuildings.clear();
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    if ( enabled == enable )
        return CR_OK;
    enabled = enable;
    EventManager::unregisterAll(plugin_self);
    if ( enabled ) {
        EventManager::EventHandler handler(buildingCreated,1);
        EventManager::registerListener(EventManager::EventType::BUILDING, handler, plugin_self);
        checkBuildings(out, 0);
    }
    return CR_OK;
}

void destroy(df::building* building) {
    if ( Buildings::deconstruct(building) )
        return;
    building->flags.bits.almost_deleted = 1;
}

void checkBuildings(color_ostream& out, void* data) {
    if ( !enabled )
        return;
    
    std::vector<df::building*>& buildings = df::global::world->buildings.all;
    for ( size_t a = 0; a < buildings.size(); a++ ) {
        df::building* building = buildings[a];
        if ( building == NULL )
            continue;
        if ( building->getCustomType() < 0 )
            continue;
        df::coord pos(building->centerx,building->centery,building->z);
        df::tile_designation* des = Maps::getTileDesignation(pos);
        bool outside = des->bits.outside;
        df::building_def* def = df::global::world->raws.buildings.all[building->getCustomType()];
        int32_t type = registeredBuildings[def->code];
        
        if ( type == EITHER ) {
            registeredBuildings.erase(def->code);
        } else if ( type == OUTSIDE_ONLY ) {
            if ( outside )
                continue;
            destroy(building);
        } else if ( type == INSIDE_ONLY ) {
            if ( !outside )
                continue;
            destroy(building);
        } else {
            if ( DFHack::Once::doOnce("outsideOnly invalid setting") ) {
                out.print("Error: outsideOnly: building has invalid setting: %s %d\n", def->code.c_str(), type);
            }
        }
    }
    
    if ( checkEvery < 0 )
        return;
    EventManager::EventHandler timeHandler(checkBuildings,-1);
    EventManager::registerTick(timeHandler, checkEvery, plugin_self);
}

command_result outsideOnly(color_ostream& out, vector<string>& parameters) {
    int32_t status = 2;
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "clear" ) {
            registeredBuildings.clear();
        } else if ( parameters[a] == "outside" ) {
            status = OUTSIDE_ONLY;
        } else if ( parameters[a] == "inside" ) {
            status = INSIDE_ONLY;
        } else if ( parameters[a] == "either" ) {
            status = EITHER;
        } else if ( parameters[a] == "checkEvery" ) {
            if (a+1 >= parameters.size()) {
                out.printerr("You must specify how often to check.\n");
                return CR_WRONG_USAGE;
            }
            checkEvery = atoi(parameters[a].c_str());
        }
        else {
            if ( status == 2 ) {
                out.printerr("Error: you need to tell outsideOnly whether the building is inside only, outside-only or either.\n");
                return CR_WRONG_USAGE;
            }
            registeredBuildings[parameters[a]] = status;
        }
    }
    out.print("outsideOnly is %s\n", enabled ? "enabled" : "disabled");
    if ( enabled ) {
        
    }
    return CR_OK;
}

void buildingCreated(color_ostream& out, void* data) {
    int32_t id = (int32_t)data;
    df::building* building = df::building::find(id);
    if ( building == NULL )
        return;
    
    if ( building->getCustomType() < 0 )
        return;
    //check if it was created inside or outside
    df::coord pos(building->centerx,building->centery,building->z);
    df::tile_designation* des = Maps::getTileDesignation(pos);
    bool outside = des->bits.outside;
    
    df::building_def* def = df::global::world->raws.buildings.all[building->getCustomType()];
    int32_t type = registeredBuildings[def->code];
    if ( type == EITHER ) {
        registeredBuildings.erase(def->code);
    } else if ( type == OUTSIDE_ONLY ) {
        if ( outside )
            return;
        Buildings::deconstruct(building);
    } else if ( type == INSIDE_ONLY ) {
        if ( !outside )
            return;
        Buildings::deconstruct(building);
    } else {
        if ( DFHack::Once::doOnce("outsideOnly invalid setting") ) {
            out.print("Error: outsideOnly: building has invalid setting: %s %d\n", def->code.c_str(), type);
        }
    }
}

