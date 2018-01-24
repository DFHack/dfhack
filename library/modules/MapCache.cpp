/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <iostream>
using namespace std;

#include "ColorText.h"
#include "Core.h"
#include "DataDefs.h"
#include "Error.h"
#include "MemAccess.h"
#include "MiscUtils.h"
#include "ModuleFactory.h"
#include "VersionInfo.h"

#include "modules/Buildings.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/block_square_event_designation_priorityst.h"
#include "df/block_square_event_frozen_liquidst.h"
#include "df/block_square_event_grassst.h"
#include "df/building_type.h"
#include "df/builtin_mats.h"
#include "df/burrow.h"
#include "df/feature_init.h"
#include "df/flow_info.h"
#include "df/plant.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#include "df/region_map_entry.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/world_region_details.h"
#include "df/world_underground_region.h"
#include "df/z_level_flags.h"

using namespace DFHack;
using namespace MapExtras;
using namespace df::enums;
using df::global::world;

extern bool GetLocalFeature(t_feature &feature, df::coord2d rgn_pos, int32_t index);

#ifdef LINUX_BUILD
const unsigned MapExtras::BiomeInfo::MAX_LAYERS;
#endif

const BiomeInfo MapCache::biome_stub = {
    df::coord2d(),
    -1, -1, -1, -1,
    NULL, NULL, NULL,
    { -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1 }
};

#define COPY(a,b) memcpy(&a,&b,sizeof(a))

MapExtras::Block::Block(MapCache *parent, DFCoord _bcoord) : parent(parent)
{
    dirty_designations = false;
    dirty_tiles = false;
    dirty_veins = false;
    dirty_temperatures = false;
    dirty_occupancies = false;
    valid = false;
    bcoord = _bcoord;
    block = Maps::getBlock(bcoord);
    tags = NULL;

    init();
}

void MapExtras::Block::init()
{
    item_counts = NULL;
    tiles = NULL;
    basemats = NULL;

    if(block)
    {
        COPY(designation, block->designation);
        COPY(occupancy, block->occupancy);

        COPY(temp1, block->temperature_1);
        COPY(temp2, block->temperature_2);

        valid = true;
    }
    else
    {
        memset(designation,0,sizeof(designation));
        memset(occupancy,0,sizeof(occupancy));
        memset(temp1,0,sizeof(temp1));
        memset(temp2,0,sizeof(temp2));
    }
}

bool MapExtras::Block::Allocate()
{
    if (block)
        return true;

    block = Maps::ensureTileBlock(bcoord.x*16, bcoord.y*16, bcoord.z);
    if (!block)
        return false;

    delete[] item_counts;
    delete tiles;
    delete basemats;
    init();

    return true;
}

MapExtras::Block::~Block()
{
    delete[] item_counts;
    delete[] tags;
    delete tiles;
    delete basemats;
}

void MapExtras::Block::init_tags()
{
    if (!tags)
        tags = new T_tags[16];
    memset(tags,0,sizeof(T_tags)*16);
}

void MapExtras::Block::init_tiles(bool basemat)
{
    if (!tiles)
    {
        tiles = new TileInfo();

        dirty_tiles = false;

        if (block)
            ParseTiles(tiles);
    }

    if (basemat && !basemats)
    {
        basemats = new BasematInfo();

        dirty_veins = false;

        if (block)
            ParseBasemats(tiles, basemats);
    }
}

MapExtras::Block::TileInfo::TileInfo()
{
    dirty_raw.clear();
    memset(raw_tiles,0,sizeof(raw_tiles));
    ice_info = NULL;
    con_info = NULL;
    memset(base_tiles,0,sizeof(base_tiles));
}

MapExtras::Block::TileInfo::~TileInfo()
{
    delete ice_info;
    delete con_info;
}

void MapExtras::Block::TileInfo::init_iceinfo()
{
    if (ice_info)
        return;

    ice_info = new IceInfo();
}

void MapExtras::Block::TileInfo::init_coninfo()
{
    if (con_info)
        return;

    con_info = new ConInfo();
    con_info->constructed.clear();
    COPY(con_info->tiles, base_tiles);
    memset(con_info->mat_type, -1, sizeof(con_info->mat_type));
    memset(con_info->mat_index, -1, sizeof(con_info->mat_index));
}

MapExtras::Block::BasematInfo::BasematInfo()
{
    vein_dirty.clear();
    memset(mat_type,0,sizeof(mat_type));
    memset(mat_index,-1,sizeof(mat_index));
    memset(veinmat,-1,sizeof(veinmat));
}

