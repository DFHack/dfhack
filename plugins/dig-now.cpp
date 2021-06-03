/*
 * Simulates completion of dig designations.
 */

#include "DataFuncs.h"
#include "PluginManager.h"
#include "TileTypes.h"
#include "LuaTools.h"

#include "modules/Maps.h"
#include "modules/MapCache.h"

#include <df/tile_designation.h>
#include <df/tile_occupancy.h>
#include <df/world.h>
#include <df/map_block.h>

DFHACK_PLUGIN("dig-now");
REQUIRE_GLOBAL(world);

using namespace DFHack;

static void flood_unhide(color_ostream &out, const DFCoord &pos) {
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 2)
            || !Lua::PushModulePublic(out, L, "plugins.reveal", "unhideFlood"))
        return;

    Lua::Push(L, pos);
    Lua::SafeCall(out, L, 1, 0);
}

// inherit flags from passable tiles above and propagate to passable tiles below
static void propagate_vertical_flags(MapExtras::MapCache &map,
                                     const DFCoord &pos) {
    df::tile_designation td = map.designationAt(pos);

    if (!map.ensureBlockAt(DFCoord(pos.x, pos.y, pos.z+1))) {
        // only the sky above
        td.bits.light = true;
        td.bits.outside = true;
        td.bits.subterranean = false;
    }

    int32_t zlevel = pos.z;
    df::tiletype_shape shape =
            tileShape(map.tiletypeAt(DFCoord(pos.x, pos.y, zlevel)));
    while ((shape == df::tiletype_shape::EMPTY
            || shape == df::tiletype_shape::RAMP_TOP)
           && map.ensureBlockAt(DFCoord(pos.x, pos.y, --zlevel))) {
        DFCoord pos_below(pos.x, pos.y, zlevel);
        df::tile_designation td_below = map.designationAt(pos_below);
        if (td_below.bits.light == td.bits.light
                && td_below.bits.outside == td.bits.outside
                && td_below.bits.subterranean == td.bits.subterranean)
            break;
        td_below.bits.light = td.bits.light;
        td_below.bits.outside = td.bits.outside;
        td_below.bits.subterranean = td.bits.subterranean;
        map.setDesignationAt(pos_below, td_below);
        shape = tileShape(map.tiletypeAt(pos_below));
    }
}

static bool can_dig_default(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape == df::tiletype_shape::WALL ||
        shape == df::tiletype_shape::FORTIFICATION ||
        shape == df::tiletype_shape::RAMP ||
        shape == df::tiletype_shape::STAIR_UP ||
        shape == df::tiletype_shape::STAIR_UPDOWN;
}

static bool can_dig_channel(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape != df::tiletype_shape::EMPTY &&
        shape != df::tiletype_shape::ENDLESS_PIT &&
        shape != df::tiletype_shape::NONE &&
        shape != df::tiletype_shape::RAMP_TOP &&
        shape != df::tiletype_shape::TRUNK_BRANCH;
}

static bool can_dig_up_stair(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape == df::tiletype_shape::WALL ||
        shape == df::tiletype_shape::FORTIFICATION;
}

static bool can_dig_down_stair(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape == df::tiletype_shape::BOULDER ||
        shape == df::tiletype_shape::BROOK_BED ||
        shape == df::tiletype_shape::BROOK_TOP ||
        shape == df::tiletype_shape::FLOOR ||
        shape == df::tiletype_shape::FORTIFICATION ||
        shape == df::tiletype_shape::PEBBLES ||
        shape == df::tiletype_shape::RAMP ||
        shape == df::tiletype_shape::SAPLING ||
        shape == df::tiletype_shape::SHRUB ||
        shape == df::tiletype_shape::TWIG ||
        shape == df::tiletype_shape::WALL;
}

static bool can_dig_up_down_stair(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape == df::tiletype_shape::WALL ||
        shape == df::tiletype_shape::FORTIFICATION ||
        shape == df::tiletype_shape::STAIR_UP;
}

static bool can_dig_ramp(df::tiletype tt) {
    df::tiletype_shape shape = tileShape(tt);
    return shape == df::tiletype_shape::WALL ||
        shape == df::tiletype_shape::FORTIFICATION;
}

static void dig_type(MapExtras::MapCache &map, const DFCoord &pos,
                     df::tiletype tt) {
    auto blk = map.BlockAtTile(pos);
    if (!blk)
        return;

    // ensure we run this even if one of the later steps fails (e.g. OpenSpace)
    map.setTiletypeAt(pos, tt);

    // digging a tile reverts it to the layer soil/stone material
    if (!blk->setStoneAt(pos, tt, map.layerMaterialAt(pos)) &&
            !blk->setSoilAt(pos, tt, map.layerMaterialAt(pos)))
        return;

    // un-smooth dug tiles
    tt = map.tiletypeAt(pos);
    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      df::tiletype_special::NORMAL, tileDirection(tt));
    map.setTiletypeAt(pos, tt);
}

