/*
 * Simulates completion of dig designations.
 */

#include "DataFuncs.h"
#include "PluginManager.h"
#include "TileTypes.h"
#include "LuaTools.h"
#include "Debug.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Random.h"
#include "modules/Units.h"
#include "modules/World.h"
#include "modules/EventManager.h"
#include "modules/Job.h"

#include "df/building.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/map_block.h"
#include "df/reaction_product_itemst.h"
#include "df/tile_designation.h"
#include "df/tile_occupancy.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/vermin.h"
#include "df/world.h"
#include "df/world_site.h"

#include <cinttypes>
#include <unordered_set>
#include <unordered_map>

DFHACK_PLUGIN("dig-now");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

// Debugging
namespace DFHack {
    DBG_DECLARE(dignow, general, DebugCategory::LINFO);
    DBG_DECLARE(dignow, channels, DebugCategory::LINFO);
}

#define COORD "%" PRIi16 " %" PRIi16 " %" PRIi16
#define COORDARGS(id) id.x, id.y, id.z

using namespace DFHack;

struct designation{
    df::coord pos;
    df::tile_designation type;
    df::tile_occupancy occupancy;
    designation() = default;
    designation(const df::coord &c, const df::tile_designation &td, const df::tile_occupancy &to) : pos(c), type(td), occupancy(to) {}

    bool operator==(const designation &rhs) const {
        return pos == rhs.pos;
    }

    bool operator!=(const designation &rhs) const {
        return !(rhs == *this);
    }
};

namespace std {
    template <>
    struct hash<designation> {
        std::size_t operator()(const designation &c) const {
            std::hash<df::coord> hash_coord;
            return hash_coord(c.pos);
        }
    };
}

class DesignationJobs {
private:
    std::unordered_map<df::coord, designation> designations;
    std::unordered_map<df::coord, df::job*> jobs;
public:
    void load(MapExtras::MapCache &map) {
        designations.clear();
        DEBUG(general).print("DesignationJobs: reading jobs list\n");
        df::job_list_link* node = df::global::world->jobs.list.next;
        while (node) {
            df::job* job = node->item;
            node = node->next;

            if(!job || !Maps::isValidTilePos(job->pos))
                continue;

            df::tile_designation td = map.designationAt(job->pos);
            df::tile_occupancy to = map.occupancyAt(job->pos);
            const auto ctd = td.whole;
            const auto cto = to.whole;
            switch (job->job_type){
                case job_type::Dig:
                    td.bits.dig = tile_dig_designation::Default;
                    break;
                case job_type::DigChannel:
                    td.bits.dig = tile_dig_designation::Channel;
                    break;
                case job_type::CarveRamp:
                    td.bits.dig = tile_dig_designation::Ramp;
                    break;
                case job_type::CarveUpwardStaircase:
                    td.bits.dig = tile_dig_designation::UpStair;
                    break;
                case job_type::CarveDownwardStaircase:
                    td.bits.dig = tile_dig_designation::DownStair;
                    break;
                case job_type::CarveUpDownStaircase:
                    td.bits.dig = tile_dig_designation::UpDownStair;
                    break;
                case job_type::SmoothWall:
                case job_type::SmoothFloor:
                    td.bits.smooth = 1;
                    break;
                case job_type::CarveTrack:
                    to.bits.carve_track_north = job->specflag.carve_track_flags.bits.carve_track_north;
                    to.bits.carve_track_south = job->specflag.carve_track_flags.bits.carve_track_south;
                    to.bits.carve_track_west = job->specflag.carve_track_flags.bits.carve_track_west;
                    to.bits.carve_track_east = job->specflag.carve_track_flags.bits.carve_track_east;
                    break;
                default:
                    break;
            }
            if (ctd != td.whole || cto != to.whole) {
                // we found a designation job
                designations.emplace(job->pos, designation(job->pos, td, to));
                jobs.emplace(job->pos, job);
            }
        }
        DEBUG(general).print("DesignationJobs: DONE reading jobs list\n");
    }
    void remove(const df::coord &pos) {
        if(jobs.count(pos)) {
            Job::removeJob(jobs[pos]);
            jobs.erase(pos);
        }
    }
    designation get(const df::coord &pos) {
        if (designations.count(pos)) {
            return designations[pos];
        }
        return {};
    }
    bool count(const df::coord &pos) {
        return jobs.count(pos);
    }
};

