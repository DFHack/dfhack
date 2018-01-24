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

#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/block_square_event_grassst.h"
#include "df/building_type.h"
#include "df/builtin_mats.h"
#include "df/burrow.h"
#include "df/feature_init.h"
#include "df/flow_info.h"
#include "df/plant.h"
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
    case feature_type::underworld_from_layer:
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

df::map_block_column *Maps::getBlockColumn(int32_t blockx, int32_t blocky)
{
    if (!IsValid())
        return NULL;
    if ((blockx < 0) || (blocky < 0))
        return NULL;
    if ((blockx >= world->map.x_count_block) || (blocky >= world->map.y_count_block))
        return NULL;
    return world->map.column_index[blockx][blocky];
}

bool Maps::isValidTilePos(int32_t x, int32_t y, int32_t z)
{
    if (!IsValid())
        return false;
    if ((x < 0) || (y < 0) || (z < 0))
        return false;
    if ((x >= world->map.x_count) || (y >= world->map.y_count) || (z >= world->map.z_count))
        return false;
    return true;
}

bool Maps::isTileVisible(int32_t x, int32_t y, int32_t z)
{
    df::map_block *block = getTileBlock(x, y, z);
    if (!block)
        return false;
    if (block->designation[x % 16][y % 16].bits.hidden)
        return false;

    return true;
}

df::map_block *Maps::getTileBlock (int32_t x, int32_t y, int32_t z)
{
    if (!isValidTilePos(x,y,z))
        return NULL;
    return world->map.block_index[x >> 4][y >> 4][z];
}

df::map_block *Maps::ensureTileBlock (int32_t x, int32_t y, int32_t z)
{
    if (!isValidTilePos(x,y,z))
        return NULL;

    auto column = world->map.block_index[x >> 4][y >> 4];
    auto &slot = column[z];
    if (slot)
        return slot;

    // Find another block below
    int z2 = z;
    while (z2 >= 0 && !column[z2]) z2--;
    if (z2 < 0)
        return NULL;

    slot = new df::map_block();
    slot->region_pos = column[z2]->region_pos;
    slot->map_pos = column[z2]->map_pos;
    slot->map_pos.z = z;

    // Assume sky
    df::tile_designation dsgn;
    dsgn.bits.light = true;
    dsgn.bits.outside = true;

    for (int tx = 0; tx < 16; tx++)
        for (int ty = 0; ty < 16; ty++) {
            slot->designation[tx][ty] = dsgn;
            slot->temperature_1[tx][ty] = column[z2]->temperature_1[tx][ty];
            slot->temperature_2[tx][ty] = column[z2]->temperature_2[tx][ty];
        }

    df::global::world->map.map_blocks.push_back(slot);
    return slot;
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

df::region_map_entry *Maps::getRegionBiome(df::coord2d rgn_pos)
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

    auto z_flags = world->map_extras.z_level_flags;
    int z_level = blk->map_pos.z;

    if (z_flags && z_level >= 0 && z_level < world->map.z_count_block)
    {
        z_flags += z_level;
        z_flags->bits.update = true;
        z_flags->bits.update_twice = true;
    }
}

df::flow_info *Maps::spawnFlow(df::coord pos, df::flow_type type, int mat_type, int mat_index, int density)
{
    using df::global::flows;

    auto block = getTileBlock(pos);
    if (!flows || !block)
        return NULL;

    auto flow = new df::flow_info();
    flow->type = type;
    flow->mat_type = mat_type;
    flow->mat_index = mat_index;
    flow->density = std::min(100, density);
    flow->pos = pos;

    block->flows.push_back(flow);
    flows->push_back(flow);
    return flow;
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
    auto fptr = data->feature_map[bigregion.x][bigregion.y].features;
    if (!fptr)
        return NULL;

    df::coord2d sub = rgn_pos & 15;

    vector <df::feature_init *> &features = fptr->feature_init[sub.x][sub.y];

    return vector_get(features, index);
}

bool GetLocalFeature(t_feature &feature, df::coord2d rgn_pos, int32_t index)
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
    vector <df::block_square_event_material_spatterst *> *materials,
    vector <df::block_square_event_grassst *> *grasses,
    vector <df::block_square_event_world_constructionst *> *constructions,
    vector <df::block_square_event_spoorst *> *spoors,
    vector <df::block_square_event_item_spatterst *> *items,
    vector <df::block_square_event_designation_priorityst *> *priorities)
{
    if (veins)
        veins->clear();
    if (ices)
        ices->clear();
    if (constructions)
        constructions->clear();
    if (materials)
        materials->clear();
    if (grasses)
        grasses->clear();
    if (spoors)
        spoors->clear();
    if (items)
        items->clear();

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
        case block_square_event_type::world_construction:
            if (constructions)
                constructions->push_back((df::block_square_event_world_constructionst *)evt);
            break;
        case block_square_event_type::material_spatter:
            if (materials)
                materials->push_back((df::block_square_event_material_spatterst *)evt);
            break;
        case block_square_event_type::grass:
            if (grasses)
                grasses->push_back((df::block_square_event_grassst *)evt);
            break;
        case block_square_event_type::spoor:
            if (spoors)
                spoors->push_back((df::block_square_event_spoorst *)evt);
            break;
        case block_square_event_type::item_spatter:
            if (items)
                items->push_back((df::block_square_event_item_spatterst *)evt);
            break;
        case block_square_event_type::designation_priority:
            if (priorities)
                priorities->push_back((df::block_square_event_designation_priorityst *)evt);
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

    auto des = index_tile<df::tile_designation>(block->designation,pos);
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

    auto tile1 = index_tile<uint16_t>(block1->walkable, pos1);
    auto tile2 = index_tile<uint16_t>(block2->walkable, pos2);

    return tile1 && tile1 == tile2;
}

