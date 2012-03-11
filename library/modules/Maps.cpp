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
#include "Error.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "DataDefs.h"
#include "df/world_data.h"
#include "df/world_underground_region.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/feature_init.h"

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
        throw DFHack::Error::ModuleNotInitialized();
    x = world->map.x_count_block;
    y = world->map.y_count_block;
    z = world->map.z_count_block;
}

// getter for map position
void Maps::getPosition (int32_t& x, int32_t& y, int32_t& z)
{
    if (!IsValid())
        throw DFHack::Error::ModuleNotInitialized();
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

df::map_block *Maps::getBlockAbs (int32_t x, int32_t y, int32_t z)
{
    if (!IsValid())
        return NULL;
    if ((x < 0) || (y < 0) || (z < 0))
        return NULL;
    if ((x >= world->map.x_count) || (y >= world->map.y_count) || (z >= world->map.z_count))
        return NULL;
    return world->map.block_index[x >> 4][y >> 4][z];
}

bool Maps::ReadBlock40d(uint32_t x, uint32_t y, uint32_t z, mapblock40d * buffer)
{
    df::map_block * block = getBlock(x,y,z);
    if (block)
    {
        buffer->position = DFCoord(x,y,z);
        memcpy(buffer->tiletypes,block->tiletype, sizeof(tiletypes40d));
        memcpy(buffer->designation,block->designation, sizeof(designations40d));
        memcpy(buffer->occupancy,block->occupancy, sizeof(occupancies40d));
        memcpy(buffer->biome_indices,block->region_offset, sizeof(block->region_offset));
        buffer->global_feature = block->global_feature;
        buffer->local_feature = block->local_feature;
        buffer->mystery = block->unk2;
        buffer->origin = block;
        buffer->blockflags.whole = block->flags.whole;
        return true;
    }
    return false;
}

/*
 * Tiletypes
 */

bool Maps::ReadTileTypes (uint32_t x, uint32_t y, uint32_t z, tiletypes40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->tiletype, sizeof(tiletypes40d));
        return true;
    }
    return false;
}

bool Maps::WriteTileTypes (uint32_t x, uint32_t y, uint32_t z, tiletypes40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->tiletype, buffer, sizeof(tiletypes40d));
        return true;
    }
    return false;
}

/*
 * Dirty bit
 */
bool Maps::ReadDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool &dirtybit)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        dirtybit = block->flags.bits.designated;
        return true;
    }
    return false;
}

bool Maps::WriteDirtyBit(uint32_t x, uint32_t y, uint32_t z, bool dirtybit)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        block->flags.bits.designated = true;
        return true;
    }
    return false;
}
/*
 * Block flags
 */
// FIXME: maybe truncates, still bullshit
bool Maps::ReadBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags &blockflags)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        blockflags.whole = block->flags.whole;
        return true;
    }
    return false;
}
//FIXME: maybe truncated, still bullshit
bool Maps::WriteBlockFlags(uint32_t x, uint32_t y, uint32_t z, t_blockflags blockflags)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        block->flags.whole = blockflags.whole;
        return true;
    }
    return false;
}

/*
 * Designations
 */
bool Maps::ReadDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->designation, sizeof(designations40d));
        return true;
    }
    return false;
}

bool Maps::WriteDesignations (uint32_t x, uint32_t y, uint32_t z, designations40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->designation, buffer, sizeof(designations40d));
        return true;
    }
    return false;
}

/*
 * Occupancies
 */
bool Maps::ReadOccupancy (uint32_t x, uint32_t y, uint32_t z, occupancies40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->occupancy, sizeof(occupancies40d));
        return true;
    }
    return false;
}

bool Maps::WriteOccupancy (uint32_t x, uint32_t y, uint32_t z, occupancies40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(block->occupancy, buffer, sizeof(occupancies40d));
        return true;
    }
    return false;
}

/*
 * Temperatures
 */
bool Maps::ReadTemperatures(uint32_t x, uint32_t y, uint32_t z, t_temperatures *temp1, t_temperatures *temp2)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        if(temp1)
            memcpy(temp1, block->temperature_1, sizeof(t_temperatures));
        if(temp2)
            memcpy(temp2, block->temperature_2, sizeof(t_temperatures));
        return true;
    }
    return false;
}
bool Maps::WriteTemperatures (uint32_t x, uint32_t y, uint32_t z, t_temperatures *temp1, t_temperatures *temp2)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        if(temp1)
            memcpy(block->temperature_1, temp1, sizeof(t_temperatures));
        if(temp2)
            memcpy(block->temperature_2, temp2, sizeof(t_temperatures));
        return true;
    }
    return false;
}

/*
 * Region Offsets - used for layer geology
 */
bool Maps::ReadRegionOffsets (uint32_t x, uint32_t y, uint32_t z, biome_indices40d *buffer)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        memcpy(buffer, block->region_offset,sizeof(biome_indices40d));
        return true;
    }
    return false;
}

bool Maps::GetGlobalFeature(t_feature &feature, int32_t index)
{
    feature.type = (df::feature_type)-1;
    if (!world->world_data)
        return false;

    if ((index < 0) || (index >= world->world_data->underground_regions.size()))
        return false;

	df::feature_init *f = world->world_data->underground_regions[index]->feature_init;

    feature.discovered = false;
    feature.origin = f;
    feature.type = f->getType();
    f->getMaterial(&feature.main_material, &feature.sub_material);
    return true;
}

