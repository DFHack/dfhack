// Building and removing construction on a mineral floor will destroy the mineral, changing it to the layer stone
// Farm plots or paved roads do the same thing periodically (once every 500 ticks or so)
// This tool changes said tiles back into the mineral type they originally had

// It also fixes tiles marked as "mineral inclusion" where no inclusion is present,
// which generally happen as a result of improper use of the tiletypes plugin

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
using namespace DFHack::Simple;
using namespace df::enums;

using df::global::world;

DFhackCExport command_result df_fixveins (Core * c, vector <string> & parameters)
{
    if (parameters.size())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int removed = 0;
    int added = 0;

    int num_blocks = 0, blocks_total = world->map.map_blocks.size();
    for (int i = 0; i < blocks_total; i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        uint16_t has_mineral[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        for (int j = 0; j < block->block_events.size(); j++)
        {
            df::block_square_event *evt = block->block_events[j];
            if (evt->getType() != block_square_event_type::mineral)
                continue;
            df::block_square_event_mineralst *mineral = (df::block_square_event_mineralst *)evt;
            for (int k = 0; k < 16; k++)
                has_mineral[k] |= mineral->tile_bitmask[k];
        }
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                int16_t oldT = block->tiletype[x][y];
                int16_t newT = oldT;
                TileMaterial mat = tileMaterial(oldT);
                if ((mat == VEIN) && !(has_mineral[y] & (1 << x)))
                {
                    newT = findTileType(tileShape(oldT), STONE, tileVariant(oldT), tileSpecial(oldT), tileDirection(oldT));
                    if ((newT != -1) && (newT != oldT))
                    {
                        block->tiletype[x][y] = newT;
                        removed++;
                    }
                }
                if ((mat == STONE) && (has_mineral[y] & (1 << x)))
                {
                    newT = findTileType(tileShape(oldT), VEIN, tileVariant(oldT), tileSpecial(oldT), tileDirection(oldT));
                    if ((newT != -1) && (newT != oldT))
                    {
                        block->tiletype[x][y] = newT;
                        added++;
                    }
                }
            }
        }
    }
    if (removed)
        c->con.print("Removed %i invalid references to mineral inclusions.\n", removed);
    if (added)
        c->con.print("Restored %i missing references to mineral inclusions.\n", added);
    return CR_OK;
}

DFhackCExport const char * plugin_name ( void )
{
    return "fixveins";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("fixveins",
        "Remove invalid references to mineral inclusions and restore missing ones.",
        df_fixveins));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
