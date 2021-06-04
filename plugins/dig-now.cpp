/*
 * Simulates completion of dig designations.
 */

#include "DataFuncs.h"
#include "PluginManager.h"
#include "TileTypes.h"
#include "LuaTools.h"

#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Random.h"
#include "modules/World.h"

#include <df/historical_entity.h>
#include <df/map_block.h>
#include <df/reaction_product_itemst.h>
#include <df/tile_designation.h>
#include <df/tile_occupancy.h>
#include <df/ui.h>
#include <df/unit.h>
#include <df/world.h>
#include <df/world_site.h>

DFHACK_PLUGIN("dig-now");
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

using namespace DFHack;

struct dig_now_options {
    DFCoord start; // upper-left coordinate, min z-level
    DFCoord end;   // lower-right coordinate, max z-level

    // whether to unhide dug regions that are unreachable from the main fortress
    bool unhide_unreachable;

    // percent chance ([0..100]) for creating a boulder for the given rock type
    uint32_t boulder_percent_layer;
    uint32_t boulder_percent_vein;
    uint32_t boulder_percent_small_cluster;
    uint32_t boulder_percent_deep;

    // whether to generate boulders at the cursor position instead of at their
    // dig locations
    bool dump_boulders;

    // if set to the pos of a walkable tile, will dump at this position instead
    // of the in-game cursor
    DFCoord cursor;

    static DFCoord getMapSize() {
        uint32_t endx, endy, endz;
        Maps::getTileSize(endx, endy, endz);
        return DFCoord(endx, endy, endz);
    }

    // boulder percentage defaults from
    // https://dwarffortresswiki.org/index.php/DF2014:Mining
    dig_now_options() :
        start(0, 0, 0),
        end(getMapSize()),
        unhide_unreachable(false),
        boulder_percent_layer(25),
        boulder_percent_vein(33),
        boulder_percent_small_cluster(100),
        boulder_percent_deep(100),
        dump_boulders(false) { }
};

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

// returns material type of walls at the given position, or -1 if not a wall
static int16_t get_wall_mat(MapExtras::MapCache &map, const DFCoord &pos) {
    df::tiletype tt = map.tiletypeAt(pos);
    df::tiletype_shape shape = tileShape(tt);

    if (shape != df::tiletype_shape::WALL)
        return -1;

    t_matpair matpair;
    if (map.isVeinAt(pos))
        matpair = map.veinMaterialAt(pos);
    else if (map.isLayerAt(pos))
        matpair = map.layerMaterialAt(pos);
    return matpair.mat_type;
}

