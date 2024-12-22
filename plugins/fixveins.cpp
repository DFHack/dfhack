// Building and removing construction on a mineral floor will destroy the mineral, changing it to the layer stone
// Farm plots or paved roads do the same thing periodically (once every 500 ticks or so)
// This tool changes said tiles back into the mineral type they originally had

// It also fixes tiles marked as "mineral inclusion" where no inclusion is present,
// which generally happen as a result of improper use of the tiletypes plugin

#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "modules/Maps.h"
#include "TileTypes.h"

#include "df/block_square_event.h"
#include "df/block_square_event_mineralst.h"
#include "df/map_block.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("fixveins");
REQUIRE_GLOBAL(world);

bool setTileMaterial(df::tiletype &tile, const df::tiletype_material mat)
{
    df::tiletype newTile = findTileType(tileShape(tile), mat, tileVariant(tile), tileSpecial(tile), tileDirection(tile));
    if (newTile == tiletype::Void)
        return false;
    if (newTile != tile)
    {
        tile = newTile;
        return true;
    }
    return false;
}

command_result df_fixveins (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size())
        return CR_WRONG_USAGE;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int mineral_removed = 0, feature_removed = 0;
    int mineral_added = 0, feature_added = 0;

    int blocks_total = world->map.map_blocks.size();
    for (int i = 0; i < blocks_total; i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        uint16_t has_mineral[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        for (size_t j = 0; j < block->block_events.size(); j++)
        {
            df::block_square_event *evt = block->block_events[j];
            if (evt->getType() != block_square_event_type::mineral)
                continue;
            df::block_square_event_mineralst *mineral = (df::block_square_event_mineralst *)evt;
            for (int k = 0; k < 16; k++)
                has_mineral[k] |= mineral->tile_bitmask[k];
        }
        t_feature local, global;
        Maps::ReadFeatures(block, &local, &global);
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                bool has_feature = ((block->designation[x][y].bits.feature_global) && (global.main_material != -1) && (global.sub_material != -1)) ||
                    ((block->designation[x][y].bits.feature_local) && (local.main_material != -1) && (local.sub_material != -1));
                bool has_vein = has_mineral[y] & (1 << x);
                if (has_feature)
                    has_vein = false;
                df::tiletype oldT = block->tiletype[x][y];
                df::tiletype_material mat = tileMaterial(oldT);
                if ((mat == tiletype_material::MINERAL) && !has_vein)
                    mineral_removed += setTileMaterial(block->tiletype[x][y], tiletype_material::STONE);
                if ((mat == tiletype_material::STONE) && has_vein)
                    mineral_added += setTileMaterial(block->tiletype[x][y], tiletype_material::MINERAL);
                if ((mat == tiletype_material::FEATURE) && !has_feature)
                    feature_removed += setTileMaterial(block->tiletype[x][y], tiletype_material::STONE);
                if ((mat == tiletype_material::STONE) && has_feature)
                    feature_added += setTileMaterial(block->tiletype[x][y], tiletype_material::FEATURE);
            }
        }
    }
    if (mineral_removed || feature_removed)
        out.print("Removed invalid references from %i mineral inclusion and %i map feature tiles.\n", mineral_removed, feature_removed);
    if (mineral_added || feature_added)
        out.print("Restored missing references to %i mineral inclusion and %i map feature tiles.\n", mineral_added, feature_added);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "fixveins",
        "Restore missing mineral inclusions.",
        df_fixveins));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
