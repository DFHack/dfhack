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
#include "df/region_map_entry.h"
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
        "rprobe", "Display region information from embark screen",
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
    int to_set, set_field, set_val;

    // Embark screen active: estimate using world geology data
    VIRTUAL_CAST_VAR(screen, df::viewscreen_choose_start_sitest, Core::getTopViewscreen());

    if (!screen)
        return CR_WRONG_USAGE;

    if (!world || !world->world_data)
    {
        out.printerr("World data is not available.\n");
        return CR_FAILURE;
    }


    if (parameters.size() == 2)
    {
        if (parameters[0] == "rai")
            set_field = 0;
        else if (parameters[0] == "veg")
            set_field = 1;
        else if (parameters[0] == "tem")
            set_field = 2;
        else if (parameters[0] == "evi")
            set_field = 3;
        else if (parameters[0] == "dra")
            set_field = 4;
        else if (parameters[0] == "sav")
            set_field = 5;
        else if (parameters[0] == "sal")
            set_field = 6;
        else
            return CR_WRONG_USAGE;

        if (screen->biome_highlighted)
            to_set = screen->biome_idx;
        else
            to_set = 0;

        set = true;
        set_val = atoi(parameters[1].c_str());
    }

    df::world_data *data = world->world_data;
    coord2d cur_region = screen->location.region_pos;

    // Compute biomes
    for (int i = 0; i < screen->location.biome_rgn.size(); i++)
    {
        coord2d rg = screen->location.biome_rgn[i];

        auto rd = &data->region_map[rg.x][rg.y];

        if (set && i == to_set) {
            if (set_field == 0)
                rd->rainfall = set_val;
            else if (set_field == 1)
                rd->vegetation = set_val;
            else if (set_field == 2)
                rd->temperature = set_val;
            else if (set_field == 3)
                rd->evilness = set_val;
            else if (set_field == 4)
                rd->drainage = set_val;
            else if (set_field == 5)
                rd->savagery = set_val;
            else if (set_field == 6)
                rd->salinity = set_val;
        }

        out << i << ": x = " << rg.x << ", y = " << rg.y;

        out <<
            " region_id: " << rd->region_id <<
            " geo_index: " << rd->geo_index <<
            " landmass_id: " << rd->landmass_id <<
            " flags: " << hex << rd->flags.as_int() << dec << endl;
        out <<
            "rai: " << rd->rainfall << " " <<
            "veg: " << rd->vegetation << " " <<
            "tem: " << rd->temperature << " " <<
            "evi: " << rd->evilness << " " <<
            "dra: " << rd->drainage << " " <<
            "sav: " << rd->savagery << " " <<
            "sal: " << rd->salinity;

        int32_t *p = (int32_t *)rd;
        int c = sizeof(*rd) / sizeof(int32_t);
        for (int j = 0; j < c; j++) {
            if (j % 8 == 0)
            out << endl << setfill('0') << setw(8) << hex << (intptr_t)(rd+j) << ": ";
            out << " " << setfill('0') << setw(8) << hex << p[j];
        }
        out << setfill(' ') << setw(0) << dec << endl;

    }

    return CR_OK;
}