bool MapExtras::Block::setFlagAt(df::coord2d p, df::tile_designation::Mask mask, bool set)
{
    if(!valid) return false;
    auto &val = index_tile<df::tile_designation&>(designation,p);
    bool cur = (val.whole & mask) != 0;
    if (cur != set)
    {
        dirty_designations = true;
        val.whole = (set ? val.whole | mask : val.whole & ~mask);
    }
    return true;
}

bool MapExtras::Block::setFlagAt(df::coord2d p, df::tile_occupancy::Mask mask, bool set)
{
    if(!valid) return false;
    auto &val = index_tile<df::tile_occupancy&>(occupancy,p);
    bool cur = (val.whole & mask) != 0;
    if (cur != set)
    {
        dirty_occupancies = true;
        val.whole = (set ? val.whole | mask : val.whole & ~mask);
    }
    return true;
}

bool MapExtras::Block::setTiletypeAt(df::coord2d pos, df::tiletype tt, bool force)
{
    if (!block)
        return false;

    if (!basemats)
        init_tiles(true);

    pos = pos & 15;

    dirty_tiles = true;
    tiles->raw_tiles[pos.x][pos.y] = tt;
    tiles->dirty_raw.setassignment(pos, true);

    return true;
}

static df::block_square_event_designation_priorityst *getPriorityEvent(df::map_block *block, bool write)
{
    vector<df::block_square_event_designation_priorityst*> events;
    Maps::SortBlockEvents(block, 0, 0, 0, 0, 0, 0, 0, &events);
    if (events.empty())
    {
        if (!write)
            return NULL;

        auto event = df::allocate<df::block_square_event_designation_priorityst>();
        block->block_events.push_back((df::block_square_event*)event);
        return event;
    }
    return events[0];
}

int32_t MapExtras::Block::priorityAt(df::coord2d pos)
{
    if (!block)
        return false;

    if (auto event = getPriorityEvent(block, false))
        return event->priority[pos.x % 16][pos.y % 16];

    return 0;
}

bool MapExtras::Block::setPriorityAt(df::coord2d pos, int32_t priority)
{
    if (!block || priority < 0)
        return false;

    auto event = getPriorityEvent(block, true);
    event->priority[pos.x % 16][pos.y % 16] = priority;
    return true;
}

bool MapExtras::Block::setVeinMaterialAt(df::coord2d pos, int16_t mat, df::inclusion_type type)
{
    using namespace df::enums::tiletype_material;

    if (!block)
        return false;

    if (!basemats)
        init_tiles(true);

    pos = pos & 15;
    auto &cur_mat = basemats->veinmat[pos.x][pos.y];
    auto &cur_type = basemats->veintype[pos.x][pos.y];

    if (cur_mat == mat && (mat < 0 || cur_type == type))
        return true;

    if (mat >= 0)
    {
        // Cannot allocate veins?
        if (!df::block_square_event_mineralst::_identity.can_instantiate())
            return false;

        // Bad material?
        if (!isStoneInorganic(mat))
            return false;
    }

    dirty_veins = true;
    cur_mat = mat;
    cur_type = (uint8_t)type;
    basemats->vein_dirty.setassignment(pos, true);

    if (tileMaterial(tiles->base_tiles[pos.x][pos.y]) == MINERAL)
        basemats->set_base_mat(tiles, pos, 0, mat);

    return true;
}

bool MapExtras::Block::setStoneAt(df::coord2d pos, df::tiletype tile, int16_t mat, df::inclusion_type type, bool force_vein, bool kill_veins)
{
    using namespace df::enums::tiletype_material;

    if (!block)
        return false;

    if (!isStoneInorganic(mat) || !isCoreMaterial(tile))
        return false;

    if (!basemats)
        init_tiles(true);

    // Check if anything needs to be done
    pos = pos & 15;
    auto &cur_tile = tiles->base_tiles[pos.x][pos.y];
    auto &cur_mattype = basemats->mat_type[pos.x][pos.y];
    auto &cur_matidx = basemats->mat_index[pos.x][pos.y];

    if (!force_vein && cur_tile == tile && cur_mattype == 0 && cur_matidx == mat)
        return true;

    bool vein = false;

    if (force_vein && type != inclusion_type::CLUSTER)
        vein = true;
    else if (mat == lavaStoneAt(pos))
        tile = matchTileMaterial(tile, LAVA_STONE);
    else if (mat == layerMaterialAt(pos))
        tile = matchTileMaterial(tile, STONE);
    else
        vein = true;

    if (vein)
        tile = matchTileMaterial(tile, MINERAL);

    if (tile == tiletype::Void)
        return false;
    if ((vein || kill_veins) && !setVeinMaterialAt(pos, vein ? mat : -1, type))
        return false;

    if (cur_tile != tile)
    {
        dirty_tiles = true;
        tiles->set_base_tile(pos, tile);
    }

    basemats->set_base_mat(tiles, pos, 0, mat);

    return true;
}

