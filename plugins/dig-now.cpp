/*
 * Simulates completion of dig designations.
 */

#include "DataFuncs.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Random.h"
#include "modules/Units.h"
#include "modules/World.h"

#include <df/block_square_event.h>
#include <df/block_square_event_frozen_liquidst.h>
#include <df/block_square_event_mineralst.h>
#include "df/builtin_mats.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/map_block.h"
#include "df/material.h"
#include "df/plotinfost.h"
#include "df/reaction_product_itemst.h"
#include "df/region_map_entry.h"
#include "df/tile_designation.h"
#include "df/tile_occupancy.h"
#include "df/unit.h"
#include "df/vermin.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/world_region_details.h"
#include "df/world_site.h"

#include <cinttypes>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

DFHACK_PLUGIN("dig-now");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

// Debugging
namespace DFHack {
    DBG_DECLARE(dignow, general, DebugCategory::LINFO);
    DBG_DECLARE(dignow, channels, DebugCategory::LINFO);
}

using std::min;
using std::vector;
using namespace DFHack;

static const df::region_map_entry* biome_at(const df::map_block &block, const df::coord2d p)
{
    auto blockptr = &static_cast<df::map_block &>(const_cast<df::map_block &>(block));
    auto biome = Maps::getRegionBiome(Maps::getBlockTileBiomeRgn(blockptr, p));
    return biome;
}

static const df::region_map_entry* biome_at(const df::coord pos)
{
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return nullptr;
    auto blockref = static_cast<const df::map_block &>(*block);
    return biome_at(blockref, df::coord2d(pos) & 15);
}

static const df::world_geo_biome* geo_biome_at(const df::map_block &block, const df::coord2d p)
{
    auto biome = biome_at(block, p);
    if (!biome)
        return nullptr;
    if (biome->geo_index < 0)
        return nullptr;
    if (biome->geo_index >= df::world_geo_biome::get_vector().size())
        return nullptr;
    auto geo_biome = df::world_geo_biome::get_vector()[biome->geo_index];
    return geo_biome;
}

static const df::world_geo_biome* geo_biome_at(const df::coord pos)
{
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return nullptr;
    auto blockref = static_cast<const df::map_block &>(*block);
    return geo_biome_at(blockref, df::coord2d(pos) & 15);
}

static const t_matpair layer_inorganic_n(const df::map_block &block, const df::coord2d p, size_t layer)
{
    using namespace df::enums::builtin_mats;
    auto geo_biome = geo_biome_at(block, p);
    if (!geo_biome)
        return t_matpair(INORGANIC, -1);    // can't happen, generic "rock"
    layer = clip_range(layer, 0, geo_biome->layers.size() - 1);
    return t_matpair(INORGANIC, geo_biome->layers[layer]->mat_index);
}

static const t_matpair layer_inorganic_n(const df::coord pos, size_t layer)
{
    using namespace df::enums::builtin_mats;
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return t_matpair(INORGANIC, -1);    // can't happen, generic "rock"
    auto blockref = static_cast<const df::map_block &>(*block);
    return layer_inorganic_n(blockref, df::coord2d(pos) & 15, layer);
}

static const size_t geolayer_at(const df::map_block &block, df::coord2d p)
{
    return index_tile(block.designation, p).bits.geolayer_index;
}

static const t_matpair layer_inorganic_at(const df::map_block &block, df::coord2d p)
{
    return layer_inorganic_n(block, p, geolayer_at(block, p));
}

static const t_matpair layer_inorganic_at(const df::coord pos)
{
    using namespace df::enums::builtin_mats;
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return t_matpair(INORGANIC, -1);    // generic "rock"
    auto blockref = const_cast<const df::map_block &>(static_cast<df::map_block &>(*block));
    return layer_inorganic_at(blockref, df::coord2d(pos) & 15);
}

static const df::enums::tiletype_material::tiletype_material getGroundType(int32_t mat_index)
{
    if (isSoilInorganic(mat_index))
        return df::enums::tiletype_material::SOIL;
    if (isStoneInorganic(mat_index))
        return df::enums::tiletype_material::STONE;
    return df::enums::tiletype_material::NONE;
}

static const df::enums::tiletype_material::tiletype_material getGroundType(const t_matpair matpair)
{
    using namespace df::enums::builtin_mats;
    if (matpair.mat_type != INORGANIC)
        return df::enums::tiletype_material::NONE;
    return getGroundType(matpair.mat_index);
}

// copied from Maps, used by getBiomeRgnPos()
static df::coord2d biome_offsets[9] = {
    df::coord2d(-1,-1), df::coord2d(0,-1), df::coord2d(1,-1),
    df::coord2d(-1,0), df::coord2d(0,0), df::coord2d(1,0),
    df::coord2d(-1,1), df::coord2d(0,1), df::coord2d(1,1)
};

