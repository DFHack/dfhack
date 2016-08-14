#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Maps.h"

#include "df/block_square_event_frozen_liquidst.h"
#include "df/map_block.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

int changeLiquid (df::tile_liquid type)
{
    int tiles = 0;
    for (int i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        for (size_t j = 0; j < block->block_events.size(); j++)
        {
            df::block_square_event *evt = block->block_events[j];
            if (evt->getType() != block_square_event_type::frozen_liquid)
                continue;
            df::block_square_event_frozen_liquidst *frozen = (df::block_square_event_frozen_liquidst *)evt;
            for (int x = 0; x < 16; x++)
            {
                for (int y = 0; y < 16; y++)
                {
                    if ((frozen->tiles[x][y] != tiletype::Void) && (frozen->liquid_type[x][y] != type))
                    {
                        frozen->liquid_type[x][y] = type;
                        tiles++;
                    }
                }
            }
        }
    }
    return tiles;
}

command_result df_frozenlava (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    int tiles = changeLiquid(tile_liquid::Magma);

    if (tiles)
        out.print("Changed %i tiles of ice into frozen lava.\n", tiles);
    return CR_OK;
}

command_result df_frozenwater (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    int tiles = changeLiquid(tile_liquid::Water);

    if (tiles)
        out.print("Changed %i tiles of ice into frozen water.\n", tiles);
    return CR_OK;
}

DFHACK_PLUGIN("frozen");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("frozenlava", "Changes all ice into frozen magma.", df_frozenlava));
    commands.push_back(PluginCommand("frozenwater", "Changes all ice into frozen water.", df_frozenwater));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