bool MapExtras::Block::setSoilAt(df::coord2d pos, df::tiletype tile, bool kill_veins)
{
    using namespace df::enums::tiletype_material;

    if (!block)
        return false;

    if (!isCoreMaterial(tile))
        return false;

    if (!basemats)
        init_tiles(true);

    pos = pos & 15;
    auto &cur_tile = tiles->base_tiles[pos.x][pos.y];

    tile = matchTileMaterial(tile, SOIL);
    if (tile == tiletype::Void)
        return false;

    if (kill_veins && !setVeinMaterialAt(pos, -1))
        return false;

    if (cur_tile != tile)
    {
        dirty_tiles = true;
        tiles->set_base_tile(pos, tile);
    }

    int mat = layerMaterialAt(pos);
    if (BlockInfo::getGroundType(mat) == BlockInfo::G_STONE)
        mat = biomeInfoAt(pos).default_soil;

    basemats->set_base_mat(tiles, pos, 0, mat);

    return true;
}

void MapExtras::Block::ParseTiles(TileInfo *tiles)
{
    tiletypes40d icetiles;
    BlockInfo::SquashFrozenLiquids(block, icetiles);

    COPY(tiles->raw_tiles, block->tiletype);

    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            using namespace df::enums::tiletype_material;

            df::tiletype tt = tiles->raw_tiles[x][y];
            df::coord coord = block->map_pos + df::coord(x,y,0);
            bool had_ice = false;

            // Frozen liquid comes topmost
            if (tileMaterial(tt) == FROZEN_LIQUID)
            {
                had_ice = true;
                tiles->init_iceinfo();

                tiles->ice_info->frozen.setassignment(x,y,true);
                if (icetiles[x][y] != tiletype::Void)
                {
                    tt = icetiles[x][y];
                }
            }

            // The next layer may be construction
            bool is_con = false;

            if (tileMaterial(tt) == CONSTRUCTION)
            {
                df::construction *con = df::construction::find(coord);
                if (con)
                {
                    if (!tiles->con_info)
                        tiles->init_coninfo();

                    is_con = true;
                    tiles->con_info->constructed.setassignment(x,y,true);
                    tiles->con_info->tiles[x][y] = tt;
                    tiles->con_info->mat_type[x][y] = con->mat_type;
                    tiles->con_info->mat_index[x][y] = con->mat_index;

                    tt = con->original_tile;

                    // Ice under construction is buggy:
                    // http://www.bay12games.com/dwarves/mantisbt/view.php?id=6330
                    // Therefore we just pretend it wasn't there (if it isn't too late),
                    // and overwrite it if/when we write the base layer.
                    if (!had_ice && tileMaterial(tt) == FROZEN_LIQUID)
                    {
                        if (icetiles[x][y] != tiletype::Void)
                            tt = icetiles[x][y];
                    }
                }
            }

            // Finally, base material
            tiles->base_tiles[x][y] = tt;

            // Copy base info back to construction layer
            if (tiles->con_info && !is_con)
                tiles->con_info->tiles[x][y] = tt;
        }
    }
}

void MapExtras::Block::TileInfo::set_base_tile(df::coord2d pos, df::tiletype tile)
{
    base_tiles[pos.x][pos.y] = tile;

    if (con_info)
    {
        if (con_info->constructed.getassignment(pos))
        {
            con_info->dirty.setassignment(pos, true);
            return;
        }

        con_info->tiles[pos.x][pos.y] = tile;
    }

    if (ice_info && ice_info->frozen.getassignment(pos))
    {
        ice_info->dirty.setassignment(pos, true);
        return;
    }

    dirty_raw.setassignment(pos, true);
    raw_tiles[pos.x][pos.y] = tile;
}

