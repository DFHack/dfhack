// Adamantine tube filler. Replaces mined out tiles but leaves hollow tubes alone (to prevent problems)

#include <stdint.h>
#include <iostream>
#include <map>
#include <cinttypes>
#include <vector>
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/Gui.h"
#include "TileTypes.h"

#include "df/deep_vein_hollow.h"
#include "df/map_block.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("tubefill");
REQUIRE_GLOBAL(world);

bool isDesignatedHollow(df::coord pos)
{
    for (size_t i = 0; i < world->event.deep_vein_hollows.size(); i++)
    {
        auto *vein = world->event.deep_vein_hollows[i];
        for (size_t j = 0; j < vein->tiles.x.size(); j++)
            if (pos == df::coord(vein->tiles.x[j], vein->tiles.y[j], vein->tiles.z[j]))
                return true;
    }
    return false;
}

command_result tubefill(color_ostream &out, std::vector<std::string> & params);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "tubefill",
        "Replentishes mined-out adamantine.",
        tubefill));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result tubefill(color_ostream &out, std::vector<std::string> & params)
{
    uint64_t count = 0;
    bool hollow = false;

    for(size_t i = 0; i < params.size();i++)
    {
        if(params[i] == "help" || params[i] == "?")
            return CR_WRONG_USAGE;
        if (params[i] == "hollow")
            hollow = true;
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    // walk the map
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        df::map_block *above = Maps::getTileBlock(block->map_pos + df::coord(0,0,1));
        DFHack::t_feature feature;
        if (!Maps::ReadFeatures(block, &feature, NULL))
            continue;
        if (feature.type != feature_type::deep_special_tube)
            continue;
        for (uint32_t x = 0; x < 16; x++)
        {
            for (uint32_t y = 0; y < 16; y++)
            {
                if (!block->designation[x][y].bits.feature_local)
                    continue;

                if (!hollow && isDesignatedHollow(block->map_pos + df::coord(x,y,0)))
                    continue;

                // Is the tile already a wall?
                if (tileShape(block->tiletype[x][y]) == tiletype_shape::WALL)
                    continue;

                // Does the tile contain liquid?
                if (block->designation[x][y].bits.flow_size)
                    continue;

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
                world->reindex_pathfinding = true;
                ++count;
            }
        }
    }
    out.print("Found and changed %" PRId64 " tiles.\n", count);
    return CR_OK;
}