struct boulder_percent_options {
    // percent chance ([0..100]) for creating a boulder for the given rock type
    uint32_t layer;
    uint32_t vein;
    uint32_t small_cluster;
    uint32_t deep;

    // defaults from
    // https://dwarffortresswiki.org/index.php/DF2014:Mining
    boulder_percent_options() :
            layer(25), vein(33), small_cluster(100), deep(100) { }

    static struct_identity _identity;
};
static const struct_field_info boulder_percent_options_fields[] = {
    { struct_field_info::PRIMITIVE, "layer",         offsetof(boulder_percent_options, layer),         &df::identity_traits<uint32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "vein",          offsetof(boulder_percent_options, vein),          &df::identity_traits<uint32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "small_cluster", offsetof(boulder_percent_options, small_cluster), &df::identity_traits<uint32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "deep",          offsetof(boulder_percent_options, deep),          &df::identity_traits<uint32_t>::identity, 0, 0 },
    { struct_field_info::END }
};
struct_identity boulder_percent_options::_identity(sizeof(boulder_percent_options), &df::allocator_fn<boulder_percent_options>, NULL, "boulder_percents", NULL, boulder_percent_options_fields);

struct dig_now_options {
    bool help; // whether to show the short help

    DFCoord start; // upper-left coordinate, min z-level
    DFCoord end;   // lower-right coordinate, max z-level

    boulder_percent_options boulder_percents;

    // if set to the pos of a walkable tile (or somewhere above such a tile),
    // will dump generated boulders at this position instead of at their dig
    // locations
    DFCoord dump_pos;

    static DFCoord getMapSize() {
        uint32_t endx, endy, endz;
        Maps::getTileSize(endx, endy, endz);
        return DFCoord(endx - 1, endy - 1, endz - 1);
    }

    dig_now_options() : help(false), start(0, 0, 0), end(getMapSize()) { }

    static struct_identity _identity;
};
static const struct_field_info dig_now_options_fields[] = {
    { struct_field_info::PRIMITIVE, "help",             offsetof(dig_now_options, help),             &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::SUBSTRUCT, "start",            offsetof(dig_now_options, start),            &df::coord::_identity,                0, 0 },
    { struct_field_info::SUBSTRUCT, "end",              offsetof(dig_now_options, end),              &df::coord::_identity,                0, 0 },
    { struct_field_info::SUBSTRUCT, "boulder_percents", offsetof(dig_now_options, boulder_percents), &boulder_percent_options::_identity,  0, 0 },
    { struct_field_info::SUBSTRUCT, "dump_pos",         offsetof(dig_now_options, dump_pos),         &df::coord::_identity,                0, 0 },
    { struct_field_info::END }
};
struct_identity dig_now_options::_identity(sizeof(dig_now_options), &df::allocator_fn<dig_now_options>, NULL, "dig_now_options", NULL, dig_now_options_fields);

// propagate light, outside, and subterranean flags to open tiles below this one
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

    map.setTiletypeAt(pos, tt);

    // digging a tile should revert it to the layer soil/stone material
    if (!blk->setStoneAt(pos, tt, map.layerMaterialAt(pos)))
        blk->setSoilAt(pos, tt, map.layerMaterialAt(pos));
}

static df::tiletype get_target_type(df::tiletype tt, df::tiletype_shape shape) {
    tt = findSimilarTileType(tt, shape);

    // un-smooth dug tiles
    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      df::tiletype_special::NORMAL, tileDirection(tt));

    return findRandomVariant(tt);
}

static void dig_shape(MapExtras::MapCache &map, const DFCoord &pos,
                      df::tiletype tt, df::tiletype_shape shape) {
    dig_type(map, pos, get_target_type(tt, shape));
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
            is_wall(map, DFCoord(pos.x, pos.y+1, pos.z)) ||
            is_wall(map, DFCoord(pos.x-1, pos.y-1, pos.z)) ||
            is_wall(map, DFCoord(pos.x-1, pos.y+1, pos.z)) ||
            is_wall(map, DFCoord(pos.x+1, pos.y-1, pos.z)) ||
            is_wall(map, DFCoord(pos.x+1, pos.y+1, pos.z)))
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
    clean_ramp(map, DFCoord(pos.x-1, pos.y-1, pos.z));
    clean_ramp(map, DFCoord(pos.x-1, pos.y+1, pos.z));
    clean_ramp(map, DFCoord(pos.x+1, pos.y-1, pos.z));
    clean_ramp(map, DFCoord(pos.x+1, pos.y+1, pos.z));
}