void MapExtras::Block::WriteTiles(TileInfo *tiles)
{
    if (tiles->con_info)
    {
        for (int y = 0; y < 16; y++)
        {
            if (!tiles->con_info->dirty[y])
                continue;

            for (int x = 0; x < 16; x++)
            {
                if (!tiles->con_info->dirty.getassignment(x,y))
                    continue;

                df::coord coord = block->map_pos + df::coord(x,y,0);
                df::construction *con = df::construction::find(coord);
                if (con)
                    con->original_tile = tiles->base_tiles[x][y];
            }
        }

        tiles->con_info->dirty.clear();
    }

    if (tiles->ice_info && tiles->ice_info->dirty.has_assignments())
    {
        df::tiletype (*newtiles)[16] = (tiles->con_info ? tiles->con_info->tiles : tiles->base_tiles);

        for (int i = block->block_events.size()-1; i >= 0; i--)
        {
            auto event = block->block_events[i];
            auto ice = strict_virtual_cast<df::block_square_event_frozen_liquidst>(event);
            if (!ice)
                continue;

            for (int y = 0; y < 16; y++)
            {
                if (!tiles->ice_info->dirty[y])
                    continue;

                for (int x = 0; x < 16; x++)
                {
                    if (!tiles->ice_info->dirty.getassignment(x,y))
                        continue;
                    if (ice->tiles[x][y] == tiletype::Void)
                        continue;

                    ice->tiles[x][y] = newtiles[x][y];
                }
            }
        }

        tiles->ice_info->dirty.clear();
    }

    for (int y = 0; y < 16; y++)
    {
        if (!tiles->dirty_raw[y])
            continue;

        for (int x = 0; x < 16; x++)
        {
            if (tiles->dirty_raw.getassignment(x,y))
                block->tiletype[x][y] = tiles->raw_tiles[x][y];
        }
    }
}

void MapExtras::Block::ParseBasemats(TileInfo *tiles, BasematInfo *bmats)
{
    BlockInfo info;

    info.prepare(this);

    COPY(bmats->veinmat, info.veinmats);
    COPY(bmats->veintype, info.veintype);

    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            using namespace df::enums::tiletype_material;

            auto tt = tiles->base_tiles[x][y];
            auto mat = info.getBaseMaterial(tt, df::coord2d(x,y));

            bmats->set_base_mat(tiles, df::coord2d(x,y), mat.mat_type, mat.mat_index);
        }
    }
}

void MapExtras::Block::BasematInfo::set_base_mat(TileInfo *tiles, df::coord2d pos, int16_t type, int16_t idx)
{
    mat_type[pos.x][pos.y] = type;
    mat_index[pos.x][pos.y] = idx;

    // Copy base info back to construction layer
    if (tiles->con_info && !tiles->con_info->constructed.getassignment(pos))
    {
        tiles->con_info->mat_type[pos.x][pos.y] = type;
        tiles->con_info->mat_index[pos.x][pos.y] = idx;
    }
}

void MapExtras::Block::WriteVeins(TileInfo *tiles, BasematInfo *bmats)
{
    // Classify modified tiles into distinct buckets
    typedef std::pair<int, df::inclusion_type> t_vein_key;
    std::map<t_vein_key, df::tile_bitmask> added;
    std::set<t_vein_key> discovered;

    for (int y = 0; y < 16; y++)
    {
        if (!bmats->vein_dirty[y])
            continue;

        for (int x = 0; x < 16; x++)
        {
            using namespace df::enums::tiletype_material;

            if (!bmats->vein_dirty.getassignment(x,y))
                continue;

            int matidx = bmats->veinmat[x][y];
            if (matidx >= 0)
            {
                t_vein_key key(matidx, (df::inclusion_type)bmats->veintype[x][y]);

                added[key].setassignment(x,y,true);
                if (!designation[x][y].bits.hidden)
                    discovered.insert(key);
            }
        }
    }

    // Adjust existing veins
    for (int i = block->block_events.size()-1; i >= 0; i--)
    {
        auto event = block->block_events[i];
        auto vein = strict_virtual_cast<df::block_square_event_mineralst>(event);
        if (!vein)
            continue;

        // First clear all dirty tiles
        vein->tile_bitmask -= bmats->vein_dirty;

        // Then add new if there are any matching ones
        t_vein_key key(vein->inorganic_mat, BlockInfo::getVeinType(vein->flags));

        if (added.count(key))
        {
            vein->tile_bitmask |= added[key];
            if (discovered.count(key))
                vein->flags.bits.discovered = true;

            added.erase(key);
            discovered.erase(key);
        }

        // Delete if became empty
        if (!vein->tile_bitmask.has_assignments())
        {
            vector_erase_at(block->block_events, i);
            delete vein;
        }
    }

    // Finally add new vein objects if there are new unmatched
    for (auto it = added.begin(); it != added.end(); ++it)
    {
        auto vein = df::allocate<df::block_square_event_mineralst>();
        if (!vein)
            break;

        block->block_events.push_back(vein);

        vein->inorganic_mat = it->first.first;
        vein->tile_bitmask = it->second;
        vein->flags.bits.discovered = discovered.count(it->first)>0;
        BlockInfo::setVeinType(vein->flags, it->first.second);
    }

    bmats->vein_dirty.clear();
}