bool Maps::canStepBetween(df::coord pos1, df::coord pos2)
{
    color_ostream& out = Core::getInstance().getConsole();
    int32_t dx = pos2.x-pos1.x;
    int32_t dy = pos2.y-pos1.y;
    int32_t dz = pos2.z-pos1.z;

    if ( dx*dx > 1 || dy*dy > 1 || dz*dz > 1 )
        return false;

    if ( pos2.z < pos1.z ) {
        df::coord temp = pos1;
        pos1 = pos2;
        pos2 = temp;
    }

    df::map_block* block1 = getTileBlock(pos1);
    df::map_block* block2 = getTileBlock(pos2);

    if ( !block1 || !block2 )
        return false;

    if ( !index_tile<uint16_t>(block1->walkable,pos1) || !index_tile<uint16_t>(block2->walkable,pos2) ) {
        return false;
    }

    if ( block1->designation[pos1.x&0xF][pos1.y&0xF].bits.flow_size >= 4 ||
         block2->designation[pos2.x&0xF][pos2.y&0xF].bits.flow_size >= 4 )
        return false;

    if ( dz == 0 )
        return true;

    df::tiletype* type1 = Maps::getTileType(pos1);
    df::tiletype* type2 = Maps::getTileType(pos2);

    df::tiletype_shape shape1 = ENUM_ATTR(tiletype,shape,*type1);
    df::tiletype_shape shape2 = ENUM_ATTR(tiletype,shape,*type2);

    if ( dx == 0 && dy == 0 ) {
        //check for forbidden hatches and floors and such
        df::tile_building_occ upOcc = index_tile<df::tile_occupancy>(block2->occupancy,pos2).bits.building;
        if ( upOcc == tile_building_occ::Impassable || upOcc == tile_building_occ::Obstacle || upOcc == tile_building_occ::Floored )
            return false;

        if ( shape1 == tiletype_shape::STAIR_UPDOWN && shape2 == shape1 )
            return true;
        if ( shape1 == tiletype_shape::STAIR_UPDOWN && shape2 == tiletype_shape::STAIR_DOWN )
            return true;
        if ( shape1 == tiletype_shape::STAIR_UP && shape2 == tiletype_shape::STAIR_UPDOWN )
            return true;
        if ( shape1 == tiletype_shape::STAIR_UP && shape2 == tiletype_shape::STAIR_DOWN )
            return true;
        if ( shape1 == tiletype_shape::RAMP && shape2 == tiletype_shape::RAMP_TOP ) {
            //it depends
            //there has to be a wall next to the ramp
            bool foundWall = false;
            for ( int32_t x = -1; x <= 1; x++ ) {
                for ( int32_t y = -1; y <= 1; y++ ) {
                    if ( x == 0 && y == 0 )
                        continue;
                    df::tiletype* type = Maps::getTileType(df::coord(pos1.x+x,pos1.y+y,pos1.z));
                    df::tiletype_shape shape1 = ENUM_ATTR(tiletype,shape,*type);
                    if ( shape1 == tiletype_shape::WALL ) {
                        foundWall = true;
                        x = 2;
                        break;
                    }
                }
            }
            if ( !foundWall )
                return false; //unusable ramp

            //there has to be an unforbidden hatch above the ramp
            if ( index_tile<df::tile_occupancy>(block2->occupancy,pos2).bits.building != tile_building_occ::Dynamic )
                return false;
            //note that forbidden hatches have Floored occupancy. unforbidden ones have dynamic occupancy
            df::building* building = Buildings::findAtTile(pos2);
            if ( building == NULL ) {
                out << __FILE__ << ", line " << __LINE__ << ": couldn't find hatch.\n";
                return false;
            }
            if ( building->getType() != building_type::Hatch ) {
                return false;
            }
            return true;
        }
        return false;
    }

    //diagonal up: has to be a ramp
    if ( shape1 == tiletype_shape::RAMP /*&& shape2 == tiletype_shape::RAMP*/ ) {
        df::coord up = df::coord(pos1.x,pos1.y,pos1.z+1);
        bool foundWall = false;
        for ( int32_t x = -1; x <= 1; x++ ) {
            for ( int32_t y = -1; y <= 1; y++ ) {
                if ( x == 0 && y == 0 )
                    continue;
                df::tiletype* type = Maps::getTileType(df::coord(pos1.x+x,pos1.y+y,pos1.z));
                df::tiletype_shape shape1 = ENUM_ATTR(tiletype,shape,*type);
                if ( shape1 == tiletype_shape::WALL ) {
                    foundWall = true;
                    x = 2;
                    break;
                }
            }
        }
        if ( !foundWall )
            return false; //unusable ramp
        df::tiletype* typeUp = Maps::getTileType(up);
        df::tiletype_shape shapeUp = ENUM_ATTR(tiletype,shape,*typeUp);
        if ( shapeUp != tiletype_shape::RAMP_TOP )
            return false;

        df::map_block* blockUp = getTileBlock(up);
        if ( !blockUp )
            return false;

        df::tile_building_occ occupancy = index_tile<df::tile_occupancy>(blockUp->occupancy,up).bits.building;
        if ( occupancy == tile_building_occ::Obstacle || occupancy == tile_building_occ::Floored || occupancy == tile_building_occ::Impassable )
            return false;
        return true;
    }

    return false;
}
