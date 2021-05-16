/*
 * Simulates completion of dig designations.
 */

#include "DataFuncs.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Maps.h"

#include <df/tile_designation.h>
#include <df/tile_occupancy.h>
#include <df/world.h>
#include <df/map_block.h>

DFHACK_PLUGIN("dig-dug");
REQUIRE_GLOBAL(world);

using namespace DFHack;

// returns true iff tile is in map bounds and was hidden before this function
// unhid it.
static bool unhide(int32_t x, int32_t y, int32_t z) {
    // ensures coords are in map bounds and ensures that the map block exists
    // so we can unhide the tiles
    if (!Maps::ensureTileBlock(x, y, z))
        return false;

    df::tile_designation &td = *Maps::getTileDesignation(x, y, z);
    if (!td.bits.hidden)
        return false;

    td.bits.hidden = false;
    return true;
}

// unhide self and adjacent tiles if hidden and flood fill unhidden state
static void flood_unhide(int32_t x, int32_t y, int32_t z) {
    df::tiletype tt = *Maps::getTileType(x, y, z);
    if (tileShape(tt) == df::tiletype_shape::WALL
            && tileMaterial(tt) != df::tiletype_material::TREE)
        return;

    for (int32_t xoff = -1; xoff <= 1; ++xoff) {
        for (int32_t yoff = -1; yoff <= 1; ++yoff) {
            if (xoff == 0 && yoff == 0)
                continue;
            if (unhide(x + xoff, y + yoff, z))
                flood_unhide(x + xoff, y + yoff, z);
        }
    }

    if (LowPassable(tt) && unhide(x, y, z-1))
        flood_unhide(x, y, z-1);

    // note that checking HighPassable for the current tile gives false
    // positives. You have to check LowPassable for the tile above.
    if (Maps::ensureTileBlock(x, y, z+1)
            && LowPassable(*Maps::getTileType(x, y, z+1))
            && unhide(x, y, z+1)) {
        flood_unhide(x, y, z+1);
    }
}

// inherit flags from passable tiles above and propagate to passable tiles below
static void propagate_vertical_flags(int32_t x, int32_t y, int32_t z) {
    df::tile_designation &td = *Maps::getTileDesignation(x, y, z);

    if (!Maps::isValidTilePos(x, y, z-1)) {
        // only the sky above
        td.bits.light = true;
        td.bits.outside = true;
        td.bits.subterranean = false;
    }

    int32_t zlevel = z;
    df::tiletype_shape shape = tileShape(*Maps::getTileType(x, y, zlevel));
    while ((shape == df::tiletype_shape::EMPTY
            || shape == df::tiletype_shape::RAMP_TOP)
           && Maps::ensureTileBlock(x, y, --zlevel)) {
        df::tile_designation *td_below = Maps::getTileDesignation(x, y, zlevel);
        if (td_below->bits.light == td.bits.light
                && td_below->bits.outside == td.bits.outside
                && td_below->bits.subterranean == td.bits.subterranean)
            break;
        td_below->bits.light = td.bits.light;
        td_below->bits.outside = td.bits.outside;
        td_below->bits.subterranean = td.bits.subterranean;
        shape = tileShape(*Maps::getTileType(x, y, zlevel));
    }
}

static void set_tile_type(int32_t x, int32_t y, int32_t z,
                          df::tiletype target_type) {
    if (target_type == df::tiletype::Void)
        return;
    df::map_block *block = Maps::getTileBlock(x, y, z);
    block->tiletype[x&15][y&15] = target_type;
}