bool MapExtras::Block::isDirty()
{
    return valid && (
        dirty_designations ||
        dirty_tiles ||
        dirty_veins ||
        dirty_temperatures ||
        dirty_occupancies
    );
}

bool MapExtras::Block::Write ()
{
    if(!valid) return false;

    if(dirty_designations)
    {
        COPY(block->designation, designation);
        block->flags.bits.designated = true;
        dirty_designations = false;
    }
    if(dirty_tiles || dirty_veins)
    {
        if (tiles && dirty_tiles)
            WriteTiles(tiles);
        if (basemats && dirty_veins)
            WriteVeins(tiles, basemats);

        dirty_tiles = dirty_veins = false;

        delete tiles; tiles = NULL;
        delete basemats; basemats = NULL;
    }
    if(dirty_temperatures)
    {
        COPY(block->temperature_1, temp1);
        COPY(block->temperature_2, temp2);
        dirty_temperatures = false;
    }
    if(dirty_occupancies)
    {
        COPY(block->occupancy, occupancy);
        dirty_occupancies = false;
    }
    return true;
}

void MapExtras::BlockInfo::prepare(Block *mblock)
{
    this->mblock = mblock;

    block = mblock->getRaw();
    parent = mblock->getParent();
    column = Maps::getBlockColumn((block->map_pos.x / 48) * 3, (block->map_pos.y / 48) * 3);

    SquashVeins(block, veinmats, veintype);
    SquashGrass(block, grass);

    for (size_t i = 0; i < column->plants.size(); i++)
    {
        auto pp = column->plants[i];
        // A plant without tree_info is single tile
        // TODO: verify that x any y lie inside the block.
        if (!pp->tree_info)
        {
            if (pp->pos.z == block->map_pos.z)
                plants[pp->pos] = pp;
            continue;
        }

        // tree_info contains vertical slices of the tree. This ensures there's a slice for our Z-level.
        df::plant_tree_info * info = pp->tree_info;
        if (!((pp->pos.z - info->roots_depth <= block->map_pos.z) && ((pp->pos.z + info->body_height) > block->map_pos.z)))
            continue;

        // Parse through a single horizontal slice of the tree.
        for (int xx = 0; xx < info->dim_x; xx++)
        for (int yy = 0; yy < info->dim_y; yy++)
        {
            // Any non-zero value here other than blocked means there's some sort of branch here.
            // If the block is at or above the plant's base level, we use the body array
            // otherwise we use the roots.
            // TODO: verify that the tree bounds intersect the block.
            df::plant_tree_tile tile;
            int z_diff = block->map_pos.z - pp->pos.z;
            if (z_diff >= 0)
                tile = info->body[z_diff][xx + (yy*info->dim_x)];
            else
                tile = info->roots[-1 - z_diff][xx + (yy*info->dim_x)];
            if (tile.whole && !(tile.bits.blocked))
            {
                df::coord pos = pp->pos;
                pos.x = pos.x - (info->dim_x / 2) + xx;
                pos.y = pos.y - (info->dim_y / 2) + yy;
                pos.z = block->map_pos.z;
                plants[pos] = pp;
            }
        }
    }

    global_feature = Maps::getGlobalInitFeature(block->global_feature);
    local_feature = Maps::getLocalInitFeature(block->region_pos, block->local_feature);
}

BlockInfo::GroundType MapExtras::BlockInfo::getGroundType(int material)
{
    auto raw = df::inorganic_raw::find(material);
    if (!raw)
        return G_UNKNOWN;

    if (raw->flags.is_set(inorganic_flags::SOIL_ANY))
        return G_SOIL;

    return G_STONE;
}