// destroys any colonies located at pos
static void destroy_colony(const DFCoord &pos) {
    auto same_pos = [&](df::vermin *colony){ return colony->pos == pos; };

    auto &colonies = world->event.vermin_colonies;
    auto found_colony = std::find_if(begin(colonies), end(colonies), same_pos);
    if (found_colony == end(colonies))
        return;
    colonies.erase(found_colony);

    auto &all_vermin = world->event.vermin;
    all_vermin.erase(
        std::find_if(begin(all_vermin), end(all_vermin), same_pos));
}

struct dug_tile_info {
    DFCoord pos;
    df::tiletype_material tmat;
    df::item_type itype;
    int32_t imat; // mat idx of boulder/gem potentially generated at this pos

    dug_tile_info(MapExtras::MapCache &map, const DFCoord &pos) {
        this->pos = pos;

        df::tiletype tt = map.tiletypeAt(pos);
        tmat = tileMaterial(tt);

        itype = df::item_type::BOULDER;
        imat = -1;

        df::tiletype_shape shape = tileShape(tt);
        if (shape == df::tiletype_shape::WALL || shape == df::tiletype_shape::FORTIFICATION) {
            switch (tmat) {
                case df::tiletype_material::STONE:
                case df::tiletype_material::MINERAL:
                case df::tiletype_material::FEATURE:
                    imat = map.baseMaterialAt(pos).mat_index;
                    break;
                case df::tiletype_material::LAVA_STONE:
                {
                    MaterialInfo mi;
                    if (mi.findInorganic("OBSIDIAN"))
                        imat = mi.index;
                    return; // itype should always be BOULDER, regardless of vein
                }
                default:
                    break;
            }
        }

        switch (map.BlockAtTile(pos)->veinTypeAt(pos)) {
            case df::inclusion_type::CLUSTER_ONE:
            case df::inclusion_type::CLUSTER_SMALL:
                itype = df::item_type::ROUGH;
                break;
            default:
                break;
        }
    }
};

static bool is_diggable(MapExtras::MapCache &map, const DFCoord &pos,
                        df::tiletype tt) {
    df::tiletype_material mat = tileMaterial(tt);
    switch (mat) {
    case df::tiletype_material::CONSTRUCTION:
    case df::tiletype_material::POOL:
    case df::tiletype_material::RIVER:
    case df::tiletype_material::TREE:
    case df::tiletype_material::ROOT:
    case df::tiletype_material::MAGMA:
    case df::tiletype_material::HFS:
    case df::tiletype_material::UNDERWORLD_GATE:
        return false;
    default:
        break;
    }

    if (mat == df::tiletype_material::FEATURE) {
        // adamantine is the only is diggable feature
        t_feature feature;
        return map.BlockAtTile(pos)->GetLocalFeature(&feature)
                && feature.type == feature_type::deep_special_tube;
    }

    return true;
}

