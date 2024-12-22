// De-ramp.  All ramps marked for removal are replaced with given tile (presently, normal floor).

#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "modules/Maps.h"
#include "modules/Job.h"
#include "TileTypes.h"

#include "df/map_block.h"
#include "df/world.h"
#include "df/job.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("deramp");
REQUIRE_GLOBAL(world);

static void doDeramp(df::map_block* block, df::map_block* above, int x, int y, df::tiletype oldT)
{
    // Current tile is a ramp.
    // Set current tile, as accurately as can be expected
    df::tiletype newT = findSimilarTileType(oldT, tiletype_shape::FLOOR);

    // If no change, skip it (couldn't find a good tile type)
    if (oldT == newT)
        return;
    // Set new tile type, clear designation
    block->tiletype[x][y] = newT;
    block->designation[x][y].bits.dig = tile_dig_designation::No;

    // Check the tile above this one, in case a downward slope needs to be removed.
    if ((above) && (tileShape(above->tiletype[x][y]) == tiletype_shape::RAMP_TOP))
        above->tiletype[x][y] = tiletype::OpenSpace; // open space
}

command_result df_deramp (color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int count = 0;
    int countbad = 0;

    df::job_list_link* next;
    for (df::job_list_link* jl = world->jobs.list.next;jl; jl = next) {
        next = jl->next;
        df::job* job = jl->item;
        if (job->job_type != df::job_type::RemoveStairs)
            continue;
        df::map_block *block = Maps::getTileBlock(job->pos);
        df::coord2d bpos = job->pos - block->map_pos;
        df::tiletype oldT = block->tiletype[bpos.x][bpos.y];
        if (tileShape(oldT) != tiletype_shape::RAMP)
            continue;
        df::map_block *above = Maps::getTileBlock(job->pos + df::coord(0,0,1));
        doDeramp(block, above, bpos.x, bpos.y, oldT);
        count++;
        Job::removeJob(job);
    }

    int blocks_total = world->map.map_blocks.size();
    for (int i = 0; i < blocks_total; i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        df::map_block *above = Maps::getTileBlock(block->map_pos + df::coord(0,0,1));

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                df::tiletype oldT = block->tiletype[x][y];
                if ((tileShape(oldT) == tiletype_shape::RAMP) &&
                    (block->designation[x][y].bits.dig == tile_dig_designation::Default))
                {
                    doDeramp(block, above, x, y, oldT);
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
        out.print("Found and changed %d tiles.\n", count);
    if (countbad)
        out.print("Fixed %d bad down ramps.\n", countbad);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
            "deramp",
            "Removes all ramps designated for removal from the map.",
            df_deramp));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
