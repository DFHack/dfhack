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

#include "df/biome_type.h"
#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/block_square_event_grassst.h"
#include "df/building_type.h"
#include "df/builtin_mats.h"
#include "df/burrow.h"
#include "df/feature_init.h"
#include "df/flow_info.h"
#include "df/map_block_column.h"
#include "df/plant.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#include "df/region_map_entry.h"
#include "df/world.h"
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

// getter for map size in blocks
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

// getter for map size in tiles
void Maps::getTileSize (uint32_t& x, uint32_t& y, uint32_t& z)
{
    getSize(x, y, z);
    x *= 16;
    y *= 16;
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

    auto des = index_tile(block->designation,pos);
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

    auto tile1 = index_tile(block1->walkable, pos1);
    auto tile2 = index_tile(block2->walkable, pos2);

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

    if ( !index_tile(block1->walkable,pos1) || !index_tile(block2->walkable,pos2) ) {
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
        df::tile_building_occ upOcc = index_tile(block2->occupancy,pos2).bits.building;
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
            if ( index_tile(block2->occupancy,pos2).bits.building != tile_building_occ::Dynamic )
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

        df::tile_building_occ occupancy = index_tile(blockUp->occupancy,up).bits.building;
        if ( occupancy == tile_building_occ::Obstacle || occupancy == tile_building_occ::Floored || occupancy == tile_building_occ::Impassable )
            return false;
        return true;
    }

    return false;
}

/*
* Plants
*/
df::plant *Maps::getPlantAtTile(int32_t x, int32_t y, int32_t z)
{
    if (x < 0 || x >= world->map.x_count || y < 0 || y >= world->map.y_count || !world->map.column_index)
        return NULL;

    df::map_block_column *mbc = world->map.column_index[(x / 48) * 3][(y / 48) * 3];
    if (!mbc)
        return NULL;

    for (size_t i = 0; i < mbc->plants.size(); i++)
    {
        df::plant *p = mbc->plants[i];
        if (p->pos.x == x && p->pos.y == y && p->pos.z == z)
            return p;

        df::plant_tree_info *t = p->tree_info;
        if (!t)
            continue;

        int32_t x_index = (t->dim_x / 2) - (p->pos.x % 48) + (x % 48);
        int32_t y_index = (t->dim_y / 2) - (p->pos.y % 48) + (y % 48);
        int32_t z_dis = z - p->pos.z;
        if (x_index < 0 || x_index >= t->dim_x || y_index < 0 || y_index >= t->dim_y || z_dis >= t->body_height)
            continue;

        if (z_dis < 0)
        {
            if (z_dis < -(t->roots_depth))
                continue;
            else if ((t->roots[-1 - z_dis][x_index + y_index * t->dim_x].whole & 0x7F) != 0) //any non-blocked tree_tile
                return p;
        }
        else if ((t->body[z_dis][x_index + y_index * t->dim_x].whole & 0x7F) != 0)
            return p;
    }

    return NULL;
}

/* The code below is a heavily refactored version of code found at
   https://github.com/ragundo/exportmaps/blob/master/cpp/df_utils/biome_type.cpp.
*/

