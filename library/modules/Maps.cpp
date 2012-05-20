/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
using namespace std;

#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "Error.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "MiscUtils.h"

#include "modules/Buildings.h"

#include "DataDefs.h"
#include "df/world_data.h"
#include "df/world_underground_region.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/feature_init.h"
#include "df/world_data.h"
#include "df/burrow.h"
#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/world_region_details.h"
#include "df/builtin_mats.h"
#include "df/block_square_event_grassst.h"
#include "df/z_level_flags.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;

const char * DFHack::sa_feature(df::feature_type index)
{
    switch(index)
    {
    case feature_type::outdoor_river:
        return "River";
    case feature_type::cave:
        return "Cave";
    case feature_type::pit:
        return "Pit";
    case feature_type::magma_pool:
        return "Magma pool";
    case feature_type::volcano:
        return "Volcano";
    case feature_type::deep_special_tube:
        return "Adamantine deposit";
    case feature_type::deep_surface_portal:
        return "Underworld portal";
    case feature_type::subterranean_from_layer:
        return "Cavern";
    case feature_type::magma_core_from_layer:
        return "Magma sea";
    case feature_type::feature_underworld_from_layer:
        return "Underworld";
    default:
        return "Unknown/Error";
    }
};

bool Maps::IsValid ()
{
    return (world->map.block_index != NULL);
}

// getter for map size
void Maps::getSize (uint32_t& x, uint32_t& y, uint32_t& z)
{
    if (!IsValid())
    {
        x = y = z = 0;
        return;
    }
    x = world->map.x_count_block;
    y = world->map.y_count_block;
    z = world->map.z_count_block;
}

// getter for map position
void Maps::getPosition (int32_t& x, int32_t& y, int32_t& z)
{
    if (!IsValid())
    {
        x = y = z = 0;
        return;
    }
    x = world->map.region_x;
    y = world->map.region_y;
    z = world->map.region_z;
}

/*
 * Block reading
 */

df::map_block *Maps::getBlock (int32_t blockx, int32_t blocky, int32_t blockz)
{
    if (!IsValid())
        return NULL;
    if ((blockx < 0) || (blocky < 0) || (blockz < 0))
        return NULL;
    if ((blockx >= world->map.x_count_block) || (blocky >= world->map.y_count_block) || (blockz >= world->map.z_count_block))
        return NULL;
    return world->map.block_index[blockx][blocky][blockz];
}

df::map_block *Maps::getTileBlock (int32_t x, int32_t y, int32_t z)
{
    if (!IsValid())
        return NULL;
    if ((x < 0) || (y < 0) || (z < 0))
        return NULL;
    if ((x >= world->map.x_count) || (y >= world->map.y_count) || (z >= world->map.z_count))
        return NULL;
    return world->map.block_index[x >> 4][y >> 4][z];
}

df::tiletype *Maps::getTileType(int32_t x, int32_t y, int32_t z)
{
    df::map_block *block = getTileBlock(x,y,z);
    return block ? &block->tiletype[x&15][y&15] : NULL;
}

df::tile_designation *Maps::getTileDesignation(int32_t x, int32_t y, int32_t z)
{
    df::map_block *block = getTileBlock(x,y,z);
    return block ? &block->designation[x&15][y&15] : NULL;
}

df::tile_occupancy *Maps::getTileOccupancy(int32_t x, int32_t y, int32_t z)
{
    df::map_block *block = getTileBlock(x,y,z);
    return block ? &block->occupancy[x&15][y&15] : NULL;
}

df::world_data::T_region_map *Maps::getRegionBiome(df::coord2d rgn_pos)
{
    auto data = world->world_data;
    if (!data)
        return NULL;

    if (rgn_pos.x < 0 || rgn_pos.x >= data->world_width ||
        rgn_pos.y < 0 || rgn_pos.y >= data->world_height)
        return NULL;

    return &data->region_map[rgn_pos.x][rgn_pos.y];
}