// copied from Maps.  arguably this should be exported from Maps; it's useful.
inline df::coord2d getBiomeRgnPos(const df::coord2d base, BiomeOffset idx)
{
    auto r = base + biome_offsets[idx];

    int world_width = world->world_data->world_width;
    int world_height = world->world_data->world_height;

    return df::coord2d(clip_range(r.x, 0, world_width-1),
            clip_range(r.y, 0, world_height-1));
}

// copied and modified from MapCache getBaseMaterial().
// returns the material that should be used for boulders/gems,
// or the invalid material t_matpair(-1).
static const t_matpair baseMaterialAt(const df::map_block &block, df::coord2d p)
{
    using namespace df::enums::builtin_mats;
    using namespace df::enums::tiletype_material;

    p = p & 15;
    t_matpair rv;
    auto tt = index_tile(block.tiletype, p);
    switch (tileMaterial(tt)) {
        // not diggable, should not drop boulders.
        case df::enums::tiletype_material::NONE:
        case AIR:
        case CONSTRUCTION:
        case HFS:
        case CAMPFIRE:
        case FIRE:
        case MAGMA:
        case POOL:
        case RIVER:
        case TREE:
        case ROOT:      //  TODO this is a bug; DF can dig roots.
        case MUSHROOM:
        case UNDERWORLD_GATE:
            rv = t_matpair(-1);
            break;

        //  diggable but should not drop boulders, so return the first layer, presumably soil.
        //  (these won't drop anyway because they are not WALL or FORTIFICATION shaped, but safety first.)
        case DRIFTWOOD:
        case BROOK:
        case ASHES:
        case PLANT:
        case GRASS_LIGHT:
        case GRASS_DARK:
        case GRASS_DRY:
        case GRASS_DEAD:
            rv = layer_inorganic_n(block, p, 0);
            break;

        case STONE:
        {
            rv = t_matpair(INORGANIC, -1);    // fallback: generic "rock"
            auto geo_biome = geo_biome_at(block, p);
            if (!geo_biome)
                break;

            //  IS THIS CODE CORRECT?
            //  intent here is to index 0..15 elements into a vector of unknown length,
            //  and get the element at vector[ min(index, last_element_offset) ] .
            auto geo_layer_it = begin(geo_biome->layers);
            std::ranges::advance(geo_layer_it, geolayer_at(block, p), end(geo_biome->layers) - 1);
            rv = t_matpair(INORGANIC, (*geo_layer_it)->mat_index);
            if (getGroundType(rv) == STONE)
                break;

            // fall back to the first stone layer, or the very last layer.

            //  IS TNIS CODE CORRECT?
            //  intent here is to search a vector of unknown length for the
            //    first element matching a predicate, and get that element
            //    OR the last element in the vector, whether or not it matches.
            //  this is a lot of faffing around for something that's more concise
            //    in C style C++; see the SOIL code below.
            //  maybe I could make the iterators use the layer_inorganic_n function?
            auto is_stone = [&](df::world_geo_layer *geo_layer)
                    { return getGroundType(geo_layer->mat_index) == STONE; };
            // this vector typically has 16 entries, but I have seen 13 in oceans.
            auto &geo_layers = geo_biome->layers;
            geo_layer_it = std::find_if(begin(geo_layers), end(geo_layers) - 1, is_stone);
            rv = t_matpair(INORGANIC, (*geo_layer_it)->mat_index);
            break;
        }

        case SOIL:
            rv = layer_inorganic_at(block, p);
            if (getGroundType(rv) != SOIL) {
                // fall back to the last soil layer, or the very first layer.
                for (auto i = 15; i >= 0; i--) {
                    rv = layer_inorganic_n(block, p, i);    // this call clips to vector size.
                    if (getGroundType(rv) == SOIL)
                        break;
                }
            }
            break;

        case FEATURE:
        {
            // fallback: no feature flag, or no event?  no drops.
            rv = t_matpair(-1);
            auto des = index_tile(block.designation, p);
            t_feature feature;
            feature.type = df::enums::feature_type::feature_type::NONE;
            if (des.bits.feature_local)
                Maps::ReadFeatures(
                        &(static_cast<df::map_block &>(const_cast<df::map_block &>(block))),
                        &feature, nullptr);
            else if (des.bits.feature_global)
                Maps::ReadFeatures(
                        &(static_cast<df::map_block &>(const_cast<df::map_block &>(block))),
                        nullptr, &feature);
            if (feature.type != df::enums::feature_type::feature_type::NONE)
                rv = t_matpair(feature.main_material, feature.sub_material);
            break;
        }

        case LAVA_STONE:
        {
            auto region_details = world->world_data->midmap_data.region_details;
            auto des = index_tile(block.designation, p);
            auto block_region_offset_idx = des.bits.biome;
            if (block_region_offset_idx >= DFHack::eBiomeCount)
                // "can't happen", fall back to the local region tile's biome.
                block_region_offset_idx = DFHack::eHere;
            auto region_details_coord2d = getBiomeRgnPos(block.map_pos,
                static_cast<BiomeOffset>(block_region_offset_idx));
            auto region_details_idx = linear_index(region_details,
                    &df::world_region_details::pos, region_details_coord2d);
            if (region_details_idx == -1)
            {
                // "can't happen"; fall back to the first region.
                DEBUG(general).print("BaseMaterialAt(block {}, tile {}, case "
                        "LAVA_STONE, didn't find region_details for region coord {}\n",
                        block.map_pos, p, region_details_coord2d);
                region_details_idx = 0;
            }
            rv = t_matpair(INORGANIC, region_details[region_details_idx]->lava_stone);
            break;
        }

        case MINERAL:
            // fall back to the layer material, whether SOIL or STONE.
            rv = layer_inorganic_at(block, p);
            for (auto event : block.block_events) {
                if (event->getType() == df::block_square_event_type::mineral) {
                    auto vein = (df::block_square_event_mineralst *)event;
                    if (vein->getassignment(p)) {
                        rv = t_matpair(INORGANIC, vein->inorganic_mat);
                        // do not early-out.  later mineral events can override earlier ones.
                    }
                }
            }
            break;

        case FROZEN_LIQUID:
            rv = t_matpair(WATER, 0);
            break;

        // no default so we get a compiler warning.
    }

    if (rv == t_matpair(-1))
        return t_matpair(-1);
    MaterialInfo mi;
    mi.decode(rv);
    if (mi.isValid() && mi.material->flags.is_set(df::material_flags::UNDIGGABLE))
        return t_matpair(-1);
    return rv;
}

