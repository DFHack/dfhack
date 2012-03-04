// De-ramp.  All ramps marked for removal are replaced with given tile (presently, normal floor).

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "modules/Maps.h"
#include "TileTypes.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

DFHACK_PLUGIN("deramp");

command_result df_deramp (Core * c, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int count = 0;
    int countbad = 0;

    int num_blocks = 0, blocks_total = world->map.map_blocks.size();
    for (int i = 0; i < blocks_total; i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        df::map_block *above = Maps::getBlockAbs(block->map_pos.x, block->map_pos.y, block->map_pos.z + 1);

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                df::tiletype oldT = block->tiletype[x][y];
                if ((tileShape(oldT) == tiletype_shape::RAMP) &&
                    (block->designation[x][y].bits.dig == tile_dig_designation::Default))
                {
                    // Current tile is a ramp.
                    // Set current tile, as accurately as can be expected
                    df::tiletype newT = findSimilarTileType(oldT, tiletype_shape::FLOOR);

                    // If no change, skip it (couldn't find a good tile type)
                    if (oldT == newT)
                        continue;
                    // Set new tile type, clear designation
                    block->tiletype[x][y] = newT;
                    block->designation[x][y].bits.dig = tile_dig_designation::No;

                    // Check the tile above this one, in case a downward slope needs to be removed.
                    if ((above) && (tileShape(above->tiletype[x][y]) == tiletype_shape::RAMP_TOP))
                        above->tiletype[x][y] = tiletype::OpenSpace; // open space
                    count++;
                }
                // ramp fixer
                else if ((tileShape(oldT) != tiletype_shape::RAMP)
                    && (above) && (tileShape(above->tiletype[x][y]) == tiletype_shape::RAMP_TOP))
                {
                    above->tiletype[x][y] = tiletype::OpenSpace; // open space
                    countbad++;
                }
            }
        }
    }
    if (count)
        c->con.print("Found and changed %d tiles.\n", count);
    if (countbad)
        c->con.print("Fixed %d bad down ramps.\n", countbad);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "deramp", "De-ramp. All ramps marked for removal are replaced with floors.",
        df_deramp, false,
        "  If there are any ramps designated for removal, they will be instantly\n"
        "  removed. Any ramps that don't have their counterpart will also be removed\n"
        "  (fixes bugs with caveins)\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