/*
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

namespace {
    //----------------------------------------------------------------------------//
    // Utility function
    //
    //----------------------------------------------------------------------------//
    std::pair<bool, bool> check_tropicality(df::region_map_entry& region,
        int y_pos
    )
    {
        int flip_latitude = df::global::world->world_data->flip_latitude;

        bool is_possible_tropical_area_by_latitude = false;
        bool is_tropical_area_by_latitude = false;

        if (flip_latitude == -1)  // NO POLES
        {
            // If there're no poles, tropical area is determined by temperature
            is_possible_tropical_area_by_latitude = region.temperature >= 75;
            is_tropical_area_by_latitude = region.temperature >= 85;
        }

        else
        {
            int v6 = 0;

            df::world_data* wdata = df::global::world->world_data;

            if (flip_latitude == 0) // NORTH POLE ONLY
            {
                v6 = y_pos;
            }

            else if (flip_latitude == 1) // SOUTH_POLE ONLY
            {
                v6 = df::global::world->world_data->world_height - y_pos - 1;
            }

            else if (flip_latitude == 2) // BOTH POLES
            {
                if (y_pos < wdata->world_height / 2)
                    v6 = 2 * y_pos;
                else
                {
                    v6 = wdata->world_height + 2 * (wdata->world_height / 2 - y_pos) - 1;
                    if (v6 < 0)
                        v6 = 0;
                    if (v6 >= wdata->world_height)
                        v6 = wdata->world_height - 1;
                }
            }

            if (wdata->world_height == 17)
                v6 *= 16;
            else if (wdata->world_height == 33)
                v6 *= 8;
            else if (wdata->world_height == 65)
                v6 *= 4;
            else if (wdata->world_height == 129)
                v6 *= 2;

            is_possible_tropical_area_by_latitude = v6 > 170;
            is_tropical_area_by_latitude = v6 >= 200;
        }

        return std::pair<bool, bool>(is_possible_tropical_area_by_latitude,
            is_tropical_area_by_latitude
            );
    }


    //----------------------------------------------------------------------------//
    // Utility function
    //
    // return some unknow parameter as a percentage
    //----------------------------------------------------------------------------//
    int get_region_parameter(int y,
        int x
    )
    {
        int world_height = df::global::world->world_data->world_height;
        if (world_height > 65) // Medium and large worlds
        {
            // access to region 2D array
            df::region_map_entry& region = df::global::world->world_data->region_map[x][y];
            int flip_latitude = df::global::world->world_data->flip_latitude;
            int rainfall = region.rainfall;
            int result;
            int y_pos = y;
            int ypos = y_pos;

            if (flip_latitude == -1) // NO POLES
                return 100;

            else if (flip_latitude == 1) // SOUTH POLE
                ypos = world_height - y_pos - 1;
            else if (flip_latitude == 2) // NORTH & SOUTH POLE
            {
                if (ypos < world_height / 2)
                    ypos *= 2;
                else
                {
                    ypos = world_height + 2 * (world_height / 2 - ypos) - 1;
                    if (ypos < 0)
                        ypos = 0;
                    if (ypos >= world_height)
                        ypos = world_height - 1;
                }
            }

            int latitude; // 0 - 256 (size of a large world)
            switch (world_height)
            {
            case 17:                                    // Pocket world
                latitude = 16 * ypos;
                break;
            case 33:                                    // Smaller world
                latitude = 8 * ypos;
                break;
            case 65:                                    // Small world
                latitude = 4 * ypos;
                break;
            case 129:                                  // Medium world
                latitude = 2 * ypos;
                break;
            default:                                  // Large world
                latitude = ypos;
                break;
            }

            // latitude > 220
            if ((latitude - 171) > 49)
                return 100;


            // Latitude between 191 and 200
            if ((latitude > 190) && (latitude < 201))
                return 0;

            // Latitude between 201 and 220
            if ((latitude > 190) && (latitude >= 201))
                result = rainfall + 16 * (latitude - 207);
            else
                // Latitude between 0 and 190
                result = (16 * (184 - latitude) - rainfall);

            if (result < 0)
                return 0;

            if (result > 100)
                return 100;

            return result;
        }

        return 100;
    }
}


/*****************************************************************************
Module main function.
Return the biome type, given a position coordinate expressed in world_tiles
The world ref coordinates are used for tropicality determination and may refer
to a tile neighboring the "official" one.
*****************************************************************************/
df::enums::biome_type::biome_type Maps::GetBiomeTypeWithRef(int world_coord_x,
    int world_coord_y,
    int world_ref_coord_y
)
{
    // Biome is per region, so get the region where this biome exists
    df::region_map_entry& region = df::global::world->world_data->region_map[world_coord_x][world_coord_y];

    // Check if the y reference position coordinate belongs to a tropical area
    std::pair<bool, bool> p = check_tropicality(region,
        world_ref_coord_y
    );
    bool is_possible_tropical_area_by_latitude = p.first;
    bool is_tropical_area_by_latitude = p.second;

    int parameter = get_region_parameter(world_coord_y, world_coord_x);

    // Begin the discrimination
    if (region.flags.is_set(df::region_map_entry_flags::is_lake)) // is it a lake?
    {
        // salinity values tell us the lake type
        // greater than 66 is a salt water lake
        // between 33 and 65 is a brackish water lake
        // less than 33 is a fresh water lake
        if (region.salinity < 33)
            if (is_possible_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::LAKE_TROPICAL_FRESHWATER; // 39
            else
                return df::enums::biome_type::biome_type::LAKE_TEMPERATE_FRESHWATER; // 36
        else if (region.salinity < 66)
            if (is_possible_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::LAKE_TROPICAL_BRACKISHWATER; // 40
            else
                return df::enums::biome_type::biome_type::LAKE_TEMPERATE_BRACKISHWATER; // 37
        else // salinity >= 66
            if (is_possible_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::LAKE_TROPICAL_SALTWATER;// 41
            else
                return df::enums::biome_type::biome_type::LAKE_TEMPERATE_SALTWATER; // 38
    }

    // Not a lake. Check elevation
    // Elevation greater then 149 means a mountain biome
    // Elevation below 100 means a ocean biome
    // Elevation between 100 and 149 are land biomes

    if (region.elevation >= 150) // is it a mountain?
        return df::enums::biome_type::biome_type::MOUNTAIN; // 0

    if (region.elevation < 100) // is it a ocean?
    {
        if (is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::OCEAN_TROPICAL; // 27
        else if (region.temperature <= -5)
            return df::enums::biome_type::biome_type::OCEAN_ARCTIC; // 29
        else
            return df::enums::biome_type::biome_type::OCEAN_TEMPERATE; // 28
    }

    // land biome. Elevation between 100 and 149

    if (region.temperature <= -5)
    {
        if (region.drainage < 75)
            return df::enums::biome_type::biome_type::TUNDRA; // 2
        else
            return df::enums::biome_type::biome_type::GLACIER; // 1
    }

    // Not a lake, mountain, ocean, glacier or tundra
    // Vegetation determines the biome type

    if (region.vegetation < 10)
    {
        if (region.drainage < 33)
            return df::enums::biome_type::biome_type::DESERT_SAND; // 26
        else if (region.drainage < 66)
            return df::enums::biome_type::biome_type::DESERT_ROCK; // 25
        else // drainage >= 66
            return df::enums::biome_type::biome_type::DESERT_BADLAND; // 24
    }
    else if (region.vegetation < 20)
    {
        if ((is_possible_tropical_area_by_latitude && (parameter < 66)) || is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::GRASSLAND_TROPICAL; // 21
        else
            return df::enums::biome_type::biome_type::GRASSLAND_TEMPERATE; //18;
    }
    else if (region.vegetation < 33)
    {
        // vegetation between 20 and 32
        if ((is_possible_tropical_area_by_latitude && (parameter <= 6)) || is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::SAVANNA_TROPICAL; // 22
        else
            return df::enums::biome_type::biome_type::SAVANNA_TEMPERATE; //19;
    }
    else if (region.vegetation < 66)
    {
        if (region.drainage >= 33)
        {
            if (is_possible_tropical_area_by_latitude && (parameter < 66 || is_tropical_area_by_latitude))
                return df::enums::biome_type::biome_type::SHRUBLAND_TROPICAL; // 23
            else
                return df::enums::biome_type::biome_type::SHRUBLAND_TEMPERATE; // 20
        }
        // drainage < 33
        {
            if (region.salinity < 66)
            {
                if ((is_possible_tropical_area_by_latitude && (parameter < 66)) || is_tropical_area_by_latitude)
                    return df::enums::biome_type::biome_type::MARSH_TROPICAL_FRESHWATER; // 10
                else
                    return df::enums::biome_type::biome_type::MARSH_TEMPERATE_FRESHWATER; // 5
            }
            else // drainage < 33, salinity >= 66
            {
                if ((is_possible_tropical_area_by_latitude && (parameter < 66)) || is_tropical_area_by_latitude)
                    return df::enums::biome_type::biome_type::MARSH_TROPICAL_SALTWATER; // 11
                else
                    return df::enums::biome_type::biome_type::MARSH_TEMPERATE_SALTWATER; // 6
            }
        }
    }

    // Not a lake, mountain, ocean, glacier, tundra, desert, grassland or savanna
    // vegetation >= 66
    else if (region.drainage >= 33)
    {
        // drainage >= 33, not tropical area
        if (!is_possible_tropical_area_by_latitude)
        {
            if ((region.rainfall < 75) || (region.temperature < 65))
            {
                if (region.temperature >= 10)
                    return df::enums::biome_type::biome_type::FOREST_TEMPERATE_CONIFER; // 13
                else
                    return df::enums::biome_type::biome_type::FOREST_TAIGA; // 12
            }
            else
                return df::enums::biome_type::biome_type::FOREST_TEMPERATE_BROADLEAF; // 14
        }
        else // drainage >= 33, tropical area
        {
            if (((parameter < 66) || is_tropical_area_by_latitude) && (region.rainfall < 75))
                return df::enums::biome_type::biome_type::FOREST_TROPICAL_CONIFER; // 15
            if (parameter < 66)
                return df::enums::biome_type::biome_type::FOREST_TROPICAL_DRY_BROADLEAF; // 16
            if (is_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::FOREST_TROPICAL_MOIST_BROADLEAF; // 17
            else
            {
                if ((region.rainfall < 75) || (region.temperature < 65))
                {
                    if (region.temperature >= 10)
                        return df::enums::biome_type::biome_type::FOREST_TEMPERATE_CONIFER; // 13
                    else
                        return df::enums::biome_type::biome_type::FOREST_TAIGA; // 12
                }
                else
                    return df::enums::biome_type::biome_type::FOREST_TEMPERATE_BROADLEAF; // 14
            }
        }
    }

    // Not a lake, mountain, ocean, glacier, tundra, desert, grassland, savanna or forest
    // vegetation >= 66, drainage < 33

    else if (is_possible_tropical_area_by_latitude)
    {
        if (region.salinity < 66)
        {
            if ((parameter < 66) || is_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::SWAMP_TROPICAL_FRESHWATER; // 7
            else
                return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_FRESHWATER;// 3
        }
        else // elevation between 100 and 149, vegetation >= 66, drainage < 33, salinity >= 66
        {
            if ((parameter < 66) || is_tropical_area_by_latitude)
                if (region.drainage < 10)
                    return df::enums::biome_type::biome_type::SWAMP_MANGROVE; //9
                else // drainage >= 10
                    return df::enums::biome_type::biome_type::SWAMP_TROPICAL_SALTWATER; // 8
            else
                return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_SALTWATER; // 4
        }
    }

    else if (region.salinity >= 66)
        // elevation between 100 and 149, vegetation >= 66, drainage < 33, not tropical area
        return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_SALTWATER; // 4
    else
        return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_FRESHWATER; // 3
}

/*****************************************************************************
Module main function.
Return the biome type, given a position coordinate expressed in world_tiles
*****************************************************************************/
df::enums::biome_type::biome_type Maps::GetBiomeType(int world_coord_x, int world_coord_y)
{
    return Maps::GetBiomeTypeWithRef(world_coord_x, world_coord_y, world_coord_y);
}

