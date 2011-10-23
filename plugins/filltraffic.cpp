// Wide-area traffic designation utility.
// Flood-fill from cursor or fill entire map.

#include <ctype.h>		//For toupper().
#include <map>
#include <vector>
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/extra/MapExtras.h>
#include <dfhack/modules/Gui.h>
using std::stack;
using MapExtras::MapCache;
using namespace DFHack;

DFhackCExport command_result filltraffic(DFHack::Core * c, std::vector<std::string> & params);
DFhackCExport command_result tiletraffic(DFHack::Core * c, std::vector<std::string> & params);

DFhackCExport const char * plugin_name ( void )
{
    return "filltraffic";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
	commands.push_back(PluginCommand("filltraffic","Flood-fill with selected traffic designation from cursor",filltraffic));
	commands.push_back(PluginCommand("tiletraffic","Set traffic for all tiles based on given criteria",tiletraffic));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result filltraffic(DFHack::Core * c, std::vector<std::string> & params)
{
	//Maximum map size.
	uint32_t x_max,y_max,z_max;
	//Source and target traffic types.
	e_traffic source = traffic_normal;
	e_traffic target = traffic_normal;
	//Option flags
	bool updown        = false;
	bool checkpit      = true;
	bool checkbuilding = true;

	//Loop through parameters
    for(int i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Flood-fill selected traffic type from the cursor.\n"
                         "Traffic Type Codes:\n"
                         "\tH: High Traffic\n"
                         "\tN: Normal Traffic\n"
                         "\tL: Low Traffic\n"
                         "\tR: Restricted Traffic\n"
                         "Other Options:\n"
                         "\tX: Fill accross z-levels.\n"
                         "\tB: Include buildings and stockpiles.\n"
                         "\tP: Include empty space.\n"
                         "Example:\n"
                         "'filltraffic H' - When used in a room with doors,\n"
                         "                  it will set traffic to HIGH in just that room."
            );
            return CR_OK;
        }

        switch (toupper(params[i][0]))
        {
            case 'H':
                target = traffic_high; break;
            case 'N':
                target = traffic_normal; break;
            case 'L':
                target = traffic_low; break;
            case 'R':
                target = traffic_restricted; break;
            case 'X':
                updown = true; break;
            case 'B':
                checkbuilding = false; break;
            case 'P':
                checkpit = false; break;
        }
    }

    //Initialization.
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
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui->getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        c->con.printerr("Cursor is not active.\n");
        c->Resume();
        return CR_FAILURE;
    }

    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    MapExtras::MapCache * MCache = new MapExtras::MapCache(Maps);

    DFHack::t_designation des = MCache->designationAt(xy);
    int16_t tt = MCache->tiletypeAt(xy);
    DFHack::t_occupancy oc;

    if (checkbuilding)
        oc = MCache->occupancyAt(xy);

    source = des.bits.traffic;
    if(source == target)
    {
        c->con.printerr("This tile is already set to the target traffic type.\n");
        delete MCache;
        c->Resume();
        return CR_FAILURE;
    }

    if(DFHack::isWallTerrain(tt))
    {
        c->con.printerr("This tile is a wall. Please select a passable tile.\n");
        delete MCache;
        c->Resume();
        return CR_FAILURE;
    }

	if(checkpit && DFHack::isOpenTerrain(tt))
	{
		c->con.printerr("This tile is a hole. Please select a passable tile.\n");
        delete MCache;
        c->Resume();
        return CR_FAILURE;
	}

	if(checkbuilding && oc.bits.building)
	{
		c->con.printerr("This tile contains a building. Please select an empty tile.\n");
        delete MCache;
        c->Resume();
        return CR_FAILURE;
	}

	c->con.print("%d/%d/%d  ... FILLING!\n", cx,cy,cz);

	//Naive four-way or six-way flood fill with possible tiles on a stack.
	stack <DFHack::DFCoord> flood;
	flood.push(xy);

	while(!flood.empty())
	{
		xy = flood.top();
		flood.pop();

		des = MCache->designationAt(xy);
		if (des.bits.traffic != source) continue;

		tt = MCache->tiletypeAt(xy);

		if(DFHack::isWallTerrain(tt)) continue;
		if(checkpit && DFHack::isOpenTerrain(tt)) continue;

		if (checkbuilding)
		{
			oc = MCache->occupancyAt(xy);
			if(oc.bits.building) continue;
		}

		//This tile is ready.  Set its traffic level and add surrounding tiles.
		if (MCache->testCoord(xy))
		{
			des.bits.traffic = target;
			MCache->setDesignationAt(xy,des);

			if (xy.x > 0)
			{
				flood.push(DFHack::DFCoord(xy.x - 1, xy.y, xy.z));
			}
			if (xy.x < tx_max - 1)
			{
				flood.push(DFHack::DFCoord(xy.x + 1, xy.y, xy.z));
			}
			if (xy.y > 0)
			{
				flood.push(DFHack::DFCoord(xy.x, xy.y - 1, xy.z));
			}
			if (xy.y < ty_max - 1)
			{
				flood.push(DFHack::DFCoord(xy.x, xy.y + 1, xy.z));
			}

			if (updown)
			{
				if (xy.z > 0 && DFHack::LowPassable(tt))
				{
					flood.push(DFHack::DFCoord(xy.x, xy.y, xy.z - 1));
				}
				if (xy.z < z_max && DFHack::HighPassable(tt))
				{
					flood.push(DFHack::DFCoord(xy.x, xy.y, xy.z + 1));
				}
			}

		}
	}

	MCache->WriteAll();
    c->Resume();
    return CR_OK;
}

enum e_checktype {no_check, check_equal, check_nequal};

DFhackCExport command_result tiletraffic(DFHack::Core * c, std::vector<std::string> & params)
{
	//Target traffic types.
	e_traffic target = traffic_normal;
	//!!! Options Later !!!

	//Loop through parameters
    for(int i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Set traffic types for all tiles on the map.\n"
						 "Traffic Type Codes:\n"
						 "	H: High Traffic\n"
						 "	N: Normal Traffic\n"
						 "	L: Low Traffic\n"
						 "	R: Restricted Traffic\n"
            );
            return CR_OK;
        }

		switch (toupper(params[i][0]))
		{
			case 'H':
				target = traffic_high; break;
			case 'N':
				target = traffic_normal; break;
			case 'L':
				target = traffic_low; break;
			case 'R':
				target = traffic_restricted; break;
		}
    }

	//Initialization.
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

	//Maximum map size.
	uint32_t x_max,y_max,z_max;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

	MapExtras::MapCache * MCache = new MapExtras::MapCache(Maps);

	c->con.print("Entire map ... FILLING!\n");

	//Loop through every single tile
	for(uint32_t x = 0; x <= tx_max; x++)
	{
		for(uint32_t y = 0; y <= ty_max; y++)
		{
			for(uint32_t z = 0; z <= z_max; z++)
			{
				DFHack::DFCoord tile = DFHack::DFCoord(x, y, z);
				DFHack::t_designation des = MCache->designationAt(tile);

				des.bits.traffic = target;
				MCache->setDesignationAt(tile, des);
			}
		}
	}

	MCache->WriteAll();
    c->Resume();
    return CR_OK;
}