static const t_matpair baseMaterialAt(df::coord pos)
{
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return t_matpair(-1);
    auto blockref = const_cast<const df::map_block &>(static_cast<df::map_block &>(*block));
    return baseMaterialAt(blockref, df::coord2d(pos) & 15);
}

static const df::inclusion_type veinTypeAt(df::map_block &block, df::coord2d p)
{
    using namespace df::enums::inclusion_type;
    auto veintype = TOTAL;
    for (auto event : block.block_events) {
        if (event->getType() == df::block_square_event_type::mineral) {
            auto vein = (df::block_square_event_mineralst *)event;
            if (vein->getassignment(p)) {
                if (vein->flags.bits.cluster_one)
                    veintype = CLUSTER_ONE;
                else if (vein->flags.bits.cluster_small)
                    veintype = CLUSTER_SMALL;
                else if (vein->flags.bits.vein)
                    veintype = VEIN;
                else if (vein->flags.bits.cluster)
                    veintype = CLUSTER;
                // do not early-out.  later mineral events can override earlier ones.
            }
        }
    }
    return veintype;
}

static const df::inclusion_type veinTypeAt(df::coord pos)
{
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return df::enums::inclusion_type::TOTAL;
    auto blockref = const_cast<const df::map_block &>(static_cast<df::map_block &>(*block));
    return veinTypeAt(blockref, df::coord2d(pos) & 15);
}

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

