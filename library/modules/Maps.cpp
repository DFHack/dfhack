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

df::feature_init *Maps::getGlobalInitFeature(int32_t index)
{
    auto data = world->world_data;
    if (!data)
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
    if (!data)
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

    int world_width = world->world_data->world_width;
    int world_height = world->world_data->world_height;

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = world->map.region_x / 16 + ((i % 3) - 1);
        int bioRY = world->map.region_y / 16 + ((i / 3) - 1);

        df::coord2d rgn_pos(clip_range(bioRX,0,world_width-1),clip_range(bioRX,0,world_height-1));

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

#define COPY(a,b) memcpy(&a,&b,sizeof(a))

MapExtras::Block::Block(MapCache *parent, DFCoord _bcoord) : parent(parent)
{
    dirty_designations = false;
    dirty_tiletypes = false;
    dirty_temperatures = false;
    dirty_blockflags = false;
    dirty_occupancies = false;
    valid = false;
    bcoord = _bcoord;
    block = Maps::getBlock(bcoord);
    item_counts = NULL;

    memset(tags,0,sizeof(tags));

    if(block)
    {
        COPY(rawtiles, block->tiletype);
        COPY(designation, block->designation);
        COPY(occupancy, block->occupancy);
        blockflags = block->flags;

        COPY(temp1, block->temperature_1);
        COPY(temp2, block->temperature_2);

        SquashVeins(block,veinmats);
        SquashConstructions(block, contiles);
        SquashFrozenLiquids(block, icetiles);
        if(parent->validgeo)
            SquashRocks(block,basemats,&parent->layer_mats);
        else
            memset(basemats,-1,sizeof(basemats));
        valid = true;
    }
    else
    {
        blockflags.whole = 0;
        memset(rawtiles,0,sizeof(rawtiles));
        memset(designation,0,sizeof(designation));
        memset(occupancy,0,sizeof(occupancy));
        memset(temp1,0,sizeof(temp1));
        memset(temp2,0,sizeof(temp2));
        memset(veinmats,-1,sizeof(veinmats));
        memset(contiles,0,sizeof(contiles));
        memset(icetiles,0,sizeof(icetiles));
        memset(basemats,-1,sizeof(basemats));
    }
}

MapExtras::Block::~Block()
{
    delete[] item_counts;
}

bool MapExtras::Block::Write ()
{
    if(!valid) return false;

    if(dirty_blockflags)
    {
        block->flags = blockflags;
        dirty_blockflags = false;
    }
    if(dirty_designations)
    {
        COPY(block->designation, designation);
        block->flags.bits.designated = true;
        dirty_designations = false;
    }
    if(dirty_tiletypes)
    {
        COPY(block->tiletype, rawtiles);
        dirty_tiletypes = false;
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

void MapExtras::Block::SquashVeins(df::map_block *mb, t_blockmaterials & materials)
{
    memset(materials,-1,sizeof(materials));
    std::vector <df::block_square_event_mineralst *> veins;
    Maps::SortBlockEvents(mb,&veins);
    for (uint32_t x = 0;x<16;x++) for (uint32_t y = 0; y< 16;y++)
    {
        df::tiletype tt = mb->tiletype[x][y];
        if (tileMaterial(tt) == tiletype_material::MINERAL)
        {
            for (size_t i = 0; i < veins.size(); i++)
            {
                if (veins[i]->getassignment(x,y))
                    materials[x][y] = veins[i]->inorganic_mat;
            }
        }
    }
}

void MapExtras::Block::SquashFrozenLiquids(df::map_block *mb, tiletypes40d & frozen)
{
    std::vector <df::block_square_event_frozen_liquidst *> ices;
    Maps::SortBlockEvents(mb,NULL,&ices);
    for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
    {
        df::tiletype tt = mb->tiletype[x][y];
        frozen[x][y] = tiletype::Void;
        if (tileMaterial(tt) == tiletype_material::FROZEN_LIQUID)
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
}

void MapExtras::Block::SquashConstructions (df::map_block *mb, tiletypes40d & constructions)
{
    for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
    {
        df::tiletype tt = mb->tiletype[x][y];
        constructions[x][y] = tiletype::Void;
        if (tileMaterial(tt) == tiletype_material::CONSTRUCTION)
        {
            DFCoord coord = mb->map_pos + df::coord(x,y,0);
            df::construction *con = df::construction::find(coord);
            if (con)
                constructions[x][y] = con->original_tile;
        }
    }
}

void MapExtras::Block::SquashRocks (df::map_block *mb, t_blockmaterials & materials,
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

df::coord2d MapExtras::Block::biomeRegionAt(df::coord2d p)
{
    if (!block)
        return df::coord2d(-30000,-30000);

    auto des = index_tile<df::tile_designation>(designation,p);
    uint8_t idx = des.bits.biome;
    if (idx >= 9)
        return block->region_pos;
    idx = block->region_offset[idx];
    if (idx >= parent->geoidx.size())
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
        index_tile<df::tile_occupancy&>(block->occupancy,item->pos).bits.item = false;
    }

    return true;
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
        if(blockcoord.x >= 0 && blockcoord.x < x_bmax &&
            blockcoord.y >= 0 && blockcoord.y < y_bmax &&
            blockcoord.z >= 0 && blockcoord.z < z_max)
        {
            Block * nblo = new Block(this, blockcoord);
            blocks[blockcoord] = nblo;
            return nblo;
        }
        return 0;
    }
}

df::burrow *Maps::findBurrowByName(std::string name)
{
    auto &vec = df::burrow::get_vector();

    for (size_t i = 0; i < vec.size(); i++)
        if (vec[i]->name == name)
            return vec[i];

    return NULL;
}

void Maps::listBurrowBlocks(std::vector<df::map_block*> *pvec, df::burrow *burrow)
{
    CHECK_NULL_POINTER(burrow);

    pvec->clear();
    pvec->reserve(burrow->block_x.size());

    df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);

    for (size_t i = 0; i < burrow->block_x.size(); i++)
    {
        df::coord pos(burrow->block_x[i], burrow->block_y[i], burrow->block_z[i]);

        auto block = getBlock(pos - base);
        if (block)
            pvec->push_back(block);
    }
}

df::block_burrow *Maps::getBlockBurrowMask(df::burrow *burrow, df::map_block *block, bool create)
{
    CHECK_NULL_POINTER(burrow);
    CHECK_NULL_POINTER(block);

    int32_t id = burrow->id;
    df::block_burrow_link *prev = &block->block_burrows;
    df::block_burrow_link *link = prev->next;

    for (; link; prev = link, link = link->next)
        if (link->item->id == id)
            return link->item;

    if (create)
    {
        link = new df::block_burrow_link;
        link->item = new df::block_burrow;

        link->item->id = burrow->id;
        memset(link->item->tile_bitmask,0,sizeof(link->item->tile_bitmask));
        link->item->link = link;

        link->next = NULL;
        link->prev = prev;
        prev->next = link;

        df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);
        df::coord pos = base + block->map_pos/16;

        burrow->block_x.push_back(pos.x);
        burrow->block_y.push_back(pos.y);
        burrow->block_z.push_back(pos.z);

        return link->item;
    }

    return NULL;
}

bool Maps::isBlockBurrowTile(df::burrow *burrow, df::map_block *block, df::coord2d tile)
{
    CHECK_NULL_POINTER(burrow);

    if (!block) return false;

    auto mask = getBlockBurrowMask(burrow, block);

    return mask ? mask->getassignment(tile & 15) : false;
}

bool Maps::setBlockBurrowTile(df::burrow *burrow, df::map_block *block, df::coord2d tile, bool enable)
{
    CHECK_NULL_POINTER(burrow);

    if (!block) return false;

    auto mask = getBlockBurrowMask(burrow, block, enable);

    if (mask)
        mask->setassignment(tile & 15, enable);

    return true;
}

