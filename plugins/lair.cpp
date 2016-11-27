#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"

#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("lair");
REQUIRE_GLOBAL(world);

enum state
{
    LAIR_RESET,
    LAIR_SET,
};

command_result lair(color_ostream &out, std::vector<std::string> & params)
{
    state do_what = LAIR_SET;
    for(auto iter = params.begin(); iter != params.end(); iter++)
    {
        if(*iter == "reset")
            do_what = LAIR_RESET;
    }
    CoreSuspender lock;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    uint32_t x_max,y_max,z_max;
    Maps::getSize(x_max,y_max,z_max);
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        DFHack::occupancies40d & occupancies = block->occupancy;
        // for each tile in block
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
        {
            // set to revealed
            occupancies[x][y].bits.monster_lair = (do_what == LAIR_SET);
        }
    }
    if(do_what == LAIR_SET)
        out.print("Map marked as lair.\n");
    else
        out.print("Map no longer marked as lair.\n");
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("lair","Mark the map as a monster lair (avoids item scatter)",lair, false,
        "Usage: 'lair' to mark entire map as monster lair, 'lair reset' to undo the operation.\n"));
    return CR_OK;
}