// TODO maybe: consider using the version in coord.methods.inc + DataDefs.h
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
    void load() {
        designations.clear();
        DEBUG(general).print("DesignationJobs: reading jobs list\n");
        df::job_list_link* node = df::global::world->jobs.list.next;
        while (node) {
            df::job* job = node->item;
            node = node->next;

            if(!job || !Maps::isValidTilePos(job->pos))
                continue;

            df::tile_designation td = *Maps::getTileDesignation(job->pos);
            df::tile_occupancy to = *Maps::getTileOccupancy(job->pos);
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

    df::coord start; // upper-left coordinate, min z-level
    df::coord end;   // lower-right coordinate, max z-level

    boulder_percent_options boulder_percents;

    // if set to the pos of a walkable tile (or somewhere above such a tile),
    // will dump generated boulders at this position instead of at their dig
    // locations
    df::coord dump_pos;

    static df::coord getMapSize() {
        uint32_t endx, endy, endz;
        Maps::getTileSize(endx, endy, endz);
        return df::coord(endx - 1, endy - 1, endz - 1);
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
static void propagate_vertical_flags(const df::coord &pos) {
    df::tile_designation td = *Maps::getTileDesignation(pos);

    if (!Maps::ensureTileBlock(df::coord(pos.x, pos.y, pos.z+1))) {
        // only the sky above
        td.bits.light = true;
        td.bits.outside = true;
        td.bits.subterranean = false;
    }

    int32_t zlevel = pos.z;
    df::tiletype_shape shape =
            tileShape(*Maps::getTileType(df::coord(pos.x, pos.y, zlevel)));
    while ((shape == df::tiletype_shape::EMPTY
            || shape == df::tiletype_shape::RAMP_TOP)
            && Maps::ensureTileBlock(df::coord(pos.x, pos.y, --zlevel))) {
        df::coord pos_below(pos.x, pos.y, zlevel);
        df::tile_designation td_below = *Maps::getTileDesignation(pos_below);
        if (td_below.bits.light == td.bits.light
                && td_below.bits.outside == td.bits.outside
                && td_below.bits.subterranean == td.bits.subterranean)
            break;
        td_below.bits.light = td.bits.light;
        td_below.bits.outside = td.bits.outside;
        td_below.bits.subterranean = td.bits.subterranean;
        *Maps::getTileDesignation(pos_below) = td_below;
        shape = tileShape(*Maps::getTileType(pos_below));
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

static void dig_type(const df::coord pos, df::tiletype in_tt) {
    using namespace df::enums::tiletype_material;
    using namespace df::enums::tiletype_shape_basic;

    auto block = Maps::getTileBlock(pos);
    if (!block)
        return;

    // digging a tile should revert it to the layer soil/stone material
    // TODO that's a bug, actually, that is not what the game does.
    //      this bug has been temporarily preserved to remain bug-for-bug compatible.
    //      this bug is NOT the same bug as the preserved bug 25 lines below.
    auto tt = in_tt;
    if (tileShapeBasic(tileShape(tt)) != Open) {
        auto matpair = baseMaterialAt(pos);
        auto ground_type = getGroundType(matpair);
        if (ground_type == SOIL || ground_type == STONE)
            tt = matchTileMaterial(tt, ground_type);
        else
            DEBUG(general).print("dig_type: getGroundType did not return SOIL or STONE."
                        "  not updating tiletype material. {} in:{} out:{} ({},{})\n",
                        pos, static_cast<size_t>(in_tt), static_cast<size_t>(tt),
                        matpair.mat_type, matpair.mat_index);
        if (tt == df::tiletype::Void)
            DEBUG(general).print("dig_type: matchTileMaterial: tiletype is Void."
                    "  {} in:{} out:{} ({},{})\n",
                    pos, static_cast<size_t>(in_tt), static_cast<size_t>(tt),
                    matpair.mat_type, matpair.mat_index);
        if (tileMaterial(tt) == NONE)   // true for tiletypes Void and Unused[0-9]+
            DEBUG(general).print("dig_type: matchTileMaterial: tiletype_material is NONE."
                    "  {} in:{} out:{} ({},{})\n",
                    pos, static_cast<size_t>(in_tt), static_cast<size_t>(tt),
                    matpair.mat_type, matpair.mat_index);
    }

    // this segment temporarily reimplements buggy behavior to remain bug-for-bug compatible.
    // specifically, STONE tiles (i.e. layer stone) which are of the material type
    // of the current region's .lava_stone (e.g. obsidian) are changed to LAVA_STONE.
    // (later: unless they are in a local feature, apparently.)
    if (true) {     // TODO when bugfixing starts, remove this block.
        auto matpair = baseMaterialAt(pos);
        auto des = index_tile(block->designation, pos);
        auto block_region_offset_idx = des.bits.biome;
        auto region_details_idx = block->region_offset[block_region_offset_idx];
        if (matpair.mat_index == world->world_data->midmap_data.region_details[region_details_idx]->lava_stone
                && ! des.bits.feature_local)
            tt = matchTileMaterial(tt, LAVA_STONE);
    }

    if (tt == df::tiletype::Void)
        DEBUG(general).print("dig_type: setting tile: tiletype is Void.  {} in:{} out:{}\n",
                pos, static_cast<size_t>(in_tt), static_cast<size_t>(tt));
    if (tileMaterial(tt) == NONE)   // true for tiletypes Void and Unused[0-9]+
        DEBUG(general).print("dig_type: setting tile: tiletype_material is NONE.  {} in:{} out:{}\n",
                pos, static_cast<size_t>(in_tt), static_cast<size_t>(tt));
    index_tile(block->tiletype, pos) = tt;
}

static df::tiletype get_target_type(df::tiletype tt, df::tiletype_shape shape) {
    tt = findSimilarTileType(tt, shape);

    // un-smooth dug tiles
    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      df::tiletype_special::NORMAL, tileDirection(tt));

    return findRandomVariant(tt);
}

static void dig_shape(const df::coord pos, df::tiletype tt, df::tiletype_shape shape) {
    dig_type(pos, get_target_type(tt, shape));
}

static void remove_ramp_top(const df::coord pos) {
    if (!Maps::ensureTileBlock(pos))
        return;

    if (tileShape(*Maps::getTileType(pos)) == df::tiletype_shape::RAMP_TOP)
        dig_type(pos, df::tiletype::OpenSpace);
}

static bool is_wall(const df::coord pos) {
    if (!Maps::ensureTileBlock(pos))
        return false;
    return tileShape(*Maps::getTileType(pos)) == df::tiletype_shape::WALL;
}

static void clean_ramp(const df::coord pos) {
    if (!Maps::ensureTileBlock(pos))
        return;

    df::tiletype tt = *Maps::getTileType(pos);
    if (tileShape(tt) != df::tiletype_shape::RAMP)
        return;

    if (    is_wall(df::coord(pos.x-1, pos.y,   pos.z)) ||
            is_wall(df::coord(pos.x+1, pos.y,   pos.z)) ||
            is_wall(df::coord(pos.x,   pos.y-1, pos.z)) ||
            is_wall(df::coord(pos.x,   pos.y+1, pos.z)) ||
            is_wall(df::coord(pos.x-1, pos.y-1, pos.z)) ||
            is_wall(df::coord(pos.x-1, pos.y+1, pos.z)) ||
            is_wall(df::coord(pos.x+1, pos.y-1, pos.z)) ||
            is_wall(df::coord(pos.x+1, pos.y+1, pos.z))   )
        return;

    remove_ramp_top(df::coord(pos.x, pos.y, pos.z+1));
    dig_shape(pos, tt, df::tiletype_shape::FLOOR);
}

// removes self and/or orthogonally adjacent ramps that are no longer adjacent
// to a wall
static void clean_ramps(const df::coord pos) {
    // TODO possible bug; should clean_ramp this tile *last*.
    clean_ramp(pos);
    clean_ramp(df::coord(pos.x-1, pos.y,   pos.z));
    clean_ramp(df::coord(pos.x+1, pos.y,   pos.z));
    clean_ramp(df::coord(pos.x,   pos.y-1, pos.z));
    clean_ramp(df::coord(pos.x,   pos.y+1, pos.z));
    clean_ramp(df::coord(pos.x-1, pos.y-1, pos.z));
    clean_ramp(df::coord(pos.x-1, pos.y+1, pos.z));
    clean_ramp(df::coord(pos.x+1, pos.y-1, pos.z));
    clean_ramp(df::coord(pos.x+1, pos.y+1, pos.z));
}

// destroys any colonies located at pos
static void destroy_colony(const df::coord pos) {
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
    df::coord pos;
    df::tiletype_material tmat;
    df::item_type itype;
    t_matpair imat; // matpair of boulder/gem potentially generated at this pos

    dug_tile_info(const df::coord pos) {
        this->pos = pos;

        auto tt = *Maps::getTileType(pos);
        tmat = tileMaterial(tt);

        itype = df::item_type::BOULDER;
        imat = t_matpair(-1);

        df::tiletype_shape shape = tileShape(tt);
        if (shape != df::tiletype_shape::WALL && shape != df::tiletype_shape::FORTIFICATION)
            return;

        imat = baseMaterialAt(pos);
        if (imat == t_matpair(-1))
            return;

        MaterialInfo mi;
        mi.decode(imat);
        if (mi.type == -1 || !mi.material)
            return;

        if (mi.material->isGem())
            itype = df::item_type::ROUGH;
    }
};

static bool is_diggable(const df::coord pos, df::tiletype tt) {
    using namespace df::enums::tiletype_material;
    df::tiletype_material mat = tileMaterial(tt);
    switch (mat) {
        case CONSTRUCTION:
        case HFS:
        case CAMPFIRE:
        case FIRE:
        case MAGMA:
        case POOL:
        case RIVER:
        case ROOT:      // TODO this is a bug; DF can dig roots.
        case TREE:
        case MUSHROOM:
        case UNDERWORLD_GATE:
            return false;
        case AIR:
            return true;
        default:
            break;
    }

    MaterialInfo mi;
    mi.decode(baseMaterialAt(pos));
    if (mi.material != nullptr && mi.material->flags.is_set(df::material_flags::UNDIGGABLE))
        return false;

    return true;
}

static bool dig_tile(color_ostream &out,
                     const df::coord pos, df::tile_dig_designation designation,
                     std::vector<dug_tile_info> &dug_tiles) {
    df::tiletype tt = *Maps::getTileType(pos);

    if (!is_diggable(pos, tt)) {
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
                    remove_ramp_top(df::coord(pos.x, pos.y, pos.z+1));
                target_type = get_target_type(tt, target_shape);
            }
            break;
        case df::tile_dig_designation::Channel:
        {
            df::coord pos_below(pos.x, pos.y, pos.z-1);
            if (can_dig_channel(tt) && Maps::ensureTileBlock(pos_below)
                    && is_diggable(pos_below, *Maps::getTileType(pos_below))) {
                TRACE(channels).print("dig_tile: channeling at ({}) [can_dig_channel: true]\n", pos_below);
                target_type = df::tiletype::OpenSpace;
                df::coord pos_above(pos.x, pos.y, pos.z+1);
                if (Maps::ensureTileBlock(pos_above))
                    remove_ramp_top(pos_above);
                df::tile_dig_designation td_below = (*Maps::getTileDesignation(pos_below)).bits.dig;
                if (dig_tile(out, pos_below, df::tile_dig_designation::Ramp, dug_tiles)) {
                    clean_ramps(pos_below);
                    if (td_below == df::tile_dig_designation::Default) {
                        dig_tile(out, pos_below, td_below, dug_tiles);
                    }
                    clean_ramps(pos);
                    propagate_vertical_flags(pos);
                    return true;
                }
            } else {
                DEBUG(channels).print("dig_tile: failed to channel at ({}) [can_dig_channel: false]\n", pos_below);
            }
            break;
        }
        case df::tile_dig_designation::UpStair:
            if (can_dig_up_stair(tt))
                target_type = get_target_type(tt, df::tiletype_shape::STAIR_UP);
            break;
        case df::tile_dig_designation::DownStair:
            if (can_dig_down_stair(tt))
                target_type = get_target_type(tt, df::tiletype_shape::STAIR_DOWN);
            break;
        case df::tile_dig_designation::UpDownStair:
            if (can_dig_up_down_stair(tt))
                target_type = get_target_type(tt, df::tiletype_shape::STAIR_UPDOWN);
            break;
        case df::tile_dig_designation::Ramp:
        {
            if (can_dig_ramp(tt)) {
                target_type = get_target_type(tt, df::tiletype_shape::RAMP);
                df::coord pos_above(pos.x, pos.y, pos.z+1);
                if (target_type != tt && Maps::ensureTileBlock(pos_above)
                        && is_diggable(pos, *Maps::getTileType(pos_above))) {
                    // only capture the tile info of pos_above if we didn't get
                    // here via the Channel case above
                    if (dug_tiles.size() == 0)
                        dug_tiles.push_back(dug_tile_info(pos_above));
                    destroy_colony(pos_above);
                    *Maps::getTileType(pos_above) = df::tiletype::RampTop;
                    remove_ramp_top(df::coord(pos.x, pos.y, pos.z+2));
                    propagate_vertical_flags(df::coord(pos.x, pos.y, pos.z + 1));
                }
            }
            break;
        }
        case df::tile_dig_designation::No:
        default:
            out.printerr(
                "unhandled dig designation for tile ({}, {}, {}): {}\n",
                pos.x, pos.y, pos.z, ENUM_AS_STR(designation));
    }

    if (target_type == df::tiletype::Void)
        DEBUG(general).print("dig_tile: target_type: tiletype is Void.  {} {}\n",
                pos, static_cast<size_t>(tt));
    if (tileMaterial(target_type) == df::tiletype_material::NONE)
        DEBUG(general).print("dig_tile: target_type: tiletype_material is NONE.  {} {}\n",
                pos, static_cast<size_t>(tt));

    // fail if unhandled or no change to tile
    if (target_type == df::tiletype::Void || target_type == tt)
        return false;

    dug_tiles.emplace_back(pos);
    TRACE(general).print("dig_tile: digging the designation tile at ({})\n",pos);
    dig_type(pos, target_type);

    clean_ramps(pos);
    return true;
}

static bool is_smooth_wall(const df::coord pos) {
    if (!Maps::ensureTileBlock(pos))
        return false;
    df::tiletype tt = *Maps::getTileType(pos);
    return tileSpecial(tt) == df::tiletype_special::SMOOTH
                && tileShape(tt) == df::tiletype_shape::WALL;
}

static bool is_connector(const df::coord pos) {
    df::building *bld = Buildings::findAtTile(pos);

    return bld &&
        (bld->getType() == df::building_type::Door ||
         bld->getType() == df::building_type::Floodgate);
}

static bool is_smooth_wall_or_connector(const df::coord pos) {
    return is_smooth_wall(pos) || is_connector(pos);
}

// adds adjacent smooth walls and doors to the given tdir
static TileDirection get_adjacent_smooth_walls(const df::coord pos,
                                               TileDirection tdir) {
    if (is_smooth_wall_or_connector(df::coord(pos.x, pos.y-1, pos.z)))
        tdir.north = 1;
    if (is_smooth_wall_or_connector(df::coord(pos.x, pos.y+1, pos.z)))
        tdir.south = 1;
    if (is_smooth_wall_or_connector(df::coord(pos.x-1, pos.y, pos.z)))
        tdir.west = 1;
    if (is_smooth_wall_or_connector(df::coord(pos.x+1, pos.y, pos.z)))
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
static bool adjust_smooth_wall_dir(const df::coord pos,
                                   TileDirection tdir = BLANK_TILE_DIRECTION) {
    if (!is_smooth_wall(pos))
        return is_connector(pos);

    tdir = ensure_valid_tdir(get_adjacent_smooth_walls(pos, tdir));

    df::tiletype tt = *Maps::getTileType(pos);
    tt = findTileType(tileShape(tt), tileMaterial(tt), tileVariant(tt),
                      tileSpecial(tt), tdir);
    if (tt == df::tiletype::Void)
        return false;

    *Maps::getTileType(pos) = tt;
    return true;
}

static void refresh_adjacent_smooth_walls(const df::coord pos) {
    adjust_smooth_wall_dir(df::coord(pos.x,   pos.y-1, pos.z));
    adjust_smooth_wall_dir(df::coord(pos.x,   pos.y+1, pos.z));
    adjust_smooth_wall_dir(df::coord(pos.x-1, pos.y,   pos.z));
    adjust_smooth_wall_dir(df::coord(pos.x+1, pos.y,   pos.z));
}

// assumes that if the game let you designate a tile for smoothing, it must be
// valid to do so.
static bool smooth_tile(color_ostream &out,
                        const df::coord pos) {
    df::tiletype tt = *Maps::getTileType(pos);

    df::tiletype_shape shape = tileShape(tt);
    df::tiletype_variant variant = tileVariant(tt);
    df::tiletype_special special = df::tiletype_special::SMOOTH;

    TileDirection tdir;
    if (is_smooth_wall(pos)) {
        // engraving is filtered out at a higher level, so this is a
        // fortification designation
        shape = tiletype_shape::FORTIFICATION;
        variant = df::tiletype_variant::NONE;
        special = df::tiletype_special::NONE;
    }
    else if (shape == df::tiletype_shape::WALL) {
        if (adjust_smooth_wall_dir(df::coord(pos.x, pos.y-1, pos.z),
                                   TileDirection(0, 1, 0, 0)))
            tdir.north = 1;
        if (adjust_smooth_wall_dir(df::coord(pos.x, pos.y+1, pos.z),
                                TileDirection(1, 0, 0, 0)))
            tdir.south = 1;
        if (adjust_smooth_wall_dir(df::coord(pos.x-1, pos.y, pos.z),
                                TileDirection(0, 0, 0, 1)))
            tdir.west = 1;
        if (adjust_smooth_wall_dir(df::coord(pos.x+1, pos.y, pos.z),
                                TileDirection(0, 0, 1, 0)))
            tdir.east = 1;
        tdir = ensure_valid_tdir(tdir);
    }

    tt = findTileType(shape, tileMaterial(tt), variant, special, tdir);
    if (tt == df::tiletype::Void)
        return false;

    *Maps::getTileType(pos) = tt;
    return true;
}

// assumes that if the game let you designate a tile for track carving, it must
// be valid to do so.
static bool carve_tile(const df::coord pos, df::tile_occupancy &to) {
    df::tiletype tt = *Maps::getTileType(pos);
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

    *Maps::getTileType(pos) = tt;
    return true;
}

static bool produces_item(const boulder_percent_options &options,
                          Random::MersenneRNG &rng,
                          const dug_tile_info &info) {
    uint32_t probability;
    if (info.tmat == df::tiletype_material::FEATURE)
        probability = options.deep;
    else {
        switch (veinTypeAt(info.pos)) {
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

typedef std::map<std::pair<df::item_type, t_matpair>, std::vector<df::coord>>
    item_coords_t;

static void do_dig(color_ostream &out, std::vector<df::coord> &dug_coords,
                   item_coords_t &item_coords, const dig_now_options &options) {
    Random::MersenneRNG rng;
    DesignationJobs jobs;

    DEBUG(general).print("do_dig(): starting..\n");
    jobs.load();
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

                df::coord pos(x, y, z);
                df::tile_designation td = *Maps::getTileDesignation(pos);
                df::tile_occupancy to = *Maps::getTileOccupancy(pos);
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

            if (dig_tile(out, pos, td.bits.dig, dug_tiles)) {
                for (auto info: dug_tiles) {
                    td = *Maps::getTileDesignation(info.pos);
                    td.bits.dig = df::tile_dig_designation::No;
                    *Maps::getTileDesignation(info.pos) = td;

                    dug_coords.push_back(info.pos);
                    refresh_adjacent_smooth_walls(info.pos);
                    if (info.imat < 0)
                        continue;
                    if (produces_item(options.boulder_percents, rng, info)) {
                        auto k = std::make_pair(info.itype, info.imat);
                        item_coords[k].push_back(info.pos);
                    }
                }
            }
        } else if (td.bits.smooth == 1) {
            if (smooth_tile(out, pos)) {
                td = *Maps::getTileDesignation(pos);
                td.bits.smooth = 0;
                *Maps::getTileDesignation(pos) = td;
            }
        } else if (to.bits.carve_track_north == 1
                   || to.bits.carve_track_east == 1
                   || to.bits.carve_track_south == 1
                   || to.bits.carve_track_west == 1) {
            if (carve_tile(pos, to)) {
                to = *Maps::getTileOccupancy(pos);
                to.bits.carve_track_north = 0;
                to.bits.carve_track_east = 0;
                to.bits.carve_track_south = 0;
                to.bits.carve_track_west = 0;
                *Maps::getTileOccupancy(pos) = to;
            }
        }
    }
}

// if pos is empty space, teleport to a floor somewhere below
// if we fall out of the world (e.g. empty space or walls all the way down),
// returned position will be invalid
static df::coord simulate_fall(const df::coord pos) {
    df::coord resting_pos(pos);

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

    df::coord dump_pos;
    if (Maps::isValidTilePos(options.dump_pos)) {
        dump_pos = simulate_fall(options.dump_pos);
        if (!Maps::ensureTileBlock(dump_pos))
            out.printerr("Invalid dump tile coordinates! Ensure the --dump"
                " option specifies an open, non-wall tile.");
    }

    for (auto entry : item_coords) {
        df::reaction_product_itemst *prod =
                df::allocate<df::reaction_product_itemst>();
        const std::vector<df::coord> &coords = entry.second;

        prod->item_type = entry.first.first;
        prod->item_subtype = -1;
        prod->mat_type = entry.first.second.mat_type;
        prod->mat_index = entry.first.second.mat_index;
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
            out.printerr("unexpected number of {} {} produced: expected {}, got {}.\n",
                         material.toString(), ENUM_KEY_STR(item_type, prod->item_type),
                         coords.size(), num_items);
            num_items = std::min(num_items, entry.second.size());
        }

        for (size_t i = 0; i < num_items; ++i) {
            df::coord pos = Maps::isValidTilePos(dump_pos) ?
                    dump_pos : simulate_fall(coords[i]);
            if (!Maps::ensureTileBlock(pos)) {
                out.printerr(
                        "unable to place boulder generated at ({}, {}, {})\n",
                        coords[i].x, coords[i].y, coords[i].z);
                continue;
            }
            out_items[i]->moveToGround(pos.x, pos.y, pos.z);
        }

        delete(prod);
    }
}

static bool needs_unhide(const df::coord pos) {
    return !Maps::ensureTileBlock(pos)
        || Maps::getTileDesignation(pos)->bits.hidden;
}

static bool needs_flood_unhide(const df::coord pos) {
    return needs_unhide(pos)
        || needs_unhide(df::coord(pos.x-1, pos.y-1, pos.z))
        || needs_unhide(df::coord(pos.x,   pos.y-1, pos.z))
        || needs_unhide(df::coord(pos.x+1, pos.y-1, pos.z))
        || needs_unhide(df::coord(pos.x-1, pos.y,   pos.z))
        || needs_unhide(df::coord(pos.x+1, pos.y,   pos.z))
        || needs_unhide(df::coord(pos.x-1, pos.y+1, pos.z))
        || needs_unhide(df::coord(pos.x,   pos.y+1, pos.z))
        || needs_unhide(df::coord(pos.x+1, pos.y+1, pos.z));
}

static void post_process_dug_tiles(color_ostream &out,
                             const std::vector<df::coord> &dug_coords) {
    for (df::coord pos : dug_coords) {
        if (needs_flood_unhide(pos)) {
            // set current tile to hidden to allow flood_unhide to work on tiles
            // that were already visible but that reveal hidden tiles when dug.
            Maps::getTileDesignation(pos)->bits.hidden = true;
            Lua::CallLuaModuleFunction(out, "plugins.reveal", "unhideFlood", std::make_tuple(pos));
        }

        df::tile_occupancy &to = *Maps::getTileOccupancy(pos);
        if (to.bits.unit || to.bits.item) {
            df::coord resting_pos = simulate_fall(pos);
            if (resting_pos == pos)
                continue;

            if (!Maps::ensureTileBlock(resting_pos)) {
                out.printerr("No valid tile beneath ({},{},{}) can't move"
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
                    for (auto item : items)
                        Items::moveToGround(item, resting_pos);
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
    auto L = DFHack::Core::getInstance().getLuaState();
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
    std::vector<df::coord> dug_coords;
    item_coords_t item_coords;

    do_dig(out, dug_coords, item_coords, options);
    create_boulders(out, item_coords, options, world->units.active[0]);
    post_process_dug_tiles(out, dug_coords);

    // force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    return true;
}

command_result dig_now(color_ostream &out, std::vector<std::string> &params) {
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
    df::coord pos;
    if (lua_gettop(L) <= 1)
        Lua::CheckDFAssign(L, &pos, 1);
    else
        pos = df::coord(luaL_checkint(L, 1), luaL_checkint(L, 2),
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
    df::coord pos;
    if (lua_gettop(L) <= 1)
        Lua::CheckDFAssign(L, &pos, 1);
    else
        pos = df::coord(luaL_checkint(L, 1), luaL_checkint(L, 2),
                      luaL_checkint(L, 3));

    adjust_smooth_wall_dir(pos);
    refresh_adjacent_smooth_walls(pos);
    return 0;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(dig_now_tile),
    DFHACK_LUA_COMMAND(link_adjacent_smooth_walls),
    DFHACK_LUA_END
};