static bool dig_tile(color_ostream &out, MapExtras::MapCache &map,
                     const DFCoord &pos, df::tile_dig_designation designation,
                     std::vector<dug_tile_info> &dug_tiles) {
    df::tiletype tt = map.tiletypeAt(pos);

    if (!is_diggable(map, pos, tt)) {
        DEBUG(general).print("dig_tile: not diggable\n");
        return false;
    }

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
                target_type = get_target_type(tt, target_shape);
            }
            break;
        case df::tile_dig_designation::Channel:
        {
            DFCoord pos_below(pos.x, pos.y, pos.z-1);
            if (can_dig_channel(tt) && map.ensureBlockAt(pos_below)
                    && is_diggable(map, pos_below, map.tiletypeAt(pos_below))) {
                TRACE(channels).print("dig_tile: channeling at (" COORD ") [can_dig_channel: true]\n",COORDARGS(pos_below));
                target_type = df::tiletype::OpenSpace;
                DFCoord pos_above(pos.x, pos.y, pos.z+1);
                if (map.ensureBlockAt(pos_above)) {
                    remove_ramp_top(map, pos_above);
                }
                df::tile_dig_designation td_below = map.designationAt(pos_below).bits.dig;
                if (dig_tile(out, map, pos_below, df::tile_dig_designation::Ramp, dug_tiles)) {
                    clean_ramps(map, pos_below);
                    if (td_below == df::tile_dig_designation::Default) {
                        dig_tile(out, map, pos_below, td_below, dug_tiles);
                    }
                    clean_ramps(map, pos);
                    propagate_vertical_flags(map, pos);
                    return true;
                }
            } else {
                DEBUG(channels).print("dig_tile: failed to channel at (" COORD ") [can_dig_channel: false]\n", COORDARGS(pos_below));
            }
            break;
        }
        case df::tile_dig_designation::UpStair:
            if (can_dig_up_stair(tt))
                target_type = get_target_type(tt, df::tiletype_shape::STAIR_UP);
            break;
        case df::tile_dig_designation::DownStair:
            if (can_dig_down_stair(tt)) {
                target_type =
                        get_target_type(tt, df::tiletype_shape::STAIR_DOWN);

            }
            break;
        case df::tile_dig_designation::UpDownStair:
            if (can_dig_up_down_stair(tt)) {
                target_type =
                        get_target_type(tt, df::tiletype_shape::STAIR_UPDOWN);
            }
            break;
        case df::tile_dig_designation::Ramp:
        {
            if (can_dig_ramp(tt)) {
                target_type = get_target_type(tt, df::tiletype_shape::RAMP);
                DFCoord pos_above(pos.x, pos.y, pos.z+1);
                if (target_type != tt && map.ensureBlockAt(pos_above)
                        && is_diggable(map, pos, map.tiletypeAt(pos_above))) {
                    // only capture the tile info of pos_above if we didn't get
                    // here via the Channel case above
                    if (dug_tiles.size() == 0)
                        dug_tiles.push_back(dug_tile_info(map, pos_above));
                    destroy_colony(pos_above);
                    // set tile type directly instead of calling dig_shape
                    // because we need to use *this* tile's material, not the
                    // material of the tile above
                    map.setTiletypeAt(pos_above,
                            get_target_type(tt, df::tiletype_shape::RAMP_TOP));
                    remove_ramp_top(map, DFCoord(pos.x, pos.y, pos.z+2));
                    propagate_vertical_flags(map, DFCoord(pos.x, pos.y, pos.z + 1));
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

    dug_tiles.emplace_back(map, pos);
    TRACE(general).print("dig_tile: digging the designation tile at (" COORD ")\n",COORDARGS(pos));
    dig_type(map, pos, target_type);

    clean_ramps(map, pos);
    return true;
}

static bool is_smooth_wall(MapExtras::MapCache &map, const DFCoord &pos) {
    if (!map.ensureBlockAt(pos))
        return false;
    df::tiletype tt = map.tiletypeAt(pos);
    return tileSpecial(tt) == df::tiletype_special::SMOOTH
                && tileShape(tt) == df::tiletype_shape::WALL;
}

static bool is_connector(MapExtras::MapCache &map, const DFCoord &pos) {
    df::building *bld = Buildings::findAtTile(pos);

    return bld &&
        (bld->getType() == df::building_type::Door ||
         bld->getType() == df::building_type::Floodgate);
}

static bool is_smooth_wall_or_connector(MapExtras::MapCache &map,
                                        const DFCoord &pos) {
    return is_smooth_wall(map, pos) || is_connector(map, pos);
}

// adds adjacent smooth walls and doors to the given tdir
static TileDirection get_adjacent_smooth_walls(MapExtras::MapCache &map,
                                               const DFCoord &pos,
                                               TileDirection tdir) {
    if (is_smooth_wall_or_connector(map, DFCoord(pos.x, pos.y-1, pos.z)))
        tdir.north = 1;
    if (is_smooth_wall_or_connector(map, DFCoord(pos.x, pos.y+1, pos.z)))
        tdir.south = 1;
    if (is_smooth_wall_or_connector(map, DFCoord(pos.x-1, pos.y, pos.z)))
        tdir.west = 1;
    if (is_smooth_wall_or_connector(map, DFCoord(pos.x+1, pos.y, pos.z)))
        tdir.east = 1;
    return tdir;
}

// ensure we have at least two directions enabled (or 0) so we can find a
// matching tiletype. The game chooses to curve "end piece" walls into
// orthogonally adjacent hidden tiles, or uses a pillar if there are no such
// tiles. we take the easier, but not quite conformant, path here and always use
// a pillar for end pieces. If we want to become faithful to how the game does
// it, this code should be moved to the post-processing phase after hidden tiles
// have been revealed. We would also have to scan for wall ends that are no
// longer adjacent to hidden tiles and convert them to pillars when we dig two
// tiles away from such a wall end and reveal their adjacent hidden tile.
static TileDirection ensure_valid_tdir(TileDirection tdir) {
    if (tdir.sum() < 2)
        tdir.whole = 0;
    return tdir;
}

// connects adjacent smooth walls to our new smooth wall
static TileDirection BLANK_TILE_DIRECTION;
static bool adjust_smooth_wall_dir(MapExtras::MapCache &map,
                                   const DFCoord &pos,
                                   TileDirection tdir = BLANK_TILE_DIRECTION) {
    if (!is_smooth_wall(map, pos))
        return is_connector(map, pos);

    tdir = ensure_valid_tdir(get_adjacent_smooth_walls(map, pos, tdir));

    df::tiletype tt = map.tiletypeAt(pos);
    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      tileSpecial(tt), tdir);
    if (tt == df::tiletype::Void)
        return false;

    map.setTiletypeAt(pos, tt);
    return true;
}

static void refresh_adjacent_smooth_walls(MapExtras::MapCache &map,
                                          const DFCoord &pos) {
    adjust_smooth_wall_dir(map, DFCoord(pos.x, pos.y-1, pos.z));
    adjust_smooth_wall_dir(map, DFCoord(pos.x, pos.y+1, pos.z));
    adjust_smooth_wall_dir(map, DFCoord(pos.x-1, pos.y, pos.z));
    adjust_smooth_wall_dir(map, DFCoord(pos.x+1, pos.y, pos.z));
}

// assumes that if the game let you designate a tile for smoothing, it must be
// valid to do so.
static bool smooth_tile(color_ostream &out, MapExtras::MapCache &map,
                        const DFCoord &pos) {
    df::tiletype tt = map.tiletypeAt(pos);

    df::tiletype_shape shape = tileShape(tt);
    df::tiletype_variant variant = tileVariant(tt);
    df::tiletype_special special = df::tiletype_special::SMOOTH;

    TileDirection tdir;
    if (is_smooth_wall(map, pos)) {
        // engraving is filtered out at a higher level, so this is a
        // fortification designation
        shape = tiletype_shape::FORTIFICATION;
        variant = df::tiletype_variant::NONE;
        special = df::tiletype_special::NONE;
    }
    else if (shape == df::tiletype_shape::WALL) {
        if (adjust_smooth_wall_dir(map, DFCoord(pos.x, pos.y-1, pos.z),
                                   TileDirection(0, 1, 0, 0)))
            tdir.north = 1;
        if (adjust_smooth_wall_dir(map, DFCoord(pos.x, pos.y+1, pos.z),
                                TileDirection(1, 0, 0, 0)))
            tdir.south = 1;
        if (adjust_smooth_wall_dir(map, DFCoord(pos.x-1, pos.y, pos.z),
                                TileDirection(0, 0, 0, 1)))
            tdir.west = 1;
        if (adjust_smooth_wall_dir(map, DFCoord(pos.x+1, pos.y, pos.z),
                                TileDirection(0, 0, 1, 0)))
            tdir.east = 1;
        tdir = ensure_valid_tdir(tdir);
    }

    tt = findTileType(shape, tileMaterial(tt), variant, special, tdir);
    if (tt == df::tiletype::Void)
        return false;

    map.setTiletypeAt(pos, tt);
    return true;
}

// assumes that if the game let you designate a tile for track carving, it must
// be valid to do so.
static bool carve_tile(MapExtras::MapCache &map,
                       const DFCoord &pos, df::tile_occupancy &to) {
    df::tiletype tt = map.tiletypeAt(pos);
    TileDirection tdir = tileDirection(tt);

    if (to.bits.carve_track_north)
        tdir.north = 1;
    if (to.bits.carve_track_east)
        tdir.east = 1;
    if (to.bits.carve_track_south)
        tdir.south = 1;
    if (to.bits.carve_track_west)
        tdir.west = 1;

    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      df::tiletype_special::TRACK, tdir);
    if (tt == df::tiletype::Void)
        return false;

    map.setTiletypeAt(pos, tt);
    return true;
}