t_matpair MapExtras::BlockInfo::getBaseMaterial(df::tiletype tt, df::coord2d pos)
{
    using namespace df::enums::tiletype_material;

    t_matpair rv(0,-1);
    int x = pos.x, y = pos.y;

    switch (tileMaterial(tt)) {
    case NONE:
    case AIR:
        rv.mat_type = -1;
        break;

    case DRIFTWOOD:
    case SOIL:
    {
        auto &biome = mblock->biomeInfoAt(pos);
        rv.mat_index = biome.layer_stone[mblock->layerIndexAt(pos)];

        if (getGroundType(rv.mat_index) == G_STONE)
        {
            int idx = biome.default_soil;
            if (idx >= 0)
                rv.mat_index = idx;
        }

        break;
    }

    case STONE:
    {
        auto &biome = mblock->biomeInfoAt(pos);
        rv.mat_index = biome.layer_stone[mblock->layerIndexAt(pos)];

        if (getGroundType(rv.mat_index) == G_SOIL)
        {
            int idx = biome.default_stone;
            if (idx >= 0)
                rv.mat_index = idx;
        }

        break;
    }

    case MINERAL:
        rv.mat_index = veinmats[x][y];
        break;

    case LAVA_STONE:
        rv.mat_index = mblock->biomeInfoAt(pos).lava_stone;
        break;

    case MUSHROOM:
    case ROOT:
    case TREE:
    case PLANT:
        rv.mat_type = MaterialInfo::PLANT_BASE;
        if (auto plant = plants[block->map_pos + df::coord(x,y,0)])
        {
            if (auto raw = df::plant_raw::find(plant->material))
            {
                rv.mat_type = raw->material_defs.type_basic_mat;
                rv.mat_index = raw->material_defs.idx_basic_mat;
            }
        }
        break;

    case GRASS_LIGHT:
    case GRASS_DARK:
    case GRASS_DRY:
    case GRASS_DEAD:
        rv.mat_type = MaterialInfo::PLANT_BASE;
        if (auto raw = df::plant_raw::find(grass[x][y]))
        {
            rv.mat_type = raw->material_defs.type_basic_mat;
            rv.mat_index = raw->material_defs.idx_basic_mat;
        }
        break;

    case FEATURE:
    {
        auto dsgn = block->designation[x][y];

        if (dsgn.bits.feature_local && local_feature)
            local_feature->getMaterial(&rv.mat_type, &rv.mat_index);
        else if (dsgn.bits.feature_global && global_feature)
            global_feature->getMaterial(&rv.mat_type, &rv.mat_index);

        break;
    }

    case CONSTRUCTION: // just a fallback
    case MAGMA:
    case HFS:
        // use generic 'rock'
        break;

    case FROZEN_LIQUID:
        rv.mat_type = builtin_mats::WATER;
        break;

    case POOL:
    case BROOK:
    case RIVER:
        rv.mat_index = mblock->layerMaterialAt(pos);
        break;

    case ASHES:
    case FIRE:
    case CAMPFIRE:
        rv.mat_type = builtin_mats::ASH;
        break;
    }

    return rv;
}

df::inclusion_type MapExtras::BlockInfo::getVeinType(DFVeinFlags &flags)
{
    using namespace df::enums::inclusion_type;

    if (flags.bits.cluster_small)
        return CLUSTER_SMALL;
    if (flags.bits.cluster_one)
        return CLUSTER_ONE;
    if (flags.bits.vein)
        return VEIN;
    if (flags.bits.cluster)
        return CLUSTER;

    return df::inclusion_type(0);
}

void MapExtras::BlockInfo::setVeinType(DFVeinFlags &info, df::inclusion_type type)
{
    using namespace df::enums::inclusion_type;

    info.bits.cluster = info.bits.vein = info.bits.cluster_small = info.bits.cluster_one = false;

    switch (type) {
        case VEIN:          info.bits.vein = true; break;
        case CLUSTER:       info.bits.cluster = true; break;
        case CLUSTER_SMALL: info.bits.cluster_small = true; break;
        case CLUSTER_ONE:   info.bits.cluster_one = true; break;
        default: break;
    }
}