void Maps::enableBlockUpdates(df::map_block *blk, bool flow, bool temperature)
{
    if (!blk || !(flow || temperature)) return;

    if (temperature)
        blk->flags.bits.update_temperature = true;

    if (flow)
    {
        blk->flags.bits.update_liquid = true;
        blk->flags.bits.update_liquid_twice = true;
    }

    auto z_flags = world->map.z_level_flags;
    int z_level = blk->map_pos.z;

    if (z_flags && z_level >= 0 && z_level < world->map.z_count_block)
    {
        z_flags += z_level;
        z_flags->bits.update = true;
        z_flags->bits.update_twice = true;
    }
}

df::feature_init *Maps::getGlobalInitFeature(int32_t index)
{
    auto data = world->world_data;
    if (!data || index < 0)
        return NULL;

    auto rgn = vector_get(data->underground_regions, index);
    if (!rgn)
        return NULL;

    return rgn->feature_init;
}

bool Maps::GetGlobalFeature(t_feature &feature, int32_t index)
{
    feature.type = (df::feature_type)-1;

    auto f = Maps::getGlobalInitFeature(index);
    if (!f)
        return false;

    feature.discovered = false;
    feature.origin = f;
    feature.type = f->getType();
    f->getMaterial(&feature.main_material, &feature.sub_material);
    return true;
}

df::feature_init *Maps::getLocalInitFeature(df::coord2d rgn_pos, int32_t index)
{
    auto data = world->world_data;
    if (!data || index < 0)
        return NULL;

    if (rgn_pos.x < 0 || rgn_pos.x >= data->world_width ||
        rgn_pos.y < 0 || rgn_pos.y >= data->world_height)
        return NULL;

    // megaregions = 16x16 squares of regions = 256x256 squares of embark squares
    df::coord2d bigregion = rgn_pos / 16;

    // bigregion is 16x16 regions. for each bigregion in X dimension:
    auto fptr = data->unk_204[bigregion.x][bigregion.y].features;
    if (!fptr)
        return NULL;

    df::coord2d sub = rgn_pos & 15;

    vector <df::feature_init *> &features = fptr->feature_init[sub.x][sub.y];

    return vector_get(features, index);
}

static bool GetLocalFeature(t_feature &feature, df::coord2d rgn_pos, int32_t index)
{
    feature.type = (df::feature_type)-1;

    auto f = Maps::getLocalInitFeature(rgn_pos, index);
    if (!f)
        return false;

    feature.discovered = false;
    feature.origin = f;
    feature.type = f->getType();
    f->getMaterial(&feature.main_material, &feature.sub_material);
    return true;
}

bool Maps::ReadFeatures(uint32_t x, uint32_t y, uint32_t z, t_feature *local, t_feature *global)
{
    df::map_block *block = getBlock(x,y,z);
    if (!block)
        return false;
    return ReadFeatures(block, local, global);
}

bool Maps::ReadFeatures(df::map_block * block, t_feature * local, t_feature * global)
{
    bool result = true;
    if (global)
    {
        if (block->global_feature != -1)
            result &= GetGlobalFeature(*global, block->global_feature);
        else
            global->type = (df::feature_type)-1;
    }
    if (local)
    {
        if (block->local_feature != -1)
            result &= GetLocalFeature(*local, block->region_pos, block->local_feature);
        else
            local->type = (df::feature_type)-1;
    }
    return result;
}

/*
 * Block events
 */
