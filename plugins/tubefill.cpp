// Adamantine tube filler. It fills the hollow ones.

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
#include "modules/Gui.h"
#include "TileTypes.h"

using namespace DFHack;
using namespace DFHack::Simple;
using namespace df::enums;
using df::global::world;

command_result tubefill(DFHack::Core * c, std::vector<std::string> & params);

DFhackCExport const char * plugin_name ( void )
{
    return "tubefill";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("tubefill","Fill in all the adamantine tubes again.",tubefill));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result tubefill(DFHack::Core * c, std::vector<std::string> & params)
{
    uint64_t count = 0;

    for(size_t i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
        {
            c->con.print("Replenishes mined out adamantine and hollow adamantine tubes.\n"
                "May cause !!FUN!!\n"
                );
            return CR_OK;
        }
    }
    CoreSuspender suspend(c);
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    // walk the map
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        df::map_block *above = Maps::getBlockAbs(block->map_pos.x, block->map_pos.y, block->map_pos.z + 1);
        if (block->local_feature == -1)
            continue;
        DFHack::t_feature feature;
        DFCoord coord(block->map_pos.x >> 4, block->map_pos.y >> 4, block->map_pos.z);
        if (!Maps::GetLocalFeature(feature, coord, block->local_feature))
            continue;
        if (feature.type != feature_type::deep_special_tube)
            continue;
        for (uint32_t x = 0; x < 16; x++)
        {
            for (uint32_t y = 0; y < 16; y++)
            {
                if (!block->designation[x][y].bits.feature_local)
                    continue;

                // Is the tile already a wall?
                if (tileShape(block->tiletype[x][y]) == tiletype_shape::WALL)
                    continue;

                // Does the tile contain liquid?
                if (block->designation[x][y].bits.flow_size)
                    continue;

                // Set current tile, as accurately as can be expected
                // block->tiletype[x][y] = findSimilarTileType(block->tiletype[x][y], WALL);

                // Check the tile above this one, in case we need to add a floor
                if (above)
                {
                    if ((tileShape(above->tiletype[x][y]) == tiletype_shape::EMPTY) || (tileShape(above->tiletype[x][y]) == tiletype_shape::RAMP_TOP))
                    {
                        // if this tile isn't a local feature, it's likely the tile underneath was originally just a floor
                        // it's also possible there was just solid non-feature stone above, but we don't care enough to check
                        if (!above->designation[x][y].bits.feature_local)
                            continue;
                        above->tiletype[x][y] = findRandomVariant(tiletype::FeatureFloor1);
                    }
                }
                block->tiletype[x][y] = tiletype::FeatureWall;
                ++count;
            }
        }
    }
    c->con.print("Found and changed %d tiles.\n", count);
    return CR_OK;
}