bool Maps::GetLocalFeature(t_feature &feature, df::coord2d coord, int32_t index)
{
    feature.type = (df::feature_type)-1;
    if (!world->world_data)
        return false;

    // regionX and regionY are in embark squares!
    // we convert to full region tiles
    // this also works in adventure mode
    // region X coord - whole regions
    uint32_t region_x = ( (coord.x / 3) + world->map.region_x ) / 16;
    // region Y coord - whole regions
    uint32_t region_y = ( (coord.y / 3) + world->map.region_y ) / 16;

    uint32_t bigregion_x = region_x / 16;
    uint32_t bigregion_y = region_y / 16;

    uint32_t sub_x = region_x % 16;
    uint32_t sub_y = region_y % 16;
    // megaregions = 16x16 squares of regions = 256x256 squares of embark squares

    // bigregion is 16x16 regions. for each bigregion in X dimension:
    if (!world->world_data->unk_204[bigregion_x][bigregion_y].features)
        return false;

    vector <df::feature_init *> &features = world->world_data->unk_204[bigregion_x][bigregion_y].features->feature_init[sub_x][sub_y];
    if ((index < 0) || (index >= features.size()))
        return false;
    df::feature_init *f = features[index];

    feature.discovered = false;
    feature.origin = f;
    feature.type = f->getType();
    f->getMaterial(&feature.main_material, &feature.sub_material);
    return true;
}

bool Maps::ReadFeatures(uint32_t x, uint32_t y, uint32_t z, int32_t & local, int32_t & global)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        local = block->local_feature;
        global = block->global_feature;
        return true;
    }
    return false;
}

bool Maps::WriteFeatures(uint32_t x, uint32_t y, uint32_t z, const int32_t & local, const int32_t & global)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        block->local_feature = local;
        block->global_feature = global;
        return true;
    }
    return false;
}

bool Maps::ReadFeatures(uint32_t x, uint32_t y, uint32_t z, t_feature *local, t_feature *global)
{
    int32_t loc, glob;
    if (!ReadFeatures(x, y, z, loc, glob))
        return false;

    bool result = true;
    if (global)
    {
        if (glob != -1)
            result &= GetGlobalFeature(*global, glob);
        else
            global->type = (df::feature_type)-1;
    }
    if (local)
    {
        if (loc != -1)
        {
            df::coord2d coord(x,y);
            result &= GetLocalFeature(*local, coord, loc);
        }
        else
            local->type = (df::feature_type)-1;
    }
    return result;
}

bool Maps::ReadFeatures(mapblock40d * block, t_feature * local, t_feature * global)
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
            result &= GetLocalFeature(*local, block->position, block->local_feature);
        else
            local->type = (df::feature_type)-1;
    }
    return result;
}

bool Maps::SetBlockLocalFeature(uint32_t x, uint32_t y, uint32_t z, int32_t local)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        block->local_feature = local;
        return true;
    }
    return false;
}

bool Maps::SetBlockGlobalFeature(uint32_t x, uint32_t y, uint32_t z, int32_t global)
{
    df::map_block *block = getBlock(x,y,z);
    if (block)
    {
        block->global_feature = global;
        return true;
    }
    return false;
}

/*
 * Block events
 */
bool Maps::SortBlockEvents(uint32_t x, uint32_t y, uint32_t z,
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

    df::map_block * block = getBlock(x,y,z);
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
    for (size_t i = 0; i < block->block_events.size(); i++)
    {
        if (block->block_events[i] == which)
        {
            delete which;
            block->block_events.erase(block->block_events.begin() + i);
            return true;
        }
    }
    return false;
}

/*
* Layer geology
*/
bool Maps::ReadGeology (vector < vector <uint16_t> >& assign)
{
    if (!world->world_data)
        return false;

    vector<uint16_t> v_geology[eBiomeCount];
    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = world->map.region_x / 16 + ((i % 3) - 1);
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= world->world_data->world_width) bioRX = world->world_data->world_width - 1;
        int bioRY = world->map.region_y / 16 + ((i / 3) - 1);
        if (bioRY < 0) bioRY = 0;
        if (bioRY >= world->world_data->world_height) bioRY = world->world_data->world_height - 1;

        // get index into geoblock vector
        uint16_t geoindex = world->world_data->region_map[bioRX][bioRY].geo_index;

        /// geology blocks have a vector of layer descriptors
        // get the vector with pointer to layers
        df::world_geo_biome *geo_biome = df::world_geo_biome::find(geoindex);
        if (!geo_biome)
            continue;

        vector <df::world_geo_layer*> &geolayers = geo_biome->layers;

        /// layer descriptor has a field that determines the type of stone/soil
        v_geology[i].reserve(geolayers.size());

        // finally, read the layer matgloss
        for (size_t j = 0; j < geolayers.size(); j++)
            v_geology[i].push_back(geolayers[j]->mat_index);
    }

    assign.clear();
    assign.reserve(eBiomeCount);
    for (int i = 0; i < eBiomeCount; i++)
        assign.push_back(v_geology[i]);
    return true;
}

bool Maps::ReadVegetation(uint32_t x, uint32_t y, uint32_t z, std::vector<df::plant *>*& plants)
{
    df::map_block *block = getBlock(x,y,z);
    if (!block)
        return false;

    plants = &block->plants;
    return true;
}