static void dig_shape(MapExtras::MapCache &map, const DFCoord &pos,
                      df::tiletype tt, df::tiletype_shape shape) {
    dig_type(map, pos, findSimilarTileType(tt, shape));
}

static void remove_ramp_top(MapExtras::MapCache &map, const DFCoord &pos) {
    if (!map.ensureBlockAt(pos))
        return;

    if (tileShape(map.tiletypeAt(pos)) == df::tiletype_shape::RAMP_TOP)
        dig_type(map, pos, df::tiletype::OpenSpace);
}

static bool is_wall(MapExtras::MapCache &map, const DFCoord &pos) {
    if (!map.ensureBlockAt(pos))
        return false;
    return tileShape(map.tiletypeAt(pos)) == df::tiletype_shape::WALL;
}

static void clean_ramp(MapExtras::MapCache &map, const DFCoord &pos) {
    if (!map.ensureBlockAt(pos))
        return;

    df::tiletype tt = map.tiletypeAt(pos);
    if (tileShape(tt) != df::tiletype_shape::RAMP)
        return;

    if (is_wall(map, DFCoord(pos.x-1, pos.y, pos.z)) ||
            is_wall(map, DFCoord(pos.x+1, pos.y, pos.z)) ||
            is_wall(map, DFCoord(pos.x, pos.y-1, pos.z)) ||
            is_wall(map, DFCoord(pos.x, pos.y+1, pos.z)))
        return;

    remove_ramp_top(map, DFCoord(pos.x, pos.y, pos.z+1));
    dig_shape(map,pos, tt, df::tiletype_shape::FLOOR);
}

// removes self and/or orthogonally adjacent ramps that are no longer adjacent
// to a wall
static void clean_ramps(MapExtras::MapCache &map, const DFCoord &pos) {
    clean_ramp(map, pos);
    clean_ramp(map, DFCoord(pos.x-1, pos.y, pos.z));
    clean_ramp(map, DFCoord(pos.x+1, pos.y, pos.z));
    clean_ramp(map, DFCoord(pos.x, pos.y-1, pos.z));
    clean_ramp(map, DFCoord(pos.x, pos.y+1, pos.z));
}

// TODO: if requested, create boulders
static bool dig_tile(color_ostream &out, MapExtras::MapCache &map,
                     const DFCoord &pos, df::tile_dig_designation designation) {
    df::tiletype tt = map.tiletypeAt(pos);

    // TODO: handle tree trunks, roots, and surface tiles
    if (!isGroundMaterial(tileMaterial(tt)))
        return false;

    df::tiletype target_type = df::tiletype::Void;
    switch(designation) {
        case df::tile_dig_designation::Default:
            // TODO: should not leave a smooth floor when removing stairs/ramps
            if (can_dig_default(tt)) {
                df::tiletype_shape shape = tileShape(tt);
                df::tiletype_shape target_shape = df::tiletype_shape::FLOOR;
                if (shape == df::tiletype_shape::STAIR_UPDOWN)
                    target_shape = df::tiletype_shape::STAIR_DOWN;
                else if (shape == df::tiletype_shape::RAMP)
                    remove_ramp_top(map, DFCoord(pos.x, pos.y, pos.z+1));
                target_type = findSimilarTileType(tt, target_shape);
            }
            break;
        case df::tile_dig_designation::Channel:
            if (can_dig_channel(tt)) {
                remove_ramp_top(map, DFCoord(pos.x, pos.y, pos.z+1));
                target_type = df::tiletype::OpenSpace;
                DFCoord pos_below(pos.x, pos.y, pos.z-1);
                if (map.ensureBlockAt(pos_below) &&
                        dig_tile(out, map, pos_below,
                                 df::tile_dig_designation::Ramp)) {
                    clean_ramps(map, pos_below);
                    // if we successfully dug out the ramp below, that took care
                    // of the ramp top here
                    return true;
                }
                break;
            }
        case df::tile_dig_designation::UpStair:
            if (can_dig_up_stair(tt))
                target_type =
                        findSimilarTileType(tt, df::tiletype_shape::STAIR_UP);
            break;
        case df::tile_dig_designation::DownStair:
            if (can_dig_down_stair(tt)) {
                target_type =
                        findSimilarTileType(tt, df::tiletype_shape::STAIR_DOWN);

            }
            break;
        case df::tile_dig_designation::UpDownStair:
            if (can_dig_up_down_stair(tt)) {
                target_type =
                        findSimilarTileType(tt,
                                            df::tiletype_shape::STAIR_UPDOWN);
            }
            break;
        case df::tile_dig_designation::Ramp:
        {
            if (can_dig_ramp(tt)) {
                target_type = findSimilarTileType(tt, df::tiletype_shape::RAMP);
                DFCoord pos_above(pos.x, pos.y, pos.z+1);
                if (target_type != tt && map.ensureBlockAt(pos_above)) {
                    // set tile type directly instead of calling dig_shape
                    // because we need to use *this* tile's material, not the
                    // material of the tile above
                    map.setTiletypeAt(pos_above,
                        findSimilarTileType(tt, df::tiletype_shape::RAMP_TOP));
                }
            }
            break;
        }
        case df::tile_dig_designation::No:
        default:
            out.printerr(
                "unhandled dig designation for tile (%d, %d, %d): %d\n",
                pos.x, pos.y, pos.z, designation);
    }

    // fail if unhandled or no change to tile
    if (target_type == df::tiletype::Void || target_type == tt)
        return false;

    dig_type(map, pos, target_type);

    // let light filter down to newly exposed tiles
    propagate_vertical_flags(map, pos);

    return true;
}

