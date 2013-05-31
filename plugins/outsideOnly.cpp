
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Maps.h"

#include "df/coord.h"
#include "df/building.h"
#include "df/building_def.h"
#include "df/map_block.h"
#include "df/tile_designation.h"

#include <string>

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN("outsideOnly");

void buildingCreated(color_ostream& out, void* data);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    EventManager::EventHandler handler(buildingCreated,1);
    EventManager::registerListener(EventManager::EventType::BUILDING, handler, plugin_self);
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

void buildingCreated(color_ostream& out, void* data) {
    int32_t id = (int32_t)data;
    df::building* building = df::building::find(id);
    if ( building == NULL )
        return;
    
    if ( building->getCustomType() < 0 )
        return;
    string prefix("OUTSIDE_ONLY");
    df::building_def* def = df::global::world->raws.buildings.all[building->getCustomType()];
    if (def->code.compare(0, prefix.size(), prefix)) {
        return;
    }
    
    //now, just check if it was created inside, and if so, scuttle it
    df::coord pos(building->centerx,building->centery,building->z);
    
    df::tile_designation* des = Maps::getTileDesignation(pos);
    if ( des->bits.outside )
        return;
    
    Buildings::deconstruct(building);
}