static bool produces_item(const boulder_percent_options &options,
                          MapExtras::MapCache &map, Random::MersenneRNG &rng,
                          const dug_tile_info &info) {
    uint32_t probability;
    if (info.tmat == df::tiletype_material::FEATURE)
        probability = options.deep;
    else {
        switch (map.BlockAtTile(info.pos)->veinTypeAt(info.pos)) {
            case df::inclusion_type::CLUSTER:
            case df::inclusion_type::VEIN:
                probability = options.vein;
                break;
            case df::inclusion_type::CLUSTER_ONE:
            case df::inclusion_type::CLUSTER_SMALL:
                probability = options.small_cluster;
                break;
            default:
                probability = options.layer;
                break;
        }
    }

    return rng.random(100) < probability;
}

typedef std::map<std::pair<df::item_type, int32_t>, std::vector<DFCoord>>
    item_coords_t;

static void do_dig(color_ostream &out, std::vector<DFCoord> &dug_coords,
                   item_coords_t &item_coords, const dig_now_options &options) {
    MapExtras::MapCache map;
    Random::MersenneRNG rng;
    DesignationJobs jobs;

    DEBUG(general).print("do_dig(): starting..\n");
    jobs.load(map);
    rng.init();

    DEBUG(general).print("do_dig(): reading map..\n");
    std::unordered_set<designation> buffer;
    // go down levels instead of up so stacked ramps behave as expected
    for (int16_t z = options.end.z; z >= options.start.z; --z) {
        for (int16_t y = options.start.y; y <= options.end.y; ++y) {
            for (int16_t x = options.start.x; x <= options.end.x; ++x) {
                // this will return NULL if the map block hasn't been allocated
                // yet, but that means there aren't any designations anyway.
                if (!Maps::getTileBlock(x, y, z))
                    continue;

                DFCoord pos(x, y, z);
                df::tile_designation td = map.designationAt(pos);
                df::tile_occupancy to = map.occupancyAt(pos);
                if (jobs.count(pos)) {
                    buffer.emplace(jobs.get(pos));
                    jobs.remove(pos);
                    // if it does get removed, then we're gonna buffer the jobs info then remove the job
                } else if ((td.bits.dig != df::tile_dig_designation::No && !to.bits.dig_marked)
                    || td.bits.smooth == 1
                    || to.bits.carve_track_north == 1
                    || to.bits.carve_track_east == 1
                    || to.bits.carve_track_south == 1
                    || to.bits.carve_track_west == 1) {

                    // we're only buffering designations, so that processing doesn't affect what we're buffering
                    buffer.emplace(pos, td, to);
                }
            }
        }
    }

    DEBUG(general).print("do_dig(): processing designations..\n");
    // process designations
    for(auto &d : buffer) {
        auto pos = d.pos;
        auto td = d.type;
        auto to = d.occupancy;

        if (td.bits.dig != df::tile_dig_designation::No && !to.bits.dig_marked) {
            std::vector<dug_tile_info> dug_tiles;

            if (dig_tile(out, map, pos, td.bits.dig, dug_tiles)) {
                for (auto info: dug_tiles) {
                    td = map.designationAt(info.pos);
                    td.bits.dig = df::tile_dig_designation::No;
                    map.setDesignationAt(info.pos, td);

                    dug_coords.push_back(info.pos);
                    refresh_adjacent_smooth_walls(map, info.pos);
                    if (info.imat < 0)
                        continue;
                    if (produces_item(options.boulder_percents,
                                      map, rng, info)) {
                        auto k = std::make_pair(info.itype, info.imat);
                        item_coords[k].push_back(info.pos);
                    }
                }
            }
        } else if (td.bits.smooth == 1) {
            if (smooth_tile(out, map, pos)) {
                td = map.designationAt(pos);
                td.bits.smooth = 0;
                map.setDesignationAt(pos, td);
            }
        } else if (to.bits.carve_track_north == 1
                   || to.bits.carve_track_east == 1
                   || to.bits.carve_track_south == 1
                   || to.bits.carve_track_west == 1) {
            if (carve_tile(map, pos, to)) {
                to = map.occupancyAt(pos);
                to.bits.carve_track_north = 0;
                to.bits.carve_track_east = 0;
                to.bits.carve_track_south = 0;
                to.bits.carve_track_west = 0;
                map.setOccupancyAt(pos, to);
            }
        }
    }

    DEBUG(general).print("do_dig(): write changes to map..\n");
    map.WriteAll();
}

