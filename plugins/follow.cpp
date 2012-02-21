// Make the camera follow the selected unit

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include "DFHack.h"
#include "DataDefs.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "modules/Maps.h"
#include <df/unit.h>
#include <df/creature_raw.h>

using namespace DFHack;
using namespace df::enums;


command_result follow (Core * c, std::vector <std::string> & parameters);

df::unit *followedUnit;
int32_t prevX, prevY, prevZ;

DFHACK_PLUGIN("follow");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "follow", "Follow the selected unit until camera control is released",
        follow, false, 
        "  Select a unit and run this plugin to make the camera follow it. Moving the camera yourself deactivates the plugin.\n"
    ));
    followedUnit = 0;
    prevX=prevY=prevZ = -1;
    return CR_OK;
}


DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.

DFhackCExport command_result plugin_onstatechange(Core* c, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
    case SC_GAME_UNLOADED:
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        break;
    default:
        break;
    }
    return CR_OK;
}


DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if (!followedUnit) return CR_OK;
    DFHack::World *world =c->getWorld();
    if (world->ReadPauseState() && prevX==-1) return CR_OK;
    Gui *gui = c->getGui();
    df::coord &unitPos = followedUnit->pos;
    int32_t x,y,z,w,h;
    gui->getViewCoords(x,y,z);
    gui->getWindowSize(w,h);
    if (prevX==-1)
    {
        prevX = x;
        prevY = y;
        prevZ = z;
    }
    else if(prevX != x || prevY != y || prevZ != z)
    {
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        c->con.print("No longer following anything.\n");
        return CR_OK;
    }
    
    uint32_t x_max, y_max, z_max;
    Simple::Maps::getSize(x_max, y_max, z_max);

    x_max *= 16;
    y_max *= 16;

    prevX = unitPos.x + w/2 >= x_max ? x_max-w+2 : (unitPos.x >= w/2 ? unitPos.x - w/2 : 0);
    prevY = unitPos.y + h/2 >= y_max ? y_max-h+2 : (unitPos.y >= h/2 ? unitPos.y - h/2 : 0);
    prevZ = unitPos.z;

    gui->setViewCoords(prevX, prevY, prevZ);
    
    return CR_OK;
}

command_result follow (Core * c, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    followedUnit = getSelectedUnit(c);
    if (followedUnit)
    {
        c->con.print("Unpause to begin following ");
        c->con.print(df::global::world->raws.creatures.all[followedUnit->race]->name[0].c_str());
        if (followedUnit->name.has_name)
        {
            c->con.print(" %s", followedUnit->name.first_name.c_str());
        }
        c->con.print(".\n");
    }
    else followedUnit = 0;
    return CR_OK;
}
