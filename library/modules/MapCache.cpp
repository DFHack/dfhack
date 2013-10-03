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

#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "ColorText.h"
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
#include "df/region_map_entry.h"
#include "df/flow_info.h"
#include "df/plant.h"
#include "df/building_type.h"

using namespace DFHack;
using namespace MapExtras;
using namespace df::enums;
using df::global::world;

extern bool GetLocalFeature(t_feature &feature, df::coord2d rgn_pos, int32_t index);

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
    memset(con_info->mat_type, -1, sizeof(con_info->mat_type));
    memset(con_info->mat_index, -1, sizeof(con_info->mat_index));
}

MapExtras::Block::BasematInfo::BasematInfo()
{
    dirty.clear();
    memset(mat_type,0,sizeof(mat_type));
    memset(mat_index,-1,sizeof(mat_index));
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
                    tiles->con_info->mat_type[x][y] = con->mat_type;
                    tiles->con_info->mat_index[x][y] = con->mat_index;

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

            bmats->mat_type[x][y] = mat.mat_type;
            bmats->mat_index[x][y] = mat.mat_index;

            // Copy base info back to construction layer
            if (tiles->con_info && !tiles->con_info->constructed.getassignment(x,y))
            {
                tiles->con_info->mat_type[x][y] = mat.mat_type;
                tiles->con_info->mat_index[x][y] = mat.mat_index;
            }
        }
    }
}

bool MapExtras::Block::isDirty()
{
    return valid && (dirty_designations || dirty_tiles || dirty_temperatures || dirty_occupancies);
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

    for (size_t i = 0; i < block->plants.size(); i++)
    {
        auto pp = block->plants[i];
        plants[pp->pos] = pp;
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

        for (size_t j = 0; j < std::min(BiomeInfo::MAX_LAYERS,layer_mats[i].size()); j++)
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