static bool dig_tile(color_ostream &out, MapExtras::MapCache &map,
                     const DFCoord &pos, df::tile_dig_designation designation,
                     int16_t *wall_mat) {
    df::tiletype tt = map.tiletypeAt(pos);

    // TODO: handle tree trunks, roots, and surface tiles
    df::tiletype_material tile_mat = tileMaterial(tt);
    if (!isGroundMaterial(tile_mat))
        return false;

    *wall_mat = get_wall_mat(map, pos);

    df::tiletype target_type = df::tiletype::Void;
    switch(designation) {
        case df::tile_dig_designation::Default:
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
                                 df::tile_dig_designation::Ramp, wall_mat)) {
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
                wall_mat[1] = get_wall_mat(map, pos_above);
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

static bool produces_boulder(const dig_now_options &options,
                             Random::MersenneRNG &rng,
                             df::inclusion_type vein_type) {
    uint32_t probability;
    switch (vein_type) {
        case df::inclusion_type::CLUSTER:
        case df::inclusion_type::VEIN:
            probability = options.boulder_percent_vein;
            break;
        case df::inclusion_type::CLUSTER_ONE:
        case df::inclusion_type::CLUSTER_SMALL:
            probability = options.boulder_percent_small_cluster;
            break;
        default:
            probability = options.boulder_percent_layer;
            break;
    }

    return rng.random(100) < probability;
}

static void do_dig(color_ostream &out, std::vector<DFCoord> &dug_coords,
                   std::map<int16_t, std::vector<DFCoord>> &boulder_coords,
                   const dig_now_options &options) {
    MapExtras::MapCache map;
    Random::MersenneRNG rng;

    rng.init();

    for (uint32_t z = options.start.z; z <= options.end.z; ++z) {
        for (uint32_t y = options.start.y; y <= options.end.y; ++y) {
            for (uint32_t x = options.start.x; x <= options.end.x; ++x) {
                // this will return NULL if the map block hasn't been allocated
                // yet, but that means there aren't any designations anyway.
                if (!Maps::getTileBlock(x, y, z))
                    continue;

                DFCoord pos(x, y, z);
                df::tile_designation td = map.designationAt(pos);
                df::tile_occupancy to = map.occupancyAt(pos);
                if (td.bits.dig != df::tile_dig_designation::No) {
                    int16_t wall_mat[2];
                    memset(wall_mat, -1, sizeof(wall_mat));
                    if (dig_tile(out, map, pos, td.bits.dig, wall_mat)) {
                        td = map.designationAt(pos);
                        td.bits.dig = df::tile_dig_designation::No;
                        map.setDesignationAt(pos, td);
                        dug_coords.push_back(pos);
                        for (size_t i = 0; i < 2; ++i) {
                            if (wall_mat[i] < 0)
                                continue;
                            if (produces_boulder(options, rng,
                                    map.BlockAtTile(pos)->veinTypeAt(pos))) {
                                boulder_coords[wall_mat[i]].push_back(pos);
                            }
                        }
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

// if pos is empty space, teleport to floor below
// if we fall out of the world, z coordinate will be negative (invalid)
static DFCoord simulate_fall(const DFCoord &pos) {
    DFCoord resting_pos(pos);

    while (resting_pos.z >= 0) {
        df::tiletype tt = *Maps::getTileType(resting_pos);
        df::tiletype_shape_basic basic_shape = tileShapeBasic(tileShape(tt));
        if (isWalkable(tt) && basic_shape != df::tiletype_shape_basic::Open)
            break;
        --resting_pos.z;
    }

    return resting_pos;
}

static void create_boulders(color_ostream &out,
                const std::map<int16_t, std::vector<DFCoord>> &boulder_coords,
                const dig_now_options &options) {
    df::unit *unit = world->units.active[0];
    df::historical_entity *civ = df::historical_entity::find(unit->civ_id);
    df::world_site *site = World::isFortressMode() ?
            df::world_site::find(ui->site_id) : NULL;

    std::vector<df::reaction_reagent *> in_reag;
    std::vector<df::item *> in_items;

    // TODO: if options.dump_boulders is set, define and use dump coordinates

    for (auto entry : boulder_coords) {
        df::reaction_product_itemst *prod =
                df::allocate<df::reaction_product_itemst>();
        const std::vector<DFCoord> &coords = entry.second;

        prod->item_type = df::item_type::BOULDER;
        prod->item_subtype = -1;
        prod->mat_type = 0;
        prod->mat_index = entry.first;
        prod->probability = 100;
        prod->count = coords.size();
        prod->product_dimension = 1;

        std::vector<df::reaction_product*> out_products;
        std::vector<df::item *> out_items;

        prod->produce(unit, &out_products, &out_items, &in_reag, &in_items, 1,
                      job_skill::NONE, 0, civ, site, NULL);

        size_t num_items = out_items.size();
        if (num_items != coords.size()) {
            out.printerr("unexpected number of boulders produced; "
                         "some boulders may be missing.\n");
            num_items = min(num_items, entry.second.size());
        }

        for (size_t i = 0; i < num_items; ++i) {
            DFCoord pos = simulate_fall(coords[i]);
            if (pos.z < 0) {
                out.printerr("unable to place boulder at (%d, %d, %d)\n",
                             coords[i].x, coords[i].y, coords[i].z);
                continue;
            }
            out_items[i]->moveToGround(pos.x, pos.y, pos.z);
        }

        delete(prod);
    }
}

static void unhide_dug_tiles(color_ostream &out,
                             const std::vector<DFCoord> &dug_coords,
                             const dig_now_options &options) {
    if (options.unhide_unreachable) {
        for (DFCoord pos : dug_coords) {
            if (Maps::getTileDesignation(pos)->bits.hidden)
                flood_unhide(out, pos);
        }
        return;
    }

    // TODO: hide all then flood unreveal starting from an active fortress unit
}

command_result dig_now(color_ostream &out, std::vector<std::string> &) {
    CoreSuspender suspend;

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (world->units.active.size() == 0) {
        out.printerr("At least one unit must be alive!\n");
        return CR_FAILURE;
    }

    dig_now_options options;

    // track which positions were modified and where to produce boulders of a
    // give material
    std::vector<DFCoord> dug_coords;
    std::map<int16_t, std::vector<DFCoord>> boulder_coords;

    do_dig(out, dug_coords, boulder_coords, options);
    create_boulders(out, boulder_coords, options);
    unhide_dug_tiles(out, dug_coords, options);

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
