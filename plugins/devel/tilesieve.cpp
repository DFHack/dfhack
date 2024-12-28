// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <set>
// DF data structure definition headers
#include "DataDefs.h"
#include "modules/Maps.h"
#include "df/map_block.h"
#include "df/world.h"
#include "TileTypes.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result tilesieve (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name. This must correspond to the filename - skeleton.plug.so or skeleton.plug.dll
DFHACK_PLUGIN("tilesieve");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "tilesieve", "Scan map for unknown tiles.",
        tilesieve, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command scans the whole map for tiles that aren't recognized yet.\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
struct xyz
{
    int x;
    int y;
    int z;
};

command_result tilesieve(color_ostream &out, std::vector<std::string> & params)
{
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    out.print("Scanning.\n");
    std::set <df::tiletype> seen;
    for (auto iter = world->map.map_blocks.begin(); iter != world->map.map_blocks.end(); iter++)
    {
        df::map_block *block = *iter;
        df::tiletype tt;
        // for each tile in block
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
        {
            tt = block->tiletype[x][y];
            const char * name = tileName(tt);
            if(tileShape(tt) != tiletype_shape::NONE )
                continue;
            if(name && strlen(name) != 0)
                continue;
            if(seen.count(tt))
                continue;
            seen.insert(tt);
            out.print("Found tile %x @ %d %d %d\n", tt, block->map_pos.x + x, block->map_pos.y + y, block->map_pos.z);
        }
    }
    return CR_OK;
}