static bool smooth_tile(color_ostream &out, MapExtras::MapCache &map,
                        const DFCoord &pos, bool engrave) {
    // TODO
    return false;
}

static bool carve_tile(color_ostream &out, MapExtras::MapCache &map,
                       const DFCoord &pos, df::tile_occupancy &to) {
    // TODO
    return false;
}

static void do_dig(color_ostream &out, std::vector<DFCoord> &dug_coords,
                   const DFCoord &start, const DFCoord &end) {
    // use the proxy layer for the layer material-setting ease-of-use functions.
    MapExtras::MapCache map;

    for (uint32_t z = start.z; z <= end.z; ++z) {
        for (uint32_t y = start.y; y <= end.y; ++y) {
            for (uint32_t x = start.x; x <= end.x; ++x) {
                // this will return NULL if the map block hasn't been allocated
                // yet, but that means there aren't any designations anyway.
                if (!Maps::getTileBlock(x, y, z))
                    continue;

                DFCoord pos(x, y, z);
                df::tile_designation td = map.designationAt(pos);
                df::tile_occupancy to = map.occupancyAt(pos);
                if (td.bits.dig != df::tile_dig_designation::No) {
                    if (dig_tile(out, map, pos, td.bits.dig)) {
                        td = map.designationAt(pos);
                        td.bits.dig = df::tile_dig_designation::No;
                        map.setDesignationAt(pos, td);
                        dug_coords.push_back(pos);
                    }
                } else if (td.bits.smooth > 0) {
                    bool want_engrave = td.bits.smooth == 2;
                    if (smooth_tile(out, map, pos, want_engrave)) {
                        to = map.occupancyAt(pos);
                        td.bits.smooth = 0;
                        map.setDesignationAt(pos, td);
                    }
                } else if (to.bits.carve_track_north == 1
                                || to.bits.carve_track_east == 1
                                || to.bits.carve_track_south == 1
                                || to.bits.carve_track_west == 1) {
                    if (carve_tile(out, map, pos, to)) {
                        to = map.occupancyAt(pos);
                        to.bits.carve_track_north = 0;
                        to.bits.carve_track_east = 0;
                        to.bits.carve_track_south = 0;
                        to.bits.carve_track_west = 0;
                        map.setOccupancyAt(pos, to);
                    }
                }
            }
        }
    }

    map.WriteAll();
}

command_result dig_now(color_ostream &out, std::vector<std::string> &) {
    CoreSuspender suspend;

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    // tracks which positions to unhide
    std::vector<DFCoord> dug_coords;

    // scan the whole map for now. we can add in configurable boundaries later
    DFCoord start(0, 0, 0);
    uint32_t endx, endy, endz;
    Maps::getTileSize(endx, endy, endz);
    DFCoord end(endx, endy, endz);

    do_dig(out, dug_coords, start, end);

    // unhide newly dug tiles. we can't do this in do_dig() since our MapCache
    // wouldn't detect the changes made by reveal.unhideFlood() without
    // invalidating and reinitializing on every call
    for (DFCoord pos : dug_coords) {
        if (Maps::getTileDesignation(pos)->bits.hidden)
            flood_unhide(out, pos);
    }

    // force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &,
                                         std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "dig-now", "Instantly complete dig designations", dig_now, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    return CR_OK;
}