bool Maps::SortBlockEvents(df::map_block *block,
    vector <df::block_square_event_mineralst *>* veins,
    vector <df::block_square_event_frozen_liquidst *>* ices,
    vector <df::block_square_event_material_spatterst *> *splatter,
    vector <df::block_square_event_grassst *> *grasses,
    vector <df::block_square_event_world_constructionst *> *constructions)
{
    if (veins)
        veins->clear();
    if (ices)
        ices->clear();
    if (splatter)
        splatter->clear();
    if (grasses)
        grasses->clear();
    if (constructions)
        constructions->clear();

    if (!block)
        return false;

    // read all events
    for (size_t i = 0; i < block->block_events.size(); i++)
    {
        df::block_square_event *evt = block->block_events[i];
        switch (evt->getType())
        {
        case block_square_event_type::mineral:
            if (veins)
                veins->push_back((df::block_square_event_mineralst *)evt);
            break;
        case block_square_event_type::frozen_liquid:
            if (ices)
                ices->push_back((df::block_square_event_frozen_liquidst *)evt);
            break;
        case block_square_event_type::material_spatter:
            if (splatter)
                splatter->push_back((df::block_square_event_material_spatterst *)evt);
            break;
        case block_square_event_type::grass:
            if (grasses)
                grasses->push_back((df::block_square_event_grassst *)evt);
            break;
        case block_square_event_type::world_construction:
            if (constructions)
                constructions->push_back((df::block_square_event_world_constructionst *)evt);
            break;
        }
    }
    return true;
}

bool Maps::RemoveBlockEvent(uint32_t x, uint32_t y, uint32_t z, df::block_square_event * which)
{
    df::map_block * block = getBlock(x,y,z);
    if (!block)
        return false;

    int idx = linear_index(block->block_events, which);
    if (idx >= 0)
    {
        delete which;
        vector_erase_at(block->block_events, idx);
        return true;
    }
    else
        return false;
}

static df::coord2d biome_offsets[9] = {
    df::coord2d(-1,-1), df::coord2d(0,-1), df::coord2d(1,-1),
    df::coord2d(-1,0), df::coord2d(0,0), df::coord2d(1,0),
    df::coord2d(-1,1), df::coord2d(0,1), df::coord2d(1,1)
};

inline df::coord2d getBiomeRgnPos(df::coord2d base, int idx)
{
    auto r = base + biome_offsets[idx];

    int world_width = world->world_data->world_width;
    int world_height = world->world_data->world_height;

    return df::coord2d(clip_range(r.x,0,world_width-1),clip_range(r.y,0,world_height-1));
}

df::coord2d Maps::getBlockTileBiomeRgn(df::map_block *block, df::coord2d pos)
{
    if (!block || !world->world_data)
        return df::coord2d();

    auto des = MapExtras::index_tile<df::tile_designation>(block->designation,pos);
    unsigned idx = des.bits.biome;
    if (idx < 9)
    {
        idx = block->region_offset[idx];
        if (idx < 9)
            return getBiomeRgnPos(block->region_pos, idx);
    }

    return df::coord2d();
}

/*
* Layer geology
*/
bool Maps::ReadGeology(vector<vector<int16_t> > *layer_mats, vector<df::coord2d> *geoidx)
{
    if (!world->world_data)
        return false;

    layer_mats->resize(eBiomeCount);
    geoidx->resize(eBiomeCount);

    for (int i = 0; i < eBiomeCount; i++)
    {
        (*layer_mats)[i].clear();
        (*geoidx)[i] = df::coord2d(-30000,-30000);
    }

    // regionX is in embark squares
    // regionX/16 is in 16x16 embark square regions
    df::coord2d map_region(world->map.region_x / 16, world->map.region_y / 16);

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        df::coord2d rgn_pos = getBiomeRgnPos(map_region, i);

        (*geoidx)[i] = rgn_pos;

        auto biome = getRegionBiome(rgn_pos);
        if (!biome)
            continue;

        // get index into geoblock vector
        int16_t geoindex = biome->geo_index;

        /// geology blocks have a vector of layer descriptors
        // get the vector with pointer to layers
        df::world_geo_biome *geo_biome = df::world_geo_biome::find(geoindex);
        if (!geo_biome)
            continue;

        auto &geolayers = geo_biome->layers;
        auto &matvec = (*layer_mats)[i];

        /// layer descriptor has a field that determines the type of stone/soil
        matvec.resize(geolayers.size());

        // finally, read the layer matgloss
        for (size_t j = 0; j < geolayers.size(); j++)
            matvec[j] = geolayers[j]->mat_index;
    }

    return true;
}