// TODO: if requested, create boulders
static void dig_tile(color_ostream &out, int32_t x, int32_t y, int32_t z,
                     df::tile_dig_designation designation) {
    df::tiletype tt = *Maps::getTileType(x, y, z);
    df::tiletype target_type = df::tiletype::Void;
    switch(designation) {
        case df::tile_dig_designation::Default:
            target_type = findSimilarTileType(tt, df::tiletype_shape::FLOOR);
            break;
        case df::tile_dig_designation::UpDownStair:
            target_type =
                    findSimilarTileType(tt, df::tiletype_shape::STAIR_UPDOWN);
            break;
        case df::tile_dig_designation::Channel:
        {
            if (Maps::ensureTileBlock(x, y, z+1)) {
                df::tiletype tt_above = *Maps::getTileType(x, y, z+1);
                if (tileShape(tt_above) == df::tiletype_shape::RAMP_TOP)
                    set_tile_type(x, y, z+1, tiletype::OpenSpace);
            }
            target_type = tiletype::OpenSpace;
            if (Maps::ensureTileBlock(x, y, z-1)) {
                df::tiletype tt_below = *Maps::getTileType(x, y, z-1);
                if (tileShape(tt_below) == df::tiletype_shape::WALL
                        && isGroundMaterial(tileMaterial(tt_below))) {
                    dig_tile(out, x, y, z-1, df::tile_dig_designation::Ramp);
                    target_type =
                        findSimilarTileType(tt, df::tiletype_shape::RAMP_TOP);
                }
            }
            break;
        }
        case df::tile_dig_designation::Ramp:
            target_type = findSimilarTileType(tt, df::tiletype_shape::RAMP);
            break;
        case df::tile_dig_designation::DownStair:
            target_type = findSimilarTileType(tt, df::tiletype_shape::STAIR_DOWN);
            break;
        case df::tile_dig_designation::UpStair:
            target_type = findSimilarTileType(tt, df::tiletype_shape::STAIR_UP);
            break;
        case df::tile_dig_designation::No:
        default:
            out.printerr(
                "unhandled dig designation for tile (%d, %d, %d): %d\n",
                x, y, z, designation);
            return;
    }

    // TODO: set tile to use layer material
    set_tile_type(x, y, z, target_type);

    // set flags for current and adjacent tiles
    unhide(x, y, z);
    flood_unhide(x, y, z); // in case we breached a cavern
    propagate_vertical_flags(x, y, z); // for new channels
}

static void smooth_tile(color_ostream &out, int32_t x, int32_t y, int32_t z,
                        bool engrave) {
    // TODO
}

static void carve_tile(color_ostream &out, int32_t x, int32_t y, int32_t z,
                       df::tile_occupancy &to) {
    // TODO
}

command_result dig_dug(color_ostream &out, std::vector<std::string> &) {
    CoreSuspender suspend;

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    // scan the whole map for now. we can add in boundaries later.
    uint32_t endx, endy, endz;
    Maps::getTileSize(endx, endy, endz);

    for (uint32_t z = 0; z <= endz; ++z) {
        for (uint32_t y = 0; y <= endy; ++y) {
            for (uint32_t x = 0; x <= endx; ++x) {
                // this will return NULL if the map block hasn't been allocated
                // yet, but that means there aren't any designations anyway.
                if (!Maps::getTileBlock(x, y, z))
                    continue;

                df::tile_designation &td = *Maps::getTileDesignation(x, y, z);
                df::tile_occupancy &to = *Maps::getTileOccupancy(x, y, z);
                if (td.bits.dig != df::tile_dig_designation::No) {
                    dig_tile(out, x, y, z, td.bits.dig);
                    td.bits.dig = df::tile_dig_designation::No;
                } else if (td.bits.smooth > 0) {
                    bool want_engrave = td.bits.smooth == 2;
                    smooth_tile(out, x, y, z, want_engrave);
                    td.bits.smooth = 0;
                } else if (to.bits.carve_track_north == 1
                    || to.bits.carve_track_east == 1
                    || to.bits.carve_track_south == 1
                    || to.bits.carve_track_west == 1) {
                    carve_tile(out, x, y, z, to);
                }
            }
        }
    }

    // Force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &,
                                         std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "dig-dug", "Simulate completion of dig designations", dig_dug, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    return CR_OK;
}