// if pos is empty space, teleport to a floor somewhere below
// if we fall out of the world (e.g. empty space or walls all the way down),
// returned position will be invalid
static DFCoord simulate_fall(const DFCoord &pos) {
    DFCoord resting_pos(pos);

    while (Maps::ensureTileBlock(resting_pos)) {
        df::tiletype tt = *Maps::getTileType(resting_pos);
        if (isWalkable(tt))
            break;
        --resting_pos.z;
    }

    return resting_pos;
}

static void create_boulders(color_ostream &out,
                const item_coords_t &item_coords,
                const dig_now_options &options,
                df::unit * creator) {
    df::historical_entity *civ = df::historical_entity::find(creator->civ_id);
    df::world_site *site = World::isFortressMode() ?
            df::world_site::find(plotinfo->site_id) : NULL;

    std::vector<df::reaction_reagent *> in_reag;
    std::vector<df::item *> in_items;

    DFCoord dump_pos;
    if (Maps::isValidTilePos(options.dump_pos)) {
        dump_pos = simulate_fall(options.dump_pos);
        if (!Maps::ensureTileBlock(dump_pos))
            out.printerr("Invalid dump tile coordinates! Ensure the --dump"
                " option specifies an open, non-wall tile.");
    }

    for (auto entry : item_coords) {
        df::reaction_product_itemst *prod =
                df::allocate<df::reaction_product_itemst>();
        const std::vector<DFCoord> &coords = entry.second;

        prod->item_type = entry.first.first;
        prod->item_subtype = -1;
        prod->mat_type = 0;
        prod->mat_index = entry.first.second;
        prod->probability = 100;
        prod->product_dimension = 1;

        std::vector<df::reaction_product*> out_products;
        std::vector<df::item *> out_items;

        size_t remaining_items = coords.size();
        while (remaining_items > 0) {
            int16_t batch_size = std::min(remaining_items,
                                          static_cast<size_t>(INT16_MAX));
            prod->count = batch_size;
            remaining_items -= batch_size;
            prod->produce(creator, &out_products, &out_items, &in_reag, &in_items,
                          1, job_skill::NONE, 0, civ, site, NULL);
        }

        size_t num_items = out_items.size();
        if (num_items != coords.size()) {
            MaterialInfo material;
            material.decode(prod->mat_type, prod->mat_index);
            out.printerr("unexpected number of %s %s produced: expected %zd,"
                         " got %zd.\n",
                         material.toString().c_str(),
                         ENUM_KEY_STR(item_type, prod->item_type).c_str(),
                         coords.size(), num_items);
            num_items = std::min(num_items, entry.second.size());
        }

        for (size_t i = 0; i < num_items; ++i) {
            DFCoord pos = Maps::isValidTilePos(dump_pos) ?
                    dump_pos : simulate_fall(coords[i]);
            if (!Maps::ensureTileBlock(pos)) {
                out.printerr(
                        "unable to place boulder generated at (%d, %d, %d)\n",
                        coords[i].x, coords[i].y, coords[i].z);
                continue;
            }
            out_items[i]->moveToGround(pos.x, pos.y, pos.z);
        }

        delete(prod);
    }
}