void MapExtras::BlockInfo::SquashVeins(df::map_block *mb, t_blockmaterials & materials, t_veintype &veintype)
{
    std::vector <df::block_square_event_mineralst *> veins;
    Maps::SortBlockEvents(mb,&veins);
    memset(materials,-1,sizeof(materials));
    memset(veintype, 0, sizeof(t_veintype));

    for (uint32_t x = 0;x<16;x++) for (uint32_t y = 0; y< 16;y++)
    {
        for (size_t i = 0; i < veins.size(); i++)
        {
            if (veins[i]->getassignment(x,y))
            {
                materials[x][y] = veins[i]->inorganic_mat;
                veintype[x][y] = (uint8_t)getVeinType(veins[i]->flags);
            }
        }
    }
}

void MapExtras::BlockInfo::SquashFrozenLiquids(df::map_block *mb, tiletypes40d & frozen)
{
    std::vector <df::block_square_event_frozen_liquidst *> ices;
    Maps::SortBlockEvents(mb,NULL,&ices);
    memset(frozen,0,sizeof(frozen));
    for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
    {
        for (size_t i = 0; i < ices.size(); i++)
        {
            df::tiletype tt2 = ices[i]->tiles[x][y];
            if (tt2 != tiletype::Void)
            {
                frozen[x][y] = tt2;
                break;
            }
        }
    }
}

void MapExtras::BlockInfo::SquashRocks (df::map_block *mb, t_blockmaterials & materials,
                                   std::vector< std::vector <int16_t> > * layerassign)
{
    // get the layer materials
    for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
    {
        materials[x][y] = -1;
        uint8_t test = mb->designation[x][y].bits.biome;
        if (test >= 9)
            continue;
        uint8_t idx = mb->region_offset[test];
        if (idx < layerassign->size())
            materials[x][y] = layerassign->at(idx)[mb->designation[x][y].bits.geolayer_index];
    }
}

void MapExtras::BlockInfo::SquashGrass(df::map_block *mb, t_blockmaterials &materials)
{
    std::vector<df::block_square_event_grassst*> grasses;
    Maps::SortBlockEvents(mb, NULL, NULL, NULL, &grasses);
    memset(materials,-1,sizeof(materials));
    for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
    {
        int amount = 0;
        for (size_t i = 0; i < grasses.size(); i++)
        {
            if (grasses[i]->amount[x][y] >= amount)
            {
                amount = grasses[i]->amount[x][y];
                materials[x][y] = grasses[i]->plant_index;
            }
        }
    }
}

int MapExtras::Block::biomeIndexAt(df::coord2d p)
{
    if (!block)
        return -1;

    auto des = index_tile<df::tile_designation>(designation,p);
    uint8_t idx = des.bits.biome;
    if (idx >= 9)
        return -1;
    idx = block->region_offset[idx];
    if (idx >= parent->biomes.size())
        return -1;
    return idx;
}

const BiomeInfo &Block::biomeInfoAt(df::coord2d p)
{
    return parent->getBiomeByIndex(biomeIndexAt(p));
}

df::coord2d MapExtras::Block::biomeRegionAt(df::coord2d p)
{
    if (!block)
        return df::coord2d(-30000,-30000);

    int idx = biomeIndexAt(p);
    if (idx < 0)
        return block->region_pos;

    return parent->biomes[idx].pos;
}

bool MapExtras::Block::GetGlobalFeature(t_feature *out)
{
    out->type = (df::feature_type)-1;
    if (!valid || block->global_feature < 0)
        return false;
    return Maps::GetGlobalFeature(*out, block->global_feature);
}

bool MapExtras::Block::GetLocalFeature(t_feature *out)
{
    out->type = (df::feature_type)-1;
    if (!valid || block->local_feature < 0)
        return false;
    return ::GetLocalFeature(*out, block->region_pos, block->local_feature);
}

void MapExtras::Block::init_item_counts()
{
    if (item_counts) return;

    item_counts = new T_item_counts[16];
    memset(item_counts, 0, sizeof(T_item_counts)*16);

    if (!block) return;

    for (size_t i = 0; i < block->items.size(); i++)
    {
        auto it = df::item::find(block->items[i]);
        if (!it || !it->flags.bits.on_ground)
            continue;

        df::coord tidx = it->pos - block->map_pos;
        if (!is_valid_tile_coord(tidx) || tidx.z != 0)
            continue;

        item_counts[tidx.x][tidx.y]++;
    }
}

