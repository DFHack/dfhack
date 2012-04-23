// Produces a list of materials available on the map.
// Options:
//  -a : show unrevealed tiles
//  -p : don't show plants
//  -s : don't show slade
//  -t : don't show demon temple

//#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>

using namespace std;
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"

#include "MiscUtils.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_region_details.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/inclusion_type.h"
#include "df/viewscreen_choose_start_sitest.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::coord2d;



command_result rprobe (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("rprobe");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rprobe", "Display assorted region information from embark screen",
        rprobe, false,
        "Display assorted region information from embark screen\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


command_result rprobe (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    bool set = false;
    int to_set, set_val;

    // Embark screen active: estimate using world geology data
    VIRTUAL_CAST_VAR(screen, df::viewscreen_choose_start_sitest, Core::getTopViewscreen());
	
	if (!screen) 
		return CR_WRONG_USAGE;
	
    if (!world || !world->world_data)
    {
        out.printerr("World data is not available.\n");
        return CR_FAILURE;
    }

    if (parameters.size() == 1) 
    {
        if (!screen->biome_highlighted) 
        {
            return CR_WRONG_USAGE;
        }
        set = true;
        to_set = screen->biome_idx;
        set_val = atoi(parameters[0].c_str());
    }

    df::world_data *data = world->world_data;
    coord2d cur_region = screen->region_pos;

    // Compute biomes
    for (int i = 0; i < screen->biome_rgn.size(); i++) 
    {
        coord2d rg = screen->biome_rgn[i];

        df::world_data::T_region_map* rd = &data->region_map[rg.x][rg.y];

        if (set && i == to_set) {
            rd->evilness = set_val;
            out << "* Set evilness to " << set_val << endl;
        }

        out << i << ": x = " << rg.x << ", y = " << rg.y;

        out <<
            " region_id: " << rd->region_id <<
            " geo_index: " << rd->geo_index <<
            " landmass_id: " << rd->landmass_id <<
            " flags: " << hex << rd->flags.as_int() << dec <<
            " sav: " << rd->savagery <<
            " evil: " << rd->evilness;

        int32_t *p = (int32_t *)rd;
        int c = sizeof(*rd) / sizeof(int32_t);
        for (int j = 0; j < c; j++) {
            if (j % 8 == 0) 
            out << endl << setfill('0') << setw(8) << hex << (int)(rd+j) << ": ";
            out << " " << setfill('0') << setw(8) << hex << p[j];
        }
        out << setfill(' ') << setw(0) << dec << endl;

    }
    
    return CR_OK;
}