static bool needs_unhide(const DFCoord &pos) {
    return !Maps::ensureTileBlock(pos)
        || Maps::getTileDesignation(pos)->bits.hidden;
}

static bool needs_flood_unhide(const DFCoord &pos) {
    return needs_unhide(pos)
        || needs_unhide(DFCoord(pos.x-1, pos.y-1, pos.z))
        || needs_unhide(DFCoord(pos.x, pos.y-1, pos.z))
        || needs_unhide(DFCoord(pos.x+1, pos.y-1, pos.z))
        || needs_unhide(DFCoord(pos.x-1, pos.y, pos.z))
        || needs_unhide(DFCoord(pos.x+1, pos.y, pos.z))
        || needs_unhide(DFCoord(pos.x-1, pos.y+1, pos.z))
        || needs_unhide(DFCoord(pos.x, pos.y+1, pos.z))
        || needs_unhide(DFCoord(pos.x+1, pos.y+1, pos.z));
}

static void post_process_dug_tiles(color_ostream &out,
                             const std::vector<DFCoord> &dug_coords) {
    for (DFCoord pos : dug_coords) {
        if (needs_flood_unhide(pos)) {
            // set current tile to hidden to allow flood_unhide to work on tiles
            // that were already visible but that reveal hidden tiles when dug.
            Maps::getTileDesignation(pos)->bits.hidden = true;
            Lua::CallLuaModuleFunction(out, "plugins.reveal", "unhideFlood", std::make_tuple(pos));
        }

        df::tile_occupancy &to = *Maps::getTileOccupancy(pos);
        if (to.bits.unit || to.bits.item) {
            DFCoord resting_pos = simulate_fall(pos);
            if (resting_pos == pos)
                continue;

            if (!Maps::ensureTileBlock(resting_pos)) {
                out.printerr("No valid tile beneath (%d, %d, %d); can't move"
                             " units and items to floor",
                             pos.x, pos.y, pos.z);
                continue;
            }

            if (to.bits.unit) {
                std::vector<df::unit*> units;
                Units::getUnitsInBox(units, pos.x, pos.y, pos.z,
                                     pos.x, pos.y, pos.z);
                for (auto unit : units)
                    Units::teleport(unit, resting_pos);
            }

            if (to.bits.item) {
                std::vector<df::item*> items;
                if (auto b = Maps::ensureTileBlock(pos)) {
                    for (auto item_id : b->items) {
                        auto item = df::item::find(item_id);
                        if (item && item->pos == pos)
                            items.emplace_back(item);
                    }
                }
                if (!items.empty()) {
                    // fresh MapCache since tile properties are being actively changed
                    MapExtras::MapCache mc;
                    for (auto item : items)
                        Items::moveToGround(mc, item, resting_pos);
                    mc.WriteAll();
                }
            }
        }

        // refresh block metadata and flows
        Maps::enableBlockUpdates(Maps::getTileBlock(pos), true, true);
    }
}