bool MapExtras::Block::addItemOnGround(df::item *item)
{
    if (!block)
        return false;

    init_item_counts();

    bool inserted;
    insert_into_vector(block->items, item->id, &inserted);

    if (inserted)
    {
        int &count = index_tile<int&>(item_counts,item->pos);

        if (count++ == 0)
        {
            index_tile<df::tile_occupancy&>(occupancy,item->pos).bits.item = true;
            index_tile<df::tile_occupancy&>(block->occupancy,item->pos).bits.item = true;
        }
    }

    return inserted;
}

bool MapExtras::Block::removeItemOnGround(df::item *item)
{
    if (!block)
        return false;

    init_item_counts();

    int idx = binsearch_index(block->items, item->id);
    if (idx < 0)
        return false;

    vector_erase_at(block->items, idx);

    int &count = index_tile<int&>(item_counts,item->pos);

    if (--count == 0)
    {
        index_tile<df::tile_occupancy&>(occupancy,item->pos).bits.item = false;

        auto &occ = index_tile<df::tile_occupancy&>(block->occupancy,item->pos);

        occ.bits.item = false;

        // Clear the 'site blocked' flag in the building, if any.
        // Otherwise the job would be re-suspended without actually checking items.
        if (occ.bits.building == tile_building_occ::Planned)
        {
            if (auto bld = Buildings::findAtTile(item->pos))
            {
                // TODO: maybe recheck other tiles like the game does.
                bld->flags.bits.site_blocked = false;
            }
        }
    }

    return true;
}

MapExtras::MapCache::MapCache()
{
    valid = 0;
    Maps::getSize(x_bmax, y_bmax, z_max);
    x_tmax = x_bmax*16; y_tmax = y_bmax*16;
    std::vector<df::coord2d> geoidx;
    std::vector<std::vector<int16_t> > layer_mats;
    validgeo = Maps::ReadGeology(&layer_mats, &geoidx);
    valid = true;

    if (auto data = df::global::world->world_data)
    {
        for (size_t i = 0; i < data->region_details.size(); i++)
        {
            auto info = data->region_details[i];
            region_details[info->pos] = info;
        }
    }

    biomes.resize(layer_mats.size());

    for (size_t i = 0; i < layer_mats.size(); i++)
    {
        biomes[i].pos = geoidx[i];
        biomes[i].biome = Maps::getRegionBiome(geoidx[i]);
        biomes[i].details = region_details[geoidx[i]];

        biomes[i].geo_index = biomes[i].biome ? biomes[i].biome->geo_index : -1;
        biomes[i].geobiome = df::world_geo_biome::find(biomes[i].geo_index);

        biomes[i].lava_stone = -1;
        biomes[i].default_soil = -1;
        biomes[i].default_stone = -1;

        if (biomes[i].details)
            biomes[i].lava_stone = biomes[i].details->lava_stone;

        memset(biomes[i].layer_stone, -1, sizeof(biomes[i].layer_stone));

        for (size_t j = 0; j < std::min<size_t>(BiomeInfo::MAX_LAYERS,layer_mats[i].size()); j++)
        {
            biomes[i].layer_stone[j] = layer_mats[i][j];

            auto raw = df::inorganic_raw::find(layer_mats[i][j]);
            if (!raw)
                continue;

            bool is_soil = raw->flags.is_set(inorganic_flags::SOIL_ANY);
            if (is_soil)
                biomes[i].default_soil = layer_mats[i][j];
            else if (biomes[i].default_stone == -1)
                biomes[i].default_stone = layer_mats[i][j];
        }

        while (layer_mats[i].size() < 16)
            layer_mats[i].push_back(-1);
    }
}

MapExtras::Block *MapExtras::MapCache::BlockAt(DFCoord blockcoord)
{
    if(!valid)
        return 0;
    std::map <DFCoord, Block*>::iterator iter = blocks.find(blockcoord);
    if(iter != blocks.end())
    {
        return (*iter).second;
    }
    else
    {
        if(unsigned(blockcoord.x) < x_bmax &&
           unsigned(blockcoord.y) < y_bmax &&
           unsigned(blockcoord.z) < z_max)
        {
            Block * nblo = new Block(this, blockcoord);
            blocks[blockcoord] = nblo;
            return nblo;
        }
        return 0;
    }
}

void MapExtras::MapCache::discardBlock(Block *block)
{
    blocks.erase(block->bcoord);
    delete block;
}

void MapExtras::MapCache::resetTags()
{
    for (auto it = blocks.begin(); it != blocks.end(); ++it)
    {
        delete[] it->second->tags;
        it->second->tags = NULL;
    }
}
