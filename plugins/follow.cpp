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
uint8_t prevMenuWidth;

DFhackCExport const char * plugin_name ( void )
{
    return "follow";
}


DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand(
        "follow", "Follow the selected unit until camera control is released",
        follow, false, 
        "  Select a unit and run this plugin to make the camera follow it. Moving the camera yourself deactivates the plugin.\n"
    ));
    followedUnit = 0;
    prevX=prevY=prevZ = -1;
    prevMenuWidth = 0;
    return CR_OK;
}


DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(Core* c, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
    case SC_GAME_UNLOADED: //Make sure our plugin's variables are clean
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        prevMenuWidth = 0;
        break;
    default:
        break;
    }
    return CR_OK;
}


DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if (!followedUnit) return CR_OK; //Don't do anything if we're not following a unit

    DFHack::World *world =c->getWorld();
    if (world->ReadPauseState() && prevX==-1) return CR_OK; //Wait until the game is unpaused after first running "follow" to begin following

    df::coord &unitPos = followedUnit->pos;

    Gui *gui = c->getGui();  //Get all of the relevant data for determining the size of the map on the window
    int32_t x,y,z,w,h,c_x,c_y,c_z;
    uint8_t menu_width, area_map_width;
    gui->getViewCoords(x,y,z);
    gui->getWindowSize(w,h);
    gui->getMenuWidth(menu_width, area_map_width);
    gui->getCursorCoords(c_x,c_y,c_z);

    if (c_z == -3000 && menu_width == 3) menu_width = 2; //Presence of the cursor means that there's actually a width-2 menu open

    h -= 2; //account for vertical borders

    if (menu_width == 1) w -= 57; //Menu is open doubly wide
    else if (menu_width == 2 && area_map_width == 3) w -= 33; //Just the menu is open
    else if (menu_width == 2 && area_map_width == 2) w -= 26; //Just the area map is open
    else w -= 2; //No menu or area map, just account for borders
    
    if (prevMenuWidth == 0) prevMenuWidth = menu_width; //have we already had a menu width?

    if (prevX==-1) //have we already had previous values for the window location?
    {
        prevX = x;
        prevY = y;
        prevZ = z;
    }
    else if((prevX != x || prevY != y || prevZ != z) && prevMenuWidth == menu_width) //User has manually moved the window, stop following the unit
    {
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        prevMenuWidth = 0;
        c->con.print("No longer following anything.\n");
        return CR_OK;
    }
    
    uint32_t x_max, y_max, z_max;
    Simple::Maps::getSize(x_max, y_max, z_max); //Get map size in tiles so we can prevent the camera from going off the edge
    x_max *= 16;
    y_max *= 16;

    x = unitPos.x + w/2 >= x_max ? x_max-w : (unitPos.x >= w/2 ? unitPos.x - w/2 : 0); //Calculate a new screen position centered on the selected unit
    y = unitPos.y + h/2 >= y_max ? y_max-h : (unitPos.y >= h/2 ? unitPos.y - h/2 : 0);
    z = unitPos.z;

    gui->setViewCoords(x, y, z); //Set the new screen position!

    if (c_x != 3000 && !world->ReadPauseState()) gui->setCursorCoords(c_x - (prevX-x), c_y - (prevY-y), z); //If, for some reason, the cursor is active and the screen is still moving, move the cursor along with the screen
    
    prevX = x; //Save this round's stuff for next time so we can monitor for changes made by the user
    prevY = y;
    prevZ = z;
    prevMenuWidth = menu_width;

    return CR_OK;
}

command_result follow (Core * c, std::vector <std::string> & parameters)
{ 
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    if (followedUnit)
    {
        c->con.print("No longer following previously selected unit.\n");
        followedUnit = 0;
    }
    followedUnit = getSelectedUnit(c);
    if (followedUnit)
    {
        c->con.print("Unpause to begin following ");
        c->con.print(df::global::world->raws.creatures.all[followedUnit->race]->name[0].c_str());
        if (followedUnit->name.has_name)
        {
            c->con.print(" %s", followedUnit->name.first_name.c_str());
        }
        c->con.print(". Simply manually move the view to break the following.\n");
        prevX=prevY=prevZ = -1;
        prevMenuWidth = 0;
    }
    else followedUnit = 0;
    return CR_OK;
}
