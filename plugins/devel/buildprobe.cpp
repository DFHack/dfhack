//Quick building occupancy flag test.
//Individual bits had no apparent meaning. Assume it's an enum, set by number.

#include "Core.h"
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

DFhackCExport DFHack::command_result readFlag (Core * c, vector <string> & parameters);
DFhackCExport DFHack::command_result writeFlag (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "buildprobe";
}

DFhackCExport DFHack::command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("bshow","Output building occupancy value",readFlag));
    commands.push_back(PluginCommand("bset","Set building occupancy value",writeFlag));
    return CR_OK;
}

DFhackCExport DFHack::command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport DFHack::command_result readFlag (Core * c, vector <string> & parameters)
{
	c->Suspend();

	DFHack::Maps * Maps = c->getMaps();
    DFHack::Gui * Gui = c->getGui();
    // init the map
    if(!Maps->Start())
    {
        c->con.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        c->Resume();
        return CR_FAILURE;
    }

	int32_t cx, cy, cz;
    Gui->getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        c->con.printerr("Cursor is not active.\n");
        c->Resume();
        return CR_FAILURE;
    }

	DFHack::DFCoord cursor = DFHack::DFCoord(cx,cy,cz);

	MapExtras::MapCache * MCache = new MapExtras::MapCache(Maps);
	DFHack::t_occupancy oc = MCache->occupancyAt(cursor);

	c->con.print("Current Value: %d\n", oc.bits.building);

	c->Resume();
	return CR_OK;
}

DFhackCExport DFHack::command_result writeFlag (Core * c, vector <string> & parameters)
{
	if (parameters.size() == 0)
	{
		c->con.print("No value specified\n");
		return CR_FAILURE;
	}

	if (parameters[0] == "help" || parameters[0] == "?")
	{
		c->con.print("Set the building occupancy flag.\n"
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
			c->con.print("Invalid value specified\n");
			return CR_FAILURE;
			break; //Redundant.
	}

	c->Suspend();

	DFHack::Maps * Maps = c->getMaps();
    DFHack::Gui * Gui = c->getGui();
    // init the map
    if(!Maps->Start())
    {
        c->con.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        c->Resume();
        return CR_FAILURE;
    }

	int32_t cx, cy, cz;
    Gui->getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        c->con.printerr("Cursor is not active.\n");
        c->Resume();
        return CR_FAILURE;
    }

	DFHack::DFCoord cursor = DFHack::DFCoord(cx,cy,cz);

	MapExtras::MapCache * MCache = new MapExtras::MapCache(Maps);
	DFHack::t_occupancy oc = MCache->occupancyAt(cursor);

	oc.bits.building = value;
	MCache->setOccupancyAt(cursor, oc);
	MCache->WriteAll();

	c->Resume();
	return CR_OK;
}