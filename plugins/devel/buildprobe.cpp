//Quick building occupancy flag test.
//Individual bits had no apparent meaning. Assume it's an enum, set by number.

#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Gui.h>
#include <modules/MapCache.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>
using std::vector;
using std::string;
using std::stack;
using namespace DFHack;

command_result readFlag (color_ostream &out, vector <string> & parameters);
command_result writeFlag (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("buildprobe");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("bshow","Output building occupancy value",readFlag));
    commands.push_back(PluginCommand("bset","Set building occupancy value",writeFlag));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result readFlag (color_ostream &out, vector <string> & parameters)
{
    // init the map
    if(!Maps::IsValid())
    {
        out.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    if(!Gui::getCursorCoords(cx,cy,cz))
    {
        out.printerr("Cursor is not active.\n");
        return CR_FAILURE;
    }

    DFCoord cursor = DFCoord(cx,cy,cz);

    MapExtras::MapCache * MCache = new MapExtras::MapCache();
    t_occupancy oc = MCache->occupancyAt(cursor);

    out.print("Current Value: %d\n", oc.bits.building);
    return CR_OK;
}

command_result writeFlag (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() == 0)
    {
        out.print("No value specified\n");
        return CR_FAILURE;
    }

    if (parameters[0] == "help" || parameters[0] == "?")
    {
        out.print("Set the building occupancy flag.\n"
                  "Value must be between 0 and 7, inclusive.\n");
        return CR_OK;
    }

    char value;

    switch (parameters[0][0])
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            value = parameters[0][0] - '0';
            break;

        default:
            out.print("Invalid value specified\n");
            return CR_FAILURE;
            break; //Redundant.
    }

    // init the map
    if(!Maps::IsValid())
    {
        out.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    if(!Gui::getCursorCoords(cx,cy,cz))
    {
        out.printerr("Cursor is not active.\n");
        return CR_FAILURE;
    }

    DFCoord cursor = DFCoord(cx,cy,cz);

    MapExtras::MapCache * MCache = new MapExtras::MapCache();
    t_occupancy oc = MCache->occupancyAt(cursor);

    oc.bits.building = df::tile_building_occ(value);
    MCache->setOccupancyAt(cursor, oc);
    MCache->WriteAll();

    return CR_OK;
}
