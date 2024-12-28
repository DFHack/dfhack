// Make the camera follow the selected unit

#include "Console.h"
#include "DataDefs.h"
#include "DFHack.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "df/creature_raw.h"
#include "df/unit.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("follow");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(world);

command_result follow (color_ostream &out, std::vector <std::string> & parameters);

df::unit *followedUnit;
int32_t prevX, prevY, prevZ;
uint8_t prevMenuWidth;

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "follow",
        "Make the screen follow the selected unit.",
        follow,
        Gui::view_unit_hotkey));
    followedUnit = 0;
    prevX=prevY=prevZ = -1;
    prevMenuWidth = 0;
    return CR_OK;
}


DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
    case SC_MAP_UNLOADED: //Make sure our plugin's variables are clean
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        prevMenuWidth = 0;
        is_enabled = false;
        break;
    default:
        break;
    }
    return CR_OK;
}


DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if (!followedUnit) return CR_OK; //Don't do anything if we're not following a unit

    if (World::ReadPauseState() && prevX==-1) return CR_OK; //Wait until the game is unpaused after first running "follow" to begin following

    df::coord &unitPos = followedUnit->pos;

    //Get all of the relevant data for determining the size of the map on the window
    int32_t x,y,z,w,h,c_x,c_y,c_z;
    uint8_t menu_width, area_map_width;
    Gui::getViewCoords(x,y,z);
    Gui::getWindowSize(w,h);
    Gui::getMenuWidth(menu_width, area_map_width);
    Gui::getCursorCoords(c_x,c_y,c_z);

    // FIXME: is this really needed? does it ever evaluate to 'true'?
    if (c_x != -30000 && menu_width == 3) menu_width = 2; //Presence of the cursor means that there's actually a width-2 menu open

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
        is_enabled = false;
        followedUnit = 0;
        prevX=prevY=prevZ = -1;
        prevMenuWidth = 0;
        out.print("No longer following anything.\n");
        return CR_OK;
    }

    //Get map size in tiles so we can prevent the camera from going off the edge
    uint32_t x_max, y_max, z_max;
    Maps::getSize(x_max, y_max, z_max);
    x_max *= 16;
    y_max *= 16;

    //Calculate a new screen position centered on the selected unit
    x = unitPos.x + w/2 >= int32_t(x_max) ? x_max-w : (unitPos.x >= w/2 ? unitPos.x - w/2 : 0);
    y = unitPos.y + h/2 >= int32_t(y_max) ? y_max-h : (unitPos.y >= h/2 ? unitPos.y - h/2 : 0);
    z = unitPos.z;

    //Set the new screen position!
    Gui::setViewCoords(x, y, z);

    //If, for some reason, the cursor is active and the screen is still moving, move the cursor along with the screen
    if (c_x != -30000 && !World::ReadPauseState())
        Gui::setCursorCoords(c_x - (prevX-x), c_y - (prevY-y), z);

    //Save this round's stuff for next time so we can monitor for changes made by the user
    prevX = x;
    prevY = y;
    prevZ = z;
    prevMenuWidth = menu_width;

    return CR_OK;
}

command_result follow (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    if (followedUnit)
    {
        out.print("No longer following previously selected unit.\n");
        followedUnit = 0;
    }
    followedUnit = Gui::getSelectedUnit(out);
    if (followedUnit)
    {
        is_enabled = true;
        std::ostringstream ss;
        ss << "Unpause to begin following " << world->raws.creatures.all[followedUnit->race]->name[0];
        if (followedUnit->name.has_name) ss << " " << followedUnit->name.first_name;
        ss << ". Simply manually move the view to break the following.\n";
        out.print("%s", ss.str().c_str());
    }
    else followedUnit = 0;
    is_enabled = (followedUnit != NULL);
    return CR_OK;
}