static bool get_options(color_ostream &out,
                        dig_now_options &opts,
                        const std::vector<std::string> &parameters) {
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, "plugins.dig-now", "parse_commandline")) {
        out.printerr("Failed to load dig-now Lua code\n");
        return false;
    }

    Lua::Push(L, &opts);

    for (const std::string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

bool dig_now_impl(color_ostream &out, const dig_now_options &options) {
    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return false;
    }

    // required for boulder generation
    if (world->units.active.size() == 0) {
        out.printerr("At least one unit must be alive!\n");
        return false;
    }

    // track which positions were modified and where to produce items
    std::vector<DFCoord> dug_coords;
    item_coords_t item_coords;

    do_dig(out, dug_coords, item_coords, options);
    create_boulders(out, item_coords, options, world->units.active[0]);
    post_process_dug_tiles(out, dug_coords);

    // force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    return true;
}

command_result dig_now(color_ostream &out, std::vector<std::string> &params) {
    CoreSuspender suspend;

    dig_now_options options;
    if (!get_options(out, options, params) || options.help)
        return CR_WRONG_USAGE;

    return dig_now_impl(out, options) ? CR_OK : CR_FAILURE;
}

DFhackCExport command_result plugin_init(color_ostream &,
                                         std::vector<PluginCommand> &commands) {
    commands.push_back(
        PluginCommand(
            "dig-now",
            "Instantly complete dig designations.",
            dig_now));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    return CR_OK;
}

// Lua API

// runs dig-now for the specified tile coordinate. default options apply.
static int dig_now_tile(lua_State *L)
{
    DFCoord pos;
    if (lua_gettop(L) <= 1)
        Lua::CheckDFAssign(L, &pos, 1);
    else
        pos = DFCoord(luaL_checkint(L, 1), luaL_checkint(L, 2),
                      luaL_checkint(L, 3));

    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();

    dig_now_options options;
    options.start = pos;
    options.end = pos;
    lua_pushboolean(L, dig_now_impl(*out, options));

    return 1;
}

static int link_adjacent_smooth_walls(lua_State *L)
{
    DFCoord pos;
    if (lua_gettop(L) <= 1)
        Lua::CheckDFAssign(L, &pos, 1);
    else
        pos = DFCoord(luaL_checkint(L, 1), luaL_checkint(L, 2),
                      luaL_checkint(L, 3));

    MapExtras::MapCache map;
    adjust_smooth_wall_dir(map, pos);
    refresh_adjacent_smooth_walls(map, pos);
    map.WriteAll();
    return 0;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(dig_now_tile),
    DFHACK_LUA_COMMAND(link_adjacent_smooth_walls),
    DFHACK_LUA_END
};