bool Maps::canWalkBetween(df::coord pos1, df::coord pos2)
{
    auto block1 = getTileBlock(pos1);
    auto block2 = getTileBlock(pos2);

    if (!block1 || !block2)
        return false;

    auto tile1 = MapExtras::index_tile<uint16_t>(block1->walkable, pos1);
    auto tile2 = MapExtras::index_tile<uint16_t>(block2->walkable, pos2);

    return tile1 && tile1 == tile2;
}

#define COPY(a,b) memcpy(&a,&b,sizeof(a))

MapExtras::Block::Block(MapCache *parent, DFCoord _bcoord) : parent(parent)
{
    dirty_designations = false;
    dirty_tiles = false;
    dirty_temperatures = false;
    dirty_occupancies = false;
    valid = false;
    bcoord = _bcoord;
    block = Maps::getBlock(bcoord);
    item_counts = NULL;
    tags = NULL;
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

        if (block)
            ParseTiles(tiles);
    }

    if (basemat && !basemats)
    {
        basemats = new BasematInfo();

        if (block)
            ParseBasemats(tiles, basemats);
    }
}

MapExtras::Block::TileInfo::TileInfo()
{
    frozen.clear();
    dirty_raw.clear();
    memset(raw_tiles,0,sizeof(raw_tiles));
    con_info = NULL;
    dirty_base.clear();
    memset(base_tiles,0,sizeof(base_tiles));
}

MapExtras::Block::TileInfo::~TileInfo()
{
    delete con_info;
}

void MapExtras::Block::TileInfo::init_coninfo()
{
    if (con_info)
        return;

    con_info = new ConInfo();
    con_info->constructed.clear();
    COPY(con_info->tiles, base_tiles);
    memset(con_info->mattype, -1, sizeof(con_info->mattype));
    memset(con_info->matindex, -1, sizeof(con_info->matindex));
}

MapExtras::Block::BasematInfo::BasematInfo()
{
    dirty.clear();
    memset(mattype,0,sizeof(mattype));
    memset(matindex,-1,sizeof(matindex));
    memset(layermat,-1,sizeof(layermat));
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

            // Frozen liquid comes topmost
            if (tileMaterial(tt) == FROZEN_LIQUID)
            {
                tiles->frozen.setassignment(x,y,true);
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
                    tiles->con_info->mattype[x][y] = con->mat_type;
                    tiles->con_info->matindex[x][y] = con->mat_index;

                    tt = con->original_tile;
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

void MapExtras::Block::ParseBasemats(TileInfo *tiles, BasematInfo *bmats)
{
    BlockInfo info;

    info.prepare(this);

    COPY(bmats->layermat, info.basemats);

    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            using namespace df::enums::tiletype_material;

            auto tt = tiles->base_tiles[x][y];
            auto mat = info.getBaseMaterial(tt, df::coord2d(x,y));

            bmats->mattype[x][y] = mat.mat_type;
            bmats->matindex[x][y] = mat.mat_index;

            // Copy base info back to construction layer
            if (tiles->con_info && !tiles->con_info->constructed.getassignment(x,y))
            {
                tiles->con_info->mattype[x][y] = mat.mat_type;
                tiles->con_info->matindex[x][y] = mat.mat_index;
            }
        }
    }
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
    if(dirty_tiles && tiles)
    {
        dirty_tiles = false;

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if (tiles->dirty_raw.getassignment(x,y))
                    block->tiletype[x][y] = tiles->raw_tiles[x][y];
            }
        }

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

    SquashVeins(block,veinmats);
    SquashGrass(block, grass);

    if (parent->validgeo)
        SquashRocks(block,basemats,&parent->layer_mats);
    else
        memset(basemats,-1,sizeof(basemats));

    for (size_t i = 0; i < block->plants.size(); i++)
    {
        auto pp = block->plants[i];
        plants[pp->pos] = pp;
    }

    global_feature = Maps::getGlobalInitFeature(block->global_feature);
    local_feature = Maps::getLocalInitFeature(block->region_pos, block->local_feature);
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
        rv.mat_index = basemats[x][y];

        if (auto raw = df::inorganic_raw::find(rv.mat_index))
        {
            if (raw->flags.is_set(inorganic_flags::SOIL_ANY))
                break;

            int biome = mblock->biomeIndexAt(pos);
            int idx = vector_get(parent->default_soil, biome, -1);
            if (idx >= 0)
                rv.mat_index = idx;
        }

        break;
    }

    case STONE:
    {
        rv.mat_index = basemats[x][y];

        if (auto raw = df::inorganic_raw::find(rv.mat_index))
        {
            if (!raw->flags.is_set(inorganic_flags::SOIL_ANY))
                break;

            int biome = mblock->biomeIndexAt(pos);
            int idx = vector_get(parent->default_stone, biome, -1);
            if (idx >= 0)
                rv.mat_index = idx;
        }

        break;
    }

    case MINERAL:
        rv.mat_index = veinmats[x][y];
        break;

    case LAVA_STONE:
        if (auto details = parent->region_details[mblock->biomeRegionAt(pos)])
            rv.mat_index = details->lava_stone;
        break;

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
        rv.mat_index = basemats[x][y];
        break;

    case ASHES:
    case FIRE:
    case CAMPFIRE:
        rv.mat_type = builtin_mats::ASH;
        break;
    }

    return rv;
}

void MapExtras::BlockInfo::SquashVeins(df::map_block *mb, t_blockmaterials & materials)
{
    std::vector <df::block_square_event_mineralst *> veins;
    Maps::SortBlockEvents(mb,&veins);
    memset(materials,-1,sizeof(materials));
    for (uint32_t x = 0;x<16;x++) for (uint32_t y = 0; y< 16;y++)
    {
        for (size_t i = 0; i < veins.size(); i++)
        {
            if (veins[i]->getassignment(x,y))
                materials[x][y] = veins[i]->inorganic_mat;
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
    if (idx >= parent->geoidx.size())
        return -1;
    return idx;
}

df::coord2d MapExtras::Block::biomeRegionAt(df::coord2d p)
{
    if (!block)
        return df::coord2d(-30000,-30000);

    int idx = biomeIndexAt(p);
    if (idx < 0)
        return block->region_pos;

    return parent->geoidx[idx];
}

int16_t MapExtras::Block::GeoIndexAt(df::coord2d p)
{
    df::coord2d biome = biomeRegionAt(p);
    if (!biome.isValid())
        return -1;

    auto pinfo = Maps::getRegionBiome(biome);
    if (!pinfo)
        return -1;

    return pinfo->geo_index;
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

    default_soil.resize(layer_mats.size());
    default_stone.resize(layer_mats.size());

    for (size_t i = 0; i < layer_mats.size(); i++)
    {
        default_soil[i] = -1;
        default_stone[i] = -1;

        for (size_t j = 0; j < layer_mats[i].size(); j++)
        {
            auto raw = df::inorganic_raw::find(layer_mats[i][j]);
            if (!raw)
                continue;

            bool is_soil = raw->flags.is_set(inorganic_flags::SOIL_ANY);
            if (is_soil)
                default_soil[i] = layer_mats[i][j];
            else if (default_stone[i] == -1)
                default_stone[i] = layer_mats[i][j];
        }
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

void MapExtras::MapCache::resetTags()
{
    for (auto it = blocks.begin(); it != blocks.end(); ++it)
    {
        delete[] it->second->tags;
        it->second->tags = NULL;
    }
}